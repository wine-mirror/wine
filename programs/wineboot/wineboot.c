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

static void initialize_xstate_features(struct _KUSER_SHARED_DATA *data)
{
    XSTATE_CONFIGURATION *xstate = &data->XState;
    unsigned int i;
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
    if (!(regs[0] & XSTATE_AVX))
        return;

    xstate->EnabledFeatures = (1 << XSTATE_LEGACY_FLOATING_POINT) | (1 << XSTATE_LEGACY_SSE) | (1 << XSTATE_AVX);
    xstate->EnabledVolatileFeatures = xstate->EnabledFeatures;
    xstate->Size = sizeof(XSAVE_FORMAT) + sizeof(XSTATE);
    xstate->AllFeatureSize = regs[1];
    xstate->AllFeatures[0] = offsetof(XSAVE_FORMAT, XmmRegisters);
    xstate->AllFeatures[1] = sizeof(M128A) * 16;
    xstate->AllFeatures[2] = sizeof(YMMCONTEXT);

    for (i = 0; i < 3; ++i)
        xstate->Features[i].Size = xstate->AllFeatures[i];

    xstate->Features[1].Offset = xstate->Features[0].Size;
    xstate->Features[2].Offset = sizeof(XSAVE_FORMAT) + offsetof(XSTATE, YmmContext);

    __cpuidex(regs, 0xd, 1);
    xstate->OptimizedSave = regs[0] & 1;
    xstate->CompactionEnabled = !!(regs[0] & 2);

    __cpuidex(regs, 0xd, 2);
    TRACE("XSAVE feature 2 %#x, %#x, %#x, %#x.\n", regs[0], regs[1], regs[2], regs[3]);
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
    SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION machines[8];
    struct _KUSER_SHARED_DATA *data;
    RTL_OSVERSIONINFOEXW version;
    SYSTEM_CPU_INFORMATION sci;
    SYSTEM_BASIC_INFORMATION sbi;
    BOOLEAN *features;
    OBJECT_ATTRIBUTES attr = {sizeof(attr)};
    UNICODE_STRING name = RTL_CONSTANT_STRING( L"\\KernelObjects\\__wine_user_shared_data" );
    NTSTATUS status;
    HANDLE handle;
    ULONG i;
    HANDLE process = 0;

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
    NtQuerySystemInformation( SystemBasicInformation, &sbi, sizeof(sbi), NULL );
    NtQuerySystemInformation( SystemCpuInformation, &sci, sizeof(sci), NULL );

    data->TickCountMultiplier         = 1 << 24;
    data->LargePageMinimum            = 2 * 1024 * 1024;
    data->NtBuildNumber               = version.dwBuildNumber;
    data->NtProductType               = version.wProductType;
    data->ProductTypeIsValid          = TRUE;
    data->NativeProcessorArchitecture = sci.ProcessorArchitecture;
    data->NtMajorVersion              = version.dwMajorVersion;
    data->NtMinorVersion              = version.dwMinorVersion;
    data->SuiteMask                   = version.wSuiteMask;
    data->NumberOfPhysicalPages       = sbi.MmNumberOfPhysicalPages;
    data->NXSupportPolicy             = NX_SUPPORT_POLICY_OPTIN;
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
        features[PF_ARM_V8_INSTRUCTIONS_AVAILABLE]        = (sci.ProcessorLevel >= 8);
        break;

    case PROCESSOR_ARCHITECTURE_ARM64:
        features[PF_ARM_V8_INSTRUCTIONS_AVAILABLE]        = TRUE;
        features[PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE]  = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_V8_CRC32);
        features[PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE] = !!(sci.ProcessorFeatureBits & CPU_FEATURE_ARM_V8_CRYPTO);
        features[PF_COMPARE_EXCHANGE_DOUBLE]              = TRUE;
        features[PF_NX_ENABLED]                           = TRUE;
        features[PF_FASTFAIL_AVAILABLE]                   = TRUE;
        /* add features for other architectures supported by wow64 */
        if (!NtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process, sizeof(process),
                                         machines, sizeof(machines), NULL ))
        {
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
        }
        break;
    }
    data->ActiveProcessorCount = NtCurrentTeb()->Peb->NumberOfProcessors;
    data->ActiveGroupCount = 1;

    initialize_xstate_features( data );

    UnmapViewOfFile( data );
}

