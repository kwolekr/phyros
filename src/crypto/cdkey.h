#define WC3_KEYLEN 26
#define WC3_BUFLEN (WC3_KEYLEN << 1)


void DecodeWC3Key(char *cdkey, unsigned int *prodval, unsigned int *pubval, char *privval);
void HashWAR3Key(uint32_t ClientToken, uint32_t ServerToken, 
				 unsigned int ProductVal, unsigned int PublicVal,
				 char *PrivateVal, char *output); 
void Accum64Mult(int rounds, const int x, int *a, int dcByte);
void DecodeKeyTable(int *keyTable);	
int CheckWC3Key(const char *cdkey);
