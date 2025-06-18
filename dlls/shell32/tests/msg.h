/* Message Sequence Testing Code
 *
 * Copyright (C) 2007 James Hawkins
 * Copyright (C) 2007 Lei Zhang
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

#include <assert.h>
#include <windows.h>

#include "wine/test.h"

/* undocumented SWP flags - from SDK 3.1 */
#define SWP_NOCLIENTSIZE	0x0800
#define SWP_NOCLIENTMOVE	0x1000

typedef enum
{
    sent = 0x1,
    posted = 0x2,
    parent = 0x4,
    wparam = 0x8,
    lparam = 0x10,
    defwinproc = 0x20,
    beginpaint = 0x40,
    optional = 0x80,
    hook = 0x100,
    winevent_hook =0x200,
    id = 0x400
} msg_flags_t;

struct message
{
    UINT message;       /* the WM_* code */
    msg_flags_t flags;  /* message props */
    WPARAM wParam;      /* expected value of wParam */
    LPARAM lParam;      /* expected value of lParam */
    UINT id;            /* extra message data: id of the window,
                           notify code etc. */
};

struct msg_sequence
{
    int count;
    int size;
    struct message *sequence;
};

static void add_message(struct msg_sequence **seq, int sequence_index,
    const struct message *msg)
{
    struct msg_sequence *msg_seq = seq[sequence_index];

    if (!msg_seq->sequence)
    {
        msg_seq->size = 10;
        msg_seq->sequence = malloc(msg_seq->size * sizeof (struct message));
    }

    if (msg_seq->count == msg_seq->size)
    {
        msg_seq->size *= 2;
        msg_seq->sequence = realloc(msg_seq->sequence, msg_seq->size * sizeof (struct message));
    }

    assert(msg_seq->sequence);

    msg_seq->sequence[msg_seq->count].message = msg->message;
    msg_seq->sequence[msg_seq->count].flags = msg->flags;
    msg_seq->sequence[msg_seq->count].wParam = msg->wParam;
    msg_seq->sequence[msg_seq->count].lParam = msg->lParam;
    msg_seq->sequence[msg_seq->count].id = msg->id;

    msg_seq->count++;
}

static void flush_sequence(struct msg_sequence **seg, int sequence_index)
{
    struct msg_sequence *msg_seq = seg[sequence_index];
    free(msg_seq->sequence);
    msg_seq->sequence = NULL;
    msg_seq->count = msg_seq->size = 0;
}

static void flush_sequences(struct msg_sequence **seq, int n)
{
    int i;

    for (i = 0; i < n; i++)
        flush_sequence(seq, i);
}

