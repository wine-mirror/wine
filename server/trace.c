/*
 * Server request tracing
 *
 * Copyright (C) 1999 Alexandre Julliard
 */

#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include "winsock2.h"
#include "winnt.h"
#include "request.h"
#include "unicode.h"


/* utility functions */

static void dump_ints( const int *ptr, int len )
{
    fputc( '{', stderr );
    while (len > 0)
    {
        fprintf( stderr, "%d", *ptr++ );
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
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

static void dump_bytes( const unsigned char *ptr, int len )
{
    fputc( '{', stderr );
    while (len > 0)
    {
        fprintf( stderr, "%02x", *ptr++ );
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
}

static void dump_string( const void *req, const char *str )
{
    int len = get_req_strlen( req, str );
    fprintf( stderr, "\"%.*s\"", len, str );
}

static void dump_unicode_string( const void *req, const WCHAR *str )
{
    size_t len = get_req_strlenW( req, str );
    fprintf( stderr, "L\"" );
    dump_strW( str, len, stderr, "\"\"" );
    fputc( '\"', stderr );
}

static void dump_path_t( const void *req, const path_t *path )
{
    dump_unicode_string( req, *path );
}

static void dump_context( const void *req, const CONTEXT *context )
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

static void dump_exc_record( const void *req, const EXCEPTION_RECORD *rec )
{
    int i;
    fprintf( stderr, "{code=%lx,flags=%lx,rec=%p,addr=%p,params={",
             rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionRecord,
             rec->ExceptionAddress );
    for (i = 0; i < rec->NumberParameters; i++)
    {
        if (i) fputc( ',', stderr );
        fprintf( stderr, "%lx", rec->ExceptionInformation[i] );
    }
    fputc( '}', stderr );
}

static void dump_debug_event_t( const void *req, const debug_event_t *event )
{
    switch(event->code)
    {
    case EXCEPTION_DEBUG_EVENT:
        fprintf( stderr, "{exception," );
        dump_exc_record( req, &event->info.exception.record );
        fprintf( stderr, ",first=%d}", event->info.exception.first );
        break;
    case CREATE_THREAD_DEBUG_EVENT:
        fprintf( stderr, "{create_thread,thread=%d,teb=%p,start=%p}",
                 event->info.create_thread.handle, event->info.create_thread.teb,
                 event->info.create_thread.start );
        break;
    case CREATE_PROCESS_DEBUG_EVENT:
        fprintf( stderr, "{create_process,file=%d,process=%d,thread=%d,base=%p,offset=%d,"
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
        fprintf( stderr, "{load_dll,file=%d,base=%p,offset=%d,size=%d,name=%p,unicode=%d}",
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
}


/* dumping for functions for requests that have a variable part */

static void dump_varargs_select_request( const struct select_request *req )
{
    int count = min( req->count, get_req_size( req, req->handles, sizeof(int) ));
    dump_ints( req->handles, count );
}

static void dump_varargs_get_apcs_reply( const struct get_apcs_request *req )
{
    int i;
    for (i = 0; i < 2 * req->count; i++)
        fprintf( stderr, "%c%p", i ? ',' : '{', req->apcs[i] );
    fprintf( stderr, "}" );
}

static void dump_varargs_get_socket_event_reply( const struct get_socket_event_request *req )
{
    dump_ints( req->errors, FD_MAX_EVENTS );
}

static void dump_varargs_read_process_memory_reply( const struct read_process_memory_request *req )
{
    int count = min( req->len, get_req_size( req, req->data, sizeof(int) ));
    dump_bytes( (unsigned char *)req->data, count * sizeof(int) );
}

static void dump_varargs_write_process_memory_request( const struct write_process_memory_request *req )
{
    int count = min( req->len, get_req_size( req, req->data, sizeof(int) ));
    dump_bytes( (unsigned char *)req->data, count * sizeof(int) );
}

static void dump_varargs_set_key_value_request( const struct set_key_value_request *req )
{
    int count = min( req->len, get_req_size( req, req->data, 1 ));
    dump_bytes( req->data, count );
}

static void dump_varargs_get_key_value_reply( const struct get_key_value_request *req )
{
    int count = min( req->len, get_req_size( req, req->data, 1 ));
    dump_bytes( req->data, count );
}

static void dump_varargs_enum_key_value_reply( const struct enum_key_value_request *req )
{
    int count = min( req->len, get_req_size( req, req->data, 1 ));
    dump_bytes( req->data, count );
}

typedef void (*dump_func)( const void *req );

/* Everything below this line is generated automatically by tools/make_requests */
/* ### make_requests begin ### */

static void dump_new_process_request( const struct new_process_request *req )
{
    fprintf( stderr, " inherit_all=%d,", req->inherit_all );
    fprintf( stderr, " create_flags=%d,", req->create_flags );
    fprintf( stderr, " start_flags=%d,", req->start_flags );
    fprintf( stderr, " exe_file=%d,", req->exe_file );
    fprintf( stderr, " hstdin=%d,", req->hstdin );
    fprintf( stderr, " hstdout=%d,", req->hstdout );
    fprintf( stderr, " hstderr=%d,", req->hstderr );
    fprintf( stderr, " cmd_show=%d,", req->cmd_show );
    fprintf( stderr, " alloc_fd=%d,", req->alloc_fd );
    fprintf( stderr, " filename=" );
    dump_string( req, req->filename );
}

static void dump_wait_process_request( const struct wait_process_request *req )
{
    fprintf( stderr, " pinherit=%d,", req->pinherit );
    fprintf( stderr, " tinherit=%d,", req->tinherit );
    fprintf( stderr, " timeout=%d,", req->timeout );
    fprintf( stderr, " cancel=%d", req->cancel );
}

static void dump_wait_process_reply( const struct wait_process_request *req )
{
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " phandle=%d,", req->phandle );
    fprintf( stderr, " tid=%p,", req->tid );
    fprintf( stderr, " thandle=%d,", req->thandle );
    fprintf( stderr, " event=%d", req->event );
}

static void dump_new_thread_request( const struct new_thread_request *req )
{
    fprintf( stderr, " suspend=%d,", req->suspend );
    fprintf( stderr, " inherit=%d", req->inherit );
}

static void dump_new_thread_reply( const struct new_thread_request *req )
{
    fprintf( stderr, " tid=%p,", req->tid );
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_boot_done_request( const struct boot_done_request *req )
{
    fprintf( stderr, " debug_level=%d", req->debug_level );
}

static void dump_init_process_request( const struct init_process_request *req )
{
    fprintf( stderr, " ldt_copy=%p,", req->ldt_copy );
    fprintf( stderr, " ldt_flags=%p,", req->ldt_flags );
    fprintf( stderr, " ppid=%d", req->ppid );
}

static void dump_init_process_reply( const struct init_process_request *req )
{
    fprintf( stderr, " start_flags=%d,", req->start_flags );
    fprintf( stderr, " exe_file=%d,", req->exe_file );
    fprintf( stderr, " hstdin=%d,", req->hstdin );
    fprintf( stderr, " hstdout=%d,", req->hstdout );
    fprintf( stderr, " hstderr=%d,", req->hstderr );
    fprintf( stderr, " cmd_show=%d,", req->cmd_show );
    fprintf( stderr, " filename=" );
    dump_string( req, req->filename );
}

static void dump_init_process_done_request( const struct init_process_done_request *req )
{
    fprintf( stderr, " module=%p,", req->module );
    fprintf( stderr, " entry=%p", req->entry );
}

static void dump_init_process_done_reply( const struct init_process_done_request *req )
{
    fprintf( stderr, " debugged=%d", req->debugged );
}

static void dump_init_thread_request( const struct init_thread_request *req )
{
    fprintf( stderr, " unix_pid=%d,", req->unix_pid );
    fprintf( stderr, " teb=%p,", req->teb );
    fprintf( stderr, " entry=%p", req->entry );
}

static void dump_get_thread_buffer_request( const struct get_thread_buffer_request *req )
{
}

static void dump_get_thread_buffer_reply( const struct get_thread_buffer_request *req )
{
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " tid=%p,", req->tid );
    fprintf( stderr, " boot=%d,", req->boot );
    fprintf( stderr, " version=%d", req->version );
}

static void dump_terminate_process_request( const struct terminate_process_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " exit_code=%d", req->exit_code );
}

static void dump_terminate_process_reply( const struct terminate_process_request *req )
{
    fprintf( stderr, " self=%d", req->self );
}

static void dump_terminate_thread_request( const struct terminate_thread_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " exit_code=%d", req->exit_code );
}

static void dump_terminate_thread_reply( const struct terminate_thread_request *req )
{
    fprintf( stderr, " self=%d,", req->self );
    fprintf( stderr, " last=%d", req->last );
}

static void dump_get_process_info_request( const struct get_process_info_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_process_info_reply( const struct get_process_info_request *req )
{
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " debugged=%d,", req->debugged );
    fprintf( stderr, " exit_code=%d,", req->exit_code );
    fprintf( stderr, " priority=%d,", req->priority );
    fprintf( stderr, " process_affinity=%d,", req->process_affinity );
    fprintf( stderr, " system_affinity=%d", req->system_affinity );
}

static void dump_set_process_info_request( const struct set_process_info_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " mask=%d,", req->mask );
    fprintf( stderr, " priority=%d,", req->priority );
    fprintf( stderr, " affinity=%d", req->affinity );
}

static void dump_get_thread_info_request( const struct get_thread_info_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " tid_in=%p", req->tid_in );
}

static void dump_get_thread_info_reply( const struct get_thread_info_request *req )
{
    fprintf( stderr, " tid=%p,", req->tid );
    fprintf( stderr, " teb=%p,", req->teb );
    fprintf( stderr, " exit_code=%d,", req->exit_code );
    fprintf( stderr, " priority=%d", req->priority );
}

static void dump_set_thread_info_request( const struct set_thread_info_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " mask=%d,", req->mask );
    fprintf( stderr, " priority=%d,", req->priority );
    fprintf( stderr, " affinity=%d", req->affinity );
}

