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
 * commands.c - 
 *    Internal/external command parsing and handling
 */


#include "main.h"
#include "fxns.h"
#include "chat.h"
#include "clan.h"
#include "queue.h"
#include "winal.h"
#include "config.h"
#include "connection.h"
#include "hashtable.h"
#include "radix.h"
#include "banning.h"
#include "blacklist.h"
#include "wildcard.h"
#include "access.h"
#include "phrase.h"
#include "pbuffer.h"
#include "packets.h"
#include "commands.h"

LPNODE cmdtree;

/*NODE nodes[] = {
	{0x9272a4c7, 29, &nodes[1], &nodes[64]},
	{0x46c911dd, 31, &nodes[2], &nodes[33]},
	{0x1839b1dd, 21, &nodes[3], &nodes[18]},
	{0x0ee2598e, 76, &nodes[4], &nodes[11]},
	{0x09dd0f33, 82, &nodes[5], &nodes[8]},
	{0x00db819b, 8, &nodes[6], &nodes[7]},
	{0x00d819ef, 42, 0, 0},
	{0x0110ee87, 96, 0, 0},
	{0x0d459d4c, 95, &nodes[9], &nodes[10]},
	{0x0b42a5e2, 61, 0, 0},
	{0x0dc2e237, 6, 0, 0},
	{0x115bc6d0, 32, &nodes[12], &nodes[15]},
	{0x10f38c66, 78, &nodes[13], &nodes[14]},
	{0x0f678e23, 52, 0, 0},
	{0x1109b569, 4, 0, 0},
	{0x11cc33f1, 47, &nodes[16], &nodes[17]},
	{0x117a36c3, 93, 0, 0},
	{0x1677e883, 58, 0, 0},
	{0x30b7a4cb, 27, &nodes[19], &nodes[26]},
	{0x231a5e9f, 38, &nodes[20], &nodes[23]},
	{0x1e9f3fdf, 79, &nodes[21], &nodes[22]},
	{0x1d106c54, 24, 0, 0},
	{0x2245b5ae, 34, 0, 0},
	{0x270e5e75, 63, &nodes[24], &nodes[25]},
	{0x258a7403, 51, 0, 0},
	{0x2fd484a9, 72, 0, 0},
	{0x38998580, 11, &nodes[27], &nodes[30]},
	{0x327e2af1, 37, &nodes[28], &nodes[29]},
	{0x3101671c, 69, 0, 0},
	{0x3734ee49, 10, 0, 0},
	{0x401777db, 50, &nodes[31], &nodes[32]},
	{0x3f3e7468, 67, 0, 0},
	{0x448da399, 54, 0, 0},
	{0x6afb55d5, 0, &nodes[34], &nodes[49]},
	{0x4ff5caf0, 5, &nodes[35], &nodes[42]},
	{0x4a1ca159, 33, &nodes[36], &nodes[39]},
	{0x489e8d97, 30, &nodes[37], &nodes[38]},
	{0x47c8d769, 35, 0, 0},
	{0x48a6141a, 26, 0, 0},
	{0x4bc594cb, 81, &nodes[40], &nodes[41]},
	{0x4b7a9df9, 94, 0, 0},
	{0x4d8e62f8, 97, 0, 0},
	{0x629712b0, 91, &nodes[43], &nodes[46]},
	{0x5dafdc72, 59, &nodes[44], &nodes[45]},
	{0x5da46b68, 86, 0, 0},
	{0x624c111b, 40, 0, 0},
	{0x68c27e33, 2, &nodes[47], &nodes[48]},
	{0x66c4c92e, 53, 0, 0},
	{0x6a5ba628, 25, 0, 0},
	{0x796a071d, 77, &nodes[50], &nodes[57]},
	{0x728e4b44, 36, &nodes[51], &nodes[54]},
	{0x6e1f5b96, 45, &nodes[52], &nodes[53]},
	{0x6d7c5516, 70, 0, 0},
	{0x71c21326, 68, 0, 0},
	{0x77934cee, 39, &nodes[55], &nodes[56]},
	{0x76cbfeff, 23, 0, 0},
	{0x78a796af, 19, 0, 0},
	{0x83f7a21b, 87, &nodes[58], &nodes[61]},
	{0x7c31755a, 80, &nodes[59], &nodes[60]},
	{0x79bd7136, 73, 0, 0},
	{0x8034004a, 13, 0, 0},
	{0x85db0731, 44, &nodes[62], &nodes[63]},
	{0x84643284, 14, 0, 0},
	{0x8e5eaaec, 74, 0, 0},
	{0xe9fc9262, 20, &nodes[65], &nodes[96]},
	{0xb7fd80c0, 17, &nodes[66], &nodes[81]},
	{0x9da9fec5, 43, &nodes[67], &nodes[74]},
	{0x96b33f77, 66, &nodes[68], &nodes[71]},
	{0x96755c25, 22, &nodes[69], &nodes[70]},
	{0x95f7321d, 49, 0, 0},
	{0x967d848d, 98, 0, 0},
	{0x9ad1435e, 55, &nodes[72], &nodes[73]},
	{0x9812d033, 16, 0, 0},
	{0x9d9e402f, 89, 0, 0},
	{0xabe03e70, 7, &nodes[75], &nodes[78]},
	{0x9f2e12ac, 48, &nodes[76], &nodes[77]},
	{0x9df5a26e, 18, 0, 0},
	{0xaa7a5a4f, 83, 0, 0},
	{0xb6a7a93e, 99, &nodes[79], &nodes[80]},
	{0xaede7a26, 75, 0, 0},
	{0xb6fad441, 57, 0, 0},
	{0xc8f1d37d, 90, &nodes[82], &nodes[89]},
	{0xbf618956, 84, &nodes[83], &nodes[86]},
	{0xba724922, 28, &nodes[84], &nodes[85]},
	{0xb86bb057, 62, 0, 0},
	{0xbb04ac63, 92, 0, 0},
	{0xc2182016, 88, &nodes[87], &nodes[88]},
	{0xbfb813f3, 71, 0, 0},
	{0xc4dd8659, 12, 0, 0},
	{0xd25d59b4, 1, &nodes[90], &nodes[93]},
	{0xcb3626f3, 9, &nodes[91], &nodes[92]},
	{0xca2e9442, 15, 0, 0},
	{0xccd130cc, 46, 0, 0},
	{0xe08d39db, 64, &nodes[94], &nodes[95]},
	{0xd6f5c294, 85, 0, 0},
	{0xe952a4ec, 3, 0, 0},
	{0xfc9100d1, 41, &nodes[97], 0},
	{0xf2336334, 65, &nodes[98], &nodes[99]},
	{0xee2db946, 60, 0, 0},
	{0xf71d9e82, 56, 0, 0}
};*/

