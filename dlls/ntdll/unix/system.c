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

#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <dirent.h>
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
#ifdef HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif
#ifdef __APPLE__
# include <CoreFoundation/CoreFoundation.h>
# include <IOKit/IOKitLib.h>
# include <IOKit/ps/IOPSKeys.h>
# include <IOKit/ps/IOPowerSources.h>
# include <mach/mach.h>
# include <mach/machine.h>
# include <mach/mach_init.h>
# include <mach/mach_host.h>
# include <mach/vm_map.h>
#endif

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

struct smbios_bios_args
{
    const char *vendor;
    size_t vendor_len;
    const char *version;
    size_t version_len;
    const char *date;
    size_t date_len;
};

struct smbios_system_args
{
    const char *vendor;
    size_t vendor_len;
    const char *product;
    size_t product_len;
    const char *version;
    size_t version_len;
    const char *serial;
    size_t serial_len;
    GUID uuid;
    const char *sku;
    size_t sku_len;
    const char *family;
    size_t family_len;
};

struct smbios_board_args
{
    const char *vendor;
    size_t vendor_len;
    const char *product;
    size_t product_len;
    const char *version;
    size_t version_len;
    const char *serial;
    size_t serial_len;
    const char *asset_tag;
    size_t asset_tag_len;
};

struct smbios_chassis_args
{
    const char *vendor;
    size_t vendor_len;
    BYTE type;
    const char *version;
    size_t version_len;
    const char *serial;
    size_t serial_len;
    const char *asset_tag;
    size_t asset_tag_len;
};

/* Firmware table providers */
#define ACPI 0x41435049
#define FIRM 0x4649524D
#define RSMB 0x52534D42

SYSTEM_CPU_INFORMATION cpu_info = { 0 };
static SYSTEM_PROCESSOR_FEATURES_INFORMATION cpu_features;
static char cpu_name[49];
static ULONG *performance_cores;
static unsigned int performance_cores_capacity = 0;
static SYSTEM_LOGICAL_PROCESSOR_INFORMATION *logical_proc_info;
static unsigned int logical_proc_info_len, logical_proc_info_alloc_len;
static SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *logical_proc_info_ex;
static unsigned int logical_proc_info_ex_size, logical_proc_info_ex_alloc_size;

static pthread_mutex_t timezone_mutex = PTHREAD_MUTEX_INITIALIZER;

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

extern void do_cpuid( unsigned int ax, unsigned int cx, unsigned int *p );
#ifdef __i386__
__ASM_GLOBAL_FUNC( do_cpuid,
                   "pushl %esi\n\t"
                   "pushl %ebx\n\t"
                   "movl 12(%esp),%eax\n\t"
                   "movl 16(%esp),%ecx\n\t"
                   "movl 20(%esp),%esi\n\t"
                   "cpuid\n\t"
                   "movl %eax,(%esi)\n\t"
                   "movl %ebx,4(%esi)\n\t"
                   "movl %ecx,8(%esi)\n\t"
                   "movl %edx,12(%esi)\n\t"
                   "popl %ebx\n\t"
                   "popl %esi\n\t"
                   "ret" )
#else
__ASM_GLOBAL_FUNC( do_cpuid,
                   "pushq %rbx\n\t"
                   "movl %edi,%eax\n\t"
                   "movl %esi,%ecx\n\t"
                   "movq %rdx,%r8\n\t"
                   "cpuid\n\t"
                   "movl %eax,(%r8)\n\t"
                   "movl %ebx,4(%r8)\n\t"
                   "movl %ecx,8(%r8)\n\t"
                   "movl %edx,12(%r8)\n\t"
                   "popq %rbx\n\t"
                   "ret" )
#endif

#ifdef __i386__
extern int have_cpuid(void);
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

static void get_cpuid_name( char *buffer )
{
    unsigned int regs[4];

    do_cpuid( 0x80000002, 0, regs );
    memcpy( buffer, regs, sizeof(regs) );
    buffer += sizeof(regs);
    do_cpuid( 0x80000003, 0, regs );
    memcpy( buffer, regs, sizeof(regs) );
    buffer += sizeof(regs);
    do_cpuid( 0x80000004, 0, regs );
    memcpy( buffer, regs, sizeof(regs) );
    buffer += sizeof(regs);
    *buffer = 0;
}

static void get_cpuinfo( SYSTEM_CPU_INFORMATION *info )
{
    unsigned int regs[4], regs2[4], regs3[4];
    ULONGLONG features;

#if defined(__i386__)
    info->ProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
#elif defined(__x86_64__)
    info->ProcessorArchitecture = PROCESSOR_ARCHITECTURE_AMD64;
#endif

    /* We're at least a 386 */
    features = CPU_FEATURE_VME | CPU_FEATURE_X86 | CPU_FEATURE_PGE;
    info->ProcessorLevel = 3;

    if (!have_cpuid()) return;

    do_cpuid( 0x00000000, 0, regs );  /* get standard cpuid level and vendor name */
    if (regs[0]>=0x00000001)   /* Check for supported cpuid version */
    {
        do_cpuid( 0x00000001, 0, regs2 ); /* get cpu features */
        if (regs2[3] & (1 << 3 )) features |= CPU_FEATURE_PSE;
        if (regs2[3] & (1 << 4 )) features |= CPU_FEATURE_TSC;
        if (regs2[3] & (1 << 6 )) features |= CPU_FEATURE_PAE;
        if (regs2[3] & (1 << 8 )) features |= CPU_FEATURE_CX8;
        if (regs2[3] & (1 << 11)) features |= CPU_FEATURE_SEP;
        if (regs2[3] & (1 << 12)) features |= CPU_FEATURE_MTRR;
        if (regs2[3] & (1 << 15)) features |= CPU_FEATURE_CMOV;
        if (regs2[3] & (1 << 16)) features |= CPU_FEATURE_PAT;
        if (regs2[3] & (1 << 23)) features |= CPU_FEATURE_MMX;
        if (regs2[3] & (1 << 24)) features |= CPU_FEATURE_FXSR;
        if (regs2[3] & (1 << 25)) features |= CPU_FEATURE_SSE;
        if (regs2[3] & (1 << 26)) features |= CPU_FEATURE_SSE2;
        if (regs2[2] & (1 << 0 )) features |= CPU_FEATURE_SSE3;
        if (regs2[2] & (1 << 9 )) features |= CPU_FEATURE_SSSE3;
        if (regs2[2] & (1 << 13)) features |= CPU_FEATURE_CX128;
        if (regs2[2] & (1 << 19)) features |= CPU_FEATURE_SSE41;
        if (regs2[2] & (1 << 20)) features |= CPU_FEATURE_SSE42;
        if (regs2[2] & (1 << 27)) features |= CPU_FEATURE_XSAVE;
        if (regs2[2] & (1 << 28)) features |= CPU_FEATURE_AVX;
        if((regs2[3] & (1 << 26)) && (regs2[3] & (1 << 24)) && have_sse_daz_mode()) /* has SSE2 and FXSAVE/FXRSTOR */
            features |= CPU_FEATURE_DAZ;

        if (regs[0] >= 0x00000007)
        {
            do_cpuid( 0x00000007, 0, regs3 ); /* get extended features */
            if (regs3[1] & (1 << 5)) features |= CPU_FEATURE_AVX2;
        }

        if (features & CPU_FEATURE_XSAVE)
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
                if (regs2[2] & (1 << 2))   features |= CPU_FEATURE_VIRT;
                if (regs2[3] & (1 << 20))  features |= CPU_FEATURE_NX;
                if (regs2[3] & (1 << 27))  features |= CPU_FEATURE_TSC;
                if (regs2[3] & (1u << 31)) features |= CPU_FEATURE_3DNOW;
            }
            if (regs[0] >= 0x80000004) get_cpuid_name( cpu_name );
        }
        else if (regs[1] == GENU && regs[3] == INEI && regs[2] == NTEL)
        {
            info->ProcessorLevel = ((regs2[0] >> 8) & 0xf) + ((regs2[0] >> 20) & 0xff); /* family + extended family */
            if(info->ProcessorLevel == 15) info->ProcessorLevel = 6;

            /* repack model and stepping to make a "revision" */
            info->ProcessorRevision  = ((regs2[0] >> 16) & 0xf) << 12; /* extended model */
            info->ProcessorRevision |= ((regs2[0] >> 4 ) & 0xf) << 8;  /* model          */
            info->ProcessorRevision |= regs2[0] & 0xf;                 /* stepping       */

            if(regs2[2] & (1 << 5))  features |= CPU_FEATURE_VIRT;
            if(regs2[3] & (1 << 21)) features |= CPU_FEATURE_DS;

            do_cpuid( 0x80000000, 0, regs );  /* get vendor cpuid level */
            if (regs[0] >= 0x80000001)
            {
                do_cpuid( 0x80000001, 0, regs2 );  /* get vendor features */
                if (regs2[3] & (1 << 20)) features |= CPU_FEATURE_NX;
                if (regs2[3] & (1 << 27)) features |= CPU_FEATURE_TSC;
            }
            if (regs[0] >= 0x80000004) get_cpuid_name( cpu_name );
        }
        else
        {
            info->ProcessorLevel = (regs2[0] >> 8) & 0xf; /* family */

            /* repack model and stepping to make a "revision" */
            info->ProcessorRevision = ((regs2[0] >> 4 ) & 0xf) << 8;  /* model    */
            info->ProcessorRevision |= regs2[0] & 0xf;                /* stepping */
        }
    }
    info->ProcessorFeatureBits = cpu_features.ProcessorFeatureBits = features;
}

