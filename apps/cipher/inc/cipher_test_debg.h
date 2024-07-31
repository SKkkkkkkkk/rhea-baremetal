#define seehi_func_enter() printf(">>> %s[%d] function enter ===>\n", __FUNCTION__, __LINE__)
#define seehi_func_exit() printf("<<< %s[%d] function exit <===\n", __FUNCTION__, __LINE__)
#define parem_invalid() printf("%s[%d] error: param Invalid !\n", __FUNCTION__, __LINE__)
#define function_err(func) printf("%s[%d] error: %s faild !\n", __FUNCTION__, __LINE__, #func)

/***********************************/
/** 基本功能测试 **/
//define TEST_CIPH_CIPHER /* 加解密 测试 --- spacc */
//define TEST_CIPH_HASH  /* HASH 计算 测试 --- spacc */
#define TEST_CIPH_ECC    /* ECC 签名校验 测试 ---- pka */
#define TEST_KEY_BY_EFUSE /* 加解密或者 ECC 签名时选择是否从eFuse中读取key */
#define TEST_KEY_BURN_EFUSE /* 加解密或者 ECC 签名时选择 efuse key 时，若板子未烧写 eFuse key ，则需要打开这个宏 */
//define TEST_EFUSE_SEC // efuse 读写保护测试
//#define TEST_CIPH_IRQ
/******************************/
/* NPU 模型文件解密测试 */
// #define TEST_DEC_NPU_FILE
// #define FILE_FROM_SD
// #define TEST_LOOP /* 循环解码模型文件 */

// #define ALG_MODEL_CLEAR_ADDR 0x15000000  /* 模型文件解密后的明文地址 */
// #define ALG_MODEL_ENC_ADDR 0x17000000   /* 模型文件密文加载地址 */
// #define ALG_MODEL_MAXLEN 0x2000000
