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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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
#ifdef HAVE_MACH_MACHINE_H
# include <mach/machine.h>
#endif

#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif


#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winternl.h"
#include "winerror.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(reg);

#define AUTH	0x68747541	/* "Auth" */
#define ENTI	0x69746e65	/* "enti" */
#define CAMD	0x444d4163	/* "cAMD" */

/* Calls cpuid with an eax of 'ax' and returns the 16 bytes in *p
 * We are compiled with -fPIC, so we can't clobber ebx.
 */
static inline void do_cpuid(unsigned int ax, unsigned int *p)
{
#ifdef __i386__
	__asm__("pushl %%ebx\n\t"
                "cpuid\n\t"
                "movl %%ebx, %%esi\n\t"
                "popl %%ebx"
                : "=a" (p[0]), "=S" (p[1]), "=c" (p[2]), "=d" (p[3])
                :  "0" (ax));
#endif
}

/* From xf86info havecpuid.c 1.11 */
static inline int have_cpuid(void)
{
#ifdef __i386__
	unsigned int f1, f2;
	__asm__("pushfl\n\t"
                "pushfl\n\t"
                "popl %0\n\t"
                "movl %0,%1\n\t"
                "xorl %2,%0\n\t"
                "pushl %0\n\t"
                "popfl\n\t"
                "pushfl\n\t"
                "popl %0\n\t"
                "popfl"
                : "=&r" (f1), "=&r" (f2)
                : "ir" (0x00200000));
	return ((f1^f2) & 0x00200000) != 0;
#else
        return 0;
#endif
}

static BYTE PF[64] = {0,};
static ULONGLONG cpuHz = 1000000000; /* default to a 1GHz */

static void create_system_registry_keys( const SYSTEM_INFO *info )
{
    static const WCHAR SystemW[] = {'M','a','c','h','i','n','e','\\',
                                    'H','a','r','d','w','a','r','e','\\',
                                    'D','e','s','c','r','i','p','t','i','o','n','\\',
                                    'S','y','s','t','e','m',0};
    static const WCHAR fpuW[] = {'F','l','o','a','t','i','n','g','P','o','i','n','t','P','r','o','c','e','s','s','o','r',0};
    static const WCHAR cpuW[] = {'C','e','n','t','r','a','l','P','r','o','c','e','s','s','o','r',0};
    static const WCHAR IdentifierW[] = {'I','d','e','n','t','i','f','i','e','r',0};
    static const WCHAR SysidW[] = {'A','T',' ','c','o','m','p','a','t','i','b','l','e',0};
    static const WCHAR mhzKeyW[] = {'~','M','H','z',0};
    static const WCHAR VendorIdentifierW[] = {'V','e','n','d','o','r','I','d','e','n','t','i','f','i','e','r',0};
    static const WCHAR VenidIntelW[] = {'G','e','n','u','i','n','e','I','n','t','e','l',0};
    /* static const WCHAR VenidAMDW[] = {'A','u','t','h','e','n','t','i','c','A','M','D',0}; */

    unsigned int i;
    HANDLE hkey, system_key, cpu_key;
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
            char num[10], id[60];

            attr.RootDirectory = cpu_key;
            sprintf( num, "%d", i );
            RtlCreateUnicodeStringFromAsciiz( &nameW, num );
            if (!NtCreateKey( &hkey, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL ))
            {
                WCHAR idW[60];
                DWORD cpuMHz = cpuHz / 1000000;

                /*TODO: report 64bit processors properly*/
                RtlInitUnicodeString( &valueW, IdentifierW );
                sprintf( id, "x86 Family %d Model %d Stepping %d",
                         info->wProcessorLevel, HIBYTE(info->wProcessorRevision), LOBYTE(info->wProcessorRevision) );

                RtlMultiByteToUnicodeN( idW, sizeof(idW), NULL, id, strlen(id)+1 );
                NtSetValueKey( hkey, &valueW, 0, REG_SZ, idW, (strlenW(idW)+1)*sizeof(WCHAR) );

                /*TODO; report amd's properly*/
                RtlInitUnicodeString( &valueW, VendorIdentifierW );
                NtSetValueKey( hkey, &valueW, 0, REG_SZ, VenidIntelW, (strlenW(VenidIntelW)+1) * sizeof(WCHAR) );

                RtlInitUnicodeString( &valueW, mhzKeyW );
                NtSetValueKey( hkey, &valueW, 0, REG_DWORD, &cpuMHz, sizeof(DWORD) );
                NtClose( hkey );
            }
            RtlFreeUnicodeString( &nameW );
        }
        NtClose( cpu_key );
    }
    NtClose( system_key );
}

