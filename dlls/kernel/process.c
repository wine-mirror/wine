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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>

#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "ntstatus.h"
#include "thread.h"
#include "drive.h"
#include "file.h"
#include "heap.h"
#include "module.h"
#include "options.h"
#include "kernel_private.h"
#include "wine/server.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(process);
WINE_DECLARE_DEBUG_CHANNEL(server);
WINE_DECLARE_DEBUG_CHANNEL(relay);

static UINT process_error_mode;

static HANDLE main_exe_file;
static DWORD shutdown_flags = 0;
static DWORD shutdown_priority = 0x280;
static DWORD process_dword;
static BOOL oem_file_apis;

static unsigned int server_startticks;
int main_create_flags = 0;

/* Process flags */
#define PDB32_DEBUGGED      0x0001  /* Process is being debugged */
#define PDB32_WIN16_PROC    0x0008  /* Win16 process */
#define PDB32_DOS_PROC      0x0010  /* Dos process */
#define PDB32_CONSOLE_PROC  0x0020  /* Console process */
#define PDB32_FILE_APIS_OEM 0x0040  /* File APIs are OEM */
#define PDB32_WIN32S_PROC   0x8000  /* Win32s process */

static const WCHAR comW[] = {'.','c','o','m',0};
static const WCHAR batW[] = {'.','b','a','t',0};
static const WCHAR winevdmW[] = {'w','i','n','e','v','d','m','.','e','x','e',0};

extern void SHELL_LoadRegistry(void);
extern void VERSION_Init( const WCHAR *appname );
extern void MODULE_InitLoadPath(void);
extern void LOCALE_Init(void);

/***********************************************************************
 *           contains_path
 */
inline static int contains_path( LPCWSTR name )
{
    return ((*name && (name[1] == ':')) || strchrW(name, '/') || strchrW(name, '\\'));
}


/***************************************************************************
 *	get_builtin_path
 *
 * Get the path of a builtin module when the native file does not exist.
 */
static BOOL get_builtin_path( const WCHAR *libname, const WCHAR *ext, WCHAR *filename, UINT size )
{
    WCHAR *file_part;
    WCHAR sysdir[MAX_PATH];
    UINT len = GetSystemDirectoryW( sysdir, MAX_PATH );

    if (contains_path( libname ))
    {
        if (RtlGetFullPathName_U( libname, size * sizeof(WCHAR),
                                  filename, &file_part ) > size * sizeof(WCHAR))
            return FALSE;  /* too long */

        if (strncmpiW( filename, sysdir, len ) || filename[len] != '\\')
            return FALSE;
        while (filename[len] == '\\') len++;
        if (filename != file_part) return FALSE;
    }
    else
    {
        if (strlenW(libname) + len + 2 >= size) return FALSE;  /* too long */
        memcpy( filename, sysdir, len * sizeof(WCHAR) );
        file_part = filename + len;
        if (file_part > filename && file_part[-1] != '\\') *file_part++ = '\\';
        strcpyW( file_part, libname );
    }
    if (ext && !strchrW( file_part, '.' ))
    {
        if (file_part + strlenW(file_part) + strlenW(ext) + 1 > filename + size)
            return FALSE;  /* too long */
        strcatW( file_part, ext );
    }
    return TRUE;
}


/***********************************************************************
 *           open_builtin_exe_file
 *
 * Open an exe file for a builtin exe.
 */
static void *open_builtin_exe_file( const WCHAR *name, char *error, int error_size,
                                    int test_only, int *file_exists )
{
    char exename[MAX_PATH];
    WCHAR *p;
    UINT i, len;

    if ((p = strrchrW( name, '/' ))) name = p + 1;
    if ((p = strrchrW( name, '\\' ))) name = p + 1;

    /* we don't want to depend on the current codepage here */
    len = strlenW( name ) + 1;
    if (len >= sizeof(exename)) return NULL;
    for (i = 0; i < len; i++)
    {
        if (name[i] > 127) return NULL;
        exename[i] = (char)name[i];
        if (exename[i] >= 'A' && exename[i] <= 'Z') exename[i] += 'a' - 'A';
    }
    return wine_dll_load_main_exe( exename, error, error_size, test_only, file_exists );
}


/***********************************************************************
 *           open_exe_file
 *
 * Open a specific exe file, taking load order into account.
 * Returns the file handle or 0 for a builtin exe.
 */
static HANDLE open_exe_file( const WCHAR *name )
{
    enum loadorder_type loadorder[LOADORDER_NTYPES];
    WCHAR buffer[MAX_PATH];
    HANDLE handle;
    int i, file_exists;

    TRACE("looking for %s\n", debugstr_w(name) );

    if ((handle = CreateFileW( name, GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, 0, 0 )) == INVALID_HANDLE_VALUE)
    {
        /* file doesn't exist, check for builtin */
        if (!contains_path( name )) goto error;
        if (!get_builtin_path( name, NULL, buffer, sizeof(buffer) )) goto error;
        name = buffer;
    }

    MODULE_GetLoadOrderW( loadorder, NULL, name, TRUE );

    for(i = 0; i < LOADORDER_NTYPES; i++)
    {
        if (loadorder[i] == LOADORDER_INVALID) break;
        switch(loadorder[i])
        {
        case LOADORDER_DLL:
            TRACE( "Trying native exe %s\n", debugstr_w(name) );
            if (handle != INVALID_HANDLE_VALUE) return handle;
            break;
        case LOADORDER_BI:
            TRACE( "Trying built-in exe %s\n", debugstr_w(name) );
            open_builtin_exe_file( name, NULL, 0, 1, &file_exists );
            if (file_exists)
            {
                if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
                return 0;
            }
        default:
            break;
        }
    }
    if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);

 error:
    SetLastError( ERROR_FILE_NOT_FOUND );
    return INVALID_HANDLE_VALUE;
}


/***********************************************************************
 *           find_exe_file
 *
 * Open an exe file, and return the full name and file handle.
 * Returns FALSE if file could not be found.
 * If file exists but cannot be opened, returns TRUE and set handle to INVALID_HANDLE_VALUE.
 * If file is a builtin exe, returns TRUE and sets handle to 0.
 */
static BOOL find_exe_file( const WCHAR *name, WCHAR *buffer, int buflen, HANDLE *handle )
{
    static const WCHAR exeW[] = {'.','e','x','e',0};

    enum loadorder_type loadorder[LOADORDER_NTYPES];
    int i, file_exists;

    TRACE("looking for %s\n", debugstr_w(name) );

    if (!SearchPathW( NULL, name, exeW, buflen, buffer, NULL ) &&
        !get_builtin_path( name, exeW, buffer, buflen ))
    {
        /* no builtin found, try native without extension in case it is a Unix app */

        if (SearchPathW( NULL, name, NULL, buflen, buffer, NULL ))
        {
            TRACE( "Trying native/Unix binary %s\n", debugstr_w(buffer) );
            if ((*handle = CreateFileW( buffer, GENERIC_READ, FILE_SHARE_READ,
                                        NULL, OPEN_EXISTING, 0, 0 )) != INVALID_HANDLE_VALUE)
                return TRUE;
        }
        return FALSE;
    }

    MODULE_GetLoadOrderW( loadorder, NULL, buffer, TRUE );

    for(i = 0; i < LOADORDER_NTYPES; i++)
    {
        if (loadorder[i] == LOADORDER_INVALID) break;
        switch(loadorder[i])
        {
        case LOADORDER_DLL:
            TRACE( "Trying native exe %s\n", debugstr_w(buffer) );
            if ((*handle = CreateFileW( buffer, GENERIC_READ, FILE_SHARE_READ,
                                        NULL, OPEN_EXISTING, 0, 0 )) != INVALID_HANDLE_VALUE)
                return TRUE;
            if (GetLastError() != ERROR_FILE_NOT_FOUND) return TRUE;
            break;
        case LOADORDER_BI:
            TRACE( "Trying built-in exe %s\n", debugstr_w(buffer) );
            open_builtin_exe_file( buffer, NULL, 0, 1, &file_exists );
            if (file_exists)
            {
                *handle = 0;
                return TRUE;
            }
            break;
        default:
            break;
        }
    }
    SetLastError( ERROR_FILE_NOT_FOUND );
    return FALSE;
}


/**********************************************************************
 *           load_pe_exe
 *
 * Load a PE format EXE file.
 */
static HMODULE load_pe_exe( const WCHAR *name, HANDLE file )
{
    IMAGE_NT_HEADERS *nt;
    HANDLE mapping;
    void *module;
    OBJECT_ATTRIBUTES attr;
    LARGE_INTEGER size;
    DWORD len = 0;
    UINT drive_type;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = 0;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;
    size.QuadPart = 0;

    if (NtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ,
                         &attr, &size, 0, SEC_IMAGE, file ) != STATUS_SUCCESS)
        return NULL;

    module = NULL;
    if (NtMapViewOfSection( mapping, GetCurrentProcess(), &module, 0, 0, &size, &len,
                            ViewShare, 0, PAGE_READONLY ) != STATUS_SUCCESS)
        return NULL;

    NtClose( mapping );

    /* virus check */
    nt = RtlImageNtHeader( module );
    if (nt->OptionalHeader.AddressOfEntryPoint)
    {
        if (!RtlImageRvaToSection( nt, module, nt->OptionalHeader.AddressOfEntryPoint ))
            MESSAGE("VIRUS WARNING: PE module %s has an invalid entrypoint (0x%08lx) "
                    "outside all sections (possibly infected by Tchernobyl/SpaceFiller virus)!\n",
                    debugstr_w(name), nt->OptionalHeader.AddressOfEntryPoint );
    }

    drive_type = GetDriveTypeW( name );
    /* don't keep the file handle open on removable media */
    if (drive_type == DRIVE_REMOVABLE || drive_type == DRIVE_CDROM)
    {
        CloseHandle( main_exe_file );
        main_exe_file = 0;
    }

    return module;
}

