/*
 * Process environment management
 *
 * Copyright 1996, 1998 Alexandre Julliard
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
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winerror.h"

#include "wine/winbase16.h"
#include "wine/server.h"
#include "wine/library.h"
#include "heap.h"
#include "winternl.h"
#include "selectors.h"

/* Win32 process environment database */
typedef struct _ENVDB
{
    LPSTR            env;              /* 00 Process environment strings */
    DWORD            unknown1;         /* 04 Unknown */
    LPSTR            cmd_line;         /* 08 Command line */
    LPSTR            cur_dir;          /* 0c Current directory */
    STARTUPINFOA    *startup_info;     /* 10 Startup information */
    HANDLE           hStdin;           /* 14 Handle for standard input */
    HANDLE           hStdout;          /* 18 Handle for standard output */
    HANDLE           hStderr;          /* 1c Handle for standard error */
    DWORD            unknown2;         /* 20 Unknown */
    DWORD            inherit_console;  /* 24 Inherit console flag */
    DWORD            break_type;       /* 28 Console events flag */
    void            *break_sem;        /* 2c SetConsoleCtrlHandler semaphore */
    void            *break_event;      /* 30 SetConsoleCtrlHandler event */
    void            *break_thread;     /* 34 SetConsoleCtrlHandler thread */
    void            *break_handlers;   /* 38 List of console handlers */
} ENVDB;


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
    0                        /* break_handlers */
};


static WCHAR *cmdlineW;  /* Unicode command line */
static WORD env_sel;     /* selector to the environment */

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
        if (!strncasecmp( name, env, len ) && (env[len] == '='))
            return env + len + 1;
        env += strlen(env) + 1;
    }
    return NULL;
}


/***********************************************************************
 *           build_environment
 *
 * Build the environment for the initial process
 */
