/* File generated automatically by tools/make_requests; DO NOT EDIT!! */

#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include "server.h"
#include "server/thread.h"

static int dump_new_process_request( struct new_process_request *req, int len )
{
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " inherit_all=%d,", req->inherit_all );
    fprintf( stderr, " start_flags=%d,", req->start_flags );
    fprintf( stderr, " hstdin=%d,", req->hstdin );
    fprintf( stderr, " hstdout=%d,", req->hstdout );
    fprintf( stderr, " hstderr=%d,", req->hstderr );
    fprintf( stderr, " cmd_line=\"%.*s\"", len - (int)sizeof(*req), (char *)(req+1) );
    return len;
}

static int dump_new_process_reply( struct new_process_reply *req, int len )
{
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_new_thread_request( struct new_thread_request *req, int len )
{
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " suspend=%d,", req->suspend );
    fprintf( stderr, " inherit=%d", req->inherit );
    return (int)sizeof(*req);
}

static int dump_new_thread_reply( struct new_thread_reply *req, int len )
{
    fprintf( stderr, " tid=%p,", req->tid );
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_set_debug_request( struct set_debug_request *req, int len )
{
    fprintf( stderr, " level=%d", req->level );
    return (int)sizeof(*req);
}

static int dump_init_process_request( struct init_process_request *req, int len )
{
    fprintf( stderr, " dummy=%d", req->dummy );
    return (int)sizeof(*req);
}

static int dump_init_process_reply( struct init_process_reply *req, int len )
{
    fprintf( stderr, " start_flags=%d,", req->start_flags );
    fprintf( stderr, " hstdin=%d,", req->hstdin );
    fprintf( stderr, " hstdout=%d,", req->hstdout );
    fprintf( stderr, " hstderr=%d", req->hstderr );
    return (int)sizeof(*req);
}

static int dump_init_thread_request( struct init_thread_request *req, int len )
{
    fprintf( stderr, " unix_pid=%d", req->unix_pid );
    return (int)sizeof(*req);
}

static int dump_init_thread_reply( struct init_thread_reply *req, int len )
{
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " tid=%p", req->tid );
    return (int)sizeof(*req);
}

static int dump_terminate_process_request( struct terminate_process_request *req, int len )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " exit_code=%d", req->exit_code );
    return (int)sizeof(*req);
}

static int dump_terminate_thread_request( struct terminate_thread_request *req, int len )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " exit_code=%d", req->exit_code );
    return (int)sizeof(*req);
}

static int dump_get_process_info_request( struct get_process_info_request *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_get_process_info_reply( struct get_process_info_reply *req, int len )
{
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " exit_code=%d,", req->exit_code );
    fprintf( stderr, " priority=%d,", req->priority );
    fprintf( stderr, " process_affinity=%d,", req->process_affinity );
    fprintf( stderr, " system_affinity=%d", req->system_affinity );
    return (int)sizeof(*req);
}

static int dump_set_process_info_request( struct set_process_info_request *req, int len )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " mask=%d,", req->mask );
    fprintf( stderr, " priority=%d,", req->priority );
    fprintf( stderr, " affinity=%d", req->affinity );
    return (int)sizeof(*req);
}

static int dump_get_thread_info_request( struct get_thread_info_request *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_get_thread_info_reply( struct get_thread_info_reply *req, int len )
{
    fprintf( stderr, " tid=%p,", req->tid );
    fprintf( stderr, " exit_code=%d,", req->exit_code );
    fprintf( stderr, " priority=%d", req->priority );
    return (int)sizeof(*req);
}

static int dump_set_thread_info_request( struct set_thread_info_request *req, int len )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " mask=%d,", req->mask );
    fprintf( stderr, " priority=%d,", req->priority );
    fprintf( stderr, " affinity=%d", req->affinity );
    return (int)sizeof(*req);
}

static int dump_suspend_thread_request( struct suspend_thread_request *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_suspend_thread_reply( struct suspend_thread_reply *req, int len )
{
    fprintf( stderr, " count=%d", req->count );
    return (int)sizeof(*req);
}

static int dump_resume_thread_request( struct resume_thread_request *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_resume_thread_reply( struct resume_thread_reply *req, int len )
{
    fprintf( stderr, " count=%d", req->count );
    return (int)sizeof(*req);
}

static int dump_debugger_request( struct debugger_request *req, int len )
{
    fprintf( stderr, " op=%d", req->op );
    return (int)sizeof(*req);
}

static int dump_queue_apc_request( struct queue_apc_request *req, int len )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " func=%p,", req->func );
    fprintf( stderr, " param=%p", req->param );
    return (int)sizeof(*req);
}

