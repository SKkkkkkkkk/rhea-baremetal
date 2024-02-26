#ifndef __GICV3_V4_REGS_H__
#define __GICV3_V4_REGS_H__

// Only support GICv3 for now!

#include <stdint.h>
#include "common.h"
#include "memmap.h"

/** \brief  Structure type to access the Generic Interrupt Controller Distributor (GICD)
*/
typedef struct
{
	__IOM uint32_t CTLR;                 /*!< \brief  Offset: 0x000 (R/W) Distributor Control Register */
	__IM  uint32_t TYPER;                /*!< \brief  Offset: 0x004 (R/ ) Interrupt Controller Type Register */
	__IM  uint32_t IIDR;                 /*!< \brief  Offset: 0x008 (R/ ) Distributor Implementer Identification Register */
	__IM  uint32_t GICD_TYPER2;           // +0x000C - RO - Interrupt controller Type Register 2
	__IOM uint32_t STATUSR;              /*!< \brief  Offset: 0x010 (R/W) Error Reporting Status Register, optional */
		RESERVED(0[11], uint32_t)
	__OM  uint32_t SETSPI_NSR;           /*!< \brief  Offset: 0x040 ( /W) Set SPI Register */
		RESERVED(1, uint32_t)
	__OM  uint32_t CLRSPI_NSR;           /*!< \brief  Offset: 0x048 ( /W) Clear SPI Register */
		RESERVED(2, uint32_t)
	__OM  uint32_t SETSPI_SR;            /*!< \brief  Offset: 0x050 ( /W) Set SPI, Secure Register */
		RESERVED(3, uint32_t)
	__OM  uint32_t CLRSPI_SR;            /*!< \brief  Offset: 0x058 ( /W) Clear SPI, Secure Register */
		RESERVED(4[9], uint32_t)
	__IOM uint32_t IGROUPR[32];          /*!< \brief  Offset: 0x080 (R/W) Interrupt Group Registers */
	__IOM uint32_t ISENABLER[32];        /*!< \brief  Offset: 0x100 (R/W) Interrupt Set-Enable Registers */
	__IOM uint32_t ICENABLER[32];        /*!< \brief  Offset: 0x180 (R/W) Interrupt Clear-Enable Registers */
	__IOM uint32_t ISPENDR[32];          /*!< \brief  Offset: 0x200 (R/W) Interrupt Set-Pending Registers */
	__IOM uint32_t ICPENDR[32];          /*!< \brief  Offset: 0x280 (R/W) Interrupt Clear-Pending Registers */
	__IOM uint32_t ISACTIVER[32];        /*!< \brief  Offset: 0x300 (R/W) Interrupt Set-Active Registers */
	__IOM uint32_t ICACTIVER[32];        /*!< \brief  Offset: 0x380 (R/W) Interrupt Clear-Active Registers */
	__IOM uint8_t  IPRIORITYR[1024];      /*!< \brief  Offset: 0x400 (R/W) Interrupt Priority Registers */
	__IOM uint32_t ITARGETSR[256];      /*!< \brief  Offset: 0x800 (R/W) Interrupt Targets Registers */
	__IOM uint32_t ICFGR[64];            /*!< \brief  Offset: 0xC00 (R/W) Interrupt Configuration Registers */
	__IOM uint32_t IGRPMODR[32];         /*!< \brief  Offset: 0xD00 (R/W) Interrupt Group Modifier Registers */
		RESERVED(5[32], uint32_t)
	__IOM uint32_t NSACR[64];            /*!< \brief  Offset: 0xE00 (R/W) Non-secure Access Control Registers */
	__OM  uint32_t SGIR;                 /*!< \brief  Offset: 0xF00 ( /W) Software Generated Interrupt Register */
		RESERVED(6[3], uint32_t)
	__IOM uint32_t CPENDSGIR[4];         /*!< \brief  Offset: 0xF10 (R/W) SGI Clear-Pending Registers */
	__IOM uint32_t SPENDSGIR[4];         /*!< \brief  Offset: 0xF20 (R/W) SGI Set-Pending Registers */
		RESERVED(7[5236], uint32_t)
	__IOM uint64_t IROUTER[988];         /*!< \brief  Offset: 0x6100(R/W) Interrupt Routing Registers */
		RESERVED(8[4104], uint32_t)
	__IOM uint32_t CHIPSR;			   /*!< \brief  Offset: 0xC000 (R/W) Chip Specific Control Register */
	__IOM uint32_t DCHIPR;			   /*!< \brief  Offset: 0xC004 (R/W) Default Chip Register */
	__IOM uint64_t CHIPR[16];			   /*!< \brief  Offset: 0xC008 (R/W) Chip Registers */
		RESERVED(9[2016], uint32_t)
	__IOM uint32_t ICLAR[62];           /*!< \brief  Offset: 0xE008 (R/W) Interrupt Class Registers */
		RESERVED(10[2], uint32_t)
	__IOM uint32_t IERRR[30];           /*!< \brief  Offset: 0xE108 (R/W) Interrupt Error Registers */
		RESERVED(11[928], uint32_t)
	__IM  uint64_t CFGID;                /*!< \brief  Offset: 0xF000 (R/ ) Configuration ID Register */
		RESERVED(12[1010], uint32_t)
	__IM  uint32_t PIDR4;                /*!< \brief  Offset: 0xFFD0 (R/ ) Peripheral ID4 Register */
	__IM  uint32_t PIDR5;                /*!< \brief  Offset: 0xFFD4 (R/ ) Peripheral ID5 Register */
	__IM  uint32_t PIDR6;                /*!< \brief  Offset: 0xFFD8 (R/ ) Peripheral ID6 Register */
	__IM  uint32_t PIDR7;                /*!< \brief  Offset: 0xFFDC (R/ ) Peripheral ID7 Register */
	__IM  uint32_t PIDR0;                /*!< \brief  Offset: 0xFFE0 (R/ ) Peripheral ID0 Register */
	__IM  uint32_t PIDR1;                /*!< \brief  Offset: 0xFFE4 (R/ ) Peripheral ID1 Register */
	__IM  uint32_t PIDR2;                /*!< \brief  Offset: 0xFFE8 (R/ ) Peripheral ID2 Register */
	__IM  uint32_t PIDR3;                /*!< \brief  Offset: 0xFFEC (R/ ) Peripheral ID3 Register */
	__IM  uint32_t CIDR0;                /*!< \brief  Offset: 0xFFF0 (R/ ) Component ID0 Register */
	__IM  uint32_t CIDR1;                /*!< \brief  Offset: 0xFFF4 (R/ ) Component ID1 Register */
	__IM  uint32_t CIDR2;                /*!< \brief  Offset: 0xFFF8 (R/ ) Component ID2 Register */
	__IM  uint32_t CIDR3;                /*!< \brief  Offset: 0xFFFC (R/ ) Component ID3 Register */
}  GICDistributor_Type;

