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

/* max request length */
#define MAX_REQUEST_LENGTH  8192

/* exit code passed to remove_client on communication error */
#define OUT_OF_MEMORY  -1
#define BROKEN_PIPE    -2
#define PROTOCOL_ERROR -3

/* request handler definition */
#define DECL_HANDLER(name) void req_##name( struct name##_request *req, int fd )

/* request functions */

extern void fatal_protocol_error( struct thread *thread, const char *err, ... );
extern void call_req_handler( struct thread *thread, enum request req, int fd );
extern void call_timeout_handler( void *thread );
extern void call_kill_handler( struct thread *thread, int exit_code );
extern void set_reply_fd( struct thread *thread, int pass_fd );
extern void send_reply( struct thread *thread );

extern void trace_request( enum request req, int fd );
extern void trace_timeout(void);
extern void trace_kill( int exit_code );
extern void trace_reply( struct thread *thread, unsigned int res, int pass_fd );

/* get the request buffer */
static inline void *get_req_ptr( struct thread *thread )
{
    return thread->buffer;
}

/* get the remaining size in the request buffer for object of a given size */
static inline int get_req_size( const void *ptr, size_t typesize )
{
    return ((char *)current->buffer + MAX_REQUEST_LENGTH - (char *)ptr) / typesize;
}

/* get the length of a request string, without going past the end of the request */
static inline size_t get_req_strlen( const char *str )
{
    const char *p = str;
    while (*p && (p < (char *)current->buffer + MAX_REQUEST_LENGTH - 1)) p++;
    return p - str;
}

/* same as above for Unicode */
static inline size_t get_req_strlenW( const WCHAR *str )
{
    const WCHAR *p = str;
    while (*p && ((char *)p < (char *)current->buffer + MAX_REQUEST_LENGTH - 2)) p++;
    return p - str;
}

/* Everything below this line is generated automatically by tools/make_requests */
/* ### make_requests begin ### */

DECL_HANDLER(new_process);
DECL_HANDLER(new_thread);
DECL_HANDLER(set_debug);
DECL_HANDLER(init_process);
DECL_HANDLER(init_process_done);
DECL_HANDLER(init_thread);
DECL_HANDLER(get_thread_buffer);
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
DECL_HANDLER(get_apcs);
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
DECL_HANDLER(alloc_file_handle);
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
DECL_HANDLER(create_socket);
DECL_HANDLER(accept_socket);
DECL_HANDLER(set_socket_event);
DECL_HANDLER(get_socket_event);
DECL_HANDLER(enable_socket_event);
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
DECL_HANDLER(read_process_memory);
DECL_HANDLER(write_process_memory);
DECL_HANDLER(create_key);
DECL_HANDLER(open_key);
DECL_HANDLER(delete_key);
DECL_HANDLER(close_key);
DECL_HANDLER(enum_key);
DECL_HANDLER(query_key_info);
DECL_HANDLER(set_key_value);
DECL_HANDLER(get_key_value);
DECL_HANDLER(enum_key_value);
DECL_HANDLER(delete_key_value);
DECL_HANDLER(load_registry);
DECL_HANDLER(save_registry);
DECL_HANDLER(set_registry_levels);

#ifdef WANT_REQUEST_HANDLERS

static const struct handler {
    void       (*handler)( void *req, int fd );
    unsigned int min_size;
} req_handlers[REQ_NB_REQUESTS] = {
    { (void(*)())req_new_process, sizeof(struct new_process_request) },
    { (void(*)())req_new_thread, sizeof(struct new_thread_request) },
    { (void(*)())req_set_debug, sizeof(struct set_debug_request) },
    { (void(*)())req_init_process, sizeof(struct init_process_request) },
    { (void(*)())req_init_process_done, sizeof(struct init_process_done_request) },
    { (void(*)())req_init_thread, sizeof(struct init_thread_request) },
    { (void(*)())req_get_thread_buffer, sizeof(struct get_thread_buffer_request) },
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
    { (void(*)())req_get_apcs, sizeof(struct get_apcs_request) },
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
    { (void(*)())req_alloc_file_handle, sizeof(struct alloc_file_handle_request) },
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
    { (void(*)())req_create_socket, sizeof(struct create_socket_request) },
    { (void(*)())req_accept_socket, sizeof(struct accept_socket_request) },
    { (void(*)())req_set_socket_event, sizeof(struct set_socket_event_request) },
    { (void(*)())req_get_socket_event, sizeof(struct get_socket_event_request) },
    { (void(*)())req_enable_socket_event, sizeof(struct enable_socket_event_request) },
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
    { (void(*)())req_read_process_memory, sizeof(struct read_process_memory_request) },
    { (void(*)())req_write_process_memory, sizeof(struct write_process_memory_request) },
    { (void(*)())req_create_key, sizeof(struct create_key_request) },
    { (void(*)())req_open_key, sizeof(struct open_key_request) },
    { (void(*)())req_delete_key, sizeof(struct delete_key_request) },
    { (void(*)())req_close_key, sizeof(struct close_key_request) },
    { (void(*)())req_enum_key, sizeof(struct enum_key_request) },
    { (void(*)())req_query_key_info, sizeof(struct query_key_info_request) },
    { (void(*)())req_set_key_value, sizeof(struct set_key_value_request) },
    { (void(*)())req_get_key_value, sizeof(struct get_key_value_request) },
    { (void(*)())req_enum_key_value, sizeof(struct enum_key_value_request) },
    { (void(*)())req_delete_key_value, sizeof(struct delete_key_value_request) },
    { (void(*)())req_load_registry, sizeof(struct load_registry_request) },
    { (void(*)())req_save_registry, sizeof(struct save_registry_request) },
    { (void(*)())req_set_registry_levels, sizeof(struct set_registry_levels_request) },
};
#endif  /* WANT_REQUEST_HANDLERS */

/* ### make_requests end ### */
/* Everything above this line is generated automatically by tools/make_requests */

#endif  /* __WINE_SERVER_REQUEST_H */
