#ifndef __SYSTIMER_H__
#define __SYSTIMER_H__

#ifdef __cplusplus
    extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum _time_precision
{
    IN_CNT = 0,
    IN_US  = 1,
    IN_MS  = 2,
    IN_S   = 3
} time_precision_t;

typedef uint8_t systimer_id_t;
#define SYSTIMER_ERR_ID UINT8_MAX
/**
 * @brief 初始化systimer组件
 * 
 */
void systimer_init(void);

/**
 * @brief 开启一个空闲时钟
 * @return 255(max(systimer_id_t)) - 没有空闲时钟,返回其它值代表开启的空闲时钟的id 
 */
systimer_id_t systimer_acquire_timer(void);

/**
 * @brief 关闭一个时钟
 * @param timer_id 待关闭的时钟id
 */
void systimer_release_timer(systimer_id_t timer_id);

/**
 * @brief 获得距离上次开启该时钟后过去的时间
 * @param timer_id 时钟id
 * @param time_precision 时间精度
 * @return 距离上次开启后过去的时间
 */
uint64_t systimer_get_elapsed_time(systimer_id_t timer_id, time_precision_t time_precision);

/**
 * @brief 判断一个时钟是否超时
 * @param timer_id 时钟
 * @param time 设置一个超时时间
 * @param time_precision 设置的超时时间精度
 * @return true - 超时,false - 未超时 
 */
bool systimer_is_timeout(systimer_id_t timer_id, uint64_t time, time_precision_t time_precision);

/**
 * @brief 阻塞延时指定时长后返回
 * @param time 设置一个delay时长
 * @param time_precision 设置时间精度
 * @return true - 延时完成,false - 延时失败，无空闲可用时钟
 */
bool systimer_delay(uint64_t time, time_precision_t time_precision);

#ifdef __cplusplus
    }
#endif

#endif
