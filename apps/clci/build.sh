#!/bin/bash

BLOCKING_MODE=ON    # ON or OFF
ISR_MODE=ON
SUFFIX=$([[ "$BLOCKING_MODE" == "OFF" ]] && echo "nonblock" || echo "block")$([[ "$ISR_MODE" == "ON" ]] && echo "_isr")
DIE_MAX_NUM=2
echo "DIE_MAX_NUM 设置为 ${DIE_MAX_NUM}"

rm ./build/die*_rom.hex.*

for (( die_id=0; die_id<DIE_MAX_NUM; die_id++ ))
do
    echo "开始处理 die${die_id}..."

    CC=aarch64-none-elf-gcc \
    cmake -S ../.. -B build -G Ninja \
    --graphviz=build/graph/dep.dot \
    -DPROJECT_NAME=clci \
    -DCMAKE_BUILD_TYPE=Release \
    -DAPP_CMAKE=app.cmake \
    -DBOARD=PLD \
    -DREAL_CLCI=ON \
    -DBLOCKING_MODE=${BLOCKING_MODE} \
    -DISR_MODE=${ISR_MODE} \
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

    echo "python3 tran2pld.py build/clci.hex build/die${die_id}_rom.hex.${SUFFIX}"
    python3 tran2pld.py build/clci.hex build/die${die_id}_rom.hex.${SUFFIX}
    if [[ $? -ne 0 ]]; then
        echo "Python 脚本执行失败，退出脚本。"
        exit 1
    fi

    echo "die${die_id} 处理完成 。"
done

md5sum ./build/die*_rom.hex.*