/*-
 * Copyright (c) 2007 Ryan Kwolek
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of
 *     conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list
 *     of conditions and the following disclaimer in the documentation and/or other materials
 *     provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* 
 * srp.c - 
 *    Secure Remote Protocol implementation for Warcraft 3's NLS (New Logon System)
 */

#include "../main.h"
#include "../fxns.h"
#include "srp.h"

const char NLS_I[] = {
    0x6c, 0x0e, 0x97, 0xed, 0x0a, 0xf9, 0x6b, 0xab, 0xb1, 0x58,
	0x89, 0xeb, 0x8b, 0xba, 0x25, 0xa4, 0xf0, 0x8c, 0x01, 0xf8
};

const char NLS_N[] = {
    0xD5, 0xA3, 0xD6, 0xAB, 0x0F, 0x0D, 0xC5, 0x0F, 0xC3, 0xFA, 0x6E, 0x78, 0x9D, 0x0B, 0xE3, 0x32,
	0xB0, 0xFA, 0x20, 0xE8, 0x42, 0x19, 0xB4, 0xA1, 0x3A, 0x3B, 0xCD, 0x0E, 0x8F, 0xB5, 0x56, 0xB5,
	0xDC, 0xE5, 0xC1, 0xFC, 0x2D, 0xBA, 0x56, 0x35, 0x29, 0x0F, 0x48, 0x0B, 0x15, 0x5A, 0x39, 0xFC,
    0x88, 0x07, 0x43, 0x9E, 0xCB, 0xF3, 0xB8, 0x73, 0xC9, 0xE1, 0x77, 0xD5, 0xA1, 0x06, 0xA6, 0x20,
	0xD0, 0x82, 0xC5, 0x2D, 0x4D, 0xD3, 0x25, 0xF4, 0xFD, 0x26, 0xFC, 0xE4, 0xC2, 0x00, 0xDD, 0x98,
	0x2A, 0xF4, 0x3D, 0x5E, 0x08, 0x8A, 0xD3, 0x20, 0x41, 0x84, 0x32, 0x69, 0x8E, 0x8A, 0x34, 0x76,
    0xEA, 0x16, 0x8E, 0x66, 0x40, 0xD9, 0x32, 0xB0, 0x2D, 0xF5, 0xBD, 0xE7, 0x57, 0x51, 0x78, 0x96,
	0xC2, 0xED, 0x40, 0x41, 0xCC, 0x54, 0x9D, 0xFD, 0xB6, 0x8D, 0xC2, 0xBA, 0x7F, 0x69, 0x8D, 0xCF
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////


LPNLS SRPInit(const char *user, const char *pass, unsigned int passlen) {
	LPNLS nls;
	char *userpass;
	unsigned int j, i = 0;
	
	nls = malloc(sizeof(NLS));
	if (!nls)
		return NULL;

	while ((user[i]) && (i < sizeof(nls->user) - 1)) {
		nls->user[i] = toupper((unsigned char)user[i]);
		i++;
	}
	nls->user[i] = 0;
	nls->userlen = i;

	userpass = malloc(i + passlen + 1);
	memcpy(userpass, nls->user, i);
	userpass[i] = ':';

	//memcpy(userpass + i + 1, pass, passlen);
	for (j = 0; j != passlen; j++)            //// Not really sure what to do here...
		userpass[i + 1 + j] = toupper((unsigned char)pass[j]);   ////
	SHA1(userpass, i + passlen + 1, nls->userpasshash);
	free(userpass);

	mpz_init_set_str(nls->n, NLS_VAR_N_STR, 16);

	gmp_randinit_default(nls->rand);
	gmp_randseed_ui(nls->rand, GenRandomSeedUL());
	mpz_init2(nls->a, 256);
	mpz_urandomm(nls->a, nls->rand, nls->n);

	nls->A = NULL;
	nls->S = NULL;
	nls->K = NULL;
	nls->M1 = NULL;
	nls->M2 = NULL;

	return nls;
}


void SRPFree(LPNLS nls) {
    mpz_clear(nls->a);
    mpz_clear(nls->n);

    gmp_randclear(nls->rand);

	if (nls->A)
		free(nls->A);
	if (nls->S)
		free(nls->S);
	if (nls->K)
		free(nls->K);
	if (nls->M1)
		free(nls->M1);
	if (nls->M2)
		free(nls->M2);

    free(nls);
}


unsigned int SRPAccountCreate(LPNLS nls, char *buf, unsigned int bufSize) {
	mpz_t s, v, x;

    if (!nls | (bufSize < nls->userlen + 65))
        return 0;
    
    mpz_init2(s, 256);
    mpz_urandomb(s, nls->rand, 256);
    mpz_export(buf, (size_t*)0, -1, 1, 0, 0, s);
    
    SRPGetX(nls, x, buf);
    SRPGetV_mpz(nls, v, x);
    mpz_export(buf + 32, (size_t*)0, -1, 1, 0, 0, v);
    
	mpz_clear(x);
    mpz_clear(v);
    mpz_clear(s);
    
    strcpy(buf + 64, nls->user);
    return nls->userlen + 65;
}


unsigned int SRPAccountLogon(LPNLS nls, char *buf, unsigned int bufSize) {
    if (!nls || (bufSize < nls->userlen + 33))
        return 0;
    
    SRPGetA(nls, buf);
    strcpy(buf + 32, nls->user);
    return nls->userlen + 33;
}


LPNLS SRPAccountChangeProof(LPNLS nls, char *buf, const char *newpass,
							const char *B, const char *salt) {
    LPNLS newnls;
    mpz_t s;
    
    if (!nls)
        return NULL;
        
    newnls = SRPInit(nls->user, newpass, strlen(newpass));
    if (!newnls)
        return NULL;

    SRPGetM1(nls, buf, B, salt);
    
    mpz_init2(s, 256);
    mpz_urandomb(s, newnls->rand, 256);
    mpz_export(buf + 20, NULL, -1, 1, 0, 0, s);
    
	SRPGetV(newnls, buf + 52, buf + 20);

    mpz_clear(s);
    
    return newnls;
}


void SRPGetS(LPNLS nls, char *out, const char *B, const char *salt) {
    mpz_t temp, S_base, S_exp, x, v;
	uint32_t u;

    if (!nls)
        return;

	if (nls->S) {
		memcpy(out, nls->S, 32);
		return;
	}
    
    mpz_init2(temp, 256);
    mpz_import(temp, 32, -1, 1, 0, 0, B);
    
    SRPGetX(nls, x, salt);
    SRPGetV_mpz(nls, v, x);
    
    mpz_init_set(S_base, nls->n);
    mpz_add(S_base, S_base, temp);
    mpz_sub(S_base, S_base, v);
    mpz_mod(S_base, S_base, nls->n);
    
    mpz_init_set(S_exp, x);
	u = SRPGetU(B);
    mpz_mul_ui(S_exp, S_exp, u);
    mpz_add(S_exp, S_exp, nls->a);
    
    mpz_clear(x);
    mpz_clear(v);
    mpz_clear(temp);
    
    mpz_init(temp);
    mpz_powm(temp, S_base, S_exp, nls->n);

    mpz_clear(S_base);
    mpz_clear(S_exp);
    mpz_export(out, (size_t*)0, -1, 1, 0, 0, temp);
    mpz_clear(temp);

	nls->S = malloc(32);
	if (nls->S)
		memcpy(nls->S, out, 32);
}


void SRPGetV(LPNLS nls, char *out, const char *salt) {
    mpz_t g, v, x;
    
    if (!nls)
        return;
    
    mpz_init_set_ui(g, NLS_VAR_g);
    mpz_init(v);
    SRPGetX(nls, x, salt);
    
    mpz_powm(v, g, x, nls->n);
    
    mpz_export(out, (size_t*)0, -1, 1, 0, 0, v);
    mpz_clear(v);
    mpz_clear(g);
    mpz_clear(x);
}


void SRPGetA(LPNLS nls, char *out) {
    mpz_t g, A;
    size_t o;
    
    if (!nls)
        return;

	if (nls->A) {
		memcpy(out, nls->A, 32);
		return;
	}
    
    mpz_init_set_ui(g, NLS_VAR_g);
    mpz_init2(A, 256);
    
    mpz_powm(A, g, nls->a, nls->n);
    mpz_export(out, &o, -1, 1, 0, 0, A);

    mpz_clear(A);
    mpz_clear(g);

	nls->A = malloc(32);
	if (nls->A)
		memcpy(nls->A, out, 32);
}


void SRPGetK(LPNLS nls, char *outbuf, const char *S) {
	char odds[16], evens[16], oddhash[20], evenhash[20];
	char *saltptr, *oddptr, *evenptr;
	int i;

	saltptr = (char *)S;
	oddptr = odds;
	evenptr = evens;

	for (i = 0; i != 16; i++) {
		*(oddptr++) = *(saltptr++);
		*(evenptr++) = *(saltptr++);
	}
	SHA1(odds, 16, oddhash);
	SHA1(evens, 16, evenhash);
	saltptr = outbuf;
	oddptr  = oddhash;
	evenptr = evenhash;
	for (i = 0; i != 20; i++) {
		*(saltptr++) = *(oddptr++);
		*(saltptr++) = *(evenptr++);
	}
}	 


void SRPGetM1(LPNLS nls, char *out, const char *B, const char *salt) {
    unsigned char userhash[20], buf[176];
    char A[32], S[32], K[40];

	if (!nls)
		return;

	if (nls->M1) {
		memcpy(out, nls->M1, 20);
		return;
	}

	SHA1(nls->user, nls->userlen, (char *)userhash);

    SRPGetA(nls, A);
    SRPGetS(nls, S, B, salt);
    SRPGetK(nls, K, S);

	memcpy(buf, NLS_I, 20);
	memcpy(buf + 20, userhash, 20);
	memcpy(buf + 40, salt, 32);
	memcpy(buf + 72, A, 32);
	memcpy(buf + 104, B, 32);
	memcpy(buf + 136, K, 40);
	SHA1((const char *)buf, 176, out);

	nls->M1 = malloc(20);
	if (nls->M1)
		memcpy(nls->M1, out, 20);
}


int SRPCheckM2(LPNLS nls, const char *var_M2, const char *B, const char *salt) {
    char local_M2[20], *A, S[32], *K, *M1, buf[176];
    unsigned char username_hash[20];
	int res, mustFree = 0;

	if (!nls)
		return 0;

	if (nls->M2)
		return (memcmp(nls->M2, var_M2, 20) == 0);

	if (nls->A && nls->K && nls->M1) {
		A = nls->A;
		K = nls->K;
		M1 = nls->M1;
	} else {
		if (!B || !salt)
			return 0;

		A = malloc(32);
		if (!A)
			return 0;
		K = malloc(40);
		if (!K) {
			free(A);
			return 0;
		}
		M1 = malloc(20);
		if (!M1) {
			free(K);
			free(A);
			return 0;
		}

		mustFree = 1;

		SRPGetA(nls, A);
		SRPGetS(nls, S, (char *)B, (char *)salt);
		SRPGetK(nls, K, S);

	    SHA1(nls->user, nls->userlen, (char *)username_hash);

		memcpy(buf, NLS_I, 20);
		memcpy(buf + 20, username_hash, 20);
		memcpy(buf + 40, salt, 32);
		memcpy(buf + 72, A, 32);
		memcpy(buf + 104, B, 32);
		memcpy(buf + 136, K, 40);
		SHA1(buf, 176, M1);
	}
    
	memcpy(buf, A, 32);
	memcpy(buf + 32, M1, 20);
	memcpy(buf + 52, K, 40);
	SHA1(buf, 92, local_M2);

	res = (memcmp(local_M2, var_M2, 20) == 0);

	if (mustFree) {
		free(A);
		free(K);
		free(M1);
	}

	nls->M2 = malloc(20);
	if (nls->M2)
		memcpy(nls->M2, local_M2, 20);
    
    return res;
}


int SRPCheckSignature(uint32_t address, const char *signature_raw) {
    char *result_raw, check[32];
    mpz_t result, modulus, signature;
	size_t size, alloc_size;
	int cmp_result;

	memcpy(check, &address, 4);
	memset(check + 4, 0xBB, 28); 
    mpz_init2(modulus, 1024);
    mpz_import(modulus, 128, -1, 1, 0, 0, NLS_N); 
    mpz_init2(signature, 1024);
    mpz_import(signature, 128, -1, 1, 0, 0, signature_raw);
    mpz_init2(result, 1024);
    mpz_powm_ui(result, signature, NLS_SIGNATURE_KEY, modulus);
    mpz_clear(signature);
    mpz_clear(modulus);

	alloc_size = mpz_size(result) * sizeof(mp_limb_t);
	result_raw = malloc(alloc_size);
	if (!result_raw) {
		mpz_clear(result);
		return 0;
	}

    mpz_export(result_raw, &size, -1, 1, 0, 0, result);
    mpz_clear(result);
    cmp_result = (memcmp(result_raw, check, 32) == 0);
	free(result_raw);
	return cmp_result;
}


void SRPGetX(LPNLS nls, mpz_t x_c, const char *raw_salt) {
    unsigned char hash[20], buf[52];

	memcpy(buf, raw_salt, 32);
	memcpy(buf + 32, nls->userpasshash, 20);
    SHA1((const char *)buf, 52, (char *)hash);

    mpz_init2(x_c, 160);
    mpz_import(x_c, 20, -1, 1, 0, 0, (char *)hash);
}


void SRPGetV_mpz(LPNLS nls, mpz_t v, mpz_t x) {
    mpz_t g;

    mpz_init_set_ui(g, NLS_VAR_g);
    mpz_init(v);
    mpz_powm(v, g, x, nls->n);
    mpz_clear(g);
}


uint32_t SRPGetU(const char *B) {
    unsigned char hash[20];
    uint32_t u;
    
    SHA1(B, 32, (char *)hash);
    u = *(uint32_t *)hash;
    u = SWAP4(u);
    return u;
}

