/*-
 * Copyright (c) 2008 Ryan Kwolek
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
 * warden.c - 
 *    Warden decoder, packet parsing, module loader/preparation/caller, crypto functions
 */


#include <zlib.h>
#include "main.h"
#include "pbuffer.h"
#include "hashtable.h"
#include "fxns.h"
#include "warden.h"

#define SWAP(a,b) ((a == b) ? (a=a) : (a ^= b ^= a ^= b))

#if !defined(__i386__) && !defined(_M_IX86)
	#error "Only i386 platforms supported for Warden module handling"
#endif


const char *wextracterrstrs[] = {
	"Module MD5 hash mismatch",
	"Module misdecryption, SIGN doesn't match",
	"zlib!uncompress() failed",
	"kernel32!CreateFile() failed",
	"kernel32!WriteFile() failed"
};

SNPFNTABLE snpFnTable = {
	WdnCbkSendPacket,
	WdnCbkCheckModule,
	WdnCbkLoadModule,
	WdnCbkMemAlloc,
	WdnCbkMemFree,
	WdnCbkSetRC4,
	WdnCbkGetRC4
};

void *lpWdnModClass;
LPWARDENFNTABLE lpWdnFnTable;

void *snpTable = &snpFnTable;

char modhash[16];
char modname[64];
char moddecryptkey[16];
int modsize;
char *rawmodule;
int moddlprog;
void *currentmodule;
uint32_t startedtick;
uint32_t wdnkeyseed;

char *newrc4;
char *pendingwpacket;
int pendingwpacketlen;

int dling;

void __stdcall foo0() {}
void __stdcall foo1(int p1) {}
void __stdcall foo2(int p1, int p2) {}
void __stdcall foo3(int p1, int p2, int p3) {}
void __stdcall foo4(int p1, int p2, int p3, int p4) {}
void __stdcall foo5(int p1, int p2, int p3, int p4, int p5) {}
void __stdcall foo6(int p1, int p2, int p3, int p4, int p5, int p6) {}
void __stdcall foo7(int p1, int p2, int p3, int p4, int p5, int p6, int p7) {}
void __stdcall foo8(int p1, int p2, int p3, int p4, int p5, int p6, int p7, int p8) {}

typedef void (__stdcall *cbks)();

cbks callbacks[] = {
	foo0, foo1, foo2, foo3,
	foo4, foo5, foo6, foo7, foo8
};


///////////////////////////////////////////////////////////////////////////////


void Parse0x5E(char *data, int index) {
	//char buf[128];
	unsigned int len = *(uint16_t *)(data + 2) - 4;
	data += 4;
	RC4Crypt(bot[index]->warden.keyin, (unsigned char *)data, len);
	switch (data[0]) {
		case 0:
			WardenParseCommand0(data, len, index);
			break;
		case 1:
			WardenParseCommand1(data, len, index);
			break;
		case 2:
			WardenParseCommand2(data, len, index);
			break;
		case 5:
			WardenParseCommand5(data, len, index);
			break;
		default:
			printf("Unhandled warden command 0x%02x!", (unsigned char)*data);
	}
}


///////////////////////////////////////////[Command Handlers]//////////////////////////////////////////


