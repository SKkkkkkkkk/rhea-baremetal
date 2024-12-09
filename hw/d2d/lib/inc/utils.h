/*
 * Copyright (c) 2024, M2Semi. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UTILS_H
#define UTILS_H

#include "types.h"

#define BIT_32(nr)			(U(1) << (nr))
#define BIT_64(nr)			(ULL(1) << (nr))

#if __riscv_xlen == 32
#define BIT				BIT_32
#else
#define BIT				BIT_64
#endif

/*
 * Create a contiguous bitmask starting at bit position @l and ending at
 * position @h. For example
 * GENMASK_64(39, 21) gives us the 64bit vector 0x000000ffffe00000.
 */
#if defined(__LINKER__) || defined(__ASSEMBLER__)
#define GENMASK_32(h, l) \
	(((0xFFFFFFFF) << (l)) & (0xFFFFFFFF >> (32 - 1 - (h))))

#define GENMASK_64(h, l) \
	((~0 << (l)) & (~0 >> (64 - 1 - (h))))
#else
#define GENMASK_32(h, l) \
	(((~UINT32_C(0)) << (l)) & (~UINT32_C(0) >> (32 - 1 - (h))))

#define GENMASK_64(h, l) \
	(((~UINT64_C(0)) << (l)) & (~UINT64_C(0) >> (64 - 1 - (h))))
#endif

#ifdef __aarch64__
#define GENMASK				GENMASK_64
#else
#define GENMASK				GENMASK_32
#endif

#define __bf_shf(x) (__builtin_ffsll(x) - 1)

#define FIELD_FIT(_mask, _val)						\
	({								\
		!((((typeof(_mask))_val) << __bf_shf(_mask)) & ~(_mask)); \
	})

#define FIELD_PREP(_mask, _val)						\
	({								\
		((typeof(_mask))(_val) << __bf_shf(_mask)) & (_mask);	\
	})

/**
 * FIELD_GET() - extract a bitfield element
 * @_mask: shifted mask defining the field's length and position
 * @_reg:  32bit value of entire bitfield
 *
 * FIELD_GET() extracts the field specified by @_mask from the
 * bitfield passed in as @_reg by masking and shifting it down.
 */
#define FIELD_GET(_mask, _reg)						\
	({								\
		(typeof(_mask))(((_reg) & (_mask)) >> __bf_shf(_mask));	\
	})

/*
 * This variant of div_round_up can be used in macro definition but should not
 * be used in C code as the `div` parameter is evaluated twice.
 */
#define DIV_ROUND_UP_2EVAL(n, d)	(((n) + (d)-1) / (d))

#define div_round_up(val, div)		__extension__({      \
	__typeof__(div) _div = (div);               \
	((val) + _div - (__typeof__(div))1) / _div; \
})

/*
 * Divide positive or negative dividend by positive or negative divisor
 * and round to closest integer. Result is undefined for negative
 * divisors if the dividend variable type is unsigned and for negative
 * dividends if the divisor variable type is unsigned.
 */
#define DIV_ROUND_CLOSEST(x, divisor)	(                   \
	{                                                 \
		typeof(x) __x = x;                        \
		typeof(divisor) __d = divisor;            \
		(((typeof(x)) - 1) > 0 ||                 \
		 ((typeof(divisor)) - 1) > 0 ||           \
		 (((__x) > 0) == ((__d) > 0))) ?          \
			      (((__x) + ((__d) / 2)) / (__d)) : \
			      (((__x) - ((__d) / 2)) / (__d));  \
	})

/*
 * The round_up() macro rounds up a value to the given boundary in a
 * type-agnostic yet type-safe manner. The boundary must be a power of two.
 * In other words, it computes the smallest multiple of boundary which is
 * greater than or equal to value.
 *
 * round_down() is similar but rounds the value down instead.
 */
#define round_boundary(value, boundary)	\
	((__typeof__(value))((boundary)-1))

#define round_up(value, boundary)	\
	((((value)-1) | round_boundary(value, boundary)) + 1)

#define round_down(value, boundary)	\
	((value) & ~round_boundary(value, boundary))

/*
 * Evaluates to 1 if (ptr + inc) overflows, 0 otherwise.
 * Both arguments must be unsigned pointer values (i.e. uintptr_t).
 */
#define check_uptr_overflow(_ptr, _inc)	\
	((_ptr) > (UINTPTR_MAX - (_inc)))

/*
 * Evaluates to 1 if (u32 + inc) overflows, 0 otherwise.
 * Both arguments must be 32-bit unsigned integers (i.e. effectively uint32_t).
 */
#define check_u32_overflow(_u32, _inc)	\
	((_u32) > (UINT32_MAX - (_inc)))

#define SIZE_FROM_LOG2_WORDS(n)		(4 << (n))

#define math_min(x, y)			((x) < (y) ? (x) : (y))
#define math_max(x, y)			((x) > (y) ? (x) : (y))

#define container_of(ptr, type, member)	({			\
	const typeof(((type *)0)->member) *__mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member)); })

/*
 * Ticks elapsed in one second with a signal of 1 MHz
 */
#define MHZ_TICKS_PER_SEC		(1000000)

/*
 * Ticks elapsed in one second with a signal of 1 KHz
 */
#define KHZ_TICKS_PER_SEC		(1000)

#define TIME_WAIT_FOREVER		(0xFFFFFFFF)

#define IS_ALIGNMENT(x, a)		(((x) & ((typeof(x))(a) - 1)) == 0)

#define ARRAY_SIZE(a)			(sizeof(a) / sizeof(a[0]))

#endif /* UTILS_H */
