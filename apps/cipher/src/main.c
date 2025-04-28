#ifndef VIRT
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "systimer.h"
#include "cipher.h"
#include "cipher_test_debg.h"

void spacc_test(void);
int pka_ecc_test(void);
int ic_test(void);
void hash_test(void);
void efuse_sec_test(void);
int main()
{
	printf(" ****************************** \n");
	systimer_init();
	printf(" ****************************** \n");
#ifdef TEST_CIPH_CIPHER
 	spacc_test();
#endif

#ifdef TEST_CIPH_HASH
 	hash_test();
#endif

#ifdef TEST_CIPH_ECC
	pka_ecc_test();
#endif
	// ic_test();

#ifdef TEST_EFUSE_SEC
	efuse_sec_test();
#endif
	while (1) {
		/* code */
	}
	return 0;
}

#else
#include "main_qemu.c"
#endif
