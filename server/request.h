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

/* request handler definition */
#define DECL_HANDLER(name) void req_##name( struct name##_request *req )

/* request functions */

#ifdef __GNUC__
extern void fatal_protocol_error( struct thread *thread,
                                  const char *err, ... ) __attribute__((format (printf,2,3)));
#else
extern void fatal_protocol_error( struct thread *thread, const char *err, ... );
#endif

extern void read_request( struct thread *thread );
extern int write_request( struct thread *thread );
extern void set_reply_fd( struct thread *thread, int pass_fd );
extern void send_reply( struct thread *thread );
extern void open_master_socket(void);
extern void close_master_socket(void);
extern void lock_master_socket( int locked );

extern void trace_request( enum request req );
extern void trace_reply( struct thread *thread );

/* get the request buffer */
static inline void *get_req_ptr( struct thread *thread )
{
    return thread->buffer;
}

/* get the remaining size in the request buffer for object of a given size */
static inline int get_req_size( const void *req, const void *ptr, size_t typesize )
{
    return ((char *)req + MAX_REQUEST_LENGTH - (char *)ptr) / typesize;
}

/* get the length of a request string, without going past the end of the request */
static inline size_t get_req_strlen( const void *req, const char *str )
{
    const char *p = str;
    while (*p && (p < (char *)req + MAX_REQUEST_LENGTH - 1)) p++;
    return p - str;
}

/* same as above for Unicode */
static inline size_t get_req_strlenW( const void *req, const WCHAR *str )
{
    const WCHAR *p = str;
    while (*p && (p < (WCHAR *)req + MAX_REQUEST_LENGTH/sizeof(WCHAR) - 1)) p++;
    return p - str;
}

/* Everything below this line is generated automatically by tools/make_requests */
/* ### make_requests begin ### */

DECL_HANDLER(new_process);
DECL_HANDLER(new_thread);
DECL_HANDLER(boot_done);
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
DECL_HANDLER(load_dll);
DECL_HANDLER(unload_dll);
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
DECL_HANDLER(exception_event);
DECL_HANDLER(output_debug_string);
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
DECL_HANDLER(save_registry_atexit);
DECL_HANDLER(set_registry_levels);
DECL_HANDLER(create_timer);
DECL_HANDLER(open_timer);
DECL_HANDLER(set_timer);
DECL_HANDLER(cancel_timer);
DECL_HANDLER(get_thread_context);
DECL_HANDLER(set_thread_context);
DECL_HANDLER(get_selector_entry);
DECL_HANDLER(add_atom);
DECL_HANDLER(delete_atom);
DECL_HANDLER(find_atom);
DECL_HANDLER(get_atom_name);

#ifdef WANT_REQUEST_HANDLERS

typedef void (*req_handler)( void *req );
static const req_handler req_handlers[REQ_NB_REQUESTS] =
{
    (req_handler)req_new_process,
    (req_handler)req_new_thread,
    (req_handler)req_boot_done,
    (req_handler)req_init_process,
    (req_handler)req_init_process_done,
    (req_handler)req_init_thread,
    (req_handler)req_get_thread_buffer,
    (req_handler)req_terminate_process,
    (req_handler)req_terminate_thread,
    (req_handler)req_get_process_info,
    (req_handler)req_set_process_info,
    (req_handler)req_get_thread_info,
    (req_handler)req_set_thread_info,
    (req_handler)req_suspend_thread,
    (req_handler)req_resume_thread,
    (req_handler)req_load_dll,
    (req_handler)req_unload_dll,
    (req_handler)req_queue_apc,
    (req_handler)req_get_apcs,
    (req_handler)req_close_handle,
    (req_handler)req_get_handle_info,
    (req_handler)req_set_handle_info,
    (req_handler)req_dup_handle,
    (req_handler)req_open_process,
    (req_handler)req_select,
    (req_handler)req_create_event,
    (req_handler)req_event_op,
    (req_handler)req_open_event,
    (req_handler)req_create_mutex,
    (req_handler)req_release_mutex,
    (req_handler)req_open_mutex,
    (req_handler)req_create_semaphore,
    (req_handler)req_release_semaphore,
    (req_handler)req_open_semaphore,
    (req_handler)req_create_file,
    (req_handler)req_alloc_file_handle,
    (req_handler)req_get_read_fd,
    (req_handler)req_get_write_fd,
    (req_handler)req_set_file_pointer,
    (req_handler)req_truncate_file,
    (req_handler)req_set_file_time,
    (req_handler)req_flush_file,
    (req_handler)req_get_file_info,
    (req_handler)req_lock_file,
    (req_handler)req_unlock_file,
    (req_handler)req_create_pipe,
    (req_handler)req_create_socket,
    (req_handler)req_accept_socket,
    (req_handler)req_set_socket_event,
    (req_handler)req_get_socket_event,
    (req_handler)req_enable_socket_event,
    (req_handler)req_alloc_console,
    (req_handler)req_free_console,
    (req_handler)req_open_console,
    (req_handler)req_set_console_fd,
    (req_handler)req_get_console_mode,
    (req_handler)req_set_console_mode,
    (req_handler)req_set_console_info,
    (req_handler)req_get_console_info,
    (req_handler)req_write_console_input,
    (req_handler)req_read_console_input,
    (req_handler)req_create_change_notification,
    (req_handler)req_create_mapping,
    (req_handler)req_open_mapping,
    (req_handler)req_get_mapping_info,
    (req_handler)req_create_device,
    (req_handler)req_create_snapshot,
    (req_handler)req_next_process,
    (req_handler)req_wait_debug_event,
    (req_handler)req_exception_event,
    (req_handler)req_output_debug_string,
    (req_handler)req_continue_debug_event,
    (req_handler)req_debug_process,
    (req_handler)req_read_process_memory,
    (req_handler)req_write_process_memory,
    (req_handler)req_create_key,
    (req_handler)req_open_key,
    (req_handler)req_delete_key,
    (req_handler)req_close_key,
    (req_handler)req_enum_key,
    (req_handler)req_query_key_info,
    (req_handler)req_set_key_value,
    (req_handler)req_get_key_value,
    (req_handler)req_enum_key_value,
    (req_handler)req_delete_key_value,
    (req_handler)req_load_registry,
    (req_handler)req_save_registry,
    (req_handler)req_save_registry_atexit,
    (req_handler)req_set_registry_levels,
    (req_handler)req_create_timer,
    (req_handler)req_open_timer,
    (req_handler)req_set_timer,
    (req_handler)req_cancel_timer,
    (req_handler)req_get_thread_context,
    (req_handler)req_set_thread_context,
    (req_handler)req_get_selector_entry,
    (req_handler)req_add_atom,
    (req_handler)req_delete_atom,
    (req_handler)req_find_atom,
    (req_handler)req_get_atom_name,
};
#endif  /* WANT_REQUEST_HANDLERS */

/* ### make_requests end ### */
/* Everything above this line is generated automatically by tools/make_requests */

#endif  /* __WINE_SERVER_REQUEST_H */