#elif defined(__arm__)

static inline void get_cpuinfo( SYSTEM_CPU_INFORMATION *info )
{
    ULONGLONG features = 0;
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
                if (strstr(value, "crc32")) features |= CPU_FEATURE_ARM_V8_CRC32;
                if (strstr(value, "aes"))   features |= CPU_FEATURE_ARM_V8_CRYPTO;
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
    if (!sysctlbyname("hw.floatingpoint", &value, &valsize, NULL, 0)) features |= CPU_FEATURE_ARM_VFP_32;
#else
    FIXME("CPU Feature detection not implemented.\n");
#endif
    info->ProcessorArchitecture = PROCESSOR_ARCHITECTURE_ARM;
    info->ProcessorFeatureBits = cpu_features.ProcessorFeatureBits = features;
}

#elif defined(__aarch64__)

static void get_cpuinfo( SYSTEM_CPU_INFORMATION *info )
{
    ULONGLONG features = 0;
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
                if (strstr(value, "crc32")) features |= CPU_FEATURE_ARM_V8_CRC32;
                if (strstr(value, "aes"))   features |= CPU_FEATURE_ARM_V8_CRYPTO;
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
    info->ProcessorFeatureBits = cpu_features.ProcessorFeatureBits = features;
}

#endif /* End architecture specific feature detection for CPUs */

static BOOL grow_logical_proc_buf(void)
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION *new_data;
    unsigned int new_len;

    if (logical_proc_info_len < logical_proc_info_alloc_len) return TRUE;

    new_len = max( logical_proc_info_alloc_len * 2, logical_proc_info_len + 1 );
    if (!(new_data = realloc( logical_proc_info, new_len * sizeof(*new_data) ))) return FALSE;
    memset( new_data + logical_proc_info_alloc_len, 0,
            (new_len - logical_proc_info_alloc_len) * sizeof(*new_data) );
    logical_proc_info = new_data;
    logical_proc_info_alloc_len = new_len;
    return TRUE;
}

static BOOL grow_logical_proc_ex_buf( unsigned int add_size )
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *new_dataex;
    DWORD new_len;

    if ( logical_proc_info_ex_size + add_size <= logical_proc_info_ex_alloc_size ) return TRUE;

    new_len  = max( logical_proc_info_ex_alloc_size * 2, logical_proc_info_ex_alloc_size + add_size );
    if (!(new_dataex = realloc( logical_proc_info_ex, new_len ))) return FALSE;
    memset( (char *)new_dataex + logical_proc_info_ex_alloc_size, 0, new_len - logical_proc_info_ex_alloc_size );
    logical_proc_info_ex = new_dataex;
    logical_proc_info_ex_alloc_size = new_len;
    return TRUE;
}

static DWORD log_proc_ex_size_plus( DWORD size )
{
    /* add SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX.Relationship and .Size */
    return sizeof(LOGICAL_PROCESSOR_RELATIONSHIP) + sizeof(DWORD) + size;
}

static DWORD count_bits( ULONG_PTR mask )
{
    DWORD count = 0;
    while (mask > 0)
    {
        if (mask & 1) ++count;
        mask >>= 1;
    }
    return count;
}

static BOOL logical_proc_info_ex_add_by_id( LOGICAL_PROCESSOR_RELATIONSHIP rel, DWORD id, ULONG_PTR mask )
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *dataex;
    unsigned int ofs = 0;

    while (ofs < logical_proc_info_ex_size)
    {
        dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)((char *)logical_proc_info_ex + ofs);
        if (rel == RelationProcessorPackage && dataex->Relationship == rel && dataex->Processor.Reserved[1] == id)
        {
            dataex->Processor.GroupMask[0].Mask |= mask;
            return TRUE;
        }
        else if (rel == RelationProcessorCore && dataex->Relationship == rel && dataex->Processor.Reserved[1] == id)
        {
            return TRUE;
        }
        ofs += dataex->Size;
    }

    /* TODO: For now, just one group. If more than 64 processors, then we
     * need another group. */
    if (!grow_logical_proc_ex_buf( log_proc_ex_size_plus( sizeof(PROCESSOR_RELATIONSHIP) ))) return FALSE;

    dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)((char *)logical_proc_info_ex + ofs);

    dataex->Relationship = rel;
    dataex->Size = log_proc_ex_size_plus( sizeof(PROCESSOR_RELATIONSHIP) );
    if (rel == RelationProcessorCore)
        dataex->Processor.Flags = count_bits( mask ) > 1 ? LTP_PC_SMT : 0;
    else
        dataex->Processor.Flags = 0;
    if (rel == RelationProcessorCore && id / 32 < performance_cores_capacity)
        dataex->Processor.EfficiencyClass = (performance_cores[id / 32] >> (id % 32)) & 1;
    else
        dataex->Processor.EfficiencyClass = 0;
    dataex->Processor.GroupCount = 1;
    dataex->Processor.GroupMask[0].Mask = mask;
    dataex->Processor.GroupMask[0].Group = 0;
    /* mark for future lookup */
    dataex->Processor.Reserved[0] = 0;
    dataex->Processor.Reserved[1] = id;

    logical_proc_info_ex_size += dataex->Size;
    return TRUE;
}

/* Store package and core information for a logical processor. Parsing of processor
 * data may happen in multiple passes; the 'id' parameter is then used to locate
 * previously stored data. The type of data stored in 'id' depends on 'rel':
 * - RelationProcessorPackage: package id ('CPU socket').
 * - RelationProcessorCore: physical core number.
 */
static BOOL logical_proc_info_add_by_id( LOGICAL_PROCESSOR_RELATIONSHIP rel, DWORD id, ULONG_PTR mask )
{
    unsigned int i;

    for (i = 0; i < logical_proc_info_len; i++)
    {
        if (rel == RelationProcessorPackage && logical_proc_info[i].Relationship == rel
            && logical_proc_info[i].Reserved[1] == id)
        {
            logical_proc_info[i].ProcessorMask |= mask;
            return logical_proc_info_ex_add_by_id( rel, id, mask );
        }
        else if (rel == RelationProcessorCore && logical_proc_info[i].Relationship == rel
                 && logical_proc_info[i].Reserved[1] == id)
            return logical_proc_info_ex_add_by_id( rel, id, mask );
    }

    if (!grow_logical_proc_buf()) return FALSE;

    logical_proc_info[i].Relationship = rel;
    logical_proc_info[i].ProcessorMask = mask;
    if (rel == RelationProcessorCore)
        logical_proc_info[i].ProcessorCore.Flags = count_bits( mask ) > 1 ? LTP_PC_SMT : 0;
    logical_proc_info[i].Reserved[0] = 0;
    logical_proc_info[i].Reserved[1] = id;
    logical_proc_info_len = i + 1;

    return logical_proc_info_ex_add_by_id( rel, id, mask );
}

static BOOL logical_proc_info_add_cache( ULONG_PTR mask, CACHE_DESCRIPTOR *cache )
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *dataex;
    unsigned int ofs = 0, i;

    for (i = 0; i < logical_proc_info_len; i++)
    {
        if (logical_proc_info[i].Relationship==RelationCache && logical_proc_info[i].ProcessorMask==mask
            && logical_proc_info[i].Cache.Level==cache->Level && logical_proc_info[i].Cache.Type==cache->Type)
            return TRUE;
    }

    if (!grow_logical_proc_buf()) return FALSE;

    logical_proc_info[i].Relationship = RelationCache;
    logical_proc_info[i].ProcessorMask = mask;
    logical_proc_info[i].Cache = *cache;
    logical_proc_info_len = i + 1;

    for (ofs = 0; ofs < logical_proc_info_ex_size; )
    {
        dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)((char *)logical_proc_info_ex + ofs);
        if (dataex->Relationship == RelationCache && dataex->Cache.GroupMask.Mask == mask
            && dataex->Cache.Level == cache->Level && dataex->Cache.Type == cache->Type)
            return TRUE;
        ofs += dataex->Size;
    }

    if (!grow_logical_proc_ex_buf( log_proc_ex_size_plus( sizeof(CACHE_RELATIONSHIP) ))) return FALSE;

    dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)((char *)logical_proc_info_ex + ofs);

    dataex->Relationship = RelationCache;
    dataex->Size = log_proc_ex_size_plus( sizeof(CACHE_RELATIONSHIP) );
    dataex->Cache.Level = cache->Level;
    dataex->Cache.Associativity = cache->Associativity;
    dataex->Cache.LineSize = cache->LineSize;
    dataex->Cache.CacheSize = cache->Size;
    dataex->Cache.Type = cache->Type;
    dataex->Cache.GroupMask.Mask = mask;
    dataex->Cache.GroupMask.Group = 0;

    logical_proc_info_ex_size += dataex->Size;

    return TRUE;
}