static void dump_suspend_thread_request( const struct suspend_thread_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_suspend_thread_reply( const struct suspend_thread_request *req )
{
    fprintf( stderr, " count=%d", req->count );
}

static void dump_resume_thread_request( const struct resume_thread_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_resume_thread_reply( const struct resume_thread_request *req )
{
    fprintf( stderr, " count=%d", req->count );
}

static void dump_load_dll_request( const struct load_dll_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " base=%p,", req->base );
    fprintf( stderr, " dbg_offset=%d,", req->dbg_offset );
    fprintf( stderr, " dbg_size=%d,", req->dbg_size );
    fprintf( stderr, " name=%p", req->name );
}

static void dump_unload_dll_request( const struct unload_dll_request *req )
{
    fprintf( stderr, " base=%p", req->base );
}

static void dump_queue_apc_request( const struct queue_apc_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " func=%p,", req->func );
    fprintf( stderr, " param=%p", req->param );
}

static void dump_get_apcs_request( const struct get_apcs_request *req )
{
}

static void dump_get_apcs_reply( const struct get_apcs_request *req )
{
    fprintf( stderr, " count=%d,", req->count );
    fprintf( stderr, " apcs=" );
    dump_varargs_get_apcs_reply( req );
}

static void dump_close_handle_request( const struct close_handle_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_handle_info_request( const struct get_handle_info_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_handle_info_reply( const struct get_handle_info_request *req )
{
    fprintf( stderr, " flags=%d", req->flags );
}

static void dump_set_handle_info_request( const struct set_handle_info_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " flags=%d,", req->flags );
    fprintf( stderr, " mask=%d", req->mask );
}

static void dump_dup_handle_request( const struct dup_handle_request *req )
{
    fprintf( stderr, " src_process=%d,", req->src_process );
    fprintf( stderr, " src_handle=%d,", req->src_handle );
    fprintf( stderr, " dst_process=%d,", req->dst_process );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " options=%d", req->options );
}