static BOOL build_environment(void)
{
    extern char **environ;
    static const WORD one = 1;
    LPSTR p, *e;
    int size;

    /* Compute the total size of the Unix environment */

    size = sizeof(BYTE) + sizeof(WORD) + sizeof(ENV_program_name);
    for (e = environ; *e; e++)
    {
        if (!memcmp( *e, "PATH=", 5 )) continue;
        size += strlen(*e) + 1;
    }

    /* Now allocate the environment */

    if (!(p = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;
    current_envdb.env = p;
    env_sel = SELECTOR_AllocBlock( p, 0x10000, WINE_LDT_FLAGS_DATA );

    /* And fill it with the Unix environment */

    for (e = environ; *e; e++)
    {
        /* skip Unix PATH and store WINEPATH as PATH */
        if (!memcmp( *e, "PATH=", 5 )) continue;
        if (!memcmp( *e, "WINEPATH=", 9 )) strcpy( p, *e + 4 );
        else strcpy( p, *e );
        p += strlen(p) + 1;
    }

    /* Now add the program name */

    *p++ = 0;
    memcpy( p, &one, sizeof(WORD) );
    strcpy( p + sizeof(WORD), ENV_program_name );
    return TRUE;
}


/***********************************************************************
 *           copy_str
 *
 * Small helper for ENV_InitStartupInfo.
 */
inline static char *copy_str( char **dst, const char **src, size_t len )
{
    char *ret;

    if (!len) return NULL;
    ret = *dst;
    memcpy( ret, *src, len );
    ret[len] = 0;
    *dst += len + 1;
    *src += len;
    return ret;
}


/***********************************************************************
 *           ENV_InitStartupInfo
 *
 * Fill the startup info structure from the server.
 */
ENVDB *ENV_InitStartupInfo( size_t info_size, char *main_exe_name, size_t main_exe_size )
{
    startup_info_t info;
    void *data;
    char *dst;
    const char *src;
    size_t len;

    if (!build_environment()) return NULL;
    if (!info_size) return &current_envdb;  /* nothing to retrieve */

    if (!(data = HeapAlloc( GetProcessHeap(), 0, info_size ))) return NULL;

    SERVER_START_REQ( get_startup_info )
    {
        wine_server_set_reply( req, data, info_size );
        wine_server_call( req );
        info_size = wine_server_reply_size( reply );
    }
    SERVER_END_REQ;
    if (info_size < sizeof(info.size)) goto done;
    len = min( info_size, ((startup_info_t *)data)->size );
    memset( &info, 0, sizeof(info) );
    memcpy( &info, data, len );
    src = (char *)data + len;
    info_size -= len;

    /* fixup the lengths */
    if (info.filename_len > info_size) info.filename_len = info_size;
    info_size -= info.filename_len;
    if (info.cmdline_len > info_size) info.cmdline_len = info_size;
    info_size -= info.cmdline_len;
    if (info.desktop_len > info_size) info.desktop_len = info_size;
    info_size -= info.desktop_len;
    if (info.title_len > info_size) info.title_len = info_size;

    /* store the filename */
    if (info.filename_len)
    {
        len = min( info.filename_len, main_exe_size-1 );
        memcpy( main_exe_name, src, len );
        main_exe_name[len] = 0;
        src += info.filename_len;
    }

    /* copy the other strings */
    len = info.cmdline_len + info.desktop_len + info.title_len;
    if (len && (dst = HeapAlloc( GetProcessHeap(), 0, len + 3 )))
    {
        current_envdb.cmd_line = copy_str( &dst, &src, info.cmdline_len );
        current_startupinfo.lpDesktop = copy_str( &dst, &src, info.desktop_len );
        current_startupinfo.lpTitle = copy_str( &dst, &src, info.title_len );
    }

    current_startupinfo.dwX             = info.x;
    current_startupinfo.dwY             = info.y;
    current_startupinfo.dwXSize         = info.cx;
    current_startupinfo.dwYSize         = info.cy;
    current_startupinfo.dwXCountChars   = info.x_chars;
    current_startupinfo.dwYCountChars   = info.y_chars;
    current_startupinfo.dwFillAttribute = info.attribute;
    current_startupinfo.wShowWindow     = info.cmd_show;
    current_startupinfo.dwFlags         = info.flags;
 done:
    HeapFree( GetProcessHeap(), 0, data );
    return &current_envdb;
}



/***********************************************************************
 *              set_library_argv
 *
 * Set the Wine library argc/argv global variables.
 */
static void set_library_argv( char **argv )
{
    int argc;
    WCHAR *p;
    WCHAR **wargv;
    DWORD total = 0;

    for (argc = 0; argv[argc]; argc++)
        total += MultiByteToWideChar( CP_ACP, 0, argv[argc], -1, NULL, 0 );

    wargv = HeapAlloc( GetProcessHeap(), 0,
                       total * sizeof(WCHAR) + (argc + 1) * sizeof(*wargv) );
    p = (WCHAR *)(wargv + argc + 1);
    for (argc = 0; argv[argc]; argc++)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, argv[argc], -1, p, total );
        wargv[argc] = p;
        p += len;
        total -= len;
    }
    wargv[argc] = NULL;

    __wine_main_argc  = argc;
    __wine_main_argv  = argv;
    __wine_main_wargv = wargv;
}


/***********************************************************************
 *           ENV_BuildCommandLine
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
 * - '\'s that are not followed by a '"' can be left as is
 *   'a\b'   == 'a\b'
 *   'a\\b'  == 'a\\b'
 */
