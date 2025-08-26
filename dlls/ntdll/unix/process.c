/*
 * NT process handling
 *
 * Copyright 1996-1998 Marcus Meissner
 * Copyright 2018, 2020 Alexandre Julliard
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/time.h>
#ifdef HAVE_SYS_TIMES_H
# include <sys/times.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>
#ifdef HAVE_SYS_SYSCTL_H
# include <sys/sysctl.h>
#endif
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYS_QUEUE_H
# include <sys/queue.h>
#endif
#ifdef HAVE_SYS_USER_H
# include <sys/user.h>
#endif
#ifdef HAVE_LIBPROCSTAT_H
# include <libprocstat.h>
#endif
#include <unistd.h>
#ifdef HAVE_MACH_MACH_H
# include <mach/mach.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/ntddk.h"
#include "unix_private.h"
#include "wine/condrv.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(process);


static ULONG execute_flags = MEM_EXECUTE_OPTION_DISABLE;

static UINT process_error_mode;

static char **build_argv( const UNICODE_STRING *cmdline, int reserved )
{
    char **argv, *arg, *src, *dst;
    int argc, in_quotes = 0, bcount = 0, len = cmdline->Length / sizeof(WCHAR);

    if (!(src = malloc( len * 3 + 1 ))) return NULL;
    len = ntdll_wcstoumbs( cmdline->Buffer, len, src, len * 3, FALSE );
    src[len++] = 0;

    argc = reserved + 2 + len / 2;
    argv = malloc( argc * sizeof(*argv) + len );
    arg = dst = (char *)(argv + argc);
    argc = reserved;
    while (*src)
    {
        if ((*src == ' ' || *src == '\t') && !in_quotes)
        {
            /* skip the remaining spaces */
            while (*src == ' ' || *src == '\t') src++;
            if (!*src) break;
            /* close the argument and copy it */
            *dst++ = 0;
            argv[argc++] = arg;
            /* start with a new argument */
            arg = dst;
            bcount = 0;
        }
        else if (*src == '\\')
        {
            *dst++ = *src++;
            bcount++;
        }
        else if (*src == '"')
        {
            if ((bcount & 1) == 0)
            {
                /* Preceded by an even number of '\', this is half that
                 * number of '\', plus a '"' which we discard.
                 */
                dst -= bcount / 2;
                src++;
                if (in_quotes && *src == '"') *dst++ = *src++;
                else in_quotes = !in_quotes;
            }
            else
            {
                /* Preceded by an odd number of '\', this is half that
                 * number of '\' followed by a '"'
                 */
                dst -= bcount / 2 + 1;
                *dst++ = *src++;
            }
            bcount = 0;
        }
        else  /* a regular character */
        {
            *dst++ = *src++;
            bcount = 0;
        }
    }
    *dst = 0;
    argv[argc++] = arg;
    argv[argc] = NULL;
    return argv;
}


/***********************************************************************
 *           get_so_file_info
 */
