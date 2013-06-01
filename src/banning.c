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
 * banning.c - 
 *    maintains banning, blacklist, banqueue, and other related routines
 */


#include "main.h"
#include "fxns.h"
#include "chat.h"
#include "clan.h"
#include "queue.h"
#include "timer.h"
#include "radix.h"
#include "access.h"
#include "vector.h"
#include "pbuffer.h"
#include "blacklist.h"
#include "hashtable.h"
#include "wildcard.h"
#include "banning.h"

const char *buktype[5] = {
	"ban",
	"unban",
	"kick",
	"squelch",
	"unsquelch"
};

//char **banqueue;
LPQUEUENODE bq_head;
LPQUEUENODE bq_tail;
int bq_count;
int bq_max;
//int bqsize;
//int bqpos;

int bots_seen_ban;

int ban_highping, ban_lowping;
int ban_winlow, ban_indexhigh;
int ban_numbershigh, ban_clients;
int ban_kickcount;

int ban_timing_normal, ban_timing_stagban;
int ban_timing_loadban;

//int banmsg_recvd;
//int banmsg_cleared;
//char banmsg_lastuser[USERNAME_MAX_LEN];

char sweep_chan_name[64];
LPVECTOR sweep_chan;

/*
	ban reasons:
	Bused - shitlisted
	Hated - tagbanned
	Pwned - clanbanned
	A - autoload
	L - lockdown
	N - numbers ban
	E - ban evasion
	IP - ipban
	H - highping
	L - lowping
	P - plugban
	I - indexban
	W - winban
	C - clientban
*/


///////////////////////////////////////////////////////////////////////////////
/* 
 * THE BAN QUEUE
 * Starts at 64 elements
 * Grows to as high as 65536
 * After that, it becomes a circular buffer, deleting the oldest entries.
 * Every idle timer, if no bans have occured after 15 seconds,
 *   with no floods/loads detected, the ban queue is reduced back to 64 elements.
 *
 * HOW THE BAN QUEUE PROCESSES REQUESTS
 * Entry is added
 * Queue count has debt added to it for the profile in question
 * Timer is set with configurable value, for normal bans it'll be maybe 4500ms
 * A profile is picked to ban AT THAT TIME, perhaps from the one with the lowest debt, possibly other factors.
 *
 * When adding to the ban queue, do a lookup on the banlist. it should be pending ban or banned already
 * if the state is pending ban, drop.  If the state is banned, check if the ban's been silently overturned by bnet
 * by hitting the banlimit of 160-something and readd in that case.
 */

