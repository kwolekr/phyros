/*-
 * Copyright (c) 2008 Ryan Kwolek
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
 * chat.c - 
 *    Contains chat event handlers and support routines for chatting
 */


#include "main.h"
#include "hashtable.h"
#include "fxns.h"
#include "commands.h"
#include "radix.h"
#include "banning.h"
#include "pbuffer.h"
#include "blacklist.h"
#include "phrase.h"
#include "asmbits.h"
#include "chat.h"

unsigned int floodthresh_tick;
unsigned int floodthresh_numticks;
unsigned int floodthresh_over;

unsigned int floodtick;
unsigned int lastjoin;

unsigned int curuserlen;

unsigned int nbots_online;
unsigned int nbots_inchannel;
unsigned int nbots_opped;

WHISP lastwhisper = {-1};


///////////////////////////////////////////////////////////////////////////////


void Parse0x0F(char *data, int index) {
	char fixeduser[USERNAME_MAX_LEN];
	uint32_t eventid = *((uint32_t *)data + 1);
	uint32_t flags = *((uint32_t *)data + 2);
	uint32_t ping = *((uint32_t *)data + 3);
	char *user = data + 28;
	char *text = data + 29 + strlen(user);

	if ((gstate & GFS_GETTINGUSERS) && (eventid != 0x01)) {
		putchar('\n');
		curuserlen = 0;
		gstate &= ~GFS_GETTINGUSERS;
	}
	if ((bot[index]->fstate & BFS_GETTINGSWEEP) && (eventid != 0x12))
		bot[index]->fstate &= ~BFS_GETTINGSWEEP;

	switch (eventid) {
		case 0x01:
		case 0x02:
			if (index == masterbot) {
				RealmFix(user, fixeduser, 0);			
				ChatHandleJoin(fixeduser, flags, ping, text, eventid == 2, index);
			}
			break;
		case 0x03:
			if (index == masterbot) {
				RealmFix(user, fixeduser, 0);			
				ChatHandleLeave(fixeduser, flags, index);
			}
			break;
		case 0x04: //////////////make this per profile, maybe?

			lastwhisper.profile = index;
			strncpy(lastwhisper.user, user, sizeof(lastwhisper.user));
			strncpy(lastwhisper.message, text, sizeof(lastwhisper.message));
			lastwhisper.user[sizeof(lastwhisper.user) - 1] = 0;
			lastwhisper.message[sizeof(lastwhisper.message) - 1] = 0;
			
			RealmFix(user, fixeduser, 0);

			printf("<From: %s> %s\n", fixeduser, text);
			CheckCommand(fixeduser, text, 1, index);
			break;
		case 0x05:
			if (index == masterbot) {
				RealmFix(user, fixeduser, 0);			
				ChatHandleTalk(fixeduser, text, index);
			}
			break;
		case 0x06:
			if (index == masterbot)
				printf("Broadcast: %s\n", text);
			break;
		case 0x07:
			ChatHandleChannel(text, flags, index);
			break;
		case 0x09:
			ChatHandleFlagsUpdate(user, flags, index);
			break;
		case 0x0A:
			printf("<To: %s> %s\n", user, text);
			break;
		case 0x0F:
			printf("%s has Clan Private enabled.\n", text);
			break;
		case 0x12:
			ChatHandleInfo(user, text, index);
			break;
		case 0x13:
			printf("error: %s\n", text);
			break;
		case 0x17:
			if (index == masterbot) {
				RealmFix(user, fixeduser, 0);
				ChatHandleEmote(fixeduser, text, index);
			}
			break;
		default:
			printf("[%d] unhandled chat eid 0x%02x!\n" \
				"   user: %s\n   flags: 0x%02x, ping: %d\n   text: %s",
				index, eventid, user, flags, ping, text);
	}
}