#if defined(__i386__) || defined(__x86_64__)

static void regs_to_str( int *regs, unsigned int len, WCHAR *buffer )
{
    unsigned int i;
    unsigned char *p = (unsigned char *)regs;

    for (i = 0; i < len; i++) { buffer[i] = *p++; }
    buffer[i] = 0;
}

static unsigned int get_model( unsigned int reg0, unsigned int *stepping, unsigned int *family )
{
    unsigned int model, family_id = (reg0 & (0x0f << 8)) >> 8;

    model = (reg0 & (0x0f << 4)) >> 4;
    if (family_id == 6 || family_id == 15) model |= (reg0 & (0x0f << 16)) >> 12;

    *family = family_id;
    if (family_id == 15) *family += (reg0 & (0xff << 20)) >> 20;

    *stepping = reg0 & 0x0f;
    return model;
}

static void get_identifier( WCHAR *buf, size_t size, const WCHAR *arch )
{
    unsigned int family, model, stepping;
    int regs[4] = {0, 0, 0, 0};

    __cpuid( regs, 1 );
    model = get_model( regs[0], &stepping, &family );
    swprintf( buf, size, L"%s Family %u Model %u Stepping %u", arch, family, model, stepping );
}

static void get_vendorid( WCHAR *buf )
{
    int tmp, regs[4] = {0, 0, 0, 0};

    __cpuid( regs, 0 );
    tmp = regs[2];      /* swap edx and ecx */
    regs[2] = regs[3];
    regs[3] = tmp;

    regs_to_str( regs + 1, 12, buf );
}

#else  /* __i386__ || __x86_64__ */

static void get_identifier( WCHAR *buf, size_t size, const WCHAR *arch ) { }
static void get_vendorid( WCHAR *buf ) { }

#endif  /* __i386__ || __x86_64__ */

#include "pshpack1.h"
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
    SMBIOS_TYPE_BIOS,
    SMBIOS_TYPE_SYSTEM,
    SMBIOS_TYPE_BASEBOARD,
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
#include "poppack.h"

#define RSMB (('R' << 24) | ('S' << 16) | ('M' << 8) | 'B')

static const struct smbios_header *find_smbios_entry( enum smbios_type type, const char *buf, UINT len )
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
            break;
        }
        else /* skip other entries and their strings */
        {
            for (ptr = (const char *)hdr + hdr->length; ptr - buf < len && *ptr; ptr++)
            {
                for (; ptr - buf < len; ptr++) if (!*ptr) break;
            }
            if (ptr == (const char *)hdr + hdr->length) ptr++;
            hdr = (const struct smbios_header *)(ptr + 1);
        }
    }

    return hdr;
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

    if (!(hdr = find_smbios_entry( SMBIOS_TYPE_BASEBOARD, buf, len ))) return;
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

    if (!(hdr = find_smbios_entry( SMBIOS_TYPE_BIOS, buf, len ))) return;
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

    if (!(hdr = find_smbios_entry( SMBIOS_TYPE_SYSTEM, buf, len ))) return;
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

static void create_bios_key( HKEY system_key )
{
    HKEY bios_key;
    UINT len;
    char *buf;

    if (RegCreateKeyExW( system_key, L"BIOS", 0, NULL, REG_OPTION_VOLATILE,
                         KEY_ALL_ACCESS, NULL, &bios_key, NULL ))
        return;

    len = GetSystemFirmwareTable( RSMB, 0, NULL, 0 );
    if (!(buf = malloc( len ))) goto done;
    len = GetSystemFirmwareTable( RSMB, 0, buf, len );

    create_bios_baseboard_values( bios_key, buf, len );
    create_bios_bios_values( bios_key, buf, len );
    create_bios_system_values( bios_key, buf, len );

done:
    free( buf );
    RegCloseKey( bios_key );
}

