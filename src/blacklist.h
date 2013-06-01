char *HandleBlacklistCmd(PLUSER pluser, char *args, char *outbuf);

void BlacklistLoad();
void BlacklistAddToDB(char *user, char type, char *addedby, char *reason, char *outbuf);
void BlacklistRemoveFromDB(char *text, char type, char *outbuf);
int BlacklistAddEntry(char *text, char type, char *addedby, char *reason);
char *BlacklistEnumerate(char type);
char *BlacklistEnumerateClans(char *buf);
char *BlacklistEnumerateShitlist(char *buf, int flags);
char *BlacklistEnumerateTags(char *buf);
char *BlacklistEnumerateTagTree(char *buf, LPTNODE tree);
char *BlacklistInfo(char *buf, char type, char *text);
int CALLBACK BlacklistFileCompare(const char *record, char *line);

PBUSER ClanBanLookup(uint32_t clan);
int ClanBanRemove(uint32_t clan);

