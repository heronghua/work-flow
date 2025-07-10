//#define ENABLE_TRACING
#include <iostream>
#include <fstream>
#include <filesystem>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <regex>
#include <opencv2/opencv.hpp>
#include "systrace.h"  // 包含Systrace头文件

// 添加内存监控所需的头文件
#ifdef __linux__
#include <sys/resource.h>
#elif defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#include <psapi.h>
#endif

namespace fs = std::filesystem;

// 获取当前进程内存使用量 (单位: KB)
size_t get_current_memory_usage() {
    //TRACE_FUNCTION();
    size_t memory_usage = 0;
    
#if defined(__linux__) || defined(__CYGWIN__)
    std::ifstream status_file("/proc/self/status");
    std::string line;
    while (std::getline(status_file, line)) {
        if (line.find("VmRSS:") != std::string::npos) {
            std::istringstream iss(line);
            std::string key;
            iss >> key;
            iss >> memory_usage;
            break;
        }
    }
#elif defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        memory_usage = static_cast<size_t>(pmc.WorkingSetSize / 1024);
    }
#endif
    
    TRACE_MEMORY(memory_usage);
    return memory_usage;
}

// 线程安全的通用队列模板
template <typename T>
class ConcurrentQueue {
public:
    void push(T&& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        while (queue_.size() >= max_size) {
            cond_producer_.wait(lock);
        }
        queue_.push(std::move(item));
        cond_consumer_.notify_one();
    }
    
    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        while (queue_.empty() && !done_) {
            cond_consumer_.wait(lock);
        }
        if (queue_.empty() && done_) {
            return false;
        }
        item = std::move(queue_.front());
        queue_.pop();
        cond_producer_.notify_one();
        return true;
    }
    
    void setDone() {
        std::lock_guard<std::mutex> lock(mutex_);
        done_ = true;
        cond_consumer_.notify_all();
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cond_consumer_;
    std::condition_variable cond_producer_;
    std::atomic<bool> done_{false};
    static const size_t max_size = 10; // 增大队列容量
};

// 解析YUV文件名获取宽度和高度
bool parse_resolution(const std::string& filename, int& width, int& height) {
    TRACE_FUNCTION();
    // 改进的正则表达式 - 处理多种常见格式
    std::regex pattern(R"(.*_(\d+)_(\d+)\.yuv$)", std::regex_constants::icase);
    
    std::smatch matches;
    if (std::regex_search(filename, matches, pattern) && matches.size() == 3) {
        try {
            width = std::stoi(matches[1]);
            height = std::stoi(matches[2]);
            return true;
        } catch (...) {
            return false;
        }
    }
    return false;
}

// 文件信息结构
struct FileInfo {
    fs::path path;
    int width;
    int height;
    std::string output_name;
};

// YUV数据项
struct YUVData {
    std::vector<uint8_t> data;
    int width;
    int height;
    std::string output_name;
};

// 图像数据项
struct ImageData {
    cv::Mat image;
    std::string output_name;
};

