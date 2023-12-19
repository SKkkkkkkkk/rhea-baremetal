/*
 * Copyright (c) 2015-2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GICV3_H
#define GICV3_H

/*******************************************************************************
 * GICv3 and 3.1 miscellaneous definitions
 ******************************************************************************/
/* Interrupt group definitions */
#define INTR_GROUP1S		U(0)
#define INTR_GROUP0			U(1)
#define INTR_GROUP1NS		U(2)

/* Interrupt IDs reported by the HPPIR and IAR registers */
#define PENDING_G1S_INTID	U(1020)
#define PENDING_G1NS_INTID	U(1021)

/* Constant to categorize LPI interrupt */
#define MIN_LPI_ID		U(8192)

/* GICv3 can only target up to 16 PEs with SGI */
#define GICV3_MAX_SGI_TARGETS	U(16)

/* PPIs INTIDs 16-31 */
#define MAX_PPI_ID		U(31)

#if GIC_EXT_INTID

/* GICv3.1 extended PPIs INTIDs 1056-1119 */
#define MIN_EPPI_ID		U(1056)
#define MAX_EPPI_ID		U(1119)

/* Total number of GICv3.1 EPPIs */
#define TOTAL_EPPI_INTR_NUM	(MAX_EPPI_ID - MIN_EPPI_ID + U(1))

/* Total number of GICv3.1 PPIs and EPPIs */
#define TOTAL_PRIVATE_INTR_NUM	(TOTAL_PCPU_INTR_NUM + TOTAL_EPPI_INTR_NUM)

/* GICv3.1 extended SPIs INTIDs 4096 - 5119 */
#define MIN_ESPI_ID		U(4096)
#define MAX_ESPI_ID		U(5119)

/* Total number of GICv3.1 ESPIs */
#define TOTAL_ESPI_INTR_NUM	(MAX_ESPI_ID - MIN_ESPI_ID + U(1))

/* Total number of GICv3.1 SPIs and ESPIs */
#define	TOTAL_SHARED_INTR_NUM	(TOTAL_SPI_INTR_NUM + TOTAL_ESPI_INTR_NUM)

/* SGIs: 0-15, PPIs: 16-31, EPPIs: 1056-1119 */
#define	IS_SGI_PPI(id)		(((id) <= MAX_PPI_ID)  || \
				(((id) >= MIN_EPPI_ID) && \
				 ((id) <= MAX_EPPI_ID)))

/* SPIs: 32-1019, ESPIs: 4096-5119 */
#define	IS_SPI(id)		((((id) >= MIN_SPI_ID)  && \
				  ((id) <= MAX_SPI_ID)) || \
				 (((id) >= MIN_ESPI_ID) && \
				  ((id) <= MAX_ESPI_ID)))
#else	/* GICv3 */

/* Total number of GICv3 PPIs */
#define TOTAL_PRIVATE_INTR_NUM	TOTAL_PCPU_INTR_NUM

/* Total number of GICv3 SPIs */
#define	TOTAL_SHARED_INTR_NUM	TOTAL_SPI_INTR_NUM

/* SGIs: 0-15, PPIs: 16-31 */
#define	IS_SGI_PPI(id)		((id) <= MAX_PPI_ID)

/* SPIs: 32-1019 */
#define	IS_SPI(id)		(((id) >= MIN_SPI_ID) && ((id) <= MAX_SPI_ID))

#endif	/* GIC_EXT_INTID */

/*******************************************************************************
 * GICv3 and 3.1 specific Distributor interface register offsets and constants
 ******************************************************************************/
#define GICD_TYPER2		U(0x0c)
#define GICD_STATUSR		U(0x10)
#define GICD_SETSPI_NSR		U(0x40)
#define GICD_CLRSPI_NSR		U(0x48)
#define GICD_SETSPI_SR		U(0x50)
#define GICD_CLRSPI_SR		U(0x58)
#define GICD_IGRPMODR		U(0xd00)
#define GICD_IGROUPRE		U(0x1000)
#define GICD_ISENABLERE		U(0x1200)
#define GICD_ICENABLERE		U(0x1400)
#define GICD_ISPENDRE		U(0x1600)
#define GICD_ICPENDRE		U(0x1800)
#define GICD_ISACTIVERE		U(0x1a00)
#define GICD_ICACTIVERE		U(0x1c00)
#define GICD_IPRIORITYRE	U(0x2000)
#define GICD_ICFGRE		U(0x3000)
#define GICD_IGRPMODRE		U(0x3400)
#define GICD_NSACRE		U(0x3600)
/*
 * GICD_IROUTER<n> register is at 0x6000 + 8n, where n is the interrupt ID
 * and n >= 32, making the effective offset as 0x6100
 */
