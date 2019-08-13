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
#include <sys/stat.h>
#include <unistd.h>
#include <windows.h>
#include <winternl.h>
#include <wine/svcctl.h>
#include <wine/asm.h>
#include <wine/debug.h>

#include <shlobj.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <shellapi.h>
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(wineboot);

extern BOOL shutdown_close_windows( BOOL force );
extern BOOL shutdown_all_desktops( BOOL force );
extern void kill_processes( BOOL kill_desktop );

static WCHAR windowsdir[MAX_PATH];
static const BOOL is_64bit = sizeof(void *) > sizeof(int);

static const WCHAR winebuilddirW[] = {'W','I','N','E','B','U','I','L','D','D','I','R',0};
static const WCHAR winedatadirW[] = {'W','I','N','E','D','A','T','A','D','I','R',0};
static const WCHAR wineconfigdirW[] = {'W','I','N','E','C','O','N','F','I','G','D','I','R',0};

/* retrieve the path to the wine.inf file */
static WCHAR *get_wine_inf_path(void)
{
    static const WCHAR loaderW[] = {'\\','l','o','a','d','e','r',0};
    static const WCHAR wine_infW[] = {'\\','w','i','n','e','.','i','n','f',0};
    WCHAR *dir, *name = NULL;

    if ((dir = _wgetenv( winebuilddirW )))
    {
        if (!(name = HeapAlloc( GetProcessHeap(), 0,
                                sizeof(loaderW) + sizeof(wine_infW) + lstrlenW(dir) * sizeof(WCHAR) )))
            return NULL;
        lstrcpyW( name, dir );
        lstrcatW( name, loaderW );
    }
    else if ((dir = _wgetenv( winedatadirW )))
    {
        if (!(name = HeapAlloc( GetProcessHeap(), 0, sizeof(wine_infW) + lstrlenW(dir) * sizeof(WCHAR) )))
            return NULL;
        lstrcpyW( name, dir );
    }
    else return NULL;

    lstrcatW( name, wine_infW );
    name[1] = '\\';  /* change \??\ to \\?\ */
    return name;
}

/* update the timestamp if different from the reference time */
static BOOL update_timestamp( const WCHAR *config_dir, unsigned long timestamp )
{
    static const WCHAR timestampW[] = {'\\','.','u','p','d','a','t','e','-','t','i','m','e','s','t','a','m','p',0};
    BOOL ret = FALSE;
    int fd, count;
    char buffer[100];
    WCHAR *file = HeapAlloc( GetProcessHeap(), 0, lstrlenW(config_dir) * sizeof(WCHAR) + sizeof(timestampW) );

    if (!file) return FALSE;
    lstrcpyW( file, config_dir );
    lstrcatW( file, timestampW );

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
    HeapFree( GetProcessHeap(), 0, file );
    return ret;
}

/* print the config directory in a more Unix-ish way */
static const char *prettyprint_configdir(void)
{
    static char buffer[MAX_PATH];
    WCHAR *path = _wgetenv( wineconfigdirW );
    char *p;

    if (!WideCharToMultiByte( CP_UNIXCP, 0, path, -1, buffer, ARRAY_SIZE(buffer), NULL, NULL ))
        strcpy( buffer + ARRAY_SIZE(buffer) - 4, "..." );

    if (!strncmp( buffer, "\\??\\unix\\", 9 ))
    {
        for (p = buffer + 9; *p; p++) if (*p == '\\') *p = '/';
        return buffer + 9;
    }
    else if (!strncmp( buffer, "\\??\\Z:\\", 7 ))
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

#if defined(__i386__) || defined(__x86_64__)

#if defined(_MSC_VER)
static void do_cpuid( unsigned int ax, unsigned int *p )
{
    __cpuid( p, ax );
}
#elif defined(__i386__)
extern void __cdecl do_cpuid( unsigned int ax, unsigned int *p );
__ASM_GLOBAL_FUNC( do_cpuid,
                   "pushl %esi\n\t"
                   "pushl %ebx\n\t"
                   "movl 12(%esp),%eax\n\t"
                   "movl 16(%esp),%esi\n\t"
                   "cpuid\n\t"
                   "movl %eax,(%esi)\n\t"
                   "movl %ebx,4(%esi)\n\t"
                   "movl %ecx,8(%esi)\n\t"
                   "movl %edx,12(%esi)\n\t"
                   "popl %ebx\n\t"
                   "popl %esi\n\t"
                   "ret" )
#else
extern void __cdecl do_cpuid( unsigned int ax, unsigned int *p );
__ASM_GLOBAL_FUNC( do_cpuid,
                   "pushq %rsi\n\t"
                   "pushq %rbx\n\t"
                   "movq %rcx,%rax\n\t"
                   "movq %rdx,%rsi\n\t"
                   "cpuid\n\t"
                   "movl %eax,(%rsi)\n\t"
                   "movl %ebx,4(%rsi)\n\t"
                   "movl %ecx,8(%rsi)\n\t"
                   "movl %edx,12(%rsi)\n\t"
                   "popq %rbx\n\t"
                   "popq %rsi\n\t"
                   "ret" )
#endif

static void regs_to_str( unsigned int *regs, unsigned int len, WCHAR *buffer )
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
    static const WCHAR fmtW[] = {'%','s',' ','F','a','m','i','l','y',' ','%','u',' ','M','o','d','e','l',
                                 ' ','%','u',' ','S','t','e','p','p','i','n','g',' ','%','u',0};
    unsigned int regs[4] = {0, 0, 0, 0}, family, model, stepping;

    do_cpuid( 1, regs );
    model = get_model( regs[0], &stepping, &family );
    swprintf( buf, size, fmtW, arch, family, model, stepping );
}

