#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <conio.h>
#include <thread>
#include <chrono>
#include <filesystem>
#include <sstream>

using namespace std;
namespace fs = filesystem;

// 配置常量
const string TASK_FILE = "tasks.txt";
const string LOG_DIR = "logs";
const int REFRESH_INTERVAL = 1000; // 毫秒

// 任务结构体
struct Task {
    int id;
    string description;
    string status;
    string log_file;
};

// 全局变量
vector<Task> tasks;
int selected_task = 0;
bool running = true;
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
CONSOLE_SCREEN_BUFFER_INFO csbi;

// 函数声明
void initialize();
void load_tasks();
void save_tasks();
void draw_ui();
void handle_input();
void update_log_display();
void create_sample_data();
void add_new_task();
void delete_selected_task();
void execute_selected_task();
void watch_log_file(const string& log_file);
void clear_rect(int left, int top, int width, int height);
void draw_border(int left, int top, int width, int height, const string& title);
void set_color(int color);
void move_cursor(int x, int y);

int main() {
    initialize();
    create_sample_data();
    load_tasks();
    
    // 初始绘制
    draw_ui();
    
    // 主循环
    while (running) {
        if (_kbhit()) {
            handle_input();
        }
        
        // 定期刷新
        static auto last_refresh = chrono::steady_clock::now();
        auto now = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - last_refresh).count();
        
        if (elapsed > REFRESH_INTERVAL) {
            load_tasks(); // 重新加载任务列表
            draw_ui();
            last_refresh = now;
        }
        
        this_thread::sleep_for(chrono::milliseconds(50));
    }
    
    return 0;
}

void initialize() {
    // 设置控制台标题
    SetConsoleTitle(L"任务队列管理器");
    
    // 创建日志目录
    if (!fs::exists(LOG_DIR)) {
        fs::create_directory(LOG_DIR);
    }
    
    // 获取控制台信息
    GetConsoleScreenBufferInfo(hConsole, &csbi);
}

void create_sample_data() {
    // 创建示例任务文件（如果不存在）
    if (!fs::exists(TASK_FILE)) {
        ofstream task_file(TASK_FILE);
        task_file << "1|处理数据文件|待执行|logs/1.log\n";
        task_file << "2|备份数据库|执行中|logs/2.log\n";
        task_file << "3|生成报告|已完成|logs/3.log\n";
        task_file << "4|清理临时文件|待执行|logs/4.log\n";
        task_file.close();
    }
    
    // 创建示例日志文件
    for (int i = 1; i <= 4; i++) {
        string log_path = LOG_DIR + "/" + to_string(i) + ".log";
        if (!fs::exists(log_path)) {
            ofstream log_file(log_path);
            log_file << "任务 #" << i << " 日志文件\n";
            log_file << "----------------------\n";
            
            if (i == 2) {
                for (int j = 0; j < 20; j++) {
                    log_file << "[" << j+1 << "] 处理中...\n";
                }
            } else if (i == 3) {
                log_file << "任务成功完成!\n";
                log_file << "完成时间: " << __DATE__ << " " << __TIME__ << "\n";
            }
            
            log_file.close();
        }
    }
}

void load_tasks() {
    ifstream task_file(TASK_FILE);
    vector<Task> new_tasks;
    string line;
    
    while (getline(task_file, line)) {
        stringstream ss(line);
        string part;
        vector<string> parts;
        
        while (getline(ss, part, '|')) {
            parts.push_back(part);
        }
        
        if (parts.size() >= 4) {
            Task task;
            task.id = stoi(parts[0]);
            task.description = parts[1];
            task.status = parts[2];
            task.log_file = parts[3];
            new_tasks.push_back(task);
        }
    }
    
    task_file.close();
    tasks = new_tasks;
    
    // 确保选中任务在有效范围内
    if (tasks.empty()) {
        selected_task = -1;
    } else if (selected_task >= tasks.size()) {
        selected_task = tasks.size() - 1;
    }
}

void save_tasks() {
    ofstream task_file(TASK_FILE);
    for (const auto& task : tasks) {
        task_file << task.id << "|" << task.description << "|" 
                 << task.status << "|" << task.log_file << "\n";
    }
    task_file.close();
}

