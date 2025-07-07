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

namespace fs = std::filesystem;

/**
 * @brief 轻量级性能分析工具，生成 Chrome Tracing 格式的 JSON 文件
 * 
 * 使用示例：
 *   Systrace::get().beginEvent("MyEvent");
 *   // ... 代码 ...
 *   Systrace::get().endEvent("MyEvent");
 *   Systrace::get().saveToFile("trace.json");
 * 
 * 或使用 RAII 包装器：
 *   {
 *     AutoTrace trace("ScopeName");
 *     // ... 代码 ...
 *   } // 自动结束事件
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

        file << "[\n";
        bool first = true;
        for (const auto& event : events_) {
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
        
        std::cout << "Systrace: Saved " << events_.size() 
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
     * @brief 获取当前线程的跟踪ID
     */
    uint32_t getCurrentThreadId() const {
        return getThreadId();
    }

private:
    Systrace() = default;
    ~Systrace() = default;
    
    // 禁止拷贝和移动
    Systrace(const Systrace&) = delete;
    Systrace& operator=(const Systrace&) = delete;

    void addEvent(const std::string& name, char type, const std::string& args = "") {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.push_back({
            name, 
            type,
            getThreadId(),
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
    
    uint32_t getThreadId() const {
        static std::mutex id_mutex;
        static std::map<std::thread::id, uint32_t> thread_ids;
        static uint32_t next_id = 1;
        
        auto this_id = std::this_thread::get_id();
        std::lock_guard<std::mutex> lock(id_mutex);
        
        auto it = thread_ids.find(this_id);
        if (it != thread_ids.end()) {
            return it->second;
        }
        
        uint32_t id = next_id++;
        thread_ids[this_id] = id;
        return id;
    }

    mutable std::mutex mutex_;
    std::vector<Event> events_;
};

// ===================== 简化使用的宏 =====================
#ifdef ENABLE_TRACING
#define TRACE_SCOPE(name) Systrace::AutoTrace __trace_scope__(name)
#define TRACE_SCOPE_ARGS(name, args) Systrace::AutoTrace __trace_scope__(name, args)
#define TRACE_FUNCTION() Systrace::AutoTrace __trace_scope__(__FUNCTION__)
#define TRACE_FUNCTION_ARGS(args) Systrace::AutoTrace __trace_scope__(__FUNCTION__, args)
#define TRACE_COUNTER(name, value) Systrace::get().counterEvent(name, value)
#define TRACE_INSTANT(name) Systrace::get().instantEvent(name)
#define TRACE_INSTANT_ARGS(name, args) Systrace::get().instantEvent(name, args)
#define TRACE_BEGIN(name) Systrace::get().beginEvent(name)
#define TRACE_BEGIN_ARGS(name, args) Systrace::get().beginEvent(name, args)
#define TRACE_END(name) Systrace::get().endEvent(name)
#define TRACE_SAVE(filepath) Systrace::get().saveToFile(filepath)
#define TRACE_THREAD_ID() Systrace::get().getCurrentThreadId()
#else
#define TRACE_SCOPE(name) 
#define TRACE_SCOPE_ARGS(name, args) 
#define TRACE_FUNCTION() 
#define TRACE_FUNCTION_ARGS(args) 
#define TRACE_COUNTER(name, value) 
#define TRACE_INSTANT(name)
#define TRACE_INSTANT_ARGS(name, args)
#define TRACE_BEGIN(name)
#define TRACE_BEGIN_ARGS(name, args)
#define TRACE_END(name)
#define TRACE_SAVE(filepath)
#define TRACE_THREAD_ID()
#endif

#endif // SYSTRACE_H