static void dump_dup_handle_reply( const struct dup_handle_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_open_process_request( const struct open_process_request *req )
{
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d", req->inherit );
}

static void dump_open_process_reply( const struct open_process_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_select_request( const struct select_request *req )
{
    fprintf( stderr, " count=%d,", req->count );
    fprintf( stderr, " flags=%d,", req->flags );
    fprintf( stderr, " timeout=%d,", req->timeout );
    fprintf( stderr, " handles=" );
    dump_varargs_select_request( req );
}

static void dump_select_reply( const struct select_request *req )
{
    fprintf( stderr, " signaled=%d", req->signaled );
}

static void dump_create_event_request( const struct create_event_request *req )
{
    fprintf( stderr, " manual_reset=%d,", req->manual_reset );
    fprintf( stderr, " initial_state=%d,", req->initial_state );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=" );
    dump_unicode_string( req, req->name );
}

static void dump_create_event_reply( const struct create_event_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_event_op_request( const struct event_op_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " op=%d", req->op );
}

static void dump_open_event_request( const struct open_event_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=" );
    dump_unicode_string( req, req->name );
}

static void dump_open_event_reply( const struct open_event_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_create_mutex_request( const struct create_mutex_request *req )
{
    fprintf( stderr, " owned=%d,", req->owned );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=" );
    dump_unicode_string( req, req->name );
}

static void dump_create_mutex_reply( const struct create_mutex_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_release_mutex_request( const struct release_mutex_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_open_mutex_request( const struct open_mutex_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=" );
    dump_unicode_string( req, req->name );
}

static void dump_open_mutex_reply( const struct open_mutex_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_create_semaphore_request( const struct create_semaphore_request *req )
{
    fprintf( stderr, " initial=%08x,", req->initial );
    fprintf( stderr, " max=%08x,", req->max );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=" );
    dump_unicode_string( req, req->name );
}

static void dump_create_semaphore_reply( const struct create_semaphore_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_release_semaphore_request( const struct release_semaphore_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " count=%08x", req->count );
}

static void dump_release_semaphore_reply( const struct release_semaphore_request *req )
{
    fprintf( stderr, " prev_count=%08x", req->prev_count );
}

static void dump_open_semaphore_request( const struct open_semaphore_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=" );
    dump_unicode_string( req, req->name );
}

static void dump_open_semaphore_reply( const struct open_semaphore_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_create_file_request( const struct create_file_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " sharing=%08x,", req->sharing );
    fprintf( stderr, " create=%d,", req->create );
    fprintf( stderr, " attrs=%08x,", req->attrs );
    fprintf( stderr, " name=" );
    dump_string( req, req->name );
}

static void dump_create_file_reply( const struct create_file_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_alloc_file_handle_request( const struct alloc_file_handle_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
}

static void dump_alloc_file_handle_reply( const struct alloc_file_handle_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_read_fd_request( const struct get_read_fd_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_write_fd_request( const struct get_write_fd_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_set_file_pointer_request( const struct set_file_pointer_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " low=%d,", req->low );
    fprintf( stderr, " high=%d,", req->high );
    fprintf( stderr, " whence=%d", req->whence );
}

static void dump_set_file_pointer_reply( const struct set_file_pointer_request *req )
{
    fprintf( stderr, " new_low=%d,", req->new_low );
    fprintf( stderr, " new_high=%d", req->new_high );
}

static void dump_truncate_file_request( const struct truncate_file_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_set_file_time_request( const struct set_file_time_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " access_time=%ld,", req->access_time );
    fprintf( stderr, " write_time=%ld", req->write_time );
}

static void dump_flush_file_request( const struct flush_file_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_file_info_request( const struct get_file_info_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_file_info_reply( const struct get_file_info_request *req )
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
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " offset_low=%08x,", req->offset_low );
    fprintf( stderr, " offset_high=%08x,", req->offset_high );
    fprintf( stderr, " count_low=%08x,", req->count_low );
    fprintf( stderr, " count_high=%08x", req->count_high );
}

static void dump_unlock_file_request( const struct unlock_file_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " offset_low=%08x,", req->offset_low );
    fprintf( stderr, " offset_high=%08x,", req->offset_high );
    fprintf( stderr, " count_low=%08x,", req->count_low );
    fprintf( stderr, " count_high=%08x", req->count_high );
}

static void dump_create_pipe_request( const struct create_pipe_request *req )
{
    fprintf( stderr, " inherit=%d", req->inherit );
}

static void dump_create_pipe_reply( const struct create_pipe_request *req )
{
    fprintf( stderr, " handle_read=%d,", req->handle_read );
    fprintf( stderr, " handle_write=%d", req->handle_write );
}

static void dump_create_socket_request( const struct create_socket_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " family=%d,", req->family );
    fprintf( stderr, " type=%d,", req->type );
    fprintf( stderr, " protocol=%d", req->protocol );
}

static void dump_create_socket_reply( const struct create_socket_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_accept_socket_request( const struct accept_socket_request *req )
{
    fprintf( stderr, " lhandle=%d,", req->lhandle );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d", req->inherit );
}

static void dump_accept_socket_reply( const struct accept_socket_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_set_socket_event_request( const struct set_socket_event_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " mask=%08x,", req->mask );
    fprintf( stderr, " event=%d", req->event );
}

static void dump_get_socket_event_request( const struct get_socket_event_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " service=%d,", req->service );
    fprintf( stderr, " s_event=%d,", req->s_event );
    fprintf( stderr, " c_event=%d", req->c_event );
}

static void dump_get_socket_event_reply( const struct get_socket_event_request *req )
{
    fprintf( stderr, " mask=%08x,", req->mask );
    fprintf( stderr, " pmask=%08x,", req->pmask );
    fprintf( stderr, " state=%08x,", req->state );
    fprintf( stderr, " errors=" );
    dump_varargs_get_socket_event_reply( req );
}

static void dump_enable_socket_event_request( const struct enable_socket_event_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " mask=%08x,", req->mask );
    fprintf( stderr, " sstate=%08x,", req->sstate );
    fprintf( stderr, " cstate=%08x", req->cstate );
}

static void dump_alloc_console_request( const struct alloc_console_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d", req->inherit );
}

static void dump_alloc_console_reply( const struct alloc_console_request *req )
{
    fprintf( stderr, " handle_in=%d,", req->handle_in );
    fprintf( stderr, " handle_out=%d", req->handle_out );
}

static void dump_free_console_request( const struct free_console_request *req )
{
    fprintf( stderr, " dummy=%d", req->dummy );
}

static void dump_open_console_request( const struct open_console_request *req )
{
    fprintf( stderr, " output=%d,", req->output );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d", req->inherit );
}

static void dump_open_console_reply( const struct open_console_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_set_console_fd_request( const struct set_console_fd_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " file_handle=%d,", req->file_handle );
    fprintf( stderr, " pid=%d", req->pid );
}

static void dump_get_console_mode_request( const struct get_console_mode_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_console_mode_reply( const struct get_console_mode_request *req )
{
    fprintf( stderr, " mode=%d", req->mode );
}

static void dump_set_console_mode_request( const struct set_console_mode_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " mode=%d", req->mode );
}

static void dump_set_console_info_request( const struct set_console_info_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " mask=%d,", req->mask );
    fprintf( stderr, " cursor_size=%d,", req->cursor_size );
    fprintf( stderr, " cursor_visible=%d,", req->cursor_visible );
    fprintf( stderr, " title=" );
    dump_string( req, req->title );
}

static void dump_get_console_info_request( const struct get_console_info_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_console_info_reply( const struct get_console_info_request *req )
{
    fprintf( stderr, " cursor_size=%d,", req->cursor_size );
    fprintf( stderr, " cursor_visible=%d,", req->cursor_visible );
    fprintf( stderr, " pid=%d,", req->pid );
    fprintf( stderr, " title=" );
    dump_string( req, req->title );
}

static void dump_write_console_input_request( const struct write_console_input_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " count=%d", req->count );
}

static void dump_write_console_input_reply( const struct write_console_input_request *req )
{
    fprintf( stderr, " written=%d", req->written );
}

static void dump_read_console_input_request( const struct read_console_input_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " count=%d,", req->count );
    fprintf( stderr, " flush=%d", req->flush );
}

static void dump_read_console_input_reply( const struct read_console_input_request *req )
{
    fprintf( stderr, " read=%d", req->read );
}

static void dump_create_change_notification_request( const struct create_change_notification_request *req )
{
    fprintf( stderr, " subtree=%d,", req->subtree );
    fprintf( stderr, " filter=%d", req->filter );
}

static void dump_create_change_notification_reply( const struct create_change_notification_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_create_mapping_request( const struct create_mapping_request *req )
{
    fprintf( stderr, " size_high=%d,", req->size_high );
    fprintf( stderr, " size_low=%d,", req->size_low );
    fprintf( stderr, " protect=%d,", req->protect );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " file_handle=%d,", req->file_handle );
    fprintf( stderr, " name=" );
    dump_unicode_string( req, req->name );
}

static void dump_create_mapping_reply( const struct create_mapping_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_open_mapping_request( const struct open_mapping_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=" );
    dump_unicode_string( req, req->name );
}

static void dump_open_mapping_reply( const struct open_mapping_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_mapping_info_request( const struct get_mapping_info_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_mapping_info_reply( const struct get_mapping_info_request *req )
{
    fprintf( stderr, " size_high=%d,", req->size_high );
    fprintf( stderr, " size_low=%d,", req->size_low );
    fprintf( stderr, " protect=%d", req->protect );
}

static void dump_create_device_request( const struct create_device_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " id=%d", req->id );
}

static void dump_create_device_reply( const struct create_device_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_create_snapshot_request( const struct create_snapshot_request *req )
{
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " flags=%d,", req->flags );
    fprintf( stderr, " pid=%p", req->pid );
}

static void dump_create_snapshot_reply( const struct create_snapshot_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_next_process_request( const struct next_process_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " reset=%d", req->reset );
}

static void dump_next_process_reply( const struct next_process_request *req )
{
    fprintf( stderr, " count=%d,", req->count );
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " threads=%d,", req->threads );
    fprintf( stderr, " priority=%d", req->priority );
}

static void dump_next_thread_request( const struct next_thread_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " reset=%d", req->reset );
}

static void dump_next_thread_reply( const struct next_thread_request *req )
{
    fprintf( stderr, " count=%d,", req->count );
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " tid=%p,", req->tid );
    fprintf( stderr, " base_pri=%d,", req->base_pri );
    fprintf( stderr, " delta_pri=%d", req->delta_pri );
}

static void dump_next_module_request( const struct next_module_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " reset=%d", req->reset );
}

static void dump_next_module_reply( const struct next_module_request *req )
{
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " base=%p", req->base );
}

static void dump_wait_debug_event_request( const struct wait_debug_event_request *req )
{
    fprintf( stderr, " timeout=%d", req->timeout );
}

static void dump_wait_debug_event_reply( const struct wait_debug_event_request *req )
{
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " tid=%p,", req->tid );
    fprintf( stderr, " event=" );
    dump_debug_event_t( req, &req->event );
}

