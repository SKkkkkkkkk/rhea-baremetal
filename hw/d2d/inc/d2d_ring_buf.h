#ifndef __D2D_RING_BUF_H__
#define __D2D_RING_BUF_H__

#include <stdint.h>

struct d2d_ring_buf {
    union {
        void *local_addr;
        uint64_t remote_addr;
    };
    uint32_t size;
    uint32_t *head;
    uint32_t *tail;
    uint8_t die_idx;

    int (*get_lock)(uint8_t die_idx);
    void (*clear_lock)(uint8_t die_idx);
};

uint32_t d2d_ring_get_avail_len(struct d2d_ring_buf *ring_buf);
uint32_t d2d_ring_get_used_len(struct d2d_ring_buf *ring_buf);
int d2d_ring_move_head(struct d2d_ring_buf *ring_buf,
                                uint32_t offset);
int d2d_ring_move_tail_remote(struct d2d_ring_buf *ring_buf,
                                uint32_t offset);
int d2d_ring_put_data_remote(struct d2d_ring_buf *ring_buf,
                        void *data, uint32_t size);
/**
 * Take out data of specified length from the local ring buffer
 *
 * Note: Taking out data will not release the corresponding space 
 * in the ring buffer, you need to use d2d_ring_move_head() to 
 * release it manually or use d2d_ring_pop_data() directly
 */
int d2d_ring_get_data(struct d2d_ring_buf *ring_buf,
                        void *data, uint32_t size);
int d2d_ring_pop_data(struct d2d_ring_buf *ring_buf,
                        void *data, uint32_t size);

#endif /* __D2D_RING_BUF_H__ */