static void get_vendorid( WCHAR *buf )
{
    unsigned int tmp, regs[4] = {0, 0, 0, 0};

    do_cpuid( 0, regs );
    tmp = regs[2];      /* swap edx and ecx */
    regs[2] = regs[3];
    regs[3] = tmp;

    regs_to_str( regs + 1, 12, buf );
}

static void get_namestring( WCHAR *buf )
{
    unsigned int regs[4] = {0, 0, 0, 0};
    int i;

    do_cpuid( 0x80000000, regs );
    if (regs[0] >= 0x80000004)
    {
        do_cpuid( 0x80000002, regs );
        regs_to_str( regs, 16, buf );
        do_cpuid( 0x80000003, regs );
        regs_to_str( regs, 16, buf + 16 );
        do_cpuid( 0x80000004, regs );
        regs_to_str( regs, 16, buf + 32 );
    }
    for (i = lstrlenW(buf) - 1; i >= 0 && buf[i] == ' '; i--) buf[i] = 0;
}

#else  /* __i386__ || __x86_64__ */

static void get_identifier( WCHAR *buf, size_t size, const WCHAR *arch ) { }
static void get_vendorid( WCHAR *buf ) { }
static void get_namestring( WCHAR *buf ) { }

#endif  /* __i386__ || __x86_64__ */

/* create the volatile hardware registry keys */
static void create_hardware_registry_keys(void)
{
    static const WCHAR SystemW[] = {'H','a','r','d','w','a','r','e','\\','D','e','s','c','r','i','p','t','i','o','n','\\',
                                    'S','y','s','t','e','m',0};
    static const WCHAR fpuW[] = {'F','l','o','a','t','i','n','g','P','o','i','n','t','P','r','o','c','e','s','s','o','r',0};
    static const WCHAR cpuW[] = {'C','e','n','t','r','a','l','P','r','o','c','e','s','s','o','r',0};
    static const WCHAR FeatureSetW[] = {'F','e','a','t','u','r','e','S','e','t',0};
    static const WCHAR IdentifierW[] = {'I','d','e','n','t','i','f','i','e','r',0};
    static const WCHAR ProcessorNameStringW[] = {'P','r','o','c','e','s','s','o','r','N','a','m','e','S','t','r','i','n','g',0};
    static const WCHAR SysidW[] = {'A','T',' ','c','o','m','p','a','t','i','b','l','e',0};
    static const WCHAR ARMSysidW[] = {'A','R','M',' ','p','r','o','c','e','s','s','o','r',' ','f','a','m','i','l','y',0};
    static const WCHAR mhzKeyW[] = {'~','M','H','z',0};
    static const WCHAR VendorIdentifierW[] = {'V','e','n','d','o','r','I','d','e','n','t','i','f','i','e','r',0};
    static const WCHAR PercentDW[] = {'%','d',0};
    static const WCHAR ARMCpuDescrW[]  = {'A','R','M',' ','F','a','m','i','l','y',' ','%','d',' ','M','o','d','e','l',' ','%','d',
                                          ' ','R','e','v','i','s','i','o','n',' ','%','d',0};
    static const WCHAR x86W[] = {'x','8','6',0};
    static const WCHAR intel64W[] = {'I','n','t','e','l','6','4',0};
    static const WCHAR amd64W[] = {'A','M','D','6','4',0};
    static const WCHAR authenticamdW[] = {'A','u','t','h','e','n','t','i','c','A','M','D',0};
    unsigned int i;
    HKEY hkey, system_key, cpu_key, fpu_key;
    SYSTEM_CPU_INFORMATION sci;
    PROCESSOR_POWER_INFORMATION* power_info;
    ULONG sizeof_power_info = sizeof(PROCESSOR_POWER_INFORMATION) * NtCurrentTeb()->Peb->NumberOfProcessors;
    WCHAR id[60], namestr[49], vendorid[13];

    get_namestring( namestr );
    get_vendorid( vendorid );
    NtQuerySystemInformation( SystemCpuInformation, &sci, sizeof(sci), NULL );

    power_info = HeapAlloc( GetProcessHeap(), 0, sizeof_power_info );
    if (power_info == NULL)
        return;
    if (NtPowerInformation( ProcessorInformation, NULL, 0, power_info, sizeof_power_info ))
        memset( power_info, 0, sizeof_power_info );

    switch (sci.Architecture)
    {
    case PROCESSOR_ARCHITECTURE_ARM:
    case PROCESSOR_ARCHITECTURE_ARM64:
        swprintf( id, ARRAY_SIZE(id), ARMCpuDescrW, sci.Level, HIBYTE(sci.Revision), LOBYTE(sci.Revision) );
        break;

    case PROCESSOR_ARCHITECTURE_AMD64:
        get_identifier( id, ARRAY_SIZE(id), !wcscmp(vendorid, authenticamdW) ? amd64W : intel64W );
        break;

    case PROCESSOR_ARCHITECTURE_INTEL:
    default:
        get_identifier( id, ARRAY_SIZE(id), x86W );
        break;
    }

    if (RegCreateKeyExW( HKEY_LOCAL_MACHINE, SystemW, 0, NULL, REG_OPTION_VOLATILE,
                         KEY_ALL_ACCESS, NULL, &system_key, NULL ))
    {
        HeapFree( GetProcessHeap(), 0, power_info );
        return;
    }

    switch (sci.Architecture)
    {
    case PROCESSOR_ARCHITECTURE_ARM:
    case PROCESSOR_ARCHITECTURE_ARM64:
        set_reg_value( system_key, IdentifierW, ARMSysidW );
        break;

    case PROCESSOR_ARCHITECTURE_INTEL:
    case PROCESSOR_ARCHITECTURE_AMD64:
    default:
        set_reg_value( system_key, IdentifierW, SysidW );
        break;
    }

    if (sci.Architecture == PROCESSOR_ARCHITECTURE_ARM ||
        sci.Architecture == PROCESSOR_ARCHITECTURE_ARM64 ||
        RegCreateKeyExW( system_key, fpuW, 0, NULL, REG_OPTION_VOLATILE,
                         KEY_ALL_ACCESS, NULL, &fpu_key, NULL ))
        fpu_key = 0;
    if (RegCreateKeyExW( system_key, cpuW, 0, NULL, REG_OPTION_VOLATILE,
                         KEY_ALL_ACCESS, NULL, &cpu_key, NULL ))
        cpu_key = 0;

    for (i = 0; i < NtCurrentTeb()->Peb->NumberOfProcessors; i++)
    {
        WCHAR numW[10];

        swprintf( numW, ARRAY_SIZE(numW), PercentDW, i );
        if (!RegCreateKeyExW( cpu_key, numW, 0, NULL, REG_OPTION_VOLATILE,
                              KEY_ALL_ACCESS, NULL, &hkey, NULL ))
        {
            RegSetValueExW( hkey, FeatureSetW, 0, REG_DWORD, (BYTE *)&sci.FeatureSet, sizeof(DWORD) );
            set_reg_value( hkey, IdentifierW, id );
            /* TODO: report ARM properly */
            set_reg_value( hkey, ProcessorNameStringW, namestr );
            set_reg_value( hkey, VendorIdentifierW, vendorid );
            RegSetValueExW( hkey, mhzKeyW, 0, REG_DWORD, (BYTE *)&power_info[i].MaxMhz, sizeof(DWORD) );
            RegCloseKey( hkey );
        }
        if (sci.Architecture != PROCESSOR_ARCHITECTURE_ARM &&
            sci.Architecture != PROCESSOR_ARCHITECTURE_ARM64 &&
            !RegCreateKeyExW( fpu_key, numW, 0, NULL, REG_OPTION_VOLATILE,
                              KEY_ALL_ACCESS, NULL, &hkey, NULL ))
        {
            set_reg_value( hkey, IdentifierW, id );
            RegCloseKey( hkey );
        }
    }
    RegCloseKey( fpu_key );
    RegCloseKey( cpu_key );
    RegCloseKey( system_key );
    HeapFree( GetProcessHeap(), 0, power_info );
}


