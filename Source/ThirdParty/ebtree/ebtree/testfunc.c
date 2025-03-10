/*
 * ebtree performance test for various functions - willy tarreau - 2013
 *
 * Build for example with :
 *   make testfunc CFLAGS="-O3 -DTYPE=eb32_node -DINSERT=__eb32_insert -DLOOKUP=__eb32_lookup -DDELETE=__eb32_delete -lm"
 *   make testfunc CFLAGS="-O3 -DTYPE=eb64_node -DINSERT=__eb64_insert -DLOOKUP=__eb64_lookup -DDELETE=__eb64_delete -lm"
 *
 */

#include <sys/time.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "ebtree.h"
#include "eb32tree.h"
#include "eb64tree.h"

#ifndef TYPE
#error "Please define the node type to use with -DTYPE=eb{32|64}_node"
#endif

#ifndef INSERT
#error "Please define the insert function to use with -DINSERT=[__]eb{32|64}[i]_insert"
#endif

#ifndef LOOKUP
#error "Please define the lookup function to use with -DLOOKUP=[__]eb{32|64}[i]_lookup"
#endif

#ifndef DELETE
#error "Please define the delete function to use with -DDELETE=[__]eb{32|64}_delete"
#endif


struct eb_root root;
struct TYPE *nodes;
int nbnodes;

static inline unsigned long long rdtsc()
{
     unsigned int a, d;
     asm volatile("rdtsc" : "=a" (a), "=d" (d));
     return a + ((unsigned long long)d << 32);
}

int main(int argc, char **argv)
{
	int i;
	uint64_t v;
	unsigned long long cal, beg, end;
	unsigned long long tot_init, tot_insert, tot_lookup, tot_delete, last_insert;

	if (argc > 1)
		nbnodes = atoi(argv[1]);
	else
		nbnodes = 100000;

	/* use malloc() + memset() instead of calloc() to ensure the area is
	 * already populated.
	 */
	nodes = malloc(nbnodes * sizeof(*nodes));
	memset(nodes, 0, nbnodes * sizeof(*nodes));

	printf("Allocated %d nodes of %d bytes = %d total\n",
	       nbnodes, sizeof(*nodes),
	       nbnodes * sizeof(*nodes));

	printf("Times in CPU cycles for %d ops, for 1, and for 1/log(#nodes) :\n",
	       nbnodes);

	/* pre-fill all the keys with large randoms */
	cal = rdtsc();
	beg = rdtsc();
	for (i = 0; i < nbnodes; i++) {
		v  = ((uint64_t)random()) << 48;
		v += ((uint64_t)random()) << 24;
		v += (uint64_t)random();
		nodes[i].key = v;
	}
	end = rdtsc();
	tot_init = (end - beg) - (beg - cal);

	printf("  Init:    %10lld %4.1f %4.1f\n",
	       tot_init,
	       (double)tot_init / nbnodes,
	       (double)tot_init / (nbnodes * (1 + log(nbnodes))));

	/* insert all nodes */
	cal = rdtsc();
	beg = rdtsc();
	for (i = 0; i < nbnodes; i++) {
		INSERT(&root, &nodes[i]);
	}
	end = rdtsc();
	tot_insert = (end - beg) - (beg - cal);

	printf("  Insert:  %10lld %4.1f %4.1f\n",
	       tot_insert,
	       (double)tot_insert / nbnodes,
	       (double)tot_insert / (nbnodes * (1 + log(nbnodes))));

	/* measure the time it takes for last node (tree full) */
	cal = rdtsc();
	beg = rdtsc();
	for (i = 0; i < nbnodes; i++) {
		DELETE(&nodes[nbnodes - 1]);
		INSERT(&root, &nodes[nbnodes - 1]);
	}
	end = rdtsc();
	last_insert = (end - beg) - (beg - cal);

	printf("  Del+Ins: %10lld %4.1f %4.1f (last node only -> tree full)\n",
	       last_insert,
	       (double)last_insert / nbnodes,
	       (double)last_insert / (nbnodes * (1 + log(nbnodes))));

	/* look up all nodes */
	cal = rdtsc();
	beg = rdtsc();
	for (i = 0; i < nbnodes; i++) {
		LOOKUP(&root, nodes[i].key);
	}
	end = rdtsc();
	tot_lookup = (end - beg) - (beg - cal);

	printf("  Lookup:  %10lld %4.1f %4.1f\n",
	       tot_lookup,
	       (double)tot_lookup / nbnodes,
	       (double)tot_lookup / (nbnodes * (1 + log(nbnodes))));

	/* delete all nodes */
	cal = rdtsc();
	beg = rdtsc();
	for (i = 0; i < nbnodes; i++) {
		DELETE(&nodes[i]);
	}
	end = rdtsc();
	tot_delete = (end - beg) - (beg - cal);

	printf("  Delete:  %10lld %4.1f %4.1f\n",
	       tot_delete,
	       (double)tot_delete / nbnodes,
	       (double)tot_delete / (nbnodes * (1 + log(nbnodes))));

	return 0;
}
