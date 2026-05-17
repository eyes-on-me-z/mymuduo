cmake_minimum_required(VERSION 3.10)

#设置项目名称
project(mymuduo)

# 添加搜索路径
include_directories(
    ${PROJECT_SOURCE_DIR}/src/base
    ${PROJECT_SOURCE_DIR}/src/http
    ${PROJECT_SOURCE_DIR}/src/logger
    ${PROJECT_SOURCE_DIR}/src/net
    ${PROJECT_SOURCE_DIR}/src/utils
)

# cmake => makefile     make
# mymuduo最终编译成so动态库，设置动态库的路径，放在根目录的lib文件夹下面
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
# 设置调试信息 以及 启动c++11语言标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++17 -fPIC")

# 定义参与编译的源代码文件
aux_source_directory(./src/base SRC_BASE)
aux_source_directory(./src/http SRC_HTTP)
aux_source_directory(./src/logger SRC_LOGGER)
aux_source_directory(./src/net SRC_NET)
aux_source_directory(./src/utils SRC_UTILS)

# 编译生成动态库mymuduo
add_library(mymuduo SHARED ${SRC_BASE} ${SRC_HTTP} ${SRC_LOGGER} ${SRC_NET} ${SRC_UTILS})

# 目标动态库所需连接的库（这里需要连接libpthread.so）
target_link_libraries(mymuduo pthread)