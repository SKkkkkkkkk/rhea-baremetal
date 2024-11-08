// GICv3 Physical LPI Example
//
// Copyright (C) Arm Limited, 2019 All rights reserved.
//
// The example code is provided to you as an aid to learning when working
// with Arm-based technology, including but not limited to programming tutorials.
// Arm hereby grants to you, subject to the terms and conditions of this Licence,
// a non-exclusive, non-transferable, non-sub-licensable, free-of-charge licence,
// to use and copy the Software solely for the purpose of demonstration and
// evaluation.
//
// You accept that the Software has not been tested by Arm therefore the Software
// is provided �as is�, without warranty of any kind, express or implied. In no
// event shall the authors or copyright holders be liable for any claim, damages
// or other liability, whether in action or contract, tort or otherwise, arising
// from, out of or in connection with the Software or the use of Software.
//
// ------------------------------------------------------------

#include <stdio.h>
#include "gicv3_basic.h"
#include "gicv3_lpis.h"
#include "memmap.h"
#include "gicv3.h"
#include "common.h"
#include "dw_apb_timers.h"

extern void sendLPI(uint32_t, uint32_t);

extern uint32_t getAffinity(void);
uint32_t initGIC(void);
uint32_t checkGICModel(void);
void d2dClciRegTest(void);

volatile unsigned int flag;

// --------------------------------------------------------

// These locations are based on the memory map of the Base Platform model

#define CONFIG_TABLE      (0x40020000)
#define PENDING_TABLE     (0x40030000)

#define CMD_QUEUE         (0x40040000)
#define DEVICE_TABLE      (0x40050000)
#define COLLECTION_TABLE  (0x40060000)

#define ITT               (0x40070000)

#define D2D_CNOC_BASE           0x9c00000000
#define D2D_MBI_TX_BASE         (D2D_CNOC_BASE + 0x20000000)
#define D2D_CLCI0_APB_BASE      (D2D_CNOC_BASE + 0x20040000)
#define D2D_CLCI2_APB_BASE      (D2D_CNOC_BASE + 0x20080000)
#define D2D_CLCI0_AHB_BASE      (D2D_CNOC_BASE + 0x20200000)
#define D2D_CLCI2_AHB_BASE      (D2D_CNOC_BASE + 0x20600000)
#define D2D_REG_CHANNEL_UP      0x000
#define D2D_REG_DEBUG_IRQ       0x100
#define D2D_REG_MCU_DB2SOC_IRQ  0x104
#define D2D_REG_APB_TEST0       0x200
#define D2D_REG_APB_TEST1       0x204
#define D2D_REG_APB_TEST2       0x208
#define D2D_REG_APB_TEST3       0x20c
#define D2D_REG_AHB_TEST0       0x000
#define D2D_REG_AHB_TEST1       0x004
#define D2D_REG_AHB_TEST2       0x008
#define D2D_REG_AHB_TEST3       0x00c

#ifdef QEMU
  #define DIST_BASE_ADDR    (VIRT_GIC_DIST)
  #define RD_BASE_ADDR      (VIRT_GIC_REDIST)
  #define ITS_BASE_ADDR     (VIRT_GIC_ITS)
#else
  #define ITScount 1
  #define RDcount  4
  #define DIST_BASE_ADDR    (GIC600_BASE)
  #define RD_BASE_ADDR      (GIC600_BASE + ((4 + (2 * ITScount)) << 16))
  #define ITS_BASE_ADDR     (GIC600_BASE + (4 << 16))
#endif

// --------------------------------------------------------