void WardenParseCommand0(char *data, int len, int index) {
	int i;
	unsigned char outbuf;

	*(int32_t *)(modname) = 'udom';
	*(int32_t *)(modname + 4) = '\\sel';
	for (i = 0; i != 16; i++)
		sprintf(modname + 8 + (i << 1), "%02x", (unsigned char)data[i + 1]);
	modname[40] = '.';
	*(int32_t *)(modname + 41) = 'dom';

	printf("Warden module %s requested.\n", modname + 8);
	if (currentmodule) {
		if (!memcmp(modhash, data + 1, 16)) {
			printf("Module already loaded, using that.\n");
			goto send1;
		} else {
			WardenUnloadModule();
			printf("New warden module requested, loading that.\n");
			goto contwprocessing;
		}
	} else {
contwprocessing:
		*(int32_t *)(modhash)        = *(int32_t *)(data + 1);
		*(int32_t *)(modhash + 0x04) = *(int32_t *)(data + 5);
		*(int32_t *)(modhash + 0x08) = *(int32_t *)(data + 9);
		*(int32_t *)(modhash + 0x0C) = *(int32_t *)(data + 13);

		*(int32_t *)(moddecryptkey)	       = *(int32_t *)(data + 17);
		*(int32_t *)(moddecryptkey + 0x04) = *(int32_t *)(data + 21);
		*(int32_t *)(moddecryptkey + 0x08) = *(int32_t *)(data + 25);
		*(int32_t *)(moddecryptkey + 0x0C) = *(int32_t *)(data + 29);

		modsize = *(int32_t *)(data + 33);

		if (!IsFilePresent(modname)) {
			if (dling) {
				printf("Module already being downloaded, waiting!\n");
				goto send1;
			}
			printf("Module not found, downloading...\n");
			rawmodule = malloc(modsize);
			moddlprog = 0;
			outbuf = 0;
			startedtick = gettick();
			dling = 1;
		} else {
			WardenPrepareModule(modname);
			WardenModuleInit((LPWMODHEADER)currentmodule);
send1:
			outbuf = 1;
		}
	}
	RC4Crypt(bot[index]->warden.keyout, &outbuf, 1);
	InsertByte(outbuf);
	SendPacket(0x5E, index);
}


void WardenParseCommand1(char *data, int len, int index) {
	char buf[128];
	int fail;
	if (rawmodule) {
		memcpy(rawmodule + moddlprog, data + 3, *(int16_t *)(data + 1));
		moddlprog += *(int16_t *)(data + 1);
		//printf("%d/%d bytes (%d%%)\n", moddlprog, modsize,
		//       (int)((float)moddlprog / (float)modsize * 100.f));
	}
	if (moddlprog >= modsize) {
		dling = 0;
		printf("Module downloaded, dl @ %d kB/s\n",
			(int)((float)(modsize) / (float)(gettick() - startedtick)));
		fail = WardenDecryptInflateModule(modname, rawmodule, modsize, moddecryptkey, index);
		if (fail) {
			printf("Warden module decryption/inflation failure %d (%s).\n",
				fail, wextracterrstrs[fail - 1]);
		} else {
			WardenPrepareModule(modname);
			WardenModuleInit((LPWMODHEADER)currentmodule);
			*buf = 1;
			RC4Crypt(bot[index]->warden.keyout, (unsigned char *)buf, 1);
			InsertByte(*buf);
			SendPacket(0x5E, index);
		}
	}
}


void *GetRealAddress(void *addr, char **mods, int modindex, int nummods) {
	/* lpModuleImage[]
		0 - war3.exe
		1 - storm.dll
		2 - game.dll */
	int wdnmodsnum;
	if (modindex > nummods)
		return 0;
	if (modindex) {	//module
		switch (*(int32_t *)mods[modindex - 1]) {
			case 'emag': //game.dll
				wdnmodsnum = 2;
				break;
			case 'rots': //storm.dll
				wdnmodsnum = 1;
				break;
			default:
				return 0;
		}
	} else { //absolute addr
		if ((int)addr >= 0x400000 && (int)addr <= 0x600000)	{
			wdnmodsnum = 0;
			addr = (void *)((int)addr - 0x400000);
		} else {
			return 0;
		}
	}
	if (!lpModuleImage[wdnmodsnum]) {
		FILE *file;
		long filesize;
		printf("Loading %s...\n", hashes[wdnmodsnum]);
		file = fopen(hashes[wdnmodsnum], "rb");
		if (!file) {
			printf("WARNING: cannot open %s for reading, errno: %d\n",
				hashes[wdnmodsnum], geterr);
			return 0;
		}
		fseek(file, 0, SEEK_END);
		filesize = ftell(file);
		rewind(file);
		lpModuleImage[wdnmodsnum] = malloc(filesize);
		fread((void *)lpModuleImage[wdnmodsnum], filesize, 1, file);
		fclose(file);
		if (*(int16_t *)lpModuleImage[wdnmodsnum] != 'ZM') {
			printf("PE file header mismatch!\n");
			return 0;
		}
	}
	return lpModuleImage[wdnmodsnum] + (int)addr;
}