void ChatHandleJoin(char *user, uint32_t flags, uint32_t ping,
					char *text, int joined, int index) {
	char tmpclient[8], ts[16];
	unsigned int tick;
	LPCHUSER tmpUser;

	tmpUser = HtGetItem(user, users, TL_BNUSERS);
	if (!joined) {
		gstate |= GFS_GETTINGUSERS;	
		if (tmpUser && (tmpUser->iflags & CHUSER_IF_WC3)) {
			tmpUser->clan = GetClanFromStatstring(text);
			printf("Clan status of %s updated.\n", user);
			return;
		}
	}

	tick = gettick();
	tmpUser		      = malloc(sizeof(CHUSER));
	tmpUser->flags	  = flags;
	tmpUser->ping	  = ping;
	tmpUser->client	  = *(uint32_t *)text;
	tmpUser->iflags	  = GetClientIFlags(tmpUser->client);
	tmpUser->jointick = tick;
	tmpUser->clan     = (tmpUser->iflags & CHUSER_IF_WC3) ? GetClanFromStatstring(text) : 0;
	strncpy(tmpUser->username, user, sizeof(tmpUser->username));
	tmpUser->username[sizeof(tmpUser->username) - 1] = 0;

	HtInsertItem(user, tmpUser, users, TL_BNUSERS);

	if (joined) {
		if ((tick - lastjoin) < floodthresh_tick) {
			floodtick++;
			if (floodtick >= floodthresh_numticks)
				gstate |= GFS_FLOODED;
		}
	}
	lastjoin = tick;
	numusers++;

	if (joined)
		CheckBan(tmpUser, text, index);

	if (!(gstate & GFS_FLOODED)) {
		*(uint32_t *)tmpclient = tmpUser->client;
		tmpclient[4] = 0;
		fastswap32((uint32_t *)tmpclient);
		if (joined) {
			GetTimeStamp(ts);
			printf("%s -- %s [%ums] [0x%02x] has joined using %s.\n",
				ts, user, ping, flags, tmpclient);
		} else {
			JoinNeatPrint(user);
			if (!strcmp(user, bot[index]->realname))
				bot[index]->flags = flags;
		}
	}
}


void __fastcall ChatHandleLeave(char *user, int flags, int index) {
	LPCHUSER lpchuser;
	char timestamp[16];

	lpchuser = HtGetItem(user, users, TL_BNUSERS);
	if (!lpchuser) {
		printf("WARNING: EID_LEAVE received for user \'%s\' but not present in channel\n", user);	
		return;
	}

	if (flags & 0x0F)
		numopbots--;
	
	if (!(gstate & GFS_FLOODED)) {
		GetTimeStamp(timestamp);
		printf("%s -- %s has left.\n", timestamp, user);
	}
	if (gstate & GFS_ADVTRACKBANS) {
		if (lpchuser->iflags & CHUSER_IF_PENDINGBAN) {
			//actually add to the banned list now
			char *buf = malloc(USERNAME_MAX_LEN);
			
			strncpy(buf, user, USERNAME_MAX_LEN); //fix realms to be relative to SC
			buf[USERNAME_MAX_LEN - 1] = 0;

			HtInsertItem(buf, buf, banned, TL_BANNED);
			//lpchuser->iflags &= ~CHUSER_BAN_EXPECTED;
			//lpchuser->iflags |= CHUSER_BANNED;

			RealmReverse(buf, buf, bot[index]->fstate & BFS_CLIENT_WC3);
			lpchuser = HtGetItem(buf, users, TL_BNUSERS);
			if (lpchuser)
				lpchuser->iflags &= ~CHUSER_IF_PENDINGBAN;
		} //TODO: figure this mess out later
	}
	//lpchuser->iflags |= CHUSER_IF_LEFT;
	HtRemoveItem(user, users, TL_BNUSERS);
	numusers--;
}


void __fastcall ChatHandleTalk(char *user, char *text, int index) {
	char timestamp[16];

	if (!(gstate & GFS_FLOODED)) {
		GetTimeStamp(timestamp);
		printf("%s <%s> %s\n", timestamp, user, text);
		CheckPhrases(user, text);
		CheckCommand(user, text, 1, index);
	}
}


void __fastcall ChatHandleEmote(char *user, char *text, int index) {
	char timestamp[16];

	if (!(gstate & GFS_FLOODED)) {
		GetTimeStamp(timestamp);
		printf("%s <%s %s>\n", timestamp, user, text);
		CheckPhrases(user, text); ////////should make this an option!
	}
}


