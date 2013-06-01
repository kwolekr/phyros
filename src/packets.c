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
 * packets.c - 
 *    Routines for logon and misc. battle.net & bnls packets
 */


#include "main.h"
#include "chat.h"
#include "clan.h"
#include "fxns.h"
#include "crypto/checkrevision.h"
#include "crypto/cdkey.h"
#include "crypto/srp.h"
#include "queue.h"
#include "warden.h"
#include "packets.h"
#include "pbuffer.h"
#include "hashtable.h"
#include "cdkeymgmt.h"
#include "connection.h"

const char *authresponse[] = {
	"Old product version",
	"Invalid product version",
	"Invalid CDKey",
	"CDKey in use",
	"Banned CDKey",
	"Wrong Product CDKey",
	"Invalid Expansion Key",
	"Expansion Key in use",
	"Banned Expansion Key",
	"Unknown 0x51 error"
};

const char *createresponse[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	"Account already exists",
	NULL,
	NULL,
	"Account name too short",
	"Name contains illegal char",
	"Name contains illegal word",
	"Too few alphanum chars",
	"Adjacent punct chars",
	"Too many punct chars"
};

int pkhndindex[] = {
    1, 0, 0, 0, 0, 0, 0, 0,   0, 0, 2, 1, 0, 0, 0, 3,
    0, 0, 0, 29, 0, 30, 0, 0,   0, 31, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 4, 5, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    6, 7, 8, 9, 10, 0, 0, 0,   0, 0, 0, 0, 0, 0, 11, 0, 
    0, 0, 0, 0, 0, 0, 1, 1,   1, 1, 0, 0, 0, 0, 0, 0,	
    12, 13, 14, 15, 16, 17, 18, 19,   20, 21, 22, 0, 23, 24, 25, 26,

    0, 27, 28, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0
};

PacketHandler pkthandlers[] = {
    IgnorePacket,
    Parse0x0A,
    Parse0x0F,
    Parse0x25,
    Parse0x26,
    Parse0x50,
    Parse0x51,
    Parse0x52,
    Parse0x53,
    Parse0x54,
    Parse0x5E,
	Parse0x70,
	Parse0x71,
	Parse0x72,
	Parse0x73,
	Parse0x74,
	Parse0x75,
	Parse0x76,
	Parse0x77,
	Parse0x78,
	Parse0x79,
	Parse0x7A,
	Parse0x7C,
	Parse0x7D,
	Parse0x7E,
	Parse0x7F,
	Parse0x81,
	Parse0x82,
	Parse0x13,
	Parse0x15,
	Parse0x19
};


///////////////////////////////////////////////////////////////////////////////


void IgnorePacket(char *data, int index) {
    return;
}


void Parse0x25(char *data, int index) {
    if (bot[index]->connected)
        send(bot[index]->sck, data, 8, 0);
}


void Parse0x26(char *data, int index) {

}


void Send0x50(int index) {
	InsertDWORD(0);
	InsertDWORD('IX86');
	InsertDWORD('WAR3');
	InsertDWORD(verbyte);
	InsertDWORD(0);
	InsertDWORD(0);
	InsertDWORD(0);
	InsertDWORD(0);
	InsertDWORD(0);
	InsertDWORD('ASU');
	InsertNTString("United States");
	SendPacket(0x50, index);

	InsertDWORD(0);
	SendPacket(0x25, index);
}


