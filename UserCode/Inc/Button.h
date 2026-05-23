#ifndef _BUTTON_H
#define _BUTTON_H

#include "main.h"

typedef enum
{
  BUTTON_STATE_UNPRESSED, // 未按下
  BUTTON_STATE_PRESSED_DEAD_ZONE, // 死区时间
  BUTTON_STATE_PRESSED,  // 按下
  BUTTON_STATE_LONG_PRESSED_TRIGGERED, // 长按触发后
  BUTTON_STATE_RELEASE_DEAD_ZONE // 释放后抖动时间
} ButtonState;

typedef struct
{
  ButtonState state;
  uint8_t dead_zone_time;
  uint32_t time;
  uint32_t duration_time;
  uint32_t long_press_threshold;
} Button; 

uint8_t ButtonUpdate(Button *button, uint8_t is_pressed);

#endif /* _BUTTON_H */