/*
 * Server request tracing
 *
 * Copyright (C) 1999 Alexandre Julliard
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include "request.h"


/* dumping for functions for requests that have a variable part */

static void dump_varargs_select( struct select_request *req )
{
    int i;
    for (i = 0; i < req->count; i++)
        fprintf( stderr, "%c%d", i ? ',' : '{', req->handles[i] );
    fprintf( stderr, "}" );
}

static void dump_varargs_get_apcs( struct get_apcs_request *req )
{
    int i;
    for (i = 0; i < 2 * req->count; i++)
        fprintf( stderr, "%c%p", i ? ',' : '{', req->apcs[i] );
    fprintf( stderr, "}" );
}


typedef void (*dump_func)( const void *req );

/* Everything below this line is generated automatically by tools/make_requests */
/* ### make_requests begin ### */

static void dump_new_process_request( struct new_process_request *req )
{
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " inherit_all=%d,", req->inherit_all );
    fprintf( stderr, " create_flags=%d,", req->create_flags );
    fprintf( stderr, " start_flags=%d,", req->start_flags );
    fprintf( stderr, " hstdin=%d,", req->hstdin );
    fprintf( stderr, " hstdout=%d,", req->hstdout );
    fprintf( stderr, " hstderr=%d,", req->hstderr );
    fprintf( stderr, " cmd_show=%d,", req->cmd_show );
    fprintf( stderr, " env_ptr=%p,", req->env_ptr );
    fprintf( stderr, " cmdline=\"%s\"", req->cmdline );
}

static void dump_new_process_reply( struct new_process_request *req )
{
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_new_thread_request( struct new_thread_request *req )
{
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " suspend=%d,", req->suspend );
    fprintf( stderr, " inherit=%d", req->inherit );
}

static void dump_new_thread_reply( struct new_thread_request *req )
{
    fprintf( stderr, " tid=%p,", req->tid );
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_set_debug_request( struct set_debug_request *req )
{
    fprintf( stderr, " level=%d", req->level );
}

static void dump_init_process_request( struct init_process_request *req )
{
}

static void dump_init_process_reply( struct init_process_request *req )
{
    fprintf( stderr, " start_flags=%d,", req->start_flags );
    fprintf( stderr, " hstdin=%d,", req->hstdin );
    fprintf( stderr, " hstdout=%d,", req->hstdout );
    fprintf( stderr, " hstderr=%d,", req->hstderr );
    fprintf( stderr, " cmd_show=%d,", req->cmd_show );
    fprintf( stderr, " env_ptr=%p,", req->env_ptr );
    fprintf( stderr, " cmdline=\"%s\"", req->cmdline );
}

static void dump_init_thread_request( struct init_thread_request *req )
{
    fprintf( stderr, " unix_pid=%d,", req->unix_pid );
    fprintf( stderr, " teb=%p", req->teb );
}

static void dump_init_thread_reply( struct init_thread_request *req )
{
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " tid=%p", req->tid );
}

static void dump_get_thread_buffer_request( struct get_thread_buffer_request *req )
{
    fprintf( stderr, " dummy=%d", req->dummy );
}

static void dump_terminate_process_request( struct terminate_process_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " exit_code=%d", req->exit_code );
}

static void dump_terminate_thread_request( struct terminate_thread_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " exit_code=%d", req->exit_code );
}

static void dump_get_process_info_request( struct get_process_info_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_process_info_reply( struct get_process_info_request *req )
{
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " exit_code=%d,", req->exit_code );
    fprintf( stderr, " priority=%d,", req->priority );
    fprintf( stderr, " process_affinity=%d,", req->process_affinity );
    fprintf( stderr, " system_affinity=%d", req->system_affinity );
}

static void dump_set_process_info_request( struct set_process_info_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " mask=%d,", req->mask );
    fprintf( stderr, " priority=%d,", req->priority );
    fprintf( stderr, " affinity=%d", req->affinity );
}

static void dump_get_thread_info_request( struct get_thread_info_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_thread_info_reply( struct get_thread_info_request *req )
{
    fprintf( stderr, " tid=%p,", req->tid );
    fprintf( stderr, " exit_code=%d,", req->exit_code );
    fprintf( stderr, " priority=%d", req->priority );
}

static void dump_set_thread_info_request( struct set_thread_info_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " mask=%d,", req->mask );
    fprintf( stderr, " priority=%d,", req->priority );
    fprintf( stderr, " affinity=%d", req->affinity );
}

