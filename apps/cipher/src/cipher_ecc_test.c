#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "cipher.h"
#include "pka_regs.h"
#include "gicv3.h"
#include "efuse.h"
#include "cipher_test_debg.h"

static uint32_t g_a[8] = {
	0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF,
};

static uint32_t g_p[8] = {
	0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0xffffffff,
};

static uint32_t g_Ax[8] = {
	0xd898c296, 0xf4a13945, 0x2deb33a0, 0x77037d81, 0x63a440f2, 0xf8bce6e5, 0xe12c4247, 0x6b17d1f2,
};

static uint32_t g_Ay[8] = {
	0x37bf51f5, 0xcbb64068, 0x6b315ece, 0x2bce3357, 0x7c0f9e16, 0x8ee7eb4a, 0xfe1a7f9b, 0x4fe342e2,
};

static uint32_t g_q[8] = {
	0xfc632551, 0xf3b9cac2, 0xa7179e84, 0xbce6faad, 0xffffffff, 0xffffffff, 0x00000000, 0xffffffff,
};

// static uint32_t g_k[8] = {
//     0xe43e380a, 0x00e5c7aa, 0x503462c9, 0xed51b40d,
//     0x87bb81bb, 0x4684d482, 0x70ccb3c6, 0x91061921,
// };
//0xc3f03f7528753177e575f408c791cb4d9ab6117c475b95fb6766748520f97a9f
static uint32_t g_k[8] = {
	0x20f97a9f, 0x67667485, 0x475b95fb, 0x9ab6117c, 0xc791cb4d, 0xe575f408, 0x28753177, 0xc3f03f75,
};

// static uint32_t g_d[8] = {
//     0x1a25f499, 0x07bdb15b, 0x7d2018f6, 0x3299309f,
//     0x1e3adfba, 0xadb162b8, 0xd514c2a8, 0x9dd0d3a3,
// };
// 0xb592d9af1b7d4f2d914fbf96203ffe551e20200c857db6ba0b35f40381b2eb7c
static uint32_t g_d[8] = {
	0x81b2eb7c, 0x0b35f403, 0x857db6ba, 0x1e20200c, 0x203ffe55, 0x914fbf96, 0x1b7d4f2d, 0xb592d9af,
};
// static uint32_t g_r[8] = {
//     0x4c043fb1, 0x8cb4b775, 0x22a71274, 0x1c9c8612,
//     0xedfd8e9b, 0x5e9d157f, 0x878b4dc0, 0x275fa760,
// };
//0x4873b13064b03ac268f15d88e976715a14a991ea803f5073a7d31427570fc3
//0x1b3eb03e39a2a9599bc5bd23ba57f3aac31f1e0690a23f4c44aa849a96bb004b
//0xa1849da033911e6fc2b6bb3d9c401fb19a68a433c41429177f4e3e3f939e2fd2

static uint32_t g_hx[8] = {
	0x939e2fd2, 0x7f4e3e3f, 0xc4142917, 0x9a68a433, 0x9c401fb1, 0xc2b6bb3d, 0x33911e6f, 0xa1849da0,
};

static uint32_t g_s[8] = {
	0x96bb004b, 0x44aa849a, 0x90a23f4c, 0xc31f1e06, 0xba57f3aa, 0x9bc5bd23, 0x39a2a959, 0x1b3eb03e,
};

static uint32_t g_r[8] = {
	0x27570fc3, 0x73a7d314, 0xea803f50, 0x5a14a991, 0x88e97671, 0xc268f15d, 0x3064b03a, 0x004873b1,
};

static uint32_t g_Bx[8] = {
	0x32b4a9f2, 0x3b2051fe, 0x981addc4, 0x08169012, 0x464a644d, 0x96bbe2b7, 0xcc7af21b, 0x5e6afeec,
};
static uint32_t g_By[8] = {
	0x04aff93f, 0xe7966363, 0x9f2f73d1, 0x55d92906, 0xcbdb4959, 0xb7b0e04e, 0x48bb52a0, 0xa3182d14,
};

#if 0
static uint32_t g_Rx[8] = {
	0x4c043fb1, 0x8cb4b775, 0x22a71274, 0x1c9c8612, 0xedfd8e9b, 0x5e9d157f, 0x878b4dc0, 0x275fa760,
};

