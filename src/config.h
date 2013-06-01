#define CFG_SERVER			0
#define CFG_PORT			1
#define CFG_BNLSSERVER		2
#define CFG_BNLSPORT		3
#define CFG_HOME			4
#define CFG_OWNER			5
#define CFG_MASTERS			6
#define CFG_USEBNLS			7
#define CFG_FORCEPROXY		8
#define CFG_IPBAN			9
#define CFG_WAR3VB			10
#define CFG_HASH1			11
#define CFG_HASH2			12
#define CFG_HASH3			13
#define CFG_UPDATESITE		14
#define CFG_UPDATEFILE		15
#define CFG_CHECKUPDATES 	16
#define CFG_PROCESSPRIORITY	17
#define CFG_FLOODTICK		18
#define CFG_FLOODNUMTICKS	19
#define CFG_FLOODOVER		20
#define CFG_KICKBANCOUNT    21
#define CFG_BANINTERVAL		22
#define CFG_LOADBANDELAY	23
#define CFG_QUEUEPERPKT		24
#define CFG_QUEUEPERBYTE	25
#define CFG_QUEUEOVERHEAD	26

#define CFG_USER		0
#define CFG_PASS		1
#define CFG_CDKEY		2
#define CFG_TRIGGER		3
#define CFG_USEPROXY	4
#define CFG_AUTOCONNECT	5

#define CFGLDR_STATE_NONE    0
#define CFGLDR_STATE_GLOBAL  1
#define CFGLDR_STATE_PROFILE 2

typedef struct _cfgstrtable {
	const char **cfgstrs;
	unsigned int arraylen;
	void (*handler)(int, char *, int);
} CFGSTRTABLE, *LPCFGSTRTABLE;

void LoadConfig();
void SetDefaultConfig();
void CfgHandleGlobalEntry(int cmdi, char *text, int profile);
void CfgHandleProfileEntry(int cmdi, char *text, int profile);
char *HandleReloadCmd(char *args);
char *HandleEraseCmd(char *args);

