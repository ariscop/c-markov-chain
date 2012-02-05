#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "markov.h"

static char wordRegex[] = "\\w+";
regex_t *wReg = NULL;

void toLower(char *text) {
	while(*text != 0) {
		if(*text <= 90 && *text >= 65) {
			*text |= 0x20;
		}
		text++;
	}
}

int train(Chain *chain, char *data) {
	int len = strlen(data);
	int count = 0;
	regmatch_t m;
	
	if(!wReg) {
		wReg = malloc(sizeof(wReg));
		if(regcomp(wReg, wordRegex, REG_EXTENDED | REG_NEWLINE) != 0) {
			printf("Error Compiling regex\n") ; return 0;
		}
	}

	int prev = 0;
	Node *cur = NULL;

	while(regexec(wReg, data, 1, &m, 0) == 0) {
		int start = m.rm_so;
		int end = m.rm_eo;
		size_t len = end - start;
		char *new = strndup(&data[start],len);
		toLower(new);
		cur = newNode(chain, new);
		
		if(prev != 0) {
			link(&chain->nodes[prev], cur);
		} else {
			startNode(chain, cur);
		}
		
		if(cur->count != 1) free(new);

		data += m.rm_eo;
		prev = cur->id;
	} 

	if(cur) endNode(chain, cur);
	return count;
}


int printNodes(Chain *chain) {
	printf(	"Markov Chain\n" \
			"Nodes: %d\n\n", chain->nodeCount);
	
	for(int i = 0; i < chain->nodeCount; i++) {
		Node *node = &chain->nodes[i];
		printf(	"Node %d, %d\n" \
				"\tText: %s\n" \
				"\tCount: %d\n" \
				"\tLink Count: %d\n" \
				"\tLink Total: %d\n" \
				"\tLinks: \n", \
					i, node->id, \
					node->data, \
					node->count, \
					node->linkCount, \
					node->linkTotal);
		
		for(int x = 0; x < node->linkCount; x++) {
			Node *lNode = &chain->nodes[node->links[x].nodeId];
			int count = node->links[x].count;
			printf("\t\tNode %d \"%s\" : %d\n", \
				lNode->id, lNode->data, count);
		}
	}
	return 0;
}

//" to fix a nano bug

char split[] = ".?!";
//char split[] = "\r\n";

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
	char *line = strtok(string, split);
	while(line != NULL) {
		count += train(chain, line);
		lines++;
		if((lines % 1024) == 0)
			fprintf(stderr, "%d Lines Parsed\n", lines);
		line = strtok(NULL, split);
	}
	//count += train(chain, string);
	fprintf(stderr, "\n%d Lines Parsed\n", lines);
	//fprintf(stderr, "Matched %d words\n", (int)len);
	//printNodes(chain);
	
	srand(time(NULL));
	
	for(int i = 0; i < 1024; i++) {
		Node *node;
		node = next(chain, &chain->nodes[0]);
		printf("SENTENCE %d: ", i);
		int x = 0;
		while(node->id != 1 && x < 128) {
			printf("%s ", node->data);
			x++;
			node = next(chain, node);
		}
		printf("\n\n");
	}
	
	/*for(int i = 0; i < 1024; i++) {
		Node *prev = &chain->nodes[0];
		Node *node = &chain->nodes[0];
		printf("SENTENCE %d: ", i);
		int x = 0;
		while(node->id != 1 && x < 16) {
			Node *new = next(chain, node);
			for(int z = 0; z < 16; z++) {
				if(getLink(prev, new) && new->id != 1) {
					printf("%s ", new->data);
					prev = node;
					node = new;
					z = 16;
				}
			}
			x++;
		}
		printf("\n\n");
	}*/
	
	return 0;
}