static BOOL get_so_file_info( int fd, struct pe_image_info *info )
{
    union
    {
        struct
        {
            unsigned char magic[4];
            unsigned char class;
            unsigned char data;
            unsigned char version;
            unsigned char ignored1[9];
            unsigned short type;
            unsigned short machine;
            unsigned char ignored2[8];
            unsigned int phoff;
            unsigned char ignored3[12];
            unsigned short phnum;
        } elf;
        struct
        {
            unsigned char magic[4];
            unsigned char class;
            unsigned char data;
            unsigned char ignored1[10];
            unsigned short type;
            unsigned short machine;
            unsigned char ignored2[12];
            unsigned __int64 phoff;
            unsigned char ignored3[16];
            unsigned short phnum;
        } elf64;
        struct
        {
            unsigned int magic;
            unsigned int cputype;
            unsigned int cpusubtype;
            unsigned int filetype;
        } macho;
        IMAGE_DOS_HEADER mz;
    } header;

    off_t pos;

    if (pread( fd, &header, sizeof(header), 0 ) != sizeof(header)) return FALSE;

    if (!memcmp( header.elf.magic, "\177ELF", 4 ))
    {
        unsigned int type;
        unsigned short phnum;

        if (header.elf.version != 1 /* EV_CURRENT */) return FALSE;
#ifdef WORDS_BIGENDIAN
        if (header.elf.data != 2 /* ELFDATA2MSB */) return FALSE;
#else
        if (header.elf.data != 1 /* ELFDATA2LSB */) return FALSE;
#endif
        switch (header.elf.machine)
        {
        case 3:   info->machine = IMAGE_FILE_MACHINE_I386; break;
        case 40:  info->machine = IMAGE_FILE_MACHINE_ARMNT; break;
        case 62:  info->machine = IMAGE_FILE_MACHINE_AMD64; break;
        case 183: info->machine = IMAGE_FILE_MACHINE_ARM64; break;
        }
        if (header.elf.type != 3 /* ET_DYN */) return FALSE;
        if (header.elf.class == 2 /* ELFCLASS64 */)
        {
            pos = header.elf64.phoff;
            phnum = header.elf64.phnum;
        }
        else
        {
            pos = header.elf.phoff;
            phnum = header.elf.phnum;
        }
        while (phnum--)
        {
            if (pread( fd, &type, sizeof(type), pos ) != sizeof(type)) return FALSE;
            if (type == 3 /* PT_INTERP */) return FALSE;
            pos += (header.elf.class == 2) ? 56 : 32;
        }
        return TRUE;
    }
    else if (header.macho.magic == 0xfeedface || header.macho.magic == 0xfeedfacf)
    {
        switch (header.macho.cputype)
        {
        case 0x00000007: info->machine = IMAGE_FILE_MACHINE_I386; break;
        case 0x01000007: info->machine = IMAGE_FILE_MACHINE_AMD64; break;
        case 0x0000000c: info->machine = IMAGE_FILE_MACHINE_ARMNT; break;
        case 0x0100000c: info->machine = IMAGE_FILE_MACHINE_ARM64; break;
        }
        if (header.macho.filetype == 8) return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *           get_pe_file_info
 */
static unsigned int get_pe_file_info( OBJECT_ATTRIBUTES *attr, UNICODE_STRING *nt_name,
                                      char **unix_name, HANDLE *handle, struct pe_image_info *info )
{
    unsigned int status;
    HANDLE mapping;

    *handle = 0;
    memset( info, 0, sizeof(*info) );
    if (!(status = get_nt_and_unix_names( attr, nt_name, unix_name, FILE_OPEN )))
    {
        status = open_unix_file( handle, *unix_name, GENERIC_READ, attr, 0,
                                 FILE_SHARE_READ | FILE_SHARE_DELETE,
                                 FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0 );
    }
    if (status)
    {
        if (is_builtin_path( attr->ObjectName, &info->machine ))
        {
            TRACE( "assuming %04x builtin for %s\n", info->machine, debugstr_us(attr->ObjectName));
            return STATUS_SUCCESS;
        }
        return status;
    }

    if (!(status = NtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY |
                                    SECTION_MAP_READ | SECTION_MAP_EXECUTE,
                                    NULL, NULL, PAGE_EXECUTE_READ, SEC_IMAGE, *handle )))
    {
        SERVER_START_REQ( get_mapping_info )
        {
            req->handle = wine_server_obj_handle( mapping );
            req->access = SECTION_QUERY;
            wine_server_set_reply( req, info, sizeof(*info) );
            status = wine_server_call( req );
        }
        SERVER_END_REQ;
        NtClose( mapping );
        if (info->image_charact & IMAGE_FILE_DLL) return STATUS_INVALID_IMAGE_FORMAT;
    }
    else if (status == STATUS_INVALID_IMAGE_NOT_MZ)
    {
        int unix_fd, needs_close;

        if (!server_get_unix_fd( *handle, FILE_READ_DATA, &unix_fd, &needs_close, NULL, NULL ))
        {
            if (get_so_file_info( unix_fd, info )) status = STATUS_SUCCESS;
            if (needs_close) close( unix_fd );
        }
    }
    return status;
}


/***********************************************************************
 *           get_env_size
 */
static ULONG get_env_size( const RTL_USER_PROCESS_PARAMETERS *params, char **winedebug )
{
    WCHAR *ptr = params->Environment;

    while (*ptr)
    {
        static const WCHAR WINEDEBUG[] = {'W','I','N','E','D','E','B','U','G','=',0};
        if (!*winedebug && !wcsncmp( ptr, WINEDEBUG, ARRAY_SIZE( WINEDEBUG ) - 1 ))
        {
            DWORD len = wcslen(ptr) * 3 + 1;
            if ((*winedebug = malloc( len )))
                ntdll_wcstoumbs( ptr, wcslen(ptr) + 1, *winedebug, len, FALSE );
        }
        ptr += wcslen(ptr) + 1;
    }
    ptr++;
    return (ptr - params->Environment) * sizeof(WCHAR);
}


/***********************************************************************
 *           get_unix_curdir
 */
static int get_unix_curdir( const RTL_USER_PROCESS_PARAMETERS *params )
{
    UNICODE_STRING nt_name, true_nt_name;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    HANDLE handle;
    int fd = -1;
    char *unix_name;

    if (get_nt_path( params->CurrentDirectory.DosPath.Buffer, &nt_name )) return -1;
    nt_name.Length = wcslen( nt_name.Buffer ) * sizeof(WCHAR);

    InitializeObjectAttributes( &attr, &nt_name, OBJ_CASE_INSENSITIVE, 0, NULL );
    status = get_nt_and_unix_names( &attr, &true_nt_name, &unix_name, FILE_OPEN );
    if (status) goto done;
    status = open_unix_file( &handle, unix_name, FILE_TRAVERSE | SYNCHRONIZE, &attr, 0,
                             FILE_SHARE_READ | FILE_SHARE_DELETE,
                             FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0 );
    if (status) goto done;
    wine_server_handle_to_fd( handle, FILE_TRAVERSE, &fd, NULL );
    NtClose( handle );

done:
    free( unix_name );
    free( nt_name.Buffer );
    free( true_nt_name.Buffer );
    return fd;
}


/***********************************************************************
 *           set_stdio_fd
 */
static void set_stdio_fd( int stdin_fd, int stdout_fd )
{
    int fd = -1;

    if (stdin_fd == -1 || stdout_fd == -1)
    {
        fd = open( "/dev/null", O_RDWR );
        if (stdin_fd == -1) stdin_fd = fd;
        if (stdout_fd == -1) stdout_fd = fd;
    }

    if (stdin_fd != 0) dup2( stdin_fd, 0 );
    if (stdout_fd != 1) dup2( stdout_fd, 1 );
    if (fd != -1) close( fd );
}


/***********************************************************************
 *           is_unix_console_handle
 */
static BOOL is_unix_console_handle( HANDLE handle )
{
    return !sync_ioctl( handle, IOCTL_CONDRV_IS_UNIX, NULL, 0, NULL, 0 );
}


/***********************************************************************
 *           spawn_process
 */
static NTSTATUS spawn_process( const RTL_USER_PROCESS_PARAMETERS *params, int socketfd,
                               int unixdir, char *winedebug, const struct pe_image_info *pe_info )
{
    NTSTATUS status = STATUS_SUCCESS;
    int stdin_fd = -1, stdout_fd = -1;
    pid_t pid;
    char **argv;

    if (wine_server_handle_to_fd( params->hStdInput, FILE_READ_DATA, &stdin_fd, NULL ) &&
        isatty(0) && is_unix_console_handle( params->hStdInput ))
        stdin_fd = 0;

    if (wine_server_handle_to_fd( params->hStdOutput, FILE_WRITE_DATA, &stdout_fd, NULL ) &&
        isatty(1) && is_unix_console_handle( params->hStdOutput ))
        stdout_fd = 1;

    if (!(pid = fork()))  /* child */
    {
        if (!(pid = fork()))  /* grandchild */
        {
            if ((peb->ProcessParameters && params->ProcessGroupId != peb->ProcessParameters->ProcessGroupId) ||
                params->ConsoleHandle == CONSOLE_HANDLE_ALLOC ||
                params->ConsoleHandle == CONSOLE_HANDLE_ALLOC_NO_WINDOW ||
                params->ConsoleHandle == NULL)
            {
                setsid();
                set_stdio_fd( -1, -1 );  /* close stdin and stdout */
            }
            else set_stdio_fd( stdin_fd, stdout_fd );

            if (stdin_fd != -1 && stdin_fd != 0) close( stdin_fd );
            if (stdout_fd != -1 && stdout_fd != 1) close( stdout_fd );

            if (winedebug) putenv( winedebug );
            if (unixdir != -1)
            {
                fchdir( unixdir );
                close( unixdir );
            }
            argv = build_argv( &params->CommandLine, 2 );

            exec_wineloader( argv, socketfd, pe_info );
            _exit(1);
        }

        _exit(pid == -1);
    }

    if (pid != -1)
    {
        /* reap child */
        pid_t wret;
        do {
            wret = waitpid(pid, NULL, 0);
        } while (wret < 0 && errno == EINTR);
    }
    else status = STATUS_NO_MEMORY;

    if (stdin_fd != -1 && stdin_fd != 0) close( stdin_fd );
    if (stdout_fd != -1 && stdout_fd != 1) close( stdout_fd );
    return status;
}


/***********************************************************************
 *           __wine_unix_spawnvp
 */
NTSTATUS WINAPI __wine_unix_spawnvp( char * const argv[], int wait )
{
    pid_t pid, wret;
    int fd[2], status, err;

#ifdef HAVE_PIPE2
    if (pipe2( fd, O_CLOEXEC ) == -1)
#endif
    {
        if (pipe(fd) == -1) return STATUS_TOO_MANY_OPENED_FILES;
        fcntl( fd[0], F_SETFD, FD_CLOEXEC );
        fcntl( fd[1], F_SETFD, FD_CLOEXEC );
    }

    if (!(pid = fork()))
    {
        /* in child */
        close( fd[0] );
        signal( SIGPIPE, SIG_DFL );
        if (!wait)
        {
            if (!(pid = fork())) execvp( argv[0], argv ); /* in grandchild */
            if (pid > 0) _exit(0); /* exit child if fork succeeded */
        }
        else execvp( argv[0], argv );

        err = errno_to_status( errno );
        write( fd[1], &err, sizeof(err) );
        _exit(1);
    }
    close( fd[1] );

    if (pid != -1)
    {
        while (pid != (wret = waitpid( pid, &status, 0 )))
            if (wret == -1 && errno != EINTR) break;

        if (read( fd[0], &err, sizeof(err) ) <= 0)  /* if we read something, exec or second fork failed */
        {
            if (pid == wret && WIFEXITED(status)) err = WEXITSTATUS(status);
            else err = 255;  /* abnormal exit with an abort or an interrupt */
        }
    }
    else err = errno_to_status( errno );

    close( fd[0] );
    return err;
}


/***********************************************************************
 *           unixcall_wine_spawnvp
 */
NTSTATUS unixcall_wine_spawnvp( void *args )
{
    struct wine_spawnvp_params *params = args;

    return __wine_unix_spawnvp( params->argv, params->wait );
}


#ifdef _WIN64
/***********************************************************************
 *		wow64_wine_spawnvp
 */
NTSTATUS wow64_wine_spawnvp( void *args )
{
    struct
    {
        ULONG argv;
        int   wait;
    } const *params32 = args;

    ULONG *argv32 = ULongToPtr( params32->argv );
    unsigned int i, count = 0;
    char **argv;
    NTSTATUS ret;

    while (argv32[count]) count++;
    argv = malloc( (count + 1) * sizeof(*argv) );
    for (i = 0; i < count; i++) argv[i] = ULongToPtr( argv32[i] );
    argv[count] = NULL;
    ret = __wine_unix_spawnvp( argv, params32->wait );
    free( argv );
    return ret;
}
#endif

/***********************************************************************
 *           fork_and_exec
 *
 * Fork and exec a new Unix binary, checking for errors.
 */
static NTSTATUS fork_and_exec( OBJECT_ATTRIBUTES *attr, const char *unix_name, int unixdir,
                               const RTL_USER_PROCESS_PARAMETERS *params )
{
    pid_t pid;
    int fd[2], stdin_fd = -1, stdout_fd = -1;
    char **argv, **envp;
    NTSTATUS status = STATUS_SUCCESS;

#ifdef HAVE_PIPE2
    if (pipe2( fd, O_CLOEXEC ) == -1)
#endif
    {
        if (pipe(fd) == -1) return STATUS_TOO_MANY_OPENED_FILES;
        fcntl( fd[0], F_SETFD, FD_CLOEXEC );
        fcntl( fd[1], F_SETFD, FD_CLOEXEC );
    }

    if (wine_server_handle_to_fd( params->hStdInput, FILE_READ_DATA, &stdin_fd, NULL ) &&
        isatty(0) && is_unix_console_handle( params->hStdInput ))
        stdin_fd = 0;

    if (wine_server_handle_to_fd( params->hStdOutput, FILE_WRITE_DATA, &stdout_fd, NULL ) &&
        isatty(1) && is_unix_console_handle( params->hStdOutput ))
        stdout_fd = 1;

    if (!(pid = fork()))  /* child */
    {
        if (!(pid = fork()))  /* grandchild */
        {
            close( fd[0] );

            if ((peb->ProcessParameters && params->ProcessGroupId != peb->ProcessParameters->ProcessGroupId) ||
                params->ConsoleHandle == CONSOLE_HANDLE_ALLOC ||
                params->ConsoleHandle == CONSOLE_HANDLE_ALLOC_NO_WINDOW ||
                params->ConsoleHandle == NULL)
            {
                setsid();
                set_stdio_fd( -1, -1 );  /* close stdin and stdout */
            }
            else set_stdio_fd( stdin_fd, stdout_fd );

            if (stdin_fd != -1 && stdin_fd != 0) close( stdin_fd );
            if (stdout_fd != -1 && stdout_fd != 1) close( stdout_fd );

            /* Reset signals that we previously set to SIG_IGN */
            signal( SIGPIPE, SIG_DFL );

            argv = build_argv( &params->CommandLine, 0 );
            envp = build_envp( params->Environment );
            if (unixdir != -1)
            {
                fchdir( unixdir );
                close( unixdir );
            }
            execve( unix_name, argv, envp );
        }

        if (pid <= 0)  /* grandchild if exec failed or child if fork failed */
        {
            switch (errno)
            {
            case EPERM:
            case EACCES: status = STATUS_ACCESS_DENIED; break;
            case ENOENT: status = STATUS_OBJECT_NAME_NOT_FOUND; break;
            case EMFILE:
            case ENFILE: status = STATUS_TOO_MANY_OPENED_FILES; break;
            case ENOEXEC:
            case EINVAL: status = STATUS_INVALID_IMAGE_FORMAT; break;
            default:     status = STATUS_NO_MEMORY; break;
            }
            write( fd[1], &status, sizeof(status) );
            _exit(1);
        }
        _exit(0); /* child if fork succeeded */
    }
    close( fd[1] );

    if (pid != -1)
    {
        /* reap child */
        pid_t wret;
        do {
            wret = waitpid(pid, NULL, 0);
        } while (wret < 0 && errno == EINTR);
        read( fd[0], &status, sizeof(status) );  /* if we read something, exec or second fork failed */
    }
    else status = STATUS_NO_MEMORY;

    close( fd[0] );
    if (stdin_fd != -1 && stdin_fd != 0) close( stdin_fd );
    if (stdout_fd != -1 && stdout_fd != 1) close( stdout_fd );
    return status;
}

static NTSTATUS alloc_handle_list( const PS_ATTRIBUTE *handles_attr, obj_handle_t **handles, data_size_t *handles_len )
{
    SIZE_T count, i;
    HANDLE *src;

    *handles = NULL;
    *handles_len = 0;

    if (!handles_attr) return STATUS_SUCCESS;

    count = handles_attr->Size / sizeof(HANDLE);

    if (!(*handles = calloc( sizeof(**handles), count ))) return STATUS_NO_MEMORY;

    src = handles_attr->ValuePtr;
    for (i = 0; i < count; ++i)
        (*handles)[i] = wine_server_obj_handle( src[i] );

    *handles_len = count * sizeof(**handles);

    return STATUS_SUCCESS;
}

/**********************************************************************
 *           NtCreateUserProcess  (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateUserProcess( HANDLE *process_handle_ptr, HANDLE *thread_handle_ptr,
                                     ACCESS_MASK process_access, ACCESS_MASK thread_access,
                                     OBJECT_ATTRIBUTES *process_attr, OBJECT_ATTRIBUTES *thread_attr,
                                     ULONG process_flags, ULONG thread_flags,
                                     RTL_USER_PROCESS_PARAMETERS *params, PS_CREATE_INFO *info,
                                     PS_ATTRIBUTE_LIST *ps_attr )
{
    unsigned int status;
    BOOL success = FALSE;
    HANDLE file_handle, process_info = 0, process_handle = 0, thread_handle = 0;
    struct object_attributes *objattr;
    data_size_t attr_len;
    char *winedebug = NULL;
    char *unix_name = NULL;
    struct startup_info_data *startup_info = NULL;
    ULONG startup_info_size, env_size;
    int unixdir, socketfd[2] = { -1, -1 };
    struct pe_image_info pe_info;
    CLIENT_ID id;
    USHORT machine = 0;
    HANDLE parent = 0, debug = 0, token = 0;
    UNICODE_STRING nt_name, path = {0};
    OBJECT_ATTRIBUTES attr, empty_attr = { sizeof(empty_attr) };
    SIZE_T i, attr_count = (ps_attr->TotalLength - sizeof(ps_attr->TotalLength)) / sizeof(PS_ATTRIBUTE);
    const PS_ATTRIBUTE *handles_attr = NULL, *jobs_attr = NULL;
    data_size_t handles_size, jobs_size;
    obj_handle_t *handles, *jobs;

    if (thread_flags & THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER)
    {
        WARN( "Invalid thread flags %#x.\n", thread_flags );

        return STATUS_INVALID_PARAMETER;
    }

    if (thread_flags & ~THREAD_CREATE_FLAGS_CREATE_SUSPENDED)
        FIXME( "Unsupported thread flags %#x.\n", thread_flags );

    for (i = 0; i < attr_count; i++)
    {
        switch (ps_attr->Attributes[i].Attribute)
        {
        case PS_ATTRIBUTE_PARENT_PROCESS:
            parent = ps_attr->Attributes[i].ValuePtr;
            break;
        case PS_ATTRIBUTE_DEBUG_PORT:
            debug = ps_attr->Attributes[i].ValuePtr;
            break;
        case PS_ATTRIBUTE_IMAGE_NAME:
            path.Length = ps_attr->Attributes[i].Size;
            path.Buffer = ps_attr->Attributes[i].ValuePtr;
            break;
        case PS_ATTRIBUTE_TOKEN:
            token = ps_attr->Attributes[i].ValuePtr;
            break;
        case PS_ATTRIBUTE_HANDLE_LIST:
            if (process_flags & PROCESS_CREATE_FLAGS_INHERIT_HANDLES)
                handles_attr = &ps_attr->Attributes[i];
            break;
        case PS_ATTRIBUTE_JOB_LIST:
            jobs_attr = &ps_attr->Attributes[i];
            break;
        case PS_ATTRIBUTE_MACHINE_TYPE:
            machine = ps_attr->Attributes[i].Value;
            break;
        default:
            if (ps_attr->Attributes[i].Attribute & PS_ATTRIBUTE_INPUT)
                FIXME( "unhandled input attribute %lx\n", ps_attr->Attributes[i].Attribute );
            break;
        }
    }
    if (!process_attr) process_attr = &empty_attr;

    TRACE( "%s image %s cmdline %s parent %p machine %x\n", debugstr_us( &path ),
           debugstr_us( &params->ImagePathName ), debugstr_us( &params->CommandLine ), parent, machine );

    unixdir = get_unix_curdir( params );

    InitializeObjectAttributes( &attr, &path, OBJ_CASE_INSENSITIVE, 0, 0 );
    if ((status = get_pe_file_info( &attr, &nt_name, &unix_name, &file_handle, &pe_info )))
    {
        if (status == STATUS_INVALID_IMAGE_NOT_MZ && !fork_and_exec( &attr, unix_name, unixdir, params ))
        {
            memset( info, 0, sizeof(*info) );
            free( unix_name );
            free( nt_name.Buffer );
            return STATUS_SUCCESS;
        }
        goto done;
    }
    if (!machine)
    {
        machine = pe_info.machine;
        if (is_arm64ec() && pe_info.is_hybrid && machine == IMAGE_FILE_MACHINE_ARM64)
            machine = main_image_info.Machine;
    }
    if (!(startup_info = create_startup_info( attr.ObjectName, process_flags, params, &pe_info, &startup_info_size )))
        goto done;
    env_size = get_env_size( params, &winedebug );

    if ((status = alloc_object_attributes( process_attr, &objattr, &attr_len ))) goto done;

    if ((status = alloc_handle_list( handles_attr, &handles, &handles_size )))
    {
        free( objattr );
        goto done;
    }

    if ((status = alloc_handle_list( jobs_attr, &jobs, &jobs_size )))
    {
        free( objattr );
        free( handles );
        goto done;
    }

    /* create the socket for the new process */

    if (socketpair( PF_UNIX, SOCK_STREAM, 0, socketfd ) == -1)
    {
        status = STATUS_TOO_MANY_OPENED_FILES;
        free( objattr );
        free( handles );
        free( jobs );
        goto done;
    }
#ifdef SO_PASSCRED
    else
    {
        int enable = 1;
        setsockopt( socketfd[0], SOL_SOCKET, SO_PASSCRED, &enable, sizeof(enable) );
    }
#endif

    wine_server_send_fd( socketfd[1] );

    /* create the process on the server side */

    SERVER_START_REQ( new_process )
    {
        req->token          = wine_server_obj_handle( token );
        req->debug          = wine_server_obj_handle( debug );
        req->parent_process = wine_server_obj_handle( parent );
        req->flags          = process_flags;
        req->socket_fd      = socketfd[1];
        req->access         = process_access;
        req->machine        = machine;
        req->info_size      = startup_info_size;
        req->handles_size   = handles_size;
        req->jobs_size      = jobs_size;
        wine_server_add_data( req, objattr, attr_len );
        wine_server_add_data( req, handles, handles_size );
        wine_server_add_data( req, jobs, jobs_size );
        wine_server_add_data( req, startup_info, startup_info_size );
        wine_server_add_data( req, params->Environment, env_size );
        if (!(status = wine_server_call( req )))
        {
            process_handle = wine_server_ptr_handle( reply->handle );
            id.UniqueProcess = ULongToHandle( reply->pid );
        }
        process_info = wine_server_ptr_handle( reply->info );
    }
    SERVER_END_REQ;
    close( socketfd[1] );
    free( objattr );
    free( handles );
    free( jobs );

    if (status)
    {
        switch (status)
        {
        case STATUS_INVALID_IMAGE_WIN_64:
            ERR( "64-bit application %s not supported in 32-bit prefix\n", debugstr_us(&path) );
            break;
        case STATUS_INVALID_IMAGE_FORMAT:
            ERR( "%s not supported on this installation (machine %04x)\n",
                 debugstr_us(&path), pe_info.machine );
            break;
        }
        goto done;
    }

    if ((status = alloc_object_attributes( thread_attr, &objattr, &attr_len ))) goto done;

    SERVER_START_REQ( new_thread )
    {
        req->process    = wine_server_obj_handle( process_handle );
        req->access     = thread_access;
        req->flags      = thread_flags;
        req->request_fd = -1;
        wine_server_add_data( req, objattr, attr_len );
        if (!(status = wine_server_call( req )))
        {
            thread_handle = wine_server_ptr_handle( reply->handle );
            id.UniqueThread = ULongToHandle( reply->tid );
        }
    }
    SERVER_END_REQ;
    free( objattr );
    if (status) goto done;

    /* create the child process */

    if ((status = spawn_process( params, socketfd[0], unixdir, winedebug, &pe_info ))) goto done;

    close( socketfd[0] );
    socketfd[0] = -1;

    /* wait for the new process info to be ready */

    NtWaitForSingleObject( process_info, FALSE, NULL );
    SERVER_START_REQ( get_new_process_info )
    {
        req->info = wine_server_obj_handle( process_info );
        wine_server_call( req );
        success = reply->success;
        status = reply->exit_code;
    }
    SERVER_END_REQ;

    if (!success)
    {
        if (!status) status = STATUS_INTERNAL_ERROR;
        goto done;
    }

    TRACE( "%s pid %04x tid %04x handles %p/%p\n", debugstr_us(&path),
           HandleToULong(id.UniqueProcess), HandleToULong(id.UniqueThread),
           process_handle, thread_handle );

    /* update output attributes */

    for (i = 0; i < attr_count; i++)
    {
        switch (ps_attr->Attributes[i].Attribute)
        {
        case PS_ATTRIBUTE_CLIENT_ID:
        {
            SIZE_T size = min( ps_attr->Attributes[i].Size, sizeof(id) );
            memcpy( ps_attr->Attributes[i].ValuePtr, &id, size );
            if (ps_attr->Attributes[i].ReturnLength) *ps_attr->Attributes[i].ReturnLength = size;
            break;
        }
        case PS_ATTRIBUTE_IMAGE_INFO:
        {
            SECTION_IMAGE_INFORMATION info;
            SIZE_T size = min( ps_attr->Attributes[i].Size, sizeof(info) );
            virtual_fill_image_information( &pe_info, &info );
            memcpy( ps_attr->Attributes[i].ValuePtr, &info, size );
            if (ps_attr->Attributes[i].ReturnLength) *ps_attr->Attributes[i].ReturnLength = size;
            break;
        }
        case PS_ATTRIBUTE_TEB_ADDRESS:
        default:
            if (!(ps_attr->Attributes[i].Attribute & PS_ATTRIBUTE_INPUT))
                FIXME( "unhandled output attribute %lx\n", ps_attr->Attributes[i].Attribute );
            break;
        }
    }
    *process_handle_ptr = process_handle;
    *thread_handle_ptr = thread_handle;
    process_handle = thread_handle = 0;
    status = STATUS_SUCCESS;

done:
    if (file_handle) NtClose( file_handle );
    if (process_info) NtClose( process_info );
    if (process_handle) NtClose( process_handle );
    if (thread_handle) NtClose( thread_handle );
    if (socketfd[0] != -1) close( socketfd[0] );
    if (unixdir != -1) close( unixdir );
    free( startup_info );
    free( winedebug );
    free( unix_name );
    free( nt_name.Buffer );
    return status;
}