BOOL ENV_BuildCommandLine( char **argv )
{
    int len;
    char *p, **arg;

    set_library_argv( argv );

    if (current_envdb.cmd_line) goto done;  /* already got it from the server */

    len = 0;
    for (arg = argv; *arg; arg++)
    {
        int has_space,bcount;
        char* a;

        has_space=0;
        bcount=0;
        a=*arg;
        while (*a!='\0') {
            if (*a=='\\') {
                bcount++;
            } else {
                if (*a==' ' || *a=='\t') {
                    has_space=1;
                } else if (*a=='"') {
                    /* doubling of '\' preceeding a '"',
                     * plus escaping of said '"'
                     */
                    len+=2*bcount+1;
                }
                bcount=0;
            }
            a++;
        }
        len+=(a-*arg)+1 /* for the separating space */;
        if (has_space)
            len+=2; /* for the quotes */
    }

    if (!(current_envdb.cmd_line = HeapAlloc( GetProcessHeap(), 0, len )))
        return FALSE;

    p = current_envdb.cmd_line;
    for (arg = argv; *arg; arg++)
    {
        int has_space,has_quote;
        char* a;

        /* Check for quotes and spaces in this argument */
        has_space=has_quote=0;
        a=*arg;
        while (*a!='\0') {
            if (*a==' ' || *a=='\t') {
                has_space=1;
                if (has_quote)
                    break;
            } else if (*a=='"') {
                has_quote=1;
                if (has_space)
                    break;
            }
            a++;
        }

        /* Now transfer it to the command line */
        if (has_space)
            *p++='"';
        if (has_quote) {
            int bcount;
            char* a;

            bcount=0;
            a=*arg;
            while (*a!='\0') {
                if (*a=='\\') {
                    *p++=*a;
                    bcount++;
                } else {
                    if (*a=='"') {
                        int i;

                        /* Double all the '\\' preceeding this '"', plus one */
                        for (i=0;i<=bcount;i++)
                            *p++='\\';
                        *p++='"';
                    } else {
                        *p++=*a;
                    }
                    bcount=0;
                }
                a++;
            }
        } else {
            strcpy(p,*arg);
            p+=strlen(*arg);
        }
        if (has_space)
            *p++='"';
        *p++=' ';
    }
    if (p > current_envdb.cmd_line)
        p--;  /* remove last space */
    *p = '\0';

    /* now allocate the Unicode version */
 done:
    len = MultiByteToWideChar( CP_ACP, 0, current_envdb.cmd_line, -1, NULL, 0 );
    if (!(cmdlineW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
        return FALSE;
    MultiByteToWideChar( CP_ACP, 0, current_envdb.cmd_line, -1, cmdlineW, len );
    return TRUE;
}


/***********************************************************************
 *           GetCommandLineA      (KERNEL32.@)
 *
 * WARNING: there's a Windows incompatibility lurking here !
 * Win32s always includes the full path of the program file,
 * whereas Windows NT only returns the full file path plus arguments
 * in case the program has been started with a full path.
 * Win9x seems to have inherited NT behaviour.
 *
 * Note that both Start Menu Execute and Explorer start programs with
 * fully specified quoted app file paths, which is why probably the only case
 * where you'll see single file names is in case of direct launch
 * via CreateProcess or WinExec.
 *
 * Perhaps we should take care of Win3.1 programs here (Win32s "feature").
 *
 * References: MS KB article q102762.txt (special Win32s handling)
 */
LPSTR WINAPI GetCommandLineA(void)
{
    return current_envdb.cmd_line;
}

/***********************************************************************
 *           GetCommandLineW      (KERNEL32.@)
 */
LPWSTR WINAPI GetCommandLineW(void)
{
    return cmdlineW;
}


/***********************************************************************
 *           GetEnvironmentStrings    (KERNEL32.@)
 *           GetEnvironmentStringsA   (KERNEL32.@)
 */
LPSTR WINAPI GetEnvironmentStringsA(void)
{
    return current_envdb.env;
}


/***********************************************************************
 *           GetEnvironmentStringsW   (KERNEL32.@)
 */
LPWSTR WINAPI GetEnvironmentStringsW(void)
{
    INT size;
    LPWSTR ret;

    RtlAcquirePebLock();
    size = HeapSize( GetProcessHeap(), 0, current_envdb.env );
    if ((ret = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) )) != NULL)
    {
        LPSTR pA = current_envdb.env;
        LPWSTR pW = ret;
        while (size--) *pW++ = (WCHAR)(BYTE)*pA++;
    }
    RtlReleasePebLock();
    return ret;
}


/***********************************************************************
 *           FreeEnvironmentStringsA   (KERNEL32.@)
 */
