#include <stdio.h>
#include <stdint.h>

int main()
{
    printf("howdy\n");
    asm volatile(".word 0"); // illegal instruction
    printf("i'm still alive\n");
    return 0;
}