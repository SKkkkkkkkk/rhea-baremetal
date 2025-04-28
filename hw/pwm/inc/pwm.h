#ifndef __PWM_H__
#define __PWM_H__

#include <stdint.h>
#include "common.h"
#include "memmap.h"

////////////////////////////////////////////////////////////////
// baisc reg info                                             //
////////////////////////////////////////////////////////////////
#define PWM_UPDATE_ADDR                   (PWM_BASE+0x000)
#define PWM_ENABLE_STATE_ADDR             (PWM_BASE+0x000)
#define PWM_CAP_DMA_EN_ADDR               (PWM_BASE+0x004)
#define PWM_CAP_DMA_EN_LSB                0
#define PWM_CAP_DMA_EN_MASK               0x00000001
#define PWM_CAP_DMA_SEL_ADDR              (PWM_BASE+0x004)
#define PWM_CAP_DMA_SEL_LSB               1
#define PWM_CAP_DMA_SEL_MASK              0x00000002
#define PWM_PWM_OUT_SEL_ADDR              (PWM_BASE+0x004)
#define PWM_PWM_OUT_SEL_LSB               2
#define PWM_PWM_OUT_SEL_MASK              0x00000004
#define PWM_CAP_DMA_FIFO_ADDR             (PWM_BASE+0x008)
#define PWM_CAP_DMA_FIFO_THR_ADDR         (PWM_BASE+0x00c)
#define PWM_CAP_DMA_FIFO_OVF_ADDR         (PWM_BASE+0x010)
#define PWM_CAP_DMA_FIFO_TMOT_THR_ADDR    (PWM_BASE+0x014)
#define PWM_CAP_DMA_FIFO_TMOT_THR_LSB     0
#define PWM_CAP_DMA_FIFO_TMOT_THR_MASK    0x7fffffff
#define PWM_CAP_DMA_FIFO_TMOT_EN_ADDR     (PWM_BASE+0x014)
#define PWM_CAP_DMA_FIFO_TMOT_EN_LSB      31
#define PWM_CAP_DMA_FIFO_TMOT_EN_MASK     0x80000000
#define PWM_CAP_DMA_FIFO_TMOT_ADDR        (PWM_BASE+0x018)
#define PWM_PWM_TZ_CONF_ADDR              (PWM_BASE+0x01c)
#define PWM_INTR_STATUS_ADDR              (PWM_BASE+0x020)
#define PWM_INTR_CLR_ADDR                 (PWM_BASE+0x020)
#define PWM_INTR_MSK_ADDR                 (PWM_BASE+0x024)
#define PWM_INTR_EN_ADDR                  (PWM_BASE+0x028)
#define PWM_A_CONF_ADDR                   (PWM_BASE+0x100)
#define PWM_A_TIMER_LAT_EN_ADDR           (PWM_BASE+0x104)
#define PWM_A_TIMER_LAT_CLR_ADDR          (PWM_BASE+0x108)
#define PWM_A_TIMER_UNIT_ADDR             (PWM_BASE+0x10c)
#define PWM_A_TIMER_CNT_ADDR              (PWM_BASE+0x110)
#define PWM_A_TIMER_LAT_ADDR              (PWM_BASE+0x114)
#define PWM_A_TIMER_PRD_ADDR              (PWM_BASE+0x118)
#define PWM_A_TIMER_CMP0_ADDR             (PWM_BASE+0x11c)
#define PWM_A_TIMER_CMP1_ADDR             (PWM_BASE+0x120)
#define PWM_A_DB_UPDLY_ADDR               (PWM_BASE+0x124)
#define PWM_A_DB_DNDLY_ADDR               (PWM_BASE+0x128)
#define PWM_A_CHP_OSHT_ADDR               (PWM_BASE+0x12c)
#define PWM_A_CHP_LLVLT_ADDR              (PWM_BASE+0x130)
#define PWM_A_CHP_PRD_ADDR                (PWM_BASE+0x134)
#define PWM_A_CAP_HPC_ADDR                (PWM_BASE+0x138)
#define PWM_A_CAP_LPC_ADDR                (PWM_BASE+0x13c)
#define PWM_B_CONF_ADDR                   (PWM_BASE+0x200)
#define PWM_B_TIMER_LAT_EN_ADDR           (PWM_BASE+0x204)
#define PWM_B_TIMER_LAT_CLR_ADDR          (PWM_BASE+0x208)
#define PWM_B_TIMER_UNIT_ADDR             (PWM_BASE+0x20c)
#define PWM_B_TIMER_CNT_ADDR              (PWM_BASE+0x210)
#define PWM_B_TIMER_LAT_ADDR              (PWM_BASE+0x214)
#define PWM_B_TIMER_PRD_ADDR              (PWM_BASE+0x218)
#define PWM_B_TIMER_CMP0_ADDR             (PWM_BASE+0x21c)
#define PWM_B_TIMER_CMP1_ADDR             (PWM_BASE+0x220)
#define PWM_B_CHP_OSHT_ADDR               (PWM_BASE+0x22c)
#define PWM_B_CHP_LLVLT_ADDR              (PWM_BASE+0x230)
#define PWM_B_CHP_PRD_ADDR                (PWM_BASE+0x234)
#define PWM_B_CAP_HPC_ADDR                (PWM_BASE+0x238)
#define PWM_B_CAP_LPC_ADDR                (PWM_BASE+0x23c)

