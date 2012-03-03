#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <regex.h>

#ifndef _MARKOV_H_
#define _MARKOV_H_

typedef struct _Link {
	int nodeId; //ID 0 IS START OF STRING, ID 1 IS END
	int count;
} Link;

typedef struct _Node {
	char *data;
	int id; //self refference
	int count; //ocourence counter

	int linkCount;
	int linkTotal;
	int _linkAlloc;
	Link *links;
} Node;

typedef struct _Chain {
	int nodeCount;
	int _nodeAlloc; //allocated 
	int *_lookup; //sorted list of indexes, alphabeticly sorted
	char *buffer; //the buffer in which text is stored
	Node *nodes;
	
	struct { 
		int cur;
		int count;
		int started;
	} state;
} Chain;

//this will be inteneded to keep and generate sentences
//to begin with, can easily generic to
//any object type

Chain *newChain();
int newNode(Chain *, char *);
int linkNode(Chain *, int , int);
int next(Chain *, int);

//Not implimented yet
//int save(Chain *, char *);
//Chain *recall(char *);

int getNode(Chain *chain, char *data) {
	
	Node *nodes = chain->nodes;
	int *lookup = chain->_lookup;
	
	//binary search, using my own because bsearch wont do the id nuts
	//that i'm working with
	
	int out = -1;
	int found = 0;
	{
		int min, max, ret;

		max = chain->nodeCount - 1;
		min = 2;

		while(max >= min) {
			int p = min + ((max - min) / 2);
			int ret = strcmp(data, nodes[lookup[p]].data);
			
			if(ret > 0) min = p + 1;
			if(ret < 0) max = p - 1;
			if(ret == 0) {
				found = 1;
				out = lookup[p];
				break;
			}
		}
	}

	return out;
}

int insertLookup(Chain *chain, Node *node) {
	//Sorted lookup array
	//dont add 0 and 1
	int id = node->id;
	
	int count = chain->nodeCount;
	int *lookup = chain->_lookup;
	Node *nodes = chain->nodes;
	
	int p = id;
	
	for(int i = 2; i < count; i++) {
		int ret = strcmp(nodes[lookup[i]].data, node->data);
		if(ret > 0) {
			p = i;
			break;
		}
	}

	//printf("Inserting \"%s\" at %d\n", node->data, p);
	
	for(int i = count - 1; i > p; i--) {
		lookup[i] = lookup[i - 1];
		//printf("Setting %d to %d\n", i, lookup[i - 1]);
	}
	
	lookup[p] = id;
	
	//for(int i = 0; i < count; i++) {
	//	printf("%d: %d \"%s\"\n", i, lookup[i], nodes[lookup[i]].data);
	//}
	
	return p;
}

int newNode(Chain *chain, char *data) {
	Node *node;
	int id;
	
	id = getNode(chain, data);
	//node = &chain->nodes[getNode(chain, data)];
	//create a new node
	if(id == -1) {
		id = chain->nodeCount;
		chain->nodeCount++;
		if(id >= chain->_nodeAlloc) {
			chain->_nodeAlloc += 65536;
			chain->nodes = realloc(chain->nodes, chain->_nodeAlloc*sizeof(Node));
			chain->_lookup = realloc(chain->_lookup, chain->_nodeAlloc* sizeof(int));
		}
		node = &chain->nodes[id];
		node->data = data;
		node->id = id;
		node->count = 1;
		node->linkCount = 0;
		node->linkTotal = 0;
		node->_linkAlloc = 0;
		node->links = NULL;

		insertLookup(chain, node);

	} else {
		//node was found
		//incriment counter
		node = &chain->nodes[id];
		node->count++;
	}
	
	return id;
}


int getLink(Node *one, Node *two) {
	for(int i = 0; i < one->linkCount; i++) {
		if(one->links[i].nodeId == two->id) return i;
	}
	return -1;
}

int nodeLink(Chain *chain, int _one, int _two) {
	Node *one = &chain->nodes[_one];
	Node *two = &chain->nodes[_two];
	
	int linkId = getLink(one, two);
	if(linkId == -1) {
		linkId = one->linkCount;
		one->linkCount++;
		one->linkTotal++;
		if(linkId >= one->_linkAlloc) {
			one->_linkAlloc += 4;
			one->links = realloc(one->links, one->_linkAlloc*sizeof(Link));
		}
		one->links[linkId].nodeId = two->id;
		one->links[linkId].count = 1;
		
	} else {
		one->links[linkId].count++;
		one->linkTotal++;
	}
	
	return linkId;
}

