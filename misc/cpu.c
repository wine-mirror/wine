/*
 * What processor?
 *
 * Copyright 1995,1997 Morten Welinder
 * Copyright 1997-1998 Marcus Meissner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
# include <sys/sysctl.h>
#endif
#ifdef HAVE_MACHINE_CPU_H
# include <machine/cpu.h>
#endif

#include <ctype.h>
#include <string.h>
#include <stdio.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "winbase.h"
#include "winnt.h"
#include "winternl.h"
#include "winerror.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(reg);

static BYTE PF[64] = {0,};

static void create_registry_keys( const SYSTEM_INFO *info )
{
    static const WCHAR SystemW[] = {'M','a','c','h','i','n','e','\\',
                                    'H','a','r','d','w','a','r','e','\\',
                                    'D','e','s','c','r','i','p','t','i','o','n','\\',
                                    'S','y','s','t','e','m',0};
    static const WCHAR fpuW[] = {'F','l','o','a','t','i','n','g','P','o','i','n','t','P','r','o','c','e','s','s','o','r',0};
    static const WCHAR cpuW[] = {'C','e','n','t','r','a','l','P','r','o','c','e','s','s','o','r',0};
    static const WCHAR IdentifierW[] = {'I','d','e','n','t','i','f','i','e','r',0};
    static const WCHAR SysidW[] = {'A','T',' ','c','o','m','p','a','t','i','b','l','e',0};

    int i;
    HKEY hkey, system_key, cpu_key;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW, valueW;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    RtlInitUnicodeString( &nameW, SystemW );
    if (NtCreateKey( &system_key, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL )) return;

    RtlInitUnicodeString( &valueW, IdentifierW );
    NtSetValueKey( system_key, &valueW, 0, REG_SZ, SysidW, (strlenW(SysidW)+1) * sizeof(WCHAR) );

    attr.RootDirectory = system_key;
    RtlInitUnicodeString( &nameW, fpuW );
    if (!NtCreateKey( &hkey, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL )) NtClose( hkey );

    RtlInitUnicodeString( &nameW, cpuW );
    if (!NtCreateKey( &cpu_key, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL ))
    {
        for (i = 0; i < info->dwNumberOfProcessors; i++)
        {
            char num[10], id[20];

            attr.RootDirectory = cpu_key;
            sprintf( num, "%d", i );
            RtlCreateUnicodeStringFromAsciiz( &nameW, num );
            if (!NtCreateKey( &hkey, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL ))
            {
                WCHAR idW[20];

                sprintf( id, "CPU %ld", info->dwProcessorType );
                RtlMultiByteToUnicodeN( idW, sizeof(idW), NULL, id, strlen(id)+1 );
                NtSetValueKey( hkey, &valueW, 0, REG_SZ, idW, (strlenW(idW)+1)*sizeof(WCHAR) );
                NtClose( hkey );
            }
            RtlFreeUnicodeString( &nameW );
        }
        NtClose( cpu_key );
    }
    NtClose( system_key );
}


/***********************************************************************
 * 			GetSystemInfo            	[KERNEL32.@]
 *
 * Gets the current system information.
 *
 * On the first call it creates cached values, so it doesn't have to determine
 * them repeatedly. On Linux, the /proc/cpuinfo special file is used.
 *
 * It creates a registry subhierarchy, looking like:
 * \HARDWARE\DESCRIPTION\System\CentralProcessor\<processornumber>\
 *							Identifier (CPU x86)
 * Note that there is a hierarchy for every processor installed, so this
 * supports multiprocessor systems. This is done like Win95 does it, I think.
 *
 * It also creates a cached flag array for IsProcessorFeaturePresent().
 *
 * No NULL ptr check for LPSYSTEM_INFO in Win9x.
 *
 * RETURNS
 *	nothing, really
 */