static void dump_exception_event_request( const struct exception_event_request *req )
{
    fprintf( stderr, " record=" );
    dump_exc_record( req, &req->record );
    fprintf( stderr, "," );
    fprintf( stderr, " first=%d,", req->first );
    fprintf( stderr, " context=" );
    dump_context( req, &req->context );
}

static void dump_exception_event_reply( const struct exception_event_request *req )
{
    fprintf( stderr, " status=%d", req->status );
}

static void dump_output_debug_string_request( const struct output_debug_string_request *req )
{
    fprintf( stderr, " string=%p,", req->string );
    fprintf( stderr, " unicode=%d,", req->unicode );
    fprintf( stderr, " length=%d", req->length );
}

static void dump_continue_debug_event_request( const struct continue_debug_event_request *req )
{
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " tid=%p,", req->tid );
    fprintf( stderr, " status=%d", req->status );
}

static void dump_debug_process_request( const struct debug_process_request *req )
{
    fprintf( stderr, " pid=%p", req->pid );
}

static void dump_read_process_memory_request( const struct read_process_memory_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " addr=%p,", req->addr );
    fprintf( stderr, " len=%d", req->len );
}

static void dump_read_process_memory_reply( const struct read_process_memory_request *req )
{
    fprintf( stderr, " data=" );
    dump_varargs_read_process_memory_reply( req );
}