static void create_env_registry_keys( const SYSTEM_INFO *info )
{
    static const WCHAR EnvironW[]  = {'M','a','c','h','i','n','e','\\',
                                      'S','y','s','t','e','m','\\',
                                      'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                      'C','o','n','t','r','o','l','\\',
                                      'S','e','s','s','i','o','n',' ','M','a','n','a','g','e','r','\\',
                                      'E','n','v','i','r','o','n','m','e','n','t',0};
    static const WCHAR NumProcW[]  = {'N','U','M','B','E','R','_','O','F','_','P','R','O','C','E','S','S','O','R','S',0};
    static const WCHAR ProcArchW[] = {'P','R','O','C','E','S','S','O','R','_','A','R','C','H','I','T','E','C','T','U','R','E',0};
    static const WCHAR x86W[]      = {'x','8','6',0};
    static const WCHAR ProcIdW[]   = {'P','R','O','C','E','S','S','O','R','_','I','D','E','N','T','I','F','I','E','R',0};
    static const WCHAR ProcLvlW[]  = {'P','R','O','C','E','S','S','O','R','_','L','E','V','E','L',0};
    static const WCHAR ProcRevW[]  = {'P','R','O','C','E','S','S','O','R','_','R','E','V','I','S','I','O','N',0};

    HANDLE env_key;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW, valueW;

    char nProc[10],id[60],procLevel[10],rev[10];
    WCHAR nProcW[10],idW[60],procLevelW[10],revW[10];

    /* Create some keys under HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Environment.
     * All these environment variables are processor related and will be read during process initialization and hence
     * show up in the environment of that process.
     */

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    RtlInitUnicodeString( &nameW, EnvironW );
    if (NtCreateKey( &env_key, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL )) return;

    sprintf( nProc, "%d", info->dwNumberOfProcessors );
    RtlMultiByteToUnicodeN( nProcW, sizeof(nProcW), NULL, nProc, strlen(nProc)+1 );
    RtlInitUnicodeString( &valueW, NumProcW );
    NtSetValueKey( env_key, &valueW, 0, REG_SZ, nProcW, (strlenW(nProcW)+1)*sizeof(WCHAR) );

    /* TODO: currently hardcoded x86, add different processors */
    RtlInitUnicodeString( &valueW, ProcArchW );
    NtSetValueKey( env_key, &valueW, 0, REG_SZ, x86W, (strlenW(x86W)+1) * sizeof(WCHAR) );

    /* TODO: currently hardcoded Intel, add different processors */
    sprintf( id, "x86 Family %d Model %d Stepping %d, GenuineIntel",
        info->wProcessorLevel, HIBYTE(info->wProcessorRevision), LOBYTE(info->wProcessorRevision) );
    RtlMultiByteToUnicodeN( idW, sizeof(idW), NULL, id, strlen(id)+1 );
    RtlInitUnicodeString( &valueW, ProcIdW );
    NtSetValueKey( env_key, &valueW, 0, REG_SZ, idW, (strlenW(idW)+1)*sizeof(WCHAR) );

    sprintf( procLevel, "%d", info->wProcessorLevel );
    RtlMultiByteToUnicodeN( procLevelW, sizeof(procLevelW), NULL, procLevel, strlen(procLevel)+1 );
    RtlInitUnicodeString( &valueW, ProcLvlW );
    NtSetValueKey( env_key, &valueW, 0, REG_SZ, procLevelW, (strlenW(procLevelW)+1)*sizeof(WCHAR) );

    /* Properly report model/stepping */
    sprintf( rev, "%04x", info->wProcessorRevision);
    RtlMultiByteToUnicodeN( revW, sizeof(revW), NULL, rev, strlen(rev)+1 );
    RtlInitUnicodeString( &valueW, ProcRevW );
    NtSetValueKey( env_key, &valueW, 0, REG_SZ, revW, (strlenW(revW)+1)*sizeof(WCHAR) );

    NtClose( env_key );
}

