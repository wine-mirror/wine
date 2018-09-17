/*
 * Unit tests for the pager control
 *
 * Copyright 2012 Alexandre Julliard
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

#include <windows.h>
#include <commctrl.h>

#include "wine/test.h"
#include "msg.h"

#define NUM_MSG_SEQUENCES   1
#define PAGER_SEQ_INDEX     0

static HWND parent_wnd, child1_wnd, child2_wnd;
static INT notify_format;
static BOOL notify_query_received;
static WCHAR test_w[] = {'t', 'e', 's', 't', 0};
static CHAR test_a[] = {'t', 'e', 's', 't', 0};
static WCHAR empty_w[] = {0};
static CHAR empty_a[] = {0};
static CHAR large_a[] = "You should have received a copy of the GNU Lesser General Public License along with this ...";
static WCHAR large_truncated_65_w[65] =
{
    'Y', 'o', 'u', ' ', 's', 'h', 'o', 'u', 'l', 'd', ' ', 'h', 'a', 'v', 'e', ' ', 'r', 'e', 'c', 'e', 'i', 'v',
    'e', 'd', ' ', 'a', ' ', 'c', 'o', 'p', 'y', ' ', 'o', 'f', ' ', 't', 'h', 'e', ' ', 'G', 'N', 'U', ' ', 'L',
    'e', 's', 's', 'e', 'r', ' ', 'G', 'e', 'n', 'e', 'r', 'a', 'l', ' ', 'P', 'u', 'b', 'l', 'i', 'c', 0
};
static WCHAR buffer[64];

/* Text field conversion test behavior flags. */
enum test_conversion_flags
{
    CONVERT_SEND = 0x01,
    DONT_CONVERT_SEND = 0x02,
    CONVERT_RECEIVE = 0x04,
    DONT_CONVERT_RECEIVE = 0x08,
    SEND_EMPTY_IF_NULL = 0x10,
    DONT_SEND_EMPTY_IF_NULL = 0x20
};

static struct notify_test_info
{
    UINT unicode;
    UINT ansi;
    UINT_PTR id_from;
    HWND hwnd_from;
    /* Whether parent received notification */
    BOOL received;
    UINT test_id;
    UINT sub_test_id;
    /* Text field conversion test behavior flag */
    DWORD flags;
} notify_test_info;

struct notify_test_send
{
    /* Data sent to pager */
    WCHAR *send_text;
    INT send_text_size;
    INT send_text_max;
    /* Data expected by parent of pager */
    void *expect_text;
};

struct notify_test_receive
{
    /* Data sent to pager */
    WCHAR *send_text;
    INT send_text_size;
    INT send_text_max;
    /* Data for parent to write */
    CHAR *write_pointer;
    CHAR *write_text;
    INT write_text_size;
    INT write_text_max;
    /* Data when message returned */
    void *return_text;
    INT return_text_max;
};

struct generic_text_helper_para
{
    void *ptr;
    size_t size;
    UINT *mask;
    UINT required_mask;
    WCHAR **text;
    INT *text_max;
    UINT code_unicode;
    UINT code_ansi;
    DWORD flags;
};

static const struct notify_test_send test_convert_send_data[] =
{
    {test_w, sizeof(test_w), ARRAY_SIZE(buffer), test_a}
};

static const struct notify_test_send test_dont_convert_send_data[] =
{
    {test_w, sizeof(test_w), ARRAY_SIZE(buffer), test_w}
};

static const struct notify_test_receive test_convert_receive_data[] =
{
    {empty_w, sizeof(empty_w), ARRAY_SIZE(buffer), NULL, test_a, sizeof(test_a), -1, test_w, ARRAY_SIZE(buffer)},
    {empty_w, sizeof(empty_w), ARRAY_SIZE(buffer), test_a, NULL, 0, -1, test_w, ARRAY_SIZE(buffer)},
    {NULL, sizeof(empty_w), ARRAY_SIZE(buffer), test_a, NULL, 0, -1, NULL, ARRAY_SIZE(buffer)},
    {empty_w, sizeof(empty_w), ARRAY_SIZE(buffer), large_a, NULL, 0, -1, large_truncated_65_w, ARRAY_SIZE(buffer)},
    {empty_w, sizeof(empty_w), ARRAY_SIZE(buffer), empty_a, 0, 0, 1, empty_w, 1},
};

