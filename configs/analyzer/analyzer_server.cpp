#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "analyzer.pb.h"

using namespace std;

const int PORT = 50051;
const int BUFFER_SIZE = 1024;

// 模拟分析函数
analyzer::AnalysisResult performAnalysis1(const analyzer::AnalysisRequest& request) {
    analyzer::AnalysisResult result;
    
    // 模拟分析处理
    string processed = "Processed: " + request.data();
    for (const auto& opt : request.options()) {
        processed += " | " + opt;
    }
    
    // 设置结果
    result.set_status(analyzer::AnalysisResult::STATUS_SUCCESS);
    result.set_result_data(processed);
    result.set_processing_time(0.15);
    
    return result;
}

// 分析函数
analyzer::AnalysisResult performAnalysis(const analyzer::AnalysisRequest& request) {
    analyzer::AnalysisResult result;
    
    if (request.issue_type() == analyzer::IssueType::ISSUE_ANR) {
        std::cout << "issue type is anr" << std::endl;
        
    }
    
    return result;
}

// 处理客户端请求
void handleClient(int clientSocket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    
    // 读取消息长度前缀
    uint32_t msgLength;
    bytesRead = recv(clientSocket, &msgLength, sizeof(msgLength), 0);
    if (bytesRead != sizeof(msgLength)) {
        cerr << "Error reading message length" << endl;
        close(clientSocket);
        return;
    }
    
    // 转换网络字节序到主机字节序
    msgLength = ntohl(msgLength);
    
    // 读取 Protobuf 消息
    vector<char> protoBuffer(msgLength);
    bytesRead = recv(clientSocket, protoBuffer.data(), msgLength, 0);
    if (bytesRead != msgLength) {
        cerr << "Error reading protobuf message" << endl;
        close(clientSocket);
        return;
    }
    
    // 解析请求
    analyzer::AnalysisRequest request;
    if (!request.ParseFromArray(protoBuffer.data(), msgLength)) {
        cerr << "Failed to parse request" << endl;
        close(clientSocket);
        return;
    }
    
    cout << "Received request for data: " << request.data() 
         << " with priority: " << request.priority() << endl;
    
    // 执行分析
    analyzer::AnalysisResult result = performAnalysis(request);
    
    // 序列化结果
    string serializedResult;
    if (!result.SerializeToString(&serializedResult)) {
        cerr << "Failed to serialize result" << endl;
        close(clientSocket);
        return;
    }
    
    // 发送结果长度前缀
    uint32_t resultLength = htonl(serializedResult.size());
    send(clientSocket, &resultLength, sizeof(resultLength), 0);
    
    // 发送结果
    send(clientSocket, serializedResult.c_str(), serializedResult.size(), 0);
    
    close(clientSocket);
}

int main() {
    // 创建 socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        cerr << "Error creating socket" << endl;
        return 1;
    }
    
    // 设置 socket 选项
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        cerr << "Setsockopt failed" << endl;
        close(serverSocket);
        return 1;
    }
    
    // 绑定地址和端口
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT);
    
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        cerr << "Bind failed" << endl;
        close(serverSocket);
        return 1;
    }
    
    // 开始监听
    if (listen(serverSocket, 10) < 0) {
        cerr << "Listen failed" << endl;
        close(serverSocket);
        return 1;
    }
    
    cout << "Analyzer server running on port " << PORT << endl;
    
    // 主循环
    while (true) {
        sockaddr_in clientAddress{};
        socklen_t clientAddrLen = sizeof(clientAddress);
        
        // 接受客户端连接
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddrLen);
        if (clientSocket < 0) {
            cerr << "Accept failed" << endl;
            continue;
        }
        
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIP, INET_ADDRSTRLEN);
        cout << "Client connected from " << clientIP << ":" << ntohs(clientAddress.sin_port) << endl;
        
        // 处理客户端请求
        handleClient(clientSocket);
    }
    
    close(serverSocket);
    return 0;
}
