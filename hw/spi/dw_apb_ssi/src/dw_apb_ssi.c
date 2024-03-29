#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "dw_apb_ssi.h"
#include "dw_apb_ssi_regs.h"
#include "dw_apb_gpio.h"

#include "memmap.h"

#define BOOTSPI ((DW_APB_SSI_TypeDef *)(BOOTSPI_BASE + 0x100))

#include "arch_features.h"
#define __DMB	dmbsy

#ifdef __GNUC__
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-label"
#endif

#define SR_BUSY 0UL
#define SR_TFNF 1UL
#define SR_TFE  2UL
#define SR_RFNE 3UL

#define SPI_CS_USE_GPIO 1

/**
 * @brief SPI 状态位.
 * 0 - 未初始化
 * 1 - 已初始化
 */
static uint8_t spi_state[BOOTSPI_ID + 1] = {0};

static inline uint8_t get_spi_state(spi_id_t spi_id)
{
	assert(spi_id <= BOOTSPI_ID);
	return spi_state[spi_id];
}

static inline void set_spi_state(spi_id_t spi_id, uint8_t state)
{
	assert(spi_id <= BOOTSPI_ID);
	spi_state[spi_id] = state;
	return;
}

static inline void spix_pinmux(spi_init_config_t const * const spi_init_config)
{
	gpio_init_config_t gpio_init_config = {
		.gpio_control_mode = Software_Mode,
		.gpio_mode = GPIO_Output_Mode
	};
	switch (spi_init_config->spi_id) {
	case BOOTSPI_ID:
		pinmux_select(PORTB, 26, 0);
	#if (SPI_CS_USE_GPIO != 1)
		pinmux_select(PORTB, 27, 0);
	#else
		gpio_init_config.port = PORTB;
		gpio_init_config.pin = 27;
		gpio_init(&gpio_init_config);
		pinmux_select(PORTB, 27, 7); //as gpio
	#endif
		pinmux_select(PORTB, 28, 0);
		pinmux_select(PORTB, 29, 0);
		pinmux_select(PORTB, 30, 0);
		pinmux_select(PORTB, 31, 0);
		return;
	default:
		return;
	}
}

static inline void* get_spi_base(spi_id_t spi_id)
{
	void* spi_base = NULL;
	switch (spi_id) {
	case BOOTSPI_ID:
		spi_base = (void*)(BOOTSPI_BASE + 0x100);
		break;
	default:
		break;
	}
	return spi_base;
}

static inline void spi_disable(spi_id_t spi_id)
{
	DW_APB_SSI_TypeDef* spix = get_spi_base(spi_id);
	assert(spix != NULL);
	spix->SSIENR = 0;
	spix->SER = 0;
	spix->DMACR = 0;
#if (SPI_CS_USE_GPIO == 1)
	switch (spi_id)
	{
	case BOOTSPI_ID:
		gpio_write_pin(PORTB, 27, GPIO_PIN_SET);
		break;
	default:
		break;
	}
#endif
}

static inline void spi_enable(spi_id_t spi_id)
{
	DW_APB_SSI_TypeDef* spix = get_spi_base(spi_id);
	assert(spix != NULL);
	spix->SSIENR = 1;
}

// static inline bool bootspi_rxfifo_is_empty(void)
// {
// 	return ((BOOTSPI->SR & SPI_RFNE_Msk) == 0)? true:false;
// }

static inline void spi_select_slave(spi_id_t spi_id, uint8_t cs)
{
	DW_APB_SSI_TypeDef* spix = get_spi_base(spi_id);
#if (SPI_CS_USE_GPIO == 1)
	switch (spi_id)
	{
	case BOOTSPI_ID:
		cs ? gpio_write_pin(PORTB, 27, GPIO_PIN_RESET) : gpio_write_pin(PORTB, 27, GPIO_PIN_SET);
	default:
		break;
	}
#endif
	spix->SER = cs;
}

void dw_spi_init(spi_init_config_t const * const spi_init_config)
{
	assert(spi_init_config);
	assert(spi_init_config->spi_id <= 3);
	assert(spi_init_config->spi_mode <= 3);

	if(spi_init_config->spi_id == BOOTSPI_ID)
		*(volatile uint32_t *)BOOTSPI_BASE = 1; // Set as NON_MAP Mode

	DW_APB_SSI_TypeDef *spix = get_spi_base(spi_init_config->spi_id);
	assert(spix != NULL);

	spix_pinmux(spi_init_config);
	//1.disable spi
	spi_disable(spi_init_config->spi_id);

	spix->RSVD = 1; // some workaround for transport speed too slow.

	//2.配置CTRLR0
	uint32_t ctrl0 = spix->CTRLR0;

	//使用SPI协议
	ctrl0 &= (~SPI_FRF_Msk);

	//SPI Mode
	ctrl0 &= ~(3UL << 6UL);
	ctrl0 |= (spi_init_config->spi_mode) << 6UL;

	//SRL
	ctrl0 &= ~SPI_SRL_Msk;

	//DFS_32, 8bit only
	ctrl0 &= ~SPI_DFS_32_Msk;
	ctrl0 |= (8 - 1) << SPI_DFS_32_Pos;

	//SSTE
	ctrl0 &= ~SPI_SSTE_Msk;

	spix->CTRLR0 = ctrl0;

	spix->BAUDR = spi_init_config->clock_div;

	spix->DMACR = 0;

	set_spi_state(spi_init_config->spi_id, 1);
}