/***********************************************************************
 *           build_initial_environment
 *
 * Build the Win32 environment from the Unix environment
 */
static BOOL build_initial_environment( char **environ )
{
    ULONG size = 1;
    char **e;
    WCHAR *p, *endptr;
    void *ptr;

    /* Compute the total size of the Unix environment */
    for (e = environ; *e; e++)
    {
        if (!memcmp(*e, "PATH=", 5)) continue;
        size += MultiByteToWideChar( CP_UNIXCP, 0, *e, -1, NULL, 0 );
    }
    size *= sizeof(WCHAR);

    /* Now allocate the environment */
    if (NtAllocateVirtualMemory(NtCurrentProcess(), &ptr, 0, &size,
                                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE) != STATUS_SUCCESS)
        return FALSE;

    NtCurrentTeb()->Peb->ProcessParameters->Environment = p = ptr;
    endptr = p + size / sizeof(WCHAR);

    /* And fill it with the Unix environment */
    for (e = environ; *e; e++)
    {
        char *str = *e;
        /* skip Unix PATH and store WINEPATH as PATH */
        if (!memcmp(str, "PATH=", 5)) continue;
        if (!memcmp(str, "WINEPATH=", 9 )) str += 4;
        MultiByteToWideChar( CP_UNIXCP, 0, str, -1, p, endptr - p );
        p += strlenW(p) + 1;
    }
    *p = 0;
    return TRUE;
}


/***********************************************************************
 *              set_library_wargv
 *
 * Set the Wine library Unicode argv global variables.
 */
