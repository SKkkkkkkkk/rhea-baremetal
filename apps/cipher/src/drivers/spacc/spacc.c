#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "utils_def.h"
#include "spacc.h"
#include "systimer.h"
#include "spacc_regs.h"
#include "cipher_debug.h"

#define writel(d, a) *(volatile uint32_t *)(a) = (d)
// #define writel(d, a) __DMB(); *(volatile uint32_t *)(a) = (d)
#define readl(a) (*(volatile uint32_t *)(a))
#define udelay(x)                                                                                                      \
	do {                                                                                                           \
		unsigned xx = x;                                                                                       \
		while (xx--) {                                                                                         \
			asm volatile("nop");                                                                           \
		}                                                                                                      \
	} while (0)
// #define BIT(b)  (1 << (b))
#define SPACC_CIPHER_KEY BIT(31)
#define M3VIEW_FST_SRC_ADDR __M3VIEW_FST_SRC_ADDR
#define M3VIEW_FST_DST_ADDR __M3VIEW_FST_DST_ADDR
#define M3VIEW_DATA_ADDR_OFFSET 0x10
#define M3VIEW_DATALEN_ADDR_OFFSET 0x4

typedef struct {
	volatile uint32_t data_addr;
	volatile uint32_t data_len;
} ddt_info;

static volatile ddt_info *g_ddt_in = NULL;
static volatile ddt_info *g_ddt_out = NULL;

#define SPACC_CIPH_ALG_AES 0x2
#define SPACC_CIPH_ALG_RC4 0x3
#define SPACC_HASH_ALG_MD5 (0x1 << 3)
#define SPACC_HASH_ALG_SHA (0x2 << 3)
#define SPACC_HASH_ALG_SHA_224 (0x3 << 3)
#define SPACC_HASH_ALG_SHA_256 (0x4 << 3)
#define SPACC_HASH_ALG_SHA_384 (0x5 << 3)
#define SPACC_HASH_ALG_SHA_512 (0x6 << 3)
#define SPACC_HASH_ALG_SHA3_224 (16 << 3)
#define SPACC_HASH_ALG_SHA3_256 (17 << 3)
#define SPACC_HASH_ALG_SHA3_384 (18 << 3)
#define SPACC_HASH_ALG_SHA3_512 (19 << 3)

#define SPACC_CIPH_MODE_ECB (0 << 8)
#define SPACC_CIPH_MODE_CBC (1 << 8)
#define SPACC_CIPH_MODE_GCM (5 << 8)
#define SPACC_CIPH_MODE_NONE (0xf << 8)

#define SPACC_HASH_MODE_RWA (0 << 12)
#define SPACC_HASH_MODE_SSLMAC (1 << 12)
#define SPACC_HASH_MODE_HMAC (2 << 12)
#define SPACC_HASH_MODE_NONE (3 << 12)

#define SPACC_MSG_BEGIN (1 << 14)
#define SPACC_MSG_END (1 << 15)
#define SPACC_ENCRYPT (1 << 24)
#define SPACC_DECRYPT (0 << 24)
#define SPACC_AAD_COPY (1 << 25)
#define SPACC_ICV_PT (1 << 26)
#define SPACC_ICV_ENC (1 << 27)
#define SPACC_ICV_APPEND (1 << 28)
#define SPACC_KEY_EXP (1 << 29)
#define SPACC_SEC_KEY (1 << 31)

#define SPACC_FIFO_STAT_CNT (0x1ff << 16)
#define SPACC_FIFO_STAT_CNT_QOS (0x7f << 24)
#define SPACC_FIFO_STAT_EMPTY (1 << 31)
#define SPACC_STAT_RET_CODE (7 << 24)
#define SPACC_SW_ID 0xb9

#define spacc_alg_aes

#define str_to_u32(str, u32data)                                                                                       \
	do {                                                                                                           \
		(u32data) = (str)[0] << 3 * 8 | (str)[1] << 2 * 8 | (str)[2] << 1 * 8 | (str)[3];                      \
		(str) += 4;                                                                                            \
	} while (0)

#define str_to_u16(str, u16data)                                                                                       \
	do {                                                                                                           \
		(u16data) = (str)[0] << 1 * 8 | (str)[1];                                                              \
		(str) += 2;                                                                                            \
	} while (0)

