//#define BANQUEUE_INITIALSIZE 64

#define BTYPE_BAN	 0
#define BTYPE_UNBAN	 1
#define BTYPE_KICK   2
#define BTYPE_FORCED 0x04
#define BTYPE_IP	 0x08
#define BTYPE_FBAN   (BTYPE_BAN | BTYPE_FORCED)
#define BTYPE_IPBAN  (BTYPE_BAN | BTYPE_IP) 
#define BTYPE_UNIP	 (BTYPE_UNBAN | BTYPE_IP)
#define BTYPE_WC3CTX 0x10
#define BTYPE_WC3USR 0x20

#define BANTYPE(x) ((x) & 3)

#define BANLIST_CLEAR         0
#define BANLIST_BAN_PENDING   1
#define BANLIST_UNBAN_PENDING 2
#define BANLIST_IN_QUEUE	  3

struct _chuser;

extern LPQUEUENODE bq_head;
extern LPQUEUENODE bq_tail;
extern int bq_count;
extern int bq_max;

extern int ban_highping, ban_lowping;
extern int ban_winlow, ban_indexhigh;
extern int ban_numbershigh, ban_clients;
extern int ban_kickcount;

extern int ban_timing_normal;
extern int ban_timing_loadban;

extern int bl_nshits, bl_nkicks, bl_ntags, bl_nclans;

extern LPTNODE tagbans[3];
extern LPVECTOR clanbl[36];

extern char sweep_chan_name[64];
extern LPVECTOR sweep_chan;


void HandleBUKUserCmd(PLUSER pluser, char *temp, int type, int index);
char *HandleBannedCmd();

void AddBanQueue(int action, char *user, char *reason);
int CALLBACK BanQueueTimerProc(unsigned int idEvent);
int BanQueueSend(int index);
void CheckBan(struct _chuser *chuser, char *text, int index);

void BanlistUpdate(char *user, char *text, int action, int index);

char *SweepStart(const char *channel, char *outbuf);
void SweepGatherUsers(char *text, int index);
char *DigestWhoInput(char *text);
int CALLBACK SweepEndTimerProc(unsigned int idEvent);