////////////////////////////////////////////////////////////////
// reg access                                                 //
////////////////////////////////////////////////////////////////
#define PWM_REG32(name)                   (*(volatile uint32_t*)name##_ADDR)

#define PWM_SET_UPDATE(x)                 PWM_REG32(PWM_UPDATE) = (uint32_t)(x)
#define PWM_GET_ENABLE_STATE              PWM_REG32(PWM_ENABLE_STATE)
#define PWM_GET_CAP_DMA_FIFO              PWM_REG32(PWM_CAP_DMA_FIFO)
#define PWM_GET_CAP_DMA_FIFO_THR          PWM_REG32(PWM_CAP_DMA_FIFO_THR)
#define PWM_SET_CAP_DMA_FIFO_THR(x)       PWM_REG32(PWM_CAP_DMA_FIFO_THR) = (uint32_t)(x)
#define PWM_GET_CAP_DMA_FIFO_OVF          PWM_REG32(PWM_CAP_DMA_FIFO_OVF)
#define PWM_GET_CAP_DMA_FIFO_TMOT         PWM_REG32(PWM_CAP_DMA_FIFO_TMOT)
#define PWM_GET_PWM_TZ_CONF               PWM_REG32(PWM_PWM_TZ_CONF)
#define PWM_SET_PWM_TZ_CONF(x)            PWM_REG32(PWM_PWM_TZ_CONF) = (uint32_t)(x)
#define PWM_GET_INTR_STATUS               PWM_REG32(PWM_INTR_STATUS)
#define PWM_SET_INTR_CLR(x)               PWM_REG32(PWM_INTR_CLR) = (uint32_t)(x)
#define PWM_GET_INTR_MSK                  PWM_REG32(PWM_INTR_MSK)
#define PWM_SET_INTR_MSK(x)               PWM_REG32(PWM_INTR_MSK) = (uint32_t)(x)
#define PWM_GET_INTR_EN                   PWM_REG32(PWM_INTR_EN)
#define PWM_SET_INTR_EN(x)                PWM_REG32(PWM_INTR_EN) = (uint32_t)(x)
#define PWM_GET_A_CONF                    PWM_REG32(PWM_A_CONF)
#define PWM_SET_A_CONF(x)                 PWM_REG32(PWM_A_CONF) = (uint32_t)(x)
#define PWM_SET_A_TIMER_LAT_EN(x)         PWM_REG32(PWM_A_TIMER_LAT_EN) = (uint32_t)(x)
#define PWM_SET_A_TIMER_LAT_CLR(x)        PWM_REG32(PWM_A_TIMER_LAT_CLR) = (uint32_t)(x)
#define PWM_GET_A_TIMER_UNIT              PWM_REG32(PWM_A_TIMER_UNIT)
#define PWM_SET_A_TIMER_UNIT(x)           PWM_REG32(PWM_A_TIMER_UNIT) = (uint32_t)(x)
#define PWM_GET_A_TIMER_CNT               PWM_REG32(PWM_A_TIMER_CNT)
#define PWM_GET_A_TIMER_LAT               PWM_REG32(PWM_A_TIMER_LAT)
#define PWM_GET_A_TIMER_PRD               PWM_REG32(PWM_A_TIMER_PRD)
#define PWM_SET_A_TIMER_PRD(x)            PWM_REG32(PWM_A_TIMER_PRD) = (uint32_t)(x)
#define PWM_GET_A_TIMER_CMP0              PWM_REG32(PWM_A_TIMER_CMP0)
#define PWM_SET_A_TIMER_CMP0(x)           PWM_REG32(PWM_A_TIMER_CMP0) = (uint32_t)(x)
#define PWM_GET_A_TIMER_CMP1              PWM_REG32(PWM_A_TIMER_CMP1)
#define PWM_SET_A_TIMER_CMP1(x)           PWM_REG32(PWM_A_TIMER_CMP1) = (uint32_t)(x)
#define PWM_GET_A_DB_UPDLY                PWM_REG32(PWM_A_DB_UPDLY)
#define PWM_SET_A_DB_UPDLY(x)             PWM_REG32(PWM_A_DB_UPDLY) = (uint32_t)(x)
#define PWM_GET_A_DB_DNDLY                PWM_REG32(PWM_A_DB_DNDLY)
#define PWM_SET_A_DB_DNDLY(x)             PWM_REG32(PWM_A_DB_DNDLY) = (uint32_t)(x)
#define PWM_GET_A_CHP_OSHT                PWM_REG32(PWM_A_CHP_OSHT)
#define PWM_SET_A_CHP_OSHT(x)             PWM_REG32(PWM_A_CHP_OSHT) = (uint32_t)(x)
#define PWM_GET_A_CHP_LLVLT               PWM_REG32(PWM_A_CHP_LLVLT)
#define PWM_SET_A_CHP_LLVLT(x)            PWM_REG32(PWM_A_CHP_LLVLT) = (uint32_t)(x)
#define PWM_GET_A_CHP_PRD                 PWM_REG32(PWM_A_CHP_PRD)
#define PWM_SET_A_CHP_PRD(x)              PWM_REG32(PWM_A_CHP_PRD) = (uint32_t)(x)
#define PWM_GET_A_CAP_HPC                 PWM_REG32(PWM_A_CAP_HPC)
#define PWM_GET_A_CAP_LPC                 PWM_REG32(PWM_A_CAP_LPC)
#define PWM_GET_B_CONF                    PWM_REG32(PWM_B_CONF)
#define PWM_SET_B_CONF(x)                 PWM_REG32(PWM_B_CONF) = (uint32_t)(x)
#define PWM_SET_B_TIMER_LAT_EN(x)         PWM_REG32(PWM_B_TIMER_LAT_EN) = (uint32_t)(x)
#define PWM_SET_B_TIMER_LAT_CLR(x)        PWM_REG32(PWM_B_TIMER_LAT_CLR) = (uint32_t)(x)
#define PWM_GET_B_TIMER_UNIT              PWM_REG32(PWM_B_TIMER_UNIT)
#define PWM_SET_B_TIMER_UNIT(x)           PWM_REG32(PWM_B_TIMER_UNIT) = (uint32_t)(x)
#define PWM_GET_B_TIMER_CNT               PWM_REG32(PWM_B_TIMER_CNT)
#define PWM_GET_B_TIMER_LAT               PWM_REG32(PWM_B_TIMER_LAT)
#define PWM_GET_B_TIMER_PRD               PWM_REG32(PWM_B_TIMER_PRD)
#define PWM_SET_B_TIMER_PRD(x)            PWM_REG32(PWM_B_TIMER_PRD) = (uint32_t)(x)
#define PWM_GET_B_TIMER_CMP0              PWM_REG32(PWM_B_TIMER_CMP0)
#define PWM_SET_B_TIMER_CMP0(x)           PWM_REG32(PWM_B_TIMER_CMP0) = (uint32_t)(x)
#define PWM_GET_B_TIMER_CMP1              PWM_REG32(PWM_B_TIMER_CMP1)
#define PWM_SET_B_TIMER_CMP1(x)           PWM_REG32(PWM_B_TIMER_CMP1) = (uint32_t)(x)
#define PWM_GET_B_CHP_OSHT                PWM_REG32(PWM_B_CHP_OSHT)
#define PWM_SET_B_CHP_OSHT(x)             PWM_REG32(PWM_B_CHP_OSHT) = (uint32_t)(x)
#define PWM_GET_B_CHP_LLVLT               PWM_REG32(PWM_B_CHP_LLVLT)
#define PWM_SET_B_CHP_LLVLT(x)            PWM_REG32(PWM_B_CHP_LLVLT) = (uint32_t)(x)
#define PWM_GET_B_CHP_PRD                 PWM_REG32(PWM_B_CHP_PRD)
#define PWM_SET_B_CHP_PRD(x)              PWM_REG32(PWM_B_CHP_PRD) = (uint32_t)(x)
#define PWM_GET_B_CAP_HPC                 PWM_REG32(PWM_B_CAP_HPC)
#define PWM_GET_B_CAP_LPC                 PWM_REG32(PWM_B_CAP_LPC)