#ifdef QEMU
#define GICDistributor      ((GICDistributor_Type      *)     VIRT_GIC_DIST ) /*!< \brief GIC Distributor register set access pointer */
#else
#define GICDistributor      ((GICDistributor_Type      *)     (GIC600_BASE) ) /*!< \brief GIC Distributor register set access pointer */
#endif
/** \brief  Structure type to access the Generic Interrupt Controller ReDistributor RD_base (GICR_LPI)
*/
typedef struct
{
    __IOM uint32_t CTLR;            /*!< \brief  Offset: 0x000 (R/W) ReDistributor Control Register */
    __IM  uint32_t IIDR;            /*!< \brief  Offset: 0x004 (R/ ) ReDistributor Implementer Identification Register */
    __IM  uint32_t TYPER[2];        /*!< \brief  Offset: 0x008 (R/ ) ReDistributor Type Register */
    __IOM uint32_t STATUSR;         /*!< \brief  Offset: 0x010 (R/W) Error Reporting Status Register, optional */
    __IOM uint32_t WAKER;           /*!< \brief  Offset: 0x014 (R/W) Redistributor Wake Register */
    __IM  uint32_t MPAMIDR;         /*!< \brief  Offset: 0x018 (R/ ) Report maximum PARTID and PMG Register */
    __IOM uint32_t PARTIDR;         /*!< \brief  Offset: 0x01C (R/W) Set PARTID and PMG Register */
	__IOM uint32_t FCTLR;			/*!< \brief  Offset: 0x020 (R/W) Redistributor Function Control Register */
	__IOM uint32_t PWRR;			/*!< \brief  Offset: 0x024 (R/W) Redistributor Power Register */
	__IOM uint32_t CLASS;			/*!< \brief  Offset: 0x028 (R/W) Secure-only register */
        RESERVED(0[5], uint32_t)
    __IOM uint64_t SETLPIR;         /*!< \brief  Offset: 0x040 ( /W) Set LPI Pending Register */
    __IOM uint64_t CLRLPIR;         /*!< \brief  Offset: 0x048 ( /W) Clear LPI Pending Register */
        RESERVED(1[8], uint32_t)
    __IOM uint64_t PROPBASER;       /*!< \brief  Offset: 0x070 (R/W) Redistributor Properties Base Address Register */
    __IOM uint64_t PENDBASER;       /*!< \brief  Offset: 0x078 (R/W) Redistributor LPI Pending Table Base Address Register */
        RESERVED(2[8], uint32_t)
    __OM  uint64_t INVLPIR;         /*!< \brief  Offset: 0x0A0 ( /W) Redistributor Invalidate LPI Register */
        RESERVED(3[2], uint32_t)
    __OM  uint64_t INVALLR;         /*!< \brief  Offset: 0x0B0 ( /W) Redistributor Invalidate All Register */
        RESERVED(4[2], uint32_t)
    __IM  uint32_t SYNCR;           /*!< \brief  Offset: 0x0C0 (R/ ) Redistributor Synchronize Register */
        RESERVED(5[16335], uint32_t)
}  GICRedistributor_LPI_Type;

