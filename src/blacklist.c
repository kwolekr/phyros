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
 * blacklist.c - 
 *    Routines to load, add, remove, view, and modify entries of
 *    the blacklist, consisting of the shitlist, kicklist, tagbans, and clanbans 
 */

#include "main.h"
#include "vector.h"
#include "radix.h"
#include "fxns.h"
#include "clan.h"
#include "access.h"
#include "wildcard.h"
#include "hashtable.h"
#include "blacklist.h"

int bl_nshits, bl_nkicks, bl_ntags, bl_nclans;

LPTNODE tagbans[3];
LPVECTOR clanbl[36];


///////////////////////////////////////////////////////////////////////////////


void BlacklistLoad() {
	char asdf[256];
	FILE *file;
	int i, len;
	char *username, *addedby, *reason, *tmp;

	i = 0;
	file = fopen(conf_path_bl, "r");
	if (file) {
		while (!feof(file)) {
			fgets(asdf, sizeof(asdf), file);
			//format: <STCK> <username> [added by] [reason]

			username = addedby = reason = NULL; 
			if (*asdf == '#')
				continue;
			len = strlen(asdf);
			if (len < 4)
				continue;
			if (asdf[len - 1] == '\n')
				asdf[len - 1] = 0;

			username = asdf + 2;
			tmp = strchr(username, ' ');
			if (tmp) {
				*tmp++ = 0;
				addedby = tmp;
				tmp = strchr(addedby, ' ');
				if (tmp) {
					*tmp++ = 0;
					reason = tmp;
				}
			}

			if (!BlacklistAddEntry(username, *asdf, addedby, reason))
				printf("WARNING: bad blacklist format on line %d\n", i);
			i++;
		}
		printf(" - loaded %d blacklisted users\n - loaded %d banned tags\n - loaded %d banned clans\n",
			bl_nshits + bl_nkicks, bl_ntags, bl_nclans); 
	} else {
		printf("WARNING: cannot open %s for reading, errno: %d\n",
			conf_path_bl, geterr);
	}
}


/*
   format:  !bl add shit text reason
--------------------------------
   add, show, remove, delete
   shit, kick, tag, clan
*/
/*
TODO: add wildcards
	Info   - no can do. look up information on a specific one.
	Show   - can do. just filter out the results later on.
	Add    - not doing, too unsafe.  The expansion would be each matching user in the channel.
	Remove - can do this too, maybe.  Just add the filter to the comparison in the removal from file,
			 and same for in-memory data structure (O(n))
*/
char *HandleBlacklistCmd(PLUSER pluser, char *args, char *outbuf) {
	char action, utype, *type, *text, *reason, *tmp;

	if (!args)
		return BlacklistEnumerate('A');

	*outbuf = 0;
	reason = NULL;
	action = toupper((int)*args);
	
	type = strchr(args, ' ');
	if (!type) {
		if (action == 'S' || action == 'L') {
			return BlacklistEnumerate('A');
		} else {
			strcpy(outbuf, "Missing specifier.");
			return NULL;
		}
	}
	*type++ = 0;
	text = strchr(type, ' ');
	if (text) {
		*text++ = 0;
		reason = strchr(type, ' ');
		if (reason)
			*reason++ = 0;
	} else {
		if (action != 'S' && action != 'L') {
			strcpy(outbuf, "Missing specifier.");
			return NULL;
		}
	}

	utype = toupper((unsigned char)*type); 
	switch (action) {
		case 'A': //add
			BlacklistAddToDB(text, utype, pluser->username, reason, outbuf);
			break;
		case 'D': //delete
		case 'R': //remove
			BlacklistRemoveFromDB(text, utype, outbuf);
			break;
		case 'I': //info (on a single bl entry) syntax !bl <info> <type> <identifier>
			BlacklistInfo(outbuf, utype, text);
			break;
		case 'S': //show
		case 'L': //list
			tmp = BlacklistEnumerate(utype);
			if (tmp)
				return tmp;
			else
				strcpy(outbuf, "Invalid type specifier.");
			break;
		default: 
			strcpy(outbuf, "Invalid action specifier.");
	}
	return NULL;
}


