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

static UNICODE_STRING *get_machine_name( USHORT machine, UNICODE_STRING *str )
{
    switch (machine)
    {
    case IMAGE_FILE_MACHINE_I386:  RtlInitUnicodeString( str, L"x86" ); break;
    case IMAGE_FILE_MACHINE_AMD64: RtlInitUnicodeString( str, L"AMD64" ); break;
    case IMAGE_FILE_MACHINE_ARMNT: RtlInitUnicodeString( str, L"ARM" ); break;
    case IMAGE_FILE_MACHINE_ARM64: RtlInitUnicodeString( str, L"ARM64" ); break;
    default:                       RtlInitUnicodeString( str, L"Unknown" ); break;
    }
    return str;
}

/***********************************************************************
 *           set_wow64_environment
 *
 * Set the environment variables that change across 32/64/Wow64.
 */
static void set_wow64_environment( WCHAR **env )
{
    WCHAR buf[256];
    UNICODE_STRING arch_strW = RTL_CONSTANT_STRING( L"PROCESSOR_ARCHITECTURE" );
    UNICODE_STRING arch6432_strW = RTL_CONSTANT_STRING( L"PROCESSOR_ARCHITEW6432" );
    UNICODE_STRING valW = { 0, sizeof(buf), buf };
    UNICODE_STRING nameW;
    USHORT machine = RtlImageNtHeader( NtCurrentTeb()->Peb->ImageBaseAddress )->FileHeader.Machine;

    /* set the PROCESSOR_ARCHITECTURE variable */

    RtlSetEnvironmentVariable( env, &arch_strW, get_machine_name( current_machine, &nameW ));
    if (!is_win64 && NtCurrentTeb()->WowTebOffset)
    {
        RtlWow64GetProcessMachines( GetCurrentProcess(), NULL, &machine );
        RtlSetEnvironmentVariable( env, &arch6432_strW, get_machine_name( machine, &nameW ));
    }
    else RtlSetEnvironmentVariable( env, &arch6432_strW, NULL );

    /* set the ProgramFiles variables */

    RtlInitUnicodeString( &nameW, is_win64 ? L"ProgramW6432" : L"ProgramFiles(x86)" );
    if (!RtlQueryEnvironmentVariable_U( *env, &nameW, &valW ))
    {
        RtlInitUnicodeString( &nameW, L"ProgramFiles" );
        RtlSetEnvironmentVariable( env, &nameW, &valW );
    }

    /* set the CommonProgramFiles variables */

    RtlInitUnicodeString( &nameW, is_win64 ? L"CommonProgramW6432" : L"CommonProgramFiles(x86)" );
    if (!RtlQueryEnvironmentVariable_U( *env, &nameW, &valW ))
    {
        RtlInitUnicodeString( &nameW, L"CommonProgramFiles" );
        RtlSetEnvironmentVariable( env, &nameW, &valW );
    }

    RtlReAllocateHeap( GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY, *env,
                       get_env_length(*env) * sizeof(WCHAR) );
}


/******************************************************************************
 *  RtlAcquirePebLock		[NTDLL.@]
 */
void WINAPI RtlAcquirePebLock(void)
{
    RtlEnterCriticalSection( NtCurrentTeb()->Peb->FastPebLock );
}

/******************************************************************************
 *  RtlReleasePebLock		[NTDLL.@]
 */
void WINAPI RtlReleasePebLock(void)
{
    RtlLeaveCriticalSection( NtCurrentTeb()->Peb->FastPebLock );
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
    SIZE_T len, copy, total_size = 1;  /* 1 for terminating '\0' */
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
            copy = len;
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
                    copy = len;
                    if (count <= copy) /* Either copy the entire value, or nothing at all */
                        copy = 0;
                    if (count && dst)  /* When the variable is the last thing that fits into dst, the string is null terminated */
                       dst[copy] = 0;  /* Either right after, or if it doesn't fit, where it would start */
                }
                else
                {
                    var = src;  /* Copy original name instead */
                    len++;
                    src += len;
                    src_len -= len;
                    copy = len;
                }
            }
            else  /* unfinished variable name, ignore it */
            {
                var = src;
                src += len;
                src_len = 0;
                copy = len;
            }
        }
        total_size += len;
        if (count && dst)
        {
            if (count < len) len = count;
            if (count <= copy) copy = count - 1; /* If the buffer is too small, we copy one character less */
            memcpy(dst, var, copy * sizeof(WCHAR));
            count -= len;
            dst += copy;
        }
    }

    if (!renv) RtlReleasePebLock();

    if (count && dst) *dst = '\0';
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


#define ROUND_SIZE(size,align) (((size) + (align) - 1) & ~((align) - 1))

/* append a unicode string to the process params data; helper for RtlCreateProcessParameters */
static void append_unicode_string( void **data, const UNICODE_STRING *src,
                                   UNICODE_STRING *dst, size_t align )
{
    dst->Length = src->Length;
    dst->MaximumLength = src->MaximumLength;
    if (dst->MaximumLength)
    {
        dst->Buffer = *data;
        memcpy( dst->Buffer, src->Buffer, dst->Length );
        *data = (char *)dst->Buffer + ROUND_SIZE( dst->MaximumLength, align );
    }
    else dst->Buffer = NULL;
}

