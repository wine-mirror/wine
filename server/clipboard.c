/*
 * Server-side clipboard management
 *
 * Copyright (C) 2002 Ulrich Czekalla
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
#include "wine/port.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "request.h"
#include "object.h"
#include "user.h"

struct thread *cbthread;        /* thread id that has clipboard open */
static user_handle_t clipboard; /* window that has clipboard open */

struct thread *cbowner;         /* thread id that owns the clipboard */
static user_handle_t owner;     /* window that owns the clipboard data */

static user_handle_t viewer;    /* first window in clipboard viewer list */
static unsigned int seqno;      /* clipboard change sequence number */
static time_t seqnots;          /* time stamp of last seqno increment */

#define MINUPDATELAPSE 2

/* Called when thread terminates to allow release of clipboard */
void cleanup_clipboard_thread(struct thread *thread)
{
    if (thread == cbthread)
    {
        clipboard = 0;
        cbthread = NULL;
    }
    if (thread == cbowner)
    {
        owner = 0;
        cbowner = NULL;
    }
}

static int set_clipboard_window(user_handle_t win, int clear)
{
    if (cbthread && cbthread != current)
    {
        set_error(STATUS_WAS_LOCKED);
        return 0;
    }
    else if (!clear)
    {
        clipboard = win;
        cbthread = current;
    }
    else
    {
        cbthread = NULL;
        clipboard = 0;
    }
    return 1;
}


static int set_clipboard_owner(user_handle_t win, int clear)
{
    if (cbthread == current)
    {
        if (!clear)
        {
            cbowner = current;
            owner = win;
        }
        else
        {
            cbowner = 0;
            owner = 0;
        }
        seqno++;
        return 1;
    }
    else
    {
        set_error(STATUS_WAS_LOCKED);
        return 0;
    }
}


static int get_seqno(void)
{
    time_t tm = time(NULL);

    if (!cbowner && (tm > (seqnots + MINUPDATELAPSE)))
    {
        seqnots = tm;
        seqno++;
    }
    return seqno;
}


DECL_HANDLER(set_clipboard_info)
{
    reply->old_clipboard = clipboard;
    reply->old_owner = owner;
    reply->old_viewer = viewer;

    if (req->flags & SET_CB_OPEN)
    {
        if (!set_clipboard_window(req->clipboard, 0))
            return;
    }
    else if (req->flags & SET_CB_CLOSE)
    {
        if (!set_clipboard_window(0, 1))
            return;
    }

    if (req->flags & SET_CB_OWNER)
    {
        if (!set_clipboard_owner(req->owner, 0))
            return;
    }
    else if (req->flags & SET_CB_RELOWNER)
    {
        if (!set_clipboard_owner(0, 1))
            return;
    }

    if (req->flags & SET_CB_VIEWER)
        viewer = req->viewer;

    if (req->flags & SET_CB_SEQNO)
        seqno++;

    reply->seqno = get_seqno();

    if (cbthread == current)
        reply->flags |= CB_OPEN;

    if (cbowner == current)
        reply->flags |= CB_OWNER;
}
