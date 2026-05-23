#ifndef _TIMESLIDE_H
#define _TIMESLIDE_H

#include "main.h"

typedef struct {
    void (*func)(void);     /* 任务函数指针 */
    uint32_t period_ms;     /* 执行周期（时间片），单位 ms */
    uint32_t last_run;      /* 上次运行的时刻 */
    uint32_t run_times;     /* 运行次数，0表示无限次 */
    uint32_t runed_times;
    uint8_t  enabled;       /* 是否启用 */
} TimeSliceTask;

void TimeSlide_Enable(TimeSliceTask * task);
void TimeSlide_Disable(TimeSliceTask * task);
uint8_t TimeSlide_Update(TimeSliceTask * task);

#endif /* _TIMESLIDE_H */