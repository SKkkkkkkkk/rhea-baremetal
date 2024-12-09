#ifndef RING_BUFF_H
#define RING_BUFF_H

#include <stdint.h>

#define RING_BUFF_LEN 64

typedef struct {
	uint8_t buff[RING_BUFF_LEN];
	uint8_t write;
	uint8_t read;
	uint8_t discard;
} ring_buf;

int ring_buf_size(ring_buf *rb);
int ring_buf_full(ring_buf *rb);
int ring_buf_empty(ring_buf *rb);
int ring_buf_put(ring_buf *rb, uint8_t cha);
int ring_buf_get(ring_buf *rb, uint8_t *out_var);
void ring_buf_init(ring_buf *rb);

#endif
