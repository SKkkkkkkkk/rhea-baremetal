#include "reg_scan.h"

// SEEHI SPI_bridge Registers
typedef struct {
	__OM uint32_t MODECTRL;
	__OM uint32_t BAUDRCTRL;
	__OM uint32_t DEVICECTRL;
	RESERVED(0[61], uint32_t)
} SEEHI_SPI_BRIDGE_TypeDef;

// DW APB SSI Registers
typedef struct {
	__IOM uint32_t CTRLR0;
	__IOM uint32_t CTRLR1;
	__IOM uint32_t SSIENR;
	__IOM uint32_t MWCR;
	__IOM uint32_t SER;
	__IOM uint32_t BAUDR;
	__IOM uint32_t TXFTLR;
	__IOM uint32_t RXFTLR;
	__IM  uint32_t TXFLR;
	__IM  uint32_t RXFLR;
	__IM  uint32_t SR;
	__IOM uint32_t IMR;
	__IM  uint32_t ISR;
	__IM  uint32_t RISR;
	__IM  uint32_t TXOICR;
	__IM  uint32_t RXOICR;
	__IM  uint32_t RXUICR;
	__IM  uint32_t MSTICR;
	__IM  uint32_t ICR;
	__IOM uint32_t DMACR;
	__IOM uint32_t DMATDLR;
	__IOM uint32_t DMARDLR;
	__IM  uint32_t IDR;
	__IM  uint32_t SSI_VERSION_ID;
	__IOM uint32_t DR;
	RESERVED(1 [35], uint32_t)
	__IOM uint32_t RX_SAMPLE_DLY;
	__IOM uint32_t SPI_CTRLR0;
	__IOM uint32_t TXD_DRIVE_EDGE;
	__IM  uint32_t RSVD;
} DW_APB_SSI_TypeDef;

typedef struct {
	SEEHI_SPI_BRIDGE_TypeDef SEEHI_SPI_BRIDGE;
	DW_APB_SSI_TypeDef DW_APB_SSI;
} SPI_TypeDef;

int main()
{
	printf("REG_SCAN_SPI scaning...\n");
	SPI_TypeDef *spi = (SPI_TypeDef *)BOOTSPI_BASE;
	WO_TEST(spi->SEEHI_SPI_BRIDGE.MODECTRL, 0x1); //! 0x1: Config mode, so we can scan the registers
	WO_TEST(spi->SEEHI_SPI_BRIDGE.BAUDRCTRL, 0x8);
	WO_TEST(spi->SEEHI_SPI_BRIDGE.DEVICECTRL, 0x1);
	RW_TEST(spi->DW_APB_SSI.CTRLR0);
	RW_TEST(spi->DW_APB_SSI.CTRLR1);
	RW_TEST(spi->DW_APB_SSI.SSIENR);
	RW_TEST(spi->DW_APB_SSI.MWCR);
	RW_TEST(spi->DW_APB_SSI.SER);
	RW_TEST(spi->DW_APB_SSI.BAUDR);
	RW_TEST(spi->DW_APB_SSI.TXFTLR);
	RW_TEST(spi->DW_APB_SSI.RXFTLR);
	RO_TEST(spi->DW_APB_SSI.TXFLR);
	RO_TEST(spi->DW_APB_SSI.RXFLR);
	RO_TEST(spi->DW_APB_SSI.SR);
	RW_TEST(spi->DW_APB_SSI.IMR);
	RO_TEST(spi->DW_APB_SSI.ISR);
	RO_TEST(spi->DW_APB_SSI.RISR);
	RO_TEST(spi->DW_APB_SSI.TXOICR);
	RO_TEST(spi->DW_APB_SSI.RXOICR);
	RO_TEST(spi->DW_APB_SSI.RXUICR);
	RO_TEST(spi->DW_APB_SSI.MSTICR);
	RO_TEST(spi->DW_APB_SSI.ICR);
	RW_TEST(spi->DW_APB_SSI.DMACR);
	RW_TEST(spi->DW_APB_SSI.DMATDLR);
	RW_TEST(spi->DW_APB_SSI.DMARDLR);
	RO_TEST(spi->DW_APB_SSI.IDR);
	RO_TEST(spi->DW_APB_SSI.SSI_VERSION_ID);
	assert(spi->DW_APB_SSI.SSI_VERSION_ID == 0x3430322a); //! make sure the version is correct
	RW_TEST(spi->DW_APB_SSI.DR);
	RW_TEST(spi->DW_APB_SSI.RX_SAMPLE_DLY);
	RW_TEST(spi->DW_APB_SSI.SPI_CTRLR0);
	RW_TEST(spi->DW_APB_SSI.TXD_DRIVE_EDGE);
	RO_TEST(spi->DW_APB_SSI.RSVD);
	printf("REG_SCAN_SPI PASS\n");
	return 0;
}