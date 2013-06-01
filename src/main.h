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

#if (defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__))
	#define __X86__
#endif

//////////  Compile-time configuration  ///////////
#define BOT_VER			 "0.7.5.0 Alpha"
#define BOT_VERSION		 0x00070500
#define DEBUG_QUEUE
#define NUM_MAX_BOTS	 32
#define RECV_BUFFER_LEN  0x4000
#define SEND_BUFFER_LEN  0x800
#define BNLSID_WAR3		 0x07
#define TL_BNUSERS		 1024 //'bnusers' hashtable is relative to sc realm
#define TL_LUSERS		 256  //'lusers' hashtable is absolutely realmed
#define TL_BANNED		 1024 //'banned' hashtable is relative to sc realm
#define TL_LOCBOTS		 16   //'locbots' hashtable is relative to wc3 realm
#define DONT_ALLOW_OWN_CLANBAN
//#define DONT_USE_LIBCMT
#define USERNAME_MAX_LEN 32
#define PHRASE_MAX_LEN   64
#define BANTEXT_MAX_LEN  192
#define BLTEXT_MAX_LEN   192
#define PHRASETEXT_MAX_LEN 192
///////////////////////////////////////////////////


#define _CRT_SECURE_NO_WARNINGS 1

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <gmp.h>

#ifdef _WIN32
	#pragma comment(lib, "WS2_32.lib")
	#pragma comment(lib, "Crypt32.lib")
	#pragma comment(lib, "pdh.lib")

	#pragma warning(disable : 4305) //stupid warning about 'int->char truncation'
	#pragma warning(disable : 4996) //stupid warning about POSIX being deprecated

	#if defined(_WIN32) && defined(_DEBUG)
		#include <crtdbg.h>
	#endif
	#if (_WIN32_WINNT >= 0x0500) //|| (WINVER < 0x0500)
		#error "Requires Windows 2000 or later! " _WIN32_WINNT
	#endif

	#include <windows.h>
	#include <wincrypt.h>
	#include <ntsecapi.h>
	#include <process.h>
	#include <time.h>
	#include <pdh.h>

	#define IsFilePresent(x) (GetFileAttributes(x) != INVALID_FILE_ATTRIBUTES)
	#define geterr GetLastError()
	#define	gettick() GetTickCount()
	#define strerror(x) StrError(x)
	#define strcasecmp(x,y) _stricmp(x,y)
	#define execvp(x,y) _execvp(x,y)
	#define alloca(x) _alloca(x)

	#ifndef DONT_USE_LIBCMT
		#define pclose(x) _pclose(x)   //_popen/_pclose only works with multithreaded
		#define popen(x,y) _popen(x,y) //versions of LIBC (LIBCMT.lib, LIBCMTD.lib)!!		   
	#endif

	#define get_self_tid() GetCurrentThreadId()
	#define pthread_equal(x, y) ((x) == (y))

	#define	SHUT_RDWR 2

	typedef __int8 int8_t;
	typedef __int16 int16_t;
	typedef __int32 int32_t;
	typedef __int64 int64_t;
	typedef unsigned __int8 uint8_t;
	typedef unsigned __int16 uint16_t;
	typedef unsigned __int32 uint32_t;
	typedef unsigned __int64 uint64_t;
	typedef unsigned long pthread_t;

	typedef BOOLEAN (WINAPI *_RtlGenRandom)(PVOID, ULONG);

#else

	#include <sys/socket.h>
	#include <sys/ioctl.h>
	#include <sys/types.h>
	#include <sys/param.h>
	#include <sys/stat.h>
	#include <sys/mman.h>
	#include <sys/utsname.h>
	#include <sys/times.h>
	#include <sys/ucontext.h>
	#include <net/if.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>

	#include <stdarg.h>
	#include <unistd.h>
	#include <string.h>
	#include <pthread.h>
	#include <netdb.h>
	#include <errno.h>
	#include <pwd.h>
	#include <ctype.h>

	#include <sha.h>
	#include <md5.h>

	#define ZeroMemory(addr, len) memset((addr), 0, (len))
	#define IsBadReadPtr(addr, len) ((addr) == 0 || (len) == 0)
	#define IsFilePresent(x) (!stat(x, NULL))
	#define getlasterr() errno
	#define closesocket(x) close(x)
	#define Sleep(x) usleep((x) * 1000)
	#define	get_self_tid() pthread_self()

	#define TRUE 1
	#define FALSE 0
	#define INVALID_SOCKET -1

	#ifndef __GNUC__
		#define __asm__ asm
		#define __inline__ inline
	#endif
	#define _inline inline

	#ifdef __i386__
		#ifndef __fastcall
			#define __fastcall __attribute__((fastcall))
		#endif
		#ifndef __stdcall
			#define __stdcall __attribute__((stdcall))
		#endif
	#else
		#define __fastcall
		#define __stdcall
	#endif
	#define CALLBACK
	#define geterr errno

	//#define USE_OPENSSL

	#ifndef _INT64_T_DECLARED
		typedef long long int64_t;
	#endif
	typedef int SOCKET;

