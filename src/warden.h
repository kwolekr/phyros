typedef void (__stdcall *lpfnSendPacket)(char *data, int len);
typedef int (__stdcall *lpfnCheckModule)(char *modname, uint32_t _2);
typedef uint32_t (__stdcall *lpfnLoadModule)(char *decryptkey, char *module, int modlen);
typedef void *(__stdcall *lpfnMemAlloc)(uint32_t len);
typedef void (__stdcall *lpfnMemFree)(void *mem);
typedef void (__stdcall *lpfnSetRC4)(void *lpKeys, uint32_t len);
typedef char *(__stdcall *lpfnGetRC4)(char *lpBuffer, uint32_t *len);

typedef void (__stdcall *lpfnGenKeys)(void *ppFncList, void *lpData, uint32_t dwSize);
typedef void (__stdcall *lpfnUnloadModule)(void *ppFncList);
typedef void (__stdcall *lpfnHandlePacket)(void *ppFncList, char *data, int len, uint32_t *dwBuffer);
typedef void (__stdcall *lpfnTick)(void *ppFncList, uint32_t _2);

/*typedef struct _wclass {
	void *lpModule;
	int   cbModSize;
	int   dllcount;
	void *moduleclass;
	void *lpfnInit;
	int   pktlenroundup;
	int   pktlenorig;
	void *pktdataorig;
	int   highflags;
} WCLASS, *LPWCLASS; */

typedef struct _wmodheader {
	uint32_t cbModSize;			//0x00
	void *lpfnFunc;						//0x04
	uint32_t rvaFxnTableReloc;	//0x08
	int reloccount;						//0x0C
	void *lpfnInit;						//0x10
	unsigned int unk1;					//0x14
	unsigned int unk2;					//0x18
	unsigned long rvaImports;			//0x1C
	int dllcount;					    //0x20
	int sectioncount;					//0x24
	unsigned long sectionlen;			//0x28
	void *lpfnExceptHandler;			//0x2C
	unsigned int unk3;					//0x30
	unsigned long rvaImportAddrOffset;  //0x34
	unsigned int unk4;					//0x38
	unsigned int unk5;					//0x3C
	unsigned long rvaGlobalVars;		//0x40
	unsigned int unk6;					//0x44
	unsigned int unk7;					//0x48
	uint16_t unk8;				//0x4C
} WMODHEADER, *LPWMODHEADER;

typedef struct _wmodprotect {
	void *base;
	uint32_t len;
	uint32_t protectdword;
} WMODPROTECT, *LPWMODPROTECT;

typedef struct _snpFnTable {
	lpfnSendPacket snpSendPacket;
	lpfnCheckModule snpCheckModule;
	lpfnLoadModule snpLoadModule;
	lpfnMemAlloc snpMemAlloc;
	lpfnMemFree snpMemFree;
	lpfnSetRC4 snpSetRC4;
	lpfnGetRC4 snpGetRC4;

} SNPFNTABLE, *LPSNPFNTABLE;

typedef struct _wardenFnTable {
	lpfnGenKeys wdnGenKeys;
	lpfnUnloadModule wdnUnloadModule;
	lpfnHandlePacket wdnHandlePacket;
	lpfnTick wdnTick;
} WARDENFNTABLE, *LPWARDENFNTABLE;

void Parse0x5E(char *data, int index);

void WardenParseCommand0(char *data, int len, int index);
void WardenParseCommand1(char *data, int len, int index);
void WardenParseCommand2(char *data, int len, int index);
void WardenParseCommand5(char *data, int len, int index);


void RC4Crypt(unsigned char *key, unsigned char *data, int length);
void WardenKeyInit(char *KeyHash, int index);
void WardenKeyDataInit(LPWDNKEYGENDATA source, char *seed, int length);
void WardenKeyGenerate(unsigned char *key_buffer, unsigned char *base, unsigned int baselen);
void WardenKeyDataUpdate(LPWDNKEYGENDATA source);
void WardenKeyDataGetBytes(LPWDNKEYGENDATA source, char *buffer, int bytes);
char WardenKeyDataGetByte(LPWDNKEYGENDATA source);

int WardenDecryptInflateModule(char *modulename, char *module, int modulelen, char *keyseed, int index);
int WardenPrepareModule(char *filename);
void WardenModuleInit(LPWMODHEADER lpwmodh);
void WardenUnloadModule();


void __stdcall WdnCbkSendPacket(char *data, int len);
int __stdcall WdnCbkCheckModule(char *modname, uint32_t _2);
uint32_t __stdcall WdnCbkLoadModule(char *decryptkey, char *module, int modlen);
void *__stdcall WdnCbkMemAlloc(uint32_t len);
void __stdcall WdnCbkMemFree(void *mem);
void __stdcall WdnCbkSetRC4(void *lpKeys, uint32_t len);
char *__stdcall WdnCbkGetRC4(char *buffer, uint32_t *len);

uint32_t WardenGenerateChecksum(char *data, int len);

int LookupApiParamNum(FILE *file, char *fnname);
void *__stdcall MyGetProcAddress(int hModule, char *lpProcName);
//FARPROC WINAPI MyGetProcAddress(HMODULE hModule, LPCSTR lpProcName);