void dw_spi_deinit(spi_id_t spi_id, bool as_master)
{
	assert(spi_id <= BOOTSPI_ID);
	
	spi_disable(spi_id);
	set_spi_state(spi_id, 0);
}

//安全模式下的发送可以被打断，发送的数据也是连续的，但是最多一次发送fifo_depth的数据
//非安全模式下的发送不可被打断，否则发送数据可能不是连续的，但是发送的数据长度不限
void dw_spi_transmit_and_receive(spi_id_t spi_id, void *t_buf, void *r_buf, uint32_t tr_size, bool safe_mode)
{
	assert(get_spi_state(spi_id)!=0);
	assert(t_buf);
	assert(r_buf);
	if (safe_mode)
		assert(tr_size <= SPI_FIFO_DEPTH);
	
	DW_APB_SSI_TypeDef* spix = get_spi_base(spi_id);
	uint8_t *t_buf_p = t_buf;
	uint8_t *r_buf_p = r_buf;

	spi_disable(spi_id);

	uint32_t ctrl0 = spix->CTRLR0;

	//TMOD = transmit_and_receive
	ctrl0 &= ~SPI_TMOD_Msk;
	// ctrl0 |= 0 << SPI_TMOD_Pos;

	ctrl0 &= ~SPI_SPI_FRF_Msk; //SPI_FRF

	spix->CTRLR0 = ctrl0;

	spix->SSIENR = 1;

	if (!safe_mode) //不限制传输数据为FIFO DEPTH大小
	{
		spi_select_slave(spi_id, 1);
	}

	//填充发送数据
	uint32_t cnt = tr_size;
	while (cnt--) {
		if (!safe_mode)
			while ((spix->SR & SPI_TFNF_Msk) == 0)
				;
		spix->DR = *t_buf_p++;
		if (!safe_mode) //非安全模式的发送下，必须发一个接收一个
		{
			while ((spix->SR & SPI_RFNE_Msk) == 0)
				;
			*r_buf_p++ = spix->DR;
		}
	}

	if (safe_mode) //安全模式下，可以发送完再进行接收(txfifo_depth==rxfifo_depth)
	{
		spi_select_slave(spi_id, 1);
		//等待数据接收完
		for (uint8_t i = 0; i < tr_size; i++) {
			while ((spix->SR & SPI_RFNE_Msk) == 0)
				;
			r_buf_p[i] = spix->DR;
		}
	}
	
	spi_disable(spi_id);
}

void dw_spi_eeprom_read(spi_id_t spi_id, void *t_buf, uint8_t t_size, void *r_buf, uint16_t r_size)
{
	//0. 判断输入合法性
	assert(get_spi_state(spi_id)!=0);
	assert(spi_id <= BOOTSPI_ID);
	assert(t_buf);
	assert(r_buf);
	assert((t_size <= SPI_FIFO_DEPTH) && (t_size!=0));
	// assert(r_size <= 65536);

	DW_APB_SSI_TypeDef* spix = get_spi_base(spi_id);
	assert(spix != NULL);

	uint8_t *t_buf_p = t_buf;
	uint8_t *r_buf_p = r_buf;

	spi_disable(spi_id);

	uint32_t ctrl0 = spix->CTRLR0;

	//TMOD = EEPROM Read
	ctrl0 &= ~SPI_TMOD_Msk;
	ctrl0 |= 3 << SPI_TMOD_Pos;

	ctrl0 &= ~SPI_SPI_FRF_Msk; //SPI_FRF

	spix->CTRLR0 = ctrl0;

	spix->CTRLR1 = r_size - 1;

	spix->SSIENR = 1;

	//填充发送数据
	while (t_size--) {
		spix->DR = *t_buf_p++;
	}

	spi_select_slave(spi_id, 1);

	//等待数据接收完 应该很难出现r_buf溢出的风险(cpu read speed < spi receive speed)
	for (uint16_t i = 0; i < r_size; i++) {
		while ((spix->SR & SPI_RFNE_Msk) == 0)
			;
		r_buf_p[i] = spix->DR;
	}

	spi_disable(spi_id);
}