static int dump_close_handle_request( struct close_handle_request *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_get_handle_info_request( struct get_handle_info_request *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_get_handle_info_reply( struct get_handle_info_reply *req, int len )
{
    fprintf( stderr, " flags=%d", req->flags );
    return (int)sizeof(*req);
}

static int dump_set_handle_info_request( struct set_handle_info_request *req, int len )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " flags=%d,", req->flags );
    fprintf( stderr, " mask=%d", req->mask );
    return (int)sizeof(*req);
}

static int dump_dup_handle_request( struct dup_handle_request *req, int len )
{
    fprintf( stderr, " src_process=%d,", req->src_process );
    fprintf( stderr, " src_handle=%d,", req->src_handle );
    fprintf( stderr, " dst_process=%d,", req->dst_process );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " options=%d", req->options );
    return (int)sizeof(*req);
}

static int dump_dup_handle_reply( struct dup_handle_reply *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_open_process_request( struct open_process_request *req, int len )
{
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d", req->inherit );
    return (int)sizeof(*req);
}

static int dump_open_process_reply( struct open_process_reply *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_select_request( struct select_request *req, int len )
{
    fprintf( stderr, " count=%d,", req->count );
    fprintf( stderr, " flags=%d,", req->flags );
    fprintf( stderr, " timeout=%d", req->timeout );
    return (int)sizeof(*req);
}

static int dump_select_reply( struct select_reply *req, int len )
{
    fprintf( stderr, " signaled=%d", req->signaled );
    return (int)sizeof(*req);
}

static int dump_create_event_request( struct create_event_request *req, int len )
{
    fprintf( stderr, " manual_reset=%d,", req->manual_reset );
    fprintf( stderr, " initial_state=%d,", req->initial_state );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=\"%.*s\"", len - (int)sizeof(*req), (char *)(req+1) );
    return len;
}

static int dump_create_event_reply( struct create_event_reply *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_event_op_request( struct event_op_request *req, int len )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " op=%d", req->op );
    return (int)sizeof(*req);
}

static int dump_create_mutex_request( struct create_mutex_request *req, int len )
{
    fprintf( stderr, " owned=%d,", req->owned );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=\"%.*s\"", len - (int)sizeof(*req), (char *)(req+1) );
    return len;
}

static int dump_create_mutex_reply( struct create_mutex_reply *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_release_mutex_request( struct release_mutex_request *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_create_semaphore_request( struct create_semaphore_request *req, int len )
{
    fprintf( stderr, " initial=%08x,", req->initial );
    fprintf( stderr, " max=%08x,", req->max );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=\"%.*s\"", len - (int)sizeof(*req), (char *)(req+1) );
    return len;
}

static int dump_create_semaphore_reply( struct create_semaphore_reply *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_release_semaphore_request( struct release_semaphore_request *req, int len )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " count=%08x", req->count );
    return (int)sizeof(*req);
}

static int dump_release_semaphore_reply( struct release_semaphore_reply *req, int len )
{
    fprintf( stderr, " prev_count=%08x", req->prev_count );
    return (int)sizeof(*req);
}

static int dump_open_named_obj_request( struct open_named_obj_request *req, int len )
{
    fprintf( stderr, " type=%d,", req->type );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " name=\"%.*s\"", len - (int)sizeof(*req), (char *)(req+1) );
    return len;
}

static int dump_open_named_obj_reply( struct open_named_obj_reply *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_create_file_request( struct create_file_request *req, int len )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " sharing=%08x,", req->sharing );
    fprintf( stderr, " create=%d,", req->create );
    fprintf( stderr, " attrs=%08x,", req->attrs );
    fprintf( stderr, " name=\"%.*s\"", len - (int)sizeof(*req), (char *)(req+1) );
    return len;
}