static const struct notify_test_receive test_dont_convert_receive_data[] =
{
    {empty_w, sizeof(empty_w), ARRAY_SIZE(buffer), NULL, test_a, sizeof(test_a), -1, test_a, ARRAY_SIZE(buffer)},
    {empty_w, sizeof(empty_w), ARRAY_SIZE(buffer), test_a, NULL, 0, -1, test_a, ARRAY_SIZE(buffer)},
};

#define CHILD1_ID 1
#define CHILD2_ID 2

static BOOL (WINAPI *pInitCommonControlsEx)(const INITCOMMONCONTROLSEX*);
static BOOL (WINAPI *pSetWindowSubclass)(HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR);

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static const struct message set_child_seq[] = {
    { PGM_SETCHILD, sent },
    { WM_WINDOWPOSCHANGING, sent },
    { WM_NCCALCSIZE, sent|wparam, TRUE },
    { WM_NOTIFY, sent|id|parent, 0, 0, PGN_CALCSIZE },
    { WM_WINDOWPOSCHANGED, sent },
    { WM_WINDOWPOSCHANGING, sent|id, 0, 0, CHILD1_ID },
    { WM_NCCALCSIZE, sent|wparam|id|optional, TRUE, 0, CHILD1_ID },
    { WM_CHILDACTIVATE, sent|id, 0, 0, CHILD1_ID },
    { WM_WINDOWPOSCHANGED, sent|id, 0, 0, CHILD1_ID },
    { WM_SIZE, sent|id|defwinproc|optional, 0, 0, CHILD1_ID },
    { 0 }
};

/* This differs from the above message list only in the child window that is
 * expected to receive the child messages. No message is sent to the old child.
 * Also child 2 is hidden while child 1 is visible. The pager does not make the
 * hidden child visible. */
static const struct message switch_child_seq[] = {
    { PGM_SETCHILD, sent },
    { WM_WINDOWPOSCHANGING, sent },
    { WM_NCCALCSIZE, sent|wparam, TRUE },
    { WM_NOTIFY, sent|id|parent, 0, 0, PGN_CALCSIZE },
    { WM_WINDOWPOSCHANGED, sent },
    { WM_WINDOWPOSCHANGING, sent|id, 0, 0, CHILD2_ID },
    { WM_NCCALCSIZE, sent|wparam|id, TRUE, 0, CHILD2_ID },
    { WM_CHILDACTIVATE, sent|id, 0, 0, CHILD2_ID },
    { WM_WINDOWPOSCHANGED, sent|id, 0, 0, CHILD2_ID },
    { WM_SIZE, sent|id|defwinproc, 0, 0, CHILD2_ID },
    { 0 }
};

static const struct message set_pos_seq[] = {
    { PGM_SETPOS, sent },
    { WM_WINDOWPOSCHANGING, sent },
    { WM_NCCALCSIZE, sent|wparam, TRUE },
    { WM_NOTIFY, sent|id|parent, 0, 0, PGN_CALCSIZE },
    { WM_WINDOWPOSCHANGED, sent },
    { WM_MOVE, sent|optional },
    /* The WM_SIZE handler sends WM_WINDOWPOSCHANGING, WM_CHILDACTIVATE
     * and WM_WINDOWPOSCHANGED (which sends WM_MOVE) to the child.
     * Another WM_WINDOWPOSCHANGING is sent afterwards.
     *
     * The 2nd WM_WINDOWPOSCHANGING is unconditional, but the comparison
     * function is too simple to roll back an accepted message, so we have
     * to mark the 2nd message optional. */
    { WM_SIZE, sent|optional },
    { WM_WINDOWPOSCHANGING, sent|id, 0, 0, CHILD1_ID }, /* Actually optional. */
    { WM_CHILDACTIVATE, sent|id, 0, 0, CHILD1_ID }, /* Actually optional. */
    { WM_WINDOWPOSCHANGED, sent|id|optional, TRUE, 0, CHILD1_ID},
    { WM_MOVE, sent|id|optional|defwinproc, 0, 0, CHILD1_ID },
    { WM_WINDOWPOSCHANGING, sent|id|optional, 0, 0, CHILD1_ID }, /* Actually not optional. */
    { WM_CHILDACTIVATE, sent|id|optional, 0, 0, CHILD1_ID }, /* Actually not optional. */
    { 0 }
};

static const struct message set_pos_empty_seq[] = {
    { PGM_SETPOS, sent },
    { 0 }
};

