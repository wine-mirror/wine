/*
 * Copyright (C) 2002 Andreas Mohr
 * Copyright (C) 2002 Shachar Shemesh
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
/* Wine "bootup" handler application
 *
 * This app handles the various "hooks" windows allows for applications to perform
 * as part of the bootstrap process. These are roughly divided into three types.
 * Knowledge base articles that explain this are 137367, 179365, 232487 and 232509.
 * Also, 119941 has some info on grpconv.exe
 * The operations performed are (by order of execution):
 *
 * Preboot (prior to fully loading the Windows kernel):
 * - wininit.exe (rename operations left in wininit.ini - Win 9x only)
 * - PendingRenameOperations (rename operations left in the registry - Win NT+ only)
 *
 * Startup (before the user logs in)
 * - Services (NT)
 * - HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\RunServicesOnce (9x, asynch)
 * - HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\RunServices (9x, asynch)
 * 
 * After log in
 * - HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\RunOnce (all, synch)
 * - HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Run (all, asynch)
 * - HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run (all, asynch)
 * - Startup folders (all, ?asynch?)
 * - HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\RunOnce (all, asynch)
 *
 * Somewhere in there is processing the RunOnceEx entries (also no imp)
 * 
 * Bugs:
 * - If a pending rename registry does not start with \??\ the entry is
 *   processed anyways. I'm not sure that is the Windows behaviour.
 * - Need to check what is the windows behaviour when trying to delete files
 *   and directories that are read-only
 * - In the pending rename registry processing - there are no traces of the files
 *   processed (requires translations from Unicode to Ansi).
 */

#define COBJMACROS

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <ws2tcpip.h>
#include <winternl.h>
#include <ddk/wdm.h>
#include <sddl.h>
#include <wine/svcctl.h>
#include <wine/list.h>
#include <wine/asm.h>
#include <wine/debug.h>

#include <shlobj.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <setupapi.h>
#include <newdev.h>
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(wineboot);

extern BOOL shutdown_close_windows( BOOL force );
extern BOOL shutdown_all_desktops( BOOL force );
extern void kill_processes( BOOL kill_desktop );

static WCHAR windowsdir[MAX_PATH];
static const BOOL is_64bit = sizeof(void *) > sizeof(int);
static SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION machines[8];

/* retrieve the path to the wine.inf file */
static WCHAR *get_wine_inf_path(void)
{
    WCHAR *dir, *name = NULL;

    if ((dir = _wgetenv( L"WINEBUILDDIR" )))
    {
        if (!(name = malloc( sizeof(L"\\loader\\wine.inf") + wcslen(dir) * sizeof(WCHAR) )))
            return NULL;
        lstrcpyW( name, dir );
        lstrcatW( name, L"\\loader" );
    }
    else if ((dir = _wgetenv( L"WINEDATADIR" )))
    {
        if (!(name = malloc( sizeof(L"\\wine.inf") + wcslen(dir) * sizeof(WCHAR) )))
            return NULL;
        lstrcpyW( name, dir );
    }
    else return NULL;

    lstrcatW( name, L"\\wine.inf" );
    name[1] = '\\';  /* change \??\ to \\?\ */
    return name;
}

/* update the timestamp if different from the reference time */
static BOOL update_timestamp( const WCHAR *config_dir, unsigned long timestamp )
{
    BOOL ret = FALSE;
    int fd, count;
    char buffer[100];
    WCHAR *file = malloc( wcslen(config_dir) * sizeof(WCHAR) + sizeof(L"\\.update-timestamp") );

    if (!file) return FALSE;
    lstrcpyW( file, config_dir );
    lstrcatW( file, L"\\.update-timestamp" );

    if ((fd = _wopen( file, O_RDWR )) != -1)
    {
        if ((count = read( fd, buffer, sizeof(buffer) - 1 )) >= 0)
        {
            buffer[count] = 0;
            if (!strncmp( buffer, "disable", sizeof("disable")-1 )) goto done;
            if (timestamp == strtoul( buffer, NULL, 10 )) goto done;
        }
        lseek( fd, 0, SEEK_SET );
        chsize( fd, 0 );
    }
    else
    {
        if (errno != ENOENT) goto done;
        if ((fd = _wopen( file, O_WRONLY | O_CREAT | O_TRUNC, 0666 )) == -1) goto done;
    }

    count = sprintf( buffer, "%lu\n", timestamp );
    if (write( fd, buffer, count ) != count)
    {
        WINE_WARN( "failed to update timestamp in %s\n", debugstr_w(file) );
        chsize( fd, 0 );
    }
    else ret = TRUE;

done:
    if (fd != -1) close( fd );
    free( file );
    return ret;
}

/* print the config directory in a more Unix-ish way */
static const WCHAR *prettyprint_configdir(void)
{
    static WCHAR buffer[MAX_PATH];
    WCHAR *p, *path = _wgetenv( L"WINECONFIGDIR" );

    lstrcpynW( buffer, path, ARRAY_SIZE(buffer) );
    if (lstrlenW( path ) >= ARRAY_SIZE(buffer) )
        lstrcpyW( buffer + ARRAY_SIZE(buffer) - 4, L"..." );

    if (!wcsncmp( buffer, L"\\??\\unix\\", 9 ))
    {
        for (p = buffer + 9; *p; p++) if (*p == '\\') *p = '/';
        return buffer + 9;
    }
    else if (!wcsncmp( buffer, L"\\??\\Z:\\", 7 ))
    {
        for (p = buffer + 6; *p; p++) if (*p == '\\') *p = '/';
        return buffer + 6;
    }
    else return buffer + 4;
}

/* wrapper for RegSetValueExW */
static DWORD set_reg_value( HKEY hkey, const WCHAR *name, const WCHAR *value )
{
    return RegSetValueExW( hkey, name, 0, REG_SZ, (const BYTE *)value, (lstrlenW(value) + 1) * sizeof(WCHAR) );
}

static DWORD set_reg_value_dword( HKEY hkey, const WCHAR *name, DWORD value )
{
    return RegSetValueExW( hkey, name, 0, REG_DWORD, (const BYTE *)&value, sizeof(value) );
}

#if defined(__i386__) || defined(__x86_64__)

extern UINT64 WINAPI do_xgetbv( unsigned int cx);
#ifdef __i386__
__ASM_STDCALL_FUNC( do_xgetbv, 4,
                   "movl 4(%esp),%ecx\n\t"
                   "xgetbv\n\t"
                   "ret $4" )
#else
__ASM_GLOBAL_FUNC( do_xgetbv,
                   "xgetbv\n\t"
                   "shlq $32,%rdx\n\t"
                   "orq %rdx,%rax\n\t"
                   "ret" )
#endif

static void initialize_xstate_features(struct _KUSER_SHARED_DATA *data)
{
    static const ULONG64 wine_xstate_supported_features = 0xfc; /* XSTATE_AVX, XSTATE_MPX_BNDREGS, XSTATE_MPX_BNDCSR,
                                                                 * XSTATE_AVX512_KMASK, XSTATE_AVX512_ZMM_H, XSTATE_AVX512_ZMM */
    XSTATE_CONFIGURATION *xstate = &data->XState;
    ULONG64 supported_mask;
    unsigned int i, off;
    int regs[4];

    if (!data->ProcessorFeatures[PF_AVX_INSTRUCTIONS_AVAILABLE])
        return;

    __cpuidex(regs, 0, 0);

    TRACE("Max cpuid level %#x.\n", regs[0]);
    if (regs[0] < 0xd)
        return;

    __cpuidex(regs, 1, 0);
    TRACE("CPU features %#x, %#x, %#x, %#x.\n", regs[0], regs[1], regs[2], regs[3]);
    if (!(regs[2] & (0x1 << 27))) /* xsave OS enabled */
        return;

    __cpuidex(regs, 0xd, 0);
    TRACE("XSAVE details %#x, %#x, %#x, %#x.\n", regs[0], regs[1], regs[2], regs[3]);
    supported_mask = ((ULONG64)regs[3] << 32) | regs[0];
    supported_mask &= do_xgetbv(0) & wine_xstate_supported_features;
    if (!(supported_mask >> 2))
        return;

    xstate->EnabledFeatures = (1 << XSTATE_LEGACY_FLOATING_POINT) | (1 << XSTATE_LEGACY_SSE) | supported_mask;
    xstate->EnabledVolatileFeatures = xstate->EnabledFeatures;
    xstate->AllFeatureSize = regs[1];

    __cpuidex(regs, 0xd, 1);
    xstate->OptimizedSave = regs[0] & 1;
    xstate->CompactionEnabled = !!(regs[0] & 2);

    xstate->Features[0].Size = xstate->AllFeatures[0] = offsetof(XSAVE_FORMAT, XmmRegisters);
    xstate->Features[1].Size = xstate->AllFeatures[1] = sizeof(M128A) * 16;
    xstate->Features[1].Offset = xstate->Features[0].Size;
    off = sizeof(XSAVE_FORMAT) + sizeof(XSAVE_AREA_HEADER);
    supported_mask >>= 2;
    for (i = 2; supported_mask; ++i, supported_mask >>= 1)
    {
        if (!(supported_mask & 1)) continue;
        __cpuidex( regs, 0xd, i );
        xstate->Features[i].Offset = regs[1];
        xstate->Features[i].Size = xstate->AllFeatures[i] = regs[0];
        if (regs[2] & 2)
        {
            xstate->AlignedFeatures |= (ULONG64)1 << i;
            off = (off + 63) & ~63;
        }
        off += xstate->Features[i].Size;
        TRACE("xstate[%d] offset %lu, size %lu, aligned %d.\n", i, xstate->Features[i].Offset, xstate->Features[i].Size, !!(regs[2] & 2));
    }
    xstate->Size = xstate->CompactionEnabled ? off : xstate->Features[i - 1].Offset + xstate->Features[i - 1].Size;
    TRACE("xstate size %lu, compacted %d, optimized %d.\n", xstate->Size, xstate->CompactionEnabled, xstate->OptimizedSave);
}

