.section .text.init
.global _start
.align 2

_start:
    # 设置栈指针
    la sp, _stack_top
    
    # 启用浮点单元 (FPU)
    # 设置 mstatus.FS 为 01 (初始状态)
    li t0, 0x00002000     # MSTATUS_FS_INITIAL (0x2000 = FS bits set to 01)
    csrrs t0, mstatus, t0 # 设置 mstatus 寄存器中的 FS 位
    
    # 初始化BSS段（清零）
    la t0, _bss_start    # t0 = BSS段开始地址
    la t1, _bss_end      # t1 = BSS段结束地址
    
clear_bss:
    bgeu t0, t1, check_data   # 如果t0 >= t1，说明BSS段已清零完毕，检查DATA段
    sw zero, 0(t0)           # 将0写入当前地址
    addi t0, t0, 4           # 移动到下一个字
    j clear_bss              # 继续循环

check_data:
    # 检查.data段的LMA和VMA是否相同
    la t0, _data_start       # t0 = DATA段在RAM中的开始地址（VMA）
    la t2, _data_rom_start   # t2 = DATA段在ROM中的开始地址（LMA）
    beq t0, t2, start_main   # 如果LMA等于VMA，不需要搬运，直接跳到main
    
    # 需要初始化DATA段（从ROM复制到RAM）
    la t1, _data_end         # t1 = DATA段在RAM中的结束地址
    
copy_data:
    bgeu t0, t1, start_main  # 如果t0 >= t1，说明DATA段已初始化完毕，跳到main
    lw t3, 0(t2)             # 从ROM加载一个字
    sw t3, 0(t0)             # 存储到RAM
    addi t0, t0, 4           # 更新RAM地址
    addi t2, t2, 4           # 更新ROM地址
    j copy_data              # 继续循环
    
start_main:
    # 跳转到main函数
    call main
    
    # Exit QEMU
    li t0, 0x5555
    li t1, 0x100000
    sw t0, 0(t1)

    j . // should never reach here