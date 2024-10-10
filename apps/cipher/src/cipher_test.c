#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "cipher.h"
#include "spacc_regs.h"
#include "gicv3.h"
#include "efuse.h"
#include "cipher_test_debg.h"

static uint32_t g_key[8] = { 0xc1e9a73d, 0x95dcc31d, 0xab59ee2d, 0xd839a704,
			     0xa503f115, 0x950d9c05, 0xaad86a0d, 0x66b8d7aa };

static uint32_t g_iv[4] = { 0x50a6ba1e, 0x18ab2e2a, 0xa71377dd, 0x01000000 };

static uint32_t g_clear_data[256] = {
	0x0,  0x1,  0x2,  0x3,	0x4,  0x5,  0x6,  0x7,	0x8,  0x9,  0xa,  0xb,	0xc,  0xd,  0xe,  0xf,	0x10, 0x11,
	0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
	0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
	0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b,
	0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d,
	0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xc0, 0xc1,
	0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3,
	0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5,
	0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
	0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
};

static uint32_t g_cbc_sec_data[256 + 4] = {
	0xa26fdd07, 0xe81c343d, 0x21a3ef0f, 0x17c8b8ce, 0x36062c12, 0x551879f3, 0xd7862e85, 0xa5769ba1, 0x6aa9f922,
	0x802e18,   0x59555ad7, 0x5e8ea7d7, 0x57fa20b7, 0xa9f0f1ff, 0x6c5ff23,	0x19659fa4, 0x9c73ef5b, 0x1907ea5e,
	0x570f51e2, 0xb55bc2e1, 0x2dafd3de, 0x76126fed, 0xf78cdd52, 0xecb9428c, 0x2bbce100, 0xa1f64738, 0xb2f9fb72,
	0x6f112997, 0xd15541eb, 0x45c7833d, 0xdbdabb40, 0x4d273c06, 0x92ca5c2a, 0x36b16ee8, 0x39f0b3af, 0x49d3e0fd,
	0xafb6b615, 0xa1217f37, 0x603ee82e, 0xf5d0b68c, 0xbe8d10b6, 0x17d838e8, 0xde921193, 0x3999743c, 0x9739d547,
	0xf7caed67, 0xe33c6697, 0x6907a73f, 0x8e64bee5, 0x86d4b5f6, 0x903df8c6, 0xb77b5954, 0x38708e1c, 0xaa4605a5,
	0xd8707250, 0x17281180, 0xebeea32d, 0x5945948d, 0xcd17bb78, 0x80f22db7, 0xed8bd8f4, 0x124fc4f2, 0x1b31d438,
	0xf5196417, 0x5889a7bf, 0x825ffb1c, 0x667c9238, 0xbb5534f5, 0xdabfb358, 0xa69fff94, 0xf858054,	0x702c193a,
	0x922a8c5e, 0x6b588437, 0xa532c695, 0x8c6cffd7, 0x5bc5a480, 0xe2b2787,	0x62017338, 0x336575e5, 0x1bb657e9,
	0xa5b87bbb, 0x55119efe, 0x824f7a6,  0x5ce58dc4, 0xab8aa3cc, 0x44f7dc,	0xbb5e071a, 0x16563ecc, 0x67783d89,
	0x835dd4b9, 0xeddec8e4, 0x2d62268d, 0xe23cb148, 0x971d7235, 0xc489f269, 0x685bdfab, 0xb88e723f, 0x17a742f0,
	0xfd5eb3d6, 0x557d7bb1, 0x7e532515, 0x2159ee9c, 0xc8372f86, 0x75d97ec4, 0xac3e8bef, 0xa30367b1, 0xd827e862,
	0x58d9c621, 0x8ef34805, 0xffcfea86, 0x348f7b04, 0xcc41587b, 0xfb935b33, 0x57c2618d, 0xe3c06a0f, 0x17f435e3,
	0xef72db4,  0x1bd8c5d,	0x2866c2b2, 0xffda9e81, 0x23a731c7, 0xff0d0180, 0x872bed41, 0x6d3dc0fb, 0x2804cd7c,
	0x1fffd5a4, 0x13c2633a, 0x8c53eb45, 0xc8eeb9e,	0x7646d20e, 0xf46cb31c, 0x41c073ce, 0x8d2bd140, 0x21830c20,
	0x4be841f5, 0x2131df4d, 0xec7e6100, 0xe3cde59c, 0x51b95fd1, 0xab481a7d, 0x33ace745, 0xa3b65048, 0x8dda6cc3,
	0xc44af85,  0x8d2ca3d9, 0x3b1c1792, 0xd045eb17, 0xd577e9e7, 0x14cc1ed,	0x2f1cf04c, 0x5666a8a,	0x6e701327,
	0x190c4041, 0x5840030f, 0x960c556a, 0x389991df, 0xb87b368b, 0x5c1875b0, 0x6700c78e, 0xe1df645c, 0xcf1e52b7,
	0x87d00e2d, 0xb06939c4, 0x3991725f, 0xf5f3b3d6, 0x85be4601, 0xbd22de8b, 0x920c4d21, 0xbd743775, 0x82db030c,
	0x7b2921a4, 0x9572ecb9, 0xd87c53dc, 0x193e90f3, 0x60909cc6, 0x8891af03, 0x8b816740, 0x5d26d7b0, 0x92253ba,
	0x767cd15d, 0xecf05252, 0xb9cbe665, 0x986756b8, 0x113f3254, 0x453d7b1a, 0x4ad42f11, 0x207e999,	0x799674e6,
	0x3ffcbe32, 0x2308e1d7, 0x7913ff66, 0x4a1db54c, 0x5e789bfe, 0x289e27ca, 0xbc05ae06, 0xff7a86b7, 0xb5b4486d,
	0xa684767b, 0x2213428b, 0x6ecdbff3, 0x58c1d68c, 0xa644b89c, 0x1686c843, 0x7f2f078d, 0xc279807f, 0x775fee77,
	0xbd631a3b, 0xaebddc0b, 0x73368936, 0x1a7d4594, 0x4dd9d276, 0xfda7748c, 0x9cc64ad,  0x513e44ea, 0x149e4a69,
	0x49d9862b, 0x94b3242f, 0xf6046a2e, 0x7111f9a4, 0x6b2bccea, 0xda2c0991, 0x894e4ab4, 0x60221c3c, 0xb3047e7d,
	0xac578406, 0x7cf5fa5c, 0x4bb6736b, 0x6bd461f1, 0x5d490719, 0x22e6f070, 0x352596fc, 0x518e6fc1, 0x96fef948,
	0xdce7a6fb, 0x16ba3fb5, 0x51784c23, 0xb959c2b1, 0xf066e66c, 0xb6a7d5fa, 0x1e936195, 0x2542b6e6, 0xbb0b2dfd,
	0x4b44ae,   0xf45901e0, 0x434e0bc5, 0x7998f918, 0xbbfbc886, 0xc581a94e, 0x810dc85d, 0x76b83e32, 0xf445d50,
	0x98518cc4, 0x9dab308,	0x25b33b8e, 0xc58cb447, 0xe600f916,	0xfa32f051,	0xe0d69245, 0x8b6aa1d1
};

