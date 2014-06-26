#include INC_PLAT(nvic.h)
#include INC_PLAT(registers.h)
#include INC_PLAT(exti.h)

#include <user_interrupt.h>
#include <l4io.h>
#include "user_exti.h"

__USER_BSS
static uint32_t exti0_interrupt_num = 0;

__USER_BSS
static uint32_t exti1_interrupt_num = 0;

__USER_TEXT
void exti0_interrupt_handler(void)
{
	exti_clear(0);
	exti0_interrupt_num++;
	exti_launch_sw_interrupt(1);

	L4_Sleep(L4_TimePeriod(500 * 1000));
	printf("%s\n", __func__);
}

__USER_TEXT
void exti1_interrupt_handler(void)
{
	exti_clear(1);
	exti1_interrupt_num++;
	exti_launch_sw_interrupt(0);

	L4_Sleep(L4_TimePeriod(500 * 1000));
	printf("%s\n", __func__);

}

static void main(user_struct *user)
{
	L4_Word_t mem;
	struct exti_dev exti;

	mem = user->fpages[0].base;
	request_irq(EXTI0_IRQn, exti0_interrupt_handler, 1,
                user->fpages[0].base);
	mem += (UTCB_SIZE + IRQ_STACK_SIZE);
	request_irq(EXTI1_IRQn, exti1_interrupt_handler, 1,
                user->fpages[0].base + (UTCB_SIZE + IRQ_STACK_SIZE));

	exti.line = 0;
	exti.mode = EXTI_INTERRUPT_MODE;
	exti.trigger = EXTI_RISING_TRIGGER;
	exti_init(&exti);

	exti.line = 1;
	exti.mode = EXTI_INTERRUPT_MODE;
	exti.trigger = EXTI_RISING_TRIGGER;
	exti_init(&exti);

	exti_launch_sw_interrupt(0);

//	do {
//		printf("exti0 num = %d\n", exti0_interrupt_num);
//		printf("exti1 num = %d\n", exti1_interrupt_num);
//		L4_Sleep(L4_TimePeriod(1000 * 1000));
//	} while(1);
}

DECLARE_USER(
	123,
	irq_test,
	main,
	DECLARE_FPAGE(0x0, 2 * (UTCB_SIZE + IRQ_STACK_SIZE))
	DECLARE_FPAGE(0x40010000, 0x4000)
);
