#include <stdio.h>
#include <vendor.h>
#include <string.h>

#include "xmodem.h"

#define VENDOR_OPS_MAX_LEN	128
typedef struct {
    unsigned short id : 15;
    unsigned short dir : 1; // 0: write, 1: read
	unsigned short len;
    char data[VENDOR_OPS_MAX_LEN - 4];
} vendor_ops_t;
vendor_ops_t vendor_ops = {0};

static unsigned int vendor_action(unsigned char *buf, int buflen)
{
    int ret_size;
    
    if (buflen != 0) {
        memcpy(&vendor_ops, (void *) buf, sizeof(vendor_ops_t));
    }
    return 0;
}

int main(void)
{
    int ret, i;

    printf("[Vendor]: Waiting for operation request ...\n");
    while (1) {
        memset(&vendor_ops, 0, sizeof(vendor_ops));
        while( (ret = xmodemReceiveWithAction(vendor_action, VENDOR_OPS_MAX_LEN)) < 0)
            printf("[Vendor]: Failed to receive request with status %d, retrying ...\n", ret);

        if (vendor_ops.dir) {
            ret = vendor_storage_read(vendor_ops.id, vendor_ops.data, vendor_ops.len);
        } else {
            ret = vendor_storage_write(vendor_ops.id, vendor_ops.data, vendor_ops.len);
        }
        vendor_ops.len = ret;
        ret = xmodemTransmit((void *) &vendor_ops, VENDOR_OPS_MAX_LEN);
        if (ret < 0) {
            printf("[Vendor]: Write back failed with status %d\n", ret);
        }
    }
	return 0;
}