static BOOL logical_proc_info_add_numa_node( ULONG_PTR mask, DWORD node_id )
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *dataex;

    if (!grow_logical_proc_buf()) return FALSE;

    logical_proc_info[logical_proc_info_len].Relationship = RelationNumaNode;
    logical_proc_info[logical_proc_info_len].ProcessorMask = mask;
    logical_proc_info[logical_proc_info_len].NumaNode.NodeNumber = node_id;
    ++logical_proc_info_len;

    if (!grow_logical_proc_ex_buf( log_proc_ex_size_plus( sizeof(NUMA_NODE_RELATIONSHIP) ))) return FALSE;

    dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)((char *)logical_proc_info_ex + logical_proc_info_ex_size);

    dataex->Relationship = RelationNumaNode;
    dataex->Size = log_proc_ex_size_plus( sizeof(NUMA_NODE_RELATIONSHIP) );
    dataex->NumaNode.NodeNumber = node_id;
    dataex->NumaNode.GroupMask.Mask = mask;
    dataex->NumaNode.GroupMask.Group = 0;

    logical_proc_info_ex_size += dataex->Size;

    return TRUE;
}

static BOOL logical_proc_info_add_group( DWORD num_cpus, ULONG_PTR mask )
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *dataex;

    if (!grow_logical_proc_ex_buf( log_proc_ex_size_plus( sizeof(GROUP_RELATIONSHIP) ))) return FALSE;

    dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(((char *)logical_proc_info_ex) + logical_proc_info_ex_size);

    dataex->Relationship = RelationGroup;
    dataex->Size = log_proc_ex_size_plus( sizeof(GROUP_RELATIONSHIP) );
    dataex->Group.MaximumGroupCount = 1;
    dataex->Group.ActiveGroupCount = 1;
    dataex->Group.GroupInfo[0].MaximumProcessorCount = num_cpus;
    dataex->Group.GroupInfo[0].ActiveProcessorCount = num_cpus;
    dataex->Group.GroupInfo[0].ActiveProcessorMask = mask;

    logical_proc_info_ex_size += dataex->Size;
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
    unsigned int r;

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
static BOOL sysfs_count_list_elements(const char *filename, unsigned int *result)
{
    FILE *f;

    f = fopen(filename, "r");
    if (!f) return FALSE;

    while (!feof(f))
    {
        char op;
        unsigned int beg, end;

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

static void fill_performance_core_info(void)
{
    FILE *fpcore_list;
    unsigned int beg, end, i;
    char op = ',';
    ULONG *p;

    fpcore_list = fopen("/sys/devices/cpu_core/cpus", "r");
    if (!fpcore_list) return;

    performance_cores = calloc(16, sizeof(ULONG));
    if (!performance_cores) goto done;
    performance_cores_capacity = 16;

    while (!feof(fpcore_list) && op == ',')
    {
        if (!fscanf(fpcore_list, "%u %c ", &beg, &op)) break;
        if (op == '-') fscanf(fpcore_list, "%u %c ", &end, &op);
        else end = beg;

        for(i = beg; i <= end; i++)
        {
            if (i / 32 >= performance_cores_capacity)
            {
                p = realloc(performance_cores, performance_cores_capacity * 2 * sizeof(ULONG));
                if (!p) goto done;
                memset(p + performance_cores_capacity, 0, performance_cores_capacity * sizeof(ULONG));
                performance_cores = p;
                performance_cores_capacity *= 2;
            }
            performance_cores[i / 32] |= 1 << (i % 32);
        }
    }
done:
    fclose(fpcore_list);
}

/* for 'data', max_len is the array count. for 'dataex', max_len is in bytes */
static NTSTATUS create_logical_proc_info(void)
{
    static const char core_info[] = "/sys/devices/system/cpu/cpu%u/topology/%s";
    static const char cache_info[] = "/sys/devices/system/cpu/cpu%u/cache/index%u/%s";
    static const char numa_info[] = "/sys/devices/system/node/node%u/cpumap";

    FILE *fcpu_list, *fnuma_list, *f;
    unsigned int beg, end, i, j, r, num_cpus = 0, max_cpus = 0;
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

    fill_performance_core_info();

    fcpu_list = fopen("/sys/devices/system/cpu/online", "r");
    if (!fcpu_list) return STATUS_NOT_IMPLEMENTED;

    while (!feof(fcpu_list))
    {
        if (!fscanf(fcpu_list, "%u%c ", &beg, &op)) break;
        if (op == '-') fscanf(fcpu_list, "%u%c ", &end, &op);
        else end = beg;

        for(i = beg; i <= end; i++)
        {
            unsigned int phys_core = 0;
            ULONG_PTR thread_mask = 0;

            if (i > 8 * sizeof(ULONG_PTR)) break;

            snprintf(name, sizeof(name), core_info, i, "physical_package_id");
            f = fopen(name, "r");
            if (f)
            {
                fscanf(f, "%u", &r);
                fclose(f);
            }
            else r = 0;
            if (!logical_proc_info_add_by_id( RelationProcessorPackage, r, (ULONG_PTR)1 << i ))
            {
                fclose(fcpu_list);
                return STATUS_NO_MEMORY;
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

            /* Mask of logical threads sharing same physical core in kernel core numbering. */
            snprintf(name, sizeof(name), core_info, i, "thread_siblings");
            if(!sysfs_parse_bitmap(name, &thread_mask)) thread_mask = 1<<i;

            /* Needed later for NumaNode and Group. */
            all_cpus_mask |= thread_mask;

            snprintf(name, sizeof(name), core_info, i, "thread_siblings_list");
            f = fopen(name, "r");
            if (f)
            {
                fscanf(f, "%d%c", &phys_core, &op);
                fclose(f);
            }
            else phys_core = i;

            if (!logical_proc_info_add_by_id( RelationProcessorCore, phys_core, thread_mask ))
            {
                fclose(fcpu_list);
                return STATUS_NO_MEMORY;
            }

            for (j = 0; j < 4; j++)
            {
                CACHE_DESCRIPTOR cache;
                ULONG_PTR mask = 0;

                snprintf(name, sizeof(name), cache_info, i, j, "shared_cpu_map");
                if(!sysfs_parse_bitmap(name, &mask)) continue;

                snprintf(name, sizeof(name), cache_info, i, j, "level");
                f = fopen(name, "r");
                if(!f) continue;
                fscanf(f, "%u", &r);
                fclose(f);
                cache.Level = r;

                snprintf(name, sizeof(name), cache_info, i, j, "ways_of_associativity");
                f = fopen(name, "r");
                if(!f) continue;
                fscanf(f, "%u", &r);
                fclose(f);
                cache.Associativity = r;

                snprintf(name, sizeof(name), cache_info, i, j, "coherency_line_size");
                f = fopen(name, "r");
                if(!f) continue;
                fscanf(f, "%u", &r);
                fclose(f);
                cache.LineSize = r;

                snprintf(name, sizeof(name), cache_info, i, j, "size");
                f = fopen(name, "r");
                if(!f) continue;
                fscanf(f, "%u%c", &r, &op);
                fclose(f);
                if(op != 'K')
                    WARN("unknown cache size %u%c\n", r, op);
                cache.Size = (op=='K' ? r*1024 : r);

                snprintf(name, sizeof(name), cache_info, i, j, "type");
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

                if (!logical_proc_info_add_cache( mask, &cache ))
                {
                    fclose(fcpu_list);
                    return STATUS_NO_MEMORY;
                }
            }
        }
    }
    fclose(fcpu_list);

    num_cpus = count_bits(all_cpus_mask);

    fnuma_list = fopen("/sys/devices/system/node/online", "r");
    if (!fnuma_list)
    {
        if (!logical_proc_info_add_numa_node( all_cpus_mask, 0 ))
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

                snprintf(name, sizeof(name), numa_info, i);
                if (!sysfs_parse_bitmap( name, &mask )) continue;

                if (!logical_proc_info_add_numa_node( mask, i ))
                {
                    fclose(fnuma_list);
                    return STATUS_NO_MEMORY;
                }
            }
        }
        fclose(fnuma_list);
    }

    logical_proc_info_add_group( num_cpus, all_cpus_mask );

    performance_cores_capacity = 0;
    free(performance_cores);
    performance_cores = NULL;

    return STATUS_SUCCESS;
}

#elif defined(__APPLE__)

/* for 'data', max_len is the array count. for 'dataex', max_len is in bytes */
static NTSTATUS create_logical_proc_info(void)
{
    unsigned int pkgs_no, cores_no, lcpu_no, lcpu_per_core, cores_per_package, assoc;
    unsigned int cache_ctrs[10] = {0};
    ULONG_PTR all_cpus_mask = 0;
    CACHE_DESCRIPTOR cache[10];
    LONGLONG cache_size, cache_line_size, cache_sharing[10];
    size_t size;
    unsigned int p, i, j, k;

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
            if(!logical_proc_info_add_by_id( RelationProcessorPackage, p, mask ))
                return STATUS_NO_MEMORY;

            /* add new core */
            phys_core = p * cores_per_package + j;
            if(!logical_proc_info_add_by_id( RelationProcessorCore, phys_core, mask ))
                return STATUS_NO_MEMORY;

            for(i = 1; i < 5; ++i)
            {
                if(cache_ctrs[i] == 0 && cache[i].Size > 0)
                {
                    mask = 0;
                    for(k = 0; k < cache_sharing[i]; ++k)
                        mask |= (ULONG_PTR)1 << (j * lcpu_per_core + k);

                    if (!logical_proc_info_add_cache( mask, &cache[i] ))
                        return STATUS_NO_MEMORY;
                }

                cache_ctrs[i] += lcpu_per_core;
                if(cache_ctrs[i] == cache_sharing[i]) cache_ctrs[i] = 0;
            }
        }
    }

    /* OSX doesn't support NUMA, so just make one NUMA node for all CPUs */
    if(!logical_proc_info_add_numa_node( all_cpus_mask, 0 ))
        return STATUS_NO_MEMORY;

    logical_proc_info_add_group( lcpu_no, all_cpus_mask );

    return STATUS_SUCCESS;
}