static inline void get_cpuinfo( SYSTEM_INFO *info )
{
    unsigned int regs[4], regs2[4];

    if (!have_cpuid()) return;

    do_cpuid(0x00000000, regs);  /* get standard cpuid level and vendor name */
    if (regs[0]>=0x00000001)   /* Check for supported cpuid version */
    {
        do_cpuid(0x00000001, regs2); /* get cpu features */
        switch ((regs2[0] >> 8) & 0xf)  /* cpu family */
        {
        case 3:
            info->dwProcessorType = PROCESSOR_INTEL_386;
            info->wProcessorLevel = 3;
            break;
        case 4:
            info->dwProcessorType = PROCESSOR_INTEL_486;
            info->wProcessorLevel = 4;
            break;
        case 5:
            info->dwProcessorType = PROCESSOR_INTEL_PENTIUM;
            info->wProcessorLevel = 5;
            break;
        case 6:
        case 15: /* PPro/2/3/4 has same info as P1 */
            info->dwProcessorType = PROCESSOR_INTEL_PENTIUM;
            info->wProcessorLevel = 6;
            break;
        default:
            FIXME("unknown cpu family %d, please report! (-> setting to 386)\n",
                  (regs2[0] >> 8)&0xf);
            break;
        }
        PF[PF_FLOATING_POINT_EMULATED]     = !(regs2[3] & 1);
        PF[PF_RDTSC_INSTRUCTION_AVAILABLE] = (regs2[3] & (1 << 4 )) >> 4;
        PF[PF_COMPARE_EXCHANGE_DOUBLE]     = (regs2[3] & (1 << 8 )) >> 8;
        PF[PF_MMX_INSTRUCTIONS_AVAILABLE]  = (regs2[3] & (1 << 23)) >> 23;

        if (regs[1] == AUTH &&
            regs[3] == ENTI &&
            regs[2] == CAMD) {
            do_cpuid(0x80000000, regs);  /* get vendor cpuid level */
            if (regs[0]>=0x80000001) {
                do_cpuid(0x80000001, regs2);  /* get vendor features */
                PF[PF_3DNOW_INSTRUCTIONS_AVAILABLE] = (regs2[3] & (1 << 31 )) >> 31;
            }
        }
    }
}

/****************************************************************************
 *		QueryPerformanceCounter (KERNEL32.@)
 *
 * Get the current value of the performance counter.
 * 
 * PARAMS
 *  counter [O] Destination for the current counter reading
 *
 * RETURNS
 *  Success: TRUE. counter contains the current reading
 *  Failure: FALSE.
 *
 * SEE ALSO
 *  See QueryPerformanceFrequency.
 */
BOOL WINAPI QueryPerformanceCounter(PLARGE_INTEGER counter)
{
    NtQueryPerformanceCounter( counter, NULL );
    return TRUE;
}


/****************************************************************************
 *		QueryPerformanceFrequency (KERNEL32.@)
 *
 * Get the resolution of the performace counter.
 *
 * PARAMS
 *  frequency [O] Destination for the counter resolution
 *
 * RETURNS
 *  Success. TRUE. Frequency contains the resolution of the counter.
 *  Failure: FALSE.
 *
 * SEE ALSO
 *  See QueryPerformanceCounter.
 */
BOOL WINAPI QueryPerformanceFrequency(PLARGE_INTEGER frequency)
{
    LARGE_INTEGER counter;
    NtQueryPerformanceCounter( &counter, frequency );
    return TRUE;
}


/***********************************************************************
 * 			GetSystemInfo            	[KERNEL32.@]
 *
 * Get information about the system.
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 * On the first call it creates cached values, so it doesn't have to determine
 * them repeatedly. On Linux, the "/proc/cpuinfo" special file is used.
 *
 * It creates a registry subhierarchy, looking like:
 * "\HARDWARE\DESCRIPTION\System\CentralProcessor\<processornumber>\Identifier (CPU x86)".
 * Note that there is a hierarchy for every processor installed, so this
 * supports multiprocessor systems. This is done like Win95 does it, I think.
 *
 * It creates some registry entries in the environment part:
 * "\HKLM\System\CurrentControlSet\Control\Session Manager\Environment". These are
 * always present. When deleted, Windows will add them again.
 *
 * It also creates a cached flag array for IsProcessorFeaturePresent().
 */
