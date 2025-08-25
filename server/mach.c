/*
 * Server-side debugger support using Mach primitives
 *
 * Copyright (C) 1999, 2006 Alexandre Julliard
 * Copyright (C) 2006 Ken Thomases for CodeWeavers
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
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"

#include "file.h"
#include "process.h"
#include "thread.h"
#include "request.h"

#ifdef USE_MACH

#include <mach/mach.h>
#include <mach/mach_error.h>
#include <mach/thread_act.h>
#include <mach/mach_vm.h>
#include <servers/bootstrap.h>

static mach_port_t server_mach_port;

void sigchld_callback(void)
{
    assert(0);  /* should never be called on MacOS */
}

static void mach_set_error(kern_return_t mach_error)
{
    switch (mach_error)
    {
        case KERN_SUCCESS:              break;
        case KERN_INVALID_ARGUMENT:     set_error(STATUS_INVALID_PARAMETER); break;
        case KERN_NO_SPACE:             set_error(STATUS_NO_MEMORY); break;
        case KERN_PROTECTION_FAILURE:   set_error(STATUS_ACCESS_DENIED); break;
        case KERN_INVALID_ADDRESS:      set_error(STATUS_ACCESS_VIOLATION); break;
        default:                        set_error(STATUS_UNSUCCESSFUL); break;
    }
}

static mach_port_t get_process_port( struct process *process )
{
    return process->trace_data;
}

static int is_rosetta( void )
{
    static int rosetta_status, did_check = 0;
    if (!did_check)
    {
        /* returns 0 for native process or on error, 1 for translated */
        int ret = 0;
        size_t size = sizeof(ret);
        if (sysctlbyname( "sysctl.proc_translated", &ret, &size, NULL, 0 ) == -1)
            rosetta_status = 0;
        else
            rosetta_status = ret;

        did_check = 1;
    }

    return rosetta_status;
}

extern kern_return_t bootstrap_register2( mach_port_t bp, name_t service_name, mach_port_t sp, uint64_t flags );

/* initialize the process control mechanism */
void init_tracing_mechanism(void)
{
    mach_port_t bp;

    if (task_get_bootstrap_port( mach_task_self(), &bp ) != KERN_SUCCESS)
        fatal_error( "Can't find bootstrap port\n" );
    if (mach_port_allocate( mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &server_mach_port ) != KERN_SUCCESS)
        fatal_error( "Can't allocate port\n" );
    if  (mach_port_insert_right( mach_task_self(),
                                 server_mach_port,
                                 server_mach_port,
                                 MACH_MSG_TYPE_MAKE_SEND ) != KERN_SUCCESS)
            fatal_error( "Error inserting rights\n" );
    if (bootstrap_register2( bp, server_dir, server_mach_port, 0 ) != KERN_SUCCESS)
        fatal_error( "Can't check in server_mach_port\n" );
    mach_port_deallocate( mach_task_self(), bp );
}

/* initialize the per-process tracing mechanism */
void init_process_tracing( struct process *process )
{
    int pid, ret;
    struct
    {
        mach_msg_header_t           header;
        mach_msg_body_t             body;
        mach_msg_port_descriptor_t  task_port;
        mach_msg_trailer_t          trailer; /* only present on receive */
    } msg;

    for (;;)
    {
        ret = mach_msg( &msg.header, MACH_RCV_MSG|MACH_RCV_TIMEOUT, 0, sizeof(msg),
                        server_mach_port, 0, 0 );
        if (ret)
        {
            if (ret != MACH_RCV_TIMED_OUT && debug_level)
                fprintf( stderr, "warning: mach port receive failed with %x\n", ret );
            return;
        }

        /* if anything in the message is invalid, ignore it */
        if (msg.header.msgh_size != offsetof(typeof(msg), trailer)) continue;
        if (msg.body.msgh_descriptor_count != 1) continue;
        if (msg.task_port.type != MACH_MSG_PORT_DESCRIPTOR) continue;
        if (msg.task_port.disposition != MACH_MSG_TYPE_PORT_SEND) continue;
        if (msg.task_port.name == MACH_PORT_NULL) continue;
        if (msg.task_port.name == MACH_PORT_DEAD) continue;

        if (!pid_for_task( msg.task_port.name, &pid ))
        {
            struct thread *thread = get_thread_from_pid( pid );

            if (thread && !thread->process->trace_data)
                thread->process->trace_data = msg.task_port.name;
            else
                mach_port_deallocate( mach_task_self(), msg.task_port.name );
        }
    }
    /* On Mach thread priorities depend on having the process port available, so
     * reapply all thread priorities here after process tracing is initialized */
    set_process_base_priority( process, process->base_priority );
}

