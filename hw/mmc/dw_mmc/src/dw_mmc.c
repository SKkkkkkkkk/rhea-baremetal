/*
 * Copyright (c) 2016-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "dw_mmc.h"
#include "arch_helpers.h"
#include "delay.h"
#include "dw_apb_gpio.h"
#include "memmap.h"

#define DWMMC_CTRL (0x00)
#define CTRL_IDMAC_EN (1 << 25)
#define CTRL_DMA_EN (1 << 5)
#define CTRL_INT_EN (1 << 4)
#define CTRL_DMA_RESET (1 << 2)
#define CTRL_FIFO_RESET (1 << 1)
#define CTRL_RESET (1 << 0)
#define CTRL_RESET_ALL (CTRL_DMA_RESET | CTRL_FIFO_RESET | CTRL_RESET)

#define DWMMC_PWREN (0x04)
#define DWMMC_CLKDIV (0x08)
#define DWMMC_CLKSRC (0x0c)
#define DWMMC_CLKENA (0x10)
#define DWMMC_TMOUT (0x14)
#define DWMMC_CTYPE (0x18)
#define CTYPE_8BIT (1 << 16)
#define CTYPE_4BIT (1)
#define CTYPE_1BIT (0)

#define DWMMC_BLKSIZ (0x1c)
#define DWMMC_BYTCNT (0x20)
#define DWMMC_INTMASK (0x24)
#define INT_EBE (1 << 15)
#define INT_SBE (1 << 13)
#define INT_HLE (1 << 12)
#define INT_FRUN (1 << 11)
#define INT_DRT (1 << 9)
#define INT_RTO (1 << 8)
#define INT_DCRC (1 << 7)
#define INT_RCRC (1 << 6)
#define INT_RXDR (1 << 5)
#define INT_TXDR (1 << 4)
#define INT_DTO (1 << 3)
#define INT_CMD_DONE (1 << 2)
#define INT_RE (1 << 1)

#define DWMMC_CMDARG (0x28)
#define DWMMC_CMD (0x2c)
#define CMD_START (U(1) << 31)
#define CMD_USE_HOLD_REG (1 << 29) /* 0 if SDR50/100 */
#define CMD_UPDATE_CLK_ONLY (1 << 21)
#define CMD_SEND_INIT (1 << 15)
#define CMD_STOP_ABORT_CMD (1 << 14)
#define CMD_WAIT_PRVDATA_COMPLETE (1 << 13)
#define CMD_WRITE (1 << 10)
#define CMD_DATA_TRANS_EXPECT (1 << 9)
#define CMD_CHECK_RESP_CRC (1 << 8)
#define CMD_RESP_LEN (1 << 7)
#define CMD_RESP_EXPECT (1 << 6)
#define CMD(x) (x & 0x3f)

#define DWMMC_RESP0 (0x30)
#define DWMMC_RESP1 (0x34)
#define DWMMC_RESP2 (0x38)
#define DWMMC_RESP3 (0x3c)
#define DWMMC_RINTSTS (0x44)
#define DWMMC_STATUS (0x48)
#define STATUS_DATA_BUSY (1 << 9)

#define DWMMC_FIFOTH (0x4c)
#define FIFOTH_TWMARK(x) (x & 0xfff)
#define FIFOTH_RWMARK(x) ((x & 0x1ff) << 16)
#define FIFOTH_DMA_BURST_SIZE(x) ((x & 0x7) << 28)

#define DWMMC_DEBNCE (0x64)
#define DWMMC_BMOD (0x80)
#define BMOD_ENABLE (1 << 7)
#define BMOD_FB (1 << 1)
#define BMOD_SWRESET (1 << 0)

#define DWMMC_RST_N (0x078)

#define DWMMC_DBADDR (0x88)
// #define DWMMC_IDSTS			(0x8c)
// #define DWMMC_IDINTEN			(0x90)
#define DWMMC_IDSTS (0x90)
#define DWMMC_IDINTEN (0x94)
#define DWMCI_DSCADDR (0x98)
#define DWMCI_BUFADDR (0xa0)

#define DWMMC_CARDTHRCTL (0x100)
#define CARDTHRCTL_RD_THR(x) ((x & 0xfff) << 16)
#define CARDTHRCTL_RD_THR_EN (1 << 0)

