/*
 * Server request tracing
 *
 * Copyright (C) 1999 Alexandre Julliard
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

#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif


#include "windef.h"
#include "winbase.h"
#include "wincon.h"
#include "request.h"
#include "unicode.h"

static int cur_pos;
static const void *cur_data;
static int cur_size;

/* utility functions */

inline static void remove_data( size_t size )
{
    cur_data = (const char *)cur_data + size;
    cur_size -= size;
}

static void dump_uints( const int *ptr, int len )
{
    fputc( '{', stderr );
    while (len > 0)
    {
        fprintf( stderr, "%08x", *ptr++ );
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
}

static void dump_abs_time( const abs_time_t *time )
{
    struct timeval tv;
    int secs, usecs;

    if (!time->sec && !time->usec)
    {
        fprintf( stderr, "0" );
        return;
    }
    gettimeofday( &tv, NULL );
    secs = time->sec - tv.tv_sec;
    if ((usecs = time->usec - tv.tv_usec) < 0)
    {
        usecs += 1000000;
        secs--;
    }
    if (secs > 0 || (secs == 0 && usecs >= 0))
        fprintf( stderr, "%d.%06d (+%d.%06d)", time->sec, time->usec, secs, usecs );
    else
        fprintf( stderr, "%d.%06d (-%d.%06d)", time->sec, time->usec, abs(secs+1), 1000000-usecs );
}

static void dump_rectangle( const rectangle_t *rect )
{
    fprintf( stderr, "{%d,%d;%d,%d}",
             rect->left, rect->top, rect->right, rect->bottom );
}

static void dump_char_info( const char_info_t *info )
{
    fprintf( stderr, "{'" );
    dump_strW( &info->ch, 1, stderr, "\'\'" );
    fprintf( stderr, "',%04x}", info->attr );
}

static void dump_context( const CONTEXT *context )
{
#ifdef __i386__
    fprintf( stderr, "{flags=%08lx,eax=%08lx,ebx=%08lx,ecx=%08lx,edx=%08lx,esi=%08lx,edi=%08lx,"
             "ebp=%08lx,eip=%08lx,esp=%08lx,eflags=%08lx,cs=%04lx,ds=%04lx,es=%04lx,"
             "fs=%04lx,gs=%04lx,dr0=%08lx,dr1=%08lx,dr2=%08lx,dr3=%08lx,dr6=%08lx,dr7=%08lx,",
             context->ContextFlags, context->Eax, context->Ebx, context->Ecx, context->Edx,
             context->Esi, context->Edi, context->Ebp, context->Eip, context->Esp, context->EFlags,
             context->SegCs, context->SegDs, context->SegEs, context->SegFs, context->SegGs,
             context->Dr0, context->Dr1, context->Dr2, context->Dr3, context->Dr6, context->Dr7 );
    fprintf( stderr, "float=" );
    dump_uints( (int *)&context->FloatSave, sizeof(context->FloatSave) / sizeof(int) );
    fprintf( stderr, "}" );
#else
    dump_uints( (int *)context, sizeof(*context) / sizeof(int) );
#endif
}

static void dump_exc_record( const EXCEPTION_RECORD *rec )
{
    int i;
    fprintf( stderr, "{code=%lx,flags=%lx,rec=%p,addr=%p,params={",
             rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionRecord,
             rec->ExceptionAddress );
    for (i = 0; i < min(rec->NumberParameters,EXCEPTION_MAXIMUM_PARAMETERS); i++)
    {
        if (i) fputc( ',', stderr );
        fprintf( stderr, "%lx", rec->ExceptionInformation[i] );
    }
    fputc( '}', stderr );
}

static void dump_varargs_ints( size_t size )
{
    const int *data = cur_data;
    size_t len = size / sizeof(*data);

    fputc( '{', stderr );
    while (len > 0)
    {
        fprintf( stderr, "%d", *data++ );
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
    remove_data( size );
}

static void dump_varargs_handles( size_t size )
{
    const obj_handle_t *data = cur_data;
    size_t len = size / sizeof(*data);

    fputc( '{', stderr );
    while (len > 0)
    {
        fprintf( stderr, "%p", *data++ );
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
    remove_data( size );
}

static void dump_varargs_user_handles( size_t size )
{
    const user_handle_t *data = cur_data;
    size_t len = size / sizeof(*data);

    fputc( '{', stderr );
    while (len > 0)
    {
        fprintf( stderr, "%p", *data++ );
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
    remove_data( size );
}

static void dump_varargs_bytes( size_t size )
{
    const unsigned char *data = cur_data;
    size_t len = size;

    fputc( '{', stderr );
    while (len > 0)
    {
        fprintf( stderr, "%02x", *data++ );
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
    remove_data( size );
}

static void dump_varargs_string( size_t size )
{
    fprintf( stderr, "\"%.*s\"", (int)size, (char *)cur_data );
    remove_data( size );
}

static void dump_varargs_unicode_str( size_t size )
{
    fprintf( stderr, "L\"" );
    dump_strW( cur_data, size / sizeof(WCHAR), stderr, "\"\"" );
    fputc( '\"', stderr );
    remove_data( size );
}

static void dump_varargs_context( size_t size )
{
    if (!size)
    {
        fprintf( stderr, "{}" );
        return;
    }
    dump_context( cur_data );
    remove_data( size );
}

static void dump_varargs_exc_event( size_t size )
{
    const CONTEXT *ptr = cur_data;

    if (!size)
    {
        fprintf( stderr, "{}" );
        return;
    }
    fprintf( stderr, "{context=" );
    dump_context( ptr );
    fprintf( stderr, ",rec=" );
    dump_exc_record( (EXCEPTION_RECORD *)(ptr + 1) );
    fputc( '}', stderr );
    remove_data( size );
}

static void dump_varargs_debug_event( size_t size )
{
    const debug_event_t *event = cur_data;

    if (!size)
    {
        fprintf( stderr, "{}" );
        return;
    }
    switch(event->code)
    {
    case EXCEPTION_DEBUG_EVENT:
        fprintf( stderr, "{exception," );
        dump_exc_record( &event->info.exception.record );
        fprintf( stderr, ",first=%d}", event->info.exception.first );
        break;
    case CREATE_THREAD_DEBUG_EVENT:
        fprintf( stderr, "{create_thread,thread=%p,teb=%p,start=%p}",
                 event->info.create_thread.handle, event->info.create_thread.teb,
                 event->info.create_thread.start );
        break;
    case CREATE_PROCESS_DEBUG_EVENT:
        fprintf( stderr, "{create_process,file=%p,process=%p,thread=%p,base=%p,offset=%d,"
                         "size=%d,teb=%p,start=%p,name=%p,unicode=%d}",
                 event->info.create_process.file, event->info.create_process.process,
                 event->info.create_process.thread, event->info.create_process.base,
                 event->info.create_process.dbg_offset, event->info.create_process.dbg_size,
                 event->info.create_process.teb, event->info.create_process.start,
                 event->info.create_process.name, event->info.create_process.unicode );
        break;
    case EXIT_THREAD_DEBUG_EVENT:
        fprintf( stderr, "{exit_thread,code=%d}", event->info.exit.exit_code );
        break;
    case EXIT_PROCESS_DEBUG_EVENT:
        fprintf( stderr, "{exit_process,code=%d}", event->info.exit.exit_code );
        break;
    case LOAD_DLL_DEBUG_EVENT:
        fprintf( stderr, "{load_dll,file=%p,base=%p,offset=%d,size=%d,name=%p,unicode=%d}",
                 event->info.load_dll.handle, event->info.load_dll.base,
                 event->info.load_dll.dbg_offset, event->info.load_dll.dbg_size,
                 event->info.load_dll.name, event->info.load_dll.unicode );
        break;
    case UNLOAD_DLL_DEBUG_EVENT:
        fprintf( stderr, "{unload_dll,base=%p}", event->info.unload_dll.base );
        break;
    case OUTPUT_DEBUG_STRING_EVENT:
        fprintf( stderr, "{output_string,data=%p,unicode=%d,len=%d}",
                 event->info.output_string.string, event->info.output_string.unicode,
                 event->info.output_string.length );
        break;
    case RIP_EVENT:
        fprintf( stderr, "{rip,err=%d,type=%d}",
                 event->info.rip_info.error, event->info.rip_info.type );
        break;
    case 0:  /* zero is the code returned on timeouts */
        fprintf( stderr, "{}" );
        break;
    default:
        fprintf( stderr, "{code=??? (%d)}", event->code );
        break;
    }
    remove_data( size );
}

static void dump_varargs_startup_info( size_t size )
{
    const startup_info_t *ptr = cur_data;
    startup_info_t info;

    if (size < sizeof(info.size))
    {
        fprintf( stderr, "{}" );
        return;
    }
    if (size > ptr->size) size = ptr->size;
    memset( &info, 0, sizeof(info) );
    memcpy( &info, ptr, min( size, sizeof(info) ));

    fprintf( stderr, "{size=%d", info.size );
    fprintf( stderr, ",x=%d", info.x );
    fprintf( stderr, ",y=%d", info.y );
    fprintf( stderr, ",cx=%d", info.cx );
    fprintf( stderr, ",cy=%d", info.cy );
    fprintf( stderr, ",x_chars=%d", info.x_chars );
    fprintf( stderr, ",y_chars=%d", info.y_chars );
    fprintf( stderr, ",attr=%d", info.attribute );
    fprintf( stderr, ",cmd_show=%d", info.cmd_show );
    fprintf( stderr, ",flags=%x", info.flags );
    remove_data( size );
    fprintf( stderr, ",filename=" );
    /* FIXME: these should be unicode */
    dump_varargs_string( min(cur_size,info.filename_len) );
    fprintf( stderr, ",cmdline=" );
    dump_varargs_string( min(cur_size,info.cmdline_len) );
    fprintf( stderr, ",desktop=" );
    dump_varargs_string( min(cur_size,info.desktop_len) );
    fprintf( stderr, ",title=" );
    dump_varargs_string( min(cur_size,info.title_len) );
    fputc( '}', stderr );
}

static void dump_varargs_input_records( size_t size )
{
    const INPUT_RECORD *rec = cur_data;
    size_t len = size / sizeof(*rec);

    fputc( '{', stderr );
    while (len > 0)
    {
        fprintf( stderr, "{%04x,...}", rec->EventType );
        rec++;
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
    remove_data( size );
}

static void dump_varargs_properties( size_t size )
{
    const property_data_t *prop = cur_data;
    size_t len = size / sizeof(*prop);

    fputc( '{', stderr );
    while (len > 0)
    {
        fprintf( stderr, "{atom=%04x,str=%d,handle=%p}",
                 prop->atom, prop->string, prop->handle );
        prop++;
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
    remove_data( size );
}

typedef void (*dump_func)( const void *req );

/* Everything below this line is generated automatically by tools/make_requests */
/* ### make_requests begin ### */

static void dump_new_process_request( const struct new_process_request *req )
{
    fprintf( stderr, " inherit_all=%d,", req->inherit_all );
    fprintf( stderr, " use_handles=%d,", req->use_handles );
    fprintf( stderr, " create_flags=%d,", req->create_flags );
    fprintf( stderr, " unix_pid=%d,", req->unix_pid );
    fprintf( stderr, " exe_file=%p,", req->exe_file );
    fprintf( stderr, " hstdin=%p,", req->hstdin );
    fprintf( stderr, " hstdout=%p,", req->hstdout );
    fprintf( stderr, " hstderr=%p,", req->hstderr );
    fprintf( stderr, " info=" );
    dump_varargs_startup_info( cur_size );
}

static void dump_new_process_reply( const struct new_process_reply *req )
{
    fprintf( stderr, " info=%p", req->info );
}

static void dump_get_new_process_info_request( const struct get_new_process_info_request *req )
{
    fprintf( stderr, " info=%p,", req->info );
    fprintf( stderr, " pinherit=%d,", req->pinherit );
    fprintf( stderr, " tinherit=%d", req->tinherit );
}

static void dump_get_new_process_info_reply( const struct get_new_process_info_reply *req )
{
    fprintf( stderr, " pid=%04x,", req->pid );
    fprintf( stderr, " phandle=%p,", req->phandle );
    fprintf( stderr, " tid=%04x,", req->tid );
    fprintf( stderr, " thandle=%p,", req->thandle );
    fprintf( stderr, " success=%d", req->success );
}

static void dump_new_thread_request( const struct new_thread_request *req )
{
    fprintf( stderr, " suspend=%d,", req->suspend );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " request_fd=%d", req->request_fd );
}

static void dump_new_thread_reply( const struct new_thread_reply *req )
{
    fprintf( stderr, " tid=%04x,", req->tid );
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_boot_done_request( const struct boot_done_request *req )
{
    fprintf( stderr, " debug_level=%d", req->debug_level );
}

static void dump_init_process_request( const struct init_process_request *req )
{
    fprintf( stderr, " ldt_copy=%p", req->ldt_copy );
}

static void dump_init_process_reply( const struct init_process_reply *req )
{
    fprintf( stderr, " create_flags=%d,", req->create_flags );
    fprintf( stderr, " server_start=%08x,", req->server_start );
    fprintf( stderr, " info_size=%d,", req->info_size );
    fprintf( stderr, " exe_file=%p,", req->exe_file );
    fprintf( stderr, " hstdin=%p,", req->hstdin );
    fprintf( stderr, " hstdout=%p,", req->hstdout );
    fprintf( stderr, " hstderr=%p", req->hstderr );
}

static void dump_get_startup_info_request( const struct get_startup_info_request *req )
{
}

static void dump_get_startup_info_reply( const struct get_startup_info_reply *req )
{
    fprintf( stderr, " info=" );
    dump_varargs_startup_info( cur_size );
}

static void dump_init_process_done_request( const struct init_process_done_request *req )
{
    fprintf( stderr, " module=%p,", req->module );
    fprintf( stderr, " module_size=%d,", req->module_size );
    fprintf( stderr, " entry=%p,", req->entry );
    fprintf( stderr, " name=%p,", req->name );
    fprintf( stderr, " exe_file=%p,", req->exe_file );
    fprintf( stderr, " gui=%d,", req->gui );
    fprintf( stderr, " filename=" );
    dump_varargs_string( cur_size );
}

static void dump_init_process_done_reply( const struct init_process_done_reply *req )
{
    fprintf( stderr, " debugged=%d", req->debugged );
}

static void dump_init_thread_request( const struct init_thread_request *req )
{
    fprintf( stderr, " unix_pid=%d,", req->unix_pid );
    fprintf( stderr, " unix_tid=%d,", req->unix_tid );
    fprintf( stderr, " teb=%p,", req->teb );
    fprintf( stderr, " entry=%p,", req->entry );
    fprintf( stderr, " reply_fd=%d,", req->reply_fd );
    fprintf( stderr, " wait_fd=%d", req->wait_fd );
}

static void dump_init_thread_reply( const struct init_thread_reply *req )
{
    fprintf( stderr, " pid=%04x,", req->pid );
    fprintf( stderr, " tid=%04x,", req->tid );
    fprintf( stderr, " boot=%d,", req->boot );
    fprintf( stderr, " version=%d", req->version );
}

static void dump_terminate_process_request( const struct terminate_process_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " exit_code=%d", req->exit_code );
}

static void dump_terminate_process_reply( const struct terminate_process_reply *req )
{
    fprintf( stderr, " self=%d", req->self );
}

static void dump_terminate_thread_request( const struct terminate_thread_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " exit_code=%d", req->exit_code );
}

static void dump_terminate_thread_reply( const struct terminate_thread_reply *req )
{
    fprintf( stderr, " self=%d,", req->self );
    fprintf( stderr, " last=%d", req->last );
}

static void dump_get_process_info_request( const struct get_process_info_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_process_info_reply( const struct get_process_info_reply *req )
{
    fprintf( stderr, " pid=%04x,", req->pid );
    fprintf( stderr, " debugged=%d,", req->debugged );
    fprintf( stderr, " exit_code=%d,", req->exit_code );
    fprintf( stderr, " priority=%d,", req->priority );
    fprintf( stderr, " process_affinity=%d,", req->process_affinity );
    fprintf( stderr, " system_affinity=%d", req->system_affinity );
}

static void dump_set_process_info_request( const struct set_process_info_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " mask=%d,", req->mask );
    fprintf( stderr, " priority=%d,", req->priority );
    fprintf( stderr, " affinity=%d", req->affinity );
}

static void dump_get_thread_info_request( const struct get_thread_info_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " tid_in=%04x", req->tid_in );
}

static void dump_get_thread_info_reply( const struct get_thread_info_reply *req )
{
    fprintf( stderr, " tid=%04x,", req->tid );
    fprintf( stderr, " teb=%p,", req->teb );
    fprintf( stderr, " exit_code=%d,", req->exit_code );
    fprintf( stderr, " priority=%d,", req->priority );
    fprintf( stderr, " creation_time=%ld,", req->creation_time );
    fprintf( stderr, " exit_time=%ld", req->exit_time );
}

static void dump_set_thread_info_request( const struct set_thread_info_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " mask=%d,", req->mask );
    fprintf( stderr, " priority=%d,", req->priority );
    fprintf( stderr, " affinity=%d", req->affinity );
}

static void dump_get_dll_info_request( const struct get_dll_info_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " base_address=%p", req->base_address );
}

static void dump_get_dll_info_reply( const struct get_dll_info_reply *req )
{
    fprintf( stderr, " size=%d,", req->size );
    fprintf( stderr, " entry_point=%p,", req->entry_point );
    fprintf( stderr, " filename=" );
    dump_varargs_string( cur_size );
}

static void dump_suspend_thread_request( const struct suspend_thread_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_suspend_thread_reply( const struct suspend_thread_reply *req )
{
    fprintf( stderr, " count=%d", req->count );
}

static void dump_resume_thread_request( const struct resume_thread_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_resume_thread_reply( const struct resume_thread_reply *req )
{
    fprintf( stderr, " count=%d", req->count );
}

static void dump_load_dll_request( const struct load_dll_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " base=%p,", req->base );
    fprintf( stderr, " size=%d,", req->size );
    fprintf( stderr, " dbg_offset=%d,", req->dbg_offset );
    fprintf( stderr, " dbg_size=%d,", req->dbg_size );
    fprintf( stderr, " name=%p,", req->name );
    fprintf( stderr, " filename=" );
    dump_varargs_string( cur_size );
}

static void dump_unload_dll_request( const struct unload_dll_request *req )
{
    fprintf( stderr, " base=%p", req->base );
}

static void dump_queue_apc_request( const struct queue_apc_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " user=%d,", req->user );
    fprintf( stderr, " func=%p,", req->func );
    fprintf( stderr, " arg1=%p,", req->arg1 );
    fprintf( stderr, " arg2=%p,", req->arg2 );
    fprintf( stderr, " arg3=%p", req->arg3 );
}

static void dump_get_apc_request( const struct get_apc_request *req )
{
    fprintf( stderr, " alertable=%d", req->alertable );
}

static void dump_get_apc_reply( const struct get_apc_reply *req )
{
    fprintf( stderr, " func=%p,", req->func );
    fprintf( stderr, " type=%d,", req->type );
    fprintf( stderr, " arg1=%p,", req->arg1 );
    fprintf( stderr, " arg2=%p,", req->arg2 );
    fprintf( stderr, " arg3=%p", req->arg3 );
}

static void dump_close_handle_request( const struct close_handle_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_close_handle_reply( const struct close_handle_reply *req )
{
    fprintf( stderr, " fd=%d", req->fd );
}

static void dump_set_handle_info_request( const struct set_handle_info_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " flags=%d,", req->flags );
    fprintf( stderr, " mask=%d,", req->mask );
    fprintf( stderr, " fd=%d", req->fd );
}

static void dump_set_handle_info_reply( const struct set_handle_info_reply *req )
{
    fprintf( stderr, " old_flags=%d,", req->old_flags );
    fprintf( stderr, " cur_fd=%d", req->cur_fd );
}

static void dump_dup_handle_request( const struct dup_handle_request *req )
{
    fprintf( stderr, " src_process=%p,", req->src_process );
    fprintf( stderr, " src_handle=%p,", req->src_handle );
    fprintf( stderr, " dst_process=%p,", req->dst_process );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " options=%d", req->options );
}

static void dump_dup_handle_reply( const struct dup_handle_reply *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " fd=%d", req->fd );
}

