#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

	beginTrain(chain);

	toLower(data);

	while(regexec(wReg, data, 1, &m, 0) == 0) {
		int start = m.rm_so;
		int end = m.rm_eo;
		
		size_t len = end - start;
		
		nodeTrainN(chain, &data[start], len);

		data += m.rm_eo;
	} 

	endTrain(chain);

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

//char split[] = ".?!";
char split[] = "\r\n";

int doTrain(Chain *chain, char *string) {
	//TODO: fix the bug
	//BUG: very long lines break the 'train' function
	int lines = 0;
	char *line = strtok(string, split);
	while(line != NULL) {
		train(chain, line);
		lines++;
		if((lines % 1024) == 0)
			fprintf(stderr, "%d Lines Parsed\n", lines);
		line = strtok(NULL, split);
	}
	//count += train(chain, string);
	fprintf(stderr, "\n%d Lines Parsed\n", lines);
	return lines;
}

int generate(Chain *chain, int count) {
	srand(time(NULL)); //random seed :P
	
	for(int i = 0; i < count; i++) {
		int node;
		node = next(chain, 0);
		printf("SENTENCE %d: ", i);
		int x = 0;
		while(node != 1 && x < 128) {
			printf("%s ", chain->nodes[node].data);
			x++;
			node = next(chain, node);
		}
		printf("\n\n");
	}
	return count;
}

long flength(FILE *fp) {
	long pos = ftell(fp);
	fseek(fp, 0L, SEEK_END);
	long len = ftell(fp);
	fseek(fp, pos, SEEK_SET);
	
	return len;
}

char *help = "markov utility\n" \
	"-l [file] : load chain\n" \
	"-t [file] : Train with file\n" \
	"-s [file] : Save to file\n" \
	"-g [count] : generate random setences\n" \
	"-h : print help\n";

int main(int argc, char *argv[]) {
	Chain *chain = NULL;
	
	char c;
	FILE *file;
	int count = 0;
	
	while ( (c = getopt(argc, argv, "t:l:g:s:hg:")) != -1) {
		count++;
		switch(c) {
			case 'l':
				file = fopen(optarg, "r");
				if(file) {
					chain = loadChain(file);
					fclose(file);
				} else {
					fprintf(stderr,
					"Failed to load chain \"%s\"\n", optarg);
				}
				break;
			case 't':
				file = fopen(optarg, "r");
				if(file) {
					long len = flength(file);
					char *string = malloc(len);
					if(!chain) chain = newChain();
					fread(string, len, 1, file);
					doTrain(chain, string);
					fclose(file);
				} else {
					fprintf(stderr,
					"Failed to load training file \"%s\"\n", optarg);
					exit(0);
				}
				break;
			case 's':
				file = fopen(optarg, "w");
				if(file) {
					fprintf(stderr, "Saving to \"%s\"...\n", optarg);
					saveChain(chain, file);
					fclose(file);
				} else {
					fprintf(stderr,
					"Failed open output file \"%s\"\n", optarg);
					exit(0);
				}
				break;
			case 'g':
				if(chain) {
					generate(chain, atoi(optarg));
				} else {
					fprintf(stderr, "Error, no chain\n");
					exit(0);
				}
				break;
			case 'h':
				printf("%s\n", help);
				exit(0);
				break;
		}
	}
	
	if(!count) printf("%s\n", help);
	
	return 0;
}
