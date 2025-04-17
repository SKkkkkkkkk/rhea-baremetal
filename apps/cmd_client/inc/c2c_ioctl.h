#ifndef __C2C_IOCTL__H
#define __C2C_IOCTL__H

// #include <linux/types.h>
#include <stdint.h>
#pragma pack(1)

enum c2c_tile_state {
	CLINK_WORKING = 0,
	CLINK_SLEEP = 1,
	CLINK_IRQ_FAIL = -1,
};

struct c2c_tile_cfg_io {
	uint8_t  local_tile_id; /* 02 03 72 73 */
	uint8_t  lane_min;
	uint8_t  lane_max;
	uint8_t  rate; /* enum c2c_link_rate  */
	uint8_t  bif_en; /* enum c2c_bit_onoff x8 x16 */
	uint8_t  mode; /* rc or ep enum c2c_mode*/
	uint64_t bdf; /* pcie bus id */
};

struct c2c_tile_link_ctr {
	uint8_t  tile_id; /* 02 03 72 73 */
	uint8_t  ctl_index; // 0 or 1
	uint8_t  onoff;
};

struct c2c_tile_link_setup {
	uint8_t  tile_id; /* 02 03 72 73 */
	uint8_t  ctl_index;
	uint8_t  msi_capable;
};
struct c2c_tile_speed {
	uint8_t tile_id;
	uint8_t tile_mode; //ep or rc
	uint8_t rate;
	uint8_t lane_min; //uer lane
	uint8_t lane_max;
};

struct c2c_tile_link_state {
	uint8_t tile_id;
	uint8_t link_status;
};

struct c2c_recv_buffer_io {
	uint8_t tile_id;
	uint32_t c2c_buffer_size;
	uint64_t buffer_addr;
};

#pragma pack()

#define A510_C2C_IOCTL			 (0x12U)
#define A510_C2C_TILE_INIT		 _IOW(A510_C2C_IOCTL, 0x01, struct c2c_tile_cfg)
#define A510_C2C_TILE_LINK_CTL	 _IOW(A510_C2C_IOCTL, 0x02, struct c2c_tile_link_ctr)
#define A510_C2C_TILE_LINK_SETUP _IOW(A510_C2C_IOCTL, 0x04, struct c2c_tile_link_setup)
#define A510_C2C_TILE_DEBUG      _IOW(A510_C2C_IOCTL, 0x05, struct c2c_tile_link_setup)
#define A510_C2C_TILE_GET_SPEED  _IOWR(A510_C2C_IOCTL, 0x03, struct c2c_tile_speed)
#define A510_C2C_TILE_GET_STATUS _IOWR(A510_C2C_IOCTL, 0x03, struct c2c_tile_link_state)
#define A510_C2C_TILE_GET_BUFFER _IOWR(A510_C2C_IOCTL, 0x03, struct c2c_recv_buffer)

int c2c_ioctl_fd_init( uint8_t tile_id, uint8_t ctl_id );
int c2c_ioctl_link_step( int fd, struct c2c_tile_link_setup *setup);
int c2c_ioctl_cfg(int fd, struct c2c_tile_cfg_io *tile_cfg);
int c2c_ioctl_link_ctl(int fd, struct c2c_tile_link_ctr *link);
int c2c_ioctl_get_speed(int fd, struct c2c_tile_speed *speed);
int c2c_ioctl_get_link_status(int fd, struct c2c_tile_link_state *status);
int c2c_ioctl_get_buffer(int fd, struct c2c_recv_buffer_io *buffer);
int c2c_delinit(int fd);

#endif