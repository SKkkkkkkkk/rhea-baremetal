#ifndef __DRV_CIPHER_H__
#define __DRV_CIPHER_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*
 * 对称解密只支持128bit位宽，单个包只支持64k大小的数据
 * 但达到64K 有数据错误，加解密不稳定，故单次加解密只能小于64K，
 * 同时必须保证 CIPHER_MAX_PKT_LEN 是128bit对其的
 */
#define CIPHER_MAX_PKT_LEN (63 * 1024)

typedef enum {
	CIPHER_ALG_AES = 0X0, /* AES 算法 */
	CIPHER_ALG_BUTT /* 等于或超出该定义为未定义算法 */
} cipher_alg;

typedef enum {
	CIPHER_BIT_WIDTH_64BIT = 0X0, /* 64bit 位宽 */
	CIPHER_BIT_WIDTH_8BIT = 0x1, /* 8bit 位宽 */
	CIPHER_BIT_WIDTH_1BIT = 0x2, /* 1bit 位宽 */
	CIPHER_BIT_WIDTH_128BIT = 0x3, /* 128bit 位宽 */
	CIPHER_BIT_WIDTH_BUTT
} cipher_bit_width;

typedef enum {
	CIPHER_WORK_MODE_EBC = 0x0,
	CIPHER_WORK_MODE_CBC = 0x1, /* CBC 模式 */
	CIPHER_WORK_MODE_GCM = 0x5, /* GCM模式 */
	CIPHER_WORK_MODE_BUTT
} cipher_work_mode;

typedef enum {
	CIPHER_KEY_AES_128BIT = 0x0, /* AES运算方式下采用128bit密钥长度 */
	CIPHER_KEY_AES_192BIT = 0x1, /* AES运算方式下采用192bit密钥长度 */
	CIPHER_KEY_AES_256BIT = 0x2, /* AES运算方式下采用256bit密钥长度 */
	CIPHER_KEY_BUTT
} cipher_key_length;

typedef struct {
	uint32_t key[8]; /* 秘钥（主要是作为自定义加/解密的key） */
	bool key_by_efuse; /* 是否使用efuse_key进行加/解密，（使用efuse_key解密的数据直接送给后级模块，无输出） */
	cipher_key_length keylen; /* 秘钥长度 */
} cipher_key_info;

typedef struct {
	cipher_alg alg; /* 加/解密使用的算法 */
	cipher_bit_width bit_width; /* 加/解密的位宽 */
	cipher_work_mode work_mode; /* 加/解密工作模式 */
} cipher_alg_info;

typedef struct {
	cipher_key_info key_info; /* key 信息 */
	uint32_t iv[4]; /* 初始向量 */
	uint32_t aad[4]; /* gcm消息队列 */
	uint32_t icv[4];
	cipher_alg_info alg_info; /* 算法信息 */
} cipher_ctrl;

/* ecc 签名校验数据结构 */
typedef enum {
	CIHPER_ECC_256BIT = 2, /* ecc 256bit */
	CIHPER_ECC_512BIT = 3, /* ecc 512bit */
	CIHPER_ECC_1024BIT = 4, /* ecc 1024bit */
	CIHPER_ECC_2048BIT = 5, /* ecc 2048bit */
	CIHPER_ECC_BUTT
} cipher_ecc_sign_scheme;

typedef struct {
	uint32_t *d; /* ecc 算法私钥 */
	uint32_t key_len; /* ecc 私钥 d 的长度，需与 cipher_ecc_sign_scheme 的bit位一致 */
} cipher_ecc_pri_key;

typedef struct {
	uint32_t *r; /* ecc 签名 r */
	uint32_t *s; /* ecc 签名 s */
	uint32_t len; /* ecc 私钥 r 及 s 的长度，需与 cipher_ecc_sign_scheme 的bit位一致 */
} ecc_sign_s;