static uint32_t g_kinv[8] = {
	0x5eb6adc8, 0xadba9757, 0xf3baca7c, 0x6d9ab10f, 0xa93ea95b, 0xc5ead82d, 0x36d3aec0, 0xe0aeb014,
};

static uint32_t g_t1[8] = {
	0x17db2406, 0x4eb69934, 0x89d56371, 0x833ba76f, 0x63e72b24, 0x75e1f919, 0x451235c4, 0xd452011e,
};

// static uint32_t g_hx[8] = {
//     0xf02aa829, 0x60463119, 0x047040be, 0x6775fd91,
//     0x62878a20, 0xf2702a89, 0x2b0347f4, 0xa59ca4dd,
// };
// 0xa1849da033911e6fc2b6bb3d9c401fb19a68a433c41429177f4e3e3f939e2fd2


static uint32_t g_t2[8] = {
	0x0ba2a6de, 0xbb42ff8b, 0xe72e05aa, 0x2dcaaa52, 0xc66eb545, 0x685223a2, 0x70157db8, 0x79eea5fc,
};

// static uint32_t g_s[8] = {
//     0xdb1e0f7e, 0x52f0503a, 0xcaae2578, 0x86939e94,
//     0x7e3b3577, 0x345af3b3, 0xb8435a05, 0x699d906b,
// };
//0x1b3eb03e39a2a9599bc5bd23ba57f3aac31f1e0690a23f4c44aa849a96bb004b

// static uint32_t g_u1Ax[8] = {
//     0x9159b442, 0x04741ffb, 0xbc9dc6eb, 0xaf61ed0f,
//     0x6118ddc8, 0x65140bd1, 0x0bc73aeb, 0xe122af7d,
// };

// static uint32_t g_u1Ay[8] = {
//     0x2e5986f3, 0x14703c61, 0x48b601b4, 0x58ceadb8,
//     0xc5f833a8, 0x95fea0b9, 0xb00ab265, 0x30b54df3,
// };
// 0x6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296
static uint32_t g_u1Ax[8] = {
	0xd898c296, 0xf4a13945, 0x2deb33a0, 0x77037d81, 0x63a440f2, 0xf8bce6e5, 0xe12c4247, 0x6b17d1f2,
};
// 0x4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5
static uint32_t g_u1Ay[8] = {
	0x37bf51f5, 0xcbb64068, 0x6b315ece, 0x2bce3357, 0x7c0f9e16, 0x8ee7eb4a, 0xfe1a7f9b, 0x4fe342e2,
};

static uint32_t g_u2Bx[8] = {
	0x87374e43, 0x2ab1bd36, 0xc2c98f5d, 0x901ec7eb, 0xd8c6f1dd, 0x2f9fe592, 0x00ae2c8d, 0x073d9d27,
};

static uint32_t g_u2By[8] = {
	0x3176e0e3, 0x057f15a8, 0x201bd5f9, 0x502c8deb, 0xbb0955e7, 0x676d41de, 0xcbf8167e, 0x3872b04a,
};
// 0x5e6afeeccc7af21b96bbe2b7464a644d08169012981addc43b2051fe32b4a9f2,
// 0xa3182d1448bb52a0b7b0e04ecbdb495955d929069f2f73d1e796636304aff93f]

static uint32_t g_u1[8] = {
	0xfe25db5d, 0xc81c2369, 0xe7e9d910, 0xa9032542, 0x05484bc0, 0x6d5153d4, 0x92721c35, 0x6ed38574,
};

static uint32_t g_u2[8] = {
	0xb14939bc, 0x81c6d022, 0xaa18f09c, 0xff891661, 0xcdecb46a, 0x10cd9b1e, 0x78f61ce9, 0x18f002cc,
};

// static uint32_t g_Bx[8] = {
//     0xcb9eaf8d, 0x970c1390, 0x3460dcf8, 0xf32137ad,
//     0xd7985dd4, 0x7b9591f9, 0x398b6ac5, 0x6b738de3,
// };
// static uint32_t g_By[8] = {
//     0xab596d57, 0xd1d3cc36, 0x4a13078f, 0xb5dde41f,
//     0x45a2bc4a, 0xcf2d2ab4, 0x6d2bbbd3, 0x83bc61e2,
// };

