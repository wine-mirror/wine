/*
 * System information APIs
 *
 * Copyright 1996-1998 Marcus Meissner
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

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include "wine/port.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <time.h>
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
# include <sys/sysctl.h>
#endif
#ifdef HAVE_SYS_UTSNAME_H
# include <sys/utsname.h>
#endif
#ifdef HAVE_MACHINE_CPU_H
# include <machine/cpu.h>
#endif
#ifdef HAVE_SYS_RANDOM_H
# include <sys/random.h>
#endif
#ifdef HAVE_IOKIT_IOKITLIB_H
# include <CoreFoundation/CoreFoundation.h>
# include <IOKit/IOKitLib.h>
# include <IOKit/pwr_mgt/IOPM.h>
# include <IOKit/pwr_mgt/IOPMLib.h>
# include <IOKit/ps/IOPowerSources.h>
#endif
#ifdef __APPLE__
# include <mach/mach.h>
# include <mach/machine.h>
# include <mach/mach_init.h>
# include <mach/mach_host.h>
# include <mach/vm_map.h>
#endif

#define NONAMELESSUNION
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "wine/asm.h"
#include "unix_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);

#include "pshpack1.h"

struct smbios_prologue
{
    BYTE calling_method;
    BYTE major_version;
    BYTE minor_version;
    BYTE revision;
    DWORD length;
};

struct smbios_header
{
    BYTE type;
    BYTE length;
    WORD handle;
};

struct smbios_bios
{
    struct smbios_header hdr;
    BYTE vendor;
    BYTE version;
    WORD start;
    BYTE date;
    BYTE size;
    UINT64 characteristics;
    BYTE characteristics_ext[2];
    BYTE system_bios_major_release;
    BYTE system_bios_minor_release;
    BYTE ec_firmware_major_release;
    BYTE ec_firmware_minor_release;
};

struct smbios_system
{
    struct smbios_header hdr;
    BYTE vendor;
    BYTE product;
    BYTE version;
    BYTE serial;
    BYTE uuid[16];
    BYTE wake_up_type;
    BYTE sku_number;
    BYTE family;
};

struct smbios_board
{
    struct smbios_header hdr;
    BYTE vendor;
    BYTE product;
    BYTE version;
    BYTE serial;
    BYTE asset_tag;
    BYTE feature_flags;
    BYTE location;
    WORD chassis_handle;
    BYTE board_type;
    BYTE num_contained_handles;
};

struct smbios_chassis
{
    struct smbios_header hdr;
    BYTE vendor;
    BYTE type;
    BYTE version;
    BYTE serial;
    BYTE asset_tag;
    BYTE boot_state;
    BYTE power_supply_state;
    BYTE thermal_state;
    BYTE security_status;
    DWORD oem_defined;
    BYTE height;
    BYTE num_power_cords;
    BYTE num_contained_elements;
    BYTE contained_element_rec_length;
};

struct smbios_boot_info
{
    struct smbios_header hdr;
    BYTE reserved[6];
    BYTE boot_status[10];
};

#include "poppack.h"

/* Firmware table providers */
#define ACPI 0x41435049
#define FIRM 0x4649524D
#define RSMB 0x52534D42

SYSTEM_CPU_INFORMATION cpu_info = { 0 };

/*******************************************************************************
 * Architecture specific feature detection for CPUs
 *
 * This a set of mutually exclusive #if define()s each providing its own get_cpuinfo() to be called
 * from init_cpu_info();
 */
#if defined(__i386__) || defined(__x86_64__)

BOOL xstate_compaction_enabled = FALSE;

#define AUTH	0x68747541	/* "Auth" */
#define ENTI	0x69746e65	/* "enti" */
#define CAMD	0x444d4163	/* "cAMD" */

#define GENU	0x756e6547	/* "Genu" */
#define INEI	0x49656e69	/* "ineI" */
#define NTEL	0x6c65746e	/* "ntel" */

static inline void do_cpuid(unsigned int ax, unsigned int cx, unsigned int *p)
{
    __asm__ ("cpuid" : "=a"(p[0]), "=b" (p[1]), "=c"(p[2]), "=d"(p[3]) : "a"(ax), "c"(cx));
}

#ifdef __i386__
extern int have_cpuid(void) DECLSPEC_HIDDEN;
__ASM_GLOBAL_FUNC( have_cpuid,
                   "pushfl\n\t"
                   "pushfl\n\t"
                   "movl (%esp),%ecx\n\t"
                   "xorl $0x00200000,(%esp)\n\t"
                   "popfl\n\t"
                   "pushfl\n\t"
                   "popl %eax\n\t"
                   "popfl\n\t"
                   "xorl %ecx,%eax\n\t"
                   "andl $0x00200000,%eax\n\t"
                   "ret" )
#else
static int have_cpuid(void)
{
    return 1;
}
#endif

/* Detect if a SSE2 processor is capable of Denormals Are Zero (DAZ) mode.
 *
 * This function assumes you have already checked for SSE2/FXSAVE support. */
static inline BOOL have_sse_daz_mode(void)
{
#ifdef __i386__
    /* Intel says we need a zeroed 16-byte aligned buffer */
    char buffer[512 + 16];
    XSAVE_FORMAT *state = (XSAVE_FORMAT *)(((ULONG_PTR)buffer + 15) & ~15);
    memset(buffer, 0, sizeof(buffer));

    __asm__ __volatile__( "fxsave %0" : "=m" (*state) : "m" (*state) );

    return (state->MxCsr_Mask & (1 << 6)) >> 6;
#else /* all x86_64 processors include SSE2 with DAZ mode */
    return TRUE;
#endif
}

static void get_cpuinfo( SYSTEM_CPU_INFORMATION *info )
{
    unsigned int regs[4], regs2[4], regs3[4];

#if defined(__i386__)
    info->ProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
#elif defined(__x86_64__)
    info->ProcessorArchitecture = PROCESSOR_ARCHITECTURE_AMD64;
#endif

    /* We're at least a 386 */
    info->ProcessorFeatureBits = CPU_FEATURE_VME | CPU_FEATURE_X86 | CPU_FEATURE_PGE;
    info->ProcessorLevel = 3;

    if (!have_cpuid()) return;

    do_cpuid( 0x00000000, 0, regs );  /* get standard cpuid level and vendor name */
    if (regs[0]>=0x00000001)   /* Check for supported cpuid version */
    {
        do_cpuid( 0x00000001, 0, regs2 ); /* get cpu features */
        if (regs2[3] & (1 << 3 )) info->ProcessorFeatureBits |= CPU_FEATURE_PSE;
        if (regs2[3] & (1 << 4 )) info->ProcessorFeatureBits |= CPU_FEATURE_TSC;
        if (regs2[3] & (1 << 6 )) info->ProcessorFeatureBits |= CPU_FEATURE_PAE;
        if (regs2[3] & (1 << 8 )) info->ProcessorFeatureBits |= CPU_FEATURE_CX8;
        if (regs2[3] & (1 << 11)) info->ProcessorFeatureBits |= CPU_FEATURE_SEP;
        if (regs2[3] & (1 << 12)) info->ProcessorFeatureBits |= CPU_FEATURE_MTRR;
        if (regs2[3] & (1 << 15)) info->ProcessorFeatureBits |= CPU_FEATURE_CMOV;
        if (regs2[3] & (1 << 16)) info->ProcessorFeatureBits |= CPU_FEATURE_PAT;
        if (regs2[3] & (1 << 23)) info->ProcessorFeatureBits |= CPU_FEATURE_MMX;
        if (regs2[3] & (1 << 24)) info->ProcessorFeatureBits |= CPU_FEATURE_FXSR;
        if (regs2[3] & (1 << 25)) info->ProcessorFeatureBits |= CPU_FEATURE_SSE;
        if (regs2[3] & (1 << 26)) info->ProcessorFeatureBits |= CPU_FEATURE_SSE2;
        if (regs2[2] & (1 << 0 )) info->ProcessorFeatureBits |= CPU_FEATURE_SSE3;
        if (regs2[2] & (1 << 9 )) info->ProcessorFeatureBits |= CPU_FEATURE_SSSE3;
        if (regs2[2] & (1 << 13)) info->ProcessorFeatureBits |= CPU_FEATURE_CX128;
        if (regs2[2] & (1 << 19)) info->ProcessorFeatureBits |= CPU_FEATURE_SSE41;
        if (regs2[2] & (1 << 20)) info->ProcessorFeatureBits |= CPU_FEATURE_SSE42;
        if (regs2[2] & (1 << 27)) info->ProcessorFeatureBits |= CPU_FEATURE_XSAVE;
        if (regs2[2] & (1 << 28)) info->ProcessorFeatureBits |= CPU_FEATURE_AVX;
        if((regs2[3] & (1 << 26)) && (regs2[3] & (1 << 24)) && have_sse_daz_mode()) /* has SSE2 and FXSAVE/FXRSTOR */
            info->ProcessorFeatureBits |= CPU_FEATURE_DAZ;

        if (regs[0] >= 0x00000007)
        {
            do_cpuid( 0x00000007, 0, regs3 ); /* get extended features */
            if (regs3[1] & (1 << 5)) info->ProcessorFeatureBits |= CPU_FEATURE_AVX2;
        }

        if (info->ProcessorFeatureBits & CPU_FEATURE_XSAVE)
        {
            do_cpuid( 0x0000000d, 1, regs3 ); /* get XSAVE details */
            if (regs3[0] & 2) xstate_compaction_enabled = TRUE;
        }

        if (regs[1] == AUTH && regs[3] == ENTI && regs[2] == CAMD)
        {
            info->ProcessorLevel = (regs2[0] >> 8) & 0xf; /* family */
            if (info->ProcessorLevel == 0xf)  /* AMD says to add the extended family to the family if family is 0xf */
                info->ProcessorLevel += (regs2[0] >> 20) & 0xff;

            /* repack model and stepping to make a "revision" */
            info->ProcessorRevision  = ((regs2[0] >> 16) & 0xf) << 12; /* extended model */
            info->ProcessorRevision |= ((regs2[0] >> 4 ) & 0xf) << 8;  /* model          */
            info->ProcessorRevision |= regs2[0] & 0xf;                 /* stepping       */

            do_cpuid( 0x80000000, 0, regs );  /* get vendor cpuid level */
            if (regs[0] >= 0x80000001)
            {
                do_cpuid( 0x80000001, 0, regs2 );  /* get vendor features */
                if (regs2[2] & (1 << 2))   info->ProcessorFeatureBits |= CPU_FEATURE_VIRT;
                if (regs2[3] & (1 << 20))  info->ProcessorFeatureBits |= CPU_FEATURE_NX;
                if (regs2[3] & (1 << 27))  info->ProcessorFeatureBits |= CPU_FEATURE_TSC;
                if (regs2[3] & (1u << 31)) info->ProcessorFeatureBits |= CPU_FEATURE_3DNOW;
            }
        }
        else if (regs[1] == GENU && regs[3] == INEI && regs[2] == NTEL)
        {
            info->ProcessorLevel = ((regs2[0] >> 8) & 0xf) + ((regs2[0] >> 20) & 0xff); /* family + extended family */
            if(info->ProcessorLevel == 15) info->ProcessorLevel = 6;

            /* repack model and stepping to make a "revision" */
            info->ProcessorRevision  = ((regs2[0] >> 16) & 0xf) << 12; /* extended model */
            info->ProcessorRevision |= ((regs2[0] >> 4 ) & 0xf) << 8;  /* model          */
            info->ProcessorRevision |= regs2[0] & 0xf;                 /* stepping       */

            if(regs2[2] & (1 << 5))  info->ProcessorFeatureBits |= CPU_FEATURE_VIRT;
            if(regs2[3] & (1 << 21)) info->ProcessorFeatureBits |= CPU_FEATURE_DS;

            do_cpuid( 0x80000000, 0, regs );  /* get vendor cpuid level */
            if (regs[0] >= 0x80000001)
            {
                do_cpuid( 0x80000001, 0, regs2 );  /* get vendor features */
                if (regs2[3] & (1 << 20)) info->ProcessorFeatureBits |= CPU_FEATURE_NX;
                if (regs2[3] & (1 << 27)) info->ProcessorFeatureBits |= CPU_FEATURE_TSC;
            }
        }
        else
        {
            info->ProcessorLevel = (regs2[0] >> 8) & 0xf; /* family */

            /* repack model and stepping to make a "revision" */
            info->ProcessorRevision = ((regs2[0] >> 4 ) & 0xf) << 8;  /* model    */
            info->ProcessorRevision |= regs2[0] & 0xf;                /* stepping */
        }
    }
}

#elif defined(__arm__)

static inline void get_cpuinfo( SYSTEM_CPU_INFORMATION *info )
{
#ifdef linux
    char line[512];
    char *s, *value;
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (f)
    {
        while (fgets( line, sizeof(line), f ))
        {
            /* NOTE: the ':' is the only character we can rely on */
            if (!(value = strchr(line,':'))) continue;
            /* terminate the valuename */
            s = value - 1;
            while ((s >= line) && (*s == ' ' || *s == '\t')) s--;
            s[1] = 0;
            /* and strip leading spaces from value */
            value += 1;
            while (*value == ' ' || *value == '\t') value++;
            if ((s = strchr( value,'\n' ))) *s = 0;
            if (!strcmp( line, "CPU architecture" ))
            {
                info->ProcessorLevel = atoi(value);
                continue;
            }
            if (!strcmp( line, "CPU revision" ))
            {
                info->ProcessorRevision = atoi(value);
                continue;
            }
            if (!strcmp( line, "Features" ))
            {
                if (strstr(value, "crc32")) info->ProcessorFeatureBits |= CPU_FEATURE_ARM_V8_CRC32;
                if (strstr(value, "aes"))   info->ProcessorFeatureBits |= CPU_FEATURE_ARM_V8_CRYPTO;
                continue;
            }
        }
        fclose( f );
    }
#elif defined(__FreeBSD__)
    size_t valsize;
    char buf[8];
    int value;

    valsize = sizeof(buf);
    if (!sysctlbyname("hw.machine_arch", &buf, &valsize, NULL, 0) && sscanf(buf, "armv%i", &value) == 1)
        info->ProcessorLevel = value;

    valsize = sizeof(value);
    if (!sysctlbyname("hw.floatingpoint", &value, &valsize, NULL, 0))
        info->ProcessorFeatureBits |= CPU_FEATURE_ARM_VFP_32;
#else
    FIXME("CPU Feature detection not implemented.\n");
#endif
    info->ProcessorArchitecture = PROCESSOR_ARCHITECTURE_ARM;
}

#elif defined(__aarch64__)

static void get_cpuinfo( SYSTEM_CPU_INFORMATION *info )
{
#ifdef linux
    char line[512];
    char *s, *value;
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (f)
    {
        while (fgets( line, sizeof(line), f ))
        {
            /* NOTE: the ':' is the only character we can rely on */
            if (!(value = strchr(line,':'))) continue;
            /* terminate the valuename */
            s = value - 1;
            while ((s >= line) && (*s == ' ' || *s == '\t')) s--;
            s[1] = 0;
            /* and strip leading spaces from value */
            value += 1;
            while (*value == ' ' || *value == '\t') value++;
            if ((s = strchr( value,'\n' ))) *s = 0;
            if (!strcmp( line, "CPU architecture" ))
            {
                info->ProcessorLevel = atoi(value);
                continue;
            }
            if (!strcmp( line, "CPU revision" ))
            {
                info->ProcessorRevision = atoi(value);
                continue;
            }
            if (!strcmp( line, "Features" ))
            {
                if (strstr(value, "crc32")) info->ProcessorFeatureBits |= CPU_FEATURE_ARM_V8_CRC32;
                if (strstr(value, "aes"))   info->ProcessorFeatureBits |= CPU_FEATURE_ARM_V8_CRYPTO;
                continue;
            }
        }
        fclose( f );
    }
#else
    FIXME("CPU Feature detection not implemented.\n");
#endif
    info->ProcessorLevel = max(info->ProcessorLevel, 8);
    info->ProcessorArchitecture = PROCESSOR_ARCHITECTURE_ARM64;
}

#endif /* End architecture specific feature detection for CPUs */