int main(void)
{
  uint32_t type, entry_size;
  uint32_t rd, target_rd;

  d2dClciRegTest();

  //
  // Configure the interrupt controller
  //
  rd = initGIC();
  
  //
  // Set up Redistributor structures used for LPIs
  //

  setLPIConfigTableAddr(rd, CONFIG_TABLE, GICV3_LPI_DEVICE_nGnRnE /*Attributes*/,   \
                        14 /* INTID in [0, 16384), Table Size = 8KB(2^14-8192) */);
  setLPIPendingTableAddr(rd, PENDING_TABLE, GICV3_LPI_DEVICE_nGnRnE /*Attributes*/, \
                        14 /* INTID in [0, 16384), Table Size = 2KB(2^14/8) */);
  enableLPIs(rd);

  //
  // Configure ITS
  //

  setITSBaseAddress((void*)ITS_BASE_ADDR);
  
  // Check that the model has been launched with the correct configuration
  if (checkGICModel() != 0)
    return 1;

  // Allocated memory for the ITS command queue
  initITSCommandQueue(CMD_QUEUE, GICV3_ITS_CQUEUE_VALID /*Attributes*/, 1 /*num_pages*/);

  // Allocate Device table
  setITSTableAddr(0 /*index*/,
                  DEVICE_TABLE /* addr */,
                  (GICV3_ITS_TABLE_PAGE_VALID | GICV3_ITS_TABLE_PAGE_DIRECT | GICV3_ITS_TABLE_PAGE_DEVICE_nGnRnE),
                  GICV3_ITS_TABLE_PAGE_SIZE_4K,
                  16 /*num_pages*/); // 16*4096/8 = 8192 = 2^13 => deviceID width = 13

  //Allocate Collection table
  setITSTableAddr(1 /*index*/,
                  COLLECTION_TABLE /* addr */,
                  (GICV3_ITS_TABLE_PAGE_VALID | GICV3_ITS_TABLE_PAGE_DIRECT | GICV3_ITS_TABLE_PAGE_DEVICE_nGnRnE),
                  GICV3_ITS_TABLE_PAGE_SIZE_4K,
                  16 /*num_pages*/);

  // Enable the ITS
  enableITS();
  
  //
  // Create ITS mapping
  //

  if (getITSPTA() == 1)
  {
     printf("main(): GITS_TYPER.PTA==1, this example expects PTA==0\n");
     return 1;
  }
  target_rd = getRdProcNumber(rd);

  #define D2D_DID 0x38
  #define D2D_CLCI0_DBG_EID 0x20
  #define D2D_CLCI2_DBG_EID 0x22
  #define D2D_CLCI0_MCU_DB2SOC_EID 0x24
  #define D2D_CLCI2_MCU_DB2SOC_EID 0x26
  #define CID 0

  // Set up a mapping
  itsMAPD(D2D_DID /*DeviceID*/, ITT /*addr of ITT*/, 8 /*bit width of ID*/);         // Map a DeviceID to a ITT
  itsMAPTI(D2D_DID /*DeviceID*/, D2D_CLCI0_DBG_EID /*EventID*/, 8192 /*intid*/, CID /*collection*/);   // Map an EventID to an INTD and collection (DeviceID specific)
  itsMAPC(target_rd /* target Redistributor*/, CID /*collection*/);              // Map a Collection to a Redistributor
  itsSYNC(target_rd /* target Redistributor*/);                                // Sync the changes

  itsMAPD(D2D_DID /*DeviceID*/, ITT /*addr of ITT*/, 8 /*bit width of ID*/);         // Map a DeviceID to a ITT
  itsMAPTI(D2D_DID /*DeviceID*/, D2D_CLCI2_DBG_EID /*EventID*/, 8193 /*intid*/, CID /*collection*/);   // Map an EventID to an INTD and collection (DeviceID specific)
  itsMAPC(target_rd /* target Redistributor*/, CID /*collection*/);              // Map a Collection to a Redistributor
  itsSYNC(target_rd /* target Redistributor*/);                                // Sync the changes

  itsMAPD(D2D_DID /*DeviceID*/, ITT /*addr of ITT*/, 8 /*bit width of ID*/);         // Map a DeviceID to a ITT
  itsMAPTI(D2D_DID /*DeviceID*/, D2D_CLCI0_MCU_DB2SOC_EID /*EventID*/, 8194 /*intid*/, CID /*collection*/);   // Map an EventID to an INTD and collection (DeviceID specific)
  itsMAPC(target_rd /* target Redistributor*/, CID /*collection*/);              // Map a Collection to a Redistributor
  itsSYNC(target_rd /* target Redistributor*/);                                // Sync the changes

  itsMAPD(D2D_DID /*DeviceID*/, ITT /*addr of ITT*/, 8 /*bit width of ID*/);         // Map a DeviceID to a ITT
  itsMAPTI(D2D_DID /*DeviceID*/, D2D_CLCI2_MCU_DB2SOC_EID /*EventID*/, 8195 /*intid*/, CID /*collection*/);   // Map an EventID to an INTD and collection (DeviceID specific)
  itsMAPC(target_rd /* target Redistributor*/, CID /*collection*/);              // Map a Collection to a Redistributor
  itsSYNC(target_rd /* target Redistributor*/);                                // Sync the changes

  //
  // Configure and generate an LPI
  //

  configureLPI(rd, 8192 /*INTID*/, GICV3_LPI_ENABLE, 0 /*Priority*/);
  printf("main(): Sending LPI 8192 (D2D_CLCI0_DBG_IRQ)\n");
  itsINV(D2D_DID /*DeviceID*/, D2D_CLCI0_DBG_EID /*EventID*/);

  configureLPI(rd, 8193 /*INTID*/, GICV3_LPI_ENABLE, 0 /*Priority*/);
  printf("main(): Sending LPI 8193 (D2D_CLCI2_DBG_IRQ)\n");
  itsINV(D2D_DID /*DeviceID*/, D2D_CLCI2_DBG_EID /*EventID*/);

  configureLPI(rd, 8194 /*INTID*/, GICV3_LPI_ENABLE, 0 /*Priority*/);
  printf("main(): Sending LPI 8194 (D2D_CLCI0_MCU_DB2SOC_IRQ)\n");
  itsINV(D2D_DID /*DeviceID*/, D2D_CLCI0_MCU_DB2SOC_EID /*EventID*/);

  configureLPI(rd, 8195 /*INTID*/, GICV3_LPI_ENABLE, 0 /*Priority*/);
  printf("main(): Sending LPI 8195 (D2D_CLCI2_MCU_DB2SOC_IRQ)\n");
  itsINV(D2D_DID /*DeviceID*/, D2D_CLCI2_MCU_DB2SOC_EID /*EventID*/);

  // INT
  // itsINT(DID /*DeviceID*/, EID /*EventID*/);
  
  // A55 write
  // WRITE_GITS_TRANSLATER(0x00000001);

  printf("====== INTERRUPT ======\n");
  REG32(D2D_MBI_TX_BASE + 0x10) = (uint64_t)0x105f0040;
  REG32(D2D_MBI_TX_BASE + 0x14) = (uint64_t)0x04;
  REG32(D2D_MBI_TX_BASE + 0x40) = 0x0;
  REG32(D2D_MBI_TX_BASE + 0x44) = 0x0;
  REG32(D2D_MBI_TX_BASE + 0x30) = 0x0;
  REG32(D2D_MBI_TX_BASE + 0x34) = 0x55;

  REG32(D2D_CLCI0_APB_BASE + D2D_REG_DEBUG_IRQ) = 0x1;
  REG32(D2D_CLCI2_APB_BASE + D2D_REG_DEBUG_IRQ) = 0x1;
  REG32(D2D_CLCI0_APB_BASE + D2D_REG_MCU_DB2SOC_IRQ) = 0x1;
  REG32(D2D_CLCI2_APB_BASE + D2D_REG_MCU_DB2SOC_IRQ) = 0x1;

  // NOTE:
  // This code assumes that the IRQ and FIQ exceptions
  // have been routed to the appropriate Exception level
  // and that the PSTATE masks are clear.  In this example
  // this is done in the startup.s file

  //
  // Spin until interrupt
  //
  while(flag < 1)
  {}
  
  printf("Main(): Test end\n");

  return 1;
}

