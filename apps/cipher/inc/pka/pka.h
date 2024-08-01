#ifndef __PKA_H__
#define __PKA_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32_t *x;
	uint32_t *y;
	uint32_t len;
} pka_piont;

typedef struct {
	uint32_t *a;
	uint32_t *b;
	uint32_t *k;
	uint32_t *l;
	uint32_t *w;
	uint32_t *p;
	uint32_t len;
} pka_ecc_param_info;

typedef enum {
	PKA_BASE_RADIX_256BIT = 2,
	PKA_BASE_RADIX_512BIT = 3,
	PKA_BASE_RADIX_1024BIT = 4,
	PKA_BASE_RADIX_2048BIT = 5,
	PKA_BASE_RADIX_4096BIT = 6,
	PKA_BASE_RADIX_BUTT
} pka_base_radix_type;

typedef struct pka_mod_adap {
	pka_base_radix_type radix_type;
	uint32_t is_sec_key;
	uint32_t is_mult_key;
	int (*mult)(struct pka_mod_adap *adp, uint32_t *x, uint32_t *y, uint32_t *m, uint32_t *c,
		    uint32_t len); /* c = x•y mod m provided (0 <= x < m) and (0 <= y < m) */
	int (*add)(struct pka_mod_adap *adp, uint32_t *x, uint32_t *y, uint32_t *m, uint32_t *c,
		   uint32_t len); /* c = x + y mod m provided (0 <= x < 2m) and (0 <= y < 2m). */
	int (*sub)(struct pka_mod_adap *adp, uint32_t *x, uint32_t *y, uint32_t *m, uint32_t *c,
		   uint32_t len); /* c = x - y mod m provided (0 <= x < 2m) and (0 <= y < 2m). */
	int (*reduc)(struct pka_mod_adap *adp, uint32_t *x, uint32_t *m, uint32_t *c,
		     uint32_t len); /* c = x mod m provided (0 <= x < 2m). */
	int (*div)(struct pka_mod_adap *adp, uint32_t *x, uint32_t *y, uint32_t *m, uint32_t *c,
		   uint32_t len); /* c = y / x mod m provided (0 < x < m) and (0 < y < m) */
	int (*inv)(struct pka_mod_adap *adp, uint32_t *x, uint32_t *m, uint32_t *c,
		   uint32_t len); /* c = 1/x mod m provided (0 <= x < m). */
	int (*non_mod_mult)(struct pka_mod_adap *adp, uint32_t *a, uint32_t *b, uint32_t *c_lo, uint32_t *c_hi,
			    uint32_t len); /* c = a * b */
	int (*bit_seri_reduc)(struct pka_mod_adap *adp, uint32_t *x, uint32_t *m, uint32_t *c,
			      uint32_t len); /* c = x mod m provided (0 < x < 2^ceil(log2(m))) */
	int (*double_bit_seri_reduc)(struct pka_mod_adap *adp, uint32_t *x_lo, uint32_t *x_hi, uint32_t *m, uint32_t *c,
				     uint32_t len); /* c = x mod m provided (0 < x < 2^2*ceil(log2(m))) */
} pka_base_modul_adapter;

pka_base_modul_adapter *pka_base_modul_adapter_init(pka_base_radix_type radix_type);

void pka_base_mod_adapter_deinit(void);

int pka_init(void);

/************************************************************************************************
 * Q = kP provided Pxy is on the curve defined by a, b, and p, and (1 < k < order-of-base-point).
 * Computes the ECC Point Multiplication over the curve y^2 = x^3 + ax + b mod p.
 ************************************************************************************************/
int pka_ecc_point_mult(pka_ecc_param_info *ecc_parm, pka_piont *P, pka_piont *Q, pka_base_radix_type radix_type);

/************************************************************************************************
 * R = P + Q provided Pxy and Qxy are on the curve defined by a, b, and p, and Px ≠ ±Qx
 * Computes the ECC Point Addition over the curve y^2 = x^3 + ax + b mod p.
 ************************************************************************************************/
int pka_ecc_point_add(pka_ecc_param_info *ecc_parm, pka_piont *P, pka_piont *Q, pka_piont *R,
		      pka_base_radix_type radix_type);

/************************************************************************************************
 * Q = 2P provided Pxy is on the curve defined by a, b, and p
 * Computes the EC Point Doubling over the curve y^2 = x^3 + ax + b mod p
 ************************************************************************************************/
int pka_ecc_point_double(pka_ecc_param_info *ecc_parm, pka_piont *P, pka_piont *Q, pka_base_radix_type radix_type);

/************************************************************************************************
 * Computes y^2 == x^3 + ax + b mod p and verified LHS == RHS.
 * The result is left in the RET_CODE.ZERO field3.
 ************************************************************************************************/
int pka_ecc_piont_verification(pka_ecc_param_info *ecc_parm, pka_piont *P, pka_base_radix_type radix_type);

/**************************************************************************************************
 * R = kP + lQ provided Pxy and Qxy are on the curve defined by a, b, and p, Pxy ≠ ± Qxy,
 * and (1 < k < order-ofbase-point) or (1 < l < order-of-base-point).
 **************************************************************************************************/
int pka_ecc_shamirs_trick(pka_ecc_param_info *ecc_parm, pka_piont *P, pka_piont *Q, pka_piont *R,
			  pka_base_radix_type radix_type);

void pka_deinit(void);

#ifdef __cplusplus
}
#endif

#endif