#define IDMAC_DES0_DIC (1 << 1)
#define IDMAC_DES0_LD (1 << 2)
#define IDMAC_DES0_FS (1 << 3)
#define IDMAC_DES0_CH (1 << 4)
#define IDMAC_DES0_ER (1 << 5)
#define IDMAC_DES0_CES (1 << 30)
#define IDMAC_DES0_OWN (U(1) << 31)
#define IDMAC_DES1_BS1(x) ((x)&0x1fff)
#define IDMAC_DES2_BS2(x) (((x)&0x1fff) << 13)

#define DWMMC_8BIT_MODE (1 << 6)

#define DWMMC_ADDRESS_MASK U(0x0f)

#define TIMEOUT 100000

#define DW_MMC_CLK_DIV 2

#define DW_MMC_CLK_IN_HZ          U(100000000)

struct dw_idmac_desc {
	unsigned int des0;
	unsigned int nouse_des1;
	unsigned int des2;
	unsigned int nouse_des3;
	unsigned int des4_l;
	unsigned int des5_h;
	unsigned int des6_l;
	unsigned int des7_h;
};

typedef struct dw_mmc_params {
	uintptr_t reg_base;
	uintptr_t desc_base;
	size_t desc_size;
	int clk_rate;
	int max_clk;
	int bus_width;
	unsigned int flags;
	enum mmc_device_type mmc_dev_type;
	int inited;
} dw_mmc_params_t;

static dw_mmc_params_t dw_params = {0};
static unsigned long int dma_desc[32 * 2] __aligned(64) = { 0 };

static inline void writel(uint32_t address, uint32_t value)
{
	uintptr_t addr = (uintptr_t)address;

	*((volatile uint32_t *)(addr)) = value;
}

static inline uint32_t readl(uint32_t address)
{
	uintptr_t addr = (uintptr_t)address;

	return *((volatile uint32_t *)(addr));
}

static void dw_update_clk(void)
{
	unsigned int data;

	writel(dw_params.reg_base + DWMMC_CMD, CMD_WAIT_PRVDATA_COMPLETE | CMD_UPDATE_CLK_ONLY | CMD_START);
	while (1) {
		data = readl(dw_params.reg_base + DWMMC_CMD);
		if ((data & CMD_START) == 0)
			break;
		data = readl(dw_params.reg_base + DWMMC_RINTSTS);
		assert((data & INT_HLE) == 0);
	}
}

static void dw_set_clk(int clk)
{
	unsigned int data;
	int div;

	assert(clk > 0);

	for (div = 1; div < 256; div++) {
		if ((dw_params.clk_rate / (2 * div)) <= clk) {
			break;
		}
	}
	assert(div < 256);

	if (clk >= dw_params.clk_rate) {
		div = 0;
	}

	/* wait until controller is idle */
	do {
		data = readl(dw_params.reg_base + DWMMC_STATUS);
	} while (data & STATUS_DATA_BUSY);

	/* disable clock before change clock rate */
	writel(dw_params.reg_base + DWMMC_CLKENA, 0);
	dw_update_clk();

	writel(dw_params.reg_base + DWMMC_CLKDIV, div);
	dw_update_clk();

	/* enable clock */
	writel(dw_params.reg_base + DWMMC_CLKENA, 1);
	writel(dw_params.reg_base + DWMMC_CLKSRC, 0);
	dw_update_clk();
}