void AddBanQueue(int action, char *user, char *reason) {
	char tmpuser[32], asdf[64], *banentry;
	PLUSER pluser;
	LPCHUSER chuser;
	LPQUEUENODE bqnode, tmpnode, prevnode;

	RealmFix(user, tmpuser, 0);

	pluser = HtGetItem(tmpuser, lusers, TL_LUSERS);
	if (pluser) {
		if (pluser->access >= 10)
			return;
	}

	if ((action != BTYPE_UNBAN) && (action != BTYPE_FBAN)) {
		chuser = HtGetItem(tmpuser, banned, TL_BANNED);
		if (chuser) {
			printf("Double ban on %s attempted!\n", user); ///:::DEBUG:::
			return;
		}

	}
		
	if (BANTYPE(action) != BTYPE_KICK) {
		banentry = malloc(USERNAME_MAX_LEN);
		strncpy(banentry, tmpuser, sizeof(USERNAME_MAX_LEN));
		banentry[USERNAME_MAX_LEN - 1] = 1 + (BANTYPE(action) == BTYPE_UNBAN); //steal the last byte for status field
		banentry[USERNAME_MAX_LEN - 2] = 0; //NT
		if (!HtInsertItemUnique(tmpuser, banentry, banned, TL_BANNED))
			free(banentry);
	}

	bqnode = malloc(sizeof(QUEUENODE) + ((reason && strlen(reason) >= USERNAME_MAX_LEN) ? 256 : 64));
	bqnode->flags = action;
	bqnode->next  = NULL;
	strcpy(bqnode->text, tmpuser);
	if (reason)
		strncpy(bqnode->text + USERNAME_MAX_LEN, reason, 256 - USERNAME_MAX_LEN);
	else
		bqnode->text[USERNAME_MAX_LEN] = 0;

	if (bq_count >= bq_max) {
		tmpnode  = bq_head;
		prevnode = tmpnode;
		while (tmpnode && (tmpnode->flags & BTYPE_FORCED)) {
			prevnode = tmpnode;
			tmpnode = tmpnode->next;
		}
		if (tmpnode) {
			bq_count--;
			prevnode->next = tmpnode->next;
			free(tmpnode);
		}
	}

	if (action & BTYPE_FORCED) {
		bqnode->next = bq_head;
		bq_head      = bqnode;
		if (!bq_tail)
			bq_tail = bqnode;
	} else {
		bq_tail->next = bqnode;
		bq_tail       = bqnode;
		if (!bq_head)
			bq_head = bqnode;
	}
	bq_count++;
	
	printf("  *** added banqueue entry: action %d, user %s\n", action, user); ///:::DEBUG:::

	if (!(bot[bot_least_debt]->fstate & BFS_BQTIMERACTIVE)) {
		//flags = TIMERID_BANQUEUE;
		/*if (gstate & GFS_LOADBAN) {
			waittime = 30000;
		} else if (gstate & GFS_STAGBAN) {
			waittime = 5000 / numopbots;
		} else {
			flags |= TIMER_ONESHOT;
			waittime = GetQueueTime(35, curbot);
		}*/
		bot[bot_least_debt]->fstate |= BFS_BQTIMERACTIVE;
		SetAsyncTimer(bot_least_debt | TIMERID_BANQUEUE | TIMER_FIRSTSHOT,
			QueueGetRemainingWait(&bot[bot_least_debt]->queue), BanQueueTimerProc);
	}
	
	/*if (action & BTYPE_IP) { // must be added to a special ipb queue, not the chat queue
		sprintf(asdf, "/%s %s", buktype[BANTYPE(action) + 3], tmpuser);
		QueueAdd(asdf, masterbot);
	}*/
}


/*unsigned int GetBanWaitTime() {
	if (gfstate & GFS_LOADBAN)
		//	Loadbanning:
		//	1). Wait 25sec to cool down _EVERYTHING ELSE_
		//	2). Set wait time to 25sec
		//	3). Ban 5 at once
		//		- If an unrelated ban or action happens, remove one ban from the cycle
		
		return 25000;
	else
		return GetQueueTime(35, curbot);
}*/


int BanQueueSend(int index) {
	LPQUEUENODE bqnode;
	int len;

	bq_count--;
	bqnode = bq_head;

	RealmFixInplace(bqnode->text, bot[index]->fstate & BFS_CLIENT_WC3);
	int action = 0; /////////FIXME
	
	// direct pbuffer access to avoid unnecessary recopying
	if (!bqnode->text[USERNAME_MAX_LEN]) {
		len = sprintf(sendbuffer, "/%s %s",
			buktype[BANTYPE(action)], bqnode->text);
	} else {
		len = sprintf(sendbuffer, "/%s %s %s", buktype[BANTYPE(action)],
			bqnode->text, bqnode->text + USERNAME_MAX_LEN);
	}
	pbufferlen += len + 1; 
	SendPacket(0x0E, index);

	bq_head = bqnode->next;
	free(bqnode);

	return len;
}


int CALLBACK BanQueueTimerProc(unsigned int idEvent) {
	int index;

	index = idEvent & ~(TIMERID_BANQUEUE | TIMER_FIRSTSHOT);

	if (!bq_count) {
		printf(" *** ERROR! BanQueueTimerProc called with bq_count == 0!\n");
		bot[index]->fstate &= ~BFS_BQTIMERACTIVE;		
		return 0;
	}

	if (gstate & GFS_LOADBAN) {
		// TODO
	} else if (gstate & GFS_STAGBAN) {
		// TODO
	} else {
		QueueGetTime(BanQueueSend(index), index);
	}

	if (bq_count) {
		if (idEvent & TIMER_FIRSTSHOT) {
			SetAsyncTimer(idEvent & ~TIMER_FIRSTSHOT, ban_timing_normal, BanQueueTimerProc);
			return 0;
		} else {
			return 1;
		}
	} else {
		bot[bot_least_debt]->fstate &= ~BFS_BQTIMERACTIVE;
		return 0;
	}
}