static void dump_write_process_memory_request( const struct write_process_memory_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " addr=%p,", req->addr );
    fprintf( stderr, " len=%d,", req->len );
    fprintf( stderr, " first_mask=%08x,", req->first_mask );
    fprintf( stderr, " last_mask=%08x,", req->last_mask );
    fprintf( stderr, " data=" );
    dump_varargs_write_process_memory_request( req );
}

static void dump_create_key_request( const struct create_key_request *req )
{
    fprintf( stderr, " parent=%d,", req->parent );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " options=%08x,", req->options );
    fprintf( stderr, " modif=%ld,", req->modif );
    fprintf( stderr, " name=" );
    dump_path_t( req, &req->name );
    fprintf( stderr, "," );
    fprintf( stderr, " class=" );
    dump_unicode_string( req, req->class );
}

static void dump_create_key_reply( const struct create_key_request *req )
{
    fprintf( stderr, " hkey=%d,", req->hkey );
    fprintf( stderr, " created=%d", req->created );
}

static void dump_open_key_request( const struct open_key_request *req )
{
    fprintf( stderr, " parent=%d,", req->parent );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " name=" );
    dump_path_t( req, &req->name );
}

static void dump_open_key_reply( const struct open_key_request *req )
{
    fprintf( stderr, " hkey=%d", req->hkey );
}

static void dump_delete_key_request( const struct delete_key_request *req )
{
    fprintf( stderr, " hkey=%d,", req->hkey );
    fprintf( stderr, " name=" );
    dump_path_t( req, &req->name );
}

static void dump_close_key_request( const struct close_key_request *req )
{
    fprintf( stderr, " hkey=%d", req->hkey );
}

