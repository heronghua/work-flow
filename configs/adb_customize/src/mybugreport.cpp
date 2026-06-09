#include "mybugreport.h"

#include <cstdlib>
#include <csignal>
#include <string>

#include <android-base/strings.h>
#include "adb_utils.h"
#include "commandline.h"
#include "sysdeps.h"

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

// 自定义回调：捕获 screenrecord 的 PID（符合 StandardStreamsCallbackInterface 要求）
class CollectPidCallback : public StandardStreamsCallbackInterface {
public:
    std::string output;
    void OnStdout(const char* buffer, int length) override {
        output.append(buffer, length);
    }
    void OnStderr(const char* buffer, int /*length*/) override {
        // 忽略 stderr
    }
    int Done(int /*status*/) override {
        return 0;
    }
};

int MyBugreport::DoIt(int argc, const char** argv) {
    std::string output_path = "/sdcard/screenrecord.mp4";
    if (argc == 2) {
        output_path = argv[1];
    } else if (argc > 2) {
        return syntax_error("adb mybugreport [PATH]");
    }
    return RunScreenRecord(output_path);
}

int MyBugreport::RunScreenRecord(const std::string& output_path) {
    line_printer_.Print("开始录制，按E停止录制。\n", LinePrinter::INFO);

    // 1. 启动 screenrecord 并获取 PID
    std::string start_cmd = "screenrecord " + output_path + " & echo $!";
    CollectPidCallback pid_callback;
    int ret = send_shell_command(start_cmd, false, &pid_callback);
    if (ret != 0 || pid_callback.output.empty()) {
        line_printer_.Print("ERROR: 无法启动 screenrecord\n", LinePrinter::ERROR);
        return 1;
    }

    // 提取 PID（去除换行符）
    std::string pid_str = pid_callback.output;
    pid_str.erase(pid_str.find_last_not_of("\n\r") + 1);
    int pid = std::stoi(pid_str);

    // 2. 等待用户按下 E 键
    WaitForEKey();

    // 3. 停止录制（先尝试 SIGINT，失败则 SIGTERM）
    std::string kill_cmd = "kill -2 " + std::to_string(pid);
    ret = send_shell_command(kill_cmd, false);
    if (ret != 0) {
        line_printer_.Print("WARNING: 优雅停止失败，尝试 SIGTERM\n", LinePrinter::WARNING);
        kill_cmd = "kill " + std::to_string(pid);
        send_shell_command(kill_cmd, false);
    }

    line_printer_.Print("录制结束，文件保存为 " + output_path + "\n", LinePrinter::INFO);
    return 0;
}

void MyBugreport::WaitForEKey() {
#ifdef _WIN32
    // Windows: 使用 _getch() 直接读取键盘
    while (true) {
        int ch = _getch();
        if (ch == 'E' || ch == 'e') break;
    }
#else
    // Linux / macOS: 关闭缓冲和回显，使用 adb_read 避免宏冲突
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    char ch;
    while (true) {
        if (adb_read(STDIN_FILENO, &ch, 1) == 1) {
            if (ch == 'E' || ch == 'e') break;
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
}
