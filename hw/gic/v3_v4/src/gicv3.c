#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "gicv3_regs.h"
#include "gic_common.h"
#include "_gicv3.h"
#include "arch_helpers.h"
#include "gicv3.h"

static uint8_t current_gicr_index = 0xff;
#define CURRENT_GICR (GICRedistributor[current_gicr_index])
static uint16_t max_spi_id = 0;

#define IRQ_GIC_LINE_COUNT      (1020U)
static IRQHandler_t IRQTable[IRQ_GIC_LINE_COUNT] = { 0U };

static inline void gic600_wait_group_not_in_transit()
{
	assert(current_gicr_index != 0xff);
	uint32_t pwrr;
	do {
		pwrr = CURRENT_GICR.RD_base.PWRR;
	/* Check group not transitioning: RDGPD == RDGPO */
	} while (((pwrr & PWRR_RDGPD) >> PWRR_RDGPD_SHIFT) !=
		 ((pwrr & PWRR_RDGPO) >> PWRR_RDGPO_SHIFT));
}

/*
 * Wait for updates to:
 * GICR_ICENABLER0
 * GICR_CTLR.DPG1S
 * GICR_CTLR.DPG1NS
 * GICR_CTLR.DPG0
 * GICR_CTLR, which clears EnableLPIs from 1 to 0
 */
static inline void gicr_wait_for_pending_write()
{
	assert(current_gicr_index != 0xff);
	while((CURRENT_GICR.RD_base.CTLR & GICR_CTLR_RWP_BIT) != 0U);
}

static inline void gic600_pwr_on()
{
	assert(current_gicr_index != 0xff);
	do {	
		/* Wait until group not transitioning */
		gic600_wait_group_not_in_transit();

		/* Power on redistributor */
		CURRENT_GICR.RD_base.PWRR = PWRR_ON;

		/*
		* Wait until the power on state is reflected.
		* If RDPD == 0 then powered on.
		*/
	} while ((CURRENT_GICR.RD_base.PWRR & PWRR_RDPD) != PWRR_ON);
}

static inline void gicv3_rdistif_mark_core_awake()
{
	/* 
	** Mard the connected PE as awake.
	** Must be done, before we can access any of the GICR registers.
	*/
	uint32_t waker = CURRENT_GICR.RD_base.WAKER;
	/*
	** Changing ProcessorSleep from 1 to 0 when ChildrenAsleep is not 1 results in UNPREDICTABLE behavior.
	*/
	assert((waker & WAKER_CA_BIT) != 0U);
	if(waker & WAKER_QSC_BIT)
	{
		/*
		** GICR_WAKER.ProcessorSleep can only be set to zero when:
		** — GICR_WAKER.Sleep bit[0] == 0.
		** — GICR_WAKER.Quiescent bit[31] == 0.
		*/
		waker &= ~WAKER_SL_BIT;
		CURRENT_GICR.RD_base.WAKER = waker;
		while((CURRENT_GICR.RD_base.WAKER & WAKER_QSC_BIT) != 0U);
	}
	waker &= ~WAKER_PS_BIT;
	CURRENT_GICR.RD_base.WAKER = waker;
	while((CURRENT_GICR.RD_base.WAKER & WAKER_CA_BIT) != 0U);
}

static void GIC_Discovery()
{
	// must support system register access
	assert((read_id_aa64pfr0_el1() & (ID_AA64PFR0_GIC_MASK << ID_AA64PFR0_GIC_SHIFT)) != 0U);

	uint8_t gic_version = (GICDistributor->PIDR2>>PIDR2_ARCH_REV_SHIFT) & PIDR2_ARCH_REV_MASK;
	// uint8_t gicv2_compat = (GICDistributor->CTLR>>CTLR_ARE_S_SHIFT) & CTLR_ARE_S_MASK;

	assert(gic_version == 0x3);
	(void)gic_version; // Silence unused variable warning
	// printf("GICv%u with%s legacy support detected.\n", gic_version,
	// 				(gicv2_compat == 0U) ? "" : "out");

	uint8_t index = 0;
	do 
	{
		// printf("GICR[%u] with Affinity: 0x%x\n", index, GICRedistributor[index].RD_base.TYPER[1]);
		if( getAffinity() == GICRedistributor[index].RD_base.TYPER[1])
			current_gicr_index = index;
		index++;
	} while((GICRedistributor[index].RD_base.TYPER[0] & (1<<4)) == 0); // Keep incrementing until GICR_TYPER.Last reports no more RDs in block
	// printf("GICR[%u] with Affinity: 0x%x\n", index, GICRedistributor[index].RD_base.TYPER[1]);
	assert(current_gicr_index != 0xff);
	// printf("Current GICR index is %u\n", current_gicr_index);
}