/* create the volatile hardware registry keys */
static void create_hardware_registry_keys(void)
{
    unsigned int i;
    HKEY hkey, system_key, cpu_key, fpu_key;
    SYSTEM_CPU_INFORMATION sci;
    PROCESSOR_POWER_INFORMATION* power_info;
    ULONG sizeof_power_info = sizeof(PROCESSOR_POWER_INFORMATION) * NtCurrentTeb()->Peb->NumberOfProcessors;
    UINT64 tsc_frequency = read_tsc_frequency();
    ULONG name_buffer[16];
    WCHAR id[60], vendorid[13];

    get_vendorid( vendorid );
    NtQuerySystemInformation( SystemCpuInformation, &sci, sizeof(sci), NULL );
    if (NtQuerySystemInformation( SystemProcessorBrandString, name_buffer, sizeof(name_buffer), NULL ))
        name_buffer[0] = 0;

    power_info = malloc( sizeof_power_info );
    if (power_info == NULL)
        return;
    if (NtPowerInformation( ProcessorInformation, NULL, 0, power_info, sizeof_power_info ))
        memset( power_info, 0, sizeof_power_info );

    switch (sci.ProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_ARM:
    case PROCESSOR_ARCHITECTURE_ARM64:
        swprintf( id, ARRAY_SIZE(id), L"ARM Family %u Model %u Revision %u",
                  sci.ProcessorLevel, HIBYTE(sci.ProcessorRevision), LOBYTE(sci.ProcessorRevision) );
        break;

    case PROCESSOR_ARCHITECTURE_AMD64:
        get_identifier( id, ARRAY_SIZE(id), !wcscmp(vendorid, L"AuthenticAMD") ? L"AMD64" : L"Intel64" );
        break;

    case PROCESSOR_ARCHITECTURE_INTEL:
    default:
        get_identifier( id, ARRAY_SIZE(id), L"x86" );
        break;
    }

    if (RegCreateKeyExW( HKEY_LOCAL_MACHINE, L"Hardware\\Description\\System", 0, NULL,
                         REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &system_key, NULL ))
    {
        free( power_info );
        return;
    }

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
        break;
    }

    if (sci.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM ||
        sci.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64 ||
        RegCreateKeyExW( system_key, L"FloatingPointProcessor", 0, NULL, REG_OPTION_VOLATILE,
                         KEY_ALL_ACCESS, NULL, &fpu_key, NULL ))
        fpu_key = 0;
    if (RegCreateKeyExW( system_key, L"CentralProcessor", 0, NULL, REG_OPTION_VOLATILE,
                         KEY_ALL_ACCESS, NULL, &cpu_key, NULL ))
        cpu_key = 0;

    for (i = 0; i < NtCurrentTeb()->Peb->NumberOfProcessors; i++)
    {
        WCHAR numW[10];

        swprintf( numW, ARRAY_SIZE(numW), L"%u", i );
        if (!RegCreateKeyExW( cpu_key, numW, 0, NULL, REG_OPTION_VOLATILE,
                              KEY_ALL_ACCESS, NULL, &hkey, NULL ))
        {
            DWORD tsc_freq_mhz = (DWORD)(tsc_frequency / 1000000ull); /* Hz -> Mhz */
            if (!tsc_freq_mhz) tsc_freq_mhz = power_info[i].MaxMhz;

            RegSetValueExW( hkey, L"FeatureSet", 0, REG_DWORD, (BYTE *)&sci.ProcessorFeatureBits, sizeof(DWORD) );
            set_reg_value( hkey, L"Identifier", id );
            /* TODO: report ARM properly */
            RegSetValueExA( hkey, "ProcessorNameString", 0, REG_SZ, (const BYTE *)name_buffer,
                            strlen( (char *)name_buffer ) + 1 );
            set_reg_value( hkey, L"VendorIdentifier", vendorid );
            RegSetValueExW( hkey, L"~MHz", 0, REG_DWORD, (BYTE *)&tsc_freq_mhz, sizeof(DWORD) );
            RegCloseKey( hkey );
        }
        if (sci.ProcessorArchitecture != PROCESSOR_ARCHITECTURE_ARM &&
            sci.ProcessorArchitecture != PROCESSOR_ARCHITECTURE_ARM64 &&
            !RegCreateKeyExW( fpu_key, numW, 0, NULL, REG_OPTION_VOLATILE,
                              KEY_ALL_ACCESS, NULL, &hkey, NULL ))
        {
            set_reg_value( hkey, L"Identifier", id );
            RegCloseKey( hkey );
        }
    }

    create_bios_key( system_key );

    RegCloseKey( fpu_key );
    RegCloseKey( cpu_key );
    RegCloseKey( system_key );
    free( power_info );
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

