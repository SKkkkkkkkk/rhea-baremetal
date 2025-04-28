#ifndef __MEMMAP_H__
#define __MEMMAP_H__

#ifdef VIRT
	#include "_memmap_qemu_virt.h"
#else
	#include "_memmap_rhea.h"
#endif

#endif