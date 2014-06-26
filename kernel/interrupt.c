#include <thread.h>
#include <ipc.h>
#include <platform/irq.h>
#include <interrupt.h>
#include <interrupt_ipc.h>
#include <debug.h>
#include <init_hook.h>

#include INC_PLAT(nvic.h)

typedef void (*irq_handler_t)(void);

struct user_irq {
	tcb_t *thr;
	uint16_t action;
	uint16_t priority;
	irq_handler_t handler;
};

static struct user_irq user_irq[IRQn_NUM];

/* TODO: Priority queue in the future */
#define INVALID_IDX		IRQn_NUM
struct irq_queue {
	uint8_t q[IRQn_NUM];
	int head;
	int tail;
};

static struct irq_queue irq_queue = {
	.head = 0,
	.tail = 0
};

uint8_t irq_queue_idx[IRQn_NUM];

static void irq_queue_push(int n)
{
	if (irq_queue_idx[n] == INVALID_IDX) {
		irq_queue.q[irq_queue.tail] = n;
		irq_queue_idx[n] = irq_queue.tail;
		irq_queue.tail = (irq_queue.tail + 1) % IRQn_NUM;
	}
}

static int irq_queue_pop(void)
{
	int n;

	if (irq_queue.head == irq_queue.tail)
		return -1;

	n = irq_queue.q[irq_queue.head];
	irq_queue.head = (irq_queue.head + 1) % IRQn_NUM;
	irq_queue_idx[n] = INVALID_IDX;

	return n;
}

static void irq_schedule_by_queue(void)
{
	tcb_t *thr;
	int irq;
	ipc_msg_tag_t tag;

	irq = irq_queue.q[irq_queue.head];
	thr = user_irq[irq].thr;

	if (thr->state == T_RECV_BLOCKED) {
		/* ipc to wake up user interrupt thread */
		tag.s.label = USER_INTERRUPT_LABEL;
		tag.s.n_untyped = IRQ_IPC_MSG_NUM;
		ipc_write_mr(thr, 0, tag.raw);
		ipc_write_mr(thr, IRQ_IPC_IRQN + 1, (uint32_t)irq);
		ipc_write_mr(thr,IRQ_IPC_HANDLER + 1, (uint32_t)user_irq[irq].handler);
		ipc_write_mr(thr,IRQ_IPC_ACTION + 1, (uint32_t)user_irq[irq].action);
		thr->utcb->sender = TID_TO_GLOBALID(THREAD_INTERRUPT);
		thr->state = T_RUNNABLE;
		thr->ipc_from = L4_NILTHREAD;
	}

	sched_slot_dispatch(SSI_INTR_THREAD, thr);
}

/*
* Push n to irq queue.
* Select the first one in queue to run.
*/
static void irq_schedule(int n)
{
	irq_queue_push(n);
	irq_schedule_by_queue();

}

static void __user_interrupt_handler(int n)
{
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

	irq_schedule(n);

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


void interrupt_init(void)
{
	int i;

	for (i = 0 ; i < IRQn_NUM ; i++)
		irq_queue_idx[i] = INVALID_IDX;
}

INIT_HOOK(interrupt_init, INIT_LEVEL_KERNEL_EARLY);

/* TODO: Ignore priority now. Handle different priority in the future. */
void register_user_irq_handler(tcb_t *from)
{
	ipc_msg_tag_t tag;
	uint16_t n;
	l4_thread_t tid;
	tcb_t *irq_thr;

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

	irq_thr = user_irq[n].thr;
	if (irq_thr->state != T_RECV_BLOCKED) {
		/* user interrupt is not ready */
		NVIC_DisableIRQ(n);
	}
}

void start_user_irq_handler(tcb_t *t)
{
	int n, i;
	tcb_t *head_thr;

	n = irq_queue.q[irq_queue.head];
	head_thr = user_irq[n].thr;

	if (head_thr == t) {
		irq_queue_pop();
		irq_schedule_by_queue();
	}

	for (i = 0 ; i < IRQn_NUM ; i++) {
		if (user_irq[i].thr == t) {
			NVIC_ClearPendingIRQ(i);
			NVIC_EnableIRQ(i);
			break;
		}
	}
}

void free_user_irq_handler(tcb_t *from)
{
}