/* terminate the per-process tracing mechanism */
void finish_process_tracing( struct process *process )
{
    if (process->trace_data)
    {
        mach_port_deallocate( mach_task_self(), process->trace_data );
        process->trace_data = 0;
    }
}

/* initialize registers in new thread if necessary */
void init_thread_context( struct thread *thread )
{
}

/* retrieve the thread x86 registers */
void get_thread_context( struct thread *thread, struct context_data *context, unsigned int flags )
{
#if defined(__i386__) || defined(__x86_64__)
    x86_debug_state_t state;
    mach_msg_type_number_t count = sizeof(state) / sizeof(int);
    mach_msg_type_name_t type;
    mach_port_t port, process_port = get_process_port( thread->process );
    kern_return_t ret;
    unsigned long dr[8];

    /* all other regs are handled on the client side */
    assert( flags == SERVER_CTX_DEBUG_REGISTERS );

    if (is_rosetta())
    {
        /* getting debug registers of a translated process is not supported cross-process, return all zeroes */
        memset( &context->debug, 0, sizeof(context->debug) );
        context->flags |= SERVER_CTX_DEBUG_REGISTERS;
        return;
    }

    if (thread->unix_pid == -1 || !process_port ||
        mach_port_extract_right( process_port, thread->unix_tid,
                                 MACH_MSG_TYPE_COPY_SEND, &port, &type ))
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }

    ret = thread_get_state( port, x86_DEBUG_STATE, (thread_state_t)&state, &count );
    if (!ret)
    {
        assert( state.dsh.flavor == x86_DEBUG_STATE32 ||
                state.dsh.flavor == x86_DEBUG_STATE64 );

        if (state.dsh.flavor == x86_DEBUG_STATE64)
        {
            dr[0] = state.uds.ds64.__dr0;
            dr[1] = state.uds.ds64.__dr1;
            dr[2] = state.uds.ds64.__dr2;
            dr[3] = state.uds.ds64.__dr3;
            dr[6] = state.uds.ds64.__dr6;
            dr[7] = state.uds.ds64.__dr7;
        }
        else
        {
            dr[0] = state.uds.ds32.__dr0;
            dr[1] = state.uds.ds32.__dr1;
            dr[2] = state.uds.ds32.__dr2;
            dr[3] = state.uds.ds32.__dr3;
            dr[6] = state.uds.ds32.__dr6;
            dr[7] = state.uds.ds32.__dr7;
        }

        switch (context->machine)
        {
        case IMAGE_FILE_MACHINE_I386:
            context->debug.i386_regs.dr0 = dr[0];
            context->debug.i386_regs.dr1 = dr[1];
            context->debug.i386_regs.dr2 = dr[2];
            context->debug.i386_regs.dr3 = dr[3];
            context->debug.i386_regs.dr6 = dr[6];
            context->debug.i386_regs.dr7 = dr[7];
            break;
        case IMAGE_FILE_MACHINE_AMD64:
            context->debug.x86_64_regs.dr0 = dr[0];
            context->debug.x86_64_regs.dr1 = dr[1];
            context->debug.x86_64_regs.dr2 = dr[2];
            context->debug.x86_64_regs.dr3 = dr[3];
            context->debug.x86_64_regs.dr6 = dr[6];
            context->debug.x86_64_regs.dr7 = dr[7];
            break;
        default:
            set_error( STATUS_INVALID_PARAMETER );
            goto done;
        }
        context->flags |= SERVER_CTX_DEBUG_REGISTERS;
    }
    else
        mach_set_error( ret );
done:
    mach_port_deallocate( mach_task_self(), port );
#endif
}

