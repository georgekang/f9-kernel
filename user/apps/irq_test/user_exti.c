#include INC_PLAT(registers.h)
#include INC_PLAT(exti.h)
#include "user_exti.h"


__USER_TEXT
void exti_init(struct exti_dev *exti)
{
	struct exti_regs *exti_regs = (struct exti_regs *)EXTI_BASE;

	/* Clear EXTI mask */
	exti_regs->IMR &= ~EXTI_LINE(exti->line);
	exti_regs->EMR &= ~EXTI_LINE(exti->line);

	*((volatile uint32_t *)(EXTI_BASE + exti->mode)) |= EXTI_LINE(exti->line);

	if (exti->trigger == EXTI_RISING_FALLING_TRIGGER) {
		exti_regs->RTSR |= EXTI_LINE(exti->line);
		exti_regs->FTSR |= EXTI_LINE(exti->line);
	} else {
		*((volatile uint32_t *)(EXTI_BASE + exti->mode)) |= EXTI_LINE(exti->line);
	}
}

__USER_TEXT
 void exti_enable(struct exti_dev *exti)
{
	*((volatile uint32_t *)(EXTI_BASE + exti->mode)) |= EXTI_LINE(exti->line);
}

__USER_TEXT
void exti_disable(struct exti_dev *exti)
{
	struct exti_regs *exti_regs = (struct exti_regs *)EXTI_BASE;

	/* Clear EXTI mask */
	exti_regs->IMR &= ~EXTI_LINE(exti->line);
	exti_regs->EMR &= ~EXTI_LINE(exti->line);
}

__USER_TEXT
void exti_launch_sw_interrupt(uint16_t line)
{
	struct exti_regs *exti_regs = (struct exti_regs *)EXTI_BASE;

	exti_regs->SWIER |= EXTI_LINE(line);
}

__USER_TEXT
void exti_clear(uint16_t line)
{
	struct exti_regs *exti_regs = (struct exti_regs *)EXTI_BASE;

	exti_regs->PR = EXTI_LINE(line);
}

__USER_TEXT
int exti_interrupt_status(uint16_t line)
{
	struct exti_regs *exti_regs = (struct exti_regs *)EXTI_BASE;

	return (exti_regs->PR & EXTI_LINE(line)) && (exti_regs->IMR & EXTI_LINE(line));
}

__USER_TEXT
void exti_interrupt_status_clear(uint16_t line)
{
	exti_clear(line);
}
