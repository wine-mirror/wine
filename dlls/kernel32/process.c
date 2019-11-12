/*
 * Win32 processes
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "winbase.h"
#include "wincon.h"
#include "kernel_private.h"
#include "psapi.h"
#include "wine/exception.h"
#include "wine/library.h"
#include "wine/server.h"
#include "wine/unicode.h"
#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(process);
WINE_DECLARE_DEBUG_CHANNEL(relay);

typedef struct
{
    LPSTR lpEnvAddress;
    LPSTR lpCmdLine;
    LPSTR lpCmdShow;
    DWORD dwReserved;
} LOADPARMS32;

static BOOL is_wow64;
static const BOOL is_win64 = (sizeof(void *) > sizeof(int));

HMODULE kernel32_handle = 0;
SYSTEM_BASIC_INFORMATION system_info = { 0 };

const WCHAR DIR_Windows[] = {'C',':','\\','w','i','n','d','o','w','s',0};
const WCHAR DIR_System[] = {'C',':','\\','w','i','n','d','o','w','s',
                            '\\','s','y','s','t','e','m','3','2',0};
const WCHAR *DIR_SysWow64 = NULL;

/* Process flags */
#define PDB32_DEBUGGED      0x0001  /* Process is being debugged */
#define PDB32_WIN16_PROC    0x0008  /* Win16 process */
#define PDB32_DOS_PROC      0x0010  /* Dos process */
#define PDB32_CONSOLE_PROC  0x0020  /* Console process */
#define PDB32_FILE_APIS_OEM 0x0040  /* File APIs are OEM */
#define PDB32_WIN32S_PROC   0x8000  /* Win32s process */

static const WCHAR exeW[] = {'.','e','x','e',0};
static const WCHAR comW[] = {'.','c','o','m',0};
static const WCHAR batW[] = {'.','b','a','t',0};
static const WCHAR cmdW[] = {'.','c','m','d',0};
static const WCHAR pifW[] = {'.','p','i','f',0};
static WCHAR winevdm[] = {'C',':','\\','w','i','n','d','o','w','s',
                          '\\','s','y','s','t','e','m','3','2',
                          '\\','w','i','n','e','v','d','m','.','e','x','e',0};


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
 *           find_exe_file
 *
 * Open an exe file, and return the full name and file handle.
 * Returns FALSE if file could not be found.
 */
static BOOL find_exe_file( const WCHAR *name, WCHAR *buffer, int buflen )
{
    WCHAR *load_path;
    BOOL ret;

    if (!set_ntstatus( RtlGetExePath( name, &load_path ))) return FALSE;

    TRACE("looking for %s in %s\n", debugstr_w(name), debugstr_w(load_path) );

    ret = (SearchPathW( load_path, name, exeW, buflen, buffer, NULL ) ||
           /* not found, try without extension in case it is a Unix app */
           SearchPathW( load_path, name, NULL, buflen, buffer, NULL ));
    RtlReleasePath( load_path );
    return ret;
}


/***********************************************************************
 *              set_library_argv
 *
 * Set the Wine library argv global variables.
 */
static void set_library_argv( WCHAR **wargv )
{
    int argc;
    char *p, **argv;
    DWORD total = 0;

    /* convert argv back from Unicode since it has to be in the Ansi codepage not the Unix one */

    for (argc = 0; wargv[argc]; argc++)
        total += WideCharToMultiByte( CP_ACP, 0, wargv[argc], -1, NULL, 0, NULL, NULL );

    argv = RtlAllocateHeap( GetProcessHeap(), 0, total + (argc + 1) * sizeof(*argv) );
    p = (char *)(argv + argc + 1);
    for (argc = 0; wargv[argc]; argc++)
    {
        DWORD reslen = WideCharToMultiByte( CP_ACP, 0, wargv[argc], -1, p, total, NULL, NULL );
        argv[argc] = p;
        p += reslen;
        total -= reslen;
    }
    argv[argc] = NULL;

    __wine_main_argc = argc;
    __wine_main_argv = argv;
    __wine_main_wargv = wargv;
}


/***********************************************************************
 *           init_windows_dirs
 */
static void init_windows_dirs(void)
{
    static const WCHAR default_syswow64W[] = {'C',':','\\','w','i','n','d','o','w','s',
                                              '\\','s','y','s','w','o','w','6','4',0};

    if (is_win64 || is_wow64)   /* SysWow64 is always defined on 64-bit */
    {
        DIR_SysWow64 = default_syswow64W;
        memcpy( winevdm, default_syswow64W, sizeof(default_syswow64W) - sizeof(WCHAR) );
    }
}


#ifdef __i386__
extern DWORD call_process_entry( PEB *peb, LPTHREAD_START_ROUTINE entry );
__ASM_GLOBAL_FUNC( call_process_entry,
                    "pushl %ebp\n\t"
                    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                    __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                    "movl %esp,%ebp\n\t"
                    __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                    "pushl 4(%ebp)\n\t"  /* deliberately mis-align the stack by 8, Doom 3 needs this */
                    "pushl 4(%ebp)\n\t"  /* Driller expects readable address at this offset */
                    "pushl 4(%ebp)\n\t"
                    "pushl 8(%ebp)\n\t"
                    "call *12(%ebp)\n\t"
                    "leave\n\t"
                    __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                    __ASM_CFI(".cfi_same_value %ebp\n\t")
                    "ret" )

extern void WINAPI start_process( LPTHREAD_START_ROUTINE entry, PEB *peb ) DECLSPEC_HIDDEN;
extern void WINAPI start_process_wrapper(void) DECLSPEC_HIDDEN;
__ASM_GLOBAL_FUNC( start_process_wrapper,
                   "pushl %ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   "movl %esp,%ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl %ebx\n\t"  /* arg */
                   "pushl %eax\n\t"  /* entry */
                   "call " __ASM_NAME("start_process") )
#else
static inline DWORD call_process_entry( PEB *peb, LPTHREAD_START_ROUTINE entry )
{
    return entry( peb );
}
static void WINAPI start_process( LPTHREAD_START_ROUTINE entry, PEB *peb );
#define start_process_wrapper start_process
#endif

/***********************************************************************
 *           start_process
 *
 * Startup routine of a new process. Runs on the new process stack.
 */
void WINAPI start_process( LPTHREAD_START_ROUTINE entry, PEB *peb )
{
    BOOL being_debugged;

    if (!entry)
    {
        ERR( "%s doesn't have an entry point, it cannot be executed\n",
             debugstr_w(peb->ProcessParameters->ImagePathName.Buffer) );
        ExitThread( 1 );
    }

    TRACE_(relay)( "\1Starting process %s (entryproc=%p)\n",
                   debugstr_w(peb->ProcessParameters->ImagePathName.Buffer), entry );

    __TRY
    {
        if (!CheckRemoteDebuggerPresent( GetCurrentProcess(), &being_debugged ))
            being_debugged = FALSE;

        SetLastError( 0 );  /* clear error code */
        if (being_debugged) DbgBreakPoint();
        ExitThread( call_process_entry( peb, entry ));
    }
    __EXCEPT(UnhandledExceptionFilter)
    {
        TerminateProcess( GetCurrentProcess(), GetExceptionCode() );
    }
    __ENDTRY
    abort();  /* should not be reached */
}


/***********************************************************************
 *           __wine_kernel_init
 *
 * Wine initialisation: load and start the main exe file.
 */
void * CDECL __wine_kernel_init(void)
{
    static const WCHAR kernel32W[] = {'k','e','r','n','e','l','3','2',0};

    PEB *peb = NtCurrentTeb()->Peb;
    RTL_USER_PROCESS_PARAMETERS *params = peb->ProcessParameters;

    /* Initialize everything */

    setbuf(stdout,NULL);
    setbuf(stderr,NULL);
    kernel32_handle = GetModuleHandleW(kernel32W);
    IsWow64Process( GetCurrentProcess(), &is_wow64 );
    RtlSetUnhandledExceptionFilter( UnhandledExceptionFilter );

    LOCALE_Init();
    init_windows_dirs();
    convert_old_config();
    set_library_argv( __wine_main_wargv );

    if (!params->CurrentDirectory.Handle) chdir("/"); /* avoid locking removable devices */

    return start_process_wrapper;
}


/***********************************************************************
 *           build_argv
 *
 * Build an argv array from a command-line.
 * 'reserved' is the number of args to reserve before the first one.
 */
