// LearnEnhance.cpp
// 编译命令: g++ LearnEnhance.cpp -o LearnEnhance.exe -luser32
// 运行方式:
//   LearnEnhance.exe           直接答题模式
//   LearnEnhance.exe --reminder 提醒模式（后台循环）

#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <ctime>
#include <cstdlib>
#include <cctype>
#include <chrono>
#include <thread>

// 记忆曲线间隔（天）
const std::vector<int> INTERVALS = {1, 2, 4, 7, 15};
const size_t MAX_INTERVAL_INDEX = INTERVALS.size() - 1;

// 数据文件路径（存储在 $HOME 目录下）
std::string getDataFilePath() {
    const char* home = getenv("HOME");
    if (!home) home = ".";
    return std::string(home) + "/.learnenhance.dat";
}

// 问答对结构
struct QA {
    std::string question;
    std::string answer;
};

// 读取 knowledge 文件，返回所有问答对
std::vector<QA> loadKnowledge() {
    const char* home = getenv("HOME");
    if (!home) {
        std::cerr << "错误: 无法获取 HOME 环境变量" << std::endl;
        return {};
    }
    std::string filename = std::string(home) + "/knowledge";
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "错误: 无法打开文件 " << filename << std::endl;
        return {};
    }

    std::vector<QA> qas;
    std::string line;
    QA current;
    int lineNum = 0;
    while (std::getline(file, line)) {
        // 跳过空行
        if (line.empty()) continue;
        if (lineNum % 2 == 0) {
            current.question = line;
        } else {
            current.answer = line;
            qas.push_back(current);
        }
        ++lineNum;
    }
    // 如果最后一行是问题但没有答案，忽略
    if (lineNum % 2 == 1 && !current.question.empty()) {
        std::cerr << "警告: 文件末尾的问题缺少答案，已忽略" << std::endl;
    }

    if (qas.empty()) {
        std::cerr << "错误: knowledge 文件中没有有效的问答对" << std::endl;
    }
    return qas;
}

// 加载记忆曲线数据 (nextReminderTime, intervalIndex)
// 返回 true 表示数据存在且有效，false 表示首次运行或数据损坏
bool loadReminderData(time_t& nextReminderTime, int& intervalIndex) {
    std::string path = getDataFilePath();
    std::ifstream file(path);
    if (!file.is_open()) return false;
    file >> nextReminderTime >> intervalIndex;
    return file.good();
}

// 保存记忆曲线数据
void saveReminderData(time_t nextReminderTime, int intervalIndex) {
    std::string path = getDataFilePath();
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "警告: 无法保存记忆曲线数据到 " << path << std::endl;
        return;
    }
    file << nextReminderTime << "\n" << intervalIndex << std::endl;
}

// 根据当前时间戳和间隔索引计算下次提醒时间
time_t computeNextReminder(time_t lastAnswerTime, int intervalIndex) {
    if (intervalIndex < 0) intervalIndex = 0;
    if (intervalIndex > (int)MAX_INTERVAL_INDEX) intervalIndex = MAX_INTERVAL_INDEX;
    return lastAnswerTime + INTERVALS[intervalIndex] * 24 * 3600;
}

// 更新记忆曲线（答题结束后调用）
void updateReminderAfterQuiz() {
    time_t now = std::time(nullptr);
    int intervalIndex = 0;
    time_t dummyNext = 0;
    bool hasData = loadReminderData(dummyNext, intervalIndex);
    if (!hasData) {
        intervalIndex = 0;  // 第一次答题，从第一个间隔开始
    } else {
        // 间隔索引递增，但不超过最大值
        if (intervalIndex < (int)MAX_INTERVAL_INDEX) {
            ++intervalIndex;
        }
    }
    time_t nextReminder = computeNextReminder(now, intervalIndex);
    saveReminderData(nextReminder, intervalIndex);
}

// 字符串修剪（去除首尾空格）
std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

// 转换为小写
std::string toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

// 比较用户答案与正确答案（忽略大小写和首尾空格）
bool isAnswerCorrect(const std::string& userAns, const std::string& correctAns) {
    return toLower(trim(userAns)) == toLower(trim(correctAns));
}

