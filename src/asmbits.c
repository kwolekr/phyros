/*-
 * Copyright (c) 2010 Ryan Kwolek
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
 * asmbits.c - 
 *    Processor architecture abstraction layer containing bits of arch-specific assembly code
 */

#include "main.h"
#include "asmbits.h"

/************************
Supported architectures:
//x86, x64
//ia64
//sparc, sparc64
//PowerPC
//PA-RISC
//MIPS
//Alpha
//68k

************************/

///////////////////////////////////////////////////////////////////////////////

void AtomicWriteData(void *address, unsigned int data) {
	//unsigned int lol = data;
	

	#ifdef _WIN32
		__asm { 
			mov edx, data
			lock xchg address, edx
			//mov lol, edx
		}
	#else
		#if defined(__x86__)
			__asm__ (
				".intel_syntax noprefix\n"
				"mov edx, %0\n"
				"lock xchg %1, edx\n"
				".att_syntax\n"
				: "m" (address)
				: "m" (data)
				: "edx", "memory");
		#elif defined(__sparc__)
			__asm__ (
				"casxa\n"
			);
			//swap
			//casxa
		#elif defined(__powerpc__)
			//retry:
			//lwarx --atomically read
			//stwcx --atomically store
			//bne- retry
		//#elif defined(__alpha__)
		#else
			*(unsigned int *)address = data;
		#endif
	#endif
}


void fastswap16(uint16_t *num) {
	#ifdef _WIN32
		__asm {
			mov ecx, dword ptr [num]
			mov eax, dword ptr [ecx]
			xchg al, ah
			mov dword ptr [ecx], eax
		}
	#else 					  
		#if defined(__x86__)
			asm(".intel_syntax noprefix\n");
			asm("mov ecx, %0\n\t" : : "m"(num));
			asm("mov eax, dword ptr [ecx]\n\t");
			asm("xchg al, ah\n\t");
			asm("mov dword ptr [ecx], eax\n\t" : : : "ecx", "memory");
			asm(".att_syntax\n");
		#elif defined(__hppa__)
			asm("dep %0, 15, 8, %0\n\t"); //FIXME
			asm("shd %%r0, %0, 8, %0" "=r" (x) : "" (x));
		#else
			*num = SWAP2(*num);
		#endif
	#endif
}


void fastswap32(uint32_t *num) {									
	#ifdef _WIN32
		__asm {
			mov ecx, dword ptr [num]
			mov eax, dword ptr [ecx]
			bswap eax
			mov dword ptr [ecx], eax
		}
	#else
		#if defined(__x86__)
			asm(".intel_syntax noprefix\n");
			asm("mov ecx, %0\n\t" : : "m"(num));
			asm("mov eax, dword [ecx]\n\t");
			asm("bswap eax\n\t");
			asm("mov dword ptr [ecx], eax\n\t" : : : "ecx", "memory");
			asm(".att_syntax\n");
		#elif defined(__hppa__)
			asm("shd %0, %0, 8, %0\n\t"); 
			asm("dep %0, 15, 8, %0\n\t");   //FIXME: doesn't have the desired effect!
			asm("shd %%r0, %0, 8, %0" : "=r" (x) : "" (x));
		#else
			*num = SWAP4(*num);
		#endif
	#endif
}