static char **build_argv( const UNICODE_STRING *cmdlineW, int reserved )
{
    int argc;
    char** argv;
    char *arg,*s,*d,*cmdline;
    int in_quotes,bcount,len;

    len = WideCharToMultiByte( CP_UNIXCP, 0, cmdlineW->Buffer, cmdlineW->Length / sizeof(WCHAR),
                               NULL, 0, NULL, NULL );
    if (!(cmdline = HeapAlloc( GetProcessHeap(), 0, len + 1 ))) return NULL;
    WideCharToMultiByte( CP_UNIXCP, 0, cmdlineW->Buffer, cmdlineW->Length / sizeof(WCHAR),
                         cmdline, len, NULL, NULL );
    cmdline[len++] = 0;

    argc=reserved+1;
    bcount=0;
    in_quotes=0;
    s=cmdline;
    while (1) {
        if (*s=='\0' || ((*s==' ' || *s=='\t') && !in_quotes)) {
            /* space */
            argc++;
            /* skip the remaining spaces */
            while (*s==' ' || *s=='\t') {
                s++;
            }
            if (*s=='\0')
                break;
            bcount=0;
            continue;
        } else if (*s=='\\') {
            /* '\', count them */
            bcount++;
        } else if ((*s=='"') && ((bcount & 1)==0)) {
            if (in_quotes && s[1] == '"') {
               s++;
            } else {
               /* unescaped '"' */
               in_quotes=!in_quotes;
               bcount=0;
            }
        } else {
            /* a regular character */
            bcount=0;
        }
        s++;
    }
    if (!(argv = HeapAlloc( GetProcessHeap(), 0, argc*sizeof(*argv) + len )))
    {
        HeapFree( GetProcessHeap(), 0, cmdline );
        return NULL;
    }

    arg = d = s = (char *)(argv + argc);
    memcpy( d, cmdline, len );
    bcount=0;
    in_quotes=0;
    argc=reserved;
    while (*s) {
        if ((*s==' ' || *s=='\t') && !in_quotes) {
            /* Close the argument and copy it */
            *d=0;
            argv[argc++]=arg;

            /* skip the remaining spaces */
            do {
                s++;
            } while (*s==' ' || *s=='\t');

            /* Start with a new argument */
            arg=d=s;
            bcount=0;
        } else if (*s=='\\') {
            /* '\\' */
            *d++=*s++;
            bcount++;
        } else if (*s=='"') {
            /* '"' */
            if ((bcount & 1)==0) {
                /* Preceded by an even number of '\', this is half that
                 * number of '\', plus a '"' which we discard.
                 */
                d-=bcount/2;
                s++;
                if(in_quotes && *s == '"') {
                  *d++='"';
                  s++;
                } else {
                  in_quotes=!in_quotes;
                }
            } else {
                /* Preceded by an odd number of '\', this is half that
                 * number of '\' followed by a '"'
                 */
                d=d-bcount/2-1;
                *d++='"';
                s++;
            }
            bcount=0;
        } else {
            /* a regular character */
            *d++=*s++;
            bcount=0;
        }
    }
    if (*arg) {
        *d='\0';
        argv[argc++]=arg;
    }
    argv[argc]=NULL;

    HeapFree( GetProcessHeap(), 0, cmdline );
    return argv;
}


/***********************************************************************
 *           build_envp
 *
 * Build the environment of a new child process.
 */