static uint32_t g_gcm_sec_data[256] = {
	0x3eede634, 0xc3be1abf, 0x2fb1f04,  0xf2e8594,	0x9eb72786, 0x93fa19fe, 0xd40ef145, 0x5f01d7ae, 0xc85d5859,
	0x3cf39455, 0x1473be9f, 0xe3e3f4d8, 0x304ed23b, 0x8555cf27, 0x6b4e48ef, 0x451501fb, 0x13dd6570, 0xa277fde5,
	0xce771f70, 0x368c70c5, 0x3c7da0bd, 0x76a3c8b6, 0xf6150ddd, 0x25d8d8f8, 0x955e98a8, 0xe56ee24b, 0xba3af0b8,
	0x6468c687, 0xd7ecf43a, 0x99bad859, 0xb63e02a0, 0x9027c56b, 0x513993bd, 0x783ffd74, 0x73945e03, 0x9bc41037,
	0x795d1d94, 0x207d5b98, 0x76aad55d, 0x978e3e8c, 0x3fda8c15, 0xc60d4637, 0xa993dcb1, 0x9c0020ba, 0xad9b8c86,
	0x56df3e30, 0x162b4bd5, 0x332f63d0, 0x18a816f,	0x5f423351, 0xad374dc4, 0x86b25e59, 0xdd63cab0, 0xfd1e442,
	0x4d32f7d9, 0x4efb3f18, 0x7f2c2ae6, 0xa73bf8a8, 0xddb4acb9, 0x1d641263, 0x67a961a,  0x7e6b358,	0xa1375b85,
	0x369f907b, 0xed9c9a81, 0x8662c501, 0x901d8ad6, 0x5b0a6f90, 0x3ec00d79, 0x5745f344, 0x79945a3a, 0x750cba69,
	0x7cb2e707, 0x20e76df9, 0x82a7b622, 0x18e58d1b, 0xd7f124aa, 0x432346ec, 0xa023e711, 0x933b0530, 0xa38d5c8f,
	0x5323d63b, 0x6c223e82, 0xc597355c, 0x2f50d567, 0xed3f999c, 0x434389c,	0xb324efb6, 0xa1e8b151, 0xcac7dc33,
	0xb110f43,  0x4de77ba5, 0x98a61694, 0x82afb9d8, 0x380e518,  0x9fb9b545, 0x52fc875d, 0x8e2ab8b9, 0x4b144591,
	0xc3922ef5, 0x4b82ae47, 0x6e597c90, 0x713e54d4, 0x690b0e6b, 0x97f18cf,	0xdaf2d8ee, 0x9576f483, 0xdfffbf40,
	0xd1ff144d, 0x7ed27298, 0x898cd948, 0xc806767,	0x9a6f986b, 0x83073bdd, 0x341fbd34, 0x55c20e0a, 0x957f451d,
	0xbdbd78b5, 0x81e4e14b, 0xc4c006f,  0x98b89730, 0x99c163de, 0x8cfeefd3, 0xe2c65b76, 0x1de61dc1, 0x74549c1f,
	0x8a30913f, 0xb38703ec, 0x454685e2, 0xda29ac,	0x99f11c67, 0xbc46373b, 0x65269e0c, 0xa4b044b2, 0x4c827c54,
	0x7c913706, 0xc7d418e,	0x421d5ac0, 0x48e3435e, 0xc591db7,  0xe7527683, 0x7b598bf7, 0x3eabba48, 0x74676c0,
	0x86a7201d, 0xfde17d11, 0xe2dfdb9e, 0xc4b1c216, 0x8afa76ca, 0x5a536ae2, 0xf4dedd09, 0xd58e19be, 0x5de0d4ed,
	0xd2125d9,  0xc27c613f, 0xdac13ea3, 0x236bb6f9, 0x3e590aa2, 0xc2899c27, 0x7265a4a5, 0x51ff773,	0x18ec44b2,
	0xf1878001, 0xcbabdd96, 0x9a72c879, 0xec00db03, 0x6d43181e, 0xe068a6e,	0xe69afca3, 0xc2ca1a8,	0x104880fd,
	0x92aec408, 0xe5bdf7f4, 0xc5806a8d, 0x91004f7a, 0x61dc10e7, 0x7d58be89, 0x417e1807, 0x7eea5d90, 0xb1dec1e8,
	0x9b78116a, 0x6947fa27, 0x27523210, 0x8c014476, 0x3c88e7e5, 0x327adba6, 0x5123923a, 0xe28d648f, 0xb703ff40,
	0xba8ddc03, 0xeabe94fd, 0x17c781c7, 0x79957d7a, 0xe1fb2054, 0xe64e4f3c, 0x88cfd773, 0xbbd1263,	0x962515ef,
	0x28e9125b, 0xcc1e11b1, 0xbd200b11, 0xce79de65, 0x5a57a6e1, 0x3835d2cb, 0x4d3100ca, 0x3697158f, 0x48a3738d,
	0x522ddb1c, 0x1a39324b, 0x95d8efba, 0x39c80275, 0x26b26778, 0x37c3d500, 0xef59999f, 0xa63ddddc, 0x1405338e,
	0xe9e20397, 0x83991016, 0xc1865c3a, 0x7e0ca241, 0x54162b5d, 0x822ce582, 0xea176290, 0x510dc656, 0xbb88f636,
	0xebf2001f, 0xc91b9c43, 0x43b6c6e7, 0x68c98161, 0x34bdf2a3, 0x7ed990be, 0xf1db10c8, 0xd36bd43f, 0x380fa7ff,
	0xca123dfe, 0x6fc852f5, 0x709cd959, 0xf6a41dda, 0x3e406324, 0x9ba27fc,	0x6165c0bc, 0x368a79b3, 0x732ab02a,
	0x66fc81fe, 0xc4291c12, 0x9f2b4b1,  0x68d6737a, 0xbd5e3a2d, 0x6b44f6bf, 0x23bcef1c, 0x7d2b7c06, 0x508d8be4,
	0x8cc6a577, 0x30246173, 0x389d7150, 0x90700166,
};

