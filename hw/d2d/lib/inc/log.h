#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <stddef.h>

enum {
	LOG_VERBOSE,
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR,
	LOG_FATAL,
	LOG_MAX
};

#define LOG_DEFAULT_LEVEL			LOG_INFO

#define clci_print(level, module, fmt, ...)	\
    do { \
        if (level >= LOG_DEFAULT_LEVEL) { \
            printf("[%s]\t", #module); \
            printf(fmt, ##__VA_ARGS__); \
            printf("\n"); \
        } \
    } while (0)

#define ASSERT(_cond)                                                \
	do {                                                         \
		if (!(_cond)) {                                      \
			printf("ASSERT %s: %d: %s: %s\n",            \
			      __FILE__, __LINE__, __func__, #_cond); \
			while (1)                                    \
				;                                    \
		}                                                    \
	} while (0)

#define VERBOSE(module, ...)			clci_print(LOG_VERBOSE, module, __VA_ARGS__)
#define INFO(module, ...)			clci_print(LOG_INFO, module, __VA_ARGS__)
#define WARNING(module, ...)			clci_print(LOG_WARNING, module, __VA_ARGS__)
#define ERROR(module, ...)			clci_print(LOG_ERROR, module, __VA_ARGS__)
#define FATAL(module, ...)			clci_print(LOG_FATAL, module, __VA_ARGS__)

#endif