void Parse0x50(char *data, int index) {
	char buf[128], keyhash[20];
	char *mpqName, *ChecksumFormula;
	const char *files[3];
	uint32_t checksum;
	int mpqNumber, result;

	bot[index]->ClientToken = gettick();
	bot[index]->ServerToken = *(int32_t *)(data + 8);
	mpqName = data + 24;
	ChecksumFormula = data + 25 + strlen(mpqName);
	mpqNumber = *(strchr(mpqName, '.') - 1) - 0x30;

	if (!(bot[index]->cdkey->status & KEY_DECODED)) {
		DecodeWC3Key(bot[index]->cdkey->encoded, &bot[index]->cdkey->prod,
			&bot[index]->cdkey->pub, bot[index]->cdkey->priv);
	}
	
	HashWAR3Key(bot[index]->ClientToken, bot[index]->ServerToken,
		bot[index]->cdkey->prod, bot[index]->cdkey->pub,
		bot[index]->cdkey->priv, keyhash);

	if (gstate & GFS_USEBNLS) {
		if (ConnectSocket(bnlsserver, bnlsport, &sBNLS)) {
			InsertDWORD(BNLSID_WAR3);
			InsertDWORD(mpqNumber);
			InsertNTString(ChecksumFormula);
			SendBNLSPacket(0x09);
			result = recv(sBNLS, buf, sizeof(buf), 0);
			if (!result || result == INVALID_SOCKET) {
				printf("[BNLS] recv() failed, errno: %d\n", errno);
				CloseBotSck(index);
				return;
			}
			if (buf[2] == 0x09) {
				printf("[BNLS] Received 0x09!\n");
				if (*(int32_t *)(buf + 3)) {
					checksum = *(uint32_t *)(buf + 11);
				} else {
					printf("[BNLS] Packet req failed.\n");
					CloseBotSck(index);
					return;
				}
			}
		} else {
			printf("[BNLS] Connection failed.\n");
			CloseBotSck(index);
			return;
		}
	} else {
		files[0] = hashes[0];
		files[1] = hashes[1];
		files[2] = hashes[2];
		result = CheckRevision(ChecksumFormula, files, 3, mpqNumber, &checksum);
		if (result) {
			printf("CheckRevision() failed: %s\n", crerrstrs[result]);
			CloseBotSck(index);
			return;
		}
	}
	WardenKeyInit(keyhash, index);
	Send0x51(checksum, bot[index]->cdkey->prod, bot[index]->cdkey->pub, keyhash, index);
}


void Send0x51(uint32_t checksum, uint32_t productvalue,
			  uint32_t publicvalue, char *keyhash, int index) {
	InsertDWORD(bot[index]->ClientToken);
	InsertDWORD(0x011804F3); // doesn't need to be correct
	InsertDWORD(checksum);
	InsertDWORD(1);
	InsertDWORD(0);
	InsertDWORD(WC3_KEYLEN);
	InsertDWORD(productvalue);
	InsertDWORD(publicvalue);
	InsertDWORD(0);
	InsertVoid(keyhash, 20);
	InsertNTString("war3.exe 03/16/10 13:27:18 471040"); //doesn't need to be correct
	InsertNTString("Phyros"); // do something special here?
	SendPacket(0x51, index);
}


void Parse0x51(char *data, int index) {
	uint32_t response;

	switch (*(int32_t *)(data + 4)) {
		case 0x0000:
			Send0x53(index);
			return;
		case 0x0100:
			response = 0;
			break;
		case 0x0101:
			response = 1;
			break;
		case 0x0200:
			bot[index]->fstate |= BFS_ERR_INVALKEY;
			bot[index]->cdkey->status |= KEY_INVALID;
			response = 2;
			break;
		case 0x0201:
			bot[index]->cdkey->status |= KEY_INUSE;
			response = 3;
			break;
		case 0x0202:
			bot[index]->cdkey->status |= KEY_BANNED;
			response = 4;
			break;
		case 0x0203:
			bot[index]->cdkey->status |= KEY_BADPROD;
			response = 5;
			break;
		case 0x0210:
			response = 6;
			break;
		case 0x0211:
			response = 7;
			break;
		case 0x0212:
			response = 8;
			break;
		default:
			response = 9;
	}
	printf("[BNET] %s [%s].\n%s\n", authresponse[response],
		(response > 1) ? bot[index]->cdkey->encoded : "|", data + 8);
	if (response > 1)
		ShiftCDKey(index);
	CloseBotSck(index);
}


void Send0x52(int index) {
	char *buf;
	unsigned int bufsize;

	bufsize = strlen(bot[index]->username) + 65;
	buf = malloc(bufsize);

	if (SRPAccountCreate(bot[index]->nls, buf, bufsize)) {
		InsertVoid(buf, bufsize);
		SendPacket(0x52, index);
	} else {
		printf("WARNING: nls_account_create() failed\n");
		CloseBotSck(index);
	}
	free(buf);
}


void Parse0x52(char *data, int index) {
	register int tmp = *(int32_t *)(data + 4);

	if (!tmp) {
		Send0x53(index);
	} else {
		if (createresponse[tmp])
			printf("[BNET] %s.\n", createresponse[tmp]);
		else
			printf("[BNET] Unkwown account creation failure.\n");
		CloseBotSck(index);
	}
}