// --------------------------------------------------------

void fiqHandler(void)
{
  uint32_t ID;
  uint32_t group = 0;

  // Read the IAR to get the INTID of the interrupt taken
  ID = readIARGrp0();

  printf("FIQ: Received INTID %d\n", ID);

  switch (ID)
  {
    case 1021:
      printf("FIQ: Received Non-secure interrupt from the ITS\n");
      ID = readIARGrp1();
      printf("FIQ: Read INTID %d from IAR1\n", ID);
      group = 1;
      break;
    case 1023:
      printf("FIQ: Interrupt was spurious\n");
      return;
    default:
      printf("FIQ: Panic, unexpected INTID\n");
  }

  // Write EOIR to deactivate interrupt
  if (group == 0)
    writeEOIGrp0(ID);
  else
    writeEOIGrp1(ID);

  flag++;
  return;
}

// --------------------------------------------------------

uint32_t initGIC(void)
{
  uint32_t rd = 0;

  // Set location of GIC
  setGICAddr((void*)DIST_BASE_ADDR, (void*)RD_BASE_ADDR);
  GIC_Init();

  // Enable GIC
  // enableGIC();

  // // Get the ID of the Redistributor connected to this PE
  // rd = getRedistID(getAffinity());

  // // Mark this core as beign active
  // wakeUpRedist(rd);

  // // Configure the CPU interface
  // // This assumes that the SRE bits are already set
  // setPriorityMask(0xFF);
  // enableGroup0Ints();
  // enableGroup1Ints();
  // enableNSGroup1Ints();  // This call only works as example runs at EL3

  return rd;
}