static inline uint16_t GIC_Get_MAX_SPI_INITD()
{
	/* (maximum SPI INTID + 1) is equal to 32 * (GICD_TYPER.ITLinesNumber+1) */
	uint16_t spi_max_id = (((GICDistributor->TYPER & TYPER_IT_LINES_NO_MASK) + 1U) << 5) - 1;
	return (spi_max_id > MAX_SPI_ID) ? MAX_SPI_ID : spi_max_id;
}

void GIC_Distributor_Init()
{
	GIC_Discovery();
	/*
	 * Clear the "enable" bits for G0/G1S/G1NS interrupts before configuring
	 * the ARE_S bit. The Distributor might generate a system error
	 * otherwise.
	 */
	GICDistributor->CTLR &= ~(CTLR_ENABLE_G0_BIT | CTLR_ENABLE_G1S_BIT | CTLR_ENABLE_G1NS_BIT);
	while((GICDistributor->CTLR & GICD_CTLR_RWP_BIT) !=0U);

	/* Set the ARE_S and ARE_NS bit now that interrupts have been disabled */
	GICDistributor->CTLR |= (CTLR_ARE_S_BIT | CTLR_ARE_NS_BIT);
	while((GICDistributor->CTLR & GICD_CTLR_RWP_BIT) !=0U);

	/* Set the default attribute of all SPIs */
	max_spi_id = GIC_Get_MAX_SPI_INITD();
	// printf("Maximum SPI INTID supported: %u\n", max_spi_id);

	/* Treat all SPIs as G0 by default. We do 32 at a time. */
	for (uint16_t i = MIN_SPI_ID; i <= max_spi_id; i += (1U << IGROUPR_SHIFT))
	{
		GICDistributor->IGROUPR[i >> IGROUPR_SHIFT] = 0U;
		GICDistributor->IGRPMODR[i >> IGROUPR_SHIFT] = 0U;
	}

	/* Setup the default SPI priorities doing four at a time */
	for (uint16_t i = MIN_SPI_ID; i <= max_spi_id; i += (1U << IPRIORITYR_SHIFT))
		GICDistributor->IPRIORITYR[i >> IPRIORITYR_SHIFT] = GIC_HIGHEST_SEC_PRIORITY;
	
	/* Treat all SPIs as level triggered by default, write 16 at a time */
	for (uint16_t i = MIN_SPI_ID; i <= max_spi_id; i += (1U << ICFGR_SHIFT))
		GICDistributor->ICFGR[i >> ICFGR_SHIFT] = 0U;
	
	/* Target SPIs to the this core & IRM set to the affinity co-ordinates specified */
	for (uint16_t i = MIN_SPI_ID; i <= max_spi_id; i++)
		GICDistributor->IROUTER[i] = (read_mpidr_el1() & 0xFF00FFFFFF) | (0U << IROUTER_IRM_SHIFT);

	/* Enable the G0/G1S/G1NS interrupts */
	GICDistributor->CTLR |= (CTLR_ENABLE_G0_BIT | CTLR_ENABLE_G1S_BIT | CTLR_ENABLE_G1NS_BIT);
	while((GICDistributor->CTLR & GICD_CTLR_RWP_BIT) !=0U);
}

void GIC_Redistributor_Init()
{
	assert(current_gicr_index != 0xff);

	/* Power on redistributor */
	gic600_pwr_on();

	// Disable all SGIs PPIs before we configure them.
	CURRENT_GICR.SGI_base.ICENABLER0 = 0xFFFFFFFFU;
	gicr_wait_for_pending_write();

	// Set all SGIs and PPIs to G0S by default.
	CURRENT_GICR.SGI_base.IGRPMODR0 = 0U;
	CURRENT_GICR.SGI_base.IGROUPR0  = 0U;

	// Setup the default PPI/SGI priorities doing 4 at a time.
	for (uint16_t i = 0; i < 32; i++)
		CURRENT_GICR.SGI_base.IPRIORITYR[i] = GIC_HIGHEST_SEC_PRIORITY;

	// Set all PPIs to be level triggered by default.
	CURRENT_GICR.SGI_base.ICFGR[1] = 0U;

	// Enable all SGIs and PPIs
	// CURRENT_GICR.SGI_base.ISENABLER0 = 0xFFFFFFFFU;
}

