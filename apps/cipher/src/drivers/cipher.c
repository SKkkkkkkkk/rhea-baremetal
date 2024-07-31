#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "cipher.h"
#include "spacc.h"
#include "cipher_debug.h"

#define ENCRYPT 1
#define DECRYPT 0

#define str_to_u32_big(str, u32data)                                                                                   \
	do {                                                                                                           \
		(u32data) = (str)[0] << 3 * 8 | (str)[1] << 2 * 8 | (str)[2] << 1 * 8 | (str)[3];                      \
		(str) += 4;                                                                                            \
	} while (0)

#define str_to_u32_lit(str, u32data)                                                                                   \
	do {                                                                                                           \
		(u32data) = (str)[0] | (str)[1] << 1 * 8 | (str)[2] << 2 * 8 | (str)[3] << 3 * 8;                      \
		(str) += 4;                                                                                            \
	} while (0)

static bool g_is_init = false;
static cipher_ctrl *g_ctrl = NULL;
static uint32_t g_block_sz = 0;

static int cipher_config_ctrl(cipher_ctrl *ctrl)
{
	int ret;
	spacc_alg_type alg_t = ALE_BUTT;
	spacc_mode_type mode_t = MODE_BUTT;

	uint32_t key_len = 0;
	switch (ctrl->key_info.keylen) {
	case CIPHER_KEY_AES_128BIT:
		key_len = 16;
		break;
	case CIPHER_KEY_AES_192BIT:
		key_len = 24;
		break;
	case CIPHER_KEY_AES_256BIT:
		key_len = 32;
		break;
	default:
		cipher_debug_err("error: Invalid keylen !\n");
		return SEEHI_FAILED;
	}

	if (g_ctrl == NULL) {
		g_ctrl = ctrl;
	}

	ret = spacc_config_key(ctrl->key_info.key, key_len, ctrl->key_info.key_by_efuse, true);
	if (ret != SEEHI_SUCCESS) {
		cipher_function_err(spacc_config_key);
		return SEEHI_FAILED;
	}

	uint32_t block_size = 0;
	switch (ctrl->alg_info.bit_width) {
	case CIPHER_BIT_WIDTH_128BIT:
		block_size = 16;
		break;
	default:
		cipher_debug_err("error: Bit width supports only 128 bits of data width !\n");
		return -1;
	}

	switch (ctrl->alg_info.alg) {
	case CIPHER_ALG_AES:
		alg_t = CIPH_ALG_AES;
		break;
	default:
		return -1;
	}

	switch (ctrl->alg_info.work_mode) {
	case CIPHER_WORK_MODE_CBC:
		mode_t = CIPH_MODE_CBC;
		break;
	case CIPHER_WORK_MODE_GCM:
		mode_t = CIPH_MODE_GCM;
	case CIPHER_WORK_MODE_EBC:
		mode_t = CIPH_MODE_GCM;
		break;

	default:
		cipher_debug_err("error: Invalid work mode!\n");
		return -1;
	}

	uint32_t iv_len = g_block_sz = block_size;
	ret = spacc_config_iv(ctrl->iv, iv_len, mode_t, key_len, true);
	if (ret != SEEHI_SUCCESS) {
		cipher_function_err(spacc_config_iv);
		return SEEHI_FAILED;
	}

	ret = spacc_cfg_paket_format(mode_t, ctrl->aad, sizeof(ctrl->aad), ctrl->icv, sizeof(ctrl->icv));
	if (ret != SEEHI_SUCCESS) {
		cipher_function_err(spacc_cfg_paket_format);
		return SEEHI_FAILED;
	}

	ret = spacc_config_ctrl(alg_t, mode_t, ctrl->key_info.key_by_efuse);
	if (ret != SEEHI_SUCCESS) {
		cipher_function_err(spacc_config_ctrl);
		return SEEHI_FAILED;
	}

	return 0;
}

static int cipher_algorithm_operate(const uint8_t *src_data, uint32_t src_len, uint8_t *dest_data, uint32_t *out_len,
				    bool is_encrypt, bool last_pkt)
{
	int ret;
	ret = spacc_send_data(src_data, src_len, (spacc_run_type)is_encrypt, last_pkt);
	if (ret != SEEHI_SUCCESS) {
		cipher_function_err(spacc_send_data);
		return SEEHI_FAILED;
	}
	// L1C_CleanDCacheAll();

	ret = spacc_run((spacc_run_type)is_encrypt);
	if (ret != SEEHI_SUCCESS) {
		cipher_function_err(spacc_run);
		return SEEHI_FAILED;
	}

	// L1C_InvalidateDCacheAll();
	ret = spacc_get_data(dest_data, out_len, (spacc_run_type)is_encrypt, last_pkt);
	if (ret != SEEHI_SUCCESS) {
		cipher_function_err(spacc_get_data);
		return SEEHI_FAILED;
	}

	return 0;
}