static int dump_create_file_reply( struct create_file_reply *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_get_read_fd_request( struct get_read_fd_request *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_get_write_fd_request( struct get_write_fd_request *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_set_file_pointer_request( struct set_file_pointer_request *req, int len )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " low=%d,", req->low );
    fprintf( stderr, " high=%d,", req->high );
    fprintf( stderr, " whence=%d", req->whence );
    return (int)sizeof(*req);
}

static int dump_set_file_pointer_reply( struct set_file_pointer_reply *req, int len )
{
    fprintf( stderr, " low=%d,", req->low );
    fprintf( stderr, " high=%d", req->high );
    return (int)sizeof(*req);
}

static int dump_truncate_file_request( struct truncate_file_request *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_set_file_time_request( struct set_file_time_request *req, int len )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " access_time=%ld,", req->access_time );
    fprintf( stderr, " write_time=%ld", req->write_time );
    return (int)sizeof(*req);
}

static int dump_flush_file_request( struct flush_file_request *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_get_file_info_request( struct get_file_info_request *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_get_file_info_reply( struct get_file_info_reply *req, int len )
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
    return (int)sizeof(*req);
}

static int dump_lock_file_request( struct lock_file_request *req, int len )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " offset_low=%08x,", req->offset_low );
    fprintf( stderr, " offset_high=%08x,", req->offset_high );
    fprintf( stderr, " count_low=%08x,", req->count_low );
    fprintf( stderr, " count_high=%08x", req->count_high );
    return (int)sizeof(*req);
}

static int dump_unlock_file_request( struct unlock_file_request *req, int len )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " offset_low=%08x,", req->offset_low );
    fprintf( stderr, " offset_high=%08x,", req->offset_high );
    fprintf( stderr, " count_low=%08x,", req->count_low );
    fprintf( stderr, " count_high=%08x", req->count_high );
    return (int)sizeof(*req);
}

static int dump_create_pipe_request( struct create_pipe_request *req, int len )
{
    fprintf( stderr, " inherit=%d", req->inherit );
    return (int)sizeof(*req);
}

static int dump_create_pipe_reply( struct create_pipe_reply *req, int len )
{
    fprintf( stderr, " handle_read=%d,", req->handle_read );
    fprintf( stderr, " handle_write=%d", req->handle_write );
    return (int)sizeof(*req);
}

static int dump_alloc_console_request( struct alloc_console_request *req, int len )
{
    return (int)sizeof(*req);
}

static int dump_free_console_request( struct free_console_request *req, int len )
{
    return (int)sizeof(*req);
}

static int dump_open_console_request( struct open_console_request *req, int len )
{
    fprintf( stderr, " output=%d,", req->output );
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d", req->inherit );
    return (int)sizeof(*req);
}

static int dump_open_console_reply( struct open_console_reply *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_set_console_fd_request( struct set_console_fd_request *req, int len )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " pid=%d", req->pid );
    return (int)sizeof(*req);
}

static int dump_get_console_mode_request( struct get_console_mode_request *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_get_console_mode_reply( struct get_console_mode_reply *req, int len )
{
    fprintf( stderr, " mode=%d", req->mode );
    return (int)sizeof(*req);
}

static int dump_set_console_mode_request( struct set_console_mode_request *req, int len )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " mode=%d", req->mode );
    return (int)sizeof(*req);
}

static int dump_set_console_info_request( struct set_console_info_request *req, int len )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " mask=%d,", req->mask );
    fprintf( stderr, " cursor_size=%d,", req->cursor_size );
    fprintf( stderr, " cursor_visible=%d,", req->cursor_visible );
    fprintf( stderr, " title=\"%.*s\"", len - (int)sizeof(*req), (char *)(req+1) );
    return len;
}

static int dump_get_console_info_request( struct get_console_info_request *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_get_console_info_reply( struct get_console_info_reply *req, int len )
{
    fprintf( stderr, " cursor_size=%d,", req->cursor_size );
    fprintf( stderr, " cursor_visible=%d,", req->cursor_visible );
    fprintf( stderr, " pid=%d", req->pid );
    return (int)sizeof(*req);
}

static int dump_write_console_input_request( struct write_console_input_request *req, int len )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " count=%d", req->count );
    return (int)sizeof(*req);
}

static int dump_write_console_input_reply( struct write_console_input_reply *req, int len )
{
    fprintf( stderr, " written=%d", req->written );
    return (int)sizeof(*req);
}

static int dump_read_console_input_request( struct read_console_input_request *req, int len )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " count=%d,", req->count );
    fprintf( stderr, " flush=%d", req->flush );
    return (int)sizeof(*req);
}