static int dw_send_cmd(struct mmc_cmd *cmd)
{
	unsigned int op, data, err_mask;
	uintptr_t base;
	int timeout;

	assert(cmd);

	base = dw_params.reg_base;

	switch (cmd->cmd_idx) {
	case 0:
		op = CMD_SEND_INIT;
		break;
	case 12:
		op = CMD_STOP_ABORT_CMD;
		break;
	case 13:
		op = CMD_WAIT_PRVDATA_COMPLETE;
		break;
	case 8:
		if (dw_params.mmc_dev_type == MMC_IS_EMMC)
			op = CMD_DATA_TRANS_EXPECT | CMD_WAIT_PRVDATA_COMPLETE;
		else
			op = CMD_WAIT_PRVDATA_COMPLETE;
		break;
	case 17:
	case 18:
		op = CMD_DATA_TRANS_EXPECT | CMD_WAIT_PRVDATA_COMPLETE;
		break;
	case 24:
	case 25:
		op = CMD_WRITE | CMD_DATA_TRANS_EXPECT | CMD_WAIT_PRVDATA_COMPLETE;
		break;
	case 51:
		op = CMD_DATA_TRANS_EXPECT;
		break;
	default:
		op = 0;
		break;
	}
	op |= CMD_USE_HOLD_REG | CMD_START;
	switch (cmd->resp_type) {
	case 0:
		break;
	case MMC_RESPONSE_R2:
		op |= CMD_RESP_EXPECT | CMD_CHECK_RESP_CRC | CMD_RESP_LEN;
		break;
	case MMC_RESPONSE_R3:
		op |= CMD_RESP_EXPECT;
		break;
	default:
		op |= CMD_RESP_EXPECT | CMD_CHECK_RESP_CRC;
		break;
	}
	timeout = 1000 * 1000; // 1s
	do {
		data = readl(base + DWMMC_STATUS);
		if (--timeout <= 0) {
			printf("%s: Timeout on send cmd %d\n",
					__func__, cmd->cmd_idx);
			return -ETIMEDOUT;
		}
		if (data & STATUS_DATA_BUSY)
			udelay(1);
	} while (data & STATUS_DATA_BUSY);

	writel(base + DWMMC_RINTSTS, ~0);
	writel(base + DWMMC_CMDARG, cmd->cmd_arg);
	writel(base + DWMMC_CMD, op | cmd->cmd_idx);

	err_mask = INT_EBE | INT_HLE | INT_RTO | INT_RCRC | INT_RE | INT_DCRC | INT_DRT | INT_SBE;
	timeout = 5000 * 1000; // 5s
	do {
		data = readl(base + DWMMC_RINTSTS);

		if (data & err_mask) {
			printf("cmd%d RINTSTS:0x%x\n", cmd->cmd_idx, data);
			return -EIO;
		}
		if (data & INT_DTO)
			break;
		if (--timeout == 0) {
			printf("cmd%d RINTSTS:0x%x\n", cmd->cmd_idx, data);
			return -ETIMEDOUT;
		}
		if (!(data & INT_CMD_DONE)) {
			udelay(1);
		}
	} while (!(data & INT_CMD_DONE));

	if (op & CMD_RESP_EXPECT) {
		cmd->resp_data[0] = readl(base + DWMMC_RESP0);
		if (op & CMD_RESP_LEN) {
			cmd->resp_data[1] = readl(base + DWMMC_RESP1);
			cmd->resp_data[2] = readl(base + DWMMC_RESP2);
			cmd->resp_data[3] = readl(base + DWMMC_RESP3);
		}
	}
	return 0;
}

static int dw_set_ios(unsigned int clk, unsigned int width)
{
	switch (width) {
	case MMC_BUS_WIDTH_1:
		writel(dw_params.reg_base + DWMMC_CTYPE, CTYPE_1BIT);
		break;
	case MMC_BUS_WIDTH_4:
		writel(dw_params.reg_base + DWMMC_CTYPE, CTYPE_4BIT);
		break;
	case MMC_BUS_WIDTH_8:
		writel(dw_params.reg_base + DWMMC_CTYPE, CTYPE_8BIT);
		break;
	default:
		assert(0);
		break;
	}
	dw_set_clk(clk);
	return 0;
}