static void dump_open_process_request( const struct open_process_request *req )
{
    fprintf( stderr, " pid=%04x,", req->pid );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d", req->inherit );
}

static void dump_open_process_reply( const struct open_process_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_open_thread_request( const struct open_thread_request *req )
{
    fprintf( stderr, " tid=%04x,", req->tid );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d", req->inherit );
}

static void dump_open_thread_reply( const struct open_thread_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_select_request( const struct select_request *req )
{
    fprintf( stderr, " flags=%d,", req->flags );
    fprintf( stderr, " cookie=%p,", req->cookie );
    fprintf( stderr, " timeout=" );
    dump_abs_time( &req->timeout );
    fprintf( stderr, "," );
    fprintf( stderr, " handles=" );
    dump_varargs_handles( cur_size );
}

static void dump_create_event_request( const struct create_event_request *req )
{
    fprintf( stderr, " manual_reset=%d,", req->manual_reset );
    fprintf( stderr, " initial_state=%d,", req->initial_state );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_create_event_reply( const struct create_event_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_event_op_request( const struct event_op_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " op=%d", req->op );
}

static void dump_open_event_request( const struct open_event_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_open_event_reply( const struct open_event_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_create_mutex_request( const struct create_mutex_request *req )
{
    fprintf( stderr, " owned=%d,", req->owned );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_create_mutex_reply( const struct create_mutex_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_release_mutex_request( const struct release_mutex_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_open_mutex_request( const struct open_mutex_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_open_mutex_reply( const struct open_mutex_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_create_semaphore_request( const struct create_semaphore_request *req )
{
    fprintf( stderr, " initial=%08x,", req->initial );
    fprintf( stderr, " max=%08x,", req->max );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_create_semaphore_reply( const struct create_semaphore_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_release_semaphore_request( const struct release_semaphore_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " count=%08x", req->count );
}

static void dump_release_semaphore_reply( const struct release_semaphore_reply *req )
{
    fprintf( stderr, " prev_count=%08x", req->prev_count );
}

static void dump_open_semaphore_request( const struct open_semaphore_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_open_semaphore_reply( const struct open_semaphore_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_create_file_request( const struct create_file_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " sharing=%08x,", req->sharing );
    fprintf( stderr, " create=%d,", req->create );
    fprintf( stderr, " attrs=%08x,", req->attrs );
    fprintf( stderr, " drive_type=%d,", req->drive_type );
    fprintf( stderr, " filename=" );
    dump_varargs_string( cur_size );
}

static void dump_create_file_reply( const struct create_file_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_alloc_file_handle_request( const struct alloc_file_handle_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " fd=%d", req->fd );
}

static void dump_alloc_file_handle_reply( const struct alloc_file_handle_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_handle_fd_request( const struct get_handle_fd_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " access=%08x", req->access );
}

static void dump_get_handle_fd_reply( const struct get_handle_fd_reply *req )
{
    fprintf( stderr, " fd=%d,", req->fd );
    fprintf( stderr, " type=%d,", req->type );
    fprintf( stderr, " flags=%d", req->flags );
}

static void dump_set_file_pointer_request( const struct set_file_pointer_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " low=%d,", req->low );
    fprintf( stderr, " high=%d,", req->high );
    fprintf( stderr, " whence=%d", req->whence );
}

static void dump_set_file_pointer_reply( const struct set_file_pointer_reply *req )
{
    fprintf( stderr, " new_low=%d,", req->new_low );
    fprintf( stderr, " new_high=%d", req->new_high );
}

static void dump_truncate_file_request( const struct truncate_file_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_set_file_time_request( const struct set_file_time_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " access_time=%ld,", req->access_time );
    fprintf( stderr, " write_time=%ld", req->write_time );
}

static void dump_flush_file_request( const struct flush_file_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_flush_file_reply( const struct flush_file_reply *req )
{
    fprintf( stderr, " event=%p", req->event );
}

static void dump_get_file_info_request( const struct get_file_info_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_file_info_reply( const struct get_file_info_reply *req )
{
    fprintf( stderr, " type=%d,", req->type );
    fprintf( stderr, " attr=%d,", req->attr );
    fprintf( stderr, " access_time=%ld,", req->access_time );
    fprintf( stderr, " write_time=%ld,", req->write_time );
    fprintf( stderr, " size_high=%d,", req->size_high );
    fprintf( stderr, " size_low=%d,", req->size_low );
    fprintf( stderr, " links=%d,", req->links );
    fprintf( stderr, " index_high=%d,", req->index_high );
    fprintf( stderr, " index_low=%d,", req->index_low );
    fprintf( stderr, " serial=%08x", req->serial );
}

static void dump_lock_file_request( const struct lock_file_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " offset_low=%08x,", req->offset_low );
    fprintf( stderr, " offset_high=%08x,", req->offset_high );
    fprintf( stderr, " count_low=%08x,", req->count_low );
    fprintf( stderr, " count_high=%08x,", req->count_high );
    fprintf( stderr, " shared=%d,", req->shared );
    fprintf( stderr, " wait=%d", req->wait );
}

static void dump_lock_file_reply( const struct lock_file_reply *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " overlapped=%d", req->overlapped );
}

static void dump_unlock_file_request( const struct unlock_file_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " offset_low=%08x,", req->offset_low );
    fprintf( stderr, " offset_high=%08x,", req->offset_high );
    fprintf( stderr, " count_low=%08x,", req->count_low );
    fprintf( stderr, " count_high=%08x", req->count_high );
}

static void dump_create_pipe_request( const struct create_pipe_request *req )
{
    fprintf( stderr, " inherit=%d", req->inherit );
}

static void dump_create_pipe_reply( const struct create_pipe_reply *req )
{
    fprintf( stderr, " handle_read=%p,", req->handle_read );
    fprintf( stderr, " handle_write=%p", req->handle_write );
}

static void dump_create_socket_request( const struct create_socket_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " family=%d,", req->family );
    fprintf( stderr, " type=%d,", req->type );
    fprintf( stderr, " protocol=%d,", req->protocol );
    fprintf( stderr, " flags=%08x", req->flags );
}

static void dump_create_socket_reply( const struct create_socket_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_accept_socket_request( const struct accept_socket_request *req )
{
    fprintf( stderr, " lhandle=%p,", req->lhandle );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d", req->inherit );
}

static void dump_accept_socket_reply( const struct accept_socket_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_set_socket_event_request( const struct set_socket_event_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " mask=%08x,", req->mask );
    fprintf( stderr, " event=%p,", req->event );
    fprintf( stderr, " window=%p,", req->window );
    fprintf( stderr, " msg=%08x", req->msg );
}

static void dump_get_socket_event_request( const struct get_socket_event_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " service=%d,", req->service );
    fprintf( stderr, " c_event=%p", req->c_event );
}

static void dump_get_socket_event_reply( const struct get_socket_event_reply *req )
{
    fprintf( stderr, " mask=%08x,", req->mask );
    fprintf( stderr, " pmask=%08x,", req->pmask );
    fprintf( stderr, " state=%08x,", req->state );
    fprintf( stderr, " errors=" );
    dump_varargs_ints( cur_size );
}

static void dump_enable_socket_event_request( const struct enable_socket_event_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " mask=%08x,", req->mask );
    fprintf( stderr, " sstate=%08x,", req->sstate );
    fprintf( stderr, " cstate=%08x", req->cstate );
}

static void dump_set_socket_deferred_request( const struct set_socket_deferred_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " deferred=%p", req->deferred );
}

static void dump_alloc_console_request( const struct alloc_console_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " pid=%04x", req->pid );
}

static void dump_alloc_console_reply( const struct alloc_console_reply *req )
{
    fprintf( stderr, " handle_in=%p,", req->handle_in );
    fprintf( stderr, " event=%p", req->event );
}

static void dump_free_console_request( const struct free_console_request *req )
{
}

static void dump_get_console_renderer_events_request( const struct get_console_renderer_events_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_console_renderer_events_reply( const struct get_console_renderer_events_reply *req )
{
    fprintf( stderr, " data=" );
    dump_varargs_bytes( cur_size );
}

static void dump_open_console_request( const struct open_console_request *req )
{
    fprintf( stderr, " from=%d,", req->from );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " share=%d", req->share );
}

static void dump_open_console_reply( const struct open_console_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_console_mode_request( const struct get_console_mode_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_console_mode_reply( const struct get_console_mode_reply *req )
{
    fprintf( stderr, " mode=%d", req->mode );
}

static void dump_set_console_mode_request( const struct set_console_mode_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " mode=%d", req->mode );
}

static void dump_set_console_input_info_request( const struct set_console_input_info_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " mask=%d,", req->mask );
    fprintf( stderr, " active_sb=%p,", req->active_sb );
    fprintf( stderr, " history_mode=%d,", req->history_mode );
    fprintf( stderr, " history_size=%d,", req->history_size );
    fprintf( stderr, " edition_mode=%d,", req->edition_mode );
    fprintf( stderr, " title=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_get_console_input_info_request( const struct get_console_input_info_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_console_input_info_reply( const struct get_console_input_info_reply *req )
{
    fprintf( stderr, " history_mode=%d,", req->history_mode );
    fprintf( stderr, " history_size=%d,", req->history_size );
    fprintf( stderr, " history_index=%d,", req->history_index );
    fprintf( stderr, " edition_mode=%d,", req->edition_mode );
    fprintf( stderr, " title=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_append_console_input_history_request( const struct append_console_input_history_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " line=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_get_console_input_history_request( const struct get_console_input_history_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " index=%d", req->index );
}

static void dump_get_console_input_history_reply( const struct get_console_input_history_reply *req )
{
    fprintf( stderr, " total=%d,", req->total );
    fprintf( stderr, " line=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_create_console_output_request( const struct create_console_output_request *req )
{
    fprintf( stderr, " handle_in=%p,", req->handle_in );
    fprintf( stderr, " access=%d,", req->access );
    fprintf( stderr, " share=%d,", req->share );
    fprintf( stderr, " inherit=%d", req->inherit );
}

static void dump_create_console_output_reply( const struct create_console_output_reply *req )
{
    fprintf( stderr, " handle_out=%p", req->handle_out );
}

static void dump_set_console_output_info_request( const struct set_console_output_info_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " mask=%d,", req->mask );
    fprintf( stderr, " cursor_size=%d,", req->cursor_size );
    fprintf( stderr, " cursor_visible=%d,", req->cursor_visible );
    fprintf( stderr, " cursor_x=%d,", req->cursor_x );
    fprintf( stderr, " cursor_y=%d,", req->cursor_y );
    fprintf( stderr, " width=%d,", req->width );
    fprintf( stderr, " height=%d,", req->height );
    fprintf( stderr, " attr=%d,", req->attr );
    fprintf( stderr, " win_left=%d,", req->win_left );
    fprintf( stderr, " win_top=%d,", req->win_top );
    fprintf( stderr, " win_right=%d,", req->win_right );
    fprintf( stderr, " win_bottom=%d,", req->win_bottom );
    fprintf( stderr, " max_width=%d,", req->max_width );
    fprintf( stderr, " max_height=%d", req->max_height );
}

static void dump_get_console_output_info_request( const struct get_console_output_info_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_console_output_info_reply( const struct get_console_output_info_reply *req )
{
    fprintf( stderr, " cursor_size=%d,", req->cursor_size );
    fprintf( stderr, " cursor_visible=%d,", req->cursor_visible );
    fprintf( stderr, " cursor_x=%d,", req->cursor_x );
    fprintf( stderr, " cursor_y=%d,", req->cursor_y );
    fprintf( stderr, " width=%d,", req->width );
    fprintf( stderr, " height=%d,", req->height );
    fprintf( stderr, " attr=%d,", req->attr );
    fprintf( stderr, " win_left=%d,", req->win_left );
    fprintf( stderr, " win_top=%d,", req->win_top );
    fprintf( stderr, " win_right=%d,", req->win_right );
    fprintf( stderr, " win_bottom=%d,", req->win_bottom );
    fprintf( stderr, " max_width=%d,", req->max_width );
    fprintf( stderr, " max_height=%d", req->max_height );
}

static void dump_write_console_input_request( const struct write_console_input_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " rec=" );
    dump_varargs_input_records( cur_size );
}

static void dump_write_console_input_reply( const struct write_console_input_reply *req )
{
    fprintf( stderr, " written=%d", req->written );
}

static void dump_read_console_input_request( const struct read_console_input_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " flush=%d", req->flush );
}

static void dump_read_console_input_reply( const struct read_console_input_reply *req )
{
    fprintf( stderr, " read=%d,", req->read );
    fprintf( stderr, " rec=" );
    dump_varargs_input_records( cur_size );
}

static void dump_write_console_output_request( const struct write_console_output_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " x=%d,", req->x );
    fprintf( stderr, " y=%d,", req->y );
    fprintf( stderr, " mode=%d,", req->mode );
    fprintf( stderr, " wrap=%d,", req->wrap );
    fprintf( stderr, " data=" );
    dump_varargs_bytes( cur_size );
}

static void dump_write_console_output_reply( const struct write_console_output_reply *req )
{
    fprintf( stderr, " written=%d,", req->written );
    fprintf( stderr, " width=%d,", req->width );
    fprintf( stderr, " height=%d", req->height );
}

static void dump_fill_console_output_request( const struct fill_console_output_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " x=%d,", req->x );
    fprintf( stderr, " y=%d,", req->y );
    fprintf( stderr, " mode=%d,", req->mode );
    fprintf( stderr, " count=%d,", req->count );
    fprintf( stderr, " wrap=%d,", req->wrap );
    fprintf( stderr, " data=" );
    dump_char_info( &req->data );
}

static void dump_fill_console_output_reply( const struct fill_console_output_reply *req )
{
    fprintf( stderr, " written=%d", req->written );
}

static void dump_read_console_output_request( const struct read_console_output_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " x=%d,", req->x );
    fprintf( stderr, " y=%d,", req->y );
    fprintf( stderr, " mode=%d,", req->mode );
    fprintf( stderr, " wrap=%d", req->wrap );
}

static void dump_read_console_output_reply( const struct read_console_output_reply *req )
{
    fprintf( stderr, " width=%d,", req->width );
    fprintf( stderr, " height=%d,", req->height );
    fprintf( stderr, " data=" );
    dump_varargs_bytes( cur_size );
}

static void dump_move_console_output_request( const struct move_console_output_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " x_src=%d,", req->x_src );
    fprintf( stderr, " y_src=%d,", req->y_src );
    fprintf( stderr, " x_dst=%d,", req->x_dst );
    fprintf( stderr, " y_dst=%d,", req->y_dst );
    fprintf( stderr, " w=%d,", req->w );
    fprintf( stderr, " h=%d", req->h );
}

static void dump_send_console_signal_request( const struct send_console_signal_request *req )
{
    fprintf( stderr, " signal=%d,", req->signal );
    fprintf( stderr, " group_id=%04x", req->group_id );
}

static void dump_create_change_notification_request( const struct create_change_notification_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " subtree=%d,", req->subtree );
    fprintf( stderr, " filter=%08x", req->filter );
}

static void dump_create_change_notification_reply( const struct create_change_notification_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_next_change_notification_request( const struct next_change_notification_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_create_mapping_request( const struct create_mapping_request *req )
{
    fprintf( stderr, " size_high=%d,", req->size_high );
    fprintf( stderr, " size_low=%d,", req->size_low );
    fprintf( stderr, " protect=%d,", req->protect );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " file_handle=%p,", req->file_handle );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_create_mapping_reply( const struct create_mapping_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_open_mapping_request( const struct open_mapping_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_open_mapping_reply( const struct open_mapping_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_mapping_info_request( const struct get_mapping_info_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_mapping_info_reply( const struct get_mapping_info_reply *req )
{
    fprintf( stderr, " size_high=%d,", req->size_high );
    fprintf( stderr, " size_low=%d,", req->size_low );
    fprintf( stderr, " protect=%d,", req->protect );
    fprintf( stderr, " header_size=%d,", req->header_size );
    fprintf( stderr, " base=%p,", req->base );
    fprintf( stderr, " shared_file=%p,", req->shared_file );
    fprintf( stderr, " shared_size=%d,", req->shared_size );
    fprintf( stderr, " drive_type=%d", req->drive_type );
}

static void dump_create_device_request( const struct create_device_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " id=%d", req->id );
}

static void dump_create_device_reply( const struct create_device_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_device_id_request( const struct get_device_id_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_device_id_reply( const struct get_device_id_reply *req )
{
    fprintf( stderr, " id=%d", req->id );
}

static void dump_create_snapshot_request( const struct create_snapshot_request *req )
{
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " flags=%d,", req->flags );
    fprintf( stderr, " pid=%04x", req->pid );
}

static void dump_create_snapshot_reply( const struct create_snapshot_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_next_process_request( const struct next_process_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " reset=%d", req->reset );
}

static void dump_next_process_reply( const struct next_process_reply *req )
{
    fprintf( stderr, " count=%d,", req->count );
    fprintf( stderr, " pid=%04x,", req->pid );
    fprintf( stderr, " ppid=%04x,", req->ppid );
    fprintf( stderr, " heap=%p,", req->heap );
    fprintf( stderr, " module=%p,", req->module );
    fprintf( stderr, " threads=%d,", req->threads );
    fprintf( stderr, " priority=%d,", req->priority );
    fprintf( stderr, " filename=" );
    dump_varargs_string( cur_size );
}

static void dump_next_thread_request( const struct next_thread_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " reset=%d", req->reset );
}

static void dump_next_thread_reply( const struct next_thread_reply *req )
{
    fprintf( stderr, " count=%d,", req->count );
    fprintf( stderr, " pid=%04x,", req->pid );
    fprintf( stderr, " tid=%04x,", req->tid );
    fprintf( stderr, " base_pri=%d,", req->base_pri );
    fprintf( stderr, " delta_pri=%d", req->delta_pri );
}

static void dump_next_module_request( const struct next_module_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " reset=%d", req->reset );
}

static void dump_next_module_reply( const struct next_module_reply *req )
{
    fprintf( stderr, " pid=%04x,", req->pid );
    fprintf( stderr, " base=%p,", req->base );
    fprintf( stderr, " size=%d,", req->size );
    fprintf( stderr, " filename=" );
    dump_varargs_string( cur_size );
}

static void dump_wait_debug_event_request( const struct wait_debug_event_request *req )
{
    fprintf( stderr, " get_handle=%d", req->get_handle );
}

static void dump_wait_debug_event_reply( const struct wait_debug_event_reply *req )
{
    fprintf( stderr, " pid=%04x,", req->pid );
    fprintf( stderr, " tid=%04x,", req->tid );
    fprintf( stderr, " wait=%p,", req->wait );
    fprintf( stderr, " event=" );
    dump_varargs_debug_event( cur_size );
}

static void dump_queue_exception_event_request( const struct queue_exception_event_request *req )
{
    fprintf( stderr, " first=%d,", req->first );
    fprintf( stderr, " record=" );
    dump_varargs_exc_event( cur_size );
}

static void dump_queue_exception_event_reply( const struct queue_exception_event_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_exception_status_request( const struct get_exception_status_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_exception_status_reply( const struct get_exception_status_reply *req )
{
    fprintf( stderr, " status=%d,", req->status );
    fprintf( stderr, " context=" );
    dump_varargs_context( cur_size );
}

static void dump_output_debug_string_request( const struct output_debug_string_request *req )
{
    fprintf( stderr, " string=%p,", req->string );
    fprintf( stderr, " unicode=%d,", req->unicode );
    fprintf( stderr, " length=%d", req->length );
}

static void dump_continue_debug_event_request( const struct continue_debug_event_request *req )
{
    fprintf( stderr, " pid=%04x,", req->pid );
    fprintf( stderr, " tid=%04x,", req->tid );
    fprintf( stderr, " status=%d", req->status );
}

static void dump_debug_process_request( const struct debug_process_request *req )
{
    fprintf( stderr, " pid=%04x,", req->pid );
    fprintf( stderr, " attach=%d", req->attach );
}

static void dump_debug_break_request( const struct debug_break_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_debug_break_reply( const struct debug_break_reply *req )
{
    fprintf( stderr, " self=%d", req->self );
}

static void dump_set_debugger_kill_on_exit_request( const struct set_debugger_kill_on_exit_request *req )
{
    fprintf( stderr, " kill_on_exit=%d", req->kill_on_exit );
}

static void dump_read_process_memory_request( const struct read_process_memory_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " addr=%p", req->addr );
}

static void dump_read_process_memory_reply( const struct read_process_memory_reply *req )
{
    fprintf( stderr, " data=" );
    dump_varargs_bytes( cur_size );
}

static void dump_write_process_memory_request( const struct write_process_memory_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " addr=%p,", req->addr );
    fprintf( stderr, " first_mask=%08x,", req->first_mask );
    fprintf( stderr, " last_mask=%08x,", req->last_mask );
    fprintf( stderr, " data=" );
    dump_varargs_bytes( cur_size );
}

static void dump_create_key_request( const struct create_key_request *req )
{
    fprintf( stderr, " parent=%p,", req->parent );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " options=%08x,", req->options );
    fprintf( stderr, " modif=%ld,", req->modif );
    fprintf( stderr, " namelen=%d,", req->namelen );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( min(cur_size,req->namelen) );
    fputc( ',', stderr );
    fprintf( stderr, " class=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_create_key_reply( const struct create_key_reply *req )
{
    fprintf( stderr, " hkey=%p,", req->hkey );
    fprintf( stderr, " created=%d", req->created );
}

static void dump_open_key_request( const struct open_key_request *req )
{
    fprintf( stderr, " parent=%p,", req->parent );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_open_key_reply( const struct open_key_reply *req )
{
    fprintf( stderr, " hkey=%p", req->hkey );
}

static void dump_delete_key_request( const struct delete_key_request *req )
{
    fprintf( stderr, " hkey=%p", req->hkey );
}

static void dump_enum_key_request( const struct enum_key_request *req )
{
    fprintf( stderr, " hkey=%p,", req->hkey );
    fprintf( stderr, " index=%d,", req->index );
    fprintf( stderr, " info_class=%d", req->info_class );
}

static void dump_enum_key_reply( const struct enum_key_reply *req )
{
    fprintf( stderr, " subkeys=%d,", req->subkeys );
    fprintf( stderr, " max_subkey=%d,", req->max_subkey );
    fprintf( stderr, " max_class=%d,", req->max_class );
    fprintf( stderr, " values=%d,", req->values );
    fprintf( stderr, " max_value=%d,", req->max_value );
    fprintf( stderr, " max_data=%d,", req->max_data );
    fprintf( stderr, " modif=%ld,", req->modif );
    fprintf( stderr, " total=%d,", req->total );
    fprintf( stderr, " namelen=%d,", req->namelen );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( min(cur_size,req->namelen) );
    fputc( ',', stderr );
    fprintf( stderr, " class=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_set_key_value_request( const struct set_key_value_request *req )
{
    fprintf( stderr, " hkey=%p,", req->hkey );
    fprintf( stderr, " type=%d,", req->type );
    fprintf( stderr, " namelen=%d,", req->namelen );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( min(cur_size,req->namelen) );
    fputc( ',', stderr );
    fprintf( stderr, " data=" );
    dump_varargs_bytes( cur_size );
}

static void dump_get_key_value_request( const struct get_key_value_request *req )
{
    fprintf( stderr, " hkey=%p,", req->hkey );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_get_key_value_reply( const struct get_key_value_reply *req )
{
    fprintf( stderr, " type=%d,", req->type );
    fprintf( stderr, " total=%d,", req->total );
    fprintf( stderr, " data=" );
    dump_varargs_bytes( cur_size );
}

static void dump_enum_key_value_request( const struct enum_key_value_request *req )
{
    fprintf( stderr, " hkey=%p,", req->hkey );
    fprintf( stderr, " index=%d,", req->index );
    fprintf( stderr, " info_class=%d", req->info_class );
}

static void dump_enum_key_value_reply( const struct enum_key_value_reply *req )
{
    fprintf( stderr, " type=%d,", req->type );
    fprintf( stderr, " total=%d,", req->total );
    fprintf( stderr, " namelen=%d,", req->namelen );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( min(cur_size,req->namelen) );
    fputc( ',', stderr );
    fprintf( stderr, " data=" );
    dump_varargs_bytes( cur_size );
}

static void dump_delete_key_value_request( const struct delete_key_value_request *req )
{
    fprintf( stderr, " hkey=%p,", req->hkey );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_load_registry_request( const struct load_registry_request *req )
{
    fprintf( stderr, " hkey=%p,", req->hkey );
    fprintf( stderr, " file=%p,", req->file );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_save_registry_request( const struct save_registry_request *req )
{
    fprintf( stderr, " hkey=%p,", req->hkey );
    fprintf( stderr, " file=%p", req->file );
}

static void dump_save_registry_atexit_request( const struct save_registry_atexit_request *req )
{
    fprintf( stderr, " hkey=%p,", req->hkey );
    fprintf( stderr, " file=" );
    dump_varargs_string( cur_size );
}

static void dump_set_registry_levels_request( const struct set_registry_levels_request *req )
{
    fprintf( stderr, " current=%d,", req->current );
    fprintf( stderr, " saving=%d,", req->saving );
    fprintf( stderr, " period=%d", req->period );
}

static void dump_set_registry_notification_request( const struct set_registry_notification_request *req )
{
    fprintf( stderr, " hkey=%p,", req->hkey );
    fprintf( stderr, " event=%p,", req->event );
    fprintf( stderr, " subtree=%d,", req->subtree );
    fprintf( stderr, " filter=%08x", req->filter );
}

static void dump_create_timer_request( const struct create_timer_request *req )
{
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " manual=%d,", req->manual );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_create_timer_reply( const struct create_timer_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_open_timer_request( const struct open_timer_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_open_timer_reply( const struct open_timer_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_set_timer_request( const struct set_timer_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " expire=" );
    dump_abs_time( &req->expire );
    fprintf( stderr, "," );
    fprintf( stderr, " period=%d,", req->period );
    fprintf( stderr, " callback=%p,", req->callback );
    fprintf( stderr, " arg=%p", req->arg );
}

static void dump_set_timer_reply( const struct set_timer_reply *req )
{
    fprintf( stderr, " signaled=%d", req->signaled );
}

static void dump_cancel_timer_request( const struct cancel_timer_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_cancel_timer_reply( const struct cancel_timer_reply *req )
{
    fprintf( stderr, " signaled=%d", req->signaled );
}

static void dump_get_thread_context_request( const struct get_thread_context_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " flags=%08x", req->flags );
}

static void dump_get_thread_context_reply( const struct get_thread_context_reply *req )
{
    fprintf( stderr, " context=" );
    dump_varargs_context( cur_size );
}

static void dump_set_thread_context_request( const struct set_thread_context_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " flags=%08x,", req->flags );
    fprintf( stderr, " context=" );
    dump_varargs_context( cur_size );
}

static void dump_get_selector_entry_request( const struct get_selector_entry_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " entry=%d", req->entry );
}

static void dump_get_selector_entry_reply( const struct get_selector_entry_reply *req )
{
    fprintf( stderr, " base=%08x,", req->base );
    fprintf( stderr, " limit=%08x,", req->limit );
    fprintf( stderr, " flags=%02x", req->flags );
}

static void dump_add_atom_request( const struct add_atom_request *req )
{
    fprintf( stderr, " local=%d,", req->local );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_add_atom_reply( const struct add_atom_reply *req )
{
    fprintf( stderr, " atom=%04x", req->atom );
}

static void dump_delete_atom_request( const struct delete_atom_request *req )
{
    fprintf( stderr, " atom=%04x,", req->atom );
    fprintf( stderr, " local=%d", req->local );
}

static void dump_find_atom_request( const struct find_atom_request *req )
{
    fprintf( stderr, " local=%d,", req->local );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_find_atom_reply( const struct find_atom_reply *req )
{
    fprintf( stderr, " atom=%04x", req->atom );
}

static void dump_get_atom_name_request( const struct get_atom_name_request *req )
{
    fprintf( stderr, " atom=%04x,", req->atom );
    fprintf( stderr, " local=%d", req->local );
}

static void dump_get_atom_name_reply( const struct get_atom_name_reply *req )
{
    fprintf( stderr, " count=%d,", req->count );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_init_atom_table_request( const struct init_atom_table_request *req )
{
    fprintf( stderr, " entries=%d", req->entries );
}

static void dump_get_msg_queue_request( const struct get_msg_queue_request *req )
{
}

static void dump_get_msg_queue_reply( const struct get_msg_queue_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_set_queue_mask_request( const struct set_queue_mask_request *req )
{
    fprintf( stderr, " wake_mask=%08x,", req->wake_mask );
    fprintf( stderr, " changed_mask=%08x,", req->changed_mask );
    fprintf( stderr, " skip_wait=%d", req->skip_wait );
}

static void dump_set_queue_mask_reply( const struct set_queue_mask_reply *req )
{
    fprintf( stderr, " wake_bits=%08x,", req->wake_bits );
    fprintf( stderr, " changed_bits=%08x", req->changed_bits );
}

static void dump_get_queue_status_request( const struct get_queue_status_request *req )
{
    fprintf( stderr, " clear=%d", req->clear );
}

static void dump_get_queue_status_reply( const struct get_queue_status_reply *req )
{
    fprintf( stderr, " wake_bits=%08x,", req->wake_bits );
    fprintf( stderr, " changed_bits=%08x", req->changed_bits );
}

static void dump_wait_input_idle_request( const struct wait_input_idle_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " timeout=%d", req->timeout );
}

static void dump_wait_input_idle_reply( const struct wait_input_idle_reply *req )
{
    fprintf( stderr, " event=%p", req->event );
}

static void dump_send_message_request( const struct send_message_request *req )
{
    fprintf( stderr, " id=%04x,", req->id );
    fprintf( stderr, " type=%d,", req->type );
    fprintf( stderr, " win=%p,", req->win );
    fprintf( stderr, " msg=%08x,", req->msg );
    fprintf( stderr, " wparam=%08x,", req->wparam );
    fprintf( stderr, " lparam=%08x,", req->lparam );
    fprintf( stderr, " x=%d,", req->x );
    fprintf( stderr, " y=%d,", req->y );
    fprintf( stderr, " time=%08x,", req->time );
    fprintf( stderr, " info=%08x,", req->info );
    fprintf( stderr, " timeout=%d,", req->timeout );
    fprintf( stderr, " data=" );
    dump_varargs_bytes( cur_size );
}

static void dump_get_message_request( const struct get_message_request *req )
{
    fprintf( stderr, " flags=%d,", req->flags );
    fprintf( stderr, " get_win=%p,", req->get_win );
    fprintf( stderr, " get_first=%08x,", req->get_first );
    fprintf( stderr, " get_last=%08x", req->get_last );
}

static void dump_get_message_reply( const struct get_message_reply *req )
{
    fprintf( stderr, " type=%d,", req->type );
    fprintf( stderr, " win=%p,", req->win );
    fprintf( stderr, " msg=%08x,", req->msg );
    fprintf( stderr, " wparam=%08x,", req->wparam );
    fprintf( stderr, " lparam=%08x,", req->lparam );
    fprintf( stderr, " x=%d,", req->x );
    fprintf( stderr, " y=%d,", req->y );
    fprintf( stderr, " time=%08x,", req->time );
    fprintf( stderr, " info=%08x,", req->info );
    fprintf( stderr, " total=%d,", req->total );
    fprintf( stderr, " data=" );
    dump_varargs_bytes( cur_size );
}

static void dump_reply_message_request( const struct reply_message_request *req )
{
    fprintf( stderr, " type=%d,", req->type );
    fprintf( stderr, " result=%08x,", req->result );
    fprintf( stderr, " remove=%d,", req->remove );
    fprintf( stderr, " data=" );
    dump_varargs_bytes( cur_size );
}

static void dump_get_message_reply_request( const struct get_message_reply_request *req )
{
    fprintf( stderr, " cancel=%d", req->cancel );
}

static void dump_get_message_reply_reply( const struct get_message_reply_reply *req )
{
    fprintf( stderr, " result=%08x,", req->result );
    fprintf( stderr, " data=" );
    dump_varargs_bytes( cur_size );
}

static void dump_set_win_timer_request( const struct set_win_timer_request *req )
{
    fprintf( stderr, " win=%p,", req->win );
    fprintf( stderr, " msg=%08x,", req->msg );
    fprintf( stderr, " id=%08x,", req->id );
    fprintf( stderr, " rate=%08x,", req->rate );
    fprintf( stderr, " lparam=%08x", req->lparam );
}

static void dump_kill_win_timer_request( const struct kill_win_timer_request *req )
{
    fprintf( stderr, " win=%p,", req->win );
    fprintf( stderr, " msg=%08x,", req->msg );
    fprintf( stderr, " id=%08x", req->id );
}

static void dump_create_serial_request( const struct create_serial_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " attributes=%08x,", req->attributes );
    fprintf( stderr, " sharing=%08x,", req->sharing );
    fprintf( stderr, " name=" );
    dump_varargs_string( cur_size );
}

static void dump_create_serial_reply( const struct create_serial_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_serial_info_request( const struct get_serial_info_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_serial_info_reply( const struct get_serial_info_reply *req )
{
    fprintf( stderr, " readinterval=%08x,", req->readinterval );
    fprintf( stderr, " readconst=%08x,", req->readconst );
    fprintf( stderr, " readmult=%08x,", req->readmult );
    fprintf( stderr, " writeconst=%08x,", req->writeconst );
    fprintf( stderr, " writemult=%08x,", req->writemult );
    fprintf( stderr, " eventmask=%08x,", req->eventmask );
    fprintf( stderr, " commerror=%08x", req->commerror );
}

static void dump_set_serial_info_request( const struct set_serial_info_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " flags=%d,", req->flags );
    fprintf( stderr, " readinterval=%08x,", req->readinterval );
    fprintf( stderr, " readconst=%08x,", req->readconst );
    fprintf( stderr, " readmult=%08x,", req->readmult );
    fprintf( stderr, " writeconst=%08x,", req->writeconst );
    fprintf( stderr, " writemult=%08x,", req->writemult );
    fprintf( stderr, " eventmask=%08x,", req->eventmask );
    fprintf( stderr, " commerror=%08x", req->commerror );
}

static void dump_register_async_request( const struct register_async_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " type=%d,", req->type );
    fprintf( stderr, " overlapped=%p,", req->overlapped );
    fprintf( stderr, " count=%d,", req->count );
    fprintf( stderr, " status=%08x", req->status );
}

static void dump_create_named_pipe_request( const struct create_named_pipe_request *req )
{
    fprintf( stderr, " openmode=%08x,", req->openmode );
    fprintf( stderr, " pipemode=%08x,", req->pipemode );
    fprintf( stderr, " maxinstances=%08x,", req->maxinstances );
    fprintf( stderr, " outsize=%08x,", req->outsize );
    fprintf( stderr, " insize=%08x,", req->insize );
    fprintf( stderr, " timeout=%08x,", req->timeout );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_create_named_pipe_reply( const struct create_named_pipe_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_open_named_pipe_request( const struct open_named_pipe_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_open_named_pipe_reply( const struct open_named_pipe_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_connect_named_pipe_request( const struct connect_named_pipe_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " overlapped=%p,", req->overlapped );
    fprintf( stderr, " func=%p", req->func );
}

static void dump_wait_named_pipe_request( const struct wait_named_pipe_request *req )
{
    fprintf( stderr, " timeout=%08x,", req->timeout );
    fprintf( stderr, " overlapped=%p,", req->overlapped );
    fprintf( stderr, " func=%p,", req->func );
    fprintf( stderr, " name=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_disconnect_named_pipe_request( const struct disconnect_named_pipe_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_disconnect_named_pipe_reply( const struct disconnect_named_pipe_reply *req )
{
    fprintf( stderr, " fd=%d", req->fd );
}

static void dump_get_named_pipe_info_request( const struct get_named_pipe_info_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_named_pipe_info_reply( const struct get_named_pipe_info_reply *req )
{
    fprintf( stderr, " flags=%08x,", req->flags );
    fprintf( stderr, " maxinstances=%08x,", req->maxinstances );
    fprintf( stderr, " outsize=%08x,", req->outsize );
    fprintf( stderr, " insize=%08x", req->insize );
}

static void dump_create_smb_request( const struct create_smb_request *req )
{
    fprintf( stderr, " fd=%d,", req->fd );
    fprintf( stderr, " tree_id=%08x,", req->tree_id );
    fprintf( stderr, " user_id=%08x,", req->user_id );
    fprintf( stderr, " file_id=%08x,", req->file_id );
    fprintf( stderr, " dialect=%08x", req->dialect );
}

static void dump_create_smb_reply( const struct create_smb_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_smb_info_request( const struct get_smb_info_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " flags=%08x,", req->flags );
    fprintf( stderr, " offset=%08x", req->offset );
}

static void dump_get_smb_info_reply( const struct get_smb_info_reply *req )
{
    fprintf( stderr, " tree_id=%08x,", req->tree_id );
    fprintf( stderr, " user_id=%08x,", req->user_id );
    fprintf( stderr, " dialect=%08x,", req->dialect );
    fprintf( stderr, " file_id=%08x,", req->file_id );
    fprintf( stderr, " offset=%08x", req->offset );
}

static void dump_create_window_request( const struct create_window_request *req )
{
    fprintf( stderr, " parent=%p,", req->parent );
    fprintf( stderr, " owner=%p,", req->owner );
    fprintf( stderr, " atom=%04x", req->atom );
}

static void dump_create_window_reply( const struct create_window_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_link_window_request( const struct link_window_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " parent=%p,", req->parent );
    fprintf( stderr, " previous=%p", req->previous );
}

static void dump_link_window_reply( const struct link_window_reply *req )
{
    fprintf( stderr, " full_parent=%p", req->full_parent );
}

static void dump_destroy_window_request( const struct destroy_window_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_set_window_owner_request( const struct set_window_owner_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " owner=%p", req->owner );
}

static void dump_set_window_owner_reply( const struct set_window_owner_reply *req )
{
    fprintf( stderr, " full_owner=%p,", req->full_owner );
    fprintf( stderr, " prev_owner=%p", req->prev_owner );
}

static void dump_get_window_info_request( const struct get_window_info_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_window_info_reply( const struct get_window_info_reply *req )
{
    fprintf( stderr, " full_handle=%p,", req->full_handle );
    fprintf( stderr, " last_active=%p,", req->last_active );
    fprintf( stderr, " pid=%04x,", req->pid );
    fprintf( stderr, " tid=%04x,", req->tid );
    fprintf( stderr, " atom=%04x", req->atom );
}

static void dump_set_window_info_request( const struct set_window_info_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " flags=%08x,", req->flags );
    fprintf( stderr, " style=%08x,", req->style );
    fprintf( stderr, " ex_style=%08x,", req->ex_style );
    fprintf( stderr, " id=%08x,", req->id );
    fprintf( stderr, " instance=%p,", req->instance );
    fprintf( stderr, " user_data=%p", req->user_data );
}

static void dump_set_window_info_reply( const struct set_window_info_reply *req )
{
    fprintf( stderr, " old_style=%08x,", req->old_style );
    fprintf( stderr, " old_ex_style=%08x,", req->old_ex_style );
    fprintf( stderr, " old_id=%08x,", req->old_id );
    fprintf( stderr, " old_instance=%p,", req->old_instance );
    fprintf( stderr, " old_user_data=%p", req->old_user_data );
}

static void dump_get_window_parents_request( const struct get_window_parents_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_window_parents_reply( const struct get_window_parents_reply *req )
{
    fprintf( stderr, " count=%d,", req->count );
    fprintf( stderr, " parents=" );
    dump_varargs_user_handles( cur_size );
}

static void dump_get_window_children_request( const struct get_window_children_request *req )
{
    fprintf( stderr, " parent=%p,", req->parent );
    fprintf( stderr, " atom=%04x,", req->atom );
    fprintf( stderr, " tid=%04x", req->tid );
}

static void dump_get_window_children_reply( const struct get_window_children_reply *req )
{
    fprintf( stderr, " count=%d,", req->count );
    fprintf( stderr, " children=" );
    dump_varargs_user_handles( cur_size );
}

static void dump_get_window_tree_request( const struct get_window_tree_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_window_tree_reply( const struct get_window_tree_reply *req )
{
    fprintf( stderr, " parent=%p,", req->parent );
    fprintf( stderr, " owner=%p,", req->owner );
    fprintf( stderr, " next_sibling=%p,", req->next_sibling );
    fprintf( stderr, " prev_sibling=%p,", req->prev_sibling );
    fprintf( stderr, " first_sibling=%p,", req->first_sibling );
    fprintf( stderr, " last_sibling=%p,", req->last_sibling );
    fprintf( stderr, " first_child=%p,", req->first_child );
    fprintf( stderr, " last_child=%p", req->last_child );
}

static void dump_set_window_rectangles_request( const struct set_window_rectangles_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " window=" );
    dump_rectangle( &req->window );
    fprintf( stderr, "," );
    fprintf( stderr, " client=" );
    dump_rectangle( &req->client );
}

static void dump_get_window_rectangles_request( const struct get_window_rectangles_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_window_rectangles_reply( const struct get_window_rectangles_reply *req )
{
    fprintf( stderr, " window=" );
    dump_rectangle( &req->window );
    fprintf( stderr, "," );
    fprintf( stderr, " client=" );
    dump_rectangle( &req->client );
}

static void dump_get_window_text_request( const struct get_window_text_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_window_text_reply( const struct get_window_text_reply *req )
{
    fprintf( stderr, " text=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_set_window_text_request( const struct set_window_text_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " text=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_inc_window_paint_count_request( const struct inc_window_paint_count_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " incr=%d", req->incr );
}

static void dump_get_windows_offset_request( const struct get_windows_offset_request *req )
{
    fprintf( stderr, " from=%p,", req->from );
    fprintf( stderr, " to=%p", req->to );
}

static void dump_get_windows_offset_reply( const struct get_windows_offset_reply *req )
{
    fprintf( stderr, " x=%d,", req->x );
    fprintf( stderr, " y=%d", req->y );
}

static void dump_set_window_property_request( const struct set_window_property_request *req )
{
    fprintf( stderr, " window=%p,", req->window );
    fprintf( stderr, " atom=%04x,", req->atom );
    fprintf( stderr, " string=%d,", req->string );
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_remove_window_property_request( const struct remove_window_property_request *req )
{
    fprintf( stderr, " window=%p,", req->window );
    fprintf( stderr, " atom=%04x", req->atom );
}

static void dump_remove_window_property_reply( const struct remove_window_property_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_window_property_request( const struct get_window_property_request *req )
{
    fprintf( stderr, " window=%p,", req->window );
    fprintf( stderr, " atom=%04x", req->atom );
}

static void dump_get_window_property_reply( const struct get_window_property_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_window_properties_request( const struct get_window_properties_request *req )
{
    fprintf( stderr, " window=%p", req->window );
}

static void dump_get_window_properties_reply( const struct get_window_properties_reply *req )
{
    fprintf( stderr, " total=%d,", req->total );
    fprintf( stderr, " props=" );
    dump_varargs_properties( cur_size );
}

static void dump_attach_thread_input_request( const struct attach_thread_input_request *req )
{
    fprintf( stderr, " tid_from=%04x,", req->tid_from );
    fprintf( stderr, " tid_to=%04x,", req->tid_to );
    fprintf( stderr, " attach=%d", req->attach );
}

static void dump_get_thread_input_request( const struct get_thread_input_request *req )
{
    fprintf( stderr, " tid=%04x", req->tid );
}

static void dump_get_thread_input_reply( const struct get_thread_input_reply *req )
{
    fprintf( stderr, " focus=%p,", req->focus );
    fprintf( stderr, " capture=%p,", req->capture );
    fprintf( stderr, " active=%p,", req->active );
    fprintf( stderr, " foreground=%p,", req->foreground );
    fprintf( stderr, " menu_owner=%p,", req->menu_owner );
    fprintf( stderr, " move_size=%p,", req->move_size );
    fprintf( stderr, " caret=%p,", req->caret );
    fprintf( stderr, " rect=" );
    dump_rectangle( &req->rect );
}

static void dump_get_key_state_request( const struct get_key_state_request *req )
{
    fprintf( stderr, " tid=%04x,", req->tid );
    fprintf( stderr, " key=%d", req->key );
}

static void dump_get_key_state_reply( const struct get_key_state_reply *req )
{
    fprintf( stderr, " state=%02x,", req->state );
    fprintf( stderr, " keystate=" );
    dump_varargs_bytes( cur_size );
}

static void dump_set_key_state_request( const struct set_key_state_request *req )
{
    fprintf( stderr, " tid=%04x,", req->tid );
    fprintf( stderr, " keystate=" );
    dump_varargs_bytes( cur_size );
}

static void dump_set_foreground_window_request( const struct set_foreground_window_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_set_foreground_window_reply( const struct set_foreground_window_reply *req )
{
    fprintf( stderr, " previous=%p,", req->previous );
    fprintf( stderr, " send_msg_old=%d,", req->send_msg_old );
    fprintf( stderr, " send_msg_new=%d", req->send_msg_new );
}

static void dump_set_focus_window_request( const struct set_focus_window_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_set_focus_window_reply( const struct set_focus_window_reply *req )
{
    fprintf( stderr, " previous=%p", req->previous );
}

static void dump_set_active_window_request( const struct set_active_window_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_set_active_window_reply( const struct set_active_window_reply *req )
{
    fprintf( stderr, " previous=%p", req->previous );
}

static void dump_set_capture_window_request( const struct set_capture_window_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " flags=%08x", req->flags );
}

static void dump_set_capture_window_reply( const struct set_capture_window_reply *req )
{
    fprintf( stderr, " previous=%p,", req->previous );
    fprintf( stderr, " full_handle=%p", req->full_handle );
}

static void dump_set_caret_window_request( const struct set_caret_window_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " width=%d,", req->width );
    fprintf( stderr, " height=%d", req->height );
}

static void dump_set_caret_window_reply( const struct set_caret_window_reply *req )
{
    fprintf( stderr, " previous=%p,", req->previous );
    fprintf( stderr, " old_rect=" );
    dump_rectangle( &req->old_rect );
    fprintf( stderr, "," );
    fprintf( stderr, " old_hide=%d,", req->old_hide );
    fprintf( stderr, " old_state=%d", req->old_state );
}

static void dump_set_caret_info_request( const struct set_caret_info_request *req )
{
    fprintf( stderr, " flags=%08x,", req->flags );
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " x=%d,", req->x );
    fprintf( stderr, " y=%d,", req->y );
    fprintf( stderr, " hide=%d,", req->hide );
    fprintf( stderr, " state=%d", req->state );
}

static void dump_set_caret_info_reply( const struct set_caret_info_reply *req )
{
    fprintf( stderr, " full_handle=%p,", req->full_handle );
    fprintf( stderr, " old_rect=" );
    dump_rectangle( &req->old_rect );
    fprintf( stderr, "," );
    fprintf( stderr, " old_hide=%d,", req->old_hide );
    fprintf( stderr, " old_state=%d", req->old_state );
}

static void dump_set_hook_request( const struct set_hook_request *req )
{
    fprintf( stderr, " id=%d,", req->id );
    fprintf( stderr, " tid=%04x,", req->tid );
    fprintf( stderr, " proc=%p,", req->proc );
    fprintf( stderr, " unicode=%d,", req->unicode );
    fprintf( stderr, " module=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_set_hook_reply( const struct set_hook_reply *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_remove_hook_request( const struct remove_hook_request *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " id=%d,", req->id );
    fprintf( stderr, " proc=%p", req->proc );
}

static void dump_start_hook_chain_request( const struct start_hook_chain_request *req )
{
    fprintf( stderr, " id=%d", req->id );
}

static void dump_start_hook_chain_reply( const struct start_hook_chain_reply *req )
{
    fprintf( stderr, " handle=%p,", req->handle );
    fprintf( stderr, " proc=%p,", req->proc );
    fprintf( stderr, " unicode=%d,", req->unicode );
    fprintf( stderr, " module=" );
    dump_varargs_unicode_str( cur_size );
}

static void dump_finish_hook_chain_request( const struct finish_hook_chain_request *req )
{
    fprintf( stderr, " id=%d", req->id );
}

static void dump_get_next_hook_request( const struct get_next_hook_request *req )
{
    fprintf( stderr, " handle=%p", req->handle );
}

static void dump_get_next_hook_reply( const struct get_next_hook_reply *req )
{
    fprintf( stderr, " next=%p,", req->next );
    fprintf( stderr, " id=%d,", req->id );
    fprintf( stderr, " proc=%p,", req->proc );
    fprintf( stderr, " prev_unicode=%d,", req->prev_unicode );
    fprintf( stderr, " next_unicode=%d,", req->next_unicode );
    fprintf( stderr, " module=" );
    dump_varargs_unicode_str( cur_size );
}

static const dump_func req_dumpers[REQ_NB_REQUESTS] = {
    (dump_func)dump_new_process_request,
    (dump_func)dump_get_new_process_info_request,
    (dump_func)dump_new_thread_request,
    (dump_func)dump_boot_done_request,
    (dump_func)dump_init_process_request,
    (dump_func)dump_get_startup_info_request,
    (dump_func)dump_init_process_done_request,
    (dump_func)dump_init_thread_request,
    (dump_func)dump_terminate_process_request,
    (dump_func)dump_terminate_thread_request,
    (dump_func)dump_get_process_info_request,
    (dump_func)dump_set_process_info_request,
    (dump_func)dump_get_thread_info_request,
    (dump_func)dump_set_thread_info_request,
    (dump_func)dump_get_dll_info_request,
    (dump_func)dump_suspend_thread_request,
    (dump_func)dump_resume_thread_request,
    (dump_func)dump_load_dll_request,
    (dump_func)dump_unload_dll_request,
    (dump_func)dump_queue_apc_request,
    (dump_func)dump_get_apc_request,
    (dump_func)dump_close_handle_request,
    (dump_func)dump_set_handle_info_request,
    (dump_func)dump_dup_handle_request,
    (dump_func)dump_open_process_request,
    (dump_func)dump_open_thread_request,
    (dump_func)dump_select_request,
    (dump_func)dump_create_event_request,
    (dump_func)dump_event_op_request,
    (dump_func)dump_open_event_request,
    (dump_func)dump_create_mutex_request,
    (dump_func)dump_release_mutex_request,
    (dump_func)dump_open_mutex_request,
    (dump_func)dump_create_semaphore_request,
    (dump_func)dump_release_semaphore_request,
    (dump_func)dump_open_semaphore_request,
    (dump_func)dump_create_file_request,
    (dump_func)dump_alloc_file_handle_request,
    (dump_func)dump_get_handle_fd_request,
    (dump_func)dump_set_file_pointer_request,
    (dump_func)dump_truncate_file_request,
    (dump_func)dump_set_file_time_request,
    (dump_func)dump_flush_file_request,
    (dump_func)dump_get_file_info_request,
    (dump_func)dump_lock_file_request,
    (dump_func)dump_unlock_file_request,
    (dump_func)dump_create_pipe_request,
    (dump_func)dump_create_socket_request,
    (dump_func)dump_accept_socket_request,
    (dump_func)dump_set_socket_event_request,
    (dump_func)dump_get_socket_event_request,
    (dump_func)dump_enable_socket_event_request,
    (dump_func)dump_set_socket_deferred_request,
    (dump_func)dump_alloc_console_request,
    (dump_func)dump_free_console_request,
    (dump_func)dump_get_console_renderer_events_request,
    (dump_func)dump_open_console_request,
    (dump_func)dump_get_console_mode_request,
    (dump_func)dump_set_console_mode_request,
    (dump_func)dump_set_console_input_info_request,
    (dump_func)dump_get_console_input_info_request,
    (dump_func)dump_append_console_input_history_request,
    (dump_func)dump_get_console_input_history_request,
    (dump_func)dump_create_console_output_request,
    (dump_func)dump_set_console_output_info_request,
    (dump_func)dump_get_console_output_info_request,
    (dump_func)dump_write_console_input_request,
    (dump_func)dump_read_console_input_request,
    (dump_func)dump_write_console_output_request,
    (dump_func)dump_fill_console_output_request,
    (dump_func)dump_read_console_output_request,
    (dump_func)dump_move_console_output_request,
    (dump_func)dump_send_console_signal_request,
    (dump_func)dump_create_change_notification_request,
    (dump_func)dump_next_change_notification_request,
    (dump_func)dump_create_mapping_request,
    (dump_func)dump_open_mapping_request,
    (dump_func)dump_get_mapping_info_request,
    (dump_func)dump_create_device_request,
    (dump_func)dump_get_device_id_request,
    (dump_func)dump_create_snapshot_request,
    (dump_func)dump_next_process_request,
    (dump_func)dump_next_thread_request,
    (dump_func)dump_next_module_request,
    (dump_func)dump_wait_debug_event_request,
    (dump_func)dump_queue_exception_event_request,
    (dump_func)dump_get_exception_status_request,
    (dump_func)dump_output_debug_string_request,
    (dump_func)dump_continue_debug_event_request,
    (dump_func)dump_debug_process_request,
    (dump_func)dump_debug_break_request,
    (dump_func)dump_set_debugger_kill_on_exit_request,
    (dump_func)dump_read_process_memory_request,
    (dump_func)dump_write_process_memory_request,
    (dump_func)dump_create_key_request,
    (dump_func)dump_open_key_request,
    (dump_func)dump_delete_key_request,
    (dump_func)dump_enum_key_request,
    (dump_func)dump_set_key_value_request,
    (dump_func)dump_get_key_value_request,
    (dump_func)dump_enum_key_value_request,
    (dump_func)dump_delete_key_value_request,
    (dump_func)dump_load_registry_request,
    (dump_func)dump_save_registry_request,
    (dump_func)dump_save_registry_atexit_request,
    (dump_func)dump_set_registry_levels_request,
    (dump_func)dump_set_registry_notification_request,
    (dump_func)dump_create_timer_request,
    (dump_func)dump_open_timer_request,
    (dump_func)dump_set_timer_request,
    (dump_func)dump_cancel_timer_request,
    (dump_func)dump_get_thread_context_request,
    (dump_func)dump_set_thread_context_request,
    (dump_func)dump_get_selector_entry_request,
    (dump_func)dump_add_atom_request,
    (dump_func)dump_delete_atom_request,
    (dump_func)dump_find_atom_request,
    (dump_func)dump_get_atom_name_request,
    (dump_func)dump_init_atom_table_request,
    (dump_func)dump_get_msg_queue_request,
    (dump_func)dump_set_queue_mask_request,
    (dump_func)dump_get_queue_status_request,
    (dump_func)dump_wait_input_idle_request,
    (dump_func)dump_send_message_request,
    (dump_func)dump_get_message_request,
    (dump_func)dump_reply_message_request,
    (dump_func)dump_get_message_reply_request,
    (dump_func)dump_set_win_timer_request,
    (dump_func)dump_kill_win_timer_request,
    (dump_func)dump_create_serial_request,
    (dump_func)dump_get_serial_info_request,
    (dump_func)dump_set_serial_info_request,
    (dump_func)dump_register_async_request,
    (dump_func)dump_create_named_pipe_request,
    (dump_func)dump_open_named_pipe_request,
    (dump_func)dump_connect_named_pipe_request,
    (dump_func)dump_wait_named_pipe_request,
    (dump_func)dump_disconnect_named_pipe_request,
    (dump_func)dump_get_named_pipe_info_request,
    (dump_func)dump_create_smb_request,
    (dump_func)dump_get_smb_info_request,
    (dump_func)dump_create_window_request,
    (dump_func)dump_link_window_request,
    (dump_func)dump_destroy_window_request,
    (dump_func)dump_set_window_owner_request,
    (dump_func)dump_get_window_info_request,
    (dump_func)dump_set_window_info_request,
    (dump_func)dump_get_window_parents_request,
    (dump_func)dump_get_window_children_request,
    (dump_func)dump_get_window_tree_request,
    (dump_func)dump_set_window_rectangles_request,
    (dump_func)dump_get_window_rectangles_request,
    (dump_func)dump_get_window_text_request,
    (dump_func)dump_set_window_text_request,
    (dump_func)dump_inc_window_paint_count_request,
    (dump_func)dump_get_windows_offset_request,
    (dump_func)dump_set_window_property_request,
    (dump_func)dump_remove_window_property_request,
    (dump_func)dump_get_window_property_request,
    (dump_func)dump_get_window_properties_request,
    (dump_func)dump_attach_thread_input_request,
    (dump_func)dump_get_thread_input_request,
    (dump_func)dump_get_key_state_request,
    (dump_func)dump_set_key_state_request,
    (dump_func)dump_set_foreground_window_request,
    (dump_func)dump_set_focus_window_request,
    (dump_func)dump_set_active_window_request,
    (dump_func)dump_set_capture_window_request,
    (dump_func)dump_set_caret_window_request,
    (dump_func)dump_set_caret_info_request,
    (dump_func)dump_set_hook_request,
    (dump_func)dump_remove_hook_request,
    (dump_func)dump_start_hook_chain_request,
    (dump_func)dump_finish_hook_chain_request,
    (dump_func)dump_get_next_hook_request,
};

static const dump_func reply_dumpers[REQ_NB_REQUESTS] = {
    (dump_func)dump_new_process_reply,
    (dump_func)dump_get_new_process_info_reply,
    (dump_func)dump_new_thread_reply,
    (dump_func)0,
    (dump_func)dump_init_process_reply,
    (dump_func)dump_get_startup_info_reply,
    (dump_func)dump_init_process_done_reply,
    (dump_func)dump_init_thread_reply,
    (dump_func)dump_terminate_process_reply,
    (dump_func)dump_terminate_thread_reply,
    (dump_func)dump_get_process_info_reply,
    (dump_func)0,
    (dump_func)dump_get_thread_info_reply,
    (dump_func)0,
    (dump_func)dump_get_dll_info_reply,
    (dump_func)dump_suspend_thread_reply,
    (dump_func)dump_resume_thread_reply,
    (dump_func)0,
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_get_apc_reply,
    (dump_func)dump_close_handle_reply,
    (dump_func)dump_set_handle_info_reply,
    (dump_func)dump_dup_handle_reply,
    (dump_func)dump_open_process_reply,
    (dump_func)dump_open_thread_reply,
    (dump_func)0,
    (dump_func)dump_create_event_reply,
    (dump_func)0,
    (dump_func)dump_open_event_reply,
    (dump_func)dump_create_mutex_reply,
    (dump_func)0,
    (dump_func)dump_open_mutex_reply,
    (dump_func)dump_create_semaphore_reply,
    (dump_func)dump_release_semaphore_reply,
    (dump_func)dump_open_semaphore_reply,
    (dump_func)dump_create_file_reply,
    (dump_func)dump_alloc_file_handle_reply,
    (dump_func)dump_get_handle_fd_reply,
    (dump_func)dump_set_file_pointer_reply,
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_flush_file_reply,
    (dump_func)dump_get_file_info_reply,
    (dump_func)dump_lock_file_reply,
    (dump_func)0,
    (dump_func)dump_create_pipe_reply,
    (dump_func)dump_create_socket_reply,
    (dump_func)dump_accept_socket_reply,
    (dump_func)0,
    (dump_func)dump_get_socket_event_reply,
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_alloc_console_reply,
    (dump_func)0,
    (dump_func)dump_get_console_renderer_events_reply,
    (dump_func)dump_open_console_reply,
    (dump_func)dump_get_console_mode_reply,
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_get_console_input_info_reply,
    (dump_func)0,
    (dump_func)dump_get_console_input_history_reply,
    (dump_func)dump_create_console_output_reply,
    (dump_func)0,
    (dump_func)dump_get_console_output_info_reply,
    (dump_func)dump_write_console_input_reply,
    (dump_func)dump_read_console_input_reply,
    (dump_func)dump_write_console_output_reply,
    (dump_func)dump_fill_console_output_reply,
    (dump_func)dump_read_console_output_reply,
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_create_change_notification_reply,
    (dump_func)0,
    (dump_func)dump_create_mapping_reply,
    (dump_func)dump_open_mapping_reply,
    (dump_func)dump_get_mapping_info_reply,
    (dump_func)dump_create_device_reply,
    (dump_func)dump_get_device_id_reply,
    (dump_func)dump_create_snapshot_reply,
    (dump_func)dump_next_process_reply,
    (dump_func)dump_next_thread_reply,
    (dump_func)dump_next_module_reply,
    (dump_func)dump_wait_debug_event_reply,
    (dump_func)dump_queue_exception_event_reply,
    (dump_func)dump_get_exception_status_reply,
    (dump_func)0,
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_debug_break_reply,
    (dump_func)0,
    (dump_func)dump_read_process_memory_reply,
    (dump_func)0,
    (dump_func)dump_create_key_reply,
    (dump_func)dump_open_key_reply,
    (dump_func)0,
    (dump_func)dump_enum_key_reply,
    (dump_func)0,
    (dump_func)dump_get_key_value_reply,
    (dump_func)dump_enum_key_value_reply,
    (dump_func)0,
    (dump_func)0,
    (dump_func)0,
    (dump_func)0,
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_create_timer_reply,
    (dump_func)dump_open_timer_reply,
    (dump_func)dump_set_timer_reply,
    (dump_func)dump_cancel_timer_reply,
    (dump_func)dump_get_thread_context_reply,
    (dump_func)0,
    (dump_func)dump_get_selector_entry_reply,
    (dump_func)dump_add_atom_reply,
    (dump_func)0,
    (dump_func)dump_find_atom_reply,
    (dump_func)dump_get_atom_name_reply,
    (dump_func)0,
    (dump_func)dump_get_msg_queue_reply,
    (dump_func)dump_set_queue_mask_reply,
    (dump_func)dump_get_queue_status_reply,
    (dump_func)dump_wait_input_idle_reply,
    (dump_func)0,
    (dump_func)dump_get_message_reply,
    (dump_func)0,
    (dump_func)dump_get_message_reply_reply,
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_create_serial_reply,
    (dump_func)dump_get_serial_info_reply,
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_create_named_pipe_reply,
    (dump_func)dump_open_named_pipe_reply,
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_disconnect_named_pipe_reply,
    (dump_func)dump_get_named_pipe_info_reply,
    (dump_func)dump_create_smb_reply,
    (dump_func)dump_get_smb_info_reply,
    (dump_func)dump_create_window_reply,
    (dump_func)dump_link_window_reply,
    (dump_func)0,
    (dump_func)dump_set_window_owner_reply,
    (dump_func)dump_get_window_info_reply,
    (dump_func)dump_set_window_info_reply,
    (dump_func)dump_get_window_parents_reply,
    (dump_func)dump_get_window_children_reply,
    (dump_func)dump_get_window_tree_reply,
    (dump_func)0,
    (dump_func)dump_get_window_rectangles_reply,
    (dump_func)dump_get_window_text_reply,
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_get_windows_offset_reply,
    (dump_func)0,
    (dump_func)dump_remove_window_property_reply,
    (dump_func)dump_get_window_property_reply,
    (dump_func)dump_get_window_properties_reply,
    (dump_func)0,
    (dump_func)dump_get_thread_input_reply,
    (dump_func)dump_get_key_state_reply,
    (dump_func)0,
    (dump_func)dump_set_foreground_window_reply,
    (dump_func)dump_set_focus_window_reply,
    (dump_func)dump_set_active_window_reply,
    (dump_func)dump_set_capture_window_reply,
    (dump_func)dump_set_caret_window_reply,
    (dump_func)dump_set_caret_info_reply,
    (dump_func)dump_set_hook_reply,
    (dump_func)0,
    (dump_func)dump_start_hook_chain_reply,
    (dump_func)0,
    (dump_func)dump_get_next_hook_reply,
};

static const char * const req_names[REQ_NB_REQUESTS] = {
    "new_process",
    "get_new_process_info",
    "new_thread",
    "boot_done",
    "init_process",
    "get_startup_info",
    "init_process_done",
    "init_thread",
    "terminate_process",
    "terminate_thread",
    "get_process_info",
    "set_process_info",
    "get_thread_info",
    "set_thread_info",
    "get_dll_info",
    "suspend_thread",
    "resume_thread",
    "load_dll",
    "unload_dll",
    "queue_apc",
    "get_apc",
    "close_handle",
    "set_handle_info",
    "dup_handle",
    "open_process",
    "open_thread",
    "select",
    "create_event",
    "event_op",
    "open_event",
    "create_mutex",
    "release_mutex",
    "open_mutex",
    "create_semaphore",
    "release_semaphore",
    "open_semaphore",
    "create_file",
    "alloc_file_handle",
    "get_handle_fd",
    "set_file_pointer",
    "truncate_file",
    "set_file_time",
    "flush_file",
    "get_file_info",
    "lock_file",
    "unlock_file",
    "create_pipe",
    "create_socket",
    "accept_socket",
    "set_socket_event",
    "get_socket_event",
    "enable_socket_event",
    "set_socket_deferred",
    "alloc_console",
    "free_console",
    "get_console_renderer_events",
    "open_console",
    "get_console_mode",
    "set_console_mode",
    "set_console_input_info",
    "get_console_input_info",
    "append_console_input_history",
    "get_console_input_history",
    "create_console_output",
    "set_console_output_info",
    "get_console_output_info",
    "write_console_input",
    "read_console_input",
    "write_console_output",
    "fill_console_output",
    "read_console_output",
    "move_console_output",
    "send_console_signal",
    "create_change_notification",
    "next_change_notification",
    "create_mapping",
    "open_mapping",
    "get_mapping_info",
    "create_device",
    "get_device_id",
    "create_snapshot",
    "next_process",
    "next_thread",
    "next_module",
    "wait_debug_event",
    "queue_exception_event",
    "get_exception_status",
    "output_debug_string",
    "continue_debug_event",
    "debug_process",
    "debug_break",
    "set_debugger_kill_on_exit",
    "read_process_memory",
    "write_process_memory",
    "create_key",
    "open_key",
    "delete_key",
    "enum_key",
    "set_key_value",
    "get_key_value",
    "enum_key_value",
    "delete_key_value",
    "load_registry",
    "save_registry",
    "save_registry_atexit",
    "set_registry_levels",
    "set_registry_notification",
    "create_timer",
    "open_timer",
    "set_timer",
    "cancel_timer",
    "get_thread_context",
    "set_thread_context",
    "get_selector_entry",
    "add_atom",
    "delete_atom",
    "find_atom",
    "get_atom_name",
    "init_atom_table",
    "get_msg_queue",
    "set_queue_mask",
    "get_queue_status",
    "wait_input_idle",
    "send_message",
    "get_message",
    "reply_message",
    "get_message_reply",
    "set_win_timer",
    "kill_win_timer",
    "create_serial",
    "get_serial_info",
    "set_serial_info",
    "register_async",
    "create_named_pipe",
    "open_named_pipe",
    "connect_named_pipe",
    "wait_named_pipe",
    "disconnect_named_pipe",
    "get_named_pipe_info",
    "create_smb",
    "get_smb_info",
    "create_window",
    "link_window",
    "destroy_window",
    "set_window_owner",
    "get_window_info",
    "set_window_info",
    "get_window_parents",
    "get_window_children",
    "get_window_tree",
    "set_window_rectangles",
    "get_window_rectangles",
    "get_window_text",
    "set_window_text",
    "inc_window_paint_count",
    "get_windows_offset",
    "set_window_property",
    "remove_window_property",
    "get_window_property",
    "get_window_properties",
    "attach_thread_input",
    "get_thread_input",
    "get_key_state",
    "set_key_state",
    "set_foreground_window",
    "set_focus_window",
    "set_active_window",
    "set_capture_window",
    "set_caret_window",
    "set_caret_info",
    "set_hook",
    "remove_hook",
    "start_hook_chain",
    "finish_hook_chain",
    "get_next_hook",
};

/* ### make_requests end ### */
/* Everything above this line is generated automatically by tools/make_requests */

static const char *get_status_name( unsigned int status )
{
#define NAME(status)  { #status, STATUS_##status }
    static const struct
    {
        const char  *name;
        unsigned int value;
    } status_names[] =
    {
        NAME(ACCESS_DENIED),
        NAME(ACCESS_VIOLATION),
        NAME(BUFFER_OVERFLOW),
        NAME(CHILD_MUST_BE_VOLATILE),
        NAME(DIRECTORY_NOT_EMPTY),
        NAME(DISK_FULL),
        NAME(FILE_LOCK_CONFLICT),
        NAME(INVALID_FILE_FOR_SECTION),
        NAME(INVALID_HANDLE),
        NAME(INVALID_PARAMETER),
        NAME(KEY_DELETED),
        NAME(MEDIA_WRITE_PROTECTED),
        NAME(MUTANT_NOT_OWNED),
        NAME(NOT_REGISTRY_FILE),
        NAME(NO_MEMORY),
        NAME(NO_MORE_ENTRIES),
        NAME(NO_MORE_FILES),
        NAME(NO_SUCH_FILE),
        NAME(OBJECT_NAME_COLLISION),
        NAME(OBJECT_NAME_INVALID),
        NAME(OBJECT_NAME_NOT_FOUND),
        NAME(OBJECT_PATH_INVALID),
        NAME(OBJECT_TYPE_MISMATCH),
        NAME(PENDING),
        NAME(PIPE_BROKEN),
        NAME(SEMAPHORE_LIMIT_EXCEEDED),
        NAME(SHARING_VIOLATION),
        NAME(SUSPEND_COUNT_EXCEEDED),
        NAME(TIMEOUT),
        NAME(TOO_MANY_OPENED_FILES),
        NAME(UNSUCCESSFUL),
        NAME(USER_APC),
        { NULL, 0 }  /* terminator */
    };
#undef NAME

    int i;
    static char buffer[10];

    if (status)
    {
        for (i = 0; status_names[i].name; i++)
            if (status_names[i].value == status) return status_names[i].name;
    }
    sprintf( buffer, "%x", status );
    return buffer;
}

void trace_request(void)
{
    enum request req = current->req.request_header.req;
    if (req < REQ_NB_REQUESTS)
    {
        fprintf( stderr, "%04x: %s(", current->id, req_names[req] );
        if (req_dumpers[req])
        {
            cur_pos = 0;
            cur_data = get_req_data();
            cur_size = get_req_data_size();
            req_dumpers[req]( &current->req );
        }
        fprintf( stderr, " )\n" );
    }
    else fprintf( stderr, "%04x: %d(?)\n", current->id, req );
}

void trace_reply( enum request req, const union generic_reply *reply )
{
    if (req < REQ_NB_REQUESTS)
    {
        fprintf( stderr, "%04x: %s() = %s",
                 current->id, req_names[req], get_status_name(current->error) );
        if (reply_dumpers[req])
        {
            fprintf( stderr, " {" );
            cur_pos = 0;
            cur_data = current->reply_data;
            cur_size = reply->reply_header.reply_size;
            reply_dumpers[req]( reply );
            fprintf( stderr, " }" );
        }
        fputc( '\n', stderr );
    }
    else fprintf( stderr, "%04x: %d() = %s\n",
                  current->id, req, get_status_name(current->error) );
}