static void set_library_wargv( char **argv )
{
    int argc;
    WCHAR *p;
    WCHAR **wargv;
    DWORD total = 0;

    for (argc = 0; argv[argc]; argc++)
        total += MultiByteToWideChar( CP_UNIXCP, 0, argv[argc], -1, NULL, 0 );

    wargv = RtlAllocateHeap( GetProcessHeap(), 0,
                             total * sizeof(WCHAR) + (argc + 1) * sizeof(*wargv) );
    p = (WCHAR *)(wargv + argc + 1);
    for (argc = 0; argv[argc]; argc++)
    {
        DWORD reslen = MultiByteToWideChar( CP_UNIXCP, 0, argv[argc], -1, p, total );
        wargv[argc] = p;
        p += reslen;
        total -= reslen;
    }
    wargv[argc] = NULL;
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
 * - '\'s that are not followed by a '"' can be left as is
 *   'a\b'   == 'a\b'
 *   'a\\b'  == 'a\\b'
 */
static BOOL build_command_line( WCHAR **argv )
{
    int len;
    WCHAR **arg;
    LPWSTR p;
    RTL_USER_PROCESS_PARAMETERS* rupp = NtCurrentTeb()->Peb->ProcessParameters;

    if (rupp->CommandLine.Buffer) return TRUE; /* already got it from the server */

    len = 0;
    for (arg = argv; *arg; arg++)
    {
        int has_space,bcount;
        WCHAR* a;

        has_space=0;
        bcount=0;
        a=*arg;
        if( !*a ) has_space=1;
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

    if (!(rupp->CommandLine.Buffer = RtlAllocateHeap( GetProcessHeap(), 0, len * sizeof(WCHAR))))
        return FALSE;

    p = rupp->CommandLine.Buffer;
    rupp->CommandLine.Length = (len - 1) * sizeof(WCHAR);
    rupp->CommandLine.MaximumLength = len * sizeof(WCHAR);
    for (arg = argv; *arg; arg++)
    {
        int has_space,has_quote;
        WCHAR* a;

        /* Check for quotes and spaces in this argument */
        has_space=has_quote=0;
        a=*arg;
        if( !*a ) has_space=1;
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
            WCHAR* a;

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
            WCHAR* x = *arg;
            while ((*p=*x++)) p++;
        }
        if (has_space)
            *p++='"';
        *p++=' ';
    }
    if (p > rupp->CommandLine.Buffer)
        p--;  /* remove last space */
    *p = '\0';

    return TRUE;
}


/* make sure the unicode string doesn't point beyond the end pointer */
static inline void fix_unicode_string( UNICODE_STRING *str, char *end_ptr )
{
    if ((char *)str->Buffer >= end_ptr)
    {
        str->Length = str->MaximumLength = 0;
        str->Buffer = NULL;
        return;
    }
    if ((char *)str->Buffer + str->MaximumLength > end_ptr)
    {
        str->MaximumLength = (end_ptr - (char *)str->Buffer) & ~(sizeof(WCHAR) - 1);
    }
    if (str->Length >= str->MaximumLength)
    {
        if (str->MaximumLength >= sizeof(WCHAR))
            str->Length = str->MaximumLength - sizeof(WCHAR);
        else
            str->Length = str->MaximumLength = 0;
    }
}


/***********************************************************************
 *           init_user_process_params
 *
 * Fill the RTL_USER_PROCESS_PARAMETERS structure from the server.
 */
static RTL_USER_PROCESS_PARAMETERS *init_user_process_params( size_t info_size )
{
    void *ptr;
    DWORD size;
    NTSTATUS status;
    RTL_USER_PROCESS_PARAMETERS *params;

    size = info_size;
    if ((status = NtAllocateVirtualMemory( NtCurrentProcess(), &ptr, NULL, &size,
                                           MEM_COMMIT, PAGE_READWRITE )) != STATUS_SUCCESS)
        return NULL;

    SERVER_START_REQ( get_startup_info )
    {
        wine_server_set_reply( req, ptr, info_size );
        wine_server_call( req );
        info_size = wine_server_reply_size( reply );
    }
    SERVER_END_REQ;

    params = ptr;
    params->Size = info_size;
    params->AllocationSize = size;

    /* make sure the strings are valid */
    fix_unicode_string( &params->CurrentDirectoryName, (char *)info_size );
    fix_unicode_string( &params->DllPath, (char *)info_size );
    fix_unicode_string( &params->ImagePathName, (char *)info_size );
    fix_unicode_string( &params->CommandLine, (char *)info_size );
    fix_unicode_string( &params->WindowTitle, (char *)info_size );
    fix_unicode_string( &params->Desktop, (char *)info_size );
    fix_unicode_string( &params->ShellInfo, (char *)info_size );
    fix_unicode_string( &params->RuntimeInfo, (char *)info_size );

    return RtlNormalizeProcessParams( params );
}


/***********************************************************************
 *           process_init
 *
 * Main process initialisation code
 */
static BOOL process_init( char *argv[], char **environ )
{
    BOOL ret;
    size_t info_size = 0;
    RTL_USER_PROCESS_PARAMETERS *params;
    PEB *peb = NtCurrentTeb()->Peb;
    HANDLE hstdin, hstdout, hstderr;

    setbuf(stdout,NULL);
    setbuf(stderr,NULL);
    setlocale(LC_CTYPE,"");

    /* Retrieve startup info from the server */
    SERVER_START_REQ( init_process )
    {
        req->peb      = peb;
        req->ldt_copy = &wine_ldt_copy;
        if ((ret = !wine_server_call_err( req )))
        {
            main_exe_file     = reply->exe_file;
            main_create_flags = reply->create_flags;
            info_size         = reply->info_size;
            server_startticks = reply->server_start;
            hstdin            = reply->hstdin;
            hstdout           = reply->hstdout;
            hstderr           = reply->hstderr;
        }
    }
    SERVER_END_REQ;
    if (!ret) return FALSE;

    if (info_size == 0)
    {
        params = peb->ProcessParameters;

        /* This is wine specific: we have no parent (we're started from unix)
         * so, create a simple console with bare handles to unix stdio 
         * input & output streams (aka simple console)
	 */
        wine_server_fd_to_handle( 0, GENERIC_READ|SYNCHRONIZE,  TRUE, &params->hStdInput );
        wine_server_fd_to_handle( 1, GENERIC_WRITE|SYNCHRONIZE, TRUE, &params->hStdOutput );
        wine_server_fd_to_handle( 2, GENERIC_WRITE|SYNCHRONIZE, TRUE, &params->hStdError );

        /* <hack: to be changed later on> */
        params->CurrentDirectoryName.Length = 3 * sizeof(WCHAR);
        params->CurrentDirectoryName.MaximumLength = RtlGetLongestNtPathLength() * sizeof(WCHAR);
        params->CurrentDirectoryName.Buffer = RtlAllocateHeap( GetProcessHeap(), 0, params->CurrentDirectoryName.MaximumLength);
        params->CurrentDirectoryName.Buffer[0] = 'C';
        params->CurrentDirectoryName.Buffer[1] = ':';
        params->CurrentDirectoryName.Buffer[2] = '\\';
        params->CurrentDirectoryName.Buffer[3] = '\0';
        /* </hack: to be changed later on> */
    }
    else
    {
        if (!(params = init_user_process_params( info_size ))) return FALSE;
        peb->ProcessParameters = params;

        /* convert value from server:
         * + 0 => INVALID_HANDLE_VALUE
         * + console handle need to be mapped
         */
        if (!hstdin)
            hstdin = INVALID_HANDLE_VALUE;
        else if (VerifyConsoleIoHandle(console_handle_map(hstdin)))
            hstdin = console_handle_map(hstdin);

        if (!hstdout)
            hstdout = INVALID_HANDLE_VALUE;
        else if (VerifyConsoleIoHandle(console_handle_map(hstdout)))
            hstdout = console_handle_map(hstdout);

        if (!hstderr)
            hstderr = INVALID_HANDLE_VALUE;
        else if (VerifyConsoleIoHandle(console_handle_map(hstderr)))
            hstderr = console_handle_map(hstderr);

        params->hStdInput  = hstdin;
        params->hStdOutput = hstdout;
        params->hStdError  = hstderr;
    }

    LOCALE_Init();

    /* Copy the parent environment */
    if (!build_initial_environment( environ )) return FALSE;

    /* Parse command line arguments */
    OPTIONS_ParseOptions( !info_size ? argv : NULL );

    /* initialise DOS drives */
    if (!DRIVE_Init()) return FALSE;

    /* initialise DOS directories */
    if (!DIR_Init()) return FALSE;

    /* registry initialisation */
    SHELL_LoadRegistry();

    /* global boot finished, the rest is process-local */
    SERVER_START_REQ( boot_done )
    {
        req->debug_level = TRACE_ON(server);
        wine_server_call( req );
    }
    SERVER_END_REQ;

    return TRUE;
}


/***********************************************************************
 *           start_process
 *
 * Startup routine of a new process. Runs on the new process stack.
 */
static void start_process( void *arg )
{
    __TRY
    {
        PEB *peb = NtCurrentTeb()->Peb;
        IMAGE_NT_HEADERS *nt;
        LPTHREAD_START_ROUTINE entry;

        LdrInitializeThunk( main_exe_file, CreateFileW, 0, 0 );

        nt = RtlImageNtHeader( peb->ImageBaseAddress );
        entry = (LPTHREAD_START_ROUTINE)((char *)peb->ImageBaseAddress +
                                         nt->OptionalHeader.AddressOfEntryPoint);

        if (TRACE_ON(relay))
            DPRINTF( "%04lx:Starting process %s (entryproc=%p)\n", GetCurrentThreadId(),
                     debugstr_w(peb->ProcessParameters->ImagePathName.Buffer), entry );

        SetLastError( 0 );  /* clear error code */
        if (peb->BeingDebugged) DbgBreakPoint();
        ExitProcess( entry( peb ) );
    }
    __EXCEPT(UnhandledExceptionFilter)
    {
        TerminateThread( GetCurrentThread(), GetExceptionCode() );
    }
    __ENDTRY
}


/***********************************************************************
 *           __wine_kernel_init
 *
 * Wine initialisation: load and start the main exe file.
 */
void __wine_kernel_init(void)
{
    WCHAR *main_exe_name, *p;
    char error[1024];
    DWORD stack_size = 0;
    int file_exists;
    PEB *peb = NtCurrentTeb()->Peb;

    /* Initialize everything */
    if (!process_init( __wine_main_argv, __wine_main_environ )) exit(1);
    /* update argc in case options have been removed */
    for (__wine_main_argc = 0; __wine_main_argv[__wine_main_argc]; __wine_main_argc++) /*nothing*/;

    __wine_main_argv++;  /* remove argv[0] (wine itself) */
    __wine_main_argc--;

    if (!(main_exe_name = peb->ProcessParameters->ImagePathName.Buffer))
    {
        WCHAR buffer[MAX_PATH];
        WCHAR exe_nameW[MAX_PATH];

        if (!__wine_main_argv[0]) OPTIONS_Usage();

        MultiByteToWideChar( CP_UNIXCP, 0, __wine_main_argv[0], -1, exe_nameW, MAX_PATH );
        if (!find_exe_file( exe_nameW, buffer, MAX_PATH, &main_exe_file ))
        {
            MESSAGE( "wine: cannot find '%s'\n", __wine_main_argv[0] );
            ExitProcess(1);
        }
        if (main_exe_file == INVALID_HANDLE_VALUE)
        {
            MESSAGE( "wine: cannot open %s\n", debugstr_w(main_exe_name) );
            ExitProcess(1);
        }
        RtlCreateUnicodeString( &peb->ProcessParameters->ImagePathName, buffer );
        main_exe_name = peb->ProcessParameters->ImagePathName.Buffer;
    }

    TRACE( "starting process name=%s file=%p argv[0]=%s\n",
           debugstr_w(main_exe_name), main_exe_file, debugstr_a(__wine_main_argv[0]) );

    MODULE_InitLoadPath();
    VERSION_Init( main_exe_name );

    if (!main_exe_file)  /* no file handle -> Winelib app */
    {
        TRACE( "starting Winelib app %s\n", debugstr_w(main_exe_name) );
        if (open_builtin_exe_file( main_exe_name, error, sizeof(error), 0, &file_exists ))
            goto found;
        MESSAGE( "wine: cannot open builtin library for %s: %s\n",
                 debugstr_w(main_exe_name), error );
        ExitProcess(1);
    }

    switch( MODULE_GetBinaryType( main_exe_file ))
    {
    case BINARY_PE_EXE:
        TRACE( "starting Win32 binary %s\n", debugstr_w(main_exe_name) );
        if ((peb->ImageBaseAddress = load_pe_exe( main_exe_name, main_exe_file )))
            goto found;
        MESSAGE( "wine: could not load %s as Win32 binary\n", debugstr_w(main_exe_name) );
        ExitProcess(1);
    case BINARY_PE_DLL:
        MESSAGE( "wine: %s is a DLL, not an executable\n", debugstr_w(main_exe_name) );
        ExitProcess(1);
    case BINARY_UNKNOWN:
        /* check for .com extension */
        if (!(p = strrchrW( main_exe_name, '.' )) || strcmpiW( p, comW ))
        {
            MESSAGE( "wine: cannot determine executable type for %s\n",
                     debugstr_w(main_exe_name) );
            ExitProcess(1);
        }
        /* fall through */
    case BINARY_WIN16:
    case BINARY_DOS:
        TRACE( "starting Win16/DOS binary %s\n", debugstr_w(main_exe_name) );
        CloseHandle( main_exe_file );
        main_exe_file = 0;
        __wine_main_argv--;
        __wine_main_argc++;
        __wine_main_argv[0] = "winevdm.exe";
        if (open_builtin_exe_file( winevdmW, error, sizeof(error), 0, &file_exists ))
            goto found;
        MESSAGE( "wine: trying to run %s, cannot open builtin library for 'winevdm.exe': %s\n",
                 debugstr_w(main_exe_name), error );
        ExitProcess(1);
    case BINARY_OS216:
        MESSAGE( "wine: %s is an OS/2 binary, not supported\n", debugstr_w(main_exe_name) );
        ExitProcess(1);
    case BINARY_UNIX_EXE:
        MESSAGE( "wine: %s is a Unix binary, not supported\n", debugstr_w(main_exe_name) );
        ExitProcess(1);
    case BINARY_UNIX_LIB:
        {
            DOS_FULL_NAME full_name;

            TRACE( "starting Winelib app %s\n", debugstr_w(main_exe_name) );
            CloseHandle( main_exe_file );
            main_exe_file = 0;
            if (DOSFS_GetFullName( main_exe_name, TRUE, &full_name ) &&
                wine_dlopen( full_name.long_name, RTLD_NOW, error, sizeof(error) ))
            {
                static const WCHAR soW[] = {'.','s','o',0};
                if ((p = strrchrW( main_exe_name, '.' )) && !strcmpW( p, soW ))
                {
                    *p = 0;
                    /* update the unicode string */
                    RtlInitUnicodeString( &peb->ProcessParameters->ImagePathName, main_exe_name );
                }
                goto found;
            }
            MESSAGE( "wine: could not load %s: %s\n", debugstr_w(main_exe_name), error );
            ExitProcess(1);
        }
    }

 found:
    /* build command line */
    set_library_wargv( __wine_main_argv );
    if (!build_command_line( __wine_main_wargv )) goto error;

    stack_size = RtlImageNtHeader(peb->ImageBaseAddress)->OptionalHeader.SizeOfStackReserve;

    /* allocate main thread stack */
    if (!THREAD_InitStack( NtCurrentTeb(), stack_size )) goto error;

    /* switch to the new stack */
    wine_switch_to_stack( start_process, NULL, NtCurrentTeb()->Tib.StackBase );

 error:
    ExitProcess( GetLastError() );
}


/***********************************************************************
 *           build_argv
 *
 * Build an argv array from a command-line.
 * 'reserved' is the number of args to reserve before the first one.
 */
static char **build_argv( const WCHAR *cmdlineW, int reserved )
{
    int argc;
    char** argv;
    char *arg,*s,*d,*cmdline;
    int in_quotes,bcount,len;

    len = WideCharToMultiByte( CP_UNIXCP, 0, cmdlineW, -1, NULL, 0, NULL, NULL );
    if (!(cmdline = malloc(len))) return NULL;
    WideCharToMultiByte( CP_UNIXCP, 0, cmdlineW, -1, cmdline, len, NULL, NULL );

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
            /* unescaped '"' */
            in_quotes=!in_quotes;
            bcount=0;
        } else {
            /* a regular character */
            bcount=0;
        }
        s++;
    }
    argv=malloc(argc*sizeof(*argv));
    if (!argv)
        return NULL;

    arg=d=s=cmdline;
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
                /* Preceeded by an even number of '\', this is half that
                 * number of '\', plus a '"' which we discard.
                 */
                d-=bcount/2;
                s++;
                in_quotes=!in_quotes;
            } else {
                /* Preceeded by an odd number of '\', this is half that
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

    return argv;
}


/***********************************************************************
 *           alloc_env_string
 *
 * Allocate an environment string; helper for build_envp
 */
static char *alloc_env_string( const char *name, const char *value )
{
    char *ret = malloc( strlen(name) + strlen(value) + 1 );
    strcpy( ret, name );
    strcat( ret, value );
    return ret;
}

/***********************************************************************
 *           build_envp
 *
 * Build the environment of a new child process.
 */
static char **build_envp( const WCHAR *envW, const WCHAR *extra_envW )
{
    const WCHAR *p;
    char **envp;
    char *env, *extra_env = NULL;
    int count = 0, length;

    if (extra_envW)
    {
        for (p = extra_envW; *p; count++) p += strlenW(p) + 1;
        p++;
        length = WideCharToMultiByte( CP_UNIXCP, 0, extra_envW, p - extra_envW,
                                      NULL, 0, NULL, NULL );
        if ((extra_env = malloc( length )))
            WideCharToMultiByte( CP_UNIXCP, 0, extra_envW, p - extra_envW,
                                 extra_env, length, NULL, NULL );
    }
    for (p = envW; *p; count++) p += strlenW(p) + 1;
    p++;
    length = WideCharToMultiByte( CP_UNIXCP, 0, envW, p - envW, NULL, 0, NULL, NULL );
    if (!(env = malloc( length ))) return NULL;
    WideCharToMultiByte( CP_UNIXCP, 0, envW, p - envW, env, length, NULL, NULL );

    count += 4;

    if ((envp = malloc( count * sizeof(*envp) )))
    {
        char **envptr = envp;
        char *p;

        /* first the extra strings */
        if (extra_env) for (p = extra_env; *p; p += strlen(p) + 1) *envptr++ = p;
        /* then put PATH, HOME and WINEPREFIX from the unix env */
        if ((p = getenv("PATH"))) *envptr++ = alloc_env_string( "PATH=", p );
        if ((p = getenv("HOME"))) *envptr++ = alloc_env_string( "HOME=", p );
        if ((p = getenv("WINEPREFIX"))) *envptr++ = alloc_env_string( "WINEPREFIX=", p );
        /* now put the Windows environment strings */
        for (p = env; *p; p += strlen(p) + 1)
        {
            if (!memcmp( p, "PATH=", 5 ))  /* store PATH as WINEPATH */
                *envptr++ = alloc_env_string( "WINEPATH=", p + 5 );
            else if (memcmp( p, "HOME=", 5 ) &&
                     memcmp( p, "WINEPATH=", 9 ) &&
                     memcmp( p, "WINEPREFIX=", 11 )) *envptr++ = p;
        }
        *envptr = 0;
    }
    return envp;
}


/***********************************************************************
 *           exec_wine_binary
 *
 * Locate the Wine binary to exec for a new Win32 process.
 */
static void exec_wine_binary( char **argv, char **envp )
{
    const char *path, *pos, *ptr;

    /* first, try for a WINELOADER environment variable */
    argv[0] = getenv("WINELOADER");
    if (argv[0])
        execve( argv[0], argv, envp );

    /* next, try bin directory */
    argv[0] = BINDIR "/wine";
    execve( argv[0], argv, envp );

    /* now try the path of argv0 of the current binary */
    if ((path = wine_get_argv0_path()))
    {
        if (!(argv[0] = malloc( strlen(path) + sizeof("wine") ))) return;
        strcpy( argv[0], path );
        strcat( argv[0], "wine" );
        execve( argv[0], argv, envp );
        free( argv[0] );
    }

    /* now search in the Unix path */
    if ((path = getenv( "PATH" )))
    {
        if (!(argv[0] = malloc( strlen(path) + 6 ))) return;
        pos = path;
        for (;;)
        {
            while (*pos == ':') pos++;
            if (!*pos) break;
            if (!(ptr = strchr( pos, ':' ))) ptr = pos + strlen(pos);
            memcpy( argv[0], pos, ptr - pos );
            strcpy( argv[0] + (ptr - pos), "/wine" );
            execve( argv[0], argv, envp );
            pos = ptr;
        }
    }
    free( argv[0] );
}


/***********************************************************************
 *           fork_and_exec
 *
 * Fork and exec a new Unix binary, checking for errors.
 */
static int fork_and_exec( const char *filename, const WCHAR *cmdline,
                          const WCHAR *env, const char *newdir )
{
    int fd[2];
    int pid, err;

    if (!env) env = GetEnvironmentStringsW();

    if (pipe(fd) == -1)
    {
        FILE_SetDosError();
        return -1;
    }
    fcntl( fd[1], F_SETFD, 1 );  /* set close on exec */
    if (!(pid = fork()))  /* child */
    {
        char **argv = build_argv( cmdline, 0 );
        char **envp = build_envp( env, NULL );
        close( fd[0] );

        /* Reset signals that we previously set to SIG_IGN */
        signal( SIGPIPE, SIG_DFL );
        signal( SIGCHLD, SIG_DFL );

        if (newdir) chdir(newdir);

        if (argv && envp) execve( filename, argv, envp );
        err = errno;
        write( fd[1], &err, sizeof(err) );
        _exit(1);
    }
    close( fd[1] );
    if ((pid != -1) && (read( fd[0], &err, sizeof(err) ) > 0))  /* exec failed */
    {
        errno = err;
        pid = -1;
    }
    if (pid == -1) FILE_SetDosError();
    close( fd[0] );
    return pid;
}


/***********************************************************************
 *           create_user_params
 */
static RTL_USER_PROCESS_PARAMETERS *create_user_params( LPCWSTR filename, LPCWSTR cmdline,
                                                        const STARTUPINFOW *startup )
{
    RTL_USER_PROCESS_PARAMETERS *params;
    UNICODE_STRING image_str, cmdline_str, desktop, title;
    NTSTATUS status;
    WCHAR buffer[MAX_PATH];

    if (GetLongPathNameW( filename, buffer, MAX_PATH ))
        RtlInitUnicodeString( &image_str, buffer );
    else
        RtlInitUnicodeString( &image_str, filename );

    RtlInitUnicodeString( &cmdline_str, cmdline );
    if (startup->lpDesktop) RtlInitUnicodeString( &desktop, startup->lpDesktop );
    if (startup->lpTitle) RtlInitUnicodeString( &title, startup->lpTitle );

    status = RtlCreateProcessParameters( &params, &image_str, NULL, NULL, &cmdline_str, NULL,
                                         startup->lpTitle ? &title : NULL,
                                         startup->lpDesktop ? &desktop : NULL,
                                         NULL, NULL );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    params->Environment     = NULL;  /* we pass it through the Unix environment */
    params->hStdInput       = startup->hStdInput;
    params->hStdOutput      = startup->hStdOutput;
    params->hStdError       = startup->hStdError;
    params->dwX             = startup->dwX;
    params->dwY             = startup->dwY;
    params->dwXSize         = startup->dwXSize;
    params->dwYSize         = startup->dwYSize;
    params->dwXCountChars   = startup->dwXCountChars;
    params->dwYCountChars   = startup->dwYCountChars;
    params->dwFillAttribute = startup->dwFillAttribute;
    params->dwFlags         = startup->dwFlags;
    params->wShowWindow     = startup->wShowWindow;
    return params;
}


/***********************************************************************
 *           create_process
 *
 * Create a new process. If hFile is a valid handle we have an exe
 * file, otherwise it is a Winelib app.
 */
static BOOL create_process( HANDLE hFile, LPCWSTR filename, LPWSTR cmd_line, LPWSTR env,
                            LPSECURITY_ATTRIBUTES psa, LPSECURITY_ATTRIBUTES tsa,
                            BOOL inherit, DWORD flags, LPSTARTUPINFOW startup,
                            LPPROCESS_INFORMATION info, LPCSTR unixdir )
{
    BOOL ret, success = FALSE;
    HANDLE process_info;
    RTL_USER_PROCESS_PARAMETERS *params;
    WCHAR *extra_env = NULL;
    int startfd[2];
    int execfd[2];
    pid_t pid;
    int err;
    char dummy = 0;

    if (!env)
    {
        env = GetEnvironmentStringsW();
        extra_env = DRIVE_BuildEnv();
    }

    if (!(params = create_user_params( filename, cmd_line, startup )))
    {
        if (extra_env) HeapFree( GetProcessHeap(), 0, extra_env );
        return FALSE;
    }

    /* create the synchronization pipes */

    if (pipe( startfd ) == -1)
    {
        FILE_SetDosError();
        RtlDestroyProcessParameters( params );
        if (extra_env) HeapFree( GetProcessHeap(), 0, extra_env );
        return FALSE;
    }
    if (pipe( execfd ) == -1)
    {
        FILE_SetDosError();
        close( startfd[0] );
        close( startfd[1] );
        RtlDestroyProcessParameters( params );
        if (extra_env) HeapFree( GetProcessHeap(), 0, extra_env );
        return FALSE;
    }
    fcntl( execfd[1], F_SETFD, 1 );  /* set close on exec */

    /* create the child process */

    if (!(pid = fork()))  /* child */
    {
        char **argv = build_argv( cmd_line, 1 );
        char **envp = build_envp( env, extra_env );

        close( startfd[1] );
        close( execfd[0] );

        /* wait for parent to tell us to start */
        if (read( startfd[0], &dummy, 1 ) != 1) _exit(1);

        close( startfd[0] );
        /* Reset signals that we previously set to SIG_IGN */
        signal( SIGPIPE, SIG_DFL );
        signal( SIGCHLD, SIG_DFL );

        if (unixdir) chdir(unixdir);

        if (argv && envp) exec_wine_binary( argv, envp );

        err = errno;
        write( execfd[1], &err, sizeof(err) );
        _exit(1);
    }

    /* this is the parent */

    close( startfd[0] );
    close( execfd[1] );
    if (extra_env) HeapFree( GetProcessHeap(), 0, extra_env );
    if (pid == -1)
    {
        close( startfd[1] );
        close( execfd[0] );
        FILE_SetDosError();
        RtlDestroyProcessParameters( params );
        return FALSE;
    }

    /* create the process on the server side */

    SERVER_START_REQ( new_process )
    {
        req->inherit_all  = inherit;
        req->create_flags = flags;
        req->unix_pid     = pid;
        req->exe_file     = hFile;
        if (startup->dwFlags & STARTF_USESTDHANDLES)
        {
            req->hstdin  = startup->hStdInput;
            req->hstdout = startup->hStdOutput;
            req->hstderr = startup->hStdError;
        }
        else
        {
            req->hstdin  = GetStdHandle( STD_INPUT_HANDLE );
            req->hstdout = GetStdHandle( STD_OUTPUT_HANDLE );
            req->hstderr = GetStdHandle( STD_ERROR_HANDLE );
        }

        if ((flags & (CREATE_NEW_CONSOLE | DETACHED_PROCESS)) != 0)
        {
            /* this is temporary (for console handles). We have no way to control that the handle is invalid in child process otherwise */
            if (is_console_handle(req->hstdin))  req->hstdin  = INVALID_HANDLE_VALUE;
            if (is_console_handle(req->hstdout)) req->hstdout = INVALID_HANDLE_VALUE;
            if (is_console_handle(req->hstderr)) req->hstderr = INVALID_HANDLE_VALUE;
        }
        else
        {
            if (is_console_handle(req->hstdin))  req->hstdin  = console_handle_unmap(req->hstdin);
            if (is_console_handle(req->hstdout)) req->hstdout = console_handle_unmap(req->hstdout);
            if (is_console_handle(req->hstderr)) req->hstderr = console_handle_unmap(req->hstderr);
        }

        wine_server_add_data( req, params, params->Size );
        ret = !wine_server_call_err( req );
        process_info = reply->info;
    }
    SERVER_END_REQ;

    RtlDestroyProcessParameters( params );
    if (!ret)
    {
        close( startfd[1] );
        close( execfd[0] );
        return FALSE;
    }

    /* tell child to start and wait for it to exec */

    write( startfd[1], &dummy, 1 );
    close( startfd[1] );

    if (read( execfd[0], &err, sizeof(err) ) > 0) /* exec failed */
    {
        errno = err;
        FILE_SetDosError();
        close( execfd[0] );
        CloseHandle( process_info );
        return FALSE;
    }

    /* wait for the new process info to be ready */

    WaitForSingleObject( process_info, INFINITE );
    SERVER_START_REQ( get_new_process_info )
    {
        req->info     = process_info;
        req->pinherit = (psa && (psa->nLength >= sizeof(*psa)) && psa->bInheritHandle);
        req->tinherit = (tsa && (tsa->nLength >= sizeof(*tsa)) && tsa->bInheritHandle);
        if ((ret = !wine_server_call_err( req )))
        {
            info->dwProcessId = (DWORD)reply->pid;
            info->dwThreadId  = (DWORD)reply->tid;
            info->hProcess    = reply->phandle;
            info->hThread     = reply->thandle;
            success           = reply->success;
        }
    }
    SERVER_END_REQ;

    if (ret && !success)  /* new process failed to start */
    {
        DWORD exitcode;
        if (GetExitCodeProcess( info->hProcess, &exitcode )) SetLastError( exitcode );
        CloseHandle( info->hThread );
        CloseHandle( info->hProcess );
        ret = FALSE;
    }
    CloseHandle( process_info );
    return ret;
}


/***********************************************************************
 *           create_vdm_process
 *
 * Create a new VDM process for a 16-bit or DOS application.
 */
static BOOL create_vdm_process( LPCWSTR filename, LPWSTR cmd_line, LPWSTR env,
                                LPSECURITY_ATTRIBUTES psa, LPSECURITY_ATTRIBUTES tsa,
                                BOOL inherit, DWORD flags, LPSTARTUPINFOW startup,
                                LPPROCESS_INFORMATION info, LPCSTR unixdir )
{
    static const WCHAR argsW[] = {'%','s',' ','-','-','a','p','p','-','n','a','m','e',' ','"','%','s','"',' ','%','s',0};

    BOOL ret;
    LPWSTR new_cmd_line = HeapAlloc( GetProcessHeap(), 0,
                                     (strlenW(filename) + strlenW(cmd_line) + 30) * sizeof(WCHAR) );

    if (!new_cmd_line)
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return FALSE;
    }
    sprintfW( new_cmd_line, argsW, winevdmW, filename, cmd_line );
    ret = create_process( 0, winevdmW, new_cmd_line, env, psa, tsa, inherit,
                          flags, startup, info, unixdir );
    HeapFree( GetProcessHeap(), 0, new_cmd_line );
    return ret;
}