char *BlacklistInfo(char *buf, char type, char *text) {
	PBUSER pbuser;
	int tagtype;
	const char *list;
	char formatstr[64], asdf[sizeof(pbuser->username)];
	char *tmp;

	pbuser = NULL;
	switch (type) {
		case 'C':
			list = "clanbans";
			pbuser = ClanBanLookup(*(uint32_t *)text);
			break;
		case 'K':
		case 'S':
			pbuser = HtGetItem(text, lusers, TL_LUSERS);
			if (pbuser && (pbuser->flags & (LUR_SHITLIST | LUR_KICKLIST)))
				list = (pbuser->flags & LUR_KICKLIST) ? "kicklist" : "shitlist";
			else
				pbuser = NULL;
			break;
		case 'T':
			list = "tagbans";
			strcpy(asdf, text);
			tmp = DigestTag(asdf, &tagtype);
			if (tmp)
				pbuser = RadixSearch(tagbans[tagtype], tmp);
			else
				strcpy(buf, "Missing wildcard.");
			break;
		default:
			strcpy(buf, "Invaild type specifier.");
			return buf;
	}
	if (pbuser) {
		strcpy(formatstr, "%s - added to %s ");
		strcat(formatstr, pbuser->addedby[0] ? "by %s for reason: \"%s\"." : "manually.");
		sprintf(buf, formatstr, text, list, pbuser->addedby, pbuser->reason);
	} else {
		sprintf(buf, "%s not found in %s.", text, list);
	}
	return buf;
}


char *BlacklistEnumerate(char type) {
	char *buf, *tmp;

	switch (type) {
		case 'A':
			buf = malloc((bl_nshits + bl_nkicks + bl_nclans) * sizeof(((PBUSER)NULL)->username)
				+ bl_ntags * (sizeof(((PBUSER)NULL)->username) + 2) + (32 * 5) + 16);
			tmp = buf;
			tmp = BlacklistEnumerateShitlist(tmp, LUR_SHITLIST);
			strcpy(tmp, "  |  ");
			tmp += 5;
			tmp = BlacklistEnumerateShitlist(tmp, LUR_KICKLIST);
			strcpy(tmp, "  |  ");
			tmp += 5;
			tmp = BlacklistEnumerateTags(tmp);
			strcpy(tmp, "  |  ");
			tmp += 5;
			tmp = BlacklistEnumerateClans(tmp);
			break;
		case 'C':
			buf = malloc(bl_nclans * sizeof(((PBUSER)NULL)->username) + 32);
			BlacklistEnumerateClans(buf);
			break;
		case 'K':
		case 'S':
			buf = malloc(bl_nkicks * sizeof(((PBUSER)NULL)->username) + 32);
			buf = malloc(bl_nshits * sizeof(((PBUSER)NULL)->username) + 32);
			BlacklistEnumerateShitlist(buf, (type == 'S') ? LUR_SHITLIST : LUR_KICKLIST);
			break;
		case 'T':
			buf = malloc(bl_ntags * (sizeof(((PBUSER)NULL)->username) + 2) + 32);
			BlacklistEnumerateTags(buf);
			break;
		default: 
			return NULL;
	}
	return buf;
}


char *BlacklistEnumerateClans(char *buf) {
	int i, j, n;
	PBUSER pbuser;

	n = 0;
	buf += sprintf(buf, "%d banned clans: ", bl_nclans);

	for (i = 0; i != (sizeof(clanbl) / sizeof(clanbl[0])); i++) {
		if (clanbl[i]) {
			for (j = 0; j != clanbl[i]->numelem; j++) {
				pbuser = clanbl[i]->elem[j];
				strcpy(buf, pbuser->username);
				buf += strlen(pbuser->username);
				n++;
				if (n != bl_nclans) {
					*(uint16_t *)buf  = ' ,';
					buf += 2;
				}
			}
		}
	}
	*buf = 0;
	return buf;
}


