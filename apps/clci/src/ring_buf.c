#include <string.h>
#include <ring_buf.h>

int ring_buf_size(ring_buf *rb)
{
	return (rb->write >= rb->read) ? rb->write - rb->read : RING_BUFF_LEN - rb->read + rb->write;
}

int ring_buf_full(ring_buf *rb)
{
	return ((rb->read + 1) % RING_BUFF_LEN) == rb->write;
}

int ring_buf_empty(ring_buf *rb)
{
	return rb->read == rb->write;
}

int ring_buf_put(ring_buf *rb, uint8_t cha)
{
	uint8_t size = (rb->write >= rb->read) ? rb->write - rb->read : RING_BUFF_LEN - rb->read + rb->write;

	if (size < RING_BUFF_LEN) {
		rb->buff[rb->write] = cha;
		rb->write = (rb->write + 1) % RING_BUFF_LEN;
		return 1;
	} else {
		rb->discard++;
		return 0;
	}
}

int ring_buf_get(ring_buf *rb, uint8_t *out_var)
{
	if (rb->write == rb->read)
		return 0;

	*out_var = rb->buff[rb->read];

	rb->read = (rb->read + 1) % RING_BUFF_LEN;

	return 1;
}

void ring_buf_init(ring_buf *rb)
{
	memset(rb, 0x00, sizeof(ring_buf));
}