#define u32_to_str(str, u32data)                                                                                       \
	do {                                                                                                           \
		(str)[0] = ((u32data) >> 3 * 8) & 0xff;                                                                \
		(str)[1] = ((u32data) >> 2 * 8) & 0xff;                                                                \
		(str)[2] = ((u32data) >> 1 * 8) & 0xff;                                                                \
		(str)[3] = (u32data)&0xff;                                                                             \
		(str) += 4;                                                                                            \
	} while (0)

#define u16_to_str(str, u16data)                                                                                       \
	do {                                                                                                           \
		(str)[0] = ((u16data) >> 1 * 8) & 0xff;                                                                \
		(str)[1] = (u16data)&0xff;                                                                             \
		(str) += 2;                                                                                            \
	} while (0)

typedef struct {
	uint32_t offset;
	uint32_t pre_add_len;
	uint32_t post_add_len;
	uint32_t proc_len;
	uint32_t icv_len;
	uint32_t icv_offset;
	uint32_t iv_offset;
} spacc_protected;

typedef struct {
	uint32_t *aad;
	uint32_t aad_len;
	uint32_t *icv;
	uint32_t icv_len;
} spacc_gcm_protected;

typedef struct {
	bool is_init;
	uint32_t block_size;
	uint32_t ctrl_cfg;
	volatile ddt_info *ddt_in;
	volatile ddt_info *ddt_out;
} spacc_run_info;

static spacc_run_info g_spacc_run_info[2];
static spacc_gcm_protected g_gcm_protected = { 0 };

static spacc_reg *__spacc_get_reg_base(bool cipher)
{
	if (g_spacc_run_info[cipher].is_init != true) {
		return NULL;
	}
	return SPACC;
}

static uint32_t __spacc_get_m3view_src_addr(void)
{
	return M3VIEW_FST_SRC_ADDR;
}

static uint32_t __spacc_get_m3view_dst_addr(void)
{
	return M3VIEW_FST_DST_ADDR;
}

static uint32_t __spacc_get_ctrl(bool cipher)
{
	if (g_spacc_run_info[cipher].is_init) {
		return g_spacc_run_info[cipher].ctrl_cfg;
	}
	cipher_debug_err("error: this not init !!\n");
	return 0;
}

static int __spacc_set_ctrl(uint32_t ctrl, bool cipher)
{
	if (g_spacc_run_info[cipher].is_init == false) {
		cipher_debug_err("error: not init !!\n");
		return -1;
	}

	g_spacc_run_info[cipher].ctrl_cfg = ctrl;

	return 0;
}

static uint32_t __spacc_get_block_size(bool cipher)
{
	if (g_spacc_run_info[cipher].is_init) {
		return g_spacc_run_info[cipher].block_size;
	}
	cipher_debug_err("error: this not init !!\n");
	return 0;
}

static int __spacc_set_block_size(uint32_t block_size, bool cipher)
{
	g_spacc_run_info[cipher].block_size = block_size;

	return 0;
}

static void __spacc_set_gcm_proteted(uint32_t *aad, uint32_t aad_len, uint32_t *icv, uint32_t icv_len)
{
	g_gcm_protected.aad = aad;
	g_gcm_protected.aad_len = aad_len;
	g_gcm_protected.icv = icv;
	g_gcm_protected.icv_len = icv_len;
}

static spacc_gcm_protected *__spacc_get_gcm_proteted(void)
{
	return &g_gcm_protected;
}

static int __spacc_get_status(uint32_t *timeout, bool is_cipher)
{
	spacc_reg *reg_ptr = __spacc_get_reg_base(is_cipher);
	uint32_t version = 0;

	if (reg_ptr == NULL) {
		return SEEHI_FAILED;
	}
	uint32_t fifo_stat = 0;
	uint32_t cnt = 0;
	while (1) {
		fifo_stat = readl(&reg_ptr->FIFO_STAT);
		version = readl(&reg_ptr->VERSION);
		if ((version & (0x1 << 8)) &&
			(((fifo_stat & SPACC_FIFO_STAT_CNT_QOS) >> 24) == 0x1))
			break;
		else if (!(version & (0x1 << 8)) &&
				(((fifo_stat & SPACC_FIFO_STAT_CNT) >> 16) == 0x1))
			break;
		else {
			udelay(1000);
			cnt++;
		}

		if (cnt > 10000) { /* 10ms */
			*timeout = 1;
			cipher_debug_err("fifo stat is NOT empty, stats=%x in hex \n", fifo_stat);
			return -1;
		}
	}

	writel(0x1, &reg_ptr->STAT_POP);
	udelay(150);
	uint32_t stat = readl(&reg_ptr->STATUS);
	if ((stat & SPACC_STAT_RET_CODE) == 0) {
		return 0;
	} else {
		cipher_debug_err("status fail status = %d \n", (stat & SPACC_STAT_RET_CODE) >> 24);
		return ((stat & SPACC_STAT_RET_CODE) >> 24);
	}
}

