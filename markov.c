#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "markov.h"

int printNodes(Chain *chain) {
	printf(	"Markov Chain\n" \
			"Nodes: %d\n\n", chain->nodeCount);
	
	for(int i = 0; i < chain->nodeCount; i++) {
		Node *node = &chain->nodes[i];
		printf(	"Node %d, %d\n" \
				"\tText: %s\n" \
				"\tCount: %d\n" \
				"\tLink Count: %d\n" \
				"\tLinks: \n", \
					i, node->id, \
					node->data, \
					node->count, \
					node->linkCount);
		
		for(int x = 0; x < node->linkCount; x++) {
			Node *lNode = &chain->nodes[node->links[x].nodeId];
			int count = node->links[x].count;
			printf("\t\tNode %d \"%s\" : %d\n", \
				lNode->id, lNode->data, count);
		}
	}
	return 0;
}

int main(int argc, char *argv[]) {
	FILE *fp = fopen(argv[1], "r");
	fseek(fp, 0L, SEEK_END);
	long len = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	
	char *string = malloc(len);
	fread(string, len, 1, fp);
	fclose(fp);
	
	Chain *chain = newChain();
	fprintf(stderr, "Training...\n");
	int count = 0;

	//TODO: fix the bug
	//BUG: very long lines break the 'train' function
	int lines = 0;
	char *line = strtok(string, "\r\n");
	while(line != NULL) {
		count += train(chain, line);
		lines++;
		if((lines % 100) == 0)
			fprintf(stderr, "%d Lines Parsed\n", lines);
		line = strtok(NULL, "\r\n");
	}
	//count += train(chain, string);
	//fprintf(stderr, "%d Lines Parsed\n", lines);
	fprintf(stderr, "Matched %d words\n", len);
	printNodes(chain);
	return 0;
}
