#ifndef INTERRUPT_H_
#define INTERRUPT_H_

void register_user_irq_handler(tcb_t *from);
void start_user_irq_handler(tcb_t *t);
void free_user_irq_handler(tcb_t *from);

#endif