int main(int argc, char* argv[]) {
    TRACE_SET_THREAD_NAME("MainThread");
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <directory_path>" << std::endl;
        return EXIT_FAILURE;
    }

    fs::path dir_path(argv[1]);
    if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
        std::cerr << "Invalid directory: " << dir_path << std::endl;
        return EXIT_FAILURE;
    }

    // 创建输出目录
    fs::path output_dir = dir_path / "pngs";
    fs::create_directories(output_dir);

    // 创建队列
    ConcurrentQueue<FileInfo> file_queue;   // 文件信息队列
    ConcurrentQueue<YUVData> yuv_queue;     // YUV数据队列
    ConcurrentQueue<ImageData> image_queue; // 图像队列

    // 原子计数器用于进度监控
    std::atomic<int> files_processed(0);
    std::atomic<int> files_total(0);
    std::atomic<bool> running(true);

    // 进度监控线程
    auto monitor_thread = std::thread([&]() {
        TRACE_SET_THREAD_NAME("MonitorThread");
        TRACE_INSTANT("MonitorThreadStart");
        while (running || file_queue.size() > 0 || yuv_queue.size() > 0 || image_queue.size() > 0) {
            {
                TRACE_SCOPE("MonitorSleep");
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            
            // 获取并记录内存使用情况
            size_t mem_usage = get_current_memory_usage();
            
            std::cout << "\rProgress: " 
                      << files_processed << "/" << files_total << " files | "
                      << "FileQ: " << file_queue.size() << " | "
                      << "YUVQ: " << yuv_queue.size() << " | "
                      << "ImgQ: " << image_queue.size() << " | "
                      << "Mem: " << mem_usage << " KB     " << std::flush;
        }
        std::cout << "\nProcessing completed." << std::endl;
        TRACE_INSTANT("MonitorThreadEnd");
    });

    // 启动文件读取线程
	auto reader_thread = std::thread([&]() {
		TRACE_SET_THREAD_NAME("ReaderThread");
		TRACE_INSTANT("ReaderThreadStart");
		
		FileInfo file_info;
		while (true) {
			// 记录等待开始
			TRACE_BEGIN("WaitForFileItem");
			
			// 实际 pop 操作
			if (!file_queue.pop(file_info)) {
				TRACE_END("WaitForFileItem");
				break;
			}
			
			// 记录等待结束
			TRACE_END("WaitForFileItem");
			
			std::string trace_args = "{\"file\":\"" + file_info.path.filename().string() + "\"}";
			TRACE_SCOPE_ARGS("ReadFile", trace_args);
			
			// 计算帧大小 (I420格式)
			const size_t frame_size = file_info.width * file_info.height * 3 / 2;
			
			{
				TRACE_SCOPE("FileOpen");
				std::ifstream file(file_info.path, std::ios::binary);
				
				if (!file) {
					std::cerr << "Error opening: " << file_info.path << std::endl;
					continue;
				}
				
				// 读取文件内容
				std::vector<uint8_t> buffer(frame_size);
				{
					TRACE_SCOPE("FileRead");
					if (!file.read(reinterpret_cast<char*>(buffer.data()), frame_size)) {
						std::cerr << "Error reading: " << file_info.path << std::endl;
						continue;
					}
				}
				
				// 记录内存使用量
				size_t mem_usage = get_current_memory_usage();
				TRACE_MEMORY(mem_usage);
				
				// 放入YUV队列
				{
					TRACE_SCOPE("PushToYUVQueue");
					yuv_queue.push({
						std::move(buffer),
						file_info.width,
						file_info.height,
						file_info.output_name
					});
					TRACE_INSTANT("YUVQueuePushed");
				}
			}
		}
		TRACE_INSTANT("ReaderThreadEnd");
		yuv_queue.setDone();
	});

    // 启动转换线程
	auto converter_thread = std::thread([&]() {
		TRACE_SET_THREAD_NAME("ConverterThread");
		TRACE_INSTANT("ConverterThreadStart");
		
		YUVData yuv_item;
		while (true) {
			// 记录等待开始
			TRACE_BEGIN("WaitForYUVItem");
			
			// 实际 pop 操作
			if (!yuv_queue.pop(yuv_item)) {
				TRACE_END("WaitForYUVItem");
				break;
			}
			
			// 记录等待结束
			TRACE_END("WaitForYUVItem");
			
			std::string trace_args = "{\"file\":\"" + yuv_item.output_name + "\"}";
			TRACE_SCOPE_ARGS("ConvertYUV", trace_args);
			
			try {
				// 创建YUV矩阵 (I420格式)
				cv::Mat yuv_mat(yuv_item.height * 3/2, yuv_item.width, CV_8UC1, yuv_item.data.data());
				cv::Mat bgr;
				
				{
					TRACE_SCOPE("ColorConversion");
					cv::cvtColor(yuv_mat, bgr, cv::COLOR_YUV2BGR_I420);
				}
				
				// 记录内存使用量
				size_t mem_usage = get_current_memory_usage();
				TRACE_MEMORY(mem_usage);
				
				{
					TRACE_SCOPE("PushToImageQueue");
					image_queue.push({std::move(bgr), yuv_item.output_name});
					TRACE_INSTANT("ImageQueuePushed");
				}
			} catch (const cv::Exception& e) {
				std::cerr << "Conversion error: " << e.what() << std::endl;
			}
		}
		TRACE_INSTANT("ConverterThreadEnd");
		image_queue.setDone();
	});
	
    // 启动写入线程
	auto writer_thread = std::thread([&]() {
		TRACE_SET_THREAD_NAME("WriterThread");
		TRACE_INSTANT("WriterThreadStart");
		
		ImageData img_item;
		while (true) {
			// 记录等待开始
			TRACE_BEGIN("WaitForImageItem");
			
			// 实际 pop 操作
			if (!image_queue.pop(img_item)) {
				TRACE_END("WaitForImageItem");
				break;
			}
			
			// 记录等待结束
			TRACE_END("WaitForImageItem");
			
			std::string trace_args = "{\"file\":\"" + img_item.output_name + "\"}";
			TRACE_SCOPE_ARGS("WritePNG", trace_args);
			
			try {
				fs::path output_path = output_dir / img_item.output_name;
				{
					TRACE_SCOPE("ImageWrite");
					if (!cv::imwrite(output_path.string(), img_item.image)) {
						std::cerr << "Error writing: " << output_path << std::endl;
					}
				}
				
				// 记录内存使用量
				size_t mem_usage = get_current_memory_usage();
				TRACE_MEMORY(mem_usage);
				
				files_processed++;
			} catch (const std::exception& e) {
				std::cerr << "Write error: " << e.what() << std::endl;
			}
		}
		TRACE_INSTANT("WriterThreadEnd");
	});

    // 主线程：仅遍历目录并收集文件信息
    TRACE_INSTANT("MainThreadStart");
    {
        TRACE_SCOPE("DirectoryScan");
        for (const auto& entry : fs::directory_iterator(dir_path)) {
            if (!entry.is_regular_file()) continue;
            
            const fs::path& file_path = entry.path();
            if (file_path.extension() != ".yuv" && file_path.extension() != ".YUV") {
                continue;
            }
            
            int width = 0, height = 0;
            if (!parse_resolution(file_path.filename().string(), width, height)) {
                std::cerr << "Skipping invalid file: " << file_path << std::endl;
                continue;
            }
            
            // 生成输出文件名
            std::string output_name = file_path.stem().string() + ".png";
            
            // 放入文件队列（仅元数据）
            {
                TRACE_SCOPE("PushToFileQueue");
                file_queue.push({file_path, width, height, output_name});
            }
            
            // 记录内存使用量
            get_current_memory_usage();
            
            files_total++;
        }
    }
    TRACE_INSTANT("MainThreadEnd");
    
    // 通知文件读取线程结束
    file_queue.setDone();
    
    // 等待所有工作线程完成
    reader_thread.join();
    converter_thread.join();
    writer_thread.join();
    
    // 停止监控线程
    running = false;
    monitor_thread.join();
    
    // 保存 trace 文件
    fs::path trace_file = dir_path / "conversion_trace.json";
    TRACE_SAVE(trace_file);
    
    std::cout << "\nConversion completed. " << files_processed << "/" << files_total
              << " files processed. PNGs saved to: " << output_dir << std::endl;    
    
    return EXIT_SUCCESS;
}