////////////////////////////////////////////////////////////////
// reg bits access                                            //
////////////////////////////////////////////////////////////////
#define PWM_GET_REG32_BITS(name)          ((PWM_REG32(name) & name##_MASK) >> name##_LSB)
#define PWM_SET_REG32_BITS(name,x)        do { \
    uint32_t val = PWM_REG32(name); \
    val &= ~name##_MASK; \
    val |= ((x) << name##_LSB) & name##_MASK; \
    PWM_REG32(name) = val; \
} while(0)

#define PWM_GET_CAP_DMA_EN                PWM_GET_REG32_BITS(PWM_CAP_DMA_EN)
#define PWM_SET_CAP_DMA_EN(x)             PWM_SET_REG32_BITS(PWM_CAP_DMA_EN,x)
#define PWM_GET_CAP_DMA_SEL               PWM_GET_REG32_BITS(PWM_CAP_DMA_SEL)
#define PWM_SET_CAP_DMA_SEL(x)            PWM_SET_REG32_BITS(PWM_CAP_DMA_SEL,x)
#define PWM_GET_PWM_OUT_SEL               PWM_GET_REG32_BITS(PWM_PWM_OUT_SEL)
#define PWM_SET_PWM_OUT_SEL(x)            PWM_SET_REG32_BITS(PWM_PWM_OUT_SEL,x)
#define PWM_GET_CAP_DMA_FIFO_TMOT_THR     PWM_GET_REG32_BITS(PWM_CAP_DMA_FIFO_TMOT_THR)
#define PWM_SET_CAP_DMA_FIFO_TMOT_THR(x)  PWM_SET_REG32_BITS(PWM_CAP_DMA_FIFO_TMOT_THR,x)
#define PWM_GET_CAP_DMA_FIFO_TMOT_EN      PWM_GET_REG32_BITS(PWM_CAP_DMA_FIFO_TMOT_EN)
#define PWM_SET_CAP_DMA_FIFO_TMOT_EN(x)   PWM_SET_REG32_BITS(PWM_CAP_DMA_FIFO_TMOT_EN,x)

