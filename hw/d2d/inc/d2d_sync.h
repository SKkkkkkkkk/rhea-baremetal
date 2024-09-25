#ifndef __D2D_SYNC_H__
#define __D2D_SYNC_H__

#include "d2d_api.h"

enum d2d_sync_cmd_id {
    D2D_SYNC_DATA = 1,
    D2D_SYNC_WRITEL,
    D2D_SYNC_READL,
    D2D_SYNC_WRITEW,
    D2D_SYNC_READW,
    D2D_SYNC_WRITEB,
    D2D_SYNC_READB,

    D2D_SYNC_CMD_MAX,
};

struct d2d_sync_put_cmd {
    unsigned char die_idx;
    enum d2d_sync_cmd_id cmd_id;
    union {
        struct {
            void* data_addr;
            unsigned int data_size;
        };
        struct {
            unsigned int reg_addr;
            unsigned int reg_val;
        };
    };
};

int d2d_sync_wait_reg(struct d2d_sync_put_cmd *put_cmd,
                        uint32_t pos);
int d2d_sync_remote(struct d2d_sync_put_cmd *put_cmd);
int d2d_sync_query_cmd(enum d2d_sync_cmd_id cmd_id,
                                uint32_t *size_addr);
int d2d_sync_get_data(enum d2d_sync_cmd_id cmd_id,
                                void *data_addr);
int d2d_sync_obtain_cmd(void);
int rhea_d2d_sync_init(void);
void rhea_d2d_sync_exit(void);

#endif /* __D2D_SYNC_H__ */