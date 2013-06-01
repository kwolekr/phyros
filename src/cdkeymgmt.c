/*-
 * Copyright (c) 2011 Ryan Kwolek
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
 * cdkeymgmt.c - 
 *    Routines responsible for cd-key management, not the crypto part
 */

#include "main.h"
#include "vector.h"
#include "crypto/cdkey.h"
#include "cdkeymgmt.h"

int defcdkeys;

///////////////////////////////////////////////////////////////////////////////


void LoadCDKeys() {
	char asdf[256];
	int i, j, len;
	FILE *file;

	i = 0;
	file = fopen(conf_path_cdkeys, "r");
	if (file) {  ////clear cdkeys if reloaded!
		while (!feof(file)) {
			fgets(asdf, sizeof(asdf), file);
			len = strlen(asdf);
			if (asdf[len - 1] == '\n')
				asdf[len - 1] = 0;

			i++;
			if (CheckWC3Key(asdf)) {
				for (j = 0; j != defcdkeys; j++) {
					if (!strcmp(((LPCDKEY)cdkeys->elem[j])->encoded, asdf))
						goto founddup;
				}
				AddCDKeyToList(asdf, -1);
founddup: ;
			}
		}
		printf(" - loaded %d cdkeys\n",
			cdkeys->numelem - defcdkeys);
		fclose(file);
	} else { 
		printf("WARNING: cannot open %s for reading, errno: %d\n",
			conf_path_cdkeys, geterr);
	}
}


void __fastcall AddCDKeyToList(const char *key, char profile) {
	LPCDKEY lpCDKey;

	lpCDKey = malloc(sizeof(CDKEY));
	strncpy(lpCDKey->encoded, key, sizeof(lpCDKey->encoded));
	lpCDKey->profileusing = profile;
	lpCDKey->status		  = 0;
	cdkeys = VectorAdd(cdkeys, lpCDKey);
}


void ShiftCDKey(int index) {
	int oldkeypos;
	LPCDKEY lpCDKey;

	bot[index]->cdkey->profileusing = -1;
	oldkeypos = curkeypos;
	do {
		curkeypos++;
		if (curkeypos == cdkeys->numelem)
			curkeypos = 0;
		else if (curkeypos == oldkeypos) {
			printf("No usable keys!!\n");
			break;
		}
		lpCDKey = (LPCDKEY)cdkeys->elem[curkeypos];
	} while (!(lpCDKey->status & KEY_BAD) && lpCDKey->profileusing == -1);
	bot[index]->cdkey = (LPCDKEY)cdkeys->elem[curkeypos];
}