void spacc_init(bool is_cipher)
{
	if (g_ddt_in == NULL) {
		g_spacc_run_info[is_cipher].ddt_in = g_ddt_in = (ddt_info *)M3VIEW_FST_SRC_ADDR;
	}

	if (g_ddt_out == NULL) {
		g_spacc_run_info[is_cipher].ddt_out = g_ddt_out = (ddt_info *)M3VIEW_FST_DST_ADDR;
	}
	g_spacc_run_info[is_cipher].is_init = true;
}

int spacc_config_key(const uint32_t *key, uint32_t key_len, bool efuese_key, bool cipher)
{
	int i = 0;
	spacc_reg *reg_ptr = __spacc_get_reg_base(cipher);
	if (reg_ptr == NULL) {
		cipher_parem_invalid();
		return SEEHI_FAILED;
	}

	if (cipher == true) {
		if (key_len == 0) {
			cipher_parem_invalid();
			return -1;
		}

		writel(SPACC_CIPHER_KEY | key_len, &reg_ptr->KEY_SZ);
		if (efuese_key == true) {
			return 0;
		}

		if (key == NULL) {
			cipher_parem_invalid();
			return -1;
		}

		for (i = 0; i < key_len / 4; i ++) {
			writel(key[i], &reg_ptr->CIPH_KEY[i]);
		}
	} else {
		if (key_len != 0) {
			writel(key_len, &reg_ptr->KEY_SZ);
			for (i = 0; i < key_len / 4; i ++) {
				writel(key[i], &reg_ptr->HASH_KEY[i]);
			}
		}
	}
	return SEEHI_SUCCESS;
}

int spacc_config_iv(const uint32_t *iv, uint32_t len, spacc_mode_type mode_t, uint32_t key_len, bool cipher)
{
	int i = 0;
	spacc_reg *reg_ptr = __spacc_get_reg_base(cipher);
	if (reg_ptr == NULL) {
		cipher_parem_invalid();
		return SEEHI_FAILED;
	}

	if (cipher != true) { /* hash */
		writel(0, &reg_ptr->IV_OFFSET);
		return 0;
	}

	if (len != 16) { /* SPACC only supports 128-bit wide encryption and decryption  */
		cipher_debug_err("iv len error!! SPACC only supports 128-bit wide encryption and decryption\n");
		return SEEHI_FAILED;
	}

	if (mode_t == CIPH_MODE_ECB || mode_t == CIPH_MODE_CBC || mode_t == CIPH_MODE_GCM) {
		if (__spacc_set_block_size(len, true) != 0) {
			cipher_debug_err("block_size error!\n");
			return -1;
		}
	}

	if (mode_t == CIPH_MODE_CBC || mode_t == CIPH_MODE_GCM) { /* IV 紧跟在 KEY 后面 */
		for (i = 0; i < len / 4; i++) {
			writel(iv[i], &reg_ptr->CIPH_KEY[key_len / 4 + i]);
		}
	}

	for (i = ((key_len + len) / 4); i < sizeof(reg_ptr->CIPH_KEY) / 4; i++) {
		writel(0, &reg_ptr->CIPH_KEY[i]);
	}

	writel(0, &reg_ptr->IV_OFFSET);
	return SEEHI_SUCCESS;
}