#define GICD_IROUTER		U(0x6000)
#define GICD_IROUTERE		U(0x8000)

#define GICD_PIDR0_GICV3	U(0xffe0)
#define GICD_PIDR1_GICV3	U(0xffe4)
#define GICD_PIDR2_GICV3	U(0xffe8)

#define IGRPMODR_SHIFT		5

/* GICD_CTLR bit definitions */
#define CTLR_ENABLE_G1NS_SHIFT		1
#define CTLR_ENABLE_G1S_SHIFT		2
#define CTLR_ARE_S_SHIFT		4
#define CTLR_ARE_NS_SHIFT		5
#define CTLR_DS_SHIFT			6
#define CTLR_E1NWF_SHIFT		7
#define GICD_CTLR_RWP_SHIFT		31

#define CTLR_ENABLE_G1NS_MASK		U(0x1)
#define CTLR_ENABLE_G1S_MASK		U(0x1)
#define CTLR_ARE_S_MASK			U(0x1)
#define CTLR_ARE_NS_MASK		U(0x1)
#define CTLR_DS_MASK			U(0x1)
#define CTLR_E1NWF_MASK			U(0x1)
#define GICD_CTLR_RWP_MASK		U(0x1)

#define CTLR_ENABLE_G1NS_BIT		BIT_32(CTLR_ENABLE_G1NS_SHIFT)
#define CTLR_ENABLE_G1S_BIT		BIT_32(CTLR_ENABLE_G1S_SHIFT)
#define CTLR_ARE_S_BIT			BIT_32(CTLR_ARE_S_SHIFT)
#define CTLR_ARE_NS_BIT			BIT_32(CTLR_ARE_NS_SHIFT)
#define CTLR_DS_BIT			BIT_32(CTLR_DS_SHIFT)
#define CTLR_E1NWF_BIT			BIT_32(CTLR_E1NWF_SHIFT)
#define GICD_CTLR_RWP_BIT		BIT_32(GICD_CTLR_RWP_SHIFT)

/* GICD_IROUTER shifts and masks */
#define IROUTER_SHIFT		0
#define IROUTER_IRM_SHIFT	31
#define IROUTER_IRM_MASK	U(0x1)

#define GICV3_IRM_PE		U(0)
#define GICV3_IRM_ANY		U(1)

#define NUM_OF_DIST_REGS	30

/* GICD_TYPER shifts and masks */
#define	TYPER_ESPI		U(1 << 8)
#define	TYPER_DVIS		U(1 << 18)
#define	TYPER_ESPI_RANGE_MASK	U(0x1f)
#define	TYPER_ESPI_RANGE_SHIFT	U(27)
#define	TYPER_ESPI_RANGE	U(TYPER_ESPI_MASK << TYPER_ESPI_SHIFT)

/*******************************************************************************
 * Common GIC Redistributor interface registers & constants
 ******************************************************************************/
#define GICR_V4_PCPUBASE_SHIFT	0x12
#define GICR_V3_PCPUBASE_SHIFT	0x11
#define GICR_SGIBASE_OFFSET	U(65536)	/* 64 KB */
#define GICR_CTLR		U(0x0)
#define GICR_IIDR		U(0x04)
#define GICR_TYPER		U(0x08)
#define GICR_STATUSR		U(0x10)
#define GICR_WAKER		U(0x14)
#define GICR_PROPBASER		U(0x70)
#define GICR_PENDBASER		U(0x78)
#define GICR_IGROUPR0		(GICR_SGIBASE_OFFSET + U(0x80))
#define GICR_ISENABLER0		(GICR_SGIBASE_OFFSET + U(0x100))
#define GICR_ICENABLER0		(GICR_SGIBASE_OFFSET + U(0x180))
#define GICR_ISPENDR0		(GICR_SGIBASE_OFFSET + U(0x200))
#define GICR_ICPENDR0		(GICR_SGIBASE_OFFSET + U(0x280))
#define GICR_ISACTIVER0		(GICR_SGIBASE_OFFSET + U(0x300))
#define GICR_ICACTIVER0		(GICR_SGIBASE_OFFSET + U(0x380))
#define GICR_IPRIORITYR		(GICR_SGIBASE_OFFSET + U(0x400))
#define GICR_ICFGR0		(GICR_SGIBASE_OFFSET + U(0xc00))
#define GICR_ICFGR1		(GICR_SGIBASE_OFFSET + U(0xc04))
#define GICR_IGRPMODR0		(GICR_SGIBASE_OFFSET + U(0xd00))
#define GICR_NSACR		(GICR_SGIBASE_OFFSET + U(0xe00))

