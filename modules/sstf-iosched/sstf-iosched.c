/*
 * SSTF IO Scheduler
 *
 * For Kernel 4.13.9
 */

#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

/* SSTF data structure. */
struct sstf_data {
	struct list_head queue;
};

static void sstf_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}

/* Identifies the position of the last block dispatched */
static unsigned long long int last_blk_pos_dispatched = 0;

/* Identifies the position of the last block added */
static unsigned long long int last_blk_pos_added = 0;

static char sstf_get_cursor_direction(unsigned long long int lastPos, unsigned long long int currPos){
	if (lastPos <= currPos) {
		return 'R';
	} else {
		return 'L';
	}
}

static void sstf_get_next_rq(struct sstf_data *nd, struct request **next){
	unsigned long long int min_blk_distance = ULLONG_MAX;
	struct request *currentRq = NULL;
	struct list_head* position; struct list_head* temp; list_for_each_safe(position, temp, &nd->queue) {
		struct request *rq = list_entry(position, struct request, queuelist);
		unsigned long long int blk_distance = abs(blk_rq_pos(rq) - last_blk_pos_dispatched);

		if (blk_distance < min_blk_distance) {
			min_blk_distance = blk_distance;
			currentRq = rq;
		}

	}

	*next = currentRq;
}

/* Esta função despacha o próximo bloco a ser lido. */
static int sstf_dispatch(struct request_queue *q, int force){
	struct sstf_data *nd = q->elevator->elevator_data;
	struct request *rq = NULL;
	sstf_get_next_rq(nd, &rq);


	/* Aqui deve-se retirar uma requisição da fila e enviá-la para processamento.
	 * Use como exemplo o driver noop-iosched.c. Veja como a requisição é tratada.
	 *
	 * Antes de retornar da função, imprima o sector que foi atendido.
	 */
	if (rq) {
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);

		char direction = sstf_get_cursor_direction(last_blk_pos_dispatched, blk_rq_pos(rq));
		last_blk_pos_dispatched = blk_rq_pos(rq);
		printk(KERN_EMERG "[SSTF] dsp %c %llu\n", direction, blk_rq_pos(rq));

		return 1;
	}
	return 0;
}

static void sstf_add_request(struct request_queue *q, struct request *rq){
	struct sstf_data *nd = q->elevator->elevator_data;
	char direction = sstf_get_cursor_direction(last_blk_pos_added, blk_rq_pos(rq));

	/* Aqui deve-se adicionar uma requisição na fila do driver.
	 * Use como exemplo o driver noop-iosched.c
	 *
	 * Antes de retornar da função, imprima o sector que foi adicionado na lista.
	 */

	list_add_tail(&rq->queuelist, &nd->queue);
	last_blk_pos_added = blk_rq_pos(rq);
	printk(KERN_EMERG "[SSTF] add %c %llu\n", direction, blk_rq_pos(rq));
}

static int sstf_init_queue(struct request_queue *q, struct elevator_type *e){
	struct sstf_data *nd;
	struct elevator_queue *eq;

	/* Implementação da inicialização da fila (queue).
	 *
	 * Use como exemplo a inicialização da fila no driver noop-iosched.c
	 *
	 */

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if (!nd) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = nd;

	INIT_LIST_HEAD(&nd->queue);

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);

	return 0;
}

static void sstf_exit_queue(struct elevator_queue *e)
{
	struct sstf_data *nd = e->elevator_data;

	/* Implementação da finalização da fila (queue).
	 *
	 * Use como exemplo o driver noop-iosched.c
	 *
	 */
	BUG_ON(!list_empty(&nd->queue));
	kfree(nd);
}

/* Infrastrutura dos drivers de IO Scheduling. */
static struct elevator_type elevator_sstf = {
	.ops.sq = {
		.elevator_merge_req_fn		= sstf_merged_requests,
		.elevator_dispatch_fn		= sstf_dispatch,
		.elevator_add_req_fn		= sstf_add_request,
		.elevator_init_fn		= sstf_init_queue,
		.elevator_exit_fn		= sstf_exit_queue,
	},
	.elevator_name = "sstf",
	.elevator_owner = THIS_MODULE,
};

/* Inicialização do driver. */
static int __init sstf_init(void)
{
	return elv_register(&elevator_sstf);
}

/* Finalização do driver. */
static void __exit sstf_exit(void)
{
	elv_unregister(&elevator_sstf);
}

module_init(sstf_init);
module_exit(sstf_exit);

MODULE_AUTHOR("Miguel Xavier");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SSTF IO scheduler");