void draw_ui() {
    system("cls");
    
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    int width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    int left_width = width * 4 / 10;
    int right_width = width - left_width - 1;
    
    // 绘制标题
    set_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    move_cursor(0, 0);
    cout << " 任务队列管理器 ";
    set_color(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    cout << " ↑/↓ 选择任务 ";
    set_color(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    cout << " A:添加 D:删除 E:执行 Q:退出 ";
    
    // 绘制左侧面板
    draw_border(0, 2, left_width, height - 4, "任务队列");
    
    // 绘制任务列表
    move_cursor(2, 4);
    set_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    cout << "ID  状态        描述";
    
    for (int i = 0; i < tasks.size(); i++) {
        move_cursor(2, 6 + i);
        
        if (i == selected_task) {
            set_color(BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED);
        }
        
        // 根据状态设置颜色
        if (tasks[i].status == "待执行") {
            set_color(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        } else if (tasks[i].status == "执行中") {
            set_color(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        } else if (tasks[i].status == "已完成") {
            set_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        }
        
        cout << tasks[i].id;
        move_cursor(7, 6 + i);
        cout << tasks[i].status;
        move_cursor(20, 6 + i);
        cout << tasks[i].description.substr(0, left_width - 22);
        
        if (i == selected_task) {
            set_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }
    }
    
    // 绘制右侧面板
    draw_border(left_width + 1, 2, right_width - 1, height - 4, "任务日志");
    update_log_display();
    
    // 绘制底部帮助
    move_cursor(0, height - 1);
    set_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    cout << " A:添加任务  D:删除任务  E:执行任务  Q:退出";
}

void update_log_display() {
    if (selected_task < 0 || selected_task >= tasks.size()) {
        return;
    }
    
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    int width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    int left_width = width * 4 / 10;
    int right_width = width - left_width - 1;
    
    // 清空日志区域
    clear_rect(left_width + 2, 4, right_width - 3, height - 7);
    
    // 读取并显示日志
    ifstream log_file(tasks[selected_task].log_file);
    if (!log_file) {
        move_cursor(left_width + 3, 5);
        set_color(FOREGROUND_RED | FOREGROUND_INTENSITY);
        cout << "日志文件不存在";
        return;
    }
    
    string line;
    int y = 4;
    while (getline(log_file, line) {
        if (y >= height - 4) break;
        
        move_cursor(left_width + 3, y++);
        set_color(FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        cout << line.substr(0, right_width - 4);
    }
    
    log_file.close();
}

void handle_input() {
    int key = _getch();
    
    // 特殊键处理
    if (key == 0 || key == 0xE0) {
        key = _getch();
        
        switch (key) {
            case 72: // 上箭头
                if (selected_task > 0) {
                    selected_task--;
                    draw_ui();
                }
                break;
                
            case 80: // 下箭头
                if (selected_task < tasks.size() - 1) {
                    selected_task++;
                    draw_ui();
                }
                break;
        }
    } else {
        switch (toupper(key)) {
            case 'Q':
                running = false;
                break;
                
            case 'A':
                add_new_task();
                draw_ui();
                break;
                
            case 'D':
                delete_selected_task();
                draw_ui();
                break;
                
            case 'E':
                execute_selected_task();
                draw_ui();
                break;
        }
    }
}

void add_new_task() {
    system("cls");
    cout << "添加新任务\n";
    cout << "==========\n\n";
    
    string description;
    cout << "输入任务描述: ";
    getline(cin, description);
    
    if (description.empty()) {
        return;
    }
    
    // 创建新任务
    Task new_task;
    new_task.id = tasks.empty() ? 1 : tasks.back().id + 1;
    new_task.description = description;
    new_task.status = "待执行";
    new_task.log_file = LOG_DIR + "/" + to_string(new_task.id) + ".log";
    
    tasks.push_back(new_task);
    save_tasks();
    
    // 创建日志文件
    ofstream log_file(new_task.log_file);
    log_file << "任务 #" << new_task.id << " 已创建: " << description << "\n";
    log_file << "状态: " << new_task.status << "\n";
    log_file.close();
}

void delete_selected_task() {
    if (selected_task < 0 || selected_task >= tasks.size()) {
        return;
    }
    
    // 删除日志文件
    if (fs::exists(tasks[selected_task].log_file)) {
        fs::remove(tasks[selected_task].log_file);
    }
    
    // 从列表中移除
    tasks.erase(tasks.begin() + selected_task);
    
    // 更新选中索引
    if (selected_task >= tasks.size()) {
        selected_task = max(0, (int)tasks.size() - 1);
    }
    
    save_tasks();
}

void execute_selected_task() {
    if (selected_task < 0 || selected_task >= tasks.size()) {
        return;
    }
    
    // 更新任务状态
    tasks[selected_task].status = "执行中";
    save_tasks();
    draw_ui();
    
    // 在新线程中模拟任务执行
    thread([&]() {
        // 打开日志文件追加模式
        ofstream log_file(tasks[selected_task].log_file, ios::app);
        log_file << "\n--- 任务开始执行 ---\n";
        log_file.close();
        
        // 模拟任务执行
        for (int i = 1; i <= 10; i++) {
            this_thread::sleep_for(chrono::milliseconds(500));
            
            // 更新日志
            ofstream log_file(tasks[selected_task].log_file, ios::app);
            log_file << "[" << i << "/10] 处理中...\n";
            log_file.close();
            
            // 刷新UI
            update_log_display();
        }
        
        // 完成任务
        tasks[selected_task].status = "已完成";
        save_tasks();
        
        ofstream log_file_final(tasks[selected_task].log_file, ios::app);
        log_file_final << "任务成功完成!\n";
        log_file_final << "完成时间: " << __DATE__ << " " << __TIME__ << "\n";
        log_file_final.close();
        
        update_log_display();
    }).detach();
}

// 辅助函数
void set_color(int color) {
    SetConsoleTextAttribute(hConsole, color);
}

void move_cursor(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(hConsole, coord);
}

void draw_border(int left, int top, int width, int height, const string& title) {
    set_color(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    
    // 上边框
    move_cursor(left, top);
    cout << "┌";
    for (int i = 0; i < width - 2; i++) {
        cout << "─";
    }
    cout << "┐";
    
    // 标题
    if (!title.empty()) {
        move_cursor(left + 2, top);
        cout << " " << title << " ";
    }
    
    // 侧边框
    for (int i = 1; i < height - 1; i++) {
        move_cursor(left, top + i);
        cout << "│";
        move_cursor(left + width - 1, top + i);
        cout << "│";
    }
    
    // 下边框
    move_cursor(left, top + height - 1);
    cout << "└";
    for (int i = 0; i < width - 2; i++) {
        cout << "─";
    }
    cout << "┘";
    
    set_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

void clear_rect(int left, int top, int width, int height) {
    string blank(width, ' ');
    for (int y = 0; y < height; y++) {
        move_cursor(left, top + y);
        cout << blank;
    }
}