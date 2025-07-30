#!/usr/bin/env python3

import os
import sys
import socket
import struct

# 获取当前脚本所在目录
script_dir = os.path.dirname(os.path.abspath(__file__))
build_dir = os.path.dirname(script_dir)  # 假设在 build 目录中

# 添加可能的导入路径
sys.path.insert(0, script_dir)  # 当前目录
sys.path.insert(0, build_dir)   # 父目录（项目根目录）
sys.path.insert(0, os.path.join(build_dir, "build", "generated"))  # CMake 生成目录

try:
    import analyzer_pb2
    print(f"Successfully imported analyzer_pb2 from: {analyzer_pb2.__file__}")
except ImportError as e:
    print(f"Failed to import analyzer_pb2: {str(e)}")
    print("Tried paths:")
    for path in sys.path:
        print(f"  - {path}")
    sys.exit(1)

def send_message(sock, message):
    """发送带长度前缀的 Protobuf 消息"""
    # 序列化消息
    data = message.SerializeToString()
    # 发送消息长度前缀 (4字节网络字节序)
    sock.sendall(struct.pack("!I", len(data)))
    # 发送消息体
    sock.sendall(data)

def recv_message(sock, message_type):
    """接收带长度前缀的 Protobuf 消息"""
    # 接收消息长度前缀
    raw_msglen = recv_all(sock, 4)
    if not raw_msglen:
        return None
    msglen = struct.unpack("!I", raw_msglen)[0]
    # 接收消息体
    data = recv_all(sock, msglen)
    return message_type.FromString(data)

def recv_all(sock, n):
    """接收指定长度的数据"""
    data = bytearray()
    while len(data) < n:
        packet = sock.recv(n - len(data))
        if not packet:
            return None
        data.extend(packet)
    return data

def main():
    """主函数：连接服务器并发送请求"""
    # 创建请求
    request = analyzer_pb2.AnalysisRequest()
    request.data = "Sample data for analysis"
    request.issue_type = analyzer_pb2.ISSUE_ANR
    request.priority = 5
    request.options.append("option1")
    request.options.append("option2")
    
    print("Sending request:")
    print(f"  Data: {request.data}")
    print(f"  Priority: {request.priority}")
    print(f"  Options: {list(request.options)}")
    
    # 连接服务器
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect(("localhost", 50051))
            
            # 发送请求
            send_message(s, request)
            print("\nRequest sent. Waiting for response...")
            
            # 接收响应
            response = recv_message(s, analyzer_pb2.AnalysisResult)
            
            if response:
                print("\nReceived response:")
                print(f"  Status: {analyzer_pb2.AnalysisResult.Status.Name(response.status)}")
                print(f"  Result: {response.result_data}")
                print(f"  Processing time: {response.processing_time:.2f}s")
                if response.error_message:
                    print(f"  Error: {response.error_message}")
            else:
                print("No response received")
                
    except ConnectionRefusedError:
        print("\nError: Connection refused. Is the server running?")
    except socket.timeout:
        print("\nError: Connection timed out")
    except socket.error as e:
        print(f"\nSocket error: {str(e)}")

if __name__ == "__main__":
    main()