static void ok_sequence_(struct msg_sequence **seq, int sequence_index,
    const struct message *expected, const char *context, BOOL todo,
    const char *file, int line)
{
    struct msg_sequence *msg_seq = seq[sequence_index];
    static const struct message end_of_sequence = {0, 0, 0, 0};
    const struct message *actual, *sequence;
    int failcount = 0;

    add_message(seq, sequence_index, &end_of_sequence);

    sequence = msg_seq->sequence;
    actual = sequence;

    while (expected->message && actual->message)
    {
        trace_( file, line)("expected %04x - actual %04x\n", expected->message, actual->message);

        if (expected->message == actual->message)
        {
            if (expected->flags & wparam)
            {
                if (expected->wParam != actual->wParam && todo)
                {
                    todo_wine
                    {
                        failcount++;
                        ok_(file, line) (FALSE,
                            "%s: in msg 0x%04x expecting wParam 0x%Ix got 0x%Ix\n",
                            context, expected->message, expected->wParam, actual->wParam);
                    }
                }
                else
                {
                    ok_(file, line) (expected->wParam == actual->wParam,
                        "%s: in msg 0x%04x expecting wParam 0x%Ix got 0x%Ix\n",
                        context, expected->message, expected->wParam, actual->wParam);
                }
            }

            if (expected->flags & lparam)
            {
                if (expected->lParam != actual->lParam && todo)
                {
                    todo_wine
                    {
                        failcount++;
                        ok_(file, line) (FALSE,
                            "%s: in msg 0x%04x expecting lParam 0x%Ix got 0x%Ix\n",
                            context, expected->message, expected->lParam, actual->lParam);
                    }
                }
                else
                {
                    ok_(file, line) (expected->lParam == actual->lParam,
                        "%s: in msg 0x%04x expecting lParam 0x%Ix got 0x%Ix\n",
                        context, expected->message, expected->lParam, actual->lParam);
                }
            }

            if (expected->flags & id)
            {
                if (expected->id != actual->id && expected->flags & optional)
                {
                    expected++;
                    continue;
                }
                if (expected->id != actual->id && todo)
                {
                    todo_wine
                    {
                        failcount++;
                        ok_(file, line) (FALSE,
                            "%s: in msg 0x%04x expecting id 0x%x got 0x%x\n",
                            context, expected->message, expected->id, actual->id);
                    }
                }
                else
                {
                    ok_(file, line) (expected->id == actual->id,
                        "%s: in msg 0x%04x expecting id 0x%x got 0x%x\n",
                        context, expected->message, expected->id, actual->id);
                }
            }

            if ((expected->flags & defwinproc) != (actual->flags & defwinproc) && todo)
            {
                todo_wine
                {
                    failcount++;
                    ok_(file, line) (FALSE,
                        "%s: the msg 0x%04x should %shave been sent by DefWindowProc\n",
                        context, expected->message, (expected->flags & defwinproc) ? "" : "NOT ");
                }
            }
            else
            {
                ok_(file, line) ((expected->flags & defwinproc) == (actual->flags & defwinproc),
                    "%s: the msg 0x%04x should %shave been sent by DefWindowProc\n",
                    context, expected->message, (expected->flags & defwinproc) ? "" : "NOT ");
            }

            ok_(file, line) ((expected->flags & beginpaint) == (actual->flags & beginpaint),
                "%s: the msg 0x%04x should %shave been sent by BeginPaint\n",
                context, expected->message, (expected->flags & beginpaint) ? "" : "NOT ");
            ok_(file, line) ((expected->flags & (sent|posted)) == (actual->flags & (sent|posted)),
                "%s: the msg 0x%04x should have been %s\n",
                context, expected->message, (expected->flags & posted) ? "posted" : "sent");
            ok_(file, line) ((expected->flags & parent) == (actual->flags & parent),
                "%s: the msg 0x%04x was expected in %s\n",
                context, expected->message, (expected->flags & parent) ? "parent" : "child");
            ok_(file, line) ((expected->flags & hook) == (actual->flags & hook),
                "%s: the msg 0x%04x should have been sent by a hook\n",
                context, expected->message);
            ok_(file, line) ((expected->flags & winevent_hook) == (actual->flags & winevent_hook),
                "%s: the msg 0x%04x should have been sent by a winevent hook\n",
                context, expected->message);
            expected++;
            actual++;
        }
        else if (expected->flags & optional)
            expected++;
        else if (todo)
        {
            failcount++;
            todo_wine
            {
                ok_(file, line) (FALSE, "%s: the msg 0x%04x was expected, but got msg 0x%04x instead\n",
                    context, expected->message, actual->message);
            }

            flush_sequence(seq, sequence_index);
            return;
        }
        else
        {
            ok_(file, line) (FALSE, "%s: the msg 0x%04x was expected, but got msg 0x%04x instead\n",
                context, expected->message, actual->message);
            expected++;
            actual++;
        }
    }

    /* skip all optional trailing messages */
    while (expected->message && ((expected->flags & optional)))
        expected++;

    if (todo)
    {
        todo_wine
        {
            if (expected->message || actual->message)
            {
                failcount++;
                ok_(file, line) (FALSE, "%s: the msg sequence is not complete: expected %04x - actual %04x\n",
                    context, expected->message, actual->message);
            }
        }
    }
    else if (expected->message || actual->message)
    {
        ok_(file, line) (FALSE, "%s: the msg sequence is not complete: expected %04x - actual %04x\n",
            context, expected->message, actual->message);
    }

    if(todo && !failcount) /* succeeded yet marked todo */
    {
        todo_wine
        {
            ok_(file, line)(TRUE, "%s: marked \"todo_wine\" but succeeds\n", context);
        }
    }

    flush_sequence(seq, sequence_index);
}

#define ok_sequence(seq, index, exp, contx, todo) \
        ok_sequence_(seq, index, (exp), (contx), (todo), __FILE__, __LINE__)


static void init_msg_sequences(struct msg_sequence **seq, int n)
{
    int i;

    for (i = 0; i < n; i++)
        seq[i] = calloc(1, sizeof(struct msg_sequence));
}