/* create the DynData registry keys */
static void create_dynamic_registry_keys(void)
{
    static const WCHAR StatDataW[] = {'P','e','r','f','S','t','a','t','s','\\',
                                      'S','t','a','t','D','a','t','a',0};
    static const WCHAR ConfigManagerW[] = {'C','o','n','f','i','g',' ','M','a','n','a','g','e','r','\\',
                                           'E','n','u','m',0};
    HKEY key;

    if (!RegCreateKeyExW( HKEY_DYN_DATA, StatDataW, 0, NULL, 0, KEY_WRITE, NULL, &key, NULL ))
        RegCloseKey( key );
    if (!RegCreateKeyExW( HKEY_DYN_DATA, ConfigManagerW, 0, NULL, 0, KEY_WRITE, NULL, &key, NULL ))
        RegCloseKey( key );
}

/* create the platform-specific environment registry keys */
static void create_environment_registry_keys( void )
{
    static const WCHAR EnvironW[]  = {'S','y','s','t','e','m','\\',
                                      'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                      'C','o','n','t','r','o','l','\\',
                                      'S','e','s','s','i','o','n',' ','M','a','n','a','g','e','r','\\',
                                      'E','n','v','i','r','o','n','m','e','n','t',0};
    static const WCHAR NumProcW[]  = {'N','U','M','B','E','R','_','O','F','_','P','R','O','C','E','S','S','O','R','S',0};
    static const WCHAR ProcArchW[] = {'P','R','O','C','E','S','S','O','R','_','A','R','C','H','I','T','E','C','T','U','R','E',0};
    static const WCHAR x86W[]      = {'x','8','6',0};
    static const WCHAR intel64W[]  = {'I','n','t','e','l','6','4',0};
    static const WCHAR amd64W[]    = {'A','M','D','6','4',0};
    static const WCHAR authenticamdW[] = {'A','u','t','h','e','n','t','i','c','A','M','D',0};
    static const WCHAR commaW[]    = {',',' ',0};
    static const WCHAR ProcIdW[]   = {'P','R','O','C','E','S','S','O','R','_','I','D','E','N','T','I','F','I','E','R',0};
    static const WCHAR ProcLvlW[]  = {'P','R','O','C','E','S','S','O','R','_','L','E','V','E','L',0};
    static const WCHAR ProcRevW[]  = {'P','R','O','C','E','S','S','O','R','_','R','E','V','I','S','I','O','N',0};
    static const WCHAR PercentDW[] = {'%','d',0};
    static const WCHAR Percent04XW[] = {'%','0','4','x',0};
    static const WCHAR ARMCpuDescrW[]  = {'A','R','M',' ','F','a','m','i','l','y',' ','%','d',' ','M','o','d','e','l',' ','%','d',
                                          ' ','R','e','v','i','s','i','o','n',' ','%','d',0};
    HKEY env_key;
    SYSTEM_CPU_INFORMATION sci;
    WCHAR buffer[60], vendorid[13];
    const WCHAR *arch, *parch;

    if (RegCreateKeyW( HKEY_LOCAL_MACHINE, EnvironW, &env_key )) return;

    get_vendorid( vendorid );
    NtQuerySystemInformation( SystemCpuInformation, &sci, sizeof(sci), NULL );

    swprintf( buffer, ARRAY_SIZE(buffer), PercentDW, NtCurrentTeb()->Peb->NumberOfProcessors );
    set_reg_value( env_key, NumProcW, buffer );

    switch (sci.Architecture)
    {
    case PROCESSOR_ARCHITECTURE_AMD64:
        arch = amd64W;
        parch = !wcscmp(vendorid, authenticamdW) ? amd64W : intel64W;
        break;

    case PROCESSOR_ARCHITECTURE_INTEL:
    default:
        arch = parch = x86W;
        break;
    }
    set_reg_value( env_key, ProcArchW, arch );

    switch (sci.Architecture)
    {
    case PROCESSOR_ARCHITECTURE_ARM:
    case PROCESSOR_ARCHITECTURE_ARM64:
        swprintf( buffer, ARRAY_SIZE(buffer), ARMCpuDescrW,
                  sci.Level, HIBYTE(sci.Revision), LOBYTE(sci.Revision) );
        break;

    case PROCESSOR_ARCHITECTURE_AMD64:
    case PROCESSOR_ARCHITECTURE_INTEL:
    default:
        get_identifier( buffer, ARRAY_SIZE(buffer), parch );
        lstrcatW( buffer, commaW );
        lstrcatW( buffer, vendorid );
        break;
    }
    set_reg_value( env_key, ProcIdW, buffer );

    swprintf( buffer, ARRAY_SIZE(buffer), PercentDW, sci.Level );
    set_reg_value( env_key, ProcLvlW, buffer );

    swprintf( buffer, ARRAY_SIZE(buffer), Percent04XW, sci.Revision );
    set_reg_value( env_key, ProcRevW, buffer );

    RegCloseKey( env_key );
}

