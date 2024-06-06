#include "arch.h"
#include "asm_macros.S"

#define PRIMARY_CORE 0

	.globl entrypoint
func entrypoint
	mov x0, #PRIMARY_CORE
	mov x19, x0 // x19 save the primary core id.

	mov	x1, #(SCTLR_I_BIT | SCTLR_A_BIT | SCTLR_SA_BIT)
	// mov	x1, #(SCTLR_I_BIT | SCTLR_SA_BIT)
	asm_read_sctlr_elx
	orr	x0, x0, x1
	asm_write_sctlr_elx x1
	isb

	ldr x0, =vector_table
	asm_write_vbar_elx x1
	isb

	mrs     x1, CurrentEL
	cmp     x1, #(MODE_EL3 << MODE_EL_SHIFT)
	b.ne    rtl_init_start
	mrs x0, scr_el3
	mov x0, #0x53e                 	// #568
	msr scr_el3, x0

rtl_init_start:
#if defined(RTL)
	// Initialise the register bank
	mov x0, xzr
	mov x1, xzr
	mov x2, xzr
	mov x3, xzr
	mov x4, xzr
	mov x5, xzr
	mov x6, xzr
	mov x7, xzr
	mov x8, xzr
	mov x9, xzr
	mov x10, xzr
	mov x11, xzr
	mov x12, xzr
	mov x13, xzr
	mov x14, xzr
	mov x15, xzr
	mov x16, xzr
	mov x17, xzr
	mov x18, xzr
	// mov x19, xzr
	mov x20, xzr
	mov x21, xzr
	mov x22, xzr
	mov x23, xzr
	mov x24, xzr
	mov x25, xzr
	mov x26, xzr
	mov x27, xzr
	mov x28, xzr
	mov x29, xzr
	mov x30, xzr

	// Enable NEON and initialize the register bank if this
	// feature has been implemented
	mrs     x0, ID_AA64PFR0_EL1
	sbfx    x5, x0, #16, #4         // Extract the floating-point field
    cmp     x5, #-1                 // x0 == -1 if FP not present
	b.eq    rtl_init_end            // Skip FP initialization if not present

	movi    v0.2d,   #0
	movi    v1.2d,   #0
	movi    v2.2d,   #0
	movi    v3.2d,   #0
	movi    v4.2d,   #0
	movi    v5.2d,   #0
	movi    v6.2d,   #0
	movi    v7.2d,   #0
	movi    v8.2d,   #0
	movi    v9.2d,   #0
	movi    v10.2d,  #0
	movi    v11.2d,  #0
	movi    v12.2d,  #0
	movi    v13.2d,  #0
	movi    v14.2d,  #0
	movi    v15.2d,  #0
	movi    v16.2d,  #0
	movi    v17.2d,  #0
	movi    v18.2d,  #0
	movi    v19.2d,  #0
	movi    v20.2d,  #0
	movi    v21.2d,  #0
	movi    v22.2d,  #0
	movi    v23.2d,  #0
	movi    v24.2d,  #0
	movi    v25.2d,  #0
	movi    v26.2d,  #0
	movi    v27.2d,  #0
	movi    v28.2d,  #0
	movi    v29.2d,  #0
	movi    v30.2d,  #0
	movi    v31.2d,  #0

	//! This code may cause synchronization issues if all cores are not released at the same time.
	bl  get_core_id
	ldr x1, =secondary_cores_entry
	mov x2, x0
	lsl x2, x2, 4 // x0 = x0 * 16
	str xzr, [x1, x2]
	add x2, x2, 8
	str xzr, [x1, x2]
#endif
rtl_init_end:

	msr spsel, 1
	bl get_core_id
	bl setup_stack

	bl get_core_id
	cmp x0, x19
	bne data_init_end

	ldr	x0, =_bss_start
	ldr	x1, =_bss_end
	cmp x0, x1
	b.eq clear_loop_end
	movi    v0.2d,   #0
clear_loop:
	stp	q0, q0, [x0], #32
	cmp	x0, x1
	b.lo	clear_loop
clear_loop_end:

	ldr x0, =_data_start
	ldr x1, =_data_start_lma
	ldr x2, =_data_size
	cmp x0, x1
	b.eq data_init_end
data_init_start:
	cbz x2, data_init_end
	ldp	q0, q1, [x1], #32
	stp	q0, q1, [x0], #32
	sub	x2, x2, #32
	b data_init_start
data_init_end:

	msr daifclr, #0xf	//enable syserror,irq,fiq

	bl get_core_id
	cmp x0, x19
	bne secondary_core_boot
#if defined(ENABLE_MMU_CACHE) && (ENABLE_MMU_CACHE != 0)
	bl config_mmu
#endif
	bl main
dead:
	wfi
	b	dead
endfunc entrypoint

#define CORE_STACK_SIZE (16*1024/4)
// x0 - core_id
func setup_stack
	.extern _stack_top
	ldr x1, =_stack_top
	bfi x1, xzr, #0, #4 // clear bit0-3

	/* 
	** x0 => core_id
	** x1 => stack_top
	** x2 => offset_size 
	*/
	ldr x2, =CORE_STACK_SIZE
	mul x2, x2, x0                  //stack offset for each cpuid
	sub x1, x1, x2                  //stack top for each cpuid
	mov sp, x1

	ret
endfunc setup_stack

/*
	x0 - core-id
 */
#define CORE_WAKEUP_KEY 0x123456788654321
func secondary_core_boot
	ldr x1, =secondary_cores_entry
	mov x2, x0
	lsl x2, x2, 4 // x0 = x0 * 16

	ldr x3, [x1, x2]
	cbz x3, 1f
	
	add x2, x2, 8
	ldr x4, [x1, x2]
	mov_imm x5, CORE_WAKEUP_KEY
	cmp x4, x5
	bne 1f

	blr x3
	b	.

1:
	wfe
	b secondary_core_boot
endfunc secondary_core_boot

	.globl get_core_id
func get_core_id
	mrs x0, mpidr_el1
	mov x1, x0
	and x0, x0, 0xffff
	and x1, x1, #(1<<24)
	cbnz x1, MT
	ret
MT:
	lsr x0, x0, 8
	ret
endfunc get_core_id

	.globl getAffinity
func getAffinity
  mrs      x0, mpidr_el1
  ubfx     x1, x0, #32, #8
  bfi      w0, w1, #24, #8
  ret
endfunc getAffinity