#else

static NTSTATUS create_logical_proc_info(void)
{
    FIXME("stub\n");
    return STATUS_NOT_IMPLEMENTED;
}
#endif

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
    unsigned int status;
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
           (int)cpu_info.ProcessorArchitecture, (int)cpu_info.ProcessorLevel,
           (int)cpu_info.ProcessorRevision, (int)cpu_info.ProcessorFeatureBits );

    if ((status = create_logical_proc_info()))
    {
        FIXME( "Failed to get logical processor information, status %#x.\n", status );
        free( logical_proc_info );
        logical_proc_info = NULL;
        logical_proc_info_len = 0;

        free( logical_proc_info_ex );
        logical_proc_info_ex = NULL;
        logical_proc_info_ex_size = 0;
    }
    else
    {
        logical_proc_info = realloc( logical_proc_info, logical_proc_info_len * sizeof(*logical_proc_info) );
        logical_proc_info_alloc_len = logical_proc_info_len;
        logical_proc_info_ex = realloc( logical_proc_info_ex, logical_proc_info_ex_size );
        logical_proc_info_ex_alloc_size = logical_proc_info_ex_size;
    }
}

static NTSTATUS create_cpuset_info(SYSTEM_CPU_SET_INFORMATION *info)
{
    const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *proc_info;
    const DWORD cpu_info_size = logical_proc_info_ex_size;
    BYTE core_index, cache_index, max_cache_level;
    unsigned int i, j, count;
    ULONG64 cpu_mask;

    if (!logical_proc_info_ex) return STATUS_NOT_IMPLEMENTED;

    count = peb->NumberOfProcessors;

    max_cache_level = 0;
    proc_info = logical_proc_info_ex;
    for (i = 0; (char *)proc_info != (char *)logical_proc_info_ex + cpu_info_size; ++i)
    {
        if (proc_info->Relationship == RelationCache)
        {
            if (max_cache_level < proc_info->Cache.Level)
                max_cache_level = proc_info->Cache.Level;
        }
        proc_info = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)((BYTE *)proc_info + proc_info->Size);
    }

    memset(info, 0, count * sizeof(*info));

    core_index = 0;
    cache_index = 0;
    proc_info = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)logical_proc_info_ex;
    for (i = 0; i < count; ++i)
    {
        info[i].Size = sizeof(*info);
        info[i].Type = CpuSetInformation;
        info[i].CpuSet.Id = 0x100 + i;
        info[i].CpuSet.LogicalProcessorIndex = i;
    }

    for (i = 0; (char *)proc_info != (char *)logical_proc_info_ex + cpu_info_size; ++i)
    {
        if (proc_info->Relationship == RelationProcessorCore)
        {
            if (proc_info->Processor.GroupCount != 1)
            {
                FIXME("Unsupported group count %u.\n", proc_info->Processor.GroupCount);
                continue;
            }
            cpu_mask = proc_info->Processor.GroupMask[0].Mask;
            for (j = 0; j < count; ++j)
                if (((ULONG64)1 << j) & cpu_mask)
                {
                    info[j].CpuSet.CoreIndex = core_index;
                    info[j].CpuSet.EfficiencyClass = proc_info->Processor.EfficiencyClass;
                }
            ++core_index;
        }
        else if (proc_info->Relationship == RelationCache)
        {
            if (proc_info->Cache.Level == max_cache_level)
            {
                cpu_mask = proc_info->Cache.GroupMask.Mask;
                for (j = 0; j < count; ++j)
                    if (((ULONG64)1 << j) & cpu_mask)
                        info[j].CpuSet.LastLevelCacheIndex = cache_index;
            }
            ++cache_index;
        }
        else if (proc_info->Relationship == RelationNumaNode)
        {
            cpu_mask = proc_info->NumaNode.GroupMask.Mask;
            for (j = 0; j < count; ++j)
                if (((ULONG64)1 << j) & cpu_mask)
                    info[j].CpuSet.NumaNodeIndex = proc_info->NumaNode.NodeNumber;
        }
        proc_info = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)((char *)proc_info + proc_info->Size);
    }

    return STATUS_SUCCESS;
}

#if defined(linux) || defined(__APPLE__)

static void copy_smbios_string( char **buffer, const char *s, size_t len )
{
    if (!len) return;
    memcpy(*buffer, s, len + 1);
    *buffer += len + 1;
}

static NTSTATUS create_smbios_tables( SYSTEM_FIRMWARE_TABLE_INFORMATION *sfti, ULONG available_len,
                                      ULONG *required_len,
                                      const struct smbios_bios_args *bios_args,
                                      const struct smbios_system_args *system_args,
                                      const struct smbios_board_args *board_args,
                                      const struct smbios_chassis_args *chassis_args )
{
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

    *required_len = sizeof(struct smbios_prologue);

#define L(l) (l + (l ? 1 : 0))
    *required_len += sizeof(struct smbios_bios);
    *required_len += max(L(bios_args->vendor_len) + L(bios_args->version_len) + L(bios_args->date_len) + 1, 2);

    *required_len += sizeof(struct smbios_system);
    *required_len += max(L(system_args->vendor_len) + L(system_args->product_len) + L(system_args->version_len) +
                         L(system_args->serial_len) + L(system_args->sku_len) + L(system_args->family_len) + 1, 2);

    *required_len += sizeof(struct smbios_board);
    *required_len += max(L(board_args->vendor_len) + L(board_args->product_len) + L(board_args->version_len) +
                         L(board_args->serial_len) + L(board_args->asset_tag_len) + 1, 2);

    *required_len += sizeof(struct smbios_chassis);
    *required_len += max(L(chassis_args->vendor_len) + L(chassis_args->version_len) + L(chassis_args->serial_len) +
                         L(chassis_args->asset_tag_len) + 1, 2);

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
    bios->vendor = bios_args->vendor_len ? ++string_count : 0;
    bios->version = bios_args->version_len ? ++string_count : 0;
    bios->start = 0;
    bios->date = bios_args->date_len ? ++string_count : 0;
    bios->size = 0;
    bios->characteristics = 0x4; /* not supported */
    bios->characteristics_ext[0] = 0;
    bios->characteristics_ext[1] = 0;
    bios->system_bios_major_release = 0xFF; /* not supported */
    bios->system_bios_minor_release = 0xFF; /* not supported */
    bios->ec_firmware_major_release = 0xFF; /* not supported */
    bios->ec_firmware_minor_release = 0xFF; /* not supported */
    buffer += sizeof(struct smbios_bios);

    copy_smbios_string(&buffer, bios_args->vendor, bios_args->vendor_len);
    copy_smbios_string(&buffer, bios_args->version, bios_args->version_len);
    copy_smbios_string(&buffer, bios_args->date, bios_args->date_len);
    if (!string_count) *buffer++ = 0;
    *buffer++ = 0;

    string_count = 0;
    system = (struct smbios_system*)buffer;
    system->hdr.type = 1;
    system->hdr.length = sizeof(struct smbios_system);
    system->hdr.handle = handle_count++;
    system->vendor = system_args->vendor_len ? ++string_count : 0;
    system->product = system_args->product_len ? ++string_count : 0;
    system->version = system_args->version_len ? ++string_count : 0;
    system->serial = system_args->serial_len ? ++string_count : 0;
    memcpy( system->uuid, &system_args->uuid, sizeof(GUID) );
    system->wake_up_type = 0x02; /* unknown */
    system->sku_number = system_args->sku_len ? ++string_count : 0;
    system->family = system_args->family_len ? ++string_count : 0;
    buffer += sizeof(struct smbios_system);

    copy_smbios_string(&buffer, system_args->vendor, system_args->vendor_len);
    copy_smbios_string(&buffer, system_args->product, system_args->product_len);
    copy_smbios_string(&buffer, system_args->version, system_args->version_len);
    copy_smbios_string(&buffer, system_args->serial, system_args->serial_len);
    copy_smbios_string(&buffer, system_args->sku, system_args->sku_len);
    copy_smbios_string(&buffer, system_args->family, system_args->family_len);
    if (!string_count) *buffer++ = 0;
    *buffer++ = 0;

    string_count = 0;
    chassis = (struct smbios_chassis*)buffer;
    chassis->hdr.type = 3;
    chassis->hdr.length = sizeof(struct smbios_chassis);
    chassis->hdr.handle = handle_count++;
    chassis->vendor = chassis_args->vendor_len ? ++string_count : 0;
    chassis->type = chassis_args->type;
    chassis->version = chassis_args->version_len ? ++string_count : 0;
    chassis->serial = chassis_args->serial_len ? ++string_count : 0;
    chassis->asset_tag = chassis_args->asset_tag_len ? ++string_count : 0;
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

    copy_smbios_string(&buffer, chassis_args->vendor, chassis_args->vendor_len);
    copy_smbios_string(&buffer, chassis_args->version, chassis_args->version_len);
    copy_smbios_string(&buffer, chassis_args->serial, chassis_args->serial_len);
    copy_smbios_string(&buffer, chassis_args->asset_tag, chassis_args->asset_tag_len);
    if (!string_count) *buffer++ = 0;
    *buffer++ = 0;

    string_count = 0;
    board = (struct smbios_board*)buffer;
    board->hdr.type = 2;
    board->hdr.length = sizeof(struct smbios_board);
    board->hdr.handle = handle_count++;
    board->vendor = board_args->vendor_len ? ++string_count : 0;
    board->product = board_args->product_len ? ++string_count : 0;
    board->version = board_args->version_len ? ++string_count : 0;
    board->serial = board_args->serial_len ? ++string_count : 0;
    board->asset_tag = board_args->asset_tag_len ? ++string_count : 0;
    board->feature_flags = 0x5; /* hosting board, removable */
    board->location = 0;
    board->chassis_handle = chassis->hdr.handle;
    board->board_type = 0xa; /* motherboard */
    board->num_contained_handles = 0;
    buffer += sizeof(struct smbios_board);

    copy_smbios_string(&buffer, board_args->vendor, board_args->vendor_len);
    copy_smbios_string(&buffer, board_args->product, board_args->product_len);
    copy_smbios_string(&buffer, board_args->version, board_args->version_len);
    copy_smbios_string(&buffer, board_args->serial, board_args->serial_len);
    copy_smbios_string(&buffer, board_args->asset_tag, board_args->asset_tag_len);
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

#endif

#ifdef linux

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
        struct smbios_bios_args bios_args;
        char system_vendor[128], system_product[128], system_version[128], system_serial[128];
        char system_sku[128], system_family[128];
        struct smbios_system_args system_args;
        char board_vendor[128], board_product[128], board_version[128], board_serial[128], board_asset_tag[128];
        struct smbios_board_args board_args;
        char chassis_vendor[128], chassis_version[128], chassis_serial[128], chassis_asset_tag[128];
        char chassis_type[11] = "2"; /* unknown */
        struct smbios_chassis_args chassis_args;

#define S(s) s, sizeof(s)
        bios_args.vendor_len = get_smbios_string("/sys/class/dmi/id/bios_vendor", S(bios_vendor));
        bios_args.vendor = bios_vendor;
        bios_args.version_len = get_smbios_string("/sys/class/dmi/id/bios_version", S(bios_version));
        bios_args.version = bios_version;
        bios_args.date_len = get_smbios_string("/sys/class/dmi/id/bios_date", S(bios_date));
        bios_args.date = bios_date;

        system_args.vendor_len = get_smbios_string("/sys/class/dmi/id/sys_vendor", S(system_vendor));
        system_args.vendor = system_vendor;
        system_args.product_len = get_smbios_string("/sys/class/dmi/id/product_name", S(system_product));
        system_args.product = system_product;
        system_args.version_len = get_smbios_string("/sys/class/dmi/id/product_version", S(system_version));
        system_args.version = system_version;
        system_args.serial_len = get_smbios_string("/sys/class/dmi/id/product_serial", S(system_serial));
        system_args.serial = system_serial;
        get_system_uuid(&system_args.uuid);
        system_args.sku_len = get_smbios_string("/sys/class/dmi/id/product_sku", S(system_sku));
        system_args.sku = system_sku;
        system_args.family_len = get_smbios_string("/sys/class/dmi/id/product_family", S(system_family));
        system_args.family = system_family;

        board_args.vendor_len = get_smbios_string("/sys/class/dmi/id/board_vendor", S(board_vendor));
        board_args.vendor = board_vendor;
        board_args.product_len = get_smbios_string("/sys/class/dmi/id/board_name", S(board_product));
        board_args.product = board_product;
        board_args.version_len = get_smbios_string("/sys/class/dmi/id/board_version", S(board_version));
        board_args.version = board_version;
        board_args.serial_len = get_smbios_string("/sys/class/dmi/id/board_serial", S(board_serial));
        board_args.serial = board_serial;
        board_args.asset_tag_len = get_smbios_string("/sys/class/dmi/id/board_asset_tag", S(board_asset_tag));
        board_args.asset_tag = board_asset_tag;

        chassis_args.vendor_len = get_smbios_string("/sys/class/dmi/id/chassis_vendor", S(chassis_vendor));
        chassis_args.vendor = chassis_vendor;
        get_smbios_string("/sys/class/dmi/id/chassis_type", S(chassis_type));
        chassis_args.type = atoi(chassis_type);
        chassis_args.version_len = get_smbios_string("/sys/class/dmi/id/chassis_version", S(chassis_version));
        chassis_args.version = chassis_version;
        chassis_args.serial_len = get_smbios_string("/sys/class/dmi/id/chassis_serial", S(chassis_serial));
        chassis_args.serial = chassis_serial;
        chassis_args.asset_tag_len = get_smbios_string("/sys/class/dmi/id/chassis_tag", S(chassis_asset_tag));
        chassis_args.asset_tag = chassis_asset_tag;
#undef S

        return create_smbios_tables( sfti, available_len, required_len,
                                     &bios_args, &system_args, &board_args, &chassis_args );
    }
    default:
        FIXME("info_class SYSTEM_FIRMWARE_TABLE_INFORMATION provider %08x\n", (int)sfti->ProviderSignature);
        return STATUS_NOT_IMPLEMENTED;
    }
}