////////////////////////////////////////////////////////////////
// reg group                                                  //
////////////////////////////////////////////////////////////////
typedef struct {
    // 0x000
    uint32_t update;
    // 0x004
    uint32_t cap_dma_en            :  1;  //  0 :  0
    uint32_t cap_dma_sel           :  1;  //  1 :  1
    uint32_t pwm_out_sel           :  1;  //  2 :  2
    uint32_t _reserved0            : 29;  // 31 :  3
    // 0x008
    uint32_t cap_dma_fifo;
    // 0x00c
    uint32_t cap_dma_fifo_thr;
    // 0x010
    uint32_t cap_dma_fifo_ovf;
    // 0x014
    uint32_t cap_dma_fifo_tmot_thr : 31;  // 30 :  0
    uint32_t cap_dma_fifo_tmot_en  :  1;  // 31 : 31
    // 0x018
    uint32_t cap_dma_fifo_tmot;
    // 0x01c
    uint32_t cap_dma_ready;
    // 0x020
    uint32_t pwm_tz_conf;
    // 0x024
    uint32_t intr_status;
    // 0x028
    uint32_t intr_msk;
    // 0x02c
    uint32_t intr_en;
    RESERVED(0[52], uint32_t); //0x030 ~ 0x0fc
    // 0x100
    uint32_t a_conf;
    // 0x104
    uint32_t a_timer_lat_en;
    // 0x108
    uint32_t a_timer_lat_clr;
    // 0x10c
    uint32_t a_timer_unit;
    // 0x110
    uint32_t a_timer_cnt;
    // 0x114
    uint32_t a_timer_lat;
    // 0x118
    uint32_t a_timer_prd;
    // 0x11c
    uint32_t a_timer_cmp0;
    // 0x120
    uint32_t a_timer_cmp1;
    // 0x124
    uint32_t a_db_updly;
    // 0x128
    uint32_t a_db_dndly;
    // 0x12c
    uint32_t a_chp_osht;
    // 0x130
    uint32_t a_chp_llvlt;
    // 0x134
    uint32_t a_chp_prd;
    // 0x138
    uint32_t a_cap_hpc;
    // 0x13c
    uint32_t a_cap_lpc;
    RESERVED(1[48], uint32_t); //0x140 ~ 0x1fc
    // 0x200
    uint32_t b_conf;
    // 0x204
    uint32_t b_timer_lat_en;
    // 0x208
    uint32_t b_timer_lat_clr;
    // 0x20c
    uint32_t b_timer_unit;
    // 0x210
    uint32_t b_timer_cnt;
    // 0x214
    uint32_t b_timer_lat;
    // 0x218
    uint32_t b_timer_prd;
    // 0x21c
    uint32_t b_timer_cmp0;
    // 0x220
    uint32_t b_timer_cmp1;
    RESERVED(2[2], uint32_t); //0x224 ~ 0x228
    // 0x22c
    uint32_t b_chp_osht;
    // 0x230
    uint32_t b_chp_llvlt;
    // 0x234
    uint32_t b_chp_prd;
    // 0x238
    uint32_t b_cap_hpc;
    // 0x23c
    uint32_t b_cap_lpc;
} pwm_regs;