char *BlacklistEnumerateShitlist(char *buf, int flags) {
	int i, j, n, nitems;
	PBUSER pbuser;

	n = 0;
	nitems = (flags == LUR_SHITLIST) ? bl_nshits : bl_nkicks;
	buf += sprintf(buf, "%d %slisted users: ", nitems,
		(flags == LUR_SHITLIST) ? "shit" : "kick");

	for (i = 0; i != TL_LUSERS; i++) {
		if (lusers[i]) {
			for (j = 0; j != lusers[i]->numelem; j++) {
				pbuser = lusers[i]->elem[j];
				if (pbuser->flags & flags) {
					strcpy(buf, pbuser->username);
					buf += strlen(pbuser->username);
					n++;
					if (n != nitems) {
						*(uint16_t *)buf  = ' ,';
						buf += 2;
					}
				}
			}
		}
	}
	*buf = 0;
	return buf;
}


char *BlacklistEnumerateTags(char *buf) {
	buf += sprintf(buf, "%d tagbans: ", bl_ntags);

	buf = BlacklistEnumerateTagTree(buf, tagbans[WCP_BACK]);
	*buf++ = ' ';
	buf = BlacklistEnumerateTagTree(buf, tagbans[WCP_FRONT]);
	*buf++ = ' ';
	buf = BlacklistEnumerateTagTree(buf, tagbans[WCP_BOTH]);

	*buf = 0;
	return buf;
}


char *BlacklistEnumerateTagTree(char *buf, LPTNODE tree) {
	void **items;
	int i, nitems;
	PBUSER pbuser;

	items = RadixSearchAll(tree, "", &nitems);
	if (items) {
		for (i = 0; i != nitems; i++) {
			pbuser = items[i];
			strcpy(buf, pbuser->username);
			buf += strlen(pbuser->username);
			if (i != nitems - 1) {
				*(uint16_t *)buf   = ' ,';
				buf += 2;
			}
		}
	}
	return buf;
}


int BlacklistAddEntry(char *text, char type, char *addedby, char *reason) {
	PBUSER buser;
	int bleh;
	unsigned char mchr;

	buser = malloc(sizeof(BUSER));
	strncpy(buser->username, text, sizeof(buser->username));
	if (addedby)
		strncpy(buser->addedby, addedby, sizeof(buser->addedby));
	else
		buser->addedby[0] = 0;
	if (reason)
		strncpy(buser->reason, reason, sizeof(buser->reason));
	else
		buser->reason[0] = 0;

	buser->access = 0;
	buser->flags  = 0;
	buser->oacces = 0;

	type = toupper((unsigned char)type);

	switch (type) {
		case 'C':
			mchr = clantagchrmap[(unsigned int)*text];
			if (mchr == 0xFF)
				return 0;
			clanbl[mchr] = VectorAdd(clanbl[mchr], buser);
			bl_nclans++;
			break;
		case 'K':
		case 'S':
			buser->flags = (type == 'S') ? LUR_SHITLIST : LUR_KICKLIST;
			HtInsertItem(buser->username, buser, lusers, TL_LUSERS);
			if (type == 'S')
				bl_nshits++;
			else
				bl_nkicks++;
			numlusers++;
			break;
		case 'T':
			text = DigestTag(text, &bleh);
			if (!text)
				return 0;
			tagbans[bleh] = RadixInsert(tagbans[bleh], text, buser, 0);
			bl_ntags++;
			break;
		default:
			return 0;
	}
	return 1;
}


