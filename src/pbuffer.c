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
 * pbuffer.c - 
 *    Centralized buffer for abstracting and simplifying packet construction/sending
 */


#include "main.h"
#include "pbuffer.h"

unsigned char sendbuffer[SEND_BUFFER_LEN];
unsigned int pbufferlen = 4;


///////////////////////////////////////////////////////////////////////////////


void InsertByte(unsigned char data) {
	*(sendbuffer + pbufferlen) = data;
	pbufferlen++;
}


void InsertWORD(uint16_t data) {
	#ifdef NEEDS_ALIGNMENT
		memcpy(sendbuffer + pbufferlen, &data, sizeof(data));
	#else
		*(uint16_t *)(sendbuffer + pbufferlen) = data;
	#endif
	pbufferlen += 2;
}


void InsertDWORD(uint32_t data) {
	#ifdef NEEDS_ALIGNMENT
		memcpy(sendbuffer + pbufferlen, &data, sizeof(data));
	#else
		*(uint32_t *)(sendbuffer + pbufferlen) = data;
	#endif
	pbufferlen += 4;
}


void InsertNTString(char *data) {
	int datalen = strlen(data);
	strcpy((char *)sendbuffer + pbufferlen, data);
	pbufferlen += datalen;
	sendbuffer[pbufferlen] = 0;
	pbufferlen++;
}


void InsertNonNTString(char *data) {
	strcpy((char *)sendbuffer + pbufferlen, data);
	pbufferlen += strlen(data);
}


void InsertVoid(void *data, int len) {
	memcpy(sendbuffer + pbufferlen, data, len);
	pbufferlen += len;
}


void SendPacket(unsigned char PacketID, int index) {
	sendbuffer[0] = 0xFF;
	sendbuffer[1] = PacketID;
	*(uint16_t *)(sendbuffer + 2) = (uint16_t)pbufferlen;

	send(bot[index]->sck, sendbuffer, pbufferlen, 0);
	if (!bot[index]->connected)
		printf("[BNET] Sending 0x%02x...\n", PacketID);

	pbufferlen = 4;
}


void SendBNLSPacket(unsigned char PacketID) {
	pbufferlen--;
	#ifdef NEEDS_ALIGNMENT
		sendbuffer[1] = pbufferlen & 0xFF;
		sendbuffer[2] = (pbufferlen >> 8) & 0xFF;
	#else
		*(uint16_t *)(sendbuffer + 1) = (uint16_t)pbufferlen;
	#endif
	sendbuffer[3] = PacketID;

	send(sBNLS, sendbuffer + 1, pbufferlen, 0);
	printf("[BNLS] Sending 0x%02x...\n", PacketID);

	pbufferlen = 4;
}

