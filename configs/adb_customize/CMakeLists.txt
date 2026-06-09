cmake_minimum_required(VERSION 3.10)
project(adb_customize)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ANDROID_BUILD_TOP $ENV{Android_build_top})
if(NOT ANDROID_BUILD_TOP)
        message(FATAL_ERROR "Environment variable Android_build_top is not set")
endif()

#SOurce and destination
set(dst_dir "${ANDROID_BUILD_TOP}/system/core/adb")
set(src_mybugrepot_h "${CMAKE_CURRENT_SOURCE_DIR}/src/mybugreport.h")
set(dst_mybugreport_h "${dst_dir}/mybugreport.h") 
set(src_mybugrepot_cpp "${CMAKE_CURRENT_SOURCE_DIR}/src/mybugreport.cpp")
set(dst_mybugreport_cpp "${dst_dir}/mybugreport.cpp") 

add_custom_target(copy_mybugreport_code ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${src_mybugrepot_cpp}" "${dst_mybugreport_cpp}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${src_mybugrepot_h}" "${dst_mybugreport_h}"
        DEPENDS "${src_mybugrepot_h}" "${src_mybugrepot_cpp}"
        COMMENT "Copying ${src_file} to ${dst_dir}"
)
