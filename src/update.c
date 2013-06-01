/*-
 * Copyright (c) 2009 Ryan Kwolek
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
 * update.c - 
 *    Routines for updating and decrypting the bot from a trusted update provider via http
 */


#include "main.h"
#include "fxns.h"
#include "connection.h"
#include "warden.h"
#include "update.h"

const unsigned char updatekey[258] = { //just some 256 bytes from /dev/random, nothing special
	0x3d, 0x12, 0xca, 0xef, 0x77, 0xd9, 0x98, 0xd3, 0x6c, 0x37, 0xe6, 0x65, 0xa4, 0x1c, 0xff, 0x46,
	0xfd, 0xc4, 0xfb, 0x71, 0x5b, 0xca, 0x67, 0x37, 0x33, 0xf7, 0xec, 0x62, 0x5e, 0xed, 0xea, 0xfb,
	0x08, 0x05, 0x12, 0x6c, 0x55, 0xd7, 0xa3, 0x3f, 0x98, 0x99, 0x11, 0x36, 0xde, 0xa9, 0xc4, 0x33,
	0x4e, 0xb6, 0xee, 0x07, 0xc8, 0xa2, 0x17, 0xea, 0xfc, 0xca, 0xa1, 0x2c, 0xad, 0xc9, 0xb7, 0x31,
	0xa1, 0x96, 0x89, 0xdc, 0x80, 0xfc, 0x8a, 0x28, 0x00, 0x65, 0x07, 0xd5, 0x02, 0x43, 0xb9, 0x03,
	0xee, 0x72, 0x2a, 0xff, 0x3c, 0xbd, 0xdd, 0x11, 0x59, 0x84, 0x47, 0x47, 0x28, 0x03, 0x75, 0xdd,
	0xa7, 0xa1, 0xc6, 0x61, 0x22, 0x1c, 0xa0, 0xf0, 0x58, 0x10, 0xa6, 0x29, 0x03, 0x4d, 0x82, 0x23,
	0x91, 0x5f, 0x14, 0x4e, 0x23, 0xcd, 0xa5, 0x6e, 0x36, 0xa5, 0x36, 0xf5, 0xfd, 0x24, 0x19, 0x76,
	0xcf, 0x42, 0xcc, 0xfa, 0x2f, 0x1a, 0x70, 0xae, 0x64, 0x09, 0x8a, 0x28, 0x5d, 0xdb, 0xb5, 0xd7,
	0xaa, 0x5f, 0x5f, 0x78, 0xa8, 0x2f, 0xfe, 0x37, 0x4b, 0xce, 0xaa, 0x97, 0xe1, 0xf9, 0x2e, 0x3b,
	0xe5, 0xd0, 0x46, 0xa7, 0x75, 0x02, 0xe7, 0x36, 0x8f, 0xce, 0x3f, 0xf7, 0xb8, 0x1c, 0xba, 0xea,
	0x6f, 0x76, 0x2d, 0x83, 0xb1, 0xd2, 0x87, 0x23, 0x57, 0x26, 0x75, 0xe4, 0x12, 0x29, 0xf2, 0xe1,
	0xb9, 0xf4, 0xb9, 0x6f, 0xee, 0x9f, 0x19, 0xd8, 0x8f, 0x10, 0x8c, 0x9f, 0xcb, 0xf8, 0x02, 0x18,
	0xfe, 0xac, 0x7f, 0x41, 0x46, 0x99, 0xa2, 0x2c, 0x5f, 0xe9, 0x6b, 0x18, 0x70, 0xa3, 0x8b, 0xa3,
	0x33, 0x7c, 0xe7, 0xf1, 0xbe, 0x60, 0x1b, 0x60, 0x9a, 0x97, 0x44, 0x61, 0x7b, 0xf9, 0x53, 0x48,
	0x9d, 0xad, 0x2a, 0x4d, 0x29, 0x12, 0x6a, 0xe3, 0xda, 0x00, 0x87, 0x9d, 0x04, 0x32, 0xeb, 0x04,
	0x00, 0x00
};

const char *updateresult[] = {
	"Failed to retrieve version file!",
	"Invalid version file!",
	"Failed to download update!",
	"Downloaded update failed integrity check!",
	"Failed to open update file for writing!",
	"Okay then :-/",
	"Bot is up-to-date.",
	"Autoupdates not supported for this platform."
};

const int httpchars[] = {
	0xFFFFFFFF,
	0xFC00987B,
	0x78000001,
	0xF8000001,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF
};

char updatesite[32];
char updateverfile[64];

jmp_buf httpesc;


////////////////////////////////////////////////////////////////////////////////////////////////////


