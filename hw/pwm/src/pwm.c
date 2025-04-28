#include "pwm.h"

// 配置通道A工作参数
void configurePwmAConf(pwm_regs* PWM, uint32_t mode, uint32_t cap_pon, uint32_t out_cnt_dir, uint32_t out_act_cnt_one,
    uint32_t out_act_cmp0_up, uint32_t out_act_cmp0_down, uint32_t out_act_cmp1_up, uint32_t out_act_cmp1_down,
    uint32_t out_act_cnt_prd, uint32_t out_ab_mode, uint32_t out_osht_num, uint32_t db_en, uint32_t chp_en)
{
    uint32_t regValue = PWM->a_conf;

    regValue |= ((mode              & A_MODE_MASK             ) << A_MODE_SHIFT)              |
                ((cap_pon           & A_CAP_PON_MASK          ) << A_CAP_PON_SHIFT)           |
                ((out_cnt_dir       & A_OUT_CNT_DIR_MASK      ) << A_OUT_CNT_DIR_SHIFT)       |
                ((out_act_cnt_one   & A_OUT_ACT_CNT_ONE_MASK  ) << A_OUT_ACT_CNT_ONE_SHIFT)   |
                ((out_act_cmp0_up   & A_OUT_ACT_CMP0_UP_MASK  ) << A_OUT_ACT_CMP0_UP_SHIFT)   |
                ((out_act_cmp0_down & A_OUT_ACT_CMP0_DOWN_MASK) << A_OUT_ACT_CMP0_DOWN_SHIFT) |
                ((out_act_cmp1_up   & A_OUT_ACT_CMP1_UP_MASK  ) << A_OUT_ACT_CMP1_UP_SHIFT)   |
                ((out_act_cmp1_down & A_OUT_ACT_CMP1_DOWN_MASK) << A_OUT_ACT_CMP1_DOWN_SHIFT) |
                ((out_act_cnt_prd   & A_OUT_ACT_CNT_PRD_MASK  ) << A_OUT_ACT_CNT_PRD_SHIFT)   |
                ((out_ab_mode       & A_OUT_AB_MODE_MASK      ) << A_OUT_AB_MODE_SHIFT)       |
                ((out_osht_num      & A_OUT_OSHT_NUM_MASK     ) << A_OUT_OSHT_NUM_SHIFT)      |
                ((db_en             & A_DB_EN_MASK            ) << A_DB_EN_SHIFT)             |
                ((chp_en            & A_CHP_EN_MASK           ) << A_CHP_EN_SHIFT);

    PWM->a_conf = regValue;
}

// 配置通道B工作参数
void configurePwmBConf(pwm_regs* PWM, uint32_t mode, uint32_t cap_pon, uint32_t out_cnt_dir, uint32_t out_act_cnt_one, 
    uint32_t out_act_cmp0_up, uint32_t out_act_cmp0_down, uint32_t out_act_cmp1_up, uint32_t out_act_cmp1_down,
    uint32_t out_act_cnt_prd, uint32_t out_ab_mode, uint32_t out_osht_num, uint32_t db_en, uint32_t chp_en)
{

    uint32_t regValue = PWM->b_conf;

    regValue |= ((mode              & B_MODE_MASK             ) << B_MODE_SHIFT)              |
                ((cap_pon           & B_CAP_PON_MASK          ) << B_CAP_PON_SHIFT)           |
                ((out_cnt_dir       & B_OUT_CNT_DIR_MASK      ) << B_OUT_CNT_DIR_SHIFT)       |
                ((out_act_cnt_one   & B_OUT_ACT_CNT_ONE_MASK  ) << B_OUT_ACT_CNT_ONE_SHIFT)   |
                ((out_act_cmp0_up   & B_OUT_ACT_CMP0_UP_MASK  ) << B_OUT_ACT_CMP0_UP_SHIFT)   |
                ((out_act_cmp0_down & B_OUT_ACT_CMP0_DOWN_MASK) << B_OUT_ACT_CMP0_DOWN_SHIFT) |
                ((out_act_cmp1_up   & B_OUT_ACT_CMP1_UP_MASK  ) << B_OUT_ACT_CMP1_UP_SHIFT)   |
                ((out_act_cmp1_down & B_OUT_ACT_CMP1_DOWN_MASK) << B_OUT_ACT_CMP1_DOWN_SHIFT) |
                ((out_act_cnt_prd   & B_OUT_ACT_CNT_PRD_MASK  ) << B_OUT_ACT_CNT_PRD_SHIFT)   |
                ((out_ab_mode       & B_OUT_AB_MODE_MASK      ) << B_OUT_AB_MODE_SHIFT)       |
                ((out_osht_num      & B_OUT_OSHT_NUM_MASK     ) << B_OUT_OSHT_NUM_SHIFT)      |
                ((db_en             & B_DB_EN_MASK            ) << B_DB_EN_SHIFT)             |
                ((chp_en            & B_CHP_EN_MASK           ) << B_CHP_EN_SHIFT);

    PWM->b_conf = regValue;
}

// 配置`enable_state`位字段
void configurePwmAEnableState(pwm_regs* PWM, uint16_t channelState)
{
    uint32_t regValue = PWM->update;

    // 构建写入值，根据文档要求设置pwdata[16]为1
    regValue = (0x1 << 16) | channelState;

    PWM->update = regValue;
}

void configurePwmBEnableState(pwm_regs* PWM, uint16_t channelState)
{
    uint32_t regValue = PWM->update;

    // 构建写入值，根据文档要求设置pwdata[17]为1
    regValue = (0x1 << 17) | (channelState << 1);

    PWM->update = regValue;
}

void configureDMARegisters(pwm_regs* PWM, uint32_t capDmaEn, uint32_t capDmaSel, uint32_t pwmOutSel)
{
    PWM->cap_dma_en = capDmaEn;

    PWM->cap_dma_sel = capDmaSel;

    PWM->pwm_out_sel = pwmOutSel;
}
