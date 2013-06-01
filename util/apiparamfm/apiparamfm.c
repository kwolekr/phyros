/*-
 * Copyright (c) 2009 Ryan Kwolek
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
 * apiparamfm.c - 
 *    Utility to create .prm files from a list of space-delimited key/value pairs; the binary
 *    representation of a hashtable in the format used by Phyros.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct _chain {
	int totallen;
	int numentries;
	char data[0];
} CHAIN, *LPCHAIN;


///////////////////////////////////////////////////////////////////////////////


unsigned int hash(unsigned char *key, int arraylen) {
    unsigned int hash = 0;
    unsigned int i;
    for (i = 0; i < strlen((const char *)key); i++) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash & arraylen;
}


void InsertValue(char *key, int val, LPCHAIN *table, int tablelen) {
	char *newentry;
	int index = hash((unsigned char *)key, tablelen);
	int dx = strlen(key) + 2;
	if (table[index]) {
		table[index] = (LPCHAIN)realloc(table[index], table[index]->totallen + dx);
		newentry = ((char *)table[index] + table[index]->totallen);
		table[index]->numentries++;
		table[index]->totallen += dx;
	} else {
		table[index] = (LPCHAIN)malloc(sizeof(CHAIN) + dx);
		table[index]->numentries = 1;
		newentry = table[index]->data;
		table[index]->totallen = dx + 8;
	}
	*newentry = (char)(val & 0xFF);
	strcpy(newentry + 1, key);	
}


void main(int argc, char *argv[]) {
	int hashtable[0x400];
	char buf[256];
	char outputfile[256];
	int pos, i, len;
	FILE *file;

	if (argc < 2) {
		printf("Not enough arguments!\n");
		return;
	} else if (argc == 2) {
		strncpy(outputfile, argv[1], sizeof(outputfile));
		len = strlen(outputfile);
		if (len > 3)
			*(int *)(outputfile + len - 4) = 'mrp.';
	} else {
		strncpy(outputfile, argv[2], sizeof(outputfile));
	}

	memset(hashtable, 0, 0x400 * sizeof(int));
	file = fopen(argv[1], "r");
	if (!file) {
		printf("Error opening input file!\n");
		return;
	}

	while (!feof(file)) {
		int blah, val;
		char *tmp;

		fgets(buf, sizeof(buf), file);	
		blah = strlen(buf);
		if (buf && (buf[blah - 1] == 0x0D || buf[blah - 1] == 0x0A))
			buf[blah - 1] = 0;
		tmp = buf;
		while (*tmp && *tmp != ' ')
			tmp++;
		*tmp++ = 0;
		val = atoi(buf);
		InsertValue(tmp, val, (LPCHAIN *)hashtable, 0x3FF);
	}

	fclose(file);

	pos = 0x400 * sizeof(int);
	file = fopen(outputfile, "wb");
	if (!file) {
		printf("Invalid output filename!\n");
		return;
	}

	for (i = 0; i != 0x400; i++) {
		if (hashtable[i]) {
			fseek(file, pos, SEEK_SET);
			fwrite((void *)hashtable[i], ((LPCHAIN)hashtable[i])->totallen, 1, file);
			fseek(file, i * sizeof(int), SEEK_SET);
			fwrite(&pos, sizeof(int), 1, file);
			pos += ((LPCHAIN)hashtable[i])->totallen;
		}
	}

	fclose(file);
}