/* set the thread x86 registers */
void set_thread_context( struct thread *thread, const struct context_data *context, unsigned int flags )
{
#if defined(__i386__) || defined(__x86_64__)
    x86_debug_state_t state;
    mach_msg_type_number_t count = sizeof(state) / sizeof(int);
    mach_msg_type_name_t type;
    mach_port_t port, process_port = get_process_port( thread->process );
    unsigned long dr[8];
    kern_return_t ret;

    /* all other regs are handled on the client side */
    assert( flags == SERVER_CTX_DEBUG_REGISTERS );

    if (is_rosetta())
    {
        /* Setting debug registers of a translated process is not supported cross-process
         * (and even in-process, setting debug registers never has the desired effect).
         */
        set_error( STATUS_UNSUCCESSFUL );
        return;
    }

    if (thread->unix_pid == -1 || !process_port ||
        mach_port_extract_right( process_port, thread->unix_tid,
                                 MACH_MSG_TYPE_COPY_SEND, &port, &type ))
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }

    /* get the debug state to determine which flavor to use */
    ret = thread_get_state(port, x86_DEBUG_STATE, (thread_state_t)&state, &count);
    if (ret)
    {
        mach_set_error( ret );
        goto done;
    }
    assert( state.dsh.flavor == x86_DEBUG_STATE32 ||
            state.dsh.flavor == x86_DEBUG_STATE64 );

    switch (context->machine)
    {
        case IMAGE_FILE_MACHINE_I386:
            dr[0] = context->debug.i386_regs.dr0;
            dr[1] = context->debug.i386_regs.dr1;
            dr[2] = context->debug.i386_regs.dr2;
            dr[3] = context->debug.i386_regs.dr3;
            dr[6] = context->debug.i386_regs.dr6;
            dr[7] = context->debug.i386_regs.dr7;
            break;
        case IMAGE_FILE_MACHINE_AMD64:
            dr[0] = context->debug.x86_64_regs.dr0;
            dr[1] = context->debug.x86_64_regs.dr1;
            dr[2] = context->debug.x86_64_regs.dr2;
            dr[3] = context->debug.x86_64_regs.dr3;
            dr[6] = context->debug.x86_64_regs.dr6;
            dr[7] = context->debug.x86_64_regs.dr7;
            break;
        default:
            set_error( STATUS_INVALID_PARAMETER );
            goto done;
    }

    /* Mac OS doesn't allow setting the global breakpoint flags */
    dr[7] = (dr[7] & ~0xaa) | ((dr[7] & 0xaa) >> 1);

    if (state.dsh.flavor == x86_DEBUG_STATE64)
    {
        state.dsh.count = sizeof(state.uds.ds64) / sizeof(int);
        state.uds.ds64.__dr0 = dr[0];
        state.uds.ds64.__dr1 = dr[1];
        state.uds.ds64.__dr2 = dr[2];
        state.uds.ds64.__dr3 = dr[3];
        state.uds.ds64.__dr4 = 0;
        state.uds.ds64.__dr5 = 0;
        state.uds.ds64.__dr6 = dr[6];
        state.uds.ds64.__dr7 = dr[7];
    }
    else
    {
        state.dsh.count = sizeof(state.uds.ds32) / sizeof(int);
        state.uds.ds32.__dr0 = dr[0];
        state.uds.ds32.__dr1 = dr[1];
        state.uds.ds32.__dr2 = dr[2];
        state.uds.ds32.__dr3 = dr[3];
        state.uds.ds32.__dr4 = 0;
        state.uds.ds32.__dr5 = 0;
        state.uds.ds32.__dr6 = dr[6];
        state.uds.ds32.__dr7 = dr[7];
    }
    ret = thread_set_state( port, x86_DEBUG_STATE, (thread_state_t)&state, count );
    if (ret)
        mach_set_error( ret );
done:
    mach_port_deallocate( mach_task_self(), port );
#endif
}

extern int __pthread_kill( mach_port_t, int );

int send_thread_signal( struct thread *thread, int sig )
{
    int ret = -1;
    mach_port_t process_port = get_process_port( thread->process );

    if (thread->unix_pid != -1 && process_port)
    {
        mach_msg_type_name_t type;
        mach_port_t port;

        if (!mach_port_extract_right( process_port, thread->unix_tid,
                                      MACH_MSG_TYPE_COPY_SEND, &port, &type ))
        {
            ret = __pthread_kill( port, sig );
            mach_port_deallocate( mach_task_self(), port );
        }
        else errno = ESRCH;

        if (ret == -1 && errno == ESRCH) /* thread got killed */
        {
            thread->unix_pid = -1;
            thread->unix_tid = -1;
        }
    }
    if (debug_level && ret != -1)
        fprintf( stderr, "%04x: *sent signal* signal=%d\n", thread->id, sig );
    return (ret != -1);
}