static void create_volatile_environment_registry_key(void)
{
    static const WCHAR VolatileEnvW[] = {'V','o','l','a','t','i','l','e',' ','E','n','v','i','r','o','n','m','e','n','t',0};
    static const WCHAR AppDataW[] = {'A','P','P','D','A','T','A',0};
    static const WCHAR ClientNameW[] = {'C','L','I','E','N','T','N','A','M','E',0};
    static const WCHAR HomeDriveW[] = {'H','O','M','E','D','R','I','V','E',0};
    static const WCHAR HomePathW[] = {'H','O','M','E','P','A','T','H',0};
    static const WCHAR HomeShareW[] = {'H','O','M','E','S','H','A','R','E',0};
    static const WCHAR LocalAppDataW[] = {'L','O','C','A','L','A','P','P','D','A','T','A',0};
    static const WCHAR LogonServerW[] = {'L','O','G','O','N','S','E','R','V','E','R',0};
    static const WCHAR SessionNameW[] = {'S','E','S','S','I','O','N','N','A','M','E',0};
    static const WCHAR UserNameW[] = {'U','S','E','R','N','A','M','E',0};
    static const WCHAR UserDomainW[] = {'U','S','E','R','D','O','M','A','I','N',0};
    static const WCHAR UserProfileW[] = {'U','S','E','R','P','R','O','F','I','L','E',0};
    static const WCHAR ConsoleW[] = {'C','o','n','s','o','l','e',0};
    static const WCHAR EmptyW[] = {0};
    WCHAR path[MAX_PATH];
    WCHAR computername[MAX_COMPUTERNAME_LENGTH + 1 + 2];
    DWORD size;
    HKEY hkey;
    HRESULT hr;

    if (RegCreateKeyExW( HKEY_CURRENT_USER, VolatileEnvW, 0, NULL, REG_OPTION_VOLATILE,
                         KEY_ALL_ACCESS, NULL, &hkey, NULL ))
        return;

    hr = SHGetFolderPathW( NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, path );
    if (SUCCEEDED(hr)) set_reg_value( hkey, AppDataW, path );

    set_reg_value( hkey, ClientNameW, ConsoleW );

    /* Write the profile path's drive letter and directory components into
     * HOMEDRIVE and HOMEPATH respectively. */
    hr = SHGetFolderPathW( NULL, CSIDL_PROFILE | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, path );
    if (SUCCEEDED(hr))
    {
        set_reg_value( hkey, UserProfileW, path );
        set_reg_value( hkey, HomePathW, path + 2 );
        path[2] = '\0';
        set_reg_value( hkey, HomeDriveW, path );
    }

    size = ARRAY_SIZE(path);
    if (GetUserNameW( path, &size )) set_reg_value( hkey, UserNameW, path );

    set_reg_value( hkey, HomeShareW, EmptyW );

    hr = SHGetFolderPathW( NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, path );
    if (SUCCEEDED(hr))
        set_reg_value( hkey, LocalAppDataW, path );

    size = ARRAY_SIZE(computername) - 2;
    if (GetComputerNameW(&computername[2], &size))
    {
        set_reg_value( hkey, UserDomainW, &computername[2] );
        computername[0] = computername[1] = '\\';
        set_reg_value( hkey, LogonServerW, computername );
    }

    set_reg_value( hkey, SessionNameW, ConsoleW );
    RegCloseKey( hkey );
}

