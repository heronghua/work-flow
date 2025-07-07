#ifndef SYSTRACE_H
#define SYSTRACE_H

#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <map>
#include <atomic>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <cassert>
#include <iostream>
#include <memory>
#include <cstring>
#include <unordered_map>

namespace fs = std::filesystem;

/**
 * @brief 跨平台性能分析工具，支持线程名称和内存追踪（兼容Cygwin）
 */
class Systrace {
public:
    /// 跟踪事件类型
    struct Event {
        std::string name;       ///< 事件名称
        char type;              ///< 事件类型 (B, E, I, C)
        uint32_t tid;           ///< 线程ID
        std::chrono::high_resolution_clock::time_point ts; ///< 时间戳
        std::string args;       ///< 额外参数 (JSON 格式)
    };

    /**
     * @brief 获取单例实例
     */
    static Systrace& get() {
        static Systrace instance;
        return instance;
    }

    /**
     * @brief 开始一个跟踪事件
     * @param name 事件名称
     * @param args 额外参数 (JSON 格式字符串)
     */
    void beginEvent(const std::string& name, const std::string& args = "") {
        addEvent(name, 'B', args);
    }

    /**
     * @brief 结束一个跟踪事件
     * @param name 事件名称
     */
    void endEvent(const std::string& name) {
        addEvent(name, 'E');
    }

    /**
     * @brief 添加即时事件
     * @param name 事件名称
     * @param args 额外参数 (JSON 格式字符串)
     */
    void instantEvent(const std::string& name, const std::string& args = "") {
        addEvent(name, 'I', args);
    }

    /**
     * @brief 添加计数器事件
     * @param name 计数器名称
     * @param value 计数器值
     */
    void counterEvent(const std::string& name, int value) {
        addEvent(name, 'C', "{\"value\":" + std::to_string(value) + "}");
    }

    /**
     * @brief 添加内存使用事件
     * @param usage 内存使用量(KB)
     */
    void memoryEvent(size_t usage) {
        counterEvent("MemoryUsage", static_cast<int>(usage));
    }

    /**
     * @brief 设置当前线程名称
     * @param name 线程名称
     */
    void setThreadName(const std::string& name) {
        std::lock_guard<std::mutex> lock(thread_name_mutex_);
        thread_names_[getCurrentThreadId()] = name;
    }

    /**
     * @brief 获取当前线程名称
     */
    std::string getThreadName(uint32_t tid) const {
        std::lock_guard<std::mutex> lock(thread_name_mutex_);
        auto it = thread_names_.find(tid);
        if (it != thread_names_.end()) {
            return it->second;
        }
        
        // 如果未设置名称，返回默认名称
        return "Thread-" + std::to_string(tid);
    }

    /**
     * @brief 保存跟踪数据到文件
     * @param filepath 输出文件路径
     */
    void saveToFile(const fs::path& filepath) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Systrace: Failed to open trace file: " << filepath << std::endl;
            return;
        }

        // 添加元数据事件：线程名称
        std::vector<Event> metadata_events;
        {
            std::lock_guard<std::mutex> name_lock(thread_name_mutex_);
            auto start_time = std::chrono::high_resolution_clock::now();
            
            for (const auto& [tid, name] : thread_names_) {
                Event md_event;
                md_event.name = "thread_name";
                md_event.type = 'M';
                md_event.tid = tid;
                md_event.ts = start_time;
                md_event.args = "{\"name\":\"" + escapeJson(name) + "\"}";
                metadata_events.push_back(md_event);
            }
        }

        // 合并元数据事件和普通事件
        std::vector<Event> all_events;
        all_events.reserve(metadata_events.size() + events_.size());
        all_events.insert(all_events.end(), metadata_events.begin(), metadata_events.end());
        all_events.insert(all_events.end(), events_.begin(), events_.end());

        // 按时间戳排序
        std::sort(all_events.begin(), all_events.end(), [](const Event& a, const Event& b) {
            return a.ts < b.ts;
        });

        // 写入文件
        file << "[\n";
        bool first = true;
        for (const auto& event : all_events) {
            auto us = std::chrono::time_point_cast<std::chrono::microseconds>(event.ts);
            auto epoch = us.time_since_epoch();
            auto value = std::chrono::duration_cast<std::chrono::microseconds>(epoch).count();

            if (!first) file << ",\n";
            first = false;

            file << "{";
            file << "\"name\":\"" << escapeJson(event.name) << "\",";
            file << "\"ph\":\"" << event.type << "\",";
            file << "\"ts\":" << value << ",";
            file << "\"pid\":0,";
            file << "\"tid\":" << event.tid;
            
            if (!event.args.empty()) {
                file << ",\"args\":" << event.args;
            }
            
            file << "}";
        }
        file << "\n]";
        
        std::cout << "Systrace: Saved " << all_events.size() 
                  << " events to " << filepath << std::endl;
        std::cout << "Open chrome://tracing in Chrome browser and load this file for visualization." << std::endl;
    }

    /**
     * @brief 自动跟踪作用域的 RAII 包装器
     */
    class AutoTrace {
    public:
        AutoTrace(const std::string& name, const std::string& args = "") 
            : name_(name) {
            Systrace::get().beginEvent(name, args);
        }
        
        ~AutoTrace() {
            Systrace::get().endEvent(name_);
        }
        
        // 禁止拷贝和移动
        AutoTrace(const AutoTrace&) = delete;
        AutoTrace& operator=(const AutoTrace&) = delete;
        
    private:
        std::string name_;
    };

    /**
     * @brief 获取当前线程的跟踪ID（兼容Cygwin）
     */
    uint32_t getCurrentThreadId() const {
        static std::mutex id_mutex;
        static std::unordered_map<std::thread::id, uint32_t> thread_id_map;
        static uint32_t next_id = 1;
        
        auto this_id = std::this_thread::get_id();
        std::lock_guard<std::mutex> lock(id_mutex);
        
        auto it = thread_id_map.find(this_id);
        if (it != thread_id_map.end()) {
            return it->second;
        }
        
        uint32_t id = next_id++;
        thread_id_map[this_id] = id;
        return id;
    }