int spacc_cfg_paket_format(spacc_mode_type mode_t, uint32_t *aad, uint32_t aad_len, uint32_t *icv, uint32_t icv_len)
{
	uint32_t m3view_data_src_addr = __spacc_get_m3view_src_addr() + M3VIEW_DATA_ADDR_OFFSET;
	uint32_t m3view_data_dst_addr = __spacc_get_m3view_dst_addr() + M3VIEW_DATA_ADDR_OFFSET;
	uint32_t version_ext = 0;

	if (g_ddt_in == NULL || g_ddt_out == NULL) {
		return -1;
	}
	bool is_cipher = (mode_t < HASH_MODE_RAW);
	g_ddt_in->data_addr = m3view_data_src_addr;
	g_ddt_out->data_addr = m3view_data_dst_addr;

	spacc_reg *reg_ptr = __spacc_get_reg_base(is_cipher);
	if (reg_ptr == NULL) {
		return SEEHI_FAILED;
	}

	if (mode_t == CIPH_MODE_GCM) {
		if (aad == NULL || icv == NULL) {
			return -1;
		}

		__spacc_set_gcm_proteted(aad, aad_len, icv, icv_len);
		writel(0, &reg_ptr->OFFSET);
		writel(aad_len, &reg_ptr->PRE_AAD_LEN);
		writel(icv_len, &reg_ptr->ICV_LEN);
		writel(0, &reg_ptr->ICV_OFFSET);
	} else {
		writel(0, &reg_ptr->OFFSET);
		writel(0, &reg_ptr->PRE_AAD_LEN);
		writel(0, &reg_ptr->ICV_LEN);
		writel(0, &reg_ptr->ICV_OFFSET);
	}
	writel(0, &reg_ptr->POST_AAD_LEN);
	writel(SPACC_SW_ID, &reg_ptr->SW_CTRL);
	writel(0, &reg_ptr->AUX_INFO);

	version_ext = readl(&reg_ptr->VERSION_EXT);
	switch ((version_ext >> 28) & 0x3) {
		case 1:		/* DDT */
			writel((uint32_t)(uintptr_t)g_ddt_in, &reg_ptr->SRC_PTR);
			writel((uint32_t)(uintptr_t)g_ddt_out, &reg_ptr->DST_PTR);
			break;
		case 2:		/* Linear */
			writel((uint32_t)(uintptr_t)g_ddt_in->data_addr, &reg_ptr->SRC_PTR);
			writel((uint32_t)(uintptr_t)g_ddt_out->data_addr, &reg_ptr->DST_PTR);
			break;
		default:
			printf("read version ext 0x%x regitser fialed\n", version_ext);
			return SEEHI_FAILED;
	}

	return SEEHI_SUCCESS;
}

int spacc_config_ctrl(spacc_alg_type alg_t, spacc_mode_type mode_t, bool efuese_key)
{
	uint32_t ctrl = 0;
	uint32_t alg = 0, mode = 0;
	spacc_run_type run_type = 0;
	switch (alg_t) {
	case CIPH_ALG_AES:
		alg = SPACC_CIPH_ALG_AES;
		break;

	case HASH_ALE_SHA3_256:
		alg = SPACC_HASH_ALG_SHA3_256;
		if (__spacc_set_block_size(32, false) != 0) {
			return -1;
		}
		break;
	default:
		cipher_debug_err("error: spacc not support this alg type, error type = %d \n", alg_t);
		return -1;
	}

	switch (mode_t) {
	case CIPH_MODE_ECB:
		mode = SPACC_CIPH_MODE_ECB | SPACC_HASH_MODE_SSLMAC;
		ctrl = alg | mode;
		break;
	case CIPH_MODE_CBC:
		mode = SPACC_CIPH_MODE_CBC | SPACC_HASH_MODE_SSLMAC;
		ctrl = alg | mode | SPACC_KEY_EXP | SPACC_MSG_BEGIN | SPACC_MSG_END;
		break;
	case CIPH_MODE_GCM:
		mode = SPACC_CIPH_MODE_GCM | SPACC_HASH_MODE_NONE;
		ctrl = alg | mode | SPACC_ICV_APPEND | SPACC_MSG_BEGIN | SPACC_MSG_END;
		break;
	case HASH_MODE_RAW:
		mode = SPACC_HASH_MODE_RWA;
		ctrl = alg | mode;
		run_type = RUN_HASH;

		break;
	default:
		cipher_debug_err("error: spacc not support this mode type, error type = %d \n", mode);
		return -1;
		;
	}

	if ((efuese_key == true) && (mode_t != HASH_MODE_RAW)) {
		ctrl |= SPACC_SEC_KEY;
	}
	bool is_cipher = (run_type != RUN_HASH);

	if (__spacc_set_ctrl(ctrl, is_cipher) != 0) {
		return -1;
	}

	return SEEHI_SUCCESS;
}

/* PKCS#7 padding */
static uint32_t padding(uint8_t *pand_data, uint32_t all_len, uint32_t block_sz)
{
	uint32_t padd_num = block_sz - (all_len % block_sz);

	for (int i = 0; i < padd_num; ++i) {
		pand_data[i] = (uint8_t)padd_num;
	}

	return padd_num;
}

