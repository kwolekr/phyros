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
 * config.c - 
 *    Configuration loading routines
 */

#include "main.h"
#include "fxns.h"
#include "chat.h"
#include "winal.h"
#include "access.h"
#include "update.h"
#include "queue.h"
#include "radix.h"
#include "banning.h"
#include "cdkeymgmt.h"
#include "crypto/cdkey.h"
#include "config.h"

/* Sample config file:

	cfg main {
		server=
		bnlsserver=
		home=

		ipban=

		owner=
        masters=

		usebnls=
		forceproxy=

		war3verbyte=
		war3.exe=
		storm.dll=
		game.dll=
	}

	cfg profile 1 {
		user=
		pass=
		cdkey=
		trigger=
		useproxy=
		autoconnect=
	}
*/


const char *maincfgstrs[] = {
	"bnet_server",
	"bnet_port",
	"bnls_server",
	"bnls_port",
	"home",
	"owner",
	"masters",
	"usebnls",
	"forceproxy",
	"ipban",
	"war3_verbyte",
	"war3.exe",
	"storm.dll",
	"game.dll",
	"update_site",
	"update_file",
	"checkupdates",
	"processpriority",
	"flood_tick",
	"flood_numticks",
	"flood_over",
	"kickbancount",
	"baninterval",
	"loadbandelay",
	"queue_ms_perpkt",
	"queue_ms_perbyte",
	"queue_bytes_overhead"
};

const char *profcfgstrs[] = {
	"user",
	"pass",
	"cdkey",
	"trigger",
	"useproxy",
	"autoconnect"
};

CFGSTRTABLE cfgstrtables[] = {
	{maincfgstrs, ARRAYLEN(maincfgstrs), CfgHandleGlobalEntry},
	{profcfgstrs, ARRAYLEN(profcfgstrs), CfgHandleProfileEntry}
};


///////////////////////////////////////////////////////////////////////////////


void SetDefaultConfig() {
	#ifdef _WIN32
		strcpy(conf_path_config, "phyros.conf");
		strcpy(conf_path_access, "access.conf");
		strcpy(conf_path_phrase, "phrases.conf");
		strcpy(conf_path_bl,     "blacklist.conf");
		strcpy(conf_path_cmdacc, "cmdaccess.conf");
		strcpy(conf_path_cdkeys, "cdkeys.conf");
	#else
		char *homedir;

		homedir = getenv("HOME");
		sprintf(conf_path_config, "%s/.phyros/phyros.conf", homedir);
		sprintf(conf_path_access, "%s/.phyros/access.conf",  homedir);
		sprintf(conf_path_phrase, "%s/.phyros/phrases.conf", homedir);
		sprintf(conf_path_bl,     "%s/.phyros/blacklist.conf", homedir);
		sprintf(conf_path_cmdacc, "%s/.phyros/cmdaccess.conf", homedir);
		sprintf(conf_path_cdkeys, "%s/.phyros/cdkeys.conf", homedir);
	#endif

	strcpy(server,        "useast.battle.net");
	strcpy(bnlsserver,    "bnls.valhallalegends.com");
	strcpy(home,          "Default Channel");
	strcpy(updatesite,    "darkblizz.org");
	strcpy(updateverfile, "/phyros/version.php");

	verbyte		         = 0x1A;

	floodthresh_tick     = 700;
	floodthresh_numticks = 5;
	floodthresh_over     = 10000;

	ban_kickcount		 = 4;
	ban_timing_normal	 = 4500;
	ban_timing_loadban	 = 1000;

	queue_ms_perpacket   = 600;
	queue_ms_perbyte     = 30;
	queue_bytes_overhead = 65;
}


/****************************************************************************************************
 * Note: numbots is actually the highest profile's number loaded + 1, not the real number of bots   *
 * To actually iterate through the bots, all NUM_MAX_BOTS must be visited and checked if non-null.  *
 ****************************************************************************************************/