#endif

#define GFS_FORCEPROXY		0x1
#define GFS_USEBNLS			0x2
#define GFS_IPBAN		    0x4
#define GFS_CHECKUPDATES	0x8
#define GFS_CHECKINGUPDATES 0x10
#define GFS_LOCKED			0x20
#define GFS_LOGGING			0x40
#define GFS_FLOODED			0x80
#define GFS_LOADBAN			0x100
#define GFS_AUTOLOAD		0x200
#define GFS_HIGHPINGBAN		0x400
#define GFS_LOWPINGBAN		0x800
#define GFS_WINBAN			0x1000
#define GFS_STAGBAN			0x2000
#define GFS_ACCEPTINVS		0x4000
#define GFS_LOCKDOWN		0x8000
#define GFS_NUMBERSBAN		0x10000
#define GFS_BANEVASION		0x20000
#define GFS_PLUGBAN			0x40000
#define GFS_INDEXBAN		0x80000
#define GFS_CLIENTBAN		0x100000
#define GFS_ALTCAPSBAN		0x200000
#define GFS_GETTINGUSERS	0x400000
#define GFS_SWEEPREQ		0x800000
#define GFS_SWEEPING		0x1000000
#define GFS_ADVTRACKBANS	0x2000000

#define BFS_USEPROXY		0x01
#define BFS_AUTOCONN		0x02
#define BFS_EXTERNALREQ		0x04
#define BFS_CREATEINV		0x10
#define BFS_GETTINGSWEEP	0x20
#define BFS_BQTIMERACTIVE	0x40

#define BFS_ERR_FLOODOUT	0x100
#define BFS_ERR_INVALKEY	0x200

#define BFS_CLIENT_WC3		0x1000

#define BFS_TEMP_FLAGS (BFS_EXTERNALREQ | BFS_CREATEINV | BFS_GETTINGSWEEP)

#define TIMER_NULLPKT  1
#define TIMER_IDLE     2
#define TIMER_CHKFLOOD 3

#define KEY_DECODED 0x01
#define KEY_INUSE	0x02
#define KEY_BANNED	0x04
#define KEY_MUTED	0x08
#define KEY_VOIDED	0x10
#define KEY_BADPROD 0x20
#define KEY_INVALID 0x40
#define KEY_BAD		(KEY_BANNED | KEY_VOIDED | KEY_BADPROD | KEY_INVALID)

#define ACT_BAN		0
#define ACT_UNBAN	1
#define ACT_KICK	2
#define ACT_IPBAN	3
#define	ACT_SHIT	4
#define ACT_SAY		5

#define TIMERID_QUEUE   	0x800000
#define TIMERID_BANQUEUE	0x400000
#define TIMERID_REQWHODONE	0x200000
#define TIMERID_AUTORC		0x100000

#define ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))
#define SWAP2(num) ((((num) >> 8) & 0x00FF) | (((num) << 8) & 0xFF00))
#define SWAP4(num) ((((num) >> 24) & 0x000000FF) | (((num) >> 8) & 0x0000FF00) | (((num) << 8) & 0x00FF0000) | (((num) << 24) & 0xFF000000))


typedef void *(CALLBACK *THREADPROC)(void *);

typedef struct _vector {
	int numelem;
	int maxelem;
	void *elem[0];
} VECTOR, *LPVECTOR;

typedef struct _queuenode {
	struct _queuenode *next;
	int flags;
	char text[0];
} QUEUENODE, *LPQUEUENODE;

typedef struct _invitation {
	unsigned int time;
	uint32_t tag;
	uint32_t cookie;
	char inviter[USERNAME_MAX_LEN];
} INVITE, *LPINVITE;

