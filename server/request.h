/*
 * Wine server requests
 *
 * Copyright (C) 1999 Alexandre Julliard
 */

#ifndef __WINE_SERVER_REQUEST_H
#define __WINE_SERVER_REQUEST_H

#ifndef __WINE_SERVER__
#error This file can only be used in the Wine server
#endif

#include "thread.h"

/* request handler definition */

#define DECL_HANDLER(name) void req_##name( struct name##_request *req, int fd )

/* request functions */

extern void fatal_protocol_error( const char *err );
extern void call_req_handler( struct thread *thread, int fd );
extern void call_timeout_handler( void *thread );
extern void call_kill_handler( struct thread *thread, int exit_code );
extern void set_reply_fd( struct thread *thread, int pass_fd );
extern void send_reply( struct thread *thread );

extern void trace_request( enum request req, int fd );
extern void trace_timeout(void);
extern void trace_kill( int exit_code );
extern void trace_reply( struct thread *thread, int pass_fd );


/* Warning: the buffer is shared between request and reply,
 * so make sure you are finished using the request before starting
 * to add data for the reply.
 */

/* remove some data from the current request */
static inline void *get_req_data( size_t len )
{
    void *old = current->req_pos;
    current->req_pos = (char *)old + len;
    return old;
}

/* check that there is enough data available in the current request */
static inline int check_req_data( size_t len )
{
    return (char *)current->req_pos + len <= (char *)current->req_end;
}

/* get the length of a request string, without going past the end of the request */
static inline size_t get_req_strlen(void)
{
    char *p = current->req_pos;
    while (*p && (p < (char *)current->req_end - 1)) p++;
    return p - (char *)current->req_pos;
}

/* make space for some data in the current reply */
static inline void *push_reply_data( struct thread *thread, size_t len )
{
    void *old = thread->reply_pos;
    thread->reply_pos = (char *)old + len;
    return old;
}

/* add some data to the current reply */
static inline void add_reply_data( struct thread *thread, const void *data, size_t len )
{
    memcpy( push_reply_data( thread, len ), data, len );
}

/* Everything below this line is generated automatically by tools/make_requests */
/* ### make_requests begin ### */

DECL_HANDLER(new_process);
DECL_HANDLER(new_thread);
DECL_HANDLER(set_debug);
DECL_HANDLER(init_process);
DECL_HANDLER(init_thread);
DECL_HANDLER(terminate_process);
DECL_HANDLER(terminate_thread);
DECL_HANDLER(get_process_info);
DECL_HANDLER(set_process_info);
DECL_HANDLER(get_thread_info);
DECL_HANDLER(set_thread_info);
DECL_HANDLER(suspend_thread);
DECL_HANDLER(resume_thread);
DECL_HANDLER(debugger);
DECL_HANDLER(queue_apc);
DECL_HANDLER(close_handle);
DECL_HANDLER(get_handle_info);
DECL_HANDLER(set_handle_info);
DECL_HANDLER(dup_handle);
DECL_HANDLER(open_process);
DECL_HANDLER(select);
DECL_HANDLER(create_event);
DECL_HANDLER(event_op);
DECL_HANDLER(open_event);
DECL_HANDLER(create_mutex);
DECL_HANDLER(release_mutex);
DECL_HANDLER(open_mutex);
DECL_HANDLER(create_semaphore);
DECL_HANDLER(release_semaphore);
DECL_HANDLER(open_semaphore);
DECL_HANDLER(create_file);
DECL_HANDLER(get_read_fd);
DECL_HANDLER(get_write_fd);
DECL_HANDLER(set_file_pointer);
DECL_HANDLER(truncate_file);
DECL_HANDLER(set_file_time);
DECL_HANDLER(flush_file);
DECL_HANDLER(get_file_info);
DECL_HANDLER(lock_file);
DECL_HANDLER(unlock_file);
DECL_HANDLER(create_pipe);
DECL_HANDLER(alloc_console);
DECL_HANDLER(free_console);
DECL_HANDLER(open_console);
DECL_HANDLER(set_console_fd);
DECL_HANDLER(get_console_mode);
DECL_HANDLER(set_console_mode);
DECL_HANDLER(set_console_info);
DECL_HANDLER(get_console_info);
DECL_HANDLER(write_console_input);
DECL_HANDLER(read_console_input);
DECL_HANDLER(create_change_notification);
DECL_HANDLER(create_mapping);
DECL_HANDLER(open_mapping);
DECL_HANDLER(get_mapping_info);
DECL_HANDLER(create_device);
DECL_HANDLER(create_snapshot);
DECL_HANDLER(next_process);
DECL_HANDLER(wait_debug_event);
DECL_HANDLER(send_debug_event);
DECL_HANDLER(continue_debug_event);
DECL_HANDLER(debug_process);

#ifdef WANT_REQUEST_HANDLERS