static uint32_t g_icv[4] = { 0x7a67ae31, 0x9fb2a572, 0xd6c3a93f, 0x41d355dc };
static uint32_t g_aad[4] = { 0x73696874, 0x412d612d, 0x65684444, 0x72656461 };

static int encrypt_cbc_test(void)
{
	seehi_func_enter();
	uint32_t *pkt_data = g_clear_data;
	// uint8_t *u8_src_data = (uint8_t *)0x55000000;
	// uint8_t *u8_dst_data = (uint8_t *)0x56000000;
	uint8_t u8_src_data[2048] = {0};
	uint8_t u8_dst_data[2048] = {0};

	//Data 0x692d616d 2d7a6861 7a686977 65692d2d 692d616d 2d7a6861 7a686977 65692d2d 692d616d 2d7a6861 7a686977 65692d2d
	// pkt_data[3]  =0x2d2d6965;
	// pkt_data[2]  =0x7769687a;
	// pkt_data[1]  =0x61687a2d;
	// pkt_data[0]  =0x6d612d69;
	// pkt_data[7]  =0x2d2d6965;
	// pkt_data[6]  =0x7769687a;
	// pkt_data[5]  =0x61687a2d;
	// pkt_data[4]  =0x6d612d69;

	// pkt_data[11] =0x2d2d6965;
	// pkt_data[10] =0x7769687a;
	// pkt_data[9]  =0x61687a2d;
	// pkt_data[8]  =0x6d612d69;

	uint32_t *sec_data = g_cbc_sec_data;
	// sec_data[0]  = 0x5646f3ce;
	// sec_data[1]  = 0xa9b1dd5d;
	// sec_data[2]  = 0x3d449313;
	// sec_data[3]  = 0xc163d243;
	// sec_data[4]  = 0xbe224065;
	// sec_data[5]  = 0xf030a711;
	// sec_data[6]  = 0x8ade21a6;
	// sec_data[7]  = 0xdbb02436;
	// sec_data[8]  = 0x99ad23be;
	// sec_data[9]  = 0xa7006678;
	// sec_data[10] = 0x48c8085d;
	// sec_data[11] = 0x01200efb;
	int ret;

	cipher_ctrl ciph_ctrl;
	memset(&ciph_ctrl, 0x0, sizeof(ciph_ctrl));
	ciph_ctrl.alg_info.alg = CIPHER_ALG_AES;
	ciph_ctrl.alg_info.bit_width = CIPHER_BIT_WIDTH_128BIT;
	ciph_ctrl.alg_info.work_mode = CIPHER_WORK_MODE_CBC;
#ifdef TEST_KEY_BY_EFUSE
	ciph_ctrl.key_info.key_by_efuse = true;
#else
	ciph_ctrl.key_info.key_by_efuse = false;
#endif
	ciph_ctrl.key_info.keylen = CIPHER_KEY_AES_256BIT;

	memcpy(ciph_ctrl.iv, g_iv, sizeof(ciph_ctrl.iv));

	cipher_init();

	int hcipher = -1;
	ret = cipher_creat_handle(&hcipher);
	if (ret != 0 || hcipher == -1) {
		printf("%s [%d] creat cipher handle fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	if (ciph_ctrl.key_info.key_by_efuse) {
		ret = cipher_efuse_attach_key(CIPHER_EFUSE_TARGET_AES);
		if (ret != 0) {
			cipher_deinit();
			return -1;
		}
	} else {
		memcpy(ciph_ctrl.key_info.key, g_key, sizeof(ciph_ctrl.key_info.key));
	}

	ret = cipher_config_handle(hcipher, &ciph_ctrl);
	if (ret != 0) {
		printf("%s [%d] config cipher handle fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	memcpy(u8_src_data, pkt_data, sizeof(g_clear_data));
	uint32_t out_len = 0;
	ret = cipher_encrypt_mult(hcipher, u8_src_data, sizeof(g_clear_data), u8_dst_data, &out_len);
	if (ret != 0) {
		printf("%s [%d] encrypt fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}
	printf("out_len = %d\n", out_len);
	int result_data[256 + 4];
	// memset(result_data, 0, sizeof(result_data));
	uint32_t error_cnt = 0;
	memcpy(result_data, u8_dst_data, out_len);
	int i = 0;
	// printf("int sec_data[256] {\n");
	printf("out_len = %d\n", out_len);
	for (i = 0; i < sizeof(g_clear_data) / 4 + 4; i++) {
		if (result_data[i] == sec_data[i]) {
			// printf("des_pkt, no=%d data=%x \n", i, result_data[i]);
		} else {
			error_cnt = error_cnt + 1;
			printf("compare error data_no=%d rdata= %x  exp_data=%x \n", i, result_data[i],
				     sec_data[i]);
		}
		// if (i % 16 == 0) {
		//     printf("\n    ");
		// }
		// printf("0x%x, ", result_data[i]);
	}
	// printf("};\n");
	if (error_cnt != 0) {
		printf("%s [%d] AES-CBC encrypt test error! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	} else {
		printf("%s [%d] AES-CBC encrypt test success! \n", __FUNCTION__, __LINE__);
	}

	cipher_deinit();
	seehi_func_exit();
	return 0;
}

static int decrypt_cbc_test(void)
{
	seehi_func_enter();
	uint32_t *pkt_data = g_clear_data;
	// uint8_t *u8_src_data = (uint8_t *)0x55000000;
	// uint8_t *u8_dst_data = (uint8_t *)0x56000000;
	uint8_t u8_src_data[2048] = {0};
	uint8_t u8_dst_data[2048] = {0};

	//Data 0x692d616d 2d7a6861 7a686977 65692d2d 692d616d 2d7a6861 7a686977 65692d2d 692d616d 2d7a6861 7a686977 65692d2d
	// pkt_data[3]  =0x2d2d6965;
	// pkt_data[2]  =0x7769687a;
	// pkt_data[1]  =0x61687a2d;
	// pkt_data[0]  =0x6d612d69;
	// pkt_data[7]  =0x2d2d6965;
	// pkt_data[6]  =0x7769687a;
	// pkt_data[5]  =0x61687a2d;
	// pkt_data[4]  =0x6d612d69;

	// pkt_data[11] =0x2d2d6965;
	// pkt_data[10] =0x7769687a;
	// pkt_data[9]  =0x61687a2d;
	// pkt_data[8]  =0x6d612d69;

	uint32_t *sec_data = g_cbc_sec_data;
	// sec_data[0]  = 0x5646f3ce;
	// sec_data[1]  = 0xa9b1dd5d;
	// sec_data[2]  = 0x3d449313;
	// sec_data[3]  = 0xc163d243;
	// sec_data[4]  = 0xbe224065;
	// sec_data[5]  = 0xf030a711;
	// sec_data[6]  = 0x8ade21a6;
	// sec_data[7]  = 0xdbb02436;
	// sec_data[8]  = 0x99ad23be;
	// sec_data[9]  = 0xa7006678;
	// sec_data[10] = 0x48c8085d;
	// sec_data[11] = 0x01200efb;

	cipher_ctrl ciph_ctrl;
	memset(&ciph_ctrl, 0x0, sizeof(ciph_ctrl));
	ciph_ctrl.alg_info.alg = CIPHER_ALG_AES;
	ciph_ctrl.alg_info.bit_width = CIPHER_BIT_WIDTH_128BIT;
	ciph_ctrl.alg_info.work_mode = CIPHER_WORK_MODE_CBC;
#ifdef TEST_KEY_BY_EFUSE
	ciph_ctrl.key_info.key_by_efuse = true;
#else
	ciph_ctrl.key_info.key_by_efuse = false;
#endif
	ciph_ctrl.key_info.keylen = CIPHER_KEY_AES_256BIT;
	memcpy(ciph_ctrl.iv, g_iv, sizeof(ciph_ctrl.iv));

	cipher_init();

	int hcipher = -1;
	int ret = cipher_creat_handle(&hcipher);
	if (ret != 0 || hcipher == -1) {
		printf("%s [%d] creat cipher handle fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	if (ciph_ctrl.key_info.key_by_efuse) {
		ret = cipher_efuse_attach_key(CIPHER_EFUSE_TARGET_AES);
		if (ret != 0) {
			cipher_deinit();
			return -1;
		}
	} else {
		memcpy(ciph_ctrl.key_info.key, g_key, sizeof(ciph_ctrl.key_info.key));
	}

	ret = cipher_config_handle(hcipher, &ciph_ctrl);
	if (ret != 0) {
		printf("%s [%d] config cipher handle fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	memcpy(u8_src_data, sec_data, sizeof(g_cbc_sec_data));

	uint32_t out_len = 0;
	ret = cipher_decrypt_mult(hcipher, u8_src_data, sizeof(g_cbc_sec_data), u8_dst_data, &out_len);
	if (ret != 0) {
		printf("%s [%d] decrypt fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	uint32_t result_data[256];
	uint32_t error_cnt = 0;
	int i = 0;
	memcpy(result_data, u8_dst_data, out_len);
	for (i = 0; i < out_len / 4; i++) {
		if (result_data[i] == pkt_data[i]) {
			// printf("des_pkt, no=%d data=%x \n", i, result_data[i]);
		} else {
			error_cnt = error_cnt + 1;
			printf("compare error data_no=%d rdata= 0x%x  exp_data = 0x%x \n", i, result_data[i],
				     pkt_data[i]);
		}
	}

	if (error_cnt != 0) {
		printf("%s [%d] AES-CBC decrypt test error! error_cnt = %d \n", __FUNCTION__, __LINE__,
			     error_cnt);
		cipher_deinit();
		return -1;
	} else {
		printf("%s [%d] AES-CBC decrypt test success! \n", __FUNCTION__, __LINE__);
	}

	cipher_deinit();
	seehi_func_exit();
	return 0;
}

static int encrypt_gcm_test(void)
{
	seehi_func_enter();
	uint32_t *pkt_data = g_clear_data;
	uint32_t *exp_data = g_gcm_sec_data;
	uint8_t u8_src_data[1024] = {0};
	uint8_t u8_dst_data[1024] = {0};
	int i = 0;

	// pkt_data[3] = 0x2d2d6965;
	// pkt_data[2] = 0x7769687a;
	// pkt_data[1] = 0x61687a2d;
	// pkt_data[0] = 0x6d612d69;

	// exp_data[0] = 0x538ccb5d;
	// exp_data[1] = 0xa2d66093;
	// exp_data[2] = 0x7592777c;
	// exp_data[3] = 0x2203ecf2;
	// exp_data[4] = 0x841053bc;
	// exp_data[5] = 0x6a90576c;
	// exp_data[6] = 0xdc0628d2;
	// exp_data[7] = 0x3e4f853d;

	cipher_ctrl ciph_ctrl;
	memset(&ciph_ctrl, 0x0, sizeof(ciph_ctrl));
	ciph_ctrl.alg_info.alg = CIPHER_ALG_AES;
	ciph_ctrl.alg_info.bit_width = CIPHER_BIT_WIDTH_128BIT;
	ciph_ctrl.alg_info.work_mode = CIPHER_WORK_MODE_GCM;
#ifdef TEST_KEY_BY_EFUSE
	ciph_ctrl.key_info.key_by_efuse = true;
#else
	ciph_ctrl.key_info.key_by_efuse = false;
#endif
	ciph_ctrl.key_info.keylen = CIPHER_KEY_AES_256BIT;
	memcpy(ciph_ctrl.key_info.key, g_key, sizeof(ciph_ctrl.key_info.key));
	memcpy(ciph_ctrl.iv, g_iv, sizeof(ciph_ctrl.iv));

	ciph_ctrl.aad[3] = 0x72656461;
	ciph_ctrl.aad[2] = 0x65684444;
	ciph_ctrl.aad[1] = 0x412d612d;
	ciph_ctrl.aad[0] = 0x73696874;

	cipher_init();

	int hcipher = -1;
	int ret = cipher_creat_handle(&hcipher);
	if (ret != 0 || hcipher == -1) {
		printf("%s [%d] creat cipher handle fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	ret = cipher_config_handle(hcipher, &ciph_ctrl);
	if (ret != 0) {
		printf("%s [%d] config cipher handle fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	memcpy(u8_src_data, pkt_data, sizeof(g_clear_data));
	uint32_t out_len = 0;
	ret = cipher_encrypt(hcipher, u8_src_data, sizeof(u8_dst_data), u8_dst_data, &out_len);
	if (ret != 0) {
		printf("%s [%d] encrypt fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	uint32_t error_cnt = 0;
	uint32_t tag[4];
	ret = cipher_get_tag(hcipher, (uint8_t *)tag);
	for (i = 0; i < sizeof(g_icv) / 4; i++) {
		if (tag[i] != g_icv[i]) {
			printf("tag[%d] = 0x%x, icv[%d] = 0x%x\n", i, tag[i], i, g_icv[i]);
			error_cnt++;
		}
	}

	uint32_t result_data[256];
	// memset(result_data, 0, sizeof(result_data));
	memcpy(result_data, u8_dst_data, sizeof(result_data));

	// printf("static int g_gcm_sec_data[256] = {\n");
	for (i = 0; i < sizeof(u8_dst_data) / 4; i++) {
		if (result_data[i] == exp_data[i]) {
			// printf("des_pkt, no=%d data= 0x%x \n", i, result_data[i]);
		} else {
			error_cnt = error_cnt + 1;
			printf("compare error data_no=%d rdata= 0x%x  exp_data= 0x%x \n", i, result_data[i],
				     exp_data[i]);
		}

		// if (i % 16 == 0) {
		//     printf("\n    ");
		// }
		// printf("0x%x, ", result_data[i]);
	}
	// printf("};\n");
	// for (i = 0; i < 4; i++) {
	//     if (ciph_ctrl.icv[i] == exp_data[i + 4] ) {
	//         printf("des_pkt, no=%d icv_data=%x \n", i, ciph_ctrl.icv[i]);
	//     } else {
	//         error_cnt = error_cnt + 1;
	//         printf("compare error icv_no=%d icv_data= 0x%x  exp_data= 0x%x \n", i, ciph_ctrl.icv[i], exp_data[i + 4] );
	//     }
	// }

	if (error_cnt != 0) {
		printf("%s [%d] AES-GCM encrypt test error! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	} else {
		printf("%s [%d] AES-GCM encrypt test success! \n", __FUNCTION__, __LINE__);
	}

	cipher_deinit();
	seehi_func_exit();
	return 0;
}

static int decrypt_gcm_test(void)
{
	seehi_func_enter();
	uint32_t *pkt_data = g_gcm_sec_data;
	uint32_t *exp_data = g_clear_data;
	uint8_t u8_src_data[1024] = {0};
	uint8_t u8_dst_data[1024] = {0};

	// int pkt_data[12];
	// uint8_t u8_src_data[16] = {0};
	// uint8_t u8_dst_data[16] = {0};
	// int exp_data[12];

	// pkt_data[0] = 0x538ccb5d ;
	// pkt_data[1] = 0xa2d66093 ;
	// pkt_data[2] = 0x7592777c ;
	// pkt_data[3] = 0x2203ecf2 ;

	// exp_data[3] = 0x2d2d6965;
	// exp_data[2] = 0x7769687a;
	// exp_data[1] = 0x61687a2d;
	// exp_data[0] = 0x6d612d69;

	cipher_ctrl ciph_ctrl;
	memset(&ciph_ctrl, 0x0, sizeof(ciph_ctrl));
	ciph_ctrl.alg_info.alg = CIPHER_ALG_AES;
	ciph_ctrl.alg_info.bit_width = CIPHER_BIT_WIDTH_128BIT;
	ciph_ctrl.alg_info.work_mode = CIPHER_WORK_MODE_GCM;
#ifdef TEST_KEY_BY_EFUSE
	ciph_ctrl.key_info.key_by_efuse = true;
#else
	ciph_ctrl.key_info.key_by_efuse = false;
#endif
	ciph_ctrl.key_info.keylen = CIPHER_KEY_AES_256BIT;
	memcpy(ciph_ctrl.key_info.key, g_key, sizeof(ciph_ctrl.key_info.key));
	memcpy(ciph_ctrl.iv, g_iv, sizeof(ciph_ctrl.iv));
	memcpy(ciph_ctrl.icv, g_icv, sizeof(ciph_ctrl.icv));
	memcpy(ciph_ctrl.aad, g_aad, sizeof(ciph_ctrl.aad));

	// ciph_ctrl.icv[0] = 0x841053bc;
	// ciph_ctrl.icv[1] = 0x6a90576c;
	// ciph_ctrl.icv[2] = 0xdc0628d2;
	// ciph_ctrl.icv[3] = 0x3e4f853d;

	cipher_init();

	int hcipher = -1;
	int ret = cipher_creat_handle(&hcipher);
	if (ret != 0 || hcipher == -1) {
		printf("%s [%d] creat cipher handle fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	ret = cipher_config_handle(hcipher, &ciph_ctrl);
	if (ret != 0) {
		printf("%s [%d] config cipher handle fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	memcpy(u8_src_data, pkt_data, sizeof(g_gcm_sec_data));
	uint32_t out_len = 0;
	ret = cipher_decrypt(hcipher, u8_src_data, sizeof(u8_dst_data), u8_dst_data, &out_len);
	if (ret != 0) {
		printf("%s [%d] decrypt fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	uint32_t result_data[256];
	// memset(result_data, 0, sizeof(result_data));
	uint32_t error_cnt = 0;
	memcpy(result_data, u8_dst_data, out_len);
	int i = 0;
	for (i = 0; i < sizeof(u8_dst_data) / 4; i++) {
		if (result_data[i] == exp_data[i]) {
			// printf("des_pkt, no=%d data= 0x%x \n", i, result_data[i]);
		} else {
			error_cnt = error_cnt + 1;
			printf("compare error data_no=%d rdata= 0x%x  exp_data= 0x%x \n", i, result_data[i],
				     exp_data[i]);
		}
	}

	// for (i = 0; i < 4; i++) {
	//     if (ciph_ctrl.icv[i] == exp_data[i + 4] ) {
	//         printf("des_pkt, no=%d icv_data=%x \n", i, ciph_ctrl.icv[i]);
	//     } else {
	//         error_cnt = error_cnt + 1;
	//         printf("compare error icv_no=%d icv_data= %x  exp_data=%x \n", i, ciph_ctrl.icv[i], exp_data[i + 4] );
	//     }
	// }

	if (error_cnt != 0) {
		printf("%s [%d] AES-GCM decrypt test error! \n", __FUNCTION__, __LINE__);

	} else {
		printf("%s [%d] AES-GCM decrypt test success! \n", __FUNCTION__, __LINE__);
	}

	cipher_deinit();
	seehi_func_exit();
	return 0;
}

#if 0
static int encrypt_cbc_second_blc_test(void)
{
	seehi_func_enter();
	uint32_t *pkt_data = g_clear_data;
	int i = 0;
	uint8_t u8_src_data[1024] = { 0 };
	uint8_t u8_dst_data[1024] = { 0 };

	uint32_t *sec_data = g_cbc_sec_data;

	cipher_ctrl ciph_ctrl;
	memset(&ciph_ctrl, 0x0, sizeof(ciph_ctrl));
	ciph_ctrl.alg_info.alg = CIPHER_ALG_AES;
	ciph_ctrl.alg_info.bit_width = CIPHER_BIT_WIDTH_128BIT;
	ciph_ctrl.alg_info.work_mode = CIPHER_WORK_MODE_CBC;
#ifdef TEST_KEY_BY_EFUSE
	ciph_ctrl.key_info.key_by_efuse = true;
#else
	ciph_ctrl.key_info.key_by_efuse = false;
#endif
	ciph_ctrl.key_info.keylen = CIPHER_KEY_AES_256BIT;
	memcpy(ciph_ctrl.key_info.key, g_key, sizeof(ciph_ctrl.key_info.key));
	memcpy(ciph_ctrl.iv, &sec_data[0], sizeof(ciph_ctrl.iv));
	for (i = 0; i < 4; i++) {
		printf("iv[%d] = 0x%x\n", i, ciph_ctrl.iv[i]);
	}

	cipher_init();

	int hcipher = -1;
	int ret = cipher_creat_handle(&hcipher);
	if (ret != 0 || hcipher == -1) {
		printf("%s [%d] creat cipher handle fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	ret = cipher_config_handle(hcipher, &ciph_ctrl);
	if (ret != 0) {
		printf("%s [%d] config cipher handle fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	memcpy(u8_src_data, pkt_data, sizeof(u8_src_data));
	for (i = 0; i < 16; i++) {
		printf("u8_src_data[%ld] = 0x%x\n", i + sizeof(ciph_ctrl.iv),
			     u8_src_data[i + sizeof(ciph_ctrl.iv)]);
	}

	uint32_t out_len = 0;
	ret = cipher_encrypt(hcipher, &u8_src_data[sizeof(ciph_ctrl.iv)], sizeof(ciph_ctrl.iv), u8_dst_data, &out_len);
	if (ret != 0) {
		printf("%s [%d] encrypt fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	uint32_t result_data[256];
	// memset(result_data, 0, sizeof(result_data));
	uint32_t error_cnt = 0;
	memcpy(result_data, u8_dst_data, sizeof(ciph_ctrl.iv));

	// printf("int sec_data[256] {\n");
	for (i = 0; i < sizeof(ciph_ctrl.iv) / 4; i++) {
		if (result_data[i] == sec_data[i + 4]) {
			// printf("des_pkt, no=%d data=%x \n", i, result_data[i]);
		} else {
			error_cnt = error_cnt + 1;
			printf("compare error data_no=%d rdata= %x  exp_data=%x \n", i, result_data[i],
				     sec_data[i + 4]);
		}
		// if (i % 16 == 0) {
		//     printf("\n    ");
		// }
		// printf("0x%x, ", result_data[i]);
	}
	// printf("};\n");
	if (error_cnt != 0) {
		printf("%s [%d] AES-CBC encrypt second block test error! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	} else {
		printf("%s [%d] AES-CBC encrypt second block success! \n", __FUNCTION__, __LINE__);
	}

	cipher_deinit();
	seehi_func_exit();
	return 0;
}

static int decrypt_cbc_second_blc_test(void)
{
	seehi_func_enter();
	uint32_t *pkt_data = g_clear_data;
	int i = 0;
	uint8_t u8_src_data[1024] = { 0 };
	uint8_t u8_dst_data[1024] = { 0 };

	uint32_t *sec_data = g_cbc_sec_data;

	cipher_ctrl ciph_ctrl;
	memset(&ciph_ctrl, 0x0, sizeof(ciph_ctrl));
	ciph_ctrl.alg_info.bit_width = CIPHER_BIT_WIDTH_128BIT;
	ciph_ctrl.alg_info.work_mode = CIPHER_WORK_MODE_CBC;
#ifdef TEST_KEY_BY_EFUSE
	ciph_ctrl.key_info.key_by_efuse = true;
#else
	ciph_ctrl.key_info.key_by_efuse = false;
#endif
	ciph_ctrl.key_info.keylen = CIPHER_KEY_AES_256BIT;
	memcpy(ciph_ctrl.key_info.key, g_key, sizeof(ciph_ctrl.key_info.key));
	memcpy(ciph_ctrl.iv, &sec_data[0], sizeof(ciph_ctrl.iv));
	for (i = 0; i < 4; i++) {
		printf("iv[%d] = 0x%x\n", i, ciph_ctrl.iv[i]);
	}

	cipher_init();

	int hcipher = -1;
	int ret = cipher_creat_handle(&hcipher);
	if (ret != 0 || hcipher == -1) {
		printf("%s [%d] creat cipher handle fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	ret = cipher_config_handle(hcipher, &ciph_ctrl);
	if (ret != 0) {
		printf("%s [%d] config cipher handle fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	memcpy(u8_src_data, sec_data, sizeof(u8_src_data));
	for (i = 0; i < 16; i++) {
		printf("u8_src_data[%ld] = 0x%x\n", i + sizeof(ciph_ctrl.iv),
			     u8_src_data[i + sizeof(ciph_ctrl.iv)]);
	}

	uint32_t out_len = 0;
	ret = cipher_decrypt(hcipher, &u8_src_data[sizeof(ciph_ctrl.iv)], sizeof(ciph_ctrl.iv), u8_dst_data, &out_len);
	if (ret != 0) {
		printf("%s [%d] encrypt fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	uint32_t result_data[256];
	// memset(result_data, 0, sizeof(result_data));
	uint32_t error_cnt = 0;
	memcpy(result_data, u8_dst_data, sizeof(ciph_ctrl.iv));

	// printf("int sec_data[256] {\n");
	for (i = 0; i < sizeof(ciph_ctrl.iv) / 4; i++) {
		if (result_data[i] == pkt_data[i + 4]) {
			// printf("des_pkt, no=%d data=%x \n", i, result_data[i]);
		} else {
			error_cnt = error_cnt + 1;
			printf("compare error data_no=%d rdata= %x  exp_data=%x \n", i, result_data[i],
				     sec_data[i + 4]);
		}
		// if (i % 16 == 0) {
		//     printf("\n    ");
		// }
		// printf("0x%x, ", result_data[i]);
	}
	// printf("};\n");
	if (error_cnt != 0) {
		printf("%s [%d] AES-CBC decrypt second block test error! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	} else {
		printf("%s [%d] AES-CBC decrypt second block success! \n", __FUNCTION__, __LINE__);
	}

	cipher_deinit();
	seehi_func_exit();
	return 0;
}

static void spacc_irq_handler(void)
{
	// printf("================ spacc irq !!!, irq status = 0x%08x \n", SPACC->IRQ_STAT);
	// // if (SPACC->IRQ_STAT
	// SPACC->IRQ_STAT = 0x1000;
}

static int aes_align_test(void)
{
	seehi_func_enter();
	uint32_t *pkt_data = g_clear_data;
	uint8_t u8_src_data[1024] = { 0 };
	// uint8_t u8_dst_data[1024] = { 0 };

	//Data 0x692d616d 2d7a6861 7a686977 65692d2d 692d616d 2d7a6861 7a686977 65692d2d 692d616d 2d7a6861 7a686977 65692d2d
	// pkt_data[3]  =0x33333333;
	// pkt_data[2]  =0x22222222;
	// pkt_data[1]  =0x11111111;
	// pkt_data[0]  =0xffffffff;
	// pkt_data[7]  =0x2d2d6965;
	// pkt_data[6]  =0x66666666;
	// pkt_data[5]  =0x55555555;
	// pkt_data[4]  =0x44444444;

	// pkt_data[11] =0x2d2d6965;
	// pkt_data[10] =0x7769687a;
	// pkt_data[9]  =0x61687a2d;
	// pkt_data[8]  =0x6d612d69;

	uint32_t sec_data[8] = { 0 };
	// sec_data[0]  = 0x5646f3ce;
	// sec_data[1]  = 0xa9b1dd5d;
	// sec_data[2]  = 0x3d449313;
	// sec_data[3]  = 0xc163d243;
	// sec_data[4]  = 0xbe224065;
	// sec_data[5]  = 0xf030a711;
	// sec_data[6]  = 0x8ade21a6;
	// sec_data[7]  = 0xdbb02436;
	// sec_data[8]  = 0x99ad23be;
	// sec_data[9]  = 0xa7006678;
	// sec_data[10] = 0x48c8085d;
	// sec_data[11] = 0x01200efb;
	int ret;

	cipher_ctrl ciph_ctrl;
	memset(&ciph_ctrl, 0x0, sizeof(ciph_ctrl));
	ciph_ctrl.alg_info.alg = CIPHER_ALG_AES;
	ciph_ctrl.alg_info.bit_width = CIPHER_BIT_WIDTH_128BIT;
	ciph_ctrl.alg_info.work_mode = CIPHER_WORK_MODE_CBC;
#ifdef TEST_KEY_BY_EFUSE
	ciph_ctrl.key_info.key_by_efuse = true;
#else
	ciph_ctrl.key_info.key_by_efuse = false;
#endif
	ciph_ctrl.key_info.keylen = CIPHER_KEY_AES_256BIT;

	memcpy(ciph_ctrl.iv, g_iv, sizeof(ciph_ctrl.iv));

	cipher_init();

	int hcipher = -1;
	ret = cipher_creat_handle(&hcipher);
	if (ret != 0 || hcipher == -1) {
		printf("%s [%d] creat cipher handle fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	if (ciph_ctrl.key_info.key_by_efuse) {
		ret = cipher_efuse_attach_key(CIPHER_EFUSE_TARGET_AES);
		if (ret != 0) {
			cipher_deinit();
			return -1;
		}
	} else {
		memcpy(ciph_ctrl.key_info.key, g_key, sizeof(ciph_ctrl.key_info.key));
	}

	ret = cipher_config_handle(hcipher, &ciph_ctrl);
	if (ret != 0) {
		printf("%s [%d] config cipher handle fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	memcpy(u8_src_data, pkt_data, sizeof(u8_src_data));
	uint32_t out_len = 0;
	ret = cipher_encrypt_mult(hcipher, (uint8_t *)u8_src_data, 0x20, (uint8_t *)sec_data, &out_len);
	if (ret != 0) {
		printf("%s [%d] encrypt fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	uint32_t dec_data[8] = { 0 };

	ret = cipher_decrypt_mult(hcipher, (uint8_t *)sec_data, sizeof(dec_data), (uint8_t *)dec_data, &out_len);
	if (ret != 0) {
		printf("%s [%d] decrypt fail! \n", __FUNCTION__, __LINE__);
		cipher_deinit();
		return -1;
	}

	int i = 0;
	for (i = 0; i < 8; i++) {
		printf("dec_data[%d] = 0x%08x\n", i, dec_data[i]);
	}

	cipher_deinit();
	seehi_func_exit();
	return 0;
}
#endif

#ifdef TEST_CIPH_IRQ
static void spacc_irq_handler(void)
{
	printf("================ spacc irq !!!, irq status = 0x%08x \n", SPACC->IRQ_STAT);
	// if (SPACC->IRQ_STAT
	SPACC->IRQ_STAT = 0x1000;
}

int spacc_irq_test(void)
{
	seehi_func_enter();
	IRQ_Initialize();
	SPACC->IRQ_EN = 0xffffffff;// 1 | (1 << 4) | (1 << 12) | (1 << 31);
	SPACC->IRQ_CTRL = (1 << 0) | (1 << 16);
	IRQ_SetHandler(50 + 32, spacc_irq_handler);
	IRQ_SetPriority(50 + 32, 0 << 3);
	IRQ_Enable(50 + 32);
	// encrypt_cbc_test();
	return 0;
}
#endif

// int efuse_spacc_cbc256_test(void);
void spacc_test(void)
{
	int ret = 0;
	bool gcm_test = false;
	seehi_func_enter();
	// printf("EFUSE->EFUSE_MOD_U.ALL = 0x%8x\n", &EFUSE->EFUSE_MOD_U.ALL);
	// printf("EFUSE->EFUSE_INT_MASK_U.ALL = 0x%8x\n", &EFUSE->EFUSE_INT_MASK_U.ALL);
	// printf("EFUSE->EFUSE_INT_STATUS_U.ALL = 0x%8x\n", &EFUSE->EFUSE_INT_STATUS_U.ALL);
	// printf("EFUSE->EFUSE_INT_CLEAR_U.ALL = 0x%8x\n", &EFUSE->EFUSE_INT_CLEAR_U.ALL);
	// printf("EFUSE->EFUSE_USR_MOD_U.ALL = 0x%8x\n", &EFUSE->EFUSE_USR_MOD_U.ALL);
	// printf("EFUSE->EFUSE_PKA_MEM_U.ALL = 0x%8x\n", &EFUSE->EFUSE_PKA_MEM_U.ALL);
	// printf("EFUSE->EFUSE_T_CSB_LD = 0x%8x\n", &EFUSE->EFUSE_T_CSB_LD);

	// EFUSE->EFUSE_T_CSB_LD

#ifdef TEST_CIPH_IRQ
	spacc_irq_test();
#endif

#ifdef TEST_KEY_BURN_EFUSE
#define EFUSE_SKEY_BASE 12
 	ret = cipher_efuse_burn_key(CIPHER_EFUSE_TARGET_AES, g_key, sizeof(g_key));
	static uint32_t data[8] = {0};
	memset(data, 0, sizeof(data));
	ret |= efuse_auto_read_data(EFUSE_SKEY_BASE, data, sizeof(data));
	for (int i = 0; i < 8; i++) {
		if (data[i] != g_key[i]) {
			printf("burn key error !!! read data[%d] = 0x%x, write key[%d] = 0x%x \n", i, data[i], i, g_key[i]);
			ret = -1;
		}
	}
#endif
	// ret |= cipher_efuse_attach_key(CIPHER_EFUSE_TARGET_AES);
	// efuse_spacc_cbc256_test();
	// return ;
	printf("\n\n");
	// SPACC->IRQ_STAT = 0x1000;
	// printf("irq status = 0x%08x \n", SPACC->IRQ_STAT);
	ret |= encrypt_cbc_test();
	printf("\n");
	ret |= decrypt_cbc_test();
	printf("\n");
	// ret |= aes_align_test();
	if (gcm_test) {
		printf("\n");
		ret |= encrypt_gcm_test();
		printf("\n");
		ret |= decrypt_gcm_test();
		printf("\n");
	}
	// encrypt_cbc_second_blc_test();
	// printf("\n");
	// decrypt_cbc_second_blc_test();
	printf("\n");
	// ret |= decrypt_cbc_test();
	// printf("irq status = 0x%08x \n", SPACC->IRQ_STAT);

	if (ret == 0) {
		printf("spacc test success !\n\n");
	} else {
		printf("spacc test failed !!!\n\n");
	}
	seehi_func_exit();

	// while (1) {
	// }
}