void WardenParseCommand2(char *data, int len, int index) {
	char buf[512];
	char *tosend = buf + 7;
	char *mods[8];
	void *address;
	int pos = 1, nummods = 0, length, lentoread;
	memset(buf, 0, sizeof(buf));
	//StrToHexOut(data, len, hWnd_status_re);
	while (data[pos]) {
		mods[nummods] = malloc(data[pos] + 1);
		memcpy(mods[nummods], data + pos + 1, data[pos]);
		*(mods[nummods] + data[pos]) = 0;
		nummods++;
		pos = pos + data[pos] + 1;
		if (nummods == 8)
			break;
	}
	pos++;
	while (pos + 1 < len) {
		//data[i] ^= data[len - 1];
		if (pos >= 256) {
			printf("WARNING! WARDEN BUFFER OVER THRESHHOLD!\n");
			break;
		}
		pos++; //skip command id
		if ((unsigned char)data[pos] <= nummods && !data[pos + 4]) { //memcheck
			address = GetRealAddress(*(void **)(data + pos + 1),
				mods, data[pos], nummods);
			if (address) {
				lentoread = data[pos + 5];
				if (IsBadReadPtr(address, lentoread))
				    goto failcheck;
				*tosend++ = 0;
				memcpy(tosend, address, lentoread);
				tosend += lentoread;
			} else {
failcheck:
				printf("[%d] Unreadable memcheck, addr 0x%p, len %d. at pos %d\n!",
					index, address, lentoread, pos);
				StrToHexOut(data, len);
				goto pagecheck;
			}
			pos += 6;
		} else { //pagecheck
pagecheck:
			*tosend = (char)0xE9; //0;
			tosend++;
			pos += 29;
		}
	}
	length = tosend - buf;
	*buf = 2;
	*(int16_t *)(buf + 1) = length - 7;
	*(int32_t *)(buf + 3) = WardenGenerateChecksum(buf + 7, length - 7);
	RC4Crypt(bot[index]->warden.keyout, (unsigned char *)buf, length);
	InsertVoid(buf, length);
	SendPacket(0x5E, index);
	while (nummods--)
		free(mods[nummods]);
}


void WardenParseCommand5(char *data, int len, int index) {
	int buf32;
	char *tmpdata;
	unsigned char tmpkey[0x102];
	if (!lpWdnFnTable || !lpWdnFnTable->wdnGenKeys) {
		printf("Unable to generate new RC4 keys (wdnGenKeys == NULL)!\n");
		return;
	}
	newrc4 = NULL;
#ifdef _WIN32
	__asm {
		push 4
		push offset wdnkeyseed
		mov ecx, lpWdnModClass
		mov eax, lpWdnFnTable
		call dword ptr [eax]
		//call [eax]WARDENFNTABLE.wdnGenKeys
	}
#else
    asm(".intel_syntax noprefix\n");
    asm("push 4\n");
    asm("push offset wdnkeyseed\n");
    asm("mov ecx, lpWdnModClass\n");
    asm("mov eax, lpWdnFnTable\n");
    asm("call dword ptr [eax]\n");
    asm(".att_syntax\n");
#endif
	if (!newrc4) {
		printf("wdnGenKeys() failed!\n");
		return;
	}
	tmpdata = malloc(len);
	memcpy(tmpdata, data, len);
	memcpy(tmpkey, newrc4 + 0x102, sizeof(tmpkey));
	RC4Crypt(tmpkey, (unsigned char *)tmpdata, len);
	memcpy(tmpkey, newrc4, sizeof(tmpkey));
	if (pendingwpacket)	{
		free(pendingwpacket);
		pendingwpacket = NULL;
	}

#ifdef _WIN32
	__asm {
		lea eax, [buf32]
		push eax
		push len
		push tmpdata
		mov ecx, lpWdnModClass
		mov eax, lpWdnFnTable
		call dword ptr [eax + 8]
		//call [eax]WARDENFNTABLE.wdnHandlePacket
	}
#else
    asm(".intel_syntax noprefix\n");
    asm("lea eax, %0\n" : : "m"(buf32));
    asm("push eax\n");
    asm("push %0\n" : : "m"(len));
    asm("push %0\n" : : "m"(tmpdata));
    asm("mov ecx, lpWdnModClass\n");
    asm("mov eax, lpWdnFnTable\n");
    asm("call dword ptr [eax + 8]\n");
    asm(".att_syntax\n");
#endif

	if (!pendingwpacket) {
		printf("wdnSendPacket() failed!\n");
		return;
	}
	RC4Crypt(tmpkey, (unsigned char *)pendingwpacket, pendingwpacketlen);
	RC4Crypt((unsigned char *)bot[index]->warden.keyout, (unsigned char *)pendingwpacket, pendingwpacketlen);
	memcpy(bot[index]->warden.keyout, newrc4, 0x102);
	memcpy(bot[index]->warden.keyin, (char *)newrc4 + 0x102, 0x102);
	InsertVoid(pendingwpacket, pendingwpacketlen);
	SendPacket(0x5E, index);
}


