#ifndef __CIPHER_DEBUG_H__
#define __CIPHER_DEBUG_H__

#define cipher_parem_invalid()      printf("%s[%d] param Invalid !\n", __FUNCTION__, __LINE__)
#define cipher_pointer_invalid(ptr) printf("%s[%d] %s is null pointer !\n", __FUNCTION__, __LINE__, #ptr)
#define cipher_function_err(func)   printf("%s[%d] %s faild !\n", __FUNCTION__, __LINE__, #func)
#define cipher_check_chanel(id)     printf("%s[%d] channel id %d is nvalid !\n", __FUNCTION__, __LINE__, (id))
#define cipher_debug_err            printf
#define cipher_debug_info           printf
#define cipher_debug_dbg            printf

#define IRQ_SetPriority GIC_SetPriority
#define IRQ_Enable GIC_EnableIRQ
#endif
