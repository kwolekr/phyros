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
 * access.c - 
 *    local user database loading, modification, and command response routines
 */


#include "main.h"
#include "fxns.h"
#include "hashtable.h"
#include "commands.h"
#include "access.h"


///////////////////////////////////////////////////////////////////////////////


void LoadUsers() {
	char asdf[128];
	FILE *file;
	int i, access;
	char *sdfg;
	PLUSER pluser;

	i = 0;
	file = fopen(conf_path_access, "r");
	if (file) {
		//HtResetContents(lusers, TL_LUSERS); do this in a special ResetAll routine for reloading!
		while (!feof(file)) {
			fgets(asdf, sizeof(asdf), file);
			sdfg = strchr(asdf, ' ');
			if (!sdfg)
				continue;
			*sdfg++ = 0;
			//lcase(asdf);
			access = atoi(sdfg);
			if (access > 0 && access <= 100) {
				if (!HtGetItem(asdf, lusers, TL_LUSERS)) {
					pluser = malloc(sizeof(LUSER));
					pluser->access = access;
					pluser->flags  = 0;
					strncpy(pluser->username, asdf, sizeof(pluser->username));
					//lcase(pluser->username);
					HtInsertItem(pluser->username, pluser, lusers, TL_LUSERS);
					i++;
				}
			}
		}
		fclose(file);
		printf(" - loaded %d users\n", i);
		numlusers += i;
	} else {
		printf("WARNING: cannot open %s for reading, errno: %d\n",
		conf_path_access, geterr);
	}
}


void AddOwner(char *username) {
	PLUSER pluser = malloc(sizeof(LUSER));
	pluser->access = 102;
	pluser->flags  = 0;
	strncpy(pluser->username, username, sizeof(pluser->username));
	//lcase(pluser->username);
	HtInsertItem(pluser->username, pluser, lusers, TL_LUSERS);
	numlusers++;
}


void AddMasters(char *usernames) {
	PLUSER pluser;
	char *tok = strtok(usernames, " ,");
	while (tok) {
		pluser = malloc(sizeof(LUSER));
		pluser->access = 101;
		pluser->flags  = 0;
		strncpy(pluser->username, tok, sizeof(pluser->username));
		//lcase(pluser->username);
		HtInsertItem(pluser->username, pluser, lusers, TL_LUSERS);
		tok = strtok(NULL, " ,");
		numlusers++;
	}
}


void LoadAccessModifications() {
	char asdf[256], *tmp;
	FILE *file;
	int len, index;

	file = fopen(conf_path_cmdacc, "r");
	if (!file) {
		printf(" - command access unmodified\n");
		return;
	}
	while (!feof(file)) {
		fgets(asdf, sizeof(asdf), file);
		len = strlen(asdf);
		if (asdf[len - 2] == 0x0A)
			asdf[len - 2] = 0;
		tmp = strchr(asdf, ' ');
		if (!tmp)
			continue;
		*tmp++ = 0;
		index = CmdGetIndex(hash((unsigned char *)asdf));
		if (index != -1)
			cmddesc[index].access = atoi(tmp);
	}
	fclose(file);
}


char *HandleUsersCmd() {
	PLUSER pluser, *plusers;
	int i, j, k;
	char *buf, *tmp;

	k = 0;
	plusers = alloca(numlusers * sizeof(PLUSER));
	for (i = 0; i != ARRAYLEN(lusers); i++) {
		if (lusers[i]) {
			for (j = 0; j != lusers[i]->numelem; j++) {
				pluser = lusers[i]->elem[j];
				if (!(pluser->flags & (LUR_SHITLIST | LUR_KICKLIST)))
					plusers[k++] = pluser;
			}
		}
	}

	qsort(plusers, k, sizeof(PLUSER), UserAccessComparator);

	//this looks unsafe, but it should NEVER overflow.
	//32 for username, 3 for access, 5 for separators, 48 bytes each, plus 24 for the beginning.
	buf = malloc(k * (sizeof(pluser->username) + 16) + 24); 
	tmp = buf;
	tmp += sprintf(tmp,	"%d users: ", k);
	for (i = 0; i != k; i++)
		tmp += sprintf(tmp, "%s - %d, ", (plusers[i])->username, (plusers[i])->access);
	tmp[-2] = 0;
	return buf;
}


int UserAccessComparator(const void *elem1, const void *elem2) {
	return (*(PLUSER *)elem2)->access - (*(PLUSER *)elem1)->access;
}


void HandleAddUserCmd(PLUSER pluser, char *args, char *outbuf) {
	int access, oldaccess;
	char *arg2;
	PLUSER tmpuser;
	FILE *file;

	if (!args)
		return;

	arg2 = strchr(args, ' ');
	if (!arg2)
		return;
	*arg2++ = 0;
	access = atoi(arg2);
	if ((access >= pluser->access) || access < 0 || access > 100) {
		strcpy(outbuf, "Access out of range.");
		return;
	}

	//lcase(args);
	tmpuser = HtGetItem(args, lusers, TL_LUSERS);

	if (tmpuser && tmpuser->access >= pluser->access)
		return;

	if (!access) {
		HandleRemoveUserCmd(pluser, args, outbuf);
		return;
	}

	if (tmpuser) {
		oldaccess       = tmpuser->access;
		tmpuser->access = access;

		FileModifyRecord(conf_path_access,  args, access, AccessFileCompare, AccessFileModifyRecord);
		sprintf(outbuf, "%s\'s access has been set from %d to %d.", args, oldaccess, access);
	} else {
		tmpuser = malloc(sizeof(LUSER));
		tmpuser->access = access;
		strncpy(tmpuser->username, args, sizeof(tmpuser->username));
		HtInsertItem(args, tmpuser, lusers, TL_LUSERS);
		file = fopen(conf_path_access, "a");
		if (!file) {
			printf("WARNING: cannot open %s for append, errno: %d\n",
			conf_path_access, geterr);
			return;
		}
		fprintf(file, "\n%s %d", args, access);
		fclose(file);
		sprintf(outbuf, "%s has been added with %d access.", args, access);
		numlusers++;
	}
}


void HandleRemoveUserCmd(PLUSER pluser, char *args, char *outbuf) {
	PLUSER tmpuser;

	if (!args)
		return;

	//lcase(args);
	tmpuser = HtGetItem(args, lusers, TL_LUSERS);
	if (!tmpuser) {
		sprintf(outbuf, "\'%s\' not found in access database.\n", args);
		return;
	}
	if (tmpuser->access >= pluser->access || tmpuser->access > 100) {
		strcpy(outbuf, "Insufficient access.");
		return;
	}

	if (!FileModifyRecord(conf_path_access, args, 0, AccessFileCompare, FileDeleteRecord))
		return;

	HtRemoveItem(args, lusers, TL_LUSERS);
	sprintf(outbuf, "%s has been removed from the access database.", args);
	numlusers--;
}


int CALLBACK AccessFileCompare(const char *record, char *line) {
	char *tmp;

	tmp = strchr(line, ' ');
	if (tmp)
		*tmp = 0;

	return strcasecmp(record, line); ////modify for wildcards
}


void CALLBACK AccessFileModifyRecord(char *buffer, int data) { 
	char *tmp; //The buffer will always be big enough

	tmp = strchr(buffer, ' ');
	if (tmp) {
		tmp++;
		sprintf(tmp, "%d\n", data);
	}
}