/////////////////////////////////[Warden Module Preparation]///////////////////////////////////////////


int WardenDecryptInflateModule(char *modulename, char *module, int modulelen, char *keyseed, int index) {
	unsigned char cryptkey[0x102];
	unsigned long extlen;
	char *extmodule;
	FILE *file;

	MD5((char *)module, modulelen, (char *)cryptkey);
	if (memcmp(modhash, cryptkey, 16))
		return 1;

	WardenKeyGenerate(cryptkey, (unsigned char *)keyseed, 0x10);
	RC4Crypt(cryptkey, (unsigned char *)module, modulelen);
	if (*(int32_t *)(module + modulelen - 0x104) != 'SIGN')
		return 2;

	extlen = *(uint32_t *)module;
	extmodule = malloc(extlen);
	if (uncompress((unsigned char *)extmodule, &extlen, (unsigned char *)module + 4, modulelen - 4)) {
		free(extmodule);
		return 3;
	}

	file = fopen(modulename, "wb");
	if (!file) {
		free(extmodule);
		return 4;
	}

    if (fwrite(extmodule, extlen, 1, file) != 1) {
		free(extmodule);
		fclose(file);
		return 5;
	}

	free(extmodule);
	fclose(file);
	return 0;
}


int WardenPrepareModule(char *filename) {
	uint32_t srcloc, destloc, procstart, procoffset, oldprotect, filelen;
	int i, sections, sectionsize, fnnameoffset, tmp, numparams, j;
	char *tmpmod, *lmodule, *libtoload, *fnstr, asdf[32];
	LPWMODHEADER lpwmodh;
	LPWMODPROTECT lpwmodp;

    FILE *file = fopen(filename, "rb");
    if (!file)
        return FALSE;

	fseek(file, 0, SEEK_END);
    filelen = ftell(file);
	rewind(file);
    tmpmod = malloc(filelen);
    fread(tmpmod, filelen, 1, file);
    fclose(file);

	lpwmodh = (LPWMODHEADER)tmpmod;

	#ifdef _WIN32
		lmodule = (char *)VirtualAlloc(0, lpwmodh->cbModSize, MEM_COMMIT, PAGE_READWRITE);
	#else
		lmodule = mmap(0, lpwmodh->cbModSize, PROT_READ | PROT_WRITE, 0, 0, 0);
	#endif

	memcpy(lmodule, tmpmod, 40);

	srcloc   = lpwmodh->sectioncount * 12 + 0x28;
	destloc  = lpwmodh->sectionlen;
	sections = 0;
	while (destloc < lpwmodh->cbModSize) {
		sectionsize = *(uint16_t *)(tmpmod + srcloc);
		srcloc += 2;
		sections++;
		if (sections & 1) {
			memcpy(lmodule + destloc, tmpmod + srcloc, sectionsize);
			srcloc += sectionsize;
		}

		destloc += sectionsize;
	}

	srcloc  = lpwmodh->rvaFxnTableReloc;
	destloc = 0;
	for (i = 0; i != lpwmodh->reloccount; i++) {
		if (lmodule[srcloc] < 0) {
			destloc = ((lmodule[srcloc + 0] & 0x07F) << 24) |
					  ((lmodule[srcloc + 1] & 0x0FF) << 16) |
					  ((lmodule[srcloc + 2] & 0x0FF) << 8)  |
					  ((lmodule[srcloc + 3] & 0x0FF));
			srcloc += 4;
			printf("[WARDEN] module is using an absolute address!!!\n");
		} else {
			destloc += (lmodule[srcloc + 1] & 0x0FF) + (lmodule[srcloc] << 8);
			srcloc += 2;
		}
		*(unsigned int *)(lmodule + destloc) += (unsigned int)lmodule;
	}

	for (i = 0; i != lpwmodh->dllcount; i++) {
		procstart  = lpwmodh->rvaImports + i * 8;
		procoffset = *(unsigned int *)(lmodule + procstart + 4);
		libtoload = lmodule + *(unsigned int *)(lmodule + procstart);

		j = 0;
		while (*libtoload && *libtoload != '.' && j < 27) {
			asdf[j] = tolower((unsigned char)*libtoload);
			libtoload++;
			j++;
		}
		*(int32_t *)(asdf + j) = 'mrp.';
		asdf[j + 4] = 0;

		file = fopen(asdf, "rb");
		if (file) {
			fnnameoffset = *(unsigned int *)(lmodule + procoffset);
			while (fnnameoffset) {
				fnnameoffset = *(unsigned int *)(lmodule + procoffset);
				if (fnnameoffset > 0) {
					fnstr = lmodule + fnnameoffset;
					if (!strcmp(fnstr, "GetProcAddress")) {
						tmp = (int)MyGetProcAddress;
					} else {
						numparams = LookupApiParamNum(file, fnstr);
						if (numparams != -1)
							tmp = (int)callbacks[numparams];
						else
							printf("WARNING: %s not found in api param file %s\n", fnstr, asdf);
					}
				} else {
					tmp &= ~0x80000000;
				}
				*(unsigned int *)(lmodule + procoffset) = tmp;
				procoffset += 4;
			}
			fclose(file);
		} else {
			printf("WARNING: cannot open %s for reading, errno: %d", asdf, geterr);
		}
	}

	for (i = 0; i != lpwmodh->sectioncount; i++) {
		lpwmodp = (LPWMODPROTECT)(lmodule + lpwmodh->sectioncount * 12 + 0x28);
		lpwmodp->base = lmodule + (int)lpwmodp->base;

#ifdef _WIN32
		VirtualProtect(lpwmodp->base, lpwmodp->len, lpwmodp->protectdword, &oldprotect);
		if (oldprotect & 0xF0)
			FlushInstructionCache(GetCurrentProcess(), lpwmodp->base, lpwmodp->len);
#else
        //TODO: TRANSLATE FROM WIN32 PROTECTION CONSTANTS!!!
        mprotect(lpwmodp->base, lpwmodp->len, PROT_READ | PROT_WRITE | PROT_EXEC);
        //flush_icache_range(lpwmodp->base, lpwmodp->base + lpwmodp->len); //broken on FreeBSD, OpenBSD, NetBSD, Mac OSx
#endif

	}
	if (lpwmodh->cbModSize < lpwmodh->rvaFxnTableReloc)
		return FALSE;

#ifdef _WIN32
	VirtualFree(lmodule + lpwmodh->rvaFxnTableReloc,
		lpwmodh->cbModSize - lpwmodh->rvaFxnTableReloc, MEM_RELEASE);
#endif


	currentmodule = (void *)lmodule;

	return TRUE;
}


