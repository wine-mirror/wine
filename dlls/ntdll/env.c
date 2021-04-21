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

#include <assert.h>
#include <stdarg.h>
#include <sys/types.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/debug.h"
#include "ntdll_misc.h"
#include "winnt.h"

WINE_DEFAULT_DEBUG_CHANNEL(environ);

static WCHAR empty[] = L"";
static const UNICODE_STRING empty_str = { 0, sizeof(empty), empty };
static const UNICODE_STRING null_str = { 0, 0, NULL };

static const BOOL is_win64 = (sizeof(void *) > sizeof(int));

static BOOL first_prefix_start;  /* first ever process start in this prefix? */

static inline SIZE_T get_env_length( const WCHAR *env )
{
    const WCHAR *end = env;
    while (*end) end += wcslen(end) + 1;
    return end + 1 - env;
}


/***********************************************************************
 *           set_env_var
 */
static void set_env_var( WCHAR **env, const WCHAR *name, const WCHAR *val )
{
    UNICODE_STRING nameW, valW;

    RtlInitUnicodeString( &nameW, name );
    if (val)
    {
        RtlInitUnicodeString( &valW, val );
        RtlSetEnvironmentVariable( env, &nameW, &valW );
    }
    else RtlSetEnvironmentVariable( env, &nameW, NULL );
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
            !wcsnicmp( env_name.Buffer, pathW, ARRAY_SIZE( pathW )) &&
            !RtlQueryEnvironmentVariable_U( *env, &env_name, &tmp ))
        {
            RtlAppendUnicodeToString( &tmp, L";" );
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
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HANDLE hkey;
    BOOL ret = FALSE;

    /* first the system environment variables */
    InitializeObjectAttributes( &attr, &nameW, 0, 0, NULL );
    RtlInitUnicodeString( &nameW, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\"
                          "Session Manager\\Environment" );
    if (first_time && !NtOpenKey( &hkey, KEY_READ, &attr ))
    {
        set_registry_variables( env, hkey, REG_SZ );
        set_registry_variables( env, hkey, REG_EXPAND_SZ );
        NtClose( hkey );
    }
    else ret = TRUE;

    /* then the ones for the current user */
    if (RtlOpenCurrentUser( KEY_READ, &attr.RootDirectory ) != STATUS_SUCCESS) return ret;
    RtlInitUnicodeString( &nameW, L"Environment" );
    if (first_time && !NtOpenKey( &hkey, KEY_READ, &attr ))
    {
        set_registry_variables( env, hkey, REG_SZ );
        set_registry_variables( env, hkey, REG_EXPAND_SZ );
        NtClose( hkey );
    }

    RtlInitUnicodeString( &nameW, L"Volatile Environment" );
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
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    WCHAR *val;
    HANDLE hkey;

    /* set the user profile variables */

    InitializeObjectAttributes( &attr, &nameW, 0, 0, NULL );
    RtlInitUnicodeString( &nameW, L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\"
                          "CurrentVersion\\ProfileList" );
    if (!NtOpenKey( &hkey, KEY_READ, &attr ))
    {
        if ((val = get_registry_value( *env, hkey, L"ProgramData" )))
        {
            set_env_var( env, L"ALLUSERSPROFILE", val );
            set_env_var( env, L"ProgramData", val );
            RtlFreeHeap( GetProcessHeap(), 0, val );
        }
        if ((val = get_registry_value( *env, hkey, L"Public" )))
        {
            set_env_var( env, L"PUBLIC", val );
            RtlFreeHeap( GetProcessHeap(), 0, val );
        }
        NtClose( hkey );
    }

    /* set the computer name */

    RtlInitUnicodeString( &nameW, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\"
                          "ComputerName\\ActiveComputerName" );
    if (!NtOpenKey( &hkey, KEY_READ, &attr ))
    {
        if ((val = get_registry_value( *env, hkey, L"ComputerName" )))
        {
            set_env_var( env, L"COMPUTERNAME", val );
            RtlFreeHeap( GetProcessHeap(), 0, val );
        }
        NtClose( hkey );
    }
}


