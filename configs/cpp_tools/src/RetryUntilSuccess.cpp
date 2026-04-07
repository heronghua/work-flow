#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <sys/wait.h>
#include <getopt.h>
#include <cstdlib>
#include <algorithm>

// ---------- 补全相关函数 ----------
// 检查是否处于 Bash 补全环境
bool is_completion_context() {
    const char* comp_words = getenv("COMP_WORDS");
    return comp_words != nullptr && std::string(comp_words).length() > 0;
}

// 输出补全候选，每行一个
void output_completion_suggestions(const std::vector<std::string>& suggestions) {
    for (const auto& s : suggestions) {
        std::cout << s << "\n";
    }
    std::cout.flush();
    exit(0);
}

// 根据当前输入单词返回补全候选
std::vector<std::string> get_completions(const std::string& cur_word, const std::string& prev_word) {
    std::vector<std::string> suggestions;
    // 长选项列表
    static const std::vector<std::string> long_options = {"--duration", "--count", "--help"};
    static const std::vector<std::string> short_options = {"-d", "-c", "-h"};

    // 如果前一个单词是 --duration 或 -d，补全时间值
    if (prev_word == "--duration" || prev_word == "-d") {
        static const std::vector<std::string> time_suggestions = {"5s", "10s", "30s", "1m", "5m", "500ms"};
        for (const auto& t : time_suggestions) {
            if (t.find(cur_word) == 0) suggestions.push_back(t);
        }
    }
    // 如果前一个单词是 --count 或 -c，补全数字
    else if (prev_word == "--count" || prev_word == "-c") {
        static const std::vector<std::string> count_suggestions = {"1", "2", "3", "5", "10", "0"};
        for (const auto& c : count_suggestions) {
            if (c.find(cur_word) == 0) suggestions.push_back(c);
        }
    }
    // 否则补全选项和子命令（这里主要是选项）
    else {
        for (const auto& opt : long_options) {
            if (opt.find(cur_word) == 0) suggestions.push_back(opt);
        }
        for (const auto& opt : short_options) {
            if (opt.find(cur_word) == 0) suggestions.push_back(opt);
        }
        // 还可以补全文件名/命令（由 shell 默认处理），此处不覆盖
    }
    return suggestions;
}

// 补全入口：解析 shell 传递的环境变量并输出建议
void handle_completion() {
    const char* comp_line = getenv("COMP_LINE");
    const char* comp_point = getenv("COMP_POINT");
    if (!comp_line) return;

    std::string line(comp_line);
    size_t point = comp_point ? std::stoul(comp_point) : line.size();
    
    // 模拟 shell 的单词分割（简化版）
    std::vector<std::string> words;
    std::string cur_word;
    bool in_word = false;
    for (size_t i = 0; i < point; ++i) {
        char c = line[i];
        if (c == ' ' || c == '\t') {
            if (in_word) {
                words.push_back(cur_word);
                cur_word.clear();
                in_word = false;
            }
        } else {
            if (!in_word) in_word = true;
            cur_word += c;
        }
    }
    std::string current = cur_word;
    std::string previous = words.empty() ? "" : words.back();
    
    auto suggestions = get_completions(current, previous);
    output_completion_suggestions(suggestions);
}

// ---------- 原有的重试逻辑 ----------
double parse_duration(const std::string& dur_str) {
    if (dur_str.empty()) return 5.0;
    double value = 0.0;
    char unit = '\0';
    if (sscanf(dur_str.c_str(), "%lf%c", &value, &unit) >= 1) {
        switch (unit) {
            case 'm':
                if (dur_str.find("ms") != std::string::npos) return value / 1000.0;
                return value * 60.0;
            case 'h': return value * 3600.0;
            case 's': return value;
            default: return value;
        }
    }
    return std::stod(dur_str);
}