/******************************************************************
 *		init_cpu_info
 *
 * inits a couple of places with CPU related information:
 * - cpu_info in this file
 * - Peb->NumberOfProcessors
 * - SharedUserData->ProcessFeatures[] array
 */
void init_cpu_info(void)
{
    long num;

#ifdef _SC_NPROCESSORS_ONLN
    num = sysconf(_SC_NPROCESSORS_ONLN);
    if (num < 1)
    {
        num = 1;
        WARN("Failed to detect the number of processors.\n");
    }
#elif defined(CTL_HW) && defined(HW_NCPU)
    int mib[2];
    size_t len = sizeof(num);
    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    if (sysctl(mib, 2, &num, &len, NULL, 0) != 0)
    {
        num = 1;
        WARN("Failed to detect the number of processors.\n");
    }
#else
    num = 1;
    FIXME("Detecting the number of processors is not supported.\n");
#endif
    peb->NumberOfProcessors = num;
    get_cpuinfo( &cpu_info );
    TRACE( "<- CPU arch %d, level %d, rev %d, features 0x%x\n",
           cpu_info.ProcessorArchitecture, cpu_info.ProcessorLevel, cpu_info.ProcessorRevision,
           cpu_info.ProcessorFeatureBits );
}

static BOOL grow_logical_proc_buf( SYSTEM_LOGICAL_PROCESSOR_INFORMATION **pdata, DWORD *max_len )
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION *new_data;

    *max_len *= 2;
    if (!(new_data = realloc( *pdata, *max_len*sizeof(*new_data) ))) return FALSE;
    *pdata = new_data;
    return TRUE;
}

static BOOL grow_logical_proc_ex_buf( SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **pdataex, DWORD *max_len )
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *new_dataex;
    DWORD new_len = *max_len * 2;
    if (!(new_dataex = realloc( *pdataex, new_len * sizeof(*new_dataex) ))) return FALSE;
    memset( new_dataex + *max_len, 0, (new_len - *max_len) * sizeof(*new_dataex) );
    *pdataex = new_dataex;
    *max_len = new_len;
    return TRUE;
}

static DWORD log_proc_ex_size_plus(DWORD size)
{
    /* add SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX.Relationship and .Size */
    return sizeof(LOGICAL_PROCESSOR_RELATIONSHIP) + sizeof(DWORD) + size;
}

static DWORD count_bits(ULONG_PTR mask)
{
    DWORD count = 0;
    while (mask > 0)
    {
        if (mask & 1) ++count;
        mask >>= 1;
    }
    return count;
}

/* Store package and core information for a logical processor. Parsing of processor
 * data may happen in multiple passes; the 'id' parameter is then used to locate
 * previously stored data. The type of data stored in 'id' depends on 'rel':
 * - RelationProcessorPackage: package id ('CPU socket').
 * - RelationProcessorCore: physical core number.
 */
static BOOL logical_proc_info_add_by_id( SYSTEM_LOGICAL_PROCESSOR_INFORMATION **pdata,
                                         SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **pdataex, DWORD *len,
                                         DWORD *pmax_len, LOGICAL_PROCESSOR_RELATIONSHIP rel,
                                         DWORD id, ULONG_PTR mask )
{
    if (pdata)
    {
        DWORD i;

        for (i = 0; i < *len; i++)
        {
            if (rel == RelationProcessorPackage && (*pdata)[i].Relationship == rel && (*pdata)[i].u.Reserved[1] == id)
            {
                (*pdata)[i].ProcessorMask |= mask;
                return TRUE;
            }
            else if (rel == RelationProcessorCore && (*pdata)[i].Relationship == rel && (*pdata)[i].u.Reserved[1] == id)
                return TRUE;
        }

        while (*len == *pmax_len)
        {
            if (!grow_logical_proc_buf(pdata, pmax_len)) return FALSE;
        }

        (*pdata)[i].Relationship = rel;
        (*pdata)[i].ProcessorMask = mask;
        if (rel == RelationProcessorCore)
            (*pdata)[i].u.ProcessorCore.Flags = count_bits(mask) > 1 ? LTP_PC_SMT : 0;
        (*pdata)[i].u.Reserved[0] = 0;
        (*pdata)[i].u.Reserved[1] = id;
        *len = i+1;
    }
    else
    {
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *dataex;
        DWORD ofs = 0;

        while (ofs < *len)
        {
            dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(((char *)*pdataex) + ofs);
            if (rel == RelationProcessorPackage && dataex->Relationship == rel && dataex->u.Processor.Reserved[1] == id)
            {
                dataex->u.Processor.GroupMask[0].Mask |= mask;
                return TRUE;
            }
            else if (rel == RelationProcessorCore && dataex->Relationship == rel && dataex->u.Processor.Reserved[1] == id)
            {
                return TRUE;
            }
            ofs += dataex->Size;
        }

        /* TODO: For now, just one group. If more than 64 processors, then we
         * need another group. */

        while (ofs + log_proc_ex_size_plus(sizeof(PROCESSOR_RELATIONSHIP)) > *pmax_len)
        {
            if (!grow_logical_proc_ex_buf(pdataex, pmax_len)) return FALSE;
        }

        dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(((char *)*pdataex) + ofs);

        dataex->Relationship = rel;
        dataex->Size = log_proc_ex_size_plus(sizeof(PROCESSOR_RELATIONSHIP));
        if (rel == RelationProcessorCore)
            dataex->u.Processor.Flags = count_bits(mask) > 1 ? LTP_PC_SMT : 0;
        else
            dataex->u.Processor.Flags = 0;
        dataex->u.Processor.EfficiencyClass = 0;
        dataex->u.Processor.GroupCount = 1;
        dataex->u.Processor.GroupMask[0].Mask = mask;
        dataex->u.Processor.GroupMask[0].Group = 0;
        /* mark for future lookup */
        dataex->u.Processor.Reserved[0] = 0;
        dataex->u.Processor.Reserved[1] = id;

        *len += dataex->Size;
    }

    return TRUE;
}

static BOOL logical_proc_info_add_cache( SYSTEM_LOGICAL_PROCESSOR_INFORMATION **pdata,
                                         SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **pdataex, DWORD *len,
                                         DWORD *pmax_len, ULONG_PTR mask, CACHE_DESCRIPTOR *cache )
{
    if (pdata)
    {
        DWORD i;

        for (i = 0; i < *len; i++)
        {
            if ((*pdata)[i].Relationship==RelationCache && (*pdata)[i].ProcessorMask==mask
                    && (*pdata)[i].u.Cache.Level==cache->Level && (*pdata)[i].u.Cache.Type==cache->Type)
                return TRUE;
        }

        while (*len == *pmax_len)
            if (!grow_logical_proc_buf(pdata, pmax_len)) return FALSE;

        (*pdata)[i].Relationship = RelationCache;
        (*pdata)[i].ProcessorMask = mask;
        (*pdata)[i].u.Cache = *cache;
        *len = i+1;
    }
    else
    {
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *dataex;
        DWORD ofs;

        for (ofs = 0; ofs < *len; )
        {
            dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(((char *)*pdataex) + ofs);
            if (dataex->Relationship == RelationCache && dataex->u.Cache.GroupMask.Mask == mask &&
                    dataex->u.Cache.Level == cache->Level && dataex->u.Cache.Type == cache->Type)
                return TRUE;
            ofs += dataex->Size;
        }

        while (ofs + log_proc_ex_size_plus(sizeof(CACHE_RELATIONSHIP)) > *pmax_len)
        {
            if (!grow_logical_proc_ex_buf(pdataex, pmax_len)) return FALSE;
        }

        dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(((char *)*pdataex) + ofs);

        dataex->Relationship = RelationCache;
        dataex->Size = log_proc_ex_size_plus(sizeof(CACHE_RELATIONSHIP));
        dataex->u.Cache.Level = cache->Level;
        dataex->u.Cache.Associativity = cache->Associativity;
        dataex->u.Cache.LineSize = cache->LineSize;
        dataex->u.Cache.CacheSize = cache->Size;
        dataex->u.Cache.Type = cache->Type;
        dataex->u.Cache.GroupMask.Mask = mask;
        dataex->u.Cache.GroupMask.Group = 0;

        *len += dataex->Size;
    }

    return TRUE;
}

static BOOL logical_proc_info_add_numa_node( SYSTEM_LOGICAL_PROCESSOR_INFORMATION **pdata,
                                             SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **pdataex, DWORD *len,
                                             DWORD *pmax_len, ULONG_PTR mask, DWORD node_id )
{
    if (pdata)
    {
        while (*len == *pmax_len)
            if (!grow_logical_proc_buf(pdata, pmax_len)) return FALSE;

        (*pdata)[*len].Relationship = RelationNumaNode;
        (*pdata)[*len].ProcessorMask = mask;
        (*pdata)[*len].u.NumaNode.NodeNumber = node_id;
        (*len)++;
    }
    else
    {
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *dataex;

        while (*len + log_proc_ex_size_plus(sizeof(NUMA_NODE_RELATIONSHIP)) > *pmax_len)
        {
            if (!grow_logical_proc_ex_buf(pdataex, pmax_len)) return FALSE;
        }

        dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(((char *)*pdataex) + *len);

        dataex->Relationship = RelationNumaNode;
        dataex->Size = log_proc_ex_size_plus(sizeof(NUMA_NODE_RELATIONSHIP));
        dataex->u.NumaNode.NodeNumber = node_id;
        dataex->u.NumaNode.GroupMask.Mask = mask;
        dataex->u.NumaNode.GroupMask.Group = 0;

        *len += dataex->Size;
    }

    return TRUE;
}

static BOOL logical_proc_info_add_group( SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **pdataex,
                                         DWORD *len, DWORD *pmax_len, DWORD num_cpus, ULONG_PTR mask )
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *dataex;

    while (*len + log_proc_ex_size_plus(sizeof(GROUP_RELATIONSHIP)) > *pmax_len)
        if (!grow_logical_proc_ex_buf(pdataex, pmax_len)) return FALSE;

    dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(((char *)*pdataex) + *len);

    dataex->Relationship = RelationGroup;
    dataex->Size = log_proc_ex_size_plus(sizeof(GROUP_RELATIONSHIP));
    dataex->u.Group.MaximumGroupCount = 1;
    dataex->u.Group.ActiveGroupCount = 1;
    dataex->u.Group.GroupInfo[0].MaximumProcessorCount = num_cpus;
    dataex->u.Group.GroupInfo[0].ActiveProcessorCount = num_cpus;
    dataex->u.Group.GroupInfo[0].ActiveProcessorMask = mask;

    *len += dataex->Size;
    return TRUE;
}

#ifdef linux

/* Helper function for counting bitmap values as commonly used by the Linux kernel
 * for storing CPU masks in sysfs. The format is comma separated lists of hex values
 * each max 32-bit e.g. "00ff" or even "00,00000000,0000ffff".
 *
 * Example files include:
 * - /sys/devices/system/cpu/cpu0/cache/index0/shared_cpu_map
 * - /sys/devices/system/cpu/cpu0/topology/thread_siblings
 */
static BOOL sysfs_parse_bitmap(const char *filename, ULONG_PTR *mask)
{
    FILE *f;
    DWORD r;

    f = fopen(filename, "r");
    if (!f) return FALSE;

    while (!feof(f))
    {
        char op;
        if (!fscanf(f, "%x%c ", &r, &op)) break;
        *mask = (sizeof(ULONG_PTR)>sizeof(int) ? *mask << (8 * sizeof(DWORD)) : 0) + r;
    }
    fclose( f );
    return TRUE;
}

/* Helper function for counting number of elements in interval lists as used by
 * the Linux kernel. The format is comma separated list of intervals of which
 * each interval has the format of "begin-end" where begin and end are decimal
 * numbers. E.g. "0-7", "0-7,16-23"
 *
 * Example files include:
 * - /sys/devices/system/cpu/online
 * - /sys/devices/system/cpu/cpu0/cache/index0/shared_cpu_list
 * - /sys/devices/system/cpu/cpu0/topology/thread_siblings_list.
 */
static BOOL sysfs_count_list_elements(const char *filename, DWORD *result)
{
    FILE *f;

    f = fopen(filename, "r");
    if (!f) return FALSE;

    while (!feof(f))
    {
        char op;
        DWORD beg, end;

        if (!fscanf(f, "%u%c ", &beg, &op)) break;
        if(op == '-')
            fscanf(f, "%u%c ", &end, &op);
        else
            end = beg;

        *result += end - beg + 1;
    }
    fclose( f );
    return TRUE;
}

