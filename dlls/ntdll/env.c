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
    WCHAR *val;

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
static void get_image_path( const WCHAR *name, WCHAR *full_name, UINT size )
{
    WCHAR *load_path, *file_part;
    DWORD len;

    if (RtlDetermineDosPathNameType_U( name ) != RELATIVE_PATH ||
        wcschr( name, '/' ) || wcschr( name, '\\' ))
    {
        len = RtlGetFullPathName_U( name, size, full_name, &file_part );
        if (!len || len > size) goto failed;
        /* try first without extension */
        if (RtlDoesFileExists_U( full_name )) return;
        if (len < size - 4 * sizeof(WCHAR) && !wcschr( file_part, '.' ))
        {
            wcscat( file_part, L".exe" );
            if (RtlDoesFileExists_U( full_name )) return;
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
        len = RtlDosSearchPath_U( load_path, name, L".exe", size, full_name, &file_part );
        RtlReleasePath( load_path );
        if (!len || len > size)
        {
            /* build builtin path inside system directory */
            len = wcslen( system_dir );
            if (wcslen( name ) >= size/sizeof(WCHAR) - 4 - len) goto failed;
            wcscpy( full_name, system_dir );
            wcscat( full_name, name );
            if (!wcschr( name, '.' )) wcscat( full_name, L".exe" );
        }
    }
    return;

failed:
    MESSAGE( "wine: cannot find %s\n", debugstr_w(name) );
    RtlExitUserProcess( STATUS_DLL_NOT_FOUND );
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
    NtCurrentTeb()->Peb->ProcessParameters->EnvironmentSize = RtlSizeHeap( GetProcessHeap(), 0, new_env );

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
        SIZE_T new_size = (old_size + len) * sizeof(WCHAR);
        LPWSTR new_env = RtlAllocateHeap( GetProcessHeap(), 0, new_size );

        if (!new_env)
        {
            nts = STATUS_NO_MEMORY;
            goto done;
        }
        memcpy(new_env, env, (p - env) * sizeof(WCHAR));
        memcpy(new_env + (p - env) + len, p, (old_size - (p - env)) * sizeof(WCHAR));
        p = new_env + (p - env);

        RtlDestroyEnvironment(env);
        if (!penv)
        {
            NtCurrentTeb()->Peb->ProcessParameters->Environment = new_env;
            NtCurrentTeb()->Peb->ProcessParameters->EnvironmentSize = new_size;
        }
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

    if ((ptr = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, size + ROUND_SIZE( env_size ) )))
    {
        RTL_USER_PROCESS_PARAMETERS *params = ptr;
        params->AllocationSize  = size;
        params->Size            = size;
        params->Flags           = PROCESS_PARAMS_FLAG_NORMALIZED;
        params->EnvironmentSize = ROUND_SIZE( env_size );
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


/***********************************************************************
 *           init_user_process_params
 *
 * Fill the initial RTL_USER_PROCESS_PARAMETERS structure from the server.
 */
void init_user_process_params(void)
{
    WCHAR *env, *load_path, *dummy, image[MAX_PATH];
    SIZE_T env_size;
    RTL_USER_PROCESS_PARAMETERS *new_params, *params = NtCurrentTeb()->Peb->ProcessParameters;
    UNICODE_STRING curdir, dllpath, cmdline;

    /* environment needs to be a separate memory block */
    env_size = params->EnvironmentSize;
    env = params->Environment;
    if ((env = RtlAllocateHeap( GetProcessHeap(), 0, max( env_size, sizeof(WCHAR) ))))
    {
        if (env_size) memcpy( env, params->Environment, env_size );
        else env[0] = 0;
        params->Environment = env;
    }

    if (!params->DllPath.MaximumLength)  /* not inherited from parent process */
    {
        set_additional_environment( &params->Environment );

        get_image_path( params->ImagePathName.Buffer, image, sizeof(image) );
        RtlInitUnicodeString( &params->ImagePathName, image );

        cmdline.Length = params->ImagePathName.Length + params->CommandLine.MaximumLength + 3 * sizeof(WCHAR);
        cmdline.MaximumLength = cmdline.Length + sizeof(WCHAR);
        cmdline.Buffer = RtlAllocateHeap( GetProcessHeap(), 0, cmdline.MaximumLength );
        swprintf( cmdline.Buffer, cmdline.MaximumLength / sizeof(WCHAR),
                  L"\"%s\" %s", params->ImagePathName.Buffer, params->CommandLine.Buffer );

        LdrGetDllPath( params->ImagePathName.Buffer, 0, &load_path, &dummy );
        RtlInitUnicodeString( &dllpath, load_path );

        env = params->Environment;
        params->Environment = NULL;  /* avoid copying it */
        if (RtlCreateProcessParametersEx( &new_params, &params->ImagePathName, &dllpath,
                                          &params->CurrentDirectory.DosPath,
                                          &cmdline, NULL, &params->ImagePathName, NULL, NULL, NULL,
                                          PROCESS_PARAMS_FLAG_NORMALIZED ))
            return;

        new_params->Environment   = env;
        new_params->hStdInput     = params->hStdInput;
        new_params->hStdOutput    = params->hStdOutput;
        new_params->hStdError     = params->hStdError;
        new_params->ConsoleHandle = params->ConsoleHandle;
        new_params->dwXCountChars = params->dwXCountChars;
        new_params->dwYCountChars = params->dwYCountChars;
        new_params->wShowWindow   = params->wShowWindow;
        NtCurrentTeb()->Peb->ProcessParameters = params = new_params;

        RtlFreeUnicodeString( &cmdline );
        RtlReleasePath( load_path );
    }

    if (RtlSetCurrentDirectory_U( &params->CurrentDirectory.DosPath ))
    {
        MESSAGE("wine: could not open working directory %s, starting in the Windows directory.\n",
                debugstr_w( params->CurrentDirectory.DosPath.Buffer ));
        RtlInitUnicodeString( &curdir, windows_dir );
        RtlSetCurrentDirectory_U( &curdir );
    }
    set_wow64_environment( &params->Environment );
    params->EnvironmentSize = RtlSizeHeap( GetProcessHeap(), 0, params->Environment );
}