static char **build_envp( const WCHAR *envW )
{
    static const char * const unix_vars[] = { "PATH", "TEMP", "TMP", "HOME" };

    const WCHAR *end;
    char **envp;
    char *env, *p;
    int count = 1, length;
    unsigned int i;

    for (end = envW; *end; count++) end += strlenW(end) + 1;
    end++;
    length = WideCharToMultiByte( CP_UNIXCP, 0, envW, end - envW, NULL, 0, NULL, NULL );
    if (!(env = HeapAlloc( GetProcessHeap(), 0, length ))) return NULL;
    WideCharToMultiByte( CP_UNIXCP, 0, envW, end - envW, env, length, NULL, NULL );

    for (p = env; *p; p += strlen(p) + 1)
        if (is_special_env_var( p )) length += 4; /* prefix it with "WINE" */

    for (i = 0; i < ARRAY_SIZE( unix_vars ); i++)
    {
        if (!(p = getenv(unix_vars[i]))) continue;
        length += strlen(unix_vars[i]) + strlen(p) + 2;
        count++;
    }

    if ((envp = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*envp) + length )))
    {
        char **envptr = envp;
        char *dst = (char *)(envp + count);

        /* some variables must not be modified, so we get them directly from the unix env */
        for (i = 0; i < ARRAY_SIZE( unix_vars ); i++)
        {
            if (!(p = getenv(unix_vars[i]))) continue;
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
    HeapFree( GetProcessHeap(), 0, env );
    return envp;
}


/***********************************************************************
 *           fork_and_exec
 *
 * Fork and exec a new Unix binary, checking for errors.
 */
static NTSTATUS fork_and_exec( const RTL_USER_PROCESS_PARAMETERS *params, const char *newdir )
{
    int fd[2], stdin_fd = -1, stdout_fd = -1, stderr_fd = -1;
    int pid, err;
    char **argv, **envp;
    UNICODE_STRING nt_name;
    ANSI_STRING unix_name;
    NTSTATUS status;

    status = RtlDosPathNameToNtPathName_U_WithStatus( params->ImagePathName.Buffer, &nt_name, NULL, NULL );
    if (!status)
    {
        status = wine_nt_to_unix_file_name( &nt_name, &unix_name, FILE_OPEN, FALSE );
        RtlFreeUnicodeString( &nt_name );
    }
    if (status) return status;

#ifdef HAVE_PIPE2
    if (pipe2( fd, O_CLOEXEC ) == -1)
#endif
    {
        if (pipe(fd) == -1)
        {
            RtlFreeAnsiString( &unix_name );
            return STATUS_TOO_MANY_OPENED_FILES;
        }
        fcntl( fd[0], F_SETFD, FD_CLOEXEC );
        fcntl( fd[1], F_SETFD, FD_CLOEXEC );
    }

    wine_server_handle_to_fd( params->hStdInput, FILE_READ_DATA, &stdin_fd, NULL );
    wine_server_handle_to_fd( params->hStdOutput, FILE_WRITE_DATA, &stdout_fd, NULL );
    wine_server_handle_to_fd( params->hStdError, FILE_WRITE_DATA, &stderr_fd, NULL );

    argv = build_argv( &params->CommandLine, 0 );
    envp = build_envp( params->Environment );

    if (!(pid = fork()))  /* child */
    {
        if (!(pid = fork()))  /* grandchild */
        {
            close( fd[0] );

            if (params->ConsoleFlags || params->ConsoleHandle == KERNEL32_CONSOLE_ALLOC ||
                (params->hStdInput == INVALID_HANDLE_VALUE && params->hStdOutput == INVALID_HANDLE_VALUE))
            {
                int nullfd = open( "/dev/null", O_RDWR );
                setsid();
                /* close stdin and stdout */
                if (nullfd != -1)
                {
                    dup2( nullfd, 0 );
                    dup2( nullfd, 1 );
                    close( nullfd );
                }
            }
            else
            {
                if (stdin_fd != -1)
                {
                    dup2( stdin_fd, 0 );
                    close( stdin_fd );
                }
                if (stdout_fd != -1)
                {
                    dup2( stdout_fd, 1 );
                    close( stdout_fd );
                }
                if (stderr_fd != -1)
                {
                    dup2( stderr_fd, 2 );
                    close( stderr_fd );
                }
            }

            /* Reset signals that we previously set to SIG_IGN */
            signal( SIGPIPE, SIG_DFL );

            if (newdir) chdir(newdir);

            if (argv && envp) execve( unix_name.Buffer, argv, envp );
        }

        if (pid <= 0)  /* grandchild if exec failed or child if fork failed */
        {
            status = pid ? STATUS_NO_MEMORY : STATUS_ACCESS_DENIED; /* FIXME: FILE_GetNtStatus() */
            write( fd[1], &status, sizeof(status) );
            _exit(1);
        }

        _exit(0); /* child if fork succeeded */
    }
    HeapFree( GetProcessHeap(), 0, argv );
    HeapFree( GetProcessHeap(), 0, envp );
    RtlFreeAnsiString( &unix_name );
    if (stdin_fd != -1) close( stdin_fd );
    if (stdout_fd != -1) close( stdout_fd );
    if (stderr_fd != -1) close( stderr_fd );
    close( fd[1] );
    if (pid != -1)
    {
        /* reap child */
        do {
            err = waitpid(pid, NULL, 0);
        } while (err < 0 && errno == EINTR);
        read( fd[0], &status, sizeof(status) );  /* if we read something, exec or second fork failed */
    }
    else status = STATUS_NO_MEMORY;
    close( fd[0] );
    return status;
}


/***********************************************************************
 *           create_process_params
 */
static RTL_USER_PROCESS_PARAMETERS *create_process_params( LPCWSTR filename, LPCWSTR cmdline,
                                                           LPCWSTR cur_dir, void *env, DWORD flags,
                                                           const STARTUPINFOW *startup )
{
    RTL_USER_PROCESS_PARAMETERS *params;
    UNICODE_STRING imageW, dllpathW, curdirW, cmdlineW, titleW, desktopW, runtimeW, newdirW;
    WCHAR imagepath[MAX_PATH];
    WCHAR *load_path, *dummy, *envW = env;

    if(!GetLongPathNameW( filename, imagepath, MAX_PATH ))
        lstrcpynW( imagepath, filename, MAX_PATH );
    if(!GetFullPathNameW( imagepath, MAX_PATH, imagepath, NULL ))
        lstrcpynW( imagepath, filename, MAX_PATH );

    if (env && !(flags & CREATE_UNICODE_ENVIRONMENT))  /* convert environment to unicode */
    {
        char *e = env;
        DWORD lenW;

        while (*e) e += strlen(e) + 1;
        e++;  /* final null */
        lenW = MultiByteToWideChar( CP_ACP, 0, env, e - (char *)env, NULL, 0 );
        if ((envW = HeapAlloc( GetProcessHeap(), 0, lenW * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_ACP, 0, env, e - (char *)env, envW, lenW );
    }

    newdirW.Buffer = NULL;
    if (cur_dir)
    {
        if (RtlDosPathNameToNtPathName_U( cur_dir, &newdirW, NULL, NULL ))
            cur_dir = newdirW.Buffer + 4;  /* skip \??\ prefix */
        else
            cur_dir = NULL;
    }
    LdrGetDllPath( imagepath, LOAD_WITH_ALTERED_SEARCH_PATH, &load_path, &dummy );
    RtlInitUnicodeString( &imageW, imagepath );
    RtlInitUnicodeString( &dllpathW, load_path );
    RtlInitUnicodeString( &curdirW, cur_dir );
    RtlInitUnicodeString( &cmdlineW, cmdline );
    RtlInitUnicodeString( &titleW, startup->lpTitle ? startup->lpTitle : imagepath );
    RtlInitUnicodeString( &desktopW, startup->lpDesktop );
    runtimeW.Buffer = (WCHAR *)startup->lpReserved2;
    runtimeW.Length = runtimeW.MaximumLength = startup->cbReserved2;
    if (RtlCreateProcessParametersEx( &params, &imageW, &dllpathW, cur_dir ? &curdirW : NULL,
                                      &cmdlineW, envW, &titleW, &desktopW,
                                      NULL, &runtimeW, PROCESS_PARAMS_FLAG_NORMALIZED ))
    {
        RtlReleasePath( load_path );
        if (envW != env) HeapFree( GetProcessHeap(), 0, envW );
        return NULL;
    }
    RtlReleasePath( load_path );

    if (flags & CREATE_NEW_PROCESS_GROUP) params->ConsoleFlags = 1;
    if (flags & CREATE_NEW_CONSOLE) params->ConsoleHandle = KERNEL32_CONSOLE_ALLOC;

    if (startup->dwFlags & STARTF_USESTDHANDLES)
    {
        params->hStdInput  = startup->hStdInput;
        params->hStdOutput = startup->hStdOutput;
        params->hStdError  = startup->hStdError;
    }
    else if (flags & DETACHED_PROCESS)
    {
        params->hStdInput  = INVALID_HANDLE_VALUE;
        params->hStdOutput = INVALID_HANDLE_VALUE;
        params->hStdError  = INVALID_HANDLE_VALUE;
    }
    else
    {
        params->hStdInput  = NtCurrentTeb()->Peb->ProcessParameters->hStdInput;
        params->hStdOutput = NtCurrentTeb()->Peb->ProcessParameters->hStdOutput;
        params->hStdError  = NtCurrentTeb()->Peb->ProcessParameters->hStdError;
    }

    if (flags & CREATE_NEW_CONSOLE)
    {
        /* this is temporary (for console handles). We have no way to control that the handle is invalid in child process otherwise */
        if (is_console_handle(params->hStdInput))  params->hStdInput  = INVALID_HANDLE_VALUE;
        if (is_console_handle(params->hStdOutput)) params->hStdOutput = INVALID_HANDLE_VALUE;
        if (is_console_handle(params->hStdError))  params->hStdError  = INVALID_HANDLE_VALUE;
    }
    else
    {
        if (is_console_handle(params->hStdInput))  params->hStdInput  = (HANDLE)((UINT_PTR)params->hStdInput & ~3);
        if (is_console_handle(params->hStdOutput)) params->hStdOutput = (HANDLE)((UINT_PTR)params->hStdOutput & ~3);
        if (is_console_handle(params->hStdError))  params->hStdError  = (HANDLE)((UINT_PTR)params->hStdError & ~3);
    }

    params->dwX             = startup->dwX;
    params->dwY             = startup->dwY;
    params->dwXSize         = startup->dwXSize;
    params->dwYSize         = startup->dwYSize;
    params->dwXCountChars   = startup->dwXCountChars;
    params->dwYCountChars   = startup->dwYCountChars;
    params->dwFillAttribute = startup->dwFillAttribute;
    params->dwFlags         = startup->dwFlags;
    params->wShowWindow     = startup->wShowWindow;

    if (envW != env) HeapFree( GetProcessHeap(), 0, envW );
    return params;
}

/***********************************************************************
 *           create_nt_process
 */
static NTSTATUS create_nt_process( LPSECURITY_ATTRIBUTES psa, LPSECURITY_ATTRIBUTES tsa,
                                   BOOL inherit, DWORD flags, RTL_USER_PROCESS_PARAMETERS *params,
                                   RTL_USER_PROCESS_INFORMATION *info )
{
    NTSTATUS status;
    UNICODE_STRING nameW;

    if (!params->ImagePathName.Buffer[0]) return STATUS_OBJECT_PATH_NOT_FOUND;
    status = RtlDosPathNameToNtPathName_U_WithStatus( params->ImagePathName.Buffer, &nameW, NULL, NULL );
    if (!status)
    {
        params->DebugFlags = flags;  /* hack, cf. RtlCreateUserProcess implementation */
        status = RtlCreateUserProcess( &nameW, OBJ_CASE_INSENSITIVE, params,
                                       psa ? psa->lpSecurityDescriptor : NULL,
                                       tsa ? tsa->lpSecurityDescriptor : NULL,
                                       0, inherit, 0, 0, info );
        RtlFreeUnicodeString( &nameW );
    }
    return status;
}


/***********************************************************************
 *           create_vdm_process
 *
 * Create a new VDM process for a 16-bit or DOS application.
 */
static NTSTATUS create_vdm_process( LPSECURITY_ATTRIBUTES psa, LPSECURITY_ATTRIBUTES tsa,
                                    BOOL inherit, DWORD flags, RTL_USER_PROCESS_PARAMETERS *params,
                                    RTL_USER_PROCESS_INFORMATION *info )
{
    static const WCHAR argsW[] = {'%','s',' ','-','-','a','p','p','-','n','a','m','e',' ','"','%','s','"',' ','%','s',0};

    NTSTATUS status;
    WCHAR *new_cmd_line;

    new_cmd_line = HeapAlloc(GetProcessHeap(), 0,
			     (strlenW(params->ImagePathName.Buffer) +
                              strlenW(params->CommandLine.Buffer) +
                              strlenW(winevdm) + 16) * sizeof(WCHAR));
    if (!new_cmd_line) return STATUS_NO_MEMORY;

    sprintfW( new_cmd_line, argsW, winevdm, params->ImagePathName.Buffer, params->CommandLine.Buffer );
    RtlInitUnicodeString( &params->ImagePathName, winevdm );
    RtlInitUnicodeString( &params->CommandLine, new_cmd_line );
    status = create_nt_process( psa, tsa, inherit, flags, params, info );
    HeapFree( GetProcessHeap(), 0, new_cmd_line );
    return status;
}


/***********************************************************************
 *           create_cmd_process
 *
 * Create a new cmd shell process for a .BAT file.
 */
static NTSTATUS create_cmd_process( LPSECURITY_ATTRIBUTES psa, LPSECURITY_ATTRIBUTES tsa,
                                    BOOL inherit, DWORD flags, RTL_USER_PROCESS_PARAMETERS *params,
                                    RTL_USER_PROCESS_INFORMATION *info )
{
    static const WCHAR argsW[] = {'%','s',' ','/','s','/','c',' ','"','%','s','"',0};
    static const WCHAR comspecW[] = {'C','O','M','S','P','E','C',0};
    static const WCHAR cmdW[] = {'\\','c','m','d','.','e','x','e',0};
    WCHAR comspec[MAX_PATH];
    WCHAR *newcmdline;
    NTSTATUS status;

    if (!GetEnvironmentVariableW( comspecW, comspec, ARRAY_SIZE( comspec )))
    {
        GetSystemDirectoryW( comspec, ARRAY_SIZE( comspec ) - ARRAY_SIZE( cmdW ));
        strcatW( comspec, cmdW );
    }
    if (!(newcmdline = HeapAlloc( GetProcessHeap(), 0,
                                  (strlenW(comspec) + 7 +
                                   strlenW(params->CommandLine.Buffer) + 2) * sizeof(WCHAR))))
        return STATUS_NO_MEMORY;

    sprintfW( newcmdline, argsW, comspec, params->CommandLine.Buffer );
    RtlInitUnicodeString( &params->ImagePathName, comspec );
    RtlInitUnicodeString( &params->CommandLine, newcmdline );
    status = create_nt_process( psa, tsa, inherit, flags, params, info );
    HeapFree( GetProcessHeap(), 0, newcmdline );
    return status;
}


/*************************************************************************
 *               get_file_name
 *
 * Helper for CreateProcess: retrieve the file name to load from the
 * app name and command line. Store the file name in buffer, and
 * return a possibly modified command line.
 */
static LPWSTR get_file_name( LPWSTR cmdline, LPWSTR buffer, int buflen )
{
    WCHAR *name, *pos, *first_space, *ret = NULL;
    const WCHAR *p;

    /* first check for a quoted file name */

    if ((cmdline[0] == '"') && ((p = strchrW( cmdline + 1, '"' ))))
    {
        int len = p - cmdline - 1;
        /* extract the quoted portion as file name */
        if (!(name = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) ))) return NULL;
        memcpy( name, cmdline + 1, len * sizeof(WCHAR) );
        name[len] = 0;

        if (!find_exe_file( name, buffer, buflen )) goto done;
        ret = cmdline;  /* no change necessary */
        goto done;
    }

    /* now try the command-line word by word */

    if (!(name = HeapAlloc( GetProcessHeap(), 0, (strlenW(cmdline) + 1) * sizeof(WCHAR) )))
        return NULL;
    pos = name;
    p = cmdline;
    first_space = NULL;

    for (;;)
    {
        while (*p && *p != ' ' && *p != '\t') *pos++ = *p++;
        *pos = 0;
        if (find_exe_file( name, buffer, buflen ))
        {
            ret = cmdline;
            break;
        }
        if (!first_space) first_space = pos;
        if (!(*pos++ = *p++)) break;
    }

    if (!ret)
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
    }
    else if (first_space)  /* build a new command-line with quotes */
    {
        static const WCHAR quotesW[] = {'"','%','s','"','%','s',0};

        if (!(ret = HeapAlloc( GetProcessHeap(), 0, (strlenW(cmdline) + 3) * sizeof(WCHAR) )))
            goto done;
        sprintfW( ret, quotesW, name, p );
    }

 done:
    HeapFree( GetProcessHeap(), 0, name );
    return ret;
}

