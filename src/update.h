extern char updatesite[32];
extern char updateverfile[64];

int httpsck;
//extern jmp_buf httpesc;
const char *updateresult[8];

int CheckUpdate(const char *site, const char *verfile);
char *HttpEncodeString(char *in, int *written);
//unsigned char *HttpGetFile(const char *website, const char *httpdir, int *filelength);
unsigned char *HttpGetFile(const char *website, const char *httpdir,
						   int *filelength, const char *format, ...);