private:
    Systrace() = default;
    ~Systrace() = default;
    
    // 禁止拷贝和移动
    Systrace(const Systrace&) = delete;
    Systrace& operator=(const Systrace&) = delete;

    void addEvent(const std::string& name, char type, const std::string& args = "") {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t tid = getCurrentThreadId();
        events_.push_back({
            name, 
            type,
            tid,
            std::chrono::high_resolution_clock::now(),
            args
        });
    }
    
    std::string escapeJson(const std::string& input) {
        std::ostringstream ss;
        for (char c : input) {
            switch (c) {
                case '"': ss << "\\\""; break;
                case '\\': ss << "\\\\"; break;
                case '\b': ss << "\\b"; break;
                case '\f': ss << "\\f"; break;
                case '\n': ss << "\\n"; break;
                case '\r': ss << "\\r"; break;
                case '\t': ss << "\\t"; break;
                default: ss << c;
            }
        }
        return ss.str();
    }

    mutable std::mutex mutex_;
    mutable std::mutex thread_name_mutex_;
    std::vector<Event> events_;
    std::map<uint32_t, std::string> thread_names_;
};

// ===================== 简化使用的宏 =====================
#ifdef ENABLE_TRACING
#define TRACE_SCOPE(name) Systrace::AutoTrace __trace_scope__(name)
#define TRACE_SCOPE_ARGS(name, args) Systrace::AutoTrace __trace_scope__(name, args)
#define TRACE_FUNCTION() Systrace::AutoTrace __trace_scope__(__FUNCTION__)
#define TRACE_FUNCTION_ARGS(args) Systrace::AutoTrace __trace_scope__(__FUNCTION__, args)
#define TRACE_COUNTER(name, value) Systrace::get().counterEvent(name, value)
#define TRACE_MEMORY(usage) Systrace::get().memoryEvent(usage)
#define TRACE_INSTANT(name) Systrace::get().instantEvent(name)
#define TRACE_INSTANT_ARGS(name, args) Systrace::get().instantEvent(name, args)
#define TRACE_BEGIN(name) Systrace::get().beginEvent(name)
#define TRACE_BEGIN_ARGS(name, args) Systrace::get().beginEvent(name, args)
#define TRACE_END(name) Systrace::get().endEvent(name)
#define TRACE_SAVE(filepath) Systrace::get().saveToFile(filepath)
#define TRACE_THREAD_ID() Systrace::get().getCurrentThreadId()
#define TRACE_SET_THREAD_NAME(name) Systrace::get().setThreadName(name)
#else
#define TRACE_SCOPE(name) 
#define TRACE_SCOPE_ARGS(name, args) 
#define TRACE_FUNCTION() 
#define TRACE_FUNCTION_ARGS(args) 
#define TRACE_COUNTER(name, value) 
#define TRACE_MEMORY(usage)
#define TRACE_INSTANT(name)
#define TRACE_INSTANT_ARGS(name, args)
#define TRACE_BEGIN(name)
#define TRACE_BEGIN_ARGS(name, args)
#define TRACE_END(name)
#define TRACE_SAVE(filepath)
#define TRACE_THREAD_ID()
#define TRACE_SET_THREAD_NAME(name)
#endif

#endif // SYSTRACE_H