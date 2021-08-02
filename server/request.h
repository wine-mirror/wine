/*
 * Wine server requests
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_SERVER_REQUEST_H
#define __WINE_SERVER_REQUEST_H

#include <assert.h>

#include "thread.h"
#include "wine/server_protocol.h"

/* max request length */
#define MAX_REQUEST_LENGTH  8192

/* request handler definition */
#define DECL_HANDLER(name) \
    void req_##name( const struct name##_request *req, struct name##_reply *reply )

/* request functions */

#ifdef __GNUC__
extern void fatal_protocol_error( struct thread *thread,
                                  const char *err, ... ) __attribute__((format (printf,2,3)));
extern void fatal_error( const char *err, ... )  __attribute__((noreturn,format(printf,1,2)));
#else
extern void fatal_protocol_error( struct thread *thread, const char *err, ... );
extern void fatal_error( const char *err, ... );
#endif

extern const char *get_config_dir(void);
extern void *set_reply_data_size( data_size_t size );
extern const struct object_attributes *get_req_object_attributes( const struct security_descriptor **sd,
                                                                  struct unicode_str *name,
                                                                  struct object **root );
extern const void *get_req_data_after_objattr( const struct object_attributes *attr, data_size_t *len );
extern int receive_fd( struct process *process );
extern int send_client_fd( struct process *process, int fd, obj_handle_t handle );
extern void read_request( struct thread *thread );
extern void write_reply( struct thread *thread );
extern timeout_t monotonic_counter(void);
extern void open_master_socket(void);
extern void close_master_socket( timeout_t timeout );
extern void shutdown_master_socket(void);
extern int wait_for_lock(void);
extern int kill_lock_owner( int sig );
extern char *server_dir;
extern int server_dir_fd, config_dir_fd;

extern void trace_request(void);
extern void trace_reply( enum request req, const union generic_reply *reply );

/* get current tick count to return to client */
static inline unsigned int get_tick_count(void)
{
    return monotonic_counter() / 10000;
}

/* get the request vararg data */
static inline const void *get_req_data(void)
{
    return current->req_data;
}

/* get the request vararg size */
static inline data_size_t get_req_data_size(void)
{
    return current->req.request_header.request_size;
}

/* get the request vararg as unicode string */
static inline struct unicode_str get_req_unicode_str(void)
{
    struct unicode_str ret;
    ret.str = get_req_data();
    ret.len = (get_req_data_size() / sizeof(WCHAR)) * sizeof(WCHAR);
    return ret;
}

/* get the reply maximum vararg size */
static inline data_size_t get_reply_max_size(void)
{
    return current->req.request_header.reply_size;
}

/* allocate and fill the reply data */
static inline void *set_reply_data( const void *data, data_size_t size )
{
    void *ret = set_reply_data_size( size );
    if (ret) memcpy( ret, data, size );
    return ret;
}

/* set the reply data pointer directly (will be freed by request code) */
static inline void set_reply_data_ptr( void *data, data_size_t size )
{
    assert( size <= get_reply_max_size() );
    current->reply_size = size;
    current->reply_data = data;
}


/* Everything below this line is generated automatically by tools/make_requests */
/* ### make_requests begin ### */

DECL_HANDLER(new_process);
DECL_HANDLER(get_new_process_info);
DECL_HANDLER(new_thread);
DECL_HANDLER(get_startup_info);
DECL_HANDLER(init_process_done);
DECL_HANDLER(init_first_thread);
DECL_HANDLER(init_thread);
DECL_HANDLER(terminate_process);
DECL_HANDLER(terminate_thread);
DECL_HANDLER(get_process_info);
DECL_HANDLER(get_process_debug_info);
DECL_HANDLER(get_process_image_name);
DECL_HANDLER(get_process_vm_counters);
DECL_HANDLER(set_process_info);
DECL_HANDLER(get_thread_info);
DECL_HANDLER(get_thread_times);
DECL_HANDLER(set_thread_info);
DECL_HANDLER(suspend_thread);
DECL_HANDLER(resume_thread);
DECL_HANDLER(queue_apc);
DECL_HANDLER(get_apc_result);
DECL_HANDLER(close_handle);
DECL_HANDLER(set_handle_info);
DECL_HANDLER(dup_handle);
DECL_HANDLER(make_temporary);
DECL_HANDLER(open_process);
DECL_HANDLER(open_thread);
DECL_HANDLER(select);
DECL_HANDLER(create_event);
DECL_HANDLER(event_op);
DECL_HANDLER(query_event);
DECL_HANDLER(open_event);
DECL_HANDLER(create_keyed_event);
DECL_HANDLER(open_keyed_event);
DECL_HANDLER(create_mutex);
DECL_HANDLER(release_mutex);
DECL_HANDLER(open_mutex);
DECL_HANDLER(query_mutex);
DECL_HANDLER(create_semaphore);
DECL_HANDLER(release_semaphore);
DECL_HANDLER(query_semaphore);
DECL_HANDLER(open_semaphore);
DECL_HANDLER(create_file);
DECL_HANDLER(open_file_object);
DECL_HANDLER(alloc_file_handle);
DECL_HANDLER(get_handle_unix_name);
DECL_HANDLER(get_handle_fd);
DECL_HANDLER(get_directory_cache_entry);
DECL_HANDLER(flush);
DECL_HANDLER(get_file_info);
DECL_HANDLER(get_volume_info);
DECL_HANDLER(lock_file);
DECL_HANDLER(unlock_file);
DECL_HANDLER(recv_socket);
DECL_HANDLER(poll_socket);
DECL_HANDLER(send_socket);
DECL_HANDLER(get_next_console_request);
DECL_HANDLER(read_directory_changes);
DECL_HANDLER(read_change);
DECL_HANDLER(create_mapping);
DECL_HANDLER(open_mapping);
DECL_HANDLER(get_mapping_info);
DECL_HANDLER(map_view);
DECL_HANDLER(unmap_view);
DECL_HANDLER(get_mapping_committed_range);
DECL_HANDLER(add_mapping_committed_range);
DECL_HANDLER(is_same_mapping);
DECL_HANDLER(get_mapping_filename);
DECL_HANDLER(list_processes);
DECL_HANDLER(create_debug_obj);
DECL_HANDLER(wait_debug_event);
DECL_HANDLER(queue_exception_event);
DECL_HANDLER(get_exception_status);
DECL_HANDLER(continue_debug_event);
DECL_HANDLER(debug_process);
DECL_HANDLER(set_debug_obj_info);
DECL_HANDLER(read_process_memory);
DECL_HANDLER(write_process_memory);
DECL_HANDLER(create_key);
DECL_HANDLER(open_key);
DECL_HANDLER(delete_key);
DECL_HANDLER(flush_key);
DECL_HANDLER(enum_key);
DECL_HANDLER(set_key_value);
DECL_HANDLER(get_key_value);
DECL_HANDLER(enum_key_value);
DECL_HANDLER(delete_key_value);
DECL_HANDLER(load_registry);
DECL_HANDLER(unload_registry);
DECL_HANDLER(save_registry);
DECL_HANDLER(set_registry_notification);
DECL_HANDLER(create_timer);
DECL_HANDLER(open_timer);
DECL_HANDLER(set_timer);
DECL_HANDLER(cancel_timer);
DECL_HANDLER(get_timer_info);
DECL_HANDLER(get_thread_context);
DECL_HANDLER(set_thread_context);
DECL_HANDLER(get_selector_entry);
DECL_HANDLER(add_atom);
DECL_HANDLER(delete_atom);
DECL_HANDLER(find_atom);
DECL_HANDLER(get_atom_information);
DECL_HANDLER(get_msg_queue);
DECL_HANDLER(set_queue_fd);
DECL_HANDLER(set_queue_mask);
DECL_HANDLER(get_queue_status);
DECL_HANDLER(get_process_idle_event);
DECL_HANDLER(send_message);
DECL_HANDLER(post_quit_message);
DECL_HANDLER(send_hardware_message);
DECL_HANDLER(get_message);
DECL_HANDLER(reply_message);
DECL_HANDLER(accept_hardware_message);
DECL_HANDLER(get_message_reply);
DECL_HANDLER(set_win_timer);
DECL_HANDLER(kill_win_timer);
DECL_HANDLER(is_window_hung);
DECL_HANDLER(get_serial_info);
DECL_HANDLER(set_serial_info);
DECL_HANDLER(register_async);
DECL_HANDLER(cancel_async);
DECL_HANDLER(get_async_result);
DECL_HANDLER(read);
DECL_HANDLER(write);
DECL_HANDLER(ioctl);
DECL_HANDLER(set_irp_result);
DECL_HANDLER(create_named_pipe);
DECL_HANDLER(set_named_pipe_info);
DECL_HANDLER(create_window);
DECL_HANDLER(destroy_window);
DECL_HANDLER(get_desktop_window);
DECL_HANDLER(set_window_owner);
DECL_HANDLER(get_window_info);
DECL_HANDLER(set_window_info);
DECL_HANDLER(set_parent);
DECL_HANDLER(get_window_parents);
DECL_HANDLER(get_window_children);
DECL_HANDLER(get_window_children_from_point);
DECL_HANDLER(get_window_tree);
DECL_HANDLER(set_window_pos);
DECL_HANDLER(get_window_rectangles);
DECL_HANDLER(get_window_text);
DECL_HANDLER(set_window_text);
DECL_HANDLER(get_windows_offset);
DECL_HANDLER(get_visible_region);
DECL_HANDLER(get_surface_region);
DECL_HANDLER(get_window_region);
DECL_HANDLER(set_window_region);
DECL_HANDLER(get_update_region);
DECL_HANDLER(update_window_zorder);
DECL_HANDLER(redraw_window);
DECL_HANDLER(set_window_property);
DECL_HANDLER(remove_window_property);
DECL_HANDLER(get_window_property);
DECL_HANDLER(get_window_properties);
DECL_HANDLER(create_winstation);
DECL_HANDLER(open_winstation);
DECL_HANDLER(close_winstation);
DECL_HANDLER(get_process_winstation);
DECL_HANDLER(set_process_winstation);
DECL_HANDLER(enum_winstation);
DECL_HANDLER(create_desktop);
DECL_HANDLER(open_desktop);
DECL_HANDLER(open_input_desktop);
DECL_HANDLER(close_desktop);
DECL_HANDLER(get_thread_desktop);
DECL_HANDLER(set_thread_desktop);
DECL_HANDLER(enum_desktop);
DECL_HANDLER(set_user_object_info);
DECL_HANDLER(register_hotkey);
DECL_HANDLER(unregister_hotkey);
DECL_HANDLER(attach_thread_input);
DECL_HANDLER(get_thread_input);
DECL_HANDLER(get_last_input_time);
DECL_HANDLER(get_key_state);
DECL_HANDLER(set_key_state);
DECL_HANDLER(set_foreground_window);
DECL_HANDLER(set_focus_window);
DECL_HANDLER(set_active_window);
DECL_HANDLER(set_capture_window);
DECL_HANDLER(set_caret_window);
DECL_HANDLER(set_caret_info);
DECL_HANDLER(set_hook);
DECL_HANDLER(remove_hook);
DECL_HANDLER(start_hook_chain);
DECL_HANDLER(finish_hook_chain);
DECL_HANDLER(get_hook_info);
DECL_HANDLER(create_class);
DECL_HANDLER(destroy_class);
DECL_HANDLER(set_class_info);
DECL_HANDLER(open_clipboard);
DECL_HANDLER(close_clipboard);
DECL_HANDLER(empty_clipboard);
DECL_HANDLER(set_clipboard_data);
DECL_HANDLER(get_clipboard_data);
DECL_HANDLER(get_clipboard_formats);
DECL_HANDLER(enum_clipboard_formats);
DECL_HANDLER(release_clipboard);
DECL_HANDLER(get_clipboard_info);
DECL_HANDLER(set_clipboard_viewer);
DECL_HANDLER(add_clipboard_listener);
DECL_HANDLER(remove_clipboard_listener);
DECL_HANDLER(open_token);
DECL_HANDLER(set_global_windows);
DECL_HANDLER(adjust_token_privileges);
DECL_HANDLER(get_token_privileges);
DECL_HANDLER(check_token_privileges);
DECL_HANDLER(duplicate_token);
DECL_HANDLER(filter_token);
DECL_HANDLER(access_check);
DECL_HANDLER(get_token_sid);
DECL_HANDLER(get_token_groups);
DECL_HANDLER(get_token_default_dacl);
DECL_HANDLER(set_token_default_dacl);
DECL_HANDLER(set_security_object);
DECL_HANDLER(get_security_object);
DECL_HANDLER(get_system_handles);
DECL_HANDLER(create_mailslot);
DECL_HANDLER(set_mailslot_info);
DECL_HANDLER(create_directory);
DECL_HANDLER(open_directory);
DECL_HANDLER(get_directory_entry);
DECL_HANDLER(create_symlink);
DECL_HANDLER(open_symlink);
DECL_HANDLER(query_symlink);
DECL_HANDLER(get_object_info);
DECL_HANDLER(get_object_name);
DECL_HANDLER(get_object_type);
DECL_HANDLER(get_object_types);
DECL_HANDLER(allocate_locally_unique_id);
DECL_HANDLER(create_device_manager);
DECL_HANDLER(create_device);
DECL_HANDLER(delete_device);
DECL_HANDLER(get_next_device_request);
DECL_HANDLER(get_kernel_object_ptr);
DECL_HANDLER(set_kernel_object_ptr);
DECL_HANDLER(grab_kernel_object);
DECL_HANDLER(release_kernel_object);
DECL_HANDLER(get_kernel_object_handle);
DECL_HANDLER(make_process_system);
DECL_HANDLER(get_token_info);
DECL_HANDLER(create_linked_token);
DECL_HANDLER(create_completion);
DECL_HANDLER(open_completion);
DECL_HANDLER(add_completion);
DECL_HANDLER(remove_completion);
DECL_HANDLER(query_completion);
DECL_HANDLER(set_completion_info);
DECL_HANDLER(add_fd_completion);
DECL_HANDLER(set_fd_completion_mode);
DECL_HANDLER(set_fd_disp_info);
DECL_HANDLER(set_fd_name_info);
DECL_HANDLER(set_fd_eof_info);
DECL_HANDLER(get_window_layered_info);
DECL_HANDLER(set_window_layered_info);
DECL_HANDLER(alloc_user_handle);
DECL_HANDLER(free_user_handle);
DECL_HANDLER(set_cursor);
DECL_HANDLER(get_cursor_history);
DECL_HANDLER(get_rawinput_buffer);
DECL_HANDLER(update_rawinput_devices);
DECL_HANDLER(get_rawinput_devices);
DECL_HANDLER(create_job);
DECL_HANDLER(open_job);
DECL_HANDLER(assign_job);
DECL_HANDLER(process_in_job);
DECL_HANDLER(set_job_limits);
DECL_HANDLER(set_job_completion_port);
DECL_HANDLER(get_job_info);
DECL_HANDLER(terminate_job);
DECL_HANDLER(suspend_process);
DECL_HANDLER(resume_process);
DECL_HANDLER(get_next_thread);

