cmake_minimum_required(VERSION 3.0.0)  #指定有效cmake的最低版本

project(HttpServer)  #指定工程的名称

set(CMAKE_CXX_STANDARD 11)  #指定c++11为最低标准

set(HOME /home/mical/HttpSevver/HttpServer-CPP)
set(EXECUTABLE_OUTPUT_PATH ${HOME}/build)  #指定输出路径

# aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC)  #指定源文件1
file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)  #指定源文件1  file相对复杂，可以递归查找目录（GLOB_RECURSE）

include_directories(${PROJECT_SOURCE_DIR}/include)  #指定头文件

add_executable(a.out ${SRC})  #工程会生成一个可执行程序