typedef struct {
	uint32_t *p; /* 椭圆曲线模数 p */
	uint32_t *a; /* 椭圆曲线系数 a */
	uint32_t *b; /* 椭圆曲线系数 b */
	uint32_t *q; /* 椭圆曲线素数阶 q */
	uint32_t *Ax; /* 素数阶数q的循环群的点A x坐标 */
	uint32_t *Ay; /* 素数阶数q的循环群的点A y坐标 */
	uint32_t *k; /* 随机临时秘钥 k， 且 0 < k < q */
	uint32_t len; /* 椭圆曲线每单个参数的长度，需与 cipher_ecc_sign_scheme 的bit位一致 */
} cipher_ecc_curve;

typedef struct {
	uint32_t *p;
	uint32_t *a;
	uint32_t *b;
	uint32_t *q;
	uint32_t *Ax;
	uint32_t *Ay;
	uint32_t *Bx; /* 公钥点 B 的 x 数据 */
	uint32_t *By; /* 公钥点 B 的 y 数据 */
	uint32_t key_len; /* 公钥结构体每单个参数的长度，需与 cipher_ecc_sign_scheme 的bit位一致 */
} cipher_ecc_pub_key;

typedef enum {
	CIPHER_KEY_SRC_USER = 0, /* 使用用户自动定义私钥 key 进行签名 */
	CIPHER_KEY_SRC_EFUSE = 1, /* 使用efuse私钥 key 进行签名，【备注：暂未实现】 */
	CIPHER_KEY_SRC_BUTT
} cipher_key_from_type;

typedef struct {
	cipher_ecc_sign_scheme scheme;
	cipher_ecc_pri_key pri_key;
	cipher_ecc_curve ecc_param;
	cipher_key_from_type key_from;
} cipher_ecc_sign_s;

typedef struct {
	cipher_ecc_sign_scheme scheme;
	cipher_ecc_pub_key pub_key;
} cipher_ecc_verify_s;

/* hash */
typedef enum {
	CIPH_HASH_TYPE_SHA1 = 2,
	CIPH_HASH_TYPE_SHA224 = 3,
	CIPH_HASH_TYPE_SHA256,
	CIPH_HASH_TYPE_SHA384,
	CIPH_HASH_TYPE_SHA512,
	CIPH_HASH_TYPE_SHA3_224 = 16,
	CIPH_HASH_TYPE_SHA3_256 = 17, /* 只支持 SHA3-256 */
	CIPH_HASH_TYPE_SHA3_384 = 18,
	CIPH_HASH_TYPE_SHA3_512 = 19,
	CIPH_HASH_TYPE_BUTT
} cipher_hash_type;

typedef struct {
	uint32_t *hash_key;
	uint32_t key_len;
	cipher_hash_type hash_type;
} cipher_hash_atts;

typedef enum { CIPHER_EFUSE_TARGET_AES, CIPHER_EFUSE_TARGET_ECC, CIPHER_EFUSE_TARGET_BUTT } cipher_efuse_tar_get;

/***********************************************
  * @brief 初始化cipher。
  ***********************************************/
void cipher_init(void);

/***********************************************
  * @brief 创建一路cipher。
  * @param hcipher 句柄指针;   [输出]
  * @return 0 -- success；other -- 参见错误码。
  ***********************************************/
int cipher_creat_handle(int *hcipher);

/**************************************************************************
  * @brief 配置cipher控制信息，包含密钥、初始向量、加密算法、工作模式等信息。
  * @param hcipher 句柄指针;   [输入]
  * @param ctrl 控制信息指针;   [输入]
  * @return 0 -- success；other -- 参见错误码。
  * @remark 对称加密只支持128bit的数据位宽。
  *************************************************************************/
int cipher_config_handle(int hcipher, cipher_ctrl *ctrl);

/**************************************************************************
  * @brief 小数据包加密。(备注：小数据包加密只支持小于64K bytes 的数据大小)
  * @param hcipher 句柄指针;   [输入]
  * @param src_data 输入源数据   [输入]
  * @param src_len 加密数据长度 [输入]
  * @param dest_data 加密后的数据 [输出]
  * @param out_len 加密后的数据大小 [输出]
  * @return 0 -- success；other -- 参见错误码。
  *************************************************************************/