static LRESULT WINAPI parent_wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;

    /* log system messages, except for painting */
    if (message < WM_USER &&
        message != WM_PAINT &&
        message != WM_ERASEBKGND &&
        message != WM_NCPAINT &&
        message != WM_NCHITTEST &&
        message != WM_GETTEXT &&
        message != WM_GETICON &&
        message != WM_DEVICECHANGE)
    {
        msg.message = message;
        msg.flags = sent|wparam|lparam|parent;
        if (defwndproc_counter) msg.flags |= defwinproc;
        msg.wParam = wParam;
        msg.lParam = lParam;
        if (message == WM_NOTIFY && lParam) msg.id = ((NMHDR*)lParam)->code;
        add_message(sequences, PAGER_SEQ_INDEX, &msg);
    }

    if (message == WM_NOTIFY)
    {
        NMHDR *nmhdr = (NMHDR *)lParam;

        switch (nmhdr->code)
        {
            case PGN_CALCSIZE:
            {
                NMPGCALCSIZE *nmpgcs = (NMPGCALCSIZE *)lParam;
                DWORD style = GetWindowLongA(nmpgcs->hdr.hwndFrom, GWL_STYLE);

                if (style & PGS_HORZ)
                    ok(nmpgcs->dwFlag == PGF_CALCWIDTH, "Unexpected flags %#x.\n", nmpgcs->dwFlag);
                else
                    ok(nmpgcs->dwFlag == PGF_CALCHEIGHT, "Unexpected flags %#x.\n", nmpgcs->dwFlag);
                break;
            }
            default:
                ;
        }
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
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "Pager test parent class";
    return RegisterClassA(&cls);
}

static HWND create_parent_window(void)
{
    if (!register_parent_wnd_class())
        return NULL;

    return CreateWindowA("Pager test parent class", "Pager test parent window",
                        WS_OVERLAPPED | WS_VISIBLE,
                        0, 0, 200, 200, 0, NULL, GetModuleHandleA(NULL), NULL );
}

static LRESULT WINAPI pager_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldproc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    struct message msg = { 0 };

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    msg.wParam = wParam;
    msg.lParam = lParam;
    add_message(sequences, PAGER_SEQ_INDEX, &msg);
    return CallWindowProcA(oldproc, hwnd, message, wParam, lParam);
}

static HWND create_pager_control( DWORD style )
{
    WNDPROC oldproc;
    HWND hwnd;
    RECT rect;

    GetClientRect( parent_wnd, &rect );
    hwnd = CreateWindowA( WC_PAGESCROLLERA, "pager", WS_CHILD | WS_BORDER | WS_VISIBLE | style,
                          0, 0, 100, 100, parent_wnd, 0, GetModuleHandleA(0), 0 );
    oldproc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)pager_subclass_proc);
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)oldproc);
    return hwnd;
}

