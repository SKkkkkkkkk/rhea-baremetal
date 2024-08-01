#include <stdio.h>
#include <stdint.h>
#include "cipher.h"
#include "spacc.h"
#include "cipher_debug.h"

// #define CIPHER_MAX_PKT_LEN (64 * 1024)  /* spacc只支持128bit位宽的加解密，单个包只支持64k大小的数据 */

static uint32_t g_block_size = 0;
static bool g_is_first = true;

int cipher_hash_init(cipher_hash_atts *hash_attr, int *handle)
{
	int ret;
	spacc_init(false);

	ret = spacc_config_key(hash_attr->hash_key, hash_attr->key_len, false, false);
	if (ret != 0) {
		cipher_function_err(spacc_config_key);
		spacc_deinit(false);
		return -1;
	}

	ret = spacc_config_iv(NULL, 0, MODE_BUTT, 0, false);
	if (ret != 0) {
		cipher_function_err(spacc_config_iv);
		spacc_deinit(false);
		return -1;
	}

	ret = spacc_cfg_paket_format(HASH_MODE_RAW, NULL, 0, NULL, 0);
	if (ret != 0) {
		cipher_function_err(spacc_cfg_paket_format);
		spacc_deinit(false);
		return -1;
	}

	switch (hash_attr->hash_type) {
	case CIPH_HASH_TYPE_SHA3_256:
		ret = spacc_config_ctrl(HASH_ALE_SHA3_256, HASH_MODE_RAW, false);
		if (ret != 0) {
			cipher_function_err(spacc_config_ctrl);
			spacc_deinit(false);
			return -1;
		}

		g_block_size = 32;

		break;

	default:
		cipher_debug_err("error: Only support SHA3-256 mode !\n");
		spacc_deinit(false);
		return -1;
		break;
	}

	*handle = 0;
	return 0;
}

int cipher_hash_update(int handle, const uint8_t *in_data, uint32_t data_len)
{
	if (handle == -1 || in_data == NULL || data_len == 0) {
		cipher_parem_invalid();
		return -1;
	}

	if (data_len > CIPHER_MAX_PKT_LEN) {
		cipher_parem_invalid();
		cipher_debug_err("Hash supports a single operation of up to %d bytes in size ！\n", (CIPHER_MAX_PKT_LEN));
		return -1;
	}

	int ret;
#if 0
    uint8_t data[CIPHER_MAX_PKT_LEN] = {0};
    uint32_t hash_len = 0;

    if (g_is_first != true) {
        hash_len = g_block_size;
        ret = spacc_get_data(data, hash_len, RUN_HASH);
        if (ret != SEEHI_SUCCESS) {
            function_err(spacc_get_data);
            return SEEHI_FAILED;
        }
    }

    g_is_first = false;

    memcpy((data + hash_len), in_data, data_len);
    ret = spacc_config_ctrl(HASH_ALE_SHA3_256, HASH_MODE_RAW);
#else
	ret = spacc_send_data(in_data, data_len, RUN_HASH, false);
#endif
	if (ret != SEEHI_SUCCESS) {
		cipher_function_err(spacc_send_data);
		return SEEHI_FAILED;
	}

	ret = spacc_run(RUN_HASH);
	if (ret != SEEHI_SUCCESS) {
		cipher_function_err(spacc_run);
		return SEEHI_FAILED;
	}

	return 0;
}

int cipher_hash_final(int handle, uint8_t *hash_data)
{
	if (handle == -1 || hash_data == NULL) {
		cipher_parem_invalid();
		return -1;
	}

	int ret;
	uint32_t out_len = 0;
	ret = spacc_get_data(hash_data, &out_len, RUN_HASH, false);
	if (ret != SEEHI_SUCCESS) {
		cipher_function_err(spacc_get_data);
		return SEEHI_FAILED;
	}

	g_is_first = true;
	return 0;
}

int cipher_hash(int handle, uint8_t *src_data, uint32_t length, uint8_t *hash_data)
{
	if (handle == -1 || src_data == NULL || length == 0) {
		cipher_parem_invalid();
		return SEEHI_FAILED;
	}
#if 0
    const uint8_t *in_data = src_data;
    uint8_t *out_data = hash_data;
    const uint8_t *tmp = NULL;
    int pkt_num = (length - 1 + CIPHER_MAX_PKT_LEN - g_block_size) / (CIPHER_MAX_PKT_LEN - g_block_size);

    cipher_debug_err("pkt_num = %d\n", pkt_num);

    int remain_len = length;
    int pkt_len;
    int i = 0;
    int ret;

    for (i = 0; i < pkt_num; i++) {
        cipher_seehi_printf("decrypt no.%d\n", i);
        pkt_len = (CIPHER_MAX_PKT_LEN - g_block_size) > remain_len ? remain_len : (CIPHER_MAX_PKT_LEN - g_block_size);
        ret = cipher_hash_update(handle, src_data, pkt_len);
        if (ret != 0) {
            function_err(cipher_hash_update);
            return SEEHI_FAILED;
        }
    }

    ret = cipher_hash_final(handle, hash_data);
    if (ret != SEEHI_SUCCESS) {
        cipher_function_err(cipher_hash_final);
        return SEEHI_FAILED;
    }
#endif
	return SEEHI_SUCCESS;
}

void cipher_hash_deinit(void)
{
	g_block_size = 0;
	g_is_first = true;
	spacc_deinit(false);
}
