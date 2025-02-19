#include <stdio.h>
#include <vendor.h>
#include <string.h>

#include "xmodem.h"

#define VENDOR_OPS_MAX_LEN	128
typedef struct {
    unsigned short dir : 1; // 0: write, 1: read
    unsigned short id : 15;
	unsigned short len;
    char data[VENDOR_OPS_MAX_LEN - 4];
} vendor_ops_t;
vendor_ops_t vendor_ops = {0};

static unsigned int vendor_action(unsigned char *buf, int buflen)
{
    int ret_size;
    
    if (buflen == 0) {
        memcpy(&vendor_ops, (void *) buf, sizeof(vendor_ops_t));
        pr_dbg("[Vendor]: receive %s request for item %d with %d bytes\n",
                vendor_ops.dir ? "read" : "write",
                vendor_ops.id, vendor_ops.len);
    }
    return 0;
}

int main(void)
{
    int ret, i;

    printf("[Vendor]: Waiting for operation request ...\n");

    // vendor_ops.id = 0x10;
    // vendor_ops.len = VENDOR_OPS_MAX_LEN - 4;
    // ret = vendor_storage_read(vendor_ops.id, vendor_ops.data, vendor_ops.len);
    // printf("[Vendor]: 0x%x bytes read ( ", ret);
    // for (i = 0; i < ret; i++) {
    //     printf("%02x ", vendor_ops.data[i]);
    //     vendor_ops.data[i] += 1;
    // }
    // printf(" )\n");
    // vendor_storage_write(vendor_ops.id, vendor_ops.data, ret);

    while (1) {
        memset(&vendor_ops, 0, sizeof(vendor_ops));
        while( (ret = xmodemReceiveWithAction(vendor_action, VENDOR_OPS_MAX_LEN)) < 0)
            printf("[Vendor]: Failed to receive request with status %d, retrying ...\n", ret);

        // if (vendor_ops.dir) {
        //     ret = vendor_storage_read(vendor_ops.id, vendor_ops.data, vendor_ops.len);
        // } else {
        //     ret = vendor_storage_write(vendor_ops.id, vendor_ops.data, vendor_ops.len);
        // }
        // if (ret != vendor_ops.len) {
        //     pr_dbg("[Vendor]: The %s data size 0x%x(%d) does not meet expectations 0x%x.\n",
        //             vendor_ops.dir ? "read" : "write", ret, vendor_ops.len, vendor_ops.len);
        //     vendor_ops.len = -1;
        // } else {
        //     vendor_ops.len = ret;
        //     pr_dbg("[Vendor]: 0x%x bytes %s ( ", ret, vendor_ops.dir ? "read" : "written");
        //     for (i = 0; i < ret; i++) {
        //         pr_dbg("%02x ", vendor_ops.data[i]);
        //     }
        //     pr_dbg(" \n");
        // }
        ret = xmodemTransmit((void *) &vendor_ops, ret + 4);
        if (ret < 0) {
            printf("[Vendor]: Write back failed with status %d\n", ret);
        }
    }
	return 0;
}