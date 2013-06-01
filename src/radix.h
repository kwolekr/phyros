////////// Compile-time configuration ///////////
#define RADIX_INITPTRS  4
#define RADIX_CASE_INSENSITIVE
/////////////////////////////////////////////////

#define RADIX_DIRFW		0x01
#define RADIX_DIRBW		0x02
#define RADIX_DONE		0x80


typedef struct _trienode {
	char text[32];
	void *data;
	unsigned short flags;
	unsigned char numsubnodes;	
	unsigned char numptrs;
	unsigned char index[64];
	struct _trienode *subnodes[0];
} TNODE, *LPTNODE;


LPTNODE RadixInit();
LPTNODE RadixInsert(LPTNODE rnode, const char *str, void *data, unsigned char flags);
int RadixRemove(LPTNODE rnode, const char *str);
void *RadixSearch(LPTNODE rnode, const char *str);
void **RadixSearchAll(LPTNODE rnode, const char *str, int *nresults);
void *RadixFindMatch(LPTNODE rnode, const char *str);

LPTNODE _RadixCreateNode(const char *text, unsigned short flags, void *data, int initialptrs);
LPTNODE _RadixResizeSubnodes(LPTNODE lpnode, int numsnodes);
void _RadixMergeNodes(LPTNODE lpparent, LPTNODE mergeto, LPTNODE mergefrom);
void _RadixScanTreeSize(LPTNODE lpnode, int *num);
void _RadixScanTree(LPTNODE lpnode, void **results, int *num);