static LRESULT WINAPI child_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter;
    struct message msg = { 0 };
    LRESULT ret;

    msg.message = message;
    msg.flags = sent | wparam | lparam;
    if (defwndproc_counter)
        msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;

    if (hwnd == child1_wnd)
        msg.id = CHILD1_ID;
    else if (hwnd == child2_wnd)
        msg.id = CHILD2_ID;
    else
        msg.id = 0;

    add_message(sequences, PAGER_SEQ_INDEX, &msg);

    defwndproc_counter++;
    ret = DefWindowProcA(hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static BOOL register_child_wnd_class(void)
{
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = child_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "Pager test child class";
    return RegisterClassA(&cls);
}

static void test_pager(void)
{
    HWND pager;
    RECT rect, rect2;

    pager = create_pager_control( PGS_HORZ );
    ok(pager != NULL, "Fail to create pager\n");

    register_child_wnd_class();

    child1_wnd = CreateWindowA( "Pager test child class", "button", WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 300, 300,
                           pager, 0, GetModuleHandleA(0), 0 );
    child2_wnd = CreateWindowA("Pager test child class", "button", WS_CHILD | WS_BORDER, 0, 0, 300, 300,
        pager, 0, GetModuleHandleA(0), 0);

    flush_sequences( sequences, NUM_MSG_SEQUENCES );
    SendMessageA( pager, PGM_SETCHILD, 0, (LPARAM)child1_wnd );
    ok_sequence(sequences, PAGER_SEQ_INDEX, set_child_seq, "set child", FALSE);
    GetWindowRect( pager, &rect );
    ok( rect.right - rect.left == 100 && rect.bottom - rect.top == 100,
        "pager resized %dx%d\n", rect.right - rect.left, rect.bottom - rect.top );

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    SendMessageA(pager, PGM_SETCHILD, 0, (LPARAM)child2_wnd);
    ok_sequence(sequences, PAGER_SEQ_INDEX, switch_child_seq, "switch to invisible child", FALSE);
    GetWindowRect(pager, &rect);
    ok(rect.right - rect.left == 100 && rect.bottom - rect.top == 100,
        "pager resized %dx%d\n", rect.right - rect.left, rect.bottom - rect.top);
    ok(!IsWindowVisible(child2_wnd), "Child window 2 is visible\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    SendMessageA(pager, PGM_SETCHILD, 0, (LPARAM)child1_wnd);
    ok_sequence(sequences, PAGER_SEQ_INDEX, set_child_seq, "switch to visible child", FALSE);
    GetWindowRect(pager, &rect);
    ok(rect.right - rect.left == 100 && rect.bottom - rect.top == 100,
        "pager resized %dx%d\n", rect.right - rect.left, rect.bottom - rect.top);

    flush_sequences( sequences, NUM_MSG_SEQUENCES );
    SendMessageA( pager, PGM_SETPOS, 0, 10 );
    ok_sequence(sequences, PAGER_SEQ_INDEX, set_pos_seq, "set pos", TRUE);
    GetWindowRect( pager, &rect );
    ok( rect.right - rect.left == 100 && rect.bottom - rect.top == 100,
        "pager resized %dx%d\n", rect.right - rect.left, rect.bottom - rect.top );

    flush_sequences( sequences, NUM_MSG_SEQUENCES );
    SendMessageA( pager, PGM_SETPOS, 0, 10 );
    ok_sequence(sequences, PAGER_SEQ_INDEX, set_pos_empty_seq, "set pos empty", TRUE);

    flush_sequences( sequences, NUM_MSG_SEQUENCES );
    SendMessageA( pager, PGM_SETPOS, 0, 9 );
    ok_sequence(sequences, PAGER_SEQ_INDEX, set_pos_seq, "set pos", TRUE);

    DestroyWindow( pager );

    /* Test if resizing works */
    pager = create_pager_control( CCS_NORESIZE );
    ok(pager != NULL, "failed to create pager control\n");

    GetWindowRect( pager, &rect );
    MoveWindow( pager, 0, 0, 200, 100, TRUE );
    GetWindowRect( pager, &rect2 );
    ok(rect2.right - rect2.left > rect.right - rect.left, "expected pager window to resize, %s\n",
        wine_dbgstr_rect( &rect2 ));

    DestroyWindow( pager );

    pager = create_pager_control( CCS_NORESIZE | PGS_HORZ );
    ok(pager != NULL, "failed to create pager control\n");

    GetWindowRect( pager, &rect );
    MoveWindow( pager, 0, 0, 100, 200, TRUE );
    GetWindowRect( pager, &rect2 );
    ok(rect2.bottom - rect2.top > rect.bottom - rect.top, "expected pager window to resize, %s\n",
        wine_dbgstr_rect( &rect2 ));

    DestroyWindow( pager );
}

static LRESULT WINAPI test_notifyformat_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_NOTIFYFORMAT:
        if (lParam == NF_QUERY)
        {
            notify_query_received = TRUE;
            return notify_format;
        }
        else if (lParam == NF_REQUERY)
            return SendMessageA(GetParent(hwnd), WM_NOTIFYFORMAT, (WPARAM)hwnd, NF_QUERY);
        else
            return 0;
    default:
        return notify_format == NFR_UNICODE ? DefWindowProcW(hwnd, message, wParam, lParam)
                                            : DefWindowProcA(hwnd, message, wParam, lParam);
    }
}

static BOOL register_notifyformat_class(void)
{
    static const WCHAR class_w[] = {'P', 'a', 'g', 'e', 'r', ' ', 'n', 'o', 't', 'i', 'f', 'y', 'f',
                                   'o', 'r', 'm', 'a', 't', ' ', 'c', 'l', 'a', 's', 's', 0};
    WNDCLASSW cls = {0};

    cls.lpfnWndProc = test_notifyformat_proc;
    cls.hInstance = GetModuleHandleW(NULL);
    cls.lpszClassName = class_w;
    return RegisterClassW(&cls);
}

