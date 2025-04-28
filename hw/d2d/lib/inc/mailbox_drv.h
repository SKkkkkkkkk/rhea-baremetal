#ifndef __MAILBOX_DRV_H
#define __MAILBOX_DRV_H
#include <stdint.h>
#include "config.h"
#include "ring_buf.h"

#define MAILBOX_ROLE_MASTER 0
#define MAILBOX_ROLE_SLAVE  1
#define MAILBOX_ROLE_NUM    2 /*master and slave*/

#define MAILBOX_MASTER_REG_REV	CLCI_MCU_LOCAL_CTRL_DOORBELL_TO_SOC
#define MAILBOX_MASTER_REG_SEND CLCI_MCU_LOCAL_CTRL_DOORBELL_TO_MCU

#define MAILBOX_SLAVE_REG_REV  CLCI_MCU_LOCAL_CTRL_DOORBELL_TO_MCU
#define MAILBOX_SLAVE_REG_SEND CLCI_MCU_LOCAL_CTRL_DOORBELL_TO_SOC

#define MAILBOX_CMD_HANDLE_TIMEOUT_DEFALUT 500 //ms

#define MAILBOX_DATA_LEN_MAX (4 * 1024) /*4K*/

#define MAILBOX_CMD_OFFSET	   24
#define MAILBOX_CMD_TRANSPORT_SEND 0x1
#define MAILBOX_CMD_TRANSPORT_ACK  0x2
#define MAILBOX_CMD_REPORT_ERROR   0x05 //  report an error to the SOC

#define MAILBOX_DATA_MAX    32
#define MAILBOX_REV_BUFF    40
#define MAILBOX_PKG_LEN_MIN 3 //the pkg len is bigger or equal than 4 bytes

#define MAILBOX_PKG_HEADER_OFFSET 0 //BYTE[0]  1
#define MAILBOX_PKG_REPLY_OFFSET	  1 //BYTE[1]
#define MAILBOX_PKG_LEN_OFFSET	  2 //BYTE[2]
#define MAILBOX_PKG_DATA_OFFSET	  3

#define MAILBOX_PKG_CRC_OFFSET(len) (len + 2)

#define MAILBOX_PKG_CRC_DATA_LEN(data_len) (data_len + 1)

#define MAILBOX_PKG_HEADER_TO_MCU 0x5a
#define MAILBOX_PKG_HEADER_TO_SOC 0xa5

#define MAILBOX_PKG_BACK_MSG_BUFF_LEN 16

/*len of total package*/

#define DB_DATA_CMD(DB) ((((uint32_t)DB) >> MAILBOX_CMD_OFFSET) & 0xff)
#define DB_DATA_NUM(DB) (DB & 0xff)

#define DB_DATA_POP_TRANSE(len) ((MAILBOX_CMD_TRANSPORT_SEND << MAILBOX_CMD_OFFSET) | len)
#define DB_DATA_POP_ACK(data)	((MAILBOX_CMD_TRANSPORT_ACK << MAILBOX_CMD_OFFSET) | data)

#define DB_DATA_POP_DB(cmd, data) ((cmd << MAILBOX_CMD_OFFSET) | data)

#define DB_TRANS_ONE_FRAM_LEN 4 // 1 msg register (32bit)

#define MAILBOX_STATUS_NORMAL 1
#define MAILBOX_STATUS_ERROR  0

#define MAILBOX_MSG_NEED_REPLY    1
#define MAILBOX_MSG_N0_REPLY     0

typedef enum {
	MAILBOX_DB_STATUS_NONE = 0,
	MAILBOX_DB_STATUS_SEND = 1,
	MAILBOX_DB_STATUS_SEND_ACK = 2,
} e_mailbox_db_status;

typedef struct {
	uint64_t reg_doorbell_rev;
	uint64_t reg_doorbell_send;
	uint64_t reg_send;
	uint64_t reg_rev;
} mailbox_reg_st;

typedef struct {
	uint8_t role;
	uint8_t mailbox_status;
	mailbox_reg_st reg;
	ring_buf rev_buff;
	uint8_t *send_buff;
	volatile uint32_t send_total_Num;
	volatile uint32_t Send_cur_num;
	volatile e_mailbox_db_status send_status;
} mailbox_com_st;
/*cmd*/
typedef struct {
	uint8_t data_num;
	uint8_t data_state;
	uint8_t buff[MAILBOX_REV_BUFF];
} mailbox_pkg_st;

int32_t mailbox_rev(uint8_t *out_buff, uint8_t len, uint8_t *replay, uint32_t timeout_ms);
int32_t mailbox_send(uint8_t *in_buff, uint8_t len, uint8_t reply, uint32_t timeout_ms);
void mailbox_reg_address_refresh(void);
int32_t mailbox_init(uint8_t role);

#endif
