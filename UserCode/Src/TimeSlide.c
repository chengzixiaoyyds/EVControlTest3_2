#include "TimeSlide.h"

void TimeSlide_Enable(TimeSliceTask * task)
{
    task->last_run = HAL_GetTick(); // 初始化上次运行时间为当前时间
    task->runed_times = 0;          // 重置已运行次数
    task->enabled = 1; // 启用任务
}

void TimeSlide_Disable(TimeSliceTask * task)
{
    task->enabled = 0; // 禁用任务
}

uint8_t TimeSlide_Update(TimeSliceTask * task)
{
    if (task->enabled)
    {
        uint32_t current_time = HAL_GetTick();
        if (current_time - task->last_run >= task->period_ms)
        {
            task->func(); // 执行任务函数
            task->last_run = current_time; // 更新上次运行时间
            task->runed_times++;
            if(task->run_times != 0)
            {
                if(task->runed_times >= task->run_times)
                {
                    TimeSlide_Disable(task); // 达到运行次数后禁用任务
                    return 1; // 任务完成
                }
            }
        }
    }
    return 0;
}