#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#ifdef DEBUG
#define DPRINTF printf
#else
#define DPRINTF(a, ...)
#endif

#ifdef __i386__
#define rdtscll(val) \
     __asm__ __volatile__("rdtsc" : "=A" (val))
#elif __x86_64__
#define rdtscll(val) do { \
     unsigned int __a,__d; \
     asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
     (val) = ((unsigned long)__a) | (((unsigned long)__d)<<32); \
} while(0)
#else
#define rdtscll(val)
#endif

static inline struct timeval *tv_now(struct timeval *tv) {
	gettimeofday(tv, NULL);
	return tv;
}

static inline unsigned long tv_ms_elapsed(const struct timeval *tv1, const struct timeval *tv2) {
	unsigned long ret;

	ret  = ((signed long)(tv2->tv_sec  - tv1->tv_sec))  * 1000;
	ret += ((signed long)(tv2->tv_usec - tv1->tv_usec)) / 1000;
	return ret;
}


/****************************************************************************/


#ifdef USE_RBTREE

#include "rbtree.h"

struct task {
    struct rb_node rb_node;
    struct rb_root *wq;
    long expire;
    void *data;
    char task_data[200];
};

struct rb_root wait_queue = RB_ROOT;


static inline int time_compare(struct task *task1, struct task *task2) 
{
    return (task1->expire > task2->expire);
}

static inline void __rb_insert_task_queue(struct task *newtask)
{
       struct rb_node **p = &newtask->wq->rb_node;
       struct rb_node *parent = NULL;
       struct task * task;

       while(*p)
       {
               parent = *p;
               task = rb_entry(parent, struct task, rb_node);
	       if (time_compare(task, newtask))
                       p = &(*p)->rb_left;
               else
                       p = &(*p)->rb_right;
       }
       rb_link_node(&newtask->rb_node, parent, p);
}


static inline void rb_insert_task_queue(struct task *newtask)
{
       __rb_insert_task_queue(newtask);
       rb_insert_color(&newtask->rb_node, newtask->wq);
}

#define tree_node  rb_node
#define insert_task_queue(task) rb_insert_task_queue(task)
#define tree_first(root) rb_first(root)
#define tree_last(root) rb_last(root)
#define tree_next(node) rb_next(node)
#define tree_prev(node) rb_prev(node)
#define tree_erase(node, root) rb_erase(node, root)
#define tree_entry(node) rb_entry((node), struct task, rb_node)

/****************************************************************************/

#else

#include "eb32tree.h"

struct task {
    struct eb32_node eb_node;
    struct eb_root *wq;
    void *data;
    char task_data[200];
};
#define expire eb_node.key

struct eb_root wait_queue = EB_ROOT;  /* EB_ROOT || EB_ROOT_UNIQUE */

#define tree_node               eb32_node
#define tree_first(root)        eb32_first(root)
#define tree_last(root)         eb32_last(root)
#define tree_next(node)         eb32_next(node)
#define tree_prev(node)         eb32_prev(node)
#define tree_entry(node)        eb32_entry((node), struct task, eb_node)
#define insert_task_queue(task) __eb32_insert((task)->wq, &task->eb_node)
#define tree_lookup(root, x)    __eb32_lookup(root, x)
#define tree_erase(node, root)  __eb32_delete(node);

#endif



/****************************************************************************/


unsigned long total_jumps = 0;

static unsigned long rev32(unsigned long x) {
    x = ((x & 0xFFFF0000) >> 16) | ((x & 0x0000FFFF) << 16);
    x = ((x & 0xFF00FF00) >>  8) | ((x & 0x00FF00FF) <<  8);
    x = ((x & 0xF0F0F0F0) >>  4) | ((x & 0x0F0F0F0F) <<  4);
    x = ((x & 0xCCCCCCCC) >>  2) | ((x & 0x33333333) <<  2);
    x = ((x & 0xAAAAAAAA) >>  1) | ((x & 0x55555555) <<  1);
    return x;
}