static int dump_read_console_input_reply( struct read_console_input_reply *req, int len )
{
    return (int)sizeof(*req);
}

static int dump_create_change_notification_request( struct create_change_notification_request *req, int len )
{
    fprintf( stderr, " subtree=%d,", req->subtree );
    fprintf( stderr, " filter=%d", req->filter );
    return (int)sizeof(*req);
}

static int dump_create_change_notification_reply( struct create_change_notification_reply *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_create_mapping_request( struct create_mapping_request *req, int len )
{
    fprintf( stderr, " size_high=%d,", req->size_high );
    fprintf( stderr, " size_low=%d,", req->size_low );
    fprintf( stderr, " protect=%d,", req->protect );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " name=\"%.*s\"", len - (int)sizeof(*req), (char *)(req+1) );
    return len;
}

static int dump_create_mapping_reply( struct create_mapping_reply *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_get_mapping_info_request( struct get_mapping_info_request *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_get_mapping_info_reply( struct get_mapping_info_reply *req, int len )
{
    fprintf( stderr, " size_high=%d,", req->size_high );
    fprintf( stderr, " size_low=%d,", req->size_low );
    fprintf( stderr, " protect=%d", req->protect );
    return (int)sizeof(*req);
}

static int dump_create_device_request( struct create_device_request *req, int len )
{
    fprintf( stderr, " access=%08x,", req->access );
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " id=%d", req->id );
    return (int)sizeof(*req);
}

static int dump_create_device_reply( struct create_device_reply *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_create_snapshot_request( struct create_snapshot_request *req, int len )
{
    fprintf( stderr, " inherit=%d,", req->inherit );
    fprintf( stderr, " flags=%d", req->flags );
    return (int)sizeof(*req);
}

static int dump_create_snapshot_reply( struct create_snapshot_reply *req, int len )
{
    fprintf( stderr, " handle=%d", req->handle );
    return (int)sizeof(*req);
}

static int dump_next_process_request( struct next_process_request *req, int len )
{
    fprintf( stderr, " handle=%d,", req->handle );
    fprintf( stderr, " reset=%d", req->reset );
    return (int)sizeof(*req);
}

static int dump_next_process_reply( struct next_process_reply *req, int len )
{
    fprintf( stderr, " pid=%p,", req->pid );
    fprintf( stderr, " threads=%d,", req->threads );
    fprintf( stderr, " priority=%d", req->priority );
    return (int)sizeof(*req);
}

struct dumper
{
    int (*dump_req)( void *data, int len );
    void (*dump_reply)( void *data );
};

static const struct dumper dumpers[REQ_NB_REQUESTS] =
{
    { (int(*)(void *,int))dump_new_process_request,
      (void(*)())dump_new_process_reply },
    { (int(*)(void *,int))dump_new_thread_request,
      (void(*)())dump_new_thread_reply },
    { (int(*)(void *,int))dump_set_debug_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_init_process_request,
      (void(*)())dump_init_process_reply },
    { (int(*)(void *,int))dump_init_thread_request,
      (void(*)())dump_init_thread_reply },
    { (int(*)(void *,int))dump_terminate_process_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_terminate_thread_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_get_process_info_request,
      (void(*)())dump_get_process_info_reply },
    { (int(*)(void *,int))dump_set_process_info_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_get_thread_info_request,
      (void(*)())dump_get_thread_info_reply },
    { (int(*)(void *,int))dump_set_thread_info_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_suspend_thread_request,
      (void(*)())dump_suspend_thread_reply },
    { (int(*)(void *,int))dump_resume_thread_request,
      (void(*)())dump_resume_thread_reply },
    { (int(*)(void *,int))dump_debugger_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_queue_apc_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_close_handle_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_get_handle_info_request,
      (void(*)())dump_get_handle_info_reply },
    { (int(*)(void *,int))dump_set_handle_info_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_dup_handle_request,
      (void(*)())dump_dup_handle_reply },
    { (int(*)(void *,int))dump_open_process_request,
      (void(*)())dump_open_process_reply },
    { (int(*)(void *,int))dump_select_request,
      (void(*)())dump_select_reply },
    { (int(*)(void *,int))dump_create_event_request,
      (void(*)())dump_create_event_reply },
    { (int(*)(void *,int))dump_event_op_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_create_mutex_request,
      (void(*)())dump_create_mutex_reply },
    { (int(*)(void *,int))dump_release_mutex_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_create_semaphore_request,
      (void(*)())dump_create_semaphore_reply },
    { (int(*)(void *,int))dump_release_semaphore_request,
      (void(*)())dump_release_semaphore_reply },
    { (int(*)(void *,int))dump_open_named_obj_request,
      (void(*)())dump_open_named_obj_reply },
    { (int(*)(void *,int))dump_create_file_request,
      (void(*)())dump_create_file_reply },
    { (int(*)(void *,int))dump_get_read_fd_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_get_write_fd_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_set_file_pointer_request,
      (void(*)())dump_set_file_pointer_reply },
    { (int(*)(void *,int))dump_truncate_file_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_set_file_time_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_flush_file_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_get_file_info_request,
      (void(*)())dump_get_file_info_reply },
    { (int(*)(void *,int))dump_lock_file_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_unlock_file_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_create_pipe_request,
      (void(*)())dump_create_pipe_reply },
    { (int(*)(void *,int))dump_alloc_console_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_free_console_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_open_console_request,
      (void(*)())dump_open_console_reply },
    { (int(*)(void *,int))dump_set_console_fd_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_get_console_mode_request,
      (void(*)())dump_get_console_mode_reply },
    { (int(*)(void *,int))dump_set_console_mode_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_set_console_info_request,
      (void(*)())0 },
    { (int(*)(void *,int))dump_get_console_info_request,
      (void(*)())dump_get_console_info_reply },
    { (int(*)(void *,int))dump_write_console_input_request,
      (void(*)())dump_write_console_input_reply },
    { (int(*)(void *,int))dump_read_console_input_request,
      (void(*)())dump_read_console_input_reply },
    { (int(*)(void *,int))dump_create_change_notification_request,
      (void(*)())dump_create_change_notification_reply },
    { (int(*)(void *,int))dump_create_mapping_request,
      (void(*)())dump_create_mapping_reply },
    { (int(*)(void *,int))dump_get_mapping_info_request,
      (void(*)())dump_get_mapping_info_reply },
    { (int(*)(void *,int))dump_create_device_request,
      (void(*)())dump_create_device_reply },
    { (int(*)(void *,int))dump_create_snapshot_request,
      (void(*)())dump_create_snapshot_reply },
    { (int(*)(void *,int))dump_next_process_request,
      (void(*)())dump_next_process_reply },
};

static const char * const req_names[REQ_NB_REQUESTS] =
{
    "new_process",
    "new_thread",
    "set_debug",
    "init_process",
    "init_thread",
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
    "close_handle",
    "get_handle_info",
    "set_handle_info",
    "dup_handle",
    "open_process",
    "select",
    "create_event",
    "event_op",
    "create_mutex",
    "release_mutex",
    "create_semaphore",
    "release_semaphore",
    "open_named_obj",
    "create_file",
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
    "get_mapping_info",
    "create_device",
    "create_snapshot",
    "next_process",
};

void trace_request( enum request req, void *data, int len, int fd )
{
    int size;
    current->last_req = req;
    fprintf( stderr, "%08x: %s(", (unsigned int)current, req_names[req] );
    size = dumpers[req].dump_req( data, len );
    if ((len -= size) > 0)
    {
        unsigned char *ptr = (unsigned char *)data + size;
        while (len--) fprintf( stderr, ", %02x", *ptr++ );
    }
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

void trace_reply( struct thread *thread, int type, int pass_fd,
                  struct iovec *vec, int veclen )
{
    if (!thread) return;
    fprintf( stderr, "%08x: %s() = %d",
             (unsigned int)thread, req_names[thread->last_req], type );
    if (veclen)
    {
	fprintf( stderr, " {" );
	if (dumpers[thread->last_req].dump_reply)
	{
	    dumpers[thread->last_req].dump_reply( vec->iov_base );
	    vec++;
	    veclen--;
	}
	for (; veclen; veclen--, vec++)
	{
	    unsigned char *ptr = vec->iov_base;
	    int len = vec->iov_len;
	    while (len--) fprintf( stderr, ", %02x", *ptr++ );
	}
	fprintf( stderr, " }" );
    }
    if (pass_fd != -1) fprintf( stderr, " fd=%d\n", pass_fd );
    else fprintf( stderr, "\n" );
}
