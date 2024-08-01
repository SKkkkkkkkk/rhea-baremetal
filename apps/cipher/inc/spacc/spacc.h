#ifndef __SPACC_H__
#define __SPACC_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

#define SEEHI_SUCCESS 0
#define SEEHI_FAILED -1

typedef enum {
	CIPH_ALG_DES,
	CIPH_ALG_AES,
	HASH_ALG_MD5,
	HASH_ALE_SHA_1,
	HASH_ALE_SHA_224,
	HASH_ALE_SHA_256,
	HASH_ALE_SHA_384,
	HASH_ALE_SHA_512,
	HASH_ALE_SHA3_224,
	HASH_ALE_SHA3_256,
	HASH_ALE_SHA3_384,
	HASH_ALE_SHA3_512,
	ALE_BUTT
} spacc_alg_type;

typedef enum {
	CIPH_MODE_ECB,
	CIPH_MODE_CBC,
	CIPH_MODE_GCM,
	HASH_MODE_RAW,
	HASH_MODE_SSLMAC,
	HASH_MODE_HMAC,
	MODE_BUTT
} spacc_mode_type;

typedef enum { RUN_DECRYPT = 0, RUN_ENCRYPT = 1, RUN_HASH = 2, RUN_BUTT } spacc_run_type;

void spacc_init(bool is_cipher);
int spacc_config_key(const uint32_t *key, uint32_t key_len, bool efuse_key, bool cipher);
int spacc_config_iv(const uint32_t *iv, uint32_t len, spacc_mode_type mode_t, uint32_t key_len, bool cipher);
int spacc_cfg_paket_format(spacc_mode_type mode_t, uint32_t *aad, uint32_t aad_len, uint32_t *icv, uint32_t icv_len);
int spacc_config_ctrl(spacc_alg_type alg_t, spacc_mode_type mode_t, bool efuese_key);
int spacc_send_data(const uint8_t *data, uint32_t len, spacc_run_type run_type, bool last_pkt);
int spacc_run(spacc_run_type run_type);
int spacc_get_data(uint8_t *data, uint32_t *len, spacc_run_type run_type, bool last_pkt);
void spacc_deinit(bool is_cipher);

#ifdef __cplusplus
}
#endif

#endif /* __DRV_CIPHER__ */