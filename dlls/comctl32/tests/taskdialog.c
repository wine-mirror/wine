/* Unit tests for the task dialog control.
 *
 * Copyright 2017 Fabian Maurer for the Wine project
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "commctrl.h"

#include "wine/test.h"
#include "v6util.h"
#include "msg.h"

#define WM_TD_CALLBACK (WM_APP) /* Custom dummy message to wrap callback notifications */

#define NUM_MSG_SEQUENCES     1
#define TASKDIALOG_SEQ_INDEX  0

static HRESULT (WINAPI *pTaskDialogIndirect)(const TASKDIALOGCONFIG *, int *, int *, BOOL *);
static HRESULT (WINAPI *pTaskDialog)(HWND, HINSTANCE, const WCHAR *, const WCHAR *, const WCHAR *,
        TASKDIALOG_COMMON_BUTTON_FLAGS, const WCHAR *, int *);

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

struct message_info
{
    UINT message;
    WPARAM wparam;
    LPARAM lparam;

    HRESULT callback_retval;
    const struct message_info *send;  /* Message to send to trigger the next callback message */
};

static const struct message_info *current_message_info;

/* Messages to send */
static const struct message_info msg_send_return[] =
{
    { WM_KEYDOWN, VK_RETURN, 0 },
    { 0 }
};

/* Messages to test against */
static const struct message_info msg_return_press_ok[] =
{
    { TDN_CREATED,        0,    0, S_OK, msg_send_return },
    { TDN_BUTTON_CLICKED, IDOK, 0, S_OK, NULL },
    { 0 }
};

static void init_test_message(UINT message, WPARAM wParam, LPARAM lParam, struct message *msg)
{
    msg->message = WM_TD_CALLBACK;
    msg->flags = sent|wparam|lparam|id;
    msg->wParam = wParam;
    msg->lParam = lParam;
    msg->id = message;
    msg->stage = 0;
}

#define run_test(info, expect_button, expect_radio, seq, context) \
        run_test_(info, expect_button, expect_radio, seq, context, \
                  sizeof(seq)/sizeof(seq[0]) - 1, __FILE__, __LINE__)

void run_test_(TASKDIALOGCONFIG *info, int expect_button, int expect_radio, const struct message_info *test_messages,
    const char *context, int test_messages_len, const char *file, int line)
{
    struct message *msg, *msg_start;
    int ret_button = 0;
    int ret_radio = 0;
    HRESULT hr;
    int i;

    /* Allocate messages to test against, plus 2 implicit and 1 empty */
    msg_start = msg = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*msg) * (test_messages_len + 3));

    /* Always needed, thus made implicit */
    init_test_message(TDN_DIALOG_CONSTRUCTED, 0, 0, msg++);
    for (i = 0; i < test_messages_len; i++)
        init_test_message(test_messages[i].message, test_messages[i].wparam, test_messages[i].lparam, msg++);
    /* Always needed, thus made implicit */
    init_test_message(TDN_DESTROYED, 0, 0, msg++);

    current_message_info = test_messages;
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    hr = pTaskDialogIndirect(info, &ret_button, &ret_radio, NULL);
    ok_(file, line)(hr == S_OK, "TaskDialogIndirect() failed, got %#x.\n", hr);

    ok_sequence_(sequences, TASKDIALOG_SEQ_INDEX, msg_start, context, FALSE, file, line);
    ok_(file, line)(ret_button == expect_button,
                     "Wrong button. Expected %d, got %d\n", expect_button, ret_button);
    ok_(file, line)(ret_radio == expect_radio,
                     "Wrong radio button. Expected %d, got %d\n", expect_radio, ret_radio);

    HeapFree(GetProcessHeap(), 0, msg_start);
}

static const LONG_PTR test_ref_data = 123456;

static HRESULT CALLBACK taskdialog_callback_proc(HWND hwnd, UINT notification,
    WPARAM wParam, LPARAM lParam, LONG_PTR ref_data)
{
    int msg_pos = sequences[TASKDIALOG_SEQ_INDEX]->count - 1; /* Skip implicit message */
    const struct message_info *msg_send;
    struct message msg;

    ok(test_ref_data == ref_data, "Unexpected ref data %lu.\n", ref_data);

    init_test_message(notification, wParam, lParam, &msg);
    add_message(sequences, TASKDIALOG_SEQ_INDEX, &msg);

    if (notification == TDN_DIALOG_CONSTRUCTED || notification == TDN_DESTROYED) /* Skip implicit messages */
        return S_OK;

    msg_send = current_message_info[msg_pos].send;
    for(; msg_send && msg_send->message; msg_send++)
        PostMessageW(hwnd, msg_send->message, msg_send->wparam, msg_send->lparam);

    return current_message_info[msg_pos].callback_retval;
}

static void test_invalid_parameters(void)
{
    TASKDIALOGCONFIG info = { 0 };
    HRESULT hr;

    hr = pTaskDialogIndirect(NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected return value %#x.\n", hr);

    info.cbSize = 0;
    hr = pTaskDialogIndirect(&info, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected return value %#x.\n", hr);

    info.cbSize = sizeof(TASKDIALOGCONFIG) - 1;
    hr = pTaskDialogIndirect(&info, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected return value %#x.\n", hr);

    info.cbSize = sizeof(TASKDIALOGCONFIG) + 1;
    hr = pTaskDialogIndirect(&info, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected return value %#x.\n", hr);
}

static void test_callback(void)
{
    TASKDIALOGCONFIG info = {0};

    info.cbSize = sizeof(TASKDIALOGCONFIG);
    info.pfCallback = taskdialog_callback_proc;
    info.lpCallbackData = test_ref_data;

    run_test(&info, IDOK, 0, msg_return_press_ok, "Press VK_RETURN.");
}

START_TEST(taskdialog)
{
    ULONG_PTR ctx_cookie;
    void *ptr_ordinal;
    HINSTANCE hinst;
    HANDLE hCtx;

    if (!load_v6_module(&ctx_cookie, &hCtx))
        return;

    /* Check if task dialogs are available */
    hinst = LoadLibraryA("comctl32.dll");

    pTaskDialogIndirect = (void *)GetProcAddress(hinst, "TaskDialogIndirect");
    if (!pTaskDialogIndirect)
    {
        win_skip("TaskDialogIndirect not exported by name\n");
        unload_v6_module(ctx_cookie, hCtx);
        return;
    }

    pTaskDialog = (void *)GetProcAddress(hinst, "TaskDialog");

    ptr_ordinal = GetProcAddress(hinst, (const char *)344);
    ok(pTaskDialog == ptr_ordinal, "got wrong pointer for ordinal 344, %p expected %p\n",
                                            ptr_ordinal, pTaskDialog);

    ptr_ordinal = GetProcAddress(hinst, (const char *)345);
    ok(pTaskDialogIndirect == ptr_ordinal, "got wrong pointer for ordinal 345, %p expected %p\n",
                                            ptr_ordinal, pTaskDialogIndirect);

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    test_invalid_parameters();
    test_callback();

    unload_v6_module(ctx_cookie, hCtx);
}
