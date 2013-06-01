#define CHUSER_IF_STAR 0x0001
#define CHUSER_IF_SEXP 0x0002
#define CHUSER_IF_SSHR 0x0004
#define CHUSER_IF_JSTR 0x0008

#define CHUSER_IF_DSHR 0x0010
#define CHUSER_IF_DRTL 0x0020
#define CHUSER_IF_D2DV 0x0040
#define CHUSER_IF_D2XP 0x0080

#define CHUSER_IF_W2BN 0x0100
#define CHUSER_IF_WAR3 0x0200
#define CHUSER_IF_W3XP 0x0400

#define CHUSER_IF_CHAT 0x0800

#define CHUSER_IF_SC     (CHUSER_IF_STAR | CHUSER_IF_SEXP)
#define CHUSER_IF_D2     (CHUSER_IF_D2DV | CHUSER_IF_D2XP)
#define CHUSER_IF_WC3    (CHUSER_IF_WAR3 | CHUSER_IF_W3XP)
#define CHUSER_IF_LEGACY (CHUSER_IF_SSHR | CHUSER_IF_JSTR | CHUSER_IF_DSHR | CHUSER_IF_DRTL | CHUSER_IF_W2BN)

#define CHUSER_IF_PENDINGBAN   0x1000
#define CHUSER_IF_PENDINGBAN2  0x2000


typedef struct _whisper {
	int profile;
	char user[USERNAME_MAX_LEN];
	char message[256];
} WHISP, LPWHISP;

typedef struct _chuser {
	char username[USERNAME_MAX_LEN];
	unsigned short access; ////? do i still need this?
	unsigned short iflags;
	uint32_t flags;
	uint32_t ping;
	uint32_t client;
	uint32_t clan;
	unsigned int jointick;
} CHUSER, *LPCHUSER;

extern unsigned int floodthresh_tick;
extern unsigned int floodthresh_numticks;
extern unsigned int floodthresh_over;

extern unsigned int floodtick;
extern unsigned int lastjoin;

extern unsigned int nbots_online;
extern unsigned int nbots_inchannel;
extern unsigned int nbots_opped;

extern WHISP lastwhisper;

void Parse0x0F(char *data, int index);
void ChatHandleJoin(char *user, uint32_t flags, uint32_t ping,
					char *text, int joined, int index);
void __fastcall ChatHandleLeave(char *user, int flags, int index);
void __fastcall ChatHandleTalk(char *user, char *text, int index);
void __fastcall ChatHandleEmote(char *user, char *text, int index);
void __fastcall ChatHandleChannel(char *text, uint32_t flags, int index);
void __fastcall ChatHandleFlagsUpdate(char *user, uint32_t flags, int index);
void __fastcall ChatHandleInfo(char *user, char *text, int index);
void __fastcall SendText(char *text, int index);
void __fastcall RejoinChannel(int index);
void JoinNeatPrint(const char *user);
unsigned short GetClientIFlags(uint32_t client);

void GetRealms();
void __fastcall RealmFix(const char *user, char *dest, int wc3);
void __fastcall RealmFixInplace(char *user, int wc3);
void __fastcall RealmReverse(const char *user, char *dest, int wc3);
void __fastcall RealmTagStrip(const char *user, char *dest);
int __fastcall RealmTagIsPresent(const char *user);

uint32_t GetClanFromStatstring(char *text);
int IsAltCaps(const char *str);

