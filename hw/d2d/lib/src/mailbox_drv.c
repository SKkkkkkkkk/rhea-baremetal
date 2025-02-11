#include <config.h>
#include <string.h>
#include <stdio.h>
#include <log.h>
#include <mmio.h>
#include <mailbox_drv.h>
#include <doorbell.h>
#include <local_ctrl.h>

#include "clci_errno.h"
#include "delay.h"

mailbox_com_st maiblbox_blk;
mailbox_pkg_st data_pkg;
static int mailbox_trans_db_data_rev(uint32_t db)
{
	uint32_t data;
	uint32_t i;
	uint8_t *p_char = (uint8_t *)&data;
	uint8_t num = DB_DATA_NUM(db);

	/* 4 message register, 16 bytes */
	if ((num > DB_TRANS_ONE_FRAM_LEN) || (num <= 0))
		return CLCI_E_DATA;

	/*1 msg register*/
	data = mmio_read_32(maiblbox_blk.reg.reg_rev);
	for (i = 0; i < num; i++) {
		ring_buf_put(&(maiblbox_blk.rev_buff), *(p_char + i));
	}
	return CLCI_SUCCESS;
}
inline static void mailbox_db_trans_status_set(e_mailbox_db_status status)
{
	maiblbox_blk.send_status = status;
}
static int mailbox_trans_get_db(uint32_t timeout_ms)
{
	uint32_t time_count = 0;

	while (db_is_using(maiblbox_blk.reg.reg_doorbell_send)) {
		udelay(10);
		time_count++;
		if (time_count > timeout_ms * 100) {
			ERROR(mcu_com, "Get db timeout\n");
			return CLCI_E_TIMEOUT;
		}
	}
	return CLCI_SUCCESS;
}

static int mailbox_trans_ack_set(uint32_t timeout_ms)
{
	uint32_t db_data = DB_DATA_POP_ACK(0);

	if (mailbox_trans_get_db(timeout_ms) == CLCI_E_TIMEOUT) {
		ERROR(mcu_com, "get to-soc db timeout\n");
		return CLCI_E_TIMEOUT;
	}
	db_generate_int(maiblbox_blk.reg.reg_doorbell_send, db_data);

	return CLCI_SUCCESS;
}

#if DOORBELL_MODE_ISR == 1
void mailbox_doorbell_isr()
{
	uint32_t db_data;

	db_data = mmio_read_32(maiblbox_blk.reg.reg_doorbell_rev);

	if (DB_DATA_CMD(db_data) == MAILBOX_CMD_TRANSPORT_SEND) {
		if (mailbox_trans_db_data_rev(db_data) == CLCI_SUCCESS)
			mailbox_trans_ack_set(MAILBOX_CMD_HANDLE_TIMEOUT_DEFALUT);
	} else if (DB_DATA_CMD(db_data) == MAILBOX_CMD_TRANSPORT_ACK) {
		mailbox_db_trans_status_set(MAILBOX_DB_STATUS_SEND_ACK);
	}
	db_clear_ext_interrupt(maiblbox_blk.reg.reg_doorbell_rev);
}
#else
void mailbox_doorbell_poll(void)
{
	uint32_t db_data;

	db_data = mmio_read_32(maiblbox_blk.reg.reg_doorbell_rev);

	if (db_data == 0)
		return;

	if (DB_DATA_CMD(db_data) == MAILBOX_CMD_TRANSPORT_SEND) {
		if (mailbox_trans_db_data_rev(db_data) == CLCI_SUCCESS)
			mailbox_trans_ack_set(MAILBOX_CMD_HANDLE_TIMEOUT_DEFALUT);
	} else if (DB_DATA_CMD(db_data) == MAILBOX_CMD_TRANSPORT_ACK) {
		mailbox_db_trans_status_set(MAILBOX_DB_STATUS_SEND_ACK);
	}

	mmio_write_32(maiblbox_blk.reg.reg_doorbell_rev, 0);
}
#endif
static int mailbox_pkg_data_check(uint8_t *pkg_buff, uint8_t len)
{
	uint8_t payload_len;
	uint8_t payload_header;

	/*data is too short*/
	if (len < MAILBOX_PKG_LEN_MIN)
		return CLCI_E_SIZE;

	payload_len = *(pkg_buff + MAILBOX_PKG_LEN_OFFSET);

	/*data is too short*/
	if (len < payload_len + 2)
		return CLCI_E_SIZE;

	/*data is too long*/
	if (len > payload_len + 2)
		return CLCI_E_DATA;

	if (maiblbox_blk.role == MAILBOX_ROLE_MASTER)
		payload_header = MAILBOX_PKG_HEADER_TO_SOC;
	else
		payload_header = MAILBOX_PKG_HEADER_TO_MCU;

	/*data header error*/
	if (*(pkg_buff + MAILBOX_PKG_HEADER_OFFSET) != payload_header) {
		return CLCI_E_DATA;
	}
	return CLCI_SUCCESS;
}