/******************************************************************************
 *              NtTerminateProcess  (NTDLL.@)
 */
NTSTATUS WINAPI NtTerminateProcess( HANDLE handle, LONG exit_code )
{
    unsigned int ret;
    BOOL self;

    SERVER_START_REQ( terminate_process )
    {
        req->handle    = wine_server_obj_handle( handle );
        req->exit_code = exit_code;
        ret = wine_server_call( req );
        self = reply->self;
    }
    SERVER_END_REQ;
    if (self)
    {
        if (!handle) process_exiting = TRUE;
        else if (process_exiting) exit_process( exit_code );
        else abort_process( exit_code );
    }
    return ret;
}


#if defined(HAVE_MACH_MACH_H)

void fill_vm_counters( VM_COUNTERS_EX *pvmi, int unix_pid )
{
#if defined(MACH_TASK_BASIC_INFO)
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount;

    if (unix_pid != -1) return; /* FIXME: Retrieve information for other processes. */

    infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if(task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &infoCount) == KERN_SUCCESS)
    {
        pvmi->VirtualSize = info.resident_size + info.virtual_size;
        pvmi->PagefileUsage = info.virtual_size;
        pvmi->WorkingSetSize = info.resident_size;
        pvmi->PeakWorkingSetSize = info.resident_size_max;
    }