/***********************************************************************
 *           create_cmd_process
 *
 * Create a new cmd shell process for a .BAT file.
 */
static BOOL create_cmd_process( LPCWSTR filename, LPWSTR cmd_line, LPVOID env,
                                LPSECURITY_ATTRIBUTES psa, LPSECURITY_ATTRIBUTES tsa,
                                BOOL inherit, DWORD flags, LPSTARTUPINFOW startup,
                                LPPROCESS_INFORMATION info, LPCWSTR cur_dir )

{
    static const WCHAR comspecW[] = {'C','O','M','S','P','E','C',0};
    static const WCHAR slashcW[] = {' ','/','c',' ',0};
    WCHAR comspec[MAX_PATH];
    WCHAR *newcmdline;
    BOOL ret;

    if (!GetEnvironmentVariableW( comspecW, comspec, sizeof(comspec)/sizeof(WCHAR) ))
        return FALSE;
    if (!(newcmdline = HeapAlloc( GetProcessHeap(), 0,
                                  (strlenW(comspec) + 4 + strlenW(cmd_line) + 1) * sizeof(WCHAR))))
        return FALSE;

    strcpyW( newcmdline, comspec );
    strcatW( newcmdline, slashcW );
    strcatW( newcmdline, cmd_line );
    ret = CreateProcessW( comspec, newcmdline, psa, tsa, inherit,
                          flags, env, cur_dir, startup, info );
    HeapFree( GetProcessHeap(), 0, newcmdline );
    return ret;
}