void dw_spi_receive_only(spi_id_t spi_id, void* r_buf, uint16_t r_size)
{
	assert(get_spi_state(spi_id)!=0);
	assert(spi_id <= BOOTSPI_ID);
	assert(r_buf);
	// assert(r_size <= 65536);

	DW_APB_SSI_TypeDef* spix = get_spi_base(spi_id);
	assert(spix != NULL);
	uint8_t *r_buf_p = r_buf;

	spi_disable(spi_id);

	uint32_t ctrl0 = spix->CTRLR0;
	//TMOD = receive_only
	ctrl0 &= ~SPI_TMOD_Msk;
	ctrl0 |= 2 << SPI_TMOD_Pos;
	ctrl0 &= ~SPI_SPI_FRF_Msk; //SPI_FRF
	spix->CTRLR0 = ctrl0;

	spix->CTRLR1 = r_size - 1;

	spi_enable(spi_id);
	spi_select_slave(spi_id, 1);

	//开始接收
	spix->DR = 0xff;

	//等待数据接收完
	for (uint16_t i = 0; i < r_size; i++) {
		while ((spix->SR & SPI_RFNE_Msk) == 0)
			;
		r_buf_p[i] = spix->DR;
	}

	spi_disable(spi_id);
}

void dw_spi_transmit_only(spi_id_t spi_id, void* t_buf, uint32_t t_size, bool safe_mode)
{
	assert(get_spi_state(spi_id)!=0);
	assert(spi_id <= BOOTSPI_ID);
	assert(t_buf);
	if(safe_mode)
		assert(t_size <= SPI_FIFO_DEPTH);

	DW_APB_SSI_TypeDef* spix = get_spi_base(spi_id);
	assert(spix != NULL);
	uint8_t *t_buf_p = t_buf;

	spi_disable(spi_id);

	uint32_t ctrl0 = spix->CTRLR0;
	//TMOD = transmit_only
	ctrl0 &= ~SPI_TMOD_Msk;
	ctrl0 |= 1 << SPI_TMOD_Pos;
	ctrl0 &= ~SPI_SPI_FRF_Msk; //SPI_FRF
	spix->CTRLR0 = ctrl0;

	spix->SSIENR = 1;

	if (!safe_mode) //不限制传输数据为FIFO DEPTH大小
		spi_select_slave(spi_id, 1);

	//填充发送数据
	uint32_t cnt = t_size;
	while (cnt--) {
		if (!safe_mode)
			while ((spix->SR & SPI_TFNF_Msk) == 0);
		spix->DR = *t_buf_p++;
	}

	if (safe_mode) //限制传输数据为FIFO DEPTH大小
		spi_select_slave(spi_id, 1);

	//等待数据接收完
	while (!(spix->SR & SPI_TFE_Msk));
	while ((spix->SR & SPI_BUSY_Msk));

	spi_disable(spi_id);
}