// 执行一次答题（随机抽10道，若无10道则全部使用）
void doQuiz(const std::vector<QA>& allQAs) {
    if (allQAs.empty()) {
        std::cout << "没有可用的题目，请检查 knowledge 文件。" << std::endl;
        return;
    }

    size_t total = allQAs.size();
    size_t quizCount = std::min<size_t>(10, total);
    std::vector<size_t> indices(total);
    for (size_t i = 0; i < total; ++i) indices[i] = i;
    // 随机打乱索引
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(indices.begin(), indices.end(), g);
    indices.resize(quizCount);

    int correctCount = 0;
    std::cout << "\n========== 开始答题 ==========\n" << std::endl;
    for (size_t i = 0; i < quizCount; ++i) {
        const QA& qa = allQAs[indices[i]];
        std::cout << "第 " << (i+1) << " 题: " << qa.question << std::endl;
        std::cout << "你的答案: ";
        std::string userAnswer;
        std::getline(std::cin, userAnswer);
        if (isAnswerCorrect(userAnswer, qa.answer)) {
            std::cout << "✓ 正确！" << std::endl;
            ++correctCount;
        } else {
            std::cout << "✗ 错误。正确答案是: " << qa.answer << std::endl;
            std::cout << ":) 下次加油！" << std::endl;
        }
        std::cout << std::endl;
    }

    std::cout << "========== 得分统计 ==========" << std::endl;
    std::cout << "答对: " << correctCount << " / " << quizCount << std::endl;
    if (correctCount == quizCount) {
        std::cout << "太棒了！满分！" << std::endl;
    } else if (correctCount >= quizCount / 2) {
        std::cout << "还不错，继续努力！" << std::endl;
    } else {
        std::cout << "再多复习一下，加油！" << std::endl;
    }
    std::cout << std::endl;

    // 更新记忆曲线数据
    updateReminderAfterQuiz();
}

// 提醒模式：后台循环检查是否到达提醒时间
void reminderLoop(const std::vector<QA>& allQAs) {
    std::cout << "提醒模式已启动，程序将在后台检查复习时间。" << std::endl;
    std::cout << "按 Ctrl+C 可退出程序。" << std::endl;

    time_t nextReminder = 0;
    int intervalIndex = 0;
    bool hasData = loadReminderData(nextReminder, intervalIndex);
    if (!hasData) {
        // 首次运行，立即提醒
        nextReminder = 0;
        std::cout << "未找到记忆曲线数据，将立即进行首次提醒。" << std::endl;
    }

    while (true) {
        time_t now = std::time(nullptr);
        if (now >= nextReminder) {
            // 弹窗提醒
            int result = MessageBoxA(nullptr,
                "该复习知识点了！点击“确定”开始答题。",
                "学习提醒",
                MB_OK | MB_ICONINFORMATION);
            if (result == IDOK) {
                // 用户确认后开始答题
                doQuiz(allQAs);
                // 答题结束后重新加载下一次提醒时间（已在 doQuiz 中更新）
                loadReminderData(nextReminder, intervalIndex);
            } else {
                // 理论上不会走到这里（只有一个确定按钮）
                std::this_thread::sleep_for(std::chrono::seconds(60));
            }
        } else {
            // 未到提醒时间，计算剩余秒数并休眠
            time_t diff = nextReminder - now;
            int sleepSeconds = (diff > 60) ? 60 : (diff > 0 ? diff : 60);
            std::this_thread::sleep_for(std::chrono::seconds(sleepSeconds));
        }
    }
}

// 显示帮助信息
void showHelp(const char* progName) {
    std::cout << "用法: " << progName << " [选项]" << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  --reminder    启动提醒模式（后台循环，按时弹窗提醒）" << std::endl;
    std::cout << "  --help        显示此帮助信息" << std::endl;
    std::cout << "  无参数        直接答题模式（立即开始一次答题）" << std::endl;
}

int main(int argc, char* argv[]) {
    // 处理命令行参数
    bool reminderMode = false;
    if (argc >= 2) {
        std::string arg = argv[1];
        if (arg == "--reminder") {
            reminderMode = true;
        } else if (arg == "--help") {
            showHelp(argv[0]);
            return 0;
        } else {
            std::cerr << "未知参数: " << arg << std::endl;
            showHelp(argv[0]);
            return 1;
        }
    }

    // 加载所有题目
    std::vector<QA> allQAs = loadKnowledge();
    if (allQAs.empty()) {
        std::cerr << "无法继续，请检查 $HOME/knowledge 文件。" << std::endl;
        return 1;
    }

    if (reminderMode) {
        reminderLoop(allQAs);
    } else {
        doQuiz(allQAs);
    }

    return 0;
}