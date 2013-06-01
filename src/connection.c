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
 * connection.c - 
 *    Routines for creating, establishing, servicing, and ending internet & battle.net connections
 */


#include "main.h"
#include "fxns.h"
#include "winal.h"
#include "queue.h"
#include "timer.h"
#include "chat.h"
#include "hashtable.h"
#include "crypto/srp.h"
#include "packets.h"
#include "connection.h"

int autorc_rate[] = {
	     30 * 1000,	  //30 seconds
	 5 * 60 * 1000,   //5 minutes
	20 * 60 * 1000,   //20 minutes
	     30 * 1000    //30 seconds
};


///////////////////////////////////////////////////////////////////////////////


int ConnectSocket(const char *server, u_short port, int *sck) {
	struct sockaddr_in sName;
	char *tmp;

	*sck = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*sck == INVALID_SOCKET) {
		printf("Error creating socket!\n\n");
		return 0;
	}
	sName.sin_family = AF_INET;
	sName.sin_port = htons(port);

	tmp = (char *)server;
	while (*tmp && (isdigit(*tmp) || (*tmp == '.')))
		tmp++;
	if (*tmp) {
		struct hostent *h = (struct hostent *)gethostbyname(server);
		if (!h) {
			printf("WARNING: gethostbyname() failed, errno: %d\n", geterr);
			return 0;
		}
		memcpy(&sName.sin_addr, h->h_addr_list[0], h->h_length);
	} else {
		sName.sin_addr.s_addr = inet_addr(server);
	}
	if (connect(*sck, (struct sockaddr *)&sName, sizeof(sName)) == -1)  {
		printf("WARNING: connect() failed, errno: %d\n", geterr);
		return 0;
	}
	return 1;
}


void ConnectBot(int index) {
	pthread_t tid;

	if (!bot[index]->cdkey) {
		if (cdkeys->numelem <= index) {
			printf("WARNING: no cdkeys to connect profile %d with\n", index);
			return;
		}
		bot[index]->cdkey = (LPCDKEY)cdkeys->elem[index];
		bot[index]->cdkey->profileusing = index;
	}

	if (!ConnectSocket(server, port, &bot[index]->sck)) {
        printf("Error %d connecting: %s", geterr, strerror(geterr));
        return;
	}
	printf("Connected to %s:%d!\n", server, port);
    
	tid = _CreateThread((THREADPROC)RecvThreadProc, (void *)index);
	printf("RecvData thread started: ID 0x%08x\n", (unsigned int)tid);
	bot[index]->tid	= tid;
	send(bot[index]->sck, "\x01", 1, 0);
	Send0x50(index);
}


void CloseBotSck(int index) {
	shutdown(bot[index]->sck, SHUT_RDWR);
}


void DisconnectBot(int index, int reason) {
	char ts[16];
	int i;

	if (bot[index]->connected)
		nbots_online--;

	shutdown(bot[index]->sck, SHUT_RDWR);
	closesocket(bot[index]->sck);

	HtRemoveItem(bot[index]->realname, localbots, TL_LOCBOTS);

	bot[index]->sck       = INVALID_SOCKET;
    bot[index]->connected = 0;
	bot[index]->tid       = 0;
	if (bot[index]->nls)
		SRPFree(bot[index]->nls);
	bot[index]->nls         = NULL;
	bot[index]->realname[0] = 0;
	QueueClear(&bot[index]->queue);
	bot[index]->fstate &= ~(BFS_TEMP_FLAGS | BFS_CLIENT_WC3);

	GetTimeStamp(ts);
	printf("%s [BNET] Disconnected profile %d.\n", ts, index);

	if (reason != DISCN_GRACEFUL) {
		AutoReconnectStart(index, autorc_rate[reason]);
		bot[index]->fstate &= ~(BFS_ERR_FLOODOUT | BFS_ERR_INVALKEY);
	}

	if (index == masterbot) {
		for (i = index; i != numbots; i++) {
			if (bot[i]->connected) {
				masterbot = i;
				printf(" *** profile %d (%s) set as bot master\n",
					i, bot[i]->realname);
				return;
			}
		}
		masterbot = 0;
	}
}


void AutoReconnectStart(int index, int wait) {
	printf(" *** attempting reconnect in %d seconds\n", wait);
	SetAsyncTimer(index | TIMERID_AUTORC, wait, AutoReconnectTimerProc);
}


int CALLBACK AutoReconnectTimerProc(unsigned int idEvent) {
	ConnectBot(idEvent & ~TIMERID_AUTORC);
	return 0;
}


unsigned long CALLBACK RecvThreadProc(void *arg) {
	char recvbuf[RECV_BUFFER_LEN];
	char ts[16];
	int recvlen, bnetlen;
	int tmplen, index, disc_reason;
	char *temp;

	index = (int)arg;
	#ifndef _DEBUG
		SetSegvSignalHandler();
	#endif
	setjmp(bot[index]->jmpenv);

	while (1) {
		recvlen = recv(bot[index]->sck, recvbuf, sizeof(recvbuf), 0);
		handle_err:
		if (!recvlen) {
			GetTimeStamp(ts);
			printf("%s [BNET] Server gracefully disconnected.\n", ts);
			DisconnectBot(index, DISCN_GRACEFUL);
			return 0;
		} else if (recvlen == -1) {
			GetTimeStamp(ts);
			printf("%s [BNET] recv() failure, errno: %d: %s\n",
				ts, geterr, strerror(geterr));
			if (bot[index]->fstate & BFS_ERR_FLOODOUT)
				disc_reason = DISCN_FLOODOUT;
			else if (bot[index]->fstate & BFS_ERR_INVALKEY)
				disc_reason = DISCN_INVALKEY;
			else
				disc_reason = DISCN_UNKNOWN;
			DisconnectBot(index, disc_reason);
			return 0;
		}

		temp = recvbuf;
		while (recvlen >= 4) {
			if ((unsigned char)*temp != (unsigned char)0xFF) {
				printf("warning! packet header mismatch!\n");
				break;
			}
			bnetlen = *(uint16_t *)(temp + 2);
			if (bnetlen > recvlen) {
				printf("Expecting %d more bytes...\n", bnetlen - recvlen);
				if (temp + recvlen >= recvbuf + sizeof(recvbuf)) {
					memcpy(recvbuf, temp, recvlen);
					temp = recvbuf;
				}

				tmplen = recv(bot[index]->sck, temp + recvlen,
					sizeof(recvbuf) - (temp - recvbuf) - recvlen, 0);
				if (!tmplen || tmplen == INVALID_SOCKET) {
					printf("WARNING: recv() error in nested wait\n");
					recvlen = tmplen;
					goto handle_err;
				}
				recvlen += tmplen;
				continue;
			}
            if (!bot[index]->connected)
                printf("[BNET] Received 0x%02x!\n", temp[1]);
            if (pkhndindex[(unsigned int)temp[1]])
                (*pkthandlers[pkhndindex[(unsigned int)temp[1]] - 1])(temp, index);
			else
                printf("[BNET] Unhandled packet 0x%02x!\n", (unsigned char)temp[1]);
			recvlen -= bnetlen;
			temp += bnetlen;
		}
	}
	return 0;
}