/* create the platform-specific environment registry keys */
static void create_environment_registry_keys( void )
{
    HKEY env_key;
    SYSTEM_CPU_INFORMATION sci;
    WCHAR buffer[60], vendorid[13];
    const WCHAR *arch, *parch;

    if (RegCreateKeyW( HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\Session Manager\\Environment", &env_key )) return;

    get_vendorid( vendorid );
    NtQuerySystemInformation( SystemCpuInformation, &sci, sizeof(sci), NULL );

    swprintf( buffer, ARRAY_SIZE(buffer), L"%u", NtCurrentTeb()->Peb->NumberOfProcessors );
    set_reg_value( env_key, L"NUMBER_OF_PROCESSORS", buffer );

    switch (sci.ProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_AMD64:
        arch = L"AMD64";
        parch = !wcscmp(vendorid, L"AuthenticAMD") ? L"AMD64" : L"Intel64";
        break;

    case PROCESSOR_ARCHITECTURE_INTEL:
    default:
        arch = parch = L"x86";
        break;
    }
    set_reg_value( env_key, L"PROCESSOR_ARCHITECTURE", arch );

    switch (sci.ProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_ARM:
    case PROCESSOR_ARCHITECTURE_ARM64:
        swprintf( buffer, ARRAY_SIZE(buffer), L"ARM Family %u Model %u Revision %u",
                  sci.ProcessorLevel, HIBYTE(sci.ProcessorRevision), LOBYTE(sci.ProcessorRevision) );
        break;

    case PROCESSOR_ARCHITECTURE_AMD64:
    case PROCESSOR_ARCHITECTURE_INTEL:
    default:
        get_identifier( buffer, ARRAY_SIZE(buffer), parch );
        lstrcatW( buffer, L", " );
        lstrcatW( buffer, vendorid );
        break;
    }
    set_reg_value( env_key, L"PROCESSOR_IDENTIFIER", buffer );

    swprintf( buffer, ARRAY_SIZE(buffer), L"%u", sci.ProcessorLevel );
    set_reg_value( env_key, L"PROCESSOR_LEVEL", buffer );

    swprintf( buffer, ARRAY_SIZE(buffer), L"%04x", sci.ProcessorRevision );
    set_reg_value( env_key, L"PROCESSOR_REVISION", buffer );

    RegCloseKey( env_key );
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

static INT_PTR CALLBACK wait_dlgproc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    switch (msg)
    {
    case WM_INITDIALOG:
        {
            DWORD len;
            WCHAR *buffer, text[1024];
            const WCHAR *name = (WCHAR *)lp;
            HICON icon = LoadImageW( 0, (LPCWSTR)IDI_WINLOGO, IMAGE_ICON, 48, 48, LR_SHARED );
            SendDlgItemMessageW( hwnd, IDC_WAITICON, STM_SETICON, (WPARAM)icon, 0 );
            SendDlgItemMessageW( hwnd, IDC_WAITTEXT, WM_GETTEXT, 1024, (LPARAM)text );
            len = lstrlenW(text) + lstrlenW(name) + 1;
            buffer = malloc( len * sizeof(WCHAR) );
            swprintf( buffer, len, text, name );
            SendDlgItemMessageW( hwnd, IDC_WAITTEXT, WM_SETTEXT, 0, (LPARAM)buffer );
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
                (const BYTE *)root_devices[i].hardware_id, (strlen(root_devices[i].hardware_id) + 2) * sizeof(WCHAR)))
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
        SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION machines[8];
        HANDLE process = 0;
        DWORD count = 0;

        if (NtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process, sizeof(process),
                                        machines, sizeof(machines), NULL )) machines[0].Machine = 0;

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

        WINE_MESSAGE( "wine: configuration in %s has been updated.\n", debugstr_w(prettyprint_configdir()) );
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

    /* create event to be inherited by services.exe */
    InitializeObjectAttributes( &attr, &nameW, OBJ_OPENIF | OBJ_INHERIT, 0, NULL );
    NtCreateEvent( &event, EVENT_ALL_ACCESS, &attr, NotificationEvent, 0 );

    ResetEvent( event );  /* in case this is a restart */

    create_user_shared_data();
    create_hardware_registry_keys();
    create_dynamic_registry_keys();
    create_environment_registry_keys();
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
