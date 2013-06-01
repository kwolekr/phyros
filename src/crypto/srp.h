#define NLS_VAR_N_STR "F8FF1A8B619918032186B68CA092B5557E976C78C73212D91216F6658523C787"
#define NLS_VAR_g 0x2F
#define NLS_VAR_I_STR "8018CF0A425BA8BEB8958B1AB6BF90AED970E6C"
#define NLS_SIGNATURE_KEY 0x10001

LPNLS SRPInit(const char *user, const char *pass, unsigned int passlen);
void SRPFree(LPNLS nls);
unsigned int SRPAccountCreate(LPNLS nls, char *buf, unsigned int bufSize);
unsigned int SRPAccountLogon(LPNLS nls, char *buf, unsigned int bufSize);
LPNLS SRPaccount_change_proof(LPNLS nls, char *buf, const char *new_password, const char *B, const char *salt);
void SRPGetS(LPNLS nls, char *out, const char *B, const char *salt);
void SRPGetV(LPNLS nls, char *out, const char *salt);
void SRPGetA(LPNLS nls, char *out);
void SRPGetK(LPNLS nls, char *out, const char *S);
void SRPGetM1(LPNLS nls, char *out, const char *B, const char *salt);
int SRPCheckM2(LPNLS nls, const char *var_M2, const char *B, const char *salt);
int SRPCheckSignature(uint32_t address, const char *signature_raw);
void SRPGetX(LPNLS nls, mpz_t x_c, const char *raw_salt);
void SRPGetV_mpz(LPNLS nls, mpz_t v, mpz_t x);
uint32_t SRPGetU(const char *B);

