#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include "d2d_sync.h"
#include "d2d_ring_buf.h"
#include "list.h"
#include "delay.h"
#include "crc32.h"
#include "io.h"

#define RHEA_AP_TILE_ID         0x04

struct d2d_sync_header {
    uint32_t cmd_head;
    uint32_t cmd_tail;
    uint32_t data_head;
    uint32_t data_tail;
};

struct d2d_sync_cmd {
    union {
        struct {
            uint64_t data_head;
            uint32_t data_size;
            uint32_t data_crc;
        };
        struct {
            uint64_t reg_addr;
            uint32_t reg_val;
            /**
             * 1: owned by the executor
             * 0: owned by the initiator
             */
            uint32_t reg_own;
        };
    };
    uint32_t cmd_idx;
    uint32_t cmd_crc;
};

struct d2d_sync_list {
    struct d2d_sync_cmd cmd;
    void *data;
    uint32_t size;
    DECLARE_LIST_NODE;
};

struct d2d_sync_priv {
    list_t(struct d2d_sync_list) cmd_list;
    struct d2d_sync_header *header;
    struct d2d_ring_buf rcmd;
    struct d2d_ring_buf rdata;
    struct d2d_ring_buf lcmd;
    struct d2d_ring_buf ldata;
} *sync_priv;

int d2d_sync_wait_reg(struct d2d_sync_put_cmd *put_cmd,
                        uint32_t pos)
{
    uintptr_t cmd;
    uint64_t reg_addr;
    uint32_t cmd_idx;
    uint32_t reg_own;
    uint32_t reg_val;
    uint32_t timeout;

    if ((put_cmd->cmd_id == D2D_SYNC_WRITEL) ||
        (put_cmd->cmd_id == D2D_SYNC_READL)  ||
        (put_cmd->cmd_id == D2D_SYNC_WRITEW) ||
        (put_cmd->cmd_id == D2D_SYNC_READW)  ||
        (put_cmd->cmd_id == D2D_SYNC_WRITEB) ||
        (put_cmd->cmd_id == D2D_SYNC_READB)) {
        cmd = sync_priv->lcmd.remote_addr + pos;
        rhea_d2d_readq(&reg_addr, cmd + offsetof(struct d2d_sync_cmd, reg_addr));
        if (put_cmd->reg_addr != reg_addr) {
            printf("The register address 0x%lx obtained at "
                "address 0x%lx does not match the expected 0x%lx\n",
                reg_addr, cmd, put_cmd->reg_addr);
            return -EFAULT;
        }

        rhea_d2d_readl(&cmd_idx, cmd + offsetof(struct d2d_sync_cmd, cmd_idx));
        pr_dbg("Waiting for the command %d at address 0x%lx to complete\n",
                cmd_idx, cmd);
        // Waiting for the executor to release
        for (timeout = 500; timeout > 0; timeout--) {
            rhea_d2d_readl(&reg_own, cmd + offsetof(struct d2d_sync_cmd, reg_own));
            if (!reg_own) break;
            // mdelay(1); // TODO: PLD will be blocked here
        }
        if (!timeout) return -ETIMEDOUT;

        rhea_d2d_readl(&reg_val, cmd + offsetof(struct d2d_sync_cmd, reg_val));
        if ((cmd_idx == D2D_SYNC_READL) ||
            (cmd_idx == D2D_SYNC_READW) ||
            (cmd_idx == D2D_SYNC_READB)) {
            put_cmd->reg_val = reg_val;
            pr_dbg("0x%08x has been read from address 0x%010lx\n",
                    reg_val, reg_addr);
        } else {
            pr_dbg("0x%08x has been written to address 0x%010lx\n",
                    reg_val, reg_addr);
        }
    }
    return 0;
}