int CheckUpdate(const char *site, const char *verfile) {
	int len, version;
	char asdf[256];
	unsigned char *data;
	FILE *file;
	char *updatefile, *integhash;
	char hashbuf1[20], hashbuf2[20];
	char *argv[] = {"bbupdate", NULL};
	int status;

	gstate |= GFS_CHECKINGUPDATES;

	GetOS(asdf);
	data = HttpGetFile(site, verfile, &len, 
		"?ver=%d&os=%s", BOT_VERSION, asdf);
	if (!data) {
		status = 0;
		goto done;
	}
	
	strncpy(asdf, (const char *)data, len);
	free(data);
	asdf[len] = 0;

	if (*asdf == '^') {
		status = 6;
		goto done;
	} else if (*asdf == ',') {
		status = 7;
		goto done;
	}

	updatefile = strchr(asdf, ' ');
	if (!updatefile) {
		status = 1;
		goto done;
	}
	*updatefile++ = 0;

	integhash = strchr(updatefile, ' ');
	if (!integhash) {
		status = 1;
		goto done;
	}
	*integhash++  = 0;
	integhash[20] = 0;
	HexToStr(integhash, hashbuf1);

	version = atoi(asdf);

	printf("Phyros version %d.%d.%d build %d is available. Upgrade? [y/n]\n",
		(version >> 24), (version >> 16) & 0xFF, (version >> 8) & 0xFF, (version) & 0xFF);
	if (getchar() == 'y') {
		printf("Fetching %s%s...\n", site, updatefile);
		data = HttpGetFile(site, updatefile, &len, NULL);
		if (!data) {	
			status = 2;
			goto done;
		}
		SHA1((const char *)data, len, hashbuf2);
		if (memcmp(hashbuf1, hashbuf2, 20)) {
			status = 3;
			goto done;
		}

		RC4Crypt((unsigned char *)updatekey, data, len);

		file = fopen("phyros.upd", "wb");
		if (file) {
			fwrite(data, len, 1, file);
		} else {
			status = 4;
			goto done;
		}
		fclose(file);

		free(data);

		execvp("bbupdate", argv);

		exit(1);
	} else {
		status = 5;
	}
done:
	gstate &= ~GFS_CHECKINGUPDATES;
	return status;
}

/*char httpchars[] = {
	1, 1, 1, 1, 1, 1, 1, 1,	 1, 1, 1, 1, 1, 1, 1, 1, //0
	1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1, //1
//         0x7B		               0x98
	1, 0, 1, 1, 1, 1, 1, 0,  0, 0, 0, 1, 1, 0, 0, 1, //2
//         0x00                    0xFC
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 1, 1, 1, 1, 1, 1, //3

//         0x01                    0x00
	1, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, //4
//         0x00                    0x78
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 1, 1, 1, 1, 0, //5

//         0x01					   0x00
	1, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, //6
//         0x00                    0xF8
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 1, 1, 1, 1, 1, //7
//  0  1  2  3  4  5  6  7 	 8	9  a  b  c  d  e  f
};

CONNECT 64.23.4.32:6122 HTTP/1.1


200 OK
*/


char *HttpEncodeString(char *in, int *written) {
	char *tmp, *buf;
	int explen = 0;

	tmp = in;
	while (*tmp) {
		explen += (httpchars[(*tmp >> 5)] & (1 << (*tmp & 0x1F))) ? 3 : 1;
		tmp++;
	}
	*written = explen;
	buf = malloc(explen + 1);
	tmp = buf;
	while (*in) {
		if (httpchars[(*in >> 5)] & (1 << (*in & 0x1F))) {
			sprintf(tmp, "%%%02X", (unsigned char)*in);
			tmp += 3;
		} else {
			*tmp++ = *in;
		}
		in++;
	}
	*tmp = 0;
	return buf;
} 


void HttpTranslateSpaces(char *buf) {
	while (*buf) {
		if (*buf == ' ')
			*buf = '+';
		buf++;
	}
}


unsigned char *HttpGetFile(const char *website, const char *httpdir,
						   int *filelength, const char *format, ...) {
	char request[128], request1[128];
	char request2[256], recvbuf[1024];
	int len, contentlen, pos;
	char *nextstr, *str;
	unsigned char *data;
	va_list val;

	signal(SIGINT, CtrlCHandler);

	if (!ConnectSocket(website, 80, &httpsck)) {
		printf("WARNING: failed to connect to %s:80\n", website);
		return NULL;
	}
	va_start(val, format);
	sprintf(request, "GET %s%%s http/1.0\r\nHost: %s\r\n\r\n",
		httpdir, website);
	if (format)	{
		len = vsprintf(request1, format, val);
		HttpTranslateSpaces(request1);
		//encodedbuf = HttpEncodeString(request1, &len);
	}
	va_end(val);
	len = sprintf(request2, request, format ? request1 : "");

	send(httpsck, request2, len, 0);
	//free(encodedbuf);

	len = recv(httpsck, recvbuf, sizeof(recvbuf), 0);

	str = recvbuf;
	contentlen = -1;
	do {
		nextstr = strchr(str, '\n');
		if (!nextstr)
			break;
		*nextstr++ = 0;
		if (!strncmp(str, "Content-Length:", 15))
			contentlen = atoi(str + 15);
		//else if (!strncmp(str, "Transfer-Encoding:", 18)) {
		//	if (!strncmp(str + 19, "chunked", 7)) {
			// TODO: support chunked transfers
		//	}
		//}
		str = nextstr;
	} while (*str != '\r');
	str += 2;
	if (contentlen == -1) {
		//printf("no content length specified!\n");
		contentlen = recvbuf + len - str;
		*filelength = contentlen;
		data = malloc(contentlen);
		memcpy(data, str, contentlen);
		return data;
	}
	data = malloc(contentlen);
	pos = len - (str - recvbuf);
	memcpy(data, str, pos);

	if ((pos + contentlen) > sizeof(recvbuf)) {
		do {
			len = recv(httpsck, recvbuf, sizeof(recvbuf), 0);
			if (!len || len == -1)
				break;
			memcpy(data + pos, recvbuf, len);
			pos += len;
		} while (pos < contentlen);
	}

	shutdown(httpsck, SHUT_RDWR);
	closesocket(httpsck);

	*filelength = contentlen;
	return data;
}

