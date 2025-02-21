#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eb32tree.h"

#ifdef DEBUG
#define DPRINTF printf
#else
#define DPRINTF(a, ...)
#endif

int main(int argc, char **argv) {
	unsigned long long x;
	struct eb_root e32 = EB_ROOT;
	struct eb32_node *node;
	char buffer[1024];

	/* disable output buffering */
	setbuf(stdout, NULL);

	if (argc > 1 && strcmp(argv[1], "-h") == 0) {
		fprintf(stderr, "Usage: %s [val...]\n", argv[0]);
		exit(1);
	}

	argv++;	argc--;
	while (argc >= 1) {
		char *ret = strchr(*argv, '\n');
		if (ret)
			*ret = 0;
		x = atoll(*argv);
		node = calloc(1, sizeof(*node));
		node->key = x;
		eb32_insert(&e32, node);
		argv++;
		argc--;
	}

	printf("Dump of command line values :\n");
	node = eb32_first(&e32);
	while (node) {
		printf("node %p = %d\n", node, node->key);
		node = eb32_next(node);
	}

	printf("Now enter lookup values, one per line.\n");
	while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
		char *ret = strchr(buffer, '\n');
		if (ret)
			*ret = 0;
		x = atoll(buffer);
		node =	eb32_lookup(&e32, x);
		printf("eq: node=%p, val=%d\n", node, node?node->key:0);
		node =	eb32_lookup_le(&e32, x);
		printf("le: node=%p, val=%d\n", node, node?node->key:0);
		node =	eb32_lookup_ge(&e32, x);
		printf("ge: node=%p, val=%d\n", node, node?node->key:0);
	}
	return 0;
}