/* for 'data', max_len is the array count. for 'dataex', max_len is in bytes */
static NTSTATUS create_logical_proc_info( SYSTEM_LOGICAL_PROCESSOR_INFORMATION **data,
                                          SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **dataex,
                                          DWORD *max_len, DWORD relation )
{
    static const char core_info[] = "/sys/devices/system/cpu/cpu%u/topology/%s";
    static const char cache_info[] = "/sys/devices/system/cpu/cpu%u/cache/index%u/%s";
    static const char numa_info[] = "/sys/devices/system/node/node%u/cpumap";

    FILE *fcpu_list, *fnuma_list, *f;
    DWORD len = 0, beg, end, i, j, r, num_cpus = 0, max_cpus = 0;
    char op, name[MAX_PATH];
    ULONG_PTR all_cpus_mask = 0;

    /* On systems with a large number of CPU cores (32 or 64 depending on 32-bit or 64-bit),
     * we have issues parsing processor information:
     * - ULONG_PTR masks as used in data structures can't hold all cores. Requires splitting
     *   data appropriately into "processor groups". We are hard coding 1.
     * - Thread affinity code in wineserver and our CPU parsing code here work independently.
     *   So far the Windows mask applied directly to Linux, but process groups break that.
     *   (NUMA systems you may have multiple non-full groups.)
     */
    if(sysfs_count_list_elements("/sys/devices/system/cpu/present", &max_cpus) && max_cpus > MAXIMUM_PROCESSORS)
    {
        FIXME("Improve CPU info reporting: system supports %u logical cores, but only %u supported!\n",
                max_cpus, MAXIMUM_PROCESSORS);
    }

    fcpu_list = fopen("/sys/devices/system/cpu/online", "r");
    if (!fcpu_list) return STATUS_NOT_IMPLEMENTED;

    while (!feof(fcpu_list))
    {
        if (!fscanf(fcpu_list, "%u%c ", &beg, &op)) break;
        if (op == '-') fscanf(fcpu_list, "%u%c ", &end, &op);
        else end = beg;

        for(i = beg; i <= end; i++)
        {
            DWORD phys_core = 0;
            ULONG_PTR thread_mask = 0;

            if (i > 8*sizeof(ULONG_PTR))
            {
                FIXME("skipping logical processor %d\n", i);
                continue;
            }

            if (relation == RelationAll || relation == RelationProcessorPackage)
            {
                sprintf(name, core_info, i, "physical_package_id");
                f = fopen(name, "r");
                if (f)
                {
                    fscanf(f, "%u", &r);
                    fclose(f);
                }
                else r = 0;
                if (!logical_proc_info_add_by_id(data, dataex, &len, max_len, RelationProcessorPackage, r, (ULONG_PTR)1 << i))
                {
                    fclose(fcpu_list);
                    return STATUS_NO_MEMORY;
                }
            }

            /* Sysfs enumerates logical cores (and not physical cores), but Windows enumerates
             * by physical core. Upon enumerating a logical core in sysfs, we register a physical
             * core and all its logical cores. In order to not report physical cores multiple
             * times, we pass a unique physical core ID to logical_proc_info_add_by_id and let
             * that call figure out any duplication.
             * Obtain a unique physical core ID from the first element of thread_siblings_list.
             * This list provides logical cores sharing the same physical core. The IDs are based
             * on kernel cpu core numbering as opposed to a hardware core ID like provided through
             * 'core_id', so are suitable as a unique ID.
             */
            if(relation == RelationAll || relation == RelationProcessorCore ||
               relation == RelationNumaNode || relation == RelationGroup)
            {
                /* Mask of logical threads sharing same physical core in kernel core numbering. */
                sprintf(name, core_info, i, "thread_siblings");
                if(!sysfs_parse_bitmap(name, &thread_mask)) thread_mask = 1<<i;

                /* Needed later for NumaNode and Group. */
                all_cpus_mask |= thread_mask;

                if (relation == RelationAll || relation == RelationProcessorCore)
                {
                    sprintf(name, core_info, i, "thread_siblings_list");
                    f = fopen(name, "r");
                    if (f)
                    {
                        fscanf(f, "%d%c", &phys_core, &op);
                        fclose(f);
                    }
                    else phys_core = i;

                    if (!logical_proc_info_add_by_id(data, dataex, &len, max_len, RelationProcessorCore, phys_core, thread_mask))
                    {
                        fclose(fcpu_list);
                        return STATUS_NO_MEMORY;
                    }
                }
            }

            if (relation == RelationAll || relation == RelationCache)
            {
                for(j = 0; j < 4; j++)
                {
                    CACHE_DESCRIPTOR cache;
                    ULONG_PTR mask = 0;

                    sprintf(name, cache_info, i, j, "shared_cpu_map");
                    if(!sysfs_parse_bitmap(name, &mask)) continue;

                    sprintf(name, cache_info, i, j, "level");
                    f = fopen(name, "r");
                    if(!f) continue;
                    fscanf(f, "%u", &r);
                    fclose(f);
                    cache.Level = r;

                    sprintf(name, cache_info, i, j, "ways_of_associativity");
                    f = fopen(name, "r");
                    if(!f) continue;
                    fscanf(f, "%u", &r);
                    fclose(f);
                    cache.Associativity = r;

                    sprintf(name, cache_info, i, j, "coherency_line_size");
                    f = fopen(name, "r");
                    if(!f) continue;
                    fscanf(f, "%u", &r);
                    fclose(f);
                    cache.LineSize = r;

                    sprintf(name, cache_info, i, j, "size");
                    f = fopen(name, "r");
                    if(!f) continue;
                    fscanf(f, "%u%c", &r, &op);
                    fclose(f);
                    if(op != 'K')
                        WARN("unknown cache size %u%c\n", r, op);
                    cache.Size = (op=='K' ? r*1024 : r);

                    sprintf(name, cache_info, i, j, "type");
                    f = fopen(name, "r");
                    if(!f) continue;
                    fscanf(f, "%s", name);
                    fclose(f);
                    if (!memcmp(name, "Data", 5))
                        cache.Type = CacheData;
                    else if(!memcmp(name, "Instruction", 11))
                        cache.Type = CacheInstruction;
                    else
                        cache.Type = CacheUnified;

                    if (!logical_proc_info_add_cache(data, dataex, &len, max_len, mask, &cache))
                    {
                        fclose(fcpu_list);
                        return STATUS_NO_MEMORY;
                    }
                }
            }
        }
    }
    fclose(fcpu_list);

    num_cpus = count_bits(all_cpus_mask);

    if(relation == RelationAll || relation == RelationNumaNode)
    {
        fnuma_list = fopen("/sys/devices/system/node/online", "r");
        if (!fnuma_list)
        {
            if (!logical_proc_info_add_numa_node(data, dataex, &len, max_len, all_cpus_mask, 0))
                return STATUS_NO_MEMORY;
        }
        else
        {
            while (!feof(fnuma_list))
            {
                if (!fscanf(fnuma_list, "%u%c ", &beg, &op))
                    break;
                if (op == '-') fscanf(fnuma_list, "%u%c ", &end, &op);
                else end = beg;

                for (i = beg; i <= end; i++)
                {
                    ULONG_PTR mask = 0;

                    sprintf(name, numa_info, i);
                    if (!sysfs_parse_bitmap( name, &mask )) continue;

                    if (!logical_proc_info_add_numa_node(data, dataex, &len, max_len, mask, i))
                    {
                        fclose(fnuma_list);
                        return STATUS_NO_MEMORY;
                    }
                }
            }
            fclose(fnuma_list);
        }
    }

    if(dataex && (relation == RelationAll || relation == RelationGroup))
        logical_proc_info_add_group(dataex, &len, max_len, num_cpus, all_cpus_mask);

    if(data)
        *max_len = len * sizeof(**data);
    else
        *max_len = len;

    return STATUS_SUCCESS;
}

#elif defined(__APPLE__)

/* for 'data', max_len is the array count. for 'dataex', max_len is in bytes */
static NTSTATUS create_logical_proc_info( SYSTEM_LOGICAL_PROCESSOR_INFORMATION **data,
                                          SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **dataex,
                                          DWORD *max_len, DWORD relation)
{
    DWORD pkgs_no, cores_no, lcpu_no, lcpu_per_core, cores_per_package, assoc, len = 0;
    DWORD cache_ctrs[10] = {0};
    ULONG_PTR all_cpus_mask = 0;
    CACHE_DESCRIPTOR cache[10];
    LONGLONG cache_size, cache_line_size, cache_sharing[10];
    size_t size;
    DWORD p,i,j,k;

    if (relation != RelationAll)
        FIXME("Relationship filtering not implemented: 0x%x\n", relation);

    lcpu_no = peb->NumberOfProcessors;

    size = sizeof(pkgs_no);
    if (sysctlbyname("hw.packages", &pkgs_no, &size, NULL, 0))
        pkgs_no = 1;

    size = sizeof(cores_no);
    if (sysctlbyname("hw.physicalcpu", &cores_no, &size, NULL, 0))
        cores_no = lcpu_no;

    TRACE("%u logical CPUs from %u physical cores across %u packages\n",
            lcpu_no, cores_no, pkgs_no);

    lcpu_per_core = lcpu_no / cores_no;
    cores_per_package = cores_no / pkgs_no;

    memset(cache, 0, sizeof(cache));
    cache[1].Level = 1;
    cache[1].Type = CacheInstruction;
    cache[1].Associativity = 8; /* reasonable default */
    cache[1].LineSize = 0x40; /* reasonable default */
    cache[2].Level = 1;
    cache[2].Type = CacheData;
    cache[2].Associativity = 8;
    cache[2].LineSize = 0x40;
    cache[3].Level = 2;
    cache[3].Type = CacheUnified;
    cache[3].Associativity = 8;
    cache[3].LineSize = 0x40;
    cache[4].Level = 3;
    cache[4].Type = CacheUnified;
    cache[4].Associativity = 12;
    cache[4].LineSize = 0x40;

    size = sizeof(cache_line_size);
    if (!sysctlbyname("hw.cachelinesize", &cache_line_size, &size, NULL, 0))
    {
        for (i = 1; i < 5; i++) cache[i].LineSize = cache_line_size;
    }

    /* TODO: set actual associativity for all caches */
    size = sizeof(assoc);
    if (!sysctlbyname("machdep.cpu.cache.L2_associativity", &assoc, &size, NULL, 0))
        cache[3].Associativity = assoc;

    size = sizeof(cache_size);
    if (!sysctlbyname("hw.l1icachesize", &cache_size, &size, NULL, 0))
        cache[1].Size = cache_size;
    size = sizeof(cache_size);
    if (!sysctlbyname("hw.l1dcachesize", &cache_size, &size, NULL, 0))
        cache[2].Size = cache_size;
    size = sizeof(cache_size);
    if (!sysctlbyname("hw.l2cachesize", &cache_size, &size, NULL, 0))
        cache[3].Size = cache_size;
    size = sizeof(cache_size);
    if (!sysctlbyname("hw.l3cachesize", &cache_size, &size, NULL, 0))
        cache[4].Size = cache_size;

    size = sizeof(cache_sharing);
    if (sysctlbyname("hw.cacheconfig", cache_sharing, &size, NULL, 0) < 0)
    {
        cache_sharing[1] = lcpu_per_core;
        cache_sharing[2] = lcpu_per_core;
        cache_sharing[3] = lcpu_per_core;
        cache_sharing[4] = lcpu_no;
    }
    else
    {
        /* in cache[], indexes 1 and 2 are l1 caches */
        cache_sharing[4] = cache_sharing[3];
        cache_sharing[3] = cache_sharing[2];
        cache_sharing[2] = cache_sharing[1];
    }

    for(p = 0; p < pkgs_no; ++p)
    {
        for(j = 0; j < cores_per_package && p * cores_per_package + j < cores_no; ++j)
        {
            ULONG_PTR mask = 0;
            DWORD phys_core;

            for(k = 0; k < lcpu_per_core; ++k) mask |= (ULONG_PTR)1 << (j * lcpu_per_core + k);

            all_cpus_mask |= mask;

            /* add to package */
            if(!logical_proc_info_add_by_id(data, dataex, &len, max_len, RelationProcessorPackage, p, mask))
                return STATUS_NO_MEMORY;

            /* add new core */
            phys_core = p * cores_per_package + j;
            if(!logical_proc_info_add_by_id(data, dataex, &len, max_len, RelationProcessorCore, phys_core, mask))
                return STATUS_NO_MEMORY;

            for(i = 1; i < 5; ++i)
            {
                if(cache_ctrs[i] == 0 && cache[i].Size > 0)
                {
                    mask = 0;
                    for(k = 0; k < cache_sharing[i]; ++k)
                        mask |= (ULONG_PTR)1 << (j * lcpu_per_core + k);

                    if(!logical_proc_info_add_cache(data, dataex, &len, max_len, mask, &cache[i]))
                        return STATUS_NO_MEMORY;
                }

                cache_ctrs[i] += lcpu_per_core;
                if(cache_ctrs[i] == cache_sharing[i]) cache_ctrs[i] = 0;
            }
        }
    }

    /* OSX doesn't support NUMA, so just make one NUMA node for all CPUs */
    if(!logical_proc_info_add_numa_node(data, dataex, &len, max_len, all_cpus_mask, 0))
        return STATUS_NO_MEMORY;

    if(dataex) logical_proc_info_add_group(dataex, &len, max_len, lcpu_no, all_cpus_mask);

    if(data)
        *max_len = len * sizeof(**data);
    else
        *max_len = len;

    return STATUS_SUCCESS;
}

#else

static NTSTATUS create_logical_proc_info( SYSTEM_LOGICAL_PROCESSOR_INFORMATION **data,
                                          SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **dataex,
                                          DWORD *max_len, DWORD relation )
{
    FIXME("stub\n");
    return STATUS_NOT_IMPLEMENTED;
}
#endif

static NTSTATUS create_cpuset_info(SYSTEM_CPU_SET_INFORMATION *info)
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *proc_info;
    BYTE core_index, cache_index, max_cache_level;
    unsigned int i, j, count;
    BYTE *proc_info_buffer;
    DWORD cpu_info_size;
    ULONG64 cpu_mask;
    NTSTATUS status;

    count = peb->NumberOfProcessors;

    cpu_info_size = 3 * sizeof(*proc_info);
    if (!(proc_info_buffer = malloc(cpu_info_size)))
        return STATUS_NO_MEMORY;

    if ((status = create_logical_proc_info(NULL, (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **)&proc_info_buffer,
            &cpu_info_size, RelationAll)))
    {
        free(proc_info_buffer);
        return status;
    }

    max_cache_level = 0;
    proc_info = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)proc_info_buffer;
    for (i = 0; (BYTE *)proc_info != proc_info_buffer + cpu_info_size; ++i)
    {
        if (proc_info->Relationship == RelationCache)
        {
            if (max_cache_level < proc_info->u.Cache.Level)
                max_cache_level = proc_info->u.Cache.Level;
        }
        proc_info = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)((BYTE *)proc_info + proc_info->Size);
    }

    memset(info, 0, count * sizeof(*info));

    core_index = 0;
    cache_index = 0;
    proc_info = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)proc_info_buffer;
    for (i = 0; i < count; ++i)
    {
        info[i].Size = sizeof(*info);
        info[i].Type = CpuSetInformation;
        info[i].u.CpuSet.Id = 0x100 + i;
        info[i].u.CpuSet.LogicalProcessorIndex = i;
    }

    for (i = 0; (BYTE *)proc_info != (BYTE *)proc_info_buffer + cpu_info_size; ++i)
    {
        if (proc_info->Relationship == RelationProcessorCore)
        {
            if (proc_info->u.Processor.GroupCount != 1)
            {
                FIXME("Unsupported group count %u.\n", proc_info->u.Processor.GroupCount);
                continue;
            }
            cpu_mask = proc_info->u.Processor.GroupMask[0].Mask;
            for (j = 0; j < count; ++j)
                if (((ULONG64)1 << j) & cpu_mask)
                {
                    info[j].u.CpuSet.CoreIndex = core_index;
                    info[j].u.CpuSet.EfficiencyClass = proc_info->u.Processor.EfficiencyClass;
                }
            ++core_index;
        }
        else if (proc_info->Relationship == RelationCache)
        {
            if (proc_info->u.Cache.Level == max_cache_level)
            {
                cpu_mask = proc_info->u.Cache.GroupMask.Mask;
                for (j = 0; j < count; ++j)
                    if (((ULONG64)1 << j) & cpu_mask)
                        info[j].u.CpuSet.LastLevelCacheIndex = cache_index;
            }
            ++cache_index;
        }
        else if (proc_info->Relationship == RelationNumaNode)
        {
            cpu_mask = proc_info->u.NumaNode.GroupMask.Mask;
            for (j = 0; j < count; ++j)
                if (((ULONG64)1 << j) & cpu_mask)
                    info[j].u.CpuSet.NumaNodeIndex = proc_info->u.NumaNode.NodeNumber;
        }
        proc_info = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)((BYTE *)proc_info + proc_info->Size);
    }

    free(proc_info_buffer);

    return STATUS_SUCCESS;
}

#ifdef linux

static void copy_smbios_string( char **buffer, char *s, size_t len )
{
    if (!len) return;
    memcpy(*buffer, s, len + 1);
    *buffer += len + 1;
}

static size_t get_smbios_string( const char *path, char *str, size_t size )
{
    FILE *file;
    size_t len;

    if (!(file = fopen(path, "r"))) return 0;

    len = fread( str, 1, size - 1, file );
    fclose( file );

    if (len >= 1 && str[len - 1] == '\n') len--;
    str[len] = 0;
    return len;
}

static void get_system_uuid( GUID *uuid )
{
    static const unsigned char hex[] =
    {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        /* 0x00 */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        /* 0x10 */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        /* 0x20 */
        0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,        /* 0x30 */
        0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,  /* 0x40 */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        /* 0x50 */
        0,10,11,12,13,14,15                     /* 0x60 */
    };
    int fd;

    memset( uuid, 0xff, sizeof(*uuid) );
    if ((fd = open( "/var/lib/dbus/machine-id", O_RDONLY )) != -1)
    {
        unsigned char buf[32], *p = buf;
        if (read( fd, buf, sizeof(buf) ) == sizeof(buf))
        {
            uuid->Data1 = hex[p[6]] << 28 | hex[p[7]] << 24 | hex[p[4]] << 20 | hex[p[5]] << 16 |
                          hex[p[2]] << 12 | hex[p[3]] << 8  | hex[p[0]] << 4  | hex[p[1]];

            uuid->Data2 = hex[p[10]] << 12 | hex[p[11]] << 8 | hex[p[8]]  << 4 | hex[p[9]];
            uuid->Data3 = hex[p[14]] << 12 | hex[p[15]] << 8 | hex[p[12]] << 4 | hex[p[13]];

            uuid->Data4[0] = hex[p[16]] << 4 | hex[p[17]];
            uuid->Data4[1] = hex[p[18]] << 4 | hex[p[19]];
            uuid->Data4[2] = hex[p[20]] << 4 | hex[p[21]];
            uuid->Data4[3] = hex[p[22]] << 4 | hex[p[23]];
            uuid->Data4[4] = hex[p[24]] << 4 | hex[p[25]];
            uuid->Data4[5] = hex[p[26]] << 4 | hex[p[27]];
            uuid->Data4[6] = hex[p[28]] << 4 | hex[p[29]];
            uuid->Data4[7] = hex[p[30]] << 4 | hex[p[31]];
        }
        close( fd );
    }
}

