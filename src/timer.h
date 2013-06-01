/////////// Compile-time configuration ////////////
#define TIMER_PERIODIC_NMAX 32
#define TIMER_PERIODIC_INTERVAL 30000
///////////////////////////////////////////////////

#define TIMER_FIRSTSHOT 0x80000000

typedef int (CALLBACK *TIMEEVENTPROC)(unsigned int);

typedef struct _tps {
	int id;
	int interval;
	int time;
	TIMEEVENTPROC tp;
} TPS, TIMEEVENT, *LPTPS, *LPTIMEEVENT;

pthread_t ptimertid;

void CreateAsyncTimer();
void DestroyAsyncTimer(int reason);
void SetAsyncTimer(int id, int time, TIMEEVENTPROC tp);
void RemoveAsyncTimer(int teindex);
void AddPeriodicTimer(int id, int time, TIMEEVENTPROC tp);
void RemovePeriodicTimer(int id);
void ResetAsyncTimer();
void *CALLBACK TimerWaitProc(void *ptps);
void *CALLBACK PeriodicTimerWaitProc(void *param);