static void dump_suspend_thread_request( struct suspend_thread_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_suspend_thread_reply( struct suspend_thread_request *req )
{
    fprintf( stderr, " count=%d", req->count );
}

static void dump_resume_thread_request( struct resume_thread_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_resume_thread_reply( struct resume_thread_request *req )
{
    fprintf( stderr, " count=%d", req->count );
}

static void dump_debugger_request( struct debugger_request *req )
{
    fprintf( stderr, " op=%d", req->op );
}

static void dump_queue_apc_request( struct queue_apc_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " func=%p,", req->func );
    fprintf( stderr, " param=%p", req->param );
}

static void dump_get_apcs_request( struct get_apcs_request *req )
{
}

static void dump_get_apcs_reply( struct get_apcs_request *req )
{
    fprintf( stderr, " count=%d,", req->count );
    fprintf( stderr, " apcs=" );
    dump_varargs_get_apcs( req );
}

static void dump_close_handle_request( struct close_handle_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_handle_info_request( struct get_handle_info_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_handle_info_reply( struct get_handle_info_request *req )
{
    fprintf( stderr, " flags=%d", req->flags );
}

static void dump_set_handle_info_request( struct set_handle_info_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " flags=%d,", req->flags );
    fprintf( stderr, " mask=%d", req->mask );
}

static void dump_dup_handle_request( struct dup_handle_request *req )
{
    fprintf( stderr, " src_process=%d,", req->src_process );
    fprintf( stderr, " src_handle=%d,", req->src_handle );
    fprintf( stderr, " dst_process=%d,", req->dst_process );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " options=%d", req->options );
}

static void dump_dup_handle_reply( struct dup_handle_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_open_process_request( struct open_process_request *req )
{
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d", req->inherit );
}

static void dump_open_process_reply( struct open_process_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_select_request( struct select_request *req )
{
    fprintf( stderr, " count=%d,", req->count );
    fprintf( stderr, " flags=%d,", req->flags );
    fprintf( stderr, " timeout=%d,", req->timeout );
    fprintf( stderr, " handles=" );
    dump_varargs_select( req );
}

static void dump_select_reply( struct select_request *req )
{
    fprintf( stderr, " signaled=%d", req->signaled );
}

static void dump_create_event_request( struct create_event_request *req )
{
    fprintf( stderr, " manual_reset=%d,", req->manual_reset );
    fprintf( stderr, " initial_state=%d,", req->initial_state );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=\"%s\"", req->name );
}

static void dump_create_event_reply( struct create_event_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_event_op_request( struct event_op_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " op=%d", req->op );
}

static void dump_open_event_request( struct open_event_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=\"%s\"", req->name );
}

static void dump_open_event_reply( struct open_event_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_create_mutex_request( struct create_mutex_request *req )
{
    fprintf( stderr, " owned=%d,", req->owned );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=\"%s\"", req->name );
}

static void dump_create_mutex_reply( struct create_mutex_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_release_mutex_request( struct release_mutex_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_open_mutex_request( struct open_mutex_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=\"%s\"", req->name );
}

static void dump_open_mutex_reply( struct open_mutex_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_create_semaphore_request( struct create_semaphore_request *req )
{
    fprintf( stderr, " initial=%08x,", req->initial );
    fprintf( stderr, " max=%08x,", req->max );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=\"%s\"", req->name );
}

static void dump_create_semaphore_reply( struct create_semaphore_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_release_semaphore_request( struct release_semaphore_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " count=%08x", req->count );
}

static void dump_release_semaphore_reply( struct release_semaphore_request *req )
{
    fprintf( stderr, " prev_count=%08x", req->prev_count );
}

static void dump_open_semaphore_request( struct open_semaphore_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=\"%s\"", req->name );
}

static void dump_open_semaphore_reply( struct open_semaphore_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_create_file_request( struct create_file_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " sharing=%08x,", req->sharing );
    fprintf( stderr, " create=%d,", req->create );
    fprintf( stderr, " attrs=%08x,", req->attrs );
    fprintf( stderr, " name=\"%s\"", req->name );
}

static void dump_create_file_reply( struct create_file_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_alloc_file_handle_request( struct alloc_file_handle_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
}

static void dump_alloc_file_handle_reply( struct alloc_file_handle_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_read_fd_request( struct get_read_fd_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_write_fd_request( struct get_write_fd_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_set_file_pointer_request( struct set_file_pointer_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " low=%d,", req->low );
    fprintf( stderr, " high=%d,", req->high );
    fprintf( stderr, " whence=%d", req->whence );
}

static void dump_set_file_pointer_reply( struct set_file_pointer_request *req )
{
    fprintf( stderr, " new_low=%d,", req->new_low );
    fprintf( stderr, " new_high=%d", req->new_high );
}

static void dump_truncate_file_request( struct truncate_file_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_set_file_time_request( struct set_file_time_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " access_time=%ld,", req->access_time );
    fprintf( stderr, " write_time=%ld", req->write_time );
}

static void dump_flush_file_request( struct flush_file_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_file_info_request( struct get_file_info_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_file_info_reply( struct get_file_info_request *req )
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

static void dump_lock_file_request( struct lock_file_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " offset_low=%08x,", req->offset_low );
    fprintf( stderr, " offset_high=%08x,", req->offset_high );
    fprintf( stderr, " count_low=%08x,", req->count_low );
    fprintf( stderr, " count_high=%08x", req->count_high );
}

static void dump_unlock_file_request( struct unlock_file_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " offset_low=%08x,", req->offset_low );
    fprintf( stderr, " offset_high=%08x,", req->offset_high );
    fprintf( stderr, " count_low=%08x,", req->count_low );
    fprintf( stderr, " count_high=%08x", req->count_high );
}

static void dump_create_pipe_request( struct create_pipe_request *req )
{
    fprintf( stderr, " inherit=%d", req->inherit );
}

static void dump_create_pipe_reply( struct create_pipe_request *req )
{
    fprintf( stderr, " handle_read=%d,", req->handle_read );
    fprintf( stderr, " handle_write=%d", req->handle_write );
}

static void dump_alloc_console_request( struct alloc_console_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d", req->inherit );
}

static void dump_alloc_console_reply( struct alloc_console_request *req )
{
    fprintf( stderr, " handle_in=%d,", req->handle_in );
    fprintf( stderr, " handle_out=%d", req->handle_out );
}

static void dump_free_console_request( struct free_console_request *req )
{
    fprintf( stderr, " dummy=%d", req->dummy );
}

static void dump_open_console_request( struct open_console_request *req )
{
    fprintf( stderr, " output=%d,", req->output );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d", req->inherit );
}

static void dump_open_console_reply( struct open_console_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_set_console_fd_request( struct set_console_fd_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " file_handle=%d,", req->file_handle );
    fprintf( stderr, " pid=%d", req->pid );
}

static void dump_get_console_mode_request( struct get_console_mode_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_console_mode_reply( struct get_console_mode_request *req )
{
    fprintf( stderr, " mode=%d", req->mode );
}

static void dump_set_console_mode_request( struct set_console_mode_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " mode=%d", req->mode );
}

static void dump_set_console_info_request( struct set_console_info_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " mask=%d,", req->mask );
    fprintf( stderr, " cursor_size=%d,", req->cursor_size );
    fprintf( stderr, " cursor_visible=%d,", req->cursor_visible );
    fprintf( stderr, " title=\"%s\"", req->title );
}

static void dump_get_console_info_request( struct get_console_info_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_console_info_reply( struct get_console_info_request *req )
{
    fprintf( stderr, " cursor_size=%d,", req->cursor_size );
    fprintf( stderr, " cursor_visible=%d,", req->cursor_visible );
    fprintf( stderr, " pid=%d,", req->pid );
    fprintf( stderr, " title=\"%s\"", req->title );
}

static void dump_write_console_input_request( struct write_console_input_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " count=%d", req->count );
}

static void dump_write_console_input_reply( struct write_console_input_request *req )
{
    fprintf( stderr, " written=%d", req->written );
}

static void dump_read_console_input_request( struct read_console_input_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " count=%d,", req->count );
    fprintf( stderr, " flush=%d", req->flush );
}

static void dump_read_console_input_reply( struct read_console_input_request *req )
{
    fprintf( stderr, " read=%d", req->read );
}

static void dump_create_change_notification_request( struct create_change_notification_request *req )
{
    fprintf( stderr, " subtree=%d,", req->subtree );
    fprintf( stderr, " filter=%d", req->filter );
}

static void dump_create_change_notification_reply( struct create_change_notification_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_create_mapping_request( struct create_mapping_request *req )
{
    fprintf( stderr, " size_high=%d,", req->size_high );
    fprintf( stderr, " size_low=%d,", req->size_low );
    fprintf( stderr, " protect=%d,", req->protect );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " file_handle=%d,", req->file_handle );
    fprintf( stderr, " name=\"%s\"", req->name );
}

static void dump_create_mapping_reply( struct create_mapping_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_open_mapping_request( struct open_mapping_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=\"%s\"", req->name );
}

static void dump_open_mapping_reply( struct open_mapping_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_mapping_info_request( struct get_mapping_info_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_get_mapping_info_reply( struct get_mapping_info_request *req )
{
    fprintf( stderr, " size_high=%d,", req->size_high );
    fprintf( stderr, " size_low=%d,", req->size_low );
    fprintf( stderr, " protect=%d", req->protect );
}

static void dump_create_device_request( struct create_device_request *req )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " id=%d", req->id );
}

static void dump_create_device_reply( struct create_device_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_create_snapshot_request( struct create_snapshot_request *req )
{
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " flags=%d", req->flags );
}

static void dump_create_snapshot_reply( struct create_snapshot_request *req )
{
    fprintf( stderr, " handle=%d", req->handle );
}

static void dump_next_process_request( struct next_process_request *req )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " reset=%d", req->reset );
}

