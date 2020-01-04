/*
 * Ntdll environment functions
 *
 * Copyright 1996, 1998 Alexandre Julliard
 * Copyright 2003       Eric Pouech
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

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/library.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "ntdll_misc.h"
#include "winnt.h"

WINE_DEFAULT_DEBUG_CHANNEL(environ);

static WCHAR empty[] = {0};
static const UNICODE_STRING empty_str = { 0, sizeof(empty), empty };
static const UNICODE_STRING null_str = { 0, 0, NULL };

static const BOOL is_win64 = (sizeof(void *) > sizeof(int));

static const WCHAR windows_dir[] = {'C',':','\\','w','i','n','d','o','w','s',0};

static BOOL first_prefix_start;  /* first ever process start in this prefix? */

static inline SIZE_T get_env_length( const WCHAR *env )
{
    const WCHAR *end = env;
    while (*end) end += strlenW(end) + 1;
    return end + 1 - env;
}

#ifdef __APPLE__
extern char **__wine_get_main_environment(void);
#else
extern char **__wine_main_environ;
static char **__wine_get_main_environment(void) { return __wine_main_environ; }
#endif


/***********************************************************************
 *           is_special_env_var
 *
 * Check if an environment variable needs to be handled specially when
 * passed through the Unix environment (i.e. prefixed with "WINE").
 */
static inline BOOL is_special_env_var( const char *var )
{
    return (!strncmp( var, "PATH=", sizeof("PATH=")-1 ) ||
            !strncmp( var, "PWD=", sizeof("PWD=")-1 ) ||
            !strncmp( var, "HOME=", sizeof("HOME=")-1 ) ||
            !strncmp( var, "TEMP=", sizeof("TEMP=")-1 ) ||
            !strncmp( var, "TMP=", sizeof("TMP=")-1 ) ||
            !strncmp( var, "QT_", sizeof("QT_")-1 ) ||
            !strncmp( var, "VK_", sizeof("VK_")-1 ));
}


/***********************************************************************
 *           set_env_var
 */
static void set_env_var( WCHAR **env, const WCHAR *name, const WCHAR *val )
{
    UNICODE_STRING nameW, valW;

    RtlInitUnicodeString( &nameW, name );
    RtlInitUnicodeString( &valW, val );
    RtlSetEnvironmentVariable( env, &nameW, &valW );
}


/***********************************************************************
 *           set_registry_variables
 *
 * Set environment variables by enumerating the values of a key;
 * helper for set_registry_environment().
 * Note that Windows happily truncates the value if it's too big.
 */
static void set_registry_variables( WCHAR **env, HANDLE hkey, ULONG type )
{
    static const WCHAR pathW[] = {'P','A','T','H'};
    static const WCHAR sep[] = {';',0};
    UNICODE_STRING env_name, env_value;
    NTSTATUS status;
    DWORD size;
    int index;
    char buffer[1024*sizeof(WCHAR) + sizeof(KEY_VALUE_FULL_INFORMATION)];
    WCHAR tmpbuf[1024];
    UNICODE_STRING tmp;
    KEY_VALUE_FULL_INFORMATION *info = (KEY_VALUE_FULL_INFORMATION *)buffer;

    tmp.Buffer = tmpbuf;
    tmp.MaximumLength = sizeof(tmpbuf);

    for (index = 0; ; index++)
    {
        status = NtEnumerateValueKey( hkey, index, KeyValueFullInformation,
                                      buffer, sizeof(buffer), &size );
        if (status != STATUS_SUCCESS && status != STATUS_BUFFER_OVERFLOW) break;
        if (info->Type != type) continue;
        env_name.Buffer = info->Name;
        env_name.Length = env_name.MaximumLength = info->NameLength;
        env_value.Buffer = (WCHAR *)(buffer + info->DataOffset);
        env_value.Length = info->DataLength;
        env_value.MaximumLength = sizeof(buffer) - info->DataOffset;
        if (env_value.Length && !env_value.Buffer[env_value.Length/sizeof(WCHAR)-1])
            env_value.Length -= sizeof(WCHAR);  /* don't count terminating null if any */
        if (!env_value.Length) continue;
        if (info->Type == REG_EXPAND_SZ)
        {
            status = RtlExpandEnvironmentStrings_U( *env, &env_value, &tmp, NULL );
            if (status != STATUS_SUCCESS && status != STATUS_BUFFER_OVERFLOW) continue;
            RtlCopyUnicodeString( &env_value, &tmp );
        }
        /* PATH is magic */
        if (env_name.Length == sizeof(pathW) &&
            !strncmpiW( env_name.Buffer, pathW, ARRAY_SIZE( pathW )) &&
            !RtlQueryEnvironmentVariable_U( *env, &env_name, &tmp ))
        {
            RtlAppendUnicodeToString( &tmp, sep );
            if (RtlAppendUnicodeStringToString( &tmp, &env_value )) continue;
            RtlCopyUnicodeString( &env_value, &tmp );
        }
        RtlSetEnvironmentVariable( env, &env_name, &env_value );
    }
}


/***********************************************************************
 *           set_registry_environment
 *
 * Set the environment variables specified in the registry.
 *
 * Note: Windows handles REG_SZ and REG_EXPAND_SZ in one pass with the
 * consequence that REG_EXPAND_SZ cannot be used reliably as it depends
 * on the order in which the variables are processed. But on Windows it
 * does not really matter since they only use %SystemDrive% and
 * %SystemRoot% which are predefined. But Wine defines these in the
 * registry, so we need two passes.
 */
static BOOL set_registry_environment( WCHAR **env, BOOL first_time )
{
    static const WCHAR env_keyW[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                     'M','a','c','h','i','n','e','\\',
                                     'S','y','s','t','e','m','\\',
                                     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                     'C','o','n','t','r','o','l','\\',
                                     'S','e','s','s','i','o','n',' ','M','a','n','a','g','e','r','\\',
                                     'E','n','v','i','r','o','n','m','e','n','t',0};
    static const WCHAR envW[] = {'E','n','v','i','r','o','n','m','e','n','t',0};
    static const WCHAR volatile_envW[] = {'V','o','l','a','t','i','l','e',' ','E','n','v','i','r','o','n','m','e','n','t',0};

    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HANDLE hkey;
    BOOL ret = FALSE;

    /* first the system environment variables */
    InitializeObjectAttributes( &attr, &nameW, 0, 0, NULL );
    RtlInitUnicodeString( &nameW, env_keyW );
    if (first_time && !NtOpenKey( &hkey, KEY_READ, &attr ))
    {
        set_registry_variables( env, hkey, REG_SZ );
        set_registry_variables( env, hkey, REG_EXPAND_SZ );
        NtClose( hkey );
    }
    else ret = TRUE;

    /* then the ones for the current user */
    if (RtlOpenCurrentUser( KEY_READ, &attr.RootDirectory ) != STATUS_SUCCESS) return ret;
    RtlInitUnicodeString( &nameW, envW );
    if (first_time && !NtOpenKey( &hkey, KEY_READ, &attr ))
    {
        set_registry_variables( env, hkey, REG_SZ );
        set_registry_variables( env, hkey, REG_EXPAND_SZ );
        NtClose( hkey );
    }

    RtlInitUnicodeString( &nameW, volatile_envW );
    if (!NtOpenKey( &hkey, KEY_READ, &attr ))
    {
        set_registry_variables( env, hkey, REG_SZ );
        set_registry_variables( env, hkey, REG_EXPAND_SZ );
        NtClose( hkey );
    }

    NtClose( attr.RootDirectory );
    return ret;
}


/***********************************************************************
 *           get_registry_value
 */
