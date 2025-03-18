#ifndef __A5XX_ALINK__H
#define __A5XX_ALINK__H

#include <stdint.h>

#define LOGTAG 							"clink"

#define C2C_GPU_NUMBER					(8U)
#define C2C_TILE_NUMBER					(16U)
#define C2C_BOARD_NUMBER				(1U)
#define C2C_EANBLE_DELAY_MS				(50U)
#define C2C_RETRY_NUMBER				(5U)

#define CLINKCHECKPOINT( Call ) 		do { \
											void *pcall = Call; \
											if( pcall == NULL ) { \
												LOGE("%s:%d -> %p \r\n", __FILE__, __LINE__, pcall); \
												return NULL;\
											} \
										}while(0)
#define CLINKCHECKINT( Call ) 		do { \
											int ret = Call; \
											if( ret != 0 ) { \
												LOGE("%s:%d -> %d \r\n", __FILE__, __LINE__, ret); \
												return ret;\
											} \
										}while(0)
#define CLINKCHECKPOINTINT( Call ) 		do { \
											if( Call == NULL ) { \
												LOGE("%s:%d -> %p \r\n", __FILE__, __LINE__, Call); \
												return C2C_FAIL;\
											} \
										}while(0)
#define CLINKINT( Call ) 		do { \
											if( Call < 0 ) { \
												LOGE("%s:%d -> %d \r\n", __FILE__, __LINE__, Call); \
												return C2C_FAIL;\
											} \
										}while(0)
#define CLINKCHECKBOOL( Call ) 		do { \
											int ret = Call; \
											if( !ret ) { \
												LOGE("%s:%d -> %d \r\n", __FILE__, __LINE__, ret); \
												return ret;\
											} \
										}while(0)

#pragma pack(1)
enum c2c_cmd_ctr {
	C2C_CONFIG = 0,
	C2C_LINK_ENABLE,
	C2C_LINK_SETUP,
	C2C_LINK_CHECK,
	C2C_LINK_DISABLE,
	C2C_RELINK,
	C2C_BUFFER,
	C2C_SPEED,
	C2C_RESPONSE, //return state
	C2C_SETUP_STATE,
	C2C_TEST_QUEUE_DELAY,
};

enum c2c_cmd_state {
	C2C_UNKNOWN = 9,
	C2C_PREPARE,
	C2C_CONFIG_DONE,
	C2C_LINK_DOWN,
	C2C_LINK_ON,
	C2C_LINK_WORKING,
	C2C_RETRY
};

enum c2c_err {
	C2C_SUCCEED = 0,
	C2C_FAIL = -1,
	C2C_CFG_INITED = -2,
	C2C_STATE_EXCEPTION = -3,
};

enum c2c_lane_number {
	C2C_LINK_LANE1 = 1,
	C2C_LINK_LANE2 = 2,
	C2C_LINK_LANE4 = 4,
	C2C_LINK_LANE8 = 8,
	C2C_LINK_LANE16 = 16
};

enum c2c_link_rate {
	C2C_LINK_RATE_GEN1 = 1,
	C2C_LINK_RATE_GEN2,
	C2C_LINK_RATE_GEN3,
	C2C_LINK_RATE_GEN4,
	C2C_LINK_RATE_GEN5,
};

enum c2c_pcie_mode {
	C2C_RC_MODE = 0,
	C2C_EP_MODE,
};

enum c2c_bif_onoff {
	C2C_BIF_OFF = 0,
	C2C_BIF_ON
};

enum c2c_link_onoff {
	CLINK_LINK_OFF = 0,
	CLINK_LINK_ON
};

enum c2c_trainfer_dir {
	C2C_HOST_TO_DEV = 0,
	C2C_DEV_TO_HOST,
};

enum c2c_tile_id {
	C2C_TILE_ID0 = 2,
	C2C_TILE_ID1 = 3,
	C2C_TILE_ID2 = 72,
	C2C_TILE_ID3 = 73
};

