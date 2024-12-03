#ifndef __PCIE_H__
#define __PCIE_H__

#include "memmap.h"

/***************************** MACRO Definition ******************************/
/** @defgroup PCIE_Exported_Definition_Group1 Basic Definition
 *  @{
 */

/***************************** Structure Definition **************************/

#define PHY_MODE_PCIE_AGGREGATION 4       /**< PCIE3x4 */
#define PHY_MODE_PCIE_NANBNB      0       /**< P1:PCIE3x2  +  P0:PCIE3x2 */
#define PHY_MODE_PCIE_NANBBI      1       /**< P1:PCIE3x2  +  P0:PCIE3x1*2 */
#define PHY_MODE_PCIE_NABINB      2       /**< P1:PCIE3x1*2 + P0:PCIE3x2 */
#define PHY_MODE_PCIE_NABIBI      3       /**< P1:PCIE3x1*2 + P0:PCIE3x1*2 */

#define PCIE_ATU_REGION_INDEX1 (0x1 << 0)
#define PCIE_ATU_REGION_INDEX0 (0x0 << 0)
#define PCIE_ATU_TYPE_MEM      (0x0 << 0)
#define PCIE_ATU_TYPE_IO       (0x2 << 0)
#define PCIE_ATU_TYPE_CFG0     (0x4 << 0)
#define PCIE_ATU_TYPE_CFG1     (0x5 << 0)

#define PCI_VENDOR_ID 0x00  /* 16 bits */
#define PCI_DEVICE_ID 0x02  /* 16 bits */

#define PCI_CLASS_REVISION 0x08 /* High 24 bits are class, low 8 revision */
#define PCI_CLASS_DEVICE   0x0a /* Device class */
#define PCI_CLASS_CODE     0x0b /* Device class code */

typedef int32_t HAL_PCI_DevT;

#define PCI_BUS(d)              (((d) >> 16) & 0xff)
#define PCI_DEVFN(d, f)         ((d) << 11 | (f) << 8)
#define PCI_MASK_BUS(bdf)       ((bdf) & 0xffff)
#define PCI_ADD_BUS(bus, devfn) (((bus) << 16) | (devfn))
#define PCI_BDF(b, d, f)        ((b) << 16 | PCI_DEVFN(d, f))

/**
 * @brief PCI size type
 */
typedef enum {
	PCI_SIZE_8,
	PCI_SIZE_16,
	PCI_SIZE_32,
} ePCI_Size;

/** HAL boolean type definition */
typedef enum {
	HAL_FALSE = 0x00U,
	HAL_TRUE  = 0x01U
} HAL_Check;

/** HAL error code definition */
typedef enum {
	HAL_OK      = 0x00U,
	HAL_ERROR   = (-1),
	HAL_BUSY    = (-16),
	HAL_NODEV   = (-19),
	HAL_INVAL   = (-22),
	HAL_NOSYS   = (-38),
	HAL_TIMEOUT = (-110)
} HAL_Status;

/** HAL functional status definition */
typedef enum {
	HAL_DISABLE = 0x00U,
	HAL_ENABLE  = 0x01U
} HAL_FuncStatus;

/** HAL lock structures definition */
typedef enum {
	HAL_UNLOCKED = 0x00U,
	HAL_LOCKED   = 0x01U
} HAL_LockStatus;


struct HAL_PHY_SNPS_PCIE3_DEV {
	uint32_t phyMode;
};

/** PCIe handler */
struct HAL_PCIE_DEV {
	uint64_t dbiBase;
	uint64_t engineBase;
	uint64_t apbBase;
	uint64_t cfgBase;
	uint64_t phyBase;
	uint64_t ssBase;
	uint64_t drouterBase;
	uint64_t crouterBase;
	uint64_t dniuBase;
	uint64_t cniuBase;
	uint64_t mbitxBase;
	uint8_t max_lanes;
	uint8_t lanes;
	uint8_t gen;
	uint8_t firstBusNo;
	uint32_t ltrIrqNum;
	uint32_t vdmIrqNum;
	void *phy;
};

struct HAL_PCIE_HANDLE {
	struct HAL_PCIE_DEV *dev;
};
/** @} */
/***************************** Function Declare ******************************/
/** @defgroup PCIE_Public_Function_Declare Public Function Declare
 *  @{
 */

// int HAL_PCIE_GetDmaStatus(struct HAL_PCIE_HANDLE *pcie, uint8_t chn, enum HAL_PCIE_DMA_DIR dir);
// HAL_Status HAL_PCIE_ConfigDma(struct HAL_PCIE_HANDLE *pcie, struct DMA_TABLE *table);
// HAL_Status HAL_PCIE_StartDma(struct HAL_PCIE_HANDLE *pcie, struct DMA_TABLE *table);
HAL_Check HAL_PCIE_LinkUp(struct HAL_PCIE_HANDLE *pcie);
uint32_t HAL_PCIE_GetLTSSM(struct HAL_PCIE_HANDLE *pcie);
HAL_Status HAL_PCIE_Init(struct HAL_PCIE_HANDLE *pcie, struct HAL_PCIE_DEV *dev);
HAL_Status HAL_PCIE_DeInit(struct HAL_PCIE_HANDLE *pcie);
HAL_Status HAL_PCIE_InboundConfig(struct HAL_PCIE_HANDLE *pcie, int32_t index, int32_t bar, uint64_t cpuAddr);
HAL_Status HAL_PCIE_InboundConfig_addr(struct HAL_PCIE_HANDLE *pcie, int32_t index, int type, uint64_t cpuAddr, uint64_t busAddr, uint64_t size);
HAL_Status HAL_PCIE_OutboundConfig(struct HAL_PCIE_HANDLE *pcie, int32_t index, int type, uint64_t cpuAddr, uint64_t busAddr, uint64_t size);
HAL_Status dw_pcie_prog_inbound_atu(struct HAL_PCIE_HANDLE *pcie, int32_t index, int32_t bar, uint64_t cpuAddr);
HAL_Status dw_pcie_prog_outbound_atu(struct HAL_PCIE_HANDLE *pcie, int32_t index, int type, uint64_t cpuAddr, uint64_t busAddr, uint64_t size);
int32_t HAL_PCIE_OutboundConfigCFG0(struct HAL_PCIE_HANDLE *pcie, HAL_PCI_DevT bdf, uint32_t size);

#endif // __PCIE_H__