VOID WINAPI GetSystemInfo(
	LPSYSTEM_INFO si	/* [out] Destination for system information, may not be NULL */)
{
	static int cache = 0;
	static SYSTEM_INFO cachedsi;

	TRACE("si=0x%p\n", si);
	if (cache) {
		memcpy(si,&cachedsi,sizeof(*si));
		return;
	}
	memset(PF,0,sizeof(PF));

	/* choose sensible defaults ...
	 * FIXME: perhaps overridable with precompiler flags?
	 */
	cachedsi.u.s.wProcessorArchitecture     = PROCESSOR_ARCHITECTURE_INTEL;
	cachedsi.dwPageSize 			= getpagesize();

	/* FIXME: the two entries below should be computed somehow... */
	cachedsi.lpMinimumApplicationAddress	= (void *)0x00010000;
	cachedsi.lpMaximumApplicationAddress	= (void *)0x7FFEFFFF;
	cachedsi.dwActiveProcessorMask		= 0;
	cachedsi.dwNumberOfProcessors		= 1;
	cachedsi.dwProcessorType		= PROCESSOR_INTEL_PENTIUM;
	cachedsi.dwAllocationGranularity	= 0x10000;
	cachedsi.wProcessorLevel		= 5; /* 586 */
	cachedsi.wProcessorRevision		= 0;

	cache = 1; /* even if there is no more info, we now have a cache entry */
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
		s = value - 1;
		while ((s >= line) && ((*s == ' ') || (*s == '\t'))) s--;
		*(s + 1) = '\0';

		/* and strip leading spaces from value */
		value += 1;
		while (*value==' ') value++;
		if ((s=strchr(value,'\n')))
			*s='\0';

		if (!strcasecmp(line,"processor")) {
			/* processor number counts up... */
			unsigned int x;

			if (sscanf(value,"%d",&x))
				if (x+1>cachedsi.dwNumberOfProcessors)
					cachedsi.dwNumberOfProcessors=x+1;
			
			continue;
		}
                if (!strcasecmp(line,"model")) {
                        /* First part of wProcessorRevision */
			int	x;

			if (sscanf(value,"%d",&x))
				cachedsi.wProcessorRevision = cachedsi.wProcessorRevision | (x << 8);

			continue;
                }

		/* 2.1 method */
		if (!strcasecmp(line, "cpu family")) {
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
					cachedsi.wProcessorLevel= 6;
					break;
				case 1: /* two-figure levels */
                                    if (value[1] == '5')
                                    {
                                        cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
                                        cachedsi.wProcessorLevel= 6;
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
		if (!strcasecmp(line, "cpu")) {
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
					cachedsi.wProcessorLevel= 6;
					break;
				default:
					FIXME("unknown Linux 2.0 cpu family '%s', please report ! (-> setting to 386)\n", value);
					break;
				}
			}
			continue;
		}
		if (!strcasecmp(line,"stepping")) {
                        /* Second part of wProcessorRevision */
			int	x;

			if (sscanf(value,"%d",&x))
				cachedsi.wProcessorRevision = cachedsi.wProcessorRevision | x;

			continue;
		}
		if (!strcasecmp(line, "cpu MHz")) {
			double cmz;
			if (sscanf( value, "%lf", &cmz ) == 1) {
				/* SYSTEMINFO doesn't have a slot for cpu speed, so store in a global */
				cpuHz = cmz * 1000 * 1000;
			}
			continue;
		}
		if (!strcasecmp(line,"fdiv_bug")) {
			if (!strncasecmp(value,"yes",3))
				PF[PF_FLOATING_POINT_PRECISION_ERRATA] = TRUE;

			continue;
		}
		if (!strcasecmp(line,"fpu")) {
			if (!strncasecmp(value,"no",2))
				PF[PF_FLOATING_POINT_EMULATED] = TRUE;

			continue;
		}
		if (	!strcasecmp(line,"flags")	||
			!strcasecmp(line,"features")) {
			if (strstr(value,"cx8"))
				PF[PF_COMPARE_EXCHANGE_DOUBLE] = TRUE;
			if (strstr(value,"mmx"))
				PF[PF_MMX_INSTRUCTIONS_AVAILABLE] = TRUE;
			if (strstr(value,"tsc"))
				PF[PF_RDTSC_INSTRUCTION_AVAILABLE] = TRUE;
			if (strstr(value,"3dnow"))
				PF[PF_3DNOW_INSTRUCTIONS_AVAILABLE] = TRUE;
			/* This will also catch sse2, but we have sse itself
			 * if we have sse2, so no problem */
			if (strstr(value,"sse"))
				PF[PF_XMMI_INSTRUCTIONS_AVAILABLE] = TRUE;
			if (strstr(value,"sse2"))
				PF[PF_XMMI64_INSTRUCTIONS_AVAILABLE] = TRUE;
			if (strstr(value,"pae"))
				PF[PF_PAE_ENABLED] = TRUE;
			
			continue;
		}
	}
	fclose (f);
	}
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
                            cachedsi.wProcessorLevel= 6;
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
#elif defined(__FreeBSD__)
	{
	int ret, len, num;

        get_cpuinfo( &cachedsi );

        /* Check for OS support of SSE -- Is this used, and should it be sse1 or sse2? */
        /*len = sizeof(num);
          ret = sysctlbyname("hw.instruction_sse", &num, &len, NULL, 0);
          if (!ret)
          PF[PF_XMMI_INSTRUCTIONS_AVAILABLE] = num;*/

	len = sizeof(num);
	ret = sysctlbyname("hw.ncpu", &num, &len, NULL, 0);
	if (!ret)
		cachedsi.dwNumberOfProcessors = num;
	}
