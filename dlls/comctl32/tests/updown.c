/* Unit tests for the up-down control
 *
 * Copyright 2005 C. Scott Ananian
 * Copyright (C) 2007 James Hawkins
 * Copyright (C) 2007 Leslie Choong
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

/* TO TEST:
 *   - send click messages to the up-down control, check the current position
 *   - up-down control automatically positions itself next to its buddy window
 *   - up-down control sets the caption of the buddy window
 *   - test CreateUpDownControl API
 *   - check UDS_AUTOBUDDY style, up-down control selects previous window in z-order
 *   - check UDM_SETBUDDY message
 *   - check UDM_GETBUDDY message
 *   - up-down control and buddy control must have the same parent
 *   - up-down control notifies its parent window when its position changes with UDN_DELTAPOS + WM_VSCROLL or WM_HSCROLL
 *   - check UDS_ALIGN[LEFT,RIGHT]...check that width of buddy window is decreased
 *   - check that UDS_SETBUDDYINT sets the caption of the buddy window when it is changed
 *   - check that the thousands operator is set for large numbers
 *   - check that the thousands operator is not set with UDS_NOTHOUSANDS
 *   - check UDS_ARROWKEYS, control subclasses the buddy window so that it processes the keys when it has focus
 *   - check UDS_HORZ
 *   - check changing past min/max values
 *   - check UDS_WRAP wraps values past min/max, incrementing past upper value wraps position to lower value
 *   - can change control's position, min/max pos, radix
 *   - check UDM_GETPOS, for up-down control with a buddy window, position is the caption of the buddy window, so change the
 *     caption of the buddy window then call UDM_GETPOS
 *   - check UDM_SETRANGE, max can be less than min, so clicking the up arrow decreases the current position
 *   - check UDM_GETRANGE
 *   - more stuff to test
 */

#include <assert.h>
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#include "wine/test.h"

#define NUM_MSG_SEQUENCES   3
#define PARENT_SEQ_INDEX    0
#define EDIT_SEQ_INDEX      1
#define UPDOWN_SEQ_INDEX    2

/* undocumented SWP flags - from SDK 3.1 */
#define SWP_NOCLIENTSIZE	0x0800
#define SWP_NOCLIENTMOVE	0x1000

static HWND parent_wnd, edit, updown;

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
    winevent_hook =0x200
} msg_flags_t;

struct message
{
    UINT message;       /* the WM_* code */
    msg_flags_t flags;  /* message props */
    WPARAM wParam;      /* expected value of wParam */
    LPARAM lParam;      /* expected value of lParam */
};

struct msg_sequence
{
    int count;
    int size;
    struct message *sequence;
};

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static const struct message create_parent_wnd_seq[] = {
    { WM_GETMINMAXINFO, sent },
    { WM_NCCREATE, sent },
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, 0 },
    { WM_ACTIVATEAPP, sent|wparam, 1 },
    { WM_NCACTIVATE, sent|wparam, 1 },
    { WM_ACTIVATE, sent|wparam, 1 },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|defwinproc|optional },
    { WM_SETFOCUS, sent|wparam|defwinproc, 0 },
    /* Win9x adds SWP_NOZORDER below */
    { WM_WINDOWPOSCHANGED, sent, /*|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE*/ },
    { WM_NCCALCSIZE, sent|wparam|optional, 1 },
    { WM_SIZE, sent },
    { WM_MOVE, sent },
    { 0 }
};

static const struct message add_edit_to_parent_seq[] = {
    { WM_PARENTNOTIFY, sent|wparam, WM_CREATE },
    { 0 }
};

static const struct message add_updown_with_edit_seq[] = {
    { WM_WINDOWPOSCHANGING, sent },
    { WM_NCCALCSIZE, sent|wparam, TRUE },
    { WM_WINDOWPOSCHANGED, sent },
    { WM_SIZE, sent|wparam|defwinproc, SIZE_RESTORED /*, MAKELONG(91, 75) exact size depends on font */ },
    { 0 }
};

static const struct message add_updown_to_parent_seq[] = {
    { WM_NOTIFYFORMAT, sent|lparam, 0, NF_QUERY },
    { WM_QUERYUISTATE, sent },
    { WM_PARENTNOTIFY, sent|wparam, MAKELONG(WM_CREATE, WM_CREATE) },
    { 0 }
};