BOOL WINAPI FreeEnvironmentStringsA( LPSTR ptr )
{
    if (ptr != current_envdb.env)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           FreeEnvironmentStringsW   (KERNEL32.@)
 */
BOOL WINAPI FreeEnvironmentStringsW( LPWSTR ptr )
{
    return HeapFree( GetProcessHeap(), 0, ptr );
}


/***********************************************************************
 *           GetEnvironmentVariableA   (KERNEL32.@)
 */
DWORD WINAPI GetEnvironmentVariableA( LPCSTR name, LPSTR value, DWORD size )
{
    LPCSTR p;
    INT ret = 0;

    if (!name || !*name)
    {
        SetLastError( ERROR_ENVVAR_NOT_FOUND );
        return 0;
    }

    RtlAcquirePebLock();
    if ((p = ENV_FindVariable( current_envdb.env, name, strlen(name) )))
    {
        ret = strlen(p);
        if (size <= ret)
        {
            /* If not enough room, include the terminating null
             * in the returned size */
            ret++;
        }
        else if (value) strcpy( value, p );
    }
    RtlReleasePebLock();
    if (!ret)
	SetLastError( ERROR_ENVVAR_NOT_FOUND );
    return ret;
}


/***********************************************************************
 *           GetEnvironmentVariableW   (KERNEL32.@)
 */
DWORD WINAPI GetEnvironmentVariableW( LPCWSTR nameW, LPWSTR valW, DWORD size)
{
    LPSTR name, val;
    DWORD ret;

    if (!nameW || !*nameW)
    {
        SetLastError( ERROR_ENVVAR_NOT_FOUND );
        return 0;
    }

    name = HEAP_strdupWtoA( GetProcessHeap(), 0, nameW );
    val  = valW ? HeapAlloc( GetProcessHeap(), 0, size ) : NULL;
    ret  = GetEnvironmentVariableA( name, val, size );
    if (ret && val)
    {
        if (size && !MultiByteToWideChar( CP_ACP, 0, val, -1, valW, size ))
            valW[size-1] = 0;
    }
    HeapFree( GetProcessHeap(), 0, name );
    if (val) HeapFree( GetProcessHeap(), 0, val );
    return ret;
}


/***********************************************************************
 *           SetEnvironmentVariableA   (KERNEL32.@)
 */
BOOL WINAPI SetEnvironmentVariableA( LPCSTR name, LPCSTR value )
{
    INT old_size, len, res;
    LPSTR p, env, new_env;
    BOOL ret = FALSE;

    if (!name || !*name)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    RtlAcquirePebLock();
    env = p = current_envdb.env;

    /* Find a place to insert the string */

    res = -1;
    len = strlen(name);
    while (*p)
    {
        if (!strncasecmp( name, p, len ) && (p[len] == '=')) break;
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
    if (env_sel) env_sel = SELECTOR_ReallocBlock( env_sel, new_env, old_size + len );
    p = new_env + (p - env);
    if (len > 0) memmove( p + len, p, old_size - (p - new_env) );

    /* Set the new string */

    if (value)
    {
        strcpy( p, name );
        strcat( p, "=" );
        strcat( p, value );
    }
    current_envdb.env = new_env;
    ret = TRUE;

done:
    RtlReleasePebLock();
    return ret;
}


/***********************************************************************
 *           SetEnvironmentVariableW   (KERNEL32.@)
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
 *           ExpandEnvironmentStringsA   (KERNEL32.@)
 *
 * Note: overlapping buffers are not supported; this is how it should be.
 */
DWORD WINAPI ExpandEnvironmentStringsA( LPCSTR src, LPSTR dst, DWORD count )
{
    DWORD len, total_size = 1;  /* 1 for terminating '\0' */
    LPCSTR p, var;

    if (!count) dst = NULL;
    RtlAcquirePebLock();

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
                if ((var = ENV_FindVariable( current_envdb.env, src + 1, len )))
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
    RtlReleasePebLock();

    /* Null-terminate the string */
    if (dst)
    {
        if (!count) dst--;
        *dst = '\0';
    }
    return total_size;
}


/***********************************************************************
 *           ExpandEnvironmentStringsW   (KERNEL32.@)
 */
DWORD WINAPI ExpandEnvironmentStringsW( LPCWSTR src, LPWSTR dst, DWORD len )
{
    LPSTR srcA = HEAP_strdupWtoA( GetProcessHeap(), 0, src );
    LPSTR dstA = dst ? HeapAlloc( GetProcessHeap(), 0, len ) : NULL;
    DWORD ret  = ExpandEnvironmentStringsA( srcA, dstA, len );
    if (dstA)
    {
        ret = MultiByteToWideChar( CP_ACP, 0, dstA, -1, dst, len );
        HeapFree( GetProcessHeap(), 0, dstA );
    }
    HeapFree( GetProcessHeap(), 0, srcA );
    return ret;
}


/***********************************************************************
 *           GetDOSEnvironment     (KERNEL.131)
 */
SEGPTR WINAPI GetDOSEnvironment16(void)
{
    return MAKESEGPTR( env_sel, 0 );
}


/***********************************************************************
 *           GetStdHandle    (KERNEL32.@)
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
 *           SetStdHandle    (KERNEL32.@)
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
 *              GetStartupInfoA         (KERNEL32.@)
 */
VOID WINAPI GetStartupInfoA( LPSTARTUPINFOA info )
{
    *info = current_startupinfo;
}


/***********************************************************************
 *              GetStartupInfoW         (KERNEL32.@)
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
