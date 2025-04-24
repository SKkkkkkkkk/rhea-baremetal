.section .text.init
.global _start
.align 2

_start:
    # 设置 Machine Trap-Vector Base-Address Register
    la t0, trap_entry
    csrw mtvec, t0

    # 启用浮点单元 (FPU)
    # 设置 mstatus.FS 为 01 (初始状态)
    li t0, 0x00002000     # MSTATUS_FS_INITIAL (0x2000 = FS bits set to 01)
    csrrs t0, mstatus, t0 # 设置 mstatus 寄存器中的 FS 位

    # 设置栈指针
    #define hart_num 8
    # 为每个核心设置栈指针：sp = _stack_top - hartid * (stack_size / hart_num)
    call get_hartid         # a0 = hartid
    
    # 计算每个核心的栈大小
    la t0, _stack_top
    la t1, _stack_start
    sub t0, t0, t1          # t0 = 总栈大小
    li t2, hart_num
    div t0, t0, t2          # t0 = 每个核心栈大小
    
    # 计算当前核心的栈指针
    mul t0, t0, a0          # t0 = hartid * 每个核心栈大小
    la t1, _stack_top
    sub sp, t1, t0          # sp = _stack_top - 偏移量

    # secondary core boot
    call get_hartid
    bnez a0, secondary_core_boot
    
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
    la t0, _data_vma_start       # t0 = DATA段在RAM中的开始地址（VMA）
    la t2, _data_lma_start   # t2 = DATA段在ROM中的开始地址（LMA）
    beq t0, t2, start_main   # 如果LMA等于VMA，不需要搬运，直接跳到main
    
    # 需要初始化DATA段（从ROM复制到RAM）
    la t1, _data_vma_end         # t1 = DATA段在RAM中的结束地址
    
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

#ifdef QEMU
    # Exit QEMU
    li t0, 0x5555
    li t1, 0x100000
    sw t0, 0(t1)
#endif
    j .


    .global get_hartid
    .section .text.asm.get_hartid, "ax"
    .type get_hartid, @function
get_hartid:
    csrr a0, mhartid
    ret

    .section .text.asm.secondary_core_boot, "ax"
    .type secondary_core_boot, @function
secondary_core_boot:
    j .