#ifdef WANT_REQUEST_HANDLERS

typedef void (*req_handler)( const void *req, void *reply );
static const req_handler req_handlers[REQ_NB_REQUESTS] =
{
    (req_handler)req_new_process,
    (req_handler)req_get_new_process_info,
    (req_handler)req_new_thread,
    (req_handler)req_get_startup_info,
    (req_handler)req_init_process_done,
    (req_handler)req_init_first_thread,
    (req_handler)req_init_thread,
    (req_handler)req_terminate_process,
    (req_handler)req_terminate_thread,
    (req_handler)req_get_process_info,
    (req_handler)req_get_process_debug_info,
    (req_handler)req_get_process_image_name,
    (req_handler)req_get_process_vm_counters,
    (req_handler)req_set_process_info,
    (req_handler)req_get_thread_info,
    (req_handler)req_get_thread_times,
    (req_handler)req_set_thread_info,
    (req_handler)req_suspend_thread,
    (req_handler)req_resume_thread,
    (req_handler)req_queue_apc,
    (req_handler)req_get_apc_result,
    (req_handler)req_close_handle,
    (req_handler)req_set_handle_info,
    (req_handler)req_dup_handle,
    (req_handler)req_make_temporary,
    (req_handler)req_open_process,
    (req_handler)req_open_thread,
    (req_handler)req_select,
    (req_handler)req_create_event,
    (req_handler)req_event_op,
    (req_handler)req_query_event,
    (req_handler)req_open_event,
    (req_handler)req_create_keyed_event,
    (req_handler)req_open_keyed_event,
    (req_handler)req_create_mutex,
    (req_handler)req_release_mutex,
    (req_handler)req_open_mutex,
    (req_handler)req_query_mutex,
    (req_handler)req_create_semaphore,
    (req_handler)req_release_semaphore,
    (req_handler)req_query_semaphore,
    (req_handler)req_open_semaphore,
    (req_handler)req_create_file,
    (req_handler)req_open_file_object,
    (req_handler)req_alloc_file_handle,
    (req_handler)req_get_handle_unix_name,
    (req_handler)req_get_handle_fd,
    (req_handler)req_get_directory_cache_entry,
    (req_handler)req_flush,
    (req_handler)req_get_file_info,
    (req_handler)req_get_volume_info,
    (req_handler)req_lock_file,
    (req_handler)req_unlock_file,
    (req_handler)req_recv_socket,
    (req_handler)req_poll_socket,
    (req_handler)req_send_socket,
    (req_handler)req_get_next_console_request,
    (req_handler)req_read_directory_changes,
    (req_handler)req_read_change,
    (req_handler)req_create_mapping,
    (req_handler)req_open_mapping,
    (req_handler)req_get_mapping_info,
    (req_handler)req_map_view,
    (req_handler)req_unmap_view,
    (req_handler)req_get_mapping_committed_range,
    (req_handler)req_add_mapping_committed_range,
    (req_handler)req_is_same_mapping,
    (req_handler)req_get_mapping_filename,
    (req_handler)req_list_processes,
    (req_handler)req_create_debug_obj,
    (req_handler)req_wait_debug_event,
    (req_handler)req_queue_exception_event,
    (req_handler)req_get_exception_status,
    (req_handler)req_continue_debug_event,
    (req_handler)req_debug_process,
    (req_handler)req_set_debug_obj_info,
    (req_handler)req_read_process_memory,
    (req_handler)req_write_process_memory,
    (req_handler)req_create_key,
    (req_handler)req_open_key,
    (req_handler)req_delete_key,
    (req_handler)req_flush_key,
    (req_handler)req_enum_key,
    (req_handler)req_set_key_value,
    (req_handler)req_get_key_value,
    (req_handler)req_enum_key_value,
    (req_handler)req_delete_key_value,
    (req_handler)req_load_registry,
    (req_handler)req_unload_registry,
    (req_handler)req_save_registry,
    (req_handler)req_set_registry_notification,
    (req_handler)req_create_timer,
    (req_handler)req_open_timer,
    (req_handler)req_set_timer,
    (req_handler)req_cancel_timer,
    (req_handler)req_get_timer_info,
    (req_handler)req_get_thread_context,
    (req_handler)req_set_thread_context,
    (req_handler)req_get_selector_entry,
    (req_handler)req_add_atom,
    (req_handler)req_delete_atom,
    (req_handler)req_find_atom,
    (req_handler)req_get_atom_information,
    (req_handler)req_get_msg_queue,
    (req_handler)req_set_queue_fd,
    (req_handler)req_set_queue_mask,
    (req_handler)req_get_queue_status,
    (req_handler)req_get_process_idle_event,
    (req_handler)req_send_message,
    (req_handler)req_post_quit_message,
    (req_handler)req_send_hardware_message,
    (req_handler)req_get_message,
    (req_handler)req_reply_message,
    (req_handler)req_accept_hardware_message,
    (req_handler)req_get_message_reply,
    (req_handler)req_set_win_timer,
    (req_handler)req_kill_win_timer,
    (req_handler)req_is_window_hung,
    (req_handler)req_get_serial_info,
    (req_handler)req_set_serial_info,
    (req_handler)req_register_async,
    (req_handler)req_cancel_async,
    (req_handler)req_get_async_result,
    (req_handler)req_read,
    (req_handler)req_write,
    (req_handler)req_ioctl,
    (req_handler)req_set_irp_result,
    (req_handler)req_create_named_pipe,
    (req_handler)req_set_named_pipe_info,
    (req_handler)req_create_window,
    (req_handler)req_destroy_window,
    (req_handler)req_get_desktop_window,
    (req_handler)req_set_window_owner,
    (req_handler)req_get_window_info,
    (req_handler)req_set_window_info,
    (req_handler)req_set_parent,
    (req_handler)req_get_window_parents,
    (req_handler)req_get_window_children,
    (req_handler)req_get_window_children_from_point,
    (req_handler)req_get_window_tree,
    (req_handler)req_set_window_pos,
    (req_handler)req_get_window_rectangles,
    (req_handler)req_get_window_text,
    (req_handler)req_set_window_text,
    (req_handler)req_get_windows_offset,
    (req_handler)req_get_visible_region,
    (req_handler)req_get_surface_region,
    (req_handler)req_get_window_region,
    (req_handler)req_set_window_region,
    (req_handler)req_get_update_region,
    (req_handler)req_update_window_zorder,
    (req_handler)req_redraw_window,
    (req_handler)req_set_window_property,
    (req_handler)req_remove_window_property,
    (req_handler)req_get_window_property,
    (req_handler)req_get_window_properties,
    (req_handler)req_create_winstation,
    (req_handler)req_open_winstation,
    (req_handler)req_close_winstation,
    (req_handler)req_get_process_winstation,
    (req_handler)req_set_process_winstation,
    (req_handler)req_enum_winstation,
    (req_handler)req_create_desktop,
    (req_handler)req_open_desktop,
    (req_handler)req_open_input_desktop,
    (req_handler)req_close_desktop,
    (req_handler)req_get_thread_desktop,
    (req_handler)req_set_thread_desktop,
    (req_handler)req_enum_desktop,
    (req_handler)req_set_user_object_info,
    (req_handler)req_register_hotkey,
    (req_handler)req_unregister_hotkey,
    (req_handler)req_attach_thread_input,
    (req_handler)req_get_thread_input,
    (req_handler)req_get_last_input_time,
    (req_handler)req_get_key_state,
    (req_handler)req_set_key_state,
    (req_handler)req_set_foreground_window,
    (req_handler)req_set_focus_window,
    (req_handler)req_set_active_window,
    (req_handler)req_set_capture_window,
    (req_handler)req_set_caret_window,
    (req_handler)req_set_caret_info,
    (req_handler)req_set_hook,
    (req_handler)req_remove_hook,
    (req_handler)req_start_hook_chain,
    (req_handler)req_finish_hook_chain,
    (req_handler)req_get_hook_info,
    (req_handler)req_create_class,
    (req_handler)req_destroy_class,
    (req_handler)req_set_class_info,
    (req_handler)req_open_clipboard,
    (req_handler)req_close_clipboard,
    (req_handler)req_empty_clipboard,
    (req_handler)req_set_clipboard_data,
    (req_handler)req_get_clipboard_data,
    (req_handler)req_get_clipboard_formats,
    (req_handler)req_enum_clipboard_formats,
    (req_handler)req_release_clipboard,
    (req_handler)req_get_clipboard_info,
    (req_handler)req_set_clipboard_viewer,
    (req_handler)req_add_clipboard_listener,
    (req_handler)req_remove_clipboard_listener,
    (req_handler)req_open_token,
    (req_handler)req_set_global_windows,
    (req_handler)req_adjust_token_privileges,
    (req_handler)req_get_token_privileges,
    (req_handler)req_check_token_privileges,
    (req_handler)req_duplicate_token,
    (req_handler)req_filter_token,
    (req_handler)req_access_check,
    (req_handler)req_get_token_sid,
    (req_handler)req_get_token_groups,
    (req_handler)req_get_token_default_dacl,
    (req_handler)req_set_token_default_dacl,
    (req_handler)req_set_security_object,
    (req_handler)req_get_security_object,
    (req_handler)req_get_system_handles,
    (req_handler)req_create_mailslot,
    (req_handler)req_set_mailslot_info,
    (req_handler)req_create_directory,
    (req_handler)req_open_directory,
    (req_handler)req_get_directory_entry,
    (req_handler)req_create_symlink,
    (req_handler)req_open_symlink,
    (req_handler)req_query_symlink,
    (req_handler)req_get_object_info,
    (req_handler)req_get_object_name,
    (req_handler)req_get_object_type,
    (req_handler)req_get_object_types,
    (req_handler)req_allocate_locally_unique_id,
    (req_handler)req_create_device_manager,
    (req_handler)req_create_device,
    (req_handler)req_delete_device,
    (req_handler)req_get_next_device_request,
    (req_handler)req_get_kernel_object_ptr,
    (req_handler)req_set_kernel_object_ptr,
    (req_handler)req_grab_kernel_object,
    (req_handler)req_release_kernel_object,
    (req_handler)req_get_kernel_object_handle,
    (req_handler)req_make_process_system,
    (req_handler)req_get_token_info,
    (req_handler)req_create_linked_token,
    (req_handler)req_create_completion,
    (req_handler)req_open_completion,
    (req_handler)req_add_completion,
    (req_handler)req_remove_completion,
    (req_handler)req_query_completion,
    (req_handler)req_set_completion_info,
    (req_handler)req_add_fd_completion,
    (req_handler)req_set_fd_completion_mode,
    (req_handler)req_set_fd_disp_info,
    (req_handler)req_set_fd_name_info,
    (req_handler)req_set_fd_eof_info,
    (req_handler)req_get_window_layered_info,
    (req_handler)req_set_window_layered_info,
    (req_handler)req_alloc_user_handle,
    (req_handler)req_free_user_handle,
    (req_handler)req_set_cursor,
    (req_handler)req_get_cursor_history,
    (req_handler)req_get_rawinput_buffer,
    (req_handler)req_update_rawinput_devices,
    (req_handler)req_get_rawinput_devices,
    (req_handler)req_create_job,
    (req_handler)req_open_job,
    (req_handler)req_assign_job,
    (req_handler)req_process_in_job,
    (req_handler)req_set_job_limits,
    (req_handler)req_set_job_completion_port,
    (req_handler)req_get_job_info,
    (req_handler)req_terminate_job,
    (req_handler)req_suspend_process,
    (req_handler)req_resume_process,
    (req_handler)req_get_next_thread,
};

