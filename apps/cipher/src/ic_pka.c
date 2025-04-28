#ifndef VIRT
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
// #include "pka.h"

#define GPASS
#define FAIL

#ifdef FPGA
#define TEST_PASS do { \
    printf("PASS\n"); \
    while(1) {} \
} while(0)
#else
#define TEST_PASS GPASS
#endif

#ifdef FPGA
#define TEST_FAIL do { \
    printf("FAIL\n"); \
    while(1) {} \
} while(0)
#else
#define TEST_FAIL FAIL
#endif

#define reg32(addr) (*(volatile uint32_t *)(uintptr_t)(addr))
void writel(int data,int addr) {reg32(addr)=data;}
int readl(int addr) {return reg32(addr);}

static void delay(unsigned int dly){/*{{{*/
    int i=0,j=0,k=0;
    for(i=0;i<dly;i++){
        for(j=0;j<10;j++){
            for(k=0;k<1000;k++);
        }
    }
}/*}}}*/


#define PKA_BASE    0x26020000
#define PKA_VERSION 0x804b4401

static void run(int start_code){/*{{{*/
  int addr;
  int stat=1;
  int error_cnt =0;
  //clear residula flag
  writel(0x00000000,PKA_BASE+0x0024);
  writel(0x00000000,PKA_BASE+0x0010);
  writel(start_code,PKA_BASE+0x0004);
  writel(0x80000000,PKA_BASE+0x0040);

//clear the stack pointer
  addr=PKA_BASE + 0x10 ;
  writel(0x0,addr);
  stat = readl(PKA_BASE+0x0020 );
  printf("run code:0x%x status %x\n",start_code,stat);

  //start
  //
  writel(0x80000200,PKA_BASE+0x0000);
//stat=0;

while(  1 )
{
  stat = readl(PKA_BASE+0x0020 );
  printf("run code:0x%x status %x\n",start_code,stat);

  if( ( (stat&0x400000000)!=0) ||( (stat&0x48000000)!=0) )  //done
    {
       writel(0x40000000,PKA_BASE+0x0020);//acknology the int
       break;
    }
   delay(300);
}

stat=readl(PKA_BASE + 0x0008);
if( (stat& 0x00FF0000 )!=0)
{
  error_cnt++;
  printf("verify STOP_REASON is NOT normal  , RTN_CODE stop_reason=%x hex\n",stat);
}else
{
  printf("verify STOP_REASON is  normal stop  ,RTN_CODE stop_reason=%x hex\n",stat);

}


//clear the stack pointer
addr=PKA_BASE + 0x10 ;
  writel(0x0,addr);

}/*}}}*/

static void load_data(int start_addr, int *value) {/*{{{*/
    for(int i=0;i<8; i++){
        writel(value[i],PKA_BASE+start_addr + i *4);
    }
    printf("end load data at %x\n",start_addr);
}/*}}}*/

static int  compare_result(int start_Addr,int *result){/*{{{*/
    int value;
    int err = 0;
    for(int i =0; i<8; i++){
        value = readl(PKA_BASE+start_Addr+ i*4);
        if(value != result[i]){
            printf("read value : %x  result value:%x\n",value,result[i]);
            err =1;

        }
    }
    return err;
}/*}}}*/

static void  get_data(int start_Addr,int *result){/*{{{*/
    int value;
    for(int i =0; i<8; i++){
        value = readl(PKA_BASE+start_Addr+ i*4);
        result[i] = value;
        printf("read value : %x  \n",value);


    }
}/*}}}*/