static NTSTATUS get_firmware_info( SYSTEM_FIRMWARE_TABLE_INFORMATION *sfti, ULONG available_len,
                                   ULONG *required_len )
{
    switch (sfti->ProviderSignature)
    {
    case RSMB:
    {
        char bios_vendor[128], bios_version[128], bios_date[128];
        size_t bios_vendor_len, bios_version_len, bios_date_len;
        char system_vendor[128], system_product[128], system_version[128], system_serial[128];
        size_t system_vendor_len, system_product_len, system_version_len, system_serial_len;
        char system_sku[128], system_family[128];
        size_t system_sku_len, system_family_len;
        char board_vendor[128], board_product[128], board_version[128], board_serial[128], board_asset_tag[128];
        size_t board_vendor_len, board_product_len, board_version_len, board_serial_len, board_asset_tag_len;
        char chassis_vendor[128], chassis_version[128], chassis_serial[128], chassis_asset_tag[128];
        char chassis_type[11] = "2"; /* unknown */
        size_t chassis_vendor_len, chassis_version_len, chassis_serial_len, chassis_asset_tag_len;
        char *buffer = (char*)sfti->TableBuffer;
        BYTE string_count;
        BYTE handle_count = 0;
        struct smbios_prologue *prologue;
        struct smbios_bios *bios;
        struct smbios_system *system;
        struct smbios_board *board;
        struct smbios_chassis *chassis;
        struct smbios_boot_info *boot_info;
        struct smbios_header *end_of_table;

#define S(s) s, sizeof(s)
        bios_vendor_len = get_smbios_string("/sys/class/dmi/id/bios_vendor", S(bios_vendor));
        bios_version_len = get_smbios_string("/sys/class/dmi/id/bios_version", S(bios_version));
        bios_date_len = get_smbios_string("/sys/class/dmi/id/bios_date", S(bios_date));
        system_vendor_len = get_smbios_string("/sys/class/dmi/id/sys_vendor", S(system_vendor));
        system_product_len = get_smbios_string("/sys/class/dmi/id/product_name", S(system_product));
        system_version_len = get_smbios_string("/sys/class/dmi/id/product_version", S(system_version));
        system_serial_len = get_smbios_string("/sys/class/dmi/id/product_serial", S(system_serial));
        system_sku_len = get_smbios_string("/sys/class/dmi/id/product_sku", S(system_sku));
        system_family_len = get_smbios_string("/sys/class/dmi/id/product_family", S(system_family));
        board_vendor_len = get_smbios_string("/sys/class/dmi/id/board_vendor", S(board_vendor));
        board_product_len = get_smbios_string("/sys/class/dmi/id/board_name", S(board_product));
        board_version_len = get_smbios_string("/sys/class/dmi/id/board_version", S(board_version));
        board_serial_len = get_smbios_string("/sys/class/dmi/id/board_serial", S(board_serial));
        board_asset_tag_len = get_smbios_string("/sys/class/dmi/id/board_asset_tag", S(board_asset_tag));
        chassis_vendor_len = get_smbios_string("/sys/class/dmi/id/chassis_vendor", S(chassis_vendor));
        chassis_version_len = get_smbios_string("/sys/class/dmi/id/chassis_version", S(chassis_version));
        chassis_serial_len = get_smbios_string("/sys/class/dmi/id/chassis_serial", S(chassis_serial));
        chassis_asset_tag_len = get_smbios_string("/sys/class/dmi/id/chassis_tag", S(chassis_asset_tag));
        get_smbios_string("/sys/class/dmi/id/chassis_type", S(chassis_type));
#undef S

        *required_len = sizeof(struct smbios_prologue);

#define L(l) (l + (l ? 1 : 0))
        *required_len += sizeof(struct smbios_bios);
        *required_len += max(L(bios_vendor_len) + L(bios_version_len) + L(bios_date_len) + 1, 2);

        *required_len += sizeof(struct smbios_system);
        *required_len += max(L(system_vendor_len) + L(system_product_len) + L(system_version_len) +
                             L(system_serial_len) + L(system_sku_len) + L(system_family_len) + 1, 2);

        *required_len += sizeof(struct smbios_board);
        *required_len += max(L(board_vendor_len) + L(board_product_len) + L(board_version_len) +
                             L(board_serial_len) + L(board_asset_tag_len) + 1, 2);

        *required_len += sizeof(struct smbios_chassis);
        *required_len += max(L(chassis_vendor_len) + L(chassis_version_len) + L(chassis_serial_len) +
                             L(chassis_asset_tag_len) + 1, 2);

        *required_len += sizeof(struct smbios_boot_info);
        *required_len += 2;

        *required_len += sizeof(struct smbios_header);
        *required_len += 2;
#undef L

        sfti->TableBufferLength = *required_len;

        *required_len += FIELD_OFFSET(SYSTEM_FIRMWARE_TABLE_INFORMATION, TableBuffer);

        if (available_len < *required_len)
            return STATUS_BUFFER_TOO_SMALL;

        prologue = (struct smbios_prologue*)buffer;
        prologue->calling_method = 0;
        prologue->major_version = 2;
        prologue->minor_version = 4;
        prologue->revision = 0;
        prologue->length = sfti->TableBufferLength - sizeof(struct smbios_prologue);
        buffer += sizeof(struct smbios_prologue);

        string_count = 0;
        bios = (struct smbios_bios*)buffer;
        bios->hdr.type = 0;
        bios->hdr.length = sizeof(struct smbios_bios);
        bios->hdr.handle = handle_count++;
        bios->vendor = bios_vendor_len ? ++string_count : 0;
        bios->version = bios_version_len ? ++string_count : 0;
        bios->start = 0;
        bios->date = bios_date_len ? ++string_count : 0;
        bios->size = 0;
        bios->characteristics = 0x4; /* not supported */
        bios->characteristics_ext[0] = 0;
        bios->characteristics_ext[1] = 0;
        bios->system_bios_major_release = 0xFF; /* not supported */
        bios->system_bios_minor_release = 0xFF; /* not supported */
        bios->ec_firmware_major_release = 0xFF; /* not supported */
        bios->ec_firmware_minor_release = 0xFF; /* not supported */
        buffer += sizeof(struct smbios_bios);

        copy_smbios_string(&buffer, bios_vendor, bios_vendor_len);
        copy_smbios_string(&buffer, bios_version, bios_version_len);
        copy_smbios_string(&buffer, bios_date, bios_date_len);
        if (!string_count) *buffer++ = 0;
        *buffer++ = 0;

        string_count = 0;
        system = (struct smbios_system*)buffer;
        system->hdr.type = 1;
        system->hdr.length = sizeof(struct smbios_system);
        system->hdr.handle = handle_count++;
        system->vendor = system_vendor_len ? ++string_count : 0;
        system->product = system_product_len ? ++string_count : 0;
        system->version = system_version_len ? ++string_count : 0;
        system->serial = system_serial_len ? ++string_count : 0;
        get_system_uuid( (GUID *)system->uuid );
        system->wake_up_type = 0x02; /* unknown */
        system->sku_number = system_sku_len ? ++string_count : 0;
        system->family = system_family_len ? ++string_count : 0;
        buffer += sizeof(struct smbios_system);

        copy_smbios_string(&buffer, system_vendor, system_vendor_len);
        copy_smbios_string(&buffer, system_product, system_product_len);
        copy_smbios_string(&buffer, system_version, system_version_len);
        copy_smbios_string(&buffer, system_serial, system_serial_len);
        copy_smbios_string(&buffer, system_sku, system_sku_len);
        copy_smbios_string(&buffer, system_family, system_family_len);
        if (!string_count) *buffer++ = 0;
        *buffer++ = 0;

        string_count = 0;
        chassis = (struct smbios_chassis*)buffer;
        chassis->hdr.type = 3;
        chassis->hdr.length = sizeof(struct smbios_chassis);
        chassis->hdr.handle = handle_count++;
        chassis->vendor = chassis_vendor_len ? ++string_count : 0;
        chassis->type = atoi(chassis_type);
        chassis->version = chassis_version_len ? ++string_count : 0;
        chassis->serial = chassis_serial_len ? ++string_count : 0;
        chassis->asset_tag = chassis_asset_tag_len ? ++string_count : 0;
        chassis->boot_state = 0x02; /* unknown */
        chassis->power_supply_state = 0x02; /* unknown */
        chassis->thermal_state = 0x02; /* unknown */
        chassis->security_status = 0x02; /* unknown */
        chassis->oem_defined = 0;
        chassis->height = 0; /* undefined */
        chassis->num_power_cords = 0; /* unspecified */
        chassis->num_contained_elements = 0;
        chassis->contained_element_rec_length = 3;
        buffer += sizeof(struct smbios_chassis);

        copy_smbios_string(&buffer, chassis_vendor, chassis_vendor_len);
        copy_smbios_string(&buffer, chassis_version, chassis_version_len);
        copy_smbios_string(&buffer, chassis_serial, chassis_serial_len);
        copy_smbios_string(&buffer, chassis_asset_tag, chassis_asset_tag_len);
        if (!string_count) *buffer++ = 0;
        *buffer++ = 0;

        string_count = 0;
        board = (struct smbios_board*)buffer;
        board->hdr.type = 2;
        board->hdr.length = sizeof(struct smbios_board);
        board->hdr.handle = handle_count++;
        board->vendor = board_vendor_len ? ++string_count : 0;
        board->product = board_product_len ? ++string_count : 0;
        board->version = board_version_len ? ++string_count : 0;
        board->serial = board_serial_len ? ++string_count : 0;
        board->asset_tag = board_asset_tag_len ? ++string_count : 0;
        board->feature_flags = 0x5; /* hosting board, removable */
        board->location = 0;
        board->chassis_handle = chassis->hdr.handle;
        board->board_type = 0xa; /* motherboard */
        board->num_contained_handles = 0;
        buffer += sizeof(struct smbios_board);

        copy_smbios_string(&buffer, board_vendor, board_vendor_len);
        copy_smbios_string(&buffer, board_product, board_product_len);
        copy_smbios_string(&buffer, board_version, board_version_len);
        copy_smbios_string(&buffer, board_serial, board_serial_len);
        copy_smbios_string(&buffer, board_asset_tag, board_asset_tag_len);
        if (!string_count) *buffer++ = 0;
        *buffer++ = 0;

        boot_info = (struct smbios_boot_info*)buffer;
        boot_info->hdr.type = 32;
        boot_info->hdr.length = sizeof(struct smbios_boot_info);
        boot_info->hdr.handle = handle_count++;
        memset(boot_info->reserved, 0, sizeof(boot_info->reserved));
        memset(boot_info->boot_status, 0, sizeof(boot_info->boot_status)); /* no errors detected */
        buffer += sizeof(struct smbios_boot_info);
        *buffer++ = 0;
        *buffer++ = 0;

        end_of_table = (struct smbios_header*)buffer;
        end_of_table->type = 127;
        end_of_table->length = sizeof(struct smbios_header);
        end_of_table->handle = handle_count++;
        buffer += sizeof(struct smbios_header);
        *buffer++ = 0;
        *buffer++ = 0;

        return STATUS_SUCCESS;
    }
    default:
        FIXME("info_class SYSTEM_FIRMWARE_TABLE_INFORMATION provider %08x\n", sfti->ProviderSignature);
        return STATUS_NOT_IMPLEMENTED;
    }
}

#elif defined(__APPLE__)

static NTSTATUS get_firmware_info( SYSTEM_FIRMWARE_TABLE_INFORMATION *sfti, ULONG available_len,
                                   ULONG *required_len )
{
    switch (sfti->ProviderSignature)
    {
    case RSMB:
    {
        io_service_t service;
        CFDataRef data;
        const UInt8 *ptr;
        CFIndex len;
        struct smbios_prologue *prologue;
        BYTE major_version = 2, minor_version = 0;

        if (!(service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("AppleSMBIOS"))))
        {
            WARN("can't find AppleSMBIOS service\n");
            return STATUS_NO_MEMORY;
        }

        if (!(data = IORegistryEntryCreateCFProperty(service, CFSTR("SMBIOS-EPS"), kCFAllocatorDefault, 0)))
        {
            WARN("can't find SMBIOS entry point\n");
            IOObjectRelease(service);
            return STATUS_NO_MEMORY;
        }

        len = CFDataGetLength(data);
        ptr = CFDataGetBytePtr(data);
        if (len >= 8 && !memcmp(ptr, "_SM_", 4))
        {
            major_version = ptr[6];
            minor_version = ptr[7];
        }
        CFRelease(data);

        if (!(data = IORegistryEntryCreateCFProperty(service, CFSTR("SMBIOS"), kCFAllocatorDefault, 0)))
        {
            WARN("can't find SMBIOS table\n");
            IOObjectRelease(service);
            return STATUS_NO_MEMORY;
        }

        len = CFDataGetLength(data);
        ptr = CFDataGetBytePtr(data);
        sfti->TableBufferLength = sizeof(*prologue) + len;
        *required_len = sfti->TableBufferLength + FIELD_OFFSET(SYSTEM_FIRMWARE_TABLE_INFORMATION, TableBuffer);
        if (available_len < *required_len)
        {
            CFRelease(data);
            IOObjectRelease(service);
            return STATUS_BUFFER_TOO_SMALL;
        }

        prologue = (struct smbios_prologue *)sfti->TableBuffer;
        prologue->calling_method = 0;
        prologue->major_version = major_version;
        prologue->minor_version = minor_version;
        prologue->revision = 0;
        prologue->length = sfti->TableBufferLength - sizeof(*prologue);

        memcpy(sfti->TableBuffer + sizeof(*prologue), ptr, len);

        CFRelease(data);
        IOObjectRelease(service);
        return STATUS_SUCCESS;
    }
    default:
        FIXME("info_class SYSTEM_FIRMWARE_TABLE_INFORMATION provider %08x\n", sfti->ProviderSignature);
        return STATUS_NOT_IMPLEMENTED;
    }
}

#else

static NTSTATUS get_firmware_info( SYSTEM_FIRMWARE_TABLE_INFORMATION *sfti, ULONG available_len,
                                   ULONG *required_len )
{
    FIXME("info_class SYSTEM_FIRMWARE_TABLE_INFORMATION\n");
    sfti->TableBufferLength = 0;
    return STATUS_NOT_IMPLEMENTED;
}

#endif

static void get_performance_info( SYSTEM_PERFORMANCE_INFORMATION *info )
{
    unsigned long long totalram = 0, freeram = 0, totalswap = 0, freeswap = 0;
    FILE *fp;

    memset( info, 0, sizeof(*info) );

    if ((fp = fopen("/proc/uptime", "r")))
    {
        double uptime, idle_time;

        fscanf(fp, "%lf %lf", &uptime, &idle_time);
        fclose(fp);
        info->IdleTime.QuadPart = 10000000 * idle_time;
    }
    else
    {
        static ULONGLONG idle;
        /* many programs expect IdleTime to change so fake change */
        info->IdleTime.QuadPart = ++idle;
    }

#ifdef linux
    if ((fp = fopen("/proc/meminfo", "r")))
    {
        unsigned long long value;
        char line[64];

        while (fgets(line, sizeof(line), fp))
        {
            if(sscanf(line, "MemTotal: %llu kB", &value) == 1)
                totalram += value * 1024;
            else if(sscanf(line, "MemFree: %llu kB", &value) == 1)
                freeram += value * 1024;
            else if(sscanf(line, "SwapTotal: %llu kB", &value) == 1)
                totalswap += value * 1024;
            else if(sscanf(line, "SwapFree: %llu kB", &value) == 1)
                freeswap += value * 1024;
            else if (sscanf(line, "Buffers: %llu", &value))
                freeram += value * 1024;
            else if (sscanf(line, "Cached: %llu", &value))
                freeram += value * 1024;
        }
        fclose(fp);
    }
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__) || defined(__APPLE__)
    {
#ifdef __APPLE__
        unsigned int val;
#else
        unsigned long val;
#endif
        int mib[2];
        size_t size_sys;

        mib[0] = CTL_HW;
#ifdef HW_MEMSIZE
        {
            uint64_t val64;
            mib[1] = HW_MEMSIZE;
            size_sys = sizeof(val64);
            if (!sysctl(mib, 2, &val64, &size_sys, NULL, 0) && size_sys == sizeof(val64)) totalram = val64;
        }
#endif

#ifdef HAVE_MACH_MACH_H
        {
            host_name_port_t host = mach_host_self();
            mach_msg_type_number_t count;
#ifdef HOST_VM_INFO64_COUNT
            vm_statistics64_data_t vm_stat;

            count = HOST_VM_INFO64_COUNT;
            if (host_statistics64(host, HOST_VM_INFO64, (host_info64_t)&vm_stat, &count) == KERN_SUCCESS)
                freeram = (vm_stat.free_count + vm_stat.inactive_count) * (ULONGLONG)page_size;
#endif
            if (!totalram)
            {
                host_basic_info_data_t info;
                count = HOST_BASIC_INFO_COUNT;
                if (host_info(host, HOST_BASIC_INFO, (host_info_t)&info, &count) == KERN_SUCCESS)
                    totalram = info.max_mem;
            }
            mach_port_deallocate(mach_task_self(), host);
        }
#endif

        if (!totalram)
        {
            mib[1] = HW_PHYSMEM;
            size_sys = sizeof(val);
            if (!sysctl(mib, 2, &val, &size_sys, NULL, 0) && size_sys == sizeof(val)) totalram = val;
        }
        if (!freeram)
        {
            mib[1] = HW_USERMEM;
            size_sys = sizeof(val);
            if (!sysctl(mib, 2, &val, &size_sys, NULL, 0) && size_sys == sizeof(val)) freeram = val;
        }
#ifdef VM_SWAPUSAGE
        {
            struct xsw_usage swap;
            mib[0] = CTL_VM;
            mib[1] = VM_SWAPUSAGE;
            size_sys = sizeof(swap);
            if (!sysctl(mib, 2, &swap, &size_sys, NULL, 0) && size_sys == sizeof(swap))
            {
                totalswap = swap.xsu_total;
                freeswap = swap.xsu_avail;
            }
        }
#endif
    }