static WCHAR *get_registry_value( WCHAR *env, HKEY hkey, const WCHAR *name )
{
    char buffer[1024 * sizeof(WCHAR) + sizeof(KEY_VALUE_PARTIAL_INFORMATION)];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    DWORD len, size = sizeof(buffer);
    WCHAR *ret = NULL;
    UNICODE_STRING nameW;

    RtlInitUnicodeString( &nameW, name );
    if (NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, buffer, size, &size ))
        return NULL;

    if (size <= FIELD_OFFSET( KEY_VALUE_PARTIAL_INFORMATION, Data )) return NULL;
    len = (size - FIELD_OFFSET( KEY_VALUE_PARTIAL_INFORMATION, Data )) / sizeof(WCHAR);

    if (info->Type == REG_EXPAND_SZ)
    {
        UNICODE_STRING value, expanded;

        value.MaximumLength = len * sizeof(WCHAR);
        value.Buffer = (WCHAR *)info->Data;
        if (!value.Buffer[len - 1]) len--;  /* don't count terminating null if any */
        value.Length = len * sizeof(WCHAR);
        expanded.Length = expanded.MaximumLength = 1024 * sizeof(WCHAR);
        if (!(expanded.Buffer = RtlAllocateHeap( GetProcessHeap(), 0, expanded.MaximumLength )))
            return NULL;
        if (!RtlExpandEnvironmentStrings_U( env, &value, &expanded, NULL )) ret = expanded.Buffer;
        else RtlFreeUnicodeString( &expanded );
    }
    else if (info->Type == REG_SZ)
    {
        if ((ret = RtlAllocateHeap( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) )))
        {
            memcpy( ret, info->Data, len * sizeof(WCHAR) );
            ret[len] = 0;
        }
    }
    return ret;
}


/***********************************************************************
 *           set_additional_environment
 *
 * Set some additional environment variables not specified in the registry.
 */
