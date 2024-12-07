/*
	provide a set of APIs to interact with firmware,
	SYS_CLCI_CTRL_MSG is MSG3 register which is used as a control/status register.

	MSG0-2 is data register for swap
		MSG0	:	[0:7]
		MSG1	:	[8:15]
		MSG2	:	[16:23]

	MSG3 is controll register
		bit31:24	: command type
		bit23:6		: reserved
		bit5		: sys-mcu request bit
		bit4		: clci-mcu response bit
		bit3:0		: clci status
*/
#include <commands_common.h>
#include <mmio.h>

void clci_set_resp()
{
	mmio_write_32(SYS_CLCI_CTRL_MSG, mmio_read_32(SYS_CLCI_CTRL_MSG) | (1 << 4));
}

void clci_clr_resp()
{
	mmio_write_32(SYS_CLCI_CTRL_MSG, mmio_read_32(SYS_CLCI_CTRL_MSG) & (~(1 << 4)));
}

void clci_set_req()
{
	mmio_write_32(SYS_CLCI_CTRL_MSG, mmio_read_32(SYS_CLCI_CTRL_MSG) | (1 << 5));
}

int clci_get_req()
{
	return (mmio_read_32(SYS_CLCI_CTRL_MSG) >> 5) & 0x1;
}

void clci_clr_req()
{
	mmio_write_32(SYS_CLCI_CTRL_MSG, (mmio_read_32(SYS_CLCI_CTRL_MSG) & (~(1 << 5))));
}

void clci_set_cmd(uint8_t cmd)
{
	mmio_write_32(SYS_CLCI_CTRL_MSG, (mmio_read_32(SYS_CLCI_CTRL_MSG) & (~(0xff << 24))) | (cmd << 24));
}

uint32_t clci_get_cmd()
{
	return (mmio_read_32(SYS_CLCI_CTRL_MSG) >> 24) & 0xff;
}

int clci_is_resp_busy()
{
	return (mmio_read_32(SYS_CLCI_CTRL_MSG) >> 4) & 0x1;
}

uint32_t clci_req(uint8_t cmd)
{
	clci_set_cmd(cmd);
	clci_set_req();
	while (clci_is_resp_busy() == 0)
		;
	clci_clr_resp();
	return clci_get_cmd();
}

void clci_resp(uint8_t cmd)
{
	clci_set_cmd(cmd);
	clci_set_resp();
}

void clci_set_status(int status)
{
	mmio_write_32(SYS_CLCI_CTRL_MSG, (mmio_read_32(SYS_CLCI_CTRL_MSG) & 0xfffffff0) | (status & 0xf));
}
