/* File generated automatically by tools/make_requests; DO NOT EDIT!! */

#include <stdio.h>
#include <sys/uio.h>
#include "server.h"
#include "server/thread.h"

static void dump_new_thread_request( struct new_thread_request *req )
{
    printf( " pid=%p", req->pid );
}

static void dump_new_thread_reply( struct new_thread_reply *req )
{
    printf( " tid=%p,", req->tid );
    printf( " thandle=%d,", req->thandle );
    printf( " pid=%p,", req->pid );
    printf( " phandle=%d", req->phandle );
}

static void dump_init_thread_request( struct init_thread_request *req )
{
    printf( " unix_pid=%d", req->unix_pid );
}

static void dump_terminate_process_request( struct terminate_process_request *req )
{
    printf( " handle=%d,", req->handle );
    printf( " exit_code=%d", req->exit_code );
}

static void dump_terminate_thread_request( struct terminate_thread_request *req )
{
    printf( " handle=%d,", req->handle );
    printf( " exit_code=%d", req->exit_code );
}

static void dump_get_process_info_request( struct get_process_info_request *req )
{
    printf( " handle=%d", req->handle );
}

static void dump_get_process_info_reply( struct get_process_info_reply *req )
{
    printf( " pid=%p,", req->pid );
    printf( " exit_code=%d", req->exit_code );
}

static void dump_close_handle_request( struct close_handle_request *req )
{
    printf( " handle=%d", req->handle );
}

static void dump_dup_handle_request( struct dup_handle_request *req )
{
    printf( " src_process=%d,", req->src_process );
    printf( " src_handle=%d,", req->src_handle );
    printf( " dst_process=%d,", req->dst_process );
    printf( " dst_handle=%d,", req->dst_handle );
    printf( " access=%08x,", req->access );
    printf( " inherit=%d,", req->inherit );
    printf( " options=%d", req->options );
}

static void dump_dup_handle_reply( struct dup_handle_reply *req )
{
    printf( " handle=%d", req->handle );
}

static void dump_open_process_request( struct open_process_request *req )
{
    printf( " pid=%p,", req->pid );
    printf( " access=%08x,", req->access );
    printf( " inherit=%d", req->inherit );
}

static void dump_open_process_reply( struct open_process_reply *req )
{
    printf( " handle=%d", req->handle );
}

struct dumper
{
    void (*dump_req)();
    void (*dump_reply)();
    unsigned int size;
};

static const struct dumper dumpers[REQ_NB_REQUESTS] =
{
    { (void(*)())dump_new_thread_request,
      (void(*)())dump_new_thread_reply,
      sizeof(struct new_thread_request) },
    { (void(*)())dump_init_thread_request,
      (void(*)())0,
      sizeof(struct init_thread_request) },
    { (void(*)())dump_terminate_process_request,
      (void(*)())0,
      sizeof(struct terminate_process_request) },
    { (void(*)())dump_terminate_thread_request,
      (void(*)())0,
      sizeof(struct terminate_thread_request) },
    { (void(*)())dump_get_process_info_request,
      (void(*)())dump_get_process_info_reply,
      sizeof(struct get_process_info_request) },
    { (void(*)())dump_close_handle_request,
      (void(*)())0,
      sizeof(struct close_handle_request) },
    { (void(*)())dump_dup_handle_request,
      (void(*)())dump_dup_handle_reply,
      sizeof(struct dup_handle_request) },
    { (void(*)())dump_open_process_request,
      (void(*)())dump_open_process_reply,
      sizeof(struct open_process_request) },
};

static const char * const req_names[REQ_NB_REQUESTS] =
{
    "new_thread",
    "init_thread",
    "terminate_process",
    "terminate_thread",
    "get_process_info",
    "close_handle",
    "dup_handle",
    "open_process",
};

void trace_request( enum request req, void *data, int len, int fd )
{
    current->last_req = req;
    printf( "%08x: %s(", (unsigned int)current, req_names[req] );
    dumpers[req].dump_req( data );
    if (len > dumpers[req].size)
    {
        unsigned char *ptr = (unsigned char *)data + dumpers[req].size;
	len -= dumpers[req].size;
        while (len--) printf( ", %02x", *ptr++ );
    }
    if (fd != -1) printf( " ) fd=%d\n", fd );
    else printf( " )\n" );
}

void trace_timeout(void)
{
    printf( "%08x: *timeout*\n", (unsigned int)current );
}

void trace_kill( int exit_code )
{
    printf( "%08x: *killed* exit_code=%d\n",
            (unsigned int)current, exit_code );
}

void trace_reply( struct thread *thread, int type, int pass_fd,
                  struct iovec *vec, int veclen )
{
    if (!thread) return;
    printf( "%08x: %s() = %d",
            (unsigned int)thread, req_names[thread->last_req], type );
    if (veclen)
    {
	printf( " {" );
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
	    while (len--) printf( ", %02x", *ptr++ );
	}
	printf( " }" );
    }
    if (pass_fd != -1) printf( " fd=%d\n", pass_fd );
    else printf( "\n" );
}