Chain *newChain() {
	Chain *chain = malloc(sizeof(Chain));
	chain->nodeCount = 0;
	chain->nodes = NULL;
	chain->_lookup = NULL;
	chain->_nodeAlloc = 0;
	newNode(chain, "START_STRING");
	newNode(chain, "END_STRING");
	return chain;
}

int beginTrain(Chain *chain) {
	chain->state.cur = 0;
	chain->state.count = 0;
	chain->state.started = 1;
}

int _nodeTrain(Chain *chain, char *data) {
	int node = newNode(chain, data);
	nodeLink(chain, chain->state.cur, node);
	chain->state.cur = node;
	chain->state.count++;
	return(node);	
}

int nodeTrainN(Chain *chain, char *_data, int len) {
	char *data = strndup(_data, len);
	return _nodeTrain(chain, data);
}

int nodeTrain(Chain *chain, char *_data) {
	char *data = strdup(_data);
	return _nodeTrain(chain, data);	
}

int endTrain(Chain *chain) {
	nodeLink(chain, chain->state.cur, 1);
	chain->state.started = 0;
	return chain->state.count;
}

int next(Chain *chain, int _node) {
	Node *node = &chain->nodes[_node];
	float total = node->linkTotal;
	int select = (rand() * (total)) / RAND_MAX;
	int sum = 0;
	for(int i = 0; i < node->linkCount; i++) {
		sum += node->links[i].count;
		//printf("Total: %f\t\tSelect: %d\t\tSum: %d\n", total, select, sum);
		if(sum >= select) {
			//printf("Selecting %s\n", chain->nodes[node->links[i].nodeId].data);
			return chain->nodes[node->links[i].nodeId].id;
		}
	}
	fprintf(stderr, "YOU SHOULD NEVER SEE THIS LINE, SOMTHING BROKED, FAILED ON %s\n"
					"select = %d ; sum = %d ; node->linkCount = %d\n",
					node->data ? node->data : "(null)", select, sum, node->linkCount);
	//return NULL;
	int out = chain->nodes[node->links[select - 1].nodeId].id;
	return(out);
}


Chain *saveChain(Chain *chain, FILE *file) {
	for(int x = 2; x < chain->nodeCount; x++) {
		Node *node = &chain->nodes[x];
		fprintf(file, "n:%d:%s\n", x, node->data);
	}
	
	for(int x = 0; x < chain->nodeCount; x++) {
		Node *node = &chain->nodes[x];
		for(int y = 0; y < node->linkCount; y++) {
			fprintf(file, "l:%d:%d:%d\n",
				x, node->links[y].nodeId, node->links[y].count);
		}
	}
	return chain;
}

Chain *loadChain(FILE *file) {
	long pos = ftell(file);
	fseek(file, 0L, SEEK_END);
	long len = ftell(file);
	fseek(file, 0L, SEEK_SET);
	
	char *string = malloc(len);
	fread(string, len, 1, file);	
	
	fseek(file, pos, SEEK_SET);
	
	Chain *chain = newChain();
	
	char *outer, *inner;
	
	char *line = strtok_r(string, "\n", &outer);
	while(line != NULL) {

		char *token = strtok_r(line, ":", &inner);
		if(token[0] == 'n') {
			int id = atoi(strtok_r(NULL, ":", &inner));
			char *data = strtok_r(NULL, ":", &inner);
			
			int nid = newNode(chain, strdup(data));
			//fprintf(stdout, "node:%d:%s\n", id, data);
			
		} else if(token[0] == 'l') {
			int one = atoi(strtok_r(NULL, ":", &inner));
			int two = atoi(strtok_r(NULL, ":", &inner));
			int count = atoi(strtok_r(NULL, ":", &inner));
			int linkId = nodeLink(chain, one, two);
			chain->nodes[one].links[linkId].count += count - 1;
			chain->nodes[one].linkTotal += count - 1;
			//fprintf(stdout, "link:%d:%d:%d\n", one, two, count);
		} else {
			//something else
		}	
		line = strtok_r(NULL, "\n", &outer);
	}
	
	free(string);
	
	return chain;
}

#endif // _MARKOV_H_