void HandleBUKUserCmd(PLUSER pluser, char *temp, int type, int index) {
	char *arg2;
	int ipban;
	PLUSER tmppluser;

	if (!temp)
		return;
	ipban = 0;
	arg2 = strchr(temp, ' ');
	if (arg2) {
		*arg2++ = 0;
		if (*temp == '/') {
			if (temp[1] == 'i')
				ipban = 1;
			temp = arg2;
			arg2 = strchr(temp, ' ');
			if (arg2)
				*arg2++ = 0;
		} else if (*arg2 == '/') {
			if (arg2[1] == 'i')
				ipban = 1;
			arg2 = strchr(arg2, ' ');
			if (arg2)
				*arg2++ = 0;
		}
	}

	//lcase(temp);
	tmppluser = HtGetItem(temp, lusers, TL_LUSERS);
	if (tmppluser && !(tmppluser->flags & LUR_SHITLIST)) {
		if ((type == BTYPE_BAN) ||
			((pluser->access <= tmppluser->access) && (type != BTYPE_UNBAN)))
		    return;
	}
	
	AddBanQueue(type, temp, arg2);
}


char *HandleBannedCmd() {
	char *asdf, *buf, *tmp;
	int i, j, k;

	asdf = malloc((bancount * 32) + 24);
	buf = asdf;
	buf += sprintf(buf, "%d banned users: ", bancount);
	k = 0;
	for (i = 0; i != TL_BANNED; i++) {
		if (banned[i]) {
			for (j = 0; j != banned[i]->numelem; j++) {
				tmp = banned[i]->elem[j];
				strcpy(buf, tmp);
				buf += strlen(tmp);
				k++;
				if (k != bancount) {
					*(uint16_t *)buf = ', ';
					buf += 2;
				}
			}
		}
	}
	*buf = 0;
	return asdf;
}