// --------------------------------------------------------

//
// This function checks the model has been configured the way the example expects
//

uint32_t checkGICModel(void)
{
  uint32_t type, entry_size;

  //
  // Check that LPIs supported
  //
  if (getMaxLPI(0) == 0)
  {
     printf("checkGICModel(): Physical LPIs not supported\n");
     return 1;
  }

  //
  // Check the model used to identify RD's in ITS commands
  //
  if (getITSPTA() == 1)
  {
     printf("checkGICModel(): GITS_TYPER.PTA==1, this example expects PTA==0\n");
     return 1;
  }

  //
  // Check the GITS_BASER<n> types
  //
  getITSTableType(0 /*index*/, &type, &entry_size);
  if (type != GICV3_ITS_TABLE_TYPE_DEVICE)
  {
    printf("checkGICModel() - GITS_BASER0 not expected value (seeing 0x%x, expected 0x%x).\n", type, GICV3_ITS_TABLE_TYPE_DEVICE);
    return 1;
  }

  getITSTableType(1 /*index*/, &type, &entry_size);
  if (type != GICV3_ITS_TABLE_TYPE_COLLECTION)
  {
    printf("checkGICModel() - GITS_BASER1 not expected value (seeing 0x%x, expected 0x%x).\n", type, GICV3_ITS_TABLE_TYPE_COLLECTION);
    return 1;
  }

  return 0;
}

// --------------------------------------------------------

