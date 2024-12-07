#include <stdio.h>

#include "mailbox_sys.h"
#include "commands_sys.h"
#include "commands_common.h"
#include "mmio.h"
#include "d2d_api.h"
// #include "io.h"
#include "clci_error.h"
#include "delay.h"

int print_flag = 0;

#define TCM_04_CFG_BASE         0x0015000000
void mc_init(uint64_t addr, uint8_t layer) {
	// global
	if (layer == 4) {
		mmio_write_32(addr+0x00013054, 0x00000000);
		mmio_write_32(addr+0x00013004, 0x00001000); /* 2GB: 0x00001000, 512MB: 0x00000000 */
		mmio_write_32(addr+0x00013004, 0x80001000); /* 2GB: 0x80001000, 512MB: 0x80000000 */
	} else {
		mmio_write_32(addr+0x00013054, 0x00000000);
		mmio_write_32(addr+0x00013004, 0x00000010);
		mmio_write_32(addr+0x00013004, 0x80000010);
	}

	// bank
	uint32_t i, j, k;
	for (i = 0; i < 72; i++) {
		j = i / 18;
		k = i + j; // skip hub regs
		mmio_write_32(addr+k*0x400+0x004, 0x00000005);
		mmio_write_32(addr+k*0x400+0x004, 0x00000001);
		mmio_write_32(addr+k*0x400+0x004, 0x80000001);
	}
}

int main(void)
{
    int link_status;
	int ret;
    uint32_t addr, val, i;

    printf("die%d test start\n", CONFIG_RHEA_D2D_SELF_ID);

	mc_init(TCM_04_CFG_BASE, 4);
    printf("mc_init finished\n");

    ret = rhea_d2d_init();
    printf("rhea_d2d_init finished (%d)\n", ret);
	if (ret) goto stop;

    // do {
    //     val = mmio_read_32(0x9c00000000+0x20200000+0x2105c);
    //     printf("===[%d]%s  0x2105c:0x%x, 0x3003c: 0x%x, 0x21054:0x%x, 0x30004:0x%x, 0x17c1c:0x%x, 0x17c24:0x%x, 0x17c30:0x%x\n",
    //         __LINE__, __func__, val,
    //         mmio_read_32(0x9c00000000 + 0x20200000 + 0x3003c),
    //         mmio_read_32(0x9c00000000 + 0x20200000 + 0x21054),
    //         mmio_read_32(0x9c00000000 + 0x20200000 + 0x30004),
    //         mmio_read_32(0x9c00000000 + 0x20200000 + 0x17c1c),
    //         mmio_read_32(0x9c00000000 + 0x20200000 + 0x17c24),
    //         mmio_read_32(0x9c00000000 + 0x20200000 + 0x17c30));
    // } while ((val & 0xf) != 0x2);

#if CONFIG_RHEA_D2D_SELF_ID == 0
    mailbox_sys_init();
    printf("===[%d]mailbox_sys_init (%d)\n", __LINE__, ret);
	if (ret) goto stop;

    // do {
    //     link_status = clci_link_status();
    //     printf("===[%d]%s  link_status:0x%x, 0x3003c: 0x%x, 0x21054:0x%x, 0x30004:0x%x, 0x17c1c:0x%x, 0x17c24:0x%x, 0x17c30:0x%x\n",
    //         __LINE__, __func__, link_status,
    //         mmio_read_32(0x9c00000000 + 0x20200000 + 0x3003c),
    //         mmio_read_32(0x9c00000000 + 0x20200000 + 0x21054),
    //         mmio_read_32(0x9c00000000 + 0x20200000 + 0x30004),
    //         mmio_read_32(0x9c00000000 + 0x20200000 + 0x17c1c),
    //         mmio_read_32(0x9c00000000 + 0x20200000 + 0x17c24),
    //         mmio_read_32(0x9c00000000 + 0x20200000 + 0x17c30));
    // } while (link_status);

    // ret = rhea_d2d_select_tile(1, 0x04, 0);
    // printf("===[%d]rhea_d2d_select_tile (%d)\n", __LINE__, ret);
	// if (ret) goto stop;
    // for (i = 0; i <= 5; i++) {
    //     udelay(1000);
    //     rhea_d2d_writel(i, 0x45000000);
    //     val = 0;
    //     rhea_d2d_readl(&val, 0x45000000);
    //     printf("===[%d] i: %d, val:%d\n", __LINE__, i, val);
    // }
    // rhea_d2d_release_tile();

    // while (1) {
    //     val = mmio_read_32(0x45000000);
    //     printf("===[%d] 0x45000000:0x%x\n", __LINE__, val);
    //     if (val == 5) break;
    // }

    goto stop;
#else
    mailbox_sys_init();
    printf("===[%d]mailbox_sys_init (%d)\n", __LINE__, ret);

    // while (1) {
    //     val = mmio_read_32(0x45000000);
    //     printf("===[%d] 0x45000000:0x%x\n", __LINE__, val);
    //     if (val == 5) break;
    // }

    // ret = rhea_d2d_select_tile(1, 0x04, 0);
    // printf("===[%d]rhea_d2d_select_tile (%d)\n", __LINE__, ret);
	// if (ret) goto stop;
    // for (i = 0; i <= 5; i++) {
    //     udelay(1000);
    //     rhea_d2d_writel(i, 0x45000000);
    //     val = 0;
    //     rhea_d2d_readl(&val, 0x45000000);
    //     printf("===[%d] i: %d, val:%d\n", __LINE__, i, val);
    // }
    // rhea_d2d_release_tile();

    // ret = cmd_clci_get_reg(1, 0x2105c, &val);
    // printf("===[%d]%s 1 ret %d, 0x2105c 0x%x\n", __LINE__, __func__, ret, val);

    // ret = clci_relink();
    ret = clci_relink_2();
    printf("[%d]clci_relink_2 (%d)\n", __LINE__, ret);
	if (ret) goto stop;

    while (1) {
        link_status = clci_link_status();
        printf("===[%d]%s  link_status:0x%x, 0x3003c: 0x%x, 0x21054:0x%x, 0x30004:0x%x, 0x17c1c:0x%x, 0x17c24:0x%x, 0x17c30:0x%x\n",
            __LINE__, __func__, link_status,
            mmio_read_32(0x9c00000000 + 0x20200000 + 0x3003c),
            mmio_read_32(0x9c00000000 + 0x20200000 + 0x21054),
            mmio_read_32(0x9c00000000 + 0x20200000 + 0x30004),
            mmio_read_32(0x9c00000000 + 0x20200000 + 0x17c1c),
            mmio_read_32(0x9c00000000 + 0x20200000 + 0x17c24),
            mmio_read_32(0x9c00000000 + 0x20200000 + 0x17c30));
    }

    goto stop;
#endif
stop:
	while (1)
		;
    return 0;
}