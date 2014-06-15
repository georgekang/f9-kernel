#include <user_interrupt.h>
#include <interrupt_ipc.h>
#include <l4io.h>
#include <l4/ipc.h>
#include <thread.h>



/* FIXME: */
static void __USER_TEXT start_thread(L4_ThreadId_t t, L4_Word_t ip,
                                     L4_Word_t sp, L4_Word_t stack_size)
{
	L4_Msg_t msg;

	L4_MsgClear(&msg);
	L4_MsgAppendWord(&msg, ip);
	L4_MsgAppendWord(&msg, sp);
	L4_MsgAppendWord(&msg, stack_size);
	L4_MsgLoad(&msg);

	L4_Send(t);
}

static L4_ThreadId_t __USER_TEXT create_thread(void (*func)(void), L4_Word_t addr)
{
	L4_ThreadId_t myself = L4_MyGlobalId();
	L4_ThreadId_t child;
	__USER_BSS static L4_Word_t last_thread = 0;

	child.raw = myself.raw + (++last_thread << 14);
	L4_ThreadControl(child, myself, L4_nilthread, myself, (void *)addr);
	start_thread(child, (L4_Word_t)func,
                 addr + UTCB_SIZE + IRQ_STACK_SIZE, IRQ_STACK_SIZE);

	return child;
}


__USER_TEXT
static void __interrupt_handler_thread(void)
{
	do {
		L4_Msg_t msg;
		L4_MsgTag_t tag;
		irq_handler_t handler;
		L4_Word_t action;
		int irqn;

		tag = L4_Receive((L4_ThreadId_t){
			.raw = TID_TO_GLOBALID(THREAD_INTERRUPT)
		});

		L4_MsgStore(tag, &msg);
		irqn = (int)L4_MsgWord(&msg, IRQ_IPC_IRQN);
		handler = (irq_handler_t)L4_MsgWord(&msg, IRQ_IPC_HANDLER);
		action = L4_MsgWord(&msg, IRQ_IPC_ACTION);

		if (action == IRQ_ENABLE) {
			handler();
			enable_irq(irqn);
		} else if (action == IRQ_DISABLE)
			continue;
		else
			break;
	} while(1);

	L4_Set_MsgTag(L4_Niltag);
	L4_Call(L4_Pager());
}

__USER_TEXT
static void __irq_msg(
	L4_Msg_t *out_msg, L4_ThreadId_t tid,
	unsigned int irq, irq_handler_t handler,
	L4_Word_t action, uint16_t priority)
{
	L4_Word_t irq_data[IRQ_IPC_MSG_NUM];

	L4_MsgClear(out_msg);

	irq_data[IRQ_IPC_IRQN] = (L4_Word_t)irq;
	irq_data[IRQ_IPC_TID] = (L4_Word_t)tid.raw;
	irq_data[IRQ_IPC_HANDLER] = (L4_Word_t)handler;
	irq_data[IRQ_IPC_ACTION] = (L4_Word_t)action;
	irq_data[IRQ_IPC_PRIORITY] = (L4_Word_t)priority;

	/* Create msg for irq request */
	L4_MsgPut(out_msg, USER_INTERRUPT_LABEL,
              IRQ_IPC_MSG_NUM, irq_data, 0, NULL);
}

__USER_TEXT
static L4_Word_t __request_irq(L4_Msg_t *msg)
{
	L4_MsgTag_t ret;

	/* Load msg to registers */
	L4_MsgLoad(msg);

	/* register irq in kernel */
	ret = L4_Send((L4_ThreadId_t) {
			.raw = TID_TO_GLOBALID(THREAD_IRQ_REQUEST)
		});

	return ret.raw;
}

__USER_TEXT
L4_Word_t request_irq(unsigned int irq, irq_handler_t handler, uint16_t priority, L4_Word_t addr)
{
	L4_Msg_t msg;
	L4_ThreadId_t tid;

	/* Create thread for interrupt handler */
	tid = create_thread(__interrupt_handler_thread, addr);

	__irq_msg(&msg, tid, irq, handler, IRQ_ENABLE, priority);

	return __request_irq(&msg);
}

__USER_TEXT
L4_Word_t enable_irq(unsigned int irq)
{
	L4_Msg_t msg;

	__irq_msg(&msg, L4_nilthread, irq, NULL, IRQ_ENABLE, -1);		/* priority: -1, don't care */

	return __request_irq(&msg);
}

__USER_TEXT
L4_Word_t disable_irq(unsigned int irq)
{
	L4_Msg_t msg;

	__irq_msg(&msg, L4_nilthread, irq, NULL, IRQ_DISABLE, -1);		/* priority: -1, don't care */

	return __request_irq(&msg);
}

__USER_TEXT
L4_Word_t free_irq(unsigned int irq)
{
	L4_Msg_t msg;

	__irq_msg(&msg, L4_nilthread, irq, NULL, IRQ_FREE, -1);		/* priority: -1, don't care */

	return __request_irq(&msg);
}

