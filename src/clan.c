/*-
 * Copyright (c) 2007 Ryan Kwolek
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
 * clan.c - 
 *    Clan management packet routines
 */

#include "main.h"
#include "fxns.h"
#include "queue.h"
#include "pbuffer.h"
#include "asmbits.h"
#include "clan.h"

char clantagchrmap[256] = {
    // 0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

	0xFF, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 
	0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x22, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 
	0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x22, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

const char *clanranks[] = {
	"Peon (Initiate)",
	"Peon",
	"Grunt",
	"Shaman",
	"Chieftain"
};

const char *inviteresstrs[] = {
	"Invitation accepted!",
	"Invalid user.",   		
	NULL,                 
	NULL,				  
	"Invitation declined.",	 
	"Failed to invite user.", 
	NULL,                 
	"Unauthorized to invite.",
	"Cannot invite user.",	
	"Clan is full!"			
};

const char *clanloc[] = {
	"offline",
	"online (not in chat)",
	"in a channel",
	"in a public game",
	"<unknown>",
	"in a private game"
};

const char *clanstatus[] = {
	"Success.",
	"In use.",
	"Too soon.",
	"Not enough members.",
	"Invitation was declined.",
	"Declined.",
	"Accepted.",
	"Not authorized.",
	"User not found.",
	"Clan is full.",
	"Bad tag.",
	"Bad name.",
	"Not in clan."
};

const char *p0x70resstrs[] = {
	"Clan tag available!",
	"Clan tag already taken.",
	"Clanned CDKey.",  
	NULL,			
	NULL,
	NULL,				  
	NULL,				   
	NULL,					
	"Already in a clan.",
	NULL,
	"Invalid clan tag."
};


///////////////////////////////////////////////////////////////////////////////


void Send0x70(char *clantag, int index) {
	uint32_t tag = *(uint32_t *)clantag;
	fastswap32(&tag);
	InsertDWORD(0x3713);
	InsertDWORD(tag);
	SendPacket(0x70, index);
}


void Send0x71(char *clantag, char *clanname, char *invited, int numinvited, int index) {
	int i;
	char asdf[5];
	uint32_t tag;

	strncpy(clantag, asdf, 4);
	tag = *(uint32_t *)asdf;
	InsertDWORD(0x3713);
	InsertNTString(clanname);
	InsertDWORD(tag);
	InsertByte((unsigned char)numinvited);
	for (i = 0; i != numinvited; i++)
		InsertNTString(invited + (i << 5));
	SendPacket(0x71, index);
}


void Send0x72(int response, int index) {
	if (bot[index]->invited.tag) {
		InsertDWORD(bot[index]->invited.cookie);
		InsertDWORD(bot[index]->invited.tag);
		InsertNTString(bot[index]->invited.inviter);
		InsertByte((unsigned char)(response == 'y' ? 0x06 : 0x04));
		SendPacket(0x79, index);
	}
	bot[index]->invited.tag		   = 0;
	bot[index]->invited.cookie	   = 0;
	bot[index]->invited.inviter[0] = 0;
}


void Send0x74(char *user, int index) { //change chieftain
	InsertDWORD(0x3713);
	InsertNTString(user);
	SendPacket(0x74, index);
}


void Send0x77(char *user, int index) { //clan join invite
	InsertDWORD(0x3713);
	InsertNTString(user);
	SendPacket(0x77, index);
}


void Send0x78(char *user, int index) { //remove member
	InsertDWORD(0x3713);
	InsertNTString(user ? user : "");
	SendPacket(0x78, index);
}


void Send0x79(int response, int index) {
	if (bot[index]->invited.tag) {
		InsertDWORD(bot[index]->invited.cookie);
		InsertDWORD(bot[index]->invited.tag);
		InsertNTString(bot[index]->invited.inviter);
		InsertByte((unsigned char)(response == 'y' ? 0x06 : 0x04));
		SendPacket(0x79, index);
	}
	bot[index]->invited.tag		   = 0;
	bot[index]->invited.cookie	   = 0;
	bot[index]->invited.inviter[0] = 0;
}


void Send0x7A(char *user, unsigned char newrank, int index) {
	InsertDWORD(0x3713);
	InsertNTString(user);
	InsertByte(newrank);									 
	SendPacket(0x7A, index);
}


void Send0x7D(int index) {
	InsertDWORD(0x3713);
	SendPacket(0x7D, index);
}


void Send0x82(char *user, int index) {
	InsertDWORD(0x3713);
	InsertDWORD(0); //clan tag
	InsertNTString(user);
	SendPacket(0x82, index);
}


/////////////////////////////////////////////////////////////////


void Parse0x70(char *data, int index) {
	int i;
	char asdf[64], *tmp, *curdata;

	if ((unsigned char)data[8] >= ARRAYLEN(p0x70resstrs)) {
		printf("WARNING: server response parameter out of bounds\n");
		return;
	}

	if (!p0x70resstrs[(int)data[8]]) {
		printf("WARNING: unknown 0x70 result!\n");
		return;
	}

	if (bot[index]->fstate & BFS_EXTERNALREQ)
		QueueAdd((char *)p0x70resstrs[(int)data[8]], index);
	else
		puts(p0x70resstrs[(int)data[8]]);

	if (!data[8]) {
		if (data[9] > 0) {
			sprintf(asdf, "Found %d candidates:", data[9]);
			tmp = asdf;
		} else {
		 	tmp = "Found no candidates.";
		}
		if (bot[index]->fstate & BFS_EXTERNALREQ)
			QueueAdd(tmp, index);
		else
			puts(tmp);

		if (data[9] > 0) {
			curdata = data + 10;
			for (i = 0; i != data[9]; i++) {
				curdata += strlen(curdata) + 1;
				curdata[-1] = ' ';
			}
			curdata[-1] = 0;
			puts(data + 10);
		} 
	}	
	bot[index]->fstate &= ~BFS_EXTERNALREQ;
}


void Parse0x71(char *data, int index) {
	int len;
	char asdf[64], *resultstr, *tmp;

	resultstr = NULL;
	if (data[8]) {
		switch (data[8]) {
			case 4:
				resultstr = "Declined:";
				break;
			case 5:
				resultstr = "Not available:";
				break;
			default:
				sprintf(asdf, "Unrecognized clan creation failure code: 0x%02x.", (unsigned char)data[8]);
				resultstr = asdf;
		}
		tmp = data + 9;
		while (*tmp) {
			len = strlen(tmp);
			tmp[len - 1] = ' ';
			tmp += len + 1;
		}
	} else {
		resultstr = "Successfully created clan!";
	}	
	
	if (bot[index]->fstate & BFS_EXTERNALREQ) {
		QueueAdd(resultstr, index);
		if (data[8])
			QueueAdd(data + 9, index);
		bot[index]->fstate &= ~BFS_EXTERNALREQ;
	} else {
		puts(resultstr);
		if (data[8])
			puts(data + 9);
	}

}


void Parse0x72(char *data, int index) {
	char clanstr[8], asdf[64];
	char *clanname, *invitinguser, *curpos;
	int numinvited;

	if (gstate & GFS_ACCEPTINVS) {
		bot[index]->invited.cookie = *(uint32_t *)(data + 4);
		bot[index]->invited.tag    = *(uint32_t *)(data + 8);
		*(uint32_t *)clanstr	   = *(uint32_t *)(data + 8);
		fastswap32((uint32_t *)clanstr);
		clanstr[4] = 0;
		clanname = data + 12;
		invitinguser = clanname + strlen(clanname) + 1;
		strncpy(bot[index]->invited.inviter, invitinguser,
			sizeof(bot[index]->invited.inviter));

		curpos = invitinguser + strlen(invitinguser) + 1;
		numinvited = *curpos;

		sprintf(asdf, "%s has invited me and %d others to create %s (Tag: %s). Accept?",
			bot[index]->invited.inviter, numinvited, clanname, clanstr);
		QueueAdd(asdf, index);
		//curpos++;
		//for (i = 0; i != numinvited; i++) {
		//	printf(curpos);	   ////
		//	curpos += strlen(curpos) + 1;
		//}
		bot[index]->fstate |= BFS_CREATEINV;
	}
}


void Parse0x73(char *data, int index) {	//Disband result
	if ((unsigned char)data[8] >= ARRAYLEN(clanstatus)) {
		printf("WARNING: server response parameter out of bounds\n");
		return;
	}

	if (bot[index]->fstate & BFS_EXTERNALREQ) {
		QueueAdd((char *)clanstatus[(int)data[8]], index);
		bot[index]->fstate &= ~BFS_EXTERNALREQ;
	} else {
		puts(clanstatus[(int)data[8]]);
	}
}


void Parse0x74(char *data, int index) {	//New chief result
	char *result = data[8] ? "Chieftain successfully changed." :
							 "Failed to change chieftain.";
	if (bot[index]->fstate & BFS_EXTERNALREQ) {
		QueueAdd((char *)result, index);
		bot[index]->fstate &= ~BFS_EXTERNALREQ;
	} else {
		puts(result);
	}
}


void Parse0x75(char *data, int index) { //Clan info
	if ((unsigned char)data[9] >= ARRAYLEN(clanranks)) {
		printf("WARNING: server response parameter out of bounds\n");
		return;
	}

	bot[index]->clan = *(int *)(data + 5);
	*(int *)bot[index]->clanstr = *(int *)(data + 5);
	fastswap32((uint32_t *)bot[index]->clanstr);
	bot[index]->clanstr[4] = 0;
	printf("[%d] You are a %s in Clan %s.\n", index, clanranks[(int)data[9]], bot[index]->clanstr);
}


void Parse0x76(char *data, int index) { //Clan removal
	bot[index]->clan	   = 0;
	bot[index]->clanstr[0] = 0;
	printf("[%d] You have been removed from the clan!\n", index);
}


void Parse0x77(char *data, int index) { //Invite result
	if ((unsigned char)data[8] >= ARRAYLEN(clanstatus) ||
		!inviteresstrs[(int)data[8]]) {
		printf("WARNING: server response parameter out of bounds\n");
		return;
	}

	if (bot[index]->fstate & BFS_EXTERNALREQ) {
		QueueAdd((char *)inviteresstrs[(int)data[8]], index);
		bot[index]->fstate &= ~BFS_EXTERNALREQ;
	} else {
		puts(inviteresstrs[(int)data[8]]);
	}
}


void Parse0x78(char *data, int index) { //Clan member remove result
	if ((unsigned char)data[8] >= ARRAYLEN(clanstatus)) {
		printf("WARNING: server response parameter out of bounds\n");
		return;
	}

	if (bot[index]->fstate & BFS_EXTERNALREQ) {
		QueueAdd((char *)clanstatus[(int)data[8]], index);
		bot[index]->fstate &= ~BFS_EXTERNALREQ;
	} else {
		puts(clanstatus[(int)data[8]]);
	}
}


void Parse0x79(char *data, int index) {	//Invitiation response
	char clanstr[8], blah[128], ts[16];
	char *clanname;

	if (gstate & GFS_ACCEPTINVS) {
		bot[index]->invited.time   = gettick();
		bot[index]->invited.cookie = *(uint32_t *)(data + 4);
		bot[index]->invited.tag	   = *(uint32_t *)(data + 8);
		*(uint32_t *)clanstr		   = *(uint32_t *)(data + 8);
		fastswap32((uint32_t *)clanstr);
		clanstr[4] = 0;
		clanname = data + 12;
		strncpy(bot[index]->invited.inviter, data + 13 + strlen(clanname),
			sizeof(bot[index]->invited.inviter));

		GetTimeStamp(ts);
		printf("%s -- %s sent invitation to join %s (Tag: %s)",
			ts, bot[index]->invited.inviter, clanname, clanstr);
		sprintf(blah, "%s has invited me to join %s (Tag: %s). Accept?",
			bot[index]->invited.inviter, clanname, clanstr);
		QueueAdd(blah, index);
	}
}


void Parse0x7A(char *data, int index) {	//Rank change
	if ((unsigned char)data[8] >= ARRAYLEN(clanstatus)) {
		printf("WARNING: server response parameter out of bounds\n");
		return;
	}

	if (bot[index]->fstate & BFS_EXTERNALREQ) {
		QueueAdd((char *)clanstatus[(int)data[8]], index);
		bot[index]->fstate &= ~BFS_EXTERNALREQ;
	} else {
		puts(clanstatus[(int)data[8]]);
	}
}


void Parse0x7C(char *data, int index) {	//MOTD
	if (bot[index]->fstate & BFS_EXTERNALREQ) {
		if (data[12] == '/') {
			data--;
			data[12] = ' ';
		}
		QueueAdd(data + 12, index);
		bot[index]->fstate &= ~BFS_EXTERNALREQ;
	} else {
		puts(data + 12);
	}
}


void Parse0x7D(char *data, int index) {
	int i, i2;
	char *tmp, *user, *buf, *blah;
	char *members[5];
	int rank, memnum[5];
	int membersize[5] = {1, 8, 32, 16, 4};

	memnum[0] = memnum[1] = memnum[2] = memnum[3] = memnum[4] = 0;
	members[0] = malloc(1 * 32);
	members[1] = malloc(8 * 32);
	members[2] = malloc(32 * 32);
	members[3] = malloc(16 * 32);
	members[4] = malloc(4 * 32); //////what the fuck!?
	tmp = data + 9;
					   
	for (i = 0; i != (unsigned char)data[8]; i++) {
		user = tmp;
		tmp += strlen(tmp) + 1;
		rank = *tmp;
		tmp += 2;
		tmp += strlen(tmp) + 1;
		if (rank < 0 || rank > 4) {
			printf("WARNING: server response parameter out of bounds\n");		
			continue;
		}
		strncpy(members[rank] + (memnum[rank] << 5), user, 32);
		memnum[rank]++;
		if (memnum[rank] == membersize[rank]) {
			membersize[rank] <<= 1;
			members[rank] = realloc(members[rank], membersize[rank]);
		}
	}

	buf = malloc((unsigned char)data[8] * 32);
	tmp = buf;
	tmp += sprintf(tmp, "Total Members: %d", (int)data[8]);
	for (i2 = 4; i2 != -1; i2--) {
		if (memnum[i2]) {
			tmp += sprintf(tmp, " | %s: %d - ", clanranks[i2], memnum[i2]);
			for (i = 0; i != memnum[i2]; i++) {
				blah = members[i2] + (i << 5);
				while (*blah)
					*tmp++ = *blah++;
				*(uint16_t *)tmp = ' ,';
				tmp += 2;
			}
			tmp -= 2;
		}
	}
	*tmp = 0;
	if (bot[index]->fstate & BFS_EXTERNALREQ) {
		QueueAdd(buf, index);
		bot[index]->fstate &= ~BFS_EXTERNALREQ;
	} else {
		puts(buf);
	}
	free(buf);
	for (i = 0; i != 5; i++)
		free(members[i]);
}


void Parse0x7E(char *data, int index) {
	printf("%s has been removed from the clan!\n", data + 4);
}


void Parse0x7F(char *data, int index) {
	char *afteruser;

	afteruser = data + strlen(data + 4) + 5;
	if ((unsigned char)afteruser[0] >= ARRAYLEN(clanranks) ||
		(unsigned char)afteruser[1] >= ARRAYLEN(clanranks)) {
		printf("WARNING: server response parameter out of bounds\n");	
		return;
	}

	printf("%s (%s) is now %s (%s).\n",
		data + 4, clanranks[(int)afteruser[0]], clanloc[(int)afteruser[1]], afteruser + 2);
}


void Parse0x81(char *data, int index) {
	if ((unsigned char)data[4] >= ARRAYLEN(clanranks) ||
		(unsigned char)data[5] >= ARRAYLEN(clanranks)) {
		printf("WARNING: server response parameter out of bounds\n");	
		return;
	}
	
	printf("%s has %smoted you from %s to %s.\n",
		data + 6, (data[5] > data[4]) ? "pro" : "de",
		clanranks[(int)data[4]], clanranks[(int)data[5]]);
}


void Parse0x82(char *data, int index) {
	/*
		(DWORD)		 Cookie
		(BYTE)		 Status code
		(STRING) 	 Clan name
		(BYTE)		 User's rank
		(FILETIME)	 Date joined
	*/
	char asdf[64];
	char *tosend, *tmp;

	if (data[8]) {
		if ((unsigned char)data[8] >= ARRAYLEN(clanranks)) {
			printf("WARNING: server response parameter out of bounds\n");
			return;
		}

		tmp = data + 10 + strlen(data + 9);
		sprintf(asdf, "%s | %s | %u %u\n",
			data + 9, clanranks[(int)data[8]],
			*(int *)(tmp + 1), *(int *)(tmp + 5));
		tosend = asdf;
	} else {
		tosend = "Clan member info request failed!";
	}
	if (bot[index]->fstate & BFS_EXTERNALREQ) {
		QueueAdd(tosend, index);
		bot[index]->fstate &= ~BFS_EXTERNALREQ;
	} else {
		printf("%s\n", tosend);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////

//example: !create </t tag> [/n name] </i stringlist> 
char *HandleClanCreateCommand(char *args, int index) {
	int numinvited, tmpsize;
	char *clantag, *clanname, *invbuf, *tmp, *invited;

	if (!args)
		return NULL;
	clantag  = NULL;
	clanname = NULL;
	invbuf   = NULL;
	args++;

	while (args) {
		tmp = strchr(args, '/');
		if (tmp) {
			if (tmp[-1] != ' ')
				continue;
			tmp[-1] = 0;
			tmp++;
		}
		switch (*args) {
			case 't':
			case 'T':
				clantag = skipws(args + 1);
				break;
			case 'n':
			case 'N':
				clanname = skipws(args + 1);
				break;
			case 'i':
			case 'I':	
				invbuf = skipws(args + 1);
		}
		args = tmp;
	}
	if (!((int)clantag | (int)clanname | (int)invbuf))
		return "Missing parameter(s)!";
	
	numinvited = 0;
	tmpsize    = 16;
	invited    = malloc(16 << 5);
	tmp        = invbuf;
	while (tmp) {
		strncpychr(invited + (numinvited << 5), tmp, ' ', 32);
		numinvited++;
		if (numinvited == tmpsize) {
			tmpsize <<= 1;
			invited   = realloc(invited, tmpsize << 5);
		}
		tmp = strchr(tmp, ' ');
		if (tmp)
			tmp++;
	}

	bot[index]->fstate |= BFS_EXTERNALREQ;
	Send0x71(clantag, clanname, invited, numinvited, index);
	free(invited);
	return "Sending creation invitations.";
}


void DemotePromote(const char *shaman, const char *grunt) {
	/*Demote(shaman);
	Promote(grunt);*/
}


void DemoteDesignatePromote() {
	//demote all shamans, designate <user>, promote all shamans
}


void ToggleCP() {
//	"/c priv" : "/c pub";
}