static void dump_enum_key_request( const struct enum_key_request *req )
{
    fprintf( stderr, " hkey=%d,", req->hkey );
    fprintf( stderr, " index=%d", req->index );
}

static void dump_enum_key_reply( const struct enum_key_request *req )
{
    fprintf( stderr, " modif=%ld,", req->modif );
    fprintf( stderr, " name=" );
    dump_path_t( req, &req->name );
    fprintf( stderr, "," );
    fprintf( stderr, " class=" );
    dump_unicode_string( req, req->class );
}

static void dump_query_key_info_request( const struct query_key_info_request *req )
{
    fprintf( stderr, " hkey=%d", req->hkey );
}

static void dump_query_key_info_reply( const struct query_key_info_request *req )
{
    fprintf( stderr, " subkeys=%d,", req->subkeys );
    fprintf( stderr, " max_subkey=%d,", req->max_subkey );
    fprintf( stderr, " max_class=%d,", req->max_class );
    fprintf( stderr, " values=%d,", req->values );
    fprintf( stderr, " max_value=%d,", req->max_value );
    fprintf( stderr, " max_data=%d,", req->max_data );
    fprintf( stderr, " modif=%ld,", req->modif );
    fprintf( stderr, " name=" );
    dump_path_t( req, &req->name );
    fprintf( stderr, "," );
    fprintf( stderr, " class=" );
    dump_unicode_string( req, req->class );
}

static void dump_set_key_value_request( const struct set_key_value_request *req )
{
    fprintf( stderr, " hkey=%d,", req->hkey );
    fprintf( stderr, " type=%d,", req->type );
    fprintf( stderr, " len=%d,", req->len );
    fprintf( stderr, " name=" );
    dump_path_t( req, &req->name );
    fprintf( stderr, "," );
    fprintf( stderr, " data=" );
    dump_varargs_set_key_value_request( req );
}

static void dump_get_key_value_request( const struct get_key_value_request *req )
{
    fprintf( stderr, " hkey=%d,", req->hkey );
    fprintf( stderr, " name=" );
    dump_unicode_string( req, req->name );
}

static void dump_get_key_value_reply( const struct get_key_value_request *req )
{
    fprintf( stderr, " type=%d,", req->type );
    fprintf( stderr, " len=%d,", req->len );
    fprintf( stderr, " data=" );
    dump_varargs_get_key_value_reply( req );
}

static void dump_enum_key_value_request( const struct enum_key_value_request *req )
{
    fprintf( stderr, " hkey=%d,", req->hkey );
    fprintf( stderr, " index=%d", req->index );
}

static void dump_enum_key_value_reply( const struct enum_key_value_request *req )
{
    fprintf( stderr, " type=%d,", req->type );
    fprintf( stderr, " len=%d,", req->len );
    fprintf( stderr, " name=" );
    dump_path_t( req, &req->name );
    fprintf( stderr, "," );
    fprintf( stderr, " data=" );
    dump_varargs_enum_key_value_reply( req );
}

static void dump_delete_key_value_request( const struct delete_key_value_request *req )
{
    fprintf( stderr, " hkey=%d,", req->hkey );
    fprintf( stderr, " name=" );
    dump_path_t( req, &req->name );
}

static void dump_load_registry_request( const struct load_registry_request *req )
{
    fprintf( stderr, " hkey=%d,", req->hkey );
    fprintf( stderr, " file=%d,", req->file );
    fprintf( stderr, " name=" );
    dump_path_t( req, &req->name );
}

static void dump_save_registry_request( const struct save_registry_request *req )
{
    fprintf( stderr, " hkey=%d,", req->hkey );
    fprintf( stderr, " file=%d", req->file );
}

static void dump_save_registry_atexit_request( const struct save_registry_atexit_request *req )
{
    fprintf( stderr, " hkey=%d,", req->hkey );
    fprintf( stderr, " file=" );
    dump_string( req, req->file );
}

static void dump_set_registry_levels_request( const struct set_registry_levels_request *req )
{
    fprintf( stderr, " current=%d,", req->current );
    fprintf( stderr, " saving=%d,", req->saving );
    fprintf( stderr, " version=%d,", req->version );
    fprintf( stderr, " period=%d", req->period );
}

static void dump_create_timer_request( const struct create_timer_request *req )
{
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " manual=%d,", req->manual );
    fprintf( stderr, " name=" );
    dump_unicode_string( req, req->name );
}

static void dump_create_timer_reply( const struct create_timer_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_open_timer_request( const struct open_timer_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=" );
    dump_unicode_string( req, req->name );
}

static void dump_open_timer_reply( const struct open_timer_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_set_timer_request( const struct set_timer_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " sec=%d,", req->sec );
    fprintf( stderr, " usec=%d,", req->usec );
    fprintf( stderr, " period=%d,", req->period );
    fprintf( stderr, " callback=%p,", req->callback );
    fprintf( stderr, " arg=%p", req->arg );
}

static void dump_cancel_timer_request( const struct cancel_timer_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_thread_context_request( const struct get_thread_context_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " flags=%08x", req->flags );
}

static void dump_get_thread_context_reply( const struct get_thread_context_request *req )
{
    fprintf( stderr, " context=" );
    dump_context( req, &req->context );
}

static void dump_set_thread_context_request( const struct set_thread_context_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " flags=%08x,", req->flags );
    fprintf( stderr, " context=" );
    dump_context( req, &req->context );
}

