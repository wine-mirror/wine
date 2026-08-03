/*
 * Server-side /proc support for Solaris
 *
 * Copyright (C) 2007 Alexandre Julliard
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
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"

#include "file.h"
#include "process.h"
#include "thread.h"

#ifdef USE_PROCFS

/* procfs doesn't support large files */
# undef _FILE_OFFSET_BITS
# define _FILE_OFFSET_BITS 32
#include <procfs.h>

static int open_proc_as( struct process *process, int flags )
{
    char buffer[32];
    int fd;

    if (process->unix_pid == -1)
    {
        set_error( STATUS_ACCESS_DENIED );
        return -1;
    }

    snprintf( buffer, sizeof(buffer), "/proc/%u/as", process->unix_pid );
    if ((fd = open( buffer, flags )) == -1)
    {
        if (errno == ENOENT)  /* probably got killed */
        {
            process->unix_pid = -1;
            set_error( STATUS_ACCESS_DENIED );
        }
        else file_set_error();
    }
    return fd;
}

static int open_proc_lwpctl( struct thread *thread )
{
    char buffer[48];
    int fd;

    if (thread->unix_pid == -1) return -1;

    snprintf( buffer, sizeof(buffer), "/proc/%u/lwp/%u/lwpctl", thread->unix_pid, thread->unix_tid );
    if ((fd = open( buffer, O_WRONLY )) == -1)
    {
        if (errno == ENOENT)  /* probably got killed */
            thread->unix_pid = thread->unix_tid = -1;
        else
            file_set_error();
    }
    return fd;
}


/* handle a SIGCHLD signal */
void sigchld_callback(void)
{
    assert( 0 );  /* should only be called when using ptrace */
}

/* initialize the process tracing mechanism */
void init_tracing_mechanism(void)
{
    /* no initialization needed */
}

/* initialize the per-process tracing mechanism */
void init_process_tracing( struct process *process )
{
    /* setup is done on-demand */
}

/* terminate the per-process tracing mechanism */
void finish_process_tracing( struct process *process )
{
}

/* send a Unix signal to a specific thread */
int send_thread_signal( struct thread *thread, int sig )
{
    int fd = open_proc_lwpctl( thread );
    long kill[2];
    ssize_t ret;

    if (fd == -1) return 0;

    kill[0] = PCKILL;
    kill[1] = sig;
    ret = write( fd, kill, sizeof(kill) );
    close( fd );
    return (ret == sizeof(kill));
}

/* read data from a process memory space */
int read_process_memory( struct process *process, client_ptr_t ptr, size_t size, char *dest )
{
    ssize_t ret;
    int fd;

    if ((off_t)ptr != ptr)
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }

    if ((fd = open_proc_as( process, O_RDONLY )) == -1) return 0;

    ret = pread( fd, dest, size, (off_t)ptr );
    close( fd );
    if (ret == size) return 1;

    if (ret == -1) file_set_error();
    else set_error( STATUS_ACCESS_VIOLATION );
    return 0;
}

/* write data to a process memory space */
int write_process_memory( struct process *process, client_ptr_t ptr, size_t size, const char *src )
{
    ssize_t ret;
    int fd;

    if ((off_t)ptr != ptr)
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }

    if ((fd = open_proc_as( process, O_WRONLY )) == -1) return 0;

    ret = pwrite( fd, src, size, (off_t)ptr );
    close( fd );
    if (ret == size) return 1;

    if (ret == -1) file_set_error();
    else set_error( STATUS_ACCESS_VIOLATION );
    return 0;
}

/* retrieve an LDT selector entry */
void get_selector_entry( struct thread *thread, int entry, unsigned int *base,
                         unsigned int *limit, unsigned char *flags )
{
    ssize_t ret;
    off_t pos = thread->process->ldt_copy;
    int fd;

    if (!pos)
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }
    if ((fd = open_proc_as( thread->process, O_RDONLY )) == -1) return;

    ret = pread( fd, base, sizeof(*base), pos + entry*sizeof(int) );
    if (ret != sizeof(*base)) goto error;
    ret = pread( fd, limit, sizeof(*limit), pos + (8192 + entry)*sizeof(int) );
    if (ret != sizeof(*limit)) goto error;
    ret = pread( fd, flags, sizeof(*flags), pos + 2*8192*sizeof(int) + entry );
    if (ret != sizeof(*flags)) goto error;
    close( fd );
    return;

error:
    if (ret == -1) file_set_error();
    else set_error( STATUS_ACCESS_VIOLATION );
    close( fd );
}

/* initialize registers in new thread if necessary */
void init_thread_context( struct thread *thread )
{
}

/* retrieve the thread registers */
void get_thread_context( struct thread *thread, struct context_data *context, unsigned int flags )
{
    /* FIXME: get debug registers */
}

/* set the thread registers */
void set_thread_context( struct thread *thread, const struct context_data *context, unsigned int flags )
{
    /* FIXME: set debug registers */
}

#endif /* USE_PROCFS */