#endif
    info->AvailablePages      = freeram / page_size;
    info->TotalCommittedPages = (totalram + totalswap - freeram - freeswap) / page_size;
    info->TotalCommitLimit    = (totalram + totalswap) / page_size;
}


/* calculate the mday of dst change date, so that for instance Sun 5 Oct 2007
 * (last Sunday in October of 2007) becomes Sun Oct 28 2007
 *
 * Note: year, day and month must be in unix format.
 */
static int weekday_to_mday(int year, int day, int mon, int day_of_week)
{
    struct tm date;
    time_t tmp;
    int wday, mday;

    /* find first day in the month matching week day of the date */
    memset(&date, 0, sizeof(date));
    date.tm_year = year;
    date.tm_mon = mon;
    date.tm_mday = -1;
    date.tm_wday = -1;
    do
    {
        date.tm_mday++;
        tmp = mktime(&date);
    } while (date.tm_wday != day_of_week || date.tm_mon != mon);

    mday = date.tm_mday;

    /* find number of week days in the month matching week day of the date */
    wday = 1; /* 1 - 1st, ...., 5 - last */
    while (wday < day)
    {
        struct tm *tm;

        date.tm_mday += 7;
        tmp = mktime(&date);
        tm = localtime(&tmp);
        if (tm->tm_mon != mon)
            break;
        mday = tm->tm_mday;
        wday++;
    }

    return mday;
}

static BOOL match_tz_date( const RTL_SYSTEM_TIME *st, const RTL_SYSTEM_TIME *reg_st )
{
    WORD wDay;

    if (st->wMonth != reg_st->wMonth) return FALSE;
    if (!st->wMonth) return TRUE; /* no transition dates */
    wDay = reg_st->wDay;
    if (!reg_st->wYear) /* date in a day-of-week format */
        wDay = weekday_to_mday(st->wYear - 1900, reg_st->wDay, reg_st->wMonth - 1, reg_st->wDayOfWeek);

    return (st->wDay == wDay &&
            st->wHour == reg_st->wHour &&
            st->wMinute == reg_st->wMinute &&
            st->wSecond == reg_st->wSecond &&
            st->wMilliseconds == reg_st->wMilliseconds);
}

static BOOL match_tz_info( const RTL_DYNAMIC_TIME_ZONE_INFORMATION *tzi,
                           const RTL_DYNAMIC_TIME_ZONE_INFORMATION *reg_tzi )
{
    return (tzi->Bias == reg_tzi->Bias &&
            match_tz_date(&tzi->StandardDate, &reg_tzi->StandardDate) &&
            match_tz_date(&tzi->DaylightDate, &reg_tzi->DaylightDate));
}

static BOOL match_past_tz_bias( time_t past_time, LONG past_bias )
{
    LONG bias;
    struct tm *tm;
    if (!past_time) return TRUE;

    tm = gmtime( &past_time );
    bias = (LONG)(mktime(tm) - past_time) / 60;
    return bias == past_bias;
}

static BOOL match_tz_name( const char *tz_name, const RTL_DYNAMIC_TIME_ZONE_INFORMATION *reg_tzi )
{
    static const struct {
        WCHAR key_name[32];
        const char *short_name;
        time_t past_time;
        LONG past_bias;
    }
    mapping[] =
    {
        { {'N','o','r','t','h',' ','K','o','r','e','a',' ','S','t','a','n','d','a','r','d',' ','T','i','m','e',0 },
          "KST", 1451606400 /* 2016-01-01 00:00:00 UTC */, -510 },
        { {'K','o','r','e','a',' ','S','t','a','n','d','a','r','d',' ','T','i','m','e',0 },
          "KST", 1451606400 /* 2016-01-01 00:00:00 UTC */, -540 },
        { {'T','o','k','y','o',' ','S','t','a','n','d','a','r','d',' ','T','i','m','e',0 },
          "JST" },
        { {'Y','a','k','u','t','s','k',' ','S','t','a','n','d','a','r','d',' ','T','i','m','e',0 },
          "+09" }, /* YAKST was used until tzdata 2016f */
    };
    unsigned int i;

    if (reg_tzi->DaylightDate.wMonth) return TRUE;
    for (i = 0; i < ARRAY_SIZE(mapping); i++)
    {
        if (!wcscmp( mapping[i].key_name, reg_tzi->TimeZoneKeyName ))
            return !strcmp( mapping[i].short_name, tz_name )
                && match_past_tz_bias( mapping[i].past_time, mapping[i].past_bias );
    }
    return TRUE;
}

static BOOL reg_query_value( HKEY key, LPCWSTR name, DWORD type, void *data, DWORD count )
{
    char buf[256];
    UNICODE_STRING nameW;
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buf;

    if (count > sizeof(buf) - sizeof(KEY_VALUE_PARTIAL_INFORMATION)) return FALSE;

    nameW.Buffer = (WCHAR *)name;
    nameW.Length = wcslen( name ) * sizeof(WCHAR);
    if (NtQueryValueKey( key, &nameW, KeyValuePartialInformation, buf, sizeof(buf), &count ))
        return FALSE;

    if (info->Type != type) return FALSE;
    memcpy( data, info->Data, info->DataLength );
    return TRUE;
}

static void find_reg_tz_info(RTL_DYNAMIC_TIME_ZONE_INFORMATION *tzi, const char* tz_name, int year)
{
    static const WCHAR stdW[] = { 'S','t','d',0 };
    static const WCHAR dltW[] = { 'D','l','t',0 };
    static const WCHAR mui_stdW[] = { 'M','U','I','_','S','t','d',0 };
    static const WCHAR mui_dltW[] = { 'M','U','I','_','D','l','t',0 };
    static const WCHAR tziW[] = { 'T','Z','I',0 };
    static const WCHAR Time_ZonesW[] = { 'M','a','c','h','i','n','e','\\',
        'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\',
        'W','i','n','d','o','w','s',' ','N','T','\\',
        'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
        'T','i','m','e',' ','Z','o','n','e','s',0 };
    static const WCHAR Dynamic_DstW[] = { 'D','y','n','a','m','i','c',' ','D','S','T',0 };
    RTL_DYNAMIC_TIME_ZONE_INFORMATION reg_tzi;
    HANDLE key, subkey, subkey_dyn = 0;
    ULONG idx, len;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    WCHAR yearW[16];
    char buffer[128];
    KEY_BASIC_INFORMATION *info = (KEY_BASIC_INFORMATION *)buffer;

    sprintf( buffer, "%u", year );
    ascii_to_unicode( yearW, buffer, strlen(buffer) + 1 );
    init_unicode_string( &nameW, Time_ZonesW );
    InitializeObjectAttributes( &attr, &nameW, 0, 0, NULL );
    if (NtOpenKey( &key, KEY_READ, &attr )) return;

    idx = 0;
    while (!NtEnumerateKey( key, idx++, KeyBasicInformation, buffer, sizeof(buffer), &len ))
    {
        struct tz_reg_data
        {
            LONG bias;
            LONG std_bias;
            LONG dlt_bias;
            RTL_SYSTEM_TIME std_date;
            RTL_SYSTEM_TIME dlt_date;
        } tz_data;
        BOOL is_dynamic = FALSE;

        nameW.Buffer = info->Name;
        nameW.Length = info->NameLength;
        attr.RootDirectory = key;
        if (NtOpenKey( &subkey, KEY_READ, &attr )) continue;

        memset( &reg_tzi, 0, sizeof(reg_tzi) );
        memcpy(reg_tzi.TimeZoneKeyName, nameW.Buffer, nameW.Length);
        reg_tzi.TimeZoneKeyName[nameW.Length/sizeof(WCHAR)] = 0;

        if (!reg_query_value(subkey, mui_stdW, REG_SZ, reg_tzi.StandardName, sizeof(reg_tzi.StandardName)) &&
            !reg_query_value(subkey, stdW, REG_SZ, reg_tzi.StandardName, sizeof(reg_tzi.StandardName)))
            goto next;

        if (!reg_query_value(subkey, mui_dltW, REG_SZ, reg_tzi.DaylightName, sizeof(reg_tzi.DaylightName)) &&
            !reg_query_value(subkey, dltW, REG_SZ, reg_tzi.DaylightName, sizeof(reg_tzi.DaylightName)))
            goto next;

        /* Check for Dynamic DST entry first */
        nameW.Buffer = (WCHAR *)Dynamic_DstW;
        nameW.Length = sizeof(Dynamic_DstW) - sizeof(WCHAR);
        attr.RootDirectory = subkey;
        if (!NtOpenKey( &subkey_dyn, KEY_READ, &attr ))
        {
            is_dynamic = reg_query_value( subkey_dyn, yearW, REG_BINARY, &tz_data, sizeof(tz_data) );
            NtClose( subkey_dyn );
        }
        if (!is_dynamic && !reg_query_value( subkey, tziW, REG_BINARY, &tz_data, sizeof(tz_data) ))
            goto next;

        reg_tzi.Bias = tz_data.bias;
        reg_tzi.StandardBias = tz_data.std_bias;
        reg_tzi.DaylightBias = tz_data.dlt_bias;
        reg_tzi.StandardDate = tz_data.std_date;
        reg_tzi.DaylightDate = tz_data.dlt_date;

        TRACE("%s: bias %d\n", debugstr_us(&nameW), reg_tzi.Bias);
        TRACE("std (d/m/y): %u/%02u/%04u day of week %u %u:%02u:%02u.%03u bias %d\n",
              reg_tzi.StandardDate.wDay, reg_tzi.StandardDate.wMonth,
              reg_tzi.StandardDate.wYear, reg_tzi.StandardDate.wDayOfWeek,
              reg_tzi.StandardDate.wHour, reg_tzi.StandardDate.wMinute,
              reg_tzi.StandardDate.wSecond, reg_tzi.StandardDate.wMilliseconds,
              reg_tzi.StandardBias);
        TRACE("dst (d/m/y): %u/%02u/%04u day of week %u %u:%02u:%02u.%03u bias %d\n",
              reg_tzi.DaylightDate.wDay, reg_tzi.DaylightDate.wMonth,
              reg_tzi.DaylightDate.wYear, reg_tzi.DaylightDate.wDayOfWeek,
              reg_tzi.DaylightDate.wHour, reg_tzi.DaylightDate.wMinute,
              reg_tzi.DaylightDate.wSecond, reg_tzi.DaylightDate.wMilliseconds,
              reg_tzi.DaylightBias);

        if (match_tz_info( tzi, &reg_tzi ) && match_tz_name( tz_name, &reg_tzi ))
        {
            *tzi = reg_tzi;
            NtClose( subkey );
            NtClose( key );
            return;
        }
    next:
        NtClose( subkey );
    }
    NtClose( key );

    if (idx == 1) return;  /* registry info not initialized yet */

    FIXME("Can't find matching timezone information in the registry for "
          "%s, bias %d, std (d/m/y): %u/%02u/%04u, dlt (d/m/y): %u/%02u/%04u\n",
          tz_name, tzi->Bias,
          tzi->StandardDate.wDay, tzi->StandardDate.wMonth, tzi->StandardDate.wYear,
          tzi->DaylightDate.wDay, tzi->DaylightDate.wMonth, tzi->DaylightDate.wYear);
}

static time_t find_dst_change(unsigned long min, unsigned long max, int *is_dst)
{
    time_t start;
    struct tm *tm;

    start = min;
    tm = localtime(&start);
    *is_dst = !tm->tm_isdst;
    TRACE("starting date isdst %d, %s", !*is_dst, ctime(&start));

    while (min <= max)
    {
        time_t pos = (min + max) / 2;
        tm = localtime(&pos);

        if (tm->tm_isdst != *is_dst)
            min = pos + 1;
        else
            max = pos - 1;
    }
    return min;
}

static void get_timezone_info( RTL_DYNAMIC_TIME_ZONE_INFORMATION *tzi )
{
    static pthread_mutex_t tz_mutex = PTHREAD_MUTEX_INITIALIZER;
    static RTL_DYNAMIC_TIME_ZONE_INFORMATION cached_tzi;
    static int current_year = -1, current_bias = 65535;
    struct tm *tm;
    char tz_name[16];
    time_t year_start, year_end, tmp, dlt = 0, std = 0;
    int is_dst, bias;

    mutex_lock( &tz_mutex );

    year_start = time(NULL);
    tm = gmtime(&year_start);
    bias = (LONG)(mktime(tm) - year_start) / 60;

    tm = localtime(&year_start);
    if (current_year == tm->tm_year && current_bias == bias)
    {
        *tzi = cached_tzi;
        mutex_unlock( &tz_mutex );
        return;
    }

    memset(tzi, 0, sizeof(*tzi));
    if (!strftime(tz_name, sizeof(tz_name), "%Z", tm)) {
        /* not enough room or another error */
        tz_name[0] = '\0';
    }

    TRACE("tz data will be valid through year %d, bias %d\n", tm->tm_year + 1900, bias);
    current_year = tm->tm_year;
    current_bias = bias;

    tzi->Bias = bias;

    tm->tm_isdst = 0;
    tm->tm_mday = 1;
    tm->tm_mon = tm->tm_hour = tm->tm_min = tm->tm_sec = tm->tm_wday = tm->tm_yday = 0;
    year_start = mktime(tm);
    TRACE("year_start: %s", ctime(&year_start));

    tm->tm_mday = tm->tm_wday = tm->tm_yday = 0;
    tm->tm_mon = 12;
    tm->tm_hour = 23;
    tm->tm_min = tm->tm_sec = 59;
    year_end = mktime(tm);
    TRACE("year_end: %s", ctime(&year_end));

    tmp = find_dst_change(year_start, year_end, &is_dst);
    if (is_dst)
        dlt = tmp;
    else
        std = tmp;

    tmp = find_dst_change(tmp, year_end, &is_dst);
    if (is_dst)
        dlt = tmp;
    else
        std = tmp;

    TRACE("std: %s", ctime(&std));
    TRACE("dlt: %s", ctime(&dlt));

    if (dlt == std || !dlt || !std)
        TRACE("there is no daylight saving rules in this time zone\n");
    else
    {
        tmp = dlt - tzi->Bias * 60;
        tm = gmtime(&tmp);
        TRACE("dlt gmtime: %s", asctime(tm));

        tzi->DaylightBias = -60;
        tzi->DaylightDate.wYear = tm->tm_year + 1900;
        tzi->DaylightDate.wMonth = tm->tm_mon + 1;
        tzi->DaylightDate.wDayOfWeek = tm->tm_wday;
        tzi->DaylightDate.wDay = tm->tm_mday;
        tzi->DaylightDate.wHour = tm->tm_hour;
        tzi->DaylightDate.wMinute = tm->tm_min;
        tzi->DaylightDate.wSecond = tm->tm_sec;
        tzi->DaylightDate.wMilliseconds = 0;

        TRACE("daylight (d/m/y): %u/%02u/%04u day of week %u %u:%02u:%02u.%03u bias %d\n",
            tzi->DaylightDate.wDay, tzi->DaylightDate.wMonth,
            tzi->DaylightDate.wYear, tzi->DaylightDate.wDayOfWeek,
            tzi->DaylightDate.wHour, tzi->DaylightDate.wMinute,
            tzi->DaylightDate.wSecond, tzi->DaylightDate.wMilliseconds,
            tzi->DaylightBias);

        tmp = std - tzi->Bias * 60 - tzi->DaylightBias * 60;
        tm = gmtime(&tmp);
        TRACE("std gmtime: %s", asctime(tm));

        tzi->StandardBias = 0;
        tzi->StandardDate.wYear = tm->tm_year + 1900;
        tzi->StandardDate.wMonth = tm->tm_mon + 1;
        tzi->StandardDate.wDayOfWeek = tm->tm_wday;
        tzi->StandardDate.wDay = tm->tm_mday;
        tzi->StandardDate.wHour = tm->tm_hour;
        tzi->StandardDate.wMinute = tm->tm_min;
        tzi->StandardDate.wSecond = tm->tm_sec;
        tzi->StandardDate.wMilliseconds = 0;

        TRACE("standard (d/m/y): %u/%02u/%04u day of week %u %u:%02u:%02u.%03u bias %d\n",
            tzi->StandardDate.wDay, tzi->StandardDate.wMonth,
            tzi->StandardDate.wYear, tzi->StandardDate.wDayOfWeek,
            tzi->StandardDate.wHour, tzi->StandardDate.wMinute,
            tzi->StandardDate.wSecond, tzi->StandardDate.wMilliseconds,
            tzi->StandardBias);
    }

    find_reg_tz_info(tzi, tz_name, current_year + 1900);
    cached_tzi = *tzi;
    mutex_unlock( &tz_mutex );
}