#elif defined(__sun)
	{
            int num = sysconf( _SC_NPROCESSORS_ONLN );

            if (num == -1) num = 1;
            get_cpuinfo( &cachedsi );
            cachedsi.dwNumberOfProcessors = num;
	}
#elif defined (__APPLE__)
	{
	size_t valSize;
	unsigned long long longVal;
	int value;
	int cputype;
        char buffer[256];

	valSize = sizeof(int);
	if (sysctlbyname ("hw.optional.floatingpoint", &value, &valSize, NULL, 0) == 0)
	{
	    if (value)
		PF[PF_FLOATING_POINT_EMULATED] = FALSE;
	    else
		PF[PF_FLOATING_POINT_EMULATED] = TRUE;
	}
	valSize = sizeof(int);
	if (sysctlbyname ("hw.ncpu", &value, &valSize, NULL, 0) == 0)
	cachedsi.dwNumberOfProcessors = value;
 
	valSize = sizeof(int);
	if (sysctlbyname ("hw.activecpu", &value, &valSize, NULL, 0) == 0)
	    cachedsi.dwActiveProcessorMask = value;
	    
	valSize = sizeof(int);
	if (sysctlbyname ("hw.cputype", &cputype, &valSize, NULL, 0) == 0)
	{
            switch (cputype)
            {
            case CPU_TYPE_POWERPC:
                cachedsi.u.s.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_PPC;
                valSize = sizeof(int);
                if (sysctlbyname ("hw.cpusubtype", &value, &valSize, NULL, 0) == 0)
                {
			switch (value)
			{
			    case CPU_SUBTYPE_POWERPC_601:
			    case CPU_SUBTYPE_POWERPC_602:
				cachedsi.dwProcessorType = PROCESSOR_PPC_601;
				cachedsi.wProcessorLevel = 1;
				break;
			    case CPU_SUBTYPE_POWERPC_603:
				cachedsi.dwProcessorType = PROCESSOR_PPC_603;
				cachedsi.wProcessorLevel = 3;
				break;
			    case CPU_SUBTYPE_POWERPC_603e:
			    case CPU_SUBTYPE_POWERPC_603ev:
				cachedsi.dwProcessorType = PROCESSOR_PPC_603;
				cachedsi.wProcessorLevel = 6;
				break;
			    case CPU_SUBTYPE_POWERPC_604:
				cachedsi.dwProcessorType = PROCESSOR_PPC_604;
				cachedsi.wProcessorLevel = 4;
				break;
			    case CPU_SUBTYPE_POWERPC_604e:
				cachedsi.dwProcessorType = PROCESSOR_PPC_604;
				cachedsi.wProcessorLevel = 9;
				break;
			    case CPU_SUBTYPE_POWERPC_620:
				cachedsi.dwProcessorType = PROCESSOR_PPC_620;
				cachedsi.wProcessorLevel = 20;
				break;
			    case CPU_SUBTYPE_POWERPC_750:
			    case CPU_SUBTYPE_POWERPC_7400:
			    case CPU_SUBTYPE_POWERPC_7450:
				/* G3/G4 derivate from 603 so ... */
				cachedsi.dwProcessorType = PROCESSOR_PPC_603;
				cachedsi.wProcessorLevel = 6;
				break;
			    case CPU_SUBTYPE_POWERPC_970:
				cachedsi.dwProcessorType = PROCESSOR_PPC_604;
				cachedsi.wProcessorLevel = 9;
				/* :o) PF[PF_ALTIVEC_INSTRUCTIONS_AVAILABLE] ;-) */
				break;
			    default: break;
			}
                }
                break; /* CPU_TYPE_POWERPC */
            case CPU_TYPE_I386:
                cachedsi.u.s.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
                valSize = sizeof(int);
                if (sysctlbyname ("machdep.cpu.family", &value, &valSize, NULL, 0) == 0)
                {
                    cachedsi.wProcessorLevel = value;
                    switch (value)
                    {
                    case 3: cachedsi.dwProcessorType = PROCESSOR_INTEL_386; break;
                    case 4: cachedsi.dwProcessorType = PROCESSOR_INTEL_486; break;
                    default: cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM; break;
                    }
                }
                valSize = sizeof(int);
                if (sysctlbyname ("machdep.cpu.model", &value, &valSize, NULL, 0) == 0)
                    cachedsi.wProcessorRevision = (value << 8);
                valSize = sizeof(int);
                if (sysctlbyname ("machdep.cpu.stepping", &value, &valSize, NULL, 0) == 0)
                    cachedsi.wProcessorRevision |= value;
                valSize = sizeof(buffer);
                if (sysctlbyname ("machdep.cpu.features", buffer, &valSize, NULL, 0) == 0)
                {
                    cachedsi.wProcessorRevision |= value;
                    if (strstr(buffer,"CX8"))  PF[PF_COMPARE_EXCHANGE_DOUBLE] = TRUE;
                    if (strstr(buffer,"MMX"))   PF[PF_MMX_INSTRUCTIONS_AVAILABLE] = TRUE;
                    if (strstr(buffer,"TSC"))   PF[PF_RDTSC_INSTRUCTION_AVAILABLE] = TRUE;
                    if (strstr(buffer,"3DNOW")) PF[PF_3DNOW_INSTRUCTIONS_AVAILABLE] = TRUE;
                    if (strstr(buffer,"SSE"))   PF[PF_XMMI_INSTRUCTIONS_AVAILABLE] = TRUE;
                    if (strstr(buffer,"SSE2"))  PF[PF_XMMI64_INSTRUCTIONS_AVAILABLE] = TRUE;
                    if (strstr(buffer,"PAE"))   PF[PF_PAE_ENABLED] = TRUE;
                }
                break; /* CPU_TYPE_I386 */
            default: break;
            } /* switch (cputype) */
	}
	valSize = sizeof(longVal);
	if (!sysctlbyname("hw.cpufrequency", &longVal, &valSize, NULL, 0))
	    cpuHz = longVal;
	}