int main(int argc, char **argv) {
    char buffer[1024];
    unsigned int total = 0;
    long i;
    unsigned long links_used = 0;
    unsigned long neighbours = 0;
    unsigned long long x;
    struct task *task, *lasttask, *firsttask;
    struct tree_node *node;
    struct timeval t_start, t_random, t_insert, t_lookup, t_walk, t_move, t_delete;
    static unsigned long long start, calibrate, end, cycles, cycles2, cycles3;
    static unsigned long long start1, stop1, count;


    /* disable output buffering */
    setbuf(stdout, NULL);

    printf("Sizeof struct task=%d\n", sizeof(struct task));
    cycles = 0;
    if (argc < 2) {
	tv_now(&t_start);
	while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
		void *p;
	    char *ret = strchr(buffer, '\n');
	    if (ret)
		*ret = 0;
	    //printf("read=%lld\n", x);
	    x = atoll(buffer);
	    total++;
	    task = (struct task *)calloc(1, sizeof(*task));
	    task->expire = x;
	    task->wq = &wait_queue;
	    p = insert_task_queue(task);
	    if (p == task)
		    printf("Inserted task %p\n", p);
	    else
		    printf("Reused task %p\n", p);
	}
	tv_now(&t_random);
	tv_now(&t_insert);
    }
    else {
	total = atol(argv[1]);

	/* preallocation */
	tv_now(&t_start);

	//printf("Timing %d random()+malloc... ", total);

	rdtscll(start);
	firsttask = lasttask = NULL;
	for (i = 0; i < total; i++) {
	    ////unsigned long long x = (i << 16) + ((random() & 63ULL) << 32) + (random() % 1000);
	    //unsigned long l = (random() % 1000)*1000; // to emulate tv_usec based on milliseconds
	    //unsigned long h = ((random() & 63ULL)<<8) + (i%100); // to emulate lots of close seconds
	    ////unsigned long long x = ((unsigned long long)h << 32) + l;
	    //unsigned long long x = ((unsigned long long)h << 16) + l;
	    unsigned long j, x;

	    /* makes a worst case with high bits moving fast */
	    x = i;
	    for (j = 0; j < 32; j++)
		x ^= (1UL << (32-j)) >> (i%(j+1));

	    //printf("x=%08x\n", x);

	    //x=((1<<31) >> (i%32)) ^ ((1<<30) >> (i%31)) ^ ((1<<29) >> (i%30)) ^ ((1<<28) >> (i%29)) ^
	    //((1<<27) >> (i%28)) ^ ((1<<26) >> (i%27)) ^ ((1<<31) >> (i%26)) ^ ((1<<31) >> (i%25)) ^
	    //((1<<31) >> (i%24)) ^ ((1<<31) >> (i%23)) ^ ((1<<31) >> (i%22)) ^ ((1<<31) >> (i%21)) ^
	    //0;

	    //x = i & 16383;// ^ (1 << (i&31));//(i < 32) ? (1 << i) : 1/*(i & 1023)*/;
	    //x = 1UL << (i&31);
	    //x = (i >> 10) << 20 | (i & 1023);
	    //x = rev32(i);
	    //x = (x >> 10) << 20 | (x & 1023);
	    //x = (x >> 16) ^ (x << 16);

	    //x = ((i >> 10) << 20) + (i & 1023) * 3;
	    //x = random();
	    //x = i;
	    //x = total-i;
	    //x = rev32(i);
	    //x = random() & 1023;

	    // simulates some sparse groups of values like with a scheduler
	    x = (i / 1000) * 50000 + (i % 1000) * 4 - 1500;
	    //x = i>>2;
	    //x = i;
	    //x = 1000;
	    task = (struct task *)calloc(1,sizeof(*task));
	    task->expire = x;//*x;//total-i-1;//*/(x>>10)&65535;//i&65535;//(x>>8)&65535;//rev32(i);//i&32767;//x;//i ^ (long)lasttask;
	    task->wq = &wait_queue;

	    if (!firsttask)
		firsttask = task;
	    if (lasttask)
		lasttask->data = (void *)task;
	    lasttask = task;
	    task->data = NULL;

	    /* tasks will be queued backwards */
	    memcpy(task->task_data, &i, sizeof(i));
	    lasttask = task;
	    DPRINTF("task %p = %ld (data=%ld)\n", task, x, i);
	}
	rdtscll(end);
	tv_now(&t_random);
	//printf("%llu cycles/ent\n", (end - start)/total);

	printf("Timing %d insert... ", total);
	cycles = 0;
	task = firsttask;
	for (i = 0; i < total; i++) {
	    rdtscll(start); rdtscll(calibrate); // account for the time spent calling rdtsc too !
	    insert_task_queue(task);
	    rdtscll(end); cycles += (end - calibrate) - (calibrate - start);
	    task = task->data;
	}
	tv_now(&t_insert);
	printf("%llu cycles/ent avg, last = %llu cycles\n", cycles/total, (end - calibrate) - (calibrate - start));

#if defined(tree_lookup)
	printf("Timing %d lookups... ", total);
	cycles3 = 0;
	task = firsttask;
	for (i = 0; i < total; i++) {
	    rdtscll(start); rdtscll(calibrate); // account for the time spent calling rdtsc too !
	    node = tree_lookup(&wait_queue, task->expire);
	    rdtscll(end); cycles3 += (end - calibrate) - (calibrate - start);
	    //if (!node)
	    //	*(int*)0=0;
	    //if (tree_entry(node)->expire != task->expire)
	    //	*(int*)0 = 0;
	    task = task->data;
	}
	tv_now(&t_lookup);
	printf("%llu cycles/ent avg, last = %llu cycles\n", cycles3/total, (end - calibrate) - (calibrate - start));
#else
    tv_now(&t_lookup);
#endif
    }
    cycles2 = cycles;

    printf("Walking right through %d entries... ", total);
    node = tree_first(&wait_queue);
    cycles = 0;

    DPRINTF("\n");
    rdtscll(start);
    while (node) {
#ifdef DEBUG
	task = tree_entry(node);
	memcpy(&i, task->task_data, sizeof(i));
	DPRINTF("next: %p = %ld (data=%ld)\n", task, task->expire, i);
#endif
	node = tree_next(node);
    }
    rdtscll(end);
    cycles = end - start;

    printf("%llu cycles/ent\n", cycles/total);
    cycles2 += cycles;

    printf("Walking left through %d entries... ", total);
    node = tree_last(&wait_queue);
    cycles = 0;

    DPRINTF("\n");
    rdtscll(start);
    while (node) {
#ifdef DEBUG
	task = tree_entry(node);
	memcpy(&i, task->task_data, sizeof(i));
	DPRINTF("prev: %p = %ld (data=%ld)\n", task, task->expire, i);
#endif
	node = tree_prev(node);
    }
    rdtscll(end);
    cycles = end - start;

    printf("%llu cycles/ent\n", cycles/total);
    cycles2 += cycles;

    printf("Deleting %d entries... ", total);
    node = tree_first(&wait_queue);
    lasttask = tree_entry(node);
    cycles = 0;
    count = 0;

    rdtscll(start);
    start1 = start;
    while (node) {
	struct tree_node *next;

	next = tree_next(node);
	task = tree_entry(node);

	//printf("deleting node = %p, node->val = 0x%08x, link_p=%p, leaf_p=%p, bit=%d, leaf_p->bit=%d\n",
	//       node, node->val, node->node.link_p, node->node.leaf_p, node->node.bit,
	//       node->node.leaf_p ? node->node.leaf_p->bit : -1);

	//if (task->expire < lasttask->expire)
	//    printf("old=%p, new=%p, o_exp=%d, n_exp=%d\n", lasttask, task, lasttask->expire, task->expire);

	rdtscll(start); rdtscll(calibrate); // account for the time spent calling rdtsc too !
	tree_erase(node, task->wq);
	rdtscll(end); cycles += (end - calibrate) - (calibrate - start);
	node = next;
	lasttask = task;
	count++;
    }
    rdtscll(end);
    stop1 = end;

    tv_now(&t_delete);
    printf("%llu cycles/ent, %llu ent, %llu cycles tot, %llu cycles/ent(avg)\n",
	   cycles/total, count, stop1-start1, (stop1-start1)/count);
    printf("Total for %d entries : %llu cycles/ent = %llu kilocycles\n", total, (cycles+cycles2)/total, (cycles+cycles2)/1000);

    node = tree_first(&wait_queue);
    if (node)
	printf("ERROR!! rb_first now returns %p\n", node);

    //printf("total=%u, links=%lu, neighbours=%lu entries, total_jumps=%lu\n", total, links_used, neighbours, total_jumps);
    //printf("random+malloc =%lu ms\n", tv_ms_elapsed(&t_start, &t_random));
    //printf("insert        =%lu ms\n", tv_ms_elapsed(&t_random, &t_insert));
    //printf("delete        =%lu ms\n", tv_ms_elapsed(&t_move, &t_delete));

    return 0;
}