static void test_wm_notifyformat(void)
{
    static const WCHAR class_w[] = {'P', 'a', 'g', 'e', 'r', ' ', 'n', 'o', 't', 'i', 'f', 'y', 'f',
                                    'o', 'r', 'm', 'a', 't', ' ', 'c', 'l', 'a', 's', 's', 0};
    static const WCHAR parent_w[] = {'p', 'a', 'r', 'e', 'n', 't', 0};
    static const WCHAR pager_w[] = {'p', 'a', 'g', 'e', 'r', 0};
    static const WCHAR child_w[] = {'c', 'h', 'i', 'l', 'd', 0};
    static const INT formats[] = {NFR_UNICODE, NFR_ANSI};
    HWND parent, pager, child;
    LRESULT ret;
    INT i;

    ok(register_notifyformat_class(), "Register test class failed, error 0x%08x\n", GetLastError());

    for (i = 0; i < ARRAY_SIZE(formats); i++)
    {
        notify_format = formats[i];
        parent = CreateWindowW(class_w, parent_w, WS_OVERLAPPED, 0, 0, 100, 100, 0, 0, GetModuleHandleW(0), 0);
        ok(parent != NULL, "CreateWindow failed\n");
        pager = CreateWindowW(WC_PAGESCROLLERW, pager_w, WS_CHILD, 0, 0, 100, 100, parent, 0, GetModuleHandleW(0), 0);
        ok(pager != NULL, "CreateWindow failed\n");
        child = CreateWindowW(class_w, child_w, WS_CHILD, 0, 0, 100, 100, pager, 0, GetModuleHandleW(0), 0);
        ok(child != NULL, "CreateWindow failed\n");
        SendMessageW(pager, PGM_SETCHILD, 0, (LPARAM)child);

        /* Test parent */
        notify_query_received = FALSE;
        ret = SendMessageW(pager, WM_NOTIFYFORMAT, (WPARAM)parent, NF_REQUERY);
        ok(ret == notify_format, "Expect %d, got %ld\n", notify_format, ret);
        ok(notify_query_received, "Didn't receive notify\n");

        /* Send NF_QUERY directly to parent */
        notify_query_received = FALSE;
        ret = SendMessageW(parent, WM_NOTIFYFORMAT, (WPARAM)pager, NF_QUERY);
        ok(ret == notify_format, "Expect %d, got %ld\n", notify_format, ret);
        ok(notify_query_received, "Didn't receive notify\n");

        /* Pager send notifications to its parent regardless of wParam */
        notify_query_received = FALSE;
        ret = SendMessageW(pager, WM_NOTIFYFORMAT, (WPARAM)parent_wnd, NF_REQUERY);
        ok(ret == notify_format, "Expect %d, got %ld\n", notify_format, ret);
        ok(notify_query_received, "Didn't receive notify\n");

        /* Pager always wants Unicode notifications from children */
        ret = SendMessageW(child, WM_NOTIFYFORMAT, (WPARAM)pager, NF_REQUERY);
        ok(ret == NFR_UNICODE, "Expect %d, got %ld\n", NFR_UNICODE, ret);
        ret = SendMessageW(pager, WM_NOTIFYFORMAT, (WPARAM)child, NF_QUERY);
        ok(ret == NFR_UNICODE, "Expect %d, got %ld\n", NFR_UNICODE, ret);

        DestroyWindow(parent);
    }

    UnregisterClassW(class_w, GetModuleHandleW(NULL));
}

