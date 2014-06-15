#include <thread.h>
#include <ipc.h>
#include <platform/irq.h>
#include <interrupt.h>
#include <interrupt_ipc.h>
#include <debug.h>

#include INC_PLAT(nvic.h)

typedef void (*irq_handler_t)(void);

struct user_irq {
	tcb_t *thr;
	uint16_t action;
	uint16_t priority;
	irq_handler_t handler;
};

static struct user_irq user_irq[IRQn_NUM];

static void __user_interrupt_handler(int n)
{
	tcb_t *thr;
	ipc_msg_tag_t tag;

	/* TODO: remove it to support different priority */
	irq_disable();

	/* FIXME:  Orz........ */
	NVIC_DisableIRQ(n);

	/* TODO: What behavior is better? */
	if (user_irq[n].handler == NULL)
		goto INT_OUT;

	/* TODO: distinguish disable and free in the future */
	if (user_irq[n].action != IRQ_ENABLE)
		goto INT_OUT;

	/* create ipc to wake up user interrupt handler */
	thr = user_irq[n].thr;

	/* Chekc if this thr is waiting for us */
	if (thr->ipc_from != TID_TO_GLOBALID(THREAD_INTERRUPT) &&
		thr->state != T_RECV_BLOCKED)
		goto INT_OUT;

	/* ipc message to user interrupt handler */
	tag.s.label = USER_INTERRUPT_LABEL;
	tag.s.n_untyped = IRQ_IPC_MSG_NUM;
	ipc_write_mr(thr, 0, tag.raw);
	ipc_write_mr(thr, IRQ_IPC_IRQN + 1, (uint32_t)n);
	ipc_write_mr(thr,IRQ_IPC_HANDLER + 1, (uint32_t)user_irq[n].handler);
	ipc_write_mr(thr,IRQ_IPC_ACTION + 1, (uint32_t)user_irq[n].action);
	thr->utcb->sender = TID_TO_GLOBALID(THREAD_INTERRUPT);
	thr->state = T_RUNNABLE;
	thr->ipc_from = L4_NILTHREAD;

	sched_slot_dispatch(SSI_INTR_THREAD, thr);

INT_OUT:
	irq_enable();
}

#define USER_IRQ_VEC(n)							\
	void nvic_handler##n(void) __NAKED;			\
	void nvic_handler##n(void)					\
	{											\
		irq_enter();							\
		__user_interrupt_handler(n);			\
		request_schedule();						\
		irq_return();							\
	}

#define USER_INTERRUPT
#define IRQ_VEC_N_OP	USER_IRQ_VEC
#include INC_PLAT(nvic_private.h)
#undef IRQ_VEC_N_OP
#undef USER_INTERRUPT

#define USER_IRQ_REGISTER	0
#define USER_IRQ_MODIFY		1

static void nvic_config(int irq, int state)
{
	if (state == USER_IRQ_REGISTER) {
		NVIC_ClearPendingIRQ(irq);
		NVIC_SetPriority(irq, 0xf, 0);
	}

	if (user_irq[irq].action == IRQ_ENABLE) {
		NVIC_ClearPendingIRQ(irq);
		NVIC_EnableIRQ(irq);
	}  else
		NVIC_DisableIRQ(irq);
}

/* TODO: Ignore priority now. Handle different priority in the future. */
void register_user_irq_handler(tcb_t *from)
{
	ipc_msg_tag_t tag;
	uint16_t n;
	l4_thread_t tid;

	tag.raw = ipc_read_mr(from, 0);

	/* FIXME: how to return if there is error? */
	if (tag.s.label != USER_INTERRUPT_LABEL)
		return;

	n = (uint16_t)from->ctx.regs[IRQ_IPC_IRQN + 1];
	tid = (l4_thread_t)from->ctx.regs[IRQ_IPC_TID + 1];

	/* FIXME: how to handle invalid irqn */
	if (n > MAX_IRQn)
		return;

	if (tid != L4_NILTHREAD) {
		user_irq[n].thr = thread_by_globalid(tid);
		user_irq[n].handler = (irq_handler_t)from->ctx.regs[IRQ_IPC_HANDLER + 1];
		user_irq[n].action = (uint16_t)from->ctx.regs[IRQ_IPC_ACTION + 1];
		user_irq[n].priority = (uint16_t)from->ctx.regs[IRQ_IPC_PRIORITY + 1];
		nvic_config(n, USER_IRQ_REGISTER);
	} else {
		/* just modify action */
		user_irq[n].action = (uint16_t)from->ctx.regs[IRQ_IPC_ACTION + 1];
		nvic_config(n, USER_IRQ_MODIFY);
	}
}


void free_user_irq_handler(tcb_t *from)
{
	
}