/*poll the mcu transe buffer*/
int mailbox_buff_polling(void)
{
	uint8_t char_data;
	int check_res;

#if DOORBELL_MODE_ISR == 0
	mailbox_doorbell_poll();
#endif
	if (ring_buf_empty(&(maiblbox_blk.rev_buff)))
		return CLCI_E_SIZE;

	ring_buf_get(&(maiblbox_blk.rev_buff), &char_data);

	data_pkg.buff[data_pkg.data_num] = char_data;
	data_pkg.data_num++;

	check_res = mailbox_pkg_data_check(data_pkg.buff, data_pkg.data_num);

	return check_res;
}
static int mailbox_db_trans_wait(uint32_t time_out_ms)
{
	uint32_t time_count = 0;

#if DOORBELL_MODE_ISR == 0
	mailbox_doorbell_poll();
#endif
	while (maiblbox_blk.send_status != MAILBOX_DB_STATUS_SEND_ACK) {
#if DOORBELL_MODE_ISR == 0
		mailbox_doorbell_poll();
#endif
		udelay(10);
		time_count++;
		if (time_count > time_out_ms * 100) {
			ERROR(mailbox, "trans timeout\n");
			return CLCI_E_TIMEOUT;
		}
	}
	return CLCI_SUCCESS;
}

static int mailbox_db_trans_start(uint8_t len, uint32_t time_out_ms)
{
	uint32_t out_db_data = DB_DATA_POP_TRANSE(len);

	mailbox_db_trans_status_set(MAILBOX_DB_STATUS_SEND);

	if (mailbox_trans_get_db(time_out_ms) == CLCI_E_TIMEOUT)
		return CLCI_E_TIMEOUT;

	db_generate_int(maiblbox_blk.reg.reg_doorbell_send, out_db_data);

	return CLCI_SUCCESS;
}
/*populate the data package*/
static int32_t mailbox_pop_send_pkg(uint8_t *data_buff, uint8_t data_len, uint8_t *pkg_buff)
{
	if (maiblbox_blk.role == MAILBOX_ROLE_MASTER)
		pkg_buff[MAILBOX_PKG_HEADER_OFFSET] = MAILBOX_PKG_HEADER_TO_MCU;
	else
		pkg_buff[MAILBOX_PKG_HEADER_OFFSET] = MAILBOX_PKG_HEADER_TO_SOC;

	pkg_buff[MAILBOX_PKG_LEN_OFFSET] = data_len;
	memcpy(&pkg_buff[MAILBOX_PKG_LEN_OFFSET + 1], data_buff, data_len);

	return (data_len + 2);
}

int32_t mailbox_rev(uint8_t *out_buff, uint8_t len, uint32_t timeout_ms)
{
	int res;
	uint32_t content_len = 0;
	uint32_t time_count = 0;

	if (maiblbox_blk.mailbox_status == MAILBOX_STATUS_ERROR)
		return CLCI_E_DEVICE;

	res = mailbox_buff_polling();

	if (res == CLCI_E_DATA) {
		maiblbox_blk.mailbox_status = MAILBOX_STATUS_ERROR;
		return CLCI_E_DEVICE;
	} else if (res == CLCI_E_SIZE) {
		if (timeout_ms == 0)
			return CLCI_E_SIZE;

		while (res == CLCI_E_SIZE) {
			time_count += 10;
			if (time_count >= timeout_ms * 1000) {
				ERROR(mailbox, "db rev data timeout\n");
				return CLCI_E_TIMEOUT;
			}
			udelay(10);
			res = mailbox_buff_polling();

			if (res == CLCI_SUCCESS)
				break;

			if (res == CLCI_E_DATA) {
				maiblbox_blk.mailbox_status = MAILBOX_STATUS_ERROR;
				return CLCI_E_DEVICE;
			}
		}
	}

	content_len = data_pkg.buff[MAILBOX_PKG_LEN_OFFSET];
	if (content_len > len) {
		res = len;
	} else {
		res = content_len;
	}

	memcpy(out_buff, &data_pkg.buff[MAILBOX_PKG_DATA_OFFSET], res);
	memset(&data_pkg, 0x00, sizeof(mailbox_pkg_st));

	return res;
}

