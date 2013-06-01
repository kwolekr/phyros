/*-
 * Copyright (c) 2010 Ryan Kwolek
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of
 *     conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list
 *     of conditions and the following disclaimer in the documentation and/or other materials
 *     provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* 
 * radix.c - 
 *    Contains routines and support routines for a tree-based string data 
 *    storage structure (space optimised trie)
 */


#include "main.h"
#include "radix.h"

#ifdef RADIX_CASE_INSENSITIVE
unsigned char chrmap[256] = {
    // 0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, //00
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, //10
	0xFF, 0x0a, 0xFF, 0x0b, 0x0c, 0xFF, 0x0d, 0x0e, 0x0f, 0x10, 0xFF, 0x11, 0xFF, 0x12, 0x13, 0xFF, //20
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x14, 0x15, 0xFF, 0x16, 0xFF, 0xFF, //30

	0x17, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, //40
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x18, 0xFF, 0x19, 0x1a, 0x1b, //50

	0x1c, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, //60
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x1d, 0x1e, 0x20, 0x21, 0xFF, //70

	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
#else
unsigned char chrmap[256] = {
    // 0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, //00
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, //10
	0xFF, 0x0a, 0xFF, 0x0b, 0x0c, 0xFF, 0x0d, 0x0e, 0x0f, 0x10, 0xFF, 0x11, 0xFF, 0x12, 0x13, 0xFF, //20
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x14, 0x15, 0xFF, 0x16, 0xFF, 0xFF, //30

	0x17, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, //40
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x18, 0xFF, 0x19, 0x1a, 0x1b, //50
	0x1c, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, //60
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x1d, 0x1e, 0x20, 0x21, 0xFF, //70

	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
#endif


///////////////////////////////////////////////////////////////////////////


LPTNODE RadixInit() {
	return _RadixCreateNode("", 0, NULL, RADIX_INITPTRS);
}


LPTNODE RadixInsert(LPTNODE rnode, const char *str, void *data, unsigned char flags) {
	int i, mchr;
	char *tmp, ot;
	const char *tsr;
	LPTNODE tmpnode, lpparent, lpnode, rnnode, ronode;

	if (!rnode || !str)
		return rnode;
	
	lpnode = rnode;
	lpparent = NULL;

	while (*str) {
		mchr = chrmap[(unsigned int)*str];
		if (mchr == 0xFF)
			return rnode;
			
		if (lpnode->index[mchr] == 0xFF) {

			tmpnode = _RadixCreateNode(str, flags | RADIX_DONE, data, RADIX_INITPTRS);

			lpnode->index[mchr] = (unsigned char)lpnode->numsubnodes;
			lpnode->subnodes[lpnode->numsubnodes] = tmpnode;
			lpnode->numsubnodes++;
			
			if (lpnode->numsubnodes >= lpnode->numptrs) {
				lpnode->numptrs <<= 1; 
				tmpnode = realloc(lpnode, sizeof(TNODE) + lpnode->numptrs * sizeof(LPTNODE));
				if (lpnode == rnode)
					rnode = tmpnode;

				lpnode = tmpnode;
				if (lpparent)
					lpparent->subnodes[(int)lpparent->index[chrmap[(unsigned int)*lpnode->text]]] = lpnode;
			}
			return rnode;
		} else {
			lpparent = lpnode;
			lpnode = lpnode->subnodes[(int)lpnode->index[mchr]];
			
			tmp = lpnode->text;
			tsr = str;
			while (*tmp) {
				if (*tsr != *tmp) {                
					ronode = _RadixCreateNode(tmp, lpnode->flags, lpnode->data, lpnode->numsubnodes);
					ot = *tmp;
					*tmp = 0;

					if (lpnode->numsubnodes) {
						for (i = 0; i != lpnode->numsubnodes; i++)
							ronode->subnodes[i] = lpnode->subnodes[i];
						ronode->numsubnodes = lpnode->numsubnodes;
					}		

					memcpy(ronode->index, lpnode->index, sizeof(lpnode->index));
					memset(lpnode->index, -1, sizeof(lpnode->index));
					lpnode = _RadixResizeSubnodes(lpnode, RADIX_INITPTRS);
					
					lpnode->data = data;
					lpnode->subnodes[0] = ronode;
					lpnode->index[chrmap[(unsigned char)ot]] = 0;
					lpnode->numsubnodes++;

					if (lpparent)
						lpparent->subnodes[lpparent->index[chrmap[(unsigned char)*lpnode->text]]] = lpnode;

					if (*tsr) {
						rnnode = _RadixCreateNode(tsr, RADIX_DONE, data, RADIX_INITPTRS);
						
						lpnode->flags &= ~RADIX_DONE;
						lpnode->data   = NULL;
						lpnode->subnodes[1] = rnnode;
						lpnode->index[chrmap[(unsigned char)*tsr]] = 1;
						lpnode->numsubnodes++;
					}
					return rnode;
				}
				tsr++;
				tmp++;
				//*tsr++;
				//*tmp++;
			}
		}
		str = tsr;
	}

	return rnode;
}


int RadixRemove(LPTNODE rnode, const char *str) {
	char *tmp;
	int mchr;
	unsigned char index;
	LPTNODE tmpnode, lpparent, lpnode;
	
	if (!rnode || !str)
		return 0;

	lpnode = NULL;
	tmpnode = rnode;

	while (*str) {
		lpparent = lpnode;
		lpnode = tmpnode;

		mchr = chrmap[(unsigned char)*str];
		if (mchr == 0xFF)
			return 0;
			
		index = lpnode->index[mchr];
		if (index == 0xFF)
			return 0;

		tmpnode = lpnode->subnodes[index];

		tmp = tmpnode->text;
		while (*tmp) {
			if (*str != *tmp)
				return 0;
			str++;
			tmp++;
		}
	}

	if (!(tmpnode->flags & RADIX_DONE))
		return 0;

	if (!tmpnode->numsubnodes) {
		lpnode->numsubnodes--;

		lpnode->subnodes[index] = lpnode->subnodes[lpnode->numsubnodes];
		lpnode->index[chrmap[(unsigned char)lpnode->subnodes[lpnode->numsubnodes]->text[0]]] = index;
		lpnode->index[mchr] = 0xFF;

		if (lpnode->numsubnodes == 1)
			_RadixMergeNodes(lpparent, lpnode->subnodes[0], lpnode);
	
		free(tmpnode);
	} else {
		tmpnode->flags &= ~RADIX_DONE;

		if (lpnode->numsubnodes == 1)
			_RadixMergeNodes(lpparent, lpnode->subnodes[0], lpnode);
	}
	free(tmpnode->data);
	return 1;
}


//// get the associated key for a specific item in the trie
void *RadixSearch(LPTNODE rnode, const char *str) {
	LPTNODE lpnode, tmpnode;
	char *tmp;
	int mchr;
	unsigned char nmchar;

	if (!rnode || !str)
		return NULL;

	lpnode = rnode;
	while (*str) {
		mchr = chrmap[(unsigned char)*str];
		if (mchr == 0xFF)
			return NULL;

		nmchar = lpnode->index[mchr]; 
		if (nmchar == 0xFF)
			return NULL;

		tmpnode = lpnode->subnodes[nmchar];
		if (!tmpnode)
			return NULL;

		tmp = tmpnode->text;
		while (*tmp) {
			if (*str != *tmp)
				return NULL;
			str++;
			tmp++;
		}
		lpnode = tmpnode;
	}

	return (lpnode->flags & RADIX_DONE) ? lpnode->data : NULL;
}


//// return all items in the tree
void **RadixSearchAll(LPTNODE rnode, const char *str, int *nresults) {
	LPTNODE lpnode, tmpnode;
	char *tmp;
	unsigned char index;
	void **results;
	int num, mchr;

	if (!rnode || !str || !nresults)
		return NULL;

	*nresults = 0;
	
	lpnode = rnode;
	while (*str) {
		mchr = chrmap[(unsigned char)*str];
		if (mchr == 0xFF)
			return NULL;

		index = lpnode->index[mchr];
		if (index == 0xFF)
			return NULL;

		tmpnode = lpnode->subnodes[index];
		tmp = tmpnode->text;
		while (*str && *tmp) {
			if (*str != *tmp)
				return NULL;
			str++;
			tmp++;
		}
		lpnode = tmpnode;
	}
	
	_RadixScanTreeSize(lpnode, nresults);
	if (!*nresults)
		return NULL;

	results = malloc(*nresults * sizeof(void *));
	num		= 0;
	_RadixScanTree(lpnode, results, &num);

	return results;
}


//// find the node matching a needle
void *RadixFindMatch(LPTNODE rnode, const char *str) {
	LPTNODE lpnode, tmpnode;
	char *tmp;
	unsigned char nmchar;
	int mchr;

	if (!rnode || !str)
		return NULL;

	lpnode = rnode;
	while (lpnode->numsubnodes && !(lpnode->flags & RADIX_DONE)) {
		mchr = chrmap[(unsigned char)*str];
		if (mchr == 0xFF)
			return NULL;

		nmchar = lpnode->index[mchr]; 
		if (nmchar == 0xFF)
			return NULL;

		tmpnode = lpnode->subnodes[nmchar];
		if (!tmpnode)
			return NULL;

		tmp = tmpnode->text;
		while (*tmp) {
			if (*str != *tmp)
				return NULL;
			str++;
			tmp++;
		}
		lpnode = tmpnode;
	}

	return (lpnode->flags & RADIX_DONE) ? lpnode->data : NULL;
}


/*
bad
bead
bear
beast
bed
beer
bool
boose
bsod

        |ad
        |
        |                 |d
        |                 |
        |        |a-------|r
        |        |        |
        |        |        |st
        |        |
        |e-------|d
        |        |
        |        |er
b-------|
        |        |l
        |oo------|
        |        |se
        |
        |sod


if the node is an edge node, with protrusions, the child nodes take precedence (higher height)
all nodes in between edge nodes must be contained within the range of a parent node
might be able to compress the first line of logic with a clever rule

recurse through to get the lengths
put that and the levels in an array then sort based on whatever and for whatever amount of lines, print out whatever elements are coming first
*/
/*void RadixPrintTree(LPTNODE rnode) {
	int nnodes = 0;

	_RadixPrintTraversal(rnode, 0, 32, &nnodes);
}


void _RadixPrintTraversal(LPTNODE node, int x, int y, int *nnodes) {
	int i;

	for (i = node->numsubnodes; i >= 0; i--) {
		_RadixPrintTraversal(node->subnodes[i], bleh, y, nnodes);
	}
}*/


LPTNODE _RadixCreateNode(const char *text, unsigned short flags, void *data, int initialptrs) {
	LPTNODE rnode	   = malloc(sizeof(TNODE) + initialptrs * sizeof(LPTNODE));
	rnode->data		   = data;
	rnode->flags	   = flags;
	rnode->numsubnodes = 0;
	rnode->numptrs	   = RADIX_INITPTRS;
	strncpy(rnode->text, text, sizeof(rnode->text));
	memset(rnode->index, -1, sizeof(rnode->index));
	return rnode;
}


LPTNODE _RadixResizeSubnodes(LPTNODE lpnode, int numsnodes) {
	lpnode				= realloc(lpnode, sizeof(TNODE) + numsnodes * sizeof(LPTNODE));
	lpnode->numptrs		= numsnodes;
	lpnode->numsubnodes = 0;
	return lpnode;
}


void _RadixMergeNodes(LPTNODE lpparent, LPTNODE mergeto, LPTNODE mergefrom) {
	char temp[32];
	unsigned char index;
	int mchr;

	mchr = chrmap[(unsigned char)mergefrom->text[0]];
	if (mchr == 0xFF)
		return;

	index = lpparent->index[mchr];
	if (index == 0xFF)
		return;

	lpparent->subnodes[index] = mergeto;

	strncpy(temp, mergeto->text, sizeof(temp));
	temp[sizeof(temp) - 1] = 0;
	strncpy(mergeto->text, mergefrom->text, sizeof(mergeto->text));
	mergeto->text[sizeof(mergeto->text) - 1] = 0;
	strncat(mergeto->text, temp, sizeof(mergeto->text));
	mergeto->text[sizeof(mergeto->text) - 1] = 0;
}


void _RadixScanTreeSize(LPTNODE lpnode, int *num) {
	int i;

	if (lpnode->flags & RADIX_DONE)
		(*num)++;
	for (i = 0; i != lpnode->numsubnodes; i++)
		_RadixScanTreeSize(lpnode->subnodes[i], num);
}


void _RadixScanTree(LPTNODE lpnode, void **results, int *num) {
	int i;

	if (lpnode->flags & RADIX_DONE) {
		results[*num] = lpnode->data;
		(*num)++;
	}
	for (i = 0; i != lpnode->numsubnodes; i++)
		_RadixScanTree(lpnode->subnodes[i], results, num);
}

