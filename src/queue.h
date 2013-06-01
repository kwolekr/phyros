//#define MS_PERPACKET   600
//#define MS_PERBYTE	   30
//#define MS_PKTOVERHEAD 65

extern int queue_ms_perpacket, queue_ms_perbyte, queue_bytes_overhead;
extern int bot_least_debt;

void QueueAdd(char *text, int index);
int CALLBACK QueueTimerProc(unsigned int idEvent);
int QueueGetTime(int len, int index);
void QueueSetWait(LPQUEUEDESC queue, int time);
int QueueGetRemainingWait(LPQUEUEDESC queue);
void QueueClear(LPQUEUEDESC queue);
void QueueClearAll();

