#include <errno.h>
#include <string.h>

#include "d2d_ring_buf.h"
#include "d2d_api.h"

uint32_t d2d_ring_get_avail_len(struct d2d_ring_buf *ring_buf)
{
    int64_t avail = (int64_t) *ring_buf->head - *ring_buf->tail;
    return avail > 0 ? avail : ring_buf->size + avail;
}

uint32_t d2d_ring_get_used_len(struct d2d_ring_buf *ring_buf)
{
    int64_t used = (int64_t) *ring_buf->tail - *ring_buf->head;
    return used < 0 ? ring_buf->size + used : used;
}

inline uint32_t d2d_ring_calc_pos(uint32_t pos, uint32_t size, uint32_t offset)
{
    uint32_t end_size = size - pos;
    if (end_size <= offset)
        return offset - end_size;
    else
        return pos + offset;
}

int d2d_ring_move_head(struct d2d_ring_buf *ring_buf,
                                uint32_t offset)
{
    if (!ring_buf)
        return -EFAULT;

    if (d2d_ring_get_used_len(ring_buf) < offset)
        return -ENOMEM;

    *ring_buf->head = 
            d2d_ring_calc_pos(*ring_buf->head, ring_buf->size, offset);
    return 0;
}

int d2d_ring_move_tail_remote(struct d2d_ring_buf *ring_buf,
                                uint32_t offset)
{
    if (!ring_buf)
        return -EFAULT;

    /**
     * When adding data, if the tail overlaps with the head, 
     * it will be impossible to distinguish whether the buffer
     * is empty or full
     */
    if (d2d_ring_get_avail_len(ring_buf) <= offset)
        return -ENOMEM;

    *ring_buf->tail = 
            d2d_ring_calc_pos(*ring_buf->tail, ring_buf->size, offset);
    return 0;
}

int d2d_ring_put_data_remote(struct d2d_ring_buf *ring_buf,
                        void *data, uint32_t size)
{
    uint32_t end_size;
    int ret;

    if (!data || !ring_buf || !ring_buf->remote_addr ||
        !ring_buf->head || !ring_buf->tail)
        return -EFAULT;

    if (d2d_ring_get_avail_len(ring_buf) <= size)
        return -ENOMEM;

    end_size = ring_buf->size - *ring_buf->tail;
    if (end_size <= size) {
        ret = rhea_d2d_write_data(data, 
                    ring_buf->remote_addr + *ring_buf->tail, end_size);
        if (ret)
            return ret;
        ret = d2d_ring_move_tail_remote(ring_buf, end_size);
        if (ret)
            return ret;
        data += end_size;
        size -= end_size;
    }

    ret = rhea_d2d_write_data(data, 
                    ring_buf->remote_addr + *ring_buf->tail, size);
    if (ret)
        return ret;

    return d2d_ring_move_tail_remote(ring_buf, size);
}

int d2d_ring_get_data(struct d2d_ring_buf *ring_buf,
                        void *data, uint32_t size)
{
    uint32_t end_size, head;

    if (!data || !ring_buf || !ring_buf->local_addr ||
        !ring_buf->head || !ring_buf->tail)
        return -EFAULT;

    if (d2d_ring_get_used_len(ring_buf) < size)
        return -ENOMEM;

    head = *ring_buf->head;
    end_size = ring_buf->size - head;
    if (end_size <= size) {
        memcpy(data, ring_buf->local_addr + head, end_size);
        data += end_size;
        size -= end_size;
        head = 0;
    }
    memcpy(data, ring_buf->local_addr + head, size);

    return 0;
}

int d2d_ring_pop_data(struct d2d_ring_buf *ring_buf,
                        void *data, uint32_t size)
{
    int ret;

    ret = d2d_ring_get_data(ring_buf, data, size);
    if (ret)
        return ret;
    return d2d_ring_move_head(ring_buf, size);
}