struct c2c_buffer_info {
	uint8_t die;
	uint8_t tile_id;
	uint8_t ctr_index;
	uint32_t c2c_buffer_size;
	uint64_t buffer_addr;
};

struct c2c_link_state {
	uint8_t  enable;
	uint32_t remote_id;
	uint8_t	 remote_die;
	uint8_t  remote_tile_id;
	uint8_t  ctr_index;
};

struct c2c_tile_cfg {
	uint8_t  die;
	uint8_t  local_tile_id; /* 02 03 72 73 */
	uint8_t  ctr_index;
	uint8_t  lane_min;
	uint8_t  lane_max;
	uint8_t  rate; /* enum c2c_link_rate  */
	uint8_t  bif_en; /* enum c2c_bit_onoff x8 x16 */
	uint8_t  mode; /* rc or ep enum c2c_mode*/
	uint64_t bdf; /* pcie bus id */
	uint8_t  msi_capable;
	struct c2c_link_state remote_state;
	uint64_t delaytime; /* host to board time */
};

struct c2c_param {
	uint8_t  die;
	uint8_t  tile_id; /* 02 03 72 73 */
	uint32_t clk;
};

struct c2c_link_check {
	uint8_t die;
	uint8_t tile_id;
	uint8_t device_id;
	uint8_t ctr_index;
	uint8_t remote_device_id;
	uint8_t remote_ctr_index;
	uint8_t link_state;
};

struct c2c_link_speed {
	uint8_t die;
	uint8_t tile_id;
	uint8_t ctr_index;
	uint8_t link_rate;
	uint8_t state;
};

struct c2c_link_ctr {
	uint8_t die;
	uint8_t tile_id; //current tile_id = 0, colse 2 3 72 73 tile link
	uint8_t ctr_index;
	uint8_t onoff;
	uint8_t msi_capable;
};

/*
	c2c 不通的tile可以单独控制lane，但是rc必须对应ep
*/
struct c2c_pcie_config {
	uint32_t device_id;
	uint32_t die;
	uint32_t npu_logic_id;
	struct c2c_tile_cfg tile_cfg[C2C_TILE_NUMBER]; 	/* 02 03 72 73 */
};

struct c2c_recv_send_state {
	uint8_t die;
	uint8_t tile_id;
	uint8_t ctr_index;
	uint8_t state;
};

/* 8 Byte alignment */
struct c2c_trainfer {
	uint8_t c2c_cmd;
	uint8_t dir; /* enum c2c_trainfer_dir */
	uint32_t timeout; /* ms */
	union {
		struct c2c_tile_cfg c2c_pcie_cfg;	/* c2c pcie config  */
		struct c2c_buffer_info c2c_buffer;   /* get buffer info */
		struct c2c_link_speed c2c_speed;	/* get/set c2c speed */
		struct c2c_param param;			/* param */
		struct c2c_link_ctr ctr_link;	/* link enable and disable */
		struct c2c_recv_send_state state; /* work status */
	}msg;
};

struct c2c_recv_speed {
	uint8_t rate;
	uint8_t lane_mode;
	uint8_t lane_min;
	uint8_t lane_max;
};

struct c2c_recv_buffer {
	uint32_t c2c_buffer_size;
	uint64_t buffer_addr;
};

struct c2c_check_link {
	uint8_t link_state;
};

struct c2c_receive {
	uint8_t die;
	uint8_t tile;
	uint8_t cmd;
	int8_t state;
	union {
		struct c2c_recv_speed rate;
		struct c2c_check_link link_state;
		struct c2c_recv_buffer buffer;
	}rcv_msg;
};

struct c2c_comm_die {
	uint32_t dev_index;
	struct c2c_trainfer cfg_msg;
	uint32_t crc;
	uint8_t release[9];
};
#pragma pack()
#endif