static uint32_t g_w[8] = {
	0x81f2d21f, 0x5532bb4c, 0xcb7dcf78, 0xac98aa37, 0xa7c7c711, 0x4e74d249, 0xe1c587fb, 0x9d32ece0,
};

#endif

static int compare_result(uint32_t *input, uint32_t *result)
{
	int err = 0;
	for (int i = 0; i < 8; i++) {
		if (input[i] != result[i]) {
			printf("read value%d : 0x%x  result value:0x%x\n", i, input[i], result[i]);
			err++;
		}
	}
	return err;
}

// void JC3_pka_fw_init();
#if 0
static int pka_ecc_point_muilt_test(void)
{
    printf("R = d*A test !\n");
    uint32_t r[8] = {0};
    int ret;
    int err_cnt = 0;
    int i = 0;

    pka_ecc_param_info pka_p = {0};
    pka_p.a = g_a;
    pka_p.b = 0;
    pka_p.k = g_k;
    pka_p.p = g_p;
    pka_p.w = 0;
    pka_p.len = 32;

    pka_piont A = {g_Ax, g_Ay, 32};
    pka_piont R = {r, NULL, 32};

    // JC3_pka_fw_init();

    ret = pka_ecc_point_mult(&pka_p, &A, &R, CIHPER_ECC_256BIT);
    if (ret != 0) {
        function_err(pka_ecc_point_mult);
        goto err;
    }

    err_cnt = compare_result(g_Rx, R.x);
    if (err_cnt != 0) {
        goto err;
    }
    printf("R = d*A success !\n");
    return 0;
err:
    printf("R = d*A faild !\n");
    return -1;
}

static int pka_ecc_point_add_test(void)
{
    printf(" P = u1A + u2B test !\n");
    uint32_t r[8] = {0};
    int ret;
    int err_cnt = 0;
    int i = 0;

    pka_ecc_param_info pka_p = {0};
    pka_p.a = g_a;
    pka_p.b = 0;
    pka_p.k = 0;
    pka_p.p = g_p;
    pka_p.w = 0;
    pka_p.len = 32;

    pka_piont u1A = {g_u1Ax, g_u1Ay, 32};
    pka_piont u2B = {g_u2Bx, g_u2By, 32};
    pka_piont P = {r, NULL, 32};

    // JC3_pka_fw_init();

    ret = pka_ecc_point_add(&pka_p, &u1A, &u2B, &P, CIHPER_ECC_256BIT);
    if (ret != 0) {
        function_err(pka_ecc_point_add);
        goto err;
    }

    err_cnt = compare_result(g_Rx, P.x);
    if (err_cnt != 0) {
        goto err;
    }
    printf("P = u1A + u2B success !\n");
    return 0;
err:
    printf("P = u1A + u2Bfaild !\n");
    return -1;
}

static int pka_ecc_shamirs_trick_test(void)
{
    printf(" P = u1 * A + u2 * B test !\n");
    uint32_t r[8] = {0};
    int ret;
    int err_cnt = 0;
    int i = 0;

    pka_ecc_param_info pka_p = {0};
    pka_p.a = g_a;
    pka_p.b = 0;
    pka_p.k = g_u1;
    pka_p.l = g_u2;
    pka_p.p = g_p;
    pka_p.w = 0;
    pka_p.len = 32;

    pka_piont A = {g_Ax, g_Ay, 32};
    pka_piont B = {g_Bx, g_By, 32};
    pka_piont P = {r, NULL, 32};

    // JC3_pka_fw_init();

    ret = pka_ecc_shamirs_trick(&pka_p, &A, &B, &P, CIHPER_ECC_256BIT);
    if (ret != 0) {
        function_err(pka_ecc_shamirs_trick);
        goto err;
    }

    err_cnt = compare_result(g_Rx, P.x);
    if (err_cnt != 0) {
        goto err;
    }
    printf("P = u1 * A + u2 * B success !\n");
    return 0;
err:
    printf("P = u1 * A + u2 * B faild !\n");
    return -1;
}