static void dump_next_process_reply( struct next_process_request *req )
{
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " threads=%d,", req->threads );
    fprintf( stderr, " priority=%d", req->priority );
}

static void dump_wait_debug_event_request( struct wait_debug_event_request *req )
{
    fprintf( stderr, " timeout=%d", req->timeout );
}

static void dump_wait_debug_event_reply( struct wait_debug_event_request *req )
{
    fprintf( stderr, " code=%d,", req->code );
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " tid=%p", req->tid );
}

static void dump_send_debug_event_request( struct send_debug_event_request *req )
{
    fprintf( stderr, " code=%d", req->code );
}

static void dump_send_debug_event_reply( struct send_debug_event_request *req )
{
    fprintf( stderr, " status=%d", req->status );
}

static void dump_continue_debug_event_request( struct continue_debug_event_request *req )
{
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " tid=%p,", req->tid );
    fprintf( stderr, " status=%d", req->status );
}

static void dump_debug_process_request( struct debug_process_request *req )
{
    fprintf( stderr, " pid=%p", req->pid );
}

static const dump_func req_dumpers[REQ_NB_REQUESTS] = {
    (dump_func)dump_new_process_request,
    (dump_func)dump_new_thread_request,
    (dump_func)dump_set_debug_request,
    (dump_func)dump_init_process_request,
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
    (dump_func)dump_debugger_request,
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
    (dump_func)dump_wait_debug_event_request,
    (dump_func)dump_send_debug_event_request,
    (dump_func)dump_continue_debug_event_request,
    (dump_func)dump_debug_process_request,
};

