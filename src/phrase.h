extern LPTNODE phrases;

void LoadPhrases();
void CheckPhrases(char *user, char *text);
int PhraseAdd(uint32_t action, const char *phrase, const char *reason);
void PhraseAddToDB(uint32_t action, const char *phrase, const char *reason);
void PhraseRemoveFromDB(const char *phrase);
char *HandlePhraseCmd(PLUSER pluser, const char *args, char *outbuf);