/**********************************************************************
 *      CreateProcessInternalW    (KERNEL32.@)
 */
BOOL WINAPI CreateProcessInternalW( HANDLE token, LPCWSTR app_name, LPWSTR cmd_line,
                                    LPSECURITY_ATTRIBUTES process_attr, LPSECURITY_ATTRIBUTES thread_attr,
                                    BOOL inherit, DWORD flags, LPVOID env, LPCWSTR cur_dir,
                                    LPSTARTUPINFOW startup_info, LPPROCESS_INFORMATION info,
                                    HANDLE *new_token )
{
    char *unixdir = NULL;
    WCHAR name[MAX_PATH];
    WCHAR *p, *tidy_cmdline = cmd_line;
    RTL_USER_PROCESS_PARAMETERS *params = NULL;
    RTL_USER_PROCESS_INFORMATION rtl_info;
    NTSTATUS status;

    /* Process the AppName and/or CmdLine to get module name and path */

    TRACE("app %s cmdline %s\n", debugstr_w(app_name), debugstr_w(cmd_line) );

    if (token) FIXME("Creating a process with a token is not yet implemented\n");
    if (new_token) FIXME("No support for returning created process token\n");

    if (app_name)
    {
        if (!cmd_line || !cmd_line[0]) /* no command-line, create one */
        {
            static const WCHAR quotesW[] = {'"','%','s','"',0};
            if (!(tidy_cmdline = HeapAlloc( GetProcessHeap(), 0, (strlenW(app_name)+3) * sizeof(WCHAR) )))
                return FALSE;
            sprintfW( tidy_cmdline, quotesW, app_name );
        }
    }
    else
    {
        if (!(tidy_cmdline = get_file_name( cmd_line, name, ARRAY_SIZE(name) ))) return FALSE;
        app_name = name;
    }

    /* Warn if unsupported features are used */

    if (flags & (IDLE_PRIORITY_CLASS | HIGH_PRIORITY_CLASS | REALTIME_PRIORITY_CLASS |
                 CREATE_DEFAULT_ERROR_MODE | CREATE_NO_WINDOW |
                 PROFILE_USER | PROFILE_KERNEL | PROFILE_SERVER))
        WARN("(%s,...): ignoring some flags in %x\n", debugstr_w(app_name), flags);

    if (cur_dir)
    {
        if (!(unixdir = wine_get_unix_file_name( cur_dir )))
        {
            status = STATUS_NOT_A_DIRECTORY;
            goto done;
        }
    }
    else
    {
        WCHAR buf[MAX_PATH];
        if (GetCurrentDirectoryW(MAX_PATH, buf)) unixdir = wine_get_unix_file_name( buf );
    }

    info->hThread = info->hProcess = 0;
    info->dwProcessId = info->dwThreadId = 0;

    if (!(params = create_process_params( app_name, tidy_cmdline, cur_dir, env, flags, startup_info )))
    {
        status = STATUS_NO_MEMORY;
        goto done;
    }

    status = create_nt_process( process_attr, thread_attr, inherit, flags, params, &rtl_info );
    switch (status)
    {
    case STATUS_SUCCESS:
        break;
    case STATUS_INVALID_IMAGE_WIN_16:
    case STATUS_INVALID_IMAGE_NE_FORMAT:
    case STATUS_INVALID_IMAGE_PROTECT:
        TRACE( "starting %s as Win16/DOS binary\n", debugstr_w(app_name) );
        status = create_vdm_process( process_attr, thread_attr, inherit, flags, params, &rtl_info );
        break;
    case STATUS_INVALID_IMAGE_NOT_MZ:
        /* check for .com or .bat extension */
        if ((p = strrchrW( app_name, '.' )))
        {
            if (!strcmpiW( p, comW ) || !strcmpiW( p, pifW ))
            {
                TRACE( "starting %s as DOS binary\n", debugstr_w(app_name) );
                status = create_vdm_process( process_attr, thread_attr, inherit, flags, params, &rtl_info );
                break;
            }
            if (!strcmpiW( p, batW ) || !strcmpiW( p, cmdW ))
            {
                TRACE( "starting %s as batch binary\n", debugstr_w(app_name) );
                status = create_cmd_process( process_attr, thread_attr, inherit, flags, params, &rtl_info );
                break;
            }
        }
        /* unknown file, try as unix executable */
        TRACE( "starting %s as Unix binary\n", debugstr_w(app_name) );
        status = fork_and_exec( params, unixdir );
        break;
    }

    if (!status)
    {
        info->hProcess    = rtl_info.Process;
        info->hThread     = rtl_info.Thread;
        info->dwProcessId = HandleToUlong( rtl_info.ClientId.UniqueProcess );
        info->dwThreadId  = HandleToUlong( rtl_info.ClientId.UniqueThread );
        if (!(flags & CREATE_SUSPENDED)) NtResumeThread( rtl_info.Thread, NULL );
        TRACE( "started process pid %04x tid %04x\n", info->dwProcessId, info->dwThreadId );
    }

 done:
    RtlDestroyProcessParameters( params );
    if (tidy_cmdline != cmd_line) HeapFree( GetProcessHeap(), 0, tidy_cmdline );
    HeapFree( GetProcessHeap(), 0, unixdir );
    return set_ntstatus( status );
}


/**********************************************************************
 *       CreateProcessInternalA          (KERNEL32.@)
 */
