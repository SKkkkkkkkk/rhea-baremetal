#!/bin/bash

rm ./build/boot_rom*.hex

for (( chip_id=0; chip_id<2; chip_id++ ))
do
    for (( die_id=0; die_id<2; die_id++ ))
    do
        echo "开始处理 chip${chip_id} die${die_id}..."

        CC=aarch64-none-elf-gcc \
        cmake -S ../.. -B build -G Ninja \
        --graphviz=build/graph/dep.dot \
        -DPROJECT_NAME=d2d_c2c \
        -DAPP_CMAKE=app.cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DPROJECT_PATH=. \
        -DBOARD=PLD \
        -DCHIP_ID=${chip_id} \
        -DDIE_MAX_NUM=2 \
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

        echo "python3 tran2pld.py build/d2d_c2c.hex build/boot_rom${chip_id}${die_id}.hex"
        python3 tran2pld.py build/d2d_c2c.hex build/boot_rom${chip_id}${die_id}.hex
        if [[ $? -ne 0 ]]; then
            echo "Python 脚本执行失败，退出脚本。"
            exit 1
        fi

        echo "chip${chip_id} die${die_id} 处理完成 。"
    done
done

md5sum ./build/boot_rom*.hex