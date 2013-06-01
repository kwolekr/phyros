/////////// Compile-time configuration ////////////
#define HT_INITIAL_SLOTS 2
#define HT_CASE_INSENSITIVE
///////////////////////////////////////////////////

#ifndef HT_CASE_INSENSITIVE
	#define strilcmp(x,y) strcmp(x,y)
	#define __key key
#endif

void HtInsertItem(const char *key, void *newentry, LPVECTOR *table, unsigned int tablelen);
int HtInsertItemUnique(const char *key, void *newentry, LPVECTOR *table, unsigned int tablelen);
int HtRemoveItem(const char *key, LPVECTOR *table, unsigned int tablelen);
void *HtUnassociateItem(const char *key, LPVECTOR *table, unsigned int tablelen);
void *HtGetItem(const char *key, LPVECTOR *table, unsigned int tablelen);
void HtResetContents(LPVECTOR *table, unsigned int tablelen);
unsigned int __fastcall hash(unsigned char *key);