static BOOL is_tsc_trusted_by_the_kernel(void)
{
    char buf[4] = {0};
    DWORD num_read;
    HANDLE handle;
    BOOL ret = TRUE;

    /* Darwin for x86-64 uses the TSC internally for timekeeping, so it can always
     * be trusted.
     * For BSDs there seems to be no unified interface to query TSC quality.
     * If there is a sysfs entry with clocksource information, use it to check though. */
    handle = CreateFileW( L"\\??\\unix\\sys\\bus\\clocksource\\devices\\clocksource0\\current_clocksource",
                          GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
    if (handle == INVALID_HANDLE_VALUE) return TRUE;

    if (ReadFile( handle, buf, sizeof(buf) - 1, &num_read, NULL ) && strcmp( "tsc", buf ))
        ret = FALSE;

    CloseHandle( handle );
    return ret;
}

static UINT64 read_tsc_frequency(void)
{
    UINT64 freq = 0;
    LONGLONG time0, time1, tsc0, tsc1, tsc2, tsc3, freq0, freq1, error;
    BOOL has_rdtscp = FALSE;
    unsigned int aux;
    UINT retries = 50;
    int regs[4];

    if (!is_tsc_trusted_by_the_kernel())
    {
        WARN( "Failed to compute TSC frequency, not trusted by the kernel.\n" );
        return 0;
    }

    __cpuid(regs, 1);
    if (!(regs[3] & (1 << 4)))
    {
        WARN( "Failed to compute TSC frequency, RDTSC instruction not supported.\n" );
        return 0;
    }

    __cpuid( regs, 0x80000000 );
    if (regs[0] < 0x80000007)
    {
        WARN( "Failed to compute TSC frequency, unable to check invariant TSC.\n" );
        return 0;
    }

    /* check for invariant tsc bit */
    __cpuid( regs, 0x80000007 );
    if (!(regs[3] & (1 << 8)))
    {
        WARN( "Failed to compute TSC frequency, no invariant TSC.\n" );
        return 0;
    }

    /* check for rdtscp support bit */
    __cpuid( regs, 0x80000001 );
    if ((regs[3] & (1 << 27))) has_rdtscp = TRUE;

    do
    {
        if (has_rdtscp)
        {
            tsc0 = __rdtscp( &aux );
            time0 = RtlGetSystemTimePrecise();
            tsc1 = __rdtscp( &aux );
            Sleep( 1 );
            tsc2 = __rdtscp( &aux );
            time1 = RtlGetSystemTimePrecise();
            tsc3 = __rdtscp( &aux );
        }
        else
        {
            tsc0 = __rdtsc(); __cpuid( regs, 0 );
            time0 = RtlGetSystemTimePrecise();
            tsc1 = __rdtsc(); __cpuid( regs, 0 );
            Sleep( 1 );
            tsc2 = __rdtsc(); __cpuid( regs, 0 );
            time1 = RtlGetSystemTimePrecise();
            tsc3 = __rdtsc(); __cpuid( regs, 0 );
        }

        freq0 = (tsc2 - tsc0) * 10000000 / (time1 - time0);
        freq1 = (tsc3 - tsc1) * 10000000 / (time1 - time0);
        error = llabs( (freq1 - freq0) * 1000000 / min( freq1, freq0 ) );
    }
    while (error > 500 && --retries);

    if (!retries) WARN( "TSC frequency calibration failed, unstable TSC?\n" );
    else
    {
        freq = (freq0 + freq1) / 2;
        TRACE( "TSC frequency calibration complete, found %I64u Hz\n", freq );
    }

    return freq;
}

#else

static void initialize_xstate_features(struct _KUSER_SHARED_DATA *data)
{
}

static UINT64 read_tsc_frequency(void)
{
    return 0;
}

#endif

static void create_user_shared_data(void)
{
    struct _KUSER_SHARED_DATA *data;
    RTL_OSVERSIONINFOEXW version;
    SYSTEM_CPU_INFORMATION sci;
    BOOLEAN *features;
    OBJECT_ATTRIBUTES attr = {sizeof(attr)};
    UNICODE_STRING name = RTL_CONSTANT_STRING( L"\\KernelObjects\\__wine_user_shared_data" );
    NTSTATUS status;
    HANDLE handle;
    ULONG i;

    InitializeObjectAttributes( &attr, &name, OBJ_OPENIF, NULL, NULL );
    if ((status = NtOpenSection( &handle, SECTION_ALL_ACCESS, &attr )))
    {
        ERR( "cannot open __wine_user_shared_data: %lx\n", status );
        return;
    }
    data = MapViewOfFile( handle, FILE_MAP_WRITE, 0, 0, sizeof(*data) );
    CloseHandle( handle );
    if (!data)
    {
        ERR( "cannot map __wine_user_shared_data\n" );
        return;
    }

    version.dwOSVersionInfoSize = sizeof(version);
    RtlGetVersion( &version );
    NtQuerySystemInformation( SystemCpuInformation, &sci, sizeof(sci), NULL );

    data->NtBuildNumber               = version.dwBuildNumber;
    data->NtProductType               = version.wProductType;
    data->ProductTypeIsValid          = TRUE;
    data->NativeProcessorArchitecture = sci.ProcessorArchitecture;
    data->NtMajorVersion              = version.dwMajorVersion;
    data->NtMinorVersion              = version.dwMinorVersion;
    data->SuiteMask                   = version.wSuiteMask;
    wcscpy( data->NtSystemRoot, L"C:\\windows" );

    features = data->ProcessorFeatures;
    switch (sci.ProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_INTEL:
    case PROCESSOR_ARCHITECTURE_AMD64:
        features[PF_COMPARE_EXCHANGE_DOUBLE]              = !!(sci.ProcessorFeatureBits & CPU_FEATURE_CX8);
        features[PF_MMX_INSTRUCTIONS_AVAILABLE]           = !!(sci.ProcessorFeatureBits & CPU_FEATURE_MMX);
        features[PF_XMMI_INSTRUCTIONS_AVAILABLE]          = !!(sci.ProcessorFeatureBits & CPU_FEATURE_SSE);
        features[PF_3DNOW_INSTRUCTIONS_AVAILABLE]         = !!(sci.ProcessorFeatureBits & CPU_FEATURE_3DNOW);
        features[PF_RDTSC_INSTRUCTION_AVAILABLE]          = !!(sci.ProcessorFeatureBits & CPU_FEATURE_TSC);
        features[PF_PAE_ENABLED]                          = !!(sci.ProcessorFeatureBits & CPU_FEATURE_PAE);
        features[PF_XMMI64_INSTRUCTIONS_AVAILABLE]        = !!(sci.ProcessorFeatureBits & CPU_FEATURE_SSE2);
        features[PF_SSE3_INSTRUCTIONS_AVAILABLE]          = !!(sci.ProcessorFeatureBits & CPU_FEATURE_SSE3);
        features[PF_SSSE3_INSTRUCTIONS_AVAILABLE]         = !!(sci.ProcessorFeatureBits & CPU_FEATURE_SSSE3);
        features[PF_XSAVE_ENABLED]                        = !!(sci.ProcessorFeatureBits & CPU_FEATURE_XSAVE);
        features[PF_COMPARE_EXCHANGE128]                  = !!(sci.ProcessorFeatureBits & CPU_FEATURE_CX128);
        features[PF_SSE_DAZ_MODE_AVAILABLE]               = !!(sci.ProcessorFeatureBits & CPU_FEATURE_DAZ);
        features[PF_NX_ENABLED]                           = !!(sci.ProcessorFeatureBits & CPU_FEATURE_NX);
        features[PF_SECOND_LEVEL_ADDRESS_TRANSLATION]     = !!(sci.ProcessorFeatureBits & CPU_FEATURE_2NDLEV);
        features[PF_VIRT_FIRMWARE_ENABLED]                = !!(sci.ProcessorFeatureBits & CPU_FEATURE_VIRT);
        features[PF_RDWRFSGSBASE_AVAILABLE]               = !!(sci.ProcessorFeatureBits & CPU_FEATURE_RDFS);
        features[PF_FASTFAIL_AVAILABLE]                   = TRUE;
        features[PF_SSE4_1_INSTRUCTIONS_AVAILABLE]        = !!(sci.ProcessorFeatureBits & CPU_FEATURE_SSE41);
        features[PF_SSE4_2_INSTRUCTIONS_AVAILABLE]        = !!(sci.ProcessorFeatureBits & CPU_FEATURE_SSE42);
        features[PF_AVX_INSTRUCTIONS_AVAILABLE]           = !!(sci.ProcessorFeatureBits & CPU_FEATURE_AVX);
        features[PF_AVX2_INSTRUCTIONS_AVAILABLE]          = !!(sci.ProcessorFeatureBits & CPU_FEATURE_AVX2);
        break;

    case PROCESSOR_ARCHITECTURE_ARM:
        features[PF_ARM_VFP_32_REGISTERS_AVAILABLE]       = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_VFP_32);
        features[PF_ARM_NEON_INSTRUCTIONS_AVAILABLE]      = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_NEON);
        break;

    case PROCESSOR_ARCHITECTURE_ARM64:
        features[PF_ARM_V8_INSTRUCTIONS_AVAILABLE]        = TRUE;
        features[PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE]  = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_V8_CRC32);
        features[PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE] = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_V8_CRYPTO);
        features[PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE]= !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_V81_ATOMIC);
        features[PF_ARM_V82_DP_INSTRUCTIONS_AVAILABLE]    = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_V82_DP);
        features[PF_ARM_V83_JSCVT_INSTRUCTIONS_AVAILABLE] = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_V83_JSCVT);
        features[PF_ARM_V83_LRCPC_INSTRUCTIONS_AVAILABLE] = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_V83_LRCPC);
        features[PF_ARM_SVE_INSTRUCTIONS_AVAILABLE]       = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_SVE);
        features[PF_ARM_SVE2_INSTRUCTIONS_AVAILABLE]      = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_SVE2);
        features[PF_ARM_SVE2_1_INSTRUCTIONS_AVAILABLE]    = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_SVE2_1);
        features[PF_ARM_SVE_AES_INSTRUCTIONS_AVAILABLE]   = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_SVE_AES);
        features[PF_ARM_SVE_PMULL128_INSTRUCTIONS_AVAILABLE] = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_SVE_PMULL128);
        features[PF_ARM_SVE_BITPERM_INSTRUCTIONS_AVAILABLE] = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_SVE_BITPERM);
        features[PF_ARM_SVE_BF16_INSTRUCTIONS_AVAILABLE]  = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_SVE_BF16);
        features[PF_ARM_SVE_EBF16_INSTRUCTIONS_AVAILABLE] = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_SVE_EBF16);
        features[PF_ARM_SVE_B16B16_INSTRUCTIONS_AVAILABLE] = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_SVE_B16B16);
        features[PF_ARM_SVE_SHA3_INSTRUCTIONS_AVAILABLE]  = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_SVE_SHA3);
        features[PF_ARM_SVE_SM4_INSTRUCTIONS_AVAILABLE]   = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_SVE_SM4);
        features[PF_ARM_SVE_I8MM_INSTRUCTIONS_AVAILABLE]  = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_SVE_I8MM);
        features[PF_ARM_SVE_F32MM_INSTRUCTIONS_AVAILABLE] = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_SVE_F32MM);
        features[PF_ARM_SVE_F64MM_INSTRUCTIONS_AVAILABLE] = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_SVE_F64MM);
        features[PF_COMPARE_EXCHANGE_DOUBLE]              = TRUE;
        features[PF_NX_ENABLED]                           = TRUE;
        features[PF_FASTFAIL_AVAILABLE]                   = TRUE;
        /* add features for other architectures supported by wow64 */
        for (i = 0; machines[i].Machine; i++)
        {
            switch (machines[i].Machine)
            {
            case IMAGE_FILE_MACHINE_ARMNT:
                features[PF_ARM_VFP_32_REGISTERS_AVAILABLE]  = TRUE;
                features[PF_ARM_NEON_INSTRUCTIONS_AVAILABLE] = TRUE;
                break;
            case IMAGE_FILE_MACHINE_I386:
                features[PF_MMX_INSTRUCTIONS_AVAILABLE]    = TRUE;
                features[PF_XMMI_INSTRUCTIONS_AVAILABLE]   = TRUE;
                features[PF_RDTSC_INSTRUCTION_AVAILABLE]   = TRUE;
                features[PF_XMMI64_INSTRUCTIONS_AVAILABLE] = TRUE;
                features[PF_SSE3_INSTRUCTIONS_AVAILABLE]   = TRUE;
                features[PF_RDTSCP_INSTRUCTION_AVAILABLE]  = TRUE;
                features[PF_SSSE3_INSTRUCTIONS_AVAILABLE]  = TRUE;
                features[PF_SSE4_1_INSTRUCTIONS_AVAILABLE] = TRUE;
                features[PF_SSE4_2_INSTRUCTIONS_AVAILABLE] = TRUE;
                break;
            }
        }
        break;
    }
    data->ActiveProcessorCount = NtCurrentTeb()->Peb->NumberOfProcessors;
    data->ActiveGroupCount = 1;

    initialize_xstate_features( data );

    UnmapViewOfFile( data );
}