// 定义寄存器位域掩码
#define A_MODE_MASK              0x3
#define A_CAP_PON_MASK           0x1
#define A_OUT_CNT_DIR_MASK       0x3
#define A_OUT_ACT_CNT_ONE_MASK   0x3
#define A_OUT_ACT_CMP0_UP_MASK   0x3
#define A_OUT_ACT_CMP0_DOWN_MASK 0x3
#define A_OUT_ACT_CMP1_UP_MASK   0x3
#define A_OUT_ACT_CMP1_DOWN_MASK 0x3
#define A_OUT_ACT_CNT_PRD_MASK   0x3
#define A_OUT_AB_MODE_MASK       0x3
#define A_OUT_OSHT_NUM_MASK      0x1FF
#define A_DB_EN_MASK             0x1
#define A_CHP_EN_MASK            0x1

// 定义寄存器位域偏移
#define A_MODE_SHIFT              0
#define A_CAP_PON_SHIFT           2
#define A_OUT_CNT_DIR_SHIFT       3
#define A_OUT_ACT_CNT_ONE_SHIFT   5
#define A_OUT_ACT_CMP0_UP_SHIFT   7
#define A_OUT_ACT_CMP0_DOWN_SHIFT 9
#define A_OUT_ACT_CMP1_UP_SHIFT   11
#define A_OUT_ACT_CMP1_DOWN_SHIFT 13
#define A_OUT_ACT_CNT_PRD_SHIFT   15
#define A_OUT_AB_MODE_SHIFT       17
#define A_OUT_OSHT_NUM_SHIFT      19
#define A_DB_EN_SHIFT             29
#define A_CHP_EN_SHIFT            30