#endif
}

#elif defined(linux)

void fill_vm_counters( VM_COUNTERS_EX *pvmi, int unix_pid )
{
    FILE *f;
    char line[256], path[26];
    unsigned long value;

    if (unix_pid == -1)
        strcpy( path, "/proc/self/status" );
    else
        snprintf( path, sizeof(path), "/proc/%u/status", unix_pid);
    f = fopen( path, "r" );
    if (!f) return;

    while (fgets(line, sizeof(line), f))
    {
        if (sscanf(line, "VmPeak: %lu", &value))
            pvmi->PeakVirtualSize = (ULONG64)value * 1024;
        else if (sscanf(line, "VmSize: %lu", &value))
            pvmi->VirtualSize = (ULONG64)value * 1024;
        else if (sscanf(line, "VmHWM: %lu", &value))
            pvmi->PeakWorkingSetSize = (ULONG64)value * 1024;
        else if (sscanf(line, "VmRSS: %lu", &value))
            pvmi->WorkingSetSize = (ULONG64)value * 1024;
        else if (sscanf(line, "RssAnon: %lu", &value))
            pvmi->PagefileUsage += (ULONG64)value * 1024;
        else if (sscanf(line, "VmSwap: %lu", &value))
            pvmi->PagefileUsage += (ULONG64)value * 1024;
    }
    pvmi->PeakPagefileUsage = pvmi->PagefileUsage;

    fclose(f);
}

