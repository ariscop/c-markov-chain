#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <regex.h>


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
} Chain;

//this will be inteneded to keep and generate sentences
//to begin with, can easily generic to
//any object type
/*
Chain *newChain();
Chain *newNode(Chain *chain, char *data);
Node *linkNode(Node *node, Node *node);
Node *next(node *);
*/
//Not implimented yet
//int save(Chain *, char *);
//Chain *recall(char *);

Node *getNode(Chain *chain, char *data) {
	
	Node *nodes = chain->nodes;
	int *lookup = chain->_lookup;
	
	//binary search, using my own because bsearch wont do the id nuts
	//that i'm working with
	
	Node *out = NULL;
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
				out = &nodes[lookup[p]];
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

Node *newNode(Chain *chain, char *data) {
	Node *node;
	int id;
	
	node = getNode(chain, data);
	//create a new node
	if(node == NULL) {
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
		node->count++;
	}
	
	return node;
}


int getLink(Node *one, Node *two) {
	for(int i = 0; i < one->linkCount; i++) {
		if(one->links[i].nodeId == two->id) return i;
	}
	return -1;
}

int link(Node *one, Node *two) {
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

int startNode(Chain *chain, Node *node) {
	return link(&chain->nodes[0], node);
}

int endNode(Chain *chain, Node *node) {
	return link(node, &chain->nodes[1]);
}

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
		int len = end - start;
		char *new = strndup(&data[start], len);
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

Node *next(Chain *chain, Node *node) {
	float total = node->linkTotal;
	int select = (rand() * (total)) / RAND_MAX;
	int sum = 0;
	for(int i = 0; i < node->linkCount; i++) {
		sum += node->links[i].count;
		//printf("Total: %f\t\tSelect: %d\t\tSum: %d\n", total, select, sum);
		if(sum > select) {
			//printf("Selecting %s\n", chain->nodes[node->links[i].nodeId].data);
			return &chain->nodes[node->links[i].nodeId];
		}
	}
	printf("YOU SHOULD NEVER SEE THIS LINE, SOMTHING BROKED\n");
	exit(-1);
	return NULL;
}
