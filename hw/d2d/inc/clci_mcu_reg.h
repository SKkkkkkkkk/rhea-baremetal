// Base Register Group(0x0 – 0x2FF)
#define CLCI_REG_BASE_CTRL (0x0) // Basel Control Register
#define CLCI_REG_BIT_LOCK0_PAT0 (0x4) // BIT Lock0 Pattern Register0
#define CLCI_REG_BIT_LOCK0_PAT1 (0x8) // BIT Lock0 Pattern Register1
#define CLCI_REG_BIT_LOCK0_PAT2 (0xC) // BIT Lock0 Pattern Register2
#define CLCI_REG_BIT_LOCK1_PAT0 (0x10) // BIT Lock1 Pattern Register0
#define CLCI_REG_BIT_LOCK1_PAT1 (0x14) // BIT Lock1 Pattern Register1
#define CLCI_REG_BIT_LOCK1_PAT2 (0x18) // BIT Lock1 Pattern Register2
#define CLCI_REG_BLK_LOCK0_PAT0 (0x1C) // BLK Lock Pattern0 Register0
#define CLCI_REG_BLK_LOCK0_PAT1 (0x20) // BLK Lock Pattern0 Register1
#define CLCI_REG_BLK_LOCK1_PAT0 (0x24) // BLK Lock Pattern1 Register0
#define CLCI_REG_BLK_LOCK1_PAT1 (0x28) // BLK Lock Pattern1 Register1
#define CLCI_REG_PCS_IDLE_PAT0 (0x2C) // PCS Idle Pattern Register0
#define CLCI_REG_PCS_IDLE_PAT1 (0x30) // PCS Idle Pattern Register1
#define CLCI_REG_BIT_LOCK_WAIT_CYC (0x34) // BIT Lock RX delay update wait cycle
#define CLCI_REG_MAC_CTRL (0x3C) // MAC layer control Register
#define CLCI_REG_TX_LANE_MAP_SEL0 (0x40) // TX Lane map select0
#define CLCI_REG_TX_LANE_MAP_SEL1 (0x44) // TX Lane map select1
#define CLCI_REG_RX_LANE_MAP_SEL0 (0x48) // RX Lane map select0
#define CLCI_REG_RX_LANE_MAP_SEL1 (0x4C) // RX Lane map select1
#define CLCI_REG_MAC_ECC_CNT (0x50) // MAC ECC detect correct counter.
#define CLCI_REG_MAC_ECC_CRC_DEC_CNT (0x54) // MAC ECC/CRC decode ok counter.
#define CLCI_REG_MAC_MON_CTRL (0x58) // MAC monitor counter & controller
#define CLCI_REG_MAC_MON2 (0x5C) // MAC monitor 2
#define CLCI_REG_MAC_MON3 (0x60) // MAC monitor 2
#define CLCI_REG_MAC_DEBUG_S0 (0x64) // MAC layer debug signal0
#define CLCI_REG_MAC_DEBUG_S1 (0x68) // MAC layer debug signal1
#define CLCI_REG_MAC_DEBUG_S2 (0x6C) // MAC layer debug signal2
#define CLCI_REG_MAC_DEBUG_S3 (0x70) // MAC layer debug signal3
#define CLCI_REG_PMD_CTRL (0x74) // PMD Control Register.
#define CLCI_REG_PMD_TX_LANE_MASK (0x78) // PMD TX Lane Mask Register.
#define CLCI_REG_PMD_RX_LANE_MASK (0x7C) // PMD RX Lane Mask Register.
#define CLCI_REG_PMD_RX_LANE_SRAM_ CTRL (0x80) // PMD RX SRAM Read Control Register.
#define CLCI_REG_PMD_RX_LANE_SRAM_RD0 (0x84) // PMD RX SRAM Read Data0 Register.
#define CLCI_REG_PMD_RX_LANE_SRAM_RD1 (0x88) // PMD RX SRAM Read Data1 Register.
#define CLCI_REG_PMD_RX_LANE_SRAM_RD2 (0x8C) // PMD RX SRAM Read Data2 Register.
#define CLCI_REG_PMD_DEBUG_CTRL (0x90) // PMD Debug control Register.
#define CLCI_REG_MAC_LOCK_VALUE (0x94) // PMD Lock Value.
#define CLCI_REG_MISSION_INT_STS0 (0x98) // Mission Interrupt Status0
#define CLCI_REG_MISSION_INT_EN0 (0x9C) // Mission Interrupt Enable0
#define CLCI_REG_MISSION_INT_STS1 (0xA0) // Mission Interrupt Status1
#define CLCI_REG_MISSION_INT_EN1 (0xA4) // Mission Interrupt Enable1
#define CLCI_REG_ERROR_INT_STS0 (0xA8) // Error Interrupt Status0
#define CLCI_REG_ERROR_INT_EN0 (0xAC) // Error Interrupt Enable0
#define CLCI_REG_ERROR_INT_STS1 (0xB0) // Error Interrupt Status1
#define CLCI_REG_ERROR_INT_EN1 (0xB4) // Error Interrupt Enable1
#define CLCI_REG_ERROR_INT_STS2 (0xB8) // Error Interrupt Status2
#define CLCI_REG_ERROR_INT_EN2 (0xBC) // Error Interrupt Enable2
#define CLCI_REG_ERROR_INT_STS3 (0xC0) // Error Interrupt Status3
#define CLCI_REG_ERROR_INT_EN3 (0xC4) // Error Interrupt Enable3
#define CLCI_REG_ERROR_INT_STS4 (0xC8) // Error Interrupt Status4
#define CLCI_REG_ERROR_INT_EN4 (0xCC) // Error Interrupt Enable4
#define CLCI_REG_ERROR_INT_STS5 (0xD0) // Error Interrupt Status5
#define CLCI_REG_ERROR_INT_EN5 (0xD4) // Error Interrupt Enable5
#define CLCI_REG_ERROR_INT_STS6 (0xD8) // Error Interrupt Status6
#define CLCI_REG_ERROR_INT_EN6 (0xDC) // Error Interrupt Enable6
#define CLCI_REG_COMMON_ERROR_INT_STS7 (0xE0) // Common Error Interrupt Status7
#define CLCI_REG_COMMON_ERROR_INT_EN7 (0xE4) // Common Error Interrupt Enable7
#define CLCI_REG_COMMON_ERROR_INT_STS8 (0xE8) // Common Error Interrupt Status8
#define CLCI_REG_COMMON_ERROR_INT_EN8 (0xEC) // Common Error Interrupt Enable8
#define CLCI_REG_COMMON_ERROR_INT_STS9 (0xF0) // Common Error Interrupt Status9
#define CLCI_REG_COMMON_ERROR_INT_EN9 (0xF4) // Common Error Interrupt Enable9
#define CLCI_REG_PCS_RETRY_CTRL0 (0xF8) // PCS Retry Control Register0
#define CLCI_REG_PCS_RETRY_CTRL1 (0xFC) // PCS Retry Control Register1
#define CLCI_REG_PCS_SCRAMBLE_SEED_REG (0x100) // PCS Scramble Seed Register
// PHY Registers
#define CLCI_REG_PHY_TX_CLK_CTRL_0 (0x180) // PHY TX Clock Control Register0
#define CLCI_REG_PHY_TX_CLK_CTRL_1 (0x184) // PHY TX Clock Control Register1
#define CLCI_REG_PHY_TX_CLK_CTRL_2 (0x188) // PHY TX Clock Control Register2
#define CLCI_REG_PHY_RX_CLK_CTRL_0 (0x18C) // PHY RX Clock Control Register0
#define CLCI_REG_PHY_RX_CLK_CTRL_1 (0x190) // PHY RX Clock Control Register1
#define CLCI_REG_PHY_RX_CLK_CTRL_2 (0x194) // PHY RX Clock Control Register2
#define CLCI_REG_PHY_RX_CLK_CTRL_3 (0x198) // PHY RX Clock Control Register3
#define CLCI_REG_PHY_TEST_REG0 (0x19C) // PHY Test Register0
#define CLCI_REG_PHY_TEST_REG1 (0x1A0) // PHY Test Register1
#define CLCI_REG_PHY_TEST_REG2 (0x1A4) // PHY Test Register2
#define CLCI_REG_PHY_PLL_TEST_REG0 (0x1A8) // PHY PLL Test Register0
#define CLCI_REG_PHY_PLL_TEST_REG1 (0x1AC) // PHY PLL Test Register1
// MAC&PCS Registers
#define CLCI_REG_LEGACY_MODE_CFG (0x200) // Legacy Mode Config Register
#define CLCI_REG_COMMON_MAC_ARBITER_MODE (0x204) // Common Aribter Mode Register
#define CLCI_REG_COMMON_MAC_ARBITER_WEIGHT (0x208) // Common Aribter Weight Register
#define CLCI_REG_COMMON_FLOW_CTRL_CFG (0x20C) // Common flow Control Register
#define CLCI_REG_COMMON_CHS_PORT_NUM (0x210) // Common Channels Port Number Config Register.
#define CLCI_REG_COMMON_PCS_RX_FIFO_THSH0 (0x214) // Common PCS RX Fifo Threshold Register0
#define CLCI_REG_COMMON_PCS_RX_FIFO_THSH1 (0x218) // Common PCS RX Fifo Threshold Register1
#define CLCI_REG_COMMON_MAC_BASE_CTRL0 (0x21C) // Common MAC Base Control Register
#define CLCI_REG_PCS_BASE_CTRL0 (0x220) // PCS Base Control Register0
// MAC per-channel (offset 0x300 -0x3FF)
#define CLCI_REG_COMMON_MAC_CHN_BASE0 (0x300) // Port N MAC Base Register0
#define CLCI_REG_COMMON_MAC_CHN_BASE1 (0x304) // Port N MAC Base Register1
#define CLCI_REG_COMMON_MAC_CHN_REMAP_CTRL_GP0 (0x308) // Port N MAC Remap Control Register0
#define CLCI_REG_COMMON_MAC_CHN_REMAP_VALUE_GP0 (0x30C) // Port N MAC Remap Control Target Address Register0
#define CLCI_REG_COMMON_MAC_CHN_REMAP_SRC_GP0 (0x310) // Port N MAC Remap Control Source Address Register0
#define CLCI_REG_COMMON_MAC_CHN_REMAP_CTRL_GP1 (0x314) // Port N MAC Remap Control Register1
#define CLCI_REG_COMMON_MAC_CHN_REMAP_VALUE_GP1 (0x318) // Port N MAC Remap Control Target Address Register1
#define CLCI_REG_COMMON_MAC_CHN_REMAP_SRC_GP1 (0x31C) // Port N MAC Remap Control Source Address Register1
#define CLCI_REG_COMMON_MAC_CHN_REMAP_CTRL_GP2 (0x320) // Port N MAC Remap Control Register2
#define CLCI_REG_COMMON_MAC_CHN_REMAP_VALUE_GP2 (0x324) // Port N MAC Remap Control Target Address Register2
#define CLCI_REG_COMMON_MAC_CHN_REMAP_SRC_GP2 (0x328) // Port N MAC Remap Control Source Address Register2
#define CLCI_REG_COMMON_MAC_CHN_REMAP_CTRL_GP3 (0x32C) // Port N MAC Remap Control Register3
#define CLCI_REG_COMMON_MAC_CHN_REMAP_VALUE_GP3 (0x330) // Port N MAC Remap Control Target Address Register3
#define CLCI_REG_COMMON_MAC_CHN_REMAP_SRC_GP3 (0x334) // Port N MAC Remap Control Source Address Register3
// PCS & PHY per-channel ( offset 0x400: each channel address range 0x100)
#define CLCI_REG_BIT_LOCK_CTRL0 (0x400) // Channel N Bit Lock Control Register 0
#define CLCI_REG_BIT_LOCK_CTRL1 (0x404) // Channel N Bit Lock Control Register 1
#define CLCI_REG_BIT_LOCK_CTRL2 (0x408) // Channel N Bit Lock Control Register 2
#define CLCI_REG_BIT_LOCK_CTRL3 (0x40C) // Channel N Bit Lock Control Register 3
#define CLCI_REG_BIT_LOCK_CTRL4 (0x410) // Channel N Bit Lock Control Register 4
#define CLCI_REG_BIT_LOCK_BER_RATE_CNT0 (0x414) // Channel N Bit Lock Bit Error Rate Counter 0
#define CLCI_REG_BIT_LOCK_BER_RATE_CNT1 (0x418) // Channel N Bit Lock Bit Error Rate Counter 1
#define CLCI_REG_BIT_LOCK_BER_RATE_CNT2 (0x41C) // Channel N Bit Lock Bit Error Rate Counter 2
#define CLCI_REG_BIT_LOCK_BER_RATE_CTRL (0x420) // Channel N Bit Lock Bit Error Rate Control Register
#define CLCI_REG_PAT_DEFAULT_CNT (0x424) // Channel N Pattern Default Counter Register
#define CLCI_REG_PAT0_CNT_REG (0x428) // Channel N Pattern 0 Counter Register
#define CLCI_REG_PAT1_CNT_REG (0x42C) // Channel N Pattern 1 Counter Register
#define CLCI_REG_PCS_CTRL_REG (0x430) // Channel N PCS Control Register
#define CLCI_REG_RAW_PCS_CTRL_REG (0x434) // Channel N RAW PCS Control Register
#define CLCI_REG_PCS_CTRL1_REG (0x438) // Channel N PCS Control1 Register
#define CLCI_REG_TX_LANE_CLK_DESKEW_REG (0x43C) // Channel N Tx Lane Clock Deskew Register
#define CLCI_REG_RX_LANE_CLK_DESKEW_REG (0x440) // Channel N Rx Lane Clock Deskew Register
#define CLCI_REG_PHY_TX_CTRL_REG_0 (0x480) // Channel N PHY TX Control Register 0
#define CLCI_REG_PHY_RX_CTRL_REG_0 (0x484) // Channel N PHY RX Control Register 0
#define CLCI_REG_PHY_RX_CTRL_REG_1 (0x488) // Channel N PHY RX Control Register 1
#define CLCI_REG_PHY_RX_OS_REG (0x48C) // Channel N RX Lane Oversampling Control Register