#elif defined(HAVE_LIBPROCSTAT)

void fill_vm_counters( VM_COUNTERS_EX *pvmi, int unix_pid )
{
    struct procstat *pstat;
    struct kinfo_proc *kip;
    unsigned int proc_count;

    pstat = procstat_open_sysctl();
    if (pstat)
    {
        kip = procstat_getprocs( pstat, KERN_PROC_PID, unix_pid == -1 ? getpid() : unix_pid, &proc_count );
        if (kip)
        {
            pvmi->VirtualSize = kip->ki_size;
            pvmi->PeakVirtualSize = kip->ki_size;
            pvmi->WorkingSetSize = kip->ki_rssize << PAGE_SHIFT;
            pvmi->PeakWorkingSetSize = kip->ki_rusage.ru_maxrss * 1024;
            procstat_freeprocs( pstat, kip );
        }
        procstat_close( pstat );
    }
}

#else

void fill_vm_counters( VM_COUNTERS_EX *pvmi, int unix_pid )
{
    /* FIXME : real data */
}

#endif

#define UNIMPLEMENTED_INFO_CLASS(c) \
    case c: \
        FIXME( "(process=%p) Unimplemented information class: " #c "\n", handle); \
        ret = STATUS_INVALID_INFO_CLASS; \
        break

/**********************************************************************
 *           NtQueryInformationProcess  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryInformationProcess( HANDLE handle, PROCESSINFOCLASS class, void *info,
                                           ULONG size, ULONG *ret_len )
{
    unsigned int ret = STATUS_SUCCESS;
    ULONG len = 0;

    TRACE( "(%p,0x%08x,%p,0x%08x,%p)\n", handle, class, info, size, ret_len );

    switch (class)
    {
    UNIMPLEMENTED_INFO_CLASS(ProcessBasePriority);
    UNIMPLEMENTED_INFO_CLASS(ProcessRaisePriority);
    UNIMPLEMENTED_INFO_CLASS(ProcessExceptionPort);
    UNIMPLEMENTED_INFO_CLASS(ProcessAccessToken);
    UNIMPLEMENTED_INFO_CLASS(ProcessLdtInformation);
    UNIMPLEMENTED_INFO_CLASS(ProcessLdtSize);
    UNIMPLEMENTED_INFO_CLASS(ProcessIoPortHandlers);
    UNIMPLEMENTED_INFO_CLASS(ProcessPooledUsageAndLimits);
    UNIMPLEMENTED_INFO_CLASS(ProcessWorkingSetWatch);
    UNIMPLEMENTED_INFO_CLASS(ProcessUserModeIOPL);
    UNIMPLEMENTED_INFO_CLASS(ProcessEnableAlignmentFaultFixup);
    UNIMPLEMENTED_INFO_CLASS(ProcessWx86Information);
    UNIMPLEMENTED_INFO_CLASS(ProcessDeviceMap);
    UNIMPLEMENTED_INFO_CLASS(ProcessForegroundInformation);
    UNIMPLEMENTED_INFO_CLASS(ProcessLUIDDeviceMapsEnabled);
    UNIMPLEMENTED_INFO_CLASS(ProcessBreakOnTermination);
    UNIMPLEMENTED_INFO_CLASS(ProcessHandleTracing);

    case ProcessBasicInformation:
        {
            PROCESS_BASIC_INFORMATION pbi;
            const ULONG_PTR affinity_mask = get_system_affinity_mask();

            if (size >= sizeof(PROCESS_BASIC_INFORMATION))
            {
                if (!info) ret = STATUS_ACCESS_VIOLATION;
                else
                {
                    SERVER_START_REQ(get_process_info)
                    {
                        req->handle = wine_server_obj_handle( handle );
                        if ((ret = wine_server_call( req )) == STATUS_SUCCESS)
                        {
                            pbi.ExitStatus = reply->exit_code;
                            pbi.PebBaseAddress = wine_server_get_ptr( reply->peb );
                            pbi.AffinityMask = reply->affinity & affinity_mask;
                            pbi.BasePriority = reply->base_priority;
                            pbi.UniqueProcessId = reply->pid;
                            pbi.InheritedFromUniqueProcessId = reply->ppid;
                            if (is_old_wow64())
                            {
                                if (reply->machine != native_machine)
                                    pbi.PebBaseAddress = (PEB *)((char *)pbi.PebBaseAddress + 0x1000);
                                else
                                    pbi.PebBaseAddress = NULL;
                            }
                        }
                    }
                    SERVER_END_REQ;

                    memcpy( info, &pbi, sizeof(PROCESS_BASIC_INFORMATION) );
                    len = sizeof(PROCESS_BASIC_INFORMATION);
                }
                if (size > sizeof(PROCESS_BASIC_INFORMATION)) ret = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                len = sizeof(PROCESS_BASIC_INFORMATION);
                ret = STATUS_INFO_LENGTH_MISMATCH;
            }
        }
        break;

    case ProcessIoCounters:
        {
            IO_COUNTERS pii;

            if (size >= sizeof(IO_COUNTERS))
            {
                if (!info) ret = STATUS_ACCESS_VIOLATION;
                else if (!handle) ret = STATUS_INVALID_HANDLE;
                else
                {
                    /* FIXME : real data */
                    memset(&pii, 0 , sizeof(IO_COUNTERS));
                    memcpy(info, &pii, sizeof(IO_COUNTERS));
                    len = sizeof(IO_COUNTERS);
                }
                if (size > sizeof(IO_COUNTERS)) ret = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                len = sizeof(IO_COUNTERS);
                ret = STATUS_INFO_LENGTH_MISMATCH;
            }
        }
        break;

    case ProcessVmCounters:
        {
            VM_COUNTERS_EX pvmi;

            /* older Windows versions don't have the PrivateUsage field */
            if (size >= sizeof(VM_COUNTERS))
            {
                if (!info) ret = STATUS_ACCESS_VIOLATION;
                else
                {
                    memset(&pvmi, 0, sizeof(pvmi));
                    if (handle == GetCurrentProcess()) fill_vm_counters( &pvmi, -1 );
                    else
                    {
                        SERVER_START_REQ(get_process_vm_counters)
                        {
                            req->handle = wine_server_obj_handle( handle );
                            if (!(ret = wine_server_call( req )))
                            {
                                pvmi.PeakVirtualSize = reply->peak_virtual_size;
                                pvmi.VirtualSize = reply->virtual_size;
                                pvmi.PeakWorkingSetSize = reply->peak_working_set_size;
                                pvmi.WorkingSetSize = reply->working_set_size;
                                pvmi.PagefileUsage = reply->pagefile_usage;
                                pvmi.PeakPagefileUsage = reply->peak_pagefile_usage;
                            }
                        }
                        SERVER_END_REQ;
                        if (ret) break;
                    }
                    if (size >= sizeof(VM_COUNTERS_EX))
                        pvmi.PrivateUsage = pvmi.PagefileUsage;
                    len = size;
                    if (len != sizeof(VM_COUNTERS)) len = sizeof(VM_COUNTERS_EX);
                    memcpy(info, &pvmi, min(size, sizeof(pvmi)));
                }
                if (size != sizeof(VM_COUNTERS) && size != sizeof(VM_COUNTERS_EX))
                    ret = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                len = sizeof(pvmi);
                ret = STATUS_INFO_LENGTH_MISMATCH;
            }
        }
        break;

    case ProcessTimes:
        {
            KERNEL_USER_TIMES pti = {{{0}}};

            if (size >= sizeof(KERNEL_USER_TIMES))
            {
                if (!info) ret = STATUS_ACCESS_VIOLATION;
                else if (!handle) ret = STATUS_INVALID_HANDLE;
                else
                {
                    long ticks = sysconf(_SC_CLK_TCK);
                    struct tms tms;

                    /* FIXME: user/kernel times only work for current process */
                    if (ticks && times( &tms ) != -1)
                    {
                        pti.UserTime.QuadPart = (ULONGLONG)tms.tms_utime * 10000000 / ticks;
                        pti.KernelTime.QuadPart = (ULONGLONG)tms.tms_stime * 10000000 / ticks;
                    }

                    SERVER_START_REQ(get_process_info)
                    {
                        req->handle = wine_server_obj_handle( handle );
                        if ((ret = wine_server_call( req )) == STATUS_SUCCESS)
                        {
                            pti.CreateTime.QuadPart = reply->start_time;
                            pti.ExitTime.QuadPart = reply->end_time;
                        }
                    }
                    SERVER_END_REQ;

                    memcpy(info, &pti, sizeof(KERNEL_USER_TIMES));
                    len = sizeof(KERNEL_USER_TIMES);
                }
                if (size > sizeof(KERNEL_USER_TIMES)) ret = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                len = sizeof(KERNEL_USER_TIMES);
                ret = STATUS_INFO_LENGTH_MISMATCH;
            }
        }
        break;

    case ProcessDebugPort:
        len = sizeof(DWORD_PTR);
        if (size != len) return STATUS_INFO_LENGTH_MISMATCH;
        if (!info) ret = STATUS_ACCESS_VIOLATION;
        else
        {
            HANDLE debug;

            SERVER_START_REQ(get_process_debug_info)
            {
                req->handle = wine_server_obj_handle( handle );
                ret = wine_server_call( req );
                debug = wine_server_ptr_handle( reply->debug );
            }
            SERVER_END_REQ;
            if (ret == STATUS_SUCCESS)
            {
                *(DWORD_PTR *)info = ~0ul;
                NtClose( debug );
            }
            else if (ret == STATUS_PORT_NOT_SET)
            {
                *(DWORD_PTR *)info = 0;
                ret = STATUS_SUCCESS;
            }
            else return ret;
        }
        break;

    case ProcessPriorityBoost:
        len = sizeof(ULONG);
        if (size != len) return STATUS_INFO_LENGTH_MISMATCH;
        if (!info) ret = STATUS_ACCESS_VIOLATION;
        else
        {
            ULONG *disable_boost = info;
            SERVER_START_REQ(get_process_info)
            {
                req->handle = wine_server_obj_handle( handle );
                ret = wine_server_call( req );
                *disable_boost = reply->disable_boost;
            }
            SERVER_END_REQ;
        }
        break;

    case ProcessDebugFlags:
        len = sizeof(DWORD);
        if (size == len)
        {
            if (!info) ret = STATUS_ACCESS_VIOLATION;
            else
            {
                HANDLE debug;

                SERVER_START_REQ(get_process_debug_info)
                {
                    req->handle = wine_server_obj_handle( handle );
                    ret = wine_server_call( req );
                    debug = wine_server_ptr_handle( reply->debug );
                    *(DWORD *)info = reply->debug_children;
                }
                SERVER_END_REQ;
                if (ret == STATUS_SUCCESS) NtClose( debug );
                else if (ret == STATUS_PORT_NOT_SET) ret = STATUS_SUCCESS;
            }
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;

    case ProcessDefaultHardErrorMode:
        len = sizeof(process_error_mode);
        if (size == len) memcpy(info, &process_error_mode, len);
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;

    case ProcessDebugObjectHandle:
        len = sizeof(HANDLE);
        if (size && ((ULONG_PTR)info & 3)) return STATUS_DATATYPE_MISALIGNMENT;
        /* STATUS_ACCESS_VIOLATION is returned on Windows for unaccessible ret_len even if ret_len is
         * not going to be written. */
        if (ret_len) *(volatile ULONG *)ret_len |= 0;
        if (size != len) return STATUS_INFO_LENGTH_MISMATCH;
        SERVER_START_REQ(get_process_debug_info)
        {
            req->handle = wine_server_obj_handle( handle );
            ret = wine_server_call( req );
            *(HANDLE *)info = wine_server_ptr_handle( reply->debug );
        }
        SERVER_END_REQ;
        break;

    case ProcessHandleCount:
        if (size >= 4)
        {
            if (!info) ret = STATUS_ACCESS_VIOLATION;
            else if (!handle) ret = STATUS_INVALID_HANDLE;
            else
            {
                FIXME( "ProcessHandleCount (%p,%p,0x%08x,%p) stub\n", handle, info, size, ret_len );
                memset(info, 0, 4);
                len = 4;
            }
            if (size > 4) ret = STATUS_INFO_LENGTH_MISMATCH;
        }
        else
        {
            len = 4;
            ret = STATUS_INFO_LENGTH_MISMATCH;
        }
        break;

    case ProcessHandleTable:
        FIXME( "ProcessHandleTable (%p,%p,0x%08x,%p) stub\n", handle, info, size, ret_len );
        len = 0;
        break;

    case ProcessAffinityMask:
        len = sizeof(ULONG_PTR);
        if (size == len)
        {
            const ULONG_PTR system_mask = get_system_affinity_mask();

            SERVER_START_REQ(get_process_info)
            {
                req->handle = wine_server_obj_handle( handle );
                if (!(ret = wine_server_call( req )))
                    *(ULONG_PTR *)info = reply->affinity & system_mask;
            }
            SERVER_END_REQ;
        }
        else return STATUS_INFO_LENGTH_MISMATCH;
        break;

    case ProcessSessionInformation:
        len = sizeof(DWORD);
        if (size == len)
        {
            SERVER_START_REQ(get_process_info)
            {
                req->handle = wine_server_obj_handle( handle );
                if (!(ret = wine_server_call( req )))
                    *(DWORD *)info = reply->session_id;
            }
            SERVER_END_REQ;
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;

    case ProcessWow64Information:
        len = sizeof(ULONG_PTR);
        if (size != len) return STATUS_INFO_LENGTH_MISMATCH;
        if (handle == GetCurrentProcess())
            *(ULONG_PTR *)info = is_old_wow64() ? (ULONG_PTR)peb : (ULONG_PTR)wow_peb;
        else
        {
            ULONG_PTR val = 0;

            SERVER_START_REQ( get_process_info )
            {
                req->handle = wine_server_obj_handle( handle );
                ret = wine_server_call( req );
                if (!ret && !is_machine_64bit( reply->machine ) && is_machine_64bit( native_machine ))
                    val = reply->peb + 0x1000;
            }
            SERVER_END_REQ;
            if (!ret) *(ULONG_PTR *)info = val;
        }
        break;

    case ProcessImageFileName:
        /* FIXME: Should return a device path */
    case ProcessImageFileNameWin32:
        SERVER_START_REQ( get_process_image_name )
        {
            const unsigned int min_size = sizeof(UNICODE_STRING) + sizeof(WCHAR);
            UNICODE_STRING *str = info;

            req->handle = wine_server_obj_handle( handle );
            req->win32  = (class == ProcessImageFileNameWin32);
            wine_server_set_reply( req, str ? str + 1 : NULL,
                                   size > min_size ? size - min_size : 0 );
            ret = wine_server_call( req );
            if (ret == STATUS_BUFFER_TOO_SMALL) ret = STATUS_INFO_LENGTH_MISMATCH;
            len = min_size + reply->len;
            if (ret == STATUS_SUCCESS)
            {
                str->Length = reply->len;
                str->MaximumLength = str->Length + sizeof(WCHAR);
                str->Buffer = (PWSTR)(str + 1);
                str->Buffer[str->Length / sizeof(WCHAR)] = 0;
            }
        }
        SERVER_END_REQ;
        break;

    case ProcessExecuteFlags:
        len = sizeof(ULONG);
        if (size != len)
            ret = STATUS_INFO_LENGTH_MISMATCH;
        else if (is_win64 && !is_wow64())
            *(ULONG *)info = MEM_EXECUTE_OPTION_DISABLE |
                             MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION |
                             MEM_EXECUTE_OPTION_PERMANENT;
        else
            *(ULONG *)info = execute_flags;
        break;

    case ProcessPriorityClass:
        len = sizeof(PROCESS_PRIORITY_CLASS);
        if (size == len)
        {
            if (!info) ret = STATUS_ACCESS_VIOLATION;
            else
            {
                PROCESS_PRIORITY_CLASS *priority = info;

                SERVER_START_REQ(get_process_info)
                {
                    req->handle = wine_server_obj_handle( handle );
                    if ((ret = wine_server_call( req )) == STATUS_SUCCESS)
                    {
                        priority->PriorityClass = reply->priority;
                        /* FIXME: Not yet supported by the wineserver */
                        priority->Foreground = FALSE;
                    }
                }
                SERVER_END_REQ;
            }
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;

    case ProcessCookie:
        FIXME( "ProcessCookie (%p,%p,0x%08x,%p) stub\n", handle, info, size, ret_len );
        if (handle == NtCurrentProcess())
        {
            len = sizeof(ULONG);
            if (size == len) *(ULONG *)info = 0;
            else ret = STATUS_INFO_LENGTH_MISMATCH;
        }
        else ret = STATUS_INVALID_PARAMETER;
        break;

    case ProcessImageInformation:
        len = sizeof(SECTION_IMAGE_INFORMATION);
        if (size == len)
        {
            if (info)
            {
                struct pe_image_info pe_info;

                SERVER_START_REQ( get_process_info )
                {
                    req->handle = wine_server_obj_handle( handle );
                    wine_server_set_reply( req, &pe_info, sizeof(pe_info) );
                    if ((ret = wine_server_call( req )) == STATUS_SUCCESS)
                        virtual_fill_image_information( &pe_info, info );
                }
                SERVER_END_REQ;
            }
            else ret = STATUS_ACCESS_VIOLATION;
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;

    case ProcessCycleTime:
        len = sizeof(PROCESS_CYCLE_TIME_INFORMATION);
        if (size == len)
        {
            if (!info) ret = STATUS_ACCESS_VIOLATION;
            else
            {
                PROCESS_CYCLE_TIME_INFORMATION cycles;

                FIXME( "ProcessCycleTime (%p,%p,0x%08x,%p) stub\n", handle, info, size, ret_len );
                cycles.AccumulatedCycles = 0;
                cycles.CurrentCycleCount = 0;

                memcpy(info, &cycles, sizeof(PROCESS_CYCLE_TIME_INFORMATION));
            }
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;

    case ProcessWineLdtCopy:
        if (handle == NtCurrentProcess())
        {
#ifdef __i386__
            len = sizeof(struct ldt_copy *);
            if (size == len) *(struct ldt_copy **)info = &__wine_ldt_copy;
            else ret = STATUS_INFO_LENGTH_MISMATCH;
#else
            ret = STATUS_NOT_IMPLEMENTED;
#endif
        }
        else ret = STATUS_INVALID_PARAMETER;
        break;

    case ProcessQuotaLimits:
        {
            QUOTA_LIMITS qlimits;

            FIXME( "ProcessQuotaLimits (%p,%p,0x%08x,%p) stub\n", handle, info, size, ret_len );

            len = sizeof(QUOTA_LIMITS);
            if (size == len)
            {
                if (!handle) ret = STATUS_INVALID_HANDLE;
                else
                {
                    /* FIXME: SetProcessWorkingSetSize can also set the quota values.
                                Quota Limits should be stored inside the process. */
                    qlimits.PagedPoolLimit = (SIZE_T)-1;
                    qlimits.NonPagedPoolLimit = (SIZE_T)-1;
                    /* Default minimum working set size is 204800 bytes (50 Pages) */
                    qlimits.MinimumWorkingSetSize = 204800;
                    /* Default maximum working set size is 1413120 bytes (345 Pages) */
                    qlimits.MaximumWorkingSetSize = 1413120;
                    qlimits.PagefileLimit = (SIZE_T)-1;
                    qlimits.TimeLimit.QuadPart = -1;
                    memcpy(info, &qlimits, len);
                }
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

    default:
        FIXME("(%p,info_class=%d,%p,0x%08x,%p) Unknown information class\n",
              handle, class, info, size, ret_len );
        ret = STATUS_INVALID_INFO_CLASS;
        break;
    }

    if (ret_len) *ret_len = len;
    return ret;
}

#ifndef _WIN64

/**********************************************************************
 *           NtWow64QueryInformationProcess64  (NTDLL.@)
 */
NTSTATUS WINAPI NtWow64QueryInformationProcess64( HANDLE handle, PROCESSINFOCLASS class, void *info,
                                                  ULONG size, ULONG *ret_len )
{
    NTSTATUS ret;
    ULONG len = 0;

    TRACE( "(%p,0x%08x,%p,0x%08x,%p)\n", handle, class, info, size, ret_len );

    switch (class)
    {
    case ProcessBasicInformation:
        {
            PROCESS_BASIC_INFORMATION64 pbi;
            const ULONG_PTR affinity_mask = get_system_affinity_mask();

            if (size >= sizeof(PROCESS_BASIC_INFORMATION64))
            {
                if (!info) ret = STATUS_ACCESS_VIOLATION;
                else
                {
                    SERVER_START_REQ(get_process_info)
                    {
                        req->handle = wine_server_obj_handle( handle );
                        if ((ret = wine_server_call( req )) == STATUS_SUCCESS)
                        {
                            pbi.ExitStatus = reply->exit_code;
                            pbi.PebBaseAddress = (ULONG)wine_server_get_ptr( reply->peb );
                            pbi.AffinityMask = reply->affinity & affinity_mask;
                            pbi.BasePriority = reply->base_priority;
                            pbi.UniqueProcessId = reply->pid;
                            pbi.InheritedFromUniqueProcessId = reply->ppid;
                        }
                    }
                    SERVER_END_REQ;

                    memcpy( info, &pbi, sizeof(PROCESS_BASIC_INFORMATION64) );
                    len = sizeof(PROCESS_BASIC_INFORMATION64);
                }
                if (size > sizeof(PROCESS_BASIC_INFORMATION64)) ret = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                len = sizeof(PROCESS_BASIC_INFORMATION64);
                ret = STATUS_INFO_LENGTH_MISMATCH;
            }
        }
        break;

    default:
        return STATUS_NOT_IMPLEMENTED;
    }

    if (ret_len) *ret_len = len;
    return ret;
}

#endif

/**********************************************************************
 *           NtSetInformationProcess  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetInformationProcess( HANDLE handle, PROCESSINFOCLASS class, void *info, ULONG size )
{
    unsigned int ret = STATUS_SUCCESS;

    switch (class)
    {
    case ProcessAccessToken:
    {
        const PROCESS_ACCESS_TOKEN *token = info;

        if (size != sizeof(PROCESS_ACCESS_TOKEN)) return STATUS_INFO_LENGTH_MISMATCH;

        SERVER_START_REQ( set_process_info )
        {
            req->handle = wine_server_obj_handle( handle );
            req->token = wine_server_obj_handle( token->Token );
            req->mask = SET_PROCESS_INFO_TOKEN;
            ret = wine_server_call( req );
        }
        SERVER_END_REQ;
        break;
    }

    case ProcessDefaultHardErrorMode:
        if (size != sizeof(UINT)) return STATUS_INVALID_PARAMETER;
        process_error_mode = *(UINT *)info;
        break;

    case ProcessAffinityMask:
    {
        const ULONG_PTR system_mask = get_system_affinity_mask();

        if (size != sizeof(DWORD_PTR)) return STATUS_INVALID_PARAMETER;
        if (*(PDWORD_PTR)info & ~system_mask)
            return STATUS_INVALID_PARAMETER;
        if (!*(PDWORD_PTR)info)
            return STATUS_INVALID_PARAMETER;
        SERVER_START_REQ( set_process_info )
        {
            req->handle   = wine_server_obj_handle( handle );
            req->affinity = *(PDWORD_PTR)info;
            req->mask     = SET_PROCESS_INFO_AFFINITY;
            ret = wine_server_call( req );
        }
        SERVER_END_REQ;
        break;
    }
    case ProcessPriorityClass:
        if (size != sizeof(PROCESS_PRIORITY_CLASS)) return STATUS_INVALID_PARAMETER;
        else
        {
            PROCESS_PRIORITY_CLASS* ppc = info;

            SERVER_START_REQ( set_process_info )
            {
                req->handle   = wine_server_obj_handle( handle );
                /* FIXME Foreground isn't used */
                req->priority = ppc->PriorityClass;
                req->mask     = SET_PROCESS_INFO_PRIORITY;
                ret = wine_server_call( req );
            }
            SERVER_END_REQ;
        }
        break;

    case ProcessBasePriority:
        if (size != sizeof(KPRIORITY)) return STATUS_INVALID_PARAMETER;
        else
        {
            KPRIORITY* base_priority = info;

            SERVER_START_REQ( set_process_info )
            {
                req->handle        = wine_server_obj_handle( handle );
                req->base_priority = *base_priority;
                req->mask          = SET_PROCESS_INFO_BASE_PRIORITY;
                ret = wine_server_call( req );
            }
            SERVER_END_REQ;
        }
        break;

    case ProcessPriorityBoost:
        if (size != sizeof(ULONG)) return STATUS_INVALID_PARAMETER;
        else
        {
            ULONG* disable_boost = info;

            SERVER_START_REQ( set_process_info )
            {
                req->handle        = wine_server_obj_handle( handle );
                req->disable_boost = *disable_boost;
                req->mask          = SET_PROCESS_INFO_DISABLE_BOOST;
                ret = wine_server_call( req );
            }
            SERVER_END_REQ;
        }
        break;

    case ProcessExecuteFlags:
        if ((is_win64 && !is_wow64()) || size != sizeof(ULONG)) return STATUS_INVALID_PARAMETER;
        if (execute_flags & MEM_EXECUTE_OPTION_PERMANENT) return STATUS_ACCESS_DENIED;
        else
        {
            BOOL enable;
            switch (*(ULONG *)info & (MEM_EXECUTE_OPTION_ENABLE|MEM_EXECUTE_OPTION_DISABLE))
            {
            case MEM_EXECUTE_OPTION_ENABLE:
                enable = TRUE;
                break;
            case MEM_EXECUTE_OPTION_DISABLE:
                enable = FALSE;
                break;
            default:
                return STATUS_INVALID_PARAMETER;
            }
            execute_flags = *(ULONG *)info;
            virtual_set_force_exec( enable );
        }
        break;

    case ProcessInstrumentationCallback:
    {
        PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION *instr = info;
        void *callback;

        if (size < sizeof(callback)) return STATUS_INFO_LENGTH_MISMATCH;
        if (size >= sizeof(PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION)) callback = instr->Callback;
        else                                                              callback = *(void **)info;
        ret = STATUS_SUCCESS;
        if (handle != GetCurrentProcess())
        {
            FIXME( "Setting ProcessInstrumentationCallback is not yet supported for other process.\n" );
            break;
        }
        set_process_instrumentation_callback( callback );
        break;
    }

    case ProcessThreadStackAllocation:
    {
        void *addr = NULL;
        SIZE_T reserve;
        PROCESS_STACK_ALLOCATION_INFORMATION *stack = info;
        if (size == sizeof(PROCESS_STACK_ALLOCATION_INFORMATION_EX))
            stack = &((PROCESS_STACK_ALLOCATION_INFORMATION_EX *)info)->AllocInfo;
        else if (size != sizeof(*stack)) return STATUS_INFO_LENGTH_MISMATCH;

        reserve = stack->ReserveSize;
        ret = NtAllocateVirtualMemory( GetCurrentProcess(), &addr, stack->ZeroBits, &reserve,
                                       MEM_RESERVE, PAGE_READWRITE );
        if (!ret)
        {
#ifdef VALGRIND_STACK_REGISTER
            VALGRIND_STACK_REGISTER( addr, (char *)addr + reserve );
#endif
            stack->StackBase = addr;
        }
        break;
    }

    case ProcessManageWritesToExecutableMemory:
    {
#ifdef __aarch64__
        const MANAGE_WRITES_TO_EXECUTABLE_MEMORY *mem = info;

        if (size != sizeof(*mem)) return STATUS_INFO_LENGTH_MISMATCH;
        if (handle != GetCurrentProcess()) return STATUS_NOT_SUPPORTED;
        if (mem->Version != 2) return STATUS_REVISION_MISMATCH;
        if (mem->ThreadAllowWrites) return STATUS_INVALID_PARAMETER;
        virtual_enable_write_exceptions( mem->ProcessEnableWriteExceptions );
        break;
#else
        return STATUS_NOT_SUPPORTED;
#endif
    }

    case ProcessWineMakeProcessSystem:
        if (size != sizeof(HANDLE *)) return STATUS_INFO_LENGTH_MISMATCH;
        SERVER_START_REQ( make_process_system )
        {
            req->handle = wine_server_obj_handle( handle );
            if (!(ret = wine_server_call( req )))
                *(HANDLE *)info = wine_server_ptr_handle( reply->event );
        }
        SERVER_END_REQ;
        return ret;

    case ProcessWineGrantAdminToken:
        SERVER_START_REQ( grant_process_admin_token )
        {
            req->handle = wine_server_obj_handle( handle );
            ret = wine_server_call( req );
        }
        SERVER_END_REQ;
        break;

    case ProcessPowerThrottlingState:
        FIXME( "ProcessPowerThrottlingState - stub\n" );
        return STATUS_SUCCESS;

    default:
        FIXME( "(%p,0x%08x,%p,0x%08x) stub\n", handle, class, info, size );
        ret = STATUS_NOT_IMPLEMENTED;
        break;
    }
    return ret;
}


/**********************************************************************
 *           NtOpenProcess  (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenProcess( HANDLE *handle, ACCESS_MASK access,
                               const OBJECT_ATTRIBUTES *attr, const CLIENT_ID *id )
{
    unsigned int status;

    *handle = 0;

    SERVER_START_REQ( open_process )
    {
        req->pid        = HandleToULong( id->UniqueProcess );
        req->access     = access;
        req->attributes = attr ? attr->Attributes : 0;
        status = wine_server_call( req );
        if (!status) *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return status;
}


/**********************************************************************
 *           NtSuspendProcess  (NTDLL.@)
 */
NTSTATUS WINAPI NtSuspendProcess( HANDLE handle )
{
    unsigned int ret;

    SERVER_START_REQ( suspend_process )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}


/**********************************************************************
 *           NtResumeProcess  (NTDLL.@)
 */
NTSTATUS WINAPI NtResumeProcess( HANDLE handle )
{
    unsigned int ret;

    SERVER_START_REQ( resume_process )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}


/**********************************************************************
 *           NtGetNextProcess  (NTDLL.@)
 */
NTSTATUS WINAPI NtGetNextProcess( HANDLE process, ACCESS_MASK access, ULONG attributes,
                                  ULONG flags, HANDLE *handle )
{
    HANDLE ret_handle = 0;
    unsigned int ret;

    TRACE( "process %p, access %#x, attributes %#x, flags %#x, handle %p.\n",
           process, access, attributes, flags, handle );

    SERVER_START_REQ( get_next_process )
    {
        req->last = wine_server_obj_handle( process );
        req->access = access;
        req->attributes = attributes;
        req->flags = flags;
        if (!(ret = wine_server_call( req ))) ret_handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    *handle = ret_handle;
    return ret;
}


/**********************************************************************
 *           NtDebugActiveProcess  (NTDLL.@)
 */
NTSTATUS WINAPI NtDebugActiveProcess( HANDLE process, HANDLE debug )
{
    unsigned int ret;

    SERVER_START_REQ( debug_process )
    {
        req->handle = wine_server_obj_handle( process );
        req->debug  = wine_server_obj_handle( debug );
        req->attach = 1;
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}


/**********************************************************************
 *           NtRemoveProcessDebug  (NTDLL.@)
 */
NTSTATUS WINAPI NtRemoveProcessDebug( HANDLE process, HANDLE debug )
{
    unsigned int ret;

    SERVER_START_REQ( debug_process )
    {
        req->handle = wine_server_obj_handle( process );
        req->debug  = wine_server_obj_handle( debug );
        req->attach = 0;
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}


/**********************************************************************
 *           NtDebugContinue  (NTDLL.@)
 */
NTSTATUS WINAPI NtDebugContinue( HANDLE handle, CLIENT_ID *client, NTSTATUS status )
{
    unsigned int ret;

    SERVER_START_REQ( continue_debug_event )
    {
        req->debug  = wine_server_obj_handle( handle );
        req->pid    = HandleToULong( client->UniqueProcess );
        req->tid    = HandleToULong( client->UniqueThread );
        req->status = status;
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}