void Send0x53(int index) {
	char A[32];

	if (!bot[index]->nls)
		bot[index]->nls = SRPInit(bot[index]->username, bot[index]->password, strlen(bot[index]->password));
	if (!bot[index]->nls) {
		printf("WARNING: SRPInit() failed\n");
		CloseBotSck(index);
	}
	SRPGetA(bot[index]->nls, A);
	InsertVoid(A, 32);
	InsertNTString(bot[index]->username);
	SendPacket(0x53, index);
}


void Parse0x53(char *data, int index) {
	char M1[32], *Salt, *Var_B;

	if  (*(int32_t *)(data + 4) == 0x01) {
		printf("[BNET] Account doesn't exist.\n");
		Send0x52(index);
	} else {
		Salt = data + 8;
		Var_B = data + 40;
		SRPGetM1(bot[index]->nls, M1, Var_B, Salt);
		InsertVoid(M1, 20);
		SendPacket(0x54, index);
	}
}


void Parse0x54(char *data, int index) {
	uint32_t reply;
	
	reply = *(int32_t *)(data + 4);
	if (reply == 0x00 || reply == 0x0E) {
		SRPFree(bot[index]->nls);
		bot[index]->nls = NULL;
		Send0x0A(index);
	} else {
		printf("[BNET] Failed logon [0x%02x]\n", reply);
		CloseBotSck(index);
	}
}


void Send0x0A(int index) {
	InsertNTString(bot[index]->username);
	InsertByte(0x00);
	SendPacket(0x0A, index);
	InsertDWORD('WAR3');
	SendPacket(0x0B, index);
	InsertDWORD(0x02);
	InsertNTString(home);
	SendPacket(0x0C, index);
}


void Parse0x0A(char *data, int index) {
	int i;
	LPLOCALBOT locbot;

	nbots_online++;
	bot[index]->connected = 1;
	bot[index]->connectedtime = getuptime();
	strncpy(bot[index]->realname, data + 4, sizeof(bot[index]->realname));
	printf("You have logged onto battle.net as %s.\n", bot[index]->realname);
	//if (bot[index]->client == 'WAR3')
	//	bot[index]->fstate |= BFS_CLIENT_WC3;

	locbot = malloc(sizeof(LOCALBOT));
	strcpy(locbot->realname, bot[index]->realname);
	if (!(bot[index]->fstate & BFS_CLIENT_WC3)) {
		strncat(locbot->realname, realm, sizeof(locbot->realname));
		locbot->realname[sizeof(locbot->realname) - 1] = 0;
	}
	locbot->index = index;
	HtInsertItem(locbot->realname, locbot, localbots, TL_LOCBOTS);

	for (i = 0; i != index; i++) {
		if (bot[i]->connected)
			return;
	}
	masterbot      = index;
	bot_least_debt = index;
	printf(" *** profile %d (%s) set as bot master\n", index, bot[index]->realname);
}


void Parse0x13(char *data, int index) {
	bot[index]->fstate |= BFS_ERR_FLOODOUT;
	
	//figure out logic to adjust appropriately - i'd have to know something about the queue beforehand!
	//hmmm... maybe keep a running total 
	queue_bytes_overhead += 5;

	printf("Flooded out. Adjusted parameters:\n queue_ms_perpacket: %d\n" \
		" queue_ms_perbyte: %d\n queue_bytes_overhead: %d\n",
		queue_ms_perpacket, queue_ms_perbyte, queue_bytes_overhead);
}
 

void Send0x15(int index) {
	InsertDWORD('IX86');
	InsertDWORD('WAR3');
	InsertDWORD(0);
	InsertDWORD(0);
	SendPacket(0x15, index);
	bot[index]->tmptick = gettick();
}


void Parse0x15(char *data, int index) {
	char asdf[256];
	sprintf(asdf, "The active ping to %s is %dms.",
		server, gettick() - bot[index]->tmptick);
	if (bot[index]->fstate & BFS_EXTERNALREQ)
		QueueAdd(asdf, index);
	else
		puts(asdf);

	sprintf(asdf, "Active advertisement: %s, URL: %s", data + 20, data + 21 + strlen(data + 20));
	if (bot[index]->fstate & BFS_EXTERNALREQ) {
		QueueAdd(asdf, index);
		bot[index]->fstate &= ~BFS_EXTERNALREQ;
	} else {
		puts(asdf);
	}
}


void Parse0x19(char *data, int index) {
	puts(data + 8);
}

