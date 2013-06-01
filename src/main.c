/***********************************************************
 *                                                         *
 *  888888ba  dP										   *
 *  88    `8b 88										   *
 * a88aaaa8P' 88d888b. dP    dP 88d888b. .d8888b. .d8888b. *
 *  88        88'  `88 88    88 88'  `88 88'  `88 Y8ooooo. *
 *  88        88    88 88.  .88 88       88.  .88       88 *
 *  dP        dP    dP `8888P88 dP       `88888P' `88888P' *
 *                        .88							   *
 *                    d8888P							   *
 *														   *
 **********************************************************/

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
 * main.c - 
 *    main(), loading routines, signal handlers
 */


/*
 Things to do....

  - add 0x26 support
  - add plugins
    - add plugin exports
    - make plugins:
	  - media players
	  - clan management system
	  - example packet handler extensions
	  - gui
  - make gui configurator
  - make reloadable and shit
  - add proxy support
  - add autoconnection
  - fix sigsegv signal handling on worker threads
  - add extra options for access (optional flags, and user who added, etc.)

  &*&*&*&*&*&*&*&*&*&*&*&*&*&* things to do right away:

  1). Finish banqueue
  2). Wire up new ban stuff to commands
  5). Add wildcard support to commands

  - add date to blacklistings and phrases?
  - fix realms for adding to the banlist

 */


#include "main.h"
#include "chat.h"
#include "clan.h"
#include "hashtable.h"
#include "packets.h"
#include "fxns.h"
#include "connection.h"
#include "commands.h"
#include "pbuffer.h"
#include "config.h"
#include "access.h"
#include "vector.h"
#include "winal.h"
#include "queue.h"
#include "timer.h"
#include "radix.h"
#include "banning.h"
#include "wildcard.h"
#include "cdkeymgmt.h"
#include "blacklist.h"
#include "phrase.h"
#include "warden.h"
#include "update.h"

const char *hash_filenames[] = {
	"war3.exe",
	"storm.dll",
	"game.dll"
};

pthread_t maintid;

jmp_buf jmpenv;

LPVECTOR users[TL_BNUSERS];
LPVECTOR lusers[TL_LUSERS];
LPVECTOR banned[TL_BANNED];
LPVECTOR localbots[TL_LOCBOTS];
int numusers;
int numlusers;
int bancount;

LPBOT bot[NUM_MAX_BOTS];
int numbots;
SOCKET sBNLS;

char server[64];
char bnlsserver[64];
char owner[32];
char hashes[3][256];
//char dir[256];
char home[64];
char *realm, *realmwc3;
int verbyte;
u_short port, bnlsport;

int gstate;
int curbot;
int numopbots;
int masterbot;
char *curchan;

unsigned int loadedtime;

PLUSER botpluser;

char *lpModuleImage[3];

LPVECTOR cdkeys;
int curkeypos;

char conf_path_config[256];
char conf_path_access[256];
char conf_path_phrase[256];
char conf_path_bl[256];
char conf_path_cmdacc[256];
char conf_path_cdkeys[256];
char conf_path_proxies[256];
char conf_path_idles[256];


///////////////////////////////////////////////////////////////////////////////


