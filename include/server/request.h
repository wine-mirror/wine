/* File generated automatically by tools/make_requests; DO NOT EDIT!! */

#ifndef __WINE_SERVER_REQUEST_H
#define __WINE_SERVER_REQUEST_H

enum request
{
    REQ_NEW_PROCESS,
    REQ_NEW_THREAD,
    REQ_SET_DEBUG,
    REQ_INIT_PROCESS,
    REQ_INIT_THREAD,
    REQ_TERMINATE_PROCESS,
    REQ_TERMINATE_THREAD,
    REQ_GET_PROCESS_INFO,
    REQ_SET_PROCESS_INFO,
    REQ_GET_THREAD_INFO,
    REQ_SET_THREAD_INFO,
    REQ_SUSPEND_THREAD,
    REQ_RESUME_THREAD,
    REQ_QUEUE_APC,
    REQ_CLOSE_HANDLE,
    REQ_GET_HANDLE_INFO,
    REQ_SET_HANDLE_INFO,
    REQ_DUP_HANDLE,
    REQ_OPEN_PROCESS,
    REQ_SELECT,
    REQ_CREATE_EVENT,
    REQ_EVENT_OP,
    REQ_CREATE_MUTEX,
    REQ_RELEASE_MUTEX,
    REQ_CREATE_SEMAPHORE,
    REQ_RELEASE_SEMAPHORE,
    REQ_OPEN_NAMED_OBJ,
    REQ_CREATE_FILE,
    REQ_GET_READ_FD,
    REQ_GET_WRITE_FD,
    REQ_SET_FILE_POINTER,
    REQ_TRUNCATE_FILE,
    REQ_SET_FILE_TIME,
    REQ_FLUSH_FILE,
    REQ_GET_FILE_INFO,
    REQ_LOCK_FILE,
    REQ_UNLOCK_FILE,
    REQ_CREATE_PIPE,
    REQ_ALLOC_CONSOLE,
    REQ_FREE_CONSOLE,
    REQ_OPEN_CONSOLE,
    REQ_SET_CONSOLE_FD,
    REQ_GET_CONSOLE_MODE,
    REQ_SET_CONSOLE_MODE,
    REQ_SET_CONSOLE_INFO,
    REQ_GET_CONSOLE_INFO,
    REQ_WRITE_CONSOLE_INPUT,
    REQ_READ_CONSOLE_INPUT,
    REQ_CREATE_CHANGE_NOTIFICATION,
    REQ_CREATE_MAPPING,
    REQ_GET_MAPPING_INFO,
    REQ_CREATE_DEVICE,
    REQ_CREATE_SNAPSHOT,
    REQ_NEXT_PROCESS,
    REQ_NB_REQUESTS
};

#ifdef WANT_REQUEST_HANDLERS

#define DECL_HANDLER(name) \
    static void req_##name( struct name##_request *req, void *data, int len, int fd )

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
DECL_HANDLER(queue_apc);
DECL_HANDLER(close_handle);
DECL_HANDLER(get_handle_info);
DECL_HANDLER(set_handle_info);
DECL_HANDLER(dup_handle);
DECL_HANDLER(open_process);
DECL_HANDLER(select);
DECL_HANDLER(create_event);
DECL_HANDLER(event_op);
DECL_HANDLER(create_mutex);
DECL_HANDLER(release_mutex);
DECL_HANDLER(create_semaphore);
DECL_HANDLER(release_semaphore);
DECL_HANDLER(open_named_obj);
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
DECL_HANDLER(get_mapping_info);
DECL_HANDLER(create_device);
DECL_HANDLER(create_snapshot);
DECL_HANDLER(next_process);

static const struct handler {
    void       (*handler)();
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
    { (void(*)())req_queue_apc, sizeof(struct queue_apc_request) },
    { (void(*)())req_close_handle, sizeof(struct close_handle_request) },
    { (void(*)())req_get_handle_info, sizeof(struct get_handle_info_request) },
    { (void(*)())req_set_handle_info, sizeof(struct set_handle_info_request) },
    { (void(*)())req_dup_handle, sizeof(struct dup_handle_request) },
    { (void(*)())req_open_process, sizeof(struct open_process_request) },
    { (void(*)())req_select, sizeof(struct select_request) },
    { (void(*)())req_create_event, sizeof(struct create_event_request) },
    { (void(*)())req_event_op, sizeof(struct event_op_request) },
    { (void(*)())req_create_mutex, sizeof(struct create_mutex_request) },
    { (void(*)())req_release_mutex, sizeof(struct release_mutex_request) },
    { (void(*)())req_create_semaphore, sizeof(struct create_semaphore_request) },
    { (void(*)())req_release_semaphore, sizeof(struct release_semaphore_request) },
    { (void(*)())req_open_named_obj, sizeof(struct open_named_obj_request) },
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
    { (void(*)())req_get_mapping_info, sizeof(struct get_mapping_info_request) },
    { (void(*)())req_create_device, sizeof(struct create_device_request) },
    { (void(*)())req_create_snapshot, sizeof(struct create_snapshot_request) },
    { (void(*)())req_next_process, sizeof(struct next_process_request) },
};
#endif  /* WANT_REQUEST_HANDLERS */

#endif  /* __WINE_SERVER_REQUEST_H */