static int pka_pka_modul_inv_test(void)
{
#if 0
    printf(" k^-1 mod q  test !\n");
    int result[8] = {0};
    int err_cnt = 0;
    int ret;
    pka_base_modul_adapter *pka_adp = pka_base_modul_adapter_init(PKA_BASE_RADIX_256BIT);
    ret = pka_adp->inv(pka_adp, g_k, g_q, result, sizeof(result));
    if (ret != 0) {
        goto err;
    }

    err_cnt = compare_result(g_kinv, result);
    if (err_cnt != 0) {
        goto err;
    }

    printf(" k^-1 mod q  success !\n");
    pka_base_mod_adapter_deinit();
    return 0;
err:
    printf("k^-1 mod q faild !\n");
    pka_base_mod_adapter_deinit();
    return -1;
#else
    printf(" s^-1 mod q  test !\n");
    int result[8] = {0};
    int err_cnt = 0;
    int ret;
    pka_base_modul_adapter *pka_adp = pka_base_modul_adapter_init(PKA_BASE_RADIX_256BIT);
    ret = pka_adp->inv(pka_adp, g_s, g_q, result, sizeof(result));
    if (ret != 0) {
        goto err;
    }

    err_cnt = compare_result(g_w, result);
    if (err_cnt != 0) {
        goto err;
    }

    printf("w = s^-1 mod q  success !\n");
    pka_base_mod_adapter_deinit();
    return 0;
err:
    printf("w = k^-1 mod q faild !\n");
    pka_base_mod_adapter_deinit();
    return -1;
#endif
}

static int pka_pka_modul_mult_test(void)
{
#if 0
    printf("t1 = (d * r) mod q test! \n");
    int result[8] = {0};
    int err_cnt = 0;
    int ret;
    pka_base_modul_adapter *pka_adp = pka_base_modul_adapter_init(PKA_BASE_RADIX_256BIT);
    ret = pka_adp->mult(pka_adp, g_d, g_r, g_q, result, sizeof(result));
    if (ret != 0) {
        goto err;
    }

    err_cnt = compare_result(g_t1, result);
    if (err_cnt != 0) {
        goto err;
    }

    printf(" t1 = (d * r) mod q success !\n");
    pka_base_mod_adapter_deinit();
    return 0;
err:
    printf("t1 = (d * r) mod q  faild !\n");
    pka_base_mod_adapter_deinit();
    return -1;
#else
    printf("s = (t2 * kinv) mod q test! \n");
    int result[8] = {0};
    int err_cnt = 0;
    int ret;
    pka_base_modul_adapter *pka_adp = pka_base_modul_adapter_init(PKA_BASE_RADIX_256BIT);
    ret = pka_adp->mult(pka_adp, g_t2, g_kinv, g_q, result, sizeof(result));
    if (ret != 0) {
        goto err;
    }

    err_cnt = compare_result(g_s, result);
    if (err_cnt != 0) {
        goto err;
    }

    printf("s = (t2 * kinv) mod q success !\n");
    pka_base_mod_adapter_deinit();
    return 0;
err:
    printf("s = (t2 * kinv) mod q  faild !\n");
    pka_base_mod_adapter_deinit();
    return -1;
#endif
}

static int pka_modul_add_test(void)
{
    printf("t2 = (hx + t1) mod q test! \n");
    int result[8] = {0};
    int err_cnt = 0;
    int ret;
    pka_base_modul_adapter *pka_adp = pka_base_modul_adapter_init(PKA_BASE_RADIX_256BIT);
    ret = pka_adp->add(pka_adp, g_hx, g_t1, g_q, result, sizeof(result));
    if (ret != 0) {
        goto err;
    }

    err_cnt = compare_result(g_t2, result);
    if (err_cnt != 0) {
        goto err;
    }

    printf(" t2 = (hx + t1) mod q success !\n");
    pka_base_mod_adapter_deinit();
    return 0;
err:
    printf(" t2 = (hx + t1) mod q  faild !\n");
    pka_base_mod_adapter_deinit();
    return -1;
}