int main(int argc, char **argv) {
	char text[256];
	int i;

	#ifdef _WIN32
		HMODULE hLib;
		WSADATA wsadata;
		WSAStartup(0x0202, &wsadata);

		InitUptimePerfCounter();

		hLib = LoadLibrary("advapi32.dll");
		if (hLib)
			lpfnRtlGenRandom = (_RtlGenRandom)GetProcAddress(hLib, "SystemFunction036");

		#ifdef _DEBUG
			_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF |
				_CRTDBG_CHECK_ALWAYS_DF |  _CRTDBG_CHECK_CRT_DF | _CRTDBG_ALLOC_MEM_DF);
		#endif
	#endif

	GetOS(text);
	printf("\n -- Phyros " BOT_VER " -- %s --\n\n", text);

	loadedtime = getuptime();

	#ifndef _WIN32
		SetPipeSignalHandler();
	
		if (getuid() == 0) {
			printf("WARNING: should not be running as root!\n");
			setuid(GetUidFromUsername("nobody"));
		} //////should set the group id instead so files can be modified at runtime
	#endif

	botpluser = malloc(sizeof(LUSER));
	botpluser->access = 103;
	strcpy(botpluser->username, "the bot");

	maintid = get_self_tid();
	
	#ifndef _DEBUG
		SetSegvSignalHandler();
	#endif

	tagbans[WCP_BACK]  = RadixInit();
	tagbans[WCP_FRONT] = RadixInit();
	tagbans[WCP_BOTH]  = RadixInit();
	phrases	 = RadixInit();
	cdkeys   = VectorInit(8);
	//banqueue = malloc(BANQUEUE_INITIALSIZE * sizeof(char *));
	//bqsize   = BANQUEUE_INITIALSIZE;

	SetDefaultConfig();

	LoadConfig();
	LoadUsers();
	LoadCDKeys();
	GetRealms();
	BlacklistLoad();
	LoadPhrases();
	CmdTreeInit();
	LoadAccessModifications();

	ParseCmdLine(argc, argv);

	for (i = 0; i != 3; i++) {
		if (!hashes[i][0]) {
			gstate |= GFS_USEBNLS;
			printf("WARNING: path for binary \'%s\' not set, falling back to BNLS checkrevision\n",
				hash_filenames[i]);
			break;
		}
	}

	if (gstate & GFS_CHECKUPDATES) {
		printf(" - checking %s%s for updates...\n", updatesite, updateverfile);
		printf("%s\n", updateresult[CheckUpdate(updatesite, updateverfile)]);
	}

	CreateAsyncTimer();
	ptimertid = _CreateThread(PeriodicTimerWaitProc, NULL);
	AddPeriodicTimer(TIMER_NULLPKT, 10, NullPacketTimerProc);
	
	setjmp(jmpenv);


	////////////////////////////////////////////TEST BATTERY//////////////////////////////////////
	/*ChatHandleChannel("Clan Blah", 0, 0);
	//ChatHandleJoin("loladfg", 0, 43, "PXES 0 0 0 0 0 PXES", 0, 0); 
	ChatHandleJoin("loladf1aaaaaaaaaa", 0, 43, "PXES 0 0 0 0 0 PXES", 0, 0); 
	ChatHandleJoin("loladf2bbbbbfffe", 0, 43, "PXES 0 0 0 0 0 PXES", 0, 0); 
	ChatHandleJoin("loladf3", 0, 43, "PXES 0 0 0 0 0 PXES", 0, 0); 
	ChatHandleJoin("loladf4", 0, 43, "PXES 0 0 0 0 0 PXES", 0, 0); 
	ChatHandleJoin("loladf5", 0, 43, "PXES 0 0 0 0 0 PXES", 0, 0); 
	ChatHandleJoin("loladf6", 0, 43, "PXES 0 0 0 0 0 PXES", 0, 0); 
	ChatHandleJoin("loladf7", 0, 43, "PXES 0 0 0 0 0 PXES", 0, 0); 
	ChatHandleJoin("loladf8", 0, 43, "PXES 0 0 0 0 0 PXES", 0, 0); 
	ChatHandleJoin("loladf9", 0, 43, "PXES 0 0 0 0 0 PXES", 0, 0); 
	ChatHandleJoin("loladfa", 0, 43, "PXES 0 0 0 0 0 PXES", 0, 0); 
	ChatHandleJoin("loladfb", 0, 43, "PXES 0 0 0 0 0 PXES", 0, 0); 
	ChatHandleJoin("loladfc", 0, 43, "PXES 0 0 0 0 0 PXES", 0, 0); 
	ChatHandleJoin("loladfd", 0, 43, "PXES 0 0 0 0 0 PXES", 0, 0); */

	/*printf("%x\n", FindTextInWildcards(tagbans, "asdfgdfg"));
	printf("%x\n", FindTextInWildcards(tagbans, "as[tg]gdfg"));
	printf("%x\n", FindTextInWildcards(tagbans, "asdfgdfg[tg]"));
	printf("%x\n", FindTextInWildcards(tagbans, "asdfgdfg"));
	printf("%x\n", FindPhraseban(phrases, "raiasdfdasdfzomgzasdfasdfasdfasdf"));
	printf("%x\n", FindPhraseban(phrases, "raiasdfgdfgbloop"));
	printf("%x\n", FindPhraseban(phrases, "bloo asfukdfgdfg zomg"));*/
	/////////////////////////////////////////////////////////////////////////////////////////////

	while (1) {
		fgets(text, sizeof(text), stdin);
		text[strlen(text) - 1] = 0;
		if (*(int16_t *)text == '//')
			CheckCommand(NULL, text, 0, 0);
		else
			QueueAdd(text, curbotinc());
	}
	return 0;
}


void ParseCmdLine(int argc, char *argv[]) {
	int i;
	char *tmp;

	for (i = 1; i < argc; i++) {
		tmp = argv[i];
		while (*tmp) {
			switch (*tmp) {
				case '-':
					break;
				case 'q':
					break;
				case 'v':
					break;
				case 'V':
					break;
				default:
					printf("WARNING: unhandled option \'%c\', ignoring\n", *tmp);
			}
		}
	}
}