void LoadConfig() {
	FILE *file;
	char asdf[512];
	char *tmp, *buf, *equsgn;
	const char **strtbl;
	int state, curprof, i, i2, len;
	int cmdi, round, line, strtbllen;
	

	line  = 0;
	state = CFGLDR_STATE_NONE;
	file  = fopen(conf_path_config, "r");
	if (!file) {
		printf("ERROR: cannot open %s for reading, errno: %d\n",
			conf_path_config, geterr);
		exit(0);
		return;
	}
	while (!feof(file)) {
		line++;
		fgets(asdf, sizeof(asdf), file);
		
		buf = skipws(asdf);
		if (*buf == '#' || !*buf)
			continue;

		len = strlen(buf);
		if (len >= 1 && buf[len - 1] == '\n')
			buf[len - 1] = 0;
		if (len >= 2 && buf[len - 2] == '\r')
			buf[len - 2] = 0;

		if (*buf == '}')
			state = CFGLDR_STATE_NONE;

		if (*(uint32_t *)buf == ' gfc') {
			buf += 4;
			switch (*(uint32_t *)buf) {
				case 'niam':
					round = 0;
					state = CFGLDR_STATE_GLOBAL;
					continue;
				case 'forp':
					buf += 4;
					curprof = atoi(buf);
					if ((curprof < 0) || (curprof >= NUM_MAX_BOTS)) {
						printf("WARNING: out of range profile descriptor on line %d, ignoring\n",
							line);
						continue;
					}
					if (curprof > numbots)
						numbots = curprof + 1;
					bot[curprof] = malloc(sizeof(BOT));
					memset(bot[curprof], 0, sizeof(BOT));
					round = 0;
					state = CFGLDR_STATE_PROFILE;
					continue;
				default:
					printf("WARNING: unhandled config profile type \'%s\' on line %d, ignoring\n",
						buf, line);
					continue;
			}
		}
		if (state) {
			equsgn = strchr(buf, '=');
			if (!equsgn)
				continue;
			*equsgn++ = 0;
			tmp = findws(buf);
			if (tmp)
				*tmp = 0;

			strtbl    = cfgstrtables[state - 1].cfgstrs;
			strtbllen = cfgstrtables[state - 1].arraylen;
			i  = round;
			i2 = 0;
			while (i2 != strtbllen) {
				if (i == strtbllen)
					i = 0;
				if (!strcasecmp(buf, strtbl[i])) {
					cmdi = i;
					break;
				}
				i++;
				i2++;
			}
			if (i2 == strtbllen) {
				printf("WARNING: unrecognized config entry \'%s\', ignoring\n", buf);
				continue;
			}
			
			equsgn = skipws(equsgn);
			cfgstrtables[state - 1].handler(cmdi, equsgn, curprof);
			round++;
		}
	}
	printf(" - loaded %d profiles\n", numbots);
}


