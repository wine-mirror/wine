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
#include <stdarg.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "ntdll_misc.h"
#include "winnt.h"

WINE_DEFAULT_DEBUG_CHANNEL(environ);

static WCHAR empty[] = {0};
static const UNICODE_STRING empty_str = { 0, sizeof(empty), empty };
static const UNICODE_STRING null_str = { 0, 0, NULL };

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
    NTSTATUS    nts;

    TRACE("(%u,%p)!\n", inherit, env);

    if (inherit)
    {
        MEMORY_BASIC_INFORMATION        mbi;

        RtlAcquirePebLock();

        nts = NtQueryVirtualMemory(NtCurrentProcess(),
                                   NtCurrentTeb()->Peb->ProcessParameters->Environment,
                                   0, &mbi, sizeof(mbi), NULL);
        if (nts == STATUS_SUCCESS)
        {
            *env = NULL;
            nts = NtAllocateVirtualMemory(NtCurrentProcess(), (void**)env, 0, &mbi.RegionSize, 
                                          MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (nts == STATUS_SUCCESS)
                memcpy(*env, NtCurrentTeb()->Peb->ProcessParameters->Environment, mbi.RegionSize);
            else *env = NULL;
        }
        RtlReleasePebLock();
    }
    else 
    {
        SIZE_T      size = 1;
        PVOID       addr = NULL;
        nts = NtAllocateVirtualMemory(NtCurrentProcess(), &addr, 0, &size,
                                      MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (nts == STATUS_SUCCESS) *env = addr;
    }

    return nts;
}

/******************************************************************************
 *  RtlDestroyEnvironment		[NTDLL.@]
 */
NTSTATUS WINAPI RtlDestroyEnvironment(PWSTR env) 
{
    SIZE_T size = 0;

    TRACE("(%p)!\n", env);

    return NtFreeVirtualMemory(NtCurrentProcess(), (void**)&env, &size, MEM_RELEASE);
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
    MEMORY_BASIC_INFORMATION mbi;

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

    /* compute current size of environment */
    for (p = env; *p; p += strlenW(p) + 1);
    old_size = p + 1 - env;

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

    nts = NtQueryVirtualMemory(NtCurrentProcess(), env, 0,
                               &mbi, sizeof(mbi), NULL);
    if (nts != STATUS_SUCCESS) goto done;

    if ((old_size + len) * sizeof(WCHAR) > mbi.RegionSize)
    {
        LPWSTR  new_env;
        SIZE_T  new_size = (old_size + len) * sizeof(WCHAR);

        new_env = NULL;
        nts = NtAllocateVirtualMemory(NtCurrentProcess(), (void**)&new_env, 0,
                                      &new_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (nts != STATUS_SUCCESS) goto done;

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
    const WCHAR *env;
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

    if (Environment)
    {
        env = Environment;
        while (*env) env += strlenW(env) + 1;
        env++;
        env_size = ROUND_SIZE( (env - Environment) * sizeof(WCHAR) );
    }

    size = (sizeof(RTL_USER_PROCESS_PARAMETERS)
            + ROUND_SIZE( ImagePathName->MaximumLength )
            + ROUND_SIZE( DllPath->MaximumLength )
            + ROUND_SIZE( curdir.MaximumLength )
            + ROUND_SIZE( CommandLine->MaximumLength )
            + ROUND_SIZE( WindowTitle->MaximumLength )
            + ROUND_SIZE( Desktop->MaximumLength )
            + ROUND_SIZE( ShellInfo->MaximumLength )
            + ROUND_SIZE( RuntimeInfo->MaximumLength ));

    if ((ptr = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, size + env_size )))
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
        if (Environment)
            params->Environment = memcpy( ptr, Environment, (env - Environment) * sizeof(WCHAR) );
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
    void *ptr;
    WCHAR *src;
    SIZE_T info_size, env_size, alloc_size;
    NTSTATUS status;
    startup_info_t *info;
    RTL_USER_PROCESS_PARAMETERS *params = NULL;
    UNICODE_STRING curdir, dllpath, imagepath, cmdline, title, desktop, shellinfo, runtime;

    if (!data_size)
    {
        if (RtlCreateProcessParametersEx( &params, &null_str, &null_str, &empty_str, &null_str, NULL,
                                          &null_str, &null_str, &null_str, &null_str,
                                          PROCESS_PARAMS_FLAG_NORMALIZED ))
            return;

        NtCurrentTeb()->Peb->ProcessParameters = params;
        if (isatty(0) || isatty(1) || isatty(2))
            params->ConsoleHandle = (HANDLE)2; /* see kernel32/kernel_private.h */
        if (!isatty(0))
            wine_server_fd_to_handle( 0, GENERIC_READ|SYNCHRONIZE,  OBJ_INHERIT, &params->hStdInput );
        if (!isatty(1))
            wine_server_fd_to_handle( 1, GENERIC_WRITE|SYNCHRONIZE, OBJ_INHERIT, &params->hStdOutput );
        if (!isatty(2))
            wine_server_fd_to_handle( 2, GENERIC_WRITE|SYNCHRONIZE, OBJ_INHERIT, &params->hStdError );
        params->wShowWindow = 1; /* SW_SHOWNORMAL */
        return;
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

    curdir.MaximumLength = MAX_PATH * sizeof(WCHAR);  /* current directory needs more space */
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
    ptr = NULL;
    alloc_size = max( 1, env_size );
    status = NtAllocateVirtualMemory( NtCurrentProcess(), &ptr, 0, &alloc_size,
                                      MEM_COMMIT, PAGE_READWRITE );
    if (status != STATUS_SUCCESS) goto done;
    memcpy( ptr, (char *)info + info_size, env_size );
    params->Environment = ptr;

done:
    RtlFreeHeap( GetProcessHeap(), 0, info );
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