int d2d_sync_remote(struct d2d_sync_put_cmd *put_cmd)
{
    int ret;
    struct d2d_sync_cmd *cmd;
    const size_t cmd_len = sizeof(struct d2d_sync_cmd);
    uint32_t cur_head = 0;

    if (put_cmd->die_idx >= CONFIG_RHEA_DIE_MAX) 
        return -EINVAL;

    pr_dbg("Write 0x%x bytes data with command %d to die%d\n",
                put_cmd->data_size, put_cmd->cmd_id, put_cmd->die_idx);

    ret = rhea_d2d_select_tile(put_cmd->die_idx, 
                    RHEA_AP_TILE_ID, 0);
    if (ret) {
        printf("Failed to select AP tile in die%d\n", 
                    put_cmd->die_idx);
        return ret;
    }
    pr_dbg("The AP tile in die%d is selected for synchronization\n",
                put_cmd->die_idx);

    cmd = (struct d2d_sync_cmd *) malloc(cmd_len);
    if (!cmd) {
        ret = -ENOMEM;
        goto release_tile;
    }
    memset(cmd, 0, cmd_len);
    cmd->cmd_idx = put_cmd->cmd_id;
    switch (cmd->cmd_idx) {
        case D2D_SYNC_DATA:
            if (!put_cmd->data_addr || !put_cmd->data_size) {
                ret = -EINVAL;
                goto free_cmd;
            }

            cmd->data_head = *sync_priv->rdata.tail;
            cmd->data_size = put_cmd->data_size;
            cmd->data_crc = crc32(0, put_cmd->data_addr, put_cmd->data_size);
            ret = d2d_ring_put_data_remote(&sync_priv->rdata, 
                                    put_cmd->data_addr, put_cmd->data_size);
            if (ret)
                goto free_cmd;
            break;
        case D2D_SYNC_WRITEL:
        case D2D_SYNC_READL:
        case D2D_SYNC_WRITEW:
        case D2D_SYNC_READW:
        case D2D_SYNC_WRITEB:
        case D2D_SYNC_READB:
            cur_head = *sync_priv->rcmd.tail;
            cmd->reg_addr = put_cmd->reg_addr;
            cmd->reg_val = put_cmd->reg_val;
            cmd->reg_own = 1;    // owned by the executor
            break;
        default:
            printf("Invalid command %d\n", cmd->cmd_idx);
            ret = -EINVAL;
            goto free_cmd;
    }
    cmd->cmd_crc = crc32(0, (uint8_t *) cmd, 
                        cmd_len - sizeof(uint32_t));
    ret = d2d_ring_put_data_remote(&sync_priv->rcmd, cmd, cmd_len);
    if (ret)
        goto free_cmd;

    pr_dbg("Put command to die%d (%08x %010lx %08x %08x %08x)\n",
                put_cmd->die_idx, cmd->cmd_idx, cmd->data_head, 
                cmd->data_size, cmd->data_crc, cmd->cmd_crc);

#if defined(CONFIG_RHEA_D2D_LOOKBACK)
    ret = cur_head;
#else
    ret = d2d_sync_wait_reg(put_cmd, cur_head);
#endif

free_cmd:
    free(cmd);
release_tile:
    rhea_d2d_release_tile();
    return ret;
}

int d2d_sync_query_cmd(enum d2d_sync_cmd_id cmd_id,
                                uint32_t *size_addr)
{
    struct d2d_sync_list *cmd_list;
    struct d2d_sync_cmd *cmd;

    if (cmd_id >= D2D_SYNC_CMD_MAX)
        return -EINVAL;

    pr_dbg("Try to query command %d\n", cmd_id);
    list_for_each(cmd_list, sync_priv->cmd_list) {
        cmd = &cmd_list->cmd;
        if (cmd->cmd_idx == cmd_id) {
            *size_addr = cmd->data_size;
            pr_dbg("Query command (%08x %010lx %08x %08x %08x)\n",
                        cmd->cmd_idx, cmd->data_head, cmd->data_size, 
                        cmd->data_crc, cmd->cmd_crc);
            return cmd->cmd_idx;
        }
    }
    return 0;
}

int d2d_sync_get_data(enum d2d_sync_cmd_id cmd_id,
                                void *data_addr)
{
    struct d2d_sync_list *cmd_list, *tmp;
    struct d2d_sync_cmd *cmd;
    struct list_head *pos, *next;

    if (cmd_id >= D2D_SYNC_CMD_MAX)
        return -EINVAL;

    pr_dbg("Try to get data carried by command %d\n", cmd_id);
    list_for_each_safe(cmd_list, sync_priv->cmd_list, tmp) {
        cmd = &cmd_list->cmd;
        if (cmd->cmd_idx == cmd_id) {
            if (!cmd_list->data)
                return -EFAULT;

            memcpy(data_addr, cmd_list->data, cmd->data_size);
            pr_dbg("Get the 0x%x bytes data carried by command %d\n",
                        cmd->data_size, cmd->cmd_idx);
            list_del(cmd_list);
            free(cmd_list->data);
            free(cmd_list);
            return cmd_id;
        }
    }
    return 0;
}

