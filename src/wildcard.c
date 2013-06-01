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
 * wildcard.c - 
 *    Functions for handling subtext searching and matching
 */

#include "main.h"
#include "fxns.h"
#include "chat.h"
#include "radix.h"
#include "banning.h"
#include "wildcard.h"

const char valid[] = "0123456789abcdefghijklmnopqrstuvwxyz!#$&'()+-.:;=@[]^_`{|}~";

LPTNODE *clan_rtnode;


///////////////////////////////////////////////////////////////////////////////


const char *bmh_memmem(const char *haystack, unsigned int hlen,
					   const char *needle,   unsigned int nlen) {  
	unsigned int bad_char_skip[256];
	unsigned int last;
	unsigned int scan = 0;

	if (nlen <= 0 || !haystack || !needle)
		return NULL;

	for (scan = 0; scan <= 255; scan++)
		bad_char_skip[scan] = nlen;
	
	last = nlen - 1;
	
	for (scan = 0; scan < last; scan++)
		bad_char_skip[(unsigned char)needle[scan]] = last - scan;
	
	while (hlen >= nlen) {
		for (scan = last; haystack[scan] == needle[scan]; scan--) {
			if (scan == 0)
				return haystack;
		}
		hlen     -= bad_char_skip[(unsigned char)haystack[last]];
		haystack += bad_char_skip[(unsigned char)haystack[last]];
	}
	return NULL;
}


//for tagbans
//here we're matching all text in a database to a single wildcard
void *FindTagban(LPTNODE wcnodes[3], char *text) {
	char tmp[32];
	void *blah;

	blah = RadixFindMatch(wcnodes[WCP_BACK], text); //  tag*
	if (blah)
		return blah;

	strrevncpy(tmp, text, sizeof(tmp));
	blah = RadixFindMatch(wcnodes[WCP_FRONT], tmp); //  *tag
	if (blah)
		return blah;

	while (*text) {
		blah = RadixFindMatch(wcnodes[WCP_BOTH], text++); //  *tag*
		if (blah)
			return blah;
	}

	return NULL;
}


//for phrasebans
//here we are testing text in a database to a portion of a given blob of text
void *FindPhraseban(LPTNODE wcnode, char *text) {
	void *blah;

	/*blah = RadixFindMatch(wcnodes[WCP_BACK], text);
	if (blah)
		return blah;*/

	while (*text) {
		blah = RadixFindMatch(wcnode, text++);
		if (blah)
			return blah;
	}

	return NULL;
}


char *DigestTag(char *text, int *type) {
	int len;

	if (!type)
		return NULL;

	len = strlen(text) - 1;
	if (text[len] == '*') {
		if (text[0] == '*') {
			text++;
			len--;
			*type = WCP_BOTH;
		} else {
			*type = WCP_BACK;
		}
		text[len] = 0;
	} else if (text[0] == '*') {
		text++;
		strrev(text);
		*type = WCP_FRONT;
	} else {
		return NULL;
	}
	return text;
}


int WildcardMatch(const char *pattern, const char *text) {
	while (*pattern) {
		if (*pattern == '*') {
			text++;
			if (*text == pattern[1]) {
				if (WildcardMatch(pattern + 1, text))
					pattern++;
			}
		} else {
			if (*pattern != *text)
				return 0;
			pattern++;
			text++;
		}
	}
	return 1;
}