static int pka_modul_reduc_test(void)
{
    printf("r = r mod q test! \n");
    int result[8] = {0};
    int err_cnt = 0;
    int ret;
    pka_base_modul_adapter *pka_adp = pka_base_modul_adapter_init(PKA_BASE_RADIX_256BIT);
    ret = pka_adp->reduc(pka_adp, g_r, g_q, result, sizeof(result));
    if (ret != 0) {
        goto err;
    }

    err_cnt = compare_result(g_r, result);
    if (err_cnt != 0) {
        goto err;
    }

    printf(" r = r mod q success !\n");
    pka_base_mod_adapter_deinit();
    return 0;
err:
    printf(" r = r mod q  faild !\n");
    pka_base_mod_adapter_deinit();
    return -1;
}
#endif
// int cipher_ecc_sign(cipher_ecc_sign_s *ecc, uint8_t *in_data, uint32_t data_len,
//                         uint8_t *hash_data, uint32_t hash_len, ecc_sign_s *out_sign);
static int cipher_ecc_sign_test(void)
{
	printf("api sign test !!\n");
	cipher_ecc_sign_s ecc = {
		.ecc_param.a = g_a,
		.ecc_param.Ax = g_Ax,
		.ecc_param.Ay = g_Ay,
		.ecc_param.k = g_k,
		.ecc_param.len = 32,
		.ecc_param.p = g_p,
		.ecc_param.q = g_q,
		.pri_key.d = g_d,
		.pri_key.key_len = 32,
		.scheme = CIHPER_ECC_256BIT,
	};

	uint32_t s[8] = { 0 }, r[8] = { 0 };

	ecc_sign_s sign = { .s = s, .r = r, .len = 32 };
#ifdef TEST_KEY_BY_EFUSE
	ecc.key_from = CIPHER_KEY_SRC_EFUSE;
#else
	ecc.key_from = CIPHER_KEY_SRC_USER;
#endif
	int err_cnt = 0, ret;
	if (ecc.key_from == CIPHER_KEY_SRC_EFUSE) {
		ret = cipher_efuse_attach_key(CIPHER_EFUSE_TARGET_ECC);
		if (ret != 0) {
			goto err;
		}
        ecc.pri_key.d = NULL;
	}
    // int i = 1;
    // while (i)
    // {
    //     /* code */
    // }

	ret = cipher_ecc_sign(&ecc, NULL, 0, (uint8_t *)(uintptr_t)g_hx, 32, &sign);
	if (ret != 0) {
		goto err;
	}

	err_cnt = compare_result(g_r, sign.r);
	err_cnt |= compare_result(g_s, sign.s);
	if (err_cnt != 0) {
		goto err;
	}

	printf(" api sign test success !\n");
	return 0;
err:
	printf(" api sign test faild !\n");
	return -1;
}

static int cipher_ecc_verify_test(void)
{
	printf("api verify test !!\n");
	cipher_ecc_verify_s ecc = {
		.pub_key.a = g_a,
		.pub_key.b = 0,
		.pub_key.p = g_p,
		.pub_key.q = g_q,
		.pub_key.Ax = g_Ax,
		.pub_key.Ay = g_Ay,
		.pub_key.Bx = g_Bx,
		.pub_key.By = g_By,
		.pub_key.key_len = 32,
		.scheme = CIHPER_ECC_256BIT,
	};

	// int s[8] = {0}, r[8] = {0};

	ecc_sign_s sign = { .s = g_s, .r = g_r, .len = 32 };
	// int err_cnt = 0;

	int ret = cipher_ecc_verify(&ecc, NULL, 0, (uint8_t *)g_hx, 32, &sign);
	if (ret != 0) {
		goto err;
	}

	// err_cnt = compare_result(g_r, sign.r);
	// err_cnt |= compare_result(g_s, sign.s);
	// if (err_cnt != 0) {
	//     goto err;
	// }

	printf(" api verify test success !\n");
	return 0;
err:
	printf(" api verify test faild !\n");
	return -1;
}