CMDDESC cmddesc[] = {
	{"v",			5   },
	{"ver",			5   },
	{"version",		5   },
	{"say",			10  },
	{"run",			102 },
	{"loud",		102 },
	{"runquiet",	102	},
	{"quiet",		102 },
	{"b",			60  },
	{"ban",			60  },
	{"uu",			55	},
	{"unban",		55  },
	{"u",			55  },
	{"k",			50	},
	{"kick",		50	},
	{"a",			5	},
	{"access",		5	},
	{"whois",		5	},
	{"whoami",		5	},
	{"cmdaccess",	5	},
	{"help",		5   },
	{"cmds",		103 },
	{"add",			70	},
	{"rem",			70	},
	{"users",		20	},
	{"bl",			70	},
	{"blacklist",	70	},
	{"shitadd",     70  },
	{"sa",			70  },
	{"tagadd",      70  },
	{"ta",          70  },
	{"shitdel",     70  },
	{"sd",          70  },
	{"tagdel",      70  },
	{"td",          70  },
	{"ipban",		70  },
	{"ip",			70  },
	{"unipban",		70  },
	{"unip",		70  },
	{"sweep",		70  },
	{"sb",			70  },
	{"ipsweep",		70  },
	{"idlesweep",   70  },
	{"p",			10  },
	{"ping",		10	},
	{"bancount",	20	},
	{"banned",		45	},
	{"j",		    95	},
	{"join",		95	},
	{"fjoin",		95	},
	{"forcejoin",	95	},
	{"fj",			95	},
	{"home",		95	},
	{"close",		101	},
	{"exit",		101	},
	{"quit",		101	},
	{"leave",		95	},
	{"daemon",		101	},
	{"hide",		101	},
	{"show",		101	},
	{"connect",		95	},
	{"disconnect",	95	},
	{"rc",			95	},
	{"reconnect",	95	},
	{"uptime",		10	},
	{"cq",			40	},
	{"scq",			40	},
	{"cbq",			70  },
	{"scbq",		70  },
	{"lw",			30	},
	{"lastwhisper",	30	},
	{"date",		5	},
	{"time",		5	},
	{"motd",		25	},
	{"setmotd",		85	},
	{"clan",		50	},
	{"crank",		95	},
	{"invite",		95	},
	{"tinvites",	100	},
	{"accept",		100	},
	{"decline",	    100	},
	{"dp",			100	},
	{"ddp",			100	},
	{"cp",			100	},
	{"autocp",		90	},
	{"greet",		90	},
	{"idle",		90	},
	{"halt",		95	},
	{"settrigger",	100	},
	{"ignpub",		100 },
	{"clientban",	90	},
	{"phrase",		80  },
	{"pb",			80	},
	{"phraseban",	80  },
	{"phraseadd",   80  },
	{"phrasedel",   80  },
	{"pingban",		90	},
	{"plugban",     90  },
	{"indexban",    90  },
	{"banevasion",  90  },
	{"numbersban",  90  },
	{"altcapsban",  90  },
	{"banning",		90  },
	{"loadban",		95	},
	{"winban",		90	},
	{"autoload",	90	},
	{"designate",	100	},
	{"op",			100	},
	{"lockdown",	100	},
	{"mass",		100	},
	{"rj",			100	},
	{"profiles",	90	},
	{"setname",		100	},
	{"setpass",		100	},
	{"reload",		95	},
	{"cremove",		100	},
	{"chieftain",	102	},
	{"mi",			40	},
	{"memberinfo",	40	},
	{"create",		100	},
	{"checkclan",	90	},
	{"lock",		101	},
	{"logging",		101	},
	{"disband",		103	},
	{"erase",		102 },
	{"plgload",		102 },
	{"plgstat",		90  },
	{"plgunload",	102 },
	{"update",		102 }
};