/*************************************************************************
 *               get_file_name
 *
 * Helper for CreateProcess: retrieve the file name to load from the
 * app name and command line. Store the file name in buffer, and
 * return a possibly modified command line.
 * Also returns a handle to the opened file if it's a Windows binary.
 */
static LPWSTR get_file_name( LPCWSTR appname, LPWSTR cmdline, LPWSTR buffer,
                             int buflen, HANDLE *handle )
{
    static const WCHAR quotesW[] = {'"','%','s','"',0};

    WCHAR *name, *pos, *ret = NULL;
    const WCHAR *p;

    /* if we have an app name, everything is easy */

    if (appname)
    {
        /* use the unmodified app name as file name */
        lstrcpynW( buffer, appname, buflen );
        *handle = open_exe_file( buffer );
        if (!(ret = cmdline) || !cmdline[0])
        {
            /* no command-line, create one */
            if ((ret = HeapAlloc( GetProcessHeap(), 0, (strlenW(appname) + 3) + sizeof(WCHAR) )))
                sprintfW( ret, quotesW, appname );
        }
        return ret;
    }

    if (!cmdline)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    /* first check for a quoted file name */

    if ((cmdline[0] == '"') && ((p = strchrW( cmdline + 1, '"' ))))
    {
        int len = p - cmdline - 1;
        /* extract the quoted portion as file name */
        if (!(name = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) ))) return NULL;
        memcpy( name, cmdline + 1, len * sizeof(WCHAR) );
        name[len] = 0;

        if (find_exe_file( name, buffer, buflen, handle ))
            ret = cmdline;  /* no change necessary */
        goto done;
    }

    /* now try the command-line word by word */

    if (!(name = HeapAlloc( GetProcessHeap(), 0, (strlenW(cmdline) + 1) * sizeof(WCHAR) )))
        return NULL;
    pos = name;
    p = cmdline;

    while (*p)
    {
        do *pos++ = *p++; while (*p && *p != ' ');
        *pos = 0;
        if (find_exe_file( name, buffer, buflen, handle ))
        {
            ret = cmdline;
            break;
        }
    }

    if (!ret || !strchrW( name, ' ' )) goto done;  /* no change necessary */

    /* now build a new command-line with quotes */

    if (!(ret = HeapAlloc( GetProcessHeap(), 0, (strlenW(cmdline) + 3) * sizeof(WCHAR) )))
        goto done;
    sprintfW( ret, quotesW, name );
    strcatW( ret, p );

 done:
    HeapFree( GetProcessHeap(), 0, name );
    return ret;
}


/**********************************************************************
 *       CreateProcessA          (KERNEL32.@)
 */
BOOL WINAPI CreateProcessA( LPCSTR app_name, LPSTR cmd_line, LPSECURITY_ATTRIBUTES process_attr,
                            LPSECURITY_ATTRIBUTES thread_attr, BOOL inherit,
                            DWORD flags, LPVOID env, LPCSTR cur_dir,
                            LPSTARTUPINFOA startup_info, LPPROCESS_INFORMATION info )
{
    BOOL ret;
    UNICODE_STRING app_nameW, cmd_lineW, cur_dirW, desktopW, titleW;
    STARTUPINFOW infoW;

    if (app_name) RtlCreateUnicodeStringFromAsciiz( &app_nameW, app_name );
    else app_nameW.Buffer = NULL;
    if (cmd_line) RtlCreateUnicodeStringFromAsciiz( &cmd_lineW, cmd_line );
    else cmd_lineW.Buffer = NULL;
    if (cur_dir) RtlCreateUnicodeStringFromAsciiz( &cur_dirW, cur_dir );
    else cur_dirW.Buffer = NULL;
    if (startup_info->lpDesktop) RtlCreateUnicodeStringFromAsciiz( &desktopW, startup_info->lpDesktop );
    else desktopW.Buffer = NULL;
    if (startup_info->lpTitle) RtlCreateUnicodeStringFromAsciiz( &titleW, startup_info->lpTitle );
    else titleW.Buffer = NULL;

    memcpy( &infoW, startup_info, sizeof(infoW) );
    infoW.lpDesktop = desktopW.Buffer;
    infoW.lpTitle = titleW.Buffer;

    if (startup_info->lpReserved)
      FIXME("StartupInfo.lpReserved is used, please report (%s)\n",
            debugstr_a(startup_info->lpReserved));

    ret = CreateProcessW( app_nameW.Buffer,  cmd_lineW.Buffer, process_attr, thread_attr,
                          inherit, flags, env, cur_dirW.Buffer, &infoW, info );

    RtlFreeUnicodeString( &app_nameW );
    RtlFreeUnicodeString( &cmd_lineW );
    RtlFreeUnicodeString( &cur_dirW );
    RtlFreeUnicodeString( &desktopW );
    RtlFreeUnicodeString( &titleW );
    return ret;
}


