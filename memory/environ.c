/*
 * Process environment management
 *
 * Copyright 1996, 1998 Alexandre Julliard
 */

#include <stdlib.h>
#include <string.h>
#include "windef.h"
#include "wine/winestring.h"
#include "process.h"
#include "heap.h"
#include "selectors.h"
#include "winerror.h"

/* Format of an environment block:
 * ASCIIZ   string 1 (xx=yy format)
 * ...
 * ASCIIZ   string n
 * BYTE     0
 * WORD     1
 * ASCIIZ   program name (e.g. C:\WINDOWS\SYSTEM\KRNL386.EXE)
 *
 * Notes:
 * - contrary to Microsoft docs, the environment strings do not appear
 *   to be sorted on Win95 (although they are on NT); so we don't bother
 *   to sort them either.
 */

static const char ENV_program_name[] = "C:\\WINDOWS\\SYSTEM\\KRNL386.EXE";

/* Maximum length of a Win16 environment string (including NULL) */
#define MAX_WIN16_LEN  128

/* Extra bytes to reserve at the end of an environment */
#define EXTRA_ENV_SIZE (sizeof(BYTE) + sizeof(WORD) + sizeof(ENV_program_name))

/* Fill the extra bytes with the program name and stuff */
#define FILL_EXTRA_ENV(p) \
    *(p) = '\0'; \
    PUT_WORD( (p) + 1, 1 ); \
    strcpy( (p) + 3, ENV_program_name );

STARTUPINFOA current_startupinfo =
{
    sizeof(STARTUPINFOA),    /* cb */
    0,                       /* lpReserved */
    0,                       /* lpDesktop */
    0,                       /* lpTitle */
    0,                       /* dwX */
    0,                       /* dwY */
    0,                       /* dwXSize */
    0,                       /* dwYSize */
    0,                       /* dwXCountChars */
    0,                       /* dwYCountChars */
    0,                       /* dwFillAttribute */
    0,                       /* dwFlags */
    0,                       /* wShowWindow */
    0,                       /* cbReserved2 */
    0,                       /* lpReserved2 */
    0,                       /* hStdInput */
    0,                       /* hStdOutput */
    0                        /* hStdError */
};

ENVDB current_envdb =
{
    0,                       /* environ */
    0,                       /* unknown1 */
    0,                       /* cmd_line */
    0,                       /* cur_dir */
    &current_startupinfo,    /* startup_info */
    0,                       /* hStdin */
    0,                       /* hStdout */
    0,                       /* hStderr */
    0,                       /* unknown2 */
    0,                       /* inherit_console */
    0,                       /* break_type */
    0,                       /* break_sem */
    0,                       /* break_event */
    0,                       /* break_thread */
    0,                       /* break_handlers */
    CRITICAL_SECTION_INIT,   /* section */
    0,                       /* cmd_lineW */
    0                        /* env_sel */
};

/***********************************************************************
 *           ENV_FindVariable
 *
 * Find a variable in the environment and return a pointer to the value.
 * Helper function for GetEnvironmentVariable and ExpandEnvironmentStrings.
 */
static LPCSTR ENV_FindVariable( LPCSTR env, LPCSTR name, INT len )
{
    while (*env)
    {
        if (!lstrncmpiA( name, env, len ) && (env[len] == '='))
            return env + len + 1;
        env += strlen(env) + 1;
    }
    return NULL;
}


/***********************************************************************
 *           ENV_BuildEnvironment
 *
 * Build the environment for the initial process
 */
BOOL ENV_BuildEnvironment(void)
{
    extern char **environ;
    LPSTR p, *e;
    int size;

    /* Compute the total size of the Unix environment */

    size = EXTRA_ENV_SIZE;
    for (e = environ; *e; e++) size += strlen(*e) + 1;

    /* Now allocate the environment */

    if (!(p = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;
    current_envdb.environ = p;
    current_envdb.env_sel = SELECTOR_AllocBlock( p, 0x10000, SEGMENT_DATA, FALSE, FALSE );

    /* And fill it with the Unix environment */

    for (e = environ; *e; e++)
    {
        strcpy( p, *e );
        p += strlen(p) + 1;
    }

    /* Now add the program name */

    FILL_EXTRA_ENV( p );
    return TRUE;
}


/***********************************************************************
 *           GetCommandLineA      (KERNEL32.289)
 */
LPSTR WINAPI GetCommandLineA(void)
{
    return current_envdb.cmd_line;
}

/***********************************************************************
 *           GetCommandLineW      (KERNEL32.290)
 */
LPWSTR WINAPI GetCommandLineW(void)
{
    EnterCriticalSection( &current_envdb.section );
    if (!current_envdb.cmd_lineW)
        current_envdb.cmd_lineW = HEAP_strdupAtoW( GetProcessHeap(), 0,
                                                  current_envdb.cmd_line );
    LeaveCriticalSection( &current_envdb.section );
    return current_envdb.cmd_lineW;
}


/***********************************************************************
 *           GetEnvironmentStringsA   (KERNEL32.319) (KERNEL32.320)
 */
LPSTR WINAPI GetEnvironmentStringsA(void)
{
    return current_envdb.environ;
}


/***********************************************************************
 *           GetEnvironmentStringsW   (KERNEL32.321)
 */
LPWSTR WINAPI GetEnvironmentStringsW(void)
{
    INT size;
    LPWSTR ret;

    EnterCriticalSection( &current_envdb.section );
    size = HeapSize( GetProcessHeap(), 0, current_envdb.environ );
    if ((ret = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) )) != NULL)
    {
        LPSTR pA = current_envdb.environ;
        LPWSTR pW = ret;
        while (size--) *pW++ = (WCHAR)(BYTE)*pA++;
    }
    LeaveCriticalSection( &current_envdb.section );
    return ret;
}