static const dump_func reply_dumpers[REQ_NB_REQUESTS] = {
    (dump_func)dump_new_process_reply,
    (dump_func)dump_new_thread_reply,
    (dump_func)0,
    (dump_func)dump_init_process_reply,
    (dump_func)dump_init_thread_reply,
    (dump_func)0,
    (dump_func)0,
    (dump_func)0,
    (dump_func)dump_get_process_info_reply,
    (dump_func)0,
    (dump_func)dump_get_thread_info_reply,
    (dump_func)0,
    (dump_func)dump_suspend_thread_reply,
    (dump_func)dump_resume_thread_reply,
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
    (dump_func)dump_wait_debug_event_reply,
    (dump_func)dump_send_debug_event_reply,
    (dump_func)0,
    (dump_func)0,
};

static const char * const req_names[REQ_NB_REQUESTS] = {
    "new_process",
    "new_thread",
    "set_debug",
    "init_process",
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
    "debugger",
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
    "wait_debug_event",
    "send_debug_event",
    "continue_debug_event",
    "debug_process",
};

/* ### make_requests end ### */
/* Everything above this line is generated automatically by tools/make_requests */

void trace_request( enum request req, int fd )
{
    current->last_req = req;
    if (req < REQ_NB_REQUESTS)
    {
        fprintf( stderr, "%08x: %s(", (unsigned int)current, req_names[req] );
        req_dumpers[req]( current->buffer );
    }
    else
        fprintf( stderr, "%08x: %d(", (unsigned int)current, req );
    if (fd != -1) fprintf( stderr, " ) fd=%d\n", fd );
    else fprintf( stderr, " )\n" );
}

void trace_timeout(void)
{
    fprintf( stderr, "%08x: *timeout*\n", (unsigned int)current );
}

void trace_kill( int exit_code )
{
    fprintf( stderr,"%08x: *killed* exit_code=%d\n",
             (unsigned int)current, exit_code );
}

void trace_reply( struct thread *thread, unsigned int res, int pass_fd )
{
    fprintf( stderr, "%08x: %s() = %d",
             (unsigned int)thread, req_names[thread->last_req], res );
    if (reply_dumpers[thread->last_req])
    {
        fprintf( stderr, " {" );
        reply_dumpers[thread->last_req]( thread->buffer );
	fprintf( stderr, " }" );
    }
    if (pass_fd != -1) fprintf( stderr, " fd=%d\n", pass_fd );
    else fprintf( stderr, "\n" );
}