///////////////////////////////////////////////////////////////////////////////


void CheckCommand(char *user, char *text, int external, int index) {
	char asdf[128];
	PLUSER pluser;
	if (external) {
		if (*text == bot[index]->trigger || *(int16_t *)text == '++') {
			//lcase(user);
			pluser = HtGetItem(user, lusers, TL_LUSERS);
			if (pluser) {
				if (*text != bot[index]->trigger)
					text += 2;
				else
					text++;
				HandleCmdReq(pluser, text, index);
			}
		} else if (*text == '?') {
			if (*(int32_t *)(text) == 'irt?' && *(int32_t *)(text + 4) == 'regg') {
			    //lcase(user);
				pluser = HtGetItem(user, lusers, TL_LUSERS);
			   	if (pluser) {
					if (((gstate & GFS_LOCKED) && (pluser->access <= 100))
						|| (pluser->access < 5))
						return;
					sprintf(asdf, "The bot's current trigger is [ %c ].", bot[index]->trigger);
					QueueAdd(asdf, index);
				}
			}
		}
	} else {
		HandleCmdReq(botpluser, text + 2, 0);
	}
}


//"Ban timing retarded by %dms"
//"Ignoring public chatter"

void HandleCmdReq(PLUSER pluser, char *text, int index) {
	char asdf[256], sdfg[256], timestamp[16], tmpuser[32];
	char *tosend, *bigmem;
	int cmdi, i;
	time_t rawtime;
	void *timeinfo;
	void (*InviteResponse)(int, int);
	LPCHUSER pchuser;
	PLUSER tmppluser;
	char *temp, *arg2, chr;

	if ((gstate & GFS_LOCKED) && (pluser->access <= 100))
		return;

	temp = strchr(text, ' ');
	if (temp) {
		*temp++ = 0;
		if ((pluser->access >= 100) && isnumstr(text)) {
			QueueAdd(temp, atoi(text));
			return;
		}
	}

	lcase(text); //// justified
	cmdi = CmdGetIndex(hash((unsigned char *)text));
	if (cmdi == -1 || strcmp(text, cmddesc[cmdi].string)) {
		if (pluser->access == 103)
			printf("\'%s\' invalid command!\n", text);
		return;
	}
    if (pluser->access < cmddesc[cmdi].access)
        return;

	tosend = NULL;
	bigmem = NULL;
	switch (cmdi) {
		case CMD_V:
		case CMD_VER:
		case CMD_VERSION:
            //*(int32_t *)0 = 0; ///for crash handle testing
			GetOS(sdfg);
			sprintf(asdf, pluser->access == 103 ?
				"Phyros " BOT_VER "  [ %s ]" :
				"/me : : : Phyros " BOT_VER "  [ %s ] : : : ", sdfg);
			tosend = asdf;
			break;
		case CMD_SAY:
			if (pluser->access < 100)
				*(--temp) = ' ';
			tosend = temp;
			break;
		case CMD_RUN:
		case CMD_LOUD:
            if (temp)
                _CreateThread(RunLoudProc, temp);
			break;
		case CMD_RUNQUIET:
		case CMD_QUIET:
            if (temp)
                _CreateThread(RunQuietProc, temp);
			break;
		case CMD_B:
		case CMD_BAN:
			HandleBUKUserCmd(pluser, temp, BTYPE_FBAN, index);
			break;
		case CMD_UU:
			sprintf(asdf, "/w %s You have been unbanned from %s.",
				temp, curchan);
			tosend = asdf;
		case CMD_UNBAN:
		case CMD_U:
			HandleBUKUserCmd(pluser, temp, BTYPE_UNBAN, index);
			break;
		case CMD_K:
		case CMD_KICK:
			HandleBUKUserCmd(pluser, temp, BTYPE_KICK, index);
			break;
		case CMD_A:
		case CMD_ACCESS:
		case CMD_WHOIS:
		case CMD_WHOAMI:
			if ((cmdi != CMD_WHOIS) && (!temp)) {
				tmppluser = pluser;
			} else {
				if (!temp) {
					tosend = asdf;
					break;
				}
				lcase(temp); /////justified
				RealmFix(temp, tmpuser, 0); //////ermm, is this a regression?
				tmppluser = HtGetItem(tmpuser, lusers, TL_LUSERS);
			}
			if (tmppluser)
				sprintf(asdf, "%s :: %d access", tmppluser->username, tmppluser->access);
			else
				sprintf(asdf, "%s not found in access database.", tmpuser);
			tosend = asdf;
			break;
		case CMD_CMDACCESS:
		case CMD_HELP:
			if (temp) {
				lcase(temp); ////justified
				i = CmdGetIndex(hash((unsigned char *)temp));
				if (i == -1 || strcmp(temp, cmddesc[i].string)) {
					sprintf(asdf, "Command \'%s\' does not exist.", temp);
					tosend = asdf;
				} else {
					if (cmdi == CMD_CMDACCESS) {
						sprintf(asdf, "Command \'%s\' requires %d access.",
							temp, cmddesc[i].access);
						tosend = asdf;
					} else {
						tosend = HelpLookupCmdStr(i);
					}
				}
				tosend = asdf;
			}
			break;
		case CMD_CMDS:
			for (i = 0; i != ARRAYLEN(cmddesc); i++) {
				if (!temp || WildcardMatch(temp, cmddesc[i].string)) {
					printf("%s - access required: %d - %s",
						cmddesc[i].string, cmddesc[i].access, HelpLookupCmdUsage(i));
				}
			}
			break;
		case CMD_ADD:
			*asdf = 0;
			HandleAddUserCmd(pluser, temp, asdf);
			if (*asdf)
				tosend = asdf;
			break;
		case CMD_REM:
			*asdf = 0;
            HandleRemoveUserCmd(pluser, temp, asdf);
			if (*asdf)
				tosend = asdf;
			break;
		case CMD_USERS:
			bigmem = HandleUsersCmd();
			tosend = bigmem;
			break;
		case CMD_BL:
		case CMD_BLACKLIST:
			tosend = HandleBlacklistCmd(pluser, temp, asdf);
			if (!tosend)
				tosend = asdf;
			else
				bigmem = tosend;
			break;
		case CMD_SHITADD:
		case CMD_SA:
			chr = 'S';
			goto over_ta;
		case CMD_TAGADD:
		case CMD_TA:
			chr = 'T';
		over_ta:
			arg2 = strchr(temp, ' ');
			if (arg2)
				*arg2++ = 0;
			BlacklistAddToDB(text, chr, pluser->username, arg2, asdf);
			tosend = asdf;
			break;
		case CMD_SHITDEL:
		case CMD_SD:
			chr = 'S';
			goto over_td;
		case CMD_TAGDEL:
		case CMD_TD:
			chr = 'T';
		over_td:
			BlacklistRemoveFromDB(text, chr, asdf);
			tosend = asdf;
			break;
		case CMD_IPBAN:
		case CMD_IP:
			HandleBUKUserCmd(pluser, temp, BTYPE_IPBAN, index);
			break;
		case CMD_UNIPBAN:
		case CMD_UNIP:
			HandleBUKUserCmd(pluser, temp, BTYPE_UNIP, index);
			break;
		case CMD_SWEEP: // also add a wakechannel command to send everybody's name
		case CMD_SB:    // sweep IDLE command (compare everybody in channel who didn't talk and kick)
			tosend = SweepStart(temp, asdf);
			break;
		case CMD_IPSWEEP:
			break;
		case CMD_IDLESWEEP:
			break;
		case CMD_P:
		case CMD_PING:
			if (temp) {
				if (*(int16_t *)temp == 's/') {
					Send0x15(index);
					break;
				} else {
					lcase(temp); //// justified
					RealmFix(temp, tmpuser, 0);
					pchuser = HtGetItem(tmpuser, users, TL_BNUSERS);
				}
			} else {
				pchuser = HtGetItem((pluser->access == 103) ?
					bot[curbotinc()]->realname : pluser->username, users, TL_BNUSERS);
			}
			if (pchuser) {
				sprintf(asdf, "%s's ping is %ums.", pchuser->username, pchuser->ping);
				tosend = asdf;
			} else {
				tosend = "Specified user cannot be found.";
			}
			break;
		case CMD_BANCOUNT:
			sprintf(asdf, "%u users have been banned.", bancount);
			tosend = asdf;
			break;
		case CMD_BANNED:
			bigmem = HandleBannedCmd();
			tosend = bigmem;
			break;
		case CMD_J:
		case CMD_JOIN:
			sprintf(asdf, "/j %s", temp);
			tosend = asdf;
			break;
		case CMD_FJOIN:
		case CMD_FORCEJOIN:
		case CMD_FJ:
			InsertDWORD(0x02);
			InsertNTString(temp);
			SendPacket(0x0C, index);
			break;
		case CMD_HOME:
			sprintf(asdf, "/j %s", home);
			tosend = asdf;
			break;
		case CMD_CLOSE:
		case CMD_EXIT:
		case CMD_QUIT:
			exit(1);
			break;
		case CMD_LEAVE:
			SendPacket(0x10, index);
			break;
		case CMD_DAEMON:
		case CMD_HIDE:
			#ifdef _WIN32
				ShowWindow(GetConsoleWindow(), SW_HIDE);
			#else

			#endif
			break;
		case CMD_SHOW:
			#ifdef _WIN32
				ShowWindow(GetConsoleWindow(), SW_SHOW);
			#else

			#endif
			break;
		case CMD_CONNECT:
			if (temp) {
				i = atoi(temp);
				if (i < 0 || i >= numbots)
					break;
				ConnectBot(i);
			} else {
				for (i = 0; i != numbots; i++) {
					if (!bot[i]->connected) {
						ConnectBot(i);
						break;
					}
				}
			}
			break;
		case CMD_DISCONNECT:
			if (temp) {
				i = atoi(temp);
				if (i < 0 || i >= numbots)
					break;
				DisconnectBot(i, DISCN_GRACEFUL);
			}
			break;
		case CMD_RC:
		case CMD_RECONNECT:
			if (temp) {
				i = atoi(temp);
				if (i < 0 || i >= numbots)
					break;
				DisconnectBot(i, DISCN_GRACEFUL);
				ConnectBot(i);
			}
			break;
		case CMD_UPTIME:
			tosend = HandleUptimeCmd(asdf, temp, index);
			break;
		case CMD_CQ:
			cmdi = 0;
			for (i = 0; i != numbots; i++)
				cmdi += bot[i]->queue.count;
			sprintf(asdf, "Cleared %d queued items.", cmdi);
			tosend = asdf;
		case CMD_SCQ:
			QueueClearAll();
			break;
		case CMD_CBQ:
		case CMD_SCBQ:

			break;
		case CMD_LW:
		case CMD_LASTWHISPER:
			if (lastwhisper.profile != -1) {
				sprintf(asdf, "Last whisper to profile %d was sent by %s: %s",
					lastwhisper.profile, lastwhisper.user, lastwhisper.message);
				tosend = asdf;
			} else {
				tosend = "No whispers received!";
			}
			break;
		case CMD_DATE:
		case CMD_TIME:
			time(&rawtime);
			timeinfo = (void *)localtime(&rawtime);
			strftime(asdf, sizeof(asdf),
				(cmdi == CMD_DATE) ? "%m/%d/%Y" : "%I:%M:%S %p",
				(void *)timeinfo);
			tosend = asdf;
			break;
		case CMD_MOTD:
			if (pluser->access != 103)
				bot[index]->fstate |= BFS_EXTERNALREQ;
			InsertDWORD(0x3713);
			SendPacket(0x7C, index);
			break;
		case CMD_SETMOTD:
			if (temp) {
				sprintf(asdf, "/c motd %s", temp);
				QueueAdd(asdf, index);
			}
			break;
		case CMD_CLAN:
			if (pluser->access != 103)
				bot[index]->fstate |= BFS_EXTERNALREQ;
			Send0x7D(index);
			break;
		case CMD_CRANK:
			if (pluser->access != 103)
				bot[index]->fstate |= BFS_EXTERNALREQ;
			if (temp) {
				arg2 = strchr(temp, ' ');
				if (arg2) {
					*arg2++ = 0;
					Send0x7A(temp, (unsigned char)atoi(arg2), index);
				}
			}
			break;
		case CMD_INVITE:
			if (temp) {
				if (pluser->access != 103)
					bot[index]->fstate |= BFS_EXTERNALREQ;
				Send0x77(temp, index);
			}
			break;
		case CMD_TINVITES:
			ToggleGlobalFlag(temp, GFS_ACCEPTINVS, asdf, 
				"%sing invintation requests.", "Accept", "Ignor");
			tosend = asdf;
			break;
		case CMD_ACCEPT:
		case CMD_DECLINE:
			if (!bot[index]->invited.tag) {
				tosend = "No pending invitations.";
			} else {
				InviteResponse = (bot[index]->fstate & BFS_CREATEINV) ? Send0x72 : Send0x79;
				(*InviteResponse)((cmdi == CMD_ACCEPT) ? 'y' : 'n', temp ? atoi(temp) : index);
				sprintf(asdf, "%sed clan invitation.", (cmdi == CMD_ACCEPT) ? "Accept" : "Declin");
				tosend = asdf;
			}
			break;
		case CMD_DP:
			break;
		case CMD_DDP:
			break;
		case CMD_CP:
			break;
		case CMD_AUTOCP:
			break;
		case CMD_GREET:
			break;
		case CMD_IDLE:
			break;
		case CMD_HALT:
			if (temp) {
				//lcase(temp);
				tmppluser = HtGetItem(temp, lusers, TL_LUSERS);
				if (tmppluser) {
					if (tmppluser->access < pluser->access) {
						if (tmppluser->flags & LUR_HALTED) {
							tmppluser->access = tmppluser->oacces;
							tmppluser->flags &= ~LUR_HALTED;
						} else {
							tmppluser->oacces = tmppluser->access;
							tmppluser->access = 1;
							tmppluser->flags |= LUR_HALTED;
							
						}
						sprintf(asdf, "User \'%s\' has been %shalted.", temp, 
							(tmppluser->flags & LUR_HALTED) ? "" : "un");
						tosend = asdf;
					}
				}
			}
			break;
		case CMD_SETTRIGGER:
			if (temp) {
				sprintf(asdf, "Trigger has been changed from \'%c\' to \'%c\'.",
					bot[index]->trigger, *temp);
				bot[index]->trigger = *temp;
				tosend = asdf;
			}
			break;
		case CMD_IGNPUB:
			if (temp) {					   
				temp = (((*(int16_t *)temp) | 0x2020) == 'on') ? "/o igpub" : "/o unigpub";
				for (i = 1; i != numbots; i++) {
					if (bot[i]->connected)
						QueueAdd(temp, i);
				} ///////////////make it per profile, add ignpriv
			}
			break;
		case CMD_CLIENTBAN:
			//SetFlagAndValue(temp, GFS_CLIENTBAN, &clientban, asdf);
			break;
		case CMD_PHRASE:
		case CMD_PB: ////phraseban alias
		case CMD_PHRASEBAN:
			HandlePhraseCmd(pluser, temp, asdf);
			break;
		case CMD_PHRASEADD:
			break;
		case CMD_PHRASEDEL:
			break;
		case CMD_PINGBAN:
			break;
		case CMD_PLUGBAN:
			break;
		case CMD_INDEXBAN:
			break;
		case CMD_BANEVASION:
			break;
		case CMD_NUMBERSBAN:
			break;
		case CMD_ALTCAPSBAN:
			break;
		case CMD_BANNING:
			break;
		case CMD_LOADBAN:
			break;
		case CMD_WINBAN:
			SetFlagAndValue(temp, GFS_WINBAN, &ban_winlow, asdf,
				"Winban set to %d.", "Winban disabled.");
			tosend = asdf;
			break;
		case CMD_AUTOLOAD:
			break;
		case CMD_DESIGNATE:
		case CMD_OP:
			if (temp) {
				RealmFix(temp, tmpuser, 0);
				sprintf(asdf, "/designate %s", tmpuser);
			} else {
				sprintf(asdf, "/designate %s", pluser->username);
			}  
			QueueAdd(asdf, index);
			if (cmdi == CMD_OP)
				RejoinChannel(index);
			break;
		case CMD_LOCKDOWN:
			break;
		case CMD_MASS:
			sprintf(asdf, "/j %s", temp);
			for (i = 0; i != numbots; i++)
				QueueAdd(asdf, i);
			break;
		case CMD_RJ:
			RejoinChannel(index);
			break;
		case CMD_PROFILES:
			bigmem = HandleProfilesCmd();
			tosend = bigmem;
			break;				   
		case CMD_SETNAME:
			if (temp) {
				arg2 = strchr(temp, ' ');
				i = arg2 ? atoi(arg2 + 1) : 0;
				strncpy(bot[i]->username, temp, sizeof(bot[i]->username));
				bot[i]->username[sizeof(bot[i]->username) - 1] = 0;
				sprintf(asdf, "Set new username for profile %d to %s.",
					i, bot[i]->username);
				tosend = asdf; ///////////////////////////////////
			}
			break;
		case CMD_SETPASS:
			if (temp) {
				arg2 = strchr(temp, ' ');
				i = arg2 ? atoi(arg2 + 1) : 0;
				strncpy(bot[i]->password, temp, sizeof(bot[i]->password));
				bot[i]->password[sizeof(bot[i]->password) - 1] = 0;
				sprintf(asdf, "Set new password for profile %d.", i);
				tosend = asdf; /////////////////set permanently in config!
			}
			break;
		case CMD_RELOAD:
			tosend = HandleReloadCmd(temp);
			break;
		case CMD_CREMOVE:
			if (pluser->access != 103)
				bot[index]->fstate |= BFS_EXTERNALREQ;
			Send0x78(temp, index);
			break;
		case CMD_CHIEFTAIN:
			if (temp) {
				if (pluser->access != 103)
					bot[index]->fstate |= BFS_EXTERNALREQ;
				Send0x74(temp, index);
			}
			break;
		case CMD_MI:
		case CMD_MEMBERINFO:
			if (temp) {	//weird, i never get a response...
				//if (pluser->access != 103)
				//	bot[index]->fstate |= BFS_EXTERNALREQ;
				Send0x82(temp, index);
			}
			break;
		case CMD_CREATE:
			tosend = HandleClanCreateCommand(temp, index);
			break;
		case CMD_CHECKCLAN:
			if (temp) ////////////////////what about external requests?
				Send0x70(temp, index);
			break;
		case CMD_LOCK:
			ToggleGlobalFlag(temp, GFS_LOCKED, asdf,
				"Access has been %slocked.", "", "un");
			tosend = asdf;
			break;
		case CMD_LOGGING:
			ToggleGlobalFlag(temp, GFS_LOGGING, asdf,
				"Logging %sabled.", "en", "dis");
			tosend = asdf;
			break;
		case CMD_DISBAND:
			InsertDWORD(0x3713);
			SendPacket(0x73, index);
			break;
		case CMD_ERASE:
			tosend = HandleEraseCmd(temp);
			break;
		case CMD_PLGLOAD:
			break;
		case CMD_PLGSTAT:
			break;
		case CMD_PLGUNLOAD:
			break;
		case CMD_UPDATE:
			break;
		default:
			printf("Invalid command index %d (unimplemented)!\n", cmdi);
	}
	if (tosend) {
		if (pluser->access == 103) {
			GetTimeStamp(timestamp);
			printf("%s %s\n", timestamp, tosend);
		} else {
			QueueAdd(tosend, index);
		}
	}
	if (bigmem)
		free(bigmem);
}