static const struct message get_edit_text_seq[] = {
    { WM_GETTEXT, sent },
    { 0 }
};

static void add_message(int sequence_index, const struct message *msg)
{
    struct msg_sequence *msg_seq = sequences[sequence_index];

    if (!msg_seq->sequence)
    {
        msg_seq->size = 10;
        msg_seq->sequence = HeapAlloc(GetProcessHeap(), 0,
                                      msg_seq->size * sizeof (struct message));
    }

    if (msg_seq->count == msg_seq->size)
    {
        msg_seq->size *= 2;
        msg_seq->sequence = HeapReAlloc(GetProcessHeap(), 0,
                                        msg_seq->sequence,
                                        msg_seq->size * sizeof (struct message));
    }

    assert(msg_seq->sequence);

    msg_seq->sequence[msg_seq->count].message = msg->message;
    msg_seq->sequence[msg_seq->count].flags = msg->flags;
    msg_seq->sequence[msg_seq->count].wParam = msg->wParam;
    msg_seq->sequence[msg_seq->count].lParam = msg->lParam;

    msg_seq->count++;
}

static void flush_sequence(int sequence_index)
{
    struct msg_sequence *msg_seq = sequences[sequence_index];
    HeapFree(GetProcessHeap(), 0, msg_seq->sequence);
    msg_seq->sequence = NULL;
    msg_seq->count = msg_seq->size = 0;
}

static void flush_sequences(void)
{
    flush_sequence(PARENT_SEQ_INDEX);
    flush_sequence(EDIT_SEQ_INDEX);
    flush_sequence(UPDOWN_SEQ_INDEX);
}

#define ok_sequence(index, exp, contx, todo) \
        ok_sequence_(index, (exp), (contx), (todo), __FILE__, __LINE__)


static void ok_sequence_(int sequence_index, const struct message *expected,
                         const char *context, int todo, const char *file, int line)
{
    struct msg_sequence *msg_seq = sequences[sequence_index];
    static const struct message end_of_sequence = {0, 0, 0, 0};
    const struct message *actual, *sequence;
    int failcount = 0;

    add_message(sequence_index, &end_of_sequence);

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
                            "%s: in msg 0x%04x expecting wParam 0x%x got 0x%x\n",
                            context, expected->message, expected->wParam, actual->wParam);
                    }
                }
                else
                {
                    ok_(file, line) (expected->wParam == actual->wParam,
                        "%s: in msg 0x%04x expecting wParam 0x%x got 0x%x\n",
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
                            "%s: in msg 0x%04x expecting lParam 0x%lx got 0x%lx\n",
                            context, expected->message, expected->lParam, actual->lParam);
                    }
                }
                else
                {
                    ok_(file, line) (expected->lParam == actual->lParam,
                        "%s: in msg 0x%04x expecting lParam 0x%lx got 0x%lx\n",
                        context, expected->message, expected->lParam, actual->lParam);
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

            flush_sequence(sequence_index);
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

    flush_sequence(sequence_index);
}

static void init_msg_sequences(void)
{
    int i;

    for (i = 0; i < NUM_MSG_SEQUENCES; i++)
        sequences[i] = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct msg_sequence));
}

