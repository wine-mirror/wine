/* File generated automatically by tools/make_requests; DO NOT EDIT!! */

#ifndef __WINE_SERVER_REQUEST_H
#define __WINE_SERVER_REQUEST_H

enum request
{
    REQ_NEW_THREAD,
    REQ_INIT_THREAD,
    REQ_TERMINATE_PROCESS,
    REQ_TERMINATE_THREAD,
    REQ_GET_PROCESS_INFO,
    REQ_GET_THREAD_INFO,
    REQ_CLOSE_HANDLE,
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
    REQ_NB_REQUESTS
};

#ifdef WANT_REQUEST_HANDLERS

#define DECL_HANDLER(name) \
    static void req_##name( struct name##_request *req, void *data, int len, int fd )

DECL_HANDLER(new_thread);
DECL_HANDLER(init_thread);
DECL_HANDLER(terminate_process);
DECL_HANDLER(terminate_thread);
DECL_HANDLER(get_process_info);
DECL_HANDLER(get_thread_info);
DECL_HANDLER(close_handle);
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

static const struct handler {
    void       (*handler)();
    unsigned int min_size;
} req_handlers[REQ_NB_REQUESTS] = {
    { (void(*)())req_new_thread, sizeof(struct new_thread_request) },
    { (void(*)())req_init_thread, sizeof(struct init_thread_request) },
    { (void(*)())req_terminate_process, sizeof(struct terminate_process_request) },
    { (void(*)())req_terminate_thread, sizeof(struct terminate_thread_request) },
    { (void(*)())req_get_process_info, sizeof(struct get_process_info_request) },
    { (void(*)())req_get_thread_info, sizeof(struct get_thread_info_request) },
    { (void(*)())req_close_handle, sizeof(struct close_handle_request) },
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
};
#endif  /* WANT_REQUEST_HANDLERS */

#endif  /* __WINE_SERVER_REQUEST_H */
