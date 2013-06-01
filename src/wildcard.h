#define WCP_BACK  0
#define WCP_FRONT 1
#define WCP_BOTH  2

const char *bmh_memmem(const char *haystack, unsigned int hlen,
					   const char *needle,   unsigned int nlen);
void *FindTagban(LPTNODE wcnodes[3], char *text);
void *FindPhraseban(LPTNODE wcnode, char *text);
char *DigestTag(char *text, int *type);
int WildcardMatch(const char *pattern, const char *text);

