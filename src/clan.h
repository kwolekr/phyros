
extern char clantagchrmap[256];

void Send0x70(char *clantag, int index);
void Send0x71(char *clantag, char *clanname, char *invited, int numinvited, int index);
void Send0x72(int response, int index);
void Send0x74(char *user, int index);
void Send0x77(char *user, int index);
void Send0x78(char *user, int index);
void Send0x79(int response, int index);
void Send0x7A(char *user, unsigned char newrank, int index);
void Send0x7D(int index);
void Send0x82(char *user, int index);

void Parse0x70(char *data, int index);
void Parse0x71(char *data, int index);
void Parse0x72(char *data, int index);
void Parse0x73(char *data, int index);
void Parse0x74(char *data, int index);
void Parse0x75(char *data, int index);
void Parse0x76(char *data, int index);
void Parse0x77(char *data, int index);
void Parse0x78(char *data, int index);
void Parse0x79(char *data, int index);
void Parse0x7A(char *data, int index);
void Parse0x7C(char *data, int index);
void Parse0x7D(char *data, int index);
void Parse0x7E(char *data, int index);
void Parse0x7F(char *data, int index);
void Parse0x81(char *data, int index);
void Parse0x82(char *data, int index);

char *HandleClanCreateCommand(char *args, int index);