static void notify_generic_text_handler(CHAR **text, INT *text_max)
{
    const struct notify_test_send *send_data;
    const struct notify_test_receive *receive_data;

    switch (notify_test_info.test_id)
    {
    case CONVERT_SEND:
    case DONT_CONVERT_SEND:
    {
        send_data = (notify_test_info.test_id == CONVERT_SEND ? test_convert_send_data : test_dont_convert_send_data)
                    + notify_test_info.sub_test_id;
        if (notify_test_info.flags & CONVERT_SEND)
            ok(!lstrcmpA(send_data->expect_text, *text), "Code 0x%08x test 0x%08x sub test %d expect %s, got %s\n",
               notify_test_info.unicode, notify_test_info.test_id, notify_test_info.sub_test_id,
               (CHAR *)send_data->expect_text, *text);
        else
            ok(!lstrcmpW((WCHAR *)send_data->expect_text, (WCHAR *)*text),
               "Code 0x%08x test 0x%08x sub test %d expect %s, got %s\n", notify_test_info.unicode,
               notify_test_info.test_id, notify_test_info.sub_test_id, wine_dbgstr_w((WCHAR *)send_data->expect_text),
               wine_dbgstr_w((WCHAR *)*text));
        if (text_max)
            ok(*text_max == send_data->send_text_max, "Code 0x%08x test 0x%08x sub test %d expect %d, got %d\n",
               notify_test_info.unicode, notify_test_info.test_id, notify_test_info.sub_test_id,
               send_data->send_text_max, *text_max);
        break;
    }
    case CONVERT_RECEIVE:
    case DONT_CONVERT_RECEIVE:
    {
        receive_data = (notify_test_info.test_id == CONVERT_RECEIVE ? test_convert_receive_data : test_dont_convert_receive_data)
                       + notify_test_info.sub_test_id;
        if (text_max)
            ok(*text_max == receive_data->send_text_max, "Code 0x%08x test 0x%08x sub test %d expect %d, got %d\n",
               notify_test_info.unicode, notify_test_info.test_id, notify_test_info.sub_test_id,
               receive_data->send_text_max, *text_max);

        if (receive_data->write_text)
            memcpy(*text, receive_data->write_text, receive_data->write_text_size);
        else
            *text = receive_data->write_pointer;
        if (text_max && receive_data->write_text_max != -1) *text_max = receive_data->write_text_max;
        break;
    }
    case SEND_EMPTY_IF_NULL:
        ok(!lstrcmpA(*text, empty_a), "Code 0x%08x test 0x%08x sub test %d expect empty text, got %s\n",
           notify_test_info.unicode, notify_test_info.test_id, notify_test_info.sub_test_id, *text);
        break;
    case DONT_SEND_EMPTY_IF_NULL:
        ok(!*text, "Code 0x%08x test 0x%08x sub test %d expect null text\n", notify_test_info.unicode,
           notify_test_info.test_id, notify_test_info.sub_test_id);
        break;
    }
}

