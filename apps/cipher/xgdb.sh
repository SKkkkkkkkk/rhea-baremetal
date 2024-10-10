#!/bin/bash

# select witch case
sed -i 6,15s/"^#"/'\/\/'/g inc/cipher_test_debg.h
test_name=NONE

echo "------------------------------------------"
echo " case 1: 测试SPACC-AES_CBC"
echo " case 2: 测试SPACC-SHA3-256"
echo " case 3: 测试PKA-ECC"
echo " case 4: 测试SPACC-AES_CBC + efuse推送key"
echo " case 5: 测试PKA-ECC + efuse推送key"
echo " case 6: 测试SPACC-AES_CBC + 测中断"
echo " case 7: 测试PKA-ECC + 测中断"
echo " case 8: 测试efuse"
echo " case 9: 测试SPACC GCM"
echo "------------------------------------------"
read -p "test case select: " sel

case ${sel} in
    1)
    sed -i 6,15s/"^\/\/.*TEST_CIPH_CIPHER"/'#define TEST_CIPH_CIPHER'/g inc/cipher_test_debg.h
    test_name=SPACC-AES-CBC
    ;;

    2)
    sed -i 6,15s/"^\/\/.*TEST_CIPH_HASH"/'#define TEST_CIPH_HASH'/g inc/cipher_test_debg.h
    test_name=SPACC-SHA3-256
    ;;

    3)
    sed -i 6,15s/"^\/\/.*TEST_CIPH_ECC"/'#define TEST_CIPH_ECC'/g inc/cipher_test_debg.h
    test_name=PKA-ECC
    ;;

    4)
    sed -i 6,15s/"^\/\/.*TEST_CIPH_CIPHER"/'#define TEST_CIPH_CIPHER'/g inc/cipher_test_debg.h
    sed -i 6,15s/"^\/\/.*TEST_KEY_BY_EFUSE"/'#define TEST_KEY_BY_EFUSE'/g inc/cipher_test_debg.h
    sed -i 6,15s/"^\/\/.*TEST_KEY_BURN_EFUSE"/'#define TEST_KEY_BURN_EFUSE'/g inc/cipher_test_debg.h
    test_name=SPACC-AES-CBC-EFUSE
    ;;

    5)
    sed -i 6,15s/"^\/\/.*TEST_CIPH_ECC"/'#define TEST_CIPH_ECC'/g inc/cipher_test_debg.h
    sed -i 6,15s/"^\/\/.*TEST_KEY_BY_EFUSE"/'#define TEST_KEY_BY_EFUSE'/g inc/cipher_test_debg.h
    sed -i 6,15s/"^\/\/.*TEST_KEY_BURN_EFUSE"/'#define TEST_KEY_BURN_EFUSE'/g inc/cipher_test_debg.h
    test_name=PKA-AES-CBC-EFUSE
    ;;

    6)
    sed -i 6,15s/"^\/\/.*TEST_CIPH_CIPHER"/'#define TEST_CIPH_CIPHER'/g inc/cipher_test_debg.h
    sed -i 6,15s/"^\/\/.*TEST_CIPH_IRQ"/'#define TEST_CIPH_IRQ'/g inc/cipher_test_debg.h
    test_name=SPACC-AES-INTR
    ;;

    7)
    sed -i 6,15s/"^\/\/.*TEST_CIPH_ECC"/'#define TEST_CIPH_ECC'/g inc/cipher_test_debg.h
    sed -i 6,15s/"^\/\/.*TEST_CIPH_IRQ"/'#define TEST_CIPH_IRQ'/g inc/cipher_test_debg.h
    test_name=PKA-ECC-INTR
    ;;

    8)
    sed -i 6,15s/"^\/\/.*TEST_EFUSE_SEC"/'#define TEST_EFUSE_SEC'/g inc/cipher_test_debg.h
    test_name=EFUSE-RW-PROT
    ;;

    9)
    sed -i 6,15s/"^\/\/.*TEST_CIPH_CIPHER"/'#define TEST_CIPH_CIPHER'/g inc/cipher_test_debg.h
    sed -i s/gcm_test.*=.*false/'gcm_test = true'/g src/cipher_test.c
    test_name=EFUSE-RW-PROT
    ;;

    *)
    echo "Input is not support!"
    test_name=NONE
    ;;
esac

#./build.sh
cmake --build build/
sed -i s/gcm_test.*=.*true/'gcm_test = false'/g src/cipher_test.c

if [ $? -ne 0 ]; then
    echo "build $test_name error, exit!"
    exit -1;
fi

# if [ ! -d "target/" ]; then
#     mkdir target
# fi
# cp build/cipher.elf ./target/${test_name}.elf
# cp build/cipher.bin ./target/${test_name}.bin

if [ $# -gt 0 ]; then
    # use fpga4
    if [ $1 == 4 ]; then
        sed -i s/"^tar .*"/"tar extended-remote 192.168.31.150:3334"/g gdbinit
        gdb-multiarch build/cipher.elf -x gdbinit
    fi

    # use fpga5
    if [ $1 == 5 ]; then
        sed -i s/"^tar .*"/"tar extended-remote 192.168.31.14:3334"/g gdbinit
        gdb-multiarch build/cipher.elf -x gdbinit
    fi


    # use fpga6
    if [ $1 == 6 ]; then
        sed -i s/"^tar .*"/"tar extended-remote 192.168.31.216:3334"/g gdbinit
        gdb-multiarch build/cipher.elf -x gdbinit
    fi

    # use fpga9
    if [ $1 == 9 ]; then
        sed -i s/"^tar .*"/"tar extended-remote 192.168.31.219:3334"/g gdbinit
        gdb-multiarch build/cipher.elf -x gdbinit
    fi

fi
