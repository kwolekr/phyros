#ifndef SM_SERVERR2
	#define SM_SERVERR2 89
#endif
#define PDH_COUNTER_INDEX_UPTIME 674

pthread_t _CreateThread(THREADPROC threadproc, void *arg);
void _ExitThread(int exitcode);
void _CancelThread(pthread_t tid, int exitcode);
void SetProcessPriority(int priority);

#ifdef _WIN32
	void InitUptimePerfCounter();
	char *StrError(int error);
	char *GetWinVersionName(LPOSVERSIONINFOEX lposvi);
#endif

#ifdef _WIN32
	extern PDH_HQUERY hQuery, hCounter;
	extern BOOLEAN (WINAPI *lpfnRtlGenRandom)(PVOID, ULONG);
#endif

