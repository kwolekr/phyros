extern unsigned char sendbuffer[SEND_BUFFER_LEN];
extern unsigned int pbufferlen;

void InsertByte(unsigned char data);
void InsertWORD(uint16_t data);
void InsertDWORD(uint32_t data);
void InsertNTString(char *data);
void InsertNonNTString(char *data);
void InsertVoid(void *data, int len);
void SendPacket(unsigned char PacketID, int index);
void SendBNLSPacket(unsigned char PacketID);

