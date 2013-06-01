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
 * fxns.c - 
 *    CRT extensions, Windows/POSIX abstraction layers, and random things that do not fit elsewhere
 */


#include "main.h"
#include "chat.h"
#include "winal.h"
#include "queue.h"
#include "vector.h"
#include "fxns.h"

const char *architectures[] = {
	"x86",
	"MIPS",
	"Alpha",
	"PPC",
	"SHX",
	"ARM",
	"IA64",
	"Alpha 64",
	"MSIL",
	"AMD64",
	"IA32 on Win64",
	"Unknown"
};

///////////////////////////////////////////////////////////////////////////////


void strncpychr(char *dest, char *src, int c, int n) {
	char t;
	int i = 0;
	while (i != n) {
		t = src[i];
		if (!t)
			break;
		if (t == c) {
			dest[i] = 0;
			break;
		}
		dest[i] = t;
		i++;
	}
	dest[n - 1] = 0;
}


void lcase(char *str) {
	while (*str) {
		*str = tolower((unsigned char)*str);
		str++;
	}
}


void lcasecpy(char *dest, const char *src) {
	while ((*dest++ = tolower((unsigned char)*src++)));
}


int strilcmp(const char *s1, const char *s2) {
	while (*s1) {
		int blah = *s1 - tolower((unsigned char)*s2);
		if (blah)
			return blah;
		s1++;
		s2++;
	}
	return *s1 - tolower((unsigned char)*s2);
}


void lcasencpy(char *dest, const char *src, unsigned int n) {
	unsigned int i = 0;

	n--;
	while (*src && i != n) {
		*dest++ = tolower((unsigned char)*src++);
		i++;
	}
	*dest = 0;
}


void strrevncpy(char *dest, const char *src, unsigned int n) {
	unsigned int len, i;
	
	n--;
	i = 0;
	len = strlen(src);
	while (len && i < n) {
		len--;
		dest[i] = src[len];
		i++;
	}
	dest[i] = 0;
}


int countchr(char *str, char find) {
	int found = 0;

	while (*str) {
		if (*str == find)
			found++;
		str++;
	}
	return found;
}


char *skipws(char *str) {
	while (*str && (*str <= 0x20))
		str++;
	return str;
}


char *findws(char *str) {
	while (*str > 0x20) { 
		str++;
		if (!*str)
			return NULL;
	}
	return str;
}


int isnumstr(char *str) {
	while (*str) {
		if ((*str - '0') > 9)
			return 0;
	}
	return 1;
}


int isstrupper(const char *str) {
	while (*str) {
		if (*str >= 'a' && *str <= 'z')
			return 0;
		str++;
	}
	return 1;
}


/*int stricmp(const char *s1, const char *s2) {
	while (1) {
		if (toupper((unsigned char)*s1) != toupper((unsigned char)*s2))
			return 1;
		if (*s1 == 0)
			return 0;
		s1++;
		s2++;
	}
}
*/


char *strrev(char *str) {
	char *p1, *p2;
	
	if (!str || !*str)
		return str;

	p2 = str + strlen(str) - 1;

	for (p1 = str; p2 > p1; ++p1, --p2) {
		*p1 ^= *p2;
		*p2 ^= *p1;
		*p1 ^= *p2;
	}
	return str;
}


char *strdup(const char *src) {
	char *tmp = malloc(strlen(src) + 1);
	strcpy(tmp, src);
	return tmp;
}