/* Enhanced SPI Modes */
void dw_spi_enhanced_read(spi_id_t spi_id, enhanced_transfer_format_t *enhanced_transfer_format)
{
	assert(get_spi_state(spi_id)!=0);
	assert(spi_id <= BOOTSPI_ID);
	assert(enhanced_transfer_format);
	assert(enhanced_transfer_format->addr_lenth<=60);
	assert(enhanced_transfer_format->ins_lenth<=16);
	assert(enhanced_transfer_format->data);
	assert(enhanced_transfer_format->data_nums!=0 && enhanced_transfer_format->data_nums<=65536);
	assert(enhanced_transfer_format->wait_cycles<=31);

	DW_APB_SSI_TypeDef* spix = get_spi_base(spi_id);
	assert(spix != NULL);
	uint8_t addr_lenth = enhanced_transfer_format->addr_lenth;
	uint8_t ins_lenth = enhanced_transfer_format->ins_lenth;
	//配置spi 
	spi_disable(spi_id);
	{
		uint32_t ctrl0 = spix->CTRLR0;
		uint32_t spi_ctrl0 = spix->SPI_CTRLR0;
		{
			// 1.1 配置几线传输
			ctrl0 &= ~SPI_SPI_FRF_Msk; //SPI_FRF
			ctrl0 |= enhanced_transfer_format->spi_frf << SPI_SPI_FRF_Pos;

			ctrl0 &= ~SPI_TMOD_Msk;
			ctrl0 |= 2 << SPI_TMOD_Pos; //read

			// 1.2 配置 TRANS_TYPE
			// 00 - Instruction and Address will be sent in Standard SPI Mode.
			// 01 - Instruction will be sent in Standard SPI Mode and Address will be sent in the mode specified by CTRLR0.SPI_FRF.
			// 10 - 10 - Both Instruction and Address will be sent in the mode specified by SPI_FRF.
			// 11 - Reserved.
			spi_ctrl0 &= ~SPI_TRANS_TYPE_Msk;
			spi_ctrl0 |= enhanced_transfer_format->trans_type << SPI_TRANS_TYPE_Pos;

			// 1.3 ADDR_L
			spi_ctrl0 &= ~SPI_ADDR_L_Msk;
			spi_ctrl0 |= (addr_lenth/4) << SPI_ADDR_L_Pos;

			// 1.4 INST_L
			spi_ctrl0 &= ~SPI_INST_L_Msk;
			if(ins_lenth==16)
				spi_ctrl0 |= 3 << SPI_INST_L_Pos;
			else
				spi_ctrl0 |= (ins_lenth/4) << SPI_INST_L_Pos;

			// 1.5 WAIT_CYCLES
			spi_ctrl0 &= ~SPI_WAIT_CYCLES_Msk;
			spi_ctrl0 |= enhanced_transfer_format->wait_cycles << SPI_WAIT_CYCLES_Pos;
		
			// 1.6 DDR disable
			spi_ctrl0 &= ~SPI_SPI_DDR_EN_Msk;
			spi_ctrl0 &= ~SPI_INST_DDR_EN_Msk;
			spi_ctrl0 &= ~SPI_SPI_RXDS_EN_Msk;
		} 
		spix->CTRLR0 = ctrl0;
		spix->SPI_CTRLR0 = spi_ctrl0;

		//配置接收数据量
		spix->CTRLR1 = enhanced_transfer_format->data_nums - 1;
	}
	spix->SSIENR = 1;


	//传输阶段
	uint8_t quotient = addr_lenth / 32; 
	uint8_t remainder = addr_lenth % 32;
	uint8_t cnt = (remainder) ? (quotient + 1) : (quotient);
	uint32_t* address = enhanced_transfer_format->address;
	uint8_t *data = enhanced_transfer_format->data;
	uint32_t data_nums = enhanced_transfer_format->data_nums;
Instruction_phase:
	if(ins_lenth!=0)
		spix->DR = enhanced_transfer_format->instruction; //An instruction takes one FIFO location
Address_phase:
	//address can take more than one FIFO locations
	for (uint8_t i = 0; i < cnt; i++)
			spix->DR = address[i];
	
	spi_select_slave(spi_id, 1);
	if(addr_lenth==0 && ins_lenth==0)
		spix->DR = 0xff; //write a dummy data to start transmit.
Wait_cycles:
	//waiting...
Data_phase:
	for (uint8_t i = 0; i < data_nums; i++) {
		while ((spix->SR & SPI_RFNE_Msk) == 0);
		data[i] = spix->DR;
	}

	spi_disable(spi_id);
	return;
}

void dw_spi_enhanced_write(spi_id_t spi_id, enhanced_transfer_format_t *enhanced_transfer_format, bool safe_mode)
{
	assert(get_spi_state(spi_id)!=0);
	assert(spi_id <= BOOTSPI_ID);
	assert(enhanced_transfer_format);
	assert(enhanced_transfer_format->addr_lenth<=60);
	assert(enhanced_transfer_format->ins_lenth<=16);

	DW_APB_SSI_TypeDef* spix = get_spi_base(spi_id);
	assert(spix != NULL);
	uint8_t addr_lenth = enhanced_transfer_format->addr_lenth;
	uint8_t ins_lenth = enhanced_transfer_format->ins_lenth;
	//配置spi 
	spi_disable(spi_id);
	{
		uint32_t ctrl0 = spix->CTRLR0;
		uint32_t spi_ctrl0 = spix->SPI_CTRLR0;
		{
			// 1.1 配置几线传输
			ctrl0 &= ~SPI_SPI_FRF_Msk; //SPI_FRF
			ctrl0 |= enhanced_transfer_format->spi_frf << SPI_SPI_FRF_Pos; //!!!标准传输中得配置回去，不然会影响标准传输

			ctrl0 &= ~SPI_TMOD_Msk;
			ctrl0 |= 1 << SPI_TMOD_Pos; //write

			// 1.2 配置 TRANS_TYPE
			// 00 - Instruction and Address will be sent in Standard SPI Mode.
			// 01 - Instruction will be sent in Standard SPI Mode and Address will be sent in the mode specified by CTRLR0.SPI_FRF.
			// 10 - 10 - Both Instruction and Address will be sent in the mode specified by SPI_FRF.
			// 11 - Reserved.
			spi_ctrl0 &= ~SPI_TRANS_TYPE_Msk;
			spi_ctrl0 |= enhanced_transfer_format->trans_type << SPI_TRANS_TYPE_Pos;

			// 1.3 ADDR_L
			spi_ctrl0 &= ~SPI_ADDR_L_Msk;
			spi_ctrl0 |= (addr_lenth/4) << SPI_ADDR_L_Pos;

			// 1.4 INST_L
			spi_ctrl0 &= ~SPI_INST_L_Msk;
			if(ins_lenth==16)
				spi_ctrl0 |= 3 << SPI_INST_L_Pos;
			else
				spi_ctrl0 |= (ins_lenth/4) << SPI_INST_L_Pos;

			// 1.5 WAIT_CYCLES
		
			// 1.6 DDR disable
			spi_ctrl0 &= ~SPI_SPI_DDR_EN_Msk;
			spi_ctrl0 &= ~SPI_INST_DDR_EN_Msk;
			spi_ctrl0 &= ~SPI_SPI_RXDS_EN_Msk;

		} 
		spix->CTRLR0 = ctrl0;
		spix->SPI_CTRLR0 = spi_ctrl0;
	}
	spix->SSIENR = 1;

	//传输阶段
	uint8_t quotient = addr_lenth / 32; 
	uint8_t remainder = addr_lenth % 32;
	uint8_t cnt = (remainder) ? (quotient + 1) : (quotient);
	uint32_t* address = enhanced_transfer_format->address;
	uint8_t *data = enhanced_transfer_format->data;
	uint32_t data_nums = enhanced_transfer_format->data_nums;
	if(!safe_mode)
		spi_select_slave(spi_id, 1);
Instruction_phase:
	if(ins_lenth!=0)
		spix->DR = enhanced_transfer_format->instruction; //An instruction takes one FIFO location
Address_phase:
	//address can take more than one FIFO locations
	for (uint8_t i = 0; i < cnt; i++)
			spix->DR = address[i];
Data_phase:
	for(uint32_t i = 0;i<data_nums;i++)
	{
		if (!safe_mode)
				while ((spix->SR & SPI_TFNF_Msk) == 0);
		spix->DR = data[i];
	}

	if(safe_mode)
		spi_select_slave(spi_id, 1);
	
	//等待数据接收完
	while (!(spix->SR & SPI_TFE_Msk));
	while ((spix->SR & SPI_BUSY_Msk));

	spi_disable(spi_id);
	return;
}