#pragma pack(push,1)
struct smbios_prologue
{
    BYTE  calling_method;
    BYTE  major_version;
    BYTE  minor_version;
    BYTE  revision;
    DWORD length;
};

enum smbios_type
{
    SMBIOS_TYPE_BIOS = 0,
    SMBIOS_TYPE_SYSTEM = 1,
    SMBIOS_TYPE_BASEBOARD = 2,
    SMBIOS_TYPE_CHASSIS = 3,
    SMBIOS_TYPE_PROCESSOR = 4,
    SMBIOS_TYPE_BOOTINFO = 32,
    SMBIOS_TYPE_PROCESSOR_ADDITIONAL_INFO = 44,
    SMBIOS_TYPE_END = 127
};

struct smbios_header
{
    BYTE type;
    BYTE length;
    WORD handle;
};

struct smbios_baseboard
{
    struct smbios_header hdr;
    BYTE                 vendor;
    BYTE                 product;
    BYTE                 version;
    BYTE                 serial;
};

struct smbios_bios
{
    struct smbios_header hdr;
    BYTE                 vendor;
    BYTE                 version;
    WORD                 start;
    BYTE                 date;
    BYTE                 size;
    UINT64               characteristics;
    BYTE                 characteristics_ext[2];
    BYTE                 system_bios_major_release;
    BYTE                 system_bios_minor_release;
    BYTE                 ec_firmware_major_release;
    BYTE                 ec_firmware_minor_release;
};

struct smbios_system
{
    struct smbios_header hdr;
    BYTE                 vendor;
    BYTE                 product;
    BYTE                 version;
    BYTE                 serial;
    BYTE                 uuid[16];
    BYTE                 wake_up_type;
    BYTE                 sku;
    BYTE                 family;
};

struct smbios_processor
{
    struct smbios_header hdr;
    BYTE                 socket;
    BYTE                 type;
    BYTE                 family;
    BYTE                 vendor;
    ULONGLONG            id;
    BYTE                 version;
    BYTE                 voltage;
    WORD                 clock;
    WORD                 max_speed;
    WORD                 cur_speed;
    BYTE                 status;
    BYTE                 upgrade;
    WORD                 l1cache;
    WORD                 l2cache;
    WORD                 l3cache;
    BYTE                 serial;
    BYTE                 asset_tag;
    BYTE                 part_number;
    BYTE                 core_count;
    BYTE                 core_enabled;
    BYTE                 thread_count;
    WORD                 characteristics;
    WORD                 family2;
    WORD                 core_count2;
    WORD                 core_enabled2;
    WORD                 thread_count2;
};

struct smbios_processor_specific_block
{
    BYTE length;
    BYTE processor_type;
    BYTE data[];
};

struct smbios_processor_additional_info
{
    struct smbios_header hdr;
    WORD ref_handle;
    struct smbios_processor_specific_block info_block;
};

struct smbios_wine_core_id_regs_arm64
{
    WORD num_regs;
    struct smbios_wine_id_reg_value_arm64
    {
        WORD reg;
        UINT64 value;
    } regs[];
};

#pragma pack(pop)

#define RSMB (('R' << 24) | ('S' << 16) | ('M' << 8) | 'B')

static const struct smbios_header *find_smbios_entry( enum smbios_type type, unsigned int index,
                                                      const char *buf, UINT len )
{
    const char *ptr, *start;
    const struct smbios_prologue *prologue;
    const struct smbios_header *hdr;

    if (len < sizeof(struct smbios_prologue)) return NULL;
    prologue = (const struct smbios_prologue *)buf;
    if (prologue->length > len - sizeof(*prologue) || prologue->length < sizeof(*hdr)) return NULL;

    start = (const char *)(prologue + 1);
    hdr = (const struct smbios_header *)start;

    for (;;)
    {
        if ((const char *)hdr - start >= prologue->length - sizeof(*hdr)) return NULL;

        if (!hdr->length)
        {
            WARN( "invalid entry\n" );
            return NULL;
        }

        if (hdr->type == type)
        {
            if ((const char *)hdr - start + hdr->length > prologue->length) return NULL;
            if (!index--) return hdr;
        }
        /* skip other entries and their strings */
        for (ptr = (const char *)hdr + hdr->length; ptr - buf < len && *ptr; ptr++)
        {
            for (; ptr - buf < len; ptr++) if (!*ptr) break;
        }
        if (ptr == (const char *)hdr + hdr->length) ptr++;
        hdr = (const struct smbios_header *)(ptr + 1);
    }
}

static inline WCHAR *heap_strdupAW( const char *src )
{
    int len;
    WCHAR *dst;
    if (!src) return NULL;
    len = MultiByteToWideChar( CP_ACP, 0, src, -1, NULL, 0 );
    if ((dst = malloc( len * sizeof(*dst) ))) MultiByteToWideChar( CP_ACP, 0, src, -1, dst, len );
    return dst;
}

static WCHAR *get_smbios_string( BYTE id, const char *buf, UINT offset, UINT buflen )
{
    const char *ptr = buf + offset;
    UINT i = 0;

    if (!id || offset >= buflen) return NULL;
    for (ptr = buf + offset; ptr - buf < buflen && *ptr; ptr++)
    {
        if (++i == id) return heap_strdupAW( ptr );
        for (; ptr - buf < buflen; ptr++) if (!*ptr) break;
    }
    return NULL;
}

static void set_value_from_smbios_string( HKEY key, const WCHAR *value, BYTE id, const char *buf, UINT offset, UINT buflen )
{
    WCHAR *str;
    str = get_smbios_string( id, buf, offset, buflen );
    set_reg_value( key, value, str ? str : L"" );
    free( str );
}

static void create_bios_baseboard_values( HKEY bios_key, const char *buf, UINT len )
{
    const struct smbios_header *hdr;
    const struct smbios_baseboard *baseboard;
    UINT offset;

    if (!(hdr = find_smbios_entry( SMBIOS_TYPE_BASEBOARD, 0, buf, len ))) return;
    baseboard = (const struct smbios_baseboard *)hdr;
    offset = (const char *)baseboard - buf + baseboard->hdr.length;

    set_value_from_smbios_string( bios_key, L"BaseBoardManufacturer", baseboard->vendor, buf, offset, len );
    set_value_from_smbios_string( bios_key, L"BaseBoardProduct", baseboard->product, buf, offset, len );
    set_value_from_smbios_string( bios_key, L"BaseBoardVersion", baseboard->version, buf, offset, len );
}

static void create_bios_bios_values( HKEY bios_key, const char *buf, UINT len )
{
    const struct smbios_header *hdr;
    const struct smbios_bios *bios;
    UINT offset;

    if (!(hdr = find_smbios_entry( SMBIOS_TYPE_BIOS, 0, buf, len ))) return;
    bios = (const struct smbios_bios *)hdr;
    offset = (const char *)bios - buf + bios->hdr.length;

    set_value_from_smbios_string( bios_key, L"BIOSVendor", bios->vendor, buf, offset, len );
    set_value_from_smbios_string( bios_key, L"BIOSVersion", bios->version, buf, offset, len );
    set_value_from_smbios_string( bios_key, L"BIOSReleaseDate", bios->date, buf, offset, len );

    if (bios->hdr.length >= 0x18)
    {
        set_reg_value_dword( bios_key, L"BiosMajorRelease", bios->system_bios_major_release );
        set_reg_value_dword( bios_key, L"BiosMinorRelease", bios->system_bios_minor_release );
        set_reg_value_dword( bios_key, L"ECFirmwareMajorVersion", bios->ec_firmware_major_release );
        set_reg_value_dword( bios_key, L"ECFirmwareMinorVersion", bios->ec_firmware_minor_release );
    }
    else
    {
        set_reg_value_dword( bios_key, L"BiosMajorRelease", 0xFF );
        set_reg_value_dword( bios_key, L"BiosMinorRelease", 0xFF );
        set_reg_value_dword( bios_key, L"ECFirmwareMajorVersion", 0xFF );
        set_reg_value_dword( bios_key, L"ECFirmwareMinorVersion", 0xFF );
    }
}

