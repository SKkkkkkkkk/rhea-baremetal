#ifndef __TYPES_H__
#define __TYPES_H__

#if defined(__LINKER__) || defined(__ASSEMBLY__) || defined(__ASSEMBLER__)
#define   U(_x)	(_x)
#define  UL(_x)	(_x)
#define ULL(_x)	(_x)
#define   L(_x)	(_x)
#define  LL(_x)	(_x)
#define _AC(X, Y) X
#define _AT(T, X) X
#else
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#define  U_(_x)	(_x##U)
#define   U(_x)	U_(_x)
#define  UL(_x)	(_x##UL)
#define ULL(_x)	(_x##ULL)
#define   L(_x)	(_x##L)
#define  LL(_x)	(_x##LL)
#define __AC(X, Y) (X##Y)
#define _AC(X, Y) __AC(X, Y)
#define _AT(T, X) ((T)(X))
#endif

#endif
