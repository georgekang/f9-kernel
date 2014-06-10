#ifndef PLATFORM_STM32F4_EXTI_H_
#define PLATFORM_STM32F4_EXTI_H_

#include <platform/stm32f4/registers.h>
#include <platform/stm32f4/gpio.h>

/* EXTI mode */
#define EXTI_INTERRUPT_MODE				0x0
#define EXTI_EVENT_MODE					0x4

/* EXTI trigger type */
#define EXTI_RISING_TRIGGER				0x8
#define EXTI_FALLING_TRIGGER			0xc
#define EXTI_RISING_FALLING_TRIGGER		0x10

#define EXTI_LINE(x)	((uint32_t)0x1 << x)

struct exti_dev {
	uint16_t line;
	uint16_t mode;
	uint16_t trigger;
};

struct exti_regs {
	volatile uint32_t IMR;
	volatile uint32_t EMR;
	volatile uint32_t RTSR;
	volatile uint32_t FTSR;
	volatile uint32_t SWIER;
	volatile uint32_t PR;
};

static inline void exti_init(struct exti_dev *exti)
{
	struct exti_regs *exti_regs = (struct exti_reg *)EXTI_BASE;

	/* Clear EXTI mask */
	exti_regs->IMR &= ~exti->line;
	exti_regs->EMR &= ~exti->line;

	*((volatile uint32_t *)(EXTI_BASE + exti->mode)) |= EXTI_LINE(exti->line);

	if (exti->trigger == EXTI_RISING_FALLING_TRIGGER) {
		exti_regs->RTSR |= EXTI_LINE(exti_regs->line);
		exti_regs->FTSR |= EXTI_LINE(exti_regs->line);
	} else {
		*((volatile uint32_t *)(EXTI_BASE + exti->mode)) |= EXTI_LINE(exti->line);
	}
}

static inline void exti_enable(struct exti_dev *exti)
{
	*((volatile uint32_t *)(EXTI_BASE + exti->mode)) |= EXTI_LINE(exti->line);
}

static inline void exti_disable(struct exti_dev *exti)
{
	struct exti_regs *exti_regs = (struct exti_reg *)EXTI_BASE;

	/* Clear EXTI mask */
	exti_regs->IMR &= ~EXTI_LINE(exti->line);
	exti_regs->EMR &= ~EXTI_LINE(exti->line);
}

static inline void exti_launch_sw_interrupt(u16 line)
{
	struct exti_regs *exti_regs = (struct exti_reg *)EXTI_BASE;

	exti_regs->SWIER |= EXTI_LINE(line);
}

static inline void exti_clear(u16 line)
{
	struct exti_regs *exti_regs = (struct exti_reg *)EXTI_BASE;

	exti_regs->PR = EXTI_LINE(line);
}

static inline int exti_interrupt_status(u16 line)
{
	struct exti_regs *exti_regs = (struct exti_reg *)EXTI_BASE;

	return (exti_regs->PR & EXTI_LINE(line)) && (exti_regs->IMR & EXTI_LINE(line));
}

static inline void exti_interrupt_status_clear(u16 line)
{
	exti_clear(line);
}

#endif