void CheckBan(LPCHUSER chuser, char *text, int index) {
	char username[32], *tmp;
	int bufm;
	PBUSER buser;
	/* bans to add:
		- fast rejoin ban
		- finish autoload
	*/

	//lcasencpy(username, chuser->username, sizeof(username));
	strncpy(username, chuser->username, sizeof(username));
	username[sizeof(username) - 1] = 0;

	////////lockdown check
	if (gstate & GFS_LOCKDOWN) {
		AddBanQueue(BTYPE_BAN, username, "L");
		return;
	}

	////////autoload ban check
	if (gstate & (GFS_AUTOLOAD | GFS_FLOODED)) {
		if (0) { ////////////////////////////////
			/*
				check amount of time between last join,
				check numbers,
				then check if plug xor starcraft client,
				then check for a random name (enthropy?)
			*/
			AddBanQueue(BTYPE_BAN, username, "A");
			return;
		}
	}

	if (gstate & GFS_NUMBERSBAN) {
		tmp = strrchr(username, '#');
		if (tmp && (atoi(tmp + 1) >= ban_numbershigh)) {
			AddBanQueue(BTYPE_BAN, username, "N");
			return;
		}
	}

	////////ban evasion check
	if (gstate & GFS_BANEVASION) {
		if (HtGetItem(username, banned, TL_BANNED)) {
			AddBanQueue(BTYPE_BAN, username, "E");
			return;
		}
	}

	////////ipban check
	if ((gstate & GFS_IPBAN) && (chuser->flags & 0x20)) {
		AddBanQueue(BTYPE_BAN, username, "IP");
		return;
	}

	////////shitlist check
	buser = HtGetItem(username, lusers, TL_LUSERS);
	if (buser) {
		if (buser->flags & (LUR_SHITLIST | LUR_KICKLIST)) {
			if (buser->flags & LUR_KICKLIST) {
				buser->oacces++;
				if (buser->oacces == ban_kickcount) {
					buser->oacces = 0;
					bufm = BTYPE_BAN;
				} else {
					bufm = BTYPE_KICK;
				}
			} else {
				bufm = BTYPE_BAN;
			}
			AddBanQueue(bufm, username, buser->reason[0] ? buser->reason : "Bused");
			return;
		}
	}

	////////tagban check
	tmp = FindTagban(tagbans, username); // needs testing
	if (tmp) {
		AddBanQueue(BTYPE_BAN, username, *tmp ? tmp : "Hated");
		return;
	}

	////////clanban check
	buser = ClanBanLookup(chuser->clan);
	if (buser) {
		AddBanQueue(BTYPE_BAN, username, buser->reason[0] ? buser->reason : "Pwned");
		return;
	}

	////////highping check "H"
	if ((gstate & GFS_HIGHPINGBAN) && ((int)chuser->ping >= ban_highping)) {
		AddBanQueue(BTYPE_BAN, username, "H");
		return;
	}

	////////lowping check "L"
	if ((gstate & GFS_LOWPINGBAN) && ((int)chuser->ping <= ban_lowping)) {
		AddBanQueue(BTYPE_BAN, username, "L");
		return;
	}
	
	////////plugban check "P"
	if ((gstate & GFS_PLUGBAN) && (chuser->flags & 0x10)) {
		AddBanQueue(BTYPE_BAN, username, "P");
		return;
	}

	////////indexban check "I"
	if ((gstate & GFS_INDEXBAN) && (numusers >= ban_indexhigh)) {
		AddBanQueue(BTYPE_BAN, username, "I");
		return;
	}

	////////clientban check "C"
	if ((gstate & GFS_CLIENTBAN) && (chuser->iflags & ban_clients)) {
		AddBanQueue(BTYPE_BAN, username, "C");
		return;
	}

	////////winban check "W"
	if (gstate & GFS_WINBAN) {
		if (countchr(text, ' ') == 6) {
			tmp = text + 8;
			while (*tmp) {
				if (*tmp == ' ') {
					*tmp = 0;
					break;
				}
				tmp++;
			}
			if (atoi(text + 8) <= ban_winlow) {
				AddBanQueue(BTYPE_BAN, username, "W");
				return;
			}
		}
	}

	///////alt caps ban
	if ((gstate & GFS_ALTCAPSBAN) && IsAltCaps(chuser->username)) {
		AddBanQueue(BTYPE_BAN, username, "Alt caps");
		return;
	}
}


/* 
 * The banned user is always correctly realm tagged if the ban info message is received by the user 
 * who performed the ban. The username field of the info packet contains the user performing the
 * ban, and always has correct realms. The format of the text field is 'X was banned by Y'.
 *
 * Because the username received on the ban info message is unreliable, bans originating from local
 * bots are inserted into the banned list as a pending ban. the (USERNAME_MAX_LEN - 2)th byte is 1.
 *
 * ====basic ban tracking
 * On ban info, if the user performing the ban was a local bot, then there must be an entry in the
 * banned list already as pending (shouldn't rely on this though, lots of ways it could screw up)
 * Mark that entry to an actual ban (0), or if it doesn't exist for some reason, insert the ban entry.
 * If the user was banned by a non-local user, simply assume the realm is SC.
 *
 * ====advanced ban tracking
 * On ban info, check if either of the variations of the username being banned are present. If so, set flags
 * CHUSER_BAN_EXPECTED and if the other realm is too, CHUSER_BAN_EXPECTED2 as well.
 * On user leave, look up and check if flags & (CHUSER_BAN_EXPECTED | CHUSER_BANEXPECTED2). We know the right
 * realm, and we reset the other user who did not leave.
 *
 */
