#include <stdio.h>
#include <stdint.h>
#include "cipher.h"
#include "efuse.h"
#include "cipher_debug.h"

int cipher_efuse_burn_key(cipher_efuse_tar_get target, uint32_t *key, uint32_t key_len)
{
	if (key == NULL) {
		cipher_parem_invalid();
		return -1;
	}

	if (key_len > 32) { /* 256bit */
		cipher_parem_invalid();
		return -1;
	}

	if (target >= CIPHER_EFUSE_TARGET_BUTT) {
		cipher_parem_invalid();
		return -1;
	}

	int ret;
	ret = efuse_init();
	if (ret != 0) {
		cipher_function_err(efuse_init);
		return -1;
	}

	ret = efuse_burn_key(key, key_len, target);
	if (ret != 0) {
		cipher_function_err(efuse_burn_key);
		return -1;
	}

	return 0;
}

int cipher_efuse_attach_key(cipher_efuse_tar_get target)
{
	int ret;
	ret = efuse_init();
	if (ret != 0) {
		cipher_function_err(efuse_init);
		return -1;
	}
	if (target == CIPHER_EFUSE_TARGET_AES) {
		ret = efuse_private_aes_key();
		if (ret != 0) {
			cipher_function_err(efuse_private_aes_key);
			return -1;
		}
	} else if (target == CIPHER_EFUSE_TARGET_ECC) {
		ret = efuse_private_ecc_key();
		if (ret != 0) {
			cipher_function_err(efuse_private_ecc_key);
			return -1;
		}
	} else {
		cipher_parem_invalid();
		return -1;
	}

	return 0;
}