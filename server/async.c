/*
 * Server-side support for async i/o operations
 *
 * Copyright (C) 1998 Alexandre Julliard
 * Copyright (C) 2000 Mike McCormack
 *
 */

#include "config.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "handle.h"
#include "thread.h"
#include "request.h"


DECL_HANDLER(create_async)
{
    struct object *obj;

    if (!(obj = get_handle_obj( current->process, req->file_handle, 0, NULL)) )
        return;

    /* FIXME: check if this object is allowed to do overlapped I/O */

    /* FIXME: this should be a function pointer */
    reply->timeout = get_serial_async_timeout(obj,req->type,req->count);

    release_object(obj);
}