void BanlistUpdate(char *user, char *text, int action, int index) {
	char *tmp, *blah, buf[USERNAME_MAX_LEN];
	LPCHUSER lpchuser;
	LPLOCALBOT locbot;
	int flags;

	switch (action) {
		case ' saw': //[was ]banned
			tmp = strchr(text, ' ');
			if (!tmp)
				return;
			*tmp = 0;
			
			bancount++;

			RealmFix(user, buf, 1);
			locbot = HtGetItem(buf, localbots, TL_LOCBOTS);
			if (locbot) { //then it was already added; just change the attribute
				RealmFix(text, buf, 0);
				blah = HtGetItem(buf, banned, TL_BANNED);
				if (!blah || blah[USERNAME_MAX_LEN - 2] != BANLIST_BAN_PENDING) {
					strncat(buf, realmwc3, USERNAME_MAX_LEN);
					buf[USERNAME_MAX_LEN - 1] = 0;
					blah = HtGetItem(buf, banned, TL_BANNED);
					if (!blah) {
						printf("ERROR: User %s banned by bot %d but not in banned list!\n",
							text, locbot->index);
						goto insertbanned;
					}
				}
				blah[USERNAME_MAX_LEN - 2] = BANLIST_CLEAR;
			} else {
				if (gstate & GFS_ADVTRACKBANS) { 
					lpchuser = HtGetItem(text, users, TL_BNUSERS);
					if (lpchuser) {
						if (lpchuser->iflags & CHUSER_IF_PENDINGBAN)
							lpchuser->iflags |= CHUSER_IF_PENDINGBAN2;
						else
							lpchuser->iflags |= CHUSER_IF_PENDINGBAN;
					}

					strncpy(buf, text, sizeof(buf));
					strncat(buf, realmwc3, sizeof(buf));
					buf[sizeof(buf) - 1] = 0;
					lpchuser = HtGetItem(buf, users, TL_BNUSERS);
					if (lpchuser) {
						if (lpchuser->iflags & CHUSER_IF_PENDINGBAN)
							lpchuser->iflags |= CHUSER_IF_PENDINGBAN2;
						else
							lpchuser->iflags |= CHUSER_IF_PENDINGBAN;
					}
				} else {
				insertbanned:
					blah = malloc(USERNAME_MAX_LEN);

					strncpy(blah, text, USERNAME_MAX_LEN); //assume SC - don't append tag
					blah[USERNAME_MAX_LEN - 1] = 0;
					blah[USERNAME_MAX_LEN - 2] = 0;
						
					HtInsertItemUnique(blah, blah, banned, TL_BANNED);
				}
			}
			break;
		case 'nu s': //wa[s un]banned
			/*
			 * Nothing much can be done here since there is no "sibling action" to unbanning.
			 *
			 * If realm is not explicitly specified via realm tag, check which realm from user's presense in banned list. 
			 * If username from both realms is banned, or neither are in banned list, 
			 * justify realms under assumption that the banned user is SC if both are in banlist.
			 */
			tmp = strchr(text, ' ');
			if (!tmp)
				return;
			*tmp = 0;

			bancount--;

			RealmFix(user, buf, 1);
			locbot = HtGetItem(buf, localbots, TL_LOCBOTS);
			if (locbot) { // fix all this 

				RealmFix(text, buf, 0);
				blah = HtGetItem(buf, banned, TL_BANNED);
				if (!blah) {
					strncat(buf, realmwc3, USERNAME_MAX_LEN);
					buf[USERNAME_MAX_LEN - 1] = 0;
					blah = HtGetItem(buf, banned, TL_BANNED);
					if (!blah) {
	
					}
				}

				blah[USERNAME_MAX_LEN - 1] = 0;
			} else {
				if (gstate & GFS_ADVTRACKBANS) { 
					/*lpchuser = HtGetItem(text, users, TL_BNUSERS);
					if (lpchuser) {
						if (lpchuser->iflags & CHUSER_IF_PENDINGBAN)
							lpchuser->iflags |= CHUSER_IF_PENDINGBAN2;
						else
							lpchuser->iflags |= CHUSER_IF_PENDINGBAN;
					}

					strncpy(buf, text, sizeof(buf));
					strncat(buf, realmwc3, sizeof(buf));
					buf[sizeof(buf) - 1] = 0;
					lpchuser = HtGetItem(buf, users, TL_BNUSERS);
					if (lpchuser) {
						if (lpchuser->iflags & CHUSER_IF_PENDINGBAN)
							lpchuser->iflags |= CHUSER_IF_PENDINGBAN2;
						else
							lpchuser->iflags |= CHUSER_IF_PENDINGBAN;
					}*/
				} else {
				/*insertbanned:
					blah = malloc(USERNAME_MAX_LEN);

					strncpy(blah, text, USERNAME_MAX_LEN); //assume SC - don't append tag
					blah[USERNAME_MAX_LEN - 1] = 0;
					blah[USERNAME_MAX_LEN - 2] = 0;
						
					HtInsertItemUnique(blah, blah, banned, TL_BANNED);*/
				}
			}

			if (gstate & GFS_ADVTRACKBANS) {
				//HtRemoveItem(text, banned, TL_BANNED);
			}
			break;
		default:
			;
	}
}