static RTL_USER_PROCESS_PARAMETERS *alloc_process_params( size_t align,
                                                          const UNICODE_STRING *image,
                                                          const UNICODE_STRING *dllpath,
                                                          const UNICODE_STRING *curdir,
                                                          const UNICODE_STRING *cmdline,
                                                          const WCHAR *env,
                                                          const UNICODE_STRING *title,
                                                          const UNICODE_STRING *desktop,
                                                          const UNICODE_STRING *shell,
                                                          const UNICODE_STRING *runtime )
{
    RTL_USER_PROCESS_PARAMETERS *params;
    SIZE_T size, env_size = 0;
    void *ptr;

    if (env) env_size = get_env_length( env ) * sizeof(WCHAR);

    size = (sizeof(RTL_USER_PROCESS_PARAMETERS)
            + ROUND_SIZE( image->MaximumLength, align )
            + ROUND_SIZE( dllpath->MaximumLength, align )
            + ROUND_SIZE( curdir->MaximumLength, align )
            + ROUND_SIZE( cmdline->MaximumLength, align )
            + ROUND_SIZE( title->MaximumLength, align )
            + ROUND_SIZE( desktop->MaximumLength, align )
            + ROUND_SIZE( shell->MaximumLength, align )
            + ROUND_SIZE( runtime->MaximumLength, align ));

    if (!(ptr = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, size + ROUND_SIZE( env_size, align ))))
        return NULL;

    params = ptr;
    params->AllocationSize  = size;
    params->Size            = size;
    params->Flags           = PROCESS_PARAMS_FLAG_NORMALIZED;
    params->EnvironmentSize = ROUND_SIZE( env_size, align );
    /* all other fields are zero */

    ptr = params + 1;
    append_unicode_string( &ptr, curdir, &params->CurrentDirectory.DosPath, align );
    append_unicode_string( &ptr, dllpath, &params->DllPath, align );
    append_unicode_string( &ptr, image, &params->ImagePathName, align );
    append_unicode_string( &ptr, cmdline, &params->CommandLine, align );
    append_unicode_string( &ptr, title, &params->WindowTitle, align );
    append_unicode_string( &ptr, desktop, &params->Desktop, align );
    append_unicode_string( &ptr, shell, &params->ShellInfo, align );
    append_unicode_string( &ptr, runtime, &params->RuntimeInfo, align );
    if (env) params->Environment = memcpy( ptr, env, env_size );
    return params;
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
    NTSTATUS status = STATUS_SUCCESS;

    RtlAcquirePebLock();
    cur_params = NtCurrentTeb()->Peb->ProcessParameters;
    if (!DllPath) DllPath = &null_str;
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

    if ((*result = alloc_process_params( sizeof(void *), ImagePathName, DllPath, &curdir, CommandLine,
                                         Environment, WindowTitle, Desktop, ShellInfo, RuntimeInfo )))
    {
        if (cur_params) (*result)->ConsoleFlags = cur_params->ConsoleFlags;
        if (!(flags & PROCESS_PARAMS_FLAG_NORMALIZED)) RtlDeNormalizeProcessParams( *result );
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
    WCHAR *env;
    SIZE_T size = 0, env_size;
    RTL_USER_PROCESS_PARAMETERS *new_params, *params = NtCurrentTeb()->Peb->ProcessParameters;
    UNICODE_STRING curdir;

    /* environment needs to be a separate memory block */
    env_size = params->EnvironmentSize;
    if ((env = RtlAllocateHeap( GetProcessHeap(), 0, max( env_size, sizeof(WCHAR) ))))
    {
        if (env_size) memcpy( env, params->Environment, env_size );
        else env[0] = 0;
    }

    if (!(new_params = alloc_process_params( 1, &params->ImagePathName, &params->DllPath,
                                             &params->CurrentDirectory.DosPath, &params->CommandLine,
                                             NULL, &params->WindowTitle, &params->Desktop,
                                             &params->ShellInfo, &params->RuntimeInfo )))
        return;

    new_params->Environment     = env;
    new_params->DebugFlags      = params->DebugFlags;
    new_params->ConsoleHandle   = params->ConsoleHandle;
    new_params->ConsoleFlags    = params->ConsoleFlags;
    new_params->hStdInput       = params->hStdInput;
    new_params->hStdOutput      = params->hStdOutput;
    new_params->hStdError       = params->hStdError;
    new_params->dwX             = params->dwX;
    new_params->dwY             = params->dwY;
    new_params->dwXSize         = params->dwXSize;
    new_params->dwYSize         = params->dwYSize;
    new_params->dwXCountChars   = params->dwXCountChars;
    new_params->dwYCountChars   = params->dwYCountChars;
    new_params->dwFillAttribute = params->dwFillAttribute;
    new_params->dwFlags         = params->dwFlags;
    new_params->wShowWindow     = params->wShowWindow;
    new_params->ProcessGroupId  = params->ProcessGroupId;

    NtCurrentTeb()->Peb->ProcessParameters = new_params;
    NtFreeVirtualMemory( GetCurrentProcess(), (void **)&params, &size, MEM_RELEASE );

    if (RtlSetCurrentDirectory_U( &new_params->CurrentDirectory.DosPath ))
    {
        MESSAGE("wine: could not open working directory %s, starting in the Windows directory.\n",
                debugstr_w( new_params->CurrentDirectory.DosPath.Buffer ));
        RtlInitUnicodeString( &curdir, windows_dir );
        RtlSetCurrentDirectory_U( &curdir );
    }
    set_wow64_environment( &new_params->Environment );
    new_params->EnvironmentSize = RtlSizeHeap( GetProcessHeap(), 0, new_params->Environment );
}