#else
	FIXME("not yet supported on this system\n");
#endif
        if (!cachedsi.dwActiveProcessorMask)
            cachedsi.dwActiveProcessorMask = (1 << cachedsi.dwNumberOfProcessors) - 1;

        memcpy(si,&cachedsi,sizeof(*si));

        TRACE("<- CPU arch %d, res'd %d, pagesize %d, minappaddr %p, maxappaddr %p,"
              " act.cpumask %08x, numcpus %d, CPU type %d, allocgran. %d, CPU level %d, CPU rev %d\n",
              si->u.s.wProcessorArchitecture, si->u.s.wReserved, si->dwPageSize,
              si->lpMinimumApplicationAddress, si->lpMaximumApplicationAddress,
              si->dwActiveProcessorMask, si->dwNumberOfProcessors, si->dwProcessorType,
              si->dwAllocationGranularity, si->wProcessorLevel, si->wProcessorRevision);

        create_system_registry_keys( &cachedsi );
        create_env_registry_keys( &cachedsi );
}


/***********************************************************************
 * 			GetNativeSystemInfo            	[KERNEL32.@]
 */
VOID WINAPI GetNativeSystemInfo(
    LPSYSTEM_INFO si	/* [out] Destination for system information, may not be NULL */)
{
    FIXME("(%p) using GetSystemInfo()\n", si);
    GetSystemInfo(si); 
}

/***********************************************************************
 * 			IsProcessorFeaturePresent	[KERNEL32.@]
 *
 * Determine if the cpu supports a given feature.
 * 
 * RETURNS
 *  TRUE, If the processor supports feature,
 *  FALSE otherwise.
 */
BOOL WINAPI IsProcessorFeaturePresent (
	DWORD feature	/* [in] Feature number, (PF_ constants from "winnt.h") */) 
{
  SYSTEM_INFO si;
  GetSystemInfo (&si); /* To ensure the information is loaded and cached */

  if (feature < 64)
    return PF[feature];
  else
    return FALSE;
}