void GIC_CPUInterfaceInit()
{
	/* Mark the connected core as awake */
	gicv3_rdistif_mark_core_awake();

	/* Disable the legacy interrupt bypass */
	uint64_t icc_sre_el3 = ICC_SRE_DIB_BIT | ICC_SRE_DFB_BIT;

	/*
	 * Enable system register access for EL3 and allow lower exception
	 * levels to configure the same for themselves. If the legacy mode is
	 * not supported, the SRE bit is RAO/WI
	 */
	icc_sre_el3 |= (ICC_SRE_EN_BIT | ICC_SRE_SRE_BIT);
	write_icc_sre_el3(read_icc_sre_el3() | icc_sre_el3);

	uint64_t scr_el3 = read_scr_el3();

	/*
	 * Switch to NS state to write Non secure ICC_SRE_EL1 and
	 * ICC_SRE_EL2 registers.
	 */
	write_scr_el3(scr_el3 | SCR_NS_BIT);
	isb();

	write_icc_sre_el2(read_icc_sre_el2() | icc_sre_el3);
	write_icc_sre_el1(ICC_SRE_SRE_BIT);
	isb();

	/* Switch to secure state. */
	write_scr_el3(scr_el3 & (~SCR_NS_BIT));
	isb();

	/* Write the secure ICC_SRE_EL1 register */
	write_icc_sre_el1(ICC_SRE_SRE_BIT);
	isb();

	/* Program the idle priority in the PMR */
	write_icc_pmr_el1(GIC_PRI_MASK);

	/* Enable Group0 interrupts */
	write_icc_igrpen0_el1(IGRPEN1_EL1_ENABLE_G0_BIT);

	/* Enable G1NS/G1S interrupts */
	write_icc_igrpen1_el3(read_icc_igrpen1_el3() |
				IGRPEN1_EL3_ENABLE_G1S_BIT | IGRPEN1_EL3_ENABLE_G1NS_BIT);
	isb();
	/* Add DSB to ensure visibility of System register writes */
	dsb();
}

void GIC_SetRouting(uint16_t int_id, uint32_t affinity, bool irm)
{
	if((int_id > max_spi_id) || (int_id < MIN_SPI_ID))
	{
		printf("INTID %u is not a valid SPI\n", int_id);
		return;
	}
	GICDistributor->IROUTER[int_id] = (uint64_t)(affinity & 0x00FFFFFF) |
								(((uint64_t)affinity & 0xFF000000) << 8U) |
								(uint64_t)irm << IROUTER_IRM_SHIFT;
}

uint32_t GIC_GetRouting(uint16_t int_id)
{
	if((int_id > max_spi_id) || (int_id < MIN_SPI_ID))
	{
		printf("INTID %u is not a valid SPI\n", int_id);
		return 0;
	}
	return GICDistributor->IROUTER[int_id];
}

void GIC_EnableIRQ(uint16_t int_id)
{
	switch (int_id)
	{
	case 0 ... 15: // SGIs
	case 16 ... 31: // PPIs
		assert(current_gicr_index != 0xff);
		CURRENT_GICR.SGI_base.ISENABLER0 = 1U << int_id;
		break;
	case 32 ... 1019: // SPIs
		GICDistributor->ISENABLER[int_id >> ISENABLER_SHIFT] = 1U << (int_id % 32);
		break;
	default:
		break;
	}
}

bool GIC_IsIRQEnabled(uint16_t int_id)
{
	switch (int_id)
	{
	case 0 ... 15: // SGIs
	case 16 ... 31: // PPIs
		assert(current_gicr_index != 0xff);
		return (CURRENT_GICR.SGI_base.ISENABLER0 & (1U << int_id)) != 0U;
	case 32 ... 1019: // SPIs
		return (GICDistributor->ISENABLER[int_id >> ISENABLER_SHIFT] & (1U << (int_id % 32))) != 0U;
	default:
		return false;
	}
}

void GIC_DisableIRQ(uint16_t int_id)
{
	switch (int_id)
	{
	case 0 ... 15: // SGIs
	case 16 ... 31: // PPIs
		assert(current_gicr_index != 0xff);
		CURRENT_GICR.SGI_base.ICENABLER0 = 1U << int_id;
		break;
	case 32 ... 1019: // SPIs
		GICDistributor->ICENABLER[int_id >> ISENABLER_SHIFT] = 1U << (int_id % 32);
		break;
	default:
		break;
	}
}