static uint32_t unpadding(uint8_t *src, uint32_t len, uint32_t block_sz)
{
	uint8_t pad_num = src[len - 1];
	int8_t i;

	for (i = pad_num - 1; i >= 0; i--) {
		// cipher_debug_dbg("src[%d] = %d\n", (uint32_t)i, (uint32_t)src[(len - pad_num) + i]);
		if (src[(len - pad_num) + i] != pad_num) {
			cipher_debug_err("unpadding error!\n");
			return 0;
		}
	}

	uint32_t unpad_len = len - (uint32_t)pad_num;
	if (pad_num != (block_sz - (unpad_len % block_sz))) {
		cipher_debug_err("unpadding number error! pad_num = %d \n", (uint32_t)pad_num);
		return 0;
	}

	return unpad_len;
}

int spacc_send_data(const uint8_t *data, uint32_t len, spacc_run_type run_type, bool last_pkt)
{
	if (data == NULL || run_type >= RUN_BUTT) {
		cipher_parem_invalid();
		return -1;
	}

	if (g_ddt_in == NULL || g_ddt_out == NULL) {
		cipher_debug_err("spacc buffer error \n");
		return -1;
	}
	volatile uint8_t *m3view_pkt_data = (uint8_t *)(uintptr_t)(g_ddt_in->data_addr);
	if (m3view_pkt_data == NULL) {
		cipher_debug_err("spacc buffer error \n");
		return -1;
	}

	volatile uint32_t remain_len = 0;
	volatile uint32_t src_pkt_len = 0;
	volatile uint32_t dst_pkt_len = 0;

	bool is_cipher = (run_type != RUN_HASH);

	uint32_t block_size = __spacc_get_block_size(is_cipher);
	if (block_size == 0) {
		cipher_debug_err("block size error \n");
		return -1;
	}

	uint32_t ctrl = __spacc_get_ctrl(is_cipher);
	if (ctrl == 0) {
		cipher_debug_err("spacc ctrol error \n");
		return -1;
	}

	if ((ctrl & SPACC_CIPH_MODE_NONE) == SPACC_CIPH_MODE_GCM) {
		spacc_gcm_protected *gcm_pro = __spacc_get_gcm_proteted();
		memcpy((void *)m3view_pkt_data, gcm_pro->aad, gcm_pro->aad_len);
		m3view_pkt_data += gcm_pro->aad_len;
		src_pkt_len += gcm_pro->aad_len;
		if (run_type == RUN_ENCRYPT) {
			dst_pkt_len += gcm_pro->aad_len;
		}
	}

	memcpy((void *)m3view_pkt_data, data, len);
	if ((run_type == RUN_ENCRYPT) && ((ctrl & SPACC_CIPH_MODE_NONE) != SPACC_CIPH_MODE_GCM) && last_pkt) {
		remain_len = padding((uint8_t *)(m3view_pkt_data + len), len, block_size);
		if ((remain_len > block_size) || (remain_len == 0)) {
			cipher_debug_err("padding error \n");
			return -1;
		}
	}

	spacc_reg *reg_ptr = __spacc_get_reg_base(is_cipher);
	if (reg_ptr == NULL) {
		return SEEHI_FAILED;
	}

	src_pkt_len += len + remain_len;
	if (run_type == RUN_HASH) {
		writel(src_pkt_len, &reg_ptr->PRE_AAD_LEN);
		dst_pkt_len = block_size;
	} else {
		dst_pkt_len += len + remain_len;
	}

	writel(src_pkt_len, &reg_ptr->PROC_LEN);
	// writel(1024, &reg_ptr->PROC_LEN);

	if (((ctrl & SPACC_CIPH_MODE_NONE) == SPACC_CIPH_MODE_GCM) && (run_type == RUN_DECRYPT)) {
		spacc_gcm_protected *gcm_pro = __spacc_get_gcm_proteted();
		memcpy((void *)(m3view_pkt_data + (len + remain_len)), gcm_pro->icv, gcm_pro->icv_len);
		src_pkt_len += gcm_pro->icv_len;
	}

	cipher_debug_dbg("src_pkt_len = 0x%x\n", src_pkt_len);

	g_ddt_in->data_len = src_pkt_len;
	g_ddt_out->data_len = dst_pkt_len;

	return SEEHI_SUCCESS;
}