void BlacklistAddToDB(char *user, char type, char *addedby, char *reason, char *outbuf) {
	PBUSER pbuser;
	FILE *file;
	char *liststr;

	switch (type) {
		case 'C':
			liststr = "clanbans";
			pbuser = ClanBanLookup(*(uint32_t *)user);
			if (pbuser) {
				sprintf(outbuf, "Clan %s is already a banned clan.", user);
				return;
			}
			#ifdef DONT_ALLOW_OWN_CLANBAN
				if (*(uint32_t *)user == bot[masterbot]->clan) {
					strcpy(outbuf, "Cannot ban own clan.");
					return;
				}
			#endif
			break;
		case 'K':
			liststr = "kicklist";
			goto overshit;
		case 'S':
			liststr = "shitlist";
		overshit:
			pbuser = HtGetItem(user, lusers, TL_LUSERS);
			if (pbuser) {
				if (pbuser->flags & (LUR_SHITLIST | LUR_KICKLIST)) {
					liststr = (pbuser->flags & LUR_SHITLIST) ? "shitlist" : "kicklist";			
					sprintf(outbuf, "User %s already in %s", user, liststr);
				} else {
					sprintf(outbuf, "User %s has access.", user);
				}
				return;	
			}
			break;
		case 'T':
			liststr = "tagbans";
			break;
		default:
			strcpy(outbuf, "Invaild type specifier.");
			return;
	}

	file = fopen(conf_path_bl, "a");
	if (!file) {
		printf("WARNING: cannot open %s for append, errno: %d",
			conf_path_bl, geterr);
		return;
	}
	fprintf(file, "\n%c %s", type, user);
	if (addedby) {
		fprintf(file, " %s", addedby);
		if (reason)
			fprintf(file, " %s", reason);
	}
	fclose(file);
	
	// uh oh, user must be lower case here then... is this a consistency issue??
	if (!BlacklistAddEntry(user, type, addedby, reason)) {
		strcpy(outbuf, "Failed to add entry to internal blacklist.");
		return;
	}

	sprintf(outbuf, "%s has been added to %s.", user, liststr);
}


void BlacklistRemoveFromDB(char *text, char type, char *outbuf) {
	int tagtype, ret;
	PBUSER pbuser;
	

	if (!FileModifyRecord(conf_path_bl, text, 0, BlacklistFileCompare, FileDeleteRecord))
		return;

	switch (type) {
		case 'C': 
			ret = ClanBanRemove(*(uint32_t *)text);
			break;
		case 'K':
		case 'S':
			pbuser = HtGetItem(text, lusers, TL_LUSERS);
			if (pbuser && (pbuser->flags & (LUR_SHITLIST | LUR_KICKLIST)))
				ret = HtRemoveItem(text, lusers, TL_LUSERS);
			else
				ret = 0;
			break;
		case 'T':
			text = DigestTag(text, &tagtype);
			if (!text) {
				strcpy(outbuf, "Missing wildcard.");
				return;
			}
			ret = RadixRemove(tagbans[tagtype], text);
			break;
		default:
			strcpy(outbuf, "Invaild type specifier.");
			return;
	}
	if (ret)
		sprintf(outbuf, "Item \'%s\' removed from blacklist.", text);
	else
		sprintf(outbuf, "Item \'%s\' not found in blacklist.", text);
}


int CALLBACK BlacklistFileCompare(const char *record, char *line) {
	char *asdf;
	int len;

	len = strlen(line);
	if (line[len - 1] == '\n')
		line[len - 1] = 0;

	asdf = strchr(line + 2, ' ');
	if (asdf)
		*asdf = 0;

	return strcasecmp(record, line + 2);
}



PBUSER ClanBanLookup(uint32_t clan) {
	int i;
	unsigned char mchr;
	PBUSER buser;

	mchr = clantagchrmap[clan & 0xFF];
	if ((mchr != 0xFF) && clanbl[mchr]) {
		for (i = 0; i != clanbl[mchr]->numelem; i++) {
			buser = (PBUSER)clanbl[mchr]->elem[i];
			if (clan == *(uint32_t *)buser->username)
				return buser;
		}
	}
	return NULL;
}


int ClanBanRemove(uint32_t clan) {
	int i;
	unsigned char mchr;
	PBUSER buser;

	mchr = clantagchrmap[clan & 0xFF];
	if ((mchr != 0xFF) && clanbl[mchr]) {
		for (i = 0; i != clanbl[mchr]->numelem; i++) {
			buser = (PBUSER)clanbl[mchr]->elem[i];
			if (clan == *(uint32_t *)buser->username) {
				free(buser);
				clanbl[mchr]->numelem--;
				clanbl[mchr]->elem[i] = clanbl[mchr]->elem[clanbl[mchr]->numelem];
				return 1;
			}
		}
	}
	return 0;
}