static int d2d_sync_obtain_data(struct d2d_sync_cmd *cmd)
{
    struct d2d_sync_list *cmd_list;
    uint32_t crc_val;
    uint8_t *data;
    int ret;

    if (!cmd->data_size)
        return -EFAULT;

    data = (uint8_t *) malloc(cmd->data_size);
    if (!data) {
        printf("Failed to allocate memory for cmd data\n");
        return -ENOMEM;
    }

    ret = d2d_ring_pop_data(&sync_priv->ldata, 
                            data, cmd->data_size);
    if (ret)
        goto free_data;

    crc_val = crc32(0, data, cmd->data_size);
    if (cmd->data_crc != crc_val) {
        printf("Data verification of command %d failed "
                "and %d bytes discarded\n",
                cmd->cmd_idx, cmd->data_size);
        goto free_data;
    }
    
    cmd_list = (struct d2d_sync_list *) 
                malloc(sizeof(struct d2d_sync_list));
    if (!cmd_list) {
        printf("Failed to allocate memory for cmd_list\n");
        ret = -ENOMEM;
        goto free_data;
    }
    memcpy(&cmd_list->cmd, cmd, sizeof(struct d2d_sync_cmd));
    cmd_list->data = data;
    cmd_list->size = cmd->data_size;
    list_add_tail(cmd_list, sync_priv->cmd_list);
    pr_dbg("Got 0x%x bytes of data in command %d\n",
            cmd_list->size, cmd_list->cmd.cmd_idx);

    return 0;

free_data:
    free(data);
    return ret;
}

static int d2d_sync_operation_reg(struct d2d_sync_cmd *cmd)
{
    int ret;

    if (!cmd->reg_own) {
        printf("Command %d not owned", cmd->cmd_idx);
        return -EFAULT;
    }

    switch(cmd->cmd_idx) {
        case D2D_SYNC_WRITEL:
            writel(cmd->reg_val, (void *) ((uintptr_t) cmd->reg_addr));
            break;
        case D2D_SYNC_READL:
            cmd->reg_val = readl((void *) ((uintptr_t) cmd->reg_addr));
            break;
        case D2D_SYNC_WRITEW:
            writew((uint16_t) cmd->reg_val, (void *) ((uintptr_t) cmd->reg_addr));
            break;
        case D2D_SYNC_READW:
            cmd->reg_val = readw((void *) ((uintptr_t) cmd->reg_addr));
            break;
        case D2D_SYNC_WRITEB:
            writeb((uint8_t) cmd->reg_val, (void *) ((uintptr_t) cmd->reg_addr));
            break;
        case D2D_SYNC_READB:
            cmd->reg_val = readb((void *) ((uintptr_t) cmd->reg_addr));
            break;
        default:
            printf("Invalid command %d\n", cmd->cmd_idx);
            return -EINVAL;
    }
    cmd->reg_own = 0;   // owned by the initiator
    pr_dbg("Command %d done (addr: 0x%010lx, val: 0x%08x)\n",
            cmd->cmd_idx, cmd->reg_addr, cmd->reg_val);
    return 0;
}