void __fastcall ChatHandleChannel(char *text, uint32_t flags, int index) {
	int i;

	strncpy(bot[index]->curchannel, text, sizeof(bot[index]->curchannel));
	if (index == masterbot) {
		HtResetContents(users, TL_BNUSERS);
		HtResetContents(banned, TL_BANNED);
		numusers  = 0;
		numopbots = 0;
		bancount  = 0;
		gstate &= ~GFS_FLOODED;
		printf(" -- Joined channel %s -- Flags: 0x%08x.\n  Users:\n", text, flags);
		curchan = bot[masterbot]->curchannel;
		nbots_inchannel = 0;
		nbots_opped		= 0;
		for (i = 0; i != numbots; i++) {
			if (bot[i] && !strcmp(curchan, bot[i]->curchannel))
				nbots_inchannel++;
		}
	} else {
		nbots_inchannel += strcmp(curchan, text) ? -1 : 1;
	}
}


void __fastcall ChatHandleFlagsUpdate(char *user, uint32_t flags, int index) {
	LPCHUSER lpUser;
	
	lpUser = HtGetItem(user, users, TL_BNUSERS);
	if (!lpUser)
		return;

	if (index == masterbot) {
		if ((flags & 0x0F) && !(lpUser->flags & 0x0F)) {
			printf("%s has gained operator status.\n", user);
			numopbots++;
		}
	}

	lpUser->flags = flags;

	if (flags & 0x20) {
		//squelched - means ipban
		//RealmFix(user, foo, 0);
	} else if (!strcmp(user, bot[index]->realname)) {
		bot[index]->flags = flags;
		if (flags & 0x0F)
			nbots_opped++;
	}
}


void __fastcall ChatHandleInfo(char *user, char *text, int index) {
	char *btext;

	if (index == masterbot) {
		printf("%s\n", text);

		btext = strstr(text, "ban");
		if (btext && btext - text >= 4)
			BanlistUpdate(user, text, *(uint32_t *)(btext - 4), index);
	}

	SweepGatherUsers(text, index);
}


void __fastcall SendText(char *text, int index) {
	char timestamp[16];

	if (*text) {
		#ifdef DEBUG_QUEUE
			printf("[%d] Send: %s\n", index, text);
		#endif
		if (bot[index]->connected) {
			InsertNTString(text);
			SendPacket(0x0E, index);
			if (*text != '/') {
				GetTimeStamp(timestamp);
				printf("%s <%s> %s\n", timestamp, bot[index]->realname, text);
			}
		}
	}
}


void __fastcall RejoinChannel(int index) {
	SendPacket(0x10, index);
	InsertDWORD(0x02);
	InsertNTString(bot[index]->curchannel);
	SendPacket(0x0C, index);
}


void JoinNeatPrint(const char *user) {
	unsigned int tmplen;
	unsigned int lol;

	tmplen = strlen(user) + 5;
	tmplen = (tmplen > 20) ? 40 : 20;

	if (curuserlen + tmplen > 80) {
		if (curuserlen != 80)
			putchar('\n');
		curuserlen = tmplen;
	} else {
		curuserlen += tmplen;
	}
	lol = printf("[ %s", user) + 2;
	if (tmplen > lol) {
		while (tmplen - lol++)
			putchar(' ');
	}
	putchar(']');
	putchar(' ');
}


void GetRealms() {
	switch (*(int32_t *)(server + 2)) {
		case '042.':
		case 'tsae':
			realm    = "@useast";
			realmwc3 = "@azeroth";
			break;
		case '142.':
		case 'tsew':
			realm    = "@uswest";
			realmwc3 = "@lordaeron";
			break;
		case '42.3':
		case 'epor':
			realm    = "@europe";
			realmwc3 = "@northrend";
			break;
		case '32.1':
		case 'aisa':
			realm    = "@asia";
			realmwc3 = "@kalimdor";
			break;
		default:
			realm    = "@unk";
			realmwc3 = "@unk";
	}
}


unsigned short GetClientIFlags(uint32_t client) {
	switch (client) {
		case 'STAR':
			return CHUSER_IF_STAR;
		case 'SEXP':
			return CHUSER_IF_SEXP;
		case 'SSHR':
			return CHUSER_IF_SSHR;
		case 'JSTR':
			return CHUSER_IF_JSTR;

		case 'DSHR':
			return CHUSER_IF_DSHR;
		case 'DRTL':
			return CHUSER_IF_DRTL;
		case 'D2DV':
			return CHUSER_IF_D2DV;
		case 'D2XP':
			return CHUSER_IF_D2XP;

		case 'W2BN':
			return CHUSER_IF_W2BN;
		case 'WAR3':
			return CHUSER_IF_WAR3;
		case 'W3XP':
			return CHUSER_IF_W3XP;

		default:
			return 0;
	}
}