static inline void spi_set_rx_sample_dly(spi_id_t spi_id, uint8_t rx_sample_dly)
{
	assert(spi_id <= BOOTSPI_ID);
	spi_disable(spi_id);
	DW_APB_SSI_TypeDef* spix = get_spi_base(spi_id);
	spix->RX_SAMPLE_DLY = rx_sample_dly;
}

int16_t dw_spi_tuning(spi_id_t spi_id, void *t_buf, uint8_t t_size, void *r_buf, uint16_t r_size)
{
	int16_t min = -1;
	int16_t max = 0;
	uint8_t _r_buf[r_size];
	for(int16_t i=0;i<=8;i++)
	{
		spi_set_rx_sample_dly(spi_id, i);
		dw_spi_eeprom_read(spi_id, t_buf, t_size, _r_buf, r_size);
		if(memcmp(r_buf, _r_buf, r_size) == 0)
		{
			if(min==-1)
				min = i;
			max = i;
		}
		else
		{
			if(min!=-1)
				break;
		}
	}
	if(min==-1)
		return -1;
	uint8_t mid = (min+max)/2;
	spi_set_rx_sample_dly(spi_id, mid);
	return mid;
}

/****************** STD & DMA **********************/
uint8_t dma_channel_state[8] = {0};
#include "dma.h"

DMA_Channel_t dw_spi_eeprom_read_dma_start(spi_id_t spi_id, void *t_buf, uint8_t t_size, void *r_buf, uint32_t r_size)
{
	//0. 判断输入合法性
	assert(get_spi_state(spi_id)!=0);
	assert(spi_id <= BOOTSPI_ID);
	assert(t_buf);
	assert(r_buf);
	assert((t_size <= SPI_FIFO_DEPTH) && (t_size!=0));
	assert(r_size <= DMAC_FIFO_DEPTH);

	DW_APB_SSI_TypeDef* spix = get_spi_base(spi_id);
	assert(spix != NULL);
	uint8_t handshake;
	switch (spi_id)
	{
	case BOOTSPI_ID:
		handshake = HW_BOOTSPI_RX;
		break;
	default:
		break;
	}
	uint8_t *t_buf_p = t_buf;
	uint8_t *r_buf_p = r_buf;

	spi_disable(spi_id);

	uint32_t ctrl0 = spix->CTRLR0;

	//TMOD = EEPROM Read
	ctrl0 &= ~SPI_TMOD_Msk;
	ctrl0 |= 3 << SPI_TMOD_Pos;

	ctrl0 &= ~SPI_SPI_FRF_Msk; //SPI_FRF

	spix->CTRLR0 = ctrl0;

	spix->CTRLR1 = r_size - 1;

	//build-in dma配置
	spix->DMACR = 0;
	spix->DMARDLR = 15; //16触发DMA请求
	// spix->DMARDLR = 0; //1触发DMA请求
	spix->DMACR = 1;

	spix->SSIENR = 1;

	//填充发送数据
	while (t_size--) {
		spix->DR = *t_buf_p++;
	}

	// dma配置
	DMA_Channel_t ch;
	while((ch = get_a_free_dma_channel()) == NO_FREE_DMA_CHANNEL);
	dma_config_t config = {
		.ch = ch,
		.sar = (uintptr_t)(&(spix->DR)),
		.dar = (uintptr_t)r_buf_p,

		.axi_dst_burst_length = 15,
		.axi_src_burst_length = 15,
		.block_ts = r_size -1,
		
		.is_src_addr_increse = SRC_ADDR_NOCHANGE,
		.is_dst_addr_increse = DST_ADDR_INCREMENT,
		.src_transfer_width = SRC_TRANSFER_WIDTH_8,
		.dst_transfer_width = DST_TRANSFER_WIDTH_8,
		.dst_msize = DST_MSIZE_16,
		.src_msize = SRC_MSIZE_16,

		.handle_shake = handshake,
		.dir = PER_TO_MEM
	};
	dma_config(&config);
	dma_channel_start(ch);
	spi_select_slave(spi_id, 1);
	return ch;
}

bool is_dw_spi_eeprom_read_dma_end(spi_id_t spi_id, DMA_Channel_t ch)
{
	if(is_dma_channel_transfer_done(ch))
	{
		clear_channel_transfer_done_irq(ch);
		free_dma_channel(ch);
		spi_disable(spi_id);
		return true;
	}
	return false;
}

void dw_spi_eeprom_read_dma(spi_id_t spi_id, void *t_buf, uint8_t t_size, void *r_buf, uint16_t r_size)
{
	DMA_Channel_t ch = dw_spi_eeprom_read_dma_start(spi_id, t_buf, t_size, r_buf, r_size);
	while(!is_dw_spi_eeprom_read_dma_end(spi_id, ch));
}

void dw_spi_transmit_only_dma(spi_id_t spi_id, void* t_buf, uint32_t t_size)
{
	assert(spi_id <= BOOTSPI_ID);
	assert(get_spi_state(spi_id)!=0);
	assert(t_buf);
	assert(t_size<=DMAC_FIFO_DEPTH);

	__DMB();
	DW_APB_SSI_TypeDef* spix = get_spi_base(spi_id);
	assert(spix != NULL);

	uint8_t handshake;
	switch (spi_id)
	{
	case BOOTSPI_ID:
		handshake = HW_BOOTSPI_TX;
		break;
	default:
		assert(0);
		break;
	}
	spi_disable(spi_id);

	uint32_t ctrl0 = spix->CTRLR0;
	//TMOD = transmit_only
	ctrl0 &= ~SPI_TMOD_Msk;
	ctrl0 |= 1 << SPI_TMOD_Pos;

	ctrl0 &= ~SPI_SPI_FRF_Msk; //SPI_FRF
	spix->CTRLR0 = ctrl0;

	//build-in DMA配置
	spix->DMACR = 0;
	spix->DMATDLR = 16; //<=16触发DMA请求
	// spix->DMATDLR = 31; //<=31触发DMA请求
	spix->DMACR = 2;

	spix->SSIENR = 1;

	// dma配置
	while((REG32(DMA_BASE + DMAC_CHENREG_0) & 1) == 1);
	REG32(DMA_BASE + CH1_SAR_0) = (uintptr_t)t_buf;
	REG32(DMA_BASE + CH1_DAR_0) = (uintptr_t)(&(spix->DR));

	REG32(DMA_BASE + CH1_CTL_0) = AXI_MASTER_0 << 0 |
									AXI_MASTER_0 << 2 |
									SRC_ADDR_INCREMENT << 4 |
									DST_ADDR_NOCHANGE << 6 |
									SRC_TRANSFER_WIDTH_8 << 8 |
									DST_TRANSFER_WIDTH_8 << 11 |
									//   SRC_MSIZE_16 << 14 |
									// DST_MSIZE_1 << 18 |
									DST_MSIZE_16 << 18 |
									NONPOSTED_LASTWRITE_EN << 30;

	REG32(DMA_BASE + CH1_CTL_32) = ARLEN_EN << (38 - 32) |
									15 << (39 - 32) | //axi source burst length
									AWLEN_EN << (47 - 32) |
									15 << (48 - 32) | //axi destination burst length
									SRC_STATUS_DISABLE << (56 - 32) |
									DST_STATUS_DISABLE << (57 - 32) |
									INTDISABLE_COMPLETOFBLKTRANS_SHADORLLI << (58 - 32) |
									NOTLAST_SHADORLLI << (62 - 32) |
									SHADORLLI_INVALID << (63 - 32);

	REG32(DMA_BASE + CH1_BLOCK_TS_0) = t_size - 1;

	REG32(DMA_BASE + CH1_CFG2_0) = SRC_CONTIGUOUS << 0 |
									DST_CONTIGUOUS << 2 |
									handshake      << 11;

	REG32(DMA_BASE + CH1_CFG2_32) = MEM_TO_PER_DMAC << (32 - 32) |
									DST_HARDWARE_HS << (36 - 32) |
									/*26 << (44 - 32) | //src handshake*/
									CHANNEL_PRIORITY7 << (47 - 32) |
									CHANNEL_LOCK_DISABLE << (52 - 32) |
									0x4 << (55 - 32) | //Source Outstanding Request Limit == 3
									0x4 << (59 - 32);  //Destination Outstanding Request Limit == 3

	REG32(DMA_BASE + CH1_INTSTATUS_ENABLEREG_0) = 0xffffffff; //Enable interrupt generation bit is valid
	REG32(DMA_BASE + CH1_INTSIGNAL_ENABLEREG_0) = 0xffffffff; //Enable interrupt generation bit is valid
	REG32(DMA_BASE + DMAC_CFGREG_0) = 0x3;	//enable DMAC and its interrupt logic
	REG32(DMA_BASE + DMAC_CHENREG_0) = 0x101; //EN channel1  while(1)
	spi_select_slave(spi_id, 1);

	while ((REG32(DMA_BASE + CH1_INTSTATUS_0) & 0x00000002) == 0)
		;
	REG32(DMA_BASE + CH1_INTCLEARREG_0) = 0x00000002;

	//等待数据接收完
	while (!(spix->SR & SPI_TFE_Msk));
	while ((spix->SR & SPI_BUSY_Msk));

	spi_disable(spi_id);
}