static int cipher_produce_ecc_pub_key_test(void)
{
	printf("api produce_ecc_pub_key_test test !!\n");
	cipher_ecc_sign_s ecc = {
		.ecc_param.a = g_a,
		.ecc_param.Ax = g_Ax,
		.ecc_param.Ay = g_Ay,
		.ecc_param.k = g_k,
		.ecc_param.len = 32,
		.ecc_param.p = g_p,
		.ecc_param.q = g_q,
		.pri_key.d = g_d,
		.pri_key.key_len = 32,
		.scheme = CIHPER_ECC_256BIT,
	};

	uint32_t Bx[8] = { 0 }, By[8] = { 0 };

	int err_cnt = 0;

	int ret = cipher_get_pubkey(&ecc, Bx, By);
	if (ret != 0) {
		goto err;
	}

	err_cnt = compare_result(g_Bx, Bx);
	err_cnt |= compare_result(g_By, By);
	if (err_cnt != 0) {
		goto err;
	}

	printf(" api produce_ecc_pub_key_test success !\n");
	return 0;
err:
	printf(" api produce_ecc_pub_key_test faild !\n");
	return -1;
}
// extern int ecc_shamirs_test(void);

#ifdef TEST_CIPH_IRQ
static void pka_irq_handler(void)
{
	printf("================ pka irq !!!, irq status = 0x%08x \n", PKA->RTN_CODE);
	// // if (SPACC->IRQ_STAT
	//  {
	// }
    PKA->STAT = (PKA->STAT) | (1 << 30);
}
int pka_irq_test(void)
{
	seehi_func_enter();
    IRQ_Initialize();
	PKA->IRQ_EN = (PKA->IRQ_EN) | (1 << 30);
	IRQ_SetHandler(51 + 32, pka_irq_handler);
	IRQ_SetPriority(51 + 32, 0 << 3);
	IRQ_Enable(51 + 32);
	printf("irq status = 0x%08x \n", PKA->RTN_CODE);
	// encrypt_cbc_test();
	return 0;
}
#endif

int pka_ecc_test(void)
{
    // REG32(0x10350384) = 0x59c4d241;
	cipher_ecc_init();
    bool make_pk = false;
#ifdef TEST_CIPH_IRQ
	pka_irq_test();
#endif
	// pka_ecc_point_muilt_test();
	// pka_pka_modul_inv_test();
	// pka_pka_modul_mult_test();
	// pka_modul_add_test();
	// cipher_ecc_sign_test();
	// pka_ecc_point_add_test();

    if (make_pk) {
	    cipher_produce_ecc_pub_key_test();
    }
	// pka_ecc_shamirs_trick_test();
	// ecc_shamirs_test();
	// pka_ecc_point_muilt_test();

	int cnt = 1;
	int ret = 0;
#ifdef TEST_KEY_BURN_EFUSE
#define EFUSE_PKEY_BASE 20
	ret = cipher_efuse_burn_key(CIPHER_EFUSE_TARGET_ECC, g_d, sizeof(g_d));

    static uint32_t data[8] = {0};
    memset(data, 0, sizeof(data));
    ret |= efuse_auto_read_data(EFUSE_PKEY_BASE, data, sizeof(data));
    for (int i = 0; i < 8; i++) {
        if (data[i] != g_d[i]) {
            printf("burn key error !!! read data[%d] = 0x%x, write key[%d] = 0x%x \n", i, data[i], i, g_d[i]);
            ret = -1;
        }
    }
#endif

	while (cnt--) {
		/* code */

		// printf("\n\n\n");

		// pka_ecc_shamirs_trick_test();
		ret |= cipher_ecc_sign_test();
		// ret |= pka_modul_reduc_test();
		ret |= cipher_ecc_verify_test();
		// ret |= cipher_produce_ecc_pub_key_test();
		// ret |= pka_pka_modul_inv_test();
		// ret |= pka_pka_modul_mult_test();
		// ret |= pka_ecc_point_muilt_test();
		// ret |= pka_ecc_point_add_test();
		// ret |= cipher_produce_ecc_pub_key_test();
		// printf("irq status = 0x%08x \n", PKA->RTN_CODE);
		if (ret != 0) {
			printf("pka ecc test error !!!!\n");
			break;
		} else {
			printf("pka ecc test success !!!!\n");
        }
		// pka_ecc_shamirs_trick_test();
		// ecc_shamirs_test();
		// pka_ecc_point_muilt_test();
	}

	// while (1) {
	// 	/* code */
	// }
    return 0;
}
