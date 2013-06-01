
#define LUR_HALTED			0x01
#define LUR_SHITLIST		0x02
#define LUR_KICKLIST		0x04

void LoadUsers();
void AddOwner(char *username);
void AddMasters(char *usernames);
char *HandleUsersCmd();
int UserAccessComparator(const void *elem1, const void *elem2);
void HandleAddUserCmd(PLUSER pluser, char *args, char *outbuf);
void HandleRemoveUserCmd(PLUSER pluser, char *args, char *outbuf);
void LoadAccessModifications();

int CALLBACK AccessFileCompare(const char *record, char *line);
void CALLBACK AccessFileModifyRecord(char *buffer, int data);

