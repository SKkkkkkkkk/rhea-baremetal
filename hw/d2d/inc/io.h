#ifndef __IO_H__
#define __IO_H__

#include <stdint.h>
#include <stdio.h>

static inline void writeb(uint8_t value, void *address)
{
	uintptr_t addr = (uintptr_t)address;

	*((volatile uint8_t *)(addr)) = value;
}

static inline uint8_t readb(void *address)
{
	uintptr_t addr = (uintptr_t)address;

	return *((volatile uint8_t *)(addr));
}

static inline void writew(uint16_t value, void *address)
{
	uintptr_t addr = (uintptr_t)address;

	*((volatile uint16_t *)(addr)) = value;
}

static inline uint16_t readw(void *address)
{
	uintptr_t addr = (uintptr_t)address;

	return *((volatile uint16_t *)(addr));
}

static inline void writel(uint32_t value, void *address)
{
	uintptr_t addr = (uintptr_t)address;

	*((volatile uint32_t *)(addr)) = value;
}

static inline uint32_t readl(void *address)
{
	uintptr_t addr = (uintptr_t)address;

	return *((volatile uint32_t *)(addr));
}

static inline void writeq(uint64_t value, void *address)
{
	uintptr_t addr = (uintptr_t)address;

	*((volatile uint64_t *)(addr)) = value;
}

static inline uint64_t readq(void *address)
{
	uintptr_t addr = (uintptr_t)address;

	return *((volatile uint64_t *)(addr));
}

#endif