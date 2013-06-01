/*-
 * Copyright (c) 2010 Ryan Kwolek
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
 * winal.c - 
 *    Microsoft Win32 API abstraction layer
 */

#include "main.h"
#include "fxns.h"
#include "winal.h"

#if defined(_WIN32) && defined(DONT_USE_LIBCMT)
	#include <io.h>

	#define PIPE_READ  0
	#define PIPE_WRITE 1

	#define _O_TEXT 0

	PROCESS_INFORMATION pi;

	HANDLE hStdout[2];
	HANDLE hStdin[2];
#endif

#ifdef _WIN32
	int priorities[] = {
		IDLE_PRIORITY_CLASS,
		BELOW_NORMAL_PRIORITY_CLASS,
		NORMAL_PRIORITY_CLASS,
		ABOVE_NORMAL_PRIORITY_CLASS,
		HIGH_PRIORITY_CLASS,
		REALTIME_PRIORITY_CLASS
	};

	_RtlGenRandom lpfnRtlGenRandom;
#else
	#include <sys/resource.h>
#endif

#ifdef _WIN32
	PDH_HQUERY hQuery, hCounter;
#endif

///////////////////////////////////////////////////////////////////////////////


#if defined(_WIN32) && defined(DONT_USE_LIBCMT)

FILE *popen(char *command, char *mode) {
	STARTUPINFO si;
	FILE *file;

	CreatePipe(&hStdout[PIPE_READ], &hStdout[PIPE_WRITE], NULL, 0);
	CreatePipe(&hStdin[PIPE_READ], &hStdout[PIPE_WRITE], NULL, 0);

	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb	      = sizeof(STARTUPINFO);
	si.dwFlags    = STARTF_USESTDHANDLES;
	si.hStdInput  = hStdin[PIPE_READ];
	si.hStdOutput = hStdout[PIPE_WRITE];
	si.hStdError  =	hStdout[PIPE_WRITE];

	if (!CreateProcess(NULL, command, NULL, NULL, 1,
		 DETACHED_PROCESS, NULL, NULL, &si, &pi)) {
		goto epicfail;
	}

	file = _fdopen(_open_osfhandle((long)
		((*mode == 'r') ? hStdout[PIPE_READ] : hStdin[PIPE_WRITE]),
		_O_TEXT), mode);

	if (!file) {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
epicfail:
		CloseHandle(hStdout[PIPE_READ]);
		CloseHandle(hStdout[PIPE_WRITE]);
		CloseHandle(hStdin[PIPE_READ]);
		CloseHandle(hStdin[PIPE_WRITE]);
		return NULL;
	}

	setvbuf(file, NULL, _IONBF, 0);

	return file;
}


void pclose(FILE *file) {
	fclose(file);
	CloseHandle(hStdout[PIPE_READ]);
	CloseHandle(hStdout[PIPE_WRITE]);
	CloseHandle(hStdin[PIPE_READ]);
	CloseHandle(hStdin[PIPE_WRITE]);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
}

#endif


pthread_t _CreateThread(THREADPROC tproc, void *arg) {
    pthread_t tid;
	#ifdef _WIN32
		CloseHandle(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)tproc, arg, 0, &tid));
	#else
		pthread_create(&tid, NULL, tproc, arg);
	#endif
	return tid;
}


void _ExitThread(int exitcode) {
	#ifdef _WIN32
		ExitThread(exitcode);
	#else
		pthread_exit(&exitcode);
	#endif
}


void _CancelThread(pthread_t tid, int exitcode) {
	#ifdef _WIN32
		/*
		 * IMPORTANT TODO:
		 * TerminateThread() should NOT be used!!
		 * Termination with this function is abrupt and could severely damage the application
		 * (it could hold a critical section within a system library, for instance)
		 * Under Windows 2000, XP, and Server 2003, the stack isn't freed resulting in a leak.
		 *
		 *   GET PROPER EXITING EVENT CODE IN HERE ASAP!
		 *
		 */
		HANDLE hThread;
		hThread = OpenThread(THREAD_TERMINATE, 0, tid);
		if (hThread) {
			TerminateThread(hThread, exitcode);
			CloseHandle(hThread);
		}
	#else
		pthread_cancel(tid);
	#endif
}


#ifdef _WIN32

void InitUptimePerfCounter() {
	char perfcntrname[64];
	DWORD pcnlen = sizeof(perfcntrname) - 8;

	PdhOpenQuery(NULL, 0, &hQuery);
	strcpy(perfcntrname, "\\System\\");
	PdhLookupPerfNameByIndex(NULL, 674, perfcntrname + 8, &pcnlen);
	PdhAddCounter(hQuery, perfcntrname, 0, &hCounter);
}


char *GetWinVersionName(LPOSVERSIONINFOEX lposvi) {
	if (lposvi->dwPlatformId == VER_PLATFORM_WIN32_NT) { //win nt
		if (lposvi->dwMajorVersion == 5) {
			if (!lposvi->dwMinorVersion) {
				return "2000";
			} else if (lposvi->dwMinorVersion == 1)	{
				return "XP";
			} else if (lposvi->dwMinorVersion == 2) {
				if (lposvi->wProductType == VER_NT_WORKSTATION) {
					return "XP Professional";
				} else {
					if (GetSystemMetrics(SM_SERVERR2))
						return "Server 2003 R2";
					else
						return "Server 2003";
				}
			}
		} else if (lposvi->dwMajorVersion == 6) {
			if (!lposvi->dwMinorVersion) {
				if (lposvi->wProductType == VER_NT_WORKSTATION)
					return "Vista";
				else
					return "Server 2008";
			} else if (lposvi->dwMinorVersion == 1) {
				if (lposvi->wProductType == VER_NT_WORKSTATION)
					return "7";
				else
					return "Server 2008 R2";
			}
		} else {
			return "NT";
		}
	} else if (lposvi->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) { //win 9x
		return "9x";

	}
	return NULL;
}


char *StrError(int error) {
	char buf[1024];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
				  FORMAT_MESSAGE_IGNORE_INSERTS,
				  NULL, error, 0, buf, sizeof(buf), NULL);
	printf(buf);
	return "";
}

#endif


void SetProcessPriority(int priority) {
	#ifdef _WIN32
		if ((priority >= 0) && (priority <= 5))
			SetPriorityClass(GetCurrentProcess(), priorities[priority]);
	#else
		if ((priority >= -20) && (priority <= 20))
			setpriority(PRIO_PROCESS, 0, priority);
	#endif
}