static void dw_init(void)
{
	unsigned int data;
	uintptr_t base;

	assert((dw_params.reg_base & MMC_BLOCK_MASK) == 0);

	base = dw_params.reg_base;

	/* card poweroff poweron */
	if (dw_params.mmc_dev_type != MMC_IS_EMMC) {
		writel(base + DWMMC_PWREN, 0x0);
		udelay(2000);
		writel(base + DWMMC_PWREN, 0x1);
	}

	/* card reset*/
	data = readl(base + DWMMC_RST_N);
	writel(base + DWMMC_RST_N, data & 0xfffffffe);
	udelay(400);
	data = readl(base + DWMMC_RST_N);
	writel(base + DWMMC_RST_N, data | 0x1);

	/* ctr reset */
	writel(base + DWMMC_CTRL, CTRL_RESET_ALL);
	do {
		data = readl(base + DWMMC_CTRL);
	} while (data);

	/* enable DMA in CTRL */
	data = CTRL_INT_EN | CTRL_DMA_EN | CTRL_IDMAC_EN;
	writel(base + DWMMC_CTRL, data);
	writel(base + DWMMC_RINTSTS, ~0);
	writel(base + DWMMC_INTMASK, 0);
	writel(base + DWMMC_TMOUT, ~0);
	writel(base + DWMMC_IDINTEN, ~0);
	writel(base + DWMMC_BLKSIZ, MMC_BLOCK_SIZE);
	writel(base + DWMMC_BYTCNT, 256 * 1024);
	writel(base + DWMMC_DEBNCE, 0x00ffffff);
	writel(base + DWMMC_BMOD, BMOD_SWRESET);
	do {
		data = readl(base + DWMMC_BMOD);
	} while (data & BMOD_SWRESET);
	/* enable DMA in BMOD */
	data |= BMOD_ENABLE | BMOD_FB;
	writel(base + DWMMC_BMOD, data);

	dw_set_ios(MMC_BOOT_CLK_RATE, MMC_BUS_WIDTH_1);
}

static int dw_prepare(int lba, uintptr_t buf, size_t size)
{
	struct dw_idmac_desc *desc;
	int desc_cnt, i, last;
	uintptr_t base;

	assert(((buf & DWMMC_ADDRESS_MASK) == 0) &&
		   (size > 0) &&
	       (dw_params.desc_size > 0));

	flush_dcache_range(buf, size);

	desc_cnt = (size + DWMMC_DMA_MAX_BUFFER_SIZE - 1) / DWMMC_DMA_MAX_BUFFER_SIZE;
	assert((desc_cnt * sizeof(struct dw_idmac_desc) <= dw_params.desc_size));

	base = dw_params.reg_base;
	desc = (struct dw_idmac_desc *)dw_params.desc_base;
	writel(base + DWMMC_BYTCNT, size);

	if (size < MMC_BLOCK_SIZE)
		writel(base + DWMMC_BLKSIZ, size);
	else
		writel(base + DWMMC_BLKSIZ, MMC_BLOCK_SIZE);

	writel(base + DWMMC_RINTSTS, ~0);
	for (i = 0; i < desc_cnt; i++) {
		desc[i].des0 = IDMAC_DES0_OWN | IDMAC_DES0_CH | IDMAC_DES0_DIC;
		desc[i].des2 = IDMAC_DES1_BS1(DWMMC_DMA_MAX_BUFFER_SIZE);
		desc[i].des4_l = buf + DWMMC_DMA_MAX_BUFFER_SIZE * i;
		desc[i].des6_l = dw_params.desc_base + (sizeof(struct dw_idmac_desc)) * (i + 1);
		desc[i].des5_h = 0;
		desc[i].des7_h = 0;
	}
	/* first descriptor */
	desc->des0 |= IDMAC_DES0_FS;
	/* last descriptor */
	last = desc_cnt - 1;
	(desc + last)->des0 |= IDMAC_DES0_LD;
	(desc + last)->des0 &= ~(IDMAC_DES0_DIC | IDMAC_DES0_CH);
	(desc + last)->des2 = IDMAC_DES1_BS1(size - (last * DWMMC_DMA_MAX_BUFFER_SIZE));
	/* set next descriptor address as 0 */
	(desc + last)->des6_l = 0;

	writel(base + DWMMC_DBADDR, dw_params.desc_base);
	flush_dcache_range(dw_params.desc_base, desc_cnt * DWMMC_DMA_MAX_BUFFER_SIZE);

	return 0;
}

static int dw_read(int lba, uintptr_t buf, size_t size)
{
	uint32_t data = 0;
	int timeout = 5000 * 1000;

	do {
		data = readl(dw_params.reg_base + DWMMC_RINTSTS);
		if (!(data & INT_DTO)) {
			udelay(1);
		}
	} while (!(data & INT_DTO) && timeout-- > 0);

	inv_dcache_range(buf, size);

	return 0;
}

static int dw_write(int lba, uintptr_t buf, size_t size)
{
	uint32_t data = 0;
	int timeout = 5000 * 1000;

	do {
		data = readl(dw_params.reg_base + DWMMC_RINTSTS);
		if (!(data & INT_DTO)) {
			udelay(1);
		}
	} while (!(data & INT_DTO) && timeout-- > 0);

	inv_dcache_range(buf, size);
	return 0;
}

