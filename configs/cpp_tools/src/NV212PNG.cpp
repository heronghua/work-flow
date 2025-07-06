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

namespace fs = std::filesystem;

// 解析YUV文件名获取宽度和高度
bool parse_resolution(const std::string& filename, int& width, int& height) {
    std::regex pattern(R"(.*\.(\d+)_(\d+)\.yuv$)", std::regex_constants::icase);
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

// 线程安全的通用队列模板
template <typename T>
class ConcurrentQueue {
public:
    void push(const T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        while (queue_.size() >= max_size && !done_) {
            cond_producer_.wait(lock);
        }
        if (!done_) {
            queue_.push(item);
            cond_consumer_.notify_one();
        }
    }

    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        while (queue_.empty() && !done_) {
            cond_consumer_.wait(lock);
        }
        if (queue_.empty() && done_) {
            return false;
        }
        item = queue_.front();
        queue_.pop();
        cond_producer_.notify_one();
        return true;
    }

    void setDone() {
        std::lock_guard<std::mutex> lock(mutex_);
        done_ = true;
        cond_consumer_.notify_all();
        cond_producer_.notify_all();
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_consumer_;
    std::condition_variable cond_producer_;
    std::atomic<bool> done_{false};
    static const size_t max_size = 10; // 队列最大容量
};

// YUV数据项
struct YUVData {
    std::vector<uint8_t> data;
    int width;
    int height;
    std::string filename;
};

// 图像数据项
struct ImageData {
    cv::Mat image;
    std::string filename;
};

int main(int argc, char* argv[]) {
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
    ConcurrentQueue<YUVData> yuv_queue;
    ConcurrentQueue<ImageData> image_queue;

    // 启动转换线程
    std::thread converter_thread([&]() {
        YUVData yuv_item;
        while (yuv_queue.pop(yuv_item)) {
            // 创建YUV矩阵 (I420格式)
            cv::Mat yuv_mat(yuv_item.height * 3/2, yuv_item.width, CV_8UC1, yuv_item.data.data());
            cv::Mat bgr;
            
            // 转换YUV(I420)到BGR
            cv::cvtColor(yuv_mat, bgr, cv::COLOR_YUV2BGR_I420);
            
            // 放入图像队列
            image_queue.push({bgr, yuv_item.filename});
        }
        // 通知写入线程结束
        image_queue.setDone();
    });

    // 启动写入线程
    std::thread writer_thread([&]() {
        ImageData img_item;
        while (image_queue.pop(img_item)) {
            fs::path output_path = output_dir / img_item.filename;
            if (!cv::imwrite(output_path.string(), img_item.image)) {
                std::cerr << "Error writing: " << output_path << std::endl;
            }
        }
    });

    // 主线程：遍历目录并读取YUV文件
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

        // 计算帧大小 (I420格式)
        const size_t frame_size = width * height * 3 / 2;
        std::ifstream file(file_path, std::ios::binary);
        if (!file) {
            std::cerr << "Error opening: " << file_path << std::endl;
            continue;
        }

        // 读取整个文件作为一帧
        std::vector<uint8_t> buffer(frame_size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), frame_size)) {
            std::cerr << "Error reading: " << file_path << std::endl;
            continue;
        }

        // 生成输出文件名
        std::string output_name = file_path.stem().string() + ".png";
        
        // 放入YUV队列
        yuv_queue.push({std::move(buffer), width, height, output_name});
    }

    // 通知转换线程结束
    yuv_queue.setDone();
    
    // 等待所有线程完成
    converter_thread.join();
    writer_thread.join();
    
    std::cout << "Conversion completed. PNGs saved to: " << output_dir << std::endl;
    return EXIT_SUCCESS;
}