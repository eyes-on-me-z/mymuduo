#!/bin/bash

# 只要有一条命令失败，脚本立即退出
set -e

# 如果没有build目录，创建该目录
if [ ! -d $(pwd)/build ]; then
    mkdir $(pwd)/build
fi

rm -rf $(pwd)/build/*

cd $(pwd)/build &&  # 进入 build 目录
    cmake .. &&     # 运行 CMake
    make            # 编译

# 回到项目根目录
cd ..

# 把头文件拷贝到 /usr/include/mymuduo  so库拷贝到  /usr/lib     PATH
if [ ! -d /usr/include/mymuduo ]; then
    mkdir /usr/include/mymuduo
fi

for header in ./src/*.h
do 
    cp $header /usr/include/mymuduo
done

cp $(pwd)/lib/libmymuduo.so /usr/lib

# 刷新动态库缓存
ldconfig