static void create_bios_system_values( HKEY bios_key, const char *buf, UINT len )
{
    const struct smbios_header *hdr;
    const struct smbios_system *system;
    UINT offset;

    if (!(hdr = find_smbios_entry( SMBIOS_TYPE_SYSTEM, 0, buf, len ))) return;
    system = (const struct smbios_system *)hdr;
    offset = (const char *)system - buf + system->hdr.length;

    set_value_from_smbios_string( bios_key, L"SystemManufacturer", system->vendor, buf, offset, len );
    set_value_from_smbios_string( bios_key, L"SystemProductName", system->product, buf, offset, len );
    set_value_from_smbios_string( bios_key, L"SystemVersion", system->version, buf, offset, len );

    if (system->hdr.length >= 0x1B)
    {
        set_value_from_smbios_string( bios_key, L"SystemSKU", system->sku, buf, offset, len );
        set_value_from_smbios_string( bios_key, L"SystemFamily", system->family, buf, offset, len );
    }
    else
    {
        set_value_from_smbios_string( bios_key, L"SystemSKU", 0, buf, offset, len );
        set_value_from_smbios_string( bios_key, L"SystemFamily", 0, buf, offset, len );
    }
}

#ifdef __aarch64__

static void create_id_reg_keys_arm64( HKEY core_key, UINT core, const char *buf, UINT len )
{
    const struct smbios_header *hdr;
    const struct smbios_processor_additional_info *additional_info;
    const struct smbios_wine_core_id_regs_arm64 *core_id_regs;
    static const char *reg_value_name = "CP %04X";
    char buffer[9];
    UINT i;

    if (!(hdr = find_smbios_entry( SMBIOS_TYPE_PROCESSOR_ADDITIONAL_INFO, core, buf, len ))) return;

    additional_info = (const struct smbios_processor_additional_info *)hdr;
    core_id_regs = (const struct smbios_wine_core_id_regs_arm64 *)additional_info->info_block.data;

    for (i = 0; i < core_id_regs->num_regs; i++)
    {
        snprintf( buffer, sizeof(buffer), reg_value_name, core_id_regs->regs[i].reg );
        RegSetValueExA( core_key, buffer, 0, REG_QWORD,
                        (const BYTE *)&core_id_regs->regs[i].value, sizeof(UINT64) );
    }
}

#endif

static void create_bios_processor_values( HKEY system_key, const char *buf, UINT len )
{
    const struct smbios_header *hdr;
    const struct smbios_processor *proc;
    unsigned int pkg, core, offset, i, thread_count;
    HKEY hkey, cpu_key, fpu_key = 0, env_key;
    SYSTEM_CPU_INFORMATION sci;
    PROCESSOR_POWER_INFORMATION* power_info;
    ULONG sizeof_power_info = sizeof(PROCESSOR_POWER_INFORMATION) * NtCurrentTeb()->Peb->NumberOfProcessors;
    UINT64 tsc_frequency = read_tsc_frequency();
    const WCHAR *arch;
    WCHAR id[60], buffer[128], *version, *vendorid;

    NtQuerySystemInformation( SystemCpuInformation, &sci, sizeof(sci), NULL );

    power_info = malloc( sizeof_power_info );
    if (power_info == NULL)
        return;
    if (NtPowerInformation( ProcessorInformation, NULL, 0, power_info, sizeof_power_info ))
        memset( power_info, 0, sizeof_power_info );

    switch (sci.ProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_ARM:
    case PROCESSOR_ARCHITECTURE_ARM64:
        set_reg_value( system_key, L"Identifier", L"ARM processor family" );
        break;

    case PROCESSOR_ARCHITECTURE_INTEL:
    case PROCESSOR_ARCHITECTURE_AMD64:
    default:
        set_reg_value( system_key, L"Identifier", L"AT compatible" );
        if (RegCreateKeyExW( system_key, L"FloatingPointProcessor", 0, NULL, REG_OPTION_VOLATILE,
                             KEY_ALL_ACCESS, NULL, &fpu_key, NULL ))
            fpu_key = 0;
        break;
    }
    set_reg_value( system_key, L"SystemBiosDate", L"01/01/70" );

    if (RegCreateKeyExW( system_key, L"CentralProcessor", 0, NULL, REG_OPTION_VOLATILE,
                         KEY_ALL_ACCESS, NULL, &cpu_key, NULL ))
        cpu_key = 0;

    for (pkg = core = 0; ; pkg++)
    {
        if (!(hdr = find_smbios_entry( SMBIOS_TYPE_PROCESSOR, pkg, buf, len ))) break;
        if (hdr->length < 0x28) break; /* version 2.5 is required */
        proc = (const struct smbios_processor *)hdr;
        offset = (const char *)proc - buf + proc->hdr.length;
        version = get_smbios_string( proc->version, buf, offset, len );
        vendorid = get_smbios_string( proc->vendor, buf, offset, len );

        switch (sci.ProcessorArchitecture)
        {
        case PROCESSOR_ARCHITECTURE_ARM:
            arch = L"ARM";
            swprintf( id, ARRAY_SIZE(id), L"ARM Family 7 Model %X Revision %X",
                      sci.ProcessorLevel, sci.ProcessorRevision );
            break;
        case PROCESSOR_ARCHITECTURE_ARM64:
            arch = L"ARM64";
            swprintf( id, ARRAY_SIZE(id), L"ARMv8 (64-bit) Family 8 Model %X Revision %X",
                      sci.ProcessorLevel, sci.ProcessorRevision );
            break;
        case PROCESSOR_ARCHITECTURE_AMD64:
            arch = L"AMD64";
            swprintf( id, ARRAY_SIZE(id), L"%s Family %u Model %u Stepping %u",
                      vendorid && !wcscmp(vendorid, L"AuthenticAMD") ? L"AMD64" : L"Intel64",
                      sci.ProcessorLevel, HIBYTE(sci.ProcessorRevision), LOBYTE(sci.ProcessorRevision) );
            break;
        case PROCESSOR_ARCHITECTURE_INTEL:
        default:
            arch = L"x86";
            swprintf( id, ARRAY_SIZE(id), L"x86 Family %u Model %u Stepping %u",
                      sci.ProcessorLevel, HIBYTE(sci.ProcessorRevision), LOBYTE(sci.ProcessorRevision) );
            break;
        }

        thread_count = (proc->hdr.length >= 0x30) ? proc->thread_count2 : proc->thread_count;
        for (i = 0; i < thread_count; i++, core++)
        {
            swprintf( buffer, ARRAY_SIZE(buffer), L"%u", core );
            if (!RegCreateKeyExW( cpu_key, buffer, 0, NULL, REG_OPTION_VOLATILE,
                                  KEY_ALL_ACCESS, NULL, &hkey, NULL ))
            {
                DWORD tsc_freq_mhz = (DWORD)(tsc_frequency / 1000000ull); /* Hz -> Mhz */
                if (!tsc_freq_mhz) tsc_freq_mhz = power_info[core].MaxMhz;

                RegSetValueExW( hkey, L"FeatureSet", 0, REG_DWORD,
                                (BYTE *)&sci.ProcessorFeatureBits, sizeof(DWORD) );
                set_reg_value( hkey, L"Identifier", id );
                if (vendorid) set_reg_value( hkey, L"VendorIdentifier", vendorid );
                if (version) set_reg_value( hkey, L"ProcessorNameString", version );
                RegSetValueExW( hkey, L"~MHz", 0, REG_DWORD, (BYTE *)&tsc_freq_mhz, sizeof(DWORD) );
#ifdef __aarch64__
                create_id_reg_keys_arm64( hkey, core, buf, len );
#endif
                RegCloseKey( hkey );
            }
            if (fpu_key && !RegCreateKeyExW( fpu_key, buffer, 0, NULL, REG_OPTION_VOLATILE,
                                             KEY_ALL_ACCESS, NULL, &hkey, NULL ))
            {
                set_reg_value( hkey, L"Identifier", id );
                RegCloseKey( hkey );
            }
        }

        if (!pkg && !RegCreateKeyW( HKEY_LOCAL_MACHINE,
                                    L"System\\CurrentControlSet\\Control\\Session Manager\\Environment",
                                    &env_key ))
        {
            set_reg_value( env_key, L"PROCESSOR_ARCHITECTURE", arch );

            swprintf( buffer, ARRAY_SIZE(buffer), L"%s, %s", id, vendorid ? vendorid : L"Unknown" );
            set_reg_value( env_key, L"PROCESSOR_IDENTIFIER", buffer );

            swprintf( buffer, ARRAY_SIZE(buffer), L"%u", sci.ProcessorLevel );
            set_reg_value( env_key, L"PROCESSOR_LEVEL", buffer );

            swprintf( buffer, ARRAY_SIZE(buffer), L"%04x", sci.ProcessorRevision );
            set_reg_value( env_key, L"PROCESSOR_REVISION", buffer );

            swprintf( buffer, ARRAY_SIZE(buffer), L"%u", NtCurrentTeb()->Peb->NumberOfProcessors );
            set_reg_value( env_key, L"NUMBER_OF_PROCESSORS", buffer );
            RegCloseKey( env_key );
        }

        free( version );
        free( vendorid );
    }

    RegCloseKey( fpu_key );
    RegCloseKey( cpu_key );
    free( power_info );
}

/* create the volatile hardware registry keys */
static void create_hardware_registry_keys(void)
{
    HKEY system_key, bios_key;
    UINT len;
    char *buf;

    len = GetSystemFirmwareTable( RSMB, 0, NULL, 0 );
    if (!(buf = malloc( len ))) return;
    len = GetSystemFirmwareTable( RSMB, 0, buf, len );

    if (RegCreateKeyExW( HKEY_LOCAL_MACHINE, L"Hardware\\Description\\System", 0, NULL,
                         REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &system_key, NULL ))
    {
        free( buf );
        return;
    }
    if (!RegCreateKeyExW( system_key, L"BIOS", 0, NULL, REG_OPTION_VOLATILE,
                          KEY_ALL_ACCESS, NULL, &bios_key, NULL ))
    {
        create_bios_baseboard_values( bios_key, buf, len );
        create_bios_bios_values( bios_key, buf, len );
        create_bios_system_values( bios_key, buf, len );
        RegCloseKey( bios_key );
    }
    create_bios_processor_values( system_key, buf, len );
    RegCloseKey( system_key );
    free( buf );
}


