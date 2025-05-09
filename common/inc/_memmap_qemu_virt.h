#ifdef A55
#   define VIRT_FLASH               0x00000000
#   define VIRT_CPUPERIPHS          0x08000000
#   define VIRT_GIC_DIST            0x08000000
#   define VIRT_GIC_CPU             0x08010000
#   define VIRT_GIC_V2M             0x08020000
#   define VIRT_GIC_HYP             0x08030000
#   define VIRT_GIC_VCPU            0x08040000
#   define VIRT_GIC_ITS             0x08080000
#   define VIRT_GIC_REDIST          0x080A0000
#   define VIRT_UART                0x09000000
#   define VIRT_RTC                 0x09010000
#   define VIRT_FW_CFG              0x09020000
#   define VIRT_GPIO                0x09030000
#   define VIRT_SECURE_UART         0x09040000
#   define VIRT_SMMU                0x09050000
#   define VIRT_PCDIMM_ACPI         0x09070000
#   define VIRT_ACPI_GED            0x09080000
#   define VIRT_NVDIMM_ACPI         0x09090000
#   define VIRT_PVTIME              0x090a0000
#   define VIRT_SECURE_GPIO         0x090b0000
#   define VIRT_MMIO                0x0a000000
#   define VIRT_PLATFORM_BUS        0x0c000000
#   define VIRT_SECURE_MEM          0x0e000000
#   define VIRT_PCIE_MMIO           0x10000000
#   define VIRT_PCIE_PIO            0x3eff0000
#   define VIRT_PCIE_ECAM           0x3f000000
#   define VIRT_MEM                 0x40000000
#else
#   define VIRT_DEBUG               0x0
#   define VIRT_MROM                0x1000
#   define VIRT_TEST                0x100000
#   define VIRT_RTC                 0x101000
#   define VIRT_CLINT               0x2000000
#   define VIRT_ACLINT_SSWI         0x2F00000
#   define VIRT_PCIE_PIO            0x3000000
#   define VIRT_IOMMU_SYS           0x3010000
#   define VIRT_PLATFORM_BUS        0x4000000
#   define VIRT_PLIC                0xc000000 // VIRT_PLIC_SIZE(VIRT_CPUS_MAX * 2)
#   define VIRT_APLIC_M             0xc000000 // APLIC_SIZE(VIRT_CPUS_MAX)
#   define VIRT_APLIC_S             0xd000000 // APLIC_SIZE(VIRT_CPUS_MAX)
#   define VIRT_UART0               0x10000000
#   define VIRT_VIRTIO              0x10001000
#   define VIRT_FW_CFG              0x10100000
#   define VIRT_FLASH               0x20000000
#   define VIRT_IMSIC_M             0x24000000 // VIRT_IMSIC_MAX_SIZE
#   define VIRT_IMSIC_S             0x28000000 // VIRT_IMSIC_MAX_SIZE
#   define VIRT_PCIE_ECAM           0x30000000
#   define VIRT_PCIE_MMIO           0x40000000
#   define VIRT_DRAM                0x80000000
#endif