void WardenModuleInit(LPWMODHEADER lpwmodh) {
	unsigned int lpfnInit;
	int tmp = 1 - lpwmodh->unk2;
	if (tmp > (int)lpwmodh->unk1)
		return;
	lpfnInit = (unsigned int)lpwmodh + *(unsigned int *)((int)lpwmodh + (int)lpwmodh->lpfnInit + (tmp << 2));
#ifdef _WIN32
	__asm {
		mov ecx, offset snpTable
		call lpfnInit
		mov lpWdnModClass, eax
		mov eax, dword ptr [eax]
		mov lpWdnFnTable, eax
	}
#else
    __asm__ (
        ".intel_syntax noprefix\n"
        "mov ecx, offset snpTable\n"
        "mov eax, %0\n"
        "call eax\n"
        "mov lpWdnModClass, eax\n"
        "mov eax, dword [eax]\n"
        "mov lpWdnFnTable, eax\n"
        ".att_syntax\n"
        :
        : "m" (lpfnInit),
        [snpTable] "rm" (snpTable),
        [lpWdnModClass] "rm" (lpWdnModClass),
        [lpWdnFnTable] "rm" (lpWdnFnTable)
        : "eax", "ecx", "memory"
    );
#endif
}


void WardenUnloadModule() {
#ifdef _WIN32
	__asm {
		mov ecx, lpWdnModClass
		mov eax, lpWdnFnTable
		call dword ptr [eax + 4]
		//call [eax]WARDENFNTABLE.wdnUnloadModule
	}
	VirtualFree(currentmodule, 0, MEM_RELEASE);
#else
    __asm__ (
      ".intel_syntax noprefix\n"
      "mov ecx, lpWdnModClass\n"
      "mov eax, lpWdnFnTable\n"
      "call dword [eax + 4]\n"
      ".att_syntax\n"
      :
      : [lpWdnModClass] "rm" (lpWdnModClass),
      [lpWdnFnTable] "rm" (lpWdnFnTable)
      : "eax", "ecx", "memory"
    );
    //asm("mov ecx, lpWdnModClass\n");
    //asm("mov eax, lpwdnFnTable\n");
    //asm("call dword ptr [eax + 4]\n");
    munmap(currentmodule, ((LPWMODHEADER)currentmodule)->cbModSize);
#endif
	currentmodule = NULL;
}