/* create the DynData registry keys */
static void create_dynamic_registry_keys(void)
{
    HKEY key;

    if (!RegCreateKeyExW( HKEY_DYN_DATA, L"PerfStats\\StatData", 0, NULL, 0, KEY_WRITE, NULL, &key, NULL ))
        RegCloseKey( key );
    if (!RegCreateKeyExW( HKEY_DYN_DATA, L"Config Manager\\Enum", 0, NULL, 0, KEY_WRITE, NULL, &key, NULL ))
        RegCloseKey( key );
}

/* create the ComputerName registry keys */
static void create_computer_name_keys(void)
{
    struct addrinfo hints = {0}, *res;
    char *dot, buffer[256], *name = buffer;
    HKEY key, subkey;

    if (gethostname( buffer, sizeof(buffer) )) return;
    hints.ai_flags = AI_CANONNAME;
    if (!getaddrinfo( buffer, NULL, &hints, &res ) &&
        res->ai_canonname && strcasecmp(res->ai_canonname, "localhost") != 0)
        name = res->ai_canonname;
    dot = strchr( name, '.' );
    if (dot) *dot++ = 0;
    else dot = name + strlen(name);
    SetComputerNameExA( ComputerNamePhysicalDnsDomain, dot );
    SetComputerNameExA( ComputerNamePhysicalDnsHostname, name );
    if (name != buffer) freeaddrinfo( res );

    if (RegOpenKeyW( HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\ComputerName", &key ))
        return;

    if (!RegOpenKeyW( key, L"ComputerName", &subkey ))
    {
        DWORD type, size = sizeof(buffer);

        if (RegQueryValueExW( subkey, L"ComputerName", NULL, &type, (BYTE *)buffer, &size )) size = 0;
        RegCloseKey( subkey );
        if (size && !RegCreateKeyExW( key, L"ActiveComputerName", 0, NULL, REG_OPTION_VOLATILE,
                                      KEY_ALL_ACCESS, NULL, &subkey, NULL ))
        {
            RegSetValueExW( subkey, L"ComputerName", 0, type, (const BYTE *)buffer, size );
            RegCloseKey( subkey );
        }
    }
    RegCloseKey( key );
}

static void create_volatile_environment_registry_key(void)
{
    WCHAR path[MAX_PATH];
    WCHAR computername[MAX_COMPUTERNAME_LENGTH + 1 + 2];
    DWORD size;
    HKEY hkey;
    HRESULT hr;

    if (RegCreateKeyExW( HKEY_CURRENT_USER, L"Volatile Environment", 0, NULL, REG_OPTION_VOLATILE,
                         KEY_ALL_ACCESS, NULL, &hkey, NULL ))
        return;

    hr = SHGetFolderPathW( NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, path );
    if (SUCCEEDED(hr)) set_reg_value( hkey, L"APPDATA", path );

    set_reg_value( hkey, L"CLIENTNAME", L"Console" );

    /* Write the profile path's drive letter and directory components into
     * HOMEDRIVE and HOMEPATH respectively. */
    hr = SHGetFolderPathW( NULL, CSIDL_PROFILE | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, path );
    if (SUCCEEDED(hr))
    {
        set_reg_value( hkey, L"USERPROFILE", path );
        set_reg_value( hkey, L"HOMEPATH", path + 2 );
        path[2] = '\0';
        set_reg_value( hkey, L"HOMEDRIVE", path );
    }

    size = ARRAY_SIZE(path);
    if (GetUserNameW( path, &size )) set_reg_value( hkey, L"USERNAME", path );

    set_reg_value( hkey, L"HOMESHARE", L"" );

    hr = SHGetFolderPathW( NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, path );
    if (SUCCEEDED(hr))
        set_reg_value( hkey, L"LOCALAPPDATA", path );

    size = ARRAY_SIZE(computername) - 2;
    if (GetComputerNameW(&computername[2], &size))
    {
        set_reg_value( hkey, L"USERDOMAIN", &computername[2] );
        computername[0] = computername[1] = '\\';
        set_reg_value( hkey, L"LOGONSERVER", computername );
    }

    set_reg_value( hkey, L"SESSIONNAME", L"Console" );
    RegCloseKey( hkey );
}

static const WCHAR *get_known_dll_ntdir( WORD machine )
{
    switch (machine)
    {
    case IMAGE_FILE_MACHINE_TARGET_HOST: return L"\\KnownDlls";
    case IMAGE_FILE_MACHINE_I386:        return L"\\KnownDlls32";
    case IMAGE_FILE_MACHINE_ARMNT:       return L"\\KnownDllsArm32";
    default: return NULL;
    }
}

static HANDLE create_known_dll_section( const WCHAR *sysdir, HANDLE root, const WCHAR *name )
{
    WCHAR buffer[MAX_PATH];
    HANDLE handle, mapping;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING strW;
    IO_STATUS_BLOCK io;
    LARGE_INTEGER size;

    swprintf( buffer, MAX_PATH, L"\\??\\%s\\%s", sysdir, name );
    RtlInitUnicodeString( &strW, buffer );
    InitializeObjectAttributes( &attr, &strW, OBJ_CASE_INSENSITIVE, 0, NULL );

    if (NtOpenFile( &handle, GENERIC_READ | SYNCHRONIZE, &attr, &io,
                    FILE_SHARE_READ | FILE_SHARE_DELETE,
                    FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE ))
        return 0;

    RtlInitUnicodeString( &strW, name );
    InitializeObjectAttributes( &attr, &strW, OBJ_CASE_INSENSITIVE | OBJ_PERMANENT, root, NULL );
    size.QuadPart = 0;
    if (NtCreateSection( &mapping,
                         STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_EXECUTE,
                         &attr, &size, PAGE_EXECUTE_READ, SEC_IMAGE, handle ))
        mapping = 0;
    NtClose( handle );
    return mapping;
}

struct known_dll
{
    struct list entry;
    WCHAR       name[64];
};

static void add_known_dll( struct list *dll_list, const WCHAR *name )
{
    struct known_dll *dll;

    LIST_FOR_EACH_ENTRY( dll, dll_list, struct known_dll, entry )
        if (!wcsicmp( name, dll->name )) return;

    dll = malloc( sizeof(*dll) );
    wcscpy( dll->name, name );
    list_add_tail( dll_list, &dll->entry );
}

static void add_imports( struct list *dll_list, HANDLE mapping )
{
    const IMAGE_IMPORT_DESCRIPTOR *imp;
    DWORD i, size;
    void *module;

    if (!(module = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 ))) return;
    if ((imp = RtlImageDirectoryEntryToData( module, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &size )))
    {
        for ( ; imp->Name; imp++)
        {
            unsigned char *str = (unsigned char *)module + imp->Name;
            WCHAR name[64];
            for (i = 0; str[i] && i < ARRAY_SIZE(name); i++) name[i] = (unsigned char)str[i];
            if (i == ARRAY_SIZE(name)) continue;
            name[i] = 0;
            add_known_dll( dll_list, name );
        }
    }
    UnmapViewOfFile( module );
}


static void create_known_dlls(void)
{
    struct list dlls = LIST_INIT( dlls );
    struct known_dll *dll, *next;
    WCHAR name[64], value[64], sysdir[MAX_PATH];
    UNICODE_STRING str, target;
    OBJECT_ATTRIBUTES attr;
    HANDLE handle, root, mapping;
    DWORD type, idx = 0;
    HKEY key;

    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                       L"System\\CurrentControlSet\\Control\\Session Manager\\KnownDLLs",
                       0, KEY_ALL_ACCESS, &key ))
        return;

    for (;;)
    {
        DWORD name_size = ARRAY_SIZE(name);
        DWORD val_size = sizeof(value);
        DWORD res = RegEnumValueW( key, idx++, name, &name_size, 0, &type, (BYTE *)value, &val_size );
        if (res == ERROR_MORE_DATA) continue;  /* ignore it */
        if (res) break;
        add_known_dll( &dlls, value );
    }

    for (int i = 0; machines[i].Machine; i++)
    {
        WORD machine = machines[i].Native ? IMAGE_FILE_MACHINE_TARGET_HOST : machines[i].Machine;

        if (!GetSystemWow64Directory2W( sysdir, MAX_PATH, machine )) continue;
        RtlInitUnicodeString( &str, get_known_dll_ntdir( machine ));
        InitializeObjectAttributes( &attr, &str, OBJ_PERMANENT, 0, NULL );
        if (NtCreateDirectoryObject( &root, DIRECTORY_ALL_ACCESS, &attr )) continue;

        RtlInitUnicodeString( &target, sysdir );
        RtlInitUnicodeString( &str, L"KnownDllPath" );
        InitializeObjectAttributes( &attr, &str, OBJ_PERMANENT, root, NULL );
        if (!NtCreateSymbolicLinkObject( &handle, SYMBOLIC_LINK_ALL_ACCESS, &attr, &target ))
            NtClose( handle );

        LIST_FOR_EACH_ENTRY( dll, &dlls, struct known_dll, entry )
        {
            if (!(mapping = create_known_dll_section( sysdir, root, dll->name ))) continue;
            if (machines[i].Native) add_imports( &dlls, mapping );
            NtClose( mapping );
        }
        NtClose( root );
    }

    LIST_FOR_EACH_ENTRY_SAFE( dll, next, &dlls, struct known_dll, entry )
    {
        list_remove( &dll->entry );
        free( dll );
    }
    RegCloseKey( key );
}


/* Performs the rename operations dictated in %SystemRoot%\Wininit.ini.
 * Returns FALSE if there was an error, or otherwise if all is ok.
 */