/***********************************************************************
 *           FreeEnvironmentStringsA   (KERNEL32.268)
 */
BOOL WINAPI FreeEnvironmentStringsA( LPSTR ptr )
{
    if (ptr != current_envdb.environ)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           FreeEnvironmentStringsW   (KERNEL32.269)
 */
BOOL WINAPI FreeEnvironmentStringsW( LPWSTR ptr )
{
    return HeapFree( GetProcessHeap(), 0, ptr );
}


/***********************************************************************
 *           GetEnvironmentVariableA   (KERNEL32.322)
 */
DWORD WINAPI GetEnvironmentVariableA( LPCSTR name, LPSTR value, DWORD size )
{
    LPCSTR p;
    INT ret = 0;

    if (!name || !*name)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    EnterCriticalSection( &current_envdb.section );
    if ((p = ENV_FindVariable( current_envdb.environ, name, strlen(name) )))
    {
        ret = strlen(p);
        if (size <= ret)
        {
            /* If not enough room, include the terminating null
             * in the returned size and return an empty string */
            ret++;
            if (value) *value = '\0';
        }
        else if (value) strcpy( value, p );
    }
    LeaveCriticalSection( &current_envdb.section );
    return ret;  /* FIXME: SetLastError */
}


/***********************************************************************
 *           GetEnvironmentVariableW   (KERNEL32.323)
 */
DWORD WINAPI GetEnvironmentVariableW( LPCWSTR nameW, LPWSTR valW, DWORD size)
{
    LPSTR name = HEAP_strdupWtoA( GetProcessHeap(), 0, nameW );
    LPSTR val  = valW ? HeapAlloc( GetProcessHeap(), 0, size ) : NULL;
    DWORD res  = GetEnvironmentVariableA( name, val, size );
    HeapFree( GetProcessHeap(), 0, name );
    if (val)
    {
        lstrcpynAtoW( valW, val, size );
        HeapFree( GetProcessHeap(), 0, val );
    }
    return res;
}


/***********************************************************************
 *           SetEnvironmentVariableA   (KERNEL32.641)
 */
BOOL WINAPI SetEnvironmentVariableA( LPCSTR name, LPCSTR value )
{
    INT old_size, len, res;
    LPSTR p, env, new_env;
    BOOL ret = FALSE;

    EnterCriticalSection( &current_envdb.section );
    env = p = current_envdb.environ;

    /* Find a place to insert the string */

    res = -1;
    len = strlen(name);
    while (*p)
    {
        if (!lstrncmpiA( name, p, len ) && (p[len] == '=')) break;
        p += strlen(p) + 1;
    }
    if (!value && !*p) goto done;  /* Value to remove doesn't exist */

    /* Realloc the buffer */

    len = value ? strlen(name) + strlen(value) + 2 : 0;
    if (*p) len -= strlen(p) + 1;  /* The name already exists */
    old_size = HeapSize( GetProcessHeap(), 0, env );
    if (len < 0)
    {
        LPSTR next = p + strlen(p) + 1;  /* We know there is a next one */
        memmove( next + len, next, old_size - (next - env) );
    }
    if (!(new_env = HeapReAlloc( GetProcessHeap(), 0, env, old_size + len )))
        goto done;
    if (current_envdb.env_sel)
        SELECTOR_MoveBlock( current_envdb.env_sel, new_env );
    p = new_env + (p - env);
    if (len > 0) memmove( p + len, p, old_size - (p - new_env) );

    /* Set the new string */

    if (value)
    {
        strcpy( p, name );
        strcat( p, "=" );
        strcat( p, value );
    }
    current_envdb.environ = new_env;
    ret = TRUE;

done:
    LeaveCriticalSection( &current_envdb.section );
    return ret;
}


/***********************************************************************
 *           SetEnvironmentVariableW   (KERNEL32.642)
 */
BOOL WINAPI SetEnvironmentVariableW( LPCWSTR name, LPCWSTR value )
{
    LPSTR nameA  = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    LPSTR valueA = HEAP_strdupWtoA( GetProcessHeap(), 0, value );
    BOOL ret = SetEnvironmentVariableA( nameA, valueA );
    HeapFree( GetProcessHeap(), 0, nameA );
    HeapFree( GetProcessHeap(), 0, valueA );
    return ret;
}


/***********************************************************************
 *           ExpandEnvironmentStringsA   (KERNEL32.216)
 *
 * Note: overlapping buffers are not supported; this is how it should be.
 */
DWORD WINAPI ExpandEnvironmentStringsA( LPCSTR src, LPSTR dst, DWORD count )
{
    DWORD len, total_size = 1;  /* 1 for terminating '\0' */
    LPCSTR p, var;

    if (!count) dst = NULL;
    EnterCriticalSection( &current_envdb.section );

    while (*src)
    {
        if (*src != '%')
        {
            if ((p = strchr( src, '%' ))) len = p - src;
            else len = strlen(src);
            var = src;
            src += len;
        }
        else  /* we are at the start of a variable */
        {
            if ((p = strchr( src + 1, '%' )))
            {
                len = p - src - 1;  /* Length of the variable name */
                if ((var = ENV_FindVariable( current_envdb.environ,
                                             src + 1, len )))
                {
                    src += len + 2;  /* Skip the variable name */
                    len = strlen(var);
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
                len = strlen(src);  /* Copy whole string */
                src += len;
            }
        }
        total_size += len;
        if (dst)
        {
            if (count < len) len = count;
            memcpy( dst, var, len );
            dst += len;
            count -= len;
        }
    }
    LeaveCriticalSection( &current_envdb.section );

    /* Null-terminate the string */
    if (dst)
    {
        if (!count) dst--;
        *dst = '\0';
    }
    return total_size;
}


/***********************************************************************
 *           ExpandEnvironmentStringsW   (KERNEL32.217)
 */
DWORD WINAPI ExpandEnvironmentStringsW( LPCWSTR src, LPWSTR dst, DWORD len )
{
    LPSTR srcA = HEAP_strdupWtoA( GetProcessHeap(), 0, src );
    LPSTR dstA = dst ? HeapAlloc( GetProcessHeap(), 0, len ) : NULL;
    DWORD ret  = ExpandEnvironmentStringsA( srcA, dstA, len );
    if (dstA)
    {
        lstrcpyAtoW( dst, dstA );
        HeapFree( GetProcessHeap(), 0, dstA );
    }
    HeapFree( GetProcessHeap(), 0, srcA );
    return ret;
}


/***********************************************************************
 *           GetStdHandle    (KERNEL32.276)
 */
HANDLE WINAPI GetStdHandle( DWORD std_handle )
{
    switch(std_handle)
    {
        case STD_INPUT_HANDLE:  return current_envdb.hStdin;
        case STD_OUTPUT_HANDLE: return current_envdb.hStdout;
        case STD_ERROR_HANDLE:  return current_envdb.hStderr;
    }
    SetLastError( ERROR_INVALID_PARAMETER );
    return INVALID_HANDLE_VALUE;
}


/***********************************************************************
 *           SetStdHandle    (KERNEL32.506)
 */
BOOL WINAPI SetStdHandle( DWORD std_handle, HANDLE handle )
{
    switch(std_handle)
    {
        case STD_INPUT_HANDLE:  current_envdb.hStdin = handle;  return TRUE;
        case STD_OUTPUT_HANDLE: current_envdb.hStdout = handle; return TRUE;
        case STD_ERROR_HANDLE:  current_envdb.hStderr = handle; return TRUE;
    }
    SetLastError( ERROR_INVALID_PARAMETER );
    return FALSE;
}


/***********************************************************************
 *              GetStartupInfoA         (KERNEL32.273)
 */
VOID WINAPI GetStartupInfoA( LPSTARTUPINFOA info )
{
    *info = current_startupinfo;
}


/***********************************************************************
 *              GetStartupInfoW         (KERNEL32.274)
 */
VOID WINAPI GetStartupInfoW( LPSTARTUPINFOW info )
{
    info->cb              = sizeof(STARTUPINFOW);
    info->dwX             = current_startupinfo.dwX;
    info->dwY             = current_startupinfo.dwY;
    info->dwXSize         = current_startupinfo.dwXSize;
    info->dwXCountChars   = current_startupinfo.dwXCountChars;
    info->dwYCountChars   = current_startupinfo.dwYCountChars;
    info->dwFillAttribute = current_startupinfo.dwFillAttribute;
    info->dwFlags         = current_startupinfo.dwFlags;
    info->wShowWindow     = current_startupinfo.wShowWindow;
    info->cbReserved2     = current_startupinfo.cbReserved2;
    info->lpReserved2     = current_startupinfo.lpReserved2;
    info->hStdInput       = current_startupinfo.hStdInput;
    info->hStdOutput      = current_startupinfo.hStdOutput;
    info->hStdError       = current_startupinfo.hStdError;
    info->lpReserved = HEAP_strdupAtoW (GetProcessHeap(), 0, current_startupinfo.lpReserved );
    info->lpDesktop  = HEAP_strdupAtoW (GetProcessHeap(), 0, current_startupinfo.lpDesktop );
    info->lpTitle    = HEAP_strdupAtoW (GetProcessHeap(), 0, current_startupinfo.lpTitle );
}