bool GIC_GetPendingIRQ(uint16_t int_id)
{
	switch (int_id)
	{
	case 0 ... 15: // SGIs
	case 16 ... 31: // PPIs
		assert(current_gicr_index != 0xff);
		return (CURRENT_GICR.SGI_base.ISPENDR0 & (1U << int_id)) != 0U;
	case 32 ... 1019: // SPIs
		return (GICDistributor->ISPENDR[int_id >> ISPENDR_SHIFT] & (1U << (int_id % 32))) != 0U;
	default:
		return false;
	}
}

void GIC_SetPendingIRQ(uint16_t int_id)
{
	switch (int_id)
	{
	case 0 ... 15: // SGIs
	case 16 ... 31: // PPIs
		assert(current_gicr_index != 0xff);
		CURRENT_GICR.SGI_base.ISPENDR0 = 1U << int_id;
		break;
	case 32 ... 1019: // SPIs
		GICDistributor->ISPENDR[int_id >> ISPENDR_SHIFT] = 1U << (int_id % 32);
		break;
	default:
		break;
	}
}

void GIC_ClearPendingIRQ(uint16_t int_id)
{
	switch (int_id)
	{
	case 0 ... 15: // SGIs
	case 16 ... 31: // PPIs
		assert(current_gicr_index != 0xff);
		CURRENT_GICR.SGI_base.ICPENDR0 = 1U << int_id;
		break;
	case 32 ... 1019: // SPIs
		GICDistributor->ICPENDR[int_id >> ICPENDR_SHIFT] = 1U << (int_id % 32);
		break;
	default:
		break;
	}
}

void GIC_SetEdgeOrLevel(uint16_t int_id, bool is_level_triggered)
{
	switch (int_id)
	{
	case 0 ... 15: // SGIs
		// SGIs are always treated as edge-triggered
		break;
	case 16 ... 31: // PPIs
		assert(current_gicr_index != 0xff);
		CURRENT_GICR.SGI_base.ICFGR[1] &= ~(0b11 << ((int_id % 16) << 1U));
		if (!is_level_triggered)
			CURRENT_GICR.SGI_base.ICFGR[1] |= 0b10 << ((int_id % 16) << 1U);
		break;
	case 32 ... 1019: // SPIs
		GICDistributor->ICFGR[int_id >> ICFGR_SHIFT] &= ~(0b11 << ((int_id % 16) << 1U));
		if (!is_level_triggered)
			GICDistributor->ICFGR[int_id >> ICFGR_SHIFT] |= 0b10 << ((int_id % 16) << 1U);
		break;
	default:
		break;
	}
}

bool GIC_IsLevelSensitive(uint16_t int_id)
{
	switch (int_id)
	{
	case 0 ... 15: // SGIs
		// SGIs are always treated as edge-triggered
		return true;
	case 16 ... 31: // PPIs
		assert(current_gicr_index != 0xff);
		return (CURRENT_GICR.SGI_base.ICFGR[1] & (0b11 << ((int_id % 16) << 1U))) == 0U;
	case 32 ... 1019: // SPIs
		return (GICDistributor->ICFGR[int_id >> ICFGR_SHIFT] & (0b11 << ((int_id % 16) << 1U))) == 0U;
	default:
		return false;
	}
}

void GIC_SetPriority(uint16_t int_id, uint8_t priority)
{
	switch (int_id)
	{
	case 0 ... 15: // SGIs
	case 16 ... 31: // PPIs
		assert(current_gicr_index != 0xff);
		CURRENT_GICR.SGI_base.IPRIORITYR[int_id] = priority;
		break;
	case 32 ... 1019: // SPIs
		GICDistributor->IPRIORITYR[int_id] = priority;
		break;
	default:
		break;
	}
}

uint8_t GIC_GetPriority(uint16_t int_id)
{
	switch (int_id)
	{
	case 0 ... 15: // SGIs
	case 16 ... 31: // PPIs
		assert(current_gicr_index != 0xff);
		return CURRENT_GICR.SGI_base.IPRIORITYR[int_id];
	case 32 ... 1019: // SPIs
		return GICDistributor->IPRIORITYR[int_id];
	default:
		return 0;
	}
}