#ifndef _WIN32
unsigned int gettick() {
	#ifdef __FreeBSD_version
		struct timespec ts;

		clock_gettime(CLOCK_MONOTONIC_FAST, &ts);
		return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
	#else
		struct timeval tp;

		gettimeofday(&tp, NULL);
		return (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
	#endif
}
#endif


// http://predef.sourceforge.net/preos.html
unsigned int getuptime() {
	//#if defined(BSD) || defined(__bsdi__) || defined(__DragonFly__)
	//	|| defined(hpux) || defined(_AIX) || defined(sgi) 

	#ifdef _linux

		float secs;
		char buf[32], *tmp;
		FILE *file = fopen("/proc/uptime", "r");
		if (!file)
			return 0;
		fgets(buf, sizeof(buf), file);
		tmp = strchr(buf, ' ');
		if (tmp) { 
			*tmp = 0; //get rid of the logon time
			scanf("%f", &secs);
			return (unsigned int)(secs * 1000);
		}
		fclose(file);

	#elif defined(_WIN32)
		
		//LARGE_INTEGER pc, pcf;
		//QueryPerformanceCounter(&pc);
		//QueryPerformanceFrequency(&pcf);
		//return (unsigned int)((unsigned __int64)pc.QuadPart / (unsigned __int64)pcf.QuadPart);

		PDH_FMT_COUNTERVALUE uptimeval;
		PdhCollectQueryData(hQuery);
		PdhGetFormattedCounterValue(hCounter, PDH_FMT_LARGE, NULL, &uptimeval);
		return uptimeval.longValue;

	#else

		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return ts.tv_sec;

	#endif
}


void HexToStr(char *in, char *out) {
	while (*in) {
		if ((unsigned char)(*in - 'a') <= 'f' - 'a')
			*out = (*in - 'a' + 10) << 4;
		else if ((unsigned char)(*in - '0') <= '9' - '0')
			*out = (*in - '0') << 4;
		else
			return;
		in++;
		if ((unsigned char)(*in - 'a') <= 'f' - 'a')
			*out |= (*in - 'a' + 10);
		else if ((unsigned char)(*in - '0') <= '9' - '0')
			*out |= (*in - '0');
		else
			return;
		in++;
		out++;
	}
}


void StrToHexOut(char *data, int len) {
    int i;

    for (i = 0; i != len; i++)
        printf("%02x ", (unsigned char)data[i]);
	putchar('\n');
}


void GetOS(char *buf) {
	#ifdef _WIN32
		OSVERSIONINFOEX osviex;
		SYSTEM_INFO sysinfo;

		osviex.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		GetVersionEx((LPOSVERSIONINFO)&osviex);
		GetSystemInfo(&sysinfo);

		if (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_UNKNOWN)
			sysinfo.wProcessorArchitecture = 11;

		sprintf(buf, "Microsoft Windows %s %d.%d.%d %s",
			GetWinVersionName(&osviex), osviex.dwMajorVersion, osviex.dwMinorVersion,
			osviex.dwBuildNumber, architectures[sysinfo.wProcessorArchitecture]);
	#else
		struct utsname name;
		if (uname(&name))
			strcpy(buf, "uname() failed!");
		else
			sprintf(buf, "%s %s %s", name.sysname, name.release, name.machine);
	#endif
}


void SHA1(const char *source, unsigned long len, char *output) {
	#ifdef _WIN32
		unsigned long hashlen = 20;
		CryptHashCertificate((unsigned long)NULL, 0, 0, (const unsigned char *)source,
			len, (unsigned char *)output, &hashlen);
	#else
		char temp[64];
		SHA1_Data((unsigned char *)source, (unsigned int)len, temp);
		HexToStr(temp, output);
	#endif
}


void MD5(const char *source, unsigned long len, char *output) {
	#ifdef _WIN32
		unsigned long hashlen = 16;
		CryptHashCertificate((unsigned long)NULL, CALG_MD5, 0, (const unsigned char *)source,
			len, (unsigned char *)output, &hashlen);
	#else
		char temp[64];
		MD5Data((unsigned char *)source, (unsigned int)len, temp);
		HexToStr(temp, output);
	#endif
}


__inline div_t udiv(unsigned int numer, unsigned int denom) {
	div_t blah = {numer / denom, numer % denom};
	return blah;
}


int GetUptimeString(unsigned long time, char *outbuf) {
	unsigned long days, hours, minutes, seconds;
	div_t tmp; //time is in SECONDS

	tmp          = udiv(time, 86400);
	days         = tmp.quot;
	time         = tmp.rem;
	tmp          = udiv(time, 3600);
	hours        = tmp.quot;
	time         = tmp.rem;
	tmp          = udiv(time, 60);
	minutes      = tmp.quot;
	seconds      = tmp.rem;
	return sprintf(outbuf, "%ud, %uh, %um, %us",
		(unsigned int)days, (unsigned int)hours,
		(unsigned int)minutes, (unsigned int)seconds);
}


void __fastcall GetTimeStamp(char *buf) {
	time_t rawtime;
	void *timeinfo;
	time(&rawtime);
	timeinfo = (void *)localtime(&rawtime);
	strftime(buf, 16, "[%I:%M:%S %p]", (void *)timeinfo);
}


int curbotinc() {
	int i, current;

	current = curbot;
	for (i = curbot + 1; i != numbots; i++) {
		if (bot[i]->connected) {
			curbot = i;
			return current;
		}
	}
	curbot = 0;
	return current;
}


void CALLBACK FileDeleteRecord(char *buffer, int data) {
	*buffer = 0;
}


int FileModifyRecord(const char *filename, const char *record, int data,
					  int (CALLBACK *record_compare)(const char *, char *),
					  void (CALLBACK *record_modify)(char *, int)) {
	char asdf[256], *tempfn;
	FILE *in, *out;

	tempfn = tmpnam(NULL);

	in  = fopen(filename, "r");
	if (!in) {
		printf("WARNING: cannot open %s for reading, errno: %d\n",
			filename, geterr);
		return 0;
	}

	out = fopen(tempfn, "w");
	if (!out) {
		printf("WARNING: cannot open temp file %s for writing, errno: %d\n",
			tempfn, geterr);
		fclose(in);
		return 0;
	}

	while (!feof(in)) {
		fgets(asdf, sizeof(asdf), in);
		if (!record_compare(record, asdf))
			record_modify(asdf, data);
		fputs(asdf, out);
	}

	fclose(out);
	fclose(in);

	if (remove(filename) == -1) {
		printf("WARNING: cannot remove %s, errno: %d\n",
			filename, errno);
		return 0;
	}
	if (rename(tempfn, filename) == -1) {
		printf("WARNING: cannot rename %s to %s, errno: %d\n",
			tempfn, filename, errno);
		return 0;
	}
	remove(tempfn);
	return 1;
}



void *CALLBACK RunLoudProc(void *cmd) {
    char buf[256];
	int len;
	FILE *file;

	strncpy(buf, (const char *)cmd, sizeof(buf));
	strcat(buf, " 2>&1");
	file = popen(buf, "r");
	if (!file) {
		QueueAdd("Cannot execute command!", curbotinc());
		return NULL;
	}
	while (!feof(file)) {
		if (!fgets(buf, sizeof(buf), file))
			break;
		len = strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = 0;
		QueueAdd(buf, curbotinc());
	}
	pclose(file);
	return NULL;
}


void *CALLBACK RunQuietProc(void *cmd) {
    system((char *)cmd);
	return NULL;
}


#ifndef _WIN32

int GetUidFromUsername(const char *username) {
	struct passwd *pass;

	if (!username)
		return -1;

	while ((pass = getpwent())) {
		if (!strcmp(pass->pw_name, username)) {
			endpwent();		
			return pass->pw_uid;
		}
	}
	endpwent();
	return -1;
}

#endif

unsigned long GenRandomSeedUL() {
	unsigned long r;

	#ifdef _WIN32
		#if WINVER >= 0x501
				if (lpfnRtlGenRandom && lpfnRtlGenRandom(&r, sizeof(unsigned long)))
					return r;
				else
					return (GetTickCount() ^ GetCurrentProcessId());
		#else
				return (GetTickCount() ^ GetCurrentProcessId());
		#endif
	#else
		FILE *f;

		f = fopen("/dev/urandom", "rb");
		if (!f) {
			f = fopen("/dev/random", "rb");
			if (!f) {
				srand(time(NULL));
				return (unsigned long)rand();
			}
		}
		if (fread(&r, sizeof(unsigned long), 1, f) != 1) {
			fclose(f);
			srand(time(NULL));
			return (unsigned long)rand();
		}
		fclose(f);
		return r;
	#endif
}

