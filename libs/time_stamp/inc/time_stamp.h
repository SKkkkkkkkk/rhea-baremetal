#ifndef __TIME_STAMP_H__
#define __TIME_STAMP_H__

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "time_stamp_port.h"

#ifndef TIME_STAMP_CNT_BITS
	#error "TIME_STAMP_CNT_BITS is not defined"
#else
	#if TIME_STAMP_CNT_BITS == 16
		typedef uint16_t cnt_type;
	#elif TIME_STAMP_CNT_BITS == 32
		typedef uint32_t cnt_type;
	#elif TIME_STAMP_CNT_BITS == 64
		typedef uint64_t cnt_type;
	#else
		#error "TIME_STAMP_CNT_BITS is not supported"
	#endif
#endif

typedef struct __attribute__((packed, aligned(1))) {
	char func[32]; // 0  32B
	uint32_t line; // 32 4B
	cnt_type tick; // 36
} time_stamp_entry_t;

#define ENTRY_COUNT (*(uint32_t*)(uintptr_t)TIME_STAMP_BASE)

#define TIME_STAMP_INIT() ENTRY_COUNT = 0

#define TIME_STAMP() \
	do { \
		time_stamp_entry_t *entry = (void*)((uintptr_t)TIME_STAMP_BASE + sizeof(ENTRY_COUNT) + (ENTRY_COUNT * sizeof(time_stamp_entry_t)) ); \
		entry->tick = TIME_STAMP_GET_TICK(); \
		strncpy(entry->func, __func__, sizeof(entry->func)); \
		entry->line = __LINE__; \
		++ENTRY_COUNT; \
	} while (0)

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif