#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gicv3.h"
#include "rhea_pcie_rc.h"
#include "systimer.h"
#include "common.h"
#include "dw_apb_gpio.h"
#include "utils_def.h"
#include "dw_apb_timers.h"

#include "lpi.h"

#define PCIE_X86_LINK_FOR_03		1

#define PCIE_C2C_LINK_FOR_03		0

#define PCIE_C2C_LINK_FOR_73		1

struct HAL_PCIE_DEV g_pcieDev_02_x16;
struct HAL_PCIE_DEV g_pcieDev_02_x8;
struct HAL_PCIE_DEV g_pcieDev_03_x16;
struct HAL_PCIE_DEV g_pcieDev_03_x8;
struct HAL_PCIE_DEV g_pcieDev_72_x16;
struct HAL_PCIE_DEV g_pcieDev_72_x8;
struct HAL_PCIE_DEV g_pcieDev_73_x16;
struct HAL_PCIE_DEV g_pcieDev_73_x8;

typedef void (*operation_t)(void);
/********************* Private Structure Definition **************************/

typedef struct {
	union { /* 0x00 */
		struct {
			__IOM uint32_t cfg_ven_msg_fmt : 2;
			__IOM uint32_t cfg_ven_msg_type : 5;
			__IOM uint32_t cfg_ven_msg_tc : 3;
			__IOM uint32_t cfg_ven_msg_td : 1;
			__IOM uint32_t cfg_ven_msg_ep : 1;
			__IOM uint32_t cfg_ven_msg_attr : 2;
			__IOM uint32_t : 2;
			__IOM uint32_t cfg_ven_msg_len : 10;
			__IOM uint32_t : 2;
			__IOM uint32_t cfg_ven_msg_func_num : 3;
			__IOM uint32_t cfg_ven_msg_req : 1;
		};
		__IOM uint32_t all;
	}cfg_ven_msg_0;

	union { /* 0x04 */
		struct {
			__IOM uint32_t cfg_ven_msg_tag : 10;
			__IOM uint32_t sts_ven_msg_busy : 1;
			__IOM uint32_t : 5;
			__IOM uint32_t cfg_ven_msg_code : 8;
			__IOM uint32_t : 8;
		};
		__IOM uint32_t all;
	}cfg_ven_msg_4;

	__IOM uint32_t cfg_ven_msg_data_l; /* 0x08 */
	__IOM uint32_t cfg_ven_msg_data_h; /* 0x0c */

} vdm_msg_typedef;

typedef struct {
	union { /* 0x00 */
		struct {
			__IOM uint32_t app_ltr_msg_req : 1;
			__IOM uint32_t app_ltr_msg_req_stat : 1;
			__IOM uint32_t : 2;
			__IOM uint32_t app_ltr_msg_func_num : 3;
			__IOM uint32_t : 1;
			__IOM uint32_t cfg_ltr_m_en : 1;
			__IOM uint32_t cfg_disable_ltr_clr_msg : 1;
			__IOM uint32_t : 22;
		};
		__IOM uint32_t all;
	}cfg_ltr_msg_0;

	__IOM uint32_t app_ltr_msg_latency; /* 0x04 */

} ltr_msg_typedef;

/********************* Private Variable Definition ***************************/

/********************* Private Function Definition ***************************/
static void delay(uint32_t value)
{
	volatile uint32_t i, j;

	for(i = 0; i < value; i++)
		for(j = 0; j < 1000; j++);
}

static void dump_regs(char * name, uint64_t addr, uint32_t length)
{
	if(name == NULL)
		return;

	printf("%s\n", name);
	for(int i = 0; i < length; i = i + 4){
		if((i % 16) == 0 && i != 0){
			printf("\n");
		}
		if((i % 16) == 0){
			printf("0x%lx: ", addr + i);
		}
		printf("0x%08x ", readl(addr + i));
	}
	printf("\n");
}

#if  SEEHI_PLD_PCIE_TEST
static void __attribute__((unused)) mc_init(uint64_t addr, uint8_t layer) {
	// global
	if (layer == 4) {
		REG32(addr+0x00013054) = 0x00000000;
		REG32(addr+0x00013004) = 0x00001000; /* 2GB: 0x00001000, 512MB: 0x00000000 */
		REG32(addr+0x00013004) = 0x80001000; /* 2GB: 0x80001000, 512MB: 0x80000000 */
	} else {
		REG32(addr+0x00013054) = 0x00000000;
		REG32(addr+0x00013004) = 0x00000010;
		REG32(addr+0x00013004) = 0x80000010;
	}

	// bank
	uint32_t i, j, k;
	for (i = 0; i < 72; i++) {
		j = i / 18;
		k = i + j; // skip hub regs
		REG32(addr+k*0x400+0x004) = 0x00000005;
		REG32(addr+k*0x400+0x004) = 0x00000001;
		REG32(addr+k*0x400+0x004) = 0x80000001;
	}
}
#endif

static void BSP_PCIE_EP_VDM(const struct HAL_PCIE_DEV *dev , int cnt, uint32_t l, uint32_t h)
{
	uint64_t apb_base = dev->apbBase;
	vdm_msg_typedef *vdm = (vdm_msg_typedef *)(apb_base + 0x130);

	cnt %= 1024;

	while(vdm->cfg_ven_msg_4.sts_ven_msg_busy == 1);

	vdm->cfg_ven_msg_0.cfg_ven_msg_fmt = 0x1;
	vdm->cfg_ven_msg_0.cfg_ven_msg_type = 0x10;
	vdm->cfg_ven_msg_0.cfg_ven_msg_tc = 0x0;
	vdm->cfg_ven_msg_0.cfg_ven_msg_td = 0x0;
	vdm->cfg_ven_msg_0.cfg_ven_msg_ep = 0x0;
	vdm->cfg_ven_msg_0.cfg_ven_msg_attr = 0x0;
	vdm->cfg_ven_msg_0.cfg_ven_msg_len = 0x0;
	vdm->cfg_ven_msg_0.cfg_ven_msg_func_num = 0x0;

	vdm->cfg_ven_msg_4.cfg_ven_msg_tag = cnt;
	vdm->cfg_ven_msg_4.cfg_ven_msg_code = 0x7e;

	vdm->cfg_ven_msg_data_l = l;
	vdm->cfg_ven_msg_data_h = h;

	// printf("0x130 0x%x 0x134 0x%x 8byte 0x%x 12byte 0x%x\n", readl(apb_base + 0x130), readl(apb_base + 0x134), l, h);
	vdm->cfg_ven_msg_0.cfg_ven_msg_req = 0x1;
}

void BSP_PCIE_EP_LTR(const struct HAL_PCIE_DEV *dev, int cnt)
{
	uint64_t apb_base = dev->apbBase;
	ltr_msg_typedef *ltr = (ltr_msg_typedef *)(apb_base + 0x120);

	cnt %= 8;

	if(ltr->cfg_ltr_msg_0.cfg_ltr_m_en){

		while(ltr->cfg_ltr_msg_0.app_ltr_msg_req_stat == 1);

		ltr->cfg_ltr_msg_0.app_ltr_msg_func_num = cnt;
		ltr->app_ltr_msg_latency = 0x7f007f + cnt;

		printf("ltr->app_ltr_msg_latency 0x%x\n", ltr->app_ltr_msg_latency);
		ltr->cfg_ltr_msg_0.app_ltr_msg_req = 0x1;
	}

}

void BSP_PCIE_EP_INTX(const struct HAL_PCIE_DEV *dev, int cnt)
{
	uint64_t apb_base = dev->apbBase;

	printf("seehi--> %s line: %d cnt 0x%x\n", __func__, __LINE__, cnt);
	writel(0x2, apb_base + 0x108);
	dump_regs("intx:", apb_base + 0x108, 4);
	delay(50);
	writel(0x0, apb_base + 0x108);
	dump_regs("intx:", apb_base + 0x108, 4);
}

void BSP_PCIE_EP_TEST_MSI(const struct HAL_PCIE_DEV *dev)
{
	uint64_t apb_base = dev->apbBase;
	int i;

	printf("seehi--> %s line: %d\n", __func__, __LINE__);
	for(i = 0; i < 32; i++){
		writel((i << 4) | 0x1, apb_base + 0x70);    //4:8  产生msi对应中断 bit0=1
		delay(2);
	}
}

void BSP_PCIE_EP_TEST_MSIX(const struct HAL_PCIE_DEV *dev)
{
	uint64_t dbi_base = dev->dbiBase;
	int i;

	printf("seehi--> %s line: %d\n", __func__, __LINE__);
	for(i = 0; i < 32; i++){
		writel(i, dbi_base + 0x948);              //0:10 vector
		delay(2);
	}
}

void BSP_PCIE_EP_TEST_GEN5(const struct HAL_PCIE_DEV *dev)
{
	uint64_t dbi_base = dev->dbiBase;
	int i;

	printf("seehi--> %s line: %d\n", __func__, __LINE__);
	writel(0x00436D05, dbi_base + 0x7c);
	writel(0x01000005, dbi_base + 0xa0);
	writel(0x300310C8, dbi_base + 0x80c);
	writel(0x300110C8, dbi_base + 0x80c);
}

void BSP_PCIE_EP_TEST_GEN4(const struct HAL_PCIE_DEV *dev)
{
	uint64_t dbi_base = dev->dbiBase;
	int i;

	printf("seehi--> %s line: %d\n", __func__, __LINE__);
	writel(0x00436D04, dbi_base + 0x7c);
	writel(0x01000004, dbi_base + 0xa0);
	writel(0x300310C8, dbi_base + 0x80c);
	writel(0x300110C8, dbi_base + 0x80c);
}

void BSP_PCIE_EP_TEST_GEN3(const struct HAL_PCIE_DEV *dev)
{
	uint64_t dbi_base = dev->dbiBase;
	int i;

	printf("seehi--> %s line: %d\n", __func__, __LINE__);
	writel(0x00436D03, dbi_base + 0x7c);
	writel(0x01000003, dbi_base + 0xa0);
	writel(0x300310C8, dbi_base + 0x80c);
	writel(0x300110C8, dbi_base + 0x80c);
}

void BSP_PCIE_EP_TEST_GEN2(const struct HAL_PCIE_DEV *dev)
{
	uint64_t dbi_base = dev->dbiBase;
	int i;

	printf("seehi--> %s line: %d\n", __func__, __LINE__);
	writel(0x00436D02, dbi_base + 0x7c);
	writel(0x01000002, dbi_base + 0xa0);
	writel(0x300310C8, dbi_base + 0x80c);
	writel(0x300110C8, dbi_base + 0x80c);
}

void BSP_PCIE_EP_TEST_GEN1(const struct HAL_PCIE_DEV *dev)
{
	uint64_t dbi_base = dev->dbiBase;
	int i;

	printf("seehi--> %s line: %d\n", __func__, __LINE__);
	writel(0x00436D01, dbi_base + 0x7c);
	writel(0x01000001, dbi_base + 0xa0);
	writel(0x300310C8, dbi_base + 0x80c);
	writel(0x300110C8, dbi_base + 0x80c);
}

static void a510_radm_msg_payload_parse(struct HAL_PCIE_DEV *dev, uint32_t *req, uint32_t *byte8, uint32_t * byte12)
{
	uint32_t val, i = 0, cnt = 0;
	uint32_t temp0, temp1;
	uint32_t lanes = dev->max_lanes;

	pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_LATCH);
	val = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_RX_MODE);
	if(val){
		if(lanes == 8){
			val = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM);
			// printf("A510_APB_PCIE_MSG_VDM 0x%x\n", val);
			while((val & A510_APB_PCIE_MSG_VDM_VLD_MASK) != 0){

				if((val & A510_APB_PCIE_MSG_VDM_RVLD_MASK) >> 16 == 1){

					*req = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG4);
					*(byte8 + i) = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG0);
					*(byte12 + i) = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG1);

					temp0 = *(byte8 + i);
					temp1 = *(byte12 + i);
					printf("seehi--> %s line: %d x8 fifo | bayte8 0x%x byte12 0x%x\n", __func__, __LINE__, temp0, temp1);

					// if(temp0 == 0x1){
						// BSP_PCIE_EP_LTR(dev, cnt++);
					// }else if(temp0 == 0x2){
						// BSP_PCIE_EP_INTX(dev, cnt++);
					// }else if(temp0 == 0x3){
						// BSP_PCIE_EP_TEST_MSI(dev);
					// }else if(temp0 == 0x4){
						// BSP_PCIE_EP_TEST_MSIX(dev);
					// }else if(temp0 == 0x5){
						// BSP_PCIE_EP_TEST_GEN5(dev);
					// }else if(temp0 == 0x6){
						// BSP_PCIE_EP_TEST_GEN4(dev);
					// }else if(temp0 == 0x7){
						// BSP_PCIE_EP_TEST_GEN3(dev);
					// }else if(temp0 == 0x8){
						// BSP_PCIE_EP_TEST_GEN2(dev);
					// }else if(temp0 == 0x9){
						// BSP_PCIE_EP_TEST_GEN1(dev);
					// }else{
						// BSP_PCIE_EP_VDM(dev, cnt++, temp0, temp1);
					// }
				}

				if((val & A510_APB_PCIE_MSG_VDM_RVLD_MASK) >> 16 == 2){

					*req = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG4);
					*(byte8 + i) = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG2);
					*(byte12 + i) = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG3);
				}

				if((val & A510_APB_PCIE_MSG_VDM_RVLD_MASK) >> 16 == 3){

					*req = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG4);
					*(byte8 + i) = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG2);
					*(byte12 + i) = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG3);
					*(byte8 + i + 1) = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG0);
					*(byte12 + i + 1) = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG1);
				}

				i++;
				if(i >= 10){
					printf("fifo full break!!!\n");
					break;
				}

				pcie_writel_apb(dev, A510_APB_PCIE_MSG_VDM_CLR, A510_APB_PCIE_MSG_VDM);
				pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_LATCH);
				val = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM);
			}
		}else{
			val = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM);
			while((val & A510_APB_PCIE_MSG_VDM_VLD_MASK) != 0){

				*req = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG4);
				*(byte8 + i) = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG0);
				*(byte12 + i) = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG1);

				temp0 = *(byte8 + i);
				temp1 = *(byte12 + i);
				printf("seehi--> %s line: %d x16 fifo | bayte8 0x%x byte12 0x%x\n", __func__, __LINE__, temp0, temp1);

				// if(temp0 == 0x1){
					// BSP_PCIE_EP_LTR(dev, cnt++);
				// }else if(temp0 == 0x2){
					// BSP_PCIE_EP_INTX(dev, cnt++);
				// }else if(temp0 == 0x3){
					// BSP_PCIE_EP_TEST_MSI(dev);
				// }else if(temp0 == 0x4){
					// BSP_PCIE_EP_TEST_MSIX(dev);
				// }else if(temp0 == 0x5){
					// BSP_PCIE_EP_TEST_GEN5(dev);
				// }else if(temp0 == 0x6){
					// BSP_PCIE_EP_TEST_GEN4(dev);
				// }else if(temp0 == 0x7){
					// BSP_PCIE_EP_TEST_GEN3(dev);
				// }else if(temp0 == 0x8){
					// BSP_PCIE_EP_TEST_GEN2(dev);
				// }else if(temp0 == 0x9){
					// BSP_PCIE_EP_TEST_GEN1(dev);
				// }else{
					// BSP_PCIE_EP_VDM(dev, cnt++, temp0, temp1);
				// }

				i++;
				if(i >= 10){
					printf("fifo full break!!!\n");
					break;
				}

				pcie_writel_apb(dev, A510_APB_PCIE_MSG_VDM_CLR, A510_APB_PCIE_MSG_VDM);
				pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_LATCH);
				val = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM);
			}
		}
	}else{
		if(lanes == 8){
			val = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM);
			// printf("A510_APB_PCIE_MSG_VDM 0x%x\n", val);
			if((val & A510_APB_PCIE_MSG_VDM_VLD_MASK) >> 1 == 1){

				*req = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG4);
				*byte8 = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG0);
				*byte12 = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG1);

				temp0 = *byte8;
				temp1 = *byte12;
				printf("seehi--> %s line: %d x8 no_fifo | bayte8 0x%x byte12 0x%x\n", __func__, __LINE__, temp0, temp1);

				// if(temp0 == 0x1){
					// BSP_PCIE_EP_LTR(dev, cnt++);
				// }else if(temp0 == 0x2){
					// BSP_PCIE_EP_INTX(dev, cnt++);
				// }else if(temp0 == 0x3){
					// BSP_PCIE_EP_TEST_MSI(dev);
				// }else if(temp0 == 0x4){
					// BSP_PCIE_EP_TEST_MSIX(dev);
				// }else if(temp0 == 0x5){
					// BSP_PCIE_EP_TEST_GEN5(dev);
				// }else if(temp0 == 0x6){
					// BSP_PCIE_EP_TEST_GEN4(dev);
				// }else if(temp0 == 0x7){
					// BSP_PCIE_EP_TEST_GEN3(dev);
				// }else if(temp0 == 0x8){
					// BSP_PCIE_EP_TEST_GEN2(dev);
				// }else if(temp0 == 0x9){
					// BSP_PCIE_EP_TEST_GEN1(dev);
				// }else{
					// BSP_PCIE_EP_VDM(dev, cnt++, temp0, temp1);
				// }
			}

			if((val & A510_APB_PCIE_MSG_VDM_VLD_MASK) >> 1 == 2){

				*req = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG4);
				*byte8 = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG2);
				*byte12 = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG3);
			}

			if((val & A510_APB_PCIE_MSG_VDM_VLD_MASK) >> 1 == 3){

				*req = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG4);
				*byte8 = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG2);
				*byte12 = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG3);
				*(byte8 + 1) = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG0);
				*(byte12 + 1) = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG1);
			}

			pcie_writel_apb(dev, A510_APB_PCIE_MSG_VDM_CLR, A510_APB_PCIE_MSG_VDM);
		}else{
			val = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM);
			if((val & A510_APB_PCIE_MSG_VDM_VLD_MASK) >> 1 == 1){

				*req = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG4);
				*byte8 = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG0);
				*byte12 = pcie_readl_apb(dev, A510_APB_PCIE_MSG_VDM_REG1);

				temp0 = *byte8;
				temp1 = *byte12;
				printf("seehi--> %s line: %d x16 no_fifo | bayte8 0x%x byte12 0x%x\n", __func__, __LINE__, temp0, temp1);

				// if(temp0 == 0x1){
					// BSP_PCIE_EP_LTR(dev, cnt++);
				// }else if(temp0 == 0x2){
					// BSP_PCIE_EP_INTX(dev, cnt++);
				// }else if(temp0 == 0x3){
					// BSP_PCIE_EP_TEST_MSI(dev);
				// }else if(temp0 == 0x4){
					// BSP_PCIE_EP_TEST_MSIX(dev);
				// }else if(temp0 == 0x5){
					// BSP_PCIE_EP_TEST_GEN5(dev);
				// }else if(temp0 == 0x6){
					// BSP_PCIE_EP_TEST_GEN4(dev);
				// }else if(temp0 == 0x7){
					// BSP_PCIE_EP_TEST_GEN3(dev);
				// }else if(temp0 == 0x8){
					// BSP_PCIE_EP_TEST_GEN2(dev);
				// }else if(temp0 == 0x9){
					// BSP_PCIE_EP_TEST_GEN1(dev);
				// }else{
					// BSP_PCIE_EP_VDM(dev, cnt++, temp0, temp1);
				// }
			}

			pcie_writel_apb(dev, A510_APB_PCIE_MSG_VDM_CLR, A510_APB_PCIE_MSG_VDM);
		}
	}
}

void pcie_irq_handler(struct HAL_PCIE_DEV *dev)
{
	uint32_t reg, val, req, byte8[10], byte12[10];

	reg = pcie_readl_apb(dev, A510_APB_PCIE_STAT_INT0);

	printf("seehi--> %s line: %d A510_APB_PCIE_STAT_INT0 = 0x%x \n", __func__, __LINE__, reg);
	if (reg & A510_APB_PCIE_CFG_LINK_EQ_REQ_INT)
		printf("A510_APB_PCIE_CFG_LINK_EQ_REQ_INT\n");

	if (reg & A510_APB_PCIE_CFG_BW_MGT_MSI)
		printf("A510_APB_PCIE_CFG_BW_MGT_MSI\n");

	if (reg & A510_APB_PCIE_CFG_LINK_AUTO_BW_MSI)
		printf("A510_APB_PCIE_CFG_LINK_AUTO_BW_MSI\n");

	if (reg & A510_APB_PCIE_CFG_BW_MGT_INT)
		printf("A510_APB_PCIE_CFG_BW_MGT_INT\n");

	if (reg & A510_APB_PCIE_CFG_LINK_AUTO_BW_INT)
		printf("A510_APB_PCIE_CFG_LINK_AUTO_BW_INT\n");

	if (reg & A510_APB_PCIE_RADM_MSG_UNLOCK_INT)
		printf("A510_APB_PCIE_RADM_MSG_UNLOCK_INT\n");

	if (reg & A510_APB_PCIE_RADM_MSG_LTR_INT){
		printf("A510_APB_PCIE_RADM_MSG_LTR_INT\n");
		pcie_writel_apb(dev, 0x1, A510_APB_PCIE_MSG_LTR_LATCH);
		val = pcie_readl_apb(dev, A510_APB_PCIE_MSG_LTR);
		if(val & A510_APB_PCIE_RADM_MSG_LTR){

			req = pcie_readl_apb(dev, A510_APB_PCIE_MSG_LTR_REG4);
			byte8[0] = pcie_readl_apb(dev, A510_APB_PCIE_MSG_LTR_REG1);
			byte12[0] = pcie_readl_apb(dev, A510_APB_PCIE_MSG_LTR_REG0);

			pcie_writel_apb(dev, A510_APB_PCIE_MSG_LTR_CLR, A510_APB_PCIE_MSG_LTR);
			printf("seehi--> %s line: %d req 0x%x bayte8 0x%x byte12 0x%x\n", __func__, __LINE__, req, byte8[0], byte12[0]);
		}
	}

	if (reg & A510_APB_PCIE_RADM_MSG_VDM_INT){
		printf("A510_APB_PCIE_RADM_MSG_VDM_INT\n");
		a510_radm_msg_payload_parse(dev, &req, byte8, byte12);
	}

	if (reg & A510_APB_PCIE_RADM_MSG_SLOT_PWR_INT)
		printf("A510_APB_PCIE_RADM_MSG_SLOT_PWR_INT\n");

	pcie_writel_apb(dev, reg, A510_APB_PCIE_CLR_INT0);

	return;
}

void pcie_irq_handler_02_x16(void)
{
	pcie_irq_handler(&g_pcieDev_02_x16);
}

void pcie_irq_handler_03_x16(void)
{
	pcie_irq_handler(&g_pcieDev_03_x16);
}

void pcie_irq_handler_72_x16(void)
{
	pcie_irq_handler(&g_pcieDev_72_x16);
}

void pcie_irq_handler_73_x16(void)
{
	pcie_irq_handler(&g_pcieDev_73_x16);
}

void pcie_irq_handler_02_x8(void)
{
	pcie_irq_handler(&g_pcieDev_02_x8);
}

void pcie_irq_handler_03_x8(void)
{
	pcie_irq_handler(&g_pcieDev_03_x8);
}

void pcie_irq_handler_72_x8(void)
{
	pcie_irq_handler(&g_pcieDev_72_x8);
}

void pcie_irq_handler_73_x8(void)
{
	pcie_irq_handler(&g_pcieDev_73_x8);
}

void LPI_8192_Handler(void)
{
	printf("LPI 8192 received !!!!\n");
}

void LPI_8193_Handler(void)
{
  printf("LPI 8193 received !!!!\n");
}

static operation_t get_pcie_irq_func(struct HAL_PCIE_DEV *dev, HAL_TileSelect tile, HAL_ControlType control)
{
	switch(tile){
	case TILE_02:
		if(control == HAL_X8){
			return pcie_irq_handler_02_x8;
		}else{
			return pcie_irq_handler_02_x16;
		}
		break;
	case TILE_03:
		if(control == HAL_X8){
			return pcie_irq_handler_03_x8;
		}else{
			return pcie_irq_handler_03_x16;
		}
		break;
	case TILE_72:
		if(control == HAL_X8){
			return pcie_irq_handler_72_x8;
		}else{
			return pcie_irq_handler_72_x16;
		}
		break;
	case TILE_73:
		if(control == HAL_X8){
			return pcie_irq_handler_73_x8;
		}else{
			return pcie_irq_handler_73_x16;
		}
		break;
	default:
		return NULL;
	}
}

