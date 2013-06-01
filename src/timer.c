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
 * timer.c - 
 *    Scalable, high accuracy single threaded asynchronous timer and a simple periodic timer 
 */


#include "main.h"
#include "fxns.h"
#include "winal.h"
#include "timer.h"

#ifdef _WIN32
	//#define CRITICAL_SYNC
#else 
	#include <semaphore.h>
	#define WAIT_OBJECT_0     0
	#define WAIT_TIMEOUT  0x102
#endif

volatile int numtes;
int tessize;
LPTIMEEVENT tes;

#ifdef _WIN32
	HANDLE hTimeEvent;
#else 
	sem_t timeevent;
#endif

int cancel;
pthread_t timertid;

volatile int numptes;
TIMEEVENT ptes[TIMER_PERIODIC_NMAX];
pthread_t ptimertid;

#ifdef CRITICAL_SYNC
	CRITICAL_SECTION critsec;
#endif


///////////////////////////////////////////////////////////////////////////////


void CreateAsyncTimer() {
	#ifdef CRITICAL_SYNC
		InitializeCriticalSection(&critsec);
	#endif
	cancel  = 0;
	numtes  = 0;
	tessize = 16;
	tes = malloc(tessize * sizeof(TIMEEVENT)); 
	#ifdef _WIN32
		hTimeEvent = CreateEvent(NULL, 0, 0, NULL);
	#else
		sem_init(&timeevent, 0, 0);
	#endif
	timertid = _CreateThread(TimerWaitProc, NULL);
}


void DestroyAsyncTimer(int reason) {
	free(tes);
	cancel = 0;
	#ifdef _WIN32
		CloseHandle(hTimeEvent);
	#else
		sem_destroy(&timeevent);
	#endif
	if (pthread_equal(get_self_tid(), timertid))
		_ExitThread(reason);
	else
		_CancelThread(timertid, reason);
}


void SetAsyncTimer(int id, int time, TIMEEVENTPROC tp) {
	#ifdef CRITICAL_SYNC
		EnterCriticalSection(&critsec);
	#endif

	if (!time) {
		(*tp)(id);
		return;
	}
	tes[numtes].id       = id;
	tes[numtes].interval = time;
	tes[numtes].time     = time;
	tes[numtes].tp       = tp;
	numtes++;
	if (numtes == tessize) {
		tessize <<= 1; 
		tes = realloc(tes, tessize * sizeof(TIMEEVENT));
	}
	//printf(" *** Added timer [%d]: id: %x, time: %d\n", numtes, id, time);
	#ifdef _WIN32
		SetEvent(hTimeEvent);
	#else
		sem_post(&timeevent);
	#endif

	#ifdef CRITICAL_SYNC
		LeaveCriticalSection(&critsec);
	#endif
}


void ResetAsyncTimer() {
	#ifdef CRITICAL_SYNC
		EnterCriticalSection(&critsec);
	#endif

	numtes = 0;
	cancel = 1;
	#ifdef _WIN32
		SetEvent(hTimeEvent);
	#else
		sem_post(&timeevent);
	#endif

	#ifdef CRITICAL_SYNC
		LeaveCriticalSection(&critsec);
	#endif
}


void RemoveAsyncTimer(int teindex) {
	#ifdef CRITICAL_SYNC
		EnterCriticalSection(&critsec);
	#endif

	numtes--;
	if (numtes) {
		tes[teindex].id       = tes[numtes].id;
		tes[teindex].interval = tes[numtes].interval;
		tes[teindex].time     = tes[numtes].time;
		tes[teindex].tp       = tes[numtes].tp;
	}

	#ifdef CRITICAL_SYNC
		LeaveCriticalSection(&critsec);
	#endif
}


void RequestTimerRmove(int id) {
	
}


void *CALLBACK TimerWaitProc(void *ptps) {
	int i, i2, ret;
	unsigned int lowest, timeout, tick;
	unsigned long wevent;
	#ifndef _WIN32
		struct timespec ts;
		struct timeval tp;
	#endif

	while (1) {
		#ifdef _WIN32
			WaitForSingleObject(hTimeEvent, INFINITE); 
		#else
			sem_wait(&timeevent);
		#endif		
		while (numtes) {
			lowest = 0;
			for (i = 1; i != numtes; i++) {
				if (tes[i].time < tes[lowest].time)
					lowest = i;
			}
			i = lowest;
			timeout = tes[i].time;
			tick = gettick();

			#ifdef _WIN32
				wevent = WaitForSingleObject(hTimeEvent, timeout);
			#else
				gettimeofday(&tp, NULL);
				ts.tv_sec  = (timeout / 1000) + tp.tv_sec;
				ts.tv_nsec = ((timeout % 1000) * 1000000) + (tp.tv_usec * 1000);
				if (sem_timedwait(&timeevent, &ts) == -1) {
					if (errno == ETIMEDOUT)
						wevent = WAIT_TIMEOUT;
				} else {
					wevent = WAIT_OBJECT_0;
				}
			#endif 
			if (cancel) {
				cancel = 0;
				goto dcont;
			}
			if (wevent == WAIT_OBJECT_0)
				timeout = gettick() - tick;
			for (i2 = 0; i2 != numtes; i2++)
				tes[i2].time -= timeout;
			if (wevent == WAIT_TIMEOUT) {
				ret = (*tes[i].tp)(tes[i].id);
				if (!ret) {
					RemoveAsyncTimer(i);
				} else {
					if (!timeout) {
						printf("WARNING: 0 timeout on a repeating timer "
							"(id: 0x%x, time left: %dms), canceling request\n",
							tes[i].id, tes[i].time);
						RemoveAsyncTimer(i);
					}
					tes[i].time = tes[i].interval;
				}
			}
		}
	dcont: ;
	}
	return NULL;
}


void AddPeriodicTimer(int id, int time, TIMEEVENTPROC tp) {
	if (time && numptes < ARRAYLEN(ptes)) {
		ptes[numptes].id       = id;
		ptes[numptes].interval = time;
		ptes[numptes].time     = time;
		ptes[numptes].tp       = tp;
		numptes++;
	}
}


void RemovePeriodicTimer(int id) {
	int i;

	for (i = 0; i != numptes; i++) {
		if (ptes[i].id == id) {
			numptes--;
			ptes[i].id       = ptes[numptes].id;
			ptes[i].interval = ptes[numptes].interval;
			ptes[i].time     = ptes[numptes].time;
			ptes[i].tp       = ptes[numptes].tp;
		}
	}
}


void *CALLBACK PeriodicTimerWaitProc(void *param) {
	int i;
	static int counter = 0; 

	while (1) {
		Sleep(TIMER_PERIODIC_INTERVAL);
		for (i = 0; i != numptes; i++) {
			if (!(counter % ptes[i].time))
				(ptes[i].tp)(ptes[i].id);
		
		}
		counter++;
	}
}