void __fastcall RealmFix(const char *user, char *dest, int wc3) {
	char *tmp;
	//this is supposed to justify the realm suffix in context of (wc3)
	// if in the context of wc3:
	//   - names already realm suffixed 
	//
	//
	//note: dest is assumed to be size of at least USER_NAME_MAX_LEN

	strncpy(dest, user, USERNAME_MAX_LEN);
	dest[USERNAME_MAX_LEN - 1] = 0;
	tmp = strrchr(dest, '@');
	if (tmp) {
		if (wc3) {
			if ((*(int32_t *)tmp | 0x20202000) == *(int32_t *)realmwc3) 
				*tmp = 0;
		} else {
			if ((*(int32_t *)tmp | 0x20202000) == *(int32_t *)realm)
				*tmp = 0;
		}
	} else {
		strncat(dest, wc3 ? realm : realmwc3, USERNAME_MAX_LEN);
		dest[USERNAME_MAX_LEN - 1] = 0;
	}
}


void __fastcall RealmFixInplace(char *user, int wc3) {
	char *tmp;

	tmp = strrchr(user, '@');
	if (tmp) {
		if (wc3) {
			if ((*(int32_t *)tmp | 0x20202000) == *(int32_t *)realmwc3) 
				*tmp = 0;
		} else {
			if ((*(int32_t *)tmp | 0x20202000) == *(int32_t *)realm)
				*tmp = 0;
		}
	} else {
		strncat(user, wc3 ? realm : realmwc3, USERNAME_MAX_LEN);
		user[USERNAME_MAX_LEN - 1] = 0;
	}
}


void __fastcall RealmReverse(const char *user, char *dest, int wc3) {
	char *tmp;
	int realm_is_same;

	realm_is_same = 0;

	strncpy(dest, user, USERNAME_MAX_LEN);
	dest[USERNAME_MAX_LEN - 1] = 0;

	tmp = strrchr(dest, '@');
	if (tmp) {
		realm_is_same = (*(int32_t *)tmp | 0x20202000) == (wc3 ? *(int32_t *)realmwc3 : *(int32_t *)realm);
		*tmp = 0;
	}

	if (!tmp || realm_is_same) {
		strncat(dest, (wc3 ? realm : realmwc3), USERNAME_MAX_LEN);
		dest[USERNAME_MAX_LEN - 1] = 0;
	}
}


void __fastcall RealmTagStrip(const char *user, char *dest) {
	char *tmp;

	strncpy(dest, user, USERNAME_MAX_LEN);
	dest[USERNAME_MAX_LEN - 1] = 0;

	tmp = strrchr(dest, '@');
	if (tmp) {
		if (((*(int32_t *)tmp | 0x20202000) == *(int32_t *)realm) ||
			((*(int32_t *)tmp | 0x20202000) == *(int32_t *)realmwc3))
			*tmp = 0;
	}
}


int __fastcall RealmTagIsPresent(const char *user) {
	char *tmp;

	tmp = strrchr(user, '@');
	if (tmp) {
		if (((*(int32_t *)tmp | 0x20202000) == *(int32_t *)realm) ||
			((*(int32_t *)tmp | 0x20202000) == *(int32_t *)realmwc3))
			return 1;
	}
	return 0;
}


uint32_t GetClanFromStatstring(char *text) {
	char blah[8], *tmp;

	if (countchr(text, ' ') == 3) {
		tmp = strrchr(text, ' ');
		if (!tmp)
			return 0;
		strncpy(blah, tmp + 1, sizeof(blah));
		return *(uint32_t *)blah;
	}
	return 0;
}


int IsAltCaps(const char *str) {
	const char *s;
	int flip, count;

	flip = count = 0;
	s = str;
	while (*s) { //TODO: fix this, not very accurate
		if (((*s | 0x20) >= 'a') && ((*s | 0x20) <= 'z')) {
			if (flip) {
				if (*s & 0x20) {
					flip = ~flip;
					count++;
				}
			} else {
				flip = ~flip;
			}
		}
		s++;
	}
	return (((s - str) / count) == 2);
}

