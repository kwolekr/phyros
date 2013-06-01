void UnhandledPacket(char *data, int index);
void IgnorePacket(char *data, int index);
void Parse0x25(char *data, int index);
void Parse0x26(char *data, int index);

void Send0x50(int index);
void Parse0x50(char *data, int index);
void Send0x51(uint32_t checksum, uint32_t productvalue,
			  uint32_t publicvalue, char *keyhash, int index);
void Parse0x51(char *data, int index);
void Send0x52(int index);
void Parse0x52(char *data, int index);
void Send0x53(int index);
void Parse0x53(char *data, int index);
void Parse0x54(char *data, int index);
void Send0x0A(int index);
void Parse0x0A(char *data, int index);

void Parse0x13(char *data, int index);
void Send0x15(int index);
void Parse0x15(char *data, int index);
void Parse0x19(char *data, int index);

typedef void (*PacketHandler)(char *data, int index);

extern int pkhndindex[];
extern PacketHandler pkthandlers[];