typedef struct _nls {
    char user[USERNAME_MAX_LEN];
    unsigned int userlen;
    char userpasshash[20];
    mpz_t n;
    mpz_t a;
    gmp_randstate_t rand;
	char *A;
	char *S;
	char *K;
	char *M1;
	char *M2;
} NLS, *LPNLS;

typedef struct _wdnkeygendata {
	int curpos;
	char data[20];
	char src1[20];
	char src2[20];
} WDNKEYGENDATA, *LPWDNKEYGENDATA;

typedef struct _wdnkeys {
	WDNKEYGENDATA keygendata;
	unsigned char keyout[258];
	unsigned char keyin[258];
} WDNKEYS, *LPWDNKEYS;

typedef struct _cdkey {
	unsigned char status;
	char profileusing;
	char encoded[28];
	uint32_t prod;
	uint32_t pub;
	char priv[10];
} CDKEY, *LPCDKEY;

typedef struct _queue {
	LPQUEUENODE head;
	LPQUEUENODE tail;
	int count;
	int max;
	int lasttick;
	int lastlen;
} QUEUEDESC, *LPQUEUEDESC;

typedef struct _botsettings {
	int sck;
	int connected;
	char username[USERNAME_MAX_LEN];
	char password[64];
	LPCDKEY cdkey;
	char realname[USERNAME_MAX_LEN];
	char curchannel[64];
	int flags;
	int fstate;
	uint32_t ClientToken;
	uint32_t ServerToken;

	unsigned int connectedtime;
	unsigned int tmptick;
	int trigger;

	LPNLS nls;
	WDNKEYS warden;
	QUEUEDESC queue;

	uint32_t clan;
	char clanstr[8];
	INVITE invited;

	pthread_t tid;
	jmp_buf jmpenv;
} BOT, *LPBOT;

typedef struct _luser {
	char username[USERNAME_MAX_LEN];
	int access;
	unsigned short flags;
	unsigned short oacces;
} LUSER, *PLUSER;

typedef struct _buser {
	char username[USERNAME_MAX_LEN];
	int access;
	unsigned short flags;
	unsigned short oacces;
	char addedby[USERNAME_MAX_LEN];
	char reason[BLTEXT_MAX_LEN];
} BUSER, *PBUSER;

typedef struct _phrase {
	char phrase[PHRASE_MAX_LEN];
	char reason[PHRASETEXT_MAX_LEN];
	uint16_t action;
	uint16_t phraselen;
} PHRASE, *LPPHRASE;

typedef struct _node {
	unsigned int key;
	int data;
	struct _node *lchild;
	struct _node *rchild;
} NODE, *LPNODE;

typedef struct _localbot {
	char realname[USERNAME_MAX_LEN];
	int index;
} LOCALBOT, *LPLOCALBOT;


extern LPVECTOR users[TL_BNUSERS];
extern LPVECTOR lusers[TL_LUSERS];
extern LPVECTOR banned[TL_BANNED];
extern LPVECTOR localbots[TL_LOCBOTS];
//LPVECTOR banlistfilo;
extern int gstate;
extern int numusers;
extern int bancount;
extern int numlusers;


extern LPBOT bot[NUM_MAX_BOTS];
extern int numbots;
extern SOCKET sBNLS;

extern char hashes[3][256];
extern char exeinfo[128];
uint32_t exeversion;
extern int verbyte;

extern char *realm, *realmwc3;

extern char server[64];
extern char bnlsserver[64];
extern char home[64];
//extern char dir[256];
extern u_short port, bnlsport;

extern char *lpModuleImage[3];

extern PLUSER botpluser;

extern int curbot;
extern unsigned int loadedtime;

extern LPVECTOR cdkeys;
extern int curkeypos;

extern int numopbots, masterbot;
extern char *curchan;

extern char conf_path_config[256];
extern char conf_path_access[256];
extern char conf_path_phrase[256];
extern char conf_path_bl[256];
extern char conf_path_cmdacc[256];
extern char conf_path_cdkeys[256];
extern char conf_path_proxies[256];
extern char conf_path_idles[256];

int CALLBACK IdleTimerProc(unsigned int idEvent);
int CALLBACK NullPacketTimerProc(unsigned int idEvent);

void ParseCmdLine(int argc, char *argv[]);


void SetSegvSignalHandler();
void CtrlCHandler(int signo);
#ifdef _WIN32
	void SegFaultHandler(int signo);
#else
	void SegFaultHandler(int signo, siginfo_t *info, void *context);
	void SetPipeSignalHandler();
#endif

