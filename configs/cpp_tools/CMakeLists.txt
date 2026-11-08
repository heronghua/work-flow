cmake_minimum_required(VERSION 3.10)
project(cpp_tools)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(OpenCV REQUIRED opencv)

include_directories(${OpenCV_INCLUDE_DIRS})
link_directories(${OpenCV_LIBRARY_DIRS})

add_executable(NV212PNG src/NV212PNG.cpp)
target_link_options(NV212PNG PRIVATE "-Wl,--allow-shlib-undefined")
target_link_libraries(NV212PNG ${OpenCV_LIBRARIES})

add_executable(RetryUntilSuccess src/RetryUntilSuccess.cpp)

install(TARGETS NV212PNG RetryUntilSuccess 
        RUNTIME DESTINATION bin)