BOOL WINAPI CreateProcessInternalA( HANDLE token, LPCSTR app_name, LPSTR cmd_line,
                                    LPSECURITY_ATTRIBUTES process_attr, LPSECURITY_ATTRIBUTES thread_attr,
                                    BOOL inherit, DWORD flags, LPVOID env, LPCSTR cur_dir,
                                    LPSTARTUPINFOA startup_info, LPPROCESS_INFORMATION info,
                                    HANDLE *new_token )
{
    BOOL ret = FALSE;
    WCHAR *app_nameW = NULL, *cmd_lineW = NULL, *cur_dirW = NULL;
    UNICODE_STRING desktopW, titleW;
    STARTUPINFOW infoW;

    desktopW.Buffer = NULL;
    titleW.Buffer = NULL;
    if (app_name && !(app_nameW = FILE_name_AtoW( app_name, TRUE ))) goto done;
    if (cmd_line && !(cmd_lineW = FILE_name_AtoW( cmd_line, TRUE ))) goto done;
    if (cur_dir && !(cur_dirW = FILE_name_AtoW( cur_dir, TRUE ))) goto done;

    if (startup_info->lpDesktop) RtlCreateUnicodeStringFromAsciiz( &desktopW, startup_info->lpDesktop );
    if (startup_info->lpTitle) RtlCreateUnicodeStringFromAsciiz( &titleW, startup_info->lpTitle );

    memcpy( &infoW, startup_info, sizeof(infoW) );
    infoW.lpDesktop = desktopW.Buffer;
    infoW.lpTitle = titleW.Buffer;

    if (startup_info->lpReserved)
      FIXME("StartupInfo.lpReserved is used, please report (%s)\n",
            debugstr_a(startup_info->lpReserved));

    ret = CreateProcessInternalW( token, app_nameW, cmd_lineW, process_attr, thread_attr,
                                  inherit, flags, env, cur_dirW, &infoW, info, new_token );
done:
    HeapFree( GetProcessHeap(), 0, app_nameW );
    HeapFree( GetProcessHeap(), 0, cmd_lineW );
    HeapFree( GetProcessHeap(), 0, cur_dirW );
    RtlFreeUnicodeString( &desktopW );
    RtlFreeUnicodeString( &titleW );
    return ret;
}


/**********************************************************************
 *       CreateProcessA          (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateProcessA( LPCSTR app_name, LPSTR cmd_line, LPSECURITY_ATTRIBUTES process_attr,
                                              LPSECURITY_ATTRIBUTES thread_attr, BOOL inherit,
                                              DWORD flags, LPVOID env, LPCSTR cur_dir,
                                              LPSTARTUPINFOA startup_info, LPPROCESS_INFORMATION info )
{
    return CreateProcessInternalA( NULL, app_name, cmd_line, process_attr, thread_attr,
                                   inherit, flags, env, cur_dir, startup_info, info, NULL );
}


/**********************************************************************
 *       CreateProcessW          (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateProcessW( LPCWSTR app_name, LPWSTR cmd_line, LPSECURITY_ATTRIBUTES process_attr,
                                              LPSECURITY_ATTRIBUTES thread_attr, BOOL inherit, DWORD flags,
                                              LPVOID env, LPCWSTR cur_dir, LPSTARTUPINFOW startup_info,
                                              LPPROCESS_INFORMATION info )
{
    return CreateProcessInternalW( NULL, app_name, cmd_line, process_attr, thread_attr,
                                   inherit, flags, env, cur_dir, startup_info, info, NULL );
}


/**********************************************************************
 *       CreateProcessAsUserA          (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateProcessAsUserA( HANDLE token, LPCSTR app_name, LPSTR cmd_line,
                                                    LPSECURITY_ATTRIBUTES process_attr,
                                                    LPSECURITY_ATTRIBUTES thread_attr,
                                                    BOOL inherit, DWORD flags, LPVOID env, LPCSTR cur_dir,
                                                    LPSTARTUPINFOA startup_info,
                                                    LPPROCESS_INFORMATION info )
{
    return CreateProcessInternalA( token, app_name, cmd_line, process_attr, thread_attr,
                                   inherit, flags, env, cur_dir, startup_info, info, NULL );
}


/**********************************************************************
 *       CreateProcessAsUserW          (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateProcessAsUserW( HANDLE token, LPCWSTR app_name, LPWSTR cmd_line,
                                                    LPSECURITY_ATTRIBUTES process_attr,
                                                    LPSECURITY_ATTRIBUTES thread_attr,
                                                    BOOL inherit, DWORD flags, LPVOID env, LPCWSTR cur_dir,
                                                    LPSTARTUPINFOW startup_info,
                                                    LPPROCESS_INFORMATION info )
{
    return CreateProcessInternalW( token, app_name, cmd_line, process_attr, thread_attr,
                                   inherit, flags, env, cur_dir, startup_info, info, NULL );
}


/***********************************************************************
 *           wait_input_idle
 *
 * Wrapper to call WaitForInputIdle USER function
 */
typedef DWORD (WINAPI *WaitForInputIdle_ptr)( HANDLE hProcess, DWORD dwTimeOut );

static DWORD wait_input_idle( HANDLE process, DWORD timeout )
{
    HMODULE mod = GetModuleHandleA( "user32.dll" );
    if (mod)
    {
        WaitForInputIdle_ptr ptr = (WaitForInputIdle_ptr)GetProcAddress( mod, "WaitForInputIdle" );
        if (ptr) return ptr( process, timeout );
    }
    return 0;
}


/***********************************************************************
 *           WinExec   (KERNEL32.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH WinExec( LPCSTR lpCmdLine, UINT nCmdShow )
{
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    char *cmdline;
    UINT ret;

    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = nCmdShow;

    /* cmdline needs to be writable for CreateProcess */
    if (!(cmdline = HeapAlloc( GetProcessHeap(), 0, strlen(lpCmdLine)+1 ))) return 0;
    strcpy( cmdline, lpCmdLine );

    if (CreateProcessA( NULL, cmdline, NULL, NULL, FALSE,
                        0, NULL, NULL, &startup, &info ))
    {
        /* Give 30 seconds to the app to come up */
        if (wait_input_idle( info.hProcess, 30000 ) == WAIT_FAILED)
            WARN("WaitForInputIdle failed: Error %d\n", GetLastError() );
        ret = 33;
        /* Close off the handles */
        CloseHandle( info.hThread );
        CloseHandle( info.hProcess );
    }
    else if ((ret = GetLastError()) >= 32)
    {
        FIXME("Strange error set by CreateProcess: %d\n", ret );
        ret = 11;
    }
    HeapFree( GetProcessHeap(), 0, cmdline );
    return ret;
}


/**********************************************************************
 *	    LoadModule    (KERNEL32.@)
 */
DWORD WINAPI LoadModule( LPCSTR name, LPVOID paramBlock )
{
    LOADPARMS32 *params = paramBlock;
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    DWORD ret;
    LPSTR cmdline, p;
    char filename[MAX_PATH];
    BYTE len;

    if (!name) return ERROR_FILE_NOT_FOUND;

    if (!SearchPathA( NULL, name, ".exe", sizeof(filename), filename, NULL ) &&
        !SearchPathA( NULL, name, NULL, sizeof(filename), filename, NULL ))
        return GetLastError();

    len = (BYTE)params->lpCmdLine[0];
    if (!(cmdline = HeapAlloc( GetProcessHeap(), 0, strlen(filename) + len + 2 )))
        return ERROR_NOT_ENOUGH_MEMORY;

    strcpy( cmdline, filename );
    p = cmdline + strlen(cmdline);
    *p++ = ' ';
    memcpy( p, params->lpCmdLine + 1, len );
    p[len] = 0;

    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);
    if (params->lpCmdShow)
    {
        startup.dwFlags = STARTF_USESHOWWINDOW;
        startup.wShowWindow = ((WORD *)params->lpCmdShow)[1];
    }

    if (CreateProcessA( filename, cmdline, NULL, NULL, FALSE, 0,
                        params->lpEnvAddress, NULL, &startup, &info ))
    {
        /* Give 30 seconds to the app to come up */
        if (wait_input_idle( info.hProcess, 30000 ) == WAIT_FAILED)
            WARN("WaitForInputIdle failed: Error %d\n", GetLastError() );
        ret = 33;
        /* Close off the handles */
        CloseHandle( info.hThread );
        CloseHandle( info.hProcess );
    }
    else if ((ret = GetLastError()) >= 32)
    {
        FIXME("Strange error set by CreateProcess: %u\n", ret );
        ret = 11;
    }

    HeapFree( GetProcessHeap(), 0, cmdline );
    return ret;
}


/***********************************************************************
 *           ExitProcess   (KERNEL32.@)
 *
 * Exits the current process.
 *
 * PARAMS
 *  status [I] Status code to exit with.
 *
 * RETURNS
 *  Nothing.
 */
#ifdef __i386__
__ASM_STDCALL_FUNC( ExitProcess, 4, /* Shrinker depend on this particular ExitProcess implementation */
                   "pushl %ebp\n\t"
                   ".byte 0x8B, 0xEC\n\t" /* movl %esp, %ebp */
                   ".byte 0x6A, 0x00\n\t" /* pushl $0 */
                   ".byte 0x68, 0x00, 0x00, 0x00, 0x00\n\t" /* pushl $0 - 4 bytes immediate */
                   "pushl 8(%ebp)\n\t"
                   "call " __ASM_STDCALL("RtlExitUserProcess",4) "\n\t"
                   "leave\n\t"
                   "ret $4" )
#else

void WINAPI ExitProcess( DWORD status )
{
    RtlExitUserProcess( status );
}