/* read data from a process memory space */
int read_process_memory( struct process *process, client_ptr_t ptr, data_size_t size, char *dest )
{
    kern_return_t ret;
    mach_vm_size_t bytes_read;
    mach_port_t process_port = get_process_port( process );

    if (!process_port)
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }
    if ((mach_vm_address_t)ptr != ptr)
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }

    ret = mach_vm_read_overwrite( process_port, (mach_vm_address_t)ptr, (mach_vm_size_t)size, (mach_vm_address_t)dest, &bytes_read );
    mach_set_error( ret );
    return (ret == KERN_SUCCESS);
}

/* write data to a process memory space */
int write_process_memory( struct process *process, client_ptr_t ptr, data_size_t size, const char *src,
                          data_size_t *written )
{
    kern_return_t ret;
    mach_port_t process_port = get_process_port( process );
    mach_vm_offset_t data;

    if (!process_port)
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }
    if ((mach_vm_address_t)ptr != ptr)
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }
    if (posix_memalign( (void **)&data, get_page_size(), size ))
    {
        set_error( STATUS_NO_MEMORY );
        return 0;
    }

    memcpy( (void *)data, src, size );

    ret = mach_vm_write( process_port, (mach_vm_address_t)ptr, data, (mach_msg_type_number_t)size );

    /*
     * On arm64 macOS, enabling execute permission for a memory region automatically disables write
     * permission for that region. This can also happen under Rosetta sometimes.
     * In that case mach_vm_write returns KERN_INVALID_ADDRESS.
     */

    if (ret == KERN_INVALID_ADDRESS)
    {
        mach_vm_address_t current_address = (mach_vm_address_t)ptr;
        mach_vm_address_t region_address = current_address;
        mach_vm_size_t region_size, write_size;
        vm_region_basic_info_data_t info;
        mach_msg_type_number_t info_count = VM_REGION_BASIC_INFO_COUNT_64;
        mach_port_t object_name;
        data_size_t remaining_size = size;

        ret = mach_vm_region( process_port, &region_address, &region_size, VM_REGION_BASIC_INFO_64,
                     (vm_region_info_t)&info, &info_count, &object_name );
        if (ret != KERN_SUCCESS)
            goto out;

        /*
         * Actually check that everything is sane before suspending.
         * KERN_INVALID_ADDRESS can also be returned when address is illegal or
         * specifies a non-allocated region.
         */
        if (region_address > current_address ||
            region_address + region_size <= current_address)
        {
            ret = KERN_INVALID_ADDRESS;
            goto out;
        }

        /*
         * FIXME: Rosetta can turn RWX pages into R-X pages during execution.
         * For now we will just have to ignore failures due to the wrong
         * protection here.
         */
        if (!is_rosetta() && !(info.protection & VM_PROT_WRITE))
        {
            ret = KERN_PROTECTION_FAILURE;
            goto out;
        }

        /* The following operations should seem atomic from the perspective of the
         * target process. */
        if ((ret = task_suspend( process_port )) != KERN_SUCCESS)
            goto out;

        /* Iterate over all applicable memory regions until the write is completed. */
        while (remaining_size)
        {
            region_address = current_address;
            info_count = VM_REGION_BASIC_INFO_COUNT_64;
            ret = mach_vm_region( process_port, &region_address, &region_size, VM_REGION_BASIC_INFO_64,
                     (vm_region_info_t)&info, &info_count, &object_name );
            if (ret != KERN_SUCCESS) break;

            if (region_address > current_address ||
                region_address + region_size <= current_address)
            {
                ret = KERN_INVALID_ADDRESS;
                break;
            }

            /* FIXME: See the above Rosetta remark. */
            if (!is_rosetta() && !(info.protection & VM_PROT_WRITE))
            {
                ret = KERN_PROTECTION_FAILURE;
                break;
            }

            write_size = region_size - (current_address - region_address);
            if (write_size > remaining_size) write_size = remaining_size;

            ret = mach_vm_protect( process_port, current_address, write_size, 0,
                    VM_PROT_READ | VM_PROT_WRITE );
            if (ret != KERN_SUCCESS) break;

            ret = mach_vm_write( process_port, current_address,
                    data + (current_address - (mach_vm_address_t)ptr), write_size );
            if (ret != KERN_SUCCESS) break;

            ret = mach_vm_protect( process_port, current_address, write_size, 0,
                    info.protection );
            if (ret != KERN_SUCCESS) break;

            current_address += write_size;
            remaining_size  -= write_size;
        }

        task_resume( process_port );
    }

out:
    free( (void *)data );
    mach_set_error( ret );
    if (ret == KERN_SUCCESS && written) *written = size;
    return (ret == KERN_SUCCESS);
}

#endif  /* USE_MACH */
