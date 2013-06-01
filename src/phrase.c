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
 * phrase.c - 
 *    Routines to manage phrases said during chat upon which an action occurs
 */

#include "main.h"
#include "fxns.h"
#include "radix.h"
#include "queue.h"
#include "banning.h"
#include "wildcard.h"
#include "blacklist.h"
#include "phrase.h"

LPTNODE phrases;
int nphrases;

///////////////////////////////////////////////////////////////////////////////


//<action> <phrase> [reason]
void LoadPhrases() {
	char asdf[256], *phrase, *reason, *tmp;
	int len;
	FILE *file;

	nphrases = 0;

	file = fopen(conf_path_phrase, "r");
	if (file) {
		while (!feof(file)) {
			if (!fgets(asdf, sizeof(asdf), file)) {
				if (!feof(file))
					printf("LoadPhrases():fgets() failed, errno: %d", geterr);
				break;
			}

			if (*asdf == '#')
				continue;

			len = strlen(asdf);
			if (len < 4)
				continue;

			if (asdf[len - 1] == '\n')
				asdf[len - 1] = 0;

			tmp = strchr(asdf, '"');
			if (!tmp)
				continue;
			tmp++;
			phrase = tmp;
			while (!(tmp[-1] != '\\' && *tmp == '"')) {
				if (!*tmp) {
					printf("WARNING: entry \'%s\' missing closing quotation\n", asdf);
					goto continue_outside_loop;
				}
				tmp++;
			}
			*tmp = 0;
			tmp++;
			if (*tmp)
				reason = tmp + 1;
			else
				reason = "";

			if (!PhraseAdd(*(uint32_t *)asdf & ~0x20202020, phrase, reason)) {
				//TODO: complain here
			}
			continue_outside_loop: ;
		}
		printf(" - loaded %d phrases\n", nphrases);
		fclose(file);
	} else {
		printf("WARNING: cannot open %s for reading, errno: %d\n",
			conf_path_phrase, geterr);
	}
}


void CheckPhrases(char *user, char *text) {
	LPPHRASE lpPhrase;
	char buf[256];

	lpPhrase = FindPhraseban(phrases, text);
	if (lpPhrase) {
		switch (lpPhrase->action) {
			case ACT_KICK:
			case ACT_BAN:
				AddBanQueue(lpPhrase->action, user, lpPhrase->reason);
				break;
			case ACT_SHIT:
				BlacklistAddToDB(user, 'S', NULL, lpPhrase->reason, buf); //////do something on errors!
				break;
			case ACT_SAY: //should probably check for issuing commands to battle.net
				QueueAdd(lpPhrase->phrase, curbotinc());
		}
	}
}


int PhraseAdd(uint32_t action, const char *phrase, const char *reason) {
	LPPHRASE lpPhrase;

	lpPhrase = malloc(sizeof(PHRASE));
	strncpy(lpPhrase->phrase, phrase, sizeof(lpPhrase->phrase));
	lpPhrase->phrase[sizeof(lpPhrase->phrase) - 1] = 0;
	strncpy(lpPhrase->reason, reason, sizeof(lpPhrase->reason));
	lpPhrase->reason[sizeof(lpPhrase->reason) - 1] = 0;

	switch (action) {
		case 'KCIK': //kick
			lpPhrase->action = ACT_KICK;
			break;
		case 'NAB':  //ban
			lpPhrase->action = ACT_BAN;
			break;
		case 'TIHS': //shit
			lpPhrase->action = ACT_SHIT;
			break;
		case 'YAS':  //say
			lpPhrase->action = ACT_SAY;
			break;
		default:
			return 0;
			//printf("unsupported action \'%s\'\n", asdf);
	}
	phrases = RadixInsert(phrases, phrase, lpPhrase, 0);
	nphrases++;
	return 1;
}


void PhraseAddToDB(uint32_t action, const char *phrase, const char *reason) {

	//PhraseAdd(action, phrase, reason);
}


void PhraseRemoveFromDB(const char *phrase) {
	if (!RadixRemove(phrases, phrase)) {
	
	}
	//STUB
}


char *HandlePhraseCmd(PLUSER pluser, const char *args, char *outbuf) {
	//STUB
	return NULL;
}

