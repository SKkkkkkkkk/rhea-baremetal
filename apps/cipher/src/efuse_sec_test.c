#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "efuse.h"
#include "efuse_regs.h"
#include "systimer.h"
#include "gicv3.h"
#include "cipher_test_debg.h"

#ifdef TEST_CIPH_IRQ
static void efuse_irq_handle(void)
{
    uint32_t irq_stat = EFUSE->EFUSE_INT_STATUS_U.ALL;
    printf("efuse irq handle ; irq status = 0x%x \n", irq_stat);
    EFUSE->EFUSE_INT_CLEAR_U.ALL = irq_stat;
}

void efuse_irq_test(void)
{
    seehi_func_enter();
    IRQ_SetHandler(95 + 32, efuse_irq_handle);
	IRQ_SetPriority(95 + 32, 0 << 3);
	IRQ_Enable(95 + 32);
}
#endif

void efuse_sec_test(void)
{
    uint32_t wr_data = 0xaa;
    uint32_t rd_data = 0;
    uint32_t addr = 33;
    int tmp_ret = 0;
    efuse_init();

    printf(">>>>>>>>>>>>>>>>>>>>\n");
    printf("test efuse read pgm = 0x%x \n", REG32(0x10230030));
    printf(">>>>>>>>>>>>>>>>>>>>\n");
#ifdef TEST_CIPH_IRQ
    efuse_irq_test();
#endif
    int ret = efuse_auto_write_data(addr, &wr_data, sizeof(wr_data));
    if (ret != 0) {
        function_err(efuse_auto_write_data);
    }

    printf("befor set write and read protect!!\n");

    ret |= efuse_auto_read_data(addr, &rd_data, sizeof(rd_data));
    if (ret != 0) {
        function_err(efuse_auto_read_data);
    }

    printf("check read data !!\n");
    if (wr_data == rd_data) {
        printf("PASS: check pass \n");

    } else {
        printf("ERROR: check error ! read data = 0x%x, write data = 0x%x \n", rd_data, wr_data);
        ret |= -1;
    }

    printf("after set write proct \n");
    tmp_ret = efuse_read_and_write_protect(addr, 1, true);
    if (tmp_ret != 0) {
        function_err(efuse_read_and_write_protect);
        ret |= tmp_ret;
    }

    wr_data = 0xaaaa;
    tmp_ret = efuse_auto_write_data(addr, &wr_data, sizeof(wr_data));
    if (tmp_ret != 0) {
        function_err(efuse_auto_write_data);
        ret |= tmp_ret;
    }

    rd_data = 0;
    tmp_ret = efuse_auto_read_data(addr, &rd_data, sizeof(rd_data));
    if (tmp_ret != 0) {
        function_err(efuse_auto_read_data);
        ret |= tmp_ret;
    }

    printf("check read data !!\n");
    if (wr_data == rd_data) {
        printf("ERROR: set write proct error ! read data = 0x%x, write data = 0x%x \n", rd_data, wr_data);
        ret |= -1;

    } else {
        printf("PASS: read data = 0x%x, write data = 0x%x \n", rd_data, wr_data);
    }

#if 1 /* 读保护需要在出厂的时候设置，目前不能进行读保护，避免用户操作失误 */
    printf("after set read proct \n");
    tmp_ret = efuse_read_and_write_protect(addr, 1, false);
    if (tmp_ret != 0) {
        function_err(efuse_read_and_write_protect);
        ret |= tmp_ret;
    }

    rd_data = 0;
    tmp_ret = efuse_auto_read_data(addr, &rd_data, sizeof(rd_data));
    if (tmp_ret != 0) {
        function_err(efuse_auto_read_data);
        ret |= tmp_ret;
    }

    printf("check read data !!\n");
    if (rd_data != 0) {
        printf("ERROR: set read proct error !read data = 0x%x, write data = 0x%x \n", rd_data, wr_data);
        ret |= -1;

    } else {
        printf("PASS: read data = 0x%x, write data = 0x%x \n", rd_data, wr_data);
    }
#endif

#if 0
    printf("test read soc enable !!\n");
    addr = 29;
    rd_data = 0;
    ret |= efuse_auto_read_data(addr, &rd_data, sizeof(rd_data));
    if (ret != 0) {
        function_err(efuse_auto_read_data);
        ret = -1;
    }
    printf("test read soc enable value = 0x%x !!\n", rd_data);

    int sysreg = REG32(0x2e000c44);
    printf("test read before write soc enable  sysreg default value = 0x%x !!\n", sysreg);
    if (sysreg != 0x401f) {
        printf("ERROR: test read soc default value error !!\n");
    }

    printf("test write soc enable !!\n");
    wr_data = 0xffffffff;
    ret |= efuse_auto_write_data(addr, &wr_data, sizeof(wr_data));
    if (ret != 0) {
        function_err(efuse_auto_write_data);
    }

    // systimer_delay(50 * 100, IN_US);
    efuse_init();

    printf("test read affter write soc enable !!\n");
    sysreg = REG32(0x2e000c44);
    printf("test read affter write soc enable sysreg value = 0x%x !!\n", sysreg);

    if (sysreg != ((~0x401f) & 0x7fff)) {
        printf("ERROR: test read soc enable error !!\n");
        ret = -1;
    } else {
        printf("PASS: test read soc enable PASS !!\n");
    }

#endif

    if (ret != 0) {
        printf("ERROR: test ERROR ! \n");
    } else {
        printf("PASS: test OK ! \n");
    }

}