char *HandleUptimeCmd(char *buf, char *arg, int index) {
	char *tmp = buf;

	if (!arg)
		arg = "s";

	switch (*arg) {
		case 's':
		case 'S':
		topcase:
			strcpy(tmp, "System uptime: ");
			tmp += 15;
			tmp += GetUptimeString(getuptime(), tmp);
			if (arg) {
				break;
			} else {
				strcpy(tmp, "  |  ");
				tmp += 5;
			}
		case 'l':
		case 'L':
			strcpy(tmp, "Loaded uptime: ");
			tmp += 15;
			tmp += GetUptimeString(getuptime() - loadedtime, tmp);
			if (arg) {
				break;
			} else {
				strcpy(tmp, "  |  ");
				tmp += 5;
			}
		case 'c':
		case 'C':
			strcpy(tmp, "Connection uptime: ");
			tmp += 19;
			tmp += GetUptimeString(getuptime() - bot[index]->connectedtime, tmp);
			break;
		default:
			arg = NULL;
			goto topcase;
	}
	return buf;
}


char *HandleProfilesCmd() {
	int i;
	char *temp, *buf;
	buf  = malloc(numbots * 36);
	temp = buf;
	temp += sprintf(temp, "Profiles: %d | ", numbots);
	for (i = 0; i != numbots; i++) {
		temp += sprintf(temp, "%d: %s%s, ", i, bot[i]->username,
			bot[i]->connected ? " (C)" : "");
	}
	temp[-2] = 0;
	return buf;
}