int32_t mailbox_send(uint8_t *in_buff, uint8_t len, uint32_t timeout_ms)
{
	int32_t i_pos = 0;
	int32_t res = CLCI_SUCCESS;
	uint32_t send_data = 0;
	int32_t remain_len = len;
	uint8_t send_buff[MAILBOX_DATA_MAX + 2] = { 0 };

	if (maiblbox_blk.mailbox_status == MAILBOX_STATUS_ERROR)
		return CLCI_E_DEVICE;

	remain_len = mailbox_pop_send_pkg(in_buff, len, send_buff);

	while (remain_len > DB_TRANS_ONE_FRAM_LEN) {
		memcpy(&send_data, (send_buff + i_pos), sizeof(uint32_t));
		mmio_write_32(maiblbox_blk.reg.reg_send, send_data);
		i_pos += sizeof(uint32_t);
		remain_len = remain_len - DB_TRANS_ONE_FRAM_LEN;
		if (mailbox_db_trans_start(DB_TRANS_ONE_FRAM_LEN, timeout_ms) != CLCI_SUCCESS) {
			res = CLCI_E_TIMEOUT;
			goto SEND_ERR;
		}
		if (mailbox_db_trans_wait(timeout_ms) != CLCI_SUCCESS) {
			res = CLCI_E_TIMEOUT;
			goto SEND_ERR;
		}
	}

	if (remain_len > 0) {
		memcpy(&send_data, (send_buff + i_pos), remain_len);
		mmio_write_32(maiblbox_blk.reg.reg_send, send_data);
		if (mailbox_db_trans_start(remain_len, timeout_ms) != CLCI_SUCCESS) {
			res = CLCI_E_TIMEOUT;
			goto SEND_ERR;
		}
		if (mailbox_db_trans_wait(timeout_ms) != CLCI_SUCCESS) {
			res = CLCI_E_TIMEOUT;
			goto SEND_ERR;
		}
	}

	return res;

SEND_ERR:
	maiblbox_blk.mailbox_status = MAILBOX_STATUS_ERROR;

	ERROR(mailbox, "db communication fail\n");

	return res;
}

int32_t mailbox_init(uint8_t role)
{
	if (role > MAILBOX_ROLE_SLAVE)
		return CLCI_E_PARAM;

	memset(&maiblbox_blk, 0x00, sizeof(mailbox_com_st));
	memset(&data_pkg, 0x00, sizeof(mailbox_pkg_st));
	maiblbox_blk.role = role;
	maiblbox_blk.mailbox_status = MAILBOX_STATUS_NORMAL;
	if (role == MAILBOX_ROLE_MASTER) {
		maiblbox_blk.reg.reg_doorbell_rev = CLCI_MCU_LOCAL_CTRL_DOORBELL_TO_SOC;
		maiblbox_blk.reg.reg_doorbell_send = CLCI_MCU_LOCAL_CTRL_DOORBELL_TO_MCU;

		maiblbox_blk.reg.reg_send = CLCI_MCU_LOCAL_CTRL_MSG2;
		maiblbox_blk.reg.reg_rev = CLCI_MCU_LOCAL_CTRL_MSG0;
	} else {
		maiblbox_blk.reg.reg_doorbell_rev = CLCI_MCU_LOCAL_CTRL_DOORBELL_TO_MCU;
		maiblbox_blk.reg.reg_doorbell_send = CLCI_MCU_LOCAL_CTRL_DOORBELL_TO_SOC;

		maiblbox_blk.reg.reg_send = CLCI_MCU_LOCAL_CTRL_MSG0;
		maiblbox_blk.reg.reg_rev = CLCI_MCU_LOCAL_CTRL_MSG2;
	}
#if DOORBELL_MODE_ISR == 1
	db_init(maiblbox_blk.reg.reg_doorbell_rev, mailbox_doorbell_isr);
#endif

	return CLCI_SUCCESS;
}

void mailbox_dev_addr_refresh()
{
	maiblbox_blk.reg.reg_doorbell_rev = CLCI_MCU_LOCAL_CTRL_DOORBELL_TO_SOC;
	maiblbox_blk.reg.reg_doorbell_send = CLCI_MCU_LOCAL_CTRL_DOORBELL_TO_MCU;

	maiblbox_blk.reg.reg_send = CLCI_MCU_LOCAL_CTRL_MSG2;
	maiblbox_blk.reg.reg_rev = CLCI_MCU_LOCAL_CTRL_MSG0;
}