static const struct handler {
    void       (*handler)( void *req, int fd );
    unsigned int min_size;
} req_handlers[REQ_NB_REQUESTS] = {
    { (void(*)())req_new_process, sizeof(struct new_process_request) },
    { (void(*)())req_new_thread, sizeof(struct new_thread_request) },
    { (void(*)())req_set_debug, sizeof(struct set_debug_request) },
    { (void(*)())req_init_process, sizeof(struct init_process_request) },
    { (void(*)())req_init_thread, sizeof(struct init_thread_request) },
    { (void(*)())req_terminate_process, sizeof(struct terminate_process_request) },
    { (void(*)())req_terminate_thread, sizeof(struct terminate_thread_request) },
    { (void(*)())req_get_process_info, sizeof(struct get_process_info_request) },
    { (void(*)())req_set_process_info, sizeof(struct set_process_info_request) },
    { (void(*)())req_get_thread_info, sizeof(struct get_thread_info_request) },
    { (void(*)())req_set_thread_info, sizeof(struct set_thread_info_request) },
    { (void(*)())req_suspend_thread, sizeof(struct suspend_thread_request) },
    { (void(*)())req_resume_thread, sizeof(struct resume_thread_request) },
    { (void(*)())req_debugger, sizeof(struct debugger_request) },
    { (void(*)())req_queue_apc, sizeof(struct queue_apc_request) },
    { (void(*)())req_close_handle, sizeof(struct close_handle_request) },
    { (void(*)())req_get_handle_info, sizeof(struct get_handle_info_request) },
    { (void(*)())req_set_handle_info, sizeof(struct set_handle_info_request) },
    { (void(*)())req_dup_handle, sizeof(struct dup_handle_request) },
    { (void(*)())req_open_process, sizeof(struct open_process_request) },
    { (void(*)())req_select, sizeof(struct select_request) },
    { (void(*)())req_create_event, sizeof(struct create_event_request) },
    { (void(*)())req_event_op, sizeof(struct event_op_request) },
    { (void(*)())req_open_event, sizeof(struct open_event_request) },
    { (void(*)())req_create_mutex, sizeof(struct create_mutex_request) },
    { (void(*)())req_release_mutex, sizeof(struct release_mutex_request) },
    { (void(*)())req_open_mutex, sizeof(struct open_mutex_request) },
    { (void(*)())req_create_semaphore, sizeof(struct create_semaphore_request) },
    { (void(*)())req_release_semaphore, sizeof(struct release_semaphore_request) },
    { (void(*)())req_open_semaphore, sizeof(struct open_semaphore_request) },
    { (void(*)())req_create_file, sizeof(struct create_file_request) },
    { (void(*)())req_get_read_fd, sizeof(struct get_read_fd_request) },
    { (void(*)())req_get_write_fd, sizeof(struct get_write_fd_request) },
    { (void(*)())req_set_file_pointer, sizeof(struct set_file_pointer_request) },
    { (void(*)())req_truncate_file, sizeof(struct truncate_file_request) },
    { (void(*)())req_set_file_time, sizeof(struct set_file_time_request) },
    { (void(*)())req_flush_file, sizeof(struct flush_file_request) },
    { (void(*)())req_get_file_info, sizeof(struct get_file_info_request) },
    { (void(*)())req_lock_file, sizeof(struct lock_file_request) },
    { (void(*)())req_unlock_file, sizeof(struct unlock_file_request) },
    { (void(*)())req_create_pipe, sizeof(struct create_pipe_request) },
    { (void(*)())req_alloc_console, sizeof(struct alloc_console_request) },
    { (void(*)())req_free_console, sizeof(struct free_console_request) },
    { (void(*)())req_open_console, sizeof(struct open_console_request) },
    { (void(*)())req_set_console_fd, sizeof(struct set_console_fd_request) },
    { (void(*)())req_get_console_mode, sizeof(struct get_console_mode_request) },
    { (void(*)())req_set_console_mode, sizeof(struct set_console_mode_request) },
    { (void(*)())req_set_console_info, sizeof(struct set_console_info_request) },
    { (void(*)())req_get_console_info, sizeof(struct get_console_info_request) },
    { (void(*)())req_write_console_input, sizeof(struct write_console_input_request) },
    { (void(*)())req_read_console_input, sizeof(struct read_console_input_request) },
    { (void(*)())req_create_change_notification, sizeof(struct create_change_notification_request) },
    { (void(*)())req_create_mapping, sizeof(struct create_mapping_request) },
    { (void(*)())req_open_mapping, sizeof(struct open_mapping_request) },
    { (void(*)())req_get_mapping_info, sizeof(struct get_mapping_info_request) },
    { (void(*)())req_create_device, sizeof(struct create_device_request) },
    { (void(*)())req_create_snapshot, sizeof(struct create_snapshot_request) },
    { (void(*)())req_next_process, sizeof(struct next_process_request) },
    { (void(*)())req_wait_debug_event, sizeof(struct wait_debug_event_request) },
    { (void(*)())req_send_debug_event, sizeof(struct send_debug_event_request) },
    { (void(*)())req_continue_debug_event, sizeof(struct continue_debug_event_request) },
    { (void(*)())req_debug_process, sizeof(struct debug_process_request) },
};
#endif  /* WANT_REQUEST_HANDLERS */

/* ### make_requests end ### */
/* Everything above this line is generated automatically by tools/make_requests */

#endif  /* __WINE_SERVER_REQUEST_H */
