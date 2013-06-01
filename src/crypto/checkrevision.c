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
 * checkrevision.c - 
 *    Implementation of Blizzard's CheckRevision algorithm used to verify version and integrity of files
 */
 
#include "../main.h"
#include "checkrevision.h"

#define BUCR_GETNUM(ch) (((ch) == 'S') ? 3 : ((ch) - 'A'))
#define BUCR_ISNUM(ch) (((ch) >= '0') && ((ch) <= '9'))

uint32_t checksumseeds[] = {
	0xE7F4CB62,
	0xF6A14FFC,
	0xAA5504AF,
	0x871FCDC2,
	0x11BF6A18,
	0xC57292E6,						  
	0x7927D27E,
	0x2FEC8733
};

const char *crerrstrs[] = {
	NULL,
	"MPQ Number out of range",
	"Unknown variable",
	"More than 4 operations",
	"Failed to open file",
	"File mapping failure",
	"Unknown operation"
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int CheckRevision(const char *formula, const char *files[], int numFiles, int mpqNumber, uint32_t *checksum) {
	uint32_t values[4], ovd[4], ovs1[4], ovs2[4];
	char ops[4];
	const char *token;
	int curFormula, i, k, variable;
	unsigned char *file_buffer, *pad_dest, pad;
	uint32_t *dwBuf, *current;
	unsigned int file_len, remainder, rounded_size, buffer_size, j;
	#ifdef _WIN32
		HANDLE hFile;
		HANDLE hFileMapping;
	#else
		FILE *f;
	#endif


	if (mpqNumber > 7 || mpqNumber < 0) 
		return 1;
	
	curFormula = 0;
	token = formula;
	while (token && *token) {
		if (*(token + 1) == '=') {
			variable = BUCR_GETNUM(*token);
			if (variable < 0 || variable > 3)
				return 2;
			token += 2;
			if (BUCR_ISNUM(*token)) {
				values[variable] = strtoul(token, NULL, 10);
			} else {
				if (curFormula > 3)
					return 3;
				ovd[curFormula] = variable;
				ovs1[curFormula] = BUCR_GETNUM(*token);
				ops[curFormula] = *(token + 1);
				ovs2[curFormula] = BUCR_GETNUM(*(token + 2));
				curFormula++;
			}
		}
		
		for (; *token != 0; token++) {
			if (*token == ' ') {
				token++;
				break;
			}
		}
	}
	
	values[0] ^= checksumseeds[mpqNumber];
	
	for (i = 0; i < numFiles; i++) {
		#ifdef _WIN32
			hFile = CreateFile(files[i], GENERIC_READ, FILE_SHARE_READ,
				NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (!hFile)
				return 4;
			file_len = GetFileSize(hFile, NULL);
			hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
			if (!hFileMapping)
				return 5;
			file_buffer = (unsigned char *)MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
		#else
			f = fopen(files[i], "rb");
			if (!f)
				return 4;
			fseek(f, 0, SEEK_END);
			file_len = ftell(f);
			rewind(f);
			file_buffer = (unsigned char *)mmap(NULL, file_len, PROT_READ, MAP_SHARED, fileno(f), 0);
		#endif

		if (!file_buffer) {
			#ifdef _WIN32
				CloseHandle(hFile);
			#else
				fclose(f);
			#endif
			return 5;
		}

		remainder = file_len & 0x3FF;
		rounded_size = file_len - remainder;
		if (!remainder) {
			dwBuf = (uint32_t *)file_buffer;
			buffer_size = file_len;
		} else {
			size_t extra = 1024 - remainder;
			pad = (unsigned char)0xFF;
			
			buffer_size = file_len + extra;
			dwBuf = malloc(buffer_size);
			
			memcpy(dwBuf, file_buffer, file_len);
			
			#ifdef _WIN32
				UnmapViewOfFile(file_buffer);
				CloseHandle(hFileMapping);
			#else
				munmap(file_buffer, file_len);
			#endif
			file_buffer = NULL;
			
			pad_dest = ((unsigned char *)dwBuf) + file_len;
			for (j = file_len; j < buffer_size; j++)
				*pad_dest++ = pad--;
		}

		current = dwBuf;
		for (j = 0; j < (buffer_size / sizeof(uint32_t)); j++) {
			values[3] = current[j];
			for (k = 0; k < curFormula; k++) {
				switch (ops[k]) {
					case '+':
						values[ovd[k]] = values[ovs1[k]] + values[ovs2[k]];
						break;
					case '-':
						values[ovd[k]] = values[ovs1[k]] - values[ovs2[k]];
						break;
					case '^':
						values[ovd[k]] = values[ovs1[k]] ^ values[ovs2[k]];
						break;
					default:
						#ifdef _WIN32
							UnmapViewOfFile(dwBuf);
							CloseHandle(hFileMapping);
							CloseHandle(hFile);
						#else
							munmap(dwBuf, file_len);
							fclose(f);
						#endif
						return 6;
				}
			}
		}

		if (file_buffer) {
			#ifdef _WIN32
				UnmapViewOfFile(file_buffer);
				CloseHandle(hFileMapping);
			#else
				munmap(file_buffer, file_len);
			#endif
		} else if (dwBuf && !file_buffer) {
			free(dwBuf);
		}
		#ifdef _WIN32
			CloseHandle(hFile);
		#else
			fclose(f);
		#endif
	}

	*checksum = values[2];
	return 0;
}