int d2d_sync_obtain_cmd(void)
{
    int ret;
    uint32_t used_len, crc_val, cmd_cnt = 0;
    struct d2d_sync_list *cmd_list;
    struct d2d_sync_cmd *cmd;
    const size_t cmd_len = sizeof(struct d2d_sync_cmd);

    used_len = d2d_ring_get_used_len(&sync_priv->lcmd);
    if (used_len) pr_dbg("Got 0x%x bytes in cmd buffer\n", used_len);
    while (used_len >= cmd_len) {
        cmd = (struct d2d_sync_cmd *) 
                ((uintptr_t) sync_priv->lcmd.local_addr + *sync_priv->lcmd.head);
        pr_dbg("Got command at 0x%p (%08x %010lx %08x %08x %08x), buffer length 0x%x\n",
                    cmd, cmd->cmd_idx, cmd->data_head, cmd->data_size, 
                    cmd->data_crc, cmd->cmd_crc, used_len);

        crc_val = crc32(0, (uint8_t *) cmd, cmd_len - sizeof(uint32_t));
        if (crc_val != cmd->cmd_crc) {
            printf("Command verification failed "
                    "(calculated: 0x%x, expected: 0x%x)\n",
                    crc_val, cmd->cmd_crc);
            d2d_ring_move_head(&sync_priv->lcmd, cmd_len);
            return -EIO;
        }

        switch (cmd->cmd_idx) {
            case D2D_SYNC_DATA:
                ret = d2d_sync_obtain_data(cmd);
                if (ret)
                    return ret;
                break;
            case D2D_SYNC_WRITEL:
            case D2D_SYNC_READL:
            case D2D_SYNC_WRITEW:
            case D2D_SYNC_READW:
            case D2D_SYNC_WRITEB:
            case D2D_SYNC_READB:
                ret = d2d_sync_operation_reg(cmd);
                if (ret)
                    return ret;
                break;
            default:
                printf("Invalid command %d\n",
                        cmd->cmd_idx);
                return -EFAULT;
        }
        d2d_ring_move_head(&sync_priv->lcmd, cmd_len);
        used_len -= cmd_len;
        cmd_cnt++;
    }
    if (used_len) {
        d2d_ring_move_head(&sync_priv->lcmd, used_len);
        printf("The remaining %d bytes in the command queue are discarded",
                used_len);
    }
    if (cmd_cnt) pr_dbg("A total of %d commands ware obtained\n", cmd_cnt);

    return cmd_cnt;
}

int rhea_d2d_sync_init(void)
{
	int ret;
    void *ioaddr;
    struct d2d_sync_header *header;

    ioaddr = rhea_d2d_get_dnoc_addr();
    if (ioaddr == NULL) {
        return -EFAULT;
    }

    sync_priv = malloc(sizeof(struct d2d_sync_priv));
    if (!sync_priv) {
        printf("Failed to allocate memory for device data\n");
        return -ENOMEM;
    }

    list_init(sync_priv->cmd_list);

    if (CONFIG_RHEA_D2D_SYNC_CMD_SIZE % sizeof(struct d2d_sync_cmd)) {
        printf("The command buffer must be a multiple of %ld\n",
                sizeof(struct d2d_sync_cmd));
	    free(sync_priv);
        return -EINVAL;
    }

    header = (struct d2d_sync_header *) (ioaddr +
                    CONFIG_RHEA_D2D_SYNC_HEADER_ADDR);
    sync_priv->rcmd.head = &header->cmd_head;
    sync_priv->rcmd.tail = &header->cmd_tail;
    sync_priv->rcmd.remote_addr = CONFIG_RHEA_D2D_SYNC_CMD_ADDR;
    sync_priv->rcmd.size = CONFIG_RHEA_D2D_SYNC_CMD_SIZE;
    sync_priv->rdata.head = &header->data_head;
    sync_priv->rdata.tail = &header->data_tail;
    sync_priv->rdata.remote_addr = CONFIG_RHEA_D2D_SYNC_DATA_ADDR;
    sync_priv->rdata.size = CONFIG_RHEA_D2D_SYNC_DATA_SIZE;

    header = (struct d2d_sync_header *) CONFIG_RHEA_D2D_SYNC_HEADER_ADDR;
    memset(header, 0, sizeof(struct d2d_sync_header));
    sync_priv->header = header;
    sync_priv->lcmd.head = &header->cmd_head;
    sync_priv->lcmd.tail = &header->cmd_tail;
    sync_priv->lcmd.local_addr = (void *) CONFIG_RHEA_D2D_SYNC_CMD_ADDR;
    sync_priv->lcmd.size = CONFIG_RHEA_D2D_SYNC_CMD_SIZE;
    sync_priv->ldata.head = &header->data_head;
    sync_priv->ldata.tail = &header->data_tail;
    sync_priv->ldata.local_addr = (void *) CONFIG_RHEA_D2D_SYNC_DATA_ADDR;
    sync_priv->ldata.size = CONFIG_RHEA_D2D_SYNC_DATA_SIZE;

	return 0;
}

void rhea_d2d_sync_exit(void)
{
    struct d2d_sync_list *cmd_list, *tmp;

    list_for_each_safe(cmd_list, sync_priv->cmd_list, tmp)
    {
        if (cmd_list->data)
            free(cmd_list->data);

        list_del(cmd_list); 
        free(cmd_list); 
    }

	if (sync_priv)
		free(sync_priv);
}