void __stdcall nullsub(int p1, int p2) {

}


void __stdcall nullsub2(int p1) {

}


/**************************************************
 *                PRM File Format                 *
 **************************************************
 * (uint32_t[]) Hashtable, offset to entry table  *
 * for each table item:	 					      *
 *  [uint32_t] length of entire block			  *
 *  [byte] number of unique entries			      *
 *  for each unique entry in the table item:      *
 *    [byte] number of params				      *
 *    [ntstr] fn name (key)                       *
 **************************************************/


int LookupApiParamNum(FILE *file, char *fnname) {
	char *blah, *curpos;
	int offset, totallen, numentries, i, retval;
	unsigned int index = hash((unsigned char *)fnname) & 0x3FF;

	fseek(file, index * sizeof(uint32_t), SEEK_SET);
	fread(&offset, sizeof(uint32_t), 1, file);
	if (!offset)
		return -1;
	fseek(file, offset, SEEK_SET);
	fread(&totallen, sizeof(uint32_t), 1, file);
	fread(&numentries, sizeof(uint32_t), 1, file);
	blah = malloc(totallen - sizeof(uint32_t) * 2);
	fread(blah, totallen - sizeof(uint32_t) * 2, 1, file);
	curpos = blah;
	for (i = 0; i != numentries; i++) {
		if (!strcmp(fnname, curpos + 1)) {
			retval = *curpos;
			free(blah);
			return retval;
		}
		curpos += strlen(curpos + 1) + 2;

	}
	free(blah);
	return -1;
}


void *__stdcall MyGetProcAddress(int hModule, char *lpProcName) {
	return NULL; //should make this better
}


/////////////////////////////////////[Warden Callbacks]////////////////////////////////////////////////


void __stdcall WdnCbkSendPacket(char *data, int len) {
	//char asdf[128];
	pendingwpacket = malloc(len);
	memcpy(pendingwpacket, data, len);
	pendingwpacketlen = len;
	printf("[WARDEN] module wants to send packet, %d bytes\n", len);
}


int __stdcall WdnCbkCheckModule(char *modname, uint32_t _2) {
	printf("[WARDEN] module wants to check %s. _2: 0x%08x!\n", modname, _2);
	return TRUE;
}


uint32_t __stdcall WdnCbkLoadModule(char *decryptkey, char *module, int modlen) {
	printf("[WARDEN] module wants to load a new module, %d bytes.\n", modlen);
	return 0;
}


void *__stdcall WdnCbkMemAlloc(uint32_t len) {
	void *blah = malloc(len);
	printf("[WARDEN] module allocated %d bytes at 0x%p.\n", len, blah);
	return blah;
}


void __stdcall WdnCbkMemFree(void *mem) {
    if (!mem)
        return;
	free(mem);
	printf("[WARDEN] module freed 0x%p.\n", mem);
}


