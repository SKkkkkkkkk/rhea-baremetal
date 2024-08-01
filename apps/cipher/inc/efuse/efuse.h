typedef enum {
	EFUSE_MODE_AUTORD,
	EFUSE_MODE_AUTOWR,
	EFUSE_MODE_SKEYRD,
	EFUSE_MODE_PKEYRD,
	EFUSE_MODE_USRMOD
} efuse_mode_type;

typedef enum { EFUSE_PKA_RAM_A, EFUSE_PKA_RAM_B, EFUSE_PKA_RAM_C, EFUSE_PKA_RAM_D } efuse_pka_memsel_type;


int efuse_init(void);

int efuse_burn_key(uint32_t *key, uint32_t key_len, uint32_t aes_or_ecc);

int efuse_private_aes_key(void);

int efuse_private_ecc_key(void);

int efuse_pka_get_key(efuse_pka_memsel_type memsel, uint32_t memaddr, uint32_t bytcnt);

int efuse_auto_read_data(uint32_t addr, uint32_t *data, uint32_t len);

int efuse_auto_write_data(uint32_t addr, uint32_t *data, uint32_t len);

int efuse_read_and_write_protect(uint32_t start_addr, uint32_t len, bool write);

void efuse_deinit(void);