/**********************************************************************
 *       CreateProcessW          (KERNEL32.@)
 */
BOOL WINAPI CreateProcessW( LPCWSTR app_name, LPWSTR cmd_line, LPSECURITY_ATTRIBUTES process_attr,
                            LPSECURITY_ATTRIBUTES thread_attr, BOOL inherit, DWORD flags,
                            LPVOID env, LPCWSTR cur_dir, LPSTARTUPINFOW startup_info,
                            LPPROCESS_INFORMATION info )
{
    BOOL retv = FALSE;
    HANDLE hFile = 0;
    const char *unixdir = NULL;
    DOS_FULL_NAME full_dir;
    WCHAR name[MAX_PATH];
    WCHAR *tidy_cmdline, *p, *envW = env;

    /* Process the AppName and/or CmdLine to get module name and path */

    TRACE("app %s cmdline %s\n", debugstr_w(app_name), debugstr_w(cmd_line) );

    if (!(tidy_cmdline = get_file_name( app_name, cmd_line, name, sizeof(name), &hFile )))
        return FALSE;
    if (hFile == INVALID_HANDLE_VALUE) goto done;

    /* Warn if unsupported features are used */

    if (flags & (IDLE_PRIORITY_CLASS | HIGH_PRIORITY_CLASS | REALTIME_PRIORITY_CLASS |
                 CREATE_NEW_PROCESS_GROUP | CREATE_SEPARATE_WOW_VDM | CREATE_SHARED_WOW_VDM |
                 CREATE_DEFAULT_ERROR_MODE | CREATE_NO_WINDOW |
                 PROFILE_USER | PROFILE_KERNEL | PROFILE_SERVER))
        WARN("(%s,...): ignoring some flags in %lx\n", debugstr_w(name), flags);

    if (cur_dir)
    {
        if (DOSFS_GetFullName( cur_dir, TRUE, &full_dir )) unixdir = full_dir.long_name;
    }
    else
    {
        WCHAR buf[MAX_PATH];
        if (GetCurrentDirectoryW(MAX_PATH, buf))
        {
            if (DOSFS_GetFullName( buf, TRUE, &full_dir )) unixdir = full_dir.long_name;
        }
    }

    if (env && !(flags & CREATE_UNICODE_ENVIRONMENT))  /* convert environment to unicode */
    {
        char *p = env;
        DWORD lenW;

        while (*p) p += strlen(p) + 1;
        p++;  /* final null */
        lenW = MultiByteToWideChar( CP_ACP, 0, env, p - (char*)env, NULL, 0 );
        envW = HeapAlloc( GetProcessHeap(), 0, lenW * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, env, p - (char*)env, envW, lenW );
        flags |= CREATE_UNICODE_ENVIRONMENT;
    }

    info->hThread = info->hProcess = 0;
    info->dwProcessId = info->dwThreadId = 0;

    /* Determine executable type */

    if (!hFile)  /* builtin exe */
    {
        TRACE( "starting %s as Winelib app\n", debugstr_w(name) );
        retv = create_process( 0, name, tidy_cmdline, envW, process_attr, thread_attr,
                               inherit, flags, startup_info, info, unixdir );
        goto done;
    }

    switch( MODULE_GetBinaryType( hFile ))
    {
    case BINARY_PE_EXE:
        TRACE( "starting %s as Win32 binary\n", debugstr_w(name) );
        retv = create_process( hFile, name, tidy_cmdline, envW, process_attr, thread_attr,
                               inherit, flags, startup_info, info, unixdir );
        break;
    case BINARY_WIN16:
    case BINARY_DOS:
        TRACE( "starting %s as Win16/DOS binary\n", debugstr_w(name) );
        retv = create_vdm_process( name, tidy_cmdline, envW, process_attr, thread_attr,
                                   inherit, flags, startup_info, info, unixdir );
        break;
    case BINARY_OS216:
        FIXME( "%s is OS/2 binary, not supported\n", debugstr_w(name) );
        SetLastError( ERROR_BAD_EXE_FORMAT );
        break;
    case BINARY_PE_DLL:
        TRACE( "not starting %s since it is a dll\n", debugstr_w(name) );
        SetLastError( ERROR_BAD_EXE_FORMAT );
        break;
    case BINARY_UNIX_LIB:
        TRACE( "%s is a Unix library, starting as Winelib app\n", debugstr_w(name) );
        retv = create_process( hFile, name, tidy_cmdline, envW, process_attr, thread_attr,
                               inherit, flags, startup_info, info, unixdir );
        break;
    case BINARY_UNKNOWN:
        /* check for .com or .bat extension */
        if ((p = strrchrW( name, '.' )))
        {
            if (!strcmpiW( p, comW ))
            {
                TRACE( "starting %s as DOS binary\n", debugstr_w(name) );
                retv = create_vdm_process( name, tidy_cmdline, envW, process_attr, thread_attr,
                                           inherit, flags, startup_info, info, unixdir );
                break;
            }
            if (!strcmpiW( p, batW ))
            {
                TRACE( "starting %s as batch binary\n", debugstr_w(name) );
                retv = create_cmd_process( name, tidy_cmdline, envW, process_attr, thread_attr,
                                           inherit, flags, startup_info, info, cur_dir );
                break;
            }
        }
        /* fall through */
    case BINARY_UNIX_EXE:
        {
            /* unknown file, try as unix executable */
            DOS_FULL_NAME full_name;

            TRACE( "starting %s as Unix binary\n", debugstr_w(name) );

            if (DOSFS_GetFullName( name, TRUE, &full_name ))
                retv = (fork_and_exec( full_name.long_name, tidy_cmdline, envW, unixdir ) != -1);
        }
        break;
    }
    CloseHandle( hFile );

 done:
    if (tidy_cmdline != cmd_line) HeapFree( GetProcessHeap(), 0, tidy_cmdline );
    if (envW != env) HeapFree( GetProcessHeap(), 0, envW );
    return retv;
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
UINT WINAPI WinExec( LPCSTR lpCmdLine, UINT nCmdShow )
{
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    char *cmdline;
    UINT ret;

    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = nCmdShow;

    /* cmdline needs to be writeable for CreateProcess */
    if (!(cmdline = HeapAlloc( GetProcessHeap(), 0, strlen(lpCmdLine)+1 ))) return 0;
    strcpy( cmdline, lpCmdLine );

    if (CreateProcessA( NULL, cmdline, NULL, NULL, FALSE,
                        0, NULL, NULL, &startup, &info ))
    {
        /* Give 30 seconds to the app to come up */
        if (wait_input_idle( info.hProcess, 30000 ) == WAIT_FAILED)
            WARN("WaitForInputIdle failed: Error %ld\n", GetLastError() );
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
HINSTANCE WINAPI LoadModule( LPCSTR name, LPVOID paramBlock )
{
    LOADPARAMS *params = (LOADPARAMS *)paramBlock;
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    HINSTANCE hInstance;
    LPSTR cmdline, p;
    char filename[MAX_PATH];
    BYTE len;

    if (!name) return (HINSTANCE)ERROR_FILE_NOT_FOUND;

    if (!SearchPathA( NULL, name, ".exe", sizeof(filename), filename, NULL ) &&
        !SearchPathA( NULL, name, NULL, sizeof(filename), filename, NULL ))
        return (HINSTANCE)GetLastError();

    len = (BYTE)params->lpCmdLine[0];
    if (!(cmdline = HeapAlloc( GetProcessHeap(), 0, strlen(filename) + len + 2 )))
        return (HINSTANCE)ERROR_NOT_ENOUGH_MEMORY;

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
        startup.wShowWindow = params->lpCmdShow[1];
    }

    if (CreateProcessA( filename, cmdline, NULL, NULL, FALSE, 0,
                        params->lpEnvAddress, NULL, &startup, &info ))
    {
        /* Give 30 seconds to the app to come up */
        if (wait_input_idle( info.hProcess, 30000 ) == WAIT_FAILED)
            WARN("WaitForInputIdle failed: Error %ld\n", GetLastError() );
        hInstance = (HINSTANCE)33;
        /* Close off the handles */
        CloseHandle( info.hThread );
        CloseHandle( info.hProcess );
    }
    else if ((hInstance = (HINSTANCE)GetLastError()) >= (HINSTANCE)32)
    {
        FIXME("Strange error set by CreateProcess: %p\n", hInstance );
        hInstance = (HINSTANCE)11;
    }

    HeapFree( GetProcessHeap(), 0, cmdline );
    return hInstance;
}


/******************************************************************************
 *           TerminateProcess   (KERNEL32.@)
 */
BOOL WINAPI TerminateProcess( HANDLE handle, DWORD exit_code )
{
    NTSTATUS status = NtTerminateProcess( handle, exit_code );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *           ExitProcess   (KERNEL32.@)
 */
void WINAPI ExitProcess( DWORD status )
{
    LdrShutdownProcess();
    SERVER_START_REQ( terminate_process )
    {
        /* send the exit code to the server */
        req->handle    = GetCurrentProcess();
        req->exit_code = status;
        wine_server_call( req );
    }
    SERVER_END_REQ;
    exit( status );
}


/***********************************************************************
 * GetExitCodeProcess [KERNEL32.@]
 *
 * Gets termination status of specified process
 *
 * RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 */
BOOL WINAPI GetExitCodeProcess(
    HANDLE hProcess,    /* [in] handle to the process */
    LPDWORD lpExitCode) /* [out] address to receive termination status */
{
    BOOL ret;
    SERVER_START_REQ( get_process_info )
    {
        req->handle = hProcess;
        ret = !wine_server_call_err( req );
        if (ret && lpExitCode) *lpExitCode = reply->exit_code;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           SetErrorMode   (KERNEL32.@)
 */
UINT WINAPI SetErrorMode( UINT mode )
{
    UINT old = process_error_mode;
    process_error_mode = mode;
    return old;
}


/**********************************************************************
 * TlsAlloc [KERNEL32.@]  Allocates a TLS index.
 *
 * Allocates a thread local storage index
 *
 * RETURNS
 *    Success: TLS Index
 *    Failure: 0xFFFFFFFF
 */
DWORD WINAPI TlsAlloc( void )
{
    DWORD index;

    RtlAcquirePebLock();
    index = RtlFindClearBitsAndSet( NtCurrentTeb()->Peb->TlsBitmap, 1, 0 );
    if (index != ~0UL) NtCurrentTeb()->TlsSlots[index] = 0; /* clear the value */
    else SetLastError( ERROR_NO_MORE_ITEMS );
    RtlReleasePebLock();
    return index;
}


/**********************************************************************
 * TlsFree [KERNEL32.@]  Releases a TLS index.
 *
 * Releases a thread local storage index, making it available for reuse
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI TlsFree(
    DWORD index) /* [in] TLS Index to free */
{
    BOOL ret;

    RtlAcquirePebLock();
    ret = RtlAreBitsSet( NtCurrentTeb()->Peb->TlsBitmap, index, 1 );
    if (ret)
    {
        RtlClearBits( NtCurrentTeb()->Peb->TlsBitmap, index, 1 );
        NtSetInformationThread( GetCurrentThread(), ThreadZeroTlsCell, &index, sizeof(index) );
    }
    else SetLastError( ERROR_INVALID_PARAMETER );
    RtlReleasePebLock();
    return TRUE;
}


/**********************************************************************
 * TlsGetValue [KERNEL32.@]  Gets value in a thread's TLS slot
 *
 * RETURNS
 *    Success: Value stored in calling thread's TLS slot for index
 *    Failure: 0 and GetLastError returns NO_ERROR
 */
LPVOID WINAPI TlsGetValue(
    DWORD index) /* [in] TLS index to retrieve value for */
{
    if (index >= NtCurrentTeb()->Peb->TlsBitmap->SizeOfBitMap)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    SetLastError( ERROR_SUCCESS );
    return NtCurrentTeb()->TlsSlots[index];
}


/**********************************************************************
 * TlsSetValue [KERNEL32.@]  Stores a value in the thread's TLS slot.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI TlsSetValue(
    DWORD index,  /* [in] TLS index to set value for */
    LPVOID value) /* [in] Value to be stored */
{
    if (index >= NtCurrentTeb()->Peb->TlsBitmap->SizeOfBitMap)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    NtCurrentTeb()->TlsSlots[index] = value;
    return TRUE;
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
 *           GetProcessDword    (KERNEL.485)
 *           GetProcessDword    (KERNEL32.18)
 * 'Of course you cannot directly access Windows internal structures'
 */
DWORD WINAPI GetProcessDword( DWORD dwProcessID, INT offset )
{
    DWORD               x, y;
    STARTUPINFOW        siw;

    TRACE("(%ld, %d)\n", dwProcessID, offset );

    if (dwProcessID && dwProcessID != GetCurrentProcessId())
    {
        ERR("%d: process %lx not accessible\n", offset, dwProcessID);
        return 0;
    }

    switch ( offset )
    {
    case GPD_APP_COMPAT_FLAGS:
        return GetAppCompatFlags16(0);
    case GPD_LOAD_DONE_EVENT:
        return 0;
    case GPD_HINSTANCE16:
        return GetTaskDS16();
    case GPD_WINDOWS_VERSION:
        return GetExeVersion16();
    case GPD_THDB:
        return (DWORD)NtCurrentTeb() - 0x10 /* FIXME */;
    case GPD_PDB:
        return (DWORD)NtCurrentTeb()->Peb;
    case GPD_STARTF_SHELLDATA: /* return stdoutput handle from startupinfo ??? */
        GetStartupInfoW(&siw);
        return (DWORD)siw.hStdOutput;
    case GPD_STARTF_HOTKEY: /* return stdinput handle from startupinfo ??? */
        GetStartupInfoW(&siw);
        return (DWORD)siw.hStdInput;
    case GPD_STARTF_SHOWWINDOW:
        GetStartupInfoW(&siw);
        return siw.wShowWindow;
    case GPD_STARTF_SIZE:
        GetStartupInfoW(&siw);
        x = siw.dwXSize;
        if ( (INT)x == CW_USEDEFAULT ) x = CW_USEDEFAULT16;
        y = siw.dwYSize;
        if ( (INT)y == CW_USEDEFAULT ) y = CW_USEDEFAULT16;
        return MAKELONG( x, y );
    case GPD_STARTF_POSITION:
        GetStartupInfoW(&siw);
        x = siw.dwX;
        if ( (INT)x == CW_USEDEFAULT ) x = CW_USEDEFAULT16;
        y = siw.dwY;
        if ( (INT)y == CW_USEDEFAULT ) y = CW_USEDEFAULT16;
        return MAKELONG( x, y );
    case GPD_STARTF_FLAGS:
        GetStartupInfoW(&siw);
        return siw.dwFlags;
    case GPD_PARENT:
        return 0;
    case GPD_FLAGS:
        return GetProcessFlags(0);
    case GPD_USERDATA:
        return process_dword;
    default:
        ERR("Unknown offset %d\n", offset );
        return 0;
    }
}

/***********************************************************************
 *           SetProcessDword    (KERNEL.484)
 * 'Of course you cannot directly access Windows internal structures'
 */
void WINAPI SetProcessDword( DWORD dwProcessID, INT offset, DWORD value )
{
    TRACE("(%ld, %d)\n", dwProcessID, offset );

    if (dwProcessID && dwProcessID != GetCurrentProcessId())
    {
        ERR("%d: process %lx not accessible\n", offset, dwProcessID);
        return;
    }

    switch ( offset )
    {
    case GPD_APP_COMPAT_FLAGS:
    case GPD_LOAD_DONE_EVENT:
    case GPD_HINSTANCE16:
    case GPD_WINDOWS_VERSION:
    case GPD_THDB:
    case GPD_PDB:
    case GPD_STARTF_SHELLDATA:
    case GPD_STARTF_HOTKEY:
    case GPD_STARTF_SHOWWINDOW:
    case GPD_STARTF_SIZE:
    case GPD_STARTF_POSITION:
    case GPD_STARTF_FLAGS:
    case GPD_PARENT:
    case GPD_FLAGS:
        ERR("Not allowed to modify offset %d\n", offset );
        break;
    case GPD_USERDATA:
        process_dword = value;
        break;
    default:
        ERR("Unknown offset %d\n", offset );
        break;
    }
}


/***********************************************************************
 *           ExitProcess   (KERNEL.466)
 */
void WINAPI ExitProcess16( WORD status )
{
    DWORD count;
    ReleaseThunkLock( &count );
    ExitProcess( status );
}


/*********************************************************************
 *           OpenProcess   (KERNEL32.@)
 */
HANDLE WINAPI OpenProcess( DWORD access, BOOL inherit, DWORD id )
{
    HANDLE ret = 0;
    SERVER_START_REQ( open_process )
    {
        req->pid     = id;
        req->access  = access;
        req->inherit = inherit;
        if (!wine_server_call_err( req )) ret = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}


/*********************************************************************
 *           MapProcessHandle   (KERNEL.483)
 */
DWORD WINAPI MapProcessHandle( HANDLE handle )
{
    DWORD ret = 0;
    SERVER_START_REQ( get_process_info )
    {
        req->handle = handle;
        if (!wine_server_call_err( req )) ret = reply->pid;
    }
    SERVER_END_REQ;
    return ret;
}


/*********************************************************************
 *           CloseW32Handle (KERNEL.474)
 *           CloseHandle    (KERNEL32.@)
 */
BOOL WINAPI CloseHandle( HANDLE handle )
{
    NTSTATUS status;

    /* stdio handles need special treatment */
    if ((handle == (HANDLE)STD_INPUT_HANDLE) ||
        (handle == (HANDLE)STD_OUTPUT_HANDLE) ||
        (handle == (HANDLE)STD_ERROR_HANDLE))
        handle = GetStdHandle( (DWORD)handle );

    if (is_console_handle(handle))
        return CloseConsoleHandle(handle);

    status = NtClose( handle );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/*********************************************************************
 *           GetHandleInformation   (KERNEL32.@)
 */
BOOL WINAPI GetHandleInformation( HANDLE handle, LPDWORD flags )
{
    BOOL ret;
    SERVER_START_REQ( set_handle_info )
    {
        req->handle = handle;
        req->flags  = 0;
        req->mask   = 0;
        req->fd     = -1;
        ret = !wine_server_call_err( req );
        if (ret && flags) *flags = reply->old_flags;
    }
    SERVER_END_REQ;
    return ret;
}


/*********************************************************************
 *           SetHandleInformation   (KERNEL32.@)
 */
BOOL WINAPI SetHandleInformation( HANDLE handle, DWORD mask, DWORD flags )
{
    BOOL ret;
    SERVER_START_REQ( set_handle_info )
    {
        req->handle = handle;
        req->flags  = flags;
        req->mask   = mask;
        req->fd     = -1;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/*********************************************************************
 *           DuplicateHandle   (KERNEL32.@)
 */
BOOL WINAPI DuplicateHandle( HANDLE source_process, HANDLE source,
                             HANDLE dest_process, HANDLE *dest,
                             DWORD access, BOOL inherit, DWORD options )
{
    NTSTATUS status;

    if (is_console_handle(source))
    {
        /* FIXME: this test is not sufficient, we need to test process ids, not handles */
        if (source_process != dest_process ||
            source_process != GetCurrentProcess())
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        *dest = DuplicateConsoleHandle( source, access, inherit, options );
        return (*dest != INVALID_HANDLE_VALUE);
    }
    status = NtDuplicateObject( source_process, source, dest_process, dest,
                                access, inherit ? OBJ_INHERIT : 0, options );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *           ConvertToGlobalHandle   (KERNEL.476)
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
    FIXME("(%p,%ld), stub. In case this got called by WSOCK32/WS2_32: "
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
 *           SetPriorityClass   (KERNEL32.@)
 */
BOOL WINAPI SetPriorityClass( HANDLE hprocess, DWORD priorityclass )
{
    BOOL ret;
    SERVER_START_REQ( set_process_info )
    {
        req->handle   = hprocess;
        req->priority = priorityclass;
        req->mask     = SET_PROCESS_INFO_PRIORITY;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           GetPriorityClass   (KERNEL32.@)
 */
DWORD WINAPI GetPriorityClass(HANDLE hprocess)
{
    DWORD ret = 0;
    SERVER_START_REQ( get_process_info )
    {
        req->handle = hprocess;
        if (!wine_server_call_err( req )) ret = reply->priority;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *          SetProcessAffinityMask   (KERNEL32.@)
 */
BOOL WINAPI SetProcessAffinityMask( HANDLE hProcess, DWORD affmask )
{
    BOOL ret;
    SERVER_START_REQ( set_process_info )
    {
        req->handle   = hProcess;
        req->affinity = affmask;
        req->mask     = SET_PROCESS_INFO_AFFINITY;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/**********************************************************************
 *          GetProcessAffinityMask    (KERNEL32.@)
 */
BOOL WINAPI GetProcessAffinityMask( HANDLE hProcess,
                                      LPDWORD lpProcessAffinityMask,
                                      LPDWORD lpSystemAffinityMask )
{
    BOOL ret = FALSE;
    SERVER_START_REQ( get_process_info )
    {
        req->handle = hProcess;
        if (!wine_server_call_err( req ))
        {
            if (lpProcessAffinityMask) *lpProcessAffinityMask = reply->process_affinity;
            if (lpSystemAffinityMask) *lpSystemAffinityMask = reply->system_affinity;
            ret = TRUE;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           GetProcessVersion    (KERNEL32.@)
 */
DWORD WINAPI GetProcessVersion( DWORD processid )
{
    IMAGE_NT_HEADERS *nt;

    if (processid && processid != GetCurrentProcessId())
    {
        FIXME("should use ReadProcessMemory\n");
        return 0;
    }
    if ((nt = RtlImageNtHeader( NtCurrentTeb()->Peb->ImageBaseAddress )))
        return ((nt->OptionalHeader.MajorSubsystemVersion << 16) |
                nt->OptionalHeader.MinorSubsystemVersion);
    return 0;
}


/***********************************************************************
 *		SetProcessWorkingSetSize	[KERNEL32.@]
 * Sets the min/max working set sizes for a specified process.
 *
 * PARAMS
 *    hProcess [I] Handle to the process of interest
 *    minset   [I] Specifies minimum working set size
 *    maxset   [I] Specifies maximum working set size
 *
 * RETURNS  STD
 */
BOOL WINAPI SetProcessWorkingSetSize(HANDLE hProcess, SIZE_T minset,
                                     SIZE_T maxset)
{
    FIXME("(%p,%ld,%ld): stub - harmless\n",hProcess,minset,maxset);
    if(( minset == (SIZE_T)-1) && (maxset == (SIZE_T)-1)) {
        /* Trim the working set to zero */
        /* Swap the process out of physical RAM */
    }
    return TRUE;
}

/***********************************************************************
 *           GetProcessWorkingSetSize    (KERNEL32.@)
 */
BOOL WINAPI GetProcessWorkingSetSize(HANDLE hProcess, PSIZE_T minset,
                                     PSIZE_T maxset)
{
    FIXME("(%p,%p,%p): stub\n",hProcess,minset,maxset);
    /* 32 MB working set size */
    if (minset) *minset = 32*1024*1024;
    if (maxset) *maxset = 32*1024*1024;
    return TRUE;
}


/***********************************************************************
 *           SetProcessShutdownParameters    (KERNEL32.@)
 */
BOOL WINAPI SetProcessShutdownParameters(DWORD level, DWORD flags)
{
    FIXME("(%08lx, %08lx): partial stub.\n", level, flags);
    shutdown_flags = flags;
    shutdown_priority = level;
    return TRUE;
}


/***********************************************************************
 * GetProcessShutdownParameters                 (KERNEL32.@)
 *
 */
BOOL WINAPI GetProcessShutdownParameters( LPDWORD lpdwLevel, LPDWORD lpdwFlags )
{
    *lpdwLevel = shutdown_priority;
    *lpdwFlags = shutdown_flags;
    return TRUE;
}


/***********************************************************************
 *           GetProcessPriorityBoost    (KERNEL32.@)
 */
BOOL WINAPI GetProcessPriorityBoost(HANDLE hprocess,PBOOL pDisablePriorityBoost)
{
    FIXME("(%p,%p): semi-stub\n", hprocess, pDisablePriorityBoost);
    
    /* Report that no boost is present.. */
    *pDisablePriorityBoost = FALSE;
    
    return TRUE;
}

/***********************************************************************
 *           SetProcessPriorityBoost    (KERNEL32.@)
 */
BOOL WINAPI SetProcessPriorityBoost(HANDLE hprocess,BOOL disableboost)
{
    FIXME("(%p,%d): stub\n",hprocess,disableboost);
    /* Say we can do it. I doubt the program will notice that we don't. */
    return TRUE;
}


/***********************************************************************
 *		ReadProcessMemory (KERNEL32.@)
 */
BOOL WINAPI ReadProcessMemory( HANDLE process, LPCVOID addr, LPVOID buffer, SIZE_T size,
                               SIZE_T *bytes_read )
{
    NTSTATUS status = NtReadVirtualMemory( process, addr, buffer, size, bytes_read );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *           WriteProcessMemory    		(KERNEL32.@)
 */
BOOL WINAPI WriteProcessMemory( HANDLE process, LPVOID addr, LPCVOID buffer, SIZE_T size,
                                SIZE_T *bytes_written )
{
    NTSTATUS status = NtWriteVirtualMemory( process, addr, buffer, size, bytes_written );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/****************************************************************************
 *		FlushInstructionCache (KERNEL32.@)
 */
BOOL WINAPI FlushInstructionCache(HANDLE hProcess, LPCVOID lpBaseAddress, SIZE_T dwSize)
{
    if (GetVersion() & 0x80000000) return TRUE; /* not NT, always TRUE */
    FIXME("(%p,%p,0x%08lx): stub\n",hProcess, lpBaseAddress, dwSize);
    return TRUE;
}


/******************************************************************
 *		GetProcessIoCounters (KERNEL32.@)
 */
BOOL WINAPI GetProcessIoCounters(HANDLE hProcess, PIO_COUNTERS ioc)
{
    NTSTATUS    status;

    status = NtQueryInformationProcess(hProcess, ProcessIoCounters, 
                                       ioc, sizeof(*ioc), NULL);
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/***********************************************************************
 * ProcessIdToSessionId   (KERNEL32.@)
 * This function is available on Terminal Server 4SP4 and Windows 2000
 */
BOOL WINAPI ProcessIdToSessionId( DWORD procid, DWORD *sessionid_ptr )
{
    /* According to MSDN, if the calling process is not in a terminal
     * services environment, then the sessionid returned is zero.
     */
    *sessionid_ptr = 0;
    return TRUE;
}


/***********************************************************************
 *		RegisterServiceProcess (KERNEL.491)
 *		RegisterServiceProcess (KERNEL32.@)
 *
 * A service process calls this function to ensure that it continues to run
 * even after a user logged off.
 */
DWORD WINAPI RegisterServiceProcess(DWORD dwProcessId, DWORD dwType)
{
    /* I don't think that Wine needs to do anything in that function */
    return 1; /* success */
}


/**************************************************************************
 *              SetFileApisToOEM   (KERNEL32.@)
 */
VOID WINAPI SetFileApisToOEM(void)
{
    oem_file_apis = TRUE;
}


/**************************************************************************
 *              SetFileApisToANSI   (KERNEL32.@)
 */
VOID WINAPI SetFileApisToANSI(void)
{
    oem_file_apis = FALSE;
}


/******************************************************************************
 * AreFileApisANSI [KERNEL32.@]  Determines if file functions are using ANSI
 *
 * RETURNS
 *    TRUE:  Set of file functions is using ANSI code page
 *    FALSE: Set of file functions is using OEM code page
 */
BOOL WINAPI AreFileApisANSI(void)
{
    return !oem_file_apis;
}


/***********************************************************************
 *           GetTickCount       (KERNEL32.@)
 *
 * Returns the number of milliseconds, modulo 2^32, since the start
 * of the wineserver.
 */
DWORD WINAPI GetTickCount(void)
{
    struct timeval t;
    gettimeofday( &t, NULL );
    return ((t.tv_sec * 1000) + (t.tv_usec / 1000)) - server_startticks;
}


/***********************************************************************
 *           GetCurrentProcess   (KERNEL32.@)
 */
#undef GetCurrentProcess
HANDLE WINAPI GetCurrentProcess(void)
{
    return (HANDLE)0xffffffff;
}
