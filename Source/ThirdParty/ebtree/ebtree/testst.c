int loops = 0;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ebsttree.h"

#ifdef DEBUG
#define DPRINTF printf
#else
#define DPRINTF(a, ...)
#endif

int main(int argc, char **argv) {
	struct eb_root root = EB_ROOT;
	struct ebmb_node *node;
	struct ebmb_node *node_mb, *node_len, *node_len1;
	char buffer[1024];

	/* disable output buffering */
	setbuf(stdout, NULL);

	if (argc > 1 && strcmp(argv[1], "-h") == 0) {
		fprintf(stderr, "Usage: %s [val...]\n", argv[0]);
		exit(1);
	}

	argv++;	argc--;
	while (argc >= 1) {
		int len;
		char *ret = strchr(*argv, '\n');
		if (ret)
			*ret = 0;
		len = strlen(*argv);
		node = calloc(1, sizeof(*node) + len + 1);
		memcpy(node->key, *argv, len + 1);
		ebst_insert(&root, node);
		argv++;
		argc--;
	}

	printf("Dump of command line values :\n");
	node = ebmb_first(&root);
	while (node) {
		printf("node %p = %s\n", node, (char *)node->key);
		node = ebmb_next(node);
	}

	printf("Now enter lookup values, one per line.\n");
	while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
		char *ret = strchr(buffer, '\n');
		int len;

		if (ret)
			*ret = 0;

		len  = strlen(buffer);
		node      = ebst_lookup(&root, buffer);
		node_mb   = ebmb_lookup(&root, buffer, len);
		node_len  = ebst_lookup_len(&root, buffer, len);
		node_len1 = ebst_lookup_len(&root, buffer, len ? len - 1 : 0);
		printf("lookup: st:node=%p <%s>, mb:node=%p <%s>, len:node=%p <%s>, len-1:node=%p <%s>\n",
		       node, node?(char *)node->key:"",
		       node_mb, node_mb?(char *)node_mb->key:"",
		       node_len, node_len?(char *)node_len->key:"",
		       node_len1, node_len1?(char *)node_len1->key:"");
	}

	printf("loops=%d\n", loops);
	return 0;
}