static BOOL wininit(void)
{
    WCHAR initial_buffer[1024];
    WCHAR *str, *buffer = initial_buffer;
    DWORD size = ARRAY_SIZE(initial_buffer);
    DWORD res;

    for (;;)
    {
        if (!(res = GetPrivateProfileSectionW( L"rename", buffer, size, L"wininit.ini" ))) return TRUE;
        if (res < size - 2) break;
        if (buffer != initial_buffer) free( buffer );
        size *= 2;
        if (!(buffer = malloc( size * sizeof(WCHAR) ))) return FALSE;
    }

    for (str = buffer; *str; str += lstrlenW(str) + 1)
    {
        WCHAR *value;

        if (*str == ';') continue;  /* comment */
        if (!(value = wcschr( str, '=' ))) continue;

        /* split the line into key and value */
        *value++ = 0;

        if (!lstrcmpiW( L"NUL", str ))
        {
            WINE_TRACE("Deleting file %s\n", wine_dbgstr_w(value) );
            if( !DeleteFileW( value ) )
                WINE_WARN("Error deleting file %s\n", wine_dbgstr_w(value) );
        }
        else
        {
            WINE_TRACE("Renaming file %s to %s\n", wine_dbgstr_w(value), wine_dbgstr_w(str) );

            if( !MoveFileExW(value, str, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) )
                WINE_WARN("Error renaming %s to %s\n", wine_dbgstr_w(value), wine_dbgstr_w(str) );
        }
        str = value;
    }

    if (buffer != initial_buffer) free( buffer );

    if( !MoveFileExW( L"wininit.ini", L"wininit.bak", MOVEFILE_REPLACE_EXISTING) )
    {
        WINE_ERR("Couldn't rename wininit.ini, error %ld\n", GetLastError() );

        return FALSE;
    }

    return TRUE;
}

static void pendingRename(void)
{
    WCHAR *buffer=NULL;
    const WCHAR *src=NULL, *dst=NULL;
    DWORD dataLength=0;
    HKEY hSession;

    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\Session Manager",
                       0, KEY_ALL_ACCESS, &hSession ))
        return;

    if (RegQueryValueExW( hSession, L"PendingFileRenameOperations", NULL, NULL, NULL, &dataLength ))
        goto end;
    if (!(buffer = malloc( dataLength ))) goto end;

    if (RegQueryValueExW( hSession, L"PendingFileRenameOperations", NULL, NULL,
                          (LPBYTE)buffer, &dataLength ))
        goto end;

    /* Make sure that the data is long enough and ends with two NULLs. This
     * simplifies the code later on.
     */
    if( dataLength<2*sizeof(buffer[0]) ||
            buffer[dataLength/sizeof(buffer[0])-1]!='\0' ||
            buffer[dataLength/sizeof(buffer[0])-2]!='\0' )
        goto end;

    for( src=buffer; (src-buffer)*sizeof(src[0])<dataLength && *src!='\0';
            src=dst+lstrlenW(dst)+1 )
    {
        DWORD dwFlags=0;

        dst=src+lstrlenW(src)+1;

        /* We need to skip the \??\ header */
        if( src[0]=='\\' && src[1]=='?' && src[2]=='?' && src[3]=='\\' )
            src+=4;

        if( dst[0]=='!' )
        {
            dwFlags|=MOVEFILE_REPLACE_EXISTING;
            dst++;
        }

        if( dst[0]=='\\' && dst[1]=='?' && dst[2]=='?' && dst[3]=='\\' )
            dst+=4;

        if( *dst!='\0' )
        {
            /* Rename the file */
            MoveFileExW( src, dst, dwFlags );
        } else
        {
            /* Delete the file or directory */
            if (!RemoveDirectoryW( src ) && GetLastError() == ERROR_DIRECTORY) DeleteFileW( src );
        }
    }

    RegDeleteValueW(hSession, L"PendingFileRenameOperations");

end:
    free( buffer );
    RegCloseKey( hSession );
}

#define INVALID_RUNCMD_RETURN -1
/*
 * This function runs the specified command in the specified dir.
 * [in,out] cmdline - the command line to run. The function may change the passed buffer.
 * [in] dir - the dir to run the command in. If it is NULL, then the current dir is used.
 * [in] wait - whether to wait for the run program to finish before returning.
 * [in] minimized - Whether to ask the program to run minimized.
 *
 * Returns:
 * If running the process failed, returns INVALID_RUNCMD_RETURN. Use GetLastError to get the error code.
 * If wait is FALSE - returns 0 if successful.
 * If wait is TRUE - returns the program's return value.
 */
static DWORD runCmd(LPWSTR cmdline, LPCWSTR dir, BOOL wait, BOOL minimized)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    DWORD exit_code=0;

    memset(&si, 0, sizeof(si));
    si.cb=sizeof(si);
    if( minimized )
    {
        si.dwFlags=STARTF_USESHOWWINDOW;
        si.wShowWindow=SW_MINIMIZE;
    }
    memset(&info, 0, sizeof(info));

    if( !CreateProcessW(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, dir, &si, &info) )
    {
        WINE_WARN("Failed to run command %s (%ld)\n", wine_dbgstr_w(cmdline), GetLastError() );
        return INVALID_RUNCMD_RETURN;
    }

    WINE_TRACE("Successfully ran command %s - Created process handle %p\n",
               wine_dbgstr_w(cmdline), info.hProcess );

    if(wait)
    {   /* wait for the process to exit */
        WaitForSingleObject(info.hProcess, INFINITE);
        GetExitCodeProcess(info.hProcess, &exit_code);
    }

    CloseHandle( info.hThread );
    CloseHandle( info.hProcess );

    return exit_code;
}

static void process_run_key( HKEY key, const WCHAR *keyname, BOOL delete, BOOL synchronous )
{
    HKEY runkey;
    LONG res;
    DWORD disp, i, max_cmdline = 0, max_value = 0;
    WCHAR *cmdline = NULL, *value = NULL;

    if (RegCreateKeyExW( key, keyname, 0, NULL, 0, delete ? KEY_ALL_ACCESS : KEY_READ, NULL, &runkey, &disp ))
        return;

    if (disp == REG_CREATED_NEW_KEY)
        goto end;

    if (RegQueryInfoKeyW( runkey, NULL, NULL, NULL, NULL, NULL, NULL, &i, &max_value, &max_cmdline, NULL, NULL ))
        goto end;

    if (!i)
    {
        WINE_TRACE( "No commands to execute.\n" );
        goto end;
    }
    if (!(cmdline = malloc( max_cmdline )))
    {
        WINE_ERR( "Couldn't allocate memory for the commands to be executed.\n" );
        goto end;
    }
    if (!(value = malloc( ++max_value * sizeof(*value) )))
    {
        WINE_ERR( "Couldn't allocate memory for the value names.\n" );
        goto end;
    }

    while (i)
    {
        DWORD len = max_value, len_data = max_cmdline, type;

        if ((res = RegEnumValueW( runkey, --i, value, &len, 0, &type, (BYTE *)cmdline, &len_data )))
        {
            WINE_ERR( "Couldn't read value %lu (%ld).\n", i, res );
            continue;
        }
        if (delete && (res = RegDeleteValueW( runkey, value )))
        {
            WINE_ERR( "Couldn't delete value %lu (%ld). Running command anyways.\n", i, res );
        }
        if (type != REG_SZ)
        {
            WINE_ERR( "Incorrect type of value %lu (%lu).\n", i, type );
            continue;
        }
        if (runCmd( cmdline, NULL, synchronous, FALSE ) == INVALID_RUNCMD_RETURN)
        {
            WINE_ERR( "Error running cmd %s (%lu).\n", wine_dbgstr_w(cmdline), GetLastError() );
        }
        WINE_TRACE( "Done processing cmd %lu.\n", i );
    }

end:
    free( value );
    free( cmdline );
    RegCloseKey( runkey );
    WINE_TRACE( "Done.\n" );
}

/*
 * Process a "Run" type registry key.
 * hkRoot is the HKEY from which "Software\Microsoft\Windows\CurrentVersion" is
 *      opened.
 * szKeyName is the key holding the actual entries.
 * bDelete tells whether we should delete each value right before executing it.
 * bSynchronous tells whether we should wait for the prog to complete before
 *      going on to the next prog.
 */
static void ProcessRunKeys( HKEY root, const WCHAR *keyname, BOOL delete, BOOL synchronous )
{
    HKEY key;

    if (root == HKEY_LOCAL_MACHINE)
    {
        WINE_TRACE( "Processing %s entries under HKLM.\n", wine_dbgstr_w(keyname) );
        if (!RegCreateKeyExW( root, L"Software\\Microsoft\\Windows\\CurrentVersion",
                              0, NULL, 0, KEY_READ, NULL, &key, NULL ))
        {
            process_run_key( key, keyname, delete, synchronous );
            RegCloseKey( key );
        }
        if (is_64bit && !RegCreateKeyExW( root, L"Software\\Microsoft\\Windows\\CurrentVersion",
                                          0, NULL, 0, KEY_READ|KEY_WOW64_32KEY, NULL, &key, NULL ))
        {
            process_run_key( key, keyname, delete, synchronous );
            RegCloseKey( key );
        }
    }
    else
    {
        WINE_TRACE( "Processing %s entries under HKCU.\n", wine_dbgstr_w(keyname) );
        if (!RegCreateKeyExW( root, L"Software\\Microsoft\\Windows\\CurrentVersion",
                              0, NULL, 0, KEY_READ, NULL, &key, NULL ))
        {
            process_run_key( key, keyname, delete, synchronous );
            RegCloseKey( key );
        }
    }
}

/*
 * WFP is Windows File Protection, in NT5 and Windows 2000 it maintains a cache
 * of known good dlls and scans through and replaces corrupted DLLs with these
 * known good versions. The only programs that should install into this dll
 * cache are Windows Updates and IE (which is treated like a Windows Update)
 *
 * Implementing this allows installing ie in win2k mode to actually install the
 * system dlls that we expect and need
 */
