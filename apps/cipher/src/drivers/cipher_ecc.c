#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "cipher.h"
#include "pka.h"
#include "cipher_debug.h"

#define CIHPER_ECC_MAX_LEN 256


static uint32_t cipher_get_parse_bitlen(cipher_ecc_sign_scheme scheme)
{
	uint32_t len = 0;
	switch (scheme) {
	case CIHPER_ECC_256BIT:
		len = 32;
		break;
		// case CIHPER_ECC_512BIT:
		//     len = 64;
		//     break;
		// case CIHPER_ECC_1024BIT:
		//     len = 128;
		//     break;
		// case CIHPER_ECC_2048BIT:
		//     len = 256;
		//     break;
	default:
		cipher_debug_err("error: Only support 256 - bit !\n");
		break;
	}

	return len;
}

void cipher_ecc_init(void)
{
	pka_init();
}

int cipher_get_pubkey(cipher_ecc_sign_s *ecc_sign, uint32_t *pub_key_Bx, uint32_t *pub_key_By)
{
	if (ecc_sign == NULL || pub_key_Bx == NULL || pub_key_By == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	uint32_t len = cipher_get_parse_bitlen(ecc_sign->scheme);
	if (len == 0 || len != ecc_sign->ecc_param.len || len != ecc_sign->pri_key.key_len) {
		cipher_function_err(cipher_get_parse_bitlen);
		return -1;
	}

	pka_piont A = { ecc_sign->ecc_param.Ax, ecc_sign->ecc_param.Ay, len };
	pka_piont B = { pub_key_Bx, pub_key_By, len };
	int ret;
	pka_ecc_param_info pka_p = { 0 };
	pka_p.a = ecc_sign->ecc_param.a;
	pka_p.b = ecc_sign->ecc_param.b;
	pka_p.k = ecc_sign->pri_key.d;
	pka_p.p = ecc_sign->ecc_param.p;
	pka_p.w = 0;
	pka_p.len = len;
	ret = pka_ecc_point_mult(&pka_p, &A, &B, ecc_sign->scheme);
	if (ret != 0) {
		cipher_function_err(pka_ecc_point_mult);
		return -1;
	}

	return 0;
}

int cipher_ecc_sign(cipher_ecc_sign_s *ecc, uint8_t *in_data, uint32_t data_len, uint8_t *hash_data, uint32_t hash_len,
		    ecc_sign_s *out_sign)
{
	if (ecc == NULL || out_sign == NULL || out_sign->r == NULL || out_sign->s == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	// while (1)
	// {
	// 	/* code */
	// 	printf("while===========================>\n");
	// }
	// uint32_t len = cipher_get_parse_bitlen(ecc->scheme);
	// if (len == 0 || len != ecc->ecc_param.len || len != ecc->pri_key.key_len) {
	// 	cipher_function_err(cipher_get_parse_bitlen);
	// 	return -1;
	// }

	uint32_t len = 32;//= cipher_get_parse_bitlen(ecc->scheme);
	int ret;
	uint32_t *calc_hash = NULL;
	if (hash_data == NULL) {
		if (in_data == NULL) {
			cipher_pointer_invalid();
			return -1;
		}

		// 用data计算出哈希值
		/* 暂未实现 */
		return -1;

	} else {
		if (hash_len > len) {
			cipher_parem_invalid();
			return -1;
		}
		// memcpy((void *)calc_hash, hash_data, hash_len);
		calc_hash = (uint32_t *)(uintptr_t)hash_data;
	}

	/* 计算 R = Ke*A */
	pka_ecc_param_info pka_p = { 0 };
	pka_p.a = ecc->ecc_param.a;
	pka_p.b = ecc->ecc_param.b;
	pka_p.k = ecc->ecc_param.k;
	pka_p.p = ecc->ecc_param.p;
	pka_p.w = 0;
	pka_p.len = len;

	pka_piont A = { .x = ecc->ecc_param.Ax, .y = ecc->ecc_param.Ay, .len = len };
	pka_piont R = { .x = out_sign->r, .y = NULL, .len = len };

	ret = pka_ecc_point_mult(&pka_p, &A, &R, ecc->scheme);
	if (ret != 0) {
		cipher_function_err(pka_ecc_point_mult);
		return -1;
	}
	// while (1)
	// {
	// 	/* code */
	// 	printf("while===========================>\n");
	// }

	/* 计算 s = (h(x) + d*r) * k^-1 mod q */
	pka_base_modul_adapter *pka_adp = pka_base_modul_adapter_init(ecc->scheme);
	if (pka_adp == NULL) {
		cipher_function_err(pka_base_modul_adapter_init);
		return -1;
	}

	/* 1) tmp = d * r mod q */
	static uint32_t tmp[CIHPER_ECC_MAX_LEN / 4] = { 0 };
	memset(tmp, 0, sizeof(tmp));

	if (ecc->key_from == CIPHER_KEY_SRC_USER) {
		if (ecc->pri_key.d == NULL) {
			pka_base_mod_adapter_deinit();
			cipher_parem_invalid();
			return -1;
		}

	} else if (ecc->key_from == CIPHER_KEY_SRC_EFUSE) {
		ecc->pri_key.d = NULL;
		pka_adp->is_sec_key = 1;
		pka_adp->is_mult_key = 1;
	} else {
		pka_base_mod_adapter_deinit();
		cipher_parem_invalid();
		return -1;
	}

	ret = pka_adp->mult(pka_adp, ecc->pri_key.d, out_sign->r, ecc->ecc_param.q, tmp, len);
	if (ret != 0) {
		pka_base_mod_adapter_deinit();
		cipher_function_err(pka_adp->mult);
		return -1;
	}

	pka_adp->is_mult_key = 0;

	/* 2) kinv = k^-1 mod q */
	static uint32_t kinv[CIHPER_ECC_MAX_LEN / 4] = { 0 };
	memset(kinv, 0, sizeof(kinv));

	ret = pka_adp->inv(pka_adp, ecc->ecc_param.k, ecc->ecc_param.q, kinv, len);
	if (ret != 0) {
		cipher_function_err(pka_adp->inv);
		pka_base_mod_adapter_deinit();
		return -1;
	}

	/* 3) tmp = (h(x) + tmp) mod q */
	ret = pka_adp->add(pka_adp, calc_hash, tmp, ecc->ecc_param.q, tmp, len);
	if (ret != 0) {
		cipher_function_err(pka_adp->add);
		pka_base_mod_adapter_deinit();
		return -1;
	}

	/* 4) s = tmp * kinv mod q */
	ret = pka_adp->mult(pka_adp, tmp, kinv, ecc->ecc_param.q, out_sign->s, len);
	if (ret != 0) {
		cipher_function_err(pka_adp->mult);
		pka_base_mod_adapter_deinit();
		return -1;
	}

	out_sign->len = len;
	pka_base_mod_adapter_deinit();
	return 0;
}

static int cipher_check_result(uint32_t *input, uint32_t *result, uint32_t len)
{
	int err = 0;
	for (int i = 0; i < len; i++) {
		if (input[i] != result[i]) {
			err++;
		}
	}

	if (err != 0) {
		cipher_debug_err("error: ecc sign verify check result faild!\n");
	}

	return err;
}

int cipher_ecc_verify(cipher_ecc_verify_s *ecc, uint8_t *in_data, uint32_t data_len, uint8_t *hash_data,
		      uint32_t hash_len, ecc_sign_s *in_sign)
{
	if (ecc == NULL || in_sign == NULL || in_sign->len == 0 || in_sign->r == NULL || in_sign->s == NULL) {
		cipher_pointer_invalid();
		return -1;
	}
	int ret;
	uint32_t len = cipher_get_parse_bitlen(ecc->scheme);
	if (len == 0 || len != ecc->pub_key.key_len || len != in_sign->len) {
		cipher_function_err(cipher_get_parse_bitlen);
		return -1;
	}

	static uint32_t calc_hash[CIHPER_ECC_MAX_LEN / 4] = { 0 };
	memset(calc_hash, 0, sizeof(calc_hash));

	if (hash_data == NULL) {
		if (in_data == NULL || data_len == 0) {
			cipher_pointer_invalid(hash);
			return -1;
		}

		// 用data计算出哈希值
		/* 暂未实现 */
		return -1;

	} else {
		if (hash_len > len || hash_len == 0) {
			cipher_parem_invalid();
			return -1;
		}
		memcpy(calc_hash, hash_data, hash_len);
	}

	pka_base_modul_adapter *pka_adp = pka_base_modul_adapter_init(ecc->scheme);
	if (pka_adp == NULL) {
		cipher_function_err(pka_base_modul_adapter_init);
		return -1;
	}
	/* 计算辅助值 w = s^-1 mod q */
	static uint32_t w[CIHPER_ECC_MAX_LEN / 4] = { 0 };
	memset(w, 0, sizeof(w));

	ret = pka_adp->inv(pka_adp, in_sign->s, ecc->pub_key.q, w, len);
	if (ret != 0) {
		cipher_function_err(pka_adp->inv);
		pka_base_mod_adapter_deinit();
		return -1;
	}

	/* 计算辅助值 u1 = w * h(x) mod q */
	static uint32_t u1[CIHPER_ECC_MAX_LEN / 4] = { 0 };
	memset(u1, 0, sizeof(u1));

	ret = pka_adp->mult(pka_adp, w, calc_hash, ecc->pub_key.q, u1, len);
	if (ret != 0) {
		cipher_function_err(pka_adp->mult);
		pka_base_mod_adapter_deinit();
		return -1;
	}

	/* 计算辅助值 u2 = w * r mod q */
	static uint32_t u2[CIHPER_ECC_MAX_LEN / 4] = { 0 };
	memset(u2, 0, sizeof(u2));

	ret = pka_adp->mult(pka_adp, w, in_sign->r, ecc->pub_key.q, u2, len);
	if (ret != 0) {
		cipher_function_err(pka_adp->mult);
		pka_base_mod_adapter_deinit();
		return -1;
	}

#if 1
	/* 计算P = u1*A + u2*B */
	static uint32_t T1x[CIHPER_ECC_MAX_LEN / 4] = { 0 }, T1y[CIHPER_ECC_MAX_LEN / 4] = { 0 };
	memset(T1x, 0, sizeof(T1x));
	memset(T1y, 0, sizeof(T1y));

	static uint32_t T2x[CIHPER_ECC_MAX_LEN / 4] = { 0 }, T2y[CIHPER_ECC_MAX_LEN / 4] = { 0 };
	memset(T2x, 0, sizeof(T2x));
	memset(T2y, 0, sizeof(T2y));


	/* 1) 计算 T1 = u1 * A */
	pka_ecc_param_info pka_p = { 0 };
	pka_p.a = ecc->pub_key.a;
	pka_p.b = ecc->pub_key.b;
	pka_p.k = u1;
	pka_p.p = ecc->pub_key.p;
	pka_p.w = 0;
	pka_p.len = len;
	pka_piont tmp_P = { ecc->pub_key.Ax, ecc->pub_key.Ay, len };
	pka_piont T1 = { T1x, T1y, len };

	ret = pka_ecc_point_mult(&pka_p, &tmp_P, &T1, ecc->scheme);
	if (ret != 0) {
		cipher_function_err(pka_ecc_point_mult);
		pka_base_mod_adapter_deinit();
		return -1;
	}

	/* 2) 计算 T2 = u2 * B */
	pka_p.k = u2;
	tmp_P.x = ecc->pub_key.Bx;
	tmp_P.y = ecc->pub_key.By;
	pka_piont T2 = { T2x, T2y, len };

	ret = pka_ecc_point_mult(&pka_p, &tmp_P, &T2, ecc->scheme);
	if (ret != 0) {
		cipher_function_err(pka_ecc_point_mult);
		pka_base_mod_adapter_deinit();
		return -1;
	}

	/* 3) 计算 P = T1 + T2 */
	static uint32_t Px[CIHPER_ECC_MAX_LEN / 4] = { 0 }, Py[CIHPER_ECC_MAX_LEN / 4] = { 0 };
	memset(Px, 0, sizeof(Px));
	memset(Py, 0, sizeof(Py));

	tmp_P.x = Px;
	tmp_P.y = Py;

	ret = pka_ecc_point_add(&pka_p, &T1, &T2, &tmp_P, ecc->scheme);
	if (ret != 0) {
		cipher_function_err(pka_ecc_point_add);
		pka_base_mod_adapter_deinit();
		return -1;
	}
#else
	/* 计算P = u1*A + u2*B */
	static uint32_t Px[CIHPER_ECC_MAX_LEN / 4] = { 0 };
	memset(Px, 0, sizeof(Px));
	pka_ecc_param_info pka_p = { 0 };
	pka_p.a = ecc->pub_key.a;
	pka_p.b = 0;
	pka_p.k = u1;
	pka_p.l = u2;
	pka_p.p = ecc->pub_key.p;
	pka_p.w = 0;
	pka_p.len = len;

	pka_piont A = { ecc->pub_key.Ax, ecc->pub_key.Ay, len };
	pka_piont B = { ecc->pub_key.Bx, ecc->pub_key.By, len };
	pka_piont P = { Px, NULL, 32 };
	ret = pka_ecc_shamirs_trick(&pka_p, &A, &B, &P, ecc->scheme);
	if (ret != 0) {
		cipher_function_err(pka_ecc_shamirs_trick);
		pka_base_mod_adapter_deinit();
		return -1;
	}

#endif
	/* 验证 x = r mod q ==> 有效签名 */
	uint32_t *x = Px;
	static uint32_t r_rd[CIHPER_ECC_MAX_LEN / 4] = { 0 };
	memset(r_rd, 0, sizeof(r_rd));

	ret = pka_adp->reduc(pka_adp, in_sign->r, ecc->pub_key.q, r_rd, len);
	if (ret != 0) {
		cipher_function_err(pka_adp->reduc);
		pka_base_mod_adapter_deinit();
		return -1;
	}

	ret = cipher_check_result(r_rd, x, len);
	if (ret != 0) {
		cipher_function_err(cipher_check_result);
		pka_base_mod_adapter_deinit();
		return -1;
	}

	pka_base_mod_adapter_deinit();

	return 0;
}