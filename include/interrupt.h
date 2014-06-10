#ifndef INTERRUPT_H_
#define INTERRUPT_H_

void register_user_irq_handler(tcb_t *from);

void free_user_irq_handler(tcb_t *from);

#endif