void ToggleGlobalFlag(char *arg, int flag, char *buf, const char *fmt, 
					  const char *yes, const char *no) {
	if (arg) {
		if (((*(int16_t *)arg) | 0x2020) == 'no')
			gstate |= flag;
		else
			gstate &= ~flag;
	} else {
		gstate ^= flag;
	}
	sprintf(buf, fmt, (gstate & flag) ? yes : no);
}


void SetFlagAndValue(char *arg, int flag, int *val, char *buf,
					 const char *yesfmt, const char *no) {
	if (arg) {
		*val = atoi(arg);
		gstate |= flag;
		sprintf(buf, yesfmt, *val);
	} else {
		gstate &= ~flag;
		strcpy(buf, no);
	}
}


void CmdTreeInit() {
	int i;

	for (i = 0; i != ARRAYLEN(cmddesc); i++)
		CmdTreeInsert(cmddesc[i].string, i);

	DSWBalanceTree(cmdtree);
}


void CmdTreeInsert(const char *key, int data) {
	LPNODE *tmp    = &cmdtree;
	LPNODE lpnode  = malloc(sizeof(NODE));
	lpnode->key    = hash((unsigned char *)key);
	lpnode->data   = data;
	lpnode->lchild = NULL;
	lpnode->rchild = NULL;

	while (*tmp)
		tmp = (lpnode->key > (*tmp)->key) ? &((*tmp)->rchild) : &((*tmp)->lchild);
	*tmp = lpnode;
}