int CALLBACK IdleTimerProc(unsigned int idEvent) {
	return 1;
}


int CALLBACK NullPacketTimerProc(unsigned int idEvent) {
	int i;

	for (i = 0; i != numbots; i++) {
		if (gettick() - bot[i]->invited.time > 30000) {
			bot[i]->fstate &= ~BFS_CREATEINV;
			bot[i]->invited.tag		   = 0;
			bot[i]->invited.cookie	   = 0;
			bot[i]->invited.inviter[0] = 0;
		}
		if (bot[i]->connected)
			SendPacket(0x00, i);
	}
	
	if ((gettick() - lastjoin) > floodthresh_over) {
		gstate   &= ~GFS_FLOODED;
		floodtick = 0;
	}
	return 1;
}


#ifndef _WIN32
void SetPipeSignalHandler() {
	struct sigaction sa;

	memset(&sa.sa_mask, 0, sizeof(sa.sa_mask));
	sa.sa_handler   = SIG_IGN;
	sa.sa_flags     = 0;
	sa.sa_sigaction = NULL;
	sigaction(SIGPIPE, &sa, NULL);
}
#endif


void SetSegvSignalHandler() {
	#ifdef _WIN32
		signal(SIGSEGV, SegFaultHandler);
	#else
		struct sigaction sa;

		memset(&sa.sa_mask, 0, sizeof(sa.sa_mask));
		sa.sa_handler = NULL;
		sa.sa_flags = SA_SIGINFO;
		sa.sa_sigaction = SegFaultHandler;
		sigaction(SIGSEGV, &sa, NULL);
	#endif
}


void CtrlCHandler(int signo) {
	if (gstate & GFS_CHECKINGUPDATES) {
		shutdown(httpsck, SHUT_RDWR);
		closesocket(httpsck);
		printf("Aborted update check.\n");
		gstate &= ~GFS_CHECKINGUPDATES;
	} else {
		//printf("SIGINT received, are you sure you'd like to exit? [y/n]\n");
		//if (getchar() == 'y')
			exit(1);
	}
	#ifdef _WIN32
		signal(SIGINT, CtrlCHandler);
	#endif
}

#ifdef _WIN32
void SegFaultHandler(int signo) {
#else
void SegFaultHandler(int signo, siginfo_t *info, void *context) {
#endif
	int i;
	pthread_t self;
	#ifdef _WIN32
		PEXCEPTION_POINTERS lpep = (PEXCEPTION_POINTERS)_pxcptinfoptrs;
	#else
		ucontext_t *uctx = (ucontext_t *)context;
	#endif

	self = get_self_tid();
	for (i = 0; i != numbots; i++) {
		if (bot[i]->tid == self)
			goto outloop;
	}
	i = -1;
outloop:
/*
	printf("== profile %02d =======================================================\n"
		   "WARNING! SIGSEGV encountered, dumping crash context\n"
		   "eax: 0x%08x | ebx: 0x%08x | ecx: 0x%08x | edx: 0x%08x\n"
		   "esi: 0x%08x | edi: 0x%08x | ebp: 0x%08x | esp: 0x%08x\n"
		   "eip: 0x%08x | efl: 0x%08x\n"
		   "=====================================================================\n\n",
		   i,
	#ifdef _WIN32
			   lpep->ContextRecord->Eax, lpep->ContextRecord->Ebx, lpep->ContextRecord->Ecx,
			   lpep->ContextRecord->Edx, lpep->ContextRecord->Esi, lpep->ContextRecord->Edi,
			   lpep->ContextRecord->Ebp, lpep->ContextRecord->Esp,
			   lpep->ExceptionRecord->ExceptionAddress, lpep->ContextRecord->ContextFlags);
		signal(SIGSEGV, SegFaultHandler);
	#else
			   uctx->uc_mcontext.mc_eax, uctx->uc_mcontext.mc_ebx, uctx->uc_mcontext.mc_ecx,
			   uctx->uc_mcontext.mc_edx, uctx->uc_mcontext.mc_esi, uctx->uc_mcontext.mc_edi,
			   uctx->uc_mcontext.mc_ebp, uctx->uc_mcontext.mc_esp,
			   uctx->uc_mcontext.mc_eip, uctx->uc_mcontext.mc_eflags);
	#endif
*/
	if (i == -1) {
		if (pthread_equal(maintid, self)) {
			//longjmp(jmpenv, 0); //BROKEN, WTF?
		} else {
			DestroyAsyncTimer(-1);
		}
	} else {
		longjmp(bot[i]->jmpenv, 0);
	}
}