static void read_dev_urandom( void *buf, ULONG len )
{
    int fd = open( "/dev/urandom", O_RDONLY );
    if (fd != -1)
    {
        int ret;
        do
        {
            ret = read( fd, buf, len );
        }
        while (ret == -1 && errno == EINTR);
        close( fd );
    }
    else WARN( "can't open /dev/urandom\n" );
}

/******************************************************************************
 *              NtQuerySystemInformation  (NTDLL.@)
 */
NTSTATUS WINAPI NtQuerySystemInformation( SYSTEM_INFORMATION_CLASS class,
                                          void *info, ULONG size, ULONG *ret_size )
{
    NTSTATUS ret = STATUS_SUCCESS;
    ULONG len = 0;

    TRACE( "(0x%08x,%p,0x%08x,%p)\n", class, info, size, ret_size );

    switch (class)
    {
    case SystemNativeBasicInformation:  /* 114 */
        if (!is_win64) return STATUS_INVALID_INFO_CLASS;
        /* fall through */
    case SystemBasicInformation:  /* 0 */
    {
        SYSTEM_BASIC_INFORMATION sbi;

        virtual_get_system_info( &sbi, FALSE );
        len = sizeof(sbi);
        if (size == len)
        {
            if (!info) ret = STATUS_ACCESS_VIOLATION;
            else memcpy( info, &sbi, len);
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    }

    case SystemCpuInformation:  /* 1 */
        if (size >= (len = sizeof(cpu_info)))
        {
            if (!info) ret = STATUS_ACCESS_VIOLATION;
            else memcpy(info, &cpu_info, len);
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;

    case SystemPerformanceInformation:  /* 2 */
    {
        SYSTEM_PERFORMANCE_INFORMATION spi;
        static BOOL fixme_written = FALSE;

        get_performance_info( &spi );
        len = sizeof(spi);
        if (size >= len)
        {
            if (!info) ret = STATUS_ACCESS_VIOLATION;
            else memcpy( info, &spi, len);
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        if(!fixme_written) {
            FIXME("info_class SYSTEM_PERFORMANCE_INFORMATION\n");
            fixme_written = TRUE;
        }
        break;
    }

    case SystemTimeOfDayInformation:  /* 3 */
    {
        struct tm *tm;
        time_t now;
        SYSTEM_TIMEOFDAY_INFORMATION sti = {{{ 0 }}};

        sti.BootTime.QuadPart = server_start_time;
        now = time( NULL );
        tm = gmtime( &now );
        sti.TimeZoneBias.QuadPart = mktime( tm ) - now;
        tm = localtime( &now );
        if (tm->tm_isdst) sti.TimeZoneBias.QuadPart -= 3600;
        sti.TimeZoneBias.QuadPart *= TICKSPERSEC;
        NtQuerySystemTime( &sti.SystemTime );

        if (size <= sizeof(sti))
        {
            len = size;
            if (!info) ret = STATUS_ACCESS_VIOLATION;
            else memcpy( info, &sti, size);
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    }

    case SystemProcessInformation:  /* 5 */
    {
        unsigned int process_count, i, j;
        char *buffer = NULL;
        unsigned int pos = 0;

        if (size && !(buffer = malloc( size )))
        {
            ret = STATUS_NO_MEMORY;
            break;
        }

        SERVER_START_REQ( list_processes )
        {
            wine_server_set_reply( req, buffer, size );
            ret = wine_server_call( req );
            len = reply->info_size;
            process_count = reply->process_count;
        }
        SERVER_END_REQ;

        if (ret)
        {
            free( buffer );
            break;
        }

        len = 0;

        for (i = 0; i < process_count; i++)
        {
            SYSTEM_PROCESS_INFORMATION *nt_process = (SYSTEM_PROCESS_INFORMATION *)((char *)info + len);
            const struct process_info *server_process;
            const WCHAR *server_name, *file_part;
            ULONG proc_len;
            ULONG name_len = 0;

            pos = (pos + 7) & ~7;
            server_process = (const struct process_info *)(buffer + pos);
            pos += sizeof(*server_process);

            server_name = (const WCHAR *)(buffer + pos);
            file_part = server_name + (server_process->name_len / sizeof(WCHAR));
            pos += server_process->name_len;
            while (file_part > server_name && file_part[-1] != '\\')
            {
                file_part--;
                name_len++;
            }

            proc_len = sizeof(*nt_process) + server_process->thread_count * sizeof(SYSTEM_THREAD_INFORMATION)
                         + (name_len + 1) * sizeof(WCHAR);
            len += proc_len;

            if (len <= size)
            {
                memset(nt_process, 0, sizeof(*nt_process));
                if (i < process_count - 1)
                    nt_process->NextEntryOffset = proc_len;
                nt_process->CreationTime.QuadPart = server_process->start_time;
                nt_process->dwThreadCount = server_process->thread_count;
                nt_process->dwBasePriority = server_process->priority;
                nt_process->UniqueProcessId = UlongToHandle(server_process->pid);
                nt_process->ParentProcessId = UlongToHandle(server_process->parent_pid);
                nt_process->SessionId = server_process->session_id;
                nt_process->HandleCount = server_process->handle_count;
                get_thread_times( server_process->unix_pid, -1, &nt_process->KernelTime, &nt_process->UserTime );
                fill_vm_counters( &nt_process->vmCounters, server_process->unix_pid );
            }

            pos = (pos + 7) & ~7;
            for (j = 0; j < server_process->thread_count; j++)
            {
                const struct thread_info *server_thread = (const struct thread_info *)(buffer + pos);

                if (len <= size)
                {
                    nt_process->ti[j].CreateTime.QuadPart = server_thread->start_time;
                    nt_process->ti[j].ClientId.UniqueProcess = UlongToHandle(server_process->pid);
                    nt_process->ti[j].ClientId.UniqueThread = UlongToHandle(server_thread->tid);
                    nt_process->ti[j].dwCurrentPriority = server_thread->current_priority;
                    nt_process->ti[j].dwBasePriority = server_thread->base_priority;
                    get_thread_times( server_process->unix_pid, server_thread->unix_tid,
                                      &nt_process->ti[j].KernelTime, &nt_process->ti[j].UserTime );
                }

                pos += sizeof(*server_thread);
            }

            if (len <= size)
            {
                nt_process->ProcessName.Buffer = (WCHAR *)&nt_process->ti[server_process->thread_count];
                nt_process->ProcessName.Length = name_len * sizeof(WCHAR);
                nt_process->ProcessName.MaximumLength = (name_len + 1) * sizeof(WCHAR);
                memcpy(nt_process->ProcessName.Buffer, file_part, name_len * sizeof(WCHAR));
                nt_process->ProcessName.Buffer[name_len] = 0;
            }
        }

        if (len > size) ret = STATUS_INFO_LENGTH_MISMATCH;
        free( buffer );
        break;
    }

    case SystemProcessorPerformanceInformation:  /* 8 */
    {
        SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *sppi = NULL;
        unsigned int cpus = 0;
        int out_cpus = size / sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);

        if (out_cpus == 0)
        {
            len = 0;
            ret = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }
        if (!(sppi = calloc( out_cpus, sizeof(*sppi) )))
        {
            ret = STATUS_NO_MEMORY;
            break;
        }
        else
#ifdef __APPLE__
        {
            processor_cpu_load_info_data_t *pinfo;
            mach_msg_type_number_t info_count;

            if (host_processor_info( mach_host_self (),
                                     PROCESSOR_CPU_LOAD_INFO,
                                     &cpus,
                                     (processor_info_array_t*)&pinfo,
                                     &info_count) == 0)
            {
                int i;
                cpus = min(cpus,out_cpus);
                for (i = 0; i < cpus; i++)
                {
                    sppi[i].IdleTime.QuadPart = pinfo[i].cpu_ticks[CPU_STATE_IDLE];
                    sppi[i].KernelTime.QuadPart = pinfo[i].cpu_ticks[CPU_STATE_SYSTEM];
                    sppi[i].UserTime.QuadPart = pinfo[i].cpu_ticks[CPU_STATE_USER];
                }
                vm_deallocate (mach_task_self (), (vm_address_t) pinfo, info_count * sizeof(natural_t));
            }
        }
#else
        {
            FILE *cpuinfo = fopen("/proc/stat", "r");
            if (cpuinfo)
            {
                unsigned long clk_tck = sysconf(_SC_CLK_TCK);
                unsigned long usr,nice,sys,idle,remainder[8];
                int i, count, id;
                char name[32];
                char line[255];

                /* first line is combined usage */
                while (fgets(line,255,cpuinfo))
                {
                    count = sscanf(line, "%s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                                   name, &usr, &nice, &sys, &idle,
                                   &remainder[0], &remainder[1], &remainder[2], &remainder[3],
                                   &remainder[4], &remainder[5], &remainder[6], &remainder[7]);

                    if (count < 5 || strncmp( name, "cpu", 3 )) break;
                    for (i = 0; i + 5 < count; ++i) sys += remainder[i];
                    sys += idle;
                    usr += nice;
                    id = atoi( name + 3 ) + 1;
                    if (id > out_cpus) break;
                    if (id > cpus) cpus = id;
                    sppi[id-1].IdleTime.QuadPart   = (ULONGLONG)idle * 10000000 / clk_tck;
                    sppi[id-1].KernelTime.QuadPart = (ULONGLONG)sys * 10000000 / clk_tck;
                    sppi[id-1].UserTime.QuadPart   = (ULONGLONG)usr * 10000000 / clk_tck;
                }
                fclose(cpuinfo);
            }
        }
#endif
        if (cpus == 0)
        {
            static int i = 1;
            unsigned int n;
            cpus = min(peb->NumberOfProcessors, out_cpus);
            FIXME("stub info_class SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION\n");
            /* many programs expect these values to change so fake change */
            for (n = 0; n < cpus; n++)
            {
                sppi[n].KernelTime.QuadPart = 1 * i;
                sppi[n].UserTime.QuadPart   = 2 * i;
                sppi[n].IdleTime.QuadPart   = 3 * i;
            }
            i++;
        }

        len = sizeof(*sppi) * cpus;
        if (size >= len)
        {
            if (!info) ret = STATUS_ACCESS_VIOLATION;
            else memcpy( info, sppi, len);
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;

        free( sppi );
        break;
    }

    case SystemModuleInformation:  /* 11 */
    {
        /* FIXME: return some fake info for now */
        static const char *fake_modules[] =
        {
            "\\SystemRoot\\system32\\ntoskrnl.exe",
            "\\SystemRoot\\system32\\hal.dll",
            "\\SystemRoot\\system32\\drivers\\mountmgr.sys"
        };

        ULONG i;
        RTL_PROCESS_MODULES *smi = info;

        len = offsetof( RTL_PROCESS_MODULES, Modules[ARRAY_SIZE(fake_modules)] );
        if (len <= size)
        {
            memset( smi, 0, len );
            for (i = 0; i < ARRAY_SIZE(fake_modules); i++)
            {
                RTL_PROCESS_MODULE_INFORMATION *sm = &smi->Modules[i];
                sm->ImageBaseAddress = (char *)0x10000000 + 0x200000 * i;
                sm->ImageSize = 0x200000;
                sm->LoadOrderIndex = i;
                sm->LoadCount = 1;
                strcpy( (char *)sm->Name, fake_modules[i] );
                sm->NameOffset = strrchr( fake_modules[i], '\\' ) - fake_modules[i] + 1;
            }
            smi->ModulesCount = i;
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;

        break;
    }

    case SystemHandleInformation:  /* 16 */
    {
        struct handle_info *handle_info;
        DWORD i, num_handles;

        if (size < sizeof(SYSTEM_HANDLE_INFORMATION))
        {
            ret = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        if (!info)
        {
            ret = STATUS_ACCESS_VIOLATION;
            break;
        }

        num_handles = (size - FIELD_OFFSET( SYSTEM_HANDLE_INFORMATION, Handle )) / sizeof(SYSTEM_HANDLE_ENTRY);
        if (!(handle_info = malloc( sizeof(*handle_info) * num_handles ))) return STATUS_NO_MEMORY;

        SERVER_START_REQ( get_system_handles )
        {
            wine_server_set_reply( req, handle_info, sizeof(*handle_info) * num_handles );
            if (!(ret = wine_server_call( req )))
            {
                SYSTEM_HANDLE_INFORMATION *shi = info;
                shi->Count = wine_server_reply_size( req ) / sizeof(*handle_info);
                len = FIELD_OFFSET( SYSTEM_HANDLE_INFORMATION, Handle[shi->Count] );
                for (i = 0; i < shi->Count; i++)
                {
                    memset( &shi->Handle[i], 0, sizeof(shi->Handle[i]) );
                    shi->Handle[i].OwnerPid     = handle_info[i].owner;
                    shi->Handle[i].HandleValue  = handle_info[i].handle;
                    shi->Handle[i].AccessMask   = handle_info[i].access;
                    shi->Handle[i].HandleFlags  = handle_info[i].attributes;
                    shi->Handle[i].ObjectType   = handle_info[i].type;
                    /* FIXME: Fill out ObjectPointer */
                }
            }
            else if (ret == STATUS_BUFFER_TOO_SMALL)
            {
                len = FIELD_OFFSET( SYSTEM_HANDLE_INFORMATION, Handle[reply->count] );
                ret = STATUS_INFO_LENGTH_MISMATCH;
            }
        }
        SERVER_END_REQ;

        free( handle_info );
        break;
    }

    case SystemFileCacheInformation:  /* 21 */
    {
        SYSTEM_CACHE_INFORMATION sci = { 0 };

        len = sizeof(sci);
        if (size >= len)
        {
            if (!info) ret = STATUS_ACCESS_VIOLATION;
            else memcpy( info, &sci, len);
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        FIXME("info_class SYSTEM_CACHE_INFORMATION\n");
        break;
    }

    case SystemInterruptInformation: /* 23 */
    {
        len = peb->NumberOfProcessors * sizeof(SYSTEM_INTERRUPT_INFORMATION);
        if (size >= len)
        {
            if (!info) ret = STATUS_ACCESS_VIOLATION;
            else
            {
#ifdef HAVE_GETRANDOM
                int ret;
                do
                {
                    ret = getrandom( info, len, 0 );
                }
                while (ret == -1 && errno == EINTR);

                if (ret == -1 && errno == ENOSYS) read_dev_urandom( info, len );
#else
                read_dev_urandom( info, len );
#endif
            }
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    }

    case SystemTimeAdjustmentInformation:  /* 28 */
    {
        SYSTEM_TIME_ADJUSTMENT_QUERY query = { 156250, 156250, TRUE };

        len = sizeof(query);
        if (size == len)
        {
            if (!info) ret = STATUS_ACCESS_VIOLATION;
            else memcpy( info, &query, len );
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    }

    case SystemKernelDebuggerInformation:  /* 35 */
    {
        SYSTEM_KERNEL_DEBUGGER_INFORMATION skdi;

        skdi.DebuggerEnabled = FALSE;
        skdi.DebuggerNotPresent = TRUE;
        len = sizeof(skdi);
        if (size >= len)
        {
            if (!info) ret = STATUS_ACCESS_VIOLATION;
            else memcpy( info, &skdi, len);
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    }

    case SystemRegistryQuotaInformation:  /* 37 */
    {
        /* Something to do with the size of the registry             *
         * Since we don't have a size limitation, fake it            *
         * This is almost certainly wrong.                           *
         * This sets each of the three words in the struct to 32 MB, *
         * which is enough to make the IE 5 installer happy.         */
        SYSTEM_REGISTRY_QUOTA_INFORMATION srqi;

        srqi.RegistryQuotaAllowed = 0x2000000;
        srqi.RegistryQuotaUsed = 0x200000;
        srqi.Reserved1 = (void*)0x200000;
        len = sizeof(srqi);

        if (size >= len)
        {
            if (!info) ret = STATUS_ACCESS_VIOLATION;
            else
            {
                FIXME("SystemRegistryQuotaInformation: faking max registry size of 32 MB\n");
                memcpy( info, &srqi, len);
            }
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    }

    case SystemCurrentTimeZoneInformation:  /* 44 */
    {
        RTL_DYNAMIC_TIME_ZONE_INFORMATION tz;

        get_timezone_info( &tz );
        len = sizeof(RTL_TIME_ZONE_INFORMATION);
        if (size >= len)
        {
            if (!info) ret = STATUS_ACCESS_VIOLATION;
            else memcpy( info, &tz, len);
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    }

    case SystemExtendedProcessInformation:  /* 57 */
        FIXME("SystemExtendedProcessInformation, size %u, info %p, stub!\n", size, info);
        memset( info, 0, size );
        ret = STATUS_SUCCESS;
        break;

    case SystemRecommendedSharedDataAlignment:  /* 58 */
    {
        len = sizeof(DWORD);
        if (size >= len)
        {
            if (!info) ret = STATUS_ACCESS_VIOLATION;
            else
            {
#ifdef __arm__
                *((DWORD *)info) = 32;
#else
                *((DWORD *)info) = 64;
#endif
            }
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    }

    case SystemEmulationBasicInformation:  /* 62 */
    {
        SYSTEM_BASIC_INFORMATION sbi;

        virtual_get_system_info( &sbi, !!NtCurrentTeb()->WowTebOffset );
        len = sizeof(sbi);
        if (size == len)
        {
            if (!info) ret = STATUS_ACCESS_VIOLATION;
            else memcpy( info, &sbi, len);
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    }

    case SystemEmulationProcessorInformation:  /* 63 */
        if (size >= (len = sizeof(cpu_info)))
        {
            SYSTEM_CPU_INFORMATION cpu = cpu_info;
            if (is_win64)
            {
                if (cpu_info.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
                    cpu.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
                else if (cpu_info.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64)
                    cpu.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_ARM;
            }
            memcpy(info, &cpu, len);
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;

    case SystemExtendedHandleInformation:  /* 64 */
    {
        struct handle_info *handle_info;
        DWORD i, num_handles;

        if (size < sizeof(SYSTEM_HANDLE_INFORMATION_EX))
        {
            ret = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        if (!info)
        {
            ret = STATUS_ACCESS_VIOLATION;
            break;
        }

        num_handles = (size - FIELD_OFFSET( SYSTEM_HANDLE_INFORMATION_EX, Handles ))
                      / sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX);
        if (!(handle_info = malloc( sizeof(*handle_info) * num_handles ))) return STATUS_NO_MEMORY;

        SERVER_START_REQ( get_system_handles )
        {
            wine_server_set_reply( req, handle_info, sizeof(*handle_info) * num_handles );
            if (!(ret = wine_server_call( req )))
            {
                SYSTEM_HANDLE_INFORMATION_EX *shi = info;
                shi->NumberOfHandles = wine_server_reply_size( req ) / sizeof(*handle_info);
                len = FIELD_OFFSET( SYSTEM_HANDLE_INFORMATION_EX, Handles[shi->NumberOfHandles] );
                for (i = 0; i < shi->NumberOfHandles; i++)
                {
                    memset( &shi->Handles[i], 0, sizeof(shi->Handles[i]) );
                    shi->Handles[i].UniqueProcessId  = handle_info[i].owner;
                    shi->Handles[i].HandleValue      = handle_info[i].handle;
                    shi->Handles[i].GrantedAccess    = handle_info[i].access;
                    shi->Handles[i].HandleAttributes = handle_info[i].attributes;
                    shi->Handles[i].ObjectTypeIndex  = handle_info[i].type;
                    /* FIXME: Fill out Object */
                }
            }
            else if (ret == STATUS_BUFFER_TOO_SMALL)
            {
                len = FIELD_OFFSET( SYSTEM_HANDLE_INFORMATION_EX, Handles[reply->count] );
                ret = STATUS_INFO_LENGTH_MISMATCH;
            }
        }
        SERVER_END_REQ;

        free( handle_info );
        break;
    }

    case SystemLogicalProcessorInformation:  /* 73 */
    {
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION *buf;

        /* Each logical processor may use up to 7 entries in returned table:
         * core, numa node, package, L1i, L1d, L2, L3 */
        len = 7 * peb->NumberOfProcessors;
        buf = malloc( len * sizeof(*buf) );
        if (!buf)
        {
            ret = STATUS_NO_MEMORY;
            break;
        }
        ret = create_logical_proc_info(&buf, NULL, &len, RelationAll);
        if (!ret)
        {
            if (size >= len)
            {
                if (!info) ret = STATUS_ACCESS_VIOLATION;
                else memcpy( info, buf, len);
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
        }
        free( buf );
        break;
    }

    case SystemFirmwareTableInformation:  /* 76 */
    {
        SYSTEM_FIRMWARE_TABLE_INFORMATION *sfti = info;
        len = FIELD_OFFSET(SYSTEM_FIRMWARE_TABLE_INFORMATION, TableBuffer);
        if (size < len)
        {
            ret = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        switch (sfti->Action)
        {
        case SystemFirmwareTable_Get:
            ret = get_firmware_info(sfti, size, &len);
            break;
        default:
            len = 0;
            ret = STATUS_NOT_IMPLEMENTED;
            FIXME("info_class SYSTEM_FIRMWARE_TABLE_INFORMATION action %d\n", sfti->Action);
        }
        break;
    }

    case SystemModuleInformationEx:  /* 77 */
    {
        /* FIXME: return some fake info for now */
        static const char *fake_modules[] =
        {
            "\\SystemRoot\\system32\\ntoskrnl.exe",
            "\\SystemRoot\\system32\\hal.dll",
            "\\SystemRoot\\system32\\drivers\\mountmgr.sys"
        };

        ULONG i;
        RTL_PROCESS_MODULE_INFORMATION_EX *module_info = info;

        len = sizeof(*module_info) * ARRAY_SIZE(fake_modules) + sizeof(module_info->NextOffset);
        if (len <= size)
        {
            memset( info, 0, len );
            for (i = 0; i < ARRAY_SIZE(fake_modules); i++)
            {
                RTL_PROCESS_MODULE_INFORMATION *sm = &module_info[i].BaseInfo;
                sm->ImageBaseAddress = (char *)0x10000000 + 0x200000 * i;
                sm->ImageSize = 0x200000;
                sm->LoadOrderIndex = i;
                sm->LoadCount = 1;
                strcpy( (char *)sm->Name, fake_modules[i] );
                sm->NameOffset = strrchr( fake_modules[i], '\\' ) - fake_modules[i] + 1;
                module_info[i].NextOffset = sizeof(*module_info);
            }
            module_info[ARRAY_SIZE(fake_modules)].NextOffset = 0;
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;

        break;
    }

    case SystemDynamicTimeZoneInformation:  /* 102 */
    {
        RTL_DYNAMIC_TIME_ZONE_INFORMATION tz;

        get_timezone_info( &tz );
        len = sizeof(tz);
        if (size >= len)
        {
            if (!info) ret = STATUS_ACCESS_VIOLATION;
            else memcpy( info, &tz, len);
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    }

    case SystemCodeIntegrityInformation:  /* 103 */
    {
        SYSTEM_CODEINTEGRITY_INFORMATION *integrity_info = info;

        FIXME("SystemCodeIntegrityInformation, size %u, info %p, stub!\n", size, info);

        len = sizeof(SYSTEM_CODEINTEGRITY_INFORMATION);

        if (size < len)
            integrity_info->CodeIntegrityOptions = CODEINTEGRITY_OPTION_ENABLED;
        else
            ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    }

    case SystemKernelDebuggerInformationEx:  /* 149 */
    {
        SYSTEM_KERNEL_DEBUGGER_INFORMATION_EX skdi;

        skdi.DebuggerAllowed = FALSE;
        skdi.DebuggerEnabled = FALSE;
        skdi.DebuggerPresent = FALSE;

        len = sizeof(skdi);
        if (size >= len)
        {
            if (!info) ret = STATUS_ACCESS_VIOLATION;
            else memcpy( info, &skdi, len );
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    }

    case SystemCpuSetInformation:  /* 175 */
        return NtQuerySystemInformationEx(class, NULL, 0, info, size, ret_size);

    /* Wine extensions */

    case SystemWineVersionInformation:  /* 1000 */
    {
        static const char version[] = PACKAGE_VERSION;
        extern const char wine_build[];
        struct utsname buf;

        uname( &buf );
        len = strlen(version) + strlen(wine_build) + strlen(buf.sysname) + strlen(buf.release) + 4;
        snprintf( info, size, "%s%c%s%c%s%c%s", version, 0, wine_build, 0, buf.sysname, 0, buf.release );
        if (size < len) ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    }

    default:
	FIXME( "(0x%08x,%p,0x%08x,%p) stub\n", class, info, size, ret_size );

        /* Several Information Classes are not implemented on Windows and return 2 different values
         * STATUS_NOT_IMPLEMENTED or STATUS_INVALID_INFO_CLASS
         * in 95% of the cases it's STATUS_INVALID_INFO_CLASS, so use this as the default
         */
        ret = STATUS_INVALID_INFO_CLASS;
    }

    if (ret_size) *ret_size = len;
    return ret;
}


/******************************************************************************
 *              NtQuerySystemInformationEx  (NTDLL.@)
 */
NTSTATUS WINAPI NtQuerySystemInformationEx( SYSTEM_INFORMATION_CLASS class,
                                            void *query, ULONG query_len,
                                            void *info, ULONG size, ULONG *ret_size )
{
    ULONG len = 0;
    NTSTATUS ret = STATUS_NOT_IMPLEMENTED;

    TRACE( "(0x%08x,%p,%u,%p,%u,%p) stub\n", class, query, query_len, info, size, ret_size );

    switch (class)
    {
    case SystemLogicalProcessorInformationEx:
    {
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buf;

        if (!query || query_len < sizeof(DWORD))
        {
            ret = STATUS_INVALID_PARAMETER;
            break;
        }

        len = 3 * sizeof(*buf);
        if (!(buf = malloc( len )))
        {
            ret = STATUS_NO_MEMORY;
            break;
        }
        ret = create_logical_proc_info(NULL, &buf, &len, *(DWORD *)query);
        if (!ret)
        {
            if (size >= len)
            {
                if (!info) ret = STATUS_ACCESS_VIOLATION;
                else memcpy(info, buf, len);
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
        }
        free( buf );
        break;
    }

    case SystemCpuSetInformation:
    {
        unsigned int cpu_count = peb->NumberOfProcessors;
        PROCESS_BASIC_INFORMATION pbi;
        HANDLE process;

        if (!query || query_len < sizeof(HANDLE))
            return STATUS_INVALID_PARAMETER;

        process = *(HANDLE *)query;
        if (process && (ret = NtQueryInformationProcess(process, ProcessBasicInformation, &pbi, sizeof(pbi), NULL)))
            return ret;

        if (size < (len = cpu_count * sizeof(SYSTEM_CPU_SET_INFORMATION)))
        {
            ret = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        if (!info)
            return STATUS_ACCESS_VIOLATION;

        if ((ret = create_cpuset_info(info)))
            return ret;
        break;
    }

    case SystemSupportedProcessorArchitectures:
    {
        HANDLE process;
        ULONG i;
        USHORT machine = 0;

        if (!query || query_len < sizeof(HANDLE)) return STATUS_INVALID_PARAMETER;
        process = *(HANDLE *)query;
        if (process)
        {
            SERVER_START_REQ( get_process_info )
            {
                req->handle = wine_server_obj_handle( process );
                if (!(ret = wine_server_call( req ))) machine = reply->machine;
            }
            SERVER_END_REQ;
            if (ret) return ret;
        }

        len = (supported_machines_count + 1) * sizeof(ULONG);
        if (size < len)
        {
            ret = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        for (i = 0; i < supported_machines_count; i++)
        {
            USHORT flags = 2;  /* supported (?) */
            if (!i) flags |= 5;  /* native machine (?) */
            if (supported_machines[i] == machine) flags |= 8;  /* current machine */
            ((DWORD *)info)[i] = MAKELONG( supported_machines[i], flags );
        }
        ((DWORD *)info)[i] = 0;
        ret = STATUS_SUCCESS;
        break;
    }

    default:
        FIXME( "(0x%08x,%p,%u,%p,%u,%p) stub\n", class, query, query_len, info, size, ret_size );
        break;
    }
    if (ret_size) *ret_size = len;
    return ret;
}


/******************************************************************************
 *              NtSetSystemInformation  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetSystemInformation( SYSTEM_INFORMATION_CLASS class, void *info, ULONG length )
{
    FIXME( "(0x%08x,%p,0x%08x) stub\n", class, info, length );
    return STATUS_SUCCESS;
}


/******************************************************************************
 *              NtQuerySystemEnvironmentValue  (NTDLL.@)
 */
NTSTATUS WINAPI NtQuerySystemEnvironmentValue( UNICODE_STRING *name, WCHAR *buffer, ULONG length,
                                               ULONG *retlen )
{
    FIXME( "(%s, %p, %u, %p), stub\n", debugstr_us(name), buffer, length, retlen );
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *              NtQuerySystemEnvironmentValueEx  (NTDLL.@)
 */
NTSTATUS WINAPI NtQuerySystemEnvironmentValueEx( UNICODE_STRING *name, GUID *vendor, void *buffer,
                                                 ULONG *retlen, ULONG *attrib )
{
    FIXME( "(%s, %s, %p, %p, %p), stub\n", debugstr_us(name),
           debugstr_guid(vendor), buffer, retlen, attrib );
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *              NtSystemDebugControl  (NTDLL.@)
 */
NTSTATUS WINAPI NtSystemDebugControl( SYSDBG_COMMAND command, void *in_buff, ULONG in_len,
                                      void *out_buff, ULONG out_len, ULONG *retlen )
{
    FIXME( "(%d, %p, %d, %p, %d, %p), stub\n", command, in_buff, in_len, out_buff, out_len, retlen );
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *              NtShutdownSystem  (NTDLL.@)
 */
NTSTATUS WINAPI NtShutdownSystem( SHUTDOWN_ACTION action )
{
    FIXME( "%d\n", action );
    return STATUS_SUCCESS;
}


#ifdef linux

/* Fallback using /proc/cpuinfo for Linux systems without cpufreq. For
 * most distributions on recent enough hardware, this is only likely to
 * happen while running in virtualized environments such as QEMU. */
static ULONG mhz_from_cpuinfo(void)
{
    char line[512];
    char *s, *value;
    double cmz = 0;
    FILE *f = fopen("/proc/cpuinfo", "r");
    if(f)
    {
        while (fgets(line, sizeof(line), f) != NULL)
        {
            if (!(value = strchr(line,':'))) continue;
            s = value - 1;
            while ((s >= line) && (*s == ' ' || *s == '\t')) s--;
            s[1] = 0;
            value++;
            if (!strcmp( line, "cpu MHz" ))
            {
                sscanf(value, " %lf", &cmz);
                break;
            }
        }
        fclose( f );
    }
    return cmz;
}

static const char * get_sys_str(const char *path, char *s)
{
    FILE *f = fopen(path, "r");
    const char *ret = NULL;

    if (f)
    {
        if (fgets(s, 16, f)) ret = s;
        fclose(f);
    }
    return ret;
}

static int get_sys_int(const char *path, int def)
{
    char s[16];
    return get_sys_str(path, s) ? atoi(s) : def;
}

static NTSTATUS fill_battery_state( SYSTEM_BATTERY_STATE *bs )
{
    char s[16], path[64];
    unsigned int i = 0;
    LONG64 voltage; /* microvolts */

    bs->AcOnLine = get_sys_int("/sys/class/power_supply/AC/online", 1);

    for (;;)
    {
        sprintf(path, "/sys/class/power_supply/BAT%u/status", i);
        if (!get_sys_str(path, s)) break;
        bs->Charging |= (strcmp(s, "Charging\n") == 0);
        bs->Discharging |= (strcmp(s, "Discharging\n") == 0);
        bs->BatteryPresent = TRUE;
        i++;
    }

    if (bs->BatteryPresent)
    {
        voltage = get_sys_int("/sys/class/power_supply/BAT0/voltage_now", 0);
        bs->MaxCapacity = get_sys_int("/sys/class/power_supply/BAT0/charge_full", 0) * voltage / 1e9;
        bs->RemainingCapacity = get_sys_int("/sys/class/power_supply/BAT0/charge_now", 0) * voltage / 1e9;
        bs->Rate = -get_sys_int("/sys/class/power_supply/BAT0/current_now", 0) * voltage / 1e9;
        if (!bs->Charging && (LONG)bs->Rate < 0)
            bs->EstimatedTime = 3600 * bs->RemainingCapacity / -(LONG)bs->Rate;
        else
            bs->EstimatedTime = ~0u;
    }

    return STATUS_SUCCESS;
}

#elif defined(HAVE_IOKIT_IOKITLIB_H)

static NTSTATUS fill_battery_state( SYSTEM_BATTERY_STATE *bs )
{
    CFArrayRef batteries;
    CFDictionaryRef battery;
    CFNumberRef prop;
    uint32_t value, voltage;
    CFTimeInterval remain;

    if (IOPMCopyBatteryInfo( kIOMasterPortDefault, &batteries ) != kIOReturnSuccess)
        return STATUS_ACCESS_DENIED;

    if (CFArrayGetCount( batteries ) == 0)
    {
        /* Just assume we're on AC with no battery. */
        bs->AcOnLine = TRUE;
        return STATUS_SUCCESS;
    }
    /* Just use the first battery. */
    battery = CFArrayGetValueAtIndex( batteries, 0 );

    prop = CFDictionaryGetValue( battery, CFSTR(kIOBatteryFlagsKey) );
    CFNumberGetValue( prop, kCFNumberSInt32Type, &value );

    if (value & kIOBatteryInstalled)
        bs->BatteryPresent = TRUE;
    else
        /* Since we are executing code, we must have AC power. */
        bs->AcOnLine = TRUE;
    if (value & kIOBatteryChargerConnect)
    {
        bs->AcOnLine = TRUE;
        if (value & kIOBatteryCharge)
            bs->Charging = TRUE;
    }
    else
        bs->Discharging = TRUE;

    /* We'll need the voltage to be able to interpret the other values. */
    prop = CFDictionaryGetValue( battery, CFSTR(kIOBatteryVoltageKey) );
    CFNumberGetValue( prop, kCFNumberSInt32Type, &voltage );

    prop = CFDictionaryGetValue( battery, CFSTR(kIOBatteryCapacityKey) );
    CFNumberGetValue( prop, kCFNumberSInt32Type, &value );
    bs->MaxCapacity = value * voltage;
    /* Apple uses "estimated time < 10:00" and "22%" for these, but we'll follow
     * Windows for now (5% and 33%). */
    bs->DefaultAlert1 = bs->MaxCapacity / 20;
    bs->DefaultAlert2 = bs->MaxCapacity / 3;

    prop = CFDictionaryGetValue( battery, CFSTR(kIOBatteryCurrentChargeKey) );
    CFNumberGetValue( prop, kCFNumberSInt32Type, &value );
    bs->RemainingCapacity = value * voltage;

    prop = CFDictionaryGetValue( battery, CFSTR(kIOBatteryAmperageKey) );
    CFNumberGetValue( prop, kCFNumberSInt32Type, &value );
    bs->Rate = value * voltage;

    remain = IOPSGetTimeRemainingEstimate();
    if (remain != kIOPSTimeRemainingUnknown && remain != kIOPSTimeRemainingUnlimited)
        bs->EstimatedTime = (ULONG)remain;

    CFRelease( batteries );
    return STATUS_SUCCESS;
}

#else

static NTSTATUS fill_battery_state( SYSTEM_BATTERY_STATE *bs )
{
	FIXME("SystemBatteryState not implemented on this platform\n");
	return STATUS_NOT_IMPLEMENTED;
}

#endif

/******************************************************************************
 *              NtPowerInformation  (NTDLL.@)
 */
NTSTATUS WINAPI NtPowerInformation( POWER_INFORMATION_LEVEL level, void *input, ULONG in_size,
                                    void *output, ULONG out_size )
{
    TRACE( "(%d,%p,%d,%p,%d)\n", level, input, in_size, output, out_size );
    switch (level)
    {
    case SystemPowerCapabilities:
    {
        PSYSTEM_POWER_CAPABILITIES PowerCaps = output;
        FIXME("semi-stub: SystemPowerCapabilities\n");
        if (out_size < sizeof(SYSTEM_POWER_CAPABILITIES)) return STATUS_BUFFER_TOO_SMALL;
        /* FIXME: These values are based off a native XP desktop, should probably use APM/ACPI to get the 'real' values */
        PowerCaps->PowerButtonPresent = TRUE;
        PowerCaps->SleepButtonPresent = FALSE;
        PowerCaps->LidPresent = FALSE;
        PowerCaps->SystemS1 = TRUE;
        PowerCaps->SystemS2 = FALSE;
        PowerCaps->SystemS3 = FALSE;
        PowerCaps->SystemS4 = TRUE;
        PowerCaps->SystemS5 = TRUE;
        PowerCaps->HiberFilePresent = TRUE;
        PowerCaps->FullWake = TRUE;
        PowerCaps->VideoDimPresent = FALSE;
        PowerCaps->ApmPresent = FALSE;
        PowerCaps->UpsPresent = FALSE;
        PowerCaps->ThermalControl = FALSE;
        PowerCaps->ProcessorThrottle = FALSE;
        PowerCaps->ProcessorMinThrottle = 100;
        PowerCaps->ProcessorMaxThrottle = 100;
        PowerCaps->DiskSpinDown = TRUE;
        PowerCaps->SystemBatteriesPresent = FALSE;
        PowerCaps->BatteriesAreShortTerm = FALSE;
        PowerCaps->BatteryScale[0].Granularity = 0;
        PowerCaps->BatteryScale[0].Capacity = 0;
        PowerCaps->BatteryScale[1].Granularity = 0;
        PowerCaps->BatteryScale[1].Capacity = 0;
        PowerCaps->BatteryScale[2].Granularity = 0;
        PowerCaps->BatteryScale[2].Capacity = 0;
        PowerCaps->AcOnLineWake = PowerSystemUnspecified;
        PowerCaps->SoftLidWake = PowerSystemUnspecified;
        PowerCaps->RtcWake = PowerSystemSleeping1;
        PowerCaps->MinDeviceWakeState = PowerSystemUnspecified;
        PowerCaps->DefaultLowLatencyWake = PowerSystemUnspecified;
        return STATUS_SUCCESS;
    }

    case SystemBatteryState:
    {
        if (out_size < sizeof(SYSTEM_BATTERY_STATE)) return STATUS_BUFFER_TOO_SMALL;
        memset(output, 0, sizeof(SYSTEM_BATTERY_STATE));
        return fill_battery_state(output);
    }

    case SystemExecutionState:
    {
        ULONG *state = output;
        WARN("semi-stub: SystemExecutionState\n"); /* Needed for .NET Framework, but using a FIXME is really noisy. */
        if (input != NULL) return STATUS_INVALID_PARAMETER;
        /* FIXME: The actual state should be the value set by SetThreadExecutionState which is not currently implemented. */
        *state = ES_USER_PRESENT;
        return STATUS_SUCCESS;
    }

    case ProcessorInformation:
    {
        const int cannedMHz = 1000; /* We fake a 1GHz processor if we can't conjure up real values */
        PROCESSOR_POWER_INFORMATION* cpu_power = output;
        int i, out_cpus;

        if ((output == NULL) || (out_size == 0)) return STATUS_INVALID_PARAMETER;
        out_cpus = peb->NumberOfProcessors;
        if ((out_size / sizeof(PROCESSOR_POWER_INFORMATION)) < out_cpus) return STATUS_BUFFER_TOO_SMALL;
#if defined(linux)
        {
            char filename[128];
            FILE* f;

            for(i = 0; i < out_cpus; i++) {
                sprintf(filename, "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", i);
                f = fopen(filename, "r");
                if (f && (fscanf(f, "%d", &cpu_power[i].MaxMhz) == 1)) {
                    cpu_power[i].MaxMhz /= 1000;
                    fclose(f);
                    cpu_power[i].CurrentMhz = cpu_power[i].MaxMhz;
                }
                else {
                    if(i == 0) {
                        cpu_power[0].CurrentMhz = mhz_from_cpuinfo();
                        if(cpu_power[0].CurrentMhz == 0)
                            cpu_power[0].CurrentMhz = cannedMHz;
                    }
                    else
                        cpu_power[i].CurrentMhz = cpu_power[0].CurrentMhz;
                    cpu_power[i].MaxMhz = cpu_power[i].CurrentMhz;
                    if(f) fclose(f);
                }

                sprintf(filename, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
                f = fopen(filename, "r");
                if(f && (fscanf(f, "%d", &cpu_power[i].MhzLimit) == 1)) {
                    cpu_power[i].MhzLimit /= 1000;
                    fclose(f);
                }
                else
                {
                    cpu_power[i].MhzLimit = cpu_power[i].MaxMhz;
                    if(f) fclose(f);
                }

                cpu_power[i].Number = i;
                cpu_power[i].MaxIdleState = 0;     /* FIXME */
                cpu_power[i].CurrentIdleState = 0; /* FIXME */
            }
        }
#elif defined(__FreeBSD__) || defined (__FreeBSD_kernel__) || defined(__DragonFly__)
        {
            int num;
            size_t valSize = sizeof(num);
            if (sysctlbyname("hw.clockrate", &num, &valSize, NULL, 0))
                num = cannedMHz;
            for(i = 0; i < out_cpus; i++) {
                cpu_power[i].CurrentMhz = num;
                cpu_power[i].MaxMhz = num;
                cpu_power[i].MhzLimit = num;
                cpu_power[i].Number = i;
                cpu_power[i].MaxIdleState = 0;     /* FIXME */
                cpu_power[i].CurrentIdleState = 0; /* FIXME */
            }
        }
#elif defined (__APPLE__)
        {
            size_t valSize;
            unsigned long long currentMhz;
            unsigned long long maxMhz;

            valSize = sizeof(currentMhz);
            if (!sysctlbyname("hw.cpufrequency", &currentMhz, &valSize, NULL, 0))
                currentMhz /= 1000000;
            else
                currentMhz = cannedMHz;

            valSize = sizeof(maxMhz);
            if (!sysctlbyname("hw.cpufrequency_max", &maxMhz, &valSize, NULL, 0))
                maxMhz /= 1000000;
            else
                maxMhz = currentMhz;

            for(i = 0; i < out_cpus; i++) {
                cpu_power[i].CurrentMhz = currentMhz;
                cpu_power[i].MaxMhz = maxMhz;
                cpu_power[i].MhzLimit = maxMhz;
                cpu_power[i].Number = i;
                cpu_power[i].MaxIdleState = 0;     /* FIXME */
                cpu_power[i].CurrentIdleState = 0; /* FIXME */
            }
        }
#else
        for(i = 0; i < out_cpus; i++) {
            cpu_power[i].CurrentMhz = cannedMHz;
            cpu_power[i].MaxMhz = cannedMHz;
            cpu_power[i].MhzLimit = cannedMHz;
            cpu_power[i].Number = i;
            cpu_power[i].MaxIdleState = 0; /* FIXME */
            cpu_power[i].CurrentIdleState = 0; /* FIXME */
        }
        WARN("Unable to detect CPU MHz for this platform. Reporting %d MHz.\n", cannedMHz);
#endif
        for(i = 0; i < out_cpus; i++) {
            TRACE("cpu_power[%d] = %u %u %u %u %u %u\n", i, cpu_power[i].Number,
                  cpu_power[i].MaxMhz, cpu_power[i].CurrentMhz, cpu_power[i].MhzLimit,
                  cpu_power[i].MaxIdleState, cpu_power[i].CurrentIdleState);
        }
        return STATUS_SUCCESS;
    }

    default:
        /* FIXME: Needed by .NET Framework */
        WARN( "Unimplemented NtPowerInformation action: %d\n", level );
        return STATUS_NOT_IMPLEMENTED;
    }
}


/******************************************************************************
 *              NtLoadDriver  (NTDLL.@)
 */
NTSTATUS WINAPI NtLoadDriver( const UNICODE_STRING *name )
{
    FIXME( "(%s), stub!\n", debugstr_us(name) );
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *              NtUnloadDriver  (NTDLL.@)
 */
NTSTATUS WINAPI NtUnloadDriver( const UNICODE_STRING *name )
{
    FIXME( "(%s), stub!\n", debugstr_us(name) );
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *              NtDisplayString  (NTDLL.@)
 */
NTSTATUS WINAPI NtDisplayString( UNICODE_STRING *string )
{
    ERR( "%s\n", debugstr_us(string) );
    return STATUS_SUCCESS;
}


/******************************************************************************
 *              NtRaiseHardError  (NTDLL.@)
 */
NTSTATUS WINAPI NtRaiseHardError( NTSTATUS status, ULONG count,
                                  UNICODE_STRING *params_mask, void **params,
                                  HARDERROR_RESPONSE_OPTION option, HARDERROR_RESPONSE *response )
{
    FIXME( "%08x stub\n", status );
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *              NtInitiatePowerAction  (NTDLL.@)
 */
NTSTATUS WINAPI NtInitiatePowerAction( POWER_ACTION action, SYSTEM_POWER_STATE state,
                                       ULONG flags, BOOLEAN async )
{
    FIXME( "(%d,%d,0x%08x,%d),stub\n", action, state, flags, async );
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *              NtCreatePowerRequest  (NTDLL.@)
 */
NTSTATUS WINAPI NtCreatePowerRequest( HANDLE *handle, COUNTED_REASON_CONTEXT *context )
{
    FIXME( "(%p, %p): stub\n", handle, context );
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *              NtSetPowerRequest  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetPowerRequest( HANDLE handle, POWER_REQUEST_TYPE type )
{
    FIXME( "(%p, %u): stub\n", handle, type );
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *              NtClearPowerRequest  (NTDLL.@)
 */
NTSTATUS WINAPI NtClearPowerRequest( HANDLE handle, POWER_REQUEST_TYPE type )
{
    FIXME( "(%p, %u): stub\n", handle, type );
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *              NtSetThreadExecutionState  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetThreadExecutionState( EXECUTION_STATE new_state, EXECUTION_STATE *old_state )
{
    static EXECUTION_STATE current = ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED | ES_USER_PRESENT;

    WARN( "(0x%x, %p): stub, harmless.\n", new_state, old_state );
    *old_state = current;
    if (!(current & ES_CONTINUOUS) || (new_state & ES_CONTINUOUS)) current = new_state;
    return STATUS_SUCCESS;
}
