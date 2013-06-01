
#define CMD_V			0
#define CMD_VER         1
#define CMD_VERSION     2
#define CMD_SAY         3
#define CMD_RUN         4
#define CMD_LOUD        5
#define CMD_RUNQUIET    6
#define CMD_QUIET       7
#define CMD_B			8
#define CMD_BAN         9
#define CMD_UU			10
#define CMD_UNBAN       11
#define CMD_U           12
#define CMD_K			13
#define CMD_KICK        14
#define CMD_A			15
#define CMD_ACCESS      16
#define CMD_WHOIS       17
#define CMD_WHOAMI      18
#define CMD_CMDACCESS   19
#define CMD_HELP        20
#define CMD_CMDS        21
#define CMD_ADD         22
#define CMD_REM         23
#define CMD_USERS       24
#define CMD_BL          25
#define CMD_BLACKLIST   26
#define CMD_SHITADD     27
#define CMD_SA          28
#define CMD_TAGADD      29
#define CMD_TA          30
#define CMD_SHITDEL     31
#define CMD_SD          32
#define CMD_TAGDEL      33
#define CMD_TD          34
#define CMD_IPBAN       35
#define CMD_IP          36
#define CMD_UNIPBAN     37
#define CMD_UNIP        38
#define CMD_SWEEP       39
#define CMD_SB          40
#define CMD_IPSWEEP     41
#define CMD_IDLESWEEP   42
#define CMD_P		    43
#define CMD_PING        44
#define CMD_BANCOUNT    45
#define CMD_BANNED      46
#define CMD_J	        47
#define CMD_JOIN        48
#define CMD_FJOIN       49
#define CMD_FORCEJOIN   50
#define CMD_FJ          51
#define CMD_HOME        52
#define CMD_CLOSE       53
#define CMD_EXIT        54
#define CMD_QUIT        55
#define CMD_LEAVE       56
#define CMD_DAEMON      57
#define CMD_HIDE        58
#define CMD_SHOW        59
#define CMD_CONNECT     60
#define CMD_DISCONNECT  61
#define CMD_RC          62
#define CMD_RECONNECT   63
#define CMD_UPTIME      64
#define CMD_CQ          65
#define CMD_SCQ         66
#define CMD_CBQ         67
#define CMD_SCBQ        68
#define CMD_LW          69
#define CMD_LASTWHISPER 70
#define CMD_DATE        71
#define CMD_TIME        72
#define CMD_MOTD        73
#define CMD_SETMOTD     74
#define CMD_CLAN        75
#define CMD_CRANK       76
#define CMD_INVITE      77
#define CMD_TINVITES    78
#define CMD_ACCEPT      79
#define CMD_DECLINE     80
#define CMD_DP          81
#define CMD_DDP         82
#define CMD_CP          83
#define CMD_AUTOCP      84
#define CMD_GREET       85
#define CMD_IDLE        86
#define CMD_HALT        87
#define CMD_SETTRIGGER  88
#define CMD_IGNPUB      89
#define CMD_CLIENTBAN   90
#define CMD_PHRASE      91
#define CMD_PB          92
#define CMD_PHRASEBAN   93
#define CMD_PHRASEADD   94
#define CMD_PHRASEDEL   95
#define CMD_PINGBAN     96
#define CMD_PLUGBAN     97
#define CMD_INDEXBAN    98
#define CMD_BANEVASION  99
#define CMD_NUMBERSBAN  100
#define CMD_ALTCAPSBAN  101
#define CMD_BANNING     102
#define CMD_LOADBAN     103
#define CMD_WINBAN      104
#define CMD_AUTOLOAD    105
#define CMD_DESIGNATE   106
#define CMD_OP          107
#define CMD_LOCKDOWN    108
#define CMD_MASS        109
#define CMD_RJ          110
#define CMD_PROFILES    111
#define CMD_SETNAME     112
#define CMD_SETPASS     113
#define CMD_RELOAD      114
#define CMD_CREMOVE     115
#define CMD_CHIEFTAIN   116
#define CMD_MI          117
#define CMD_MEMBERINFO  118
#define CMD_CREATE      119
#define CMD_CHECKCLAN   120
#define CMD_LOCK        121
#define CMD_LOGGING     122
#define CMD_DISBAND     123
#define CMD_ERASE       124
#define CMD_PLGLOAD     125
#define CMD_PLGSTAT     126
#define CMD_PLGUNLOAD   127
#define CMD_UPDATE      128


typedef struct _cmddesc {
	const char *string;
	int access;
} CMDDESC, *LPCMDDESC;

extern LPNODE cmdtreeroot;
extern NODE nodes[];
extern CMDDESC cmddesc[];

void CheckCommand(char *user, char *text, int external, int index);

void HandleCmdReq(PLUSER pluser, char *text, int index);


void CmdTreeInit();
void CmdTreeInsert(const char *key, int data);
int __fastcall CmdGetIndex(unsigned int cmd);
void DSWBalanceTree(LPNODE lpnode);
int RotateNodeLeft(LPNODE lpnode);
int RotateNodeRight(LPNODE lpnode);

char *HandleUptimeCmd(char *buf, char *arg, int index);
char *HandleProfilesCmd();
void ToggleGlobalFlag(char *arg, int flag, char *buf,
					  const char *fmt, const char *yes, const char *no);
void SetFlagAndValue(char *arg, int flag, int *val, char *buf,
					 const char *yesfmt, const char *no);

char *HelpLookupCmdStr(int cmdi);
char *HelpLookupCmdUsage(int cmdi);