/** \brief  Structure type to access the Generic Interrupt Controller ReDistributor SGI_base (GICR_SGI)
*/
typedef struct
{
	RESERVED(0[32], uint32_t)
	__IOM uint32_t IGROUPR0;          /*!< \brief  Offset: 0x080 (R/W) Interrupt Group Register 0 */
	__IOM uint32_t IGROUPRE[2];	  	  /*!< \brief  Offset: 0x084 (R/W) Interrupt Group Registers for extended PPI range */
	RESERVED(1[29], uint32_t)
	__IOM uint32_t ISENABLER0;        /*!< \brief  Offset: 0x100 (R/W) Interrupt Set-Enable Register 0 */
	__IOM uint32_t ISENABLERE[2];     /*!< \brief  Offset: 0x104 (R/W) Interrupt Set-Enable for extended PPI range */
	RESERVED(2[29], uint32_t)
	__IOM uint32_t ICENABLER0;        /*!< \brief  Offset: 0x180 (R/W) Interrupt Clear-Enable Register 0 */
	__IOM uint32_t ICENABLERE[2];     /*!< \brief  Offset: 0x184 (R/W) Interrupt Clear-Enable for extended PPI range */
	RESERVED(3[29], uint32_t)
	__IOM uint32_t ISPENDR0;          /*!< \brief  Offset: 0x200 (R/W) Interrupt Set-Pend Register 0 */
	__IOM uint32_t ISPENDRE[2];       /*!< \brief  Offset: 0x204 (R/W) Interrupt Set-Pend for extended PPI range */
	RESERVED(4[29], uint32_t)
	__IOM uint32_t ICPENDR0;          /*!< \brief  Offset: 0x280 (R/W) Interrupt Clear-Pend Register 0 */
	__IOM uint32_t ICPENDRE[2];       /*!< \brief  Offset: 0x284 (R/W) Interrupt Clear-Pend for extended PPI range */
	RESERVED(5[29], uint32_t)
	__IOM uint32_t ISACTIVER0;        /*!< \brief  Offset: 0x300 (R/W) Interrupt Set-Active Register 0 */
	__IOM uint32_t ISACTIVERE[2];     /*!< \brief  Offset: 0x304 (R/W) Interrupt Set-Active for extended PPI range */
	RESERVED(6[29], uint32_t)
	__IOM uint32_t ICACTIVER0;        /*!< \brief  Offset: 0x380 (R/W) Interrupt Clear-Active Register 0 */
	__IOM uint32_t ICACTIVERE[2];     /*!< \brief  Offset: 0x384 (R/W) Interrupt Clear-Active for extended PPI range */
	RESERVED(7[29], uint32_t)
	__IOM uint8_t  IPRIORITYR[32];    /*!< \brief  Offset: 0x400 (R/W) Interrupt Priority Registers */
	__IOM uint8_t  IPRIORITYRE[64];   /*!< \brief  Offset: 0x420 (R/W) Interrupt Priority for extended PPI range */
	RESERVED(8[488], uint32_t)
	__IOM uint32_t ICFGR[2];            /*!< \brief  Offset: 0xC00 (R/W) SGI Configuration Register */
	__IOM uint32_t ICFGRE[4];         /*!< \brief  Offset: 0xC08 (R/W) Extended PPI Configuration Register */
	RESERVED(9[58], uint32_t)
	__IOM uint32_t IGRPMODR0;         /*!< \brief  Offset: 0xD00 (R/W) Interrupt Group Modifier Register 0 */
	__IOM uint32_t IGRPMODRE[2];      /*!< \brief  Offset: 0xD04 (R/W) Interrupt Group Modifier for extended PPI range */
	RESERVED(10[61], uint32_t)
	__IOM uint32_t NSACR;             /*!< \brief  Offset: 0xE00 (R/W) Non-Secure Access Control Register */
	RESERVED(11[95], uint32_t)
	__IOM uint32_t INMIR0;            /*!< \brief  Offset: 0xF80 (R/W) Non-maskable Interrupt Register for PPIs */
	__IOM uint32_t INMIRE[31];        /*!< \brief  Offset: 0xF84 (R/W) Non-maskable Interrupt Registers for Extended PPIs */
	RESERVED(12[15360], uint32_t)
} GICRedistributor_SGI_Type;