static LRESULT WINAPI test_notify_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static const WCHAR test[] = {'t', 'e', 's', 't', 0};
    switch (message)
    {
    case WM_NOTIFY:
    {
        NMHDR *hdr = (NMHDR *)lParam;

        /* Not notifications we want to test */
        if (!notify_test_info.unicode) break;
        ok(!notify_test_info.received, "Extra notification received\n");

        ok(wParam == notify_test_info.id_from, "Expect %ld, got %ld\n", notify_test_info.id_from, wParam);
        ok(hdr->code == notify_test_info.ansi, "Expect 0x%08x, got 0x%08x\n", notify_test_info.ansi, hdr->code);
        ok(hdr->idFrom == notify_test_info.id_from, "Expect %ld, got %ld\n", notify_test_info.id_from, wParam);
        ok(hdr->hwndFrom == notify_test_info.hwnd_from, "Expect %p, got %p\n", notify_test_info.hwnd_from, hdr->hwndFrom);

        if (hdr->code != notify_test_info.ansi)
        {
            skip("Notification code mismatch, skipping lParam check\n");
            return 0;
        }
        switch (hdr->code)
        {
        /* Toolbar */
        case TBN_SAVE:
        {
            NMTBSAVE *nmtbs = (NMTBSAVE *)hdr;
            notify_generic_text_handler((CHAR **)&nmtbs->tbButton.iString, NULL);
            break;
        }
        case TBN_RESTORE:
        {
            NMTBRESTORE *nmtbr = (NMTBRESTORE *)hdr;
            notify_generic_text_handler((CHAR **)&nmtbr->tbButton.iString, NULL);
            break;
        }
        case TBN_GETBUTTONINFOA:
        {
            NMTOOLBARA *nmtb = (NMTOOLBARA *)hdr;
            notify_generic_text_handler(&nmtb->pszText, &nmtb->cchText);
            break;
        }
        case TBN_GETDISPINFOW:
        {
            NMTBDISPINFOW *nmtbdi = (NMTBDISPINFOW *)hdr;
            notify_generic_text_handler((CHAR **)&nmtbdi->pszText, &nmtbdi->cchText);
            break;
        }
        case TBN_GETINFOTIPA:
        {
            NMTBGETINFOTIPA *nmtbgit = (NMTBGETINFOTIPA *)hdr;
            notify_generic_text_handler(&nmtbgit->pszText, &nmtbgit->cchTextMax);
            break;
        }
        default:
            ok(0, "Unexpected message 0x%08x\n", hdr->code);
        }
        notify_test_info.received = TRUE;
        ok(!lstrcmpA(test_a, "test"), "test_a got modified\n");
        ok(!lstrcmpW(test_w, test), "test_w got modified\n");
        return 0;
    }
    case WM_NOTIFYFORMAT:
        if (lParam == NF_QUERY) return NFR_ANSI;
        break;
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

static BOOL register_test_notify_class(void)
{
    WNDCLASSA cls = {0};

    cls.lpfnWndProc = test_notify_proc;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.lpszClassName = "Pager notify class";
    return RegisterClassA(&cls);
}

static void send_notify(HWND pager, UINT unicode, UINT ansi, LPARAM lParam, BOOL code_change)
{
    NMHDR *hdr = (NMHDR *)lParam;

    notify_test_info.unicode = unicode;
    notify_test_info.id_from = 1;
    notify_test_info.hwnd_from = child1_wnd;
    notify_test_info.ansi = ansi;
    notify_test_info.received = FALSE;

    hdr->code = unicode;
    hdr->idFrom = 1;
    hdr->hwndFrom = child1_wnd;

    SendMessageW(pager, WM_NOTIFY, hdr->idFrom, lParam);
    ok(notify_test_info.received, "Expect notification received\n");
    ok(hdr->code == code_change ? ansi : unicode, "Expect 0x%08x, got 0x%08x\n", hdr->code,
       code_change ? ansi : unicode);
}

/* Send notify to test text field conversion. In parent proc notify_generic_text_handler() handles these messages */
static void test_notify_generic_text_helper(HWND pager, const struct generic_text_helper_para *para)
{
    const struct notify_test_send *send_data;
    const struct notify_test_receive *receive_data;
    INT array_size;
    INT i;

    notify_test_info.flags = para->flags;

    if (para->flags & (CONVERT_SEND | DONT_CONVERT_SEND))
    {
        if (para->flags & CONVERT_SEND)
        {
            notify_test_info.test_id = CONVERT_SEND;
            send_data = test_convert_send_data;
            array_size = ARRAY_SIZE(test_convert_send_data);
        }
        else
        {
            notify_test_info.test_id = DONT_CONVERT_SEND;
            send_data = test_dont_convert_send_data;
            array_size = ARRAY_SIZE(test_dont_convert_send_data);
        }

        for (i = 0; i < array_size; i++)
        {
            const struct notify_test_send *data = send_data + i;
            notify_test_info.sub_test_id = i;

            memset(para->ptr, 0, para->size);
            if (para->mask) *para->mask = para->required_mask;
            if (data->send_text)
            {
                memcpy(buffer, data->send_text, data->send_text_size);
                *para->text = buffer;
            }
            if (para->text_max) *para->text_max = data->send_text_max;
            send_notify(pager, para->code_unicode, para->code_ansi, (LPARAM)para->ptr, TRUE);
        }
    }

    if (para->flags & (CONVERT_RECEIVE | DONT_CONVERT_RECEIVE))
    {
        if (para->flags & CONVERT_RECEIVE)
        {
            notify_test_info.test_id = CONVERT_RECEIVE;
            receive_data = test_convert_receive_data;
            array_size = ARRAY_SIZE(test_convert_receive_data);
        }
        else
        {
            notify_test_info.test_id = DONT_CONVERT_RECEIVE;
            receive_data = test_dont_convert_receive_data;
            array_size = ARRAY_SIZE(test_dont_convert_receive_data);
        }

        for (i = 0; i < array_size; i++)
        {
            const struct notify_test_receive *data = receive_data + i;
            notify_test_info.sub_test_id = i;

            memset(para->ptr, 0, para->size);
            if (para->mask) *para->mask = para->required_mask;
            if (data->send_text)
            {
                memcpy(buffer, data->send_text, data->send_text_size);
                *para->text = buffer;
            }
            if (para->text_max) *para->text_max = data->send_text_max;
            send_notify(pager, para->code_unicode, para->code_ansi, (LPARAM)para->ptr, TRUE);
            if (data->return_text)
            {
                if (para->flags & CONVERT_RECEIVE)
                    ok(!lstrcmpW(data->return_text, *para->text), "Code 0x%08x sub test %d expect %s, got %s\n",
                       para->code_unicode, i, wine_dbgstr_w((WCHAR *)data->return_text), wine_dbgstr_w(*para->text));
                else
                    ok(!lstrcmpA(data->return_text, (CHAR *)*para->text), "Code 0x%08x sub test %d expect %s, got %s\n",
                       para->code_unicode, i, (CHAR *)data->return_text, (CHAR *)*para->text);
            }
            if (para->text_max)
                ok(data->return_text_max == *para->text_max, "Code 0x%08x sub test %d expect %d, got %d\n",
                   para->code_unicode, i, data->return_text_max, *para->text_max);
        }
    }

    /* Extra tests for other behavior flags that are not worth it to create their own test arrays */
    memset(para->ptr, 0, para->size);
    if (para->mask) *para->mask = para->required_mask;
    if (para->text_max) *para->text_max = 1;
    if (para->flags & SEND_EMPTY_IF_NULL)
        notify_test_info.test_id = SEND_EMPTY_IF_NULL;
    else
        notify_test_info.test_id = DONT_SEND_EMPTY_IF_NULL;
    send_notify(pager, para->code_unicode, para->code_ansi, (LPARAM)para->ptr, TRUE);
}

static void test_wm_notify(void)
{
    static const CHAR *class = "Pager notify class";
    HWND parent, pager;
    /* Tool Bar */
    static NMTBRESTORE nmtbr;
    static NMTBSAVE nmtbs;
    static NMTOOLBARW nmtb;
    static NMTBDISPINFOW nmtbdi;
    static NMTBGETINFOTIPW nmtbgit;
    static const struct generic_text_helper_para paras[] =
    {
        /* Tool Bar */
        {&nmtbs, sizeof(nmtbs), NULL, 0, (WCHAR **)&nmtbs.tbButton.iString, NULL, TBN_SAVE, TBN_SAVE,
         DONT_CONVERT_SEND | DONT_CONVERT_RECEIVE},
        {&nmtbr, sizeof(nmtbr), NULL, 0, (WCHAR **)&nmtbr.tbButton.iString, NULL, TBN_RESTORE, TBN_RESTORE,
         DONT_CONVERT_SEND | DONT_CONVERT_RECEIVE},
        {&nmtbdi, sizeof(nmtbdi), &nmtbdi.dwMask, TBNF_TEXT, &nmtbdi.pszText, &nmtbdi.cchText, TBN_GETDISPINFOW,
         TBN_GETDISPINFOW, DONT_CONVERT_SEND | DONT_CONVERT_RECEIVE},
        {&nmtb, sizeof(nmtb), NULL, 0, &nmtb.pszText, &nmtb.cchText, TBN_GETBUTTONINFOW, TBN_GETBUTTONINFOA,
         SEND_EMPTY_IF_NULL | CONVERT_SEND | CONVERT_RECEIVE},
        {&nmtbgit, sizeof(nmtbgit), NULL, 0, &nmtbgit.pszText, &nmtbgit.cchTextMax, TBN_GETINFOTIPW, TBN_GETINFOTIPA,
         DONT_CONVERT_SEND | CONVERT_RECEIVE}
    };
    INT i;

    ok(register_test_notify_class(), "Register test class failed, error 0x%08x\n", GetLastError());

    parent = CreateWindowA(class, "parent", WS_OVERLAPPED, 0, 0, 100, 100, 0, 0, GetModuleHandleA(0), 0);
    ok(parent != NULL, "CreateWindow failed\n");
    pager = CreateWindowA(WC_PAGESCROLLERA, "pager", WS_CHILD, 0, 0, 100, 100, parent, 0, GetModuleHandleA(0), 0);
    ok(pager != NULL, "CreateWindow failed\n");
    child1_wnd = CreateWindowA(class, "child", WS_CHILD, 0, 0, 100, 100, pager, (HMENU)1, GetModuleHandleA(0), 0);
    ok(child1_wnd != NULL, "CreateWindow failed\n");
    SendMessageW(pager, PGM_SETCHILD, 0, (LPARAM)child1_wnd);

    for (i = 0; i < ARRAY_SIZE(paras); i++)
        test_notify_generic_text_helper(pager, paras + i);

    DestroyWindow(parent);
    UnregisterClassA(class, GetModuleHandleA(NULL));
}

static void init_functions(void)
{
    HMODULE mod = LoadLibraryA("comctl32.dll");

#define X(f) p##f = (void*)GetProcAddress(mod, #f);
    X(InitCommonControlsEx);
#undef X

    pSetWindowSubclass = (void*)GetProcAddress(mod, (LPSTR)410);
}

START_TEST(pager)
{
    INITCOMMONCONTROLSEX iccex;

    init_functions();

    iccex.dwSize = sizeof(iccex);
    iccex.dwICC = ICC_PAGESCROLLER_CLASS;
    pInitCommonControlsEx(&iccex);

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    parent_wnd = create_parent_window();
    ok(parent_wnd != NULL, "Failed to create parent window!\n");

    test_pager();
    test_wm_notifyformat();
    test_wm_notify();

    DestroyWindow(parent_wnd);
}