int execute_command(const std::vector<std::string>& cmd) {
    if (cmd.empty()) return -1;
    std::vector<char*> args;
    for (const auto& arg : cmd) args.push_back(const_cast<char*>(arg.c_str()));
    args.push_back(nullptr);
    pid_t pid = fork();
    if (pid == -1) { perror("fork"); return -1; }
    if (pid == 0) {
        execvp(args[0], args.data());
        perror("execvp");
        exit(127);
    }
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
}

void print_usage(const char* prog_name) {
    std::cerr << "Usage: " << prog_name << " [options] <command> [args...]\n"
              << "Options:\n"
              << "  -d, --duration <time>   Retry interval (e.g., 5s, 500ms, 2m, 1h). Default: 5s\n"
              << "  -c, --count <N>         Max attempts (0 = infinite). Default: 0\n"
              << "  -h, --help              Show this help\n"
              << "Completion:\n"
              << "  To enable dynamic completion in Bash, add this to your .bashrc:\n"
              << "  complete -F _retry_until_success_completion " << prog_name << "\n"
              << "  And define the function:\n"
              << "  _retry_until_success_completion() { COMPREPLY=($( " << prog_name << " --complete \"${COMP_WORDS[*]}\" \"$COMP_CWORD\" 2>/dev/null)); }\n";
}

// 外部调用：--complete 参数用于显式补全（可选）
void explicit_completion(const std::vector<std::string>& words, int cword) {
    std::string current = (cword >= 0 && cword < (int)words.size()) ? words[cword] : "";
    std::string previous = (cword > 0) ? words[cword - 1] : "";
    auto suggestions = get_completions(current, previous);
    for (const auto& s : suggestions) std::cout << s << "\n";
    exit(0);
}

int main(int argc, char* argv[]) {
    // 优先检测补全环境（自动模式）
    if (is_completion_context()) {
        handle_completion();
    }

    // 显式补全模式：--complete 后跟 COMP_WORDS 和 COMP_CWORD
    if (argc >= 2 && std::string(argv[1]) == "--complete") {
        if (argc >= 4) {
            std::vector<std::string> words;
            std::string all_words = argv[2];
            size_t start = 0, end;
            while ((end = all_words.find(' ', start)) != std::string::npos) {
                if (end > start) words.push_back(all_words.substr(start, end - start));
                start = end + 1;
            }
            if (start < all_words.size()) words.push_back(all_words.substr(start));
            int cword = std::stoi(argv[3]);
            explicit_completion(words, cword);
        }
        return 0;
    }

    // 正常参数解析
    double duration_sec = 5.0;
    int max_attempts = 0;
    std::vector<std::string> command;
    static struct option long_options[] = {
        {"duration", required_argument, 0, 'd'},
        {"count",    required_argument, 0, 'c'},
        {"help",     no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    int opt;
    while ((opt = getopt_long(argc, argv, "d:c:h", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'd': duration_sec = parse_duration(optarg); break;
            case 'c': max_attempts = std::stoi(optarg); break;
            case 'h': print_usage(argv[0]); return 0;
            default: print_usage(argv[0]); return 1;
        }
    }
    for (int i = optind; i < argc; ++i) command.emplace_back(argv[i]);
    if (command.empty()) { print_usage(argv[0]); return 1; }

    int attempt = 0, exit_code = 0;
    while (true) {
        ++attempt;
        std::cout << "Attempt " << attempt;
        if (max_attempts > 0) std::cout << "/" << max_attempts;
        std::cout << ": ";
        for (size_t i = 0; i < command.size(); ++i) {
            if (i > 0) std::cout << ' ';
            std::cout << command[i];
        }
        std::cout << std::endl;
        exit_code = execute_command(command);
        if (exit_code == 0) {
            std::cout << "Command succeeded on attempt " << attempt << std::endl;
            return 0;
        }
        if (max_attempts > 0 && attempt >= max_attempts) {
            std::cerr << "Command failed after " << attempt << " attempts. Last exit code: " << exit_code << std::endl;
            return exit_code;
        }
        std::cout << "Command failed (exit " << exit_code << "), retrying in " << duration_sec << " seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::duration<double>(duration_sec));
    }
}