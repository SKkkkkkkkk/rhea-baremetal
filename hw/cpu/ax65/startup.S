/* Entry point for bare metal programs */
.section .text.start
.global _start
.type _start, @function

_start:
    /* Set up the stack pointer */
    la sp, _stack_top

    /* Call the main function */
    call main

    /* Infinite loop */
    j .