void CfgHandleGlobalEntry(int cmdi, char *text, int profile) {
	switch (cmdi) {
		case CFG_SERVER:
			strncpy(server, text, sizeof(server));
			break;
		case CFG_PORT:
			port = atoi(text);
			break;
		case CFG_BNLSSERVER:
			strncpy(bnlsserver, text, sizeof(bnlsserver));
			break;
		case CFG_BNLSPORT:
			bnlsport = atoi(text);
			break;
		case CFG_HOME:
			strncpy(home, text, sizeof(home));
			break;
		case CFG_OWNER:	
			AddOwner(text);
			break;
		case CFG_MASTERS:	
			AddMasters(text);
			break;
		case CFG_USEBNLS:	
			if (*text - '0')
				gstate |= GFS_USEBNLS;
			break;
		case CFG_FORCEPROXY:
			if (*text - '0')
				gstate |= GFS_FORCEPROXY;
			break;
		case CFG_IPBAN:
			if (*text - '0')
				gstate |= GFS_IPBAN;
			break;
		case CFG_WAR3VB:
			sscanf(text, "%X", &verbyte);
			break;
		case CFG_HASH1:
		case CFG_HASH2:		
		case CFG_HASH3:	
			strncpy(hashes[cmdi - CFG_HASH1], text, sizeof(hashes[cmdi - CFG_HASH1]));
			break;
		case CFG_UPDATESITE:
			strncpy(updatesite, text, sizeof(updatesite));
			break;
		case CFG_UPDATEFILE:
			strncpy(updateverfile, text, sizeof(updateverfile));
			break;
		case CFG_CHECKUPDATES: 
			if (*text - '0')
				gstate |= GFS_CHECKUPDATES;
			break;
		case CFG_PROCESSPRIORITY: 
			//There is no command for this since it could be set with
			//utilities such as renice and Windows Task Manager
			//This function could fail silently - check with ps ax
			if (*text)
				SetProcessPriority(atoi(text));
			break;
		case CFG_FLOODTICK:
			floodthresh_tick = atoi(text);
			break;
		case CFG_FLOODNUMTICKS:
			floodthresh_numticks = atoi(text);
			break;
		case CFG_FLOODOVER:
			floodthresh_over = atoi(text);
			break;
		case CFG_KICKBANCOUNT:
			ban_kickcount = atoi(text);
			break;
		case CFG_BANINTERVAL:
			ban_timing_normal = atoi(text);
			break;
		case CFG_LOADBANDELAY:
			ban_timing_loadban = atoi(text);
			break;
		case CFG_QUEUEPERPKT:
			queue_ms_perpacket = atoi(text);
			break;
		case CFG_QUEUEPERBYTE:
			queue_ms_perbyte = atoi(text);
			break;
		case CFG_QUEUEOVERHEAD:
			queue_bytes_overhead = atoi(text);
	}
}


void CfgHandleProfileEntry(int cmdi, char *text, int profile) {
	char *tmp;

	switch (cmdi) {
		case -1:									   
			break;
		case CFG_USER:
			strncpy(bot[profile]->username, text, sizeof(bot[profile]->username));
			bot[profile]->username[sizeof(bot[profile]->username) - 1] = 0;
			break;
		case CFG_PASS:
			strncpy(bot[profile]->password, text, sizeof(bot[profile]->password));
			tmp = &bot[profile]->password[sizeof(bot[profile]->password) - 1];
			if (*tmp) {
				printf("WARNING: password for profile %d is over %d chars, truncating\n",
					profile, sizeof(bot[profile]->password) - 1);
				*tmp = 0;
			}
			break;
		case CFG_CDKEY:
			AddCDKeyToList(text, profile);
			defcdkeys++;
			break;
		case CFG_TRIGGER:
			if (*text)
				bot[profile]->trigger = *text;
			break;
		case CFG_USEPROXY:
			if (*text - '0')
				bot[profile]->fstate |= BFS_USEPROXY;
			break;
		case CFG_AUTOCONNECT:
	 		if (*text - '0')
				bot[profile]->fstate |= BFS_AUTOCONN;
	}
}

///////////////////////////FINISH THIS!!!!!!!!!!!!!///////////////////////////////////////
int CfgWriteEntry(const char *cfgclass, int profile, const char *entryname, const char *value) {
	FILE *file;
	char asdf[256], *sdfg;
	int state;

	state = 0;
	file = fopen(conf_path_config, "w+");
	if (!file) {
		printf("WARNING: cannot open %s for writing, errno: %d",
			conf_path_config, geterr);
		return 0;
	}
	while (!feof(file)) {
		fgets(asdf, sizeof(asdf), file);

		sdfg = skipws(asdf);
		if (!state && (*(uint32_t *)sdfg == ' gfc')) {
			sdfg += 4;
			state = 1;
		}
	}
	return 1;
}


char *HandleReloadCmd(char *args) {
	return NULL;
}


char *HandleEraseCmd(char *args) {
	return NULL;
}