static void dump_get_selector_entry_request( const struct get_selector_entry_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " entry=%d", req->entry );
}

static void dump_get_selector_entry_reply( const struct get_selector_entry_request *req )
{
    fprintf( stderr, " base=%08x,", req->base );
    fprintf( stderr, " limit=%08x,", req->limit );
    fprintf( stderr, " flags=%02x", req->flags );
}

static void dump_add_atom_request( const struct add_atom_request *req )
{
    fprintf( stderr, " name=" );
    dump_unicode_string( req, req->name );
}

static void dump_add_atom_reply( const struct add_atom_request *req )
{
    fprintf( stderr, " atom=%d", req->atom );
}

static void dump_delete_atom_request( const struct delete_atom_request *req )
{
    fprintf( stderr, " atom=%d", req->atom );
}

static void dump_find_atom_request( const struct find_atom_request *req )
{
    fprintf( stderr, " name=" );
    dump_unicode_string( req, req->name );
}

static void dump_find_atom_reply( const struct find_atom_request *req )
{
    fprintf( stderr, " atom=%d", req->atom );
}

static void dump_get_atom_name_request( const struct get_atom_name_request *req )
{
    fprintf( stderr, " atom=%d", req->atom );
}

static void dump_get_atom_name_reply( const struct get_atom_name_request *req )
{
    fprintf( stderr, " count=%d,", req->count );
    fprintf( stderr, " name=" );
    dump_unicode_string( req, req->name );
}

static const dump_func req_dumpers[REQ_NB_REQUESTS] = {
    (dump_func)dump_new_process_request,
    (dump_func)dump_wait_process_request,
    (dump_func)dump_new_thread_request,
    (dump_func)dump_boot_done_request,
    (dump_func)dump_init_process_request,
    (dump_func)dump_init_process_done_request,
    (dump_func)dump_init_thread_request,
    (dump_func)dump_get_thread_buffer_request,
    (dump_func)dump_terminate_process_request,
    (dump_func)dump_terminate_thread_request,
    (dump_func)dump_get_process_info_request,
    (dump_func)dump_set_process_info_request,
    (dump_func)dump_get_thread_info_request,
    (dump_func)dump_set_thread_info_request,
    (dump_func)dump_suspend_thread_request,
    (dump_func)dump_resume_thread_request,
    (dump_func)dump_load_dll_request,
    (dump_func)dump_unload_dll_request,
    (dump_func)dump_queue_apc_request,
    (dump_func)dump_get_apcs_request,
    (dump_func)dump_close_handle_request,
    (dump_func)dump_get_handle_info_request,
    (dump_func)dump_set_handle_info_request,
    (dump_func)dump_dup_handle_request,
    (dump_func)dump_open_process_request,
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
    (dump_func)dump_get_read_fd_request,
    (dump_func)dump_get_write_fd_request,
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
    (dump_func)dump_alloc_console_request,
    (dump_func)dump_free_console_request,
    (dump_func)dump_open_console_request,
    (dump_func)dump_set_console_fd_request,
    (dump_func)dump_get_console_mode_request,
    (dump_func)dump_set_console_mode_request,
    (dump_func)dump_set_console_info_request,
    (dump_func)dump_get_console_info_request,
    (dump_func)dump_write_console_input_request,
    (dump_func)dump_read_console_input_request,
    (dump_func)dump_create_change_notification_request,
    (dump_func)dump_create_mapping_request,
    (dump_func)dump_open_mapping_request,
    (dump_func)dump_get_mapping_info_request,
    (dump_func)dump_create_device_request,
    (dump_func)dump_create_snapshot_request,
    (dump_func)dump_next_process_request,
    (dump_func)dump_next_thread_request,
    (dump_func)dump_next_module_request,
    (dump_func)dump_wait_debug_event_request,
    (dump_func)dump_exception_event_request,
    (dump_func)dump_output_debug_string_request,
    (dump_func)dump_continue_debug_event_request,
    (dump_func)dump_debug_process_request,
    (dump_func)dump_read_process_memory_request,
    (dump_func)dump_write_process_memory_request,
    (dump_func)dump_create_key_request,
    (dump_func)dump_open_key_request,
    (dump_func)dump_delete_key_request,
    (dump_func)dump_close_key_request,
    (dump_func)dump_enum_key_request,
    (dump_func)dump_query_key_info_request,
    (dump_func)dump_set_key_value_request,
    (dump_func)dump_get_key_value_request,
    (dump_func)dump_enum_key_value_request,
    (dump_func)dump_delete_key_value_request,
    (dump_func)dump_load_registry_request,
    (dump_func)dump_save_registry_request,
    (dump_func)dump_save_registry_atexit_request,
    (dump_func)dump_set_registry_levels_request,
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
};