/* Performs the rename operations dictated in %SystemRoot%\Wininit.ini.
 * Returns FALSE if there was an error, or otherwise if all is ok.
 */
static BOOL wininit(void)
{
    static const WCHAR nulW[] = {'N','U','L',0};
    static const WCHAR renameW[] = {'r','e','n','a','m','e',0};
    static const WCHAR wininitW[] = {'w','i','n','i','n','i','t','.','i','n','i',0};
    static const WCHAR wininitbakW[] = {'w','i','n','i','n','i','t','.','b','a','k',0};
    WCHAR initial_buffer[1024];
    WCHAR *str, *buffer = initial_buffer;
    DWORD size = ARRAY_SIZE(initial_buffer);
    DWORD res;

    for (;;)
    {
        if (!(res = GetPrivateProfileSectionW( renameW, buffer, size, wininitW ))) return TRUE;
        if (res < size - 2) break;
        if (buffer != initial_buffer) HeapFree( GetProcessHeap(), 0, buffer );
        size *= 2;
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) return FALSE;
    }

    for (str = buffer; *str; str += lstrlenW(str) + 1)
    {
        WCHAR *value;

        if (*str == ';') continue;  /* comment */
        if (!(value = wcschr( str, '=' ))) continue;

        /* split the line into key and value */
        *value++ = 0;

        if (!lstrcmpiW( nulW, str ))
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

    if (buffer != initial_buffer) HeapFree( GetProcessHeap(), 0, buffer );

    if( !MoveFileExW( wininitW, wininitbakW, MOVEFILE_REPLACE_EXISTING) )
    {
        WINE_ERR("Couldn't rename wininit.ini, error %d\n", GetLastError() );

        return FALSE;
    }

    return TRUE;
}