VOID WINAPI GetSystemInfo(
	LPSYSTEM_INFO si	/* [out] system information */
) {
	static int cache = 0;
	static SYSTEM_INFO cachedsi;

	if (cache) {
		memcpy(si,&cachedsi,sizeof(*si));
		return;
	}
	memset(PF,0,sizeof(PF));

	/* choose sensible defaults ...
	 * FIXME: perhaps overrideable with precompiler flags?
	 */
	cachedsi.u.s.wProcessorArchitecture     = PROCESSOR_ARCHITECTURE_INTEL;
	cachedsi.dwPageSize 			= getpagesize();

	/* FIXME: the two entries below should be computed somehow... */
	cachedsi.lpMinimumApplicationAddress	= (void *)0x00010000;
	cachedsi.lpMaximumApplicationAddress	= (void *)0x7FFFFFFF;
	cachedsi.dwActiveProcessorMask		= 1;
	cachedsi.dwNumberOfProcessors		= 1;
	cachedsi.dwProcessorType		= PROCESSOR_INTEL_PENTIUM;
	cachedsi.dwAllocationGranularity	= 0x10000;
	cachedsi.wProcessorLevel		= 5; /* 586 */
	cachedsi.wProcessorRevision		= 0;

	cache = 1; /* even if there is no more info, we now have a cacheentry */
	memcpy(si,&cachedsi,sizeof(*si));

	/* Hmm, reasonable processor feature defaults? */

#ifdef linux
	{
	char line[200];
	FILE *f = fopen ("/proc/cpuinfo", "r");

	if (!f)
		return;
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
		if (!strncasecmp(line, "cpu family",strlen("cpu family"))) {
			if (isdigit (value[0])) {
				switch (value[0] - '0') {
				case 3: cachedsi.dwProcessorType = PROCESSOR_INTEL_386;
					cachedsi.wProcessorLevel= 3;
					break;
				case 4: cachedsi.dwProcessorType = PROCESSOR_INTEL_486;
					cachedsi.wProcessorLevel= 4;
					break;
				case 5:
				case 6: /* PPro/2/3 has same info as P1 */
					cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
					cachedsi.wProcessorLevel= 5;
					break;
				case 1: /* two-figure levels */
                                    if (value[1] == '5')
                                    {
                                        cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
                                        cachedsi.wProcessorLevel= 5;
                                        break;
                                    }
                                    /* fall through */
				default:
					FIXME("unknown cpu family '%s', please report ! (-> setting to 386)\n", value);
					break;
				}
			}
			continue;
		}
		/* old 2.0 method */
		if (!strncasecmp(line, "cpu",strlen("cpu"))) {
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
				case 5:
				case 6: /* PPro/2/3 has same info as P1 */
					cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
					cachedsi.wProcessorLevel= 5;
					break;
				default:
					FIXME("unknown Linux 2.0 cpu family '%s', please report ! (-> setting to 386)\n", value);
					break;
				}
			}
			continue;
		}
		if (!strncasecmp(line,"fdiv_bug",strlen("fdiv_bug"))) {
			if (!strncasecmp(value,"yes",3))
				PF[PF_FLOATING_POINT_PRECISION_ERRATA] = TRUE;

			continue;
		}
		if (!strncasecmp(line,"fpu",strlen("fpu"))) {
			if (!strncasecmp(value,"no",2))
				PF[PF_FLOATING_POINT_EMULATED] = TRUE;

			continue;
		}
		if (!strncasecmp(line,"processor",strlen("processor"))) {
			/* processor number counts up... */
			unsigned int x;

			if (sscanf(value,"%d",&x))
				if (x+1>cachedsi.dwNumberOfProcessors)
					cachedsi.dwNumberOfProcessors=x+1;
		}
		if (!strncasecmp(line,"stepping",strlen("stepping"))) {
			int	x;

			if (sscanf(value,"%d",&x))
				cachedsi.wProcessorRevision = x;
		}
		if (	!strncasecmp(line,"flags",strlen("flags"))	||
			!strncasecmp(line,"features",strlen("features"))
		) {
			if (strstr(value,"cx8"))
				PF[PF_COMPARE_EXCHANGE_DOUBLE] = TRUE;
			if (strstr(value,"mmx"))
				PF[PF_MMX_INSTRUCTIONS_AVAILABLE] = TRUE;
			if (strstr(value,"tsc"))
				PF[PF_RDTSC_INSTRUCTION_AVAILABLE] = TRUE;

		}
	}
	fclose (f);
	}
	memcpy(si,&cachedsi,sizeof(*si));
