#!/bin/bash

if [[ $# -gt 0 ]]; then
    DIE_MAX_NUM=$1
else
    DIE_MAX_NUM=2
fi
echo "DIE_MAX_NUM 设置为 ${DIE_MAX_NUM}"

rm ./build/boot_rom_die*.hex 2>/dev/null

for (( die_id=0; die_id<DIE_MAX_NUM; die_id++ ))
do
    echo "开始处理 die${die_id}..."

    CC=aarch64-none-elf-gcc \
    cmake -S ../.. -B build -G Ninja \
    --graphviz=build/graph/dep.dot \
    -DPROJECT_NAME=pcie_ep_d2d \
    -DCMAKE_BUILD_TYPE=Release \
    -DAPP_CMAKE=app.cmake \
    -DBOARD=PLD \
    -DDIE_MAX_NUM=${DIE_MAX_NUM} \
    -DSELF_DIE_ID=${die_id} > /dev/null

    if [[ $? -ne 0 ]]; then
        echo "cmake 执行失败，退出脚本。"
        exit 1
    fi

    cmake --build build -t hex
    if [[ $? -ne 0 ]]; then
        echo "构建失败，退出脚本。"
        exit 1
    fi

    echo "python3 tran2pld.py build/pcie_ep_d2d.hex build/boot_rom_die${die_id}.hex"
    python3 tran2pld.py build/pcie_ep_d2d.hex build/boot_rom_die${die_id}.hex
    if [[ $? -ne 0 ]]; then
        echo "Python 脚本执行失败，退出脚本。"
        exit 1
    fi

    echo "die${die_id} 处理完成 。"
done

md5sum ./build/boot_rom_die*.hex