static BOOL pendingRename(void)
{
    static const WCHAR ValueName[] = {'P','e','n','d','i','n','g',
                                      'F','i','l','e','R','e','n','a','m','e',
                                      'O','p','e','r','a','t','i','o','n','s',0};
    static const WCHAR SessionW[] = { 'S','y','s','t','e','m','\\',
                                     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                     'C','o','n','t','r','o','l','\\',
                                     'S','e','s','s','i','o','n',' ','M','a','n','a','g','e','r',0};
    WCHAR *buffer=NULL;
    const WCHAR *src=NULL, *dst=NULL;
    DWORD dataLength=0;
    HKEY hSession=NULL;
    DWORD res;

    WINE_TRACE("Entered\n");

    if( (res=RegOpenKeyExW( HKEY_LOCAL_MACHINE, SessionW, 0, KEY_ALL_ACCESS, &hSession ))
            !=ERROR_SUCCESS )
    {
        WINE_TRACE("The key was not found - skipping\n");
        return TRUE;
    }

    res=RegQueryValueExW( hSession, ValueName, NULL, NULL /* The value type does not really interest us, as it is not
                                                             truly a REG_MULTI_SZ anyways */,
            NULL, &dataLength );
    if( res==ERROR_FILE_NOT_FOUND )
    {
        /* No value - nothing to do. Great! */
        WINE_TRACE("Value not present - nothing to rename\n");
        res=TRUE;
        goto end;
    }

    if( res!=ERROR_SUCCESS )
    {
        WINE_ERR("Couldn't query value's length (%d)\n", res );
        res=FALSE;
        goto end;
    }

    buffer=HeapAlloc( GetProcessHeap(),0,dataLength );
    if( buffer==NULL )
    {
        WINE_ERR("Couldn't allocate %u bytes for the value\n", dataLength );
        res=FALSE;
        goto end;
    }

    res=RegQueryValueExW( hSession, ValueName, NULL, NULL, (LPBYTE)buffer, &dataLength );
    if( res!=ERROR_SUCCESS )
    {
        WINE_ERR("Couldn't query value after successfully querying before (%u),\n"
                "please report to wine-devel@winehq.org\n", res);
        res=FALSE;
        goto end;
    }

    /* Make sure that the data is long enough and ends with two NULLs. This
     * simplifies the code later on.
     */
    if( dataLength<2*sizeof(buffer[0]) ||
            buffer[dataLength/sizeof(buffer[0])-1]!='\0' ||
            buffer[dataLength/sizeof(buffer[0])-2]!='\0' )
    {
        WINE_ERR("Improper value format - doesn't end with NULL\n");
        res=FALSE;
        goto end;
    }

    for( src=buffer; (src-buffer)*sizeof(src[0])<dataLength && *src!='\0';
            src=dst+lstrlenW(dst)+1 )
    {
        DWORD dwFlags=0;

        WINE_TRACE("processing next command\n");

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

    if((res=RegDeleteValueW(hSession, ValueName))!=ERROR_SUCCESS )
    {
        WINE_ERR("Error deleting the value (%u)\n", GetLastError() );
        res=FALSE;
    } else
        res=TRUE;
    
end:
    HeapFree(GetProcessHeap(), 0, buffer);

    if( hSession!=NULL )
        RegCloseKey( hSession );

    return res;
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
        WINE_WARN("Failed to run command %s (%d)\n", wine_dbgstr_w(cmdline), GetLastError() );
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
    if (!(cmdline = HeapAlloc( GetProcessHeap(), 0, max_cmdline )))
    {
        WINE_ERR( "Couldn't allocate memory for the commands to be executed.\n" );
        goto end;
    }
    if (!(value = HeapAlloc( GetProcessHeap(), 0, ++max_value * sizeof(*value) )))
    {
        WINE_ERR( "Couldn't allocate memory for the value names.\n" );
        goto end;
    }

    while (i)
    {
        DWORD len = max_value, len_data = max_cmdline, type;

        if ((res = RegEnumValueW( runkey, --i, value, &len, 0, &type, (BYTE *)cmdline, &len_data )))
        {
            WINE_ERR( "Couldn't read value %u (%d).\n", i, res );
            continue;
        }
        if (delete && (res = RegDeleteValueW( runkey, value )))
        {
            WINE_ERR( "Couldn't delete value %u (%d). Running command anyways.\n", i, res );
        }
        if (type != REG_SZ)
        {
            WINE_ERR( "Incorrect type of value %u (%u).\n", i, type );
            continue;
        }
        if (runCmd( cmdline, NULL, synchronous, FALSE ) == INVALID_RUNCMD_RETURN)
        {
            WINE_ERR( "Error running cmd %s (%u).\n", wine_dbgstr_w(cmdline), GetLastError() );
        }
        WINE_TRACE( "Done processing cmd %u.\n", i );
    }

end:
    HeapFree( GetProcessHeap(), 0, value );
    HeapFree( GetProcessHeap(), 0, cmdline );
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
    static const WCHAR keypathW[] =
        {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
         'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n',0};
    HKEY key;

    if (root == HKEY_LOCAL_MACHINE)
    {
        WINE_TRACE( "Processing %s entries under HKLM.\n", wine_dbgstr_w(keyname) );
        if (!RegCreateKeyExW( root, keypathW, 0, NULL, 0, KEY_READ, NULL, &key, NULL ))
        {
            process_run_key( key, keyname, delete, synchronous );
            RegCloseKey( key );
        }
        if (is_64bit && !RegCreateKeyExW( root, keypathW, 0, NULL, 0, KEY_READ|KEY_WOW64_32KEY, NULL, &key, NULL ))
        {
            process_run_key( key, keyname, delete, synchronous );
            RegCloseKey( key );
        }
    }
    else
    {
        WINE_TRACE( "Processing %s entries under HKCU.\n", wine_dbgstr_w(keyname) );
        if (!RegCreateKeyExW( root, keypathW, 0, NULL, 0, KEY_READ, NULL, &key, NULL ))
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
    static const WCHAR winlogonW[] = {'S','o','f','t','w','a','r','e','\\',
                                      'M','i','c','r','o','s','o','f','t','\\',
                                      'W','i','n','d','o','w','s',' ','N','T','\\',
                                      'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                      'W','i','n','l','o','g','o','n',0};
    static const WCHAR cachedirW[] = {'S','F','C','D','l','l','C','a','c','h','e','D','i','r',0};
    static const WCHAR dllcacheW[] = {'\\','d','l','l','c','a','c','h','e','\\','*',0};
    static const WCHAR wildcardW[] = {'\\','*',0};
    WIN32_FIND_DATAW finddata;
    HANDLE find_handle;
    BOOL find_rc;
    DWORD rc;
    HKEY hkey;
    LPWSTR dllcache = NULL;

    if (!RegOpenKeyW( HKEY_LOCAL_MACHINE, winlogonW, &hkey ))
    {
        DWORD sz = 0;
        if (!RegQueryValueExW( hkey, cachedirW, 0, NULL, NULL, &sz))
        {
            sz += sizeof(WCHAR);
            dllcache = HeapAlloc(GetProcessHeap(),0,sz + sizeof(wildcardW));
            RegQueryValueExW( hkey, cachedirW, 0, NULL, (LPBYTE)dllcache, &sz);
            lstrcatW( dllcache, wildcardW );
        }
    }
    RegCloseKey(hkey);

    if (!dllcache)
    {
        DWORD sz = GetSystemDirectoryW( NULL, 0 );
        dllcache = HeapAlloc( GetProcessHeap(), 0, sz * sizeof(WCHAR) + sizeof(dllcacheW));
        GetSystemDirectoryW( dllcache, sz );
        lstrcatW( dllcache, dllcacheW );
    }

    find_handle = FindFirstFileW(dllcache,&finddata);
    dllcache[ lstrlenW(dllcache) - 2] = 0; /* strip off wildcard */
    find_rc = find_handle != INVALID_HANDLE_VALUE;
    while (find_rc)
    {
        static const WCHAR dotW[] = {'.',0};
        static const WCHAR dotdotW[] = {'.','.',0};
        WCHAR targetpath[MAX_PATH];
        WCHAR currentpath[MAX_PATH];
        UINT sz;
        UINT sz2;
        WCHAR tempfile[MAX_PATH];

        if (wcscmp(finddata.cFileName,dotW) == 0 || wcscmp(finddata.cFileName,dotdotW) == 0)
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
            WINE_WARN("WFP: %s error 0x%x\n",wine_dbgstr_w(finddata.cFileName),rc);
            DeleteFileW(tempfile);
        }

        /* now delete the source file so that we don't try to install it over and over again */
        lstrcpynW( targetpath, dllcache, MAX_PATH - 1 );
        sz = lstrlenW( targetpath );
        targetpath[sz++] = '\\';
        lstrcpynW( targetpath + sz, finddata.cFileName, MAX_PATH - sz );
        if (!DeleteFileW( targetpath ))
            WINE_WARN( "failed to delete %s: error %u\n", wine_dbgstr_w(targetpath), GetLastError() );

        find_rc = FindNextFileW(find_handle,&finddata);
    }
    FindClose(find_handle);
    HeapFree(GetProcessHeap(),0,dllcache);
    return 1;
}

static BOOL start_services_process(void)
{
    static const WCHAR svcctl_started_event[] = SVCCTL_STARTED_EVENT;
    static const WCHAR services[] = {'\\','s','e','r','v','i','c','e','s','.','e','x','e',0};
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    HANDLE wait_handles[2];
    WCHAR path[MAX_PATH];

    if (!GetSystemDirectoryW(path, MAX_PATH - lstrlenW(services)))
        return FALSE;
    lstrcatW(path, services);
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    if (!CreateProcessW(path, path, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &si, &pi))
    {
        WINE_ERR("Couldn't start services.exe: error %u\n", GetLastError());
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
        WINE_ERR("Unexpected termination of services.exe - exit code %d\n", exit_code);
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
            buffer = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
            swprintf( buffer, len, text, name );
            SendDlgItemMessageW( hwnd, IDC_WAITTEXT, WM_SETTEXT, 0, (LPARAM)buffer );
            HeapFree( GetProcessHeap(), 0, buffer );
        }
        break;
    }
    return 0;
}

static HWND show_wait_window(void)
{
    const char *config_dir = prettyprint_configdir();
    WCHAR *name;
    HWND hwnd;
    DWORD len;

    len = MultiByteToWideChar( CP_UNIXCP, 0, config_dir, -1, NULL, 0 );
    name = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    MultiByteToWideChar( CP_UNIXCP, 0, config_dir, -1, name, len );
    hwnd = CreateDialogParamW( GetModuleHandleW(0), MAKEINTRESOURCEW(IDD_WAITDLG), 0,
                               wait_dlgproc, (LPARAM)name );
    ShowWindow( hwnd, SW_SHOWNORMAL );
    HeapFree( GetProcessHeap(), 0, name );
    return hwnd;
}

static HANDLE start_rundll32( const WCHAR *inf_path, BOOL wow64 )
{
    static const WCHAR rundll[] = {'\\','r','u','n','d','l','l','3','2','.','e','x','e',0};
    static const WCHAR setupapi[] = {' ','s','e','t','u','p','a','p','i',',',
                                     'I','n','s','t','a','l','l','H','i','n','f','S','e','c','t','i','o','n',0};
    static const WCHAR definstall[] = {' ','D','e','f','a','u','l','t','I','n','s','t','a','l','l',0};
    static const WCHAR wowinstall[] = {' ','W','o','w','6','4','I','n','s','t','a','l','l',0};
    static const WCHAR flags[] = {' ','1','2','8',' ',0};

    WCHAR app[MAX_PATH + ARRAY_SIZE(rundll)];
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    WCHAR *buffer;
    DWORD len;

    memset( &si, 0, sizeof(si) );
    si.cb = sizeof(si);

    if (wow64)
    {
        if (!GetSystemWow64DirectoryW( app, MAX_PATH )) return 0;  /* not on 64-bit */
    }
    else GetSystemDirectoryW( app, MAX_PATH );

    lstrcatW( app, rundll );

    len = lstrlenW(app) + ARRAY_SIZE(setupapi) + ARRAY_SIZE(definstall) + ARRAY_SIZE(flags) + lstrlenW(inf_path);

    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return 0;

    lstrcpyW( buffer, app );
    lstrcatW( buffer, setupapi );
    lstrcatW( buffer, wow64 ? wowinstall : definstall );
    lstrcatW( buffer, flags );
    lstrcatW( buffer, inf_path );

    if (CreateProcessW( app, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ))
        CloseHandle( pi.hThread );
    else
        pi.hProcess = 0;

    HeapFree( GetProcessHeap(), 0, buffer );
    return pi.hProcess;
}

/* execute rundll32 on the wine.inf file if necessary */
static void update_wineprefix( BOOL force )
{
    const WCHAR *config_dir = _wgetenv( wineconfigdirW );
    WCHAR *inf_path = get_wine_inf_path();
    int fd;
    struct stat st;

    if (!inf_path)
    {
        WINE_MESSAGE( "wine: failed to update %s, wine.inf not found\n", prettyprint_configdir() );
        return;
    }
    if ((fd = _wopen( inf_path, O_RDONLY )) == -1)
    {
        WINE_MESSAGE( "wine: failed to update %s with %s: %s\n",
                      prettyprint_configdir(), debugstr_w(inf_path), strerror(errno) );
        goto done;
    }
    fstat( fd, &st );
    close( fd );

    if (update_timestamp( config_dir, st.st_mtime ) || force)
    {
        HANDLE process;
        DWORD count = 0;

        if ((process = start_rundll32( inf_path, FALSE )))
        {
            HWND hwnd = show_wait_window();
            for (;;)
            {
                MSG msg;
                DWORD res = MsgWaitForMultipleObjects( 1, &process, FALSE, INFINITE, QS_ALLINPUT );
                if (res == WAIT_OBJECT_0)
                {
                    CloseHandle( process );
                    if (count++ || !(process = start_rundll32( inf_path, TRUE ))) break;
                }
                else while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
            }
            DestroyWindow( hwnd );
        }
        WINE_MESSAGE( "wine: configuration in '%s' has been updated.\n", prettyprint_configdir() );
    }

done:
    HeapFree( GetProcessHeap(), 0, inf_path );
}

/* Process items in the StartUp group of the user's Programs under the Start Menu. Some installers put
 * shell links here to restart themselves after boot. */
static BOOL ProcessStartupItems(void)
{
    BOOL ret = FALSE;
    HRESULT hr;
    IMalloc *ppM = NULL;
    IShellFolder *psfDesktop = NULL, *psfStartup = NULL;
    LPITEMIDLIST pidlStartup = NULL, pidlItem;
    ULONG NumPIDLs;
    IEnumIDList *iEnumList = NULL;
    STRRET strret;
    WCHAR wszCommand[MAX_PATH];

    WINE_TRACE("Processing items in the StartUp folder.\n");

    hr = SHGetMalloc(&ppM);
    if (FAILED(hr))
    {
	WINE_ERR("Couldn't get IMalloc object.\n");
	goto done;
    }

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

	IMalloc_Free(ppM, pidlItem);
    }

    /* Return success */
    ret = TRUE;

done:
    if (iEnumList) IEnumIDList_Release(iEnumList);
    if (psfStartup) IShellFolder_Release(psfStartup);
    if (pidlStartup) IMalloc_Free(ppM, pidlStartup);

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
    static const WCHAR RunW[] = {'R','u','n',0};
    static const WCHAR RunOnceW[] = {'R','u','n','O','n','c','e',0};
    static const WCHAR RunServicesW[] = {'R','u','n','S','e','r','v','i','c','e','s',0};
    static const WCHAR RunServicesOnceW[] = {'R','u','n','S','e','r','v','i','c','e','s','O','n','c','e',0};
    static const WCHAR wineboot_eventW[] = {'_','_','w','i','n','e','b','o','o','t','_','e','v','e','n','t',0};

    /* First, set the current directory to SystemRoot */
    int i, j;
    BOOL end_session, force, init, kill, restart, shutdown, update;
    HANDLE event;
    SECURITY_ATTRIBUTES sa;
    BOOL is_wow64;

    end_session = force = init = kill = restart = shutdown = update = FALSE;
    GetWindowsDirectoryW( windowsdir, MAX_PATH );
    if( !SetCurrentDirectoryW( windowsdir ) )
        WINE_ERR("Cannot set the dir to %s (%d)\n", wine_dbgstr_w(windowsdir), GetLastError() );

    if (IsWow64Process( GetCurrentProcess(), &is_wow64 ) && is_wow64)
    {
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        WCHAR filename[MAX_PATH];
        void *redir;
        DWORD exit_code;

        memset( &si, 0, sizeof(si) );
        si.cb = sizeof(si);
        GetModuleFileNameW( 0, filename, MAX_PATH );

        Wow64DisableWow64FsRedirection( &redir );
        if (CreateProcessW( filename, GetCommandLineW(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ))
        {
            WINE_TRACE( "restarting %s\n", wine_dbgstr_w(filename) );
            WaitForSingleObject( pi.hProcess, INFINITE );
            GetExitCodeProcess( pi.hProcess, &exit_code );
            ExitProcess( exit_code );
        }
        else WINE_ERR( "failed to restart 64-bit %s, err %d\n", wine_dbgstr_w(filename), GetLastError() );
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

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;  /* so that services.exe inherits it */
    event = CreateEventW( &sa, TRUE, FALSE, wineboot_eventW );

    ResetEvent( event );  /* in case this is a restart */

    create_hardware_registry_keys();
    create_dynamic_registry_keys();
    create_environment_registry_keys();
    wininit();
    pendingRename();

    ProcessWindowsFileProtection();
    ProcessRunKeys( HKEY_LOCAL_MACHINE, RunServicesOnceW, TRUE, FALSE );

    if (init || (kill && !restart))
    {
        ProcessRunKeys( HKEY_LOCAL_MACHINE, RunServicesW, FALSE, FALSE );
        start_services_process();
    }
    if (init || update) update_wineprefix( update );

    create_volatile_environment_registry_key();

    ProcessRunKeys( HKEY_LOCAL_MACHINE, RunOnceW, TRUE, TRUE );

    if (!init && !restart)
    {
        ProcessRunKeys( HKEY_LOCAL_MACHINE, RunW, FALSE, FALSE );
        ProcessRunKeys( HKEY_CURRENT_USER, RunW, FALSE, FALSE );
        ProcessStartupItems();
    }

    WINE_TRACE("Operation done\n");

    SetEvent( event );
    return 0;
}
