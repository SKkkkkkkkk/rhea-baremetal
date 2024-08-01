# 添加app sources
set(app_src
	${CMAKE_CURRENT_LIST_DIR}/src/main.c
	${CMAKE_CURRENT_LIST_DIR}/src/cipher_test.c
	${CMAKE_CURRENT_LIST_DIR}/src/cipher_ecc_test.c
	${CMAKE_CURRENT_LIST_DIR}/src/ic_pka.c
	${CMAKE_CURRENT_LIST_DIR}/src/hash_test.c
	${CMAKE_CURRENT_LIST_DIR}/src/efuse_sec_test.c
	${CMAKE_CURRENT_LIST_DIR}/src/drivers/cipher.c
	${CMAKE_CURRENT_LIST_DIR}/src/drivers/cipher_ecc.c
	${CMAKE_CURRENT_LIST_DIR}/src/drivers/cipher_efuse.c
	${CMAKE_CURRENT_LIST_DIR}/src/drivers/cipher_hash.c
	${CMAKE_CURRENT_LIST_DIR}/src/drivers/efuse/efuse.c
	${CMAKE_CURRENT_LIST_DIR}/src/drivers/pka/pka.c
	${CMAKE_CURRENT_LIST_DIR}/src/drivers/spacc/spacc.c
)

# 添加app include dirs
set(app_inc
	${CMAKE_CURRENT_LIST_DIR}/inc
	${CMAKE_CURRENT_LIST_DIR}/inc/efuse
	${CMAKE_CURRENT_LIST_DIR}/inc/pka
	${CMAKE_CURRENT_LIST_DIR}/inc/spacc
)

set(app_dep
	dw_apb_timers
	systimer
)