// 定义寄存器位域掩码
#define B_MODE_MASK              0x3
#define B_CAP_PON_MASK           0x1
#define B_OUT_CNT_DIR_MASK       0x3
#define B_OUT_ACT_CNT_ONE_MASK   0x3
#define B_OUT_ACT_CMP0_UP_MASK   0x3
#define B_OUT_ACT_CMP0_DOWN_MASK 0x3
#define B_OUT_ACT_CMP1_UP_MASK   0x3
#define B_OUT_ACT_CMP1_DOWN_MASK 0x3
#define B_OUT_ACT_CNT_PRD_MASK   0x3
#define B_OUT_AB_MODE_MASK       0x3
#define B_OUT_OSHT_NUM_MASK      0x1FF
#define B_DB_EN_MASK             0x1
#define B_CHP_EN_MASK            0x1

// 定义寄存器位域偏移
#define B_MODE_SHIFT              0
#define B_CAP_PON_SHIFT           2
#define B_OUT_CNT_DIR_SHIFT       3
#define B_OUT_ACT_CNT_ONE_SHIFT   5
#define B_OUT_ACT_CMP0_UP_SHIFT   7
#define B_OUT_ACT_CMP0_DOWN_SHIFT 9
#define B_OUT_ACT_CMP1_UP_SHIFT   11
#define B_OUT_ACT_CMP1_DOWN_SHIFT 13
#define B_OUT_ACT_CNT_PRD_SHIFT   15
#define B_OUT_AB_MODE_SHIFT       17
#define B_OUT_OSHT_NUM_SHIFT      19
#define B_DB_EN_SHIFT             29
#define B_CHP_EN_SHIFT            30

#define CHANNEL_A_STATE_MASK      0x1
#define CHANNEL_B_STATE_MASK      0x1

void configurePwmAConf(pwm_regs* PWM ,uint32_t mode, uint32_t cap_pon, uint32_t out_cnt_dir, uint32_t out_act_cnt_one, 
    uint32_t out_act_cmp0_up, uint32_t out_act_cmp0_down, uint32_t out_act_cmp1_up, uint32_t out_act_cmp1_down,
    uint32_t out_act_cnt_prd, uint32_t out_ab_mode, uint32_t out_osht_num, uint32_t db_en, uint32_t chp_en);

void configurePwmBConf(pwm_regs* PWM ,uint32_t mode, uint32_t cap_pon, uint32_t out_cnt_dir, uint32_t out_act_cnt_one, 
    uint32_t out_act_cmp0_up, uint32_t out_act_cmp0_down, uint32_t out_act_cmp1_up, uint32_t out_act_cmp1_down,
    uint32_t out_act_cnt_prd, uint32_t out_ab_mode, uint32_t out_osht_num, uint32_t db_en, uint32_t chp_en);

void configurePwmAEnableState(pwm_regs* PWM ,uint16_t channelState);
void configurePwmBEnableState(pwm_regs* PWM ,uint16_t channelState);
void configureDMARegisters(pwm_regs* PWM ,uint32_t capDmaEn, uint32_t capDmaSel, uint32_t pwmOutSel);

#endif // __PWM_H__