#define GICR_IGROUPR		GICR_IGROUPR0
#define GICR_ISENABLER		GICR_ISENABLER0
#define GICR_ICENABLER		GICR_ICENABLER0
#define GICR_ISPENDR		GICR_ISPENDR0
#define GICR_ICPENDR		GICR_ICPENDR0
#define GICR_ISACTIVER		GICR_ISACTIVER0
#define GICR_ICACTIVER		GICR_ICACTIVER0
#define GICR_ICFGR		GICR_ICFGR0
#define GICR_IGRPMODR		GICR_IGRPMODR0

/* GICR_CTLR bit definitions */
#define GICR_CTLR_UWP_SHIFT	31
#define GICR_CTLR_UWP_MASK	U(0x1)
#define GICR_CTLR_UWP_BIT	BIT_32(GICR_CTLR_UWP_SHIFT)
#define GICR_CTLR_RWP_SHIFT	3
#define GICR_CTLR_RWP_MASK	U(0x1)
#define GICR_CTLR_RWP_BIT	BIT_32(GICR_CTLR_RWP_SHIFT)
#define GICR_CTLR_EN_LPIS_BIT	BIT_32(0)

/* GICR_WAKER bit definitions */
#define WAKER_CA_SHIFT		2
#define WAKER_PS_SHIFT		1

#define WAKER_CA_MASK		U(0x1)
#define WAKER_PS_MASK		U(0x1)

#define WAKER_CA_BIT		BIT_32(WAKER_CA_SHIFT)
#define WAKER_PS_BIT		BIT_32(WAKER_PS_SHIFT)

/* GICR_TYPER bit definitions */
#define TYPER_AFF_VAL_SHIFT	32
#define TYPER_PROC_NUM_SHIFT	8
#define TYPER_LAST_SHIFT	4
#define TYPER_VLPI_SHIFT	1

#define TYPER_AFF_VAL_MASK	U(0xffffffff)
#define TYPER_PROC_NUM_MASK	U(0xffff)
#define TYPER_LAST_MASK		U(0x1)

#define TYPER_LAST_BIT		BIT_32(TYPER_LAST_SHIFT)
#define TYPER_VLPI_BIT		BIT_32(TYPER_VLPI_SHIFT)

#define TYPER_PPI_NUM_SHIFT	U(27)
#define TYPER_PPI_NUM_MASK	U(0x1f)

/* GICR_IIDR bit definitions */
#define IIDR_PRODUCT_ID_MASK	U(0xff000000)
#define IIDR_VARIANT_MASK	U(0x000f0000)
#define IIDR_REVISION_MASK	U(0x0000f000)
#define IIDR_IMPLEMENTER_MASK	U(0x00000fff)
#define IIDR_MODEL_MASK		(IIDR_PRODUCT_ID_MASK | \
				 IIDR_IMPLEMENTER_MASK)

/*******************************************************************************
 * GICv3 and 3.1 CPU interface registers & constants
 ******************************************************************************/
/* ICC_SRE bit definitions */
#define ICC_SRE_EN_BIT		BIT_32(3)
#define ICC_SRE_DIB_BIT		BIT_32(2)
#define ICC_SRE_DFB_BIT		BIT_32(1)
#define ICC_SRE_SRE_BIT		BIT_32(0)

/* ICC_IGRPEN1_EL3 bit definitions */
#define IGRPEN1_EL3_ENABLE_G1NS_SHIFT	0
#define IGRPEN1_EL3_ENABLE_G1S_SHIFT	1

#define IGRPEN1_EL3_ENABLE_G1NS_BIT	BIT_32(IGRPEN1_EL3_ENABLE_G1NS_SHIFT)
#define IGRPEN1_EL3_ENABLE_G1S_BIT	BIT_32(IGRPEN1_EL3_ENABLE_G1S_SHIFT)

/* ICC_IGRPEN0_EL1 bit definitions */
#define IGRPEN1_EL1_ENABLE_G0_SHIFT	0
#define IGRPEN1_EL1_ENABLE_G0_BIT	BIT_32(IGRPEN1_EL1_ENABLE_G0_SHIFT)

/* ICC_HPPIR0_EL1 bit definitions */
#define HPPIR0_EL1_INTID_SHIFT		0
#define HPPIR0_EL1_INTID_MASK		U(0xffffff)