void __stdcall WdnCbkSetRC4(void *keys, uint32_t len) {
	printf("[WARDEN] module set RC4 keys at 0x%p, %d bytes.\n", keys, len);
}


char *__stdcall WdnCbkGetRC4(char *buffer, uint32_t *len) {
	char *oldrc4;
	printf("[WARDEN] module got RC4 keys at 0x%p, %d bytes.\n", buffer, *len);
	oldrc4 = newrc4;
	newrc4 = buffer;
	return oldrc4;
}


///////////////////////////////////////[Warden Crypto]/////////////////////////////////////////////////


void RC4Crypt(unsigned char *key, unsigned char *data, int length) {
	int i;
	for (i = 0; i < length; i++) {
		key[0x100]++;
		key[0x101] += key[key[0x100]];
		SWAP(key[key[0x101]], key[key[0x100]]);
		data[i] = data[i] ^ key[(key[key[0x101]] + key[key[0x100]]) & 0xFF];
	}
}


void WardenKeyInit(char *KeyHash, int index) {
	char temp[0x10];
	wdnkeyseed = (uint32_t)KeyHash;
	WardenKeyDataInit(&bot[index]->warden.keygendata, KeyHash, 4);
	WardenKeyDataGetBytes(&bot[index]->warden.keygendata, temp, 16);
	WardenKeyGenerate((unsigned char *)bot[index]->warden.keyout, (unsigned char *)temp, 16);
	WardenKeyDataGetBytes(&bot[index]->warden.keygendata, temp, 16);
	WardenKeyGenerate((unsigned char *)bot[index]->warden.keyin, (unsigned char *)temp, 16);
}


void WardenKeyDataInit(LPWDNKEYGENDATA source, char *seed, int length) {
	int length1 = length >> 1;
	int length2 = length - length1;
	char *seed1 = seed;
	char *seed2 = seed + length1;
	memset(source, 0, sizeof(WDNKEYGENDATA));
	SHA1(seed1, length1, source->src1);
	SHA1(seed2, length2, source->src2);
	WardenKeyDataUpdate(source);
	source->curpos = 0;
}


void WardenKeyGenerate(unsigned char *key_buffer, unsigned char *base, unsigned int baselen) {
	unsigned char val = 0;
	unsigned int pos = 0;
	unsigned int i;
	for (i = 0; i < 0x100; i++)
		key_buffer[i] = i;
	key_buffer[0x100] = 0;
	key_buffer[0x101] = 0;
	for (i = 1; i <= 0x40; i++) {
		int tmp = i << 2;
		val += key_buffer[tmp - 4] + base[pos++ % baselen];
		SWAP(key_buffer[tmp - 4], key_buffer[val & 0xFF]);
		val += key_buffer[tmp - 3] + base[pos++ % baselen];
		SWAP(key_buffer[tmp - 3], key_buffer[val & 0xFF]);
		val += key_buffer[tmp - 2] + base[pos++ % baselen];
		SWAP(key_buffer[tmp - 2], key_buffer[val & 0xFF]);
		val += key_buffer[tmp - 1] + base[pos++ % baselen];
		SWAP(key_buffer[tmp - 1], key_buffer[val & 0xFF]);
	}
}


void WardenKeyDataUpdate(LPWDNKEYGENDATA source) {
	char data[60];
	memcpy(data, source->src1, 20);
	memcpy(data + 20, source->data, 20);
	memcpy(data + 40, source->src2, 20);
	SHA1(data, 60, source->data);
}


void WardenKeyDataGetBytes(LPWDNKEYGENDATA source, char *buffer, int bytes) {
	int i;
	for (i = 0; i < bytes; i++)
		buffer[i] = WardenKeyDataGetByte(source);
}


char WardenKeyDataGetByte(LPWDNKEYGENDATA source) {
	int i = source->curpos;
	char value = source->data[i];
	i++;
	if (i >= 20) {
		i = 0;
		WardenKeyDataUpdate(source);
	}
	source->curpos = i;
	return value;
}


uint32_t WardenGenerateChecksum(char *data, int len) {
	uint32_t result[5];

	SHA1(data, len, (char *)result);
	return result[0] ^ result[1] ^ result[2] ^ result[3] ^ result[4];
}