/** \brief  Structure type to access the Generic Interrupt Controller ReDistributor VLPI_base (GICR_VLPI)
*/
typedef struct
{
	RESERVED(0[28], uint32_t)
	__IOM uint64_t VPROPBASER;         /*!< \brief  Offset: 0x070 (R/W) Redistributor Properties Base Address Register */
	__IOM uint64_t VPENDBASER;         /*!< \brief  Offset: 0x078 (R/W) Redistributor LPI Pending Table Base Address Register */
	__OM  uint32_t VSGIR;              /*!< \brief  Offset: 0x080 ( /W) Software Generated Interrupt Register */
	RESERVED(1[2], uint32_t)
	__IM  uint32_t VSGIPENDR;          /*!< \brief  Offset: 0x088 (R/ ) SGI Pending Register */
	RESERVED(2[16349], uint32_t)
} GICRedistributor_VLPI_Type;

/** \brief  Structure type to access the Generic Interrupt Controller ReDistributor Reserved (GICR_RES)
*/
typedef struct
{
	RESERVED(0[16384], uint32_t)
} GICRedistributor_RES_Type;


typedef struct
{
	GICRedistributor_LPI_Type RD_base;
	GICRedistributor_SGI_Type SGI_base;
#ifdef GICV4
	GICRedistributor_VLPI_Type VLPI_base;
	GICRedistributor_RES_Type RES_base;
#endif
} GICRedistributor_Type;

#ifdef QEMU
#define GICRedistributor      ((GICRedistributor_Type      *)     VIRT_GIC_REDIST ) /*!< \brief GIC ReDistributor register set access pointer */
#else
#define ITScount 1
#define RDcount  4
#define GICRedistributor      ((GICRedistributor_Type      *)     (GIC600_BASE + ((4 + (2 * ITScount)) << 16))) /*!< \brief GIC ReDistributor register set access pointer */
#endif

extern uint32_t getAffinity(void);
// #define CURRENT_GICR GICRedistributor[getAffinity()]

// +0 from ITS_BASE
// struct GICv3_its_ctlr_if
// {
// 		__IOM uint32_t GITS_CTLR;             // +0x0000 - RW - ITS Control Register
//   __IM uint32_t GITS_IIDR;             // +0x0004 - RO - Implementer Identification Register
//   __IM uint64_t GITS_TYPER;            // +0x0008 - RO - ITS Type register
//   __IM uint32_t GITS_MPAMIDR;          // +0x0010 - RO - Reports maxmimum PARTID and PMG (GICv3.1)
// 		__IOM uint32_t GITS_PARTIDR;          // +0x0004 - RW - Sets the PARTID and PMG used for ITS memory accesses (GICv3.1)
//   __IM uint32_t GITS_MPIDR;            // +0x0018 - RO - ITS affinity, used for shared vPE table
//   __IM uint32_t padding5;              // +0x001C - RESERVED
// 		__IOM uint32_t GITS_IMPDEF[8];        // +0x0020 - RW - IMP DEF registers
//   __IM uint32_t padding2[16];          // +0x0040 - RESERVED
// 		__IOM uint64_t GITS_CBASER;           // +0x0080 - RW - Sets base address of ITS command queue
// 		__IOM uint64_t GITS_CWRITER;          // +0x0088 - RW - Points to next enrty to add command
// 		__IOM uint64_t GITS_CREADR;           // +0x0090 - RW - Points to command being processed
//   __IM uint32_t padding3[2];           // +0x0098 - RESERVED
//   __IM uint32_t padding4[24];          // +0x00A0 - RESERVED
// 		__IOM uint64_t GITS_BASER[8];         // +0x0100 - RW - Sets base address of Device and Collection tables
// };

// // +0x010000 from ITS_BASE
// struct GICv3_its_int_if
// {
//   __IM uint32_t padding1[16];          // +0x0000 - RESERVED
// 		__IOM uint32_t GITS_TRANSLATER;       // +0x0040 - RW - Written by peripherals to generate LPI
// };

// // +0x020000 from ITS_BASE
// struct GICv3_its_sgi_if
// {
//   __IM uint32_t padding1[8];           // +0x0000 - RESERVED
// 		__IOM uint64_t GITS_SGIR;             // +0x0020 - RW - Written by peripherals to generate vSGI (GICv4.1)
// };

#endif

// ----------------------------------------------------------
// End of gicv3_regs.h
// ----------------------------------------------------------