static const dump_func reply_dumpers[REQ_NB_REQUESTS] = {
    (dump_func)0,
    (dump_func)dump_wait_process_reply,
    (dump_func)dump_new_thread_reply,
    (dump_func)0,
    (dump_func)dump_init_process_reply,
    (dump_func)dump_init_process_done_reply,
    (dump_func)0,
    (dump_func)dump_get_thread_buffer_reply,
    (dump_func)dump_terminate_process_reply,
    (dump_func)dump_terminate_thread_reply,
    (dump_func)dump_get_process_info_reply,
    (dump_func)0,
    (dump_func)dump_get_thread_info_reply,
    (dump_func)0,
    (dump_func)dump_suspend_thread_reply,
    (dump_func)dump_resume_thread_reply,
    (dump_func)0,
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_get_apcs_reply,
    (dump_func)0,
    (dump_func)dump_get_handle_info_reply,
    (dump_func)0,
    (dump_func)dump_dup_handle_reply,
    (dump_func)dump_open_process_reply,
    (dump_func)dump_select_reply,
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
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_set_file_pointer_reply,
    (dump_func)0,
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_get_file_info_reply,
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_create_pipe_reply,
    (dump_func)dump_create_socket_reply,
    (dump_func)dump_accept_socket_reply,
    (dump_func)0,
    (dump_func)dump_get_socket_event_reply,
    (dump_func)0,
    (dump_func)dump_alloc_console_reply,
    (dump_func)0,
    (dump_func)dump_open_console_reply,
    (dump_func)0,
    (dump_func)dump_get_console_mode_reply,
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_get_console_info_reply,
    (dump_func)dump_write_console_input_reply,
    (dump_func)dump_read_console_input_reply,
    (dump_func)dump_create_change_notification_reply,
    (dump_func)dump_create_mapping_reply,
    (dump_func)dump_open_mapping_reply,
    (dump_func)dump_get_mapping_info_reply,
    (dump_func)dump_create_device_reply,
    (dump_func)dump_create_snapshot_reply,
    (dump_func)dump_next_process_reply,
    (dump_func)dump_next_thread_reply,
    (dump_func)dump_next_module_reply,
    (dump_func)dump_wait_debug_event_reply,
    (dump_func)dump_exception_event_reply,
    (dump_func)0,
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_read_process_memory_reply,
    (dump_func)0,
    (dump_func)dump_create_key_reply,
    (dump_func)dump_open_key_reply,
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_enum_key_reply,
    (dump_func)dump_query_key_info_reply,
    (dump_func)0,
    (dump_func)dump_get_key_value_reply,
    (dump_func)dump_enum_key_value_reply,
    (dump_func)0,
    (dump_func)0,
    (dump_func)0,
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_create_timer_reply,
    (dump_func)dump_open_timer_reply,
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_get_thread_context_reply,
    (dump_func)0,
    (dump_func)dump_get_selector_entry_reply,
    (dump_func)dump_add_atom_reply,
    (dump_func)0,
    (dump_func)dump_find_atom_reply,
    (dump_func)dump_get_atom_name_reply,
};

static const char * const req_names[REQ_NB_REQUESTS] = {
    "new_process",
    "wait_process",
    "new_thread",
    "boot_done",
    "init_process",
    "init_process_done",
    "init_thread",
    "get_thread_buffer",
    "terminate_process",
    "terminate_thread",
    "get_process_info",
    "set_process_info",
    "get_thread_info",
    "set_thread_info",
    "suspend_thread",
    "resume_thread",
    "load_dll",
    "unload_dll",
    "queue_apc",
    "get_apcs",
    "close_handle",
    "get_handle_info",
    "set_handle_info",
    "dup_handle",
    "open_process",
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
    "get_read_fd",
    "get_write_fd",
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
    "alloc_console",
    "free_console",
    "open_console",
    "set_console_fd",
    "get_console_mode",
    "set_console_mode",
    "set_console_info",
    "get_console_info",
    "write_console_input",
    "read_console_input",
    "create_change_notification",
    "create_mapping",
    "open_mapping",
    "get_mapping_info",
    "create_device",
    "create_snapshot",
    "next_process",
    "next_thread",
    "next_module",
    "wait_debug_event",
    "exception_event",
    "output_debug_string",
    "continue_debug_event",
    "debug_process",
    "read_process_memory",
    "write_process_memory",
    "create_key",
    "open_key",
    "delete_key",
    "close_key",
    "enum_key",
    "query_key_info",
    "set_key_value",
    "get_key_value",
    "enum_key_value",
    "delete_key_value",
    "load_registry",
    "save_registry",
    "save_registry_atexit",
    "set_registry_levels",
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
};

/* ### make_requests end ### */
/* Everything above this line is generated automatically by tools/make_requests */

void trace_request( enum request req )
{
    current->last_req = req;
    if (req < REQ_NB_REQUESTS)
    {
        fprintf( stderr, "%08x: %s(", (unsigned int)current, req_names[req] );
        req_dumpers[req]( current->buffer );
    }
    else
        fprintf( stderr, "%08x: %d(", (unsigned int)current, req );
    if (current->pass_fd != -1) fprintf( stderr, " ) fd=%d\n", current->pass_fd );
    else fprintf( stderr, " )\n" );
}

void trace_reply( struct thread *thread )
{
    fprintf( stderr, "%08x: %s() = %x",
             (unsigned int)thread, req_names[thread->last_req], thread->error );
    if (reply_dumpers[thread->last_req])
    {
        fprintf( stderr, " {" );
        reply_dumpers[thread->last_req]( thread->buffer );
	fprintf( stderr, " }" );
    }
    if (thread->pass_fd != -1) fprintf( stderr, " fd=%d\n", thread->pass_fd );
    else fprintf( stderr, "\n" );
}