#elif defined (__NetBSD__)
        {
             int mib[2];
             int value[2];
             char model[256];
             char *cpuclass;
             FILE *f = fopen ("/var/run/dmesg.boot", "r");

             /* first deduce as much as possible from the sysctls */
             mib[0] = CTL_MACHDEP;
#ifdef CPU_FPU_PRESENT
             mib[1] = CPU_FPU_PRESENT;
             value[1] = sizeof(int);
             if (sysctl(mib, 2, value, value+1, NULL, 0) >= 0)
                 if (value) PF[PF_FLOATING_POINT_EMULATED] = FALSE;
                 else       PF[PF_FLOATING_POINT_EMULATED] = TRUE;
#endif
#ifdef CPU_SSE
             mib[1] = CPU_SSE;   /* this should imply MMX */
             value[1] = sizeof(int);
             if (sysctl(mib, 2, value, value+1, NULL, 0) >= 0)
                 if (value) PF[PF_MMX_INSTRUCTIONS_AVAILABLE] = TRUE;
#endif
#ifdef CPU_SSE2
             mib[1] = CPU_SSE2;  /* this should imply MMX */
             value[1] = sizeof(int);
             if (sysctl(mib, 2, value, value+1, NULL, 0) >= 0)
                 if (value) PF[PF_MMX_INSTRUCTIONS_AVAILABLE] = TRUE;
#endif
             mib[0] = CTL_HW;
             mib[1] = HW_NCPU;
             value[1] = sizeof(int);
             if (sysctl(mib, 2, value, value+1, NULL, 0) >= 0)
                 if (value[0] > cachedsi.dwNumberOfProcessors)
                    cachedsi.dwNumberOfProcessors = value[0];
             mib[1] = HW_MODEL;
             value[1] = 255;
             if (sysctl(mib, 2, model, value+1, NULL, 0) >= 0) {
                  model[value[1]] = '\0'; /* just in case */
                  cpuclass = strstr(model, "-class");
                  if (cpuclass != NULL) {
                       while(cpuclass > model && cpuclass[0] != '(') cpuclass--;
                       if (!strncmp(cpuclass+1, "386", 3)) {
                            cachedsi.dwProcessorType = PROCESSOR_INTEL_386;
                            cachedsi.wProcessorLevel= 3;
                       }
                       if (!strncmp(cpuclass+1, "486", 3)) {
                            cachedsi.dwProcessorType = PROCESSOR_INTEL_486;
                            cachedsi.wProcessorLevel= 4;
                       }
                       if (!strncmp(cpuclass+1, "586", 3)) {
                            cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
                            cachedsi.wProcessorLevel= 5;
                       }
                       if (!strncmp(cpuclass+1, "686", 3)) {
                            cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
                            cachedsi.wProcessorLevel= 5;
                            /* this should imply MMX */
                            PF[PF_MMX_INSTRUCTIONS_AVAILABLE] = TRUE;
                       }
                  }
             }

             /* it may be worth reading from /var/run/dmesg.boot for
                additional information such as CX8, MMX and TSC
                (however this information should be considered less
                 reliable than that from the sysctl calls) */
             if (f != NULL)
             {
	         while (fgets(model, 255, f) != NULL) {
                        if (sscanf(model,"cpu%d: features %x<", value, value+1) == 2) {
                            /* we could scan the string but it is easier
                               to test the bits directly */
                            if (value[1] & 0x1)
                                PF[PF_FLOATING_POINT_EMULATED] = TRUE;
                            if (value[1] & 0x10)
                                PF[PF_RDTSC_INSTRUCTION_AVAILABLE] = TRUE;
                            if (value[1] & 0x100)
                                PF[PF_COMPARE_EXCHANGE_DOUBLE] = TRUE;
                            if (value[1] & 0x800000)
                                PF[PF_MMX_INSTRUCTIONS_AVAILABLE] = TRUE;

                            break;
                        }
                 }
                 fclose(f);
             }

        }
        memcpy(si,&cachedsi,sizeof(*si));
#else /* !linux && !__NetBSD__ */
	FIXME("not yet supported on this system\n");
#endif  /* !linux  && !__NetBSD__ */
        TRACE("<- CPU arch %d, res'd %d, pagesize %ld, minappaddr %p, maxappaddr %p,"
              " act.cpumask %08lx, numcpus %ld, CPU type %ld, allocgran. %ld, CPU level %d, CPU rev %d\n",
              si->u.s.wProcessorArchitecture, si->u.s.wReserved, si->dwPageSize,
              si->lpMinimumApplicationAddress, si->lpMaximumApplicationAddress,
              si->dwActiveProcessorMask, si->dwNumberOfProcessors, si->dwProcessorType,
              si->dwAllocationGranularity, si->wProcessorLevel, si->wProcessorRevision);

        create_registry_keys( &cachedsi );
}


/***********************************************************************
 * 			IsProcessorFeaturePresent	[KERNEL32.@]
 * RETURNS:
 *	TRUE if processor feature present
 *	FALSE otherwise
 */
BOOL WINAPI IsProcessorFeaturePresent (
	DWORD feature	/* [in] feature number, see PF_ defines */
) {
  SYSTEM_INFO si;
  GetSystemInfo (&si); /* To ensure the information is loaded and cached */

  if (feature < 64)
    return PF[feature];
  else
    return FALSE;
}
