/*
 * What processor?
 *
 * Copyright 1995,1997 Morten Welinder
 * Copyright 1997 Marcus Meissner
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "global.h"
#include "windows.h"
#include "winnt.h"
#include "winreg.h"

static BYTE PF[64] = {0,};

/***********************************************************************
 * 			GetSystemInfo            	[KERNELL32.404]
 */
VOID WINAPI GetSystemInfo(LPSYSTEM_INFO si)
{
	static int cache = 0;
	static SYSTEM_INFO cachedsi;
	HKEY	xhkey=0,hkey;
	char	buf[20];

	if (cache) {
		memcpy(si,&cachedsi,sizeof(*si));
		return;
	}
	memset(PF,0,sizeof(PF));

	/* choose sensible defaults ...
	 * FIXME: perhaps overrideable with precompiler flags?
	 */
	cachedsi.u.x.wProcessorArchitecture	= PROCESSOR_ARCHITECTURE_INTEL;
	cachedsi.dwPageSize 			= VIRTUAL_GetPageSize();

	/* FIXME: better values for the two entries below... */
	cachedsi.lpMinimumApplicationAddress	= (void *)0x40000000;
	cachedsi.lpMaximumApplicationAddress	= (void *)0x7FFFFFFF;
	cachedsi.dwActiveProcessorMask		= 1;
	cachedsi.dwNumberOfProcessors		= 1;
	cachedsi.dwProcessorType		= PROCESSOR_INTEL_386;
	cachedsi.dwAllocationGranularity	= 0x10000;
	cachedsi.wProcessorLevel		= 3; /* 386 */
	cachedsi.wProcessorRevision		= 0;

	cache = 1; /* even if there is no more info, we now have a cacheentry */
	memcpy(si,&cachedsi,sizeof(*si));

	/* hmm, reasonable processor feature defaults? */

#ifdef linux
	{
	char line[200];
	FILE *f = fopen ("/proc/cpuinfo", "r");

	if (!f)
		return;
        xhkey = 0;
	RegCreateKey16(HKEY_LOCAL_MACHINE,"\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor",&hkey);
	while (fgets(line,200,f)!=NULL) {
		char	*s,*value;

		/* NOTE: the ':' is the only character we can rely on */
		if (!(value = strchr(line,':')))
			continue;
		/* terminate the valuename */
		*value++ = '\0';
		/* skip any leading spaces */
		while (*value==' ') value++;
		if ((s=strchr(value,'\n')))
			*s='\0';

		/* 2.1 method */
		if (!lstrncmpi32A(line, "cpu family",strlen("cpu family"))) {
			if (isdigit (value[0])) {
				switch (value[0] - '0') {
				case 3: cachedsi.dwProcessorType = PROCESSOR_INTEL_386;
					cachedsi.wProcessorLevel= 3;
					break;
				case 4: cachedsi.dwProcessorType = PROCESSOR_INTEL_486;
					cachedsi.wProcessorLevel= 4;
					break;
				case 5: cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
					cachedsi.wProcessorLevel= 5;
					break;
				case 6: cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
					cachedsi.wProcessorLevel= 5;
					break;
				default:
					break;
				}
			}
			/* set the CPU type of the current processor */
			sprintf(buf,"CPU %ld",cachedsi.dwProcessorType);
			if (xhkey)
				RegSetValueEx32A(xhkey,"Identifier",0,REG_SZ,buf,strlen(buf));
			continue;
		}
		/* old 2.0 method */
		if (!lstrncmpi32A(line, "cpu",strlen("cpu"))) {
			if (	isdigit (value[0]) && value[1] == '8' && 
				value[2] == '6' && value[3] == 0
			) {
				switch (value[0] - '0') {
				case 3: cachedsi.dwProcessorType = PROCESSOR_INTEL_386;
					cachedsi.wProcessorLevel= 3;
					break;
				case 4: cachedsi.dwProcessorType = PROCESSOR_INTEL_486;
					cachedsi.wProcessorLevel= 4;
					break;
				case 5: cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
					cachedsi.wProcessorLevel= 5;
					break;
				case 6: cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
					cachedsi.wProcessorLevel= 5;
					break;
				default:
					break;
				}
			}
			/* set the CPU type of the current processor */
			sprintf(buf,"CPU %ld",cachedsi.dwProcessorType);
			if (xhkey)
				RegSetValueEx32A(xhkey,"Identifier",0,REG_SZ,buf,strlen(buf));
			continue;
		}
		if (!lstrncmpi32A(line,"fdiv_bug",strlen("fdiv_bug"))) {
			if (!lstrncmpi32A(value,"yes",3))
				PF[PF_FLOATING_POINT_PRECISION_ERRATA] = TRUE;

			continue;
		}
		if (!lstrncmpi32A(line,"fpu",strlen("fpu"))) {
			if (!lstrncmpi32A(value,"no",2))
				PF[PF_FLOATING_POINT_EMULATED] = TRUE;

			continue;
		}
		if (!lstrncmpi32A(line,"processor",strlen("processor"))) {
			/* processor number counts up...*/
			int	x;

			if (sscanf(value,"%d",&x))
				if (x+1>cachedsi.dwNumberOfProcessors)
					cachedsi.dwNumberOfProcessors=x+1;

			/* create a new processor subkey */
			sprintf(buf,"%d",x);
			if (xhkey)
				RegCloseKey(xhkey);
			RegCreateKey16(hkey,buf,&xhkey);
		}
		if (!lstrncmpi32A(line,"stepping",strlen("stepping"))) {
			int	x;

			if (sscanf(value,"%d",&x))
				cachedsi.wProcessorRevision = x;
		}
		if (!lstrncmpi32A(line,"flags",strlen("flags"))) {
			if (strstr(value,"cx8"))
				PF[PF_COMPARE_EXCHANGE_DOUBLE] = TRUE;
			if (strstr(value,"mmx"))
				PF[PF_MMX_INSTRUCTIONS_AVAILABLE] = TRUE;

		}
	}
	fclose (f);
	}
	memcpy(si,&cachedsi,sizeof(*si));
#else  /* linux */
	/* FIXME: how do we do this on other systems? */

	RegCreateKey16(hkey,"0",&xhkey);
	RegSetValueEx32A(xhkey,"Identifier",0,REG_SZ,"CPU 386",strlen("CPU 386"));
#endif  /* !linux */
	if (xhkey)
		RegCloseKey(xhkey);
	RegCloseKey(hkey);
}

/***********************************************************************
 * 			IsProcessorFeaturePresent	[KERNELL32.880]
 */
BOOL32 WINAPI IsProcessorFeaturePresent (DWORD feature)
{
  SYSTEM_INFO si;
  GetSystemInfo (&si); /* to ensure the information is loaded and cached */

  if (feature < 64)
    return PF[feature];
  else
    return FALSE;
}
