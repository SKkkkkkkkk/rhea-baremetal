// =====================
// newlib stubs for baremetal
// =====================
#include <errno.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>

#define DEFAULT_CONSOLE_BAUDRATE 115200U

// 平台相关串口初始化和收发
#if defined(VIRT)
#	if defined(A55)
#		include "pl011.h"
#	else
#		include "16550.h"
#	endif
#	define UART_ID 0
#	define UART_PUTCHAR uart_putchar
#	define UART_GETCHAR uart_getchar
#	define UART_CONFIG() (console_init = true)
#elif defined(RTL)
#	define UART_ID 0
#	define UART_PUTCHAR(c) (*(volatile uint32_t *)0x06400000 = (c))
#	define UART_GETCHAR() (0)
#	define UART_CONFIG() (console_init = true)
#else
#	include "cru.h"
#	include "dw_apb_uart.h" 
#	define UART_ID SEEHI_UART0
#	define UART_PUTCHAR(c) uart_sendchar(UART_ID, (c)); REG32(SYSCTRL_BASE + 0xfe0) = (c)
#	define UART_GETCHAR() uart_getchar(UART_ID)
#	define UART_CONFIG() do { \
    seehi_uart_config_baudrate(DEFAULT_CONSOLE_BAUDRATE, get_clk(CLK_UART), UART_ID); \
    console_init = true; \
} while(0)
#endif

static bool console_init = false;

// =====================
// 通用_write/_read实现
// =====================
int _write(int fd __attribute__((unused)), char *ptr, int len) {
    if (!console_init) UART_CONFIG();
    for (int i = 0; i < len; i++) {
        UART_PUTCHAR(ptr[i]);
        if (ptr[i] == '\n') UART_PUTCHAR('\r');
    }
    return len;
}

int _read(int fd __attribute__((unused)), char *ptr, int len) {
    if (!console_init) UART_CONFIG();
    for (int i = 0; i < len; i++) {
        ptr[i] = UART_GETCHAR();
    }
    return len;
}

// =====================
// Syscall stubs
// =====================
__attribute__((__used__)) void _exit(int status __attribute__((unused))) { while(1); }
__attribute__((__used__)) int _close(int file __attribute__((unused))) { return -1; }
__attribute__((__used__)) int _fstat(int file __attribute__((unused)), struct stat *st) { st->st_mode = S_IFCHR; return 0; }
__attribute__((__used__)) int _getpid(void) { return 1; }
__attribute__((__used__)) int _isatty(int file __attribute__((unused))) { return 1; }
__attribute__((__used__)) int _kill(int pid __attribute__((unused)), int sig __attribute__((unused))) { errno = EINVAL; return -1; }
__attribute__((__used__)) int _lseek(int file __attribute__((unused)), int ptr __attribute__((unused)), int dir __attribute__((unused))) { return 0; }
__attribute__((__used__)) void *_sbrk(int incr) {
	extern char _heap_start;
	extern char _heap_end;
	static unsigned char *heap = (unsigned char *)(uintptr_t)(&_heap_start);
	unsigned char *prev_heap = heap;
	if((uintptr_t)(heap + incr) > (uintptr_t)&_heap_end) {
		_write(1, "Heap Overflow!\n\r", 16);
		while(1) asm volatile("");
	}
	heap += incr;
	return prev_heap;
}

// =====================
// FreeRTOS malloc lock support
// =====================
#if defined(FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
__attribute__((__used__)) void __malloc_lock(struct _reent *r __attribute__((unused))) {
	configASSERT(!xPortIsInsideInterrupt());
	if(xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) return;
	vTaskSuspendAll();
}
__attribute__((__used__)) void __malloc_unlock(struct _reent *r __attribute__((unused))) {
	configASSERT(!xPortIsInsideInterrupt());
	if(xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) return;
	(void)xTaskResumeAll();
}
#endif

// =====================
// 环境变量
// =====================
char *__env[1] = { 0 };
char **environ = __env;

// =====================
// 其它stubs
// =====================
__attribute__((__used__)) int link(const char *old, const char *new_) { errno = EMLINK; return -1; }
__attribute__((__used__)) int unlink(const char *name) { errno = ENOENT; return -1; }
__attribute__((__used__)) int stat(const char *path, struct stat *st) { st->st_mode = S_IFCHR; return 0; }
__attribute__((__used__)) int _open(const char *name __attribute__((unused)), int flags __attribute__((unused)), int mode __attribute__((unused))) { return -1; }
__attribute__((__used__)) int fork(void) { errno = EAGAIN; return -1; }
__attribute__((__used__)) int wait(int *status __attribute__((unused))) { errno = ECHILD; return -1; }
__attribute__((__used__)) void _fini() { return; }

// =====================
// 通用__wrap_*实现，保证链接通过
// =====================
void *__wrap_memset(void *dst, int val, size_t count) {
    unsigned char *ptr = (unsigned char *)dst;
    while (count--) *ptr++ = (unsigned char)val;
    return dst;
}
void *__wrap_memcpy(void *dst, const void *src, size_t len) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (len--) *d++ = *s++;
    return dst;
}
void *__wrap_memmove(void *dst, const void *src, size_t len) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    if (d < s) {
        while (len--) *d++ = *s++;
    } else {
        d += len;
        s += len;
        while (len--) *--d = *--s;
    }
    return dst;
}
int __wrap_memcmp(const void *s1, const void *s2, size_t len) {
    const unsigned char *a = (const unsigned char *)s1, *b = (const unsigned char *)s2;
    while (len--) {
        if (*a != *b) return *a - *b;
        a++; b++;
    }
    return 0;
}
size_t __wrap_strlen(const char *s) {
    const char *p = s;
    while (*p) ++p;
    return p - s;
}
