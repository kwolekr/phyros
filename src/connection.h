#define DISCN_GRACEFUL 0 
#define DISCN_FLOODOUT 1
#define DISCN_INVALKEY 2
#define DISCN_UNKNOWN  3


int ConnectSocket(const char *server, u_short port, int *sck);

void ConnectBot(int index);
void DisconnectBot(int index, int reason);
void CloseBotSck(int index);
void AutoReconnectStart(int index, int wait);
int CALLBACK AutoReconnectTimerProc(unsigned int idEvent);

#ifdef _WIN32
unsigned long WINAPI RecvThreadProc(void *arg);
#else
unsigned long RecvThreadProc(void *arg);
#endif