/***********************************************************************
 *           set_wow64_environment
 *
 * Set the environment variables that change across 32/64/Wow64.
 */
static void set_wow64_environment( WCHAR **env )
{
    static WCHAR archW[]    = L"PROCESSOR_ARCHITECTURE";
    static WCHAR arch6432W[] = L"PROCESSOR_ARCHITEW6432";

    WCHAR buf[256];
    UNICODE_STRING arch_strW = { sizeof(archW) - sizeof(WCHAR), sizeof(archW), archW };
    UNICODE_STRING arch6432_strW = { sizeof(arch6432W) - sizeof(WCHAR), sizeof(arch6432W), arch6432W };
    UNICODE_STRING valW = { 0, sizeof(buf), buf };
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HANDLE hkey;
    SIZE_T size = 1024;
    WCHAR *ptr, *val, *p;

    for (;;)
    {
        if (!(ptr = RtlAllocateHeap( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) break;
        if (!unix_funcs->get_dynamic_environment( ptr, &size )) break;
        RtlFreeHeap( GetProcessHeap(), 0, ptr );
    }
    for (p = ptr; *p; p += wcslen(p) + 1)
    {
        if ((val = wcschr( p, '=' ))) *val++ = 0;
        set_env_var( env, p, val );
        if (val) p = val;
    }
    RtlFreeHeap( GetProcessHeap(), 0, ptr );

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
            RtlInitUnicodeString( &nameW, L"x86" );
            RtlSetEnvironmentVariable( env, &arch_strW, &nameW );
        }
    }

    InitializeObjectAttributes( &attr, &nameW, 0, 0, NULL );
    RtlInitUnicodeString( &nameW, L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion" );
    if (NtOpenKey( &hkey, KEY_READ | KEY_WOW64_64KEY, &attr )) return;

    /* set the ProgramFiles variables */

    if ((val = get_registry_value( *env, hkey, L"ProgramFilesDir" )))
    {
        if (is_win64 || is_wow64) set_env_var( env, L"ProgramW6432", val );
        if (is_win64 || !is_wow64) set_env_var( env, L"ProgramFiles", val );
        RtlFreeHeap( GetProcessHeap(), 0, val );
    }
    if ((val = get_registry_value( *env, hkey, L"ProgramFilesDir (x86)" )))
    {
        if (is_win64 || is_wow64) set_env_var( env, L"ProgramFiles(x86)", val );
        if (is_wow64) set_env_var( env, L"ProgramFiles", val );
        RtlFreeHeap( GetProcessHeap(), 0, val );
    }

    /* set the CommonProgramFiles variables */

    if ((val = get_registry_value( *env, hkey, L"CommonFilesDir" )))
    {
        if (is_win64 || is_wow64) set_env_var( env, L"CommonProgramW6432", val );
        if (is_win64 || !is_wow64) set_env_var( env, L"CommonProgramFiles", val );
        RtlFreeHeap( GetProcessHeap(), 0, val );
    }
    if ((val = get_registry_value( *env, hkey, L"CommonFilesDir (x86)" )))
    {
        if (is_win64 || is_wow64) set_env_var( env, L"CommonProgramFiles(x86)", val );
        if (is_wow64) set_env_var( env, L"CommonProgramFiles", val );
        RtlFreeHeap( GetProcessHeap(), 0, val );
    }
    NtClose( hkey );
}


/***********************************************************************
 *           build_initial_environment
 *
 * Build the Win32 environment from the Unix environment
 */
static WCHAR *build_initial_environment( WCHAR **wargv[] )
{
    SIZE_T size = 1024;
    WCHAR *ptr;

    for (;;)
    {
        if (!(ptr = RtlAllocateHeap( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) return NULL;
        if (!unix_funcs->get_initial_environment( wargv, ptr, &size )) break;
        RtlFreeHeap( GetProcessHeap(), 0, ptr );
    }
    first_prefix_start = set_registry_environment( &ptr, TRUE );
    set_additional_environment( &ptr );
    return ptr;
}


/***********************************************************************
 *           get_current_directory
 *
 * Initialize the current directory from the Unix cwd.
 */
static void get_current_directory( UNICODE_STRING *dir )
{
    unix_funcs->get_initial_directory( dir );

    if (!dir->Length)  /* still not initialized */
    {
        dir->Length = wcslen( windows_dir ) * sizeof(WCHAR);
        memcpy( dir->Buffer, windows_dir, dir->Length );
    }
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
    DWORD len = wcslen( prefix );

    if (wcsnicmp( path, prefix, len )) return FALSE;
    while (path[len] == '\\') len++;
    return path + len == file;
}


/***********************************************************************
 *           get_image_path
 */
static void get_image_path( const WCHAR *name, UNICODE_STRING *path )
{
    WCHAR *load_path, *file_part, full_name[MAX_PATH];
    DWORD len;

    if (RtlDetermineDosPathNameType_U( name ) != RELATIVE_PATH ||
        wcschr( name, '/' ) || wcschr( name, '\\' ))
    {
        len = RtlGetFullPathName_U( name, sizeof(full_name), full_name, &file_part );
        if (!len || len > sizeof(full_name)) goto failed;
        /* try first without extension */
        if (RtlDoesFileExists_U( full_name )) goto done;
        if (len < (MAX_PATH - 4) * sizeof(WCHAR) && !wcschr( file_part, '.' ))
        {
            wcscat( file_part, L".exe" );
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
        len = RtlDosSearchPath_U( load_path, name, L".exe", sizeof(full_name), full_name, &file_part );
        RtlReleasePath( load_path );
        if (!len || len > sizeof(full_name))
        {
            /* build builtin path inside system directory */
            len = wcslen( system_dir );
            if (wcslen( name ) >= MAX_PATH - 4 - len) goto failed;
            wcscpy( full_name, system_dir );
            wcscat( full_name, name );
            if (!wcschr( name, '.' )) wcscat( full_name, L".exe" );
        }
    }
done:
    RtlCreateUnicodeString( path, full_name );
    return;

failed:
    MESSAGE( "wine: cannot find %s\n", debugstr_w(name) );
    RtlExitUserProcess( STATUS_DLL_NOT_FOUND );
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
    for (arg = argv; *arg; arg++) len += 3 + 2 * wcslen( *arg );
    if (!(cmdline->Buffer = RtlAllocateHeap( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return;

    p = cmdline->Buffer;
    for (arg = argv; *arg; arg++)
    {
        BOOL has_space, has_quote;
        int i, bcount;
        WCHAR *a;

        /* check for quotes and spaces in this argument */
        if (arg == argv || !**arg) has_space = TRUE;
        else has_space = wcschr( *arg, ' ' ) || wcschr( *arg, '\t' );
        has_quote = wcschr( *arg, '"' ) != NULL;

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
            wcscpy( p, *arg );
            p += wcslen( p );
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
    if (p - cmdline->Buffer >= 32767)
    {
        ERR( "command line too long (%u)\n", (DWORD)(p - cmdline->Buffer) );
        NtTerminateProcess( GetCurrentProcess(), 1 );
    }
    cmdline->Length = (p - cmdline->Buffer) * sizeof(WCHAR);
    cmdline->MaximumLength = cmdline->Length + sizeof(WCHAR);
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
    while (*var)
    {
        /* match var names, but avoid setting a var with a name including a '='
         * (a starting '=' is valid though)
         */
        unsigned int len = wcslen( var );
        if (len > namelen &&
            var[namelen] == '=' &&
            !RtlCompareUnicodeStrings( var, namelen, name, namelen, TRUE ) &&
            wcschr(var + 1, '=') == var + namelen)
        {
            return var + namelen + 1;
        }
        var += len + 1;
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
        value->Length = wcslen(var) * sizeof(WCHAR);

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
 *		RtlQueryEnvironmentVariable   [NTDLL.@]
 */
NTSTATUS WINAPI RtlQueryEnvironmentVariable( WCHAR *env, const WCHAR *name, SIZE_T namelen,
                                             WCHAR *value, SIZE_T value_length, SIZE_T *return_length )
{
    NTSTATUS nts = STATUS_VARIABLE_NOT_FOUND;
    SIZE_T len = 0;
    const WCHAR *var;

    if (!namelen) return nts;

    if (!env)
    {
        RtlAcquirePebLock();
        var = NtCurrentTeb()->Peb->ProcessParameters->Environment;
    }
    else var = env;

    var = ENV_FindVariable(var, name, namelen);
    if (var != NULL)
    {
        len = wcslen(var);
        if (len <= value_length)
        {
            memcpy(value, var, min(len + 1, value_length) * sizeof(WCHAR));
            nts = STATUS_SUCCESS;
        }
        else
        {
            len++;
            nts = STATUS_BUFFER_TOO_SMALL;
        }
    }
    *return_length = len;

    if (!env) RtlReleasePebLock();

    return nts;
}

/******************************************************************
 *		RtlSetCurrentEnvironment        [NTDLL.@]
 *
 */
void WINAPI RtlSetCurrentEnvironment(PWSTR new_env, PWSTR* old_env)
{
    WCHAR *prev;

    TRACE("(%p %p)\n", new_env, old_env);

    RtlAcquirePebLock();

    prev = NtCurrentTeb()->Peb->ProcessParameters->Environment;
    NtCurrentTeb()->Peb->ProcessParameters->Environment = new_env;

    RtlReleasePebLock();

    if (old_env)
        *old_env = prev;
    else
        RtlDestroyEnvironment( prev );
}


/******************************************************************************
 *  RtlSetEnvironmentVariable		[NTDLL.@]
 */
NTSTATUS WINAPI RtlSetEnvironmentVariable(PWSTR* penv, PUNICODE_STRING name, 
                                          PUNICODE_STRING value)
{
    INT varlen, len, old_size;
    LPWSTR      p, env;
    NTSTATUS    nts = STATUS_SUCCESS;

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
    for (p = env; *p; p += varlen + 1)
    {
        varlen = wcslen(p);
        if (varlen > len && p[len] == '=' &&
            !RtlCompareUnicodeStrings( name->Buffer, len, p, len, TRUE )) break;
    }
    if (!value && !*p) goto done;  /* Value to remove doesn't exist */

    /* Realloc the buffer */
    len = value ? len + value->Length / sizeof(WCHAR) + 2 : 0;
    if (*p) len -= wcslen(p) + 1;  /* The name already exists */

    if (len < 0)
    {
        LPWSTR next = p + wcslen(p) + 1;  /* We know there is a next one */
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
    LPCWSTR env, var;

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
            for (len = 0; len < src_len; len++) if (src[len] == '%') break;
            var = src;
            src += len;
            src_len -= len;
        }
        else  /* we are at the start of a variable */
        {
            for (len = 1; len < src_len; len++) if (src[len] == '%') break;
            if (len < src_len)
            {
                if ((var = ENV_FindVariable( env, src + 1, len - 1 )))
                {
                    src += len + 1;  /* Skip the variable name */
                    src_len -= len + 1;
                    len = wcslen(var);
                }
                else
                {
                    var = src;  /* Copy original name instead */
                    len++;
                    src += len;
                    src_len -= len;
                }
            }
            else  /* unfinished variable name, ignore it */
            {
                var = src;
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

    RtlInitUnicodeString( &nameW, L"\\KernelObjects\\__wineboot_event" );
    InitializeObjectAttributes( &attr, &nameW, OBJ_OPENIF, 0, NULL );

    status = NtCreateEvent( &handles[0], EVENT_ALL_ACCESS, &attr, NotificationEvent, 0 );
    if (status == STATUS_OBJECT_NAME_EXISTS) goto wait;
    if (status)
    {
        ERR( "failed to create wineboot event, expect trouble\n" );
        return;
    }
    LdrGetDllPath( L"C:\\windows\\system32\\wineboot.exe", LOAD_WITH_ALTERED_SEARCH_PATH,
                   &load_path, &dummy );
    RtlInitUnicodeString( &nameW, L"C:\\windows\\system32\\wineboot.exe" );
    RtlInitUnicodeString( &dllpathW, load_path );
    RtlInitUnicodeString( &cmdlineW, L"C:\\windows\\system32\\wineboot.exe --init" );
    RtlCreateProcessParametersEx( &params, &nameW, &dllpathW, NULL, &cmdlineW, *env, NULL, NULL,
                                  NULL, NULL, PROCESS_PARAMS_FLAG_NORMALIZED );
    params->hStdInput  = 0;
    params->hStdOutput = 0;
    params->hStdError  = NtCurrentTeb()->Peb->ProcessParameters->hStdError;

    RtlInitUnicodeString( &nameW, L"\\??\\C:\\windows\\system32\\wineboot.exe" );
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
void init_user_process_params(void)
{
    WCHAR *src, *load_path, *dummy;
    SIZE_T info_size, env_size, data_size = 0;
    startup_info_t *info = NULL;
    RTL_USER_PROCESS_PARAMETERS *params = NULL;
    UNICODE_STRING curdir, dllpath, imagepath, cmdline, title, desktop, shellinfo, runtime;
    WCHAR **wargv;

    unix_funcs->get_startup_info( NULL, &data_size, &info_size );
    if (!data_size)
    {
        RTL_USER_PROCESS_PARAMETERS initial_params = {0};
        WCHAR *env, curdir_buffer[MAX_PATH];

        NtCurrentTeb()->Peb->ProcessParameters = &initial_params;
        initial_params.Environment = build_initial_environment( &wargv );
        curdir.Buffer = curdir_buffer;
        curdir.MaximumLength = sizeof(curdir_buffer);
        get_current_directory( &curdir );
        initial_params.CurrentDirectory.DosPath = curdir;
        get_image_path( wargv[0], &initial_params.ImagePathName );
        wargv[0] = initial_params.ImagePathName.Buffer;
        build_command_line( wargv, &cmdline );
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
        RtlFreeUnicodeString( &initial_params.ImagePathName );
        RtlFreeUnicodeString( &cmdline );
        RtlReleasePath( load_path );

        unix_funcs->get_initial_console( params );
        params->wShowWindow = 1; /* SW_SHOWNORMAL */

        run_wineboot( &params->Environment );
        goto done;
    }

    if (!(info = RtlAllocateHeap( GetProcessHeap(), 0, data_size ))) return;

    if (unix_funcs->get_startup_info( info, &data_size, &info_size )) goto done;

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
    env_size = data_size - info_size;
    if ((params->Environment = RtlAllocateHeap( GetProcessHeap(), 0, max( env_size, sizeof(WCHAR) ))))
    {
        if (env_size) memcpy( params->Environment, (char *)info + info_size, env_size );
        else params->Environment[0] = 0;
    }

done:
    RtlFreeHeap( GetProcessHeap(), 0, info );
    if (RtlSetCurrentDirectory_U( &params->CurrentDirectory.DosPath ))
    {
        MESSAGE("wine: could not open working directory %s, starting in the Windows directory.\n",
                debugstr_w( params->CurrentDirectory.DosPath.Buffer ));
        RtlInitUnicodeString( &curdir, windows_dir );
        RtlSetCurrentDirectory_U( &curdir );
    }
    set_wow64_environment( &params->Environment );
}