#endif

/***********************************************************************
 * GetExitCodeProcess           [KERNEL32.@]
 *
 * Gets termination status of specified process.
 *
 * PARAMS
 *   hProcess   [in]  Handle to the process.
 *   lpExitCode [out] Address to receive termination status.
 *
 * RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 */
BOOL WINAPI GetExitCodeProcess( HANDLE hProcess, LPDWORD lpExitCode )
{
    PROCESS_BASIC_INFORMATION pbi;

    if (!set_ntstatus( NtQueryInformationProcess( hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), NULL )))
        return FALSE;
    if (lpExitCode) *lpExitCode = pbi.ExitStatus;
    return TRUE;
}


/**************************************************************************
 *           FatalExit   (KERNEL32.@)
 */
void WINAPI FatalExit( int code )
{
    WARN( "FatalExit\n" );
    ExitProcess( code );
}


/***********************************************************************
 *           GetProcessFlags    (KERNEL32.@)
 */
DWORD WINAPI GetProcessFlags( DWORD processid )
{
    IMAGE_NT_HEADERS *nt;
    DWORD flags = 0;

    if (processid && processid != GetCurrentProcessId()) return 0;

    if ((nt = RtlImageNtHeader( NtCurrentTeb()->Peb->ImageBaseAddress )))
    {
        if (nt->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)
            flags |= PDB32_CONSOLE_PROC;
    }
    if (!AreFileApisANSI()) flags |= PDB32_FILE_APIS_OEM;
    if (IsDebuggerPresent()) flags |= PDB32_DEBUGGED;
    return flags;
}


/***********************************************************************
 *           ConvertToGlobalHandle  (KERNEL32.@)
 */
HANDLE WINAPI ConvertToGlobalHandle(HANDLE hSrc)
{
    HANDLE ret = INVALID_HANDLE_VALUE;
    DuplicateHandle( GetCurrentProcess(), hSrc, GetCurrentProcess(), &ret, 0, FALSE,
                     DUP_HANDLE_MAKE_GLOBAL | DUP_HANDLE_SAME_ACCESS | DUP_HANDLE_CLOSE_SOURCE );
    return ret;
}


/***********************************************************************
 *           SetHandleContext   (KERNEL32.@)
 */
BOOL WINAPI SetHandleContext(HANDLE hnd,DWORD context)
{
    FIXME("(%p,%d), stub. In case this got called by WSOCK32/WS2_32: "
          "the external WINSOCK DLLs won't work with WINE, don't use them.\n",hnd,context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *           GetHandleContext   (KERNEL32.@)
 */
DWORD WINAPI GetHandleContext(HANDLE hnd)
{
    FIXME("(%p), stub. In case this got called by WSOCK32/WS2_32: "
          "the external WINSOCK DLLs won't work with WINE, don't use them.\n",hnd);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/***********************************************************************
 *           CreateSocketHandle   (KERNEL32.@)
 */
HANDLE WINAPI CreateSocketHandle(void)
{
    FIXME("(), stub. In case this got called by WSOCK32/WS2_32: "
          "the external WINSOCK DLLs won't work with WINE, don't use them.\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return INVALID_HANDLE_VALUE;
}


/***********************************************************************
 *          SetProcessAffinityMask   (KERNEL32.@)
 */
BOOL WINAPI SetProcessAffinityMask( HANDLE hProcess, DWORD_PTR affmask )
{
    return set_ntstatus( NtSetInformationProcess( hProcess, ProcessAffinityMask,
                                                  &affmask, sizeof(DWORD_PTR) ));
}


/**********************************************************************
 *          GetProcessAffinityMask    (KERNEL32.@)
 */
BOOL WINAPI GetProcessAffinityMask( HANDLE hProcess, PDWORD_PTR process_mask, PDWORD_PTR system_mask )
{
    if (process_mask)
    {
        if (!set_ntstatus( NtQueryInformationProcess( hProcess, ProcessAffinityMask,
                                                      process_mask, sizeof(*process_mask), NULL )))
            return FALSE;
    }
    if (system_mask)
    {
        SYSTEM_BASIC_INFORMATION info;

        if (!set_ntstatus( NtQuerySystemInformation( SystemBasicInformation, &info, sizeof(info), NULL )))
            return FALSE;
        *system_mask = info.ActiveProcessorsAffinityMask;
    }
    return TRUE;
}


/***********************************************************************
 *           GetProcessVersion    (KERNEL32.@)
 */
DWORD WINAPI GetProcessVersion( DWORD pid )
{
    HANDLE process;
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION pbi;
    SIZE_T count;
    PEB peb;
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS nt;
    DWORD ver = 0;

    if (!pid || pid == GetCurrentProcessId())
    {
        IMAGE_NT_HEADERS *pnt;

        if ((pnt = RtlImageNtHeader( NtCurrentTeb()->Peb->ImageBaseAddress )))
            return ((pnt->OptionalHeader.MajorSubsystemVersion << 16) |
                    pnt->OptionalHeader.MinorSubsystemVersion);
        return 0;
    }

    process = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!process) return 0;

    status = NtQueryInformationProcess(process, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
    if (status) goto err;

    status = NtReadVirtualMemory(process, pbi.PebBaseAddress, &peb, sizeof(peb), &count);
    if (status || count != sizeof(peb)) goto err;

    memset(&dos, 0, sizeof(dos));
    status = NtReadVirtualMemory(process, peb.ImageBaseAddress, &dos, sizeof(dos), &count);
    if (status || count != sizeof(dos)) goto err;
    if (dos.e_magic != IMAGE_DOS_SIGNATURE) goto err;

    memset(&nt, 0, sizeof(nt));
    status = NtReadVirtualMemory(process, (char *)peb.ImageBaseAddress + dos.e_lfanew, &nt, sizeof(nt), &count);
    if (status || count != sizeof(nt)) goto err;
    if (nt.Signature != IMAGE_NT_SIGNATURE) goto err;

    ver = MAKELONG(nt.OptionalHeader.MinorSubsystemVersion, nt.OptionalHeader.MajorSubsystemVersion);

err:
    CloseHandle(process);

    if (status != STATUS_SUCCESS)
        SetLastError(RtlNtStatusToDosError(status));

    return ver;
}


/***********************************************************************
 *		SetProcessWorkingSetSize	[KERNEL32.@]
 * Sets the min/max working set sizes for a specified process.
 *
 * PARAMS
 *    process  [I] Handle to the process of interest
 *    minset   [I] Specifies minimum working set size
 *    maxset   [I] Specifies maximum working set size
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI SetProcessWorkingSetSize(HANDLE process, SIZE_T minset, SIZE_T maxset)
{
    return SetProcessWorkingSetSizeEx(process, minset, maxset, 0);
}

/***********************************************************************
 *           K32EmptyWorkingSet (KERNEL32.@)
 */
BOOL WINAPI K32EmptyWorkingSet(HANDLE hProcess)
{
    return SetProcessWorkingSetSize(hProcess, (SIZE_T)-1, (SIZE_T)-1);
}


/***********************************************************************
 *           GetProcessWorkingSetSize    (KERNEL32.@)
 */
BOOL WINAPI GetProcessWorkingSetSize(HANDLE process, SIZE_T *minset, SIZE_T *maxset)
{
    return GetProcessWorkingSetSizeEx(process, minset, maxset, NULL);
}


/******************************************************************
 *		GetProcessIoCounters (KERNEL32.@)
 */
BOOL WINAPI GetProcessIoCounters(HANDLE hProcess, PIO_COUNTERS ioc)
{
    return set_ntstatus( NtQueryInformationProcess(hProcess, ProcessIoCounters, ioc, sizeof(*ioc), NULL ));
}

/******************************************************************
 *		QueryFullProcessImageNameA (KERNEL32.@)
 */
BOOL WINAPI QueryFullProcessImageNameA(HANDLE hProcess, DWORD dwFlags, LPSTR lpExeName, PDWORD pdwSize)
{
    BOOL retval;
    DWORD pdwSizeW = *pdwSize;
    LPWSTR lpExeNameW = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, *pdwSize * sizeof(WCHAR));

    retval = QueryFullProcessImageNameW(hProcess, dwFlags, lpExeNameW, &pdwSizeW);

    if(retval)
        retval = (0 != WideCharToMultiByte(CP_ACP, 0, lpExeNameW, -1,
                               lpExeName, *pdwSize, NULL, NULL));
    if(retval)
        *pdwSize = strlen(lpExeName);

    HeapFree(GetProcessHeap(), 0, lpExeNameW);
    return retval;
}

/******************************************************************
 *		QueryFullProcessImageNameW (KERNEL32.@)
 */
BOOL WINAPI QueryFullProcessImageNameW(HANDLE hProcess, DWORD dwFlags, LPWSTR lpExeName, PDWORD pdwSize)
{
    BYTE buffer[sizeof(UNICODE_STRING) + MAX_PATH*sizeof(WCHAR)];  /* this buffer should be enough */
    UNICODE_STRING *dynamic_buffer = NULL;
    UNICODE_STRING *result = NULL;
    NTSTATUS status;
    DWORD needed;

    /* FIXME: On Windows, ProcessImageFileName return an NT path. In Wine it
     * is a DOS path and we depend on this. */
    status = NtQueryInformationProcess(hProcess, ProcessImageFileName, buffer,
                                       sizeof(buffer) - sizeof(WCHAR), &needed);
    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        dynamic_buffer = HeapAlloc(GetProcessHeap(), 0, needed + sizeof(WCHAR));
        status = NtQueryInformationProcess(hProcess, ProcessImageFileName, (LPBYTE)dynamic_buffer, needed, &needed);
        result = dynamic_buffer;
    }
    else
        result = (PUNICODE_STRING)buffer;

    if (status) goto cleanup;

    if (dwFlags & PROCESS_NAME_NATIVE)
    {
        WCHAR drive[3];
        WCHAR device[1024];
        DWORD ntlen, devlen;

        if (result->Buffer[1] != ':' || result->Buffer[0] < 'A' || result->Buffer[0] > 'Z')
        {
            /* We cannot convert it to an NT device path so fail */
            status = STATUS_NO_SUCH_DEVICE;
            goto cleanup;
        }

        /* Find this drive's NT device path */
        drive[0] = result->Buffer[0];
        drive[1] = ':';
        drive[2] = 0;
        if (!QueryDosDeviceW(drive, device, ARRAY_SIZE(device)))
        {
            status = STATUS_NO_SUCH_DEVICE;
            goto cleanup;
        }

        devlen = lstrlenW(device);
        ntlen = devlen + (result->Length/sizeof(WCHAR) - 2);
        if (ntlen + 1 > *pdwSize)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            goto cleanup;
        }
        *pdwSize = ntlen;

        memcpy(lpExeName, device, devlen * sizeof(*device));
        memcpy(lpExeName + devlen, result->Buffer + 2, result->Length - 2 * sizeof(WCHAR));
        lpExeName[*pdwSize] = 0;
        TRACE("NT path: %s\n", debugstr_w(lpExeName));
    }
    else
    {
        if (result->Length/sizeof(WCHAR) + 1 > *pdwSize)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            goto cleanup;
        }

        *pdwSize = result->Length/sizeof(WCHAR);
        memcpy( lpExeName, result->Buffer, result->Length );
        lpExeName[*pdwSize] = 0;
    }

cleanup:
    HeapFree(GetProcessHeap(), 0, dynamic_buffer);
    return set_ntstatus( status );
}

