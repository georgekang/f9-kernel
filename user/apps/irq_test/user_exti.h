#ifndef USER_EXTI_H_
#define USER_EXTI_H_

#include <platform/link.h>

__USER_TEXT
void exti_init(struct exti_dev *exti);

__USER_TEXT
void exti_enable(struct exti_dev *exti);

__USER_TEXT
void exti_disable(struct exti_dev *exti);

__USER_TEXT
void exti_launch_sw_interrupt(uint16_t line);

__USER_TEXT
void exti_clear(uint16_t line);

#endif