static int ProcessWindowsFileProtection(void)
{
    WIN32_FIND_DATAW finddata;
    HANDLE find_handle;
    BOOL find_rc;
    DWORD rc;
    HKEY hkey;
    LPWSTR dllcache = NULL;

    if (!RegOpenKeyW( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", &hkey ))
    {
        DWORD sz = 0;
        if (!RegQueryValueExW( hkey, L"SFCDllCacheDir", 0, NULL, NULL, &sz))
        {
            sz += sizeof(WCHAR);
            dllcache = malloc( sz + sizeof(L"\\*") );
            RegQueryValueExW( hkey, L"SFCDllCacheDir", 0, NULL, (LPBYTE)dllcache, &sz);
            lstrcatW( dllcache, L"\\*" );
        }
    }
    RegCloseKey(hkey);

    if (!dllcache)
    {
        DWORD sz = GetSystemDirectoryW( NULL, 0 );
        dllcache = malloc( sz * sizeof(WCHAR) + sizeof(L"\\dllcache\\*") );
        GetSystemDirectoryW( dllcache, sz );
        lstrcatW( dllcache, L"\\dllcache\\*" );
    }

    find_handle = FindFirstFileW(dllcache,&finddata);
    dllcache[ lstrlenW(dllcache) - 2] = 0; /* strip off wildcard */
    find_rc = find_handle != INVALID_HANDLE_VALUE;
    while (find_rc)
    {
        WCHAR targetpath[MAX_PATH];
        WCHAR currentpath[MAX_PATH];
        UINT sz;
        UINT sz2;
        WCHAR tempfile[MAX_PATH];

        if (wcscmp(finddata.cFileName,L".") == 0 || wcscmp(finddata.cFileName,L"..") == 0)
        {
            find_rc = FindNextFileW(find_handle,&finddata);
            continue;
        }

        sz = MAX_PATH;
        sz2 = MAX_PATH;
        VerFindFileW(VFFF_ISSHAREDFILE, finddata.cFileName, windowsdir,
                     windowsdir, currentpath, &sz, targetpath, &sz2);
        sz = MAX_PATH;
        rc = VerInstallFileW(0, finddata.cFileName, finddata.cFileName,
                             dllcache, targetpath, currentpath, tempfile, &sz);
        if (rc != ERROR_SUCCESS)
        {
            WINE_WARN("WFP: %s error 0x%lx\n",wine_dbgstr_w(finddata.cFileName),rc);
            DeleteFileW(tempfile);
        }

        /* now delete the source file so that we don't try to install it over and over again */
        lstrcpynW( targetpath, dllcache, MAX_PATH - 1 );
        sz = lstrlenW( targetpath );
        targetpath[sz++] = '\\';
        lstrcpynW( targetpath + sz, finddata.cFileName, MAX_PATH - sz );
        if (!DeleteFileW( targetpath ))
            WINE_WARN( "failed to delete %s: error %lu\n", wine_dbgstr_w(targetpath), GetLastError() );

        find_rc = FindNextFileW(find_handle,&finddata);
    }
    FindClose(find_handle);
    free(dllcache);
    return 1;
}

static BOOL start_services_process(void)
{
    static const WCHAR svcctl_started_event[] = SVCCTL_STARTED_EVENT;
    PROCESS_INFORMATION pi;
    STARTUPINFOW si = { sizeof(si) };
    HANDLE wait_handles[2];

    if (!CreateProcessW(L"C:\\windows\\system32\\services.exe", NULL,
                        NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &si, &pi))
    {
        WINE_ERR("Couldn't start services.exe: error %lu\n", GetLastError());
        return FALSE;
    }
    CloseHandle(pi.hThread);

    wait_handles[0] = CreateEventW(NULL, TRUE, FALSE, svcctl_started_event);
    wait_handles[1] = pi.hProcess;

    /* wait for the event to become available or the process to exit */
    if ((WaitForMultipleObjects(2, wait_handles, FALSE, INFINITE)) == WAIT_OBJECT_0 + 1)
    {
        DWORD exit_code;
        GetExitCodeProcess(pi.hProcess, &exit_code);
        WINE_ERR("Unexpected termination of services.exe - exit code %ld\n", exit_code);
        CloseHandle(pi.hProcess);
        CloseHandle(wait_handles[0]);
        return FALSE;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(wait_handles[0]);
    return TRUE;
}

static void set_wait_dialog_text( HWND hwnd, HWND text, const WCHAR *string )
{
    RECT win_rect, old_rect, new_rect;
    HDC hdc = GetDC( text );

    GetClientRect( text, &old_rect );
    new_rect = old_rect;
    SelectObject( hdc, (HFONT)SendMessageW( text, WM_GETFONT, 0, 0 ));
    DrawTextW( hdc, string, -1, &new_rect, DT_CALCRECT | DT_EDITCONTROL | DT_WORDBREAK | DT_NOPREFIX );
    ReleaseDC( text, hdc );
    if (new_rect.bottom > old_rect.bottom)
    {
        GetWindowRect( hwnd, &win_rect );
        win_rect.bottom += new_rect.bottom - old_rect.bottom;
        SetWindowPos( hwnd, 0, 0, 0, win_rect.right - win_rect.left, win_rect.bottom - win_rect.top,
                      SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER );
        SetWindowPos( text, 0, 0, 0, new_rect.right, new_rect.bottom,
                      SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER );
    }
    SendMessageW( text, WM_SETTEXT, 0, (LPARAM)string );
}

static INT_PTR CALLBACK wait_dlgproc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    switch (msg)
    {
    case WM_INITDIALOG:
        {
            DWORD len, icon_size;
            RECT rect;
            WCHAR *buffer, text[1024];
            const WCHAR *name = (WCHAR *)lp;
            HICON icon;

            GetClientRect( GetDlgItem( hwnd, IDC_WAITICON ), &rect );
            icon_size = min( rect.right, rect.bottom );
            icon = LoadImageW( 0, (LPCWSTR)IDI_WINLOGO, IMAGE_ICON, icon_size, icon_size, LR_SHARED );
            SendDlgItemMessageW( hwnd, IDC_WAITICON, STM_SETICON, (WPARAM)icon, 0 );
            SendDlgItemMessageW( hwnd, IDC_WAITTEXT, WM_GETTEXT, 1024, (LPARAM)text );
            len = lstrlenW(text) + lstrlenW(name) + 1;
            buffer = malloc( len * sizeof(WCHAR) );
            swprintf( buffer, len, text, name );
            set_wait_dialog_text( hwnd, GetDlgItem( hwnd, IDC_WAITTEXT ), buffer );
            free( buffer );
        }
        break;
    }
    return 0;
}

static HWND show_wait_window(void)
{
    HWND hwnd = CreateDialogParamW( GetModuleHandleW(0), MAKEINTRESOURCEW(IDD_WAITDLG), 0,
                                    wait_dlgproc, (LPARAM)prettyprint_configdir() );
    ShowWindow( hwnd, SW_SHOWNORMAL );
    return hwnd;
}

static HANDLE start_rundll32( const WCHAR *inf_path, const WCHAR *install, WORD machine )
{
    WCHAR app[MAX_PATH + ARRAY_SIZE(L"\\rundll32.exe" )];
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    WCHAR *buffer;
    DWORD len;

    memset( &si, 0, sizeof(si) );
    si.cb = sizeof(si);

    if (!GetSystemWow64Directory2W( app, MAX_PATH, machine )) return 0;
    lstrcatW( app, L"\\rundll32.exe" );
    TRACE( "machine %x starting %s\n", machine, debugstr_w(app) );

    len = lstrlenW(app) + ARRAY_SIZE(L" setupapi,InstallHinfSection DefaultInstall 128 ") + lstrlenW(inf_path);

    if (!(buffer = malloc( len * sizeof(WCHAR) ))) return 0;
    swprintf( buffer, len, L"%s setupapi,InstallHinfSection %s 128 %s", app, install, inf_path );

    if (CreateProcessW( app, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ))
        CloseHandle( pi.hThread );
    else
        pi.hProcess = 0;

    free( buffer );
    return pi.hProcess;
}

static void install_root_pnp_devices(void)
{
    static const struct
    {
        const char *name;
        const char *hardware_id;
        const char *infpath;
    }
    root_devices[] =
    {
        {"root\\wine\\winebth", "root\\winebth\0", "C:\\windows\\inf\\winebth.inf"},
        {"root\\wine\\winebus", "root\\winebus\0", "C:\\windows\\inf\\winebus.inf"},
        {"root\\wine\\wineusb", "root\\wineusb\0", "C:\\windows\\inf\\wineusb.inf"},
    };
    SP_DEVINFO_DATA device = {sizeof(device)};
    unsigned int i;
    HDEVINFO set;

    if ((set = SetupDiCreateDeviceInfoList( NULL, NULL )) == INVALID_HANDLE_VALUE)
    {
        WINE_ERR("Failed to create device info list, error %#lx.\n", GetLastError());
        return;
    }

    for (i = 0; i < ARRAY_SIZE(root_devices); ++i)
    {
        if (!SetupDiCreateDeviceInfoA( set, root_devices[i].name, &GUID_NULL, NULL, NULL, 0, &device))
        {
            if (GetLastError() != ERROR_DEVINST_ALREADY_EXISTS)
                WINE_ERR("Failed to create device %s, error %#lx.\n", debugstr_a(root_devices[i].name), GetLastError());
            continue;
        }

        if (!SetupDiSetDeviceRegistryPropertyA(set, &device, SPDRP_HARDWAREID,
                (const BYTE *)root_devices[i].hardware_id, strlen(root_devices[i].hardware_id) + 2))
        {
            WINE_ERR("Failed to set hardware id for %s, error %#lx.\n", debugstr_a(root_devices[i].name), GetLastError());
            continue;
        }

        if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE, set, &device))
        {
            WINE_ERR("Failed to register device %s, error %#lx.\n", debugstr_a(root_devices[i].name), GetLastError());
            continue;
        }

        if (!UpdateDriverForPlugAndPlayDevicesA(NULL, root_devices[i].hardware_id, root_devices[i].infpath, 0, NULL))
            WINE_ERR("Failed to install drivers for %s, error %#lx.\n", debugstr_a(root_devices[i].name), GetLastError());
    }

    SetupDiDestroyDeviceInfoList(set);
}

static void update_user_profile(void)
{
    char token_buf[sizeof(TOKEN_USER) + sizeof(SID) + sizeof(DWORD) * SID_MAX_SUB_AUTHORITIES];
    HANDLE token;
    WCHAR profile[MAX_PATH], *sid;
    DWORD size;
    HKEY hkey, profile_hkey;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &token))
        return;

    size = sizeof(token_buf);
    GetTokenInformation(token, TokenUser, token_buf, size, &size);
    CloseHandle(token);

    ConvertSidToStringSidW(((TOKEN_USER *)token_buf)->User.Sid, &sid);

    if (!RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                         0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, NULL))
    {
        if (!RegCreateKeyExW(hkey, sid, 0, NULL, 0,
                             KEY_ALL_ACCESS, NULL, &profile_hkey, NULL))
        {
            DWORD flags = 0;
            if (SHGetSpecialFolderPathW(NULL, profile, CSIDL_PROFILE, TRUE))
                set_reg_value(profile_hkey, L"ProfileImagePath", profile);
            RegSetValueExW( profile_hkey, L"Flags", 0, REG_DWORD, (const BYTE *)&flags, sizeof(flags) );
            RegCloseKey(profile_hkey);
        }

        RegCloseKey(hkey);
    }

    LocalFree(sid);
}

