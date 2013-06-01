#define lwr(c) ((c) | (0x20 & -(((c) >= 'A') && ((c) <= 'Z'))))
#define upr(c) ((c) & ~(0x20 & -(((c) >= 'a') && ((c) <= 'z'))))

#ifndef _WIN32
	unsigned int gettick();
	int GetUidFromUsername(const char *username);
#endif 

void strncpychr(char *dest, char *src, int c, int num);
//int stricmp(const char *s1, const char *s2);
char *strrev(char *str);
char *skipws(char *str);
char *findws(char *str);
void lcase(char *str);
int strilcmp(const char *s1, const char *s2);
int isnumstr(char *str);
void lcasecpy(char *dest, const char *src);
void lcasencpy(char *dest, const char *src, unsigned int n);
void strrevncpy(char *dest, const char *src, unsigned int n);
int countchr(char *str, char find);
int isstrupper(const char *str);

void HexToStr(char *in, char *out);

void GetOS(char *buf);
void StrToHexOut(char *data, int len);
void __fastcall GetTimeStamp(char *buf);
int GetUptimeString(unsigned long time, char *outbuf);

int curbotinc();

void *CALLBACK RunLoudProc(void *cmd);
void *CALLBACK RunQuietProc(void *cmd);

unsigned int getuptime();

void SHA1(const char *source, unsigned long len, char *output);
void MD5(const char *source, unsigned long len, char *output);

void CALLBACK FileDeleteRecord(char *buffer, int data);
int FileModifyRecord(const char *filename, const char *record, int data,
					  int (CALLBACK *record_compare)(const char *, char *),
					  void (CALLBACK *record_modify)(char *, int));

unsigned long GenRandomSeedUL();