void GIC_SetGroup(uint16_t int_id, GICv3_Goroup_t group)
{
	switch (int_id)
	{
	case 0 ... 15: // SGIs
	case 16 ... 31: // PPIs
		assert(current_gicr_index != 0xff);
		switch (group)
		{
		case G0S: // 0b00
			CURRENT_GICR.SGI_base.IGRPMODR0 &= ~(1U << int_id);
			CURRENT_GICR.SGI_base.IGROUPR0  &= ~(1U << int_id);
			break;
		case G1NS: // 0b01
			CURRENT_GICR.SGI_base.IGRPMODR0 &= ~(1U << int_id);
			CURRENT_GICR.SGI_base.IGROUPR0  |=  (1U << int_id);
			break;
		case G1S: // 0b10
			CURRENT_GICR.SGI_base.IGRPMODR0 |=  (1U << int_id);
			CURRENT_GICR.SGI_base.IGROUPR0  &= ~(1U << int_id);
			break;
		default:
			return;
		}
		break;
	case 32 ... 1019: // SPIs
		switch (group)
		{
		case G0S: // 0b00
			GICDistributor->IGRPMODR[int_id >> IGROUPR_SHIFT] &= ~(1U << (int_id % 32));
			GICDistributor->IGROUPR[int_id >> IGROUPR_SHIFT]  &= ~(1U << (int_id % 32));
			break;
		case G1NS: // 0b01
			GICDistributor->IGRPMODR[int_id >> IGROUPR_SHIFT] &= ~(1U << (int_id % 32));
			GICDistributor->IGROUPR[int_id >> IGROUPR_SHIFT]  |=  (1U << (int_id % 32));
			break;
		case G1S: // 0b10
			GICDistributor->IGRPMODR[int_id >> IGROUPR_SHIFT] |=  (1U << (int_id % 32));
			GICDistributor->IGROUPR[int_id >> IGROUPR_SHIFT]  &= ~(1U << (int_id % 32));
			break;
		default:
			return;
		}
		break;
	default:
		break;
	}
}

GICv3_Goroup_t GIC_GetGroup(uint16_t int_id)
{
	switch (int_id)
	{
	case 0 ... 15: // SGIs
	case 16 ... 31: // PPIs
		assert(current_gicr_index != 0xff);
		if((CURRENT_GICR.SGI_base.IGRPMODR0 & (1U << int_id)) == 0U)
		{
			if((CURRENT_GICR.SGI_base.IGROUPR0 & (1U << int_id)) == 0U)
				return G0S;
			else
				return G1NS;
		}
		else
			return G1S;
	case 32 ... 1019: // SPIs
		if((GICDistributor->IGRPMODR[int_id >> IGROUPR_SHIFT] & (1U << (int_id % 32))) == 0U)
		{
			if((GICDistributor->IGROUPR[int_id >> IGROUPR_SHIFT] & (1U << (int_id % 32))) == 0U)
				return G0S;
			else
				return G1NS;
		}
		else
			return G1S;
	default:
		return G0S;
	}
}

void GIC_Init()
{
	for (uint16_t i = 0U; i < IRQ_GIC_LINE_COUNT; i++)
		IRQTable[i] = (IRQHandler_t)NULL;
	GIC_Discovery();
	GIC_Distributor_Init();
	GIC_Redistributor_Init();
	GIC_CPUInterfaceInit();
}

/// Register interrupt handler.
void IRQ_SetHandler (uint16_t int_id, IRQHandler_t handler) {
	if (int_id < IRQ_GIC_LINE_COUNT)
		IRQTable[int_id] = handler;
}

/// Get the registered interrupt handler.
IRQHandler_t IRQ_GetHandler (uint16_t int_id) 
{
	return (int_id < IRQ_GIC_LINE_COUNT)? IRQTable[int_id] : (IRQHandler_t)0;
}


void fiq_handler(void)
{
	uint32_t iar;
	IRQHandler_t irq_handler;
	do {
		iar = read_icc_iar0_el1();

		switch (iar)
		{
		case 0 ... 15: // SGIs
		case 16 ... 31: // PPIs
		case 32 ... 1019: // SPIs
			irq_handler = IRQ_GetHandler(iar);
			if(irq_handler!=(IRQHandler_t)0)
				irq_handler();
			break;
		case 1020 ... 1022:
			printf("FIQ: Received Special INTID.%d\n", iar);
			break;
		case 1023:
			return;
		case 1024 ... 8191: // Reserved
			break;
		default: // >= 8192 LPIs
			// Todo: LPIs
			printf("FIQ: Received LPI INTID.%d\n", iar);
		}

		// Write EOIR to deactivate interrupt
		write_icc_eoir0_el1(iar);
		isb();
	} while(1);
}
