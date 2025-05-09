#ifdef A55
#   define BOOTROM_BASE         0x00000000
#   define APRAM_BASE           0x00100000
#   define BOOTFLASH_BASE       0x01000000
#   define UART0_BASE           0x10000000
#   define GENERIC_TIMER_BASE   0x10010000
#   define DW_TIMER_BASE        0x10020000
#   define WDT_BASE             0x10030000
#   define DAPLITE_BASE         0x10040000
#   define MBI_TX_BASE          0x10050000
#   define MAILBOX_BASE         0x10060000
#   define EMMC_BASE            0x10100000
#   define GMAC_BASE            0x10110000
#   define DMAC_BASE            0x10120000
#   define SPACC_BASE           0x10200000
#   define PKA_BASE             0x10220000
#   define EFUSE_BASE           0x10230000
#   define GPIO_BASE            0x10300000
#   define I2C0_BASE            0x10310000
#   define I2C1_BASE            0x10320000
#   define UART1_BASE           0x10330000
#   define BOOTSPI_BASE         0x10340000
#   define AP_CFG_BASE          0x10350000
#   define UART2_BASE           0x10360000
#   define UART3_BASE           0x10370000
#   define PWM_BASE             0x10380000
#   define GIC600_BASE          0x10400000
#   define MBI_RX_BASE          0x105F0000
#   define DROUTER_BASE         0x11000000
#   define CROUTER_BASE         0x11001000
#   define DNIU_BASE            0x11002000
#   define CNIU_BASE            0x11003000
#   define SYSCTRL_BASE         0x12000000
#   define VPU_BASE             0x14000000
#   define VPU_CFG_BASE         0x14100000
#   define TCM_CFG_BASE         0x15000000
#   define AP_DRAM_BASE         0x40000000
#else // AX65
#   define BOOTROM_BASE         0x00000000
#   define APRAM_BASE           0x00010000
#   define L2C_BASE            0x00020000
#   define PLIC_BASE           0x00040000
#   define PLIC_SW_BASE        0x00080000
#   define PLDM_BASE           0x000C0000
#   define PLMT_BASE           0x000D0000
#   define BOOTFLASH_BASE      0x00100000

/* PCIe Space */
#   define C2C_SYS_SPACE0_BASE 0x00200000
#   define C2C_SYS_SPACE1_BASE 0x00300000
#   define C2C_SYS_SPACE2_BASE 0x00400000
#   define C2C_SYS_SPACE3_BASE 0x00500000

/* Peripheral */
#   define UART0_BASE          0x00600000
#   define GENERIC_TIMER_BASE  0x00601000
#   define DW_TIMER_BASE       0x00602000
#   define WDT_BASE           0x00603000
#   define BOOTSPI_BASE       0x00604000
#   define MAILBOX_BASE       0x00605000

/* HiPeri Engine */
#   define EMMC_BASE          0x00610000
#   define GMAC_BASE          0x00611000
#   define DMAC_BASE          0x00612000

/* Crypto Engine */
#   define SPACC_BASE         0x00620000
#   define PKA_BASE           0x00622000
#   define EFUSE_BASE         0x00623000

/* LoPeri Engine */
#   define GPIO_BASE          0x00630000
#   define I2C0_BASE          0x00631000
#   define I2C1_BASE          0x00632000
#   define UART1_BASE         0x00633000
#   define UART2_BASE         0x00634000
#   define UART3_BASE         0x00635000
#   define PWM_BASE           0x00636000
#   define MBI_RX_BASE        0x00637000

/* System Control */
#   define SYSCTRL_BASE       0x00640000

/* VPU */
#   define VPU_BASE           0x00641000
#   define VPU_CFG_BASE       0x00651000

/* Common */
#   define AP_CFG_BASE        0x007F0000
#   define MBI_TX_BASE        0x007F1000
#   define PROCESS_MONITOR_BASE 0x007F2000
#   define DROUTER_BASE       0x007F3000
#   define CROUTER_BASE       0x007F3100
#   define DNIU_BASE          0x007F3200
#   define CNIU_BASE          0x007F3300
#   define ATU_GM_CFG_BASE    0x007F4000
#   define MC_PFC_BASE        0x007F4100
#   define MC_CFG_BASE        0x007F5000

/* Memory */
#   define TCM_MEM_BASE       0x04000000

#endif // A55