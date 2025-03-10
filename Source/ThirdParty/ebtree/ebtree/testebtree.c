#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "ebtree.h"
#include "eb32tree.h"

#define rdtscll(val) \
     __asm__ __volatile__("rdtsc" : "=A" (val))

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


static unsigned long long start, calibrate, end, cycles;


struct eb_root root = EB_ROOT;
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
    int i;
    unsigned long links_used = 0;
    unsigned long neighbours = 0;
    unsigned long long x;
    struct eb32_node *node, *lastnode;
    struct timeval t_start, t_random, t_insert, t_lookup, t_walk, t_move, t_delete;

    /* disable output buffering */
    setbuf(stdout, NULL);

    if (argc < 2) {
	tv_now(&t_start);
	while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
	    char *ret = strchr(buffer, '\n');
	    if (ret)
		*ret = 0;
	    //printf("read=%lld\n", x);
	    x = atoll(buffer);
	    total++;
	    node = (struct eb32_node *)malloc(sizeof(*node));
	    node->key = x;

	    eb32_insert(&root, node);
	}
	tv_now(&t_random);
	tv_now(&t_insert);
    }
    else {
	total = atol(argv[1]);

	/* preallocation */
	tv_now(&t_start);

	printf("Timing %d random()+malloc... ", total);

	rdtscll(start);
	lastnode = NULL;
	for (i = 0; i < total; i++) {
	    ////unsigned long long x = (i << 16) + ((random() & 63ULL) << 32) + (random() % 1000);
	    //unsigned long l = (random() % 1000)*1000; // to emulate tv_usec based on milliseconds
	    //unsigned long h = ((random() & 63ULL)<<8) + (i%100); // to emulate lots of close seconds
	    ////unsigned long long x = ((unsigned long long)h << 32) + l;
	    //unsigned long long x = ((unsigned long long)h << 16) + l;
	    unsigned long x = random();     // triggers worst case cache patterns

	    //x = i & 16383;// ^ (1 << (i&31));//(i < 32) ? (1 << i) : 1/*(i & 1023)*/;
	    //x = 1UL << (i&31);
	    //x = (i >> 10) << 20 | (i & 1023);
	    x = ((i >> 10) << 20) + (i & 1023) * 3;
	    //x = rev32(i);
	    //x = (x >> 10) << 20 | (x & 1023);
	    //x = (x >> 16) ^ (x << 16);
	    //x = i;
	    node = (struct eb32_node *)calloc(1,sizeof(*node));
	    node->key = x;//*x;//total-i-1;//*/(x>>10)&65535;//i&65535;//(x>>8)&65535;//rev32(i);//i&32767;//x;//i ^ (long)lastnode;
	    node->node.leaf_p = (void *)lastnode;
	    lastnode = node;
	}
	rdtscll(end);
	tv_now(&t_random);
	printf("%llu cycles/ent\n", (end - start)/total);

	printf("Timing %d insert... ", total);
	cycles = 0;
	for (i = 0; i < total; i++) {
	    node = lastnode;
	    lastnode = (void *)node->node.leaf_p;
	    rdtscll(start); rdtscll(calibrate); // account for the time spent calling rdtsc too !
	    eb32_insert(&root, node);
	    rdtscll(end); cycles += (end - calibrate) - (calibrate - start);
	    if (!node->node.leaf_p)
	       neighbours++;
	    else if (node->node.bit)
	       links_used++;
	}
	tv_now(&t_insert);
	printf("%llu cycles/ent\n", cycles/total);
	printf("%lu jumps during insertion = %llu jumps/1000 ins\n", total_jumps, (1000ULL*total_jumps)/total);
    }

    printf("Looking up %d entries... ", total);
    cycles = 0;
    for (i = 0; i < total; i++) {
	unsigned long long x = i;//random();//(random()>>10)&65535;//(i << 16) + ((random() & 63ULL) << 32) + (random() % 1000);
	rdtscll(start); rdtscll(calibrate); // account for the time spent calling rdtsc too !
	node = eb32_lookup(&root, x);
	rdtscll(end); cycles += (end - calibrate) - (calibrate - start);
	if (node && (node->key != (int)x)) {
	    printf("node = %p, wanted = %d, returned = %d\n", node, (int)x, node->key);
	}
	//if (!node)
	//    printf("wanted = %d\n", (int)x);
    }
    tv_now(&t_lookup);
    printf("%llu cycles/ent\n", cycles/total);

    printf("Walking forwards %d entries... ", total);

    cycles = 0;
    node = eb32_first(&root);
    while (node) {
	//printf("node = %p, node->key = 0x%08x, link_p=%p, leaf_p=%p, bit=%d, leaf_p->bit=%d\n",
	//       node, node->key, node->node.link_p, node->node.leaf_p, node->node.bit,
	//       node->node.leaf_p ? node->node.leaf_p->bit : -1);
	rdtscll(start); rdtscll(calibrate); // account for the time spent calling rdtsc too !
	node = eb32_next(node);
	rdtscll(end); cycles += (end - calibrate) - (calibrate - start);
    }
    printf("%llu cycles/ent\n", cycles/total);

    printf("Walking backwards %d entries... ", total);
    rdtscll(start);
    node = eb32_last(&root);
    while (node) {
	//printf("node = %p, node->key = 0x%08x, link_p=%p, leaf_p=%p, bit=%d, leaf_p->bit=%d\n",
	//       node, node->key, node->node.link_p, node->node.leaf_p, node->node.bit,
	//       node->node.leaf_p ? node->node.leaf_p->bit : -1);
	node = eb32_prev(node);
    }
    rdtscll(end);
    tv_now(&t_walk);
    printf("%llu cycles/ent\n", (end - start)/total);

    printf("Moving %d entries (2 times)... ", total);
    rdtscll(start);

    node = NULL;
    for (i=0; i<2 * total; i++) {
	struct eb32_node *next;

	if (!node)
	    node = eb32_first(&root);
	    
	next = eb32_next(node);
	//printf("moving node = %p, node->key = 0x%08x, link_p=%p, leaf_p=%p, bit=%d, leaf_p->bit=%d\n",
	//       node, node->key, node->node.link_p, node->node.leaf_p, node->node.bit,
	//       node->node.leaf_p ? node->node.leaf_p->bit : -1);
	    
	eb32_delete(node);
	node->key += 1000000; // jump in the future
	eb32_insert(&root, node);
	node = next;
    }
    rdtscll(end);
    printf("%llu cycles/ent\n", (end - start)/i);
    tv_now(&t_move);


    printf("Deleting %d entries... ", total);
    node = eb32_first(&root);

    rdtscll(start);
    while (node) {
	struct eb32_node *next;

	next = eb32_next(node);
	//printf("deleting node = %p, node->key = 0x%08x, link_p=%p, leaf_p=%p, bit=%d, leaf_p->bit=%d\n",
	//       node, node->key, node->node.link_p, node->node.leaf_p, node->node.bit,
	//       node->node.leaf_p ? node->node.leaf_p->bit : -1);

	eb32_delete(node);
	node = next;
    }
    rdtscll(end);

    tv_now(&t_delete);
    printf("%llu cycles/ent\n", (end - start)/total);




    node = eb32_first(&root);
    printf("eb32_first now returns %p\n", node);

    printf("total=%u, links=%lu, neighbours=%lu entries, total_jumps=%lu\n", total, links_used, neighbours, total_jumps);
    printf("random+malloc =%lu ms\n", tv_ms_elapsed(&t_start, &t_random));
    printf("insert        =%lu ms\n", tv_ms_elapsed(&t_random, &t_insert));
    printf("lookup        =%lu ms\n", tv_ms_elapsed(&t_insert, &t_lookup));
    printf("walk          =%lu ms\n", tv_ms_elapsed(&t_lookup, &t_walk));
    printf("move          =%lu ms\n", tv_ms_elapsed(&t_walk, &t_move));
    printf("delete        =%lu ms\n", tv_ms_elapsed(&t_move, &t_delete));

    return 0;
}