int cipher_encrypt(int hcipher, const uint8_t *src_data, uint32_t src_len, uint8_t *dest_data, uint32_t *out_len);

/**************************************************************************
  * @brief 小数据包加密。(备注：小数据包解密只支持小于64K bytes 的数据大小)
  * @param hcipher 句柄指针;   [输入]
  * @param src_data 输入源数据   [输入]
  * @param src_len 密文数据长度 [输入]
  * @param dest_data 解密后的数据 [输出]
  * @param out_len 解密后的数据大小 [输出]
  * @return 0 -- success；other -- 参见错误码。
  *************************************************************************/
int cipher_decrypt(int hcipher, const uint8_t *src_data, uint32_t src_len, uint8_t *dest_data, uint32_t *out_len);

/**************************************************************************
  * @brief 大数据加密。只支持 AES-CBC 的加密，
  *        大于等于64k的加密数据选择该接口，
  *        接口内部会将数据进行每包大小为 CIPHER_MAX_PKT_LEN 进行拆包加密。
  * @param hcipher 句柄指针;   [输入]
  * @param src_len 加密数据长度 [输入]
  * @param dest_data 加密后的数据 [输出]
  * @param out_len 加密后的数据大小 [输出]
  * @return 0 -- success；other -- 参见错误码。
  *************************************************************************/
int cipher_encrypt_mult(int hcipher, const uint8_t *src_data, uint32_t src_len, uint8_t *dest_data, uint32_t *out_len);

/**************************************************************************
  * @brief 大数据解密。只支持 AES-CBC 的加密，
  *        大于等于64k的加密数据选择该接口，
  *        接口内部会将数据进行每包大小为 CIPHER_MAX_PKT_LEN 进行拆包解密。
  * @param hcipher 句柄指针;   [输入]
  * @param src_data 输入源数据   [输入]
  * @param src_len 密文数据长度 [输入]
  * @param dest_data 解密后的数据 [输出]
  * @param out_len 解密后的数据大小 [输出]
  * @return 0 -- success；other -- 参见错误码。
  *************************************************************************/
int cipher_decrypt_mult(int hcipher, const uint8_t *src_data, uint32_t src_len, uint8_t *dest_data, uint32_t *out_len);

/* ***********************************************************************
 * 接口说明：AES-GCM加密时，获取加密过后的ICV的值。
 * 参数说明：
 *      hcipher     -- 句柄指针；   [输入]
 *      tags        -- 输出tag, 即ICV   [输出]
 * 返回值：0 -- success；other -- 参见错误码。
 *************************************************************************/
int cipher_get_tag(int hcipher, uint8_t *tag);

int cipher_destroy_handle(int hcipher);

void cipher_deinit(void);

void cipher_ecc_init(void);

/* ***********************************************************************
 * 接口说明：获取公钥key，即点B。
 * 参数说明：
 *      ecc_sign        -- 签名验证属性结构体    [输入]
 *      pub_key_Bx      -- 公钥点 B 的 x 值     [输出]
 *      pub_key_By      -- 公钥点 B 的 y 值     [输出]
 * 返回值：0 -- success；other -- 参见错误码。
 *************************************************************************/
int cipher_get_pubkey(cipher_ecc_sign_s *ecc_sign, uint32_t *pub_key_Bx, uint32_t *pub_key_By);

/* ****************************************************************************************************************
 * 接口说明：使用ECC算法进行数据签名。
 * 参数说明：
 *      ecc_sign    -- 签名验证属性结构体                                                      [输入]
 *      in_data     -- 待验证数据，如果hash_data不为空，则用hash_data进行验证，该参数将被忽略     [输入] 【备注：暂未实现】
 *      data_len    -- 待验证数据长度，单位：byte                                              [输入] 【备注：暂未实现】
 *      hash_data   -- 待验证文本的HASH摘要，如果为空，则自动计算in_data的HASH摘要进行验证        [输入]
 *      hash_len    -- HASH摘要长度，长度应与签名位宽一致。                                     [输入]
 *      out_sign    -- 签名数据                                                               [输出]
 * 返回值：0 -- success；other -- 参见错误码。
 *****************************************************************************************************************/