#elif defined(__APPLE__)

static NTSTATUS get_smbios_from_iokit( SYSTEM_FIRMWARE_TABLE_INFORMATION *sfti, ULONG available_len,
                                       ULONG *required_len )
{
    io_service_t service;
    CFDataRef data;
    const UInt8 *ptr;
    CFIndex len;
    struct smbios_prologue *prologue;
    BYTE major_version = 2, minor_version = 0;

    if (!(service = IOServiceGetMatchingService(0, IOServiceMatching("AppleSMBIOS"))))
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

static size_t cf_to_string( CFTypeRef type_ref, char *buffer, size_t buffer_size )
{
    size_t length = 0;

    if (!type_ref)
        return 0;

    if (CFGetTypeID(type_ref) == CFDataGetTypeID())
    {
        length = MIN(CFDataGetLength(type_ref), buffer_size);
        CFDataGetBytes(type_ref, CFRangeMake(0, length), (UInt8*)buffer);
        buffer[length] = 0;
        length = strlen(buffer);
    }
    else if (CFGetTypeID(type_ref) == CFStringGetTypeID())
    {
        if (CFStringGetCString(type_ref, buffer, buffer_size, kCFStringEncodingASCII))
            length = strlen(buffer);
    }

    CFRelease(type_ref);
    return length;
}

static NTSTATUS generate_smbios( SYSTEM_FIRMWARE_TABLE_INFORMATION *sfti, ULONG available_len,
                                 ULONG *required_len )
{
    /* Apple Silicon Macs don't have SMBIOS, we need to generate it.
     * Use strings and data from IOKit when available.
     */
    io_service_t platform_expert;
    CFDataRef cf_manufacturer, cf_model;
    CFStringRef cf_serial_number, cf_uuid_string;
    char manufacturer[128], model[128], serial_number[128];
    size_t manufacturer_len = 0, model_len = 0, serial_number_len = 0;
    GUID system_uuid = {0};
    struct smbios_bios_args bios_args;
    struct smbios_system_args system_args;
    struct smbios_board_args board_args;
    struct smbios_chassis_args chassis_args;

    platform_expert = IOServiceGetMatchingService(0, IOServiceMatching("IOPlatformExpertDevice"));
    if (!platform_expert)
        return STATUS_NO_MEMORY;

    cf_manufacturer = IORegistryEntryCreateCFProperty(platform_expert, CFSTR("manufacturer"), kCFAllocatorDefault, 0);
    cf_model = IORegistryEntryCreateCFProperty(platform_expert, CFSTR("model"), kCFAllocatorDefault, 0);
    cf_serial_number = IORegistryEntryCreateCFProperty(platform_expert, CFSTR(kIOPlatformSerialNumberKey), kCFAllocatorDefault, 0);
    cf_uuid_string = IORegistryEntryCreateCFProperty(platform_expert, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);

    manufacturer_len = cf_to_string(cf_manufacturer, manufacturer, sizeof(manufacturer));
    model_len = cf_to_string(cf_model, model, sizeof(model));
    serial_number_len = cf_to_string(cf_serial_number, serial_number, sizeof(serial_number));

    if (cf_uuid_string)
    {
        CFUUIDRef cf_uuid;
        CFUUIDBytes bytes;

        cf_uuid = CFUUIDCreateFromString(kCFAllocatorDefault, cf_uuid_string);
        bytes = CFUUIDGetUUIDBytes(cf_uuid);

        system_uuid.Data1 = (bytes.byte0 << 24) | (bytes.byte1 << 16) | (bytes.byte2 << 8) | bytes.byte3;
        system_uuid.Data2 = (bytes.byte4 << 8) | bytes.byte5;
        system_uuid.Data3 = (bytes.byte6 << 8) | bytes.byte7;
        memcpy(&system_uuid.Data4, &bytes.byte8, sizeof(system_uuid.Data4));

        CFRelease(cf_uuid);
        CFRelease(cf_uuid_string);
    }

    IOObjectRelease(platform_expert);

#define S(s, t) { s ## _len = t ## _len; s = t; }
#define STR(s, t) { s ## _len = sizeof(t)-1; s = t; }
    S(bios_args.vendor, manufacturer);
    /* BIOS version and date are both required */
    STR(bios_args.version, "1.0");
    STR(bios_args.date, "01/01/2021");

    S(system_args.vendor, manufacturer);
    S(system_args.product, model);
    STR(system_args.version, "1.0");
    S(system_args.serial, serial_number);
    system_args.uuid = system_uuid;
    system_args.sku_len = 0;
    S(system_args.family, model);

    S(board_args.vendor, manufacturer);
    S(board_args.product, model);
    S(board_args.version, model);
    S(board_args.serial, serial_number);
    board_args.asset_tag_len = 0;

    S(chassis_args.vendor, manufacturer);
    chassis_args.type = 2; /* unknown */
    chassis_args.version_len = 0;
    S(chassis_args.serial, serial_number);
    chassis_args.asset_tag_len = 0;
#undef STR
#undef S

    return create_smbios_tables( sfti, available_len, required_len,
                                 &bios_args, &system_args, &board_args, &chassis_args );
}

static NTSTATUS get_firmware_info( SYSTEM_FIRMWARE_TABLE_INFORMATION *sfti, ULONG available_len,
                                   ULONG *required_len )
{
    switch (sfti->ProviderSignature)
    {
    case RSMB:
    {
        NTSTATUS ret;
        ret = get_smbios_from_iokit(sfti, available_len, required_len);
        if (ret == STATUS_NO_MEMORY)
            ret = generate_smbios(sfti, available_len, required_len);
        return ret;
    }
    default:
        FIXME("info_class SYSTEM_FIRMWARE_TABLE_INFORMATION provider %08x\n", (unsigned int)sfti->ProviderSignature);
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

    memset( info, 0, sizeof(*info) );

#if defined(linux)
    {
        FILE *fp;

        if ((fp = fopen("/proc/uptime", "r")))
        {
            double uptime, idle_time;

            fscanf(fp, "%lf %lf", &uptime, &idle_time);
            fclose(fp);
            info->IdleTime.QuadPart = 10000000 * idle_time;
        }
    }
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    {
        static int clockrate_name[] = { CTL_KERN, KERN_CLOCKRATE };
        size_t size = 0;
        struct clockinfo clockrate;
        long ptimes[CPUSTATES];

        size = sizeof(clockrate);
        if (!sysctl(clockrate_name, 2, &clockrate, &size, NULL, 0))
        {
            size = sizeof(ptimes);
            if (!sysctlbyname("kern.cp_time", ptimes, &size, NULL, 0))
                info->IdleTime.QuadPart = (ULONGLONG)ptimes[CP_IDLE] * 10000000 / clockrate.stathz;
        }
    }
#elif defined(__APPLE__)
    {
        host_name_port_t host = mach_host_self();
        struct host_cpu_load_info load_info;
        mach_msg_type_number_t count;

        count = HOST_CPU_LOAD_INFO_COUNT;
        if (host_statistics(host, HOST_CPU_LOAD_INFO, (host_info_t)&load_info, &count) == KERN_SUCCESS)
        {
            /* Believe it or not, based on my reading of XNU source, this is
             * already in the units we want (100 ns).
             */
            info->IdleTime.QuadPart = load_info.cpu_ticks[CPU_STATE_IDLE];
        }
        mach_port_deallocate(mach_task_self(), host);
    }
#else
    {
        static ULONGLONG idle;
        /* many programs expect IdleTime to change so fake change */
        info->IdleTime.QuadPart = ++idle;
    }
#endif

#ifdef linux
    {
        FILE *fp;

        if ((fp = fopen("/proc/meminfo", "r")))
        {
            unsigned long long value, mem_available = 0;
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
                else if (sscanf(line, "MemAvailable: %llu", &value))
                    mem_available = value * 1024;
            }
            fclose(fp);
            if (mem_available) freeram = mem_available;
        }
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

    /* Titan Quest refuses to run if TotalPageFile <= TotalPhys */
    if (!totalswap) totalswap = page_size;

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

    /* special case for 23:59:59.999, match with 0:00:00.000 on the following day */
    if (!reg_st->wYear && reg_st->wHour == 23 && reg_st->wMinute == 59 &&
        reg_st->wSecond == 59 && reg_st->wMilliseconds == 999)
        return (st->wDay == wDay + 1 && !st->wHour && !st->wMinute && !st->wSecond && !st->wMilliseconds);

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

    if (count > sizeof(buf) - offsetof(KEY_VALUE_PARTIAL_INFORMATION, Data)) return FALSE;

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
    static const WCHAR Time_ZonesW[] = { '\\','R','e','g','i','s','t','r','y','\\',
        'M','a','c','h','i','n','e','\\',
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

    snprintf( buffer, sizeof(buffer), "%u", year );
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

        TRACE("%s: bias %d\n", debugstr_us(&nameW), (int)reg_tzi.Bias);
        TRACE("std (d/m/y): %u/%02u/%04u day of week %u %u:%02u:%02u.%03u bias %d\n",
              reg_tzi.StandardDate.wDay, reg_tzi.StandardDate.wMonth,
              reg_tzi.StandardDate.wYear, reg_tzi.StandardDate.wDayOfWeek,
              reg_tzi.StandardDate.wHour, reg_tzi.StandardDate.wMinute,
              reg_tzi.StandardDate.wSecond, reg_tzi.StandardDate.wMilliseconds,
              (int)reg_tzi.StandardBias);
        TRACE("dst (d/m/y): %u/%02u/%04u day of week %u %u:%02u:%02u.%03u bias %d\n",
              reg_tzi.DaylightDate.wDay, reg_tzi.DaylightDate.wMonth,
              reg_tzi.DaylightDate.wYear, reg_tzi.DaylightDate.wDayOfWeek,
              reg_tzi.DaylightDate.wHour, reg_tzi.DaylightDate.wMinute,
              reg_tzi.DaylightDate.wSecond, reg_tzi.DaylightDate.wMilliseconds,
              (int)reg_tzi.DaylightBias);

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
          tz_name, (int)tzi->Bias,
          tzi->StandardDate.wDay, tzi->StandardDate.wMonth, tzi->StandardDate.wYear,
          tzi->DaylightDate.wDay, tzi->DaylightDate.wMonth, tzi->DaylightDate.wYear);
}

static time_t find_dst_change(time_t start, time_t end, int *is_dst)
{
    struct tm *tm;
    ULONGLONG min = (sizeof(time_t) == sizeof(int)) ? (ULONG)start : start;
    ULONGLONG max = (sizeof(time_t) == sizeof(int)) ? (ULONG)end : end;

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
            (int)tzi->DaylightBias);

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
            (int)tzi->StandardBias);
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

static unsigned int get_system_process_info( SYSTEM_INFORMATION_CLASS class, void *info, ULONG size, ULONG *len )
{
    unsigned int process_count, total_thread_count, total_name_len, i, j;
    unsigned int thread_info_size;
    unsigned int pos = 0;
    char *buffer = NULL;
    unsigned int ret;

C_ASSERT( sizeof(struct thread_info) <= sizeof(SYSTEM_THREAD_INFORMATION) );
C_ASSERT( sizeof(struct process_info) <= sizeof(SYSTEM_PROCESS_INFORMATION) );

    if (class == SystemExtendedProcessInformation)
        thread_info_size = sizeof(SYSTEM_EXTENDED_THREAD_INFORMATION);
    else
        thread_info_size = sizeof(SYSTEM_THREAD_INFORMATION);

    *len = 0;
    if (size && !(buffer = malloc( size ))) return STATUS_NO_MEMORY;

    SERVER_START_REQ( list_processes )
    {
        wine_server_set_reply( req, buffer, size );
        ret = wine_server_call( req );
        total_thread_count = reply->total_thread_count;
        total_name_len = reply->total_name_len;
        process_count = reply->process_count;
    }
    SERVER_END_REQ;

    if (ret)
    {
        if (ret == STATUS_INFO_LENGTH_MISMATCH)
            *len = sizeof(SYSTEM_PROCESS_INFORMATION) * process_count
                  + (total_name_len + process_count) * sizeof(WCHAR)
                  + total_thread_count * thread_info_size;

        free( buffer );
        return ret;
    }

    for (i = 0; i < process_count; i++)
    {
        SYSTEM_PROCESS_INFORMATION *nt_process = (SYSTEM_PROCESS_INFORMATION *)((char *)info + *len);
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

        proc_len = sizeof(*nt_process) + server_process->thread_count * thread_info_size
                     + (name_len + 1) * sizeof(WCHAR);
        *len += proc_len;

        if (*len <= size)
        {
            memset(nt_process, 0, proc_len);
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
            SYSTEM_EXTENDED_THREAD_INFORMATION *ti;

            if (*len <= size)
            {
                ti = (SYSTEM_EXTENDED_THREAD_INFORMATION *)((BYTE *)nt_process->ti + j * thread_info_size);
                ti->ThreadInfo.CreateTime.QuadPart = server_thread->start_time;
                ti->ThreadInfo.ClientId.UniqueProcess = UlongToHandle(server_process->pid);
                ti->ThreadInfo.ClientId.UniqueThread = UlongToHandle(server_thread->tid);
                ti->ThreadInfo.dwCurrentPriority = server_thread->current_priority;
                ti->ThreadInfo.dwBasePriority = server_thread->base_priority;
                get_thread_times( server_process->unix_pid, server_thread->unix_tid,
                                  &ti->ThreadInfo.KernelTime, &ti->ThreadInfo.UserTime );
                if (class == SystemExtendedProcessInformation)
                {
                    ti->Win32StartAddress = wine_server_get_ptr( server_thread->entry_point );
                    ti->TebBase = wine_server_get_ptr( server_thread->teb );
                }
            }

            pos += sizeof(*server_thread);
        }

        if (*len <= size)
        {
            nt_process->ProcessName.Buffer = (WCHAR *)((BYTE *)nt_process->ti
                                                       + server_process->thread_count * thread_info_size);
            nt_process->ProcessName.Length = name_len * sizeof(WCHAR);
            nt_process->ProcessName.MaximumLength = (name_len + 1) * sizeof(WCHAR);
            memcpy(nt_process->ProcessName.Buffer, file_part, name_len * sizeof(WCHAR));
            nt_process->ProcessName.Buffer[name_len] = 0;
        }
    }

    if (*len > size) ret = STATUS_INFO_LENGTH_MISMATCH;
    free( buffer );
    return ret;
}

/******************************************************************************
 *              NtQuerySystemInformation  (NTDLL.@)
 */
NTSTATUS WINAPI NtQuerySystemInformation( SYSTEM_INFORMATION_CLASS class,
                                          void *info, ULONG size, ULONG *ret_size )
{
    unsigned int ret = STATUS_SUCCESS;
    ULONG len = 0;

    TRACE( "(0x%08x,%p,0x%08x,%p)\n", class, info, (int)size, ret_size );

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
        if (size >= (len = sizeof(cpu_info))) memcpy(info, &cpu_info, len);
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
        static LONGLONG last_bias;
        static time_t last_utc;
        struct tm *tm;
        time_t utc;
        SYSTEM_TIMEOFDAY_INFORMATION sti = {{{ 0 }}};

        sti.BootTime.QuadPart = server_start_time;

        utc = time( NULL );
        pthread_mutex_lock( &timezone_mutex );
        if (utc != last_utc)
        {
            last_utc = utc;
            tm = gmtime( &utc );
            last_bias = mktime( tm ) - utc;
            tm = localtime( &utc );
            if (tm->tm_isdst) last_bias -= 3600;
            last_bias *= TICKSPERSEC;
        }
        sti.TimeZoneBias.QuadPart = last_bias;
        pthread_mutex_unlock( &timezone_mutex );

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
        ret = get_system_process_info( class, info, size, &len );
        break;

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
            host_name_port_t host = mach_host_self ();

            if (host_processor_info( host,
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

            mach_port_deallocate (mach_task_self (), host);
        }
#elif defined(linux)
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
#elif defined(__FreeBSD__) || defined (__FreeBSD_kernel__)
        {
            static int clockrate_name[] = { CTL_KERN, KERN_CLOCKRATE };
            size_t size = 0;
            struct clockinfo clockrate;
            int have_clockrate;
            long *ptimes;

            size = sizeof(clockrate);
            have_clockrate = !sysctl(clockrate_name, 2, &clockrate, &size, NULL, 0);
            size = out_cpus * CPUSTATES * sizeof(long);
            ptimes = malloc(size + 1);
            if (ptimes)
            {
                if (have_clockrate && (!sysctlbyname("kern.cp_times", ptimes, &size, NULL, 0) || errno == ENOMEM))
                {
                    for (cpus = 0; cpus < out_cpus; cpus++)
                    {
                        if (cpus * CPUSTATES * sizeof(long) >= size) break;
                        sppi[cpus].IdleTime.QuadPart = (ULONGLONG)ptimes[cpus*CPUSTATES + CP_IDLE] * 10000000 / clockrate.stathz;
                        sppi[cpus].KernelTime.QuadPart = (ULONGLONG)ptimes[cpus*CPUSTATES + CP_SYS] * 10000000 / clockrate.stathz;
                        sppi[cpus].UserTime.QuadPart = (ULONGLONG)ptimes[cpus*CPUSTATES + CP_USER] * 10000000 / clockrate.stathz;
                    }
                }
                free(ptimes);
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
        ret = get_system_process_info( class, info, size, &len );
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
#elif defined __aarch64__
                *((DWORD *)info) = 128;
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

        virtual_get_system_info( &sbi, is_wow64() );
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
        if (!logical_proc_info)
        {
            ret = STATUS_NOT_IMPLEMENTED;
            break;
        }
        len = logical_proc_info_len * sizeof(*logical_proc_info);
        if (size >= len)
        {
            if (!info) ret = STATUS_ACCESS_VIOLATION;
            else memcpy( info, logical_proc_info, len);
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
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

        FIXME("SystemCodeIntegrityInformation, size %u, info %p, stub!\n", (int)size, info);

        len = sizeof(SYSTEM_CODEINTEGRITY_INFORMATION);

        if (size >= len)
            integrity_info->CodeIntegrityOptions = CODEINTEGRITY_OPTION_ENABLED;
        else
            ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    }

    case SystemProcessorBrandString:  /* 105 */
        if (!cpu_name[0]) return STATUS_NOT_SUPPORTED;
        if ((ULONG_PTR)info & 3) return STATUS_DATATYPE_MISALIGNMENT;
        len = sizeof(cpu_name);
        if (size >= len)
            memcpy( info, cpu_name, len );
        else
            ret = STATUS_INFO_LENGTH_MISMATCH;
        break;

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

    case SystemProcessorFeaturesInformation:  /* 154 */
        len = sizeof(cpu_features);
        if (size >= len) memcpy( info, &cpu_features, len );
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;

    case SystemCpuSetInformation:  /* 175 */
        return NtQuerySystemInformationEx(class, NULL, 0, info, size, ret_size);

    /* Wine extensions */

    case SystemWineVersionInformation:  /* 1000 */
    {
        static const char version[] = PACKAGE_VERSION;
        struct utsname buf;

        uname( &buf );
        snprintf( info, size, "%s%c%s%c%s%c%s", version, 0, wine_build, 0, buf.sysname, 0, buf.release );
        len = strlen(version) + strlen(wine_build) + strlen(buf.sysname) + strlen(buf.release) + 4;
        if (size < len) ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    }

    default:
	FIXME( "(0x%08x,%p,0x%08x,%p) stub\n", class, info, (int)size, ret_size );

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
    unsigned int ret = STATUS_NOT_IMPLEMENTED;

    TRACE( "(0x%08x,%p,%u,%p,%u,%p) stub\n", class, query, (int)query_len, info, (int)size, ret_size );

    switch (class)
    {
    case SystemLogicalProcessorInformationEx:
    {
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *p;
        DWORD relation;

        if (!query || query_len < sizeof(DWORD))
        {
            ret = STATUS_INVALID_PARAMETER;
            break;
        }
        if (!logical_proc_info_ex)
        {
            ret = STATUS_NOT_IMPLEMENTED;
            break;
        }

        relation = *(DWORD *)query;
        len = 0;
        p = logical_proc_info_ex;
        while ((char *)p != (char *)logical_proc_info_ex + logical_proc_info_ex_size)
        {
            if (relation == RelationAll || p->Relationship == relation)
            {
                if (len + p->Size <= size)
                    memcpy( (char *)info + len, p, p->Size );
                len += p->Size;
            }
            p = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)((char *)p + p->Size);
        }
        ret = size >= len ? STATUS_SUCCESS : STATUS_INFO_LENGTH_MISMATCH;
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
        SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION *machines = info;
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

        len = (supported_machines_count + 1) * sizeof(*machines);
        if (size < len)
        {
            ret = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        memset( machines, 0, len );

        /* native machine */
        machines[0].Machine = supported_machines[0];
        machines[0].UserMode = 1;
        machines[0].KernelMode = 1;
        machines[0].Native = 1;
        machines[0].Process = (supported_machines[0] == machine || is_machine_64bit( machine ));
        machines[0].WoW64Container = 0;
        machines[0].ReservedZero0 = 0;
        /* wow64 machines */
        for (i = 1; i < supported_machines_count; i++)
        {
            machines[i].Machine = supported_machines[i];
            machines[i].UserMode = 1;
            machines[i].Process = supported_machines[i] == machine;
            machines[i].WoW64Container = 1;
        }
        ret = STATUS_SUCCESS;
        break;
    }

    default:
        FIXME( "(0x%08x,%p,%u,%p,%u,%p) stub\n", class, query, (int)query_len, info, (int)size, ret_size );
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
    FIXME( "(0x%08x,%p,0x%08x) stub\n", class, info, (int)length );
    return STATUS_SUCCESS;
}


/******************************************************************************
 *              NtQuerySystemEnvironmentValue  (NTDLL.@)
 */
NTSTATUS WINAPI NtQuerySystemEnvironmentValue( UNICODE_STRING *name, WCHAR *buffer, ULONG length,
                                               ULONG *retlen )
{
    FIXME( "(%s, %p, %u, %p), stub\n", debugstr_us(name), buffer, (int)length, retlen );
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
    FIXME( "(%d, %p, %d, %p, %d, %p), stub\n",
           command, in_buff, (int)in_len, out_buff, (int)out_len, retlen );

    return STATUS_DEBUGGER_INACTIVE;
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

static const char * get_sys_str(const char *dirname, const char *basename, char *s)
{
    char path[64];
    FILE *f;
    const char *ret = NULL;

    if (snprintf(path, sizeof(path), "%s/%s", dirname, basename) >= sizeof(path)) return NULL;
    if ((f = fopen(path, "r")))
    {
        if (fgets(s, 16, f)) ret = s;
        fclose(f);
    }
    return ret;
}

static int get_sys_int(const char *dirname, const char *basename)
{
    char s[16];
    return get_sys_str(dirname, basename, s) ? atoi(s) : 0;
}

static NTSTATUS fill_battery_state( SYSTEM_BATTERY_STATE *bs )
{
    DIR *d = opendir("/sys/class/power_supply");
    struct dirent *de;
    char s[16], path[64];
    BOOL found_ac = FALSE;
    LONG64 voltage; /* microvolts */

    bs->AcOnLine = TRUE;
    if (!d) return STATUS_SUCCESS;

    while ((de = readdir(d)))
    {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
        if (snprintf(path, sizeof(path), "/sys/class/power_supply/%s", de->d_name) >= sizeof(path)) continue;
        if (get_sys_str(path, "scope", s) && strcmp(s, "Device\n") == 0) continue;
        if (!get_sys_str(path, "type", s)) continue;

        if (strcmp(s, "Mains\n") == 0)
        {
            if (!get_sys_str(path, "online", s)) continue;
            if (found_ac)
            {
                FIXME("Multiple mains found, only reporting on the first\n");
            }
            else
            {
                bs->AcOnLine = atoi(s);
                found_ac = TRUE;
            }
        }
        else if (strcmp(s, "Battery\n") == 0)
        {
            if (!get_sys_str(path, "status", s)) continue;
            if (bs->BatteryPresent)
            {
                FIXME("Multiple batteries found, only reporting on the first\n");
            }
            else
            {
                bs->Charging = (strcmp(s, "Charging\n") == 0);
                bs->Discharging = (strcmp(s, "Discharging\n") == 0);
                bs->BatteryPresent = TRUE;
                voltage = get_sys_int(path, "voltage_now");
                bs->MaxCapacity = get_sys_int(path, "charge_full") * voltage / 1e9;
                bs->RemainingCapacity = get_sys_int(path, "charge_now") * voltage / 1e9;
                bs->Rate = -get_sys_int(path, "current_now") * voltage / 1e9;
                if (!bs->Charging && (LONG)bs->Rate < 0)
                    bs->EstimatedTime = 3600 * bs->RemainingCapacity / -(LONG)bs->Rate;
                else
                    bs->EstimatedTime = ~0u;
            }
        }
    }

    closedir(d);
    return STATUS_SUCCESS;
}

#elif defined(__APPLE__)

static NTSTATUS fill_battery_state( SYSTEM_BATTERY_STATE *bs )
{
    CFTypeRef blob = IOPSCopyPowerSourcesInfo();
    CFArrayRef sources = IOPSCopyPowerSourcesList( blob );
    CFIndex count, i;
    CFDictionaryRef source = NULL;
    CFTypeRef prop;
    Boolean is_charging, is_internal, is_present;
    int32_t value, voltage;

    if (!sources)
    {
        if (blob) CFRelease( blob );
        return STATUS_ACCESS_DENIED;
    }

    count = CFArrayGetCount( sources );

    for (i = 0; i < count; i++)
    {
        source = IOPSGetPowerSourceDescription( blob, CFArrayGetValueAtIndex( sources, i ) );

        if (!source)
            continue;

        prop = CFDictionaryGetValue( source, CFSTR(kIOPSTransportTypeKey) );
        is_internal = !CFStringCompare( prop, CFSTR(kIOPSInternalType), 0 );

        prop = CFDictionaryGetValue( source, CFSTR(kIOPSIsPresentKey) );
        is_present = CFBooleanGetValue( prop );

        if (is_internal && is_present)
            break;
    }

    CFRelease( blob );

    if (!source)
    {
        /* Just assume we're on AC with no internal power source. */
        bs->AcOnLine = TRUE;
        CFRelease( sources );
        return STATUS_SUCCESS;
    }

    bs->BatteryPresent = TRUE;

    prop = CFDictionaryGetValue( source, CFSTR(kIOPSIsChargingKey) );
    is_charging = CFBooleanGetValue( prop );

    prop = CFDictionaryGetValue( source, CFSTR(kIOPSPowerSourceStateKey) );

    if (!CFStringCompare( prop, CFSTR(kIOPSACPowerValue), 0 ))
    {
        bs->AcOnLine = TRUE;
        if (is_charging)
            bs->Charging = TRUE;
    }
    else
        bs->Discharging = TRUE;

    /* We'll need the voltage to be able to interpret the other values. */
    prop = CFDictionaryGetValue( source, CFSTR(kIOPSVoltageKey) );
    if (prop)
        CFNumberGetValue( prop, kCFNumberIntType, &voltage );
    else
        /* kIOPSVoltageKey is optional and might not be populated.
         * Assume 11.4 V then, which is a common value for Apple laptops. */
        voltage = 11400;

    prop = CFDictionaryGetValue( source, CFSTR(kIOPSMaxCapacityKey) );
    CFNumberGetValue( prop, kCFNumberIntType, &value );
    bs->MaxCapacity = value * voltage;
    /* Apple uses "estimated time < 10:00" and "22%" for these, but we'll follow
     * Windows for now (5% and 33%). */
    bs->DefaultAlert1 = bs->MaxCapacity / 20;
    bs->DefaultAlert2 = bs->MaxCapacity / 3;

    prop = CFDictionaryGetValue( source, CFSTR(kIOPSCurrentCapacityKey) );
    CFNumberGetValue( prop, kCFNumberIntType, &value );
    bs->RemainingCapacity = value * voltage;

    prop = CFDictionaryGetValue( source, CFSTR(kIOPSCurrentKey) );
    if (prop)
        CFNumberGetValue( prop, kCFNumberIntType, &value );
    else
        /* kIOPSCurrentKey is optional and might not be populated. */
        value = 0;

    bs->Rate = value * voltage / 1000;

    prop = CFDictionaryGetValue( source, CFSTR(kIOPSTimeToEmptyKey) );
    if (prop)
    {
        CFNumberGetValue( prop, kCFNumberIntType, &value );
        if (value > 0)
            /*  A value of -1 indicates "Still Calculating the Time",
             * otherwise estimated minutes left on the battery. */
            bs->EstimatedTime = value * 60;
    }

    CFRelease( sources );
    return STATUS_SUCCESS;
}

#elif defined(__FreeBSD__)

#include <dev/acpica/acpiio.h>

static NTSTATUS fill_battery_state( SYSTEM_BATTERY_STATE *bs )
{
    size_t len;
    int state = 0;
    int rate_mW = 0;
    int time_mins = -1;
    int life_percent = 0;

    bs->BatteryPresent = TRUE;
    len = sizeof(state);
    bs->BatteryPresent &= !sysctlbyname("hw.acpi.battery.state", &state, &len, NULL, 0);
    len = sizeof(rate_mW);
    bs->BatteryPresent &= !sysctlbyname("hw.acpi.battery.rate", &rate_mW, &len, NULL, 0);
    len = sizeof(time_mins);
    bs->BatteryPresent &= !sysctlbyname("hw.acpi.battery.time", &time_mins, &len, NULL, 0);
    len = sizeof(life_percent);
    bs->BatteryPresent &= !sysctlbyname("hw.acpi.battery.life", &life_percent, &len, NULL, 0);

    if (bs->BatteryPresent)
    {
        bs->AcOnLine = (time_mins == -1);
        bs->Charging = state & ACPI_BATT_STAT_CHARGING;
        bs->Discharging = state & ACPI_BATT_STAT_DISCHARG;

        bs->Rate = (rate_mW >= 0 ? -rate_mW : 0);
        if (time_mins >= 0 && life_percent > 0)
        {
            bs->EstimatedTime = 60 * time_mins;
            bs->RemainingCapacity = bs->EstimatedTime * rate_mW / 3600;
            bs->MaxCapacity = bs->RemainingCapacity * 100 / life_percent;
        }
        else
        {
            bs->EstimatedTime = ~0u;
            bs->RemainingCapacity = life_percent;
            bs->MaxCapacity = 100;
        }
    }
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
    TRACE( "(%d,%p,%d,%p,%d)\n", level, input, (int)in_size, output, (int)out_size );
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
            unsigned int val;
            char filename[128];
            FILE* f;

            for(i = 0; i < out_cpus; i++) {
                snprintf(filename, sizeof(filename), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", i);
                f = fopen(filename, "r");
                if (f && (fscanf(f, "%u", &val) == 1)) {
                    cpu_power[i].MaxMhz = val / 1000;
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

                snprintf(filename, sizeof(filename), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
                f = fopen(filename, "r");
                if(f && (fscanf(f, "%u", &val) == 1)) {
                    cpu_power[i].MhzLimit = val / 1000;
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
            TRACE("cpu_power[%d] = %u %u %u %u %u %u\n", i, (int)cpu_power[i].Number,
                  (int)cpu_power[i].MaxMhz, (int)cpu_power[i].CurrentMhz, (int)cpu_power[i].MhzLimit,
                  (int)cpu_power[i].MaxIdleState, (int)cpu_power[i].CurrentIdleState);
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
    FIXME( "%08x stub\n", (int)status );
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *              NtInitiatePowerAction  (NTDLL.@)
 */
NTSTATUS WINAPI NtInitiatePowerAction( POWER_ACTION action, SYSTEM_POWER_STATE state,
                                       ULONG flags, BOOLEAN async )
{
    FIXME( "(%d,%d,0x%08x,%d),stub\n", action, state, (int)flags, async );
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *              NtSetThreadExecutionState  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetThreadExecutionState( EXECUTION_STATE new_state, EXECUTION_STATE *old_state )
{
    static EXECUTION_STATE current = ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED | ES_USER_PRESENT;

    WARN( "(0x%x, %p): stub, harmless.\n", (int)new_state, old_state );
    *old_state = current;
    if (!(current & ES_CONTINUOUS) || (new_state & ES_CONTINUOUS)) current = new_state;
    return STATUS_SUCCESS;
}