#define PKA_FW_MEM_BASE (PKA_BASE +0x4000)
static void JC3_pka_fw_init()/*{{{*/
{
   int addr;

// data from xsy 190606 in data_exchange
// -------------------------------------------------
// Copyright (C) 2009-2017, SYNOPSYS, INC., ALL RIGHTS RESERVED
//
// ------------------------------------------------------------------------
//   This software and the associated documentation are confidential and
//   proprietary to Synopsys, Inc.  Your use or disclosure of this
//   software is subject to the terms and conditions of a written
//   license agreement between you, or your company, and Synopsys, Inc.
//
//   The entire notice above must be reproduced on all authorized copies.
// ------------------------------------------------------------------------
//
// PKA F/W -- $readmemh() style
//
// Source file:           //dwh/security_ip_hw/dev/branches/pka/elp_pka_2_10_dev/hw/clue/src/clp300_ram_fw.casm
// Source revision:       #1
// Source tag:
//
// Assembler:
// Assembler revision:    #3
// Assembler tag:
//
// Origin:            0x0
// Length:            0x3ec (1004)
// Highest address:   0x3ec (1004)
// MD5 Sum:           FDF3BC2450F35E09695D164816A10D6F (covers address 0x9 to the end)
//
addr=PKA_FW_MEM_BASE + 0x0  ; writel( 0xf8000009 ,addr);
addr=PKA_FW_MEM_BASE + 0x4  ; writel( 0xf8e65044 ,addr);
addr=PKA_FW_MEM_BASE + 0x8  ; writel( 0xf8a10d6f ,addr);
addr=PKA_FW_MEM_BASE + 0xC  ; writel( 0xf8164816 ,addr);
addr=PKA_FW_MEM_BASE + 0x10 ; writel( 0xf809695d ,addr);
addr=PKA_FW_MEM_BASE + 0x14 ; writel( 0xf850f35e ,addr);
addr=PKA_FW_MEM_BASE + 0x18 ; writel( 0xf8f3bc24 ,addr);
addr=PKA_FW_MEM_BASE + 0x1C ; writel( 0xf80000fd ,addr);
addr=PKA_FW_MEM_BASE + 0x20 ; writel( 0xf80003e3 ,addr);
addr=PKA_FW_MEM_BASE + 0x24 ; writel( 0x000000ff ,addr);
addr=PKA_FW_MEM_BASE + 0x28 ; writel( 0x20000024 ,addr);
addr=PKA_FW_MEM_BASE + 0x2C ; writel( 0x20000026 ,addr);
addr=PKA_FW_MEM_BASE + 0x30 ; writel( 0x20000028 ,addr);
addr=PKA_FW_MEM_BASE + 0x34 ; writel( 0x2000002a ,addr);
addr=PKA_FW_MEM_BASE + 0x38 ; writel( 0x2000002c ,addr);
addr=PKA_FW_MEM_BASE + 0x3C ; writel( 0x2000002e ,addr);
addr=PKA_FW_MEM_BASE + 0x40 ; writel( 0x20000030 ,addr);
addr=PKA_FW_MEM_BASE + 0x44 ; writel( 0x20000032 ,addr);
addr=PKA_FW_MEM_BASE + 0x48 ; writel( 0x20000034 ,addr);
addr=PKA_FW_MEM_BASE + 0x4C ; writel( 0x20000036 ,addr);
addr=PKA_FW_MEM_BASE + 0x50 ; writel( 0x20000038 ,addr);
addr=PKA_FW_MEM_BASE + 0x54 ; writel( 0x2000003a ,addr);
addr=PKA_FW_MEM_BASE + 0x58 ; writel( 0x2000014c ,addr);
addr=PKA_FW_MEM_BASE + 0x5C ; writel( 0x2000014e ,addr);
addr=PKA_FW_MEM_BASE + 0x60 ; writel( 0x20000150 ,addr);
addr=PKA_FW_MEM_BASE + 0x64 ; writel( 0x200001b2 ,addr);
addr=PKA_FW_MEM_BASE + 0x68 ; writel( 0x200001b4 ,addr);
addr=PKA_FW_MEM_BASE + 0x6C ; writel( 0x200001b6 ,addr);
addr=PKA_FW_MEM_BASE + 0x70 ; writel( 0x200001b8 ,addr);
addr=PKA_FW_MEM_BASE + 0x74 ; writel( 0x200001ba ,addr);
addr=PKA_FW_MEM_BASE + 0x78 ; writel( 0x200001bc ,addr);
addr=PKA_FW_MEM_BASE + 0x7C ; writel( 0x200001be ,addr);
addr=PKA_FW_MEM_BASE + 0x80 ; writel( 0x200001c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x84 ; writel( 0x200001c2 ,addr);
addr=PKA_FW_MEM_BASE + 0x88 ; writel( 0x200001c4 ,addr);
addr=PKA_FW_MEM_BASE + 0x8C ; writel( 0x20000387 ,addr);
addr=PKA_FW_MEM_BASE + 0x90 ; writel( 0x220000a1 ,addr);
addr=PKA_FW_MEM_BASE + 0x94 ; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x98 ; writel( 0x2200008d ,addr);
addr=PKA_FW_MEM_BASE + 0x9C ; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xA0 ; writel( 0x22000096 ,addr);
addr=PKA_FW_MEM_BASE + 0xA4 ; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xA8 ; writel( 0x2200003c ,addr);
addr=PKA_FW_MEM_BASE + 0xAC ; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xB0 ; writel( 0x2200003e ,addr);
addr=PKA_FW_MEM_BASE + 0xB4 ; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xB8 ; writel( 0x220000b9 ,addr);
addr=PKA_FW_MEM_BASE + 0xBC ; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xC0 ; writel( 0x220000e6 ,addr);
addr=PKA_FW_MEM_BASE + 0xC4 ; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xC8 ; writel( 0x220000e1 ,addr);
addr=PKA_FW_MEM_BASE + 0xCC ; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xD0 ; writel( 0x220000d9 ,addr);
addr=PKA_FW_MEM_BASE + 0xD4 ; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xD8 ; writel( 0x22000108 ,addr);
addr=PKA_FW_MEM_BASE + 0xDC ; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xE0 ; writel( 0x2200013a ,addr);
addr=PKA_FW_MEM_BASE + 0xE4 ; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xE8 ; writel( 0xc8004400 ,addr);
addr=PKA_FW_MEM_BASE + 0xEC ; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xF0 ; writel( 0x48200000 ,addr);
addr=PKA_FW_MEM_BASE + 0xF4 ; writel( 0x20000040 ,addr);
addr=PKA_FW_MEM_BASE + 0xF8 ; writel( 0x48200000 ,addr);
addr=PKA_FW_MEM_BASE + 0xFC ; writel( 0x44000400 ,addr);
addr=PKA_FW_MEM_BASE + 0x100   ; writel( 0x40008200 ,addr);
addr=PKA_FW_MEM_BASE + 0x104   ; writel( 0x220000d2 ,addr);
addr=PKA_FW_MEM_BASE + 0x108   ; writel( 0x40004400 ,addr);
addr=PKA_FW_MEM_BASE + 0x10C   ; writel( 0x08c00000 ,addr);
addr=PKA_FW_MEM_BASE + 0x110 ; writel( 0x43000680 ,addr);
addr=PKA_FW_MEM_BASE + 0x114 ; writel( 0x4000c200 ,addr);
addr=PKA_FW_MEM_BASE + 0x118 ; writel( 0x23c0004d ,addr);
addr=PKA_FW_MEM_BASE + 0x11C ; writel( 0xd9004000 ,addr);
addr=PKA_FW_MEM_BASE + 0x120 ; writel( 0x33000084 ,addr);
addr=PKA_FW_MEM_BASE + 0x124 ; writel( 0x6b000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x128 ; writel( 0x33600055 ,addr);
addr=PKA_FW_MEM_BASE + 0x12C ; writel( 0x41000440 ,addr);
addr=PKA_FW_MEM_BASE + 0x130 ; writel( 0x40008800 ,addr);
addr=PKA_FW_MEM_BASE + 0x134 ; writel( 0x6b008000 ,addr);
addr=PKA_FW_MEM_BASE + 0x138 ; writel( 0x33600052 ,addr);
addr=PKA_FW_MEM_BASE + 0x13C ; writel( 0x41008040 ,addr);
addr=PKA_FW_MEM_BASE + 0x140 ; writel( 0x40000c00 ,addr);
addr=PKA_FW_MEM_BASE + 0x144 ; writel( 0x20000047 ,addr);
addr=PKA_FW_MEM_BASE + 0x148 ; writel( 0xd010c040 ,addr);
addr=PKA_FW_MEM_BASE + 0x14C ; writel( 0x41000c00 ,addr);
addr=PKA_FW_MEM_BASE + 0x150 ; writel( 0x20000047 ,addr);
addr=PKA_FW_MEM_BASE + 0x154 ; writel( 0x6b004000 ,addr);
addr=PKA_FW_MEM_BASE + 0x158 ; writel( 0x33600062 ,addr);
addr=PKA_FW_MEM_BASE + 0x15C ; writel( 0x41004440 ,addr);
addr=PKA_FW_MEM_BASE + 0x160 ; writel( 0x40008a00 ,addr);
addr=PKA_FW_MEM_BASE + 0x164 ; writel( 0x6b00d000 ,addr);
addr=PKA_FW_MEM_BASE + 0x168 ; writel( 0x3360005e ,addr);
addr=PKA_FW_MEM_BASE + 0x16C ; writel( 0x4100d440 ,addr);
addr=PKA_FW_MEM_BASE + 0x170 ; writel( 0x40008e80 ,addr);
addr=PKA_FW_MEM_BASE + 0x174 ; writel( 0x20000047 ,addr);
addr=PKA_FW_MEM_BASE + 0x178 ; writel( 0x4000d440 ,addr);
addr=PKA_FW_MEM_BASE + 0x17C ; writel( 0xd011c040 ,addr);
addr=PKA_FW_MEM_BASE + 0x180 ; writel( 0x41000e80 ,addr);
addr=PKA_FW_MEM_BASE + 0x184 ; writel( 0x20000047 ,addr);
addr=PKA_FW_MEM_BASE + 0x188 ; writel( 0xd8004440 ,addr);
addr=PKA_FW_MEM_BASE + 0x18C ; writel( 0x33400073 ,addr);
addr=PKA_FW_MEM_BASE + 0x190 ; writel( 0x41008800 ,addr);
addr=PKA_FW_MEM_BASE + 0x194 ; writel( 0xd810d040 ,addr);
addr=PKA_FW_MEM_BASE + 0x198 ; writel( 0x40000c00 ,addr);
addr=PKA_FW_MEM_BASE + 0x19C ; writel( 0x3140006a ,addr);
addr=PKA_FW_MEM_BASE + 0x1A0 ; writel( 0xd010c040 ,addr);
addr=PKA_FW_MEM_BASE + 0x1A4 ; writel( 0x40000c00 ,addr);
addr=PKA_FW_MEM_BASE + 0x1A8 ; writel( 0x6b008000 ,addr);
addr=PKA_FW_MEM_BASE + 0x1AC ; writel( 0x33600070 ,addr);
addr=PKA_FW_MEM_BASE + 0x1B0 ; writel( 0x48200000 ,addr);
addr=PKA_FW_MEM_BASE + 0x1B4 ; writel( 0x41008040 ,addr);
addr=PKA_FW_MEM_BASE + 0x1B8 ; writel( 0x40000c00 ,addr);
addr=PKA_FW_MEM_BASE + 0x1BC ; writel( 0x20000047 ,addr);
addr=PKA_FW_MEM_BASE + 0x1C0 ; writel( 0xd010c040 ,addr);
addr=PKA_FW_MEM_BASE + 0x1C4 ; writel( 0x41000c00 ,addr);
addr=PKA_FW_MEM_BASE + 0x1C8 ; writel( 0x20000047 ,addr);
addr=PKA_FW_MEM_BASE + 0x1CC ; writel( 0xd8080440 ,addr);
addr=PKA_FW_MEM_BASE + 0x1D0 ; writel( 0x41008a00 ,addr);
addr=PKA_FW_MEM_BASE + 0x1D4 ; writel( 0xd81a8040 ,addr);
addr=PKA_FW_MEM_BASE + 0x1D8 ; writel( 0x40000e80 ,addr);
addr=PKA_FW_MEM_BASE + 0x1DC ; writel( 0x3140007a ,addr);
addr=PKA_FW_MEM_BASE + 0x1E0 ; writel( 0xd001c440 ,addr);
addr=PKA_FW_MEM_BASE + 0x1E4 ; writel( 0x40008e80 ,addr);
addr=PKA_FW_MEM_BASE + 0x1E8 ; writel( 0x6b00d000 ,addr);
addr=PKA_FW_MEM_BASE + 0x1EC ; writel( 0x33600080 ,addr);
addr=PKA_FW_MEM_BASE + 0x1F0  ; writel( 0x48200000 ,addr);
addr=PKA_FW_MEM_BASE + 0x1F4  ; writel( 0x4100d440 ,addr);
addr=PKA_FW_MEM_BASE + 0x1F8  ; writel( 0x40008e80 ,addr);
addr=PKA_FW_MEM_BASE + 0x1FC  ; writel( 0x20000047 ,addr);
addr=PKA_FW_MEM_BASE + 0x200 ; writel( 0x4000d440 ,addr);
addr=PKA_FW_MEM_BASE + 0x204 ; writel( 0xd011c040 ,addr);
addr=PKA_FW_MEM_BASE + 0x208 ; writel( 0x41000e80 ,addr);
addr=PKA_FW_MEM_BASE + 0x20C ; writel( 0x20000047 ,addr);
addr=PKA_FW_MEM_BASE + 0x210 ; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x214 ; writel( 0x33400089 ,addr);
addr=PKA_FW_MEM_BASE + 0x218 ; writel( 0xd010c240 ,addr);
addr=PKA_FW_MEM_BASE + 0x21C ; writel( 0x40008000 ,addr);
addr=PKA_FW_MEM_BASE + 0x220 ; writel( 0x2000008c ,addr);
addr=PKA_FW_MEM_BASE + 0x224 ; writel( 0xd010c000 ,addr);
addr=PKA_FW_MEM_BASE + 0x228 ; writel( 0x40008240 ,addr);
addr=PKA_FW_MEM_BASE + 0x22C ; writel( 0x2000008c ,addr);
addr=PKA_FW_MEM_BASE + 0x230 ; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x234 ; writel( 0x48400000 ,addr);
addr=PKA_FW_MEM_BASE + 0x238 ; writel( 0x48200000 ,addr);
addr=PKA_FW_MEM_BASE + 0x23C ; writel( 0x220000d2 ,addr);
addr=PKA_FW_MEM_BASE + 0x240 ; writel( 0xd0004400 ,addr);
addr=PKA_FW_MEM_BASE + 0x244 ; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0x248 ; writel( 0xd800c400 ,addr);
addr=PKA_FW_MEM_BASE + 0x24C ; writel( 0x31400091 ,addr);
addr=PKA_FW_MEM_BASE + 0x250 ; writel( 0x20000095 ,addr);
addr=PKA_FW_MEM_BASE + 0x254 ; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x258 ; writel( 0x48200000 ,addr);
addr=PKA_FW_MEM_BASE + 0x25C ; writel( 0x220000d2 ,addr);
addr=PKA_FW_MEM_BASE + 0x260 ; writel( 0x220000c9 ,addr);
addr=PKA_FW_MEM_BASE + 0x264 ; writel( 0x3340009d ,addr);
addr=PKA_FW_MEM_BASE + 0x268 ; writel( 0xd8188040 ,addr);
addr=PKA_FW_MEM_BASE + 0x26C ; writel( 0x40008000 ,addr);
addr=PKA_FW_MEM_BASE + 0x270 ; writel( 0x200000a0 ,addr);
addr=PKA_FW_MEM_BASE + 0x274 ; writel( 0xd8188000 ,addr);
addr=PKA_FW_MEM_BASE + 0x278 ; writel( 0x40008040 ,addr);
addr=PKA_FW_MEM_BASE + 0x27C ; writel( 0x200000a0 ,addr);
addr=PKA_FW_MEM_BASE + 0x280 ; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x284 ; writel( 0x40004680 ,addr);
addr=PKA_FW_MEM_BASE + 0x288 ; writel( 0x220000ab ,addr);
addr=PKA_FW_MEM_BASE + 0x28C ; writel( 0x40000400 ,addr);
addr=PKA_FW_MEM_BASE + 0x290 ; writel( 0x4000d000 ,addr);
addr=PKA_FW_MEM_BASE + 0x294 ; writel( 0x40008680 ,addr);
addr=PKA_FW_MEM_BASE + 0x298 ; writel( 0x220000ab ,addr);
addr=PKA_FW_MEM_BASE + 0x29C ; writel( 0x4000d200 ,addr);
addr=PKA_FW_MEM_BASE + 0x2A0 ; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0x2A4 ; writel( 0x220000ae ,addr);
addr=PKA_FW_MEM_BASE + 0x2A8 ; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x2AC ; writel( 0x40000200 ,addr);
addr=PKA_FW_MEM_BASE + 0x2B0 ; writel( 0x4000d800 ,addr);
addr=PKA_FW_MEM_BASE + 0x2B4 ; writel( 0x200000af ,addr);
addr=PKA_FW_MEM_BASE + 0x2B8 ; writel( 0x44000200 ,addr);
addr=PKA_FW_MEM_BASE + 0x2BC ; writel( 0xc8004400 ,addr);
addr=PKA_FW_MEM_BASE + 0x2C0 ; writel( 0xc910ca00 ,addr);
addr=PKA_FW_MEM_BASE + 0x2C4 ; writel( 0xc808c000 ,addr);
addr=PKA_FW_MEM_BASE + 0x2C8 ; writel( 0xd0008200 ,addr);
addr=PKA_FW_MEM_BASE + 0x2CC ; writel( 0xd4018a00 ,addr);
addr=PKA_FW_MEM_BASE + 0x2D0 ; writel( 0x40004400 ,addr);
addr=PKA_FW_MEM_BASE + 0x2D4 ; writel( 0x200000ba ,addr);
addr=PKA_FW_MEM_BASE + 0x2D8 ; writel( 0x40000200 ,addr);
addr=PKA_FW_MEM_BASE + 0x2DC ; writel( 0xca004400 ,addr);
addr=PKA_FW_MEM_BASE + 0x2E0 ; writel( 0x200000b0 ,addr);
addr=PKA_FW_MEM_BASE + 0x2E4 ; writel( 0x48200000 ,addr);
addr=PKA_FW_MEM_BASE + 0x2E8 ; writel( 0x332000be ,addr);
addr=PKA_FW_MEM_BASE + 0x2EC ; writel( 0xd810c040 ,addr);
addr=PKA_FW_MEM_BASE + 0x2F0 ; writel( 0x40008040 ,addr);
addr=PKA_FW_MEM_BASE + 0x2F4 ; writel( 0x200000c1 ,addr);
addr=PKA_FW_MEM_BASE + 0x2F8 ; writel( 0xd810c040 ,addr);
addr=PKA_FW_MEM_BASE + 0x2FC ; writel( 0x40000c00 ,addr);
addr=PKA_FW_MEM_BASE + 0x300; writel( 0x200000c1 ,addr);
addr=PKA_FW_MEM_BASE + 0x304; writel( 0x40008000 ,addr);
addr=PKA_FW_MEM_BASE + 0x308; writel( 0xd800c400 ,addr);
addr=PKA_FW_MEM_BASE + 0x30C; writel( 0x334000c6 ,addr);
addr=PKA_FW_MEM_BASE + 0x310; writel( 0x40008000 ,addr);
addr=PKA_FW_MEM_BASE + 0x314; writel( 0x200000c8 ,addr);
addr=PKA_FW_MEM_BASE + 0x318; writel( 0x40000400 ,addr);
addr=PKA_FW_MEM_BASE + 0x31C; writel( 0x200000c8 ,addr);
addr=PKA_FW_MEM_BASE + 0x320; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x324; writel( 0xd8004400 ,addr);
addr=PKA_FW_MEM_BASE + 0x328; writel( 0x334000ce ,addr);
addr=PKA_FW_MEM_BASE + 0x32C; writel( 0xd9004000 ,addr);
addr=PKA_FW_MEM_BASE + 0x330; writel( 0x48400000 ,addr);
addr=PKA_FW_MEM_BASE + 0x334; writel( 0x200000d1 ,addr);
addr=PKA_FW_MEM_BASE + 0x338; writel( 0xd8080400 ,addr);
addr=PKA_FW_MEM_BASE + 0x33C; writel( 0x49400000 ,addr);
addr=PKA_FW_MEM_BASE + 0x340; writel( 0x200000d1 ,addr);
addr=PKA_FW_MEM_BASE + 0x344; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x348; writel( 0x40000680 ,addr);
addr=PKA_FW_MEM_BASE + 0x34C; writel( 0x40004400 ,addr);
addr=PKA_FW_MEM_BASE + 0x350; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0x354; writel( 0x40000200 ,addr);
addr=PKA_FW_MEM_BASE + 0x358; writel( 0x48200000 ,addr);
addr=PKA_FW_MEM_BASE + 0x35C; writel( 0x4000d400 ,addr);
addr=PKA_FW_MEM_BASE + 0x360; writel( 0x200000ba ,addr);
addr=PKA_FW_MEM_BASE + 0x364; writel( 0x08c00000 ,addr);
addr=PKA_FW_MEM_BASE + 0x368; writel( 0x40008240 ,addr);
addr=PKA_FW_MEM_BASE + 0x36C; writel( 0x43000200 ,addr);
addr=PKA_FW_MEM_BASE + 0x370; writel( 0xd808c400 ,addr);
addr=PKA_FW_MEM_BASE + 0x374; writel( 0x40004800 ,addr);
addr=PKA_FW_MEM_BASE + 0x378; writel( 0x22000044 ,addr);
addr=PKA_FW_MEM_BASE + 0x37C; writel( 0x400086c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x380; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x384; writel( 0x08c00001 ,addr);
addr=PKA_FW_MEM_BASE + 0x388; writel( 0x45000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x38C; writel( 0x44000400 ,addr);
addr=PKA_FW_MEM_BASE + 0x390; writel( 0x22000044 ,addr);
addr=PKA_FW_MEM_BASE + 0x394; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x398; writel( 0x40008680 ,addr);
addr=PKA_FW_MEM_BASE + 0x39C; writel( 0x43000200 ,addr);
addr=PKA_FW_MEM_BASE + 0x3A0; writel( 0x43000640 ,addr);
addr=PKA_FW_MEM_BASE + 0x3A4; writel( 0x43000040 ,addr);
addr=PKA_FW_MEM_BASE + 0x3A8; writel( 0x44000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x3AC; writel( 0x4100c240 ,addr);
addr=PKA_FW_MEM_BASE + 0x3B0; writel( 0x5180c000 ,addr);
addr=PKA_FW_MEM_BASE + 0x3B4; writel( 0x6a804000 ,addr);
addr=PKA_FW_MEM_BASE + 0x3B8; writel( 0x08c00000 ,addr);
addr=PKA_FW_MEM_BASE + 0x3BC; writel( 0xd0004400 ,addr);
addr=PKA_FW_MEM_BASE + 0x3C0; writel( 0xd4014c40 ,addr);
addr=PKA_FW_MEM_BASE + 0x3C4; writel( 0xd91a8800 ,addr);
addr=PKA_FW_MEM_BASE + 0x3C8; writel( 0x334000f7 ,addr);
addr=PKA_FW_MEM_BASE + 0x3CC; writel( 0x33000103 ,addr);
addr=PKA_FW_MEM_BASE + 0x3D0; writel( 0x40008000 ,addr);
addr=PKA_FW_MEM_BASE + 0x3D4; writel( 0x40008840 ,addr);
addr=PKA_FW_MEM_BASE + 0x3D8; writel( 0x6a80c800 ,addr);
addr=PKA_FW_MEM_BASE + 0x3DC; writel( 0x08c00001 ,addr);
addr=PKA_FW_MEM_BASE + 0x3E0; writel( 0x6b004800 ,addr);
addr=PKA_FW_MEM_BASE + 0x3E4; writel( 0x336000fb ,addr);
addr=PKA_FW_MEM_BASE + 0x3E8; writel( 0x08c00000 ,addr);
addr=PKA_FW_MEM_BASE + 0x3EC; writel( 0x41004400 ,addr);
addr=PKA_FW_MEM_BASE + 0x3F0; writel( 0x41004c40 ,addr);
addr=PKA_FW_MEM_BASE + 0x3F4; writel( 0x40008200 ,addr);
addr=PKA_FW_MEM_BASE + 0x3F8; writel( 0x40008a40 ,addr);
addr=PKA_FW_MEM_BASE + 0x3FC; writel( 0x21c00101 ,addr);
addr=PKA_FW_MEM_BASE + 0x400; writel( 0x6d204000 ,addr);
addr=PKA_FW_MEM_BASE + 0x404; writel( 0x11800000 ,addr);
addr=PKA_FW_MEM_BASE + 0x408; writel( 0x200000ef ,addr);
addr=PKA_FW_MEM_BASE + 0x40C; writel( 0x51c08000 ,addr);
addr=PKA_FW_MEM_BASE + 0x410; writel( 0x310000f7 ,addr);
addr=PKA_FW_MEM_BASE + 0x414; writel( 0x6a80c800 ,addr);
addr=PKA_FW_MEM_BASE + 0x418; writel( 0x4000d400 ,addr);
addr=PKA_FW_MEM_BASE + 0x41C; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x420; writel( 0x51808800 ,addr);
addr=PKA_FW_MEM_BASE + 0x424; writel( 0x3100010c ,addr);
addr=PKA_FW_MEM_BASE + 0x428; writel( 0xd910c000 ,addr);
addr=PKA_FW_MEM_BASE + 0x42C; writel( 0x33400139 ,addr);
addr=PKA_FW_MEM_BASE + 0x430; writel( 0x51c08800 ,addr);
addr=PKA_FW_MEM_BASE + 0x434; writel( 0x31000110 ,addr);
addr=PKA_FW_MEM_BASE + 0x438; writel( 0x51a08000 ,addr);
addr=PKA_FW_MEM_BASE + 0x43C; writel( 0x20000112 ,addr);
addr=PKA_FW_MEM_BASE + 0x440; writel( 0x09e00000 ,addr);
addr=PKA_FW_MEM_BASE + 0x444; writel( 0x14bb8000 ,addr);
addr=PKA_FW_MEM_BASE + 0x448; writel( 0x5180c000 ,addr);
addr=PKA_FW_MEM_BASE + 0x44C; writel( 0x15d60000 ,addr);
addr=PKA_FW_MEM_BASE + 0x450; writel( 0x09800000 ,addr);
addr=PKA_FW_MEM_BASE + 0x454; writel( 0x15930000 ,addr);
addr=PKA_FW_MEM_BASE + 0x458; writel( 0x33000118 ,addr);
addr=PKA_FW_MEM_BASE + 0x45C; writel( 0x31400120 ,addr);
addr=PKA_FW_MEM_BASE + 0x460; writel( 0x09800000 ,addr);
addr=PKA_FW_MEM_BASE + 0x464; writel( 0x159a0000 ,addr);
addr=PKA_FW_MEM_BASE + 0x468; writel( 0x43000200 ,addr);
addr=PKA_FW_MEM_BASE + 0x46C; writel( 0x6a804000 ,addr);
addr=PKA_FW_MEM_BASE + 0x470; writel( 0xc808c000 ,addr);
addr=PKA_FW_MEM_BASE + 0x474; writel( 0x40000640 ,addr);
addr=PKA_FW_MEM_BASE + 0x478; writel( 0x43000600 ,addr);
addr=PKA_FW_MEM_BASE + 0x47C; writel( 0x20000126 ,addr);
addr=PKA_FW_MEM_BASE + 0x480; writel( 0x43000200 ,addr);
addr=PKA_FW_MEM_BASE + 0x484; writel( 0x6ac04000 ,addr);
addr=PKA_FW_MEM_BASE + 0x488; writel( 0xc808c000 ,addr);
addr=PKA_FW_MEM_BASE + 0x48C; writel( 0x40000600 ,addr);
addr=PKA_FW_MEM_BASE + 0x490; writel( 0x40000e40 ,addr);
addr=PKA_FW_MEM_BASE + 0x494; writel( 0x20000126 ,addr);
addr=PKA_FW_MEM_BASE + 0x498; writel( 0x10c00000 ,addr);
addr=PKA_FW_MEM_BASE + 0x49C; writel( 0xd810c000 ,addr);
addr=PKA_FW_MEM_BASE + 0x4A0; writel( 0xda11c840 ,addr);
addr=PKA_FW_MEM_BASE + 0x4A4; writel( 0x3340012d ,addr);
addr=PKA_FW_MEM_BASE + 0x4A8; writel( 0x40000400 ,addr);
addr=PKA_FW_MEM_BASE + 0x4AC; writel( 0x40000c40 ,addr);
addr=PKA_FW_MEM_BASE + 0x4B0; writel( 0x20000127 ,addr);
addr=PKA_FW_MEM_BASE + 0x4B4; writel( 0x48200000 ,addr);
addr=PKA_FW_MEM_BASE + 0x4B8; writel( 0x48c00000 ,addr);
addr=PKA_FW_MEM_BASE + 0x4BC; writel( 0x6b00c800 ,addr);
addr=PKA_FW_MEM_BASE + 0x4C0; writel( 0x31600132 ,addr);
addr=PKA_FW_MEM_BASE + 0x4C4; writel( 0x49c00000 ,addr);
addr=PKA_FW_MEM_BASE + 0x4C8; writel( 0x4100c840 ,addr);
addr=PKA_FW_MEM_BASE + 0x4CC; writel( 0x31c00135 ,addr);
addr=PKA_FW_MEM_BASE + 0x4D0; writel( 0x49200000 ,addr);
addr=PKA_FW_MEM_BASE + 0x4D4; writel( 0x4100c000 ,addr);
addr=PKA_FW_MEM_BASE + 0x4D8; writel( 0x40000e40 ,addr);
addr=PKA_FW_MEM_BASE + 0x4DC; writel( 0x40000600 ,addr);
addr=PKA_FW_MEM_BASE + 0x4E0; writel( 0x25c00127 ,addr);
addr=PKA_FW_MEM_BASE + 0x4E4; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x4E8; writel( 0xd9188000 ,addr);
addr=PKA_FW_MEM_BASE + 0x4EC; writel( 0x3140014b ,addr);
addr=PKA_FW_MEM_BASE + 0x4F0; writel( 0x51a08000 ,addr);
addr=PKA_FW_MEM_BASE + 0x4F4; writel( 0x5180c000 ,addr);
addr=PKA_FW_MEM_BASE + 0x4F8; writel( 0x15d60000 ,addr);
addr=PKA_FW_MEM_BASE + 0x4FC; writel( 0x43000200 ,addr);
addr=PKA_FW_MEM_BASE + 0x500; writel( 0x6ac04000 ,addr);
addr=PKA_FW_MEM_BASE + 0x504; writel( 0xc808c000 ,addr);
addr=PKA_FW_MEM_BASE + 0x508; writel( 0x40000600 ,addr);
addr=PKA_FW_MEM_BASE + 0x50C; writel( 0x10c00000 ,addr);
addr=PKA_FW_MEM_BASE + 0x510; writel( 0xd810c000 ,addr);
addr=PKA_FW_MEM_BASE + 0x514; writel( 0x33400148 ,addr);
addr=PKA_FW_MEM_BASE + 0x518; writel( 0x40000400 ,addr);
addr=PKA_FW_MEM_BASE + 0x51C; writel( 0x20000144 ,addr);
addr=PKA_FW_MEM_BASE + 0x520; writel( 0x4100c000 ,addr);
addr=PKA_FW_MEM_BASE + 0x524; writel( 0x40000600 ,addr);
addr=PKA_FW_MEM_BASE + 0x528; writel( 0x25c00144 ,addr);
addr=PKA_FW_MEM_BASE + 0x52C; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x530; writel( 0x22000152 ,addr);
addr=PKA_FW_MEM_BASE + 0x534; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x538; writel( 0x22000171 ,addr);
addr=PKA_FW_MEM_BASE + 0x53C; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x540; writel( 0x22000184 ,addr);
addr=PKA_FW_MEM_BASE + 0x544; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x548; writel( 0x220000ab ,addr);
addr=PKA_FW_MEM_BASE + 0x54C; writel( 0x4000da00 ,addr);
addr=PKA_FW_MEM_BASE + 0x550; writel( 0x400006c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x554; writel( 0x44000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x558; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0x55C; writel( 0x3180015b ,addr);
addr=PKA_FW_MEM_BASE + 0x560; writel( 0x09800000 ,addr);
addr=PKA_FW_MEM_BASE + 0x564; writel( 0x11800000 ,addr);
addr=PKA_FW_MEM_BASE + 0x568; writel( 0x2000015f ,addr);
addr=PKA_FW_MEM_BASE + 0x56C; writel( 0x5180d000 ,addr);
addr=PKA_FW_MEM_BASE + 0x570; writel( 0x3100015f ,addr);
addr=PKA_FW_MEM_BASE + 0x574; writel( 0x44000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x578; writel( 0x20000170 ,addr);
addr=PKA_FW_MEM_BASE + 0x57C; writel( 0x220000b6 ,addr);
addr=PKA_FW_MEM_BASE + 0x580; writel( 0x6880d000 ,addr);
addr=PKA_FW_MEM_BASE + 0x584; writel( 0x31600168 ,addr);
addr=PKA_FW_MEM_BASE + 0x588; writel( 0x20000163 ,addr);
addr=PKA_FW_MEM_BASE + 0x58C; writel( 0x4000da00 ,addr);
addr=PKA_FW_MEM_BASE + 0x590; writel( 0x40000240 ,addr);
addr=PKA_FW_MEM_BASE + 0x594; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0x598; writel( 0x40004840 ,addr);
addr=PKA_FW_MEM_BASE + 0x59C; writel( 0x2000016e ,addr);
addr=PKA_FW_MEM_BASE + 0x5A0; writel( 0x2620016e ,addr);
addr=PKA_FW_MEM_BASE + 0x5A4; writel( 0x4000da00 ,addr);
addr=PKA_FW_MEM_BASE + 0x5A8; writel( 0x40000240 ,addr);
addr=PKA_FW_MEM_BASE + 0x5AC; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0x5B0; writel( 0x40004800 ,addr);
addr=PKA_FW_MEM_BASE + 0x5B4; writel( 0x2000016e ,addr);
addr=PKA_FW_MEM_BASE + 0x5B8; writel( 0x2880015f ,addr);
addr=PKA_FW_MEM_BASE + 0x5BC; writel( 0x220000ae ,addr);
addr=PKA_FW_MEM_BASE + 0x5C0; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x5C4; writel( 0x44000400 ,addr);
addr=PKA_FW_MEM_BASE + 0x5C8; writel( 0x40004800 ,addr);
addr=PKA_FW_MEM_BASE + 0x5CC; writel( 0x08c00000 ,addr);
addr=PKA_FW_MEM_BASE + 0x5D0; writel( 0x22000044 ,addr);
addr=PKA_FW_MEM_BASE + 0x5D4; writel( 0x400080c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x5D8; writel( 0x44000400 ,addr);
addr=PKA_FW_MEM_BASE + 0x5DC; writel( 0xd8188040 ,addr);
addr=PKA_FW_MEM_BASE + 0x5E0; writel( 0x40000e00 ,addr);
addr=PKA_FW_MEM_BASE + 0x5E4; writel( 0x4000ca00 ,addr);
addr=PKA_FW_MEM_BASE + 0x5E8; writel( 0x40004680 ,addr);
addr=PKA_FW_MEM_BASE + 0x5EC; writel( 0x4000cc00 ,addr);
addr=PKA_FW_MEM_BASE + 0x5F0; writel( 0x4000dc40 ,addr);
addr=PKA_FW_MEM_BASE + 0x5F4; writel( 0x22000108 ,addr);
addr=PKA_FW_MEM_BASE + 0x5F8; writel( 0x44000040 ,addr);
addr=PKA_FW_MEM_BASE + 0x5FC; writel( 0xd8090e00 ,addr);
addr=PKA_FW_MEM_BASE + 0x600; writel( 0x40008240 ,addr);
addr=PKA_FW_MEM_BASE + 0x604; writel( 0x4000d400 ,addr);
addr=PKA_FW_MEM_BASE + 0x608; writel( 0x4000dc40 ,addr);
addr=PKA_FW_MEM_BASE + 0x60C; writel( 0x20000108 ,addr);
addr=PKA_FW_MEM_BASE + 0x610; writel( 0x40001400 ,addr);
addr=PKA_FW_MEM_BASE + 0x614; writel( 0x40001c40 ,addr);
addr=PKA_FW_MEM_BASE + 0x618; writel( 0x40005600 ,addr);
addr=PKA_FW_MEM_BASE + 0x61C; writel( 0x22000108 ,addr);
addr=PKA_FW_MEM_BASE + 0x620; writel( 0x400087c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x624; writel( 0x40001400 ,addr);
addr=PKA_FW_MEM_BASE + 0x628; writel( 0x40001c40 ,addr);
addr=PKA_FW_MEM_BASE + 0x62C; writel( 0x40005e00 ,addr);
addr=PKA_FW_MEM_BASE + 0x630; writel( 0x22000108 ,addr);
addr=PKA_FW_MEM_BASE + 0x634; writel( 0x40008000 ,addr);
addr=PKA_FW_MEM_BASE + 0x638; writel( 0x40005e00 ,addr);
addr=PKA_FW_MEM_BASE + 0x63C; writel( 0x4000f040 ,addr);
addr=PKA_FW_MEM_BASE + 0x640; writel( 0x40000e40 ,addr);
addr=PKA_FW_MEM_BASE + 0x644; writel( 0x22000152 ,addr);
addr=PKA_FW_MEM_BASE + 0x648; writel( 0x40000780 ,addr);
addr=PKA_FW_MEM_BASE + 0x64C; writel( 0x40000400 ,addr);
addr=PKA_FW_MEM_BASE + 0x650; writel( 0x40005600 ,addr);
addr=PKA_FW_MEM_BASE + 0x654; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0x658; writel( 0x40000240 ,addr);
addr=PKA_FW_MEM_BASE + 0x65C; writel( 0x40004880 ,addr);
addr=PKA_FW_MEM_BASE + 0x660; writel( 0x40005600 ,addr);
addr=PKA_FW_MEM_BASE + 0x664; writel( 0x4000e000 ,addr);
addr=PKA_FW_MEM_BASE + 0x668; writel( 0x40000640 ,addr);
addr=PKA_FW_MEM_BASE + 0x66C; writel( 0x40009e80 ,addr);
addr=PKA_FW_MEM_BASE + 0x670; writel( 0x4000e800 ,addr);
addr=PKA_FW_MEM_BASE + 0x674; writel( 0x400006c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x678; writel( 0x4000f800 ,addr);
addr=PKA_FW_MEM_BASE + 0x67C; writel( 0x22000152 ,addr);
addr=PKA_FW_MEM_BASE + 0x680; writel( 0x40001200 ,addr);
addr=PKA_FW_MEM_BASE + 0x684; writel( 0xd8004400 ,addr);
addr=PKA_FW_MEM_BASE + 0x688; writel( 0x22000085 ,addr);
addr=PKA_FW_MEM_BASE + 0x68C; writel( 0x40000400 ,addr);
addr=PKA_FW_MEM_BASE + 0x690; writel( 0x2200013a ,addr);
addr=PKA_FW_MEM_BASE + 0x694; writel( 0x40009200 ,addr);
addr=PKA_FW_MEM_BASE + 0x698; writel( 0xc8088000 ,addr);
addr=PKA_FW_MEM_BASE + 0x69C; writel( 0x40000400 ,addr);
addr=PKA_FW_MEM_BASE + 0x6A0; writel( 0x40000c40 ,addr);
addr=PKA_FW_MEM_BASE + 0x6A4; writel( 0x40005600 ,addr);
addr=PKA_FW_MEM_BASE + 0x6A8; writel( 0x22000108 ,addr);
addr=PKA_FW_MEM_BASE + 0x6AC; writel( 0xc8105800 ,addr);
addr=PKA_FW_MEM_BASE + 0x6B0; writel( 0x430007c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x6B4; writel( 0xd01e0400 ,addr);
addr=PKA_FW_MEM_BASE + 0x6B8; writel( 0xd41f0c40 ,addr);
addr=PKA_FW_MEM_BASE + 0x6BC; writel( 0x40008000 ,addr);
addr=PKA_FW_MEM_BASE + 0x6C0; writel( 0x40008840 ,addr);
addr=PKA_FW_MEM_BASE + 0x6C4; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x6C8; writel( 0x220001e2 ,addr);
addr=PKA_FW_MEM_BASE + 0x6CC; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x6D0; writel( 0x220001cd ,addr);
addr=PKA_FW_MEM_BASE + 0x6D4; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x6D8; writel( 0x220002f2 ,addr);
addr=PKA_FW_MEM_BASE + 0x6DC; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x6E0; writel( 0x220001d6 ,addr);
addr=PKA_FW_MEM_BASE + 0x6E4; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x6E8; writel( 0x220001ca ,addr);
addr=PKA_FW_MEM_BASE + 0x6EC; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x6F0; writel( 0x2200021b ,addr);
addr=PKA_FW_MEM_BASE + 0x6F4; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x6F8; writel( 0x220001c6 ,addr);
addr=PKA_FW_MEM_BASE + 0x6FC; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x700; writel( 0x22000366 ,addr);
addr=PKA_FW_MEM_BASE + 0x704; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x708; writel( 0x2200036c ,addr);
addr=PKA_FW_MEM_BASE + 0x70C; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x710; writel( 0x2200035b ,addr);
addr=PKA_FW_MEM_BASE + 0x714; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x718; writel( 0x22000257 ,addr);
addr=PKA_FW_MEM_BASE + 0x71C; writel( 0x22000373 ,addr);
addr=PKA_FW_MEM_BASE + 0x720; writel( 0x2200025c ,addr);
addr=PKA_FW_MEM_BASE + 0x724; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x728; writel( 0x48800000 ,addr);
addr=PKA_FW_MEM_BASE + 0x72C; writel( 0x22000283 ,addr);
addr=PKA_FW_MEM_BASE + 0x730; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x734; writel( 0x440004c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x738; writel( 0x22000275 ,addr);
addr=PKA_FW_MEM_BASE + 0x73C; writel( 0x220002f2 ,addr);
addr=PKA_FW_MEM_BASE + 0x740; writel( 0x22000249 ,addr);
addr=PKA_FW_MEM_BASE + 0x744; writel( 0x22000257 ,addr);
addr=PKA_FW_MEM_BASE + 0x748; writel( 0x22000373 ,addr);
addr=PKA_FW_MEM_BASE + 0x74C; writel( 0x2200025c ,addr);
addr=PKA_FW_MEM_BASE + 0x750; writel( 0x2200023d ,addr);
addr=PKA_FW_MEM_BASE + 0x754; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x758; writel( 0x44000480 ,addr);
addr=PKA_FW_MEM_BASE + 0x75C; writel( 0x440004c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x760; writel( 0x22000267 ,addr);
addr=PKA_FW_MEM_BASE + 0x764; writel( 0x22000275 ,addr);
addr=PKA_FW_MEM_BASE + 0x768; writel( 0x22000283 ,addr);
addr=PKA_FW_MEM_BASE + 0x76C; writel( 0x33c00384 ,addr);
addr=PKA_FW_MEM_BASE + 0x770; writel( 0x22000249 ,addr);
addr=PKA_FW_MEM_BASE + 0x774; writel( 0x22000257 ,addr);
addr=PKA_FW_MEM_BASE + 0x778; writel( 0x22000373 ,addr);
addr=PKA_FW_MEM_BASE + 0x77C; writel( 0x2200025c ,addr);
addr=PKA_FW_MEM_BASE + 0x780; writel( 0x2200023d ,addr);
addr=PKA_FW_MEM_BASE + 0x784; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x788; writel( 0x22000381 ,addr);
addr=PKA_FW_MEM_BASE + 0x78C; writel( 0x318001e8 ,addr);
addr=PKA_FW_MEM_BASE + 0x790; writel( 0x40003c80 ,addr);
addr=PKA_FW_MEM_BASE + 0x794; writel( 0x22000267 ,addr);
addr=PKA_FW_MEM_BASE + 0x798; writel( 0x2200034f ,addr);
addr=PKA_FW_MEM_BASE + 0x79C; writel( 0x200001ea ,addr);
addr=PKA_FW_MEM_BASE + 0x7A0; writel( 0x44000480 ,addr);
addr=PKA_FW_MEM_BASE + 0x7A4; writel( 0x22000267 ,addr);
addr=PKA_FW_MEM_BASE + 0x7A8; writel( 0x48800000 ,addr);
addr=PKA_FW_MEM_BASE + 0x7AC; writel( 0x50a0f800 ,addr);
addr=PKA_FW_MEM_BASE + 0x7B0; writel( 0x11a00000 ,addr);
addr=PKA_FW_MEM_BASE + 0x7B4; writel( 0x22000242 ,addr);
addr=PKA_FW_MEM_BASE + 0x7B8; writel( 0x23e001f0 ,addr);
addr=PKA_FW_MEM_BASE + 0x7BC; writel( 0x20000204 ,addr);
addr=PKA_FW_MEM_BASE + 0x7C0; writel( 0x15fe8000 ,addr);
addr=PKA_FW_MEM_BASE + 0x7C4; writel( 0x21e00204 ,addr);
addr=PKA_FW_MEM_BASE + 0x7C8; writel( 0x200001f5 ,addr);
addr=PKA_FW_MEM_BASE + 0x7CC; writel( 0x22000249 ,addr);
addr=PKA_FW_MEM_BASE + 0x7D0; writel( 0x200001f5 ,addr);
addr=PKA_FW_MEM_BASE + 0x7D4; writel( 0x220002f2 ,addr);
addr=PKA_FW_MEM_BASE + 0x7D8; writel( 0x22000249 ,addr);
addr=PKA_FW_MEM_BASE + 0x7DC; writel( 0x68e0f800 ,addr);
addr=PKA_FW_MEM_BASE + 0x7E0; writel( 0x200001f9 ,addr);
addr=PKA_FW_MEM_BASE + 0x7E4; writel( 0x262001fc ,addr);
addr=PKA_FW_MEM_BASE + 0x7E8; writel( 0x22000283 ,addr);
addr=PKA_FW_MEM_BASE + 0x7EC; writel( 0x200001fc ,addr);
addr=PKA_FW_MEM_BASE + 0x7F0; writel( 0x200001fd ,addr);
addr=PKA_FW_MEM_BASE + 0x7F4; writel( 0x28e001f3 ,addr);
addr=PKA_FW_MEM_BASE + 0x7F8; writel( 0x22000242 ,addr);
addr=PKA_FW_MEM_BASE + 0x7FC; writel( 0x20000204 ,addr);
addr=PKA_FW_MEM_BASE + 0x800; writel( 0x22000249 ,addr);
addr=PKA_FW_MEM_BASE + 0x804; writel( 0x20000204 ,addr);
addr=PKA_FW_MEM_BASE + 0x808; writel( 0x22000250 ,addr);
addr=PKA_FW_MEM_BASE + 0x80C; writel( 0x20000204 ,addr);
addr=PKA_FW_MEM_BASE + 0x810; writel( 0x220002f2 ,addr);
addr=PKA_FW_MEM_BASE + 0x814; writel( 0x22000249 ,addr);
addr=PKA_FW_MEM_BASE + 0x818; writel( 0x68a0f800 ,addr);
addr=PKA_FW_MEM_BASE + 0x81C; writel( 0x3160020f ,addr);
addr=PKA_FW_MEM_BASE + 0x820; writel( 0x20000209 ,addr);
addr=PKA_FW_MEM_BASE + 0x824; writel( 0x22000283 ,addr);
addr=PKA_FW_MEM_BASE + 0x828; writel( 0x33c00384 ,addr);
addr=PKA_FW_MEM_BASE + 0x82C; writel( 0x2000020c ,addr);
addr=PKA_FW_MEM_BASE + 0x830; writel( 0x28a00200 ,addr);
addr=PKA_FW_MEM_BASE + 0x834; writel( 0x22000249 ,addr);
addr=PKA_FW_MEM_BASE + 0x838; writel( 0x20000216 ,addr);
addr=PKA_FW_MEM_BASE + 0x83C; writel( 0x26200213 ,addr);
addr=PKA_FW_MEM_BASE + 0x840; writel( 0x22000283 ,addr);
addr=PKA_FW_MEM_BASE + 0x844; writel( 0x20000212 ,addr);
addr=PKA_FW_MEM_BASE + 0x848; writel( 0x20000213 ,addr);
addr=PKA_FW_MEM_BASE + 0x84C; writel( 0x28a00202 ,addr);
addr=PKA_FW_MEM_BASE + 0x850; writel( 0x22000250 ,addr);
addr=PKA_FW_MEM_BASE + 0x854; writel( 0x20000216 ,addr);
addr=PKA_FW_MEM_BASE + 0x858; writel( 0x22000257 ,addr);
addr=PKA_FW_MEM_BASE + 0x85C; writel( 0x22000373 ,addr);
addr=PKA_FW_MEM_BASE + 0x860; writel( 0x2200025c ,addr);
addr=PKA_FW_MEM_BASE + 0x864; writel( 0x2200023d ,addr);
addr=PKA_FW_MEM_BASE + 0x868; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x86C; writel( 0xd902c000 ,addr);
addr=PKA_FW_MEM_BASE + 0x870; writel( 0x3140023a ,addr);
addr=PKA_FW_MEM_BASE + 0x874; writel( 0xd90ac000 ,addr);
addr=PKA_FW_MEM_BASE + 0x878; writel( 0x3140023a ,addr);
addr=PKA_FW_MEM_BASE + 0x87C; writel( 0x44000480 ,addr);
addr=PKA_FW_MEM_BASE + 0x880; writel( 0x4000dc00 ,addr);
addr=PKA_FW_MEM_BASE + 0x884; writel( 0x22000267 ,addr);
addr=PKA_FW_MEM_BASE + 0x888; writel( 0x40005000 ,addr);
addr=PKA_FW_MEM_BASE + 0x88C; writel( 0x220000b6 ,addr);
addr=PKA_FW_MEM_BASE + 0x890; writel( 0x40000380 ,addr);
addr=PKA_FW_MEM_BASE + 0x894; writel( 0x40001200 ,addr);
addr=PKA_FW_MEM_BASE + 0x898; writel( 0x40004000 ,addr);
addr=PKA_FW_MEM_BASE + 0x89C; writel( 0x220000b6 ,addr);
addr=PKA_FW_MEM_BASE + 0x8A0; writel( 0x40001200 ,addr);
addr=PKA_FW_MEM_BASE + 0x8A4; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0x8A8; writel( 0x40000340 ,addr);
addr=PKA_FW_MEM_BASE + 0x8AC; writel( 0x40003200 ,addr);
addr=PKA_FW_MEM_BASE + 0x8B0; writel( 0x4000d800 ,addr);
addr=PKA_FW_MEM_BASE + 0x8B4; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0x8B8; writel( 0x40001200 ,addr);
addr=PKA_FW_MEM_BASE + 0x8BC; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0x8C0; writel( 0x400002c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x8C4; writel( 0x40003a00 ,addr);
addr=PKA_FW_MEM_BASE + 0x8C8; writel( 0x4000d800 ,addr);
addr=PKA_FW_MEM_BASE + 0x8CC; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0x8D0; writel( 0xd0005c00 ,addr);
addr=PKA_FW_MEM_BASE + 0x8D4; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0x8D8; writel( 0xd0006c00 ,addr);
addr=PKA_FW_MEM_BASE + 0x8DC; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0x8E0; writel( 0xd9007000 ,addr);
addr=PKA_FW_MEM_BASE + 0x8E4; writel( 0x2000023c ,addr);
addr=PKA_FW_MEM_BASE + 0x8E8; writel( 0x48000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x8EC; writel( 0x2000023c ,addr);
addr=PKA_FW_MEM_BASE + 0x8F0; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x8F4; writel( 0x40001c00 ,addr);
addr=PKA_FW_MEM_BASE + 0x8F8; writel( 0x40008080 ,addr);
addr=PKA_FW_MEM_BASE + 0x8FC; writel( 0x40005c00 ,addr);
addr=PKA_FW_MEM_BASE + 0x900; writel( 0x40008280 ,addr);
addr=PKA_FW_MEM_BASE + 0x904; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x908; writel( 0x40001200 ,addr);
addr=PKA_FW_MEM_BASE + 0x90C; writel( 0x400040c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x910; writel( 0x40005000 ,addr);
addr=PKA_FW_MEM_BASE + 0x914; writel( 0x400002c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x918; writel( 0x40009000 ,addr);
addr=PKA_FW_MEM_BASE + 0x91C; writel( 0x400004c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x920; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x924; writel( 0x40002200 ,addr);
addr=PKA_FW_MEM_BASE + 0x928; writel( 0x400040c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x92C; writel( 0x40006000 ,addr);
addr=PKA_FW_MEM_BASE + 0x930; writel( 0x400002c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x934; writel( 0x4000a000 ,addr);
addr=PKA_FW_MEM_BASE + 0x938; writel( 0x400004c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x93C; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x940; writel( 0x40002200 ,addr);
addr=PKA_FW_MEM_BASE + 0x944; writel( 0x40004140 ,addr);
addr=PKA_FW_MEM_BASE + 0x948; writel( 0x40006000 ,addr);
addr=PKA_FW_MEM_BASE + 0x94C; writel( 0x40000340 ,addr);
addr=PKA_FW_MEM_BASE + 0x950; writel( 0x4000a000 ,addr);
addr=PKA_FW_MEM_BASE + 0x954; writel( 0x40000540 ,addr);
addr=PKA_FW_MEM_BASE + 0x958; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x95C; writel( 0x40009800 ,addr);
addr=PKA_FW_MEM_BASE + 0x960; writel( 0x44000200 ,addr);
addr=PKA_FW_MEM_BASE + 0x964; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0x968; writel( 0x400004c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x96C; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x970; writel( 0x40001a00 ,addr);
addr=PKA_FW_MEM_BASE + 0x974; writel( 0x40004000 ,addr);
addr=PKA_FW_MEM_BASE + 0x978; writel( 0x40009a00 ,addr);
addr=PKA_FW_MEM_BASE + 0x97C; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0x980; writel( 0x40000200 ,addr);
addr=PKA_FW_MEM_BASE + 0x984; writel( 0x400040c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x988; writel( 0x40005800 ,addr);
addr=PKA_FW_MEM_BASE + 0x98C; writel( 0x40009a00 ,addr);
addr=PKA_FW_MEM_BASE + 0x990; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0x994; writel( 0x400002c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x998; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x99C; writel( 0x40001200 ,addr);
addr=PKA_FW_MEM_BASE + 0x9A0; writel( 0x4000d800 ,addr);
addr=PKA_FW_MEM_BASE + 0x9A4; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0x9A8; writel( 0x40000200 ,addr);
addr=PKA_FW_MEM_BASE + 0x9AC; writel( 0x40004080 ,addr);
addr=PKA_FW_MEM_BASE + 0x9B0; writel( 0x40005000 ,addr);
addr=PKA_FW_MEM_BASE + 0x9B4; writel( 0x4000da00 ,addr);
addr=PKA_FW_MEM_BASE + 0x9B8; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0x9BC; writel( 0x40000280 ,addr);
addr=PKA_FW_MEM_BASE + 0x9C0; writel( 0x40009000 ,addr);
addr=PKA_FW_MEM_BASE + 0x9C4; writel( 0x4000da00 ,addr);
addr=PKA_FW_MEM_BASE + 0x9C8; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0x9CC; writel( 0x40000480 ,addr);
addr=PKA_FW_MEM_BASE + 0x9D0; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0x9D4; writel( 0x40001a00 ,addr);
addr=PKA_FW_MEM_BASE + 0x9D8; writel( 0x4000d800 ,addr);
addr=PKA_FW_MEM_BASE + 0x9DC; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0x9E0; writel( 0x40000200 ,addr);
addr=PKA_FW_MEM_BASE + 0x9E4; writel( 0x400040c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x9E8; writel( 0x40005800 ,addr);
addr=PKA_FW_MEM_BASE + 0x9EC; writel( 0x4000da00 ,addr);
addr=PKA_FW_MEM_BASE + 0x9F0; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0x9F4; writel( 0x400002c0 ,addr);
addr=PKA_FW_MEM_BASE + 0x9F8; writel( 0x40009800 ,addr);
addr=PKA_FW_MEM_BASE + 0x9FC; writel( 0x4000da00 ,addr);
addr=PKA_FW_MEM_BASE + 0xA00; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xA04; writel( 0x400004c0 ,addr);
addr=PKA_FW_MEM_BASE + 0xA08; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xA0C; writel( 0x40001200 ,addr);
addr=PKA_FW_MEM_BASE + 0xA10; writel( 0x40009800 ,addr);
addr=PKA_FW_MEM_BASE + 0xA14; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xA18; writel( 0x40000700 ,addr);
addr=PKA_FW_MEM_BASE + 0xA1C; writel( 0x40001a00 ,addr);
addr=PKA_FW_MEM_BASE + 0xA20; writel( 0x40009000 ,addr);
addr=PKA_FW_MEM_BASE + 0xA24; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xA28; writel( 0x40000300 ,addr);
addr=PKA_FW_MEM_BASE + 0xA2C; writel( 0xd80ce400 ,addr);
addr=PKA_FW_MEM_BASE + 0xA30; writel( 0x3100028f ,addr);
addr=PKA_FW_MEM_BASE + 0xA34; writel( 0x49c00000 ,addr);
addr=PKA_FW_MEM_BASE + 0xA38; writel( 0x20000290 ,addr);
addr=PKA_FW_MEM_BASE + 0xA3C; writel( 0x48c00000 ,addr);
addr=PKA_FW_MEM_BASE + 0xA40; writel( 0x20000291 ,addr);
addr=PKA_FW_MEM_BASE + 0xA44; writel( 0x22000085 ,addr);
addr=PKA_FW_MEM_BASE + 0xA48; writel( 0x40000500 ,addr);
addr=PKA_FW_MEM_BASE + 0xA4C; writel( 0xd00ce400 ,addr);
addr=PKA_FW_MEM_BASE + 0xA50; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0xA54; writel( 0x40000540 ,addr);
addr=PKA_FW_MEM_BASE + 0xA58; writel( 0x4000a000 ,addr);
addr=PKA_FW_MEM_BASE + 0xA5C; writel( 0x220000b6 ,addr);
addr=PKA_FW_MEM_BASE + 0xA60; writel( 0x40000740 ,addr);
addr=PKA_FW_MEM_BASE + 0xA64; writel( 0x4000a200 ,addr);
addr=PKA_FW_MEM_BASE + 0xA68; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xA6C; writel( 0x40000780 ,addr);
addr=PKA_FW_MEM_BASE + 0xA70; writel( 0x4000e800 ,addr);
addr=PKA_FW_MEM_BASE + 0xA74; writel( 0x4000aa00 ,addr);
addr=PKA_FW_MEM_BASE + 0xA78; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xA7C; writel( 0x40000340 ,addr);
addr=PKA_FW_MEM_BASE + 0xA80; writel( 0x40005000 ,addr);
addr=PKA_FW_MEM_BASE + 0xA84; writel( 0x40009a00 ,addr);
addr=PKA_FW_MEM_BASE + 0xA88; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xA8C; writel( 0x40000580 ,addr);
addr=PKA_FW_MEM_BASE + 0xA90; writel( 0x40005800 ,addr);
addr=PKA_FW_MEM_BASE + 0xA94; writel( 0x40009200 ,addr);
addr=PKA_FW_MEM_BASE + 0xA98; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xA9C; writel( 0x40000700 ,addr);
addr=PKA_FW_MEM_BASE + 0xAA0; writel( 0xd81cb380 ,addr);
addr=PKA_FW_MEM_BASE + 0xAA4; writel( 0x40007400 ,addr);
addr=PKA_FW_MEM_BASE + 0xAA8; writel( 0x22000085 ,addr);
addr=PKA_FW_MEM_BASE + 0xAAC; writel( 0x40000380 ,addr);
addr=PKA_FW_MEM_BASE + 0xAB0; writel( 0xd01cb000 ,addr);
addr=PKA_FW_MEM_BASE + 0xAB4; writel( 0x40000400 ,addr);
addr=PKA_FW_MEM_BASE + 0xAB8; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0xABC; writel( 0x400003c0 ,addr);
addr=PKA_FW_MEM_BASE + 0xAC0; writel( 0x40007000 ,addr);
addr=PKA_FW_MEM_BASE + 0xAC4; writel( 0x220000b6 ,addr);
addr=PKA_FW_MEM_BASE + 0xAC8; writel( 0x400005c0 ,addr);
addr=PKA_FW_MEM_BASE + 0xACC; writel( 0x40009000 ,addr);
addr=PKA_FW_MEM_BASE + 0xAD0; writel( 0x40009a00 ,addr);
addr=PKA_FW_MEM_BASE + 0xAD4; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xAD8; writel( 0x40000700 ,addr);
addr=PKA_FW_MEM_BASE + 0xADC; writel( 0x4000ba00 ,addr);
addr=PKA_FW_MEM_BASE + 0xAE0; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xAE4; writel( 0x40000740 ,addr);
addr=PKA_FW_MEM_BASE + 0xAE8; writel( 0xd8006c00 ,addr);
addr=PKA_FW_MEM_BASE + 0xAEC; writel( 0x22000085 ,addr);
addr=PKA_FW_MEM_BASE + 0xAF0; writel( 0x4000a200 ,addr);
addr=PKA_FW_MEM_BASE + 0xAF4; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xAF8; writel( 0x40000200 ,addr);
addr=PKA_FW_MEM_BASE + 0xAFC; writel( 0x40004100 ,addr);
addr=PKA_FW_MEM_BASE + 0xB00; writel( 0x42006800 ,addr);
addr=PKA_FW_MEM_BASE + 0xB04; writel( 0x40000400 ,addr);
addr=PKA_FW_MEM_BASE + 0xB08; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0xB0C; writel( 0xd00d0400 ,addr);
addr=PKA_FW_MEM_BASE + 0xB10; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0xB14; writel( 0x40000500 ,addr);
addr=PKA_FW_MEM_BASE + 0xB18; writel( 0x4200ec00 ,addr);
addr=PKA_FW_MEM_BASE + 0xB1C; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0xB20; writel( 0x40000200 ,addr);
addr=PKA_FW_MEM_BASE + 0xB24; writel( 0xd8144000 ,addr);
addr=PKA_FW_MEM_BASE + 0xB28; writel( 0x40000400 ,addr);
addr=PKA_FW_MEM_BASE + 0xB2C; writel( 0x22000085 ,addr);
addr=PKA_FW_MEM_BASE + 0xB30; writel( 0x40000200 ,addr);
addr=PKA_FW_MEM_BASE + 0xB34; writel( 0x40007000 ,addr);
addr=PKA_FW_MEM_BASE + 0xB38; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xB3C; writel( 0x40000500 ,addr);
addr=PKA_FW_MEM_BASE + 0xB40; writel( 0x40007800 ,addr);
addr=PKA_FW_MEM_BASE + 0xB44; writel( 0x4000f200 ,addr);
addr=PKA_FW_MEM_BASE + 0xB48; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xB4C; writel( 0xd8140740 ,addr);
addr=PKA_FW_MEM_BASE + 0xB50; writel( 0x4000ec00 ,addr);
addr=PKA_FW_MEM_BASE + 0xB54; writel( 0x22000085 ,addr);
addr=PKA_FW_MEM_BASE + 0xB58; writel( 0x40000740 ,addr);
addr=PKA_FW_MEM_BASE + 0xB5C; writel( 0x6b00e800 ,addr);
addr=PKA_FW_MEM_BASE + 0xB60; writel( 0x338002e4 ,addr);
addr=PKA_FW_MEM_BASE + 0xB64; writel( 0x336002df ,addr);
addr=PKA_FW_MEM_BASE + 0xB68; writel( 0x4000c200 ,addr);
addr=PKA_FW_MEM_BASE + 0xB6C; writel( 0xd008e840 ,addr);
addr=PKA_FW_MEM_BASE + 0xB70; writel( 0x40000680 ,addr);
addr=PKA_FW_MEM_BASE + 0xB74; writel( 0x48200000 ,addr);
addr=PKA_FW_MEM_BASE + 0xB78; writel( 0x200002ea ,addr);
addr=PKA_FW_MEM_BASE + 0xB7C; writel( 0x4000c200 ,addr);
addr=PKA_FW_MEM_BASE + 0xB80; writel( 0xd008e800 ,addr);
addr=PKA_FW_MEM_BASE + 0xB84; writel( 0x40000740 ,addr);
addr=PKA_FW_MEM_BASE + 0xB88; writel( 0x48000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xB8C; writel( 0x200002ea ,addr);
addr=PKA_FW_MEM_BASE + 0xB90; writel( 0x336002e7 ,addr);
addr=PKA_FW_MEM_BASE + 0xB94; writel( 0x48200000 ,addr);
addr=PKA_FW_MEM_BASE + 0xB98; writel( 0x200002ea ,addr);
addr=PKA_FW_MEM_BASE + 0xB9C; writel( 0x4000c200 ,addr);
addr=PKA_FW_MEM_BASE + 0xBA0; writel( 0xd008e800 ,addr);
addr=PKA_FW_MEM_BASE + 0xBA4; writel( 0x40000740 ,addr);
addr=PKA_FW_MEM_BASE + 0xBA8; writel( 0x4100eb00 ,addr);
addr=PKA_FW_MEM_BASE + 0xBAC; writel( 0x4000e000 ,addr);
addr=PKA_FW_MEM_BASE + 0xBB0; writel( 0x200002ee ,addr);
addr=PKA_FW_MEM_BASE + 0xBB4; writel( 0x40009800 ,addr);
addr=PKA_FW_MEM_BASE + 0xBB8; writel( 0x4000f200 ,addr);
addr=PKA_FW_MEM_BASE + 0xBBC; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xBC0; writel( 0x40000500 ,addr);
addr=PKA_FW_MEM_BASE + 0xBC4; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xBC8; writel( 0x40009a00 ,addr);
addr=PKA_FW_MEM_BASE + 0xBCC; writel( 0x40005800 ,addr);
addr=PKA_FW_MEM_BASE + 0xBD0; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xBD4; writel( 0x42000400 ,addr);
addr=PKA_FW_MEM_BASE + 0xBD8; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0xBDC; writel( 0x40000500 ,addr);
addr=PKA_FW_MEM_BASE + 0xBE0; writel( 0x33e00310 ,addr);
addr=PKA_FW_MEM_BASE + 0xBE4; writel( 0x40001a00 ,addr);
addr=PKA_FW_MEM_BASE + 0xBE8; writel( 0x40004000 ,addr);
addr=PKA_FW_MEM_BASE + 0xBEC; writel( 0x220000b6 ,addr);
addr=PKA_FW_MEM_BASE + 0xBF0; writel( 0x40000780 ,addr);
addr=PKA_FW_MEM_BASE + 0xBF4; writel( 0x42000200 ,addr);
addr=PKA_FW_MEM_BASE + 0xBF8; writel( 0x40004400 ,addr);
addr=PKA_FW_MEM_BASE + 0xBFC; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0xC00; writel( 0xd000f400 ,addr);
addr=PKA_FW_MEM_BASE + 0xC04; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0xC08; writel( 0x40000580 ,addr);
addr=PKA_FW_MEM_BASE + 0xC0C; writel( 0x40009800 ,addr);
addr=PKA_FW_MEM_BASE + 0xC10; writel( 0x220000b6 ,addr);
addr=PKA_FW_MEM_BASE + 0xC14; writel( 0x400004c0 ,addr);
addr=PKA_FW_MEM_BASE + 0xC18; writel( 0x40003200 ,addr);
addr=PKA_FW_MEM_BASE + 0xC1C; writel( 0x4000d800 ,addr);
addr=PKA_FW_MEM_BASE + 0xC20; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xC24; writel( 0x40009a00 ,addr);
addr=PKA_FW_MEM_BASE + 0xC28; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xC2C; writel( 0xd0160200 ,addr);
addr=PKA_FW_MEM_BASE + 0xC30; writel( 0x40004400 ,addr);
addr=PKA_FW_MEM_BASE + 0xC34; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0xC38; writel( 0x40000580 ,addr);
addr=PKA_FW_MEM_BASE + 0xC3C; writel( 0x2000031f ,addr);
addr=PKA_FW_MEM_BASE + 0xC40; writel( 0xd8039a00 ,addr);
addr=PKA_FW_MEM_BASE + 0xC44; writel( 0x40004400 ,addr);
addr=PKA_FW_MEM_BASE + 0xC48; writel( 0x22000085 ,addr);
addr=PKA_FW_MEM_BASE + 0xC4C; writel( 0x40000200 ,addr);
addr=PKA_FW_MEM_BASE + 0xC50; writel( 0xd0039a40 ,addr);
addr=PKA_FW_MEM_BASE + 0xC54; writel( 0x40004c00 ,addr);
addr=PKA_FW_MEM_BASE + 0xC58; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0xC5C; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xC60; writel( 0x40000440 ,addr);
addr=PKA_FW_MEM_BASE + 0xC64; writel( 0x40000200 ,addr);
addr=PKA_FW_MEM_BASE + 0xC68; writel( 0x42000400 ,addr);
addr=PKA_FW_MEM_BASE + 0xC6C; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0xC70; writel( 0xd0004400 ,addr);
addr=PKA_FW_MEM_BASE + 0xC74; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0xC78; writel( 0x40000580 ,addr);
addr=PKA_FW_MEM_BASE + 0xC7C; writel( 0x40001a00 ,addr);
addr=PKA_FW_MEM_BASE + 0xC80; writel( 0x40005800 ,addr);
addr=PKA_FW_MEM_BASE + 0xC84; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xC88; writel( 0x400005c0 ,addr);
addr=PKA_FW_MEM_BASE + 0xC8C; writel( 0x4000b000 ,addr);
addr=PKA_FW_MEM_BASE + 0xC90; writel( 0x220000b6 ,addr);
addr=PKA_FW_MEM_BASE + 0xC94; writel( 0x40000380 ,addr);
addr=PKA_FW_MEM_BASE + 0xC98; writel( 0x4000a000 ,addr);
addr=PKA_FW_MEM_BASE + 0xC9C; writel( 0x4000ba00 ,addr);
addr=PKA_FW_MEM_BASE + 0xCA0; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xCA4; writel( 0x400003c0 ,addr);
addr=PKA_FW_MEM_BASE + 0xCA8; writel( 0x4000a000 ,addr);
addr=PKA_FW_MEM_BASE + 0xCAC; writel( 0x220000b6 ,addr);
addr=PKA_FW_MEM_BASE + 0xCB0; writel( 0x40000700 ,addr);
addr=PKA_FW_MEM_BASE + 0xCB4; writel( 0x42007c00 ,addr);
addr=PKA_FW_MEM_BASE + 0xCB8; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0xCBC; writel( 0x40000740 ,addr);
addr=PKA_FW_MEM_BASE + 0xCC0; writel( 0x42000400 ,addr);
addr=PKA_FW_MEM_BASE + 0xCC4; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0xCC8; writel( 0x400003c0 ,addr);
addr=PKA_FW_MEM_BASE + 0xCCC; writel( 0x40007000 ,addr);
addr=PKA_FW_MEM_BASE + 0xCD0; writel( 0xd8007c00 ,addr);
addr=PKA_FW_MEM_BASE + 0xCD4; writel( 0x22000085 ,addr);
addr=PKA_FW_MEM_BASE + 0xCD8; writel( 0x4000a200 ,addr);
addr=PKA_FW_MEM_BASE + 0xCDC; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xCE0; writel( 0x40000400 ,addr);
addr=PKA_FW_MEM_BASE + 0xCE4; writel( 0x40008100 ,addr);
addr=PKA_FW_MEM_BASE + 0xCE8; writel( 0xd00fec00 ,addr);
addr=PKA_FW_MEM_BASE + 0xCEC; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0xCF0; writel( 0xd8007400 ,addr);
addr=PKA_FW_MEM_BASE + 0xCF4; writel( 0x22000085 ,addr);
addr=PKA_FW_MEM_BASE + 0xCF8; writel( 0x4000b200 ,addr);
addr=PKA_FW_MEM_BASE + 0xCFC; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xD00; writel( 0x400003c0 ,addr);
addr=PKA_FW_MEM_BASE + 0xD04; writel( 0x40005800 ,addr);
addr=PKA_FW_MEM_BASE + 0xD08; writel( 0x220000b6 ,addr);
addr=PKA_FW_MEM_BASE + 0xD0C; writel( 0x4000e200 ,addr);
addr=PKA_FW_MEM_BASE + 0xD10; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xD14; writel( 0x42000400 ,addr);
addr=PKA_FW_MEM_BASE + 0xD18; writel( 0x220000ba ,addr);
addr=PKA_FW_MEM_BASE + 0xD1C; writel( 0xd80f0400 ,addr);
addr=PKA_FW_MEM_BASE + 0xD20; writel( 0x22000085 ,addr);
addr=PKA_FW_MEM_BASE + 0xD24; writel( 0x40000300 ,addr);
addr=PKA_FW_MEM_BASE + 0xD28; writel( 0x4000a000 ,addr);
addr=PKA_FW_MEM_BASE + 0xD2C; writel( 0x4000e200 ,addr);
addr=PKA_FW_MEM_BASE + 0xD30; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xD34; writel( 0x40000500 ,addr);
addr=PKA_FW_MEM_BASE + 0xD38; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xD3C; writel( 0x40001200 ,addr);
addr=PKA_FW_MEM_BASE + 0xD40; writel( 0x40004000 ,addr);
addr=PKA_FW_MEM_BASE + 0xD44; writel( 0x22000358 ,addr);
addr=PKA_FW_MEM_BASE + 0xD48; writel( 0x40000200 ,addr);
addr=PKA_FW_MEM_BASE + 0xD4C; writel( 0x40004080 ,addr);
addr=PKA_FW_MEM_BASE + 0xD50; writel( 0x40005000 ,addr);
addr=PKA_FW_MEM_BASE + 0xD54; writel( 0x22000358 ,addr);
addr=PKA_FW_MEM_BASE + 0xD58; writel( 0x40000280 ,addr);
addr=PKA_FW_MEM_BASE + 0xD5C; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xD60; writel( 0x40009200 ,addr);
addr=PKA_FW_MEM_BASE + 0xD64; writel( 0x220000af ,addr);
addr=PKA_FW_MEM_BASE + 0xD68; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xD6C; writel( 0x44000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xD70; writel( 0x08800001 ,addr);
addr=PKA_FW_MEM_BASE + 0xD74; writel( 0x6a800000 ,addr);
addr=PKA_FW_MEM_BASE + 0xD78; writel( 0x43000200 ,addr);
addr=PKA_FW_MEM_BASE + 0xD7C; writel( 0xd8080400 ,addr);
addr=PKA_FW_MEM_BASE + 0xD80; writel( 0x22000085 ,addr);
addr=PKA_FW_MEM_BASE + 0xD84; writel( 0x40000200 ,addr);
addr=PKA_FW_MEM_BASE + 0xD88; writel( 0xd9083000 ,addr);
addr=PKA_FW_MEM_BASE + 0xD8C; writel( 0x31000365 ,addr);
addr=PKA_FW_MEM_BASE + 0xD90; writel( 0x49e00000 ,addr);
addr=PKA_FW_MEM_BASE + 0xD94; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xD98; writel( 0x40001dc0 ,addr);
addr=PKA_FW_MEM_BASE + 0xD9C; writel( 0xd902b800 ,addr);
addr=PKA_FW_MEM_BASE + 0xDA0; writel( 0x3100036b ,addr);
addr=PKA_FW_MEM_BASE + 0xDA4; writel( 0x400055c0 ,addr);
addr=PKA_FW_MEM_BASE + 0xDA8; writel( 0xd90bb800 ,addr);
addr=PKA_FW_MEM_BASE + 0xDAC; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xDB0; writel( 0x40001dc0 ,addr);
addr=PKA_FW_MEM_BASE + 0xDB4; writel( 0xd902b800 ,addr);
addr=PKA_FW_MEM_BASE + 0xDB8; writel( 0x31000372 ,addr);
addr=PKA_FW_MEM_BASE + 0xDBC; writel( 0x40005800 ,addr);
addr=PKA_FW_MEM_BASE + 0xDC0; writel( 0xd00a0400 ,addr);
addr=PKA_FW_MEM_BASE + 0xDC4; writel( 0x220000b9 ,addr);
addr=PKA_FW_MEM_BASE + 0xDC8; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xDCC; writel( 0x40009800 ,addr);
addr=PKA_FW_MEM_BASE + 0xDD0; writel( 0x44000400 ,addr);
addr=PKA_FW_MEM_BASE + 0xDD4; writel( 0x08c00000 ,addr);
addr=PKA_FW_MEM_BASE + 0xDD8; writel( 0x22000044 ,addr);
addr=PKA_FW_MEM_BASE + 0xDDC; writel( 0x40008000 ,addr);
addr=PKA_FW_MEM_BASE + 0xDE0; writel( 0x400004c0 ,addr);
addr=PKA_FW_MEM_BASE + 0xDE4; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xDE8; writel( 0x44000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xDEC; writel( 0xd91f0000 ,addr);
addr=PKA_FW_MEM_BASE + 0xDF0; writel( 0x33000380 ,addr);
addr=PKA_FW_MEM_BASE + 0xDF4; writel( 0x43000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xDF8; writel( 0xd91f0000 ,addr);
addr=PKA_FW_MEM_BASE + 0xDFC; writel( 0x33000380 ,addr);
addr=PKA_FW_MEM_BASE + 0xE00; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xE04; writel( 0x2200037a ,addr);
addr=PKA_FW_MEM_BASE + 0xE08; writel( 0x33000385 ,addr);
addr=PKA_FW_MEM_BASE + 0xE0C; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xE10; writel( 0x00000043 ,addr);
addr=PKA_FW_MEM_BASE + 0xE14; writel( 0x00000042 ,addr);
addr=PKA_FW_MEM_BASE + 0xE18; writel( 0x00000041 ,addr);
addr=PKA_FW_MEM_BASE + 0xE1C; writel( 0x22000389 ,addr);
addr=PKA_FW_MEM_BASE + 0xE20; writel( 0x00000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xE24; writel( 0x220003e6 ,addr);
addr=PKA_FW_MEM_BASE + 0xE28; writel( 0x49800000 ,addr);
addr=PKA_FW_MEM_BASE + 0xE2C; writel( 0x22000366 ,addr);
addr=PKA_FW_MEM_BASE + 0xE30; writel( 0x33000384 ,addr);
addr=PKA_FW_MEM_BASE + 0xE34; writel( 0x2200036c ,addr);
addr=PKA_FW_MEM_BASE + 0xE38; writel( 0x33000384 ,addr);
addr=PKA_FW_MEM_BASE + 0xE3C; writel( 0x44000480 ,addr);
addr=PKA_FW_MEM_BASE + 0xE40; writel( 0x440004c0 ,addr);
addr=PKA_FW_MEM_BASE + 0xE44; writel( 0x43000100 ,addr);
addr=PKA_FW_MEM_BASE + 0xE48; writel( 0x43000300 ,addr);
addr=PKA_FW_MEM_BASE + 0xE4C; writel( 0x43000500 ,addr);
addr=PKA_FW_MEM_BASE + 0xE50; writel( 0x22000267 ,addr);
addr=PKA_FW_MEM_BASE + 0xE54; writel( 0x22000275 ,addr);
addr=PKA_FW_MEM_BASE + 0xE58; writel( 0xe007fbc0 ,addr);
addr=PKA_FW_MEM_BASE + 0xE5C; writel( 0x3300039a ,addr);
addr=PKA_FW_MEM_BASE + 0xE60; writel( 0x22000283 ,addr);
addr=PKA_FW_MEM_BASE + 0xE64; writel( 0x33c00384 ,addr);
addr=PKA_FW_MEM_BASE + 0xE68; writel( 0x40001700 ,addr);
addr=PKA_FW_MEM_BASE + 0xE6C; writel( 0x40001f40 ,addr);
addr=PKA_FW_MEM_BASE + 0xE70; writel( 0x40002680 ,addr);
addr=PKA_FW_MEM_BASE + 0xE74; writel( 0x18000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xE78; writel( 0x4000e000 ,addr);
addr=PKA_FW_MEM_BASE + 0xE7C; writel( 0x40005040 ,addr);
addr=PKA_FW_MEM_BASE + 0xE80; writel( 0x40009080 ,addr);
addr=PKA_FW_MEM_BASE + 0xE84; writel( 0x4000e8c0 ,addr);
addr=PKA_FW_MEM_BASE + 0xE88; writel( 0x40005900 ,addr);
addr=PKA_FW_MEM_BASE + 0xE8C; writel( 0x40009940 ,addr);
addr=PKA_FW_MEM_BASE + 0xE90; writel( 0x40006180 ,addr);
addr=PKA_FW_MEM_BASE + 0xE94; writel( 0x4000a1c0 ,addr);
addr=PKA_FW_MEM_BASE + 0xE98; writel( 0x19000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xE9C; writel( 0x50c03800 ,addr);
addr=PKA_FW_MEM_BASE + 0xEA0; writel( 0x50e0f800 ,addr);
addr=PKA_FW_MEM_BASE + 0xEA4; writel( 0x159b8000 ,addr);
addr=PKA_FW_MEM_BASE + 0xEA8; writel( 0x334003b4 ,addr);
addr=PKA_FW_MEM_BASE + 0xEAC; writel( 0x330003bc ,addr);
addr=PKA_FW_MEM_BASE + 0xEB0; writel( 0x40001700 ,addr);
addr=PKA_FW_MEM_BASE + 0xEB4; writel( 0x4000e100 ,addr);
addr=PKA_FW_MEM_BASE + 0xEB8; writel( 0x40005700 ,addr);
addr=PKA_FW_MEM_BASE + 0xEBC; writel( 0x4000e300 ,addr);
addr=PKA_FW_MEM_BASE + 0xEC0; writel( 0x40009700 ,addr);
addr=PKA_FW_MEM_BASE + 0xEC4; writel( 0x4000e500 ,addr);
addr=PKA_FW_MEM_BASE + 0xEC8; writel( 0x16980000 ,addr);
addr=PKA_FW_MEM_BASE + 0xECC; writel( 0x200003bd ,addr);
addr=PKA_FW_MEM_BASE + 0xED0; writel( 0x40001f00 ,addr);
addr=PKA_FW_MEM_BASE + 0xED4; writel( 0x4000e100 ,addr);
addr=PKA_FW_MEM_BASE + 0xED8; writel( 0x40005f00 ,addr);
addr=PKA_FW_MEM_BASE + 0xEDC; writel( 0x4000e300 ,addr);
addr=PKA_FW_MEM_BASE + 0xEE0; writel( 0x40009f00 ,addr);
addr=PKA_FW_MEM_BASE + 0xEE4; writel( 0x4000e500 ,addr);
addr=PKA_FW_MEM_BASE + 0xEE8; writel( 0x169c0000 ,addr);
addr=PKA_FW_MEM_BASE + 0xEEC; writel( 0x200003bd ,addr);
addr=PKA_FW_MEM_BASE + 0xEF0; writel( 0x16980000 ,addr);
addr=PKA_FW_MEM_BASE + 0xEF4; writel( 0x11800000 ,addr);
addr=PKA_FW_MEM_BASE + 0xEF8; writel( 0x22000249 ,addr);
addr=PKA_FW_MEM_BASE + 0xEFC; writel( 0x220002f2 ,addr);
addr=PKA_FW_MEM_BASE + 0xF00; writel( 0x22000249 ,addr);
addr=PKA_FW_MEM_BASE + 0xF04; writel( 0x68803800 ,addr);
addr=PKA_FW_MEM_BASE + 0xF08; writel( 0x336003c6 ,addr);
addr=PKA_FW_MEM_BASE + 0xF0C; writel( 0x6880f800 ,addr);
addr=PKA_FW_MEM_BASE + 0xF10; writel( 0x336003cf ,addr);
addr=PKA_FW_MEM_BASE + 0xF14; writel( 0x200003dc ,addr);
addr=PKA_FW_MEM_BASE + 0xF18; writel( 0x6880f800 ,addr);
addr=PKA_FW_MEM_BASE + 0xF1C; writel( 0x336003d6 ,addr);
addr=PKA_FW_MEM_BASE + 0xF20; writel( 0x18000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xF24; writel( 0x40000700 ,addr);
addr=PKA_FW_MEM_BASE + 0xF28; writel( 0x40000a80 ,addr);
addr=PKA_FW_MEM_BASE + 0xF2C; writel( 0x40001480 ,addr);
addr=PKA_FW_MEM_BASE + 0xF30; writel( 0x19000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xF34; writel( 0x4000e080 ,addr);
addr=PKA_FW_MEM_BASE + 0xF38; writel( 0x200003dd ,addr);
addr=PKA_FW_MEM_BASE + 0xF3C; writel( 0x18000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xF40; writel( 0x40001f00 ,addr);
addr=PKA_FW_MEM_BASE + 0xF44; writel( 0x40002280 ,addr);
addr=PKA_FW_MEM_BASE + 0xF48; writel( 0x40002c80 ,addr);
addr=PKA_FW_MEM_BASE + 0xF4C; writel( 0x19000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xF50; writel( 0x4000e080 ,addr);
addr=PKA_FW_MEM_BASE + 0xF54; writel( 0x200003dd ,addr);
addr=PKA_FW_MEM_BASE + 0xF58; writel( 0x4000d080 ,addr);
addr=PKA_FW_MEM_BASE + 0xF5C; writel( 0x18000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xF60; writel( 0x40003280 ,addr);
addr=PKA_FW_MEM_BASE + 0xF64; writel( 0x40003c80 ,addr);
addr=PKA_FW_MEM_BASE + 0xF68; writel( 0x19000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xF6C; writel( 0x200003dd ,addr);
addr=PKA_FW_MEM_BASE + 0xF70; writel( 0x200003df ,addr);
addr=PKA_FW_MEM_BASE + 0xF74; writel( 0x22000283 ,addr);
addr=PKA_FW_MEM_BASE + 0xF78; writel( 0x33c00384 ,addr);
addr=PKA_FW_MEM_BASE + 0xF7C; writel( 0x288003be ,addr);
addr=PKA_FW_MEM_BASE + 0xF80; writel( 0x22000249 ,addr);
addr=PKA_FW_MEM_BASE + 0xF84; writel( 0x22000257 ,addr);
addr=PKA_FW_MEM_BASE + 0xF88; writel( 0x22000373 ,addr);
addr=PKA_FW_MEM_BASE + 0xF8C; writel( 0x2200025c ,addr);
addr=PKA_FW_MEM_BASE + 0xF90; writel( 0x2200023d ,addr);
addr=PKA_FW_MEM_BASE + 0xF94; writel( 0x24000000 ,addr);
addr=PKA_FW_MEM_BASE + 0xF98; writel( 0x22000381 ,addr);
addr=PKA_FW_MEM_BASE + 0xF9C; writel( 0x4000fa00 ,addr);
addr=PKA_FW_MEM_BASE + 0xFA0; writel( 0x40003fc0 ,addr);
addr=PKA_FW_MEM_BASE + 0xFA4; writel( 0x22000381 ,addr);
addr=PKA_FW_MEM_BASE + 0xFA8; writel( 0x400047c0 ,addr);
addr=PKA_FW_MEM_BASE + 0xFAC; writel( 0x24000000 ,addr);
}                     /*}}}*/

int ic_test(void)
{

  printf("PKA start \n");

  reg32(0x250900a0) = 0x7f;


/*{{{*/
    int addr;

    int err=0;

    printf(" before fw init\n");
    JC3_pka_fw_init();
    printf(" after fw init\n");


    //clear the stack pointer
    addr=PKA_BASE + 0x10 ;
    writel(0x0,addr);

    writel(0x0,PKA_BASE+0x001C);//CONFIG little endian
    writel(0x40000000,PKA_BASE+0x0040);//IRQ_EN =1

    int r_inv[7];
    int mp[8];
    int r_sqr[8];
    int a[8];
    int p[8];

    int u1Ax[8];
    int u1Ay[8];
    int u2Bx[8];
    int u2By[8];
    int R[8];

    R[0] = 0x4c043fb1;
    R[1] = 0x8cb4b775;
    R[2] = 0x22a71274;
    R[3] = 0x1c9c8612;
    R[4] = 0xedfd8e9b;
    R[5] = 0x5e9d157f;
    R[6] = 0x878b4dc0;
    R[7] = 0x275fa760;

    a[0] = 0xFFFFFFFC;
    a[1] = 0xFFFFFFFF;
    a[2] = 0xFFFFFFFF;
    a[3] = 0x00000000;
    a[4] = 0x00000000;
    a[5] = 0x00000000;
    a[6] = 0x00000001;
    a[7] = 0xFFFFFFFF;

    p[0] = 0xffffffff;
    p[1] = 0xffffffff;
    p[2] = 0xffffffff;
    p[3] = 0x00000000;
    p[4] = 0x00000000;
    p[5] = 0x00000000;
    p[6] = 0x00000001;
    p[7] = 0xffffffff;

    u1Ax[0] = 0x9159b442;
    u1Ax[1] = 0x04741ffb;
    u1Ax[2] = 0xbc9dc6eb;
    u1Ax[3] = 0xaf61ed0f;
    u1Ax[4] = 0x6118ddc8;
    u1Ax[5] = 0x65140bd1;
    u1Ax[6] = 0x0bc73aeb;
    u1Ax[7] = 0xe122af7d;

    u1Ay[0] = 0x2e5986f3;
    u1Ay[1] = 0x14703c61;
    u1Ay[2] = 0x48b601b4;
    u1Ay[3] = 0x58ceadb8;
    u1Ay[4] = 0xc5f833a8;
    u1Ay[5] = 0x95fea0b9;
    u1Ay[6] = 0xb00ab265;
    u1Ay[7] = 0x30b54df3;

    u2Bx[0] = 0x87374e43;
    u2Bx[1] = 0x2ab1bd36;
    u2Bx[2] = 0xc2c98f5d;
    u2Bx[3] = 0x901ec7eb;
    u2Bx[4] = 0xd8c6f1dd;
    u2Bx[5] = 0x2f9fe592;
    u2Bx[6] = 0x00ae2c8d;
    u2Bx[7] = 0x073d9d27;

    u2By[0] = 0x3176e0e3;
    u2By[1] = 0x057f15a8;
    u2By[2] = 0x201bd5f9;
    u2By[3] = 0x502c8deb;
    u2By[4] = 0xbb0955e7;
    u2By[5] = 0x676d41de;
    u2By[6] = 0xcbf8167e;
    u2By[7] = 0x3872b04a;

    printf("Montgomery r^-1\n");
    load_data(0x1000,p);
    run(0x11);
    get_data(0xc00,r_inv);

    printf("Montgomery p' Precomputation\n");
    run(0x10);
    get_data(0x1020,mp);

    printf("Montgomery r2 mod m Precomputation\n");
    load_data(0xc00,r_inv);
    run(0x12);
    get_data(0x1060,r_sqr);

    load_data(0x440,u1Ax);
    load_data(0x840,u1Ay);

    load_data(0x460,u2Bx);
    load_data(0x860,u2By);

    load_data(0x4c0,a);

    run(0x1c);

    err+=compare_result(0x440,R);




    if(err==0){
        TEST_PASS;
    }else{
        TEST_FAIL;
    }
    /*}}}*/

  return 0;
}


#else
#include "main_qemu.c"
#endif