/***********************************************************************
 *           K32GetProcessImageFileNameA (KERNEL32.@)
 */
DWORD WINAPI K32GetProcessImageFileNameA( HANDLE process, LPSTR file, DWORD size )
{
    return QueryFullProcessImageNameA(process, PROCESS_NAME_NATIVE, file, &size) ? size : 0;
}

/***********************************************************************
 *           K32GetProcessImageFileNameW (KERNEL32.@)
 */
DWORD WINAPI K32GetProcessImageFileNameW( HANDLE process, LPWSTR file, DWORD size )
{
    return QueryFullProcessImageNameW(process, PROCESS_NAME_NATIVE, file, &size) ? size : 0;
}

/***********************************************************************
 *           K32EnumProcesses (KERNEL32.@)
 */
BOOL WINAPI K32EnumProcesses(DWORD *lpdwProcessIDs, DWORD cb, DWORD *lpcbUsed)
{
    SYSTEM_PROCESS_INFORMATION *spi;
    ULONG size = 0x4000;
    void *buf = NULL;
    NTSTATUS status;

    do {
        size *= 2;
        HeapFree(GetProcessHeap(), 0, buf);
        buf = HeapAlloc(GetProcessHeap(), 0, size);
        if (!buf)
            return FALSE;

        status = NtQuerySystemInformation(SystemProcessInformation, buf, size, NULL);
    } while(status == STATUS_INFO_LENGTH_MISMATCH);

    if (!set_ntstatus( status ))
    {
        HeapFree(GetProcessHeap(), 0, buf);
        return FALSE;
    }

    spi = buf;

    for (*lpcbUsed = 0; cb >= sizeof(DWORD); cb -= sizeof(DWORD))
    {
        *lpdwProcessIDs++ = HandleToUlong(spi->UniqueProcessId);
        *lpcbUsed += sizeof(DWORD);

        if (spi->NextEntryOffset == 0)
            break;

        spi = (SYSTEM_PROCESS_INFORMATION *)(((PCHAR)spi) + spi->NextEntryOffset);
    }

    HeapFree(GetProcessHeap(), 0, buf);
    return TRUE;
}

/***********************************************************************
 *           K32QueryWorkingSet (KERNEL32.@)
 */
BOOL WINAPI K32QueryWorkingSet( HANDLE process, LPVOID buffer, DWORD size )
{
    TRACE( "(%p, %p, %d)\n", process, buffer, size );

    return set_ntstatus( NtQueryVirtualMemory( process, NULL, MemoryWorkingSetList, buffer, size, NULL ));
}

/***********************************************************************
 *           K32QueryWorkingSetEx (KERNEL32.@)
 */
BOOL WINAPI K32QueryWorkingSetEx( HANDLE process, LPVOID buffer, DWORD size )
{
    TRACE( "(%p, %p, %d)\n", process, buffer, size );

    return set_ntstatus( NtQueryVirtualMemory( process, NULL, MemoryWorkingSetList, buffer,  size, NULL ));
}

/***********************************************************************
 *           K32GetProcessMemoryInfo (KERNEL32.@)
 *
 * Retrieve memory usage information for a given process
 *
 */
BOOL WINAPI K32GetProcessMemoryInfo(HANDLE process,
                                    PPROCESS_MEMORY_COUNTERS pmc, DWORD cb)
{
    VM_COUNTERS vmc;

    if (cb < sizeof(PROCESS_MEMORY_COUNTERS))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    if (!set_ntstatus( NtQueryInformationProcess( process, ProcessVmCounters, &vmc, sizeof(vmc), NULL )))
        return FALSE;

    pmc->cb = sizeof(PROCESS_MEMORY_COUNTERS);
    pmc->PageFaultCount = vmc.PageFaultCount;
    pmc->PeakWorkingSetSize = vmc.PeakWorkingSetSize;
    pmc->WorkingSetSize = vmc.WorkingSetSize;
    pmc->QuotaPeakPagedPoolUsage = vmc.QuotaPeakPagedPoolUsage;
    pmc->QuotaPagedPoolUsage = vmc.QuotaPagedPoolUsage;
    pmc->QuotaPeakNonPagedPoolUsage = vmc.QuotaPeakNonPagedPoolUsage;
    pmc->QuotaNonPagedPoolUsage = vmc.QuotaNonPagedPoolUsage;
    pmc->PagefileUsage = vmc.PagefileUsage;
    pmc->PeakPagefileUsage = vmc.PeakPagefileUsage;

    return TRUE;
}

/***********************************************************************
 * ProcessIdToSessionId   (KERNEL32.@)
 * This function is available on Terminal Server 4SP4 and Windows 2000
 */
BOOL WINAPI ProcessIdToSessionId( DWORD procid, DWORD *sessionid_ptr )
{
    if (procid != GetCurrentProcessId())
        FIXME("Unsupported for other processes.\n");

    *sessionid_ptr = NtCurrentTeb()->Peb->SessionId;
    return TRUE;
}


/***********************************************************************
 *		RegisterServiceProcess (KERNEL32.@)
 *
 * A service process calls this function to ensure that it continues to run
 * even after a user logged off.
 */
DWORD WINAPI RegisterServiceProcess(DWORD dwProcessId, DWORD dwType)
{
    /* I don't think that Wine needs to do anything in this function */
    return 1; /* success */
}


/***********************************************************************
 *           GetCurrentProcess   (KERNEL32.@)
 *
 * Get a handle to the current process.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  A handle representing the current process.
 */
HANDLE WINAPI KERNEL32_GetCurrentProcess(void)
{
    return (HANDLE)~(ULONG_PTR)0;
}

/***********************************************************************
 *           CmdBatNotification   (KERNEL32.@)
 *
 * Notifies the system that a batch file has started or finished.
 *
 * PARAMS
 *  bBatchRunning [I]  TRUE if a batch file has started or 
 *                     FALSE if a batch file has finished executing.
 *
 * RETURNS
 *  Unknown.
 */
BOOL WINAPI CmdBatNotification( BOOL bBatchRunning )
{
    FIXME("%d\n", bBatchRunning);
    return FALSE;
}

