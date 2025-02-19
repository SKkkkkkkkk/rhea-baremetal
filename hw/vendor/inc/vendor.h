#ifndef __VENDOR_
#define __VENDOR_

#include <stdint.h>

/* debug print */
#if defined(DEBUG) && !defined(pr_dbg) && 0
#define pr_dbg(fmt, ...)    printf(fmt, ##__VA_ARGS__)
#else
#define pr_dbg(fmt, ...)    do {} while (0)
#endif

struct vendor_item {
	uint16_t  id;
	uint16_t  offset;
	uint16_t  size;
	uint16_t  flag;
};

int vendor_storage_read(uint16_t id, void *pbuf, uint16_t size);
int vendor_storage_write(uint16_t id, void *pbuf, uint16_t size);

#endif /* __VENDOR_ */