int cipher_ecc_sign(cipher_ecc_sign_s *eccsign, uint8_t *in_data, uint32_t data_len, uint8_t *hash_data,
		    uint32_t hash_len, ecc_sign_s *out_sign);

/* ****************************************************************************************************************
 * 接口说明：使用ECC算法进行数据签名校验。
 * 参数说明：
 *      ecc_sign    -- 签名验证属性结构体                                                      [输入]
 *      in_data     -- 待验证数据，如果hash_data不为空，则用hash_data进行验证，该参数将被忽略     [输入] 【备注：暂未实现】
 *      data_len    -- 待验证数据长度，单位：byte                                              [输入] 【备注：暂未实现】
 *      hash_data   -- 待验证文本的HASH摘要，如果为空，则自动计算in_data的HASH摘要进行验证        [输入]
 *      hash_len    -- HASH摘要长度，长度应与签名位宽一致。                                     [输入]
 *      out_sign    -- 签名数据                                                               [输入]
 * 返回值：0 -- success；other -- 参见错误码。
 *****************************************************************************************************************/
int cipher_ecc_verify(cipher_ecc_verify_s *ecc_verify, uint8_t *in_data, uint32_t data_len, uint8_t *hash_data,
		      uint32_t hash_len, ecc_sign_s *in_sign);

/* ****************************************************************************************************************
 * 接口说明：HASH 初始化。
 * 参数说明：
 *      hash_attr  -- HASH 初始化输入结构体。                                                   [输入] 【备注：目前只支持SHA3-256】
 *      handle     -- 句柄指针。                                                               [输出]
 * 返回值：0 -- success；other -- 参见错误码。
 *****************************************************************************************************************/
int cipher_hash_init(cipher_hash_atts *hash_attr, int *handle);

/* ****************************************************************************************************************
 * 接口说明：计算 HASH值 。
 * 参数说明：
 *      handle      -- hash 句柄。                                                         [输入]
 *      in_data     -- 输入数据缓冲。                                                       [输入]
 *      data_len    -- 输入数据长度。                                                       [输入]
 * 返回值：0 -- success；other -- 参见错误码。
 * 注意： 1. 可以多次输入数据，即可以多次调用此函数。
 *        2. 如果是多次输入数据，则输入的数据的长度必须为32字节(SHA3-256)对齐，最后一次数据无要求。
 *          但目前硬件只支持一次性输入，数据大小不能超过64k
 *****************************************************************************************************************/
int cipher_hash_update(int handle, const uint8_t *in_data, uint32_t data_len);

/* ****************************************************************************************************************
 * 接口说明：获取hash值。
 * 参数说明：
 *      handle      -- hash 句柄。                                                         [输入]
 *      hash_data   -- 输出的hash值，大小为32字节（SHA-256）。                               [输出]
 * 返回值：0 -- success；other -- 参见错误码。
 *****************************************************************************************************************/
int cipher_hash_final(int handle, uint8_t *hash_data);

/* ****************************************************************************************************************
 * 接口说明：计算 HASH 值并输出hash值 。
 * 参数说明：
 *      handle      -- hash 句柄。                                                         [输入]
 *      src_data    -- 输入数据缓冲。                                                       [输入]
 *      length      -- 输入数据长度。                                                       [输入]
 *      hash_data   -- 输出的hash值，大小为32字节（SHA-256）。                               [输出]
 * 返回值：0 -- success；other -- 参见错误码。
 * 备注： 结合了单/多次调用 drv_cipher_hash_update和drv_cipher_hash_final，数据大小不限制。(目前硬件只支持一次性输入, 故该接口未实现)
 *****************************************************************************************************************/
int cipher_hash(int handle, uint8_t *src_data, uint32_t length, uint8_t *hash_data);

int cipher_efuse_burn_key(cipher_efuse_tar_get target, uint32_t *key, uint32_t key_len);

int cipher_efuse_attach_key(cipher_efuse_tar_get target);

void cipher_hash_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __DRV_CIPHER__ */