char *SweepStart(const char *channel, char *outbuf) {
	int i;

	if (gstate & GFS_SWEEPING) {
		sprintf(outbuf, "Currently sweeping %s", sweep_chan_name);
		return outbuf;
	} else {
		if (channel) {
			sprintf(outbuf, "/who %s", channel);
			for (i = 0; i != numbots; i++)
				QueueAdd(outbuf, i);
			strncpy(sweep_chan_name, channel, sizeof(sweep_chan_name));
			sweep_chan_name[sizeof(sweep_chan_name) - 1] = 0;
			gstate |= (GFS_SWEEPREQ | GFS_SWEEPING);
			return NULL;
		} else {
			return "Specify channel to sweep.";
		}
	}
}


// make sure this isn't called more than once per sweep!
void SweepGatherUsers(char *text, int index) {
	int textlen;
	char *tmp, *buf;

	if (gstate & GFS_SWEEPREQ) {
		if (!strncmp(text, "Users in channel ", 17)) {
			textlen = strlen(text + 17);
			text[textlen + 16] = 0;
			if (!strcasecmp(text + 17, sweep_chan_name)) {
				if (!sweep_chan)
					sweep_chan = VectorInit(32);
				bot[index]->fstate |= BFS_GETTINGSWEEP;
				gstate &= ~GFS_SWEEPREQ;
				SetAsyncTimer(index | TIMERID_REQWHODONE, 700, SweepEndTimerProc);
			}
		}
	} else {
		if (bot[index]->fstate & BFS_GETTINGSWEEP) {
			tmp = strchr(text, ',');
			if (tmp) {
				*tmp = 0;
				tmp = DigestWhoInput(tmp + 2);
			}
			text = DigestWhoInput(text);
			
			buf = malloc(32 * sizeof(char));
			strncpy(buf, text, 32);
			buf[31] = 0;
			sweep_chan = VectorAdd(sweep_chan, buf);
			AddBanQueue(BTYPE_FBAN, buf, NULL);
			if (tmp) {
				buf = malloc(32 * sizeof(char));
				strncpy(buf, tmp, 32);
				buf[31] = 0;
				//storing in a vector in case it's unviewable later
				sweep_chan = VectorAdd(sweep_chan, buf);
				AddBanQueue(BTYPE_FBAN, buf, NULL);
			} else {
				bot[index]->fstate &= ~BFS_GETTINGSWEEP;
			}
		}
	}
}


int CALLBACK SweepEndTimerProc(unsigned int idEvent) {
	int index;

	index = idEvent & ~TIMERID_REQWHODONE;
	bot[index]->fstate &= ~BFS_GETTINGSWEEP;
	return 0;
}


char *DigestWhoInput(char *text) {
	int len;
	
	len = strlen(text);
	if ((text[0] == '[') && (text[len - 1] == ']')) {
		if (isstrupper(text + 1)) {
			text[len - 1] = 0;
			text++;
		}
	}
	return text;
}

