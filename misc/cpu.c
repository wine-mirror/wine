/*
 * What processor?
 *
 * Copyright 1995 Morten Welinder
 * Copyright 1997 Marcus Meissner
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "windows.h"

VOID
GetSystemInfo(LPSYSTEM_INFO si)
{
	static int cache = 0;
	static SYSTEM_INFO cachedsi;

	if (cache) {
		memcpy(si,&cachedsi,sizeof(*si));
		return;
	}

	/* choose sensible defaults ...
	 * FIXME: perhaps overrideable with precompiler flags?
	 */
	cachedsi.u.x.wProcessorArchitecture	= PROCESSOR_ARCHITECTURE_INTEL;
	cachedsi.dwPageSize 			= 4096;

	/* FIXME: better values for the two entries below... */
	cachedsi.lpMinimumApplicationAddress	= (void *)0x40000000;
	cachedsi.lpMaximumApplicationAddress	= (void *)0x80000000;
	cachedsi.dwActiveProcessorMask		= 1;
	cachedsi.dwNumberOfProcessors		= 1;
	cachedsi.dwProcessorType		= PROCESSOR_INTEL_386;
	cachedsi.dwAllocationGranularity	= 8;
	cachedsi.wProcessorLevel		= 3; /* 386 */
	cachedsi.wProcessorRevision		= 0;

	cache = 1; /* even if there is no more info, we now have a cacheentry */
	memcpy(si,&cachedsi,sizeof(*si));

#ifdef linux
	{
	char line[200],info[200],value[200],junk[200];
	FILE *f = fopen ("/proc/cpuinfo", "r");

	if (!f)
		return;
	while (fgets(line,200,f)!=NULL) {
		if (sscanf(line,"%s%[ \t:]%s",info,junk,value)!=3)
			continue;
		if (!lstrncmpi32A(line, "cpu",3)) {
			if (	isdigit (value[0]) && value[1] == '8' && 
				value[2] == '6' && value[3] == 0
			) {
				switch (value[0] - '0') {
				case 3:
					cachedsi.dwProcessorType = PROCESSOR_INTEL_386;
					cachedsi.wProcessorLevel= 3;
					break;
				case 4:
					cachedsi.dwProcessorType = PROCESSOR_INTEL_486;
					cachedsi.wProcessorLevel= 4;
					break;
				case 5:
					cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
					cachedsi.wProcessorLevel= 5;
					break;
				case 6: /* FIXME does the PPro have a special type? */
					cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
					cachedsi.wProcessorLevel= 5;
					break;
				default:
					break;
				}
			}
		}
		if (!lstrncmpi32A(info,"processor",9)) {
			/* processor number counts up...*/
			int	x;

			if (sscanf(value,"%d",&x))
				if (x+1>cachedsi.dwNumberOfProcessors)
					cachedsi.dwNumberOfProcessors=x+1;
		}
		if (!lstrncmpi32A(info,"stepping",8)) {
			int	x;

			if (sscanf(value,"%d",&x))
				cachedsi.wProcessorRevision = x;
		}
	}
	fclose (f);
	}
	memcpy(si,&cachedsi,sizeof(*si));
	return;
#else  /* linux */
	/* FIXME: how do we do this on other systems? */
	return;
#endif  /* linux */
}
