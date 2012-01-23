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
	int _linkAlloc;
	Link *links;
} Node;

typedef struct _Chain {
	int nodeCount;
	int _nodeAlloc;
	int *_lookup;
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
	//0 and 1 are reserved, start at 2
	Node *out = NULL;
	for(int i = 2; i < chain->nodeCount; i++) {
		char *nData = chain->nodes[i].data;
		if(strcmp(nData, data) == 0) {
			out = &chain->nodes[i];
			break;
		}
	}
	/*
	int min, max, ret, p;
	max = chain->nodeCount;
	min = 2;

	Node *nodes = chain->nodes;
	int *lookup = chain->_lookup;

	Node *out2;
	//binary search, by node id because pointers are anoying
	while(max >= min) {
		p = min + ((max - min) / 2);
		ret = strcmp(nodes[lookup[p]].data, data);
		if(ret < 0) min = p + 1;
		if(ret > 0) max = p - 1;
		if(ret == 0) {
			out2 = &nodes[lookup[p]];
			break;
		}
	}

	if(out != out2) {
		char one = 
		fprintf(strerror. "Comparison Failed, Looking for \"%s\", found \"%s\"\n", out->data, out2->data);
	}*/

	return out;
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
		node->_linkAlloc = 0;
		node->links = NULL;

		
		if(id > 1) {
			//Sorted lookup array
			//dont add 0 and 1
			int count = chain->nodeCount;
			char *lookup = chain->_lookup;
			Node *nodes = chain->nodes;
			
			lookup[id] = 0; //id equals index of last element, clear it
			
			int p = 0;
			{
				int min, max, ret;

				max = count;
				min = 2;

				while(max >= min) {
					p = min + ((max - min) / 2);
					ret = strcmp(nodes[lookup[p]].data, data);
					if(ret < 0) min = p + 1;
					if(ret > 0) max = p - 1;
				}
				//and now p should be the new position
			}
			memmove(&lookup[p], &lookup[p + 1], (count - p) * sizeof(int));
			lookup[p] = id;
		}

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
		if(linkId >= one->_linkAlloc) {
			one->_linkAlloc += 4;
			one->links = realloc(one->links, one->_linkAlloc*sizeof(Link));
		}
		one->links[linkId].nodeId = two->id;
		one->links[linkId].count = 1;
	} else {
		one->links[linkId].count++;
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
	
}
