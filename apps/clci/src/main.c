#include <stdio.h>
#include <string.h>

#include "mailbox_sys.h"
#include "commands_sys.h"
#include "commands_common.h"
#include "mmio.h"
#include "d2d_api.h"
#include "clci_error.h"
#include "delay.h"
#include "systimer.h"
#include "clci_api.h"

#define INT_TO_FIXPT(x)         ((x) << 16)
#define FIXPT_TO_INT(x)         ((x) >> 16)

#define do_div(n, base) ({						\
	unsigned int __base = (base);					\
	unsigned int __rem;						\
	__rem = ((unsigned long long)(n)) % __base;			\
	(n) = ((unsigned long long)(n)) / __base;			\
	__rem;								\
})

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

static uint64_t calculate_KBs(uint64_t time_us, size_t len)
{
	uint64_t per_sec = 1000000;

	if (time_us <= 0)
		return 0;

	/* drop precision until time_us is 32-bits */
	while (time_us > (~0U)) {
		time_us >>= 1;
		per_sec <<= 1;
	}

	per_sec *= (len >> 10);
	per_sec = INT_TO_FIXPT(per_sec);
	do_div(per_sec, time_us);

	return FIXPT_TO_INT(per_sec);
}

static int data_trans_test(void)
{
    const uint32_t size = 1024;
    const uint32_t tar_addr = 0x45000000;
    int ret;
    uint32_t i;
    systimer_id_t timer;
    uint64_t start = 0;
    uint64_t elapsed = 0;
    char buf[size];

    printf("%s\n", __func__);

    for (i = 0; i < size; i++) {
        buf[i] = i & 0xff;
    }

    ret = rhea_d2d_select_tile(1, 0x04, 0);
    if (ret) return ret;
    systimer_init();
    timer = systimer_acquire_timer();
    start = systimer_get_elapsed_time(timer, IN_US);
    ret = rhea_d2d_write_data(buf, tar_addr, size);
    if (ret) return ret;
    elapsed = systimer_get_elapsed_time(timer, IN_US) - start;
    printf("It takes %ldus to write 0x%x bytes of memory data to 0x%x (%ld KB/s)\n", 
            elapsed, size, tar_addr, 
            calculate_KBs(elapsed, size));
    memset(buf, 0, size);
    start = systimer_get_elapsed_time(timer, IN_US);
    ret = rhea_d2d_read_data(buf, tar_addr, size);
    if (ret) return ret;
    elapsed = systimer_get_elapsed_time(timer, IN_US) - start;
    printf("It takes %ldus to read 0x%x bytes of memory data from 0x%x (%ld KB/s)\n", 
            elapsed, size, tar_addr, 
            calculate_KBs(elapsed, size));
    systimer_release_timer(timer);
    rhea_d2d_release_tile();

    printf("Start verification\n");
    for (i = 0; i < size; i++) {
        if (buf[i] != (i & 0xff)) {
            printf("Got %d at index %d, but expect %d\n",
                buf[i], i, i & 0xff);
            return -1;
        }
    }
    printf("Verification successful\n");
    return 0;
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

#if CONFIG_RHEA_D2D_SELF_ID == 0
    clci_device_reg_base_set(0x9c00000000 + 0x20200000 + 0x21000);
    mailbox_sys_init();
    printf("===[%d]mailbox_sys_init (%d)\n", __LINE__, ret);
	if (ret) goto stop;

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

#else

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

    ret = data_trans_test();
	if (ret) goto stop;

    for (i = 0; i < 4; i++) {
        printf("=== clci%d relink test\n", i);
        clci_device_reg_base_set(0x9c00000000 + 0x20200000 + (i * 0x200000) + 0x21000);
        mailbox_sys_init();
        printf("=== mailbox_sys_init\n");
        ret = clci_relink();
        printf("=== clci_relink (%d)\n", ret);
        if (ret) goto stop;
        printf("===[%d]%s clc0 0x%x, clci1 0x%x, clci2 0x%x, clci3 0x%x\n", __LINE__, __func__,
            mmio_read_32(0x9c00000000 + 0x20200000 + 0x3003c),
            mmio_read_32(0x9c00000000 + 0x20400000 + 0x3003c),
            mmio_read_32(0x9c00000000 + 0x20600000 + 0x3003c),
            mmio_read_32(0x9c00000000 + 0x20800000 + 0x3003c));
        link_status = clci_link_status();
        printf("=== clci%d link_status:0x%x, 0x3003c: 0x%x, 0x21054:0x%x, 0x30004:0x%x, 0x17c1c:0x%x, 0x17c24:0x%x, 0x17c30:0x%x\n",
            i, link_status,
            mmio_read_32(0x9c00000000 + 0x20200000 + (i * 0x200000) + 0x3003c),
            mmio_read_32(0x9c00000000 + 0x20200000 + (i * 0x200000) + 0x21054),
            mmio_read_32(0x9c00000000 + 0x20200000 + (i * 0x200000) + 0x30004),
            mmio_read_32(0x9c00000000 + 0x20200000 + (i * 0x200000) + 0x17c1c),
            mmio_read_32(0x9c00000000 + 0x20200000 + (i * 0x200000) + 0x17c24),
            mmio_read_32(0x9c00000000 + 0x20200000 + (i * 0x200000) + 0x17c30));
        ret = data_trans_test();
        if (ret) goto stop;
        ret = clci_relink_2();
        printf("[%d]clci_relink_2 (%d)\n", __LINE__, ret);
        if (ret) goto stop;
        printf("===[%d]%s clc0 0x%x, clci1 0x%x, clci2 0x%x, clci3 0x%x\n", __LINE__, __func__,
            mmio_read_32(0x9c00000000 + 0x20200000 + 0x3003c),
            mmio_read_32(0x9c00000000 + 0x20400000 + 0x3003c),
            mmio_read_32(0x9c00000000 + 0x20600000 + 0x3003c),
            mmio_read_32(0x9c00000000 + 0x20800000 + 0x3003c));
        link_status = clci_link_status();
        printf("=== clci%d link_status:0x%x, 0x3003c: 0x%x, 0x21054:0x%x, 0x30004:0x%x, 0x17c1c:0x%x, 0x17c24:0x%x, 0x17c30:0x%x\n",
            i, link_status,
            mmio_read_32(0x9c00000000 + 0x20200000 + (i * 0x200000) + 0x3003c),
            mmio_read_32(0x9c00000000 + 0x20200000 + (i * 0x200000) + 0x21054),
            mmio_read_32(0x9c00000000 + 0x20200000 + (i * 0x200000) + 0x30004),
            mmio_read_32(0x9c00000000 + 0x20200000 + (i * 0x200000) + 0x17c1c),
            mmio_read_32(0x9c00000000 + 0x20200000 + (i * 0x200000) + 0x17c24),
            mmio_read_32(0x9c00000000 + 0x20200000 + (i * 0x200000) + 0x17c30));
        ret = data_trans_test();
        if (ret) goto stop;
    }


#endif
stop:
    printf("===[%d] stop\n", __LINE__);
	while (1) {
        for (i = 0; i < 4; i++) {
            printf("=== clci%d 0x3003c: 0x%x, 0x21054:0x%x, 0x30004:0x%x, 0x17c1c:0x%x, 0x17c24:0x%x, 0x17c30:0x%x\n",
                i,
                mmio_read_32(0x9c00000000 + 0x20200000 + (i * 0x200000) + 0x3003c),
                mmio_read_32(0x9c00000000 + 0x20200000 + (i * 0x200000) + 0x21054),
                mmio_read_32(0x9c00000000 + 0x20200000 + (i * 0x200000) + 0x30004),
                mmio_read_32(0x9c00000000 + 0x20200000 + (i * 0x200000) + 0x17c1c),
                mmio_read_32(0x9c00000000 + 0x20200000 + (i * 0x200000) + 0x17c24),
                mmio_read_32(0x9c00000000 + 0x20200000 + (i * 0x200000) + 0x17c30));
        }
    }
    return 0;
}