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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "config.h"

#include <assert.h>

#include "winternl.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(environ);

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

        nts = NtQueryVirtualMemory(NtCurrentProcess(), ntdll_get_process_pmts()->Environment, 
                                   0, &mbi, sizeof(mbi), NULL);
        if (nts == STATUS_SUCCESS)
        {
            *env = NULL;
            nts = NtAllocateVirtualMemory(NtCurrentProcess(), (void**)env, 0, &mbi.RegionSize, 
                                          MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (nts == STATUS_SUCCESS)
                memcpy(*env, ntdll_get_process_pmts()->Environment, mbi.RegionSize);
            else *env = NULL;
        }
        RtlReleasePebLock();
    }
    else 
    {
        ULONG       size = 1;
        nts = NtAllocateVirtualMemory(NtCurrentProcess(), (void**)env, 0, &size, 
                                      MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (nts == STATUS_SUCCESS)
            memset(*env, 0, size);
    }

    return nts;
}

/******************************************************************************
 *  RtlDestroyEnvironment		[NTDLL.@]
 */
NTSTATUS WINAPI RtlDestroyEnvironment(PWSTR env) 
{
    ULONG size = 0;

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

    TRACE("%s %s %p\n", debugstr_w(env), debugstr_w(name->Buffer), value);

    value->Length = 0;
    namelen = name->Length / sizeof(WCHAR);
    if (!namelen) return nts;

    if (!env)
    {
        RtlAcquirePebLock();
        var = ntdll_get_process_pmts()->Environment;
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

    if (old_env) *old_env = ntdll_get_process_pmts()->Environment;
    ntdll_get_process_pmts()->Environment = new_env;

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

    TRACE("(%p,%s,%s): stub!\n", penv, debugstr_w(name->Buffer), debugstr_w(value->Buffer));

    if (!name || !name->Buffer || !name->Buffer[0])
        return STATUS_INVALID_PARAMETER_1;
    /* variable names can't contain a '=' except as a first character */
    if (strchrW(name->Buffer + 1, '=')) return STATUS_INVALID_PARAMETER;

    if (!penv)
    {
        RtlAcquirePebLock();
        env = ntdll_get_process_pmts()->Environment;
    } else env = *penv;

    len = name->Length / sizeof(WCHAR);

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
        ULONG   new_size = (old_size + len) * sizeof(WCHAR);

        new_env = NULL;
        nts = NtAllocateVirtualMemory(NtCurrentProcess(), (void**)&new_env, 0,
                                      &new_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (nts != STATUS_SUCCESS) goto done;

        memmove(new_env, env, (p - env) * sizeof(WCHAR));
        assert(len > 0);
        memmove(new_env + (p - env) + len, p, (old_size - (p - env)) * sizeof(WCHAR));
        p = new_env + (p - env);

        RtlDestroyEnvironment(env);
        if (!penv) ntdll_get_process_pmts()->Environment = new_env;
        else *penv = new_env;
        env = new_env;
    }
    else
    {
        if (len > 0) memmove(p + len, p, (old_size - (p - env)) * sizeof(WCHAR));
    }

    /* Set the new string */
    if (value)
    {
        static const WCHAR equalW[] = {'=',0};

        strcpyW(p, name->Buffer);
        strcatW(p, equalW);
        strcatW(p, value->Buffer);
    }

done:
    if (!penv) RtlReleasePebLock();

    return nts;
}

/******************************************************************
 *		RtlExpandEnvironmentStrings_U (NTDLL.@)
 *
 */
NTSTATUS WINAPI RtlExpandEnvironmentStrings_U(PWSTR env, const UNICODE_STRING* us_src,
                                              PUNICODE_STRING us_dst, PULONG plen)
{
    DWORD       len, count, total_size = 1;  /* 1 for terminating '\0' */
    LPCWSTR     src, p, var;
    LPWSTR      dst;

    src = us_src->Buffer;
    count = us_dst->MaximumLength / sizeof(WCHAR);
    dst = count ? us_dst->Buffer : NULL;

    RtlAcquirePebLock();

    while (*src)
    {
        if (*src != '%')
        {
            if ((p = strchrW( src, '%' ))) len = p - src;
            else len = strlenW(src);
            var = src;
            src += len;
        }
        else  /* we are at the start of a variable */
        {
            if ((p = strchrW( src + 1, '%' )))
            {
                len = p - src - 1;  /* Length of the variable name */
                if ((var = ENV_FindVariable( env, src + 1, len )))
                {
                    src += len + 2;  /* Skip the variable name */
                    len = strlenW(var);
                }
                else
                {
                    var = src;  /* Copy original name instead */
                    len += 2;
                    src += len;
                }
            }
            else  /* unfinished variable name, ignore it */
            {
                var = src;
                len = strlenW(src);  /* Copy whole string */
                src += len;
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

    RtlReleasePebLock();

    /* Null-terminate the string */
    if (dst && count) *dst = '\0';

    us_dst->Length = (dst) ? (dst - us_dst->Buffer) * sizeof(WCHAR): 0;
    if (plen) *plen = total_size * sizeof(WCHAR);

    return (count) ? STATUS_SUCCESS : STATUS_BUFFER_TOO_SMALL;
}

/***********************************************************************
 *           build_environment
 *
 * Build the Win32 environment from the Unix environment
 */
BOOL build_initial_environment(void)
{
    extern char **environ;
    LPSTR*      e, te;
    LPWSTR      p;
    ULONG       size;
    NTSTATUS    nts;
    int         len;

    /* Compute the total size of the Unix environment */
    size = sizeof(BYTE);
    for (e = environ; *e; e++)
    {
        if (!memcmp(*e, "PATH=", 5)) continue;
        size += strlen(*e) + 1;
    }
    size *= sizeof(WCHAR);

    /* Now allocate the environment */
    nts = NtAllocateVirtualMemory(NtCurrentProcess(), (void**)&p, 0, &size, 
                                  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (nts != STATUS_SUCCESS) return FALSE;

    ntdll_get_process_pmts()->Environment = p;
    /* And fill it with the Unix environment */
    for (e = environ; *e; e++)
    {
        /* skip Unix PATH and store WINEPATH as PATH */
        if (!memcmp(*e, "PATH=", 5)) continue;
        if (!memcmp(*e, "WINEPATH=", 9 )) te = *e + 4; else te = *e;
        len = strlen(te);
        RtlMultiByteToUnicodeN(p, len * sizeof(WCHAR), NULL, te, len);
        p[len] = 0;
        p += len + 1;
    }
    *p = 0;

    return TRUE;
}
