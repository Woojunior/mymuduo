cmake_minimum_required(VERSION 2.5)
project(mymuduo)

#mymuduo最终编译成so动态库，设置动态库的路径，放在根目录的lib文件夹下
#${PROJECT_SOURCE_DIR} 是 CMake 的内置变量，指向项目的根目录（CMakeLists.txt 文件所在的目录）。
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#设置调试信息 以及 启动C++11语言标准
# CMAKE_CXX_FLAGS 是 CMake 的变量，存储编译 C++ 源文件时传递给编译器的选项。
# 这里添加了两个选项：
# -g：生成调试信息，用于调试工具（如 GDB）。
# -std=c++11：启用 C++11 标准，确保项目代码可以使用 C++11 的特性。
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

#定义参与编译的源文件
#查找当前目录（.）中的所有源文件，并将它们存储到变量 SRC_LIST 中
aux_source_directory(. SRC_LIST)
#编译生成动态库
# 功能：将源文件列表编译成一个共享库（动态库）。
# 解释：
# add_library 是 CMake 的命令，用于创建库。
# mymuduo 是生成的动态库的名字。
# SHARED 表示生成一个动态库（.so 或 .dll），而不是静态库（STATIC）。
# ${SRC_LIST} 是先前定义的变量，包含所有参与编译的源文件。
add_library(mymuduo SHARED ${SRC_LIST})