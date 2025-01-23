# This is all the targets that Rhea BareMetal supplies
# About what each target does, please check the interface of each target

# drivers
add_subdirectory(hw/cpu/aarch64 EXCLUDE_FROM_ALL)
add_subdirectory(hw/gic/v3_v4 EXCLUDE_FROM_ALL)
add_subdirectory(hw/uart/pl011 EXCLUDE_FROM_ALL)
add_subdirectory(hw/uart/dw_apb_uart EXCLUDE_FROM_ALL)
add_subdirectory(hw/gpio/dw_apb_gpio EXCLUDE_FROM_ALL)
add_subdirectory(hw/spi/dw_apb_ssi EXCLUDE_FROM_ALL)
add_subdirectory(hw/timer/dw_apb_timers EXCLUDE_FROM_ALL)
add_subdirectory(hw/system_counter EXCLUDE_FROM_ALL)
add_subdirectory(hw/mailbox EXCLUDE_FROM_ALL)
add_subdirectory(hw/pcie EXCLUDE_FROM_ALL)
add_subdirectory(hw/mmc/dw_mmc EXCLUDE_FROM_ALL)
add_subdirectory(hw/cru EXCLUDE_FROM_ALL)
add_subdirectory(hw/d2d EXCLUDE_FROM_ALL)
add_subdirectory(hw/dma/dw_axi_dma EXCLUDE_FROM_ALL)

# libs
add_subdirectory(libs/linker_script EXCLUDE_FROM_ALL)
add_subdirectory(libs/newlib_stubs EXCLUDE_FROM_ALL)
add_subdirectory(libs/flash/nor EXCLUDE_FROM_ALL)
add_subdirectory(libs/flash/nand EXCLUDE_FROM_ALL)
add_subdirectory(libs/systimer EXCLUDE_FROM_ALL)
add_subdirectory(libs/crc EXCLUDE_FROM_ALL)
add_subdirectory(libs/xmodem EXCLUDE_FROM_ALL)
add_subdirectory(libs/time_stamp EXCLUDE_FROM_ALL)
add_subdirectory(libs/freertos/kernel EXCLUDE_FROM_ALL)