static void set_additional_environment( WCHAR **env )
{
    static const WCHAR profile_keyW[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                         'M','a','c','h','i','n','e','\\',
                                         'S','o','f','t','w','a','r','e','\\',
                                         'M','i','c','r','o','s','o','f','t','\\',
                                         'W','i','n','d','o','w','s',' ','N','T','\\',
                                         'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                         'P','r','o','f','i','l','e','L','i','s','t',0};
    static const WCHAR computer_keyW[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                          'M','a','c','h','i','n','e','\\',
                                          'S','y','s','t','e','m','\\',
                                          'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                          'C','o','n','t','r','o','l','\\',
                                          'C','o','m','p','u','t','e','r','N','a','m','e','\\',
                                          'A','c','t','i','v','e','C','o','m','p','u','t','e','r','N','a','m','e',0};
    static const WCHAR computer_valueW[] = {'C','o','m','p','u','t','e','r','N','a','m','e',0};
    static const WCHAR public_valueW[] = {'P','u','b','l','i','c',0};
    static const WCHAR computernameW[] = {'C','O','M','P','U','T','E','R','N','A','M','E',0};
    static const WCHAR allusersW[] = {'A','L','L','U','S','E','R','S','P','R','O','F','I','L','E',0};
    static const WCHAR programdataW[] = {'P','r','o','g','r','a','m','D','a','t','a',0};
    static const WCHAR publicW[] = {'P','U','B','L','I','C',0};
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    WCHAR *val;
    HANDLE hkey;

    /* set the user profile variables */

    InitializeObjectAttributes( &attr, &nameW, 0, 0, NULL );
    RtlInitUnicodeString( &nameW, profile_keyW );
    if (!NtOpenKey( &hkey, KEY_READ, &attr ))
    {
        if ((val = get_registry_value( *env, hkey, programdataW )))
        {
            set_env_var( env, allusersW, val );
            set_env_var( env, programdataW, val );
            RtlFreeHeap( GetProcessHeap(), 0, val );
        }
        if ((val = get_registry_value( *env, hkey, public_valueW )))
        {
            set_env_var( env, publicW, val );
            RtlFreeHeap( GetProcessHeap(), 0, val );
        }
        NtClose( hkey );
    }

    /* set the computer name */

    RtlInitUnicodeString( &nameW, computer_keyW );
    if (!NtOpenKey( &hkey, KEY_READ, &attr ))
    {
        if ((val = get_registry_value( *env, hkey, computer_valueW )))
        {
            set_env_var( env, computernameW, val );
            RtlFreeHeap( GetProcessHeap(), 0, val );
        }
        NtClose( hkey );
    }
}


/* set an environment variable for one of the wine path variables */
static void set_wine_path_variable( WCHAR **env, const WCHAR *name, const char *unix_path )
{
    UNICODE_STRING nt_name, var_name;
    ANSI_STRING unix_name;

    RtlInitUnicodeString( &var_name, name );
    if (unix_path)
    {
        RtlInitAnsiString( &unix_name, unix_path );
        if (wine_unix_to_nt_file_name( &unix_name, &nt_name )) return;
        RtlSetEnvironmentVariable( env, &var_name, &nt_name );
        RtlFreeUnicodeString( &nt_name );
    }
    else RtlSetEnvironmentVariable( env, &var_name, NULL );
}


/***********************************************************************
 *           set_wow64_environment
 *
 * Set the environment variables that change across 32/64/Wow64.
 */
static void set_wow64_environment( WCHAR **env )
{
    static WCHAR archW[]    = {'P','R','O','C','E','S','S','O','R','_','A','R','C','H','I','T','E','C','T','U','R','E',0};
    static WCHAR arch6432W[] = {'P','R','O','C','E','S','S','O','R','_','A','R','C','H','I','T','E','W','6','4','3','2',0};
    static const WCHAR x86W[] = {'x','8','6',0};
    static const WCHAR versionW[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                     'M','a','c','h','i','n','e','\\',
                                     'S','o','f','t','w','a','r','e','\\',
                                     'M','i','c','r','o','s','o','f','t','\\',
                                     'W','i','n','d','o','w','s','\\',
                                     'C','u','r','r','e','n','t','V','e','r','s','i','o','n',0};
    static const WCHAR progdirW[]   = {'P','r','o','g','r','a','m','F','i','l','e','s','D','i','r',0};
    static const WCHAR progdir86W[] = {'P','r','o','g','r','a','m','F','i','l','e','s','D','i','r',' ','(','x','8','6',')',0};
    static const WCHAR progfilesW[] = {'P','r','o','g','r','a','m','F','i','l','e','s',0};
    static const WCHAR progfiles86W[] = {'P','r','o','g','r','a','m','F','i','l','e','s','(','x','8','6',')',0};
    static const WCHAR progw6432W[] = {'P','r','o','g','r','a','m','W','6','4','3','2',0};
    static const WCHAR commondirW[]   = {'C','o','m','m','o','n','F','i','l','e','s','D','i','r',0};
    static const WCHAR commondir86W[] = {'C','o','m','m','o','n','F','i','l','e','s','D','i','r',' ','(','x','8','6',')',0};
    static const WCHAR commonfilesW[] = {'C','o','m','m','o','n','P','r','o','g','r','a','m','F','i','l','e','s',0};
    static const WCHAR commonfiles86W[] = {'C','o','m','m','o','n','P','r','o','g','r','a','m','F','i','l','e','s','(','x','8','6',')',0};
    static const WCHAR commonw6432W[] = {'C','o','m','m','o','n','P','r','o','g','r','a','m','W','6','4','3','2',0};
    static const WCHAR winedlldirW[] = {'W','I','N','E','D','L','L','D','I','R','%','u',0};
    static const WCHAR winehomedirW[] = {'W','I','N','E','H','O','M','E','D','I','R',0};
    static const WCHAR winedatadirW[] = {'W','I','N','E','D','A','T','A','D','I','R',0};
    static const WCHAR winebuilddirW[] = {'W','I','N','E','B','U','I','L','D','D','I','R',0};
    static const WCHAR wineconfigdirW[] = {'W','I','N','E','C','O','N','F','I','G','D','I','R',0};

    WCHAR buf[64];
    UNICODE_STRING arch_strW = { sizeof(archW) - sizeof(WCHAR), sizeof(archW), archW };
    UNICODE_STRING arch6432_strW = { sizeof(arch6432W) - sizeof(WCHAR), sizeof(arch6432W), arch6432W };
    UNICODE_STRING valW = { 0, sizeof(buf), buf };
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    const char *path;
    WCHAR *val;
    HANDLE hkey;
    DWORD i;

    /* set the Wine paths */

    set_wine_path_variable( env, winedatadirW, wine_get_data_dir() );
    set_wine_path_variable( env, winehomedirW, getenv("HOME") );
    set_wine_path_variable( env, winebuilddirW, wine_get_build_dir() );
    set_wine_path_variable( env, wineconfigdirW, wine_get_config_dir() );
    for (i = 0; (path = wine_dll_enum_load_path( i )); i++)
    {
        sprintfW( buf, winedlldirW, i );
        set_wine_path_variable( env, buf, path );
    }
    sprintfW( buf, winedlldirW, i );
    set_wine_path_variable( env, buf, NULL );

    /* set the PROCESSOR_ARCHITECTURE variable */

    if (!RtlQueryEnvironmentVariable_U( *env, &arch6432_strW, &valW ))
    {
        if (is_win64)
        {
            RtlSetEnvironmentVariable( env, &arch_strW, &valW );
            RtlSetEnvironmentVariable( env, &arch6432_strW, NULL );
        }
    }
    else if (!RtlQueryEnvironmentVariable_U( *env, &arch_strW, &valW ))
    {
        if (is_wow64)
        {
            RtlSetEnvironmentVariable( env, &arch6432_strW, &valW );
            RtlInitUnicodeString( &nameW, x86W );
            RtlSetEnvironmentVariable( env, &arch_strW, &nameW );
        }
    }

    InitializeObjectAttributes( &attr, &nameW, 0, 0, NULL );
    RtlInitUnicodeString( &nameW, versionW );
    if (NtOpenKey( &hkey, KEY_READ | KEY_WOW64_64KEY, &attr )) return;

    /* set the ProgramFiles variables */

    if ((val = get_registry_value( *env, hkey, progdirW )))
    {
        if (is_win64 || is_wow64) set_env_var( env, progw6432W, val );
        if (is_win64 || !is_wow64) set_env_var( env, progfilesW, val );
        RtlFreeHeap( GetProcessHeap(), 0, val );
    }
    if ((val = get_registry_value( *env, hkey, progdir86W )))
    {
        if (is_win64 || is_wow64) set_env_var( env, progfiles86W, val );
        if (is_wow64) set_env_var( env, progfilesW, val );
        RtlFreeHeap( GetProcessHeap(), 0, val );
    }

    /* set the CommonProgramFiles variables */

    if ((val = get_registry_value( *env, hkey, commondirW )))
    {
        if (is_win64 || is_wow64) set_env_var( env, commonw6432W, val );
        if (is_win64 || !is_wow64) set_env_var( env, commonfilesW, val );
        RtlFreeHeap( GetProcessHeap(), 0, val );
    }
    if ((val = get_registry_value( *env, hkey, commondir86W )))
    {
        if (is_win64 || is_wow64) set_env_var( env, commonfiles86W, val );
        if (is_wow64) set_env_var( env, commonfilesW, val );
        RtlFreeHeap( GetProcessHeap(), 0, val );
    }
    NtClose( hkey );
}


/***********************************************************************
 *           build_initial_environment
 *
 * Build the Win32 environment from the Unix environment
 */
static WCHAR *build_initial_environment( char **env )
{
    SIZE_T size = 1;
    char **e;
    WCHAR *p, *ptr;

    /* compute the total size of the Unix environment */

    for (e = env; *e; e++)
    {
        if (is_special_env_var( *e )) continue;
        size += ntdll_umbstowcs( 0, *e, strlen(*e) + 1, NULL, 0 );
    }

    if (!(ptr = RtlAllocateHeap( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) return NULL;
    p = ptr;

    /* and fill it with the Unix environment */

    for (e = env; *e; e++)
    {
        char *str = *e;

        /* skip Unix special variables and use the Wine variants instead */
        if (!strncmp( str, "WINE", 4 ))
        {
            if (is_special_env_var( str + 4 )) str += 4;
            else if (!strncmp( str, "WINEPRELOADRESERVE=", 19 )) continue;  /* skip it */
        }
        else if (is_special_env_var( str )) continue;  /* skip it */

        ntdll_umbstowcs( 0, str, strlen(str) + 1, p, size - (p - ptr) );
        p += strlenW(p) + 1;
    }
    *p = 0;
    first_prefix_start = set_registry_environment( &ptr, TRUE );
    set_additional_environment( &ptr );
    return ptr;
}


/***********************************************************************
 *           build_envp
 *
 * Build the environment of a new child process.
 */
char **build_envp( const WCHAR *envW )
{
    static const char * const unix_vars[] = { "PATH", "TEMP", "TMP", "HOME" };
    char **envp;
    char *env, *p;
    int count = 1, length, lenW;
    unsigned int i;

    lenW = get_env_length( envW );
    length = ntdll_wcstoumbs( 0, envW, lenW, NULL, 0, NULL, NULL );
    if (!(env = RtlAllocateHeap( GetProcessHeap(), 0, length ))) return NULL;
    ntdll_wcstoumbs( 0, envW, lenW, env, length, NULL, NULL );

    for (p = env; *p; p += strlen(p) + 1, count++)
        if (is_special_env_var( p )) length += 4; /* prefix it with "WINE" */

    for (i = 0; i < ARRAY_SIZE( unix_vars ); i++)
    {
        if (!(p = getenv(unix_vars[i]))) continue;
        length += strlen(unix_vars[i]) + strlen(p) + 2;
        count++;
    }

    if ((envp = RtlAllocateHeap( GetProcessHeap(), 0, count * sizeof(*envp) + length )))
    {
        char **envptr = envp;
        char *dst = (char *)(envp + count);

        /* some variables must not be modified, so we get them directly from the unix env */
        for (i = 0; i < ARRAY_SIZE( unix_vars ); i++)
        {
            if (!(p = getenv( unix_vars[i] ))) continue;
            *envptr++ = strcpy( dst, unix_vars[i] );
            strcat( dst, "=" );
            strcat( dst, p );
            dst += strlen(dst) + 1;
        }

        /* now put the Windows environment strings */
        for (p = env; *p; p += strlen(p) + 1)
        {
            if (*p == '=') continue;  /* skip drive curdirs, this crashes some unix apps */
            if (!strncmp( p, "WINEPRELOADRESERVE=", sizeof("WINEPRELOADRESERVE=")-1 )) continue;
            if (!strncmp( p, "WINELOADERNOEXEC=", sizeof("WINELOADERNOEXEC=")-1 )) continue;
            if (!strncmp( p, "WINESERVERSOCKET=", sizeof("WINESERVERSOCKET=")-1 )) continue;
            if (is_special_env_var( p ))  /* prefix it with "WINE" */
            {
                *envptr++ = strcpy( dst, "WINE" );
                strcat( dst, p );
            }
            else
            {
                *envptr++ = strcpy( dst, p );
            }
            dst += strlen(dst) + 1;
        }
        *envptr = 0;
    }
    RtlFreeHeap( GetProcessHeap(), 0, env );
    return envp;
}


/***********************************************************************
 *           get_current_directory
 *
 * Initialize the current directory from the Unix cwd.
 */
static void get_current_directory( UNICODE_STRING *dir )
{
    const char *pwd;
    char *cwd;
    int size;

    dir->Length = 0;

    /* try to get it from the Unix cwd */

    for (size = 1024; ; size *= 2)
    {
        if (!(cwd = RtlAllocateHeap( GetProcessHeap(), 0, size ))) break;
        if (getcwd( cwd, size )) break;
        RtlFreeHeap( GetProcessHeap(), 0, cwd );
        if (errno == ERANGE) continue;
        cwd = NULL;
        break;
    }

    /* try to use PWD if it is valid, so that we don't resolve symlinks */

    pwd = getenv( "PWD" );
    if (cwd)
    {
        struct stat st1, st2;

        if (!pwd || stat( pwd, &st1 ) == -1 ||
            (!stat( cwd, &st2 ) && (st1.st_dev != st2.st_dev || st1.st_ino != st2.st_ino)))
            pwd = cwd;
    }

    if (pwd)
    {
        ANSI_STRING unix_name;
        UNICODE_STRING nt_name;

        RtlInitAnsiString( &unix_name, pwd );
        if (!wine_unix_to_nt_file_name( &unix_name, &nt_name ))
        {
            /* skip the \??\ prefix */
            if (nt_name.Length > 6 * sizeof(WCHAR) && nt_name.Buffer[5] == ':')
            {
                dir->Length = nt_name.Length - 4 * sizeof(WCHAR);
                memcpy( dir->Buffer, nt_name.Buffer + 4, dir->Length );
            }
            else  /* change \??\ to \\?\ */
            {
                dir->Length = nt_name.Length;
                memcpy( dir->Buffer, nt_name.Buffer, dir->Length );
                dir->Buffer[1] = '\\';
            }
            RtlFreeUnicodeString( &nt_name );
        }
    }

    if (!dir->Length)  /* still not initialized */
    {
        MESSAGE("Warning: could not find DOS drive for current working directory '%s', "
                "starting in the Windows directory.\n", cwd ? cwd : "" );
        dir->Length = strlenW( windows_dir ) * sizeof(WCHAR);
        memcpy( dir->Buffer, windows_dir, dir->Length );
    }
    RtlFreeHeap( GetProcessHeap(), 0, cwd );

    /* add trailing backslash */
    if (dir->Buffer[dir->Length / sizeof(WCHAR) - 1] != '\\')
    {
        dir->Buffer[dir->Length / sizeof(WCHAR)] = '\\';
        dir->Length += sizeof(WCHAR);
    }
    dir->Buffer[dir->Length / sizeof(WCHAR)] = 0;
}


/***********************************************************************
 *           is_path_prefix
 */
static inline BOOL is_path_prefix( const WCHAR *prefix, const WCHAR *path, const WCHAR *file )
{
    DWORD len = strlenW( prefix );

    if (strncmpiW( path, prefix, len )) return FALSE;
    while (path[len] == '\\') len++;
    return path + len == file;
}


/***********************************************************************
 *           get_image_path
 */
static void get_image_path( const char *argv0, UNICODE_STRING *path )
{
    static const WCHAR exeW[] = {'.','e','x','e',0};
    WCHAR *load_path, *file_part, *name, full_name[MAX_PATH];
    DWORD len;

    len = ntdll_umbstowcs( 0, argv0, strlen(argv0) + 1, NULL, 0 );
    if (!(name = RtlAllocateHeap( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) goto failed;
    ntdll_umbstowcs( 0, argv0, strlen(argv0) + 1, name, len );

    if (RtlDetermineDosPathNameType_U( name ) != RELATIVE_PATH ||
        strchrW( name, '/' ) || strchrW( name, '\\' ))
    {
        len = RtlGetFullPathName_U( name, sizeof(full_name), full_name, &file_part );
        if (!len || len > sizeof(full_name)) goto failed;
        /* try first without extension */
        if (RtlDoesFileExists_U( full_name )) goto done;
        if (len < (MAX_PATH - 4) * sizeof(WCHAR) && !strchrW( file_part, '.' ))
        {
            strcatW( file_part, exeW );
            if (RtlDoesFileExists_U( full_name )) goto done;
        }
        /* check for builtin path inside system directory */
        if (!is_path_prefix( system_dir, full_name, file_part ))
        {
            if (!is_win64 && !is_wow64) goto failed;
            if (!is_path_prefix( syswow64_dir, full_name, file_part )) goto failed;
        }
    }
    else
    {
        RtlGetExePath( name, &load_path );
        len = RtlDosSearchPath_U( load_path, name, exeW, sizeof(full_name), full_name, &file_part );
        RtlReleasePath( load_path );
        if (!len || len > sizeof(full_name))
        {
            /* build builtin path inside system directory */
            len = strlenW( system_dir );
            if (strlenW( name ) >= MAX_PATH - 4 - len) goto failed;
            strcpyW( full_name, system_dir );
            strcatW( full_name, name );
            if (!strchrW( name, '.' )) strcatW( full_name, exeW );
        }
    }
done:
    RtlCreateUnicodeString( path, full_name );
    RtlFreeHeap( GetProcessHeap(), 0, name );
    return;

failed:
    MESSAGE( "wine: cannot find '%s'\n", argv0 );
    RtlExitUserProcess( GetLastError() );
}


/***********************************************************************
 *              set_library_wargv
 *
 * Set the Wine library Unicode argv global variables.
 */
static void set_library_wargv( char **argv, const UNICODE_STRING *image )
{
    int argc;
    WCHAR *p, **wargv;
    DWORD total = 0;

    if (image) total += 1 + image->Length / sizeof(WCHAR);
    for (argc = (image != NULL); argv[argc]; argc++)
        total += ntdll_umbstowcs( 0, argv[argc], strlen(argv[argc]) + 1, NULL, 0 );

    wargv = RtlAllocateHeap( GetProcessHeap(), 0,
                             total * sizeof(WCHAR) + (argc + 1) * sizeof(*wargv) );
    p = (WCHAR *)(wargv + argc + 1);
    if (image)
    {
        strcpyW( p, image->Buffer );
        wargv[0] = p;
        p += 1 + image->Length / sizeof(WCHAR);
        total -= 1 + image->Length / sizeof(WCHAR);
    }
    for (argc = (image != NULL); argv[argc]; argc++)
    {
        DWORD reslen = ntdll_umbstowcs( 0, argv[argc], strlen(argv[argc]) + 1, p, total );
        wargv[argc] = p;
        p += reslen;
        total -= reslen;
    }
    wargv[argc] = NULL;

    __wine_main_argc = argc;
    __wine_main_wargv = wargv;
}


/***********************************************************************
 *           build_command_line
 *
 * Build the command line of a process from the argv array.
 *
 * Note that it does NOT necessarily include the file name.
 * Sometimes we don't even have any command line options at all.
 *
 * We must quote and escape characters so that the argv array can be rebuilt
 * from the command line:
 * - spaces and tabs must be quoted
 *   'a b'   -> '"a b"'
 * - quotes must be escaped
 *   '"'     -> '\"'
 * - if '\'s are followed by a '"', they must be doubled and followed by '\"',
 *   resulting in an odd number of '\' followed by a '"'
 *   '\"'    -> '\\\"'
 *   '\\"'   -> '\\\\\"'
 * - '\'s are followed by the closing '"' must be doubled,
 *   resulting in an even number of '\' followed by a '"'
 *   ' \'    -> '" \\"'
 *   ' \\'    -> '" \\\\"'
 * - '\'s that are not followed by a '"' can be left as is
 *   'a\b'   == 'a\b'
 *   'a\\b'  == 'a\\b'
 */
static void build_command_line( WCHAR **argv, UNICODE_STRING *cmdline )
{
    int len;
    WCHAR **arg;
    LPWSTR p;

    len = 1;
    for (arg = argv; *arg; arg++) len += 3 + 2 * strlenW( *arg );
    cmdline->MaximumLength = len * sizeof(WCHAR);
    if (!(cmdline->Buffer = RtlAllocateHeap( GetProcessHeap(), 0, cmdline->MaximumLength ))) return;

    p = cmdline->Buffer;
    for (arg = argv; *arg; arg++)
    {
        BOOL has_space, has_quote;
        int i, bcount;
        WCHAR *a;

        /* check for quotes and spaces in this argument */
        if (arg == argv || !**arg) has_space = TRUE;
        else has_space = strchrW( *arg, ' ' ) || strchrW( *arg, '\t' );
        has_quote = strchrW( *arg, '"' ) != NULL;

        /* now transfer it to the command line */
        if (has_space) *p++ = '"';
        if (has_quote || has_space)
        {
            bcount = 0;
            for (a = *arg; *a; a++)
            {
                if (*a == '\\') bcount++;
                else
                {
                    if (*a == '"') /* double all the '\\' preceding this '"', plus one */
                        for (i = 0; i <= bcount; i++) *p++ = '\\';
                    bcount = 0;
                }
                *p++ = *a;
            }
        }
        else
        {
            strcpyW( p, *arg );
            p += strlenW( p );
        }
        if (has_space)
        {
            /* Double all the '\' preceding the closing quote */
            for (i = 0; i < bcount; i++) *p++ = '\\';
            *p++ = '"';
        }
        *p++ = ' ';
    }
    if (p > cmdline->Buffer) p--;  /* remove last space */
    *p = 0;
    cmdline->Length = (p - cmdline->Buffer) * sizeof(WCHAR);
}


/******************************************************************************
 *  NtQuerySystemEnvironmentValue		[NTDLL.@]
 */
NTSYSAPI NTSTATUS WINAPI NtQuerySystemEnvironmentValue(PUNICODE_STRING VariableName,
                                                       PWCHAR Value,
                                                       ULONG ValueBufferLength,
                                                       PULONG RequiredLength)
{
    FIXME("(%s, %p, %u, %p), stub\n", debugstr_us(VariableName), Value, ValueBufferLength, RequiredLength);
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *  NtQuerySystemEnvironmentValueEx		[NTDLL.@]
 */
NTSYSAPI NTSTATUS WINAPI NtQuerySystemEnvironmentValueEx(PUNICODE_STRING name, LPGUID vendor,
                                                         PVOID value, PULONG retlength, PULONG attrib)
{
    FIXME("(%s, %s, %p, %p, %p), stub\n", debugstr_us(name), debugstr_guid(vendor), value, retlength, attrib);
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *  RtlCreateEnvironment		[NTDLL.@]
 */
NTSTATUS WINAPI RtlCreateEnvironment(BOOLEAN inherit, PWSTR* env)
{
    SIZE_T size;

    TRACE("(%u,%p)!\n", inherit, env);

    if (inherit)
    {
        RtlAcquirePebLock();
        size = get_env_length( NtCurrentTeb()->Peb->ProcessParameters->Environment ) * sizeof(WCHAR);
        if ((*env = RtlAllocateHeap( GetProcessHeap(), 0, size )))
            memcpy( *env, NtCurrentTeb()->Peb->ProcessParameters->Environment, size );
        RtlReleasePebLock();
    }
    else *env = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WCHAR) );

    return *env ? STATUS_SUCCESS : STATUS_NO_MEMORY;
}

/******************************************************************************
 *  RtlDestroyEnvironment		[NTDLL.@]
 */
NTSTATUS WINAPI RtlDestroyEnvironment(PWSTR env) 
{
    RtlFreeHeap( GetProcessHeap(), 0, env );
    return STATUS_SUCCESS;
}

static LPCWSTR ENV_FindVariable(PCWSTR var, PCWSTR name, unsigned namelen)
{
    for (; *var; var += strlenW(var) + 1)
    {
        /* match var names, but avoid setting a var with a name including a '='
         * (a starting '=' is valid though)
         */
        if (strncmpiW(var, name, namelen) == 0 && var[namelen] == '=' &&
            strchrW(var + 1, '=') == var + namelen) 
        {
            return var + namelen + 1;
        }
    }
    return NULL;
}

/******************************************************************
 *		RtlQueryEnvironmentVariable_U   [NTDLL.@]
 *
 * NOTES: when the buffer is too small, the string is not written, but if the
 *      terminating null char is the only char that cannot be written, then
 *      all chars (except the null) are written and success is returned
 *      (behavior of Win2k at least)
 */
NTSTATUS WINAPI RtlQueryEnvironmentVariable_U(PWSTR env,
                                              PUNICODE_STRING name,
                                              PUNICODE_STRING value)
{
    NTSTATUS    nts = STATUS_VARIABLE_NOT_FOUND;
    PCWSTR      var;
    unsigned    namelen;

    TRACE("%p %s %p\n", env, debugstr_us(name), value);

    value->Length = 0;
    namelen = name->Length / sizeof(WCHAR);
    if (!namelen) return nts;

    if (!env)
    {
        RtlAcquirePebLock();
        var = NtCurrentTeb()->Peb->ProcessParameters->Environment;
    }
    else var = env;

    var = ENV_FindVariable(var, name->Buffer, namelen);
    if (var != NULL)
    {
        value->Length = strlenW(var) * sizeof(WCHAR);

        if (value->Length <= value->MaximumLength)
        {
            memmove(value->Buffer, var, min(value->Length + sizeof(WCHAR), value->MaximumLength));
            nts = STATUS_SUCCESS;
        }
        else nts = STATUS_BUFFER_TOO_SMALL;
    }

    if (!env) RtlReleasePebLock();

    return nts;
}

/******************************************************************
 *		RtlSetCurrentEnvironment        [NTDLL.@]
 *
 */
void WINAPI RtlSetCurrentEnvironment(PWSTR new_env, PWSTR* old_env)
{
    TRACE("(%p %p)\n", new_env, old_env);

    RtlAcquirePebLock();

    if (old_env) *old_env = NtCurrentTeb()->Peb->ProcessParameters->Environment;
    NtCurrentTeb()->Peb->ProcessParameters->Environment = new_env;

    RtlReleasePebLock();
}


/******************************************************************************
 *  RtlSetEnvironmentVariable		[NTDLL.@]
 */
NTSTATUS WINAPI RtlSetEnvironmentVariable(PWSTR* penv, PUNICODE_STRING name, 
                                          PUNICODE_STRING value)
{
    INT         len, old_size;
    LPWSTR      p, env;
    NTSTATUS    nts = STATUS_VARIABLE_NOT_FOUND;

    TRACE("(%p, %s, %s)\n", penv, debugstr_us(name), debugstr_us(value));

    if (!name || !name->Buffer || !name->Length)
        return STATUS_INVALID_PARAMETER_1;

    len = name->Length / sizeof(WCHAR);

    /* variable names can't contain a '=' except as a first character */
    for (p = name->Buffer + 1; p < name->Buffer + len; p++)
        if (*p == '=') return STATUS_INVALID_PARAMETER;

    if (!penv)
    {
        RtlAcquirePebLock();
        env = NtCurrentTeb()->Peb->ProcessParameters->Environment;
    } else env = *penv;

    old_size = get_env_length( env );

    /* Find a place to insert the string */
    for (p = env; *p; p += strlenW(p) + 1)
    {
        if (!strncmpiW(name->Buffer, p, len) && (p[len] == '=')) break;
    }
    if (!value && !*p) goto done;  /* Value to remove doesn't exist */

    /* Realloc the buffer */
    len = value ? len + value->Length / sizeof(WCHAR) + 2 : 0;
    if (*p) len -= strlenW(p) + 1;  /* The name already exists */

    if (len < 0)
    {
        LPWSTR next = p + strlenW(p) + 1;  /* We know there is a next one */
        memmove(next + len, next, (old_size - (next - env)) * sizeof(WCHAR));
    }

    if ((old_size + len) * sizeof(WCHAR) > RtlSizeHeap( GetProcessHeap(), 0, env ))
    {
        SIZE_T new_size = max( old_size * 2, old_size + len ) * sizeof(WCHAR);
        LPWSTR new_env = RtlAllocateHeap( GetProcessHeap(), 0, new_size );

        if (!new_env)
        {
            nts = STATUS_NO_MEMORY;
            goto done;
        }
        memmove(new_env, env, (p - env) * sizeof(WCHAR));
        assert(len > 0);
        memmove(new_env + (p - env) + len, p, (old_size - (p - env)) * sizeof(WCHAR));
        p = new_env + (p - env);

        RtlDestroyEnvironment(env);
        if (!penv) NtCurrentTeb()->Peb->ProcessParameters->Environment = new_env;
        else *penv = new_env;
    }
    else
    {
        if (len > 0) memmove(p + len, p, (old_size - (p - env)) * sizeof(WCHAR));
    }

    /* Set the new string */
    if (value)
    {
        memcpy( p, name->Buffer, name->Length );
        p += name->Length / sizeof(WCHAR);
        *p++ = '=';
        memcpy( p, value->Buffer, value->Length );
        p[value->Length / sizeof(WCHAR)] = 0;
    }
    nts = STATUS_SUCCESS;

done:
    if (!penv) RtlReleasePebLock();

    return nts;
}

/******************************************************************************
 *		RtlExpandEnvironmentStrings (NTDLL.@)
 */
NTSTATUS WINAPI RtlExpandEnvironmentStrings( const WCHAR *renv, WCHAR *src, SIZE_T src_len,
                                             WCHAR *dst, SIZE_T count, SIZE_T *plen )
{
    SIZE_T len, total_size = 1;  /* 1 for terminating '\0' */
    LPCWSTR env, p, var;

    if (!renv)
    {
        RtlAcquirePebLock();
        env = NtCurrentTeb()->Peb->ProcessParameters->Environment;
    }
    else env = renv;

    while (src_len)
    {
        if (*src != '%')
        {
            if ((p = memchrW( src, '%', src_len ))) len = p - src;
            else len = src_len;
            var = src;
            src += len;
            src_len -= len;
        }
        else  /* we are at the start of a variable */
        {
            if ((p = memchrW( src + 1, '%', src_len - 1 )))
            {
                len = p - src - 1;  /* Length of the variable name */
                if ((var = ENV_FindVariable( env, src + 1, len )))
                {
                    src += len + 2;  /* Skip the variable name */
                    src_len -= len + 2;
                    len = strlenW(var);
                }
                else
                {
                    var = src;  /* Copy original name instead */
                    len += 2;
                    src += len;
                    src_len -= len;
                }
            }
            else  /* unfinished variable name, ignore it */
            {
                var = src;
                len = src_len;  /* Copy whole string */
                src += len;
                src_len = 0;
            }
        }
        total_size += len;
        if (dst)
        {
            if (count < len) len = count;
            memcpy(dst, var, len * sizeof(WCHAR));
            count -= len;
            dst += len;
        }
    }

    if (!renv) RtlReleasePebLock();

    if (dst && count) *dst = '\0';
    if (plen) *plen = total_size;

    return (count) ? STATUS_SUCCESS : STATUS_BUFFER_TOO_SMALL;
}

/******************************************************************
 *		RtlExpandEnvironmentStrings_U (NTDLL.@)
 */
NTSTATUS WINAPI RtlExpandEnvironmentStrings_U( const WCHAR *env, const UNICODE_STRING *src,
                                               UNICODE_STRING *dst, ULONG *plen )
{
    SIZE_T len;
    NTSTATUS ret;

    ret = RtlExpandEnvironmentStrings( env, src->Buffer, src->Length / sizeof(WCHAR),
                                       dst->Buffer, dst->MaximumLength / sizeof(WCHAR), &len );
    if (plen) *plen = len * sizeof(WCHAR);  /* FIXME: check for overflow? */
    if (len > UNICODE_STRING_MAX_CHARS) ret = STATUS_BUFFER_TOO_SMALL;
    if (!ret) dst->Length = (len - 1) * sizeof(WCHAR);
    return ret;
}

static inline void normalize( void *base, WCHAR **ptr )
{
    if (*ptr) *ptr = (WCHAR *)((char *)base + (UINT_PTR)*ptr);
}

/******************************************************************************
 *  RtlNormalizeProcessParams  [NTDLL.@]
 */
PRTL_USER_PROCESS_PARAMETERS WINAPI RtlNormalizeProcessParams( RTL_USER_PROCESS_PARAMETERS *params )
{
    if (params && !(params->Flags & PROCESS_PARAMS_FLAG_NORMALIZED))
    {
        normalize( params, &params->CurrentDirectory.DosPath.Buffer );
        normalize( params, &params->DllPath.Buffer );
        normalize( params, &params->ImagePathName.Buffer );
        normalize( params, &params->CommandLine.Buffer );
        normalize( params, &params->WindowTitle.Buffer );
        normalize( params, &params->Desktop.Buffer );
        normalize( params, &params->ShellInfo.Buffer );
        normalize( params, &params->RuntimeInfo.Buffer );
        params->Flags |= PROCESS_PARAMS_FLAG_NORMALIZED;
    }
    return params;
}


static inline void denormalize( const void *base, WCHAR **ptr )
{
    if (*ptr) *ptr = (WCHAR *)(UINT_PTR)((char *)*ptr - (const char *)base);
}

/******************************************************************************
 *  RtlDeNormalizeProcessParams  [NTDLL.@]
 */
PRTL_USER_PROCESS_PARAMETERS WINAPI RtlDeNormalizeProcessParams( RTL_USER_PROCESS_PARAMETERS *params )
{
    if (params && (params->Flags & PROCESS_PARAMS_FLAG_NORMALIZED))
    {
        denormalize( params, &params->CurrentDirectory.DosPath.Buffer );
        denormalize( params, &params->DllPath.Buffer );
        denormalize( params, &params->ImagePathName.Buffer );
        denormalize( params, &params->CommandLine.Buffer );
        denormalize( params, &params->WindowTitle.Buffer );
        denormalize( params, &params->Desktop.Buffer );
        denormalize( params, &params->ShellInfo.Buffer );
        denormalize( params, &params->RuntimeInfo.Buffer );
        params->Flags &= ~PROCESS_PARAMS_FLAG_NORMALIZED;
    }
    return params;
}


#define ROUND_SIZE(size) (((size) + sizeof(void *) - 1) & ~(sizeof(void *) - 1))

/* append a unicode string to the process params data; helper for RtlCreateProcessParameters */
static void append_unicode_string( void **data, const UNICODE_STRING *src,
                                   UNICODE_STRING *dst )
{
    dst->Length = src->Length;
    dst->MaximumLength = src->MaximumLength;
    if (dst->MaximumLength)
    {
        dst->Buffer = *data;
        memcpy( dst->Buffer, src->Buffer, dst->Length );
        *data = (char *)dst->Buffer + ROUND_SIZE( dst->MaximumLength );
    }
    else dst->Buffer = NULL;
}


/******************************************************************************
 *  RtlCreateProcessParametersEx  [NTDLL.@]
 */
NTSTATUS WINAPI RtlCreateProcessParametersEx( RTL_USER_PROCESS_PARAMETERS **result,
                                              const UNICODE_STRING *ImagePathName,
                                              const UNICODE_STRING *DllPath,
                                              const UNICODE_STRING *CurrentDirectoryName,
                                              const UNICODE_STRING *CommandLine,
                                              PWSTR Environment,
                                              const UNICODE_STRING *WindowTitle,
                                              const UNICODE_STRING *Desktop,
                                              const UNICODE_STRING *ShellInfo,
                                              const UNICODE_STRING *RuntimeInfo,
                                              ULONG flags )
{
    UNICODE_STRING curdir;
    const RTL_USER_PROCESS_PARAMETERS *cur_params;
    SIZE_T size, env_size = 0;
    void *ptr;
    NTSTATUS status = STATUS_SUCCESS;

    RtlAcquirePebLock();
    cur_params = NtCurrentTeb()->Peb->ProcessParameters;
    if (!DllPath) DllPath = &cur_params->DllPath;
    if (!CurrentDirectoryName)
    {
        if (NtCurrentTeb()->Tib.SubSystemTib)  /* FIXME: hack */
            curdir = ((WIN16_SUBSYSTEM_TIB *)NtCurrentTeb()->Tib.SubSystemTib)->curdir.DosPath;
        else
            curdir = cur_params->CurrentDirectory.DosPath;
    }
    else curdir = *CurrentDirectoryName;
    curdir.MaximumLength = MAX_PATH * sizeof(WCHAR);

    if (!CommandLine) CommandLine = ImagePathName;
    if (!Environment && cur_params) Environment = cur_params->Environment;
    if (!WindowTitle) WindowTitle = &empty_str;
    if (!Desktop) Desktop = &empty_str;
    if (!ShellInfo) ShellInfo = &empty_str;
    if (!RuntimeInfo) RuntimeInfo = &null_str;

    if (Environment) env_size = get_env_length( Environment ) * sizeof(WCHAR);

    size = (sizeof(RTL_USER_PROCESS_PARAMETERS)
            + ROUND_SIZE( ImagePathName->MaximumLength )
            + ROUND_SIZE( DllPath->MaximumLength )
            + ROUND_SIZE( curdir.MaximumLength )
            + ROUND_SIZE( CommandLine->MaximumLength )
            + ROUND_SIZE( WindowTitle->MaximumLength )
            + ROUND_SIZE( Desktop->MaximumLength )
            + ROUND_SIZE( ShellInfo->MaximumLength )
            + ROUND_SIZE( RuntimeInfo->MaximumLength ));

    if ((ptr = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, size + ROUND_SIZE(env_size) )))
    {
        RTL_USER_PROCESS_PARAMETERS *params = ptr;
        params->AllocationSize = size;
        params->Size           = size;
        params->Flags          = PROCESS_PARAMS_FLAG_NORMALIZED;
        if (cur_params) params->ConsoleFlags = cur_params->ConsoleFlags;
        /* all other fields are zero */

        ptr = params + 1;
        append_unicode_string( &ptr, &curdir, &params->CurrentDirectory.DosPath );
        append_unicode_string( &ptr, DllPath, &params->DllPath );
        append_unicode_string( &ptr, ImagePathName, &params->ImagePathName );
        append_unicode_string( &ptr, CommandLine, &params->CommandLine );
        append_unicode_string( &ptr, WindowTitle, &params->WindowTitle );
        append_unicode_string( &ptr, Desktop, &params->Desktop );
        append_unicode_string( &ptr, ShellInfo, &params->ShellInfo );
        append_unicode_string( &ptr, RuntimeInfo, &params->RuntimeInfo );
        if (Environment) params->Environment = memcpy( ptr, Environment, env_size );
        *result = params;
        if (!(flags & PROCESS_PARAMS_FLAG_NORMALIZED)) RtlDeNormalizeProcessParams( params );
    }
    else status = STATUS_NO_MEMORY;

    RtlReleasePebLock();
    return status;
}


/******************************************************************************
 *  RtlCreateProcessParameters  [NTDLL.@]
 */
NTSTATUS WINAPI RtlCreateProcessParameters( RTL_USER_PROCESS_PARAMETERS **result,
                                            const UNICODE_STRING *image,
                                            const UNICODE_STRING *dllpath,
                                            const UNICODE_STRING *curdir,
                                            const UNICODE_STRING *cmdline,
                                            PWSTR env,
                                            const UNICODE_STRING *title,
                                            const UNICODE_STRING *desktop,
                                            const UNICODE_STRING *shellinfo,
                                            const UNICODE_STRING *runtime )
{
    return RtlCreateProcessParametersEx( result, image, dllpath, curdir, cmdline,
                                         env, title, desktop, shellinfo, runtime, 0 );
}


/******************************************************************************
 *  RtlDestroyProcessParameters  [NTDLL.@]
 */
void WINAPI RtlDestroyProcessParameters( RTL_USER_PROCESS_PARAMETERS *params )
{
    RtlFreeHeap( GetProcessHeap(), 0, params );
}


static inline void get_unicode_string( UNICODE_STRING *str, WCHAR **src, UINT len )
{
    str->Buffer = *src;
    str->Length = len;
    str->MaximumLength = len + sizeof(WCHAR);
    *src += len / sizeof(WCHAR);
}


/***********************************************************************
 *           run_wineboot
 */
static void run_wineboot( WCHAR **env )
{
    static const WCHAR wineboot_eventW[] = {'\\','K','e','r','n','e','l','O','b','j','e','c','t','s',
                                            '\\','_','_','w','i','n','e','b','o','o','t','_','e','v','e','n','t',0};
    static const WCHAR wineboot[] = {'\\','?','?','\\','C',':','\\','w','i','n','d','o','w','s','\\',
                                     's','y','s','t','e','m','3','2','\\',
                                     'w','i','n','e','b','o','o','t','.','e','x','e',0};
    static const WCHAR cmdline[] = {'C',':','\\','w','i','n','d','o','w','s','\\',
                                    's','y','s','t','e','m','3','2','\\',
                                    'w','i','n','e','b','o','o','t','.','e','x','e',' ',
                                    '-','-','i','n','i','t',0};
    UNICODE_STRING nameW, cmdlineW, dllpathW;
    RTL_USER_PROCESS_PARAMETERS *params;
    RTL_USER_PROCESS_INFORMATION info;
    WCHAR *load_path, *dummy;
    OBJECT_ATTRIBUTES attr;
    LARGE_INTEGER timeout;
    HANDLE handles[2];
    NTSTATUS status;
    ULONG redir = 0;
    int count = 1;

    RtlInitUnicodeString( &nameW, wineboot_eventW );
    InitializeObjectAttributes( &attr, &nameW, OBJ_OPENIF, 0, NULL );

    status = NtCreateEvent( &handles[0], EVENT_ALL_ACCESS, &attr, NotificationEvent, 0 );
    if (status == STATUS_OBJECT_NAME_EXISTS) goto wait;
    if (status)
    {
        ERR( "failed to create wineboot event, expect trouble\n" );
        return;
    }
    LdrGetDllPath( wineboot + 4, LOAD_WITH_ALTERED_SEARCH_PATH, &load_path, &dummy );
    RtlInitUnicodeString( &nameW, wineboot + 4 );
    RtlInitUnicodeString( &dllpathW, load_path );
    RtlInitUnicodeString( &cmdlineW, cmdline );
    RtlCreateProcessParametersEx( &params, &nameW, &dllpathW, NULL, &cmdlineW, *env, NULL, NULL,
                                  NULL, NULL, PROCESS_PARAMS_FLAG_NORMALIZED );
    params->hStdInput  = 0;
    params->hStdOutput = 0;
    params->hStdError  = NtCurrentTeb()->Peb->ProcessParameters->hStdError;

    RtlInitUnicodeString( &nameW, wineboot );
    RtlWow64EnableFsRedirectionEx( TRUE, &redir );
    status = RtlCreateUserProcess( &nameW, OBJ_CASE_INSENSITIVE, params,
                                   NULL, NULL, 0, FALSE, 0, 0, &info );
    RtlWow64EnableFsRedirection( !redir );
    RtlReleasePath( load_path );
    RtlDestroyProcessParameters( params );
    if (status)
    {
        ERR( "failed to start wineboot %x\n", status );
        NtClose( handles[0] );
        return;
    }
    NtResumeThread( info.Thread, NULL );
    NtClose( info.Thread );
    handles[count++] = info.Process;

wait:
    timeout.QuadPart = (ULONGLONG)(first_prefix_start ? 5 : 2) * 60 * 1000 * -10000;
    if (NtWaitForMultipleObjects( count, handles, TRUE, FALSE, &timeout ) == WAIT_TIMEOUT)
        ERR( "boot event wait timed out\n" );
    while (count) NtClose( handles[--count] );

    /* reload environment now that wineboot has run */
    set_registry_environment( env, first_prefix_start );
    set_additional_environment( env );
}


/***********************************************************************
 *           init_user_process_params
 *
 * Fill the initial RTL_USER_PROCESS_PARAMETERS structure from the server.
 */
void init_user_process_params( SIZE_T data_size )
{
    WCHAR *src, *load_path, *dummy;
    SIZE_T info_size, env_size;
    NTSTATUS status;
    startup_info_t *info = NULL;
    RTL_USER_PROCESS_PARAMETERS *params = NULL;
    UNICODE_STRING curdir, dllpath, imagepath, cmdline, title, desktop, shellinfo, runtime;

    if (!data_size)
    {
        RTL_USER_PROCESS_PARAMETERS initial_params = {0};
        WCHAR *env, curdir_buffer[MAX_PATH];

        NtCurrentTeb()->Peb->ProcessParameters = &initial_params;
        initial_params.Environment = build_initial_environment( __wine_get_main_environment() );
        curdir.Buffer = curdir_buffer;
        curdir.MaximumLength = sizeof(curdir_buffer);
        get_current_directory( &curdir );
        initial_params.CurrentDirectory.DosPath = curdir;
        get_image_path( __wine_main_argv[0], &initial_params.ImagePathName );
        set_library_wargv( __wine_main_argv, &initial_params.ImagePathName );
        build_command_line( __wine_main_wargv, &cmdline );
        LdrGetDllPath( initial_params.ImagePathName.Buffer, 0, &load_path, &dummy );
        RtlInitUnicodeString( &dllpath, load_path );

        env = initial_params.Environment;
        initial_params.Environment = NULL;  /* avoid copying it */
        if (RtlCreateProcessParametersEx( &params, &initial_params.ImagePathName, &dllpath, &curdir,
                                          &cmdline, NULL, &initial_params.ImagePathName, NULL, NULL, NULL,
                                          PROCESS_PARAMS_FLAG_NORMALIZED ))
            return;

        params->Environment = env;
        NtCurrentTeb()->Peb->ProcessParameters = params;
        RtlFreeUnicodeString( &cmdline );
        RtlReleasePath( load_path );

        if (isatty(0) || isatty(1) || isatty(2))
            params->ConsoleHandle = (HANDLE)2; /* see kernel32/kernel_private.h */
        if (!isatty(0))
            wine_server_fd_to_handle( 0, GENERIC_READ|SYNCHRONIZE,  OBJ_INHERIT, &params->hStdInput );
        if (!isatty(1))
            wine_server_fd_to_handle( 1, GENERIC_WRITE|SYNCHRONIZE, OBJ_INHERIT, &params->hStdOutput );
        if (!isatty(2))
            wine_server_fd_to_handle( 2, GENERIC_WRITE|SYNCHRONIZE, OBJ_INHERIT, &params->hStdError );
        params->wShowWindow = 1; /* SW_SHOWNORMAL */

        run_wineboot( &params->Environment );
        goto done;
    }

    if (!(info = RtlAllocateHeap( GetProcessHeap(), 0, data_size ))) return;

    SERVER_START_REQ( get_startup_info )
    {
        wine_server_set_reply( req, info, data_size );
        if (!(status = wine_server_call( req )))
        {
            data_size = wine_server_reply_size( reply );
            info_size = reply->info_size;
            env_size  = data_size - info_size;
        }
    }
    SERVER_END_REQ;
    if (status) goto done;

    src = (WCHAR *)(info + 1);
    get_unicode_string( &curdir, &src, info->curdir_len );
    get_unicode_string( &dllpath, &src, info->dllpath_len );
    get_unicode_string( &imagepath, &src, info->imagepath_len );
    get_unicode_string( &cmdline, &src, info->cmdline_len );
    get_unicode_string( &title, &src, info->title_len );
    get_unicode_string( &desktop, &src, info->desktop_len );
    get_unicode_string( &shellinfo, &src, info->shellinfo_len );
    get_unicode_string( &runtime, &src, info->runtime_len );

    runtime.MaximumLength = runtime.Length;  /* runtime info isn't a real string */

    if (RtlCreateProcessParametersEx( &params, &imagepath, &dllpath, &curdir, &cmdline, NULL,
                                      &title, &desktop, &shellinfo, &runtime,
                                      PROCESS_PARAMS_FLAG_NORMALIZED ))
        goto done;

    NtCurrentTeb()->Peb->ProcessParameters = params;
    params->DebugFlags      = info->debug_flags;
    params->ConsoleHandle   = wine_server_ptr_handle( info->console );
    params->ConsoleFlags    = info->console_flags;
    params->hStdInput       = wine_server_ptr_handle( info->hstdin );
    params->hStdOutput      = wine_server_ptr_handle( info->hstdout );
    params->hStdError       = wine_server_ptr_handle( info->hstderr );
    params->dwX             = info->x;
    params->dwY             = info->y;
    params->dwXSize         = info->xsize;
    params->dwYSize         = info->ysize;
    params->dwXCountChars   = info->xchars;
    params->dwYCountChars   = info->ychars;
    params->dwFillAttribute = info->attribute;
    params->dwFlags         = info->flags;
    params->wShowWindow     = info->show;

    /* environment needs to be a separate memory block */
    if ((params->Environment = RtlAllocateHeap( GetProcessHeap(), 0, max( env_size, sizeof(WCHAR) ))))
    {
        if (env_size) memcpy( params->Environment, (char *)info + info_size, env_size );
        else params->Environment[0] = 0;
    }

    set_library_wargv( __wine_main_argv, NULL );

done:
    RtlFreeHeap( GetProcessHeap(), 0, info );
    if (RtlSetCurrentDirectory_U( &params->CurrentDirectory.DosPath ))
    {
        MESSAGE("wine: could not open working directory %s, starting in the Windows directory.\n",
                debugstr_w( params->CurrentDirectory.DosPath.Buffer ));
        RtlInitUnicodeString( &curdir, windows_dir );
        RtlSetCurrentDirectory_U( &curdir );
    }
    if (!params->CurrentDirectory.Handle) chdir("/"); /* avoid locking removable devices */
    set_wow64_environment( &params->Environment );
}