/* execute rundll32 on the wine.inf file if necessary */
static void update_wineprefix( BOOL force )
{
    const WCHAR *config_dir = _wgetenv( L"WINECONFIGDIR" );
    WCHAR *inf_path = get_wine_inf_path();
    int fd;
    struct stat st;

    if (!inf_path)
    {
        WINE_MESSAGE( "wine: failed to update %s, wine.inf not found\n", debugstr_w( config_dir ));
        return;
    }
    if ((fd = _wopen( inf_path, O_RDONLY )) == -1)
    {
        WINE_MESSAGE( "wine: failed to update %s with %s: %s\n",
                      debugstr_w(config_dir), debugstr_w(inf_path), strerror(errno) );
        goto done;
    }
    fstat( fd, &st );
    close( fd );

    if (update_timestamp( config_dir, st.st_mtime ) || force)
    {
        HANDLE process;
        DWORD count = 0;

        if ((process = start_rundll32( inf_path, L"PreInstall", IMAGE_FILE_MACHINE_TARGET_HOST )))
        {
            HWND hwnd = show_wait_window();
            for (;;)
            {
                if (process)
                {
                    MSG msg;
                    DWORD res = MsgWaitForMultipleObjects( 1, &process, FALSE, INFINITE, QS_ALLINPUT );
                    if (res != WAIT_OBJECT_0)
                    {
                        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
                        continue;
                    }
                    CloseHandle( process );
                }
                if (!machines[count].Machine) break;
                if (machines[count].Native)
                    process = start_rundll32( inf_path, L"DefaultInstall", IMAGE_FILE_MACHINE_TARGET_HOST );
                else
                    process = start_rundll32( inf_path, L"Wow64Install", machines[count].Machine );
                count++;
            }
            DestroyWindow( hwnd );
        }
        install_root_pnp_devices();
        update_user_profile();

        TRACE( "wine: configuration in %s has been updated.\n", debugstr_w(prettyprint_configdir()) );
    }

done:
    free( inf_path );
}

/* Process items in the StartUp group of the user's Programs under the Start Menu. Some installers put
 * shell links here to restart themselves after boot. */
static BOOL ProcessStartupItems(void)
{
    BOOL ret = FALSE;
    HRESULT hr;
    IShellFolder *psfDesktop = NULL, *psfStartup = NULL;
    LPITEMIDLIST pidlStartup = NULL, pidlItem;
    ULONG NumPIDLs;
    IEnumIDList *iEnumList = NULL;
    STRRET strret;
    WCHAR wszCommand[MAX_PATH];

    WINE_TRACE("Processing items in the StartUp folder.\n");

    hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED(hr))
    {
	WINE_ERR("Couldn't get desktop folder.\n");
	goto done;
    }

    hr = SHGetSpecialFolderLocation(NULL, CSIDL_STARTUP, &pidlStartup);
    if (FAILED(hr))
    {
	WINE_TRACE("Couldn't get StartUp folder location.\n");
	goto done;
    }

    hr = IShellFolder_BindToObject(psfDesktop, pidlStartup, NULL, &IID_IShellFolder, (LPVOID*)&psfStartup);
    if (FAILED(hr))
    {
	WINE_TRACE("Couldn't bind IShellFolder to StartUp folder.\n");
	goto done;
    }

    hr = IShellFolder_EnumObjects(psfStartup, NULL, SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &iEnumList);
    if (FAILED(hr))
    {
	WINE_TRACE("Unable to enumerate StartUp objects.\n");
	goto done;
    }

    while (IEnumIDList_Next(iEnumList, 1, &pidlItem, &NumPIDLs) == S_OK &&
	   (NumPIDLs) == 1)
    {
	hr = IShellFolder_GetDisplayNameOf(psfStartup, pidlItem, SHGDN_FORPARSING, &strret);
	if (FAILED(hr))
	    WINE_TRACE("Unable to get display name of enumeration item.\n");
	else
	{
	    hr = StrRetToBufW(&strret, pidlItem, wszCommand, MAX_PATH);
	    if (FAILED(hr))
		WINE_TRACE("Unable to parse display name.\n");
	    else
            {
                HINSTANCE hinst;

                hinst = ShellExecuteW(NULL, NULL, wszCommand, NULL, NULL, SW_SHOWNORMAL);
                if (PtrToUlong(hinst) <= 32)
                    WINE_WARN("Error %p executing command %s.\n", hinst, wine_dbgstr_w(wszCommand));
            }
	}

        ILFree(pidlItem);
    }

    /* Return success */
    ret = TRUE;

done:
    if (iEnumList) IEnumIDList_Release(iEnumList);
    if (psfStartup) IShellFolder_Release(psfStartup);
    if (pidlStartup) ILFree(pidlStartup);

    return ret;
}

static void usage( int status )
{
    WINE_MESSAGE( "Usage: wineboot [options]\n" );
    WINE_MESSAGE( "Options;\n" );
    WINE_MESSAGE( "    -h,--help         Display this help message\n" );
    WINE_MESSAGE( "    -e,--end-session  End the current session cleanly\n" );
    WINE_MESSAGE( "    -f,--force        Force exit for processes that don't exit cleanly\n" );
    WINE_MESSAGE( "    -i,--init         Perform initialization for first Wine instance\n" );
    WINE_MESSAGE( "    -k,--kill         Kill running processes without any cleanup\n" );
    WINE_MESSAGE( "    -r,--restart      Restart only, don't do normal startup operations\n" );
    WINE_MESSAGE( "    -s,--shutdown     Shutdown only, don't reboot\n" );
    WINE_MESSAGE( "    -u,--update       Update the wineprefix directory\n" );
    exit( status );
}

int __cdecl main( int argc, char *argv[] )
{
    /* First, set the current directory to SystemRoot */
    int i, j;
    BOOL end_session, force, init, kill, restart, shutdown, update;
    HANDLE event;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW = RTL_CONSTANT_STRING( L"\\KernelObjects\\__wineboot_event" );
    HANDLE process = 0;
    BOOL is_wow64;

    end_session = force = init = kill = restart = shutdown = update = FALSE;
    GetWindowsDirectoryW( windowsdir, MAX_PATH );
    if( !SetCurrentDirectoryW( windowsdir ) )
        WINE_ERR("Cannot set the dir to %s (%ld)\n", wine_dbgstr_w(windowsdir), GetLastError() );

    if (IsWow64Process( GetCurrentProcess(), &is_wow64 ) && is_wow64)
    {
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        WCHAR filename[MAX_PATH];
        void *redir;
        DWORD exit_code;

        memset( &si, 0, sizeof(si) );
        si.cb = sizeof(si);
        GetSystemDirectoryW( filename, MAX_PATH );
        wcscat( filename, L"\\wineboot.exe" );

        Wow64DisableWow64FsRedirection( &redir );
        if (CreateProcessW( filename, GetCommandLineW(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ))
        {
            WINE_TRACE( "restarting %s\n", wine_dbgstr_w(filename) );
            WaitForSingleObject( pi.hProcess, INFINITE );
            GetExitCodeProcess( pi.hProcess, &exit_code );
            ExitProcess( exit_code );
        }
        else WINE_ERR( "failed to restart 64-bit %s, err %ld\n", wine_dbgstr_w(filename), GetLastError() );
        Wow64RevertWow64FsRedirection( redir );
    }

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-') continue;
        if (argv[i][1] == '-')
        {
            if (!strcmp( argv[i], "--help" )) usage( 0 );
            else if (!strcmp( argv[i], "--end-session" )) end_session = TRUE;
            else if (!strcmp( argv[i], "--force" )) force = TRUE;
            else if (!strcmp( argv[i], "--init" )) init = TRUE;
            else if (!strcmp( argv[i], "--kill" )) kill = TRUE;
            else if (!strcmp( argv[i], "--restart" )) restart = TRUE;
            else if (!strcmp( argv[i], "--shutdown" )) shutdown = TRUE;
            else if (!strcmp( argv[i], "--update" )) update = TRUE;
            else usage( 1 );
            continue;
        }
        for (j = 1; argv[i][j]; j++)
        {
            switch (argv[i][j])
            {
            case 'e': end_session = TRUE; break;
            case 'f': force = TRUE; break;
            case 'i': init = TRUE; break;
            case 'k': kill = TRUE; break;
            case 'r': restart = TRUE; break;
            case 's': shutdown = TRUE; break;
            case 'u': update = TRUE; break;
            case 'h': usage(0); break;
            default:  usage(1); break;
            }
        }
    }

    if (end_session)
    {
        if (kill)
        {
            if (!shutdown_all_desktops( force )) return 1;
        }
        else if (!shutdown_close_windows( force )) return 1;
    }

    if (kill) kill_processes( shutdown );

    if (shutdown) return 0;

    if (NtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process, sizeof(process),
                                    machines, sizeof(machines), NULL )) machines[0].Machine = 0;

    /* create event to be inherited by services.exe */
    InitializeObjectAttributes( &attr, &nameW, OBJ_OPENIF | OBJ_INHERIT, 0, NULL );
    NtCreateEvent( &event, EVENT_ALL_ACCESS, &attr, NotificationEvent, 0 );

    ResetEvent( event );  /* in case this is a restart */

    create_user_shared_data();
    create_hardware_registry_keys();
    create_dynamic_registry_keys();
    create_computer_name_keys();
    wininit();
    pendingRename();

    ProcessWindowsFileProtection();
    ProcessRunKeys( HKEY_LOCAL_MACHINE, L"RunServicesOnce", TRUE, FALSE );

    if (init || (kill && !restart))
    {
        ProcessRunKeys( HKEY_LOCAL_MACHINE, L"RunServices", FALSE, FALSE );
        start_services_process();
    }
    if (init || update) update_wineprefix( update );

    create_volatile_environment_registry_key();
    create_known_dlls();

    ProcessRunKeys( HKEY_LOCAL_MACHINE, L"RunOnce", TRUE, TRUE );

    if (!init && !restart)
    {
        ProcessRunKeys( HKEY_LOCAL_MACHINE, L"Run", FALSE, FALSE );
        ProcessRunKeys( HKEY_CURRENT_USER, L"Run", FALSE, FALSE );
        ProcessStartupItems();
    }

    WINE_TRACE("Operation done\n");

    SetEvent( event );
    return 0;
}