void mmc_iomux_set_default(void)
{
	pinmux_select(PORTA, 0, 7); // clk
	pinmux_select(PORTA, 1, 7); // cmd
	pinmux_select(PORTA, 2, 7); // d0
	pinmux_select(PORTA, 3, 7); // d1
	pinmux_select(PORTA, 4, 7); // d2
	pinmux_select(PORTA, 5, 7); // d3
	pinmux_select(PORTA, 6, 7); // d4
	pinmux_select(PORTA, 7, 7); // d5
	pinmux_select(PORTA, 8, 7); // d6
	pinmux_select(PORTA, 9, 7); // d7
	pinmux_select(PORTA, 26, 7); // pwen
	pinmux_select(PORTA, 25, 7); // rstn
}

static const struct mmc_ops dw_mmc_ops = {
	.init = dw_init,
	.send_cmd = dw_send_cmd,
	.set_ios = dw_set_ios,
	.prepare = dw_prepare,
	.read = dw_read,
	.write = dw_write,
};

int dw_mmc_init(void)
{
	int i = 0;
	int j = 0;
	int ret = 0;
	static struct mmc_device_info mmc_info;
	unsigned char emmc_card_type[] = { MMC_IS_SD_HC, MMC_IS_EMMC };
	unsigned char emmc_bus_width[] = { MMC_BUS_WIDTH_8, MMC_BUS_WIDTH_4, MMC_BUS_WIDTH_1 };

	if (dw_params.inited)
		return 0;

	memset(&dw_params, 0, sizeof(dw_mmc_params_t));
	dw_params.reg_base = EMMC4_5_BASE;
	dw_params.desc_base = (unsigned long int)dma_desc;
	dw_params.desc_size = sizeof(dma_desc);
	dw_params.clk_rate = DW_MMC_CLK_IN_HZ;
	dw_params.flags = 0;
	dw_params.reg_base = EMMC4_5_BASE;
	dw_params.max_clk = 40000000;

	for (i = 0; i < sizeof(emmc_bus_width) / sizeof(emmc_bus_width[0]); i++) {
		for (j = 0; j < sizeof(emmc_card_type) / sizeof(emmc_card_type[0]); j++) {
			if ((emmc_bus_width[i] == MMC_BUS_WIDTH_8) &&
				(emmc_card_type[j] != MMC_IS_EMMC)) {
				continue;
			}
			mmc_iomux_set_default();

			pinmux_select(PORTA, 0, 0); // clk
			pinmux_select(PORTA, 1, 0); // cmd
			if (emmc_bus_width[i] == MMC_BUS_WIDTH_8) {
				pinmux_select(PORTA, 6, 0); // d4
				pinmux_select(PORTA, 7, 0); // d5
				pinmux_select(PORTA, 8, 0); // d6
				pinmux_select(PORTA, 9, 0); // d7
			}
			if ((emmc_bus_width[i] == MMC_BUS_WIDTH_4) ||
				(emmc_bus_width[i] == MMC_BUS_WIDTH_8)) {
				pinmux_select(PORTA, 3, 0); // d1
				pinmux_select(PORTA, 4, 0); // d2
				pinmux_select(PORTA, 5, 0); // d3
			}
			pinmux_select(PORTA, 2, 0); // d0
			pinmux_select(PORTA, 26, 0); // pwen
			pinmux_select(PORTA, 25, 0); // rstn

			dw_params.bus_width = emmc_bus_width[i];
			mmc_info.mmc_dev_type = emmc_card_type[j];
			mmc_info.ocr_voltage = (mmc_info.mmc_dev_type == MMC_IS_EMMC) ?
								OCR_VDD_MIN_1V7 : (OCR_3_3_3_4 | OCR_3_2_3_3);

			dw_params.mmc_dev_type = mmc_info.mmc_dev_type;
			dw_params.clk_rate = dw_params.clk_rate / DW_MMC_CLK_DIV;
			ret = mmc_init(&dw_mmc_ops, dw_params.max_clk, 
							dw_params.bus_width, dw_params.flags, &mmc_info);
			if (!ret) {
				dw_params.inited = 1;
				return 0;
			}
		}
	}

	printf("Init mmc/sd failed!\n");
	return -1;
}