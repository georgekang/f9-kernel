#ifndef INTERRUPT_IPC_H_
#define INTERRUPT_IPC_H_

/* interrupt ipc message */
enum {
	IRQ_IPC_IRQN = 0,
	IRQ_IPC_TID,
	IRQ_IPC_HANDLER,
	IRQ_IPC_ACTION,
	IRQ_IPC_PRIORITY,
};

#define IRQ_IPC_MSG_NUM	IRQ_IPC_PRIORITY

/* irq actions */
enum {
	IRQ_ENABLE = 0,
	IRQ_DISABLE,
	IRQ_FREE,
};

#define USER_INTERRUPT_LABEL	0x928

#endif
