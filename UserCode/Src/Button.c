#include "button.h"

uint8_t ButtonUpdate(Button *button, uint8_t is_pressed)
{
  uint32_t current_time = HAL_GetTick();

  switch (button->state)
  {
    case BUTTON_STATE_UNPRESSED: // 未按下
      if (is_pressed)
      {
        button->state = BUTTON_STATE_PRESSED_DEAD_ZONE; // 进入死区时间
        button->time = current_time;
      }
      break;

    case BUTTON_STATE_PRESSED_DEAD_ZONE: // 死区时间
      if (current_time - button->time >= button->dead_zone_time)
      {
        if (is_pressed)
        {
          button->state = BUTTON_STATE_PRESSED; // 按下
          button->duration_time = current_time; // 记录按下时刻
        }
        else
        {
          button->state = BUTTON_STATE_UNPRESSED; // 回到未按下状态
        }
      }
      break;

    case BUTTON_STATE_PRESSED: // 按下
      button->duration_time = current_time - button->time; // 计算按下持续时间
      if (!is_pressed)
      {
        button->state = BUTTON_STATE_RELEASE_DEAD_ZONE;
        button->time = current_time; // 记录释放时刻
        return 1; // 短按事件触发
      }
      else if (button->duration_time >= button->long_press_threshold)
      {
        button->state = BUTTON_STATE_LONG_PRESSED_TRIGGERED; // 长按触发后
        return 2; // 长按事件触发
      }
      break;

    case BUTTON_STATE_LONG_PRESSED_TRIGGERED: // 长按触发后
      if (!is_pressed)
      {
        button->state = BUTTON_STATE_RELEASE_DEAD_ZONE;
        button->time = current_time; // 记录释放时刻
      }
      break;

    case BUTTON_STATE_RELEASE_DEAD_ZONE: // 释放后抖动时间
      if (current_time - button->time >= button->dead_zone_time)
      {
        if (!is_pressed)
        {
          button->state = BUTTON_STATE_UNPRESSED; // 回到未按下状态
        }
        else
        {
          button->state = BUTTON_STATE_PRESSED_DEAD_ZONE; // 再次进入死区时间
          button->time = current_time;
        }
      }
      break;

    default:
      button->state = BUTTON_STATE_UNPRESSED; // 重置状态机
      break;
  }
  return 0;
}