/***********************************************************************
 *           RegisterApplicationRestart       (KERNEL32.@)
 */
HRESULT WINAPI RegisterApplicationRestart(PCWSTR pwzCommandLine, DWORD dwFlags)
{
    FIXME("(%s,%d)\n", debugstr_w(pwzCommandLine), dwFlags);

    return S_OK;
}

/**********************************************************************
 *           WTSGetActiveConsoleSessionId     (KERNEL32.@)
 */
DWORD WINAPI WTSGetActiveConsoleSessionId(void)
{
    static int once;
    if (!once++) FIXME("stub\n");
    /* Return current session id. */
    return NtCurrentTeb()->Peb->SessionId;
}

/**********************************************************************
 *           GetSystemDEPPolicy     (KERNEL32.@)
 */
DEP_SYSTEM_POLICY_TYPE WINAPI GetSystemDEPPolicy(void)
{
    FIXME("stub\n");
    return OptIn;
}

/**********************************************************************
 *           SetProcessDEPPolicy     (KERNEL32.@)
 */
BOOL WINAPI SetProcessDEPPolicy(DWORD newDEP)
{
    FIXME("(%d): stub\n", newDEP);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**********************************************************************
 *           ApplicationRecoveryFinished     (KERNEL32.@)
 */
VOID WINAPI ApplicationRecoveryFinished(BOOL success)
{
    FIXME(": stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/**********************************************************************
 *           ApplicationRecoveryInProgress     (KERNEL32.@)
 */
HRESULT WINAPI ApplicationRecoveryInProgress(PBOOL canceled)
{
    FIXME(":%p stub\n", canceled);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return E_FAIL;
}

/**********************************************************************
 *           RegisterApplicationRecoveryCallback     (KERNEL32.@)
 */
HRESULT WINAPI RegisterApplicationRecoveryCallback(APPLICATION_RECOVERY_CALLBACK callback, PVOID param, DWORD pingint, DWORD flags)
{
    FIXME("%p, %p, %d, %d: stub\n", callback, param, pingint, flags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return E_FAIL;
}

/***********************************************************************
 *           GetApplicationRestartSettings       (KERNEL32.@)
 */
HRESULT WINAPI GetApplicationRestartSettings(HANDLE process, WCHAR *cmdline, DWORD *size, DWORD *flags)
{
    FIXME("%p, %p, %p, %p)\n", process, cmdline, size, flags);
    return E_NOTIMPL;
}

/**********************************************************************
 *           GetNumaNodeProcessorMask     (KERNEL32.@)
 */
BOOL WINAPI GetNumaNodeProcessorMask(UCHAR node, PULONGLONG mask)
{
    FIXME("(%c %p): stub\n", node, mask);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**********************************************************************
 *           GetNumaAvailableMemoryNode     (KERNEL32.@)
 */
BOOL WINAPI GetNumaAvailableMemoryNode(UCHAR node, PULONGLONG available_bytes)
{
    FIXME("(%c %p): stub\n", node, available_bytes);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**********************************************************************
 *           GetNumaAvailableMemoryNodeEx     (KERNEL32.@)
 */
BOOL WINAPI GetNumaAvailableMemoryNodeEx(USHORT node, PULONGLONG available_bytes)
{
    FIXME("(%hu %p): stub\n", node, available_bytes);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           GetNumaProcessorNode (KERNEL32.@)
 */
BOOL WINAPI GetNumaProcessorNode(UCHAR processor, PUCHAR node)
{
    SYSTEM_INFO si;

    TRACE("(%d, %p)\n", processor, node);

    GetSystemInfo( &si );
    if (processor < si.dwNumberOfProcessors)
    {
        *node = 0;
        return TRUE;
    }

    *node = 0xFF;
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

/***********************************************************************
 *           GetNumaProcessorNodeEx (KERNEL32.@)
 */
BOOL WINAPI GetNumaProcessorNodeEx(PPROCESSOR_NUMBER processor, PUSHORT node_number)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           GetNumaProximityNode (KERNEL32.@)
 */
BOOL WINAPI GetNumaProximityNode(ULONG  proximity_id, PUCHAR node_number)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**********************************************************************
 *           GetProcessDEPPolicy     (KERNEL32.@)
 */
BOOL WINAPI GetProcessDEPPolicy(HANDLE process, LPDWORD flags, PBOOL permanent)
{
    ULONG dep_flags;

    TRACE("(%p %p %p)\n", process, flags, permanent);

    if (!set_ntstatus( NtQueryInformationProcess( GetCurrentProcess(), ProcessExecuteFlags,
                                                  &dep_flags, sizeof(dep_flags), NULL )))
        return FALSE;

    if (flags)
    {
        *flags = 0;
        if (dep_flags & MEM_EXECUTE_OPTION_DISABLE)
            *flags |= PROCESS_DEP_ENABLE;
        if (dep_flags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION)
            *flags |= PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION;
    }

    if (permanent) *permanent = (dep_flags & MEM_EXECUTE_OPTION_PERMANENT) != 0;
    return TRUE;
}

/**********************************************************************
 *           FlushProcessWriteBuffers     (KERNEL32.@)
 */
VOID WINAPI FlushProcessWriteBuffers(void)
{
    static int once = 0;

    if (!once++)
        FIXME(": stub\n");
}

/***********************************************************************
 *           UnregisterApplicationRestart       (KERNEL32.@)
 */
HRESULT WINAPI UnregisterApplicationRestart(void)
{
    FIXME(": stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return S_OK;
}

/***********************************************************************
 *           CreateUmsCompletionList   (KERNEL32.@)
 */
BOOL WINAPI CreateUmsCompletionList(PUMS_COMPLETION_LIST *list)
{
    FIXME( "%p: stub\n", list );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           CreateUmsThreadContext   (KERNEL32.@)
 */
BOOL WINAPI CreateUmsThreadContext(PUMS_CONTEXT *ctx)
{
    FIXME( "%p: stub\n", ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           DeleteUmsCompletionList   (KERNEL32.@)
 */
BOOL WINAPI DeleteUmsCompletionList(PUMS_COMPLETION_LIST list)
{
    FIXME( "%p: stub\n", list );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           DeleteUmsThreadContext   (KERNEL32.@)
 */
BOOL WINAPI DeleteUmsThreadContext(PUMS_CONTEXT ctx)
{
    FIXME( "%p: stub\n", ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           DequeueUmsCompletionListItems   (KERNEL32.@)
 */
BOOL WINAPI DequeueUmsCompletionListItems(void *list, DWORD timeout, PUMS_CONTEXT *ctx)
{
    FIXME( "%p,%08x,%p: stub\n", list, timeout, ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           EnterUmsSchedulingMode   (KERNEL32.@)
 */
BOOL WINAPI EnterUmsSchedulingMode(UMS_SCHEDULER_STARTUP_INFO *info)
{
    FIXME( "%p: stub\n", info );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           ExecuteUmsThread   (KERNEL32.@)
 */
BOOL WINAPI ExecuteUmsThread(PUMS_CONTEXT ctx)
{
    FIXME( "%p: stub\n", ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           GetCurrentUmsThread   (KERNEL32.@)
 */
PUMS_CONTEXT WINAPI GetCurrentUmsThread(void)
{
    FIXME( "stub\n" );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           GetNextUmsListItem   (KERNEL32.@)
 */
PUMS_CONTEXT WINAPI GetNextUmsListItem(PUMS_CONTEXT ctx)
{
    FIXME( "%p: stub\n", ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return NULL;
}

/***********************************************************************
 *           GetUmsCompletionListEvent   (KERNEL32.@)
 */
BOOL WINAPI GetUmsCompletionListEvent(PUMS_COMPLETION_LIST list, HANDLE *event)
{
    FIXME( "%p,%p: stub\n", list, event );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           QueryUmsThreadInformation   (KERNEL32.@)
 */
BOOL WINAPI QueryUmsThreadInformation(PUMS_CONTEXT ctx, UMS_THREAD_INFO_CLASS class,
                                       void *buf, ULONG length, ULONG *ret_length)
{
    FIXME( "%p,%08x,%p,%08x,%p: stub\n", ctx, class, buf, length, ret_length );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           SetUmsThreadInformation   (KERNEL32.@)
 */
BOOL WINAPI SetUmsThreadInformation(PUMS_CONTEXT ctx, UMS_THREAD_INFO_CLASS class,
                                     void *buf, ULONG length)
{
    FIXME( "%p,%08x,%p,%08x: stub\n", ctx, class, buf, length );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           UmsThreadYield   (KERNEL32.@)
 */
BOOL WINAPI UmsThreadYield(void *param)
{
    FIXME( "%p: stub\n", param );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/**********************************************************************
 *           BaseFlushAppcompatCache     (KERNEL32.@)
 */
BOOL WINAPI BaseFlushAppcompatCache(void)
{
    FIXME(": stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
