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
 * queue.c - 
 *    Variable delay message send queue	system
 */


#include "main.h"
#include "chat.h"
#include "fxns.h"
#include "timer.h"
#include "queue.h"

int queue_ms_perpacket, queue_ms_perbyte, queue_bytes_overhead;
int bot_least_debt;


///////////////////////////////////////////////////////////////////////////////


void QueueAdd(char *text, int index) {
	LPQUEUENODE qnode;
	int len, time;
	char temp;

	if (!text || !*text)
		return;

	len = strlen(text);
	if (len > 220) {
		temp = text[219];
		
		text[219] = 0;
		QueueAdd(text, index);
		text[219] = temp;

		text += 219;
		index = curbotinc();
		len   = 220;
	}

	if (!bot[index]->queue.count) {
		time = QueueGetTime(len, index);
		if (time) {
			SetAsyncTimer(index | TIMERID_QUEUE, time, QueueTimerProc);
		} else {
			SendText(text, index);
			return;
		}
	}

	qnode = malloc(sizeof(QUEUENODE) + len + 1);
	qnode->flags = 0;
	qnode->next  = NULL;
	strcpy(qnode->text, text);

	if (bot[index]->queue.count)
		bot[index]->queue.tail->next = qnode;
	else
		bot[index]->queue.head = qnode;
	bot[index]->queue.tail = qnode;

	bot[index]->queue.count++;
}


int CALLBACK QueueTimerProc(unsigned int idEvent) {
	LPQUEUENODE qnode, tmpnode;
	int index, waittime;
	
	index = idEvent & ~TIMERID_QUEUE;

	if (!bot[index]->queue.count)
		return 0;

	qnode   = bot[index]->queue.head;
	tmpnode = qnode->next;
	SendText(qnode->text, index);
	free(qnode);
	bot[index]->queue.head = tmpnode;
	bot[index]->queue.count--;

	if (bot[index]->queue.count) {
		waittime = QueueGetTime(strlen(bot[index]->queue.head->text), index);
		SetAsyncTimer(idEvent, waittime, QueueTimerProc);
	} else {
		bot[index]->queue.tail = NULL;
		////////////////// IMPORTANT!!! ///////////////////
		///////REMEMBER TO SET THE HEAD/TAIL WHEN ADDING OR REMOVING ITEMS!!!!!
	}
	return 0;
}


int QueueGetTime(int len, int index) {
	LPQUEUEDESC queue;
	unsigned int tick, tmp;
	int recovered, newlen;

	queue = &bot[index]->queue;
	tick = gettick();
	queue->lastlen -= (tick - queue->lasttick) / queue_ms_perbyte;
	queue->lasttick = tick;
	if (queue->lastlen < 0) {
		queue->lastlen = 0;
		tmp = 0;
	} else {
		tmp = queue_ms_perpacket + ((len + queue->lastlen) * queue_ms_perbyte);
	}
	queue->lastlen += queue_bytes_overhead + len;

	recovered = (tick - bot[bot_least_debt]->queue.lasttick) / queue_ms_perbyte;
	newlen    = bot[bot_least_debt]->queue.lastlen - recovered;
	if (bot[index]->queue.lastlen < newlen)
		bot_least_debt = index;

	return tmp;
}


void QueueSetWait(LPQUEUEDESC queue, int time) {
	queue->lasttick = gettick();
	queue->lastlen  = (time - queue_ms_perpacket) / queue_ms_perbyte;
}


int QueueGetRemainingWait(LPQUEUEDESC queue) {
	unsigned int tick;
	int recovered, debt;

	tick = gettick();
	recovered = (tick - queue->lasttick) / queue_ms_perbyte;
	debt = queue->lastlen - recovered;
	return (debt > 0) ? debt * queue_ms_perbyte : 0;
}


void QueueClear(LPQUEUEDESC queue) {
	LPQUEUENODE qnode, tmpnode;

	qnode = queue->head;
	while (qnode) {
		tmpnode = qnode->next;
		free(qnode);
		qnode = tmpnode;
	}

	queue->count = 0;
	queue->head  = NULL;
	queue->tail  = NULL;
}


void QueueClearAll() {
	int i;

	ResetAsyncTimer();
	for (i = 0; i != numbots; i++)
		QueueClear(&bot[i]->queue);
}