/********* Enhanced + DMA ********/
void dw_spi_enhanced_read_dma(spi_id_t spi_id, enhanced_transfer_format_t *enhanced_transfer_format)
{
	assert(get_spi_state(spi_id)!=0);
	assert(spi_id <= BOOTSPI_ID);
	assert(enhanced_transfer_format);
	assert(enhanced_transfer_format->addr_lenth<=60);
	assert(enhanced_transfer_format->ins_lenth<=16);
	assert(enhanced_transfer_format->data);
	assert(enhanced_transfer_format->data_nums!=0 && enhanced_transfer_format->data_nums<=DMAC_FIFO_DEPTH);
	assert(enhanced_transfer_format->wait_cycles<=31);

	DW_APB_SSI_TypeDef* spix = get_spi_base(spi_id);
	assert(spix != NULL);
	uint8_t handshake;
	switch (spi_id)
	{
	case BOOTSPI_ID:
		handshake = HW_BOOTSPI_RX;
		break;
	default:
		assert(0);
		return;
	}
	uint8_t addr_lenth = enhanced_transfer_format->addr_lenth;
	uint8_t ins_lenth = enhanced_transfer_format->ins_lenth;
	//配置spi 
	spi_disable(spi_id);
	{
		uint32_t ctrl0 = spix->CTRLR0;
		uint32_t spi_ctrl0 = spix->SPI_CTRLR0;
		{
			// 1.1 配置几线传输
			ctrl0 &= ~SPI_SPI_FRF_Msk; //SPI_FRF
			ctrl0 |= enhanced_transfer_format->spi_frf << SPI_SPI_FRF_Pos;

			ctrl0 &= ~SPI_TMOD_Msk;
			ctrl0 |= 2 << SPI_TMOD_Pos; //read

			// 1.2 配置 TRANS_TYPE
			// 00 - Instruction and Address will be sent in Standard SPI Mode.
			// 01 - Instruction will be sent in Standard SPI Mode and Address will be sent in the mode specified by CTRLR0.SPI_FRF.
			// 10 - 10 - Both Instruction and Address will be sent in the mode specified by SPI_FRF.
			// 11 - Reserved.
			spi_ctrl0 &= ~SPI_TRANS_TYPE_Msk;
			spi_ctrl0 |= enhanced_transfer_format->trans_type << SPI_TRANS_TYPE_Pos;

			// 1.3 ADDR_L
			spi_ctrl0 &= ~SPI_ADDR_L_Msk;
			spi_ctrl0 |= (addr_lenth/4) << SPI_ADDR_L_Pos;

			// 1.4 INST_L
			spi_ctrl0 &= ~SPI_INST_L_Msk;
			if(ins_lenth==16)
				spi_ctrl0 |= 3 << SPI_INST_L_Pos;
			else
				spi_ctrl0 |= (ins_lenth/4) << SPI_INST_L_Pos;

			// 1.5 WAIT_CYCLES
			spi_ctrl0 &= ~SPI_WAIT_CYCLES_Msk;
			spi_ctrl0 |= enhanced_transfer_format->wait_cycles << SPI_WAIT_CYCLES_Pos;
		
			// 1.6 DDR disable
			spi_ctrl0 &= ~SPI_SPI_DDR_EN_Msk;
			spi_ctrl0 &= ~SPI_INST_DDR_EN_Msk;
			spi_ctrl0 &= ~SPI_SPI_RXDS_EN_Msk;
		} 
		spix->CTRLR0 = ctrl0;
		spix->SPI_CTRLR0 = spi_ctrl0;

		//build-in dma配置
		spix->DMACR = 0;
		spix->DMARDLR = 15; //16触发DMA请求
		// spix->DMARDLR = 0; //1触发DMA请求
		spix->DMACR = 1;

		//配置接收数据量
		spix->CTRLR1 = enhanced_transfer_format->data_nums - 1;
	}
	spix->SSIENR = 1;

	// dma配置
	while((REG32(DMA_BASE + DMAC_CHENREG_0) & 1) == 1);
	REG32(DMA_BASE + CH1_SAR_0) = (uintptr_t)(&(spix->DR));
	REG32(DMA_BASE + CH1_DAR_0) = (uintptr_t)(enhanced_transfer_format->data);

	REG32(DMA_BASE + CH1_CTL_0) = AXI_MASTER_0 << 0 |
								AXI_MASTER_0 << 2 |
								SRC_ADDR_NOCHANGE << 4 |
								DST_ADDR_INCREMENT << 6 |
								SRC_TRANSFER_WIDTH_8 << 8 |
								DST_TRANSFER_WIDTH_8 << 11 |
								SRC_MSIZE_16 << 14 |
								//   DST_MSIZE_16 <<18  |
								NONPOSTED_LASTWRITE_EN << 30;

	REG32(DMA_BASE + CH1_CTL_32) = ARLEN_EN << (38 - 32) |
									15 << (39 - 32) | //source burst length
									AWLEN_EN << (47 - 32) |
									15 << (48 - 32) | //destination burst length
									SRC_STATUS_DISABLE << (56 - 32) |
									DST_STATUS_DISABLE << (57 - 32) |
									INTDISABLE_COMPLETOFBLKTRANS_SHADORLLI << (58 - 32) |
									NOTLAST_SHADORLLI << (62 - 32) |
									SHADORLLI_INVALID << (63 - 32);

	REG32(DMA_BASE + CH1_BLOCK_TS_0) = enhanced_transfer_format->data_nums - 1;

	REG32(DMA_BASE + CH1_CFG2_0) = SRC_CONTIGUOUS << 0 |
								DST_CONTIGUOUS << 2 |
								handshake      << 4;

	REG32(DMA_BASE + CH1_CFG2_32) = PER_TO_MEM_DMAC << (32 - 32) |
									SRC_HARDWARE_HS << (35 - 32) |
									/*25 << (39 - 32) | //src handshake*/
									CHANNEL_PRIORITY7 << (47 - 32) |
									CHANNEL_LOCK_DISABLE << (52 - 32) |
									0x4 << (55 - 32) | //Source Outstanding Request Limit == 3
									0x4 << (59 - 32);  //Destination Outstanding Request Limit == 3

	REG32(DMA_BASE + CH1_INTSTATUS_ENABLEREG_0) = 2; //Enable interrupt generation bit is valid
	REG32(DMA_BASE + CH1_INTSIGNAL_ENABLEREG_0) = 2; //Enable interrupt generation bit is valid
	REG32(DMA_BASE + DMAC_CFGREG_0) = 0x3;           //enable DMAC and its interrupt logic
	REG32(DMA_BASE + DMAC_CHENREG_0) = 0x101;        //EN channel1

	//传输阶段
	uint8_t quotient = addr_lenth / 32; 
	uint8_t remainder = addr_lenth % 32;
	uint8_t cnt = (remainder) ? (quotient + 1) : (quotient);
	uint32_t* address = enhanced_transfer_format->address;
	// uint8_t *data = enhanced_transfer_format->data;
	// uint32_t data_nums = enhanced_transfer_format->data_nums;
Instruction_phase:
	if(ins_lenth!=0)
		spix->DR = enhanced_transfer_format->instruction; //An instruction takes one FIFO location
Address_phase:
	//address can take more than one FIFO locations
	for (uint8_t i = 0; i < cnt; i++)
			spix->DR = address[i];
	
	spi_select_slave(spi_id, 1);
	if(addr_lenth==0 && ins_lenth==0)
		spix->DR = 0xff; //write a dummy data to start transmit.
Wait_cycles:
	//waiting...
Data_phase:
	// for (uint8_t i = 0; i < data_nums; i++) {
	// 	while ((spix->SR & SPI_RFNE_Msk) == 0);
	// 	data[i] = spix->DR;
	// }
	while ((REG32(DMA_BASE + CH1_INTSTATUS_0) & 0x00000002) == 0);
	REG32(DMA_BASE + CH1_INTCLEARREG_0) = 0x00000002;
	spi_disable(spi_id);
	return;
}

#ifdef __GNUC__
	#pragma GCC diagnostic pop
#endif