void cipher_init(void)
{
	spacc_init(true);
	g_is_init = true;
	g_block_sz = 0;
}

int cipher_creat_handle(int *hcipher)
{
	if (g_is_init != true) {
		*hcipher = -1;
		cipher_debug_err("error: cipher uninitialized !\n");
		return SEEHI_FAILED;
	}

	*hcipher = 0;
	return SEEHI_SUCCESS;
}

int cipher_config_handle(int hcipher, cipher_ctrl *ctrl)
{
	if (hcipher == -1 || ctrl == NULL) {
		cipher_parem_invalid();
		return SEEHI_FAILED;
	}

	int ret = cipher_config_ctrl(ctrl);
	if (ret != 0) {
		cipher_function_err(cipher_config_ctrl);
		return SEEHI_FAILED;
	}

	return SEEHI_SUCCESS;
}

int cipher_encrypt(int hcipher, const uint8_t *src_data, uint32_t src_len, uint8_t *dest_data, uint32_t *out_len)
{
	if (hcipher == -1 || src_data == NULL || src_len == 0) {
		cipher_parem_invalid();
		return SEEHI_FAILED;
	}

	int ret = cipher_algorithm_operate(src_data, src_len, dest_data, out_len, ENCRYPT, true);
	if (ret != 0) {
		cipher_function_err(cipher_algorithm_operate);
		return SEEHI_FAILED;
	}

	return SEEHI_SUCCESS;
}

int cipher_decrypt(int hcipher, const uint8_t *src_data, uint32_t src_len, uint8_t *dest_data, uint32_t *out_len)
{
	if (hcipher == -1 || src_data == NULL || src_len == 0) {
		cipher_parem_invalid();
		return SEEHI_FAILED;
	}

	int ret = cipher_algorithm_operate(src_data, src_len, dest_data, out_len, DECRYPT, true);
	if (ret != 0) {
		cipher_function_err(cipher_algorithm_operate);
		return SEEHI_FAILED;
	}

	return SEEHI_SUCCESS;
}