void d2dClciRegTest(void)
{
  printf("====== CLCI0 ======\n");

  printf("d2d clci0 channel_up 0x%x\n", REG32(D2D_CLCI0_APB_BASE + D2D_REG_CHANNEL_UP));

  printf("d2d clci0 test_reg0 0x%x\n", REG32(D2D_CLCI0_APB_BASE + D2D_REG_APB_TEST0));
  printf("d2d clci0 test_reg1 0x%x\n", REG32(D2D_CLCI0_APB_BASE + D2D_REG_APB_TEST1));
  printf("d2d clci0 test_reg2 0x%x\n", REG32(D2D_CLCI0_APB_BASE + D2D_REG_APB_TEST2));
  printf("d2d clci0 test_reg3 0x%x\n", REG32(D2D_CLCI0_APB_BASE + D2D_REG_APB_TEST3));

  printf("d2d clci0 ahb_test0 0x%x\n", REG32(D2D_CLCI0_AHB_BASE + D2D_REG_AHB_TEST0));
  printf("d2d clci0 ahb_test1 0x%x\n", REG32(D2D_CLCI0_AHB_BASE + D2D_REG_AHB_TEST1));
  printf("d2d clci0 ahb_test2 0x%x\n", REG32(D2D_CLCI0_AHB_BASE + D2D_REG_AHB_TEST2));
  printf("d2d clci0 ahb_test3 0x%x\n", REG32(D2D_CLCI0_AHB_BASE + D2D_REG_AHB_TEST3));

  printf("=== modify ===\n");

  REG32(D2D_CLCI0_APB_BASE + D2D_REG_APB_TEST0) = 0x01010101;
  printf("d2d clci0 test_reg0 0x%x\n", REG32(D2D_CLCI0_APB_BASE + D2D_REG_APB_TEST0));
  REG32(D2D_CLCI0_APB_BASE + D2D_REG_APB_TEST1) = 0x02020202;
  printf("d2d clci0 test_reg1 0x%x\n", REG32(D2D_CLCI0_APB_BASE + D2D_REG_APB_TEST1));
  REG32(D2D_CLCI0_APB_BASE + D2D_REG_APB_TEST2) = 0x03030303;
  printf("d2d clci0 test_reg2 0x%x\n", REG32(D2D_CLCI0_APB_BASE + D2D_REG_APB_TEST2));
  REG32(D2D_CLCI0_APB_BASE + D2D_REG_APB_TEST3) = 0x04040404;
  printf("d2d clci0 test_reg3 0x%x\n", REG32(D2D_CLCI0_APB_BASE + D2D_REG_APB_TEST3));

  REG32(D2D_CLCI0_AHB_BASE + D2D_REG_AHB_TEST0) = 0x05050505;
  printf("d2d clci0 ahb_test0 0x%x\n", REG32(D2D_CLCI0_AHB_BASE + D2D_REG_AHB_TEST0));
  REG32(D2D_CLCI0_AHB_BASE + D2D_REG_AHB_TEST1) = 0x06060606;
  printf("d2d clci0 ahb_test1 0x%x\n", REG32(D2D_CLCI0_AHB_BASE + D2D_REG_AHB_TEST1));
  REG32(D2D_CLCI0_AHB_BASE + D2D_REG_AHB_TEST2) = 0x07070707;
  printf("d2d clci0 ahb_test2 0x%x\n", REG32(D2D_CLCI0_AHB_BASE + D2D_REG_AHB_TEST2));
  REG32(D2D_CLCI0_AHB_BASE + D2D_REG_AHB_TEST3) = 0x08080808;
  printf("d2d clci0 ahb_test3 0x%x\n", REG32(D2D_CLCI0_AHB_BASE + D2D_REG_AHB_TEST3));

  printf("====== CLCI2 ======\n");

  printf("d2d clci2 channel_up 0x%x\n", REG32(D2D_CLCI2_APB_BASE + D2D_REG_CHANNEL_UP));

  printf("d2d clci2 test_reg0 0x%x\n", REG32(D2D_CLCI2_APB_BASE + D2D_REG_APB_TEST0));
  printf("d2d clci2 test_reg1 0x%x\n", REG32(D2D_CLCI2_APB_BASE + D2D_REG_APB_TEST1));
  printf("d2d clci2 test_reg2 0x%x\n", REG32(D2D_CLCI2_APB_BASE + D2D_REG_APB_TEST2));
  printf("d2d clci2 test_reg3 0x%x\n", REG32(D2D_CLCI2_APB_BASE + D2D_REG_APB_TEST3));

  printf("d2d clci2 ahb_test0 0x%x\n", REG32(D2D_CLCI2_AHB_BASE + D2D_REG_AHB_TEST0));
  printf("d2d clci2 ahb_test1 0x%x\n", REG32(D2D_CLCI2_AHB_BASE + D2D_REG_AHB_TEST1));
  printf("d2d clci2 ahb_test2 0x%x\n", REG32(D2D_CLCI2_AHB_BASE + D2D_REG_AHB_TEST2));
  printf("d2d clci2 ahb_test3 0x%x\n", REG32(D2D_CLCI2_AHB_BASE + D2D_REG_AHB_TEST3));

  printf("=== modify ===\n");

  REG32(D2D_CLCI2_APB_BASE + D2D_REG_APB_TEST0) = 0x01010101;
  printf("d2d clci2 test_reg0 0x%x\n", REG32(D2D_CLCI2_APB_BASE + D2D_REG_APB_TEST0));
  REG32(D2D_CLCI2_APB_BASE + D2D_REG_APB_TEST1) = 0x02020202;
  printf("d2d clci2 test_reg1 0x%x\n", REG32(D2D_CLCI2_APB_BASE + D2D_REG_APB_TEST1));
  REG32(D2D_CLCI2_APB_BASE + D2D_REG_APB_TEST2) = 0x03030303;
  printf("d2d clci2 test_reg2 0x%x\n", REG32(D2D_CLCI2_APB_BASE + D2D_REG_APB_TEST2));
  REG32(D2D_CLCI2_APB_BASE + D2D_REG_APB_TEST3) = 0x04040404;
  printf("d2d clci2 test_reg3 0x%x\n", REG32(D2D_CLCI2_APB_BASE + D2D_REG_APB_TEST3));

  REG32(D2D_CLCI2_AHB_BASE + D2D_REG_AHB_TEST0) = 0x05050505;
  printf("d2d clci2 ahb_test0 0x%x\n", REG32(D2D_CLCI2_AHB_BASE + D2D_REG_AHB_TEST0));
  REG32(D2D_CLCI2_AHB_BASE + D2D_REG_AHB_TEST1) = 0x06060606;
  printf("d2d clci2 ahb_test1 0x%x\n", REG32(D2D_CLCI2_AHB_BASE + D2D_REG_AHB_TEST1));
  REG32(D2D_CLCI2_AHB_BASE + D2D_REG_AHB_TEST2) = 0x07070707;
  printf("d2d clci2 ahb_test2 0x%x\n", REG32(D2D_CLCI2_AHB_BASE + D2D_REG_AHB_TEST2));
  REG32(D2D_CLCI2_AHB_BASE + D2D_REG_AHB_TEST3) = 0x08080808;
  printf("d2d clci2 ahb_test3 0x%x\n", REG32(D2D_CLCI2_AHB_BASE + D2D_REG_AHB_TEST3));
}
