cmake_minimum_required(VERSION 3.10)
project(analyzer-server CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Protobuf 包
find_package(Protobuf REQUIRED)
message(STATUS "Found Protobuf: ${PROTOBUF_VERSION}")
message(STATUS "Protobuf include dir: ${PROTOBUF_INCLUDE_DIRS}")
message(STATUS "Protobuf libraries: ${PROTOBUF_LIBRARIES}")
message(STATUS "Protobuf compiler: ${PROTOBUF_PROTOC_EXECUTABLE}")

# 设置在 build 目录下的 generated 目录
set(PROTOBUF_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${PROTOBUF_GENERATED_DIR})

# 添加自定义命令生成 Protobuf C++ 代码
add_custom_command(
    OUTPUT 
        ${PROTOBUF_GENERATED_DIR}/analyzer.pb.cc
        ${PROTOBUF_GENERATED_DIR}/analyzer.pb.h
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
        --cpp_out=${PROTOBUF_GENERATED_DIR}
        --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/analyzer.proto
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/analyzer.proto
    COMMENT "Generating C++ protobuf bindings"
)

# 添加自定义命令生成 Protobuf Python 代码
add_custom_command(
    OUTPUT ${PROTOBUF_GENERATED_DIR}/analyzer_pb2.py
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
        --python_out=${PROTOBUF_GENERATED_DIR}
        --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/analyzer.proto
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/analyzer.proto
    COMMENT "Generating Python protobuf bindings"
)

# 设置源文件
set(SERVER_SRC 
    ${CMAKE_CURRENT_SOURCE_DIR}/analyzer_server.cpp 
    ${PROTOBUF_GENERATED_DIR}/analyzer.pb.cc
)

# 添加可执行文件
add_executable(analyzer_server ${SERVER_SRC})

# 包含目录
target_include_directories(analyzer_server PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROTOBUF_GENERATED_DIR}  # 添加生成的目录
    ${PROTOBUF_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(analyzer_server PRIVATE
    ${PROTOBUF_LIBRARIES}
    pthread
)

# 添加 Python 客户端支持
find_package(PythonInterp 3 REQUIRED)
message(STATUS "Python interpreter: ${PYTHON_EXECUTABLE}")

# 复制客户端脚本到构建目录
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/client.py 
               ${CMAKE_CURRENT_BINARY_DIR}/client.py
               COPYONLY)

# 创建测试目标
add_custom_target(run_test
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/client.py
    DEPENDS analyzer_server 
            ${PROTOBUF_GENERATED_DIR}/analyzer_pb2.py
            ${CMAKE_CURRENT_BINARY_DIR}/client.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running Python client test"
)

# 添加运行目标
add_custom_target(run_server
    COMMAND ./analyzer_server
    DEPENDS analyzer_server
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running analyzer server"
)

# 确保生成目录存在
add_custom_command(TARGET analyzer_server PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROTOBUF_GENERATED_DIR}
)

# 打印重要路径信息
message(STATUS "Protobuf generated directory: ${PROTOBUF_GENERATED_DIR}")
message(STATUS "Binary directory: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "Source directory: ${CMAKE_CURRENT_SOURCE_DIR}")
