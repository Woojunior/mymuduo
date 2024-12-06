# #!/bin/bash
# #这段脚本的主要目的是将头文件 和 so库放到系统路径中


# set -e

# # 如果没有 build 目录，创建该目录
# if [ ! -d "$(pwd)/build" ]; then
#     mkdir "$(pwd)/build"
# fi

# rm -rf "$(pwd)/build/*"

# cd "$(pwd)/build" &&
#     cmake .. &&
#     make

# # 回到项目根目录
# cd ..

# # 把头文件拷贝到 /usr/include/mymuduo
# if [ ! -d /usr/include/mymuduo ]; then
#     mkdir /usr/include/mymuduo
# fi

# # 列出当前目录下所有的头文件，拷贝到目录 /usr/include/mymuduo
# for header in $(ls *.h)
# do
#     cp "$header" /usr/include/mymuduo
# done

# # 再把 so 库拷贝到 /usr/lib
# cp "$(pwd)/lib/libmymuduo.so" /usr/lib


# # 刷新动态库缓存
# sudo ldconfig

# echo "Build and installation completed successfully!"


#!/bin/bash

set -e

# 获取当前路径
PROJECT_DIR="$(pwd)"

# 如果没有 build 目录，创建该目录
if [ ! -d "$PROJECT_DIR/build" ]; then
    mkdir "$PROJECT_DIR/build"
fi

# 删除 build 目录中的所有内容
rm -rf "$PROJECT_DIR/build/*"

# 进入 build 目录并构建项目
cd "$PROJECT_DIR/build" &&
    cmake .. &&
    make

# 回到项目根目录
cd "$PROJECT_DIR"

# 将头文件拷贝到 /usr/include/mymuduo
if [ ! -d /usr/include/mymuduo ]; then
    sudo mkdir /usr/include/mymuduo
fi

# 列出当前目录下所有的头文件，并拷贝到 /usr/include/mymuduo
for header in $(ls *.h); do
    sudo cp "$header" /usr/include/mymuduo
done

# 拷贝 so 库到 /usr/lib
sudo cp "$PROJECT_DIR/lib/libmymuduo.so" /usr/lib

# 确保 libcuda.so.1 是符号链接（如果没有的话）
if [ ! -L /usr/lib/wsl/lib/libcuda.so.1 ]; then
    if [ -f /usr/lib/wsl/lib/libcuda.so ]; then
        sudo ln -sf /usr/lib/wsl/lib/libcuda.so /usr/lib/wsl/lib/libcuda.so.1
    else
        echo "libcuda.so not found, skipping symlink creation."
    fi
fi

# 刷新动态库缓存
sudo ldconfig

echo "Build and installation completed successfully!"