int spacc_run(spacc_run_type run_type)
{
	if (run_type > RUN_HASH) {
		return -1;
	}
	bool is_cipher = (run_type != RUN_HASH);
	uint32_t ctrl = __spacc_get_ctrl(is_cipher);
	if (ctrl == 0) {
		return -1;
	}

	if (run_type == RUN_ENCRYPT || run_type == RUN_HASH) {
		ctrl |= SPACC_ENCRYPT;
	} else if (run_type == RUN_DECRYPT) {
		ctrl |= SPACC_DECRYPT;
	} else {
		cipher_debug_err("error: this run type is error !\n");
		return -1;
	}

	cipher_debug_dbg("spacc ctrl = 0x%x\n", ctrl);

	spacc_reg *reg_ptr = __spacc_get_reg_base(is_cipher);
	if (reg_ptr == NULL) {
		cipher_parem_invalid();
		return SEEHI_FAILED;
	}

	// __DMB();
	// __DSB();
	// __ISB();
	writel(ctrl, &reg_ptr->CTRL);
	return SEEHI_SUCCESS;
}

static int __spacc_spacc_get_data(uint8_t *data, uint32_t *len, spacc_run_type run_type, bool last_pkt)
{
	if (g_ddt_out == NULL) {
		cipher_parem_invalid();
		return -1;
	}
	uint8_t *outpkt_data = (uint8_t *)(uintptr_t)(g_ddt_out->data_addr);
	if (outpkt_data == NULL) {
		return -1;
	}
	bool is_cipher = (run_type != RUN_HASH);
	uint32_t outpkt_len = g_ddt_out->data_len;
	if (len == NULL) {
		cipher_debug_err("data len fail \n");
		return -1;
	}

	uint32_t ctrl = __spacc_get_ctrl(is_cipher);
	if (ctrl == 0) {
		cipher_debug_err("spacc ctrl fail \n");
		return -1;
	}

	*len = outpkt_len;

	uint32_t block_size = __spacc_get_block_size(is_cipher);
	if (block_size == 0) {
		cipher_debug_err("block size fail \n");
		return -1;
	}

	if ((run_type == RUN_DECRYPT) && ((ctrl & SPACC_CIPH_MODE_NONE) != SPACC_CIPH_MODE_GCM) && last_pkt) {
		uint32_t unpad_len = 0;
		unpad_len = unpadding(outpkt_data, outpkt_len, block_size);
		if ((unpad_len > block_size + outpkt_len) || (unpad_len == 0) || (unpad_len == outpkt_len)) {
			cipher_debug_err("unpadding fail \n");
			return -1;
		}
		*len = unpad_len;
	}

	memcpy((void *)data, outpkt_data, *len);

	if (((ctrl & SPACC_CIPH_MODE_NONE) == SPACC_CIPH_MODE_GCM) && (run_type == RUN_ENCRYPT)) {
		spacc_gcm_protected *gcm_pro = __spacc_get_gcm_proteted();
		memcpy((void *)gcm_pro->icv, outpkt_data + (outpkt_len - gcm_pro->icv_len), gcm_pro->icv_len);
	}

	return 0;
}

int spacc_get_data(uint8_t *data, uint32_t *len, spacc_run_type run_type, bool last_pkt)
{
	int ret, timeout = 0;
	if (data == NULL || run_type > RUN_HASH) {
		return -1;
	}

	bool is_cipher = (run_type != RUN_HASH);
	ret = __spacc_get_status((uint32_t *)(&timeout), is_cipher);
	if (ret != 0) {
		if (timeout != 0) {
			cipher_debug_err("get status time out \n");
			return -1;
		}
		cipher_debug_err("get status fail ret = %d\n", ret);
		return -1;
	}

	ret = __spacc_spacc_get_data(data, len, run_type, last_pkt);
	if (ret != 0) {
		cipher_debug_err("spacc get data error! \n");
		return -1;
	}

	return 0;
}

void spacc_deinit(bool is_cipher)
{
	g_spacc_run_info[is_cipher].block_size = 0;
	g_spacc_run_info[is_cipher].ctrl_cfg = 0;
	g_spacc_run_info[is_cipher].is_init = false;
	g_spacc_run_info[is_cipher].ddt_in = NULL;
	g_spacc_run_info[is_cipher].ddt_out = NULL;
	if (g_spacc_run_info[0].is_init == false && g_spacc_run_info[1].is_init == false) {
		g_ddt_in = NULL;
		g_ddt_out = NULL;
	}
}