C_ASSERT( sizeof(abstime_t) == 8 );
C_ASSERT( sizeof(affinity_t) == 8 );
C_ASSERT( sizeof(apc_call_t) == 48 );
C_ASSERT( sizeof(apc_param_t) == 8 );
C_ASSERT( sizeof(apc_result_t) == 40 );
C_ASSERT( sizeof(async_data_t) == 40 );
C_ASSERT( sizeof(atom_t) == 4 );
C_ASSERT( sizeof(char) == 1 );
C_ASSERT( sizeof(client_ptr_t) == 8 );
C_ASSERT( sizeof(data_size_t) == 4 );
C_ASSERT( sizeof(file_pos_t) == 8 );
C_ASSERT( sizeof(generic_map_t) == 16 );
C_ASSERT( sizeof(hw_input_t) == 40 );
C_ASSERT( sizeof(int) == 4 );
C_ASSERT( sizeof(ioctl_code_t) == 4 );
C_ASSERT( sizeof(irp_params_t) == 32 );
C_ASSERT( sizeof(lparam_t) == 8 );
C_ASSERT( sizeof(luid_t) == 8 );
C_ASSERT( sizeof(mem_size_t) == 8 );
C_ASSERT( sizeof(mod_handle_t) == 8 );
C_ASSERT( sizeof(obj_handle_t) == 4 );
C_ASSERT( sizeof(process_id_t) == 4 );
C_ASSERT( sizeof(rectangle_t) == 16 );
C_ASSERT( sizeof(short int) == 2 );
C_ASSERT( sizeof(thread_id_t) == 4 );
C_ASSERT( sizeof(timeout_t) == 8 );
C_ASSERT( sizeof(unsigned char) == 1 );
C_ASSERT( sizeof(unsigned int) == 4 );
C_ASSERT( sizeof(unsigned short) == 2 );
C_ASSERT( sizeof(user_handle_t) == 4 );
C_ASSERT( FIELD_OFFSET(struct new_process_request, token) == 12 );
C_ASSERT( FIELD_OFFSET(struct new_process_request, debug) == 16 );
C_ASSERT( FIELD_OFFSET(struct new_process_request, parent_process) == 20 );
C_ASSERT( FIELD_OFFSET(struct new_process_request, flags) == 24 );
C_ASSERT( FIELD_OFFSET(struct new_process_request, socket_fd) == 28 );
C_ASSERT( FIELD_OFFSET(struct new_process_request, access) == 32 );
C_ASSERT( FIELD_OFFSET(struct new_process_request, machine) == 36 );
C_ASSERT( FIELD_OFFSET(struct new_process_request, info_size) == 40 );
C_ASSERT( FIELD_OFFSET(struct new_process_request, handles_size) == 44 );
C_ASSERT( FIELD_OFFSET(struct new_process_request, jobs_size) == 48 );
C_ASSERT( sizeof(struct new_process_request) == 56 );
C_ASSERT( FIELD_OFFSET(struct new_process_reply, info) == 8 );
C_ASSERT( FIELD_OFFSET(struct new_process_reply, pid) == 12 );
C_ASSERT( FIELD_OFFSET(struct new_process_reply, handle) == 16 );
C_ASSERT( sizeof(struct new_process_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_new_process_info_request, info) == 12 );
C_ASSERT( sizeof(struct get_new_process_info_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_new_process_info_reply, success) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_new_process_info_reply, exit_code) == 12 );
C_ASSERT( sizeof(struct get_new_process_info_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct new_thread_request, process) == 12 );
C_ASSERT( FIELD_OFFSET(struct new_thread_request, access) == 16 );
C_ASSERT( FIELD_OFFSET(struct new_thread_request, suspend) == 20 );
C_ASSERT( FIELD_OFFSET(struct new_thread_request, request_fd) == 24 );
C_ASSERT( sizeof(struct new_thread_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct new_thread_reply, tid) == 8 );
C_ASSERT( FIELD_OFFSET(struct new_thread_reply, handle) == 12 );
C_ASSERT( sizeof(struct new_thread_reply) == 16 );
C_ASSERT( sizeof(struct get_startup_info_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_startup_info_reply, info_size) == 8 );
C_ASSERT( sizeof(struct get_startup_info_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct init_process_done_request, teb) == 16 );
C_ASSERT( FIELD_OFFSET(struct init_process_done_request, peb) == 24 );
C_ASSERT( FIELD_OFFSET(struct init_process_done_request, ldt_copy) == 32 );
C_ASSERT( sizeof(struct init_process_done_request) == 40 );
C_ASSERT( FIELD_OFFSET(struct init_process_done_reply, entry) == 8 );
C_ASSERT( FIELD_OFFSET(struct init_process_done_reply, suspend) == 16 );
C_ASSERT( sizeof(struct init_process_done_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct init_first_thread_request, unix_pid) == 12 );
C_ASSERT( FIELD_OFFSET(struct init_first_thread_request, unix_tid) == 16 );
C_ASSERT( FIELD_OFFSET(struct init_first_thread_request, debug_level) == 20 );
C_ASSERT( FIELD_OFFSET(struct init_first_thread_request, reply_fd) == 24 );
C_ASSERT( FIELD_OFFSET(struct init_first_thread_request, wait_fd) == 28 );
C_ASSERT( sizeof(struct init_first_thread_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct init_first_thread_reply, pid) == 8 );
C_ASSERT( FIELD_OFFSET(struct init_first_thread_reply, tid) == 12 );
C_ASSERT( FIELD_OFFSET(struct init_first_thread_reply, server_start) == 16 );
C_ASSERT( FIELD_OFFSET(struct init_first_thread_reply, session_id) == 24 );
C_ASSERT( FIELD_OFFSET(struct init_first_thread_reply, info_size) == 28 );
C_ASSERT( sizeof(struct init_first_thread_reply) == 32 );
C_ASSERT( FIELD_OFFSET(struct init_thread_request, unix_tid) == 12 );
C_ASSERT( FIELD_OFFSET(struct init_thread_request, reply_fd) == 16 );
C_ASSERT( FIELD_OFFSET(struct init_thread_request, wait_fd) == 20 );
C_ASSERT( FIELD_OFFSET(struct init_thread_request, teb) == 24 );
C_ASSERT( FIELD_OFFSET(struct init_thread_request, entry) == 32 );
C_ASSERT( sizeof(struct init_thread_request) == 40 );
C_ASSERT( FIELD_OFFSET(struct init_thread_reply, suspend) == 8 );
C_ASSERT( sizeof(struct init_thread_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct terminate_process_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct terminate_process_request, exit_code) == 16 );
C_ASSERT( sizeof(struct terminate_process_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct terminate_process_reply, self) == 8 );
C_ASSERT( sizeof(struct terminate_process_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct terminate_thread_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct terminate_thread_request, exit_code) == 16 );
C_ASSERT( sizeof(struct terminate_thread_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct terminate_thread_reply, self) == 8 );
C_ASSERT( sizeof(struct terminate_thread_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_process_info_request, handle) == 12 );
C_ASSERT( sizeof(struct get_process_info_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_process_info_reply, pid) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_process_info_reply, ppid) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_process_info_reply, affinity) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_process_info_reply, peb) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_process_info_reply, start_time) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_process_info_reply, end_time) == 40 );
C_ASSERT( FIELD_OFFSET(struct get_process_info_reply, session_id) == 48 );
C_ASSERT( FIELD_OFFSET(struct get_process_info_reply, exit_code) == 52 );
C_ASSERT( FIELD_OFFSET(struct get_process_info_reply, priority) == 56 );
C_ASSERT( FIELD_OFFSET(struct get_process_info_reply, machine) == 60 );
C_ASSERT( sizeof(struct get_process_info_reply) == 64 );
C_ASSERT( FIELD_OFFSET(struct get_process_debug_info_request, handle) == 12 );
C_ASSERT( sizeof(struct get_process_debug_info_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_process_debug_info_reply, debug) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_process_debug_info_reply, debug_children) == 12 );
C_ASSERT( sizeof(struct get_process_debug_info_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_process_image_name_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_process_image_name_request, win32) == 16 );
C_ASSERT( sizeof(struct get_process_image_name_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_process_image_name_reply, len) == 8 );
C_ASSERT( sizeof(struct get_process_image_name_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_process_vm_counters_request, handle) == 12 );
C_ASSERT( sizeof(struct get_process_vm_counters_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_process_vm_counters_reply, peak_virtual_size) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_process_vm_counters_reply, virtual_size) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_process_vm_counters_reply, peak_working_set_size) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_process_vm_counters_reply, working_set_size) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_process_vm_counters_reply, pagefile_usage) == 40 );
C_ASSERT( FIELD_OFFSET(struct get_process_vm_counters_reply, peak_pagefile_usage) == 48 );
C_ASSERT( sizeof(struct get_process_vm_counters_reply) == 56 );
C_ASSERT( FIELD_OFFSET(struct set_process_info_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_process_info_request, mask) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_process_info_request, priority) == 20 );
C_ASSERT( FIELD_OFFSET(struct set_process_info_request, affinity) == 24 );
C_ASSERT( sizeof(struct set_process_info_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_thread_info_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_thread_info_request, access) == 16 );
C_ASSERT( sizeof(struct get_thread_info_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_thread_info_reply, pid) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_thread_info_reply, tid) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_thread_info_reply, teb) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_thread_info_reply, entry_point) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_thread_info_reply, affinity) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_thread_info_reply, exit_code) == 40 );
C_ASSERT( FIELD_OFFSET(struct get_thread_info_reply, priority) == 44 );
C_ASSERT( FIELD_OFFSET(struct get_thread_info_reply, last) == 48 );
C_ASSERT( FIELD_OFFSET(struct get_thread_info_reply, suspend_count) == 52 );
C_ASSERT( FIELD_OFFSET(struct get_thread_info_reply, dbg_hidden) == 56 );
C_ASSERT( FIELD_OFFSET(struct get_thread_info_reply, desc_len) == 60 );
C_ASSERT( sizeof(struct get_thread_info_reply) == 64 );
C_ASSERT( FIELD_OFFSET(struct get_thread_times_request, handle) == 12 );
C_ASSERT( sizeof(struct get_thread_times_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_thread_times_reply, creation_time) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_thread_times_reply, exit_time) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_thread_times_reply, unix_pid) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_thread_times_reply, unix_tid) == 28 );
C_ASSERT( sizeof(struct get_thread_times_reply) == 32 );
C_ASSERT( FIELD_OFFSET(struct set_thread_info_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_thread_info_request, mask) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_thread_info_request, priority) == 20 );
C_ASSERT( FIELD_OFFSET(struct set_thread_info_request, affinity) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_thread_info_request, entry_point) == 32 );
C_ASSERT( FIELD_OFFSET(struct set_thread_info_request, token) == 40 );
C_ASSERT( sizeof(struct set_thread_info_request) == 48 );
C_ASSERT( FIELD_OFFSET(struct suspend_thread_request, handle) == 12 );
C_ASSERT( sizeof(struct suspend_thread_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct suspend_thread_reply, count) == 8 );
C_ASSERT( sizeof(struct suspend_thread_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct resume_thread_request, handle) == 12 );
C_ASSERT( sizeof(struct resume_thread_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct resume_thread_reply, count) == 8 );
C_ASSERT( sizeof(struct resume_thread_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct queue_apc_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct queue_apc_request, call) == 16 );
C_ASSERT( sizeof(struct queue_apc_request) == 64 );
C_ASSERT( FIELD_OFFSET(struct queue_apc_reply, handle) == 8 );
C_ASSERT( FIELD_OFFSET(struct queue_apc_reply, self) == 12 );
C_ASSERT( sizeof(struct queue_apc_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_apc_result_request, handle) == 12 );
C_ASSERT( sizeof(struct get_apc_result_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_apc_result_reply, result) == 8 );
C_ASSERT( sizeof(struct get_apc_result_reply) == 48 );
C_ASSERT( FIELD_OFFSET(struct close_handle_request, handle) == 12 );
C_ASSERT( sizeof(struct close_handle_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_handle_info_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_handle_info_request, flags) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_handle_info_request, mask) == 20 );
C_ASSERT( sizeof(struct set_handle_info_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_handle_info_reply, old_flags) == 8 );
C_ASSERT( sizeof(struct set_handle_info_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct dup_handle_request, src_process) == 12 );
C_ASSERT( FIELD_OFFSET(struct dup_handle_request, src_handle) == 16 );
C_ASSERT( FIELD_OFFSET(struct dup_handle_request, dst_process) == 20 );
C_ASSERT( FIELD_OFFSET(struct dup_handle_request, access) == 24 );
C_ASSERT( FIELD_OFFSET(struct dup_handle_request, attributes) == 28 );
C_ASSERT( FIELD_OFFSET(struct dup_handle_request, options) == 32 );
C_ASSERT( sizeof(struct dup_handle_request) == 40 );
C_ASSERT( FIELD_OFFSET(struct dup_handle_reply, handle) == 8 );
C_ASSERT( sizeof(struct dup_handle_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct make_temporary_request, handle) == 12 );
C_ASSERT( sizeof(struct make_temporary_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_process_request, pid) == 12 );
C_ASSERT( FIELD_OFFSET(struct open_process_request, access) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_process_request, attributes) == 20 );
C_ASSERT( sizeof(struct open_process_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct open_process_reply, handle) == 8 );
C_ASSERT( sizeof(struct open_process_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_thread_request, tid) == 12 );
C_ASSERT( FIELD_OFFSET(struct open_thread_request, access) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_thread_request, attributes) == 20 );
C_ASSERT( sizeof(struct open_thread_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct open_thread_reply, handle) == 8 );
C_ASSERT( sizeof(struct open_thread_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct select_request, flags) == 12 );
C_ASSERT( FIELD_OFFSET(struct select_request, cookie) == 16 );
C_ASSERT( FIELD_OFFSET(struct select_request, timeout) == 24 );
C_ASSERT( FIELD_OFFSET(struct select_request, size) == 32 );
C_ASSERT( FIELD_OFFSET(struct select_request, prev_apc) == 36 );
C_ASSERT( sizeof(struct select_request) == 40 );
C_ASSERT( FIELD_OFFSET(struct select_reply, call) == 8 );
C_ASSERT( FIELD_OFFSET(struct select_reply, apc_handle) == 56 );
C_ASSERT( FIELD_OFFSET(struct select_reply, signaled) == 60 );
C_ASSERT( sizeof(struct select_reply) == 64 );
C_ASSERT( FIELD_OFFSET(struct create_event_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct create_event_request, manual_reset) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_event_request, initial_state) == 20 );
C_ASSERT( sizeof(struct create_event_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct create_event_reply, handle) == 8 );
C_ASSERT( sizeof(struct create_event_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct event_op_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct event_op_request, op) == 16 );
C_ASSERT( sizeof(struct event_op_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct event_op_reply, state) == 8 );
C_ASSERT( sizeof(struct event_op_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct query_event_request, handle) == 12 );
C_ASSERT( sizeof(struct query_event_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct query_event_reply, manual_reset) == 8 );
C_ASSERT( FIELD_OFFSET(struct query_event_reply, state) == 12 );
C_ASSERT( sizeof(struct query_event_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_event_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct open_event_request, attributes) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_event_request, rootdir) == 20 );
C_ASSERT( sizeof(struct open_event_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct open_event_reply, handle) == 8 );
C_ASSERT( sizeof(struct open_event_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_keyed_event_request, access) == 12 );
C_ASSERT( sizeof(struct create_keyed_event_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_keyed_event_reply, handle) == 8 );
C_ASSERT( sizeof(struct create_keyed_event_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_keyed_event_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct open_keyed_event_request, attributes) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_keyed_event_request, rootdir) == 20 );
C_ASSERT( sizeof(struct open_keyed_event_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct open_keyed_event_reply, handle) == 8 );
C_ASSERT( sizeof(struct open_keyed_event_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_mutex_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct create_mutex_request, owned) == 16 );
C_ASSERT( sizeof(struct create_mutex_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct create_mutex_reply, handle) == 8 );
C_ASSERT( sizeof(struct create_mutex_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct release_mutex_request, handle) == 12 );
C_ASSERT( sizeof(struct release_mutex_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct release_mutex_reply, prev_count) == 8 );
C_ASSERT( sizeof(struct release_mutex_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_mutex_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct open_mutex_request, attributes) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_mutex_request, rootdir) == 20 );
C_ASSERT( sizeof(struct open_mutex_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct open_mutex_reply, handle) == 8 );
C_ASSERT( sizeof(struct open_mutex_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct query_mutex_request, handle) == 12 );
C_ASSERT( sizeof(struct query_mutex_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct query_mutex_reply, count) == 8 );
C_ASSERT( FIELD_OFFSET(struct query_mutex_reply, owned) == 12 );
C_ASSERT( FIELD_OFFSET(struct query_mutex_reply, abandoned) == 16 );
C_ASSERT( sizeof(struct query_mutex_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct create_semaphore_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct create_semaphore_request, initial) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_semaphore_request, max) == 20 );
C_ASSERT( sizeof(struct create_semaphore_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct create_semaphore_reply, handle) == 8 );
C_ASSERT( sizeof(struct create_semaphore_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct release_semaphore_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct release_semaphore_request, count) == 16 );
C_ASSERT( sizeof(struct release_semaphore_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct release_semaphore_reply, prev_count) == 8 );
C_ASSERT( sizeof(struct release_semaphore_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct query_semaphore_request, handle) == 12 );
C_ASSERT( sizeof(struct query_semaphore_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct query_semaphore_reply, current) == 8 );
C_ASSERT( FIELD_OFFSET(struct query_semaphore_reply, max) == 12 );
C_ASSERT( sizeof(struct query_semaphore_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_semaphore_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct open_semaphore_request, attributes) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_semaphore_request, rootdir) == 20 );
C_ASSERT( sizeof(struct open_semaphore_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct open_semaphore_reply, handle) == 8 );
C_ASSERT( sizeof(struct open_semaphore_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_file_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct create_file_request, sharing) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_file_request, create) == 20 );
C_ASSERT( FIELD_OFFSET(struct create_file_request, options) == 24 );
C_ASSERT( FIELD_OFFSET(struct create_file_request, attrs) == 28 );
C_ASSERT( sizeof(struct create_file_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct create_file_reply, handle) == 8 );
C_ASSERT( sizeof(struct create_file_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_file_object_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct open_file_object_request, attributes) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_file_object_request, rootdir) == 20 );
C_ASSERT( FIELD_OFFSET(struct open_file_object_request, sharing) == 24 );
C_ASSERT( FIELD_OFFSET(struct open_file_object_request, options) == 28 );
C_ASSERT( sizeof(struct open_file_object_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct open_file_object_reply, handle) == 8 );
C_ASSERT( sizeof(struct open_file_object_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct alloc_file_handle_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct alloc_file_handle_request, attributes) == 16 );
C_ASSERT( FIELD_OFFSET(struct alloc_file_handle_request, fd) == 20 );
C_ASSERT( sizeof(struct alloc_file_handle_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct alloc_file_handle_reply, handle) == 8 );
C_ASSERT( sizeof(struct alloc_file_handle_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_handle_unix_name_request, handle) == 12 );
C_ASSERT( sizeof(struct get_handle_unix_name_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_handle_unix_name_reply, name_len) == 8 );
C_ASSERT( sizeof(struct get_handle_unix_name_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_handle_fd_request, handle) == 12 );
C_ASSERT( sizeof(struct get_handle_fd_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_handle_fd_reply, type) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_handle_fd_reply, cacheable) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_handle_fd_reply, access) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_handle_fd_reply, options) == 20 );
C_ASSERT( sizeof(struct get_handle_fd_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_directory_cache_entry_request, handle) == 12 );
C_ASSERT( sizeof(struct get_directory_cache_entry_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_directory_cache_entry_reply, entry) == 8 );
C_ASSERT( sizeof(struct get_directory_cache_entry_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct flush_request, async) == 16 );
C_ASSERT( sizeof(struct flush_request) == 56 );
C_ASSERT( FIELD_OFFSET(struct flush_reply, event) == 8 );
C_ASSERT( sizeof(struct flush_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_file_info_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_file_info_request, info_class) == 16 );
C_ASSERT( sizeof(struct get_file_info_request) == 24 );
C_ASSERT( sizeof(struct get_file_info_reply) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_volume_info_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_volume_info_request, async) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_volume_info_request, info_class) == 56 );
C_ASSERT( sizeof(struct get_volume_info_request) == 64 );
C_ASSERT( FIELD_OFFSET(struct get_volume_info_reply, wait) == 8 );
C_ASSERT( sizeof(struct get_volume_info_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct lock_file_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct lock_file_request, offset) == 16 );
C_ASSERT( FIELD_OFFSET(struct lock_file_request, count) == 24 );
C_ASSERT( FIELD_OFFSET(struct lock_file_request, shared) == 32 );
C_ASSERT( FIELD_OFFSET(struct lock_file_request, wait) == 36 );
C_ASSERT( sizeof(struct lock_file_request) == 40 );
C_ASSERT( FIELD_OFFSET(struct lock_file_reply, handle) == 8 );
C_ASSERT( FIELD_OFFSET(struct lock_file_reply, overlapped) == 12 );
C_ASSERT( sizeof(struct lock_file_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct unlock_file_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct unlock_file_request, offset) == 16 );
C_ASSERT( FIELD_OFFSET(struct unlock_file_request, count) == 24 );
C_ASSERT( sizeof(struct unlock_file_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct recv_socket_request, oob) == 12 );
C_ASSERT( FIELD_OFFSET(struct recv_socket_request, async) == 16 );
C_ASSERT( FIELD_OFFSET(struct recv_socket_request, status) == 56 );
C_ASSERT( FIELD_OFFSET(struct recv_socket_request, total) == 60 );
C_ASSERT( sizeof(struct recv_socket_request) == 64 );
C_ASSERT( FIELD_OFFSET(struct recv_socket_reply, wait) == 8 );
C_ASSERT( FIELD_OFFSET(struct recv_socket_reply, options) == 12 );
C_ASSERT( sizeof(struct recv_socket_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct poll_socket_request, async) == 16 );
C_ASSERT( FIELD_OFFSET(struct poll_socket_request, timeout) == 56 );
C_ASSERT( sizeof(struct poll_socket_request) == 64 );
C_ASSERT( FIELD_OFFSET(struct poll_socket_reply, wait) == 8 );
C_ASSERT( FIELD_OFFSET(struct poll_socket_reply, options) == 12 );
C_ASSERT( sizeof(struct poll_socket_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct send_socket_request, async) == 16 );
C_ASSERT( FIELD_OFFSET(struct send_socket_request, status) == 56 );
C_ASSERT( FIELD_OFFSET(struct send_socket_request, total) == 60 );
C_ASSERT( sizeof(struct send_socket_request) == 64 );
C_ASSERT( FIELD_OFFSET(struct send_socket_reply, wait) == 8 );
C_ASSERT( FIELD_OFFSET(struct send_socket_reply, options) == 12 );
C_ASSERT( sizeof(struct send_socket_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_next_console_request_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_next_console_request_request, signal) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_next_console_request_request, read) == 20 );
C_ASSERT( FIELD_OFFSET(struct get_next_console_request_request, status) == 24 );
C_ASSERT( sizeof(struct get_next_console_request_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_next_console_request_reply, code) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_next_console_request_reply, output) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_next_console_request_reply, out_size) == 16 );
C_ASSERT( sizeof(struct get_next_console_request_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct read_directory_changes_request, filter) == 12 );
C_ASSERT( FIELD_OFFSET(struct read_directory_changes_request, subtree) == 16 );
C_ASSERT( FIELD_OFFSET(struct read_directory_changes_request, want_data) == 20 );
C_ASSERT( FIELD_OFFSET(struct read_directory_changes_request, async) == 24 );
C_ASSERT( sizeof(struct read_directory_changes_request) == 64 );
C_ASSERT( FIELD_OFFSET(struct read_change_request, handle) == 12 );
C_ASSERT( sizeof(struct read_change_request) == 16 );
C_ASSERT( sizeof(struct read_change_reply) == 8 );
C_ASSERT( FIELD_OFFSET(struct create_mapping_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct create_mapping_request, flags) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_mapping_request, file_access) == 20 );
C_ASSERT( FIELD_OFFSET(struct create_mapping_request, size) == 24 );
C_ASSERT( FIELD_OFFSET(struct create_mapping_request, file_handle) == 32 );
C_ASSERT( sizeof(struct create_mapping_request) == 40 );
C_ASSERT( FIELD_OFFSET(struct create_mapping_reply, handle) == 8 );
C_ASSERT( sizeof(struct create_mapping_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_mapping_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct open_mapping_request, attributes) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_mapping_request, rootdir) == 20 );
C_ASSERT( sizeof(struct open_mapping_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct open_mapping_reply, handle) == 8 );
C_ASSERT( sizeof(struct open_mapping_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_mapping_info_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_mapping_info_request, access) == 16 );
C_ASSERT( sizeof(struct get_mapping_info_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_mapping_info_reply, size) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_mapping_info_reply, flags) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_mapping_info_reply, shared_file) == 20 );
C_ASSERT( FIELD_OFFSET(struct get_mapping_info_reply, total) == 24 );
C_ASSERT( sizeof(struct get_mapping_info_reply) == 32 );
C_ASSERT( FIELD_OFFSET(struct map_view_request, mapping) == 12 );
C_ASSERT( FIELD_OFFSET(struct map_view_request, access) == 16 );
C_ASSERT( FIELD_OFFSET(struct map_view_request, base) == 24 );
C_ASSERT( FIELD_OFFSET(struct map_view_request, size) == 32 );
C_ASSERT( FIELD_OFFSET(struct map_view_request, start) == 40 );
C_ASSERT( sizeof(struct map_view_request) == 48 );
C_ASSERT( FIELD_OFFSET(struct unmap_view_request, base) == 16 );
C_ASSERT( sizeof(struct unmap_view_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_mapping_committed_range_request, base) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_mapping_committed_range_request, offset) == 24 );
C_ASSERT( sizeof(struct get_mapping_committed_range_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_mapping_committed_range_reply, size) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_mapping_committed_range_reply, committed) == 16 );
C_ASSERT( sizeof(struct get_mapping_committed_range_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct add_mapping_committed_range_request, base) == 16 );
C_ASSERT( FIELD_OFFSET(struct add_mapping_committed_range_request, offset) == 24 );
C_ASSERT( FIELD_OFFSET(struct add_mapping_committed_range_request, size) == 32 );
C_ASSERT( sizeof(struct add_mapping_committed_range_request) == 40 );
C_ASSERT( FIELD_OFFSET(struct is_same_mapping_request, base1) == 16 );
C_ASSERT( FIELD_OFFSET(struct is_same_mapping_request, base2) == 24 );
C_ASSERT( sizeof(struct is_same_mapping_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_mapping_filename_request, process) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_mapping_filename_request, addr) == 16 );
C_ASSERT( sizeof(struct get_mapping_filename_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_mapping_filename_reply, len) == 8 );
C_ASSERT( sizeof(struct get_mapping_filename_reply) == 16 );
C_ASSERT( sizeof(struct list_processes_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct list_processes_reply, info_size) == 8 );
C_ASSERT( FIELD_OFFSET(struct list_processes_reply, process_count) == 12 );
C_ASSERT( sizeof(struct list_processes_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_debug_obj_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct create_debug_obj_request, flags) == 16 );
C_ASSERT( sizeof(struct create_debug_obj_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct create_debug_obj_reply, handle) == 8 );
C_ASSERT( sizeof(struct create_debug_obj_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct wait_debug_event_request, debug) == 12 );
C_ASSERT( sizeof(struct wait_debug_event_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct wait_debug_event_reply, pid) == 8 );
C_ASSERT( FIELD_OFFSET(struct wait_debug_event_reply, tid) == 12 );
C_ASSERT( sizeof(struct wait_debug_event_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct queue_exception_event_request, first) == 12 );
C_ASSERT( FIELD_OFFSET(struct queue_exception_event_request, code) == 16 );
C_ASSERT( FIELD_OFFSET(struct queue_exception_event_request, flags) == 20 );
C_ASSERT( FIELD_OFFSET(struct queue_exception_event_request, record) == 24 );
C_ASSERT( FIELD_OFFSET(struct queue_exception_event_request, address) == 32 );
C_ASSERT( FIELD_OFFSET(struct queue_exception_event_request, len) == 40 );
C_ASSERT( sizeof(struct queue_exception_event_request) == 48 );
C_ASSERT( FIELD_OFFSET(struct queue_exception_event_reply, handle) == 8 );
C_ASSERT( sizeof(struct queue_exception_event_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_exception_status_request, handle) == 12 );
C_ASSERT( sizeof(struct get_exception_status_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct continue_debug_event_request, debug) == 12 );
C_ASSERT( FIELD_OFFSET(struct continue_debug_event_request, pid) == 16 );
C_ASSERT( FIELD_OFFSET(struct continue_debug_event_request, tid) == 20 );
C_ASSERT( FIELD_OFFSET(struct continue_debug_event_request, status) == 24 );
C_ASSERT( sizeof(struct continue_debug_event_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct debug_process_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct debug_process_request, debug) == 16 );
C_ASSERT( FIELD_OFFSET(struct debug_process_request, attach) == 20 );
C_ASSERT( sizeof(struct debug_process_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_debug_obj_info_request, debug) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_debug_obj_info_request, flags) == 16 );
C_ASSERT( sizeof(struct set_debug_obj_info_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct read_process_memory_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct read_process_memory_request, addr) == 16 );
C_ASSERT( sizeof(struct read_process_memory_request) == 24 );
C_ASSERT( sizeof(struct read_process_memory_reply) == 8 );
C_ASSERT( FIELD_OFFSET(struct write_process_memory_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct write_process_memory_request, addr) == 16 );
C_ASSERT( sizeof(struct write_process_memory_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct create_key_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct create_key_request, options) == 16 );
C_ASSERT( sizeof(struct create_key_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct create_key_reply, hkey) == 8 );
C_ASSERT( FIELD_OFFSET(struct create_key_reply, created) == 12 );
C_ASSERT( sizeof(struct create_key_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_key_request, parent) == 12 );
C_ASSERT( FIELD_OFFSET(struct open_key_request, access) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_key_request, attributes) == 20 );
C_ASSERT( sizeof(struct open_key_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct open_key_reply, hkey) == 8 );
C_ASSERT( sizeof(struct open_key_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct delete_key_request, hkey) == 12 );
C_ASSERT( sizeof(struct delete_key_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct flush_key_request, hkey) == 12 );
C_ASSERT( sizeof(struct flush_key_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct enum_key_request, hkey) == 12 );
C_ASSERT( FIELD_OFFSET(struct enum_key_request, index) == 16 );
C_ASSERT( FIELD_OFFSET(struct enum_key_request, info_class) == 20 );
C_ASSERT( sizeof(struct enum_key_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct enum_key_reply, subkeys) == 8 );
C_ASSERT( FIELD_OFFSET(struct enum_key_reply, max_subkey) == 12 );
C_ASSERT( FIELD_OFFSET(struct enum_key_reply, max_class) == 16 );
C_ASSERT( FIELD_OFFSET(struct enum_key_reply, values) == 20 );
C_ASSERT( FIELD_OFFSET(struct enum_key_reply, max_value) == 24 );
C_ASSERT( FIELD_OFFSET(struct enum_key_reply, max_data) == 28 );
C_ASSERT( FIELD_OFFSET(struct enum_key_reply, modif) == 32 );
C_ASSERT( FIELD_OFFSET(struct enum_key_reply, total) == 40 );
C_ASSERT( FIELD_OFFSET(struct enum_key_reply, namelen) == 44 );
C_ASSERT( sizeof(struct enum_key_reply) == 48 );
C_ASSERT( FIELD_OFFSET(struct set_key_value_request, hkey) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_key_value_request, type) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_key_value_request, namelen) == 20 );
C_ASSERT( sizeof(struct set_key_value_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_key_value_request, hkey) == 12 );
C_ASSERT( sizeof(struct get_key_value_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_key_value_reply, type) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_key_value_reply, total) == 12 );
C_ASSERT( sizeof(struct get_key_value_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct enum_key_value_request, hkey) == 12 );
C_ASSERT( FIELD_OFFSET(struct enum_key_value_request, index) == 16 );
C_ASSERT( FIELD_OFFSET(struct enum_key_value_request, info_class) == 20 );
C_ASSERT( sizeof(struct enum_key_value_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct enum_key_value_reply, type) == 8 );
C_ASSERT( FIELD_OFFSET(struct enum_key_value_reply, total) == 12 );
C_ASSERT( FIELD_OFFSET(struct enum_key_value_reply, namelen) == 16 );
C_ASSERT( sizeof(struct enum_key_value_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct delete_key_value_request, hkey) == 12 );
C_ASSERT( sizeof(struct delete_key_value_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct load_registry_request, file) == 12 );
C_ASSERT( sizeof(struct load_registry_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct unload_registry_request, parent) == 12 );
C_ASSERT( FIELD_OFFSET(struct unload_registry_request, attributes) == 16 );
C_ASSERT( sizeof(struct unload_registry_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct save_registry_request, hkey) == 12 );
C_ASSERT( FIELD_OFFSET(struct save_registry_request, file) == 16 );
C_ASSERT( sizeof(struct save_registry_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_registry_notification_request, hkey) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_registry_notification_request, event) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_registry_notification_request, subtree) == 20 );
C_ASSERT( FIELD_OFFSET(struct set_registry_notification_request, filter) == 24 );
C_ASSERT( sizeof(struct set_registry_notification_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct create_timer_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct create_timer_request, manual) == 16 );
C_ASSERT( sizeof(struct create_timer_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct create_timer_reply, handle) == 8 );
C_ASSERT( sizeof(struct create_timer_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_timer_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct open_timer_request, attributes) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_timer_request, rootdir) == 20 );
C_ASSERT( sizeof(struct open_timer_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct open_timer_reply, handle) == 8 );
C_ASSERT( sizeof(struct open_timer_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_timer_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_timer_request, expire) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_timer_request, callback) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_timer_request, arg) == 32 );
C_ASSERT( FIELD_OFFSET(struct set_timer_request, period) == 40 );
C_ASSERT( sizeof(struct set_timer_request) == 48 );
C_ASSERT( FIELD_OFFSET(struct set_timer_reply, signaled) == 8 );
C_ASSERT( sizeof(struct set_timer_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct cancel_timer_request, handle) == 12 );
C_ASSERT( sizeof(struct cancel_timer_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct cancel_timer_reply, signaled) == 8 );
C_ASSERT( sizeof(struct cancel_timer_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_timer_info_request, handle) == 12 );
C_ASSERT( sizeof(struct get_timer_info_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_timer_info_reply, when) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_timer_info_reply, signaled) == 16 );
C_ASSERT( sizeof(struct get_timer_info_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_thread_context_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_thread_context_request, context) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_thread_context_request, flags) == 20 );
C_ASSERT( FIELD_OFFSET(struct get_thread_context_request, machine) == 24 );
C_ASSERT( sizeof(struct get_thread_context_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_thread_context_reply, self) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_thread_context_reply, handle) == 12 );
C_ASSERT( sizeof(struct get_thread_context_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_thread_context_request, handle) == 12 );
C_ASSERT( sizeof(struct set_thread_context_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_thread_context_reply, self) == 8 );
C_ASSERT( sizeof(struct set_thread_context_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_selector_entry_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_selector_entry_request, entry) == 16 );
C_ASSERT( sizeof(struct get_selector_entry_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_selector_entry_reply, base) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_selector_entry_reply, limit) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_selector_entry_reply, flags) == 16 );
C_ASSERT( sizeof(struct get_selector_entry_reply) == 24 );
C_ASSERT( sizeof(struct add_atom_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct add_atom_reply, atom) == 8 );
C_ASSERT( sizeof(struct add_atom_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct delete_atom_request, atom) == 12 );
C_ASSERT( sizeof(struct delete_atom_request) == 16 );
C_ASSERT( sizeof(struct find_atom_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct find_atom_reply, atom) == 8 );
C_ASSERT( sizeof(struct find_atom_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_atom_information_request, atom) == 12 );
C_ASSERT( sizeof(struct get_atom_information_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_atom_information_reply, count) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_atom_information_reply, pinned) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_atom_information_reply, total) == 16 );
C_ASSERT( sizeof(struct get_atom_information_reply) == 24 );
C_ASSERT( sizeof(struct get_msg_queue_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_msg_queue_reply, handle) == 8 );
C_ASSERT( sizeof(struct get_msg_queue_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_queue_fd_request, handle) == 12 );
C_ASSERT( sizeof(struct set_queue_fd_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_queue_mask_request, wake_mask) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_queue_mask_request, changed_mask) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_queue_mask_request, skip_wait) == 20 );
C_ASSERT( sizeof(struct set_queue_mask_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_queue_mask_reply, wake_bits) == 8 );
C_ASSERT( FIELD_OFFSET(struct set_queue_mask_reply, changed_bits) == 12 );
C_ASSERT( sizeof(struct set_queue_mask_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_queue_status_request, clear_bits) == 12 );
C_ASSERT( sizeof(struct get_queue_status_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_queue_status_reply, wake_bits) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_queue_status_reply, changed_bits) == 12 );
C_ASSERT( sizeof(struct get_queue_status_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_process_idle_event_request, handle) == 12 );
C_ASSERT( sizeof(struct get_process_idle_event_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_process_idle_event_reply, event) == 8 );
C_ASSERT( sizeof(struct get_process_idle_event_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct send_message_request, id) == 12 );
C_ASSERT( FIELD_OFFSET(struct send_message_request, type) == 16 );
C_ASSERT( FIELD_OFFSET(struct send_message_request, flags) == 20 );
C_ASSERT( FIELD_OFFSET(struct send_message_request, win) == 24 );
C_ASSERT( FIELD_OFFSET(struct send_message_request, msg) == 28 );
C_ASSERT( FIELD_OFFSET(struct send_message_request, wparam) == 32 );
C_ASSERT( FIELD_OFFSET(struct send_message_request, lparam) == 40 );
C_ASSERT( FIELD_OFFSET(struct send_message_request, timeout) == 48 );
C_ASSERT( sizeof(struct send_message_request) == 56 );
C_ASSERT( FIELD_OFFSET(struct post_quit_message_request, exit_code) == 12 );
C_ASSERT( sizeof(struct post_quit_message_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct send_hardware_message_request, win) == 12 );
C_ASSERT( FIELD_OFFSET(struct send_hardware_message_request, input) == 16 );
C_ASSERT( FIELD_OFFSET(struct send_hardware_message_request, flags) == 56 );
C_ASSERT( sizeof(struct send_hardware_message_request) == 64 );
C_ASSERT( FIELD_OFFSET(struct send_hardware_message_reply, wait) == 8 );
C_ASSERT( FIELD_OFFSET(struct send_hardware_message_reply, prev_x) == 12 );
C_ASSERT( FIELD_OFFSET(struct send_hardware_message_reply, prev_y) == 16 );
C_ASSERT( FIELD_OFFSET(struct send_hardware_message_reply, new_x) == 20 );
C_ASSERT( FIELD_OFFSET(struct send_hardware_message_reply, new_y) == 24 );
C_ASSERT( sizeof(struct send_hardware_message_reply) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_message_request, flags) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_message_request, get_win) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_message_request, get_first) == 20 );
C_ASSERT( FIELD_OFFSET(struct get_message_request, get_last) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_message_request, hw_id) == 28 );
C_ASSERT( FIELD_OFFSET(struct get_message_request, wake_mask) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_message_request, changed_mask) == 36 );
C_ASSERT( sizeof(struct get_message_request) == 40 );
C_ASSERT( FIELD_OFFSET(struct get_message_reply, win) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_message_reply, msg) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_message_reply, wparam) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_message_reply, lparam) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_message_reply, type) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_message_reply, x) == 36 );
C_ASSERT( FIELD_OFFSET(struct get_message_reply, y) == 40 );
C_ASSERT( FIELD_OFFSET(struct get_message_reply, time) == 44 );
C_ASSERT( FIELD_OFFSET(struct get_message_reply, active_hooks) == 48 );
C_ASSERT( FIELD_OFFSET(struct get_message_reply, total) == 52 );
C_ASSERT( sizeof(struct get_message_reply) == 56 );
C_ASSERT( FIELD_OFFSET(struct reply_message_request, remove) == 12 );
C_ASSERT( FIELD_OFFSET(struct reply_message_request, result) == 16 );
C_ASSERT( sizeof(struct reply_message_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct accept_hardware_message_request, hw_id) == 12 );
C_ASSERT( sizeof(struct accept_hardware_message_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_message_reply_request, cancel) == 12 );
C_ASSERT( sizeof(struct get_message_reply_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_message_reply_reply, result) == 8 );
C_ASSERT( sizeof(struct get_message_reply_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_win_timer_request, win) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_win_timer_request, msg) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_win_timer_request, rate) == 20 );
C_ASSERT( FIELD_OFFSET(struct set_win_timer_request, id) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_win_timer_request, lparam) == 32 );
C_ASSERT( sizeof(struct set_win_timer_request) == 40 );
C_ASSERT( FIELD_OFFSET(struct set_win_timer_reply, id) == 8 );
C_ASSERT( sizeof(struct set_win_timer_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct kill_win_timer_request, win) == 12 );
C_ASSERT( FIELD_OFFSET(struct kill_win_timer_request, id) == 16 );
C_ASSERT( FIELD_OFFSET(struct kill_win_timer_request, msg) == 24 );
C_ASSERT( sizeof(struct kill_win_timer_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct is_window_hung_request, win) == 12 );
C_ASSERT( sizeof(struct is_window_hung_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct is_window_hung_reply, is_hung) == 8 );
C_ASSERT( sizeof(struct is_window_hung_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_serial_info_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_serial_info_request, flags) == 16 );
C_ASSERT( sizeof(struct get_serial_info_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_serial_info_reply, eventmask) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_serial_info_reply, cookie) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_serial_info_reply, pending_write) == 16 );
C_ASSERT( sizeof(struct get_serial_info_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_serial_info_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_serial_info_request, flags) == 16 );
C_ASSERT( sizeof(struct set_serial_info_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct register_async_request, type) == 12 );
C_ASSERT( FIELD_OFFSET(struct register_async_request, async) == 16 );
C_ASSERT( FIELD_OFFSET(struct register_async_request, count) == 56 );
C_ASSERT( sizeof(struct register_async_request) == 64 );
C_ASSERT( FIELD_OFFSET(struct cancel_async_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct cancel_async_request, iosb) == 16 );
C_ASSERT( FIELD_OFFSET(struct cancel_async_request, only_thread) == 24 );
C_ASSERT( sizeof(struct cancel_async_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_async_result_request, user_arg) == 16 );
C_ASSERT( sizeof(struct get_async_result_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_async_result_reply, size) == 8 );
C_ASSERT( sizeof(struct get_async_result_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct read_request, async) == 16 );
C_ASSERT( FIELD_OFFSET(struct read_request, pos) == 56 );
C_ASSERT( sizeof(struct read_request) == 64 );
C_ASSERT( FIELD_OFFSET(struct read_reply, wait) == 8 );
C_ASSERT( FIELD_OFFSET(struct read_reply, options) == 12 );
C_ASSERT( sizeof(struct read_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct write_request, async) == 16 );
C_ASSERT( FIELD_OFFSET(struct write_request, pos) == 56 );
C_ASSERT( sizeof(struct write_request) == 64 );
C_ASSERT( FIELD_OFFSET(struct write_reply, wait) == 8 );
C_ASSERT( FIELD_OFFSET(struct write_reply, options) == 12 );
C_ASSERT( FIELD_OFFSET(struct write_reply, size) == 16 );
C_ASSERT( sizeof(struct write_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct ioctl_request, code) == 12 );
C_ASSERT( FIELD_OFFSET(struct ioctl_request, async) == 16 );
C_ASSERT( sizeof(struct ioctl_request) == 56 );
C_ASSERT( FIELD_OFFSET(struct ioctl_reply, wait) == 8 );
C_ASSERT( FIELD_OFFSET(struct ioctl_reply, options) == 12 );
C_ASSERT( sizeof(struct ioctl_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_irp_result_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_irp_result_request, status) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_irp_result_request, size) == 20 );
C_ASSERT( sizeof(struct set_irp_result_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct create_named_pipe_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct create_named_pipe_request, options) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_named_pipe_request, sharing) == 20 );
C_ASSERT( FIELD_OFFSET(struct create_named_pipe_request, maxinstances) == 24 );
C_ASSERT( FIELD_OFFSET(struct create_named_pipe_request, outsize) == 28 );
C_ASSERT( FIELD_OFFSET(struct create_named_pipe_request, insize) == 32 );
C_ASSERT( FIELD_OFFSET(struct create_named_pipe_request, timeout) == 40 );
C_ASSERT( FIELD_OFFSET(struct create_named_pipe_request, flags) == 48 );
C_ASSERT( sizeof(struct create_named_pipe_request) == 56 );
C_ASSERT( FIELD_OFFSET(struct create_named_pipe_reply, handle) == 8 );
C_ASSERT( sizeof(struct create_named_pipe_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_named_pipe_info_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_named_pipe_info_request, flags) == 16 );
C_ASSERT( sizeof(struct set_named_pipe_info_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct create_window_request, parent) == 12 );
C_ASSERT( FIELD_OFFSET(struct create_window_request, owner) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_window_request, atom) == 20 );
C_ASSERT( FIELD_OFFSET(struct create_window_request, instance) == 24 );
C_ASSERT( FIELD_OFFSET(struct create_window_request, dpi) == 32 );
C_ASSERT( FIELD_OFFSET(struct create_window_request, awareness) == 36 );
C_ASSERT( sizeof(struct create_window_request) == 40 );
C_ASSERT( FIELD_OFFSET(struct create_window_reply, handle) == 8 );
C_ASSERT( FIELD_OFFSET(struct create_window_reply, parent) == 12 );
C_ASSERT( FIELD_OFFSET(struct create_window_reply, owner) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_window_reply, extra) == 20 );
C_ASSERT( FIELD_OFFSET(struct create_window_reply, class_ptr) == 24 );
C_ASSERT( FIELD_OFFSET(struct create_window_reply, dpi) == 32 );
C_ASSERT( FIELD_OFFSET(struct create_window_reply, awareness) == 36 );
C_ASSERT( sizeof(struct create_window_reply) == 40 );
C_ASSERT( FIELD_OFFSET(struct destroy_window_request, handle) == 12 );
C_ASSERT( sizeof(struct destroy_window_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_desktop_window_request, force) == 12 );
C_ASSERT( sizeof(struct get_desktop_window_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_desktop_window_reply, top_window) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_desktop_window_reply, msg_window) == 12 );
C_ASSERT( sizeof(struct get_desktop_window_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_window_owner_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_window_owner_request, owner) == 16 );
C_ASSERT( sizeof(struct set_window_owner_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_window_owner_reply, full_owner) == 8 );
C_ASSERT( FIELD_OFFSET(struct set_window_owner_reply, prev_owner) == 12 );
C_ASSERT( sizeof(struct set_window_owner_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_window_info_request, handle) == 12 );
C_ASSERT( sizeof(struct get_window_info_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_window_info_reply, full_handle) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_window_info_reply, last_active) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_window_info_reply, pid) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_window_info_reply, tid) == 20 );
C_ASSERT( FIELD_OFFSET(struct get_window_info_reply, atom) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_window_info_reply, is_unicode) == 28 );
C_ASSERT( FIELD_OFFSET(struct get_window_info_reply, dpi) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_window_info_reply, awareness) == 36 );
C_ASSERT( sizeof(struct get_window_info_reply) == 40 );
C_ASSERT( FIELD_OFFSET(struct set_window_info_request, flags) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_window_info_request, is_unicode) == 14 );
C_ASSERT( FIELD_OFFSET(struct set_window_info_request, handle) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_window_info_request, style) == 20 );
C_ASSERT( FIELD_OFFSET(struct set_window_info_request, ex_style) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_window_info_request, id) == 28 );
C_ASSERT( FIELD_OFFSET(struct set_window_info_request, instance) == 32 );
C_ASSERT( FIELD_OFFSET(struct set_window_info_request, user_data) == 40 );
C_ASSERT( FIELD_OFFSET(struct set_window_info_request, extra_offset) == 48 );
C_ASSERT( FIELD_OFFSET(struct set_window_info_request, extra_size) == 52 );
C_ASSERT( FIELD_OFFSET(struct set_window_info_request, extra_value) == 56 );
C_ASSERT( sizeof(struct set_window_info_request) == 64 );
C_ASSERT( FIELD_OFFSET(struct set_window_info_reply, old_style) == 8 );
C_ASSERT( FIELD_OFFSET(struct set_window_info_reply, old_ex_style) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_window_info_reply, old_instance) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_window_info_reply, old_user_data) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_window_info_reply, old_extra_value) == 32 );
C_ASSERT( FIELD_OFFSET(struct set_window_info_reply, old_id) == 40 );
C_ASSERT( sizeof(struct set_window_info_reply) == 48 );
C_ASSERT( FIELD_OFFSET(struct set_parent_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_parent_request, parent) == 16 );
C_ASSERT( sizeof(struct set_parent_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_parent_reply, old_parent) == 8 );
C_ASSERT( FIELD_OFFSET(struct set_parent_reply, full_parent) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_parent_reply, dpi) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_parent_reply, awareness) == 20 );
C_ASSERT( sizeof(struct set_parent_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_window_parents_request, handle) == 12 );
C_ASSERT( sizeof(struct get_window_parents_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_window_parents_reply, count) == 8 );
C_ASSERT( sizeof(struct get_window_parents_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_window_children_request, desktop) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_window_children_request, parent) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_window_children_request, atom) == 20 );
C_ASSERT( FIELD_OFFSET(struct get_window_children_request, tid) == 24 );
C_ASSERT( sizeof(struct get_window_children_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_window_children_reply, count) == 8 );
C_ASSERT( sizeof(struct get_window_children_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_window_children_from_point_request, parent) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_window_children_from_point_request, x) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_window_children_from_point_request, y) == 20 );
C_ASSERT( FIELD_OFFSET(struct get_window_children_from_point_request, dpi) == 24 );
C_ASSERT( sizeof(struct get_window_children_from_point_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_window_children_from_point_reply, count) == 8 );
C_ASSERT( sizeof(struct get_window_children_from_point_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_window_tree_request, handle) == 12 );
C_ASSERT( sizeof(struct get_window_tree_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_window_tree_reply, parent) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_window_tree_reply, owner) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_window_tree_reply, next_sibling) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_window_tree_reply, prev_sibling) == 20 );
C_ASSERT( FIELD_OFFSET(struct get_window_tree_reply, first_sibling) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_window_tree_reply, last_sibling) == 28 );
C_ASSERT( FIELD_OFFSET(struct get_window_tree_reply, first_child) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_window_tree_reply, last_child) == 36 );
C_ASSERT( sizeof(struct get_window_tree_reply) == 40 );
C_ASSERT( FIELD_OFFSET(struct set_window_pos_request, swp_flags) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_window_pos_request, paint_flags) == 14 );
C_ASSERT( FIELD_OFFSET(struct set_window_pos_request, handle) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_window_pos_request, previous) == 20 );
C_ASSERT( FIELD_OFFSET(struct set_window_pos_request, window) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_window_pos_request, client) == 40 );
C_ASSERT( sizeof(struct set_window_pos_request) == 56 );
C_ASSERT( FIELD_OFFSET(struct set_window_pos_reply, new_style) == 8 );
C_ASSERT( FIELD_OFFSET(struct set_window_pos_reply, new_ex_style) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_window_pos_reply, surface_win) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_window_pos_reply, needs_update) == 20 );
C_ASSERT( sizeof(struct set_window_pos_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_window_rectangles_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_window_rectangles_request, relative) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_window_rectangles_request, dpi) == 20 );
C_ASSERT( sizeof(struct get_window_rectangles_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_window_rectangles_reply, window) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_window_rectangles_reply, client) == 24 );
C_ASSERT( sizeof(struct get_window_rectangles_reply) == 40 );
C_ASSERT( FIELD_OFFSET(struct get_window_text_request, handle) == 12 );
C_ASSERT( sizeof(struct get_window_text_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_window_text_reply, length) == 8 );
C_ASSERT( sizeof(struct get_window_text_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_window_text_request, handle) == 12 );
C_ASSERT( sizeof(struct set_window_text_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_windows_offset_request, from) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_windows_offset_request, to) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_windows_offset_request, dpi) == 20 );
C_ASSERT( sizeof(struct get_windows_offset_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_windows_offset_reply, x) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_windows_offset_reply, y) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_windows_offset_reply, mirror) == 16 );
C_ASSERT( sizeof(struct get_windows_offset_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_visible_region_request, window) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_visible_region_request, flags) == 16 );
C_ASSERT( sizeof(struct get_visible_region_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_visible_region_reply, top_win) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_visible_region_reply, top_rect) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_visible_region_reply, win_rect) == 28 );
C_ASSERT( FIELD_OFFSET(struct get_visible_region_reply, paint_flags) == 44 );
C_ASSERT( FIELD_OFFSET(struct get_visible_region_reply, total_size) == 48 );
C_ASSERT( sizeof(struct get_visible_region_reply) == 56 );
C_ASSERT( FIELD_OFFSET(struct get_surface_region_request, window) == 12 );
C_ASSERT( sizeof(struct get_surface_region_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_surface_region_reply, visible_rect) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_surface_region_reply, total_size) == 24 );
C_ASSERT( sizeof(struct get_surface_region_reply) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_window_region_request, window) == 12 );
C_ASSERT( sizeof(struct get_window_region_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_window_region_reply, total_size) == 8 );
C_ASSERT( sizeof(struct get_window_region_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_window_region_request, window) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_window_region_request, redraw) == 16 );
C_ASSERT( sizeof(struct set_window_region_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_update_region_request, window) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_update_region_request, from_child) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_update_region_request, flags) == 20 );
C_ASSERT( sizeof(struct get_update_region_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_update_region_reply, child) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_update_region_reply, flags) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_update_region_reply, total_size) == 16 );
C_ASSERT( sizeof(struct get_update_region_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct update_window_zorder_request, window) == 12 );
C_ASSERT( FIELD_OFFSET(struct update_window_zorder_request, rect) == 16 );
C_ASSERT( sizeof(struct update_window_zorder_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct redraw_window_request, window) == 12 );
C_ASSERT( FIELD_OFFSET(struct redraw_window_request, flags) == 16 );
C_ASSERT( sizeof(struct redraw_window_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_window_property_request, window) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_window_property_request, data) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_window_property_request, atom) == 24 );
C_ASSERT( sizeof(struct set_window_property_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct remove_window_property_request, window) == 12 );
C_ASSERT( FIELD_OFFSET(struct remove_window_property_request, atom) == 16 );
C_ASSERT( sizeof(struct remove_window_property_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct remove_window_property_reply, data) == 8 );
C_ASSERT( sizeof(struct remove_window_property_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_window_property_request, window) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_window_property_request, atom) == 16 );
C_ASSERT( sizeof(struct get_window_property_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_window_property_reply, data) == 8 );
C_ASSERT( sizeof(struct get_window_property_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_window_properties_request, window) == 12 );
C_ASSERT( sizeof(struct get_window_properties_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_window_properties_reply, total) == 8 );
C_ASSERT( sizeof(struct get_window_properties_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_winstation_request, flags) == 12 );
C_ASSERT( FIELD_OFFSET(struct create_winstation_request, access) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_winstation_request, attributes) == 20 );
C_ASSERT( FIELD_OFFSET(struct create_winstation_request, rootdir) == 24 );
C_ASSERT( sizeof(struct create_winstation_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct create_winstation_reply, handle) == 8 );
C_ASSERT( sizeof(struct create_winstation_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_winstation_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct open_winstation_request, attributes) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_winstation_request, rootdir) == 20 );
C_ASSERT( sizeof(struct open_winstation_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct open_winstation_reply, handle) == 8 );
C_ASSERT( sizeof(struct open_winstation_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct close_winstation_request, handle) == 12 );
C_ASSERT( sizeof(struct close_winstation_request) == 16 );
C_ASSERT( sizeof(struct get_process_winstation_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_process_winstation_reply, handle) == 8 );
C_ASSERT( sizeof(struct get_process_winstation_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_process_winstation_request, handle) == 12 );
C_ASSERT( sizeof(struct set_process_winstation_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct enum_winstation_request, index) == 12 );
C_ASSERT( sizeof(struct enum_winstation_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct enum_winstation_reply, next) == 8 );
C_ASSERT( sizeof(struct enum_winstation_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_desktop_request, flags) == 12 );
C_ASSERT( FIELD_OFFSET(struct create_desktop_request, access) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_desktop_request, attributes) == 20 );
C_ASSERT( sizeof(struct create_desktop_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct create_desktop_reply, handle) == 8 );
C_ASSERT( sizeof(struct create_desktop_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_desktop_request, winsta) == 12 );
C_ASSERT( FIELD_OFFSET(struct open_desktop_request, flags) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_desktop_request, access) == 20 );
C_ASSERT( FIELD_OFFSET(struct open_desktop_request, attributes) == 24 );
C_ASSERT( sizeof(struct open_desktop_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct open_desktop_reply, handle) == 8 );
C_ASSERT( sizeof(struct open_desktop_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_input_desktop_request, flags) == 12 );
C_ASSERT( FIELD_OFFSET(struct open_input_desktop_request, access) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_input_desktop_request, attributes) == 20 );
C_ASSERT( sizeof(struct open_input_desktop_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct open_input_desktop_reply, handle) == 8 );
C_ASSERT( sizeof(struct open_input_desktop_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct close_desktop_request, handle) == 12 );
C_ASSERT( sizeof(struct close_desktop_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_thread_desktop_request, tid) == 12 );
C_ASSERT( sizeof(struct get_thread_desktop_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_thread_desktop_reply, handle) == 8 );
C_ASSERT( sizeof(struct get_thread_desktop_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_thread_desktop_request, handle) == 12 );
C_ASSERT( sizeof(struct set_thread_desktop_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct enum_desktop_request, winstation) == 12 );
C_ASSERT( FIELD_OFFSET(struct enum_desktop_request, index) == 16 );
C_ASSERT( sizeof(struct enum_desktop_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct enum_desktop_reply, next) == 8 );
C_ASSERT( sizeof(struct enum_desktop_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_user_object_info_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_user_object_info_request, flags) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_user_object_info_request, obj_flags) == 20 );
C_ASSERT( sizeof(struct set_user_object_info_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_user_object_info_reply, is_desktop) == 8 );
C_ASSERT( FIELD_OFFSET(struct set_user_object_info_reply, old_obj_flags) == 12 );
C_ASSERT( sizeof(struct set_user_object_info_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct register_hotkey_request, window) == 12 );
C_ASSERT( FIELD_OFFSET(struct register_hotkey_request, id) == 16 );
C_ASSERT( FIELD_OFFSET(struct register_hotkey_request, flags) == 20 );
C_ASSERT( FIELD_OFFSET(struct register_hotkey_request, vkey) == 24 );
C_ASSERT( sizeof(struct register_hotkey_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct register_hotkey_reply, replaced) == 8 );
C_ASSERT( FIELD_OFFSET(struct register_hotkey_reply, flags) == 12 );
C_ASSERT( FIELD_OFFSET(struct register_hotkey_reply, vkey) == 16 );
C_ASSERT( sizeof(struct register_hotkey_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct unregister_hotkey_request, window) == 12 );
C_ASSERT( FIELD_OFFSET(struct unregister_hotkey_request, id) == 16 );
C_ASSERT( sizeof(struct unregister_hotkey_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct unregister_hotkey_reply, flags) == 8 );
C_ASSERT( FIELD_OFFSET(struct unregister_hotkey_reply, vkey) == 12 );
C_ASSERT( sizeof(struct unregister_hotkey_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct attach_thread_input_request, tid_from) == 12 );
C_ASSERT( FIELD_OFFSET(struct attach_thread_input_request, tid_to) == 16 );
C_ASSERT( FIELD_OFFSET(struct attach_thread_input_request, attach) == 20 );
C_ASSERT( sizeof(struct attach_thread_input_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_thread_input_request, tid) == 12 );
C_ASSERT( sizeof(struct get_thread_input_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_thread_input_reply, focus) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_thread_input_reply, capture) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_thread_input_reply, active) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_thread_input_reply, foreground) == 20 );
C_ASSERT( FIELD_OFFSET(struct get_thread_input_reply, menu_owner) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_thread_input_reply, move_size) == 28 );
C_ASSERT( FIELD_OFFSET(struct get_thread_input_reply, caret) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_thread_input_reply, cursor) == 36 );
C_ASSERT( FIELD_OFFSET(struct get_thread_input_reply, show_count) == 40 );
C_ASSERT( FIELD_OFFSET(struct get_thread_input_reply, rect) == 44 );
C_ASSERT( sizeof(struct get_thread_input_reply) == 64 );
C_ASSERT( sizeof(struct get_last_input_time_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_last_input_time_reply, time) == 8 );
C_ASSERT( sizeof(struct get_last_input_time_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_key_state_request, async) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_key_state_request, key) == 16 );
C_ASSERT( sizeof(struct get_key_state_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_key_state_reply, state) == 8 );
C_ASSERT( sizeof(struct get_key_state_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_key_state_request, async) == 12 );
C_ASSERT( sizeof(struct set_key_state_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_foreground_window_request, handle) == 12 );
C_ASSERT( sizeof(struct set_foreground_window_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_foreground_window_reply, previous) == 8 );
C_ASSERT( FIELD_OFFSET(struct set_foreground_window_reply, send_msg_old) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_foreground_window_reply, send_msg_new) == 16 );
C_ASSERT( sizeof(struct set_foreground_window_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_focus_window_request, handle) == 12 );
C_ASSERT( sizeof(struct set_focus_window_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_focus_window_reply, previous) == 8 );
C_ASSERT( sizeof(struct set_focus_window_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_active_window_request, handle) == 12 );
C_ASSERT( sizeof(struct set_active_window_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_active_window_reply, previous) == 8 );
C_ASSERT( sizeof(struct set_active_window_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_capture_window_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_capture_window_request, flags) == 16 );
C_ASSERT( sizeof(struct set_capture_window_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_capture_window_reply, previous) == 8 );
C_ASSERT( FIELD_OFFSET(struct set_capture_window_reply, full_handle) == 12 );
C_ASSERT( sizeof(struct set_capture_window_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_caret_window_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_caret_window_request, width) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_caret_window_request, height) == 20 );
C_ASSERT( sizeof(struct set_caret_window_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_caret_window_reply, previous) == 8 );
C_ASSERT( FIELD_OFFSET(struct set_caret_window_reply, old_rect) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_caret_window_reply, old_hide) == 28 );
C_ASSERT( FIELD_OFFSET(struct set_caret_window_reply, old_state) == 32 );
C_ASSERT( sizeof(struct set_caret_window_reply) == 40 );
C_ASSERT( FIELD_OFFSET(struct set_caret_info_request, flags) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_caret_info_request, handle) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_caret_info_request, x) == 20 );
C_ASSERT( FIELD_OFFSET(struct set_caret_info_request, y) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_caret_info_request, hide) == 28 );
C_ASSERT( FIELD_OFFSET(struct set_caret_info_request, state) == 32 );
C_ASSERT( sizeof(struct set_caret_info_request) == 40 );
C_ASSERT( FIELD_OFFSET(struct set_caret_info_reply, full_handle) == 8 );
C_ASSERT( FIELD_OFFSET(struct set_caret_info_reply, old_rect) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_caret_info_reply, old_hide) == 28 );
C_ASSERT( FIELD_OFFSET(struct set_caret_info_reply, old_state) == 32 );
C_ASSERT( sizeof(struct set_caret_info_reply) == 40 );
C_ASSERT( FIELD_OFFSET(struct set_hook_request, id) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_hook_request, pid) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_hook_request, tid) == 20 );
C_ASSERT( FIELD_OFFSET(struct set_hook_request, event_min) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_hook_request, event_max) == 28 );
C_ASSERT( FIELD_OFFSET(struct set_hook_request, proc) == 32 );
C_ASSERT( FIELD_OFFSET(struct set_hook_request, flags) == 40 );
C_ASSERT( FIELD_OFFSET(struct set_hook_request, unicode) == 44 );
C_ASSERT( sizeof(struct set_hook_request) == 48 );
C_ASSERT( FIELD_OFFSET(struct set_hook_reply, handle) == 8 );
C_ASSERT( FIELD_OFFSET(struct set_hook_reply, active_hooks) == 12 );
C_ASSERT( sizeof(struct set_hook_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct remove_hook_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct remove_hook_request, proc) == 16 );
C_ASSERT( FIELD_OFFSET(struct remove_hook_request, id) == 24 );
C_ASSERT( sizeof(struct remove_hook_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct remove_hook_reply, active_hooks) == 8 );
C_ASSERT( sizeof(struct remove_hook_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct start_hook_chain_request, id) == 12 );
C_ASSERT( FIELD_OFFSET(struct start_hook_chain_request, event) == 16 );
C_ASSERT( FIELD_OFFSET(struct start_hook_chain_request, window) == 20 );
C_ASSERT( FIELD_OFFSET(struct start_hook_chain_request, object_id) == 24 );
C_ASSERT( FIELD_OFFSET(struct start_hook_chain_request, child_id) == 28 );
C_ASSERT( sizeof(struct start_hook_chain_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct start_hook_chain_reply, handle) == 8 );
C_ASSERT( FIELD_OFFSET(struct start_hook_chain_reply, pid) == 12 );
C_ASSERT( FIELD_OFFSET(struct start_hook_chain_reply, tid) == 16 );
C_ASSERT( FIELD_OFFSET(struct start_hook_chain_reply, unicode) == 20 );
C_ASSERT( FIELD_OFFSET(struct start_hook_chain_reply, proc) == 24 );
C_ASSERT( FIELD_OFFSET(struct start_hook_chain_reply, active_hooks) == 32 );
C_ASSERT( sizeof(struct start_hook_chain_reply) == 40 );
C_ASSERT( FIELD_OFFSET(struct finish_hook_chain_request, id) == 12 );
C_ASSERT( sizeof(struct finish_hook_chain_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_hook_info_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_hook_info_request, get_next) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_hook_info_request, event) == 20 );
C_ASSERT( FIELD_OFFSET(struct get_hook_info_request, window) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_hook_info_request, object_id) == 28 );
C_ASSERT( FIELD_OFFSET(struct get_hook_info_request, child_id) == 32 );
C_ASSERT( sizeof(struct get_hook_info_request) == 40 );
C_ASSERT( FIELD_OFFSET(struct get_hook_info_reply, handle) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_hook_info_reply, id) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_hook_info_reply, pid) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_hook_info_reply, tid) == 20 );
C_ASSERT( FIELD_OFFSET(struct get_hook_info_reply, proc) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_hook_info_reply, unicode) == 32 );
C_ASSERT( sizeof(struct get_hook_info_reply) == 40 );
C_ASSERT( FIELD_OFFSET(struct create_class_request, local) == 12 );
C_ASSERT( FIELD_OFFSET(struct create_class_request, atom) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_class_request, style) == 20 );
C_ASSERT( FIELD_OFFSET(struct create_class_request, instance) == 24 );
C_ASSERT( FIELD_OFFSET(struct create_class_request, extra) == 32 );
C_ASSERT( FIELD_OFFSET(struct create_class_request, win_extra) == 36 );
C_ASSERT( FIELD_OFFSET(struct create_class_request, client_ptr) == 40 );
C_ASSERT( FIELD_OFFSET(struct create_class_request, name_offset) == 48 );
C_ASSERT( sizeof(struct create_class_request) == 56 );
C_ASSERT( FIELD_OFFSET(struct create_class_reply, atom) == 8 );
C_ASSERT( sizeof(struct create_class_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct destroy_class_request, atom) == 12 );
C_ASSERT( FIELD_OFFSET(struct destroy_class_request, instance) == 16 );
C_ASSERT( sizeof(struct destroy_class_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct destroy_class_reply, client_ptr) == 8 );
C_ASSERT( sizeof(struct destroy_class_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_class_info_request, window) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_class_info_request, flags) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_class_info_request, atom) == 20 );
C_ASSERT( FIELD_OFFSET(struct set_class_info_request, style) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_class_info_request, win_extra) == 28 );
C_ASSERT( FIELD_OFFSET(struct set_class_info_request, instance) == 32 );
C_ASSERT( FIELD_OFFSET(struct set_class_info_request, extra_offset) == 40 );
C_ASSERT( FIELD_OFFSET(struct set_class_info_request, extra_size) == 44 );
C_ASSERT( FIELD_OFFSET(struct set_class_info_request, extra_value) == 48 );
C_ASSERT( sizeof(struct set_class_info_request) == 56 );
C_ASSERT( FIELD_OFFSET(struct set_class_info_reply, old_atom) == 8 );
C_ASSERT( FIELD_OFFSET(struct set_class_info_reply, base_atom) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_class_info_reply, old_instance) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_class_info_reply, old_extra_value) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_class_info_reply, old_style) == 32 );
C_ASSERT( FIELD_OFFSET(struct set_class_info_reply, old_extra) == 36 );
C_ASSERT( FIELD_OFFSET(struct set_class_info_reply, old_win_extra) == 40 );
C_ASSERT( sizeof(struct set_class_info_reply) == 48 );
C_ASSERT( FIELD_OFFSET(struct open_clipboard_request, window) == 12 );
C_ASSERT( sizeof(struct open_clipboard_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_clipboard_reply, owner) == 8 );
C_ASSERT( sizeof(struct open_clipboard_reply) == 16 );
C_ASSERT( sizeof(struct close_clipboard_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct close_clipboard_reply, viewer) == 8 );
C_ASSERT( FIELD_OFFSET(struct close_clipboard_reply, owner) == 12 );
C_ASSERT( sizeof(struct close_clipboard_reply) == 16 );
C_ASSERT( sizeof(struct empty_clipboard_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_clipboard_data_request, format) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_clipboard_data_request, lcid) == 16 );
C_ASSERT( sizeof(struct set_clipboard_data_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_clipboard_data_reply, seqno) == 8 );
C_ASSERT( sizeof(struct set_clipboard_data_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_clipboard_data_request, format) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_clipboard_data_request, render) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_clipboard_data_request, cached) == 20 );
C_ASSERT( FIELD_OFFSET(struct get_clipboard_data_request, seqno) == 24 );
C_ASSERT( sizeof(struct get_clipboard_data_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_clipboard_data_reply, from) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_clipboard_data_reply, owner) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_clipboard_data_reply, seqno) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_clipboard_data_reply, total) == 20 );
C_ASSERT( sizeof(struct get_clipboard_data_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_clipboard_formats_request, format) == 12 );
C_ASSERT( sizeof(struct get_clipboard_formats_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_clipboard_formats_reply, count) == 8 );
C_ASSERT( sizeof(struct get_clipboard_formats_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct enum_clipboard_formats_request, previous) == 12 );
C_ASSERT( sizeof(struct enum_clipboard_formats_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct enum_clipboard_formats_reply, format) == 8 );
C_ASSERT( sizeof(struct enum_clipboard_formats_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct release_clipboard_request, owner) == 12 );
C_ASSERT( sizeof(struct release_clipboard_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct release_clipboard_reply, viewer) == 8 );
C_ASSERT( FIELD_OFFSET(struct release_clipboard_reply, owner) == 12 );
C_ASSERT( sizeof(struct release_clipboard_reply) == 16 );
C_ASSERT( sizeof(struct get_clipboard_info_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_clipboard_info_reply, window) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_clipboard_info_reply, owner) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_clipboard_info_reply, viewer) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_clipboard_info_reply, seqno) == 20 );
C_ASSERT( sizeof(struct get_clipboard_info_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_clipboard_viewer_request, viewer) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_clipboard_viewer_request, previous) == 16 );
C_ASSERT( sizeof(struct set_clipboard_viewer_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_clipboard_viewer_reply, old_viewer) == 8 );
C_ASSERT( FIELD_OFFSET(struct set_clipboard_viewer_reply, owner) == 12 );
C_ASSERT( sizeof(struct set_clipboard_viewer_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct add_clipboard_listener_request, window) == 12 );
C_ASSERT( sizeof(struct add_clipboard_listener_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct remove_clipboard_listener_request, window) == 12 );
C_ASSERT( sizeof(struct remove_clipboard_listener_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_token_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct open_token_request, access) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_token_request, attributes) == 20 );
C_ASSERT( FIELD_OFFSET(struct open_token_request, flags) == 24 );
C_ASSERT( sizeof(struct open_token_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct open_token_reply, token) == 8 );
C_ASSERT( sizeof(struct open_token_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_global_windows_request, flags) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_global_windows_request, shell_window) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_global_windows_request, shell_listview) == 20 );
C_ASSERT( FIELD_OFFSET(struct set_global_windows_request, progman_window) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_global_windows_request, taskman_window) == 28 );
C_ASSERT( sizeof(struct set_global_windows_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct set_global_windows_reply, old_shell_window) == 8 );
C_ASSERT( FIELD_OFFSET(struct set_global_windows_reply, old_shell_listview) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_global_windows_reply, old_progman_window) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_global_windows_reply, old_taskman_window) == 20 );
C_ASSERT( sizeof(struct set_global_windows_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct adjust_token_privileges_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct adjust_token_privileges_request, disable_all) == 16 );
C_ASSERT( FIELD_OFFSET(struct adjust_token_privileges_request, get_modified_state) == 20 );
C_ASSERT( sizeof(struct adjust_token_privileges_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct adjust_token_privileges_reply, len) == 8 );
C_ASSERT( sizeof(struct adjust_token_privileges_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_token_privileges_request, handle) == 12 );
C_ASSERT( sizeof(struct get_token_privileges_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_token_privileges_reply, len) == 8 );
C_ASSERT( sizeof(struct get_token_privileges_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct check_token_privileges_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct check_token_privileges_request, all_required) == 16 );
C_ASSERT( sizeof(struct check_token_privileges_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct check_token_privileges_reply, has_privileges) == 8 );
C_ASSERT( sizeof(struct check_token_privileges_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct duplicate_token_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct duplicate_token_request, access) == 16 );
C_ASSERT( FIELD_OFFSET(struct duplicate_token_request, primary) == 20 );
C_ASSERT( FIELD_OFFSET(struct duplicate_token_request, impersonation_level) == 24 );
C_ASSERT( sizeof(struct duplicate_token_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct duplicate_token_reply, new_handle) == 8 );
C_ASSERT( sizeof(struct duplicate_token_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct filter_token_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct filter_token_request, flags) == 16 );
C_ASSERT( FIELD_OFFSET(struct filter_token_request, privileges_size) == 20 );
C_ASSERT( sizeof(struct filter_token_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct filter_token_reply, new_handle) == 8 );
C_ASSERT( sizeof(struct filter_token_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct access_check_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct access_check_request, desired_access) == 16 );
C_ASSERT( FIELD_OFFSET(struct access_check_request, mapping) == 20 );
C_ASSERT( sizeof(struct access_check_request) == 40 );
C_ASSERT( FIELD_OFFSET(struct access_check_reply, access_granted) == 8 );
C_ASSERT( FIELD_OFFSET(struct access_check_reply, access_status) == 12 );
C_ASSERT( FIELD_OFFSET(struct access_check_reply, privileges_len) == 16 );
C_ASSERT( sizeof(struct access_check_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_token_sid_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_token_sid_request, which_sid) == 16 );
C_ASSERT( sizeof(struct get_token_sid_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_token_sid_reply, sid_len) == 8 );
C_ASSERT( sizeof(struct get_token_sid_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_token_groups_request, handle) == 12 );
C_ASSERT( sizeof(struct get_token_groups_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_token_groups_reply, user_len) == 8 );
C_ASSERT( sizeof(struct get_token_groups_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_token_default_dacl_request, handle) == 12 );
C_ASSERT( sizeof(struct get_token_default_dacl_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_token_default_dacl_reply, acl_len) == 8 );
C_ASSERT( sizeof(struct get_token_default_dacl_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_token_default_dacl_request, handle) == 12 );
C_ASSERT( sizeof(struct set_token_default_dacl_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_security_object_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_security_object_request, security_info) == 16 );
C_ASSERT( sizeof(struct set_security_object_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_security_object_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_security_object_request, security_info) == 16 );
C_ASSERT( sizeof(struct get_security_object_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_security_object_reply, sd_len) == 8 );
C_ASSERT( sizeof(struct get_security_object_reply) == 16 );
C_ASSERT( sizeof(struct get_system_handles_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_system_handles_reply, count) == 8 );
C_ASSERT( sizeof(struct get_system_handles_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_mailslot_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct create_mailslot_request, read_timeout) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_mailslot_request, max_msgsize) == 24 );
C_ASSERT( sizeof(struct create_mailslot_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct create_mailslot_reply, handle) == 8 );
C_ASSERT( sizeof(struct create_mailslot_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_mailslot_info_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_mailslot_info_request, read_timeout) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_mailslot_info_request, flags) == 24 );
C_ASSERT( sizeof(struct set_mailslot_info_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct set_mailslot_info_reply, read_timeout) == 8 );
C_ASSERT( FIELD_OFFSET(struct set_mailslot_info_reply, max_msgsize) == 16 );
C_ASSERT( sizeof(struct set_mailslot_info_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct create_directory_request, access) == 12 );
C_ASSERT( sizeof(struct create_directory_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_directory_reply, handle) == 8 );
C_ASSERT( sizeof(struct create_directory_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_directory_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct open_directory_request, attributes) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_directory_request, rootdir) == 20 );
C_ASSERT( sizeof(struct open_directory_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct open_directory_reply, handle) == 8 );
C_ASSERT( sizeof(struct open_directory_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_directory_entry_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_directory_entry_request, index) == 16 );
C_ASSERT( sizeof(struct get_directory_entry_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_directory_entry_reply, name_len) == 8 );
C_ASSERT( sizeof(struct get_directory_entry_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_symlink_request, access) == 12 );
C_ASSERT( sizeof(struct create_symlink_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_symlink_reply, handle) == 8 );
C_ASSERT( sizeof(struct create_symlink_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_symlink_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct open_symlink_request, attributes) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_symlink_request, rootdir) == 20 );
C_ASSERT( sizeof(struct open_symlink_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct open_symlink_reply, handle) == 8 );
C_ASSERT( sizeof(struct open_symlink_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct query_symlink_request, handle) == 12 );
C_ASSERT( sizeof(struct query_symlink_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct query_symlink_reply, total) == 8 );
C_ASSERT( sizeof(struct query_symlink_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_object_info_request, handle) == 12 );
C_ASSERT( sizeof(struct get_object_info_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_object_info_reply, access) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_object_info_reply, ref_count) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_object_info_reply, handle_count) == 16 );
C_ASSERT( sizeof(struct get_object_info_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_object_name_request, handle) == 12 );
C_ASSERT( sizeof(struct get_object_name_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_object_name_reply, total) == 8 );
C_ASSERT( sizeof(struct get_object_name_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_object_type_request, handle) == 12 );
C_ASSERT( sizeof(struct get_object_type_request) == 16 );
C_ASSERT( sizeof(struct get_object_type_reply) == 8 );
C_ASSERT( sizeof(struct get_object_types_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_object_types_reply, count) == 8 );
C_ASSERT( sizeof(struct get_object_types_reply) == 16 );
C_ASSERT( sizeof(struct allocate_locally_unique_id_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct allocate_locally_unique_id_reply, luid) == 8 );
C_ASSERT( sizeof(struct allocate_locally_unique_id_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_device_manager_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct create_device_manager_request, attributes) == 16 );
C_ASSERT( sizeof(struct create_device_manager_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct create_device_manager_reply, handle) == 8 );
C_ASSERT( sizeof(struct create_device_manager_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_device_request, rootdir) == 12 );
C_ASSERT( FIELD_OFFSET(struct create_device_request, user_ptr) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_device_request, manager) == 24 );
C_ASSERT( sizeof(struct create_device_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct delete_device_request, manager) == 12 );
C_ASSERT( FIELD_OFFSET(struct delete_device_request, device) == 16 );
C_ASSERT( sizeof(struct delete_device_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_next_device_request_request, manager) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_next_device_request_request, prev) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_next_device_request_request, status) == 20 );
C_ASSERT( FIELD_OFFSET(struct get_next_device_request_request, user_ptr) == 24 );
C_ASSERT( sizeof(struct get_next_device_request_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_next_device_request_reply, params) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_next_device_request_reply, next) == 40 );
C_ASSERT( FIELD_OFFSET(struct get_next_device_request_reply, client_tid) == 44 );
C_ASSERT( FIELD_OFFSET(struct get_next_device_request_reply, client_thread) == 48 );
C_ASSERT( FIELD_OFFSET(struct get_next_device_request_reply, in_size) == 56 );
C_ASSERT( sizeof(struct get_next_device_request_reply) == 64 );
C_ASSERT( FIELD_OFFSET(struct get_kernel_object_ptr_request, manager) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_kernel_object_ptr_request, handle) == 16 );
C_ASSERT( sizeof(struct get_kernel_object_ptr_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_kernel_object_ptr_reply, user_ptr) == 8 );
C_ASSERT( sizeof(struct get_kernel_object_ptr_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_kernel_object_ptr_request, manager) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_kernel_object_ptr_request, handle) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_kernel_object_ptr_request, user_ptr) == 24 );
C_ASSERT( sizeof(struct set_kernel_object_ptr_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct grab_kernel_object_request, manager) == 12 );
C_ASSERT( FIELD_OFFSET(struct grab_kernel_object_request, user_ptr) == 16 );
C_ASSERT( sizeof(struct grab_kernel_object_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct release_kernel_object_request, manager) == 12 );
C_ASSERT( FIELD_OFFSET(struct release_kernel_object_request, user_ptr) == 16 );
C_ASSERT( sizeof(struct release_kernel_object_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_kernel_object_handle_request, manager) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_kernel_object_handle_request, user_ptr) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_kernel_object_handle_request, access) == 24 );
C_ASSERT( sizeof(struct get_kernel_object_handle_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_kernel_object_handle_reply, handle) == 8 );
C_ASSERT( sizeof(struct get_kernel_object_handle_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct make_process_system_request, handle) == 12 );
C_ASSERT( sizeof(struct make_process_system_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct make_process_system_reply, event) == 8 );
C_ASSERT( sizeof(struct make_process_system_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_token_info_request, handle) == 12 );
C_ASSERT( sizeof(struct get_token_info_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_token_info_reply, token_id) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_token_info_reply, modified_id) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_token_info_reply, session_id) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_token_info_reply, primary) == 28 );
C_ASSERT( FIELD_OFFSET(struct get_token_info_reply, impersonation_level) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_token_info_reply, elevation) == 36 );
C_ASSERT( FIELD_OFFSET(struct get_token_info_reply, group_count) == 40 );
C_ASSERT( FIELD_OFFSET(struct get_token_info_reply, privilege_count) == 44 );
C_ASSERT( sizeof(struct get_token_info_reply) == 48 );
C_ASSERT( FIELD_OFFSET(struct create_linked_token_request, handle) == 12 );
C_ASSERT( sizeof(struct create_linked_token_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_linked_token_reply, linked) == 8 );
C_ASSERT( sizeof(struct create_linked_token_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_completion_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct create_completion_request, concurrent) == 16 );
C_ASSERT( sizeof(struct create_completion_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct create_completion_reply, handle) == 8 );
C_ASSERT( sizeof(struct create_completion_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_completion_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct open_completion_request, attributes) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_completion_request, rootdir) == 20 );
C_ASSERT( sizeof(struct open_completion_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct open_completion_reply, handle) == 8 );
C_ASSERT( sizeof(struct open_completion_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct add_completion_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct add_completion_request, ckey) == 16 );
C_ASSERT( FIELD_OFFSET(struct add_completion_request, cvalue) == 24 );
C_ASSERT( FIELD_OFFSET(struct add_completion_request, information) == 32 );
C_ASSERT( FIELD_OFFSET(struct add_completion_request, status) == 40 );
C_ASSERT( sizeof(struct add_completion_request) == 48 );
C_ASSERT( FIELD_OFFSET(struct remove_completion_request, handle) == 12 );
C_ASSERT( sizeof(struct remove_completion_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct remove_completion_reply, ckey) == 8 );
C_ASSERT( FIELD_OFFSET(struct remove_completion_reply, cvalue) == 16 );
C_ASSERT( FIELD_OFFSET(struct remove_completion_reply, information) == 24 );
C_ASSERT( FIELD_OFFSET(struct remove_completion_reply, status) == 32 );
C_ASSERT( sizeof(struct remove_completion_reply) == 40 );
C_ASSERT( FIELD_OFFSET(struct query_completion_request, handle) == 12 );
C_ASSERT( sizeof(struct query_completion_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct query_completion_reply, depth) == 8 );
C_ASSERT( sizeof(struct query_completion_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_completion_info_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_completion_info_request, ckey) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_completion_info_request, chandle) == 24 );
C_ASSERT( sizeof(struct set_completion_info_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct add_fd_completion_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct add_fd_completion_request, cvalue) == 16 );
C_ASSERT( FIELD_OFFSET(struct add_fd_completion_request, information) == 24 );
C_ASSERT( FIELD_OFFSET(struct add_fd_completion_request, status) == 32 );
C_ASSERT( FIELD_OFFSET(struct add_fd_completion_request, async) == 36 );
C_ASSERT( sizeof(struct add_fd_completion_request) == 40 );
C_ASSERT( FIELD_OFFSET(struct set_fd_completion_mode_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_fd_completion_mode_request, flags) == 16 );
C_ASSERT( sizeof(struct set_fd_completion_mode_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_fd_disp_info_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_fd_disp_info_request, unlink) == 16 );
C_ASSERT( sizeof(struct set_fd_disp_info_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_fd_name_info_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_fd_name_info_request, rootdir) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_fd_name_info_request, namelen) == 20 );
C_ASSERT( FIELD_OFFSET(struct set_fd_name_info_request, link) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_fd_name_info_request, replace) == 28 );
C_ASSERT( sizeof(struct set_fd_name_info_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct set_fd_eof_info_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_fd_eof_info_request, eof) == 16 );
C_ASSERT( sizeof(struct set_fd_eof_info_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_window_layered_info_request, handle) == 12 );
C_ASSERT( sizeof(struct get_window_layered_info_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_window_layered_info_reply, color_key) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_window_layered_info_reply, alpha) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_window_layered_info_reply, flags) == 16 );
C_ASSERT( sizeof(struct get_window_layered_info_reply) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_window_layered_info_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_window_layered_info_request, color_key) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_window_layered_info_request, alpha) == 20 );
C_ASSERT( FIELD_OFFSET(struct set_window_layered_info_request, flags) == 24 );
C_ASSERT( sizeof(struct set_window_layered_info_request) == 32 );
C_ASSERT( sizeof(struct alloc_user_handle_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct alloc_user_handle_reply, handle) == 8 );
C_ASSERT( sizeof(struct alloc_user_handle_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct free_user_handle_request, handle) == 12 );
C_ASSERT( sizeof(struct free_user_handle_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_cursor_request, flags) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_cursor_request, handle) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_cursor_request, show_count) == 20 );
C_ASSERT( FIELD_OFFSET(struct set_cursor_request, x) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_cursor_request, y) == 28 );
C_ASSERT( FIELD_OFFSET(struct set_cursor_request, clip) == 32 );
C_ASSERT( FIELD_OFFSET(struct set_cursor_request, clip_msg) == 48 );
C_ASSERT( sizeof(struct set_cursor_request) == 56 );
C_ASSERT( FIELD_OFFSET(struct set_cursor_reply, prev_handle) == 8 );
C_ASSERT( FIELD_OFFSET(struct set_cursor_reply, prev_count) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_cursor_reply, prev_x) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_cursor_reply, prev_y) == 20 );
C_ASSERT( FIELD_OFFSET(struct set_cursor_reply, new_x) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_cursor_reply, new_y) == 28 );
C_ASSERT( FIELD_OFFSET(struct set_cursor_reply, new_clip) == 32 );
C_ASSERT( FIELD_OFFSET(struct set_cursor_reply, last_change) == 48 );
C_ASSERT( sizeof(struct set_cursor_reply) == 56 );
C_ASSERT( sizeof(struct get_cursor_history_request) == 16 );
C_ASSERT( sizeof(struct get_cursor_history_reply) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_rawinput_buffer_request, rawinput_size) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_rawinput_buffer_request, buffer_size) == 16 );
C_ASSERT( sizeof(struct get_rawinput_buffer_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_rawinput_buffer_reply, next_size) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_rawinput_buffer_reply, count) == 12 );
C_ASSERT( sizeof(struct get_rawinput_buffer_reply) == 16 );
C_ASSERT( sizeof(struct update_rawinput_devices_request) == 16 );
C_ASSERT( sizeof(struct get_rawinput_devices_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_rawinput_devices_reply, device_count) == 8 );
C_ASSERT( sizeof(struct get_rawinput_devices_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_job_request, access) == 12 );
C_ASSERT( sizeof(struct create_job_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct create_job_reply, handle) == 8 );
C_ASSERT( sizeof(struct create_job_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_job_request, access) == 12 );
C_ASSERT( FIELD_OFFSET(struct open_job_request, attributes) == 16 );
C_ASSERT( FIELD_OFFSET(struct open_job_request, rootdir) == 20 );
C_ASSERT( sizeof(struct open_job_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct open_job_reply, handle) == 8 );
C_ASSERT( sizeof(struct open_job_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct assign_job_request, job) == 12 );
C_ASSERT( FIELD_OFFSET(struct assign_job_request, process) == 16 );
C_ASSERT( sizeof(struct assign_job_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct process_in_job_request, job) == 12 );
C_ASSERT( FIELD_OFFSET(struct process_in_job_request, process) == 16 );
C_ASSERT( sizeof(struct process_in_job_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_job_limits_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_job_limits_request, limit_flags) == 16 );
C_ASSERT( sizeof(struct set_job_limits_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct set_job_completion_port_request, job) == 12 );
C_ASSERT( FIELD_OFFSET(struct set_job_completion_port_request, port) == 16 );
C_ASSERT( FIELD_OFFSET(struct set_job_completion_port_request, key) == 24 );
C_ASSERT( sizeof(struct set_job_completion_port_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_job_info_request, handle) == 12 );
C_ASSERT( sizeof(struct get_job_info_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_job_info_reply, total_processes) == 8 );
C_ASSERT( FIELD_OFFSET(struct get_job_info_reply, active_processes) == 12 );
C_ASSERT( sizeof(struct get_job_info_reply) == 16 );
C_ASSERT( FIELD_OFFSET(struct terminate_job_request, handle) == 12 );
C_ASSERT( FIELD_OFFSET(struct terminate_job_request, status) == 16 );
C_ASSERT( sizeof(struct terminate_job_request) == 24 );
C_ASSERT( FIELD_OFFSET(struct suspend_process_request, handle) == 12 );
C_ASSERT( sizeof(struct suspend_process_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct resume_process_request, handle) == 12 );
C_ASSERT( sizeof(struct resume_process_request) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_next_thread_request, process) == 12 );
C_ASSERT( FIELD_OFFSET(struct get_next_thread_request, last) == 16 );
C_ASSERT( FIELD_OFFSET(struct get_next_thread_request, access) == 20 );
C_ASSERT( FIELD_OFFSET(struct get_next_thread_request, attributes) == 24 );
C_ASSERT( FIELD_OFFSET(struct get_next_thread_request, flags) == 28 );
C_ASSERT( sizeof(struct get_next_thread_request) == 32 );
C_ASSERT( FIELD_OFFSET(struct get_next_thread_reply, handle) == 8 );
C_ASSERT( sizeof(struct get_next_thread_reply) == 16 );

#endif  /* WANT_REQUEST_HANDLERS */

/* ### make_requests end ### */
/* Everything above this line is generated automatically by tools/make_requests */

#endif  /* __WINE_SERVER_REQUEST_H */