static LRESULT WINAPI parent_wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static long defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;

    /* do not log painting messages */
    if (message != WM_PAINT &&
        message != WM_ERASEBKGND &&
        message != WM_NCPAINT &&
        message != WM_NCHITTEST &&
        message != WM_GETTEXT &&
        message != WM_GETICON &&
        message != WM_DEVICECHANGE)
    {
        trace("parent: %p, %04x, %08x, %08lx\n", hwnd, message, wParam, lParam);

        msg.message = message;
        msg.flags = sent|wparam|lparam;
        if (defwndproc_counter) msg.flags |= defwinproc;
        msg.wParam = wParam;
        msg.lParam = lParam;
        add_message(PARENT_SEQ_INDEX, &msg);
    }

    defwndproc_counter++;
    ret = DefWindowProcA(hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static BOOL register_parent_wnd_class(void)
{
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = parent_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "Up-Down test parent class";
    return RegisterClassA(&cls);
}

static HWND create_parent_window(void)
{
    if (!register_parent_wnd_class())
        return NULL;

    return CreateWindowEx(0, "Up-Down test parent class",
                          "Up-Down test parent window",
                          WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                          WS_MAXIMIZEBOX | WS_VISIBLE,
                          0, 0, 100, 100,
                          GetDesktopWindow(), NULL, GetModuleHandleA(NULL), NULL);
}

struct subclass_info
{
    WNDPROC oldproc;
};

static LRESULT WINAPI edit_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct subclass_info *info = (struct subclass_info *)GetWindowLongA(hwnd, GWL_USERDATA);
    static long defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;

    trace("edit: %p, %04x, %08x, %08lx\n", hwnd, message, wParam, lParam);

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    add_message(EDIT_SEQ_INDEX, &msg);

    defwndproc_counter++;
    ret = CallWindowProcA(info->oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;
    return ret;
}

static HWND create_edit_control()
{
    struct subclass_info *info;
    RECT rect;

    info = HeapAlloc(GetProcessHeap(), 0, sizeof(struct subclass_info));
    if (!info)
        return NULL;

    GetClientRect(parent_wnd, &rect);
    edit = CreateWindowExA(0, "EDIT", NULL, WS_CHILD | WS_BORDER | WS_VISIBLE,
                           0, 0, rect.right, rect.bottom,
                           parent_wnd, NULL, GetModuleHandleA(NULL), NULL);
    if (!edit)
    {
        HeapFree(GetProcessHeap(), 0, info);
        return NULL;
    }

    info->oldproc = (WNDPROC)SetWindowLongA(edit, GWL_WNDPROC,
                                            (LONG)edit_subclass_proc);
    SetWindowLongA(edit, GWL_USERDATA, (LONG)info);

    return edit;
}

static LRESULT WINAPI updown_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct subclass_info *info = (struct subclass_info *)GetWindowLongA(hwnd, GWL_USERDATA);
    static long defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;

    trace("updown: %p, %04x, %08x, %08lx\n", hwnd, message, wParam, lParam);

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    add_message(EDIT_SEQ_INDEX, &msg);

    defwndproc_counter++;
    ret = CallWindowProcA(info->oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static HWND create_updown_control()
{
    struct subclass_info *info;
    HWND updown;
    RECT rect;

    info = HeapAlloc(GetProcessHeap(), 0, sizeof(struct subclass_info));
    if (!info)
        return NULL;

    GetClientRect(parent_wnd, &rect);
    updown = CreateUpDownControl(WS_CHILD | WS_BORDER | WS_VISIBLE | UDS_ALIGNRIGHT,
                                 0, 0, rect.right, rect.bottom, parent_wnd, 1, GetModuleHandleA(NULL), edit,
                                 100, 0, 50);
    if (!updown)
    {
        HeapFree(GetProcessHeap(), 0, info);
        return NULL;
    }

    info->oldproc = (WNDPROC)SetWindowLongA(updown, GWL_WNDPROC,
                                            (LONG)updown_subclass_proc);
    SetWindowLongA(updown, GWL_USERDATA, (LONG)info);

    return updown;
}

static void test_updown_pos(void)
{
    int r;

    /* Set Range from 0 to 100 */
    SendMessage(updown, UDM_SETRANGE, 0 , MAKELONG(100,0) );
    r = SendMessage(updown, UDM_GETRANGE, 0,0);
    ok(LOWORD(r) == 100, "Expected 100, got %d\n", LOWORD(r));
    ok(HIWORD(r) == 0, "Expected 0, got %d\n", HIWORD(r));

    /* Set the position to 5, return is not checked as it was set before func call */
    SendMessage(updown, UDM_SETPOS, 0 , MAKELONG(5,0) );
    /* Since UDM_SETBUDDYINT was not set at creation HIWORD(r) will always be 1 as a return from UDM_GETPOS */
    /* Get the position, which should be 5 */
    r = SendMessage(updown, UDM_GETPOS, 0 , 0 );
    ok(LOWORD(r) == 5, "Expected 5, got %d\n", LOWORD(r));
    ok(HIWORD(r) == 1, "Expected 1, got %d\n", HIWORD(r));

    /* Set the position to 0, return should be 5 */
    r = SendMessage(updown, UDM_SETPOS, 0 , MAKELONG(0,0) );
    ok(r == 5, "Expected 5, got %d\n", r);
    /* Get the position, which should be 0 */
    r = SendMessage(updown, UDM_GETPOS, 0 , 0 );
    ok(LOWORD(r) == 0, "Expected 0, got %d\n", LOWORD(r));
    ok(HIWORD(r) == 1, "Expected 1, got %d\n", HIWORD(r));

    /* Set the position to -1, return should be 0 */
    r = SendMessage(updown, UDM_SETPOS, 0 , MAKELONG(-1,0) );
    ok(r == 0, "Expected 0, got %d\n", r);
    /* Get the position, which should be 0 */
    r = SendMessage(updown, UDM_GETPOS, 0 , 0 );
    ok(LOWORD(r) == 0, "Expected 0, got %d\n", LOWORD(r));
    ok(HIWORD(r) == 1, "Expected 1, got %d\n", HIWORD(r));

    /* Set the position to 100, return should be 0 */
    r = SendMessage(updown, UDM_SETPOS, 0 , MAKELONG(100,0) );
    ok(r == 0, "Expected 0, got %d\n", r);
    /* Get the position, which should be 100 */
    r = SendMessage(updown, UDM_GETPOS, 0 , 0 );
    ok(LOWORD(r) == 100, "Expected 100, got %d\n", LOWORD(r));
    ok(HIWORD(r) == 1, "Expected 1, got %d\n", HIWORD(r));

    /* Set the position to 101, return should be 100 */
    r = SendMessage(updown, UDM_SETPOS, 0 , MAKELONG(101,0) );
    ok(r == 100, "Expected 100, got %d\n", r);
    /* Get the position, which should be 100 */
    r = SendMessage(updown, UDM_GETPOS, 0 , 0 );
    ok(LOWORD(r) == 100, "Expected 100, got %d\n", LOWORD(r));
    ok(HIWORD(r) == 1, "Expected 1, got %d\n", HIWORD(r));
}

static void test_updown_pos32(void)
{
    int r;
    int low, high;

    /* Set the position to 0 to 1000 */
    SendMessage(updown, UDM_SETRANGE32, 0 , 1000 );

    r = SendMessage(updown, UDM_GETRANGE32, (WPARAM) &low , (LPARAM) &high );
    ok(low == 0, "Expected 0, got %d\n", low);
    ok(high == 1000, "Expected 1000, got %d\n", high);

    /* Set position to 500, don't check return since it is unset*/
    r = SendMessage(updown, UDM_SETPOS32, 0 , 500 );

    /* Since UDM_SETBUDDYINT was not set at creation bRet will always be true as a return from UDM_GETPOS32 */

    r = SendMessage(updown, UDM_GETPOS32, 0 , (LPARAM) &high );
    ok(r == 500, "Expected 500, got %d\n", r);
    ok(high == 1 , "Expected 0, got %d\n", high);

    /* Set position to 0, return should be 500 */
    r = SendMessage(updown, UDM_SETPOS32, 0 , 0 );
    ok(r == 500, "Expected 500, got %d\n", r);
    r = SendMessage(updown, UDM_GETPOS32, 0 , (LPARAM) &high );
    ok(r == 0, "Expected 0, got %d\n", r);
    ok(high == 1 , "Expected 0, got %d\n", high);

    /* Set position to -1 which sohuld become 0, return should be 0 */
    r = SendMessage(updown, UDM_SETPOS32, 0 , -1 );
    ok(r == 0, "Expected 0, got %d\n", r);
    r = SendMessage(updown, UDM_GETPOS32, 0 , (LPARAM) &high );
    ok(r == 0, "Expected 0, got %d\n", r);
    ok(high == 1 , "Expected 0, got %d\n", high);

    /* Set position to 1000, return should be 0 */
    r = SendMessage(updown, UDM_SETPOS32, 0 , 1000 );
    ok(r == 0, "Expected 0, got %d\n", r);
    r = SendMessage(updown, UDM_GETPOS32, 0 , (LPARAM) &high );
    ok(r == 1000, "Expected 1000, got %d\n", r);
    ok(high == 1 , "Expected 0, got %d\n", high);

    /* Set position to 1001 which sohuld become 1000, return should be 1000 */
    r = SendMessage(updown, UDM_SETPOS32, 0 , 1001 );
    ok(r == 1000, "Expected 1000, got %d\n", r);
    r = SendMessage(updown, UDM_GETPOS32, 0 , (LPARAM) &high );
    ok(r == 1000, "Expected 0, got %d\n", r);
    ok(high == 1 , "Expected 0, got %d\n", high);
}

static void test_updown_buddy(void)
{
    HWND buddyReturn;

    buddyReturn = (HWND)SendMessage(updown, UDM_GETBUDDY, 0 , 0 );
    ok(buddyReturn == edit, "Expected edit handle\n");
}

static void test_updown_base(void)
{
    int r;

    SendMessage(updown, UDM_SETBASE, 10 , 0);
    r = SendMessage(updown, UDM_GETBASE, 0 , 0);
    ok(r == 10, "Expected 10, got %d\n", r);

    /* Set base to an invalid value, should return 0 and stay at 10 */
    r = SendMessage(updown, UDM_SETBASE, 80 , 0);
    ok(r == 0, "Expected 0, got %d\n", r);
    r = SendMessage(updown, UDM_GETBASE, 0 , 0);
    ok(r == 10, "Expected 10, got %d\n", r);

    /* Set base to 16 now, should get 16 as the return */
    r = SendMessage(updown, UDM_SETBASE, 16 , 0);
    ok(r == 10, "Expected 10, got %d\n", r);
    r = SendMessage(updown, UDM_GETBASE, 0 , 0);
    ok(r == 16, "Expected 16, got %d\n", r);

    /* Set base to an invalid value, should return 0 and stay at 16 */
    r = SendMessage(updown, UDM_SETBASE, 80 , 0);
    ok(r == 0, "Expected 0, got %d\n", r);
    r = SendMessage(updown, UDM_GETBASE, 0 , 0);
    ok(r == 16, "Expected 16, got %d\n", r);

    /* Set base back to 10, return should be 16 */
    r = SendMessage(updown, UDM_SETBASE, 10 , 0);
    ok(r == 16, "Expected 16, got %d\n", r);
    r = SendMessage(updown, UDM_GETBASE, 0 , 0);
    ok(r == 10, "Expected 10, got %d\n", r);
}

static void test_updown_unicode(void)
{
    int r;

    /* Set it to ANSI, don't check return as we don't know previous state */
    SendMessage(updown, UDM_SETUNICODEFORMAT, 0 , 0);
    r = SendMessage(updown, UDM_GETUNICODEFORMAT, 0 , 0);
    ok(r == 0, "Expected 0, got %d\n", r);

    /* Now set it to Unicode format */
    r = SendMessage(updown, UDM_SETUNICODEFORMAT, 1 , 0);
    ok(r == 0, "Expected 0, got %d\n", r);
    r = SendMessage(updown, UDM_GETUNICODEFORMAT, 0 , 0);
    ok(r == 1, "Expected 1, got %d\n", r);

    /* And now set it back to ANSI */
    r = SendMessage(updown, UDM_SETUNICODEFORMAT, 0 , 0);
    ok(r == 1, "Expected 1, got %d\n", r);
    r = SendMessage(updown, UDM_GETUNICODEFORMAT, 0 , 0);
    ok(r == 0, "Expected 0, got %d\n", r);
}


static void test_create_updown_control(void)
{
    CHAR text[MAX_PATH];

    parent_wnd = create_parent_window();
    ok(parent_wnd != NULL, "Failed to create parent window!\n");
    ok_sequence(PARENT_SEQ_INDEX, create_parent_wnd_seq, "create parent window", TRUE);

    flush_sequences();

    edit = create_edit_control();
    ok(edit != NULL, "Failed to create edit control\n");
    ok_sequence(PARENT_SEQ_INDEX, add_edit_to_parent_seq, "add edit control to parent", FALSE);

    flush_sequences();

    updown = create_updown_control();
    ok(updown != NULL, "Failed to create updown control\n");
    ok_sequence(PARENT_SEQ_INDEX, add_updown_to_parent_seq, "add updown control to parent", TRUE);
    ok_sequence(EDIT_SEQ_INDEX, add_updown_with_edit_seq, "add updown control with edit", FALSE);

    flush_sequences();

    GetWindowTextA(edit, text, MAX_PATH);
    ok(lstrlenA(text) == 0, "Expected empty string\n");
    ok_sequence(EDIT_SEQ_INDEX, get_edit_text_seq, "get edit text", FALSE);

    flush_sequences();

    test_updown_pos();
    test_updown_pos32();
    test_updown_buddy();
    test_updown_base();
    test_updown_unicode();
}

START_TEST(updown)
{
    InitCommonControls();
    init_msg_sequences();

    test_create_updown_control();
}