int cipher_get_tag(int hcipher, uint8_t *tag)
{
	if (hcipher == -1 || tag == NULL) {
		cipher_parem_invalid();
		return SEEHI_FAILED;
	}

	cipher_ctrl *ctrl = g_ctrl;
	if (ctrl == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	if (ctrl->alg_info.work_mode != CIPHER_WORK_MODE_GCM) {
		cipher_debug_err("error: this work mode is not gcm !\n");
		return -1;
	}

	memcpy(tag, ctrl->icv, sizeof(ctrl->icv));
	memset(ctrl->icv, 0, sizeof(ctrl->icv));

	return SEEHI_SUCCESS;
}

int cipher_encrypt_mult(int hcipher, const uint8_t *src_data, uint32_t src_len, uint8_t *dest_data, uint32_t *out_len)
{
	if (hcipher == -1 || src_data == NULL || src_len == 0 || dest_data == NULL || out_len == NULL) {
		cipher_parem_invalid();
		return SEEHI_FAILED;
	}

	const uint8_t *in_data = src_data;
	uint8_t *out_data = dest_data;
	int pkt_num = (src_len - 1 + CIPHER_MAX_PKT_LEN) / CIPHER_MAX_PKT_LEN;
	cipher_debug_dbg("pkt_num = %d\n", pkt_num);

	int remain_len = src_len;
	int pkt_len;
	int i = 0;
	int ret;
	bool last_pkt = false;
	cipher_ctrl *ctrl = g_ctrl;
	if (ctrl == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	if (ctrl->alg_info.work_mode == CIPHER_WORK_MODE_GCM) {
		cipher_debug_err("error: Multi-packet encryption does not support the AES-GCM algorithm pattern! \n");
		return -1;
	}

	uint32_t ori_iv[sizeof(ctrl->iv) / sizeof(uint32_t)] = { 0 };
	memcpy(ori_iv, ctrl->iv, sizeof(ctrl->iv));
	uint32_t *tmp = NULL;

	uint32_t dstpkt_len = 0;
	for (i = 0; i < pkt_num; i++) {
		last_pkt = (i == pkt_num - 1);
		dstpkt_len = 0;
		cipher_debug_dbg("encrypt no.%d\n", i);
		pkt_len = CIPHER_MAX_PKT_LEN > remain_len ? remain_len : CIPHER_MAX_PKT_LEN;
		ret = cipher_algorithm_operate(in_data, pkt_len, out_data, &dstpkt_len, ENCRYPT, last_pkt);
		if (ret != 0) {
			cipher_function_err(cipher_algorithm_operate);
			return SEEHI_FAILED;
		}

		*out_len += dstpkt_len;
		if (!last_pkt) {
			in_data += pkt_len;
			out_data += pkt_len;
			remain_len -= pkt_len;

			tmp = (uint32_t *)(out_data - sizeof(ctrl->iv));
		} else {
			tmp = ori_iv;
		}

		if (ctrl->alg_info.work_mode == CIPHER_WORK_MODE_EBC) {
			continue;
		}

		if (ctrl->alg_info.work_mode == CIPHER_WORK_MODE_CBC) {
			memcpy(ctrl->iv, (void *)tmp, sizeof(ctrl->iv));
		}

		ret = cipher_config_ctrl(ctrl);
		if (ret != 0) {
			cipher_function_err(cipher_config_ctrl);
			return SEEHI_FAILED;
		}
	}

	return 0;
}

int cipher_decrypt_mult(int hcipher, const uint8_t *src_data, uint32_t src_len, uint8_t *dest_data, uint32_t *out_len)
{
	if (hcipher == -1 || src_data == NULL || src_len == 0 || dest_data == NULL || out_len == NULL) {
		cipher_parem_invalid();
		return SEEHI_FAILED;
	}

	const uint8_t *in_data = src_data;
	uint8_t *out_data = dest_data;
	int pkt_num = (src_len - 1 + CIPHER_MAX_PKT_LEN) / CIPHER_MAX_PKT_LEN;

	cipher_debug_dbg("pkt_num = %d\n", pkt_num);

	int remain_len = src_len;
	int pkt_len;
	int i = 0;
	int ret;
	bool last_pkt = false;
	cipher_ctrl *ctrl = g_ctrl;
	if (ctrl == NULL) {
		return -1;
	}

	if (ctrl->alg_info.work_mode == CIPHER_WORK_MODE_GCM) {
		cipher_debug_err("error: Multi-packet decryption does not support the AES-GCM algorithm pattern! \n");
		return -1;
	}

	uint32_t ori_iv[sizeof(ctrl->iv) / sizeof(uint32_t)] = { 0 };
	memcpy(ori_iv, ctrl->iv, sizeof(ctrl->iv));
	uint32_t *tmp = NULL;

	uint32_t dstpkt_len;
	for (i = 0; i < pkt_num; i++) {
		last_pkt = (i == pkt_num - 1);
		cipher_debug_dbg("decrypt no.%d\n", i);
		if (CIPHER_MAX_PKT_LEN >= remain_len) {
			if (remain_len < g_block_sz) {
				return -1;
			}
			pkt_len = remain_len;
		} else {
			if ((remain_len - CIPHER_MAX_PKT_LEN) <= g_block_sz) {
				pkt_len = CIPHER_MAX_PKT_LEN - g_block_sz;
			} else {
				pkt_len = CIPHER_MAX_PKT_LEN;
			}
		}

		ret = cipher_algorithm_operate(in_data, pkt_len, out_data, &dstpkt_len, DECRYPT, last_pkt);
		if (ret != 0) {
			cipher_function_err(cipher_algorithm_operate);
			return SEEHI_FAILED;
		}

		*out_len += dstpkt_len;

		if (!last_pkt) {
			in_data += pkt_len;
			out_data += pkt_len;
			remain_len -= pkt_len;

			tmp = (uint32_t *)(in_data - sizeof(ctrl->iv));
			// break;
		} else {
			// tmp = ori_iv;
			break;
		}

		if (ctrl->alg_info.work_mode == CIPHER_WORK_MODE_EBC) {
			continue;
		}
		if (ctrl->alg_info.work_mode == CIPHER_WORK_MODE_CBC) {
			memcpy(ctrl->iv, (void *)tmp, sizeof(ctrl->iv));
		}

		ret = cipher_config_ctrl(ctrl);
		if (ret != 0) {
			cipher_function_err(cipher_config_ctrl);
			return SEEHI_FAILED;
		}
	}

	return 0;
}

int cipher_destroy_handle(int hcipher)
{
	return SEEHI_SUCCESS;
}

void cipher_deinit()
{
	spacc_deinit(false);
	g_is_init = false;
	g_ctrl = NULL;
}