static void dw_timer_mbi_tx(uint32_t id)
{
	uint32_t val;

	writel(((uint64_t)(MBI_RX_BASE+0x40) & 0xFFFFFFFF), MBI_TX_BASE + 0x18);
	writel(((uint64_t)(MBI_RX_BASE+0x40) >> 32), MBI_TX_BASE + 0x1c);

	val = readl(MBI_TX_BASE + 0x30);
	val |= id;
	writel(val, MBI_TX_BASE + 0x30);

	val = readl(MBI_TX_BASE + 0x40);
	val |= id;
	writel(val, MBI_TX_BASE + 0x40);
}

static void dw_timer_init(void)
{
	timer_init_config_t timer_init_config = {
		.int_mask = 0, .loadcount = 25000000, .timer_id = Timerx6_T2, .timer_mode = Mode_User_Defined
	};
	timer_init(&timer_init_config);
}

int main()
{
	int ret = 0;
	int cnt = 0;
	struct HAL_PCIE_DEV dev;
	HAL_TileSelect tile;
	HAL_ControlType control;
	HAL_MbiSelect __attribute__((unused)) mbi = MBI_AP;

#if SEEHI_PLD_PCIE_TEST
	mc_init(TCM_04_CFG_BASE, 4);
	mbi = MBI_AP;
#if SEEHI_NPU_PCIE_TEST
	mc_init(TCM_53_CFG_BASE, 4);
	mc_init(TCM_54_CFG_BASE, 4);
	mc_init(TCM_63_CFG_BASE, 4);
	mc_init(TCM_64_CFG_BASE, 4);
	mbi = MBI_53;
#endif
#endif

	systimer_init();
	init_gic();
	lpi_init();

#if SEEHI_NPU_PCIE_TEST
	/* when print start, please power on x86 pc */
	printf("t53:0x%08x\n", REG32(0x5340000000 + 536870912 + 0xc0));
	printf("t54:0x%08x\n", REG32(0x5440000000 + 536870912 + 0xc0));
	printf("t63:0x%08x\n", REG32(0x6340000000 + 536870912 + 0xc0));
	printf("t64:0x%08x\n", REG32(0x6440000000 + 536870912 + 0xc0));
#endif

#if(PCIE_X86_LINK_FOR_03 == 0) && (PCIE_C2C_LINK_FOR_03 == 0) && (PCIE_C2C_LINK_FOR_73 == 0)
#error
#endif
#if(PCIE_X86_LINK_FOR_03 == 1) && (PCIE_C2C_LINK_FOR_03 == 1)
#error
#endif
#if(PCIE_C2C_LINK_FOR_73 == 1) && (PCIE_C2C_LINK_FOR_03 == 1)
#error
#endif

#if (PCIE_X86_LINK_FOR_03 == 1)
	rhea_pcie_ep_init(&dev, TILE_03, HAL_X16, X86_EP, mbi);  //x86 ep
		if(ret != 0)
			printf("rhea_pcie_ep_init error\n");
	tile = 0;
	control = 0;
	mbi = MBI_AP;
#endif

#if (PCIE_C2C_LINK_FOR_03)
	tile = TILE_03;
	control = HAL_X16;
	// control = HAL_X16toX8;
	// control = HAL_X8;
	mbi = MBI_AP;
#endif

#if (PCIE_C2C_LINK_FOR_73)
	tile = TILE_73;
	control = HAL_X16;
	mbi = MBI_AP;
#endif

	if(tile == TILE_03 && control == HAL_X16){
		rhea_pcie_rc_init(&dev, tile, control, C2C_RC);
		if(ret != 0)
			printf("rhea_pcie_rc_init error\n");
		memcpy(&g_pcieDev_03_x16, &dev, sizeof(struct HAL_PCIE_DEV));
		IRQ_SetHandler(dev.vdmIrqNum, get_pcie_irq_func(&dev, tile, control));
		GIC_SetPriority(dev.vdmIrqNum, 0 << 3);
		GIC_EnableIRQ(dev.vdmIrqNum);
	}

	if(tile == TILE_03 && control == HAL_X8){
		rhea_pcie_rc_init(&dev, tile, control, C2C_RC);
		if(ret != 0)
			printf("rhea_pcie_rc_init error\n");
		memcpy(&g_pcieDev_03_x8, &dev, sizeof(struct HAL_PCIE_DEV));
		IRQ_SetHandler(dev.vdmIrqNum, get_pcie_irq_func(&dev, tile, control));
		GIC_SetPriority(dev.vdmIrqNum, 0 << 3);
		GIC_EnableIRQ(dev.vdmIrqNum);
	}

	if(tile == TILE_03 && control == HAL_X16toX8){
		rhea_pcie_rc_init(&dev, tile, control, C2C_RC);
		if(ret != 0)
			printf("rhea_pcie_rc_init error\n");
		memcpy(&g_pcieDev_03_x16, &dev, sizeof(struct HAL_PCIE_DEV));
		IRQ_SetHandler(dev.vdmIrqNum, get_pcie_irq_func(&dev, tile, control));
		GIC_SetPriority(dev.vdmIrqNum, 0 << 3);
		GIC_EnableIRQ(dev.vdmIrqNum);
	}

	if(tile == TILE_73 && control == HAL_X16){
		rhea_pcie_rc_init(&dev, tile, control, C2C_RC);
		if(ret != 0)
			printf("rhea_pcie_rc_init error\n");
		memcpy(&g_pcieDev_73_x16, &dev, sizeof(struct HAL_PCIE_DEV));
		IRQ_SetHandler(dev.vdmIrqNum, get_pcie_irq_func(&dev, tile, control));
		GIC_SetPriority(dev.vdmIrqNum, 0 << 3);
		GIC_EnableIRQ(dev.vdmIrqNum);
	}

	register_lpi(0x0, 1, 8193);  //bugs todo
	IRQ_SetHandler(8193, LPI_8193_Handler);

	register_lpi(4, 0, 8192);
	IRQ_SetHandler(8192, LPI_8192_Handler);
	dw_timer_mbi_tx(1);
	dw_timer_init();
	timer_enable(Timerx6_T2);

#if SEEHI_C2C_PCIE_TEST
	// printf("BSP_PCIE_EP_VDM !!!\n");
	systimer_delay(20, IN_MS);
	while(1){
		printf("BSP_PCIE_RC_LOOP !!! cnt %d\n", cnt);
		cnt++;
#if SEEHI_FPGA_PCIE_TEST
		systimer_delay(5000, IN_MS);
#else
		systimer_delay(50, IN_MS);
#endif
		// dump_regs("vdm int:", dev.apbBase + A510_APB_PCIE_EN_INT0, 16);
		// dump_regs("vdm message:", dev.apbBase + A510_APB_PCIE_MSG_VDM_REG0, 32);
	}
#endif

#if	SEEHI_PLD_PCIE_TEST
	REG32(0x12000fe0)=0x4;
#endif

	while(1) {
		asm volatile ("nop");
	};

	return ret;
}
