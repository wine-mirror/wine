/*
 * Tokens
 *
 * Copyright (C) 1998 Alexandre Julliard
 * Copyright (C) 2003 Mike McCormack
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"

#include "handle.h"
#include "thread.h"
#include "process.h"
#include "request.h"

struct token
{
    struct object  obj;             /* object header */
};

static void token_dump( struct object *obj, int verbose );

static const struct object_ops token_ops =
{
    sizeof(struct token),      /* size */
    token_dump,                /* dump */
    no_add_queue,              /* add_queue */
    NULL,                      /* remove_queue */
    NULL,                      /* signaled */
    NULL,                      /* satified */
    no_get_fd,                 /* get_fd */
    no_destroy                 /* destroy */
};


static void token_dump( struct object *obj, int verbose )
{
    fprintf( stderr, "Security token\n" );
}

struct token *create_token( void )
{
    struct token *token = alloc_object( &token_ops );
    return token;
}

/* open a security token */
DECL_HANDLER(open_token)
{
    if( req->flags & OPEN_TOKEN_THREAD )
    {
        struct thread *thread = get_thread_from_handle( req->handle, 0 );
        if (thread)
        {
            if (thread->token)
                reply->token = alloc_handle( current->process, thread->token, TOKEN_ALL_ACCESS, 0);
            release_object( thread );
        }
    }
    else
    {
        struct process *process = get_process_from_handle( req->handle, 0 );
        if (process)
        {
            if (process->token)
                reply->token = alloc_handle( current->process, process->token, TOKEN_ALL_ACCESS, 0);
            release_object( process );
        }
    }
}
