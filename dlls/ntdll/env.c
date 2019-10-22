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

static const WCHAR windows_dir[] = {'C',':','\\','w','i','n','d','o','w','s',0};

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
static void set_registry_environment( WCHAR **env )
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

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    /* first the system environment variables */
    RtlInitUnicodeString( &nameW, env_keyW );
    if (!NtOpenKey( &hkey, KEY_READ, &attr ))
    {
        set_registry_variables( env, hkey, REG_SZ );
        set_registry_variables( env, hkey, REG_EXPAND_SZ );
        NtClose( hkey );
    }

    /* then the ones for the current user */
    if (RtlOpenCurrentUser( KEY_READ, &attr.RootDirectory ) != STATUS_SUCCESS) return;
    RtlInitUnicodeString( &nameW, envW );
    if (!NtOpenKey( &hkey, KEY_READ, &attr ))
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
    UNICODE_STRING nameW, valW;
    WCHAR *val;
    HANDLE hkey;

    /* set the user profile variables */

    InitializeObjectAttributes( &attr, &nameW, 0, 0, NULL );
    RtlInitUnicodeString( &nameW, profile_keyW );
    if (!NtOpenKey( &hkey, KEY_READ, &attr ))
    {
        if ((val = get_registry_value( *env, hkey, programdataW )))
        {
            RtlInitUnicodeString( &valW, val );
            RtlInitUnicodeString( &nameW, allusersW );
            RtlSetEnvironmentVariable( env, &nameW, &valW );
            RtlInitUnicodeString( &nameW, programdataW );
            RtlSetEnvironmentVariable( env, &nameW, &valW );
            RtlFreeHeap( GetProcessHeap(), 0, val );
        }
        if ((val = get_registry_value( *env, hkey, public_valueW )))
        {
            RtlInitUnicodeString( &valW, val );
            RtlInitUnicodeString( &nameW, publicW );
            RtlSetEnvironmentVariable( env, &nameW, &valW );
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
            RtlInitUnicodeString( &valW, val );
            RtlInitUnicodeString( &nameW, computernameW );
            RtlSetEnvironmentVariable( env, &nameW, &valW );
            RtlFreeHeap( GetProcessHeap(), 0, val );
        }
        NtClose( hkey );
    }
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
    set_registry_environment( &ptr );
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
            dir->Length = nt_name.Length - 4 * sizeof(WCHAR);
            memcpy( dir->Buffer, nt_name.Buffer + 4, dir->Length );
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
 *           init_user_process_params
 *
 * Fill the initial RTL_USER_PROCESS_PARAMETERS structure from the server.
 */
void init_user_process_params( SIZE_T data_size )
{
    WCHAR *src;
    SIZE_T info_size, env_size;
    NTSTATUS status;
    startup_info_t *info = NULL;
    RTL_USER_PROCESS_PARAMETERS *params = NULL;
    UNICODE_STRING curdir, dllpath, imagepath, cmdline, title, desktop, shellinfo, runtime;

    if (!data_size)
    {
        if (RtlCreateProcessParametersEx( &params, &null_str, &null_str, &empty_str, &null_str, NULL,
                                          &null_str, &null_str, &null_str, &null_str,
                                          PROCESS_PARAMS_FLAG_NORMALIZED ))
            return;

        NtCurrentTeb()->Peb->ProcessParameters = params;
        params->Environment = build_initial_environment( __wine_get_main_environment() );
        get_current_directory( &params->CurrentDirectory.DosPath );

        if (isatty(0) || isatty(1) || isatty(2))
            params->ConsoleHandle = (HANDLE)2; /* see kernel32/kernel_private.h */
        if (!isatty(0))
            wine_server_fd_to_handle( 0, GENERIC_READ|SYNCHRONIZE,  OBJ_INHERIT, &params->hStdInput );
        if (!isatty(1))
            wine_server_fd_to_handle( 1, GENERIC_WRITE|SYNCHRONIZE, OBJ_INHERIT, &params->hStdOutput );
        if (!isatty(2))
            wine_server_fd_to_handle( 2, GENERIC_WRITE|SYNCHRONIZE, OBJ_INHERIT, &params->hStdError );
        params->wShowWindow = 1; /* SW_SHOWNORMAL */
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

done:
    RtlFreeHeap( GetProcessHeap(), 0, info );
    if (RtlSetCurrentDirectory_U( &params->CurrentDirectory.DosPath ))
    {
        MESSAGE("wine: could not open working directory %s, starting in the Windows directory.\n",
                debugstr_w( params->CurrentDirectory.DosPath.Buffer ));
        RtlInitUnicodeString( &curdir, windows_dir );
        RtlSetCurrentDirectory_U( &curdir );
    }
}


/***********************************************************************
 *           update_user_process_params
 *
 * Rebuild the RTL_USER_PROCESS_PARAMETERS structure once we have initialized all the fields.
 */
void update_user_process_params( const UNICODE_STRING *image )
{
    RTL_USER_PROCESS_PARAMETERS *params, *cur_params = NtCurrentTeb()->Peb->ProcessParameters;
    UNICODE_STRING title = cur_params->WindowTitle;
    WCHAR *env = cur_params->Environment;

    cur_params->Environment = NULL;  /* avoid copying it */
    if (!title.Buffer) title = *image;
    if (RtlCreateProcessParametersEx( &params, image, &cur_params->DllPath, NULL,
                                      &cur_params->CommandLine, NULL, &title, &cur_params->Desktop,
                                      &cur_params->ShellInfo, &cur_params->RuntimeInfo,
                                      PROCESS_PARAMS_FLAG_NORMALIZED ))
        return;

    params->DebugFlags      = cur_params->DebugFlags;
    params->ConsoleHandle   = cur_params->ConsoleHandle;
    params->ConsoleFlags    = cur_params->ConsoleFlags;
    params->hStdInput       = cur_params->hStdInput;
    params->hStdOutput      = cur_params->hStdOutput;
    params->hStdError       = cur_params->hStdError;
    params->dwX             = cur_params->dwX;
    params->dwY             = cur_params->dwY;
    params->dwXSize         = cur_params->dwXSize;
    params->dwYSize         = cur_params->dwYSize;
    params->dwXCountChars   = cur_params->dwXCountChars;
    params->dwYCountChars   = cur_params->dwYCountChars;
    params->dwFillAttribute = cur_params->dwFillAttribute;
    params->dwFlags         = cur_params->dwFlags;
    params->wShowWindow     = cur_params->wShowWindow;
    params->Environment     = env;

    RtlFreeHeap( GetProcessHeap(), 0, cur_params );
    NtCurrentTeb()->Peb->ProcessParameters = params;
}