int __fastcall CmdGetIndex(unsigned int cmd) {
	LPNODE *tmp = &cmdtree;
	while (*tmp) {
		if (cmd > (*tmp)->key)
			tmp = &((*tmp)->rchild);
		else if (cmd < (*tmp)->key)
			tmp = &((*tmp)->lchild);
		else
			return (*tmp)->data;
	}
	return -1;
}


void DSWBalanceTree(LPNODE lpnode) {
	LPNODE p;
	int nc, i, i2;

	nc = 0;
	p  = lpnode;
	
	//tree to vine
	while (p) {	
		while (RotateNodeRight(p) == 1);
		p = p->rchild;
		nc++;
	}

	//vine to tree
	for (i = nc >> 1; i; i >>= 1) {
		p = lpnode;	
		for (i2 = 0; i2 < i; i2++) {		
			RotateNodeLeft(p);
			p = p->rchild;
		}
	}
}


int RotateNodeLeft(LPNODE lpnode) {
	LPNODE p;
	int data;
	unsigned int key;

	if (!lpnode || !lpnode->rchild)
		return 0;

	p = lpnode->rchild;
	lpnode->rchild = p->rchild;

	p->rchild = p->lchild;
	p->lchild = lpnode->lchild;
	lpnode->lchild = p;

	key  = lpnode->key;
	data = lpnode->data;
	lpnode->key  = p->key;
	lpnode->data = p->data;
	p->key  = key;
	p->data = data;
	return 1;
}


int RotateNodeRight(LPNODE lpnode) {
	LPNODE p;
	int data;
	unsigned int key;

	if (!lpnode || !lpnode->lchild)
		return 0;

	p = lpnode->lchild;
	lpnode->lchild = p->lchild;

	p->lchild = p->rchild;
	p->rchild = lpnode->rchild;
	lpnode->rchild = p;

	key  = lpnode->key;
	data = lpnode->data;
	lpnode->key  = p->key;
	lpnode->data = p->data;
	p->key  = key;
	p->data = data;
	return 1;
}


char *HelpLookupCmdStr(int cmdi) {
	return NULL; ///////////////////////////////////////////////
}


char *HelpLookupCmdUsage(int cmdi) {
	return NULL; ///////////////////////////////////////////////
}