/* ICC_HPPIR1_EL1 bit definitions */
#define HPPIR1_EL1_INTID_SHIFT		0
#define HPPIR1_EL1_INTID_MASK		U(0xffffff)

/* ICC_IAR0_EL1 bit definitions */
#define IAR0_EL1_INTID_SHIFT		0
#define IAR0_EL1_INTID_MASK		U(0xffffff)

/* ICC_IAR1_EL1 bit definitions */
#define IAR1_EL1_INTID_SHIFT		0
#define IAR1_EL1_INTID_MASK		U(0xffffff)

/* ICC SGI macros */
#define SGIR_TGT_MASK			ULL(0xffff)
#define SGIR_AFF1_SHIFT			16
#define SGIR_INTID_SHIFT		24
#define SGIR_INTID_MASK			ULL(0xf)
#define SGIR_AFF2_SHIFT			32
#define SGIR_IRM_SHIFT			40
#define SGIR_IRM_MASK			ULL(0x1)
#define SGIR_AFF3_SHIFT			48
#define SGIR_AFF_MASK			ULL(0xf)

#define SGIR_IRM_TO_AFF			U(0)

#define GICV3_SGIR_VALUE(_aff3, _aff2, _aff1, _intid, _irm, _tgt)	\
	((((uint64_t) (_aff3) & SGIR_AFF_MASK) << SGIR_AFF3_SHIFT) |	\
	 (((uint64_t) (_irm) & SGIR_IRM_MASK) << SGIR_IRM_SHIFT) |	\
	 (((uint64_t) (_aff2) & SGIR_AFF_MASK) << SGIR_AFF2_SHIFT) |	\
	 (((_intid) & SGIR_INTID_MASK) << SGIR_INTID_SHIFT) |		\
	 (((_aff1) & SGIR_AFF_MASK) << SGIR_AFF1_SHIFT) |		\
	 ((_tgt) & SGIR_TGT_MASK))

/*****************************************************************************
 * GICv3 and 3.1 ITS registers and constants
 *****************************************************************************/
#define GITS_CTLR			U(0x0)
#define GITS_IIDR			U(0x4)
#define GITS_TYPER			U(0x8)
#define GITS_CBASER			U(0x80)
#define GITS_CWRITER			U(0x88)
#define GITS_CREADR			U(0x90)
#define GITS_BASER			U(0x100)

/* GITS_CTLR bit definitions */
#define GITS_CTLR_ENABLED_BIT		BIT_32(0)
#define GITS_CTLR_QUIESCENT_BIT		BIT_32(1)

#define GITS_TYPER_VSGI			BIT_64(39)

/* GIC-600 specific register offsets */
#define GICR_PWRR			0x24U

/* GICR_PWRR fields */
#define PWRR_RDPD_SHIFT			0
#define PWRR_RDAG_SHIFT			1
#define PWRR_RDGPD_SHIFT		2
#define PWRR_RDGPO_SHIFT		3

#define PWRR_RDPD			(1U << PWRR_RDPD_SHIFT)
#define PWRR_RDAG			(1U << PWRR_RDAG_SHIFT)
#define PWRR_RDGPD			(1U << PWRR_RDGPD_SHIFT)
#define PWRR_RDGPO			(1U << PWRR_RDGPO_SHIFT)

/*
 * Values to write to GICR_PWRR register to power redistributor
 * for operating through the core (GICR_PWRR.RDAG = 0)
 */
#define PWRR_ON				(0U << PWRR_RDPD_SHIFT)
#define PWRR_OFF			(1U << PWRR_RDPD_SHIFT)


/*******************************************************************************
 * GIC500/GIC600 Re-distributor interface registers & constants
 ******************************************************************************/

/* GICR_WAKER implementation-defined bit definitions */
#define	WAKER_SL_SHIFT		0
#define	WAKER_QSC_SHIFT		31

#define WAKER_SL_BIT		(1U << WAKER_SL_SHIFT)
#define WAKER_QSC_BIT		(1U << WAKER_QSC_SHIFT)

#define IIDR_MODEL_ARM_GIC_600		U(0x0200043b)
#define IIDR_MODEL_ARM_GIC_600AE	U(0x0300043b)
#define IIDR_MODEL_ARM_GIC_700		U(0x0400043b)

#define PIDR_COMPONENT_ARM_DIST		U(0x492)
#define PIDR_COMPONENT_ARM_REDIST	U(0x493)
#define PIDR_COMPONENT_ARM_ITS		U(0x494)


#endif /* GICV3_H */
