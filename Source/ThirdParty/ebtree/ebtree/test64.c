#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eb64tree.h"

#ifdef DEBUG
#define DPRINTF printf
#else
#define DPRINTF(a, ...)
#endif

int main(int argc, char **argv) {
	unsigned long long x;
	struct eb_root e64 = EB_ROOT;
	struct eb64_node *node;
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
		eb64_insert(&e64, node);
		argv++;
		argc--;
	}

	printf("Dump of command line values :\n");
	node = eb64_first(&e64);
	while (node) {
		printf("node %p = %lld\n", node, node->key);
		node = eb64_next(node);
	}

	printf("Now enter lookup values, one per line.\n");
	while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
		char *ret = strchr(buffer, '\n');
		if (ret)
			*ret = 0;
		x = atoll(buffer);
		node =	eb64_lookup(&e64, x);
		printf("eq: node=%p, val=%lld\n", node, node?node->key:0);
		node =	eb64_lookup_le(&e64, x);
		printf("le: node=%p, val=%lld\n", node, node?node->key:0);
		node =	eb64_lookup_ge(&e64, x);
		printf("ge: node=%p, val=%lld\n", node, node?node->key:0);
	}
	return 0;
}
