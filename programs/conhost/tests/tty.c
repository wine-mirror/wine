/*
 * 2020 Jacek Caban for CodeWeavers
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

#include "wine/test.h"

#include <windows.h>

static HRESULT (WINAPI *pCreatePseudoConsole)(COORD,HANDLE,HANDLE,DWORD,HPCON*);
static void (WINAPI *pClosePseudoConsole)(HPCON);

static char console_output[4096];
static unsigned int console_output_count;
static HANDLE console_pipe;
static HANDLE child_pipe;

#define fetch_console_output() fetch_console_output_(__LINE__)
static void fetch_console_output_(unsigned int line)
{
    OVERLAPPED o;
    DWORD count;
    BOOL ret;

    if (console_output_count == sizeof(console_output)) return;

    memset(&o, 0, sizeof(o));
    o.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    ret = ReadFile(console_pipe, console_output + console_output_count,
                   sizeof(console_output) - console_output_count, NULL, &o);
    if (!ret)
    {
        ok_(__FILE__,line)(GetLastError() == ERROR_IO_PENDING, "read failed: %lu\n", GetLastError());
        if (GetLastError() != ERROR_IO_PENDING) return;
        WaitForSingleObject(o.hEvent, 5000);
    }
    ret = GetOverlappedResult(console_pipe, &o, &count, FALSE);
    if (!ret && GetLastError() == ERROR_IO_INCOMPLETE)
        CancelIoEx(console_pipe, &o);

    ok_(__FILE__,line)(ret, "Read file failed: %lu\n", GetLastError());
    CloseHandle(o.hEvent);
    if (ret) console_output_count += count;
}

#define expect_empty_output() expect_empty_output_(__LINE__)
static void expect_empty_output_(unsigned int line)
{
    DWORD avail;
    BOOL ret;

    ret = PeekNamedPipe(console_pipe, NULL, 0, NULL, &avail, NULL);
    ok_(__FILE__,line)(ret, "PeekNamedPipe failed: %lu\n", GetLastError());
    ok_(__FILE__,line)(!avail, "avail = %lu\n", avail);
    if (avail) fetch_console_output_(line);
    ok_(__FILE__,line)(!console_output_count, "expected empty buffer, got %s\n",
                       wine_dbgstr_an(console_output, console_output_count));
    console_output_count = 0;
}

#define expect_output_sequence(a) expect_output_sequence_(__LINE__,0,a)
#define expect_output_sequence_ctx(a,b) expect_output_sequence_(__LINE__,a,b)
static void expect_output_sequence_(unsigned int line, unsigned ctx, const char *expect)
{
    size_t len = strlen(expect);
    if (console_output_count < len) fetch_console_output_(line);
    if (len <= console_output_count && !memcmp(console_output, expect, len))
    {
        console_output_count -= len;
        memmove(console_output, console_output + len, console_output_count);
    }
    else ok_(__FILE__,line)(0, "%x: expected %s got %s\n", ctx, wine_dbgstr_a(expect),
                            wine_dbgstr_an(console_output, console_output_count));
}

#define skip_sequence(a) skip_sequence_(__LINE__,a)
static BOOL skip_sequence_(unsigned int line, const char *expect)
{
    size_t len = strlen(expect);
    DWORD avail;
    BOOL r;

    r = PeekNamedPipe(console_pipe, NULL, 0, NULL, &avail, NULL);
    if (!console_output_count && r && !avail)
    {
        Sleep(50);
        r = PeekNamedPipe(console_pipe, NULL, 0, NULL, &avail, NULL);
    }
    if (r && avail) fetch_console_output_(line);

    if (len > console_output_count || memcmp(console_output, expect, len)) return FALSE;
    console_output_count -= len;
    memmove(console_output, console_output + len, console_output_count);
    return TRUE;
}

#define skip_byte(a) skip_byte_(__LINE__,a)
static BOOL skip_byte_(unsigned int line, char ch)
{
    if (!console_output_count || console_output[0] != ch) return FALSE;
    console_output_count--;
    memmove(console_output, console_output + 1, console_output_count);
    return TRUE;
}

#define expect_hide_cursor() expect_hide_cursor_(__LINE__)
static void expect_hide_cursor_(unsigned int line)
{
    if (!console_output_count) fetch_console_output_(line);
    ok_(__FILE__,line)(skip_sequence_(line, "\x1b[?25l") || broken(skip_sequence_(line, "\x1b[25l")),
                       "expected hide cursor escape\n");
}

#define skip_hide_cursor() skip_hide_cursor_(__LINE__)
static BOOL skip_hide_cursor_(unsigned int line)
{
    if (!console_output_count) fetch_console_output_(line);
    return skip_sequence_(line, "\x1b[25l") || broken(skip_sequence_(line, "\x1b[?25l"));
}

#define expect_erase_line(a) expect_erase_line_(__LINE__,a)
static BOOL expect_erase_line_(unsigned line, unsigned int cnt)
{
    char buf[16];
    if (skip_sequence("\x1b[K")) return FALSE;
    ok(broken(1), "expected erase line\n");
    sprintf(buf, "\x1b[%uX", cnt);
    expect_output_sequence_(line, cnt, buf);  /* erase the rest of the line */
    sprintf(buf, "\x1b[%uC", cnt);
    expect_output_sequence_(line, cnt, buf);  /* move cursor to the end of the line */
    return TRUE;
}

enum req_type
{
    REQ_CREATE_SCREEN_BUFFER,
    REQ_FILL_CHAR,
    REQ_GET_INPUT,
    REQ_GET_SB_INFO,
    REQ_READ_CONSOLE,
    REQ_READ_CONSOLE_A,
    REQ_READ_CONSOLE_FILE,
    REQ_READ_CONSOLE_CONTROL,
    REQ_SCROLL,
    REQ_SET_ACTIVE,
    REQ_SET_CURSOR,
    REQ_SET_INPUT_CP,
    REQ_SET_INPUT_MODE,
    REQ_SET_OUTPUT_MODE,
    REQ_SET_TITLE,
    REQ_WRITE_CHARACTERS,
    REQ_WRITE_CONSOLE,
    REQ_WRITE_OUTPUT,
};

struct pseudoconsole_req
{
    enum req_type type;
    union
    {
        WCHAR string[1];
        COORD coord;
        HANDLE handle;
        DWORD mode;
        int cp;
        size_t size;
        struct
        {
            COORD coord;
            unsigned int len;
            WCHAR buf[1];
        } write_characters;
        struct
        {
            COORD size;
            COORD coord;
            SMALL_RECT region;
            CHAR_INFO buf[1];
        } write_output;
        struct
        {
            SMALL_RECT rect;
            COORD dst;
            CHAR_INFO fill;
        } scroll;
        struct
        {
            WCHAR ch;
            DWORD count;
            COORD coord;
        } fill;
        struct
        {
            size_t size;
            DWORD mask;
            WCHAR initial[1];
        } control;
    } u;
};

static void child_string_request(enum req_type type, const WCHAR *title)
{
    char buf[4096];
    struct pseudoconsole_req *req = (void *)buf;
    size_t len = lstrlenW(title) + 1;
    DWORD count;
    BOOL ret;

    req->type = type;
    memcpy(req->u.string, title, len * sizeof(WCHAR));
    ret = WriteFile(child_pipe, req, FIELD_OFFSET(struct pseudoconsole_req, u.string[len]),
                    &count, NULL);
    ok(ret, "WriteFile failed: %lu\n", GetLastError());
}

static void child_write_characters(const WCHAR *buf, unsigned int x, unsigned int y)
{
    char req_buf[4096];
    struct pseudoconsole_req *req = (void *)req_buf;
    size_t len = lstrlenW(buf);
    DWORD count;
    BOOL ret;

    req->type = REQ_WRITE_CHARACTERS;
    req->u.write_characters.coord.X = x;
    req->u.write_characters.coord.Y = y;
    req->u.write_characters.len = len;
    memcpy(req->u.write_characters.buf, buf, len * sizeof(WCHAR));
    ret = WriteFile(child_pipe, req, FIELD_OFFSET(struct pseudoconsole_req, u.write_characters.buf[len + 1]),
                    &count, NULL);
    ok(ret, "WriteFile failed: %lu\n", GetLastError());
}

static void child_set_cursor(const unsigned int x, unsigned int y)
{
    struct pseudoconsole_req req;
    DWORD count;
    BOOL ret;

    req.type = REQ_SET_CURSOR;
    req.u.coord.X = x;
    req.u.coord.Y = y;
    ret = WriteFile(child_pipe, &req, sizeof(req), &count, NULL);
    ok(ret, "WriteFile failed: %lu\n", GetLastError());
}

static HANDLE child_create_screen_buffer(void)
{
    struct pseudoconsole_req req;
    HANDLE handle;
    DWORD count;
    BOOL ret;

    req.type = REQ_CREATE_SCREEN_BUFFER;
    ret = WriteFile(child_pipe, &req, sizeof(req), &count, NULL);
    ok(ret, "WriteFile failed: %lu\n", GetLastError());
    ret = ReadFile(child_pipe, &handle, sizeof(handle), &count, NULL);
    ok(ret, "ReadFile failed: %lu\n", GetLastError());
    return handle;
}

static void child_set_active(HANDLE handle)
{
    struct pseudoconsole_req req;
    DWORD count;
    BOOL ret;

    req.type = REQ_SET_ACTIVE;
    req.u.handle = handle;
    ret = WriteFile(child_pipe, &req, sizeof(req), &count, NULL);
    ok(ret, "WriteFile failed: %lu\n", GetLastError());
}

#define child_write_output(a,b,c,d,e,f,g,h,j,k,l,m,n) child_write_output_(__LINE__,a,b,c,d,e,f,g,h,j,k,l,m,n)
static void child_write_output_(unsigned int line, CHAR_INFO *buf, unsigned int size_x, unsigned int size_y,
                                unsigned int coord_x, unsigned int coord_y, unsigned int left,
                                unsigned int top, unsigned int right, unsigned int bottom, unsigned int out_left,
                                unsigned int out_top, unsigned int out_right, unsigned int out_bottom)
{
    char req_buf[4096];
    struct pseudoconsole_req *req = (void *)req_buf;
    SMALL_RECT region;
    DWORD count;
    BOOL ret;

    req->type = REQ_WRITE_OUTPUT;
    req->u.write_output.size.X = size_x;
    req->u.write_output.size.Y = size_y;
    req->u.write_output.coord.X = coord_x;
    req->u.write_output.coord.Y = coord_y;
    req->u.write_output.region.Left   = left;
    req->u.write_output.region.Top    = top;
    req->u.write_output.region.Right  = right;
    req->u.write_output.region.Bottom = bottom;
    memcpy(req->u.write_output.buf, buf, size_x * size_y * sizeof(*buf));
    ret = WriteFile(child_pipe, req, FIELD_OFFSET(struct pseudoconsole_req, u.write_output.buf[size_x * size_y]), &count, NULL);
    ok_(__FILE__,line)(ret, "WriteFile failed: %lu\n", GetLastError());
    ret = ReadFile(child_pipe, &region, sizeof(region), &count, NULL);
    ok_(__FILE__,line)(ret, "WriteFile failed: %lu\n", GetLastError());
    ok_(__FILE__,line)(region.Left == out_left, "Left = %u\n", region.Left);
    ok_(__FILE__,line)(region.Top == out_top, "Top = %u\n", region.Top);
    ok_(__FILE__,line)(region.Right == out_right, "Right = %u\n", region.Right);
    ok_(__FILE__,line)(region.Bottom == out_bottom, "Bottom = %u\n", region.Bottom);
}

static void child_scroll(unsigned int src_left, unsigned int src_top, unsigned int src_right,
                         unsigned int src_bottom, unsigned int dst_x, unsigned int dst_y, WCHAR fill)
{
    struct pseudoconsole_req req;
    DWORD count;
    BOOL ret;

    req.type = REQ_SCROLL;
    req.u.scroll.rect.Left   = src_left;
    req.u.scroll.rect.Top    = src_top;
    req.u.scroll.rect.Right  = src_right;
    req.u.scroll.rect.Bottom = src_bottom;
    req.u.scroll.dst.X = dst_x;
    req.u.scroll.dst.Y = dst_y;
    req.u.scroll.fill.Char.UnicodeChar = fill;
    req.u.scroll.fill.Attributes = 0;
    ret = WriteFile(child_pipe, &req, sizeof(req), &count, NULL);
    ok(ret, "WriteFile failed: %lu\n", GetLastError());
}

static void child_fill_character(WCHAR ch, DWORD count, int x, int y)
{
    struct pseudoconsole_req req;
    BOOL ret;

    req.type = REQ_FILL_CHAR;
    req.u.fill.ch = ch;
    req.u.fill.count = count;
    req.u.fill.coord.X = x;
    req.u.fill.coord.Y = y;
    ret = WriteFile(child_pipe, &req, sizeof(req), &count, NULL);
    ok(ret, "WriteFile failed: %lu\n", GetLastError());
}

static void child_set_input_mode(HANDLE pipe, DWORD mode)
{
    struct pseudoconsole_req req;
    DWORD count;
    BOOL ret;

    req.type = REQ_SET_INPUT_MODE;
    req.u.mode = mode;
    ret = WriteFile(pipe, &req, sizeof(req), &count, NULL);
    ok(ret, "WriteFile failed: %lu\n", GetLastError());
}

static void child_set_output_mode(DWORD mode)
{
    struct pseudoconsole_req req;
    DWORD count;
    BOOL ret;

    req.type = REQ_SET_OUTPUT_MODE;
    req.u.mode = mode;
    ret = WriteFile(child_pipe, &req, sizeof(req), &count, NULL);
    ok(ret, "WriteFile failed: %lu\n", GetLastError());
}

static void child_set_input_cp(int cp)
{
    struct pseudoconsole_req req;
    DWORD count;
    BOOL ret;

    req.type = REQ_SET_INPUT_CP;
    req.u.cp = cp;
    ret = WriteFile(child_pipe, &req, sizeof(req), &count, NULL);
    ok(ret, "WriteFile failed: %lu\n", GetLastError());
}

static void child_read_console(HANDLE pipe, size_t size)
{
    struct pseudoconsole_req req;
    DWORD count;
    BOOL ret;

    req.type = REQ_READ_CONSOLE;
    req.u.size = size;
    ret = WriteFile(pipe, &req, sizeof(req), &count, NULL);
    ok(ret, "WriteFile failed: %lu\n", GetLastError());
}

static void child_read_console_a(HANDLE pipe, size_t size)
{
    struct pseudoconsole_req req;
    DWORD count;
    BOOL ret;

    req.type = REQ_READ_CONSOLE_A;
    req.u.size = size;
    ret = WriteFile(pipe, &req, sizeof(req), &count, NULL);
    ok(ret, "WriteFile failed: %lu\n", GetLastError());
}

static void child_read_console_file(HANDLE pipe, size_t size)
{
    struct pseudoconsole_req req;
    DWORD count;
    BOOL ret;

    req.type = REQ_READ_CONSOLE_FILE;
    req.u.size = size;
    ret = WriteFile(pipe, &req, sizeof(req), &count, NULL);
    ok(ret, "WriteFile failed: %lu\n", GetLastError());
}

static void child_read_console_control(HANDLE pipe, size_t size, DWORD control, const WCHAR* recall)
{
    char tmp[4096];
    struct pseudoconsole_req *req = (void *)tmp;
    DWORD count;
    BOOL ret;

    req->type = REQ_READ_CONSOLE_CONTROL;
    req->u.control.size = size;
    req->u.control.mask = control;
    wcscpy(req->u.control.initial, recall);
    ret = WriteFile(pipe, req, sizeof(*req) + wcslen(recall) * sizeof(WCHAR), &count, NULL);
    ok(ret, "WriteFile failed: %lu\n", GetLastError());
}

#define child_expect_read_result(a,b) child_expect_read_result_(__LINE__,a,b)
static void child_expect_read_result_(unsigned int line, HANDLE pipe, const WCHAR *expect)
{
    size_t exlen = wcslen(expect);
    WCHAR buf[4096];
    DWORD count;
    BOOL ret;

    ret = ReadFile(pipe, buf, sizeof(buf), &count, NULL);
    ok_(__FILE__,line)(ret, "ReadFile failed: %lu\n", GetLastError());
    ok_(__FILE__,line)(count == exlen * sizeof(WCHAR), "got %lu, expected %Iu\n",
                       count, exlen * sizeof(WCHAR));
    buf[count / sizeof(WCHAR)] = 0;
    ok_(__FILE__,line)(!memcmp(expect, buf, count), "unexpected data %s\n", wine_dbgstr_w(buf));
}

#define child_expect_read_control_result(a,b,c) child_expect_read_control_result_(__LINE__,a,b,c)
static void child_expect_read_control_result_(unsigned int line, HANDLE pipe, const WCHAR *expect, DWORD state)
{
    size_t exlen = wcslen(expect);
    WCHAR buf[4096];
    WCHAR *ptr = (void *)((char *)buf + sizeof(DWORD));
    DWORD count;
    BOOL ret;

    ret = ReadFile(pipe, buf, sizeof(buf), &count, NULL);
    ok_(__FILE__,line)(ret, "ReadFile failed: %lu\n", GetLastError());
    ok_(__FILE__,line)(count == sizeof(DWORD) + exlen * sizeof(WCHAR), "got %lu, expected %Iu\n",
                       count, sizeof(DWORD) + exlen * sizeof(WCHAR));
    buf[count / sizeof(WCHAR)] = 0;
    todo_wine_if(*(DWORD *)buf != state && *(DWORD *)buf == 0)
    ok_(__FILE__,line)(*(DWORD *)buf == state, "keyboard state: got %lx, expected %lx\n", *(DWORD *)buf, state);
    ok_(__FILE__,line)(!memcmp(expect, ptr, count - sizeof(DWORD)), "unexpected data %s %s\n", wine_dbgstr_w(ptr), wine_dbgstr_w(expect));
}

#define child_expect_read_result_a(a,b) child_expect_read_result_a_(__LINE__,a,b)
static void child_expect_read_result_a_(unsigned int line, HANDLE pipe, const char *expect)
{
    size_t exlen = strlen(expect);
    char buf[4096];
    DWORD count;
    BOOL ret;

    ret = ReadFile(pipe, buf, sizeof(buf), &count, NULL);
    ok_(__FILE__,line)(ret, "ReadFile failed: %lu\n", GetLastError());
    todo_wine_if(exlen && expect[exlen - 1] == '\xcc')
    ok_(__FILE__,line)(count == exlen, "got %lu, expected %Iu\n", count, exlen);
    buf[count] = 0;
    ok_(__FILE__,line)(!memcmp(expect, buf, count), "unexpected data %s\n", wine_dbgstr_a(buf));
}

static void expect_input(unsigned int event_type, INPUT_RECORD *record)
{
    struct pseudoconsole_req req = { REQ_GET_INPUT };
    INPUT_RECORD input;
    DWORD read;
    BOOL ret;

    ret = WriteFile(child_pipe, &req, sizeof(req), &read, NULL);
    ok(ret, "WriteFile failed: %lu\n", GetLastError());

    ret = ReadFile(child_pipe, &input, sizeof(input), &read, NULL);
    ok(ret, "ReadFile failed: %lu\n", GetLastError());

    ok(input.EventType == event_type, "EventType = %u, expected %u\n", input.EventType, event_type);
    if (record) *record = input;
}

static BOOL get_key_input(unsigned int vt, INPUT_RECORD *record)
{
    static INPUT_RECORD prev_record;
    static BOOL have_prev_record;

    if (!have_prev_record)
    {
        expect_input(KEY_EVENT, &prev_record);
        have_prev_record = TRUE;
    }

    if (vt && prev_record.Event.KeyEvent.wVirtualKeyCode != vt) return FALSE;
    *record = prev_record;
    have_prev_record = FALSE;
    return TRUE;
}

#define expect_key_input(a,b,c,d) expect_key_input_(__LINE__,0,a,b,c,d)
static void expect_key_input_(unsigned int line, unsigned int ctx, WCHAR ch, unsigned int vk,
                              BOOL down, unsigned int ctrl_state)
{
    unsigned int vs = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    INPUT_RECORD record;

    get_key_input(0, &record);
    ok_(__FILE__,line)(record.Event.KeyEvent.bKeyDown == down, "%x: bKeyDown = %x\n",
                       ctx, record.Event.KeyEvent.bKeyDown);
    ok_(__FILE__,line)(record.Event.KeyEvent.wRepeatCount == 1, "%x: wRepeatCount = %x\n",
                       ctx, record.Event.KeyEvent.wRepeatCount);
    ok_(__FILE__,line)(record.Event.KeyEvent.uChar.UnicodeChar == ch, "%x: UnicodeChar = %x\n",
                       ctx, record.Event.KeyEvent.uChar.UnicodeChar);
    ok_(__FILE__,line)(record.Event.KeyEvent.wVirtualKeyCode == vk,
                       "%x: wVirtualKeyCode = %x, expected %x\n", ctx,
                       record.Event.KeyEvent.wVirtualKeyCode, vk);
    ok_(__FILE__,line)(record.Event.KeyEvent.wVirtualScanCode == vs,
                       "%x: wVirtualScanCode = %x expected %x\n", ctx,
                       record.Event.KeyEvent.wVirtualScanCode, vs);
    ok_(__FILE__,line)(record.Event.KeyEvent.dwControlKeyState == ctrl_state,
                       "%x: dwControlKeyState = %lx expected %x\n", ctx, record.Event.KeyEvent.dwControlKeyState, ctrl_state);
}

#define get_input_key_vt() get_input_key_vt_(__LINE__)
static unsigned int get_input_key_vt_(unsigned int line)
{
    INPUT_RECORD record;

    get_key_input(0, &record);
    ok_(__FILE__,line)(record.Event.KeyEvent.wRepeatCount == 1, "wRepeatCount = %x\n",
                       record.Event.KeyEvent.wRepeatCount);
    return record.Event.KeyEvent.wVirtualKeyCode;
}

#define expect_key_pressed(a,b,c) expect_key_pressed_(__LINE__,0,a,b,c)
#define expect_key_pressed_ctx(a,b,c,d) expect_key_pressed_(__LINE__,a,b,c,d)
static void expect_key_pressed_(unsigned int line, unsigned int ctx, WCHAR ch, unsigned int vk,
                                unsigned int ctrl_state)
{
    if (ctrl_state & SHIFT_PRESSED)
        expect_key_input_(line, ctx, 0, VK_SHIFT, TRUE, SHIFT_PRESSED);
    if (ctrl_state & LEFT_ALT_PRESSED)
        expect_key_input_(line, ctx, 0, VK_MENU, TRUE,
                          LEFT_ALT_PRESSED | (ctrl_state & SHIFT_PRESSED));
    if (ctrl_state & RIGHT_ALT_PRESSED)
        expect_key_input_(line, ctx, 0, VK_MENU, TRUE,
                          RIGHT_ALT_PRESSED | LEFT_CTRL_PRESSED | ENHANCED_KEY | (ctrl_state & SHIFT_PRESSED));
    else if (ctrl_state & LEFT_CTRL_PRESSED)
        expect_key_input_(line, ctx, 0, VK_CONTROL, TRUE,
                          LEFT_CTRL_PRESSED | (ctrl_state & (SHIFT_PRESSED | LEFT_ALT_PRESSED)));
    expect_key_input_(line, ctx, ch, vk, TRUE, ctrl_state);
    expect_key_input_(line, ctx, ch, vk, FALSE, ctrl_state);
    if (ctrl_state & RIGHT_ALT_PRESSED)
        expect_key_input_(line, ctx, 0, VK_MENU, FALSE, ENHANCED_KEY | (ctrl_state & SHIFT_PRESSED));
    else if (ctrl_state & LEFT_CTRL_PRESSED)
        expect_key_input_(line, ctx, 0, VK_CONTROL, FALSE,
                          ctrl_state & (SHIFT_PRESSED | LEFT_ALT_PRESSED));
    if (ctrl_state & LEFT_ALT_PRESSED)
        expect_key_input_(line, ctx, 0, VK_MENU, FALSE, ctrl_state & SHIFT_PRESSED);
    if (ctrl_state & SHIFT_PRESSED)
        expect_key_input_(line, ctx, 0, VK_SHIFT, FALSE, 0);
}

#define expect_char_key(a) expect_char_key_(__LINE__,a,0)
#define expect_char_key_ctrl(a,c) expect_char_key_(__LINE__,a,c)
static void expect_char_key_(unsigned int line, WCHAR ch, unsigned int ctrl)
{
    unsigned int vk;

    vk = VkKeyScanW(ch);
    if (vk == ~0) vk = 0;
    if (vk & 0x0100) ctrl |= SHIFT_PRESSED;
    /* Some keyboard (like German or French one) report right alt as ctrl+alt */
    if ((vk & 0x0600) == 0x0600 && !(ctrl & LEFT_ALT_PRESSED)) ctrl |= RIGHT_ALT_PRESSED;
    if (vk & 0x0200) ctrl |= LEFT_CTRL_PRESSED;
    vk &= 0xff;
    expect_key_pressed_(line, ch, ch, vk, ctrl);
}

static void fetch_child_sb_info(CONSOLE_SCREEN_BUFFER_INFO *info)
{
    struct pseudoconsole_req req = { REQ_GET_SB_INFO };
    DWORD read;
    BOOL ret;

    ret = WriteFile(child_pipe, &req, sizeof(req), &read, NULL);
    ok(ret, "WriteFile failed: %lu\n", GetLastError());

    ret = ReadFile(child_pipe, info, sizeof(*info), &read, NULL);
    ok(ret, "ReadFile failed: %lu\n", GetLastError());
}

#define test_cursor_pos(a,b) _test_cursor_pos(__LINE__,a,b)
static void _test_cursor_pos(unsigned line, int expect_x, int expect_y)
{
    CONSOLE_SCREEN_BUFFER_INFO info;

    fetch_child_sb_info(&info);

    ok_(__FILE__,line)(info.dwCursorPosition.X == expect_x, "dwCursorPosition.X = %u, expected %u\n",
                       info.dwCursorPosition.X, expect_x);
    ok_(__FILE__,line)(info.dwCursorPosition.Y == expect_y, "dwCursorPosition.Y = %u, expected %u\n",
                       info.dwCursorPosition.Y, expect_y);
}

static BOOL test_write_console(void)
{
    child_set_output_mode(ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    child_string_request(REQ_WRITE_CONSOLE, L"abc");
    skip_hide_cursor();
    if (skip_sequence("\x1b[H       \x1b[32m0\x1b[41m1\x1b[30m2"))
    {
        /* we get this on w1064v1909 (and more discrepancies afterwards).
         * This is inconsistent with ulterior w10 versions. Assume it's
         * immature result, and skip rest of tests.
         */
        console_output_count = 0;
        return FALSE;
    }

    expect_output_sequence("abc");
    skip_sequence("\x1b[?25h");            /* show cursor */

    child_string_request(REQ_WRITE_CONSOLE, L"\tt");
    skip_hide_cursor();
    if (!skip_sequence("\x1b[3C")) expect_output_sequence("   ");
    expect_output_sequence("t");
    skip_sequence("\x1b[?25h");            /* show cursor */
    expect_empty_output();

    child_string_request(REQ_WRITE_CONSOLE, L"x\rr");
    expect_hide_cursor();
    expect_output_sequence("\rr abc   tx");
    if (!skip_sequence("\x1b[9D"))
        expect_output_sequence("\x1b[4;2H"); /* set cursor */
    expect_output_sequence("\x1b[?25h");     /* show cursor */
    expect_empty_output();

    child_string_request(REQ_WRITE_CONSOLE, L"yz\r\n");
    skip_hide_cursor();
    expect_output_sequence("yz\r\n");
    skip_sequence("\x1b[?25h");              /* show cursor */
    expect_empty_output();

    child_string_request(REQ_WRITE_CONSOLE, L"abc\r\n123\r\ncde\r");
    skip_hide_cursor();
    expect_output_sequence("abc\r\n123\r\ncde\r");
    skip_sequence("\x1b[?25h");              /* show cursor */
    expect_empty_output();

    child_set_cursor(0, 39);
    expect_hide_cursor();
    expect_output_sequence("\x1b[40;1H");    /* set cursor */
    expect_output_sequence("\x1b[?25h");     /* show cursor */
    expect_empty_output();

    child_string_request(REQ_WRITE_CONSOLE, L"yz\r\n");
    skip_hide_cursor();
    expect_output_sequence("yz\r");
    if (skip_sequence("\x1b[?25h"))          /* show cursor */
        expect_output_sequence("\x1b[?25l"); /* hide cursor */
    expect_output_sequence("\n");            /* next line */
    if (skip_sequence("\x1b[30X"))           /* erase the line */
    {
        expect_output_sequence("\x1b[30C");  /* move cursor to the end of the line */
        expect_output_sequence("\r");
    }
    skip_sequence("\x1b[?25h");              /* show cursor */
    expect_empty_output();

    child_string_request(REQ_WRITE_CONSOLE, L"");
    expect_empty_output();

    child_string_request(REQ_WRITE_CONSOLE, L"ab\n");
    skip_hide_cursor();
    expect_output_sequence("ab");
    if (skip_sequence("\x1b[?25h"))          /* show cursor */
        expect_output_sequence("\x1b[?25l"); /* hide cursor */
    expect_output_sequence("\r\n");          /* next line */
    if (skip_sequence("\x1b[30X"))           /* erase the line */
    {
        expect_output_sequence("\x1b[30C");  /* move cursor to the end of the line */
        expect_output_sequence("\r");
    }
    skip_sequence("\x1b[?25h");              /* show cursor */
    expect_empty_output();

    child_set_cursor(28, 10);
    expect_hide_cursor();
    expect_output_sequence("\x1b[11;29H");   /* set cursor */
    expect_output_sequence("\x1b[?25h");     /* show cursor */
    expect_empty_output();

    child_string_request(REQ_WRITE_CONSOLE, L"xy");
    skip_hide_cursor();
    expect_output_sequence("xy");
    if (!skip_sequence("\b")) skip_sequence("\r\n");
    skip_sequence("\x1b[?25h");              /* show cursor */
    expect_empty_output();

    child_set_cursor(28, 10);
    fetch_console_output();
    if (!skip_sequence("\b"))
    {
        expect_hide_cursor();
        expect_output_sequence("\x1b[11;29H"); /* set cursor */
        expect_output_sequence("\x1b[?25h");   /* show cursor */
    }
    expect_empty_output();

    child_string_request(REQ_WRITE_CONSOLE, L"abc");
    skip_hide_cursor();
    expect_output_sequence("\r                            ab");
    expect_output_sequence("\r\nc");
    if (expect_erase_line(29))
        expect_output_sequence("\x1b[12;2H"); /* set cursor */
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();

    child_set_cursor(28, 39);
    expect_hide_cursor();
    expect_output_sequence("\x1b[40;29H");    /* set cursor */
    expect_output_sequence("\x1b[?25h");      /* show cursor */
    expect_empty_output();

    child_string_request(REQ_WRITE_CONSOLE, L"abc");
    skip_hide_cursor();
    expect_output_sequence("ab");
    skip_sequence("\x1b[40;29H");             /* set cursor */
    if (skip_sequence("\x1b[?25h"))           /* show cursor */
        expect_output_sequence("\x1b[?25l");  /* hide cursor */
    else
        skip_sequence("\b");
    expect_output_sequence("\r\nc");
    if (skip_sequence("\x1b[29X"))            /* erase the line */
    {
        expect_output_sequence("\x1b[29C");   /* move cursor to the end of the line */
        expect_output_sequence("\x1b[40;2H"); /* set cursor */
    }
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();

    child_set_cursor(28, 39);
    skip_hide_cursor();
    if (!skip_sequence("\x1b[27C"))
        expect_output_sequence("\x1b[40;29H"); /* set cursor */
    skip_sequence("\x1b[?25h");                /* show cursor */
    expect_empty_output();

    child_string_request(REQ_WRITE_CONSOLE, L"XY");
    skip_hide_cursor();
    expect_output_sequence("XY");
    skip_sequence("\x1b[40;29H");             /* set cursor */
    if (skip_sequence("\x1b[?25h"))           /* show cursor */
        skip_sequence("\x1b[?25l");           /* hide cursor */
    if (!skip_sequence("\b") && skip_sequence("\r\n"))
    {
        expect_output_sequence("\x1b[30X");   /* erase the line */
        expect_output_sequence("\x1b[30C");   /* move cursor to the end of the line */
        expect_output_sequence("\r");         /* set cursor */
    }
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();

    child_string_request(REQ_WRITE_CONSOLE, L"\n");
    skip_hide_cursor();
    if (!skip_sequence("\r\n"))
    {
        expect_output_sequence("\n");
        expect_output_sequence("\x1b[30X");   /* erase the line */
        expect_output_sequence("\x1b[30C");   /* move cursor to the end of the line */
        expect_output_sequence("\r");         /* set cursor */
    }
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();

    child_set_output_mode(ENABLE_PROCESSED_OUTPUT);

    child_set_cursor(28, 11);
    expect_hide_cursor();
    expect_output_sequence("\x1b[12;29H");    /* set cursor */
    skip_sequence("\x1b[?25h");               /* show cursor */

    child_string_request(REQ_WRITE_CONSOLE, L"xyz1234");
    skip_hide_cursor();
    expect_output_sequence("43\b");
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();

    child_set_cursor(28, 11);
    skip_hide_cursor();
    expect_output_sequence("\b");             /* backspace */
    skip_sequence("\x1b[?25h");               /* show cursor */

    child_string_request(REQ_WRITE_CONSOLE, L"xyz123");
    expect_hide_cursor();
    expect_output_sequence("23");
    if (!skip_sequence("\x1b[2D"))
        expect_output_sequence("\x1b[12;29H");/* set cursor */
    expect_output_sequence("\x1b[?25h");      /* show cursor */
    expect_empty_output();

    child_set_cursor(28, 11);
    child_string_request(REQ_WRITE_CONSOLE, L"abcdef\n\r123456789012345678901234567890xyz");
    expect_hide_cursor();
    if (skip_sequence("\x1b[?25h")) expect_hide_cursor();
    expect_output_sequence("\r                            ef\r\n");
    expect_output_sequence("xyz456789012345678901234567890");
    if (!skip_sequence("\x1b[27D"))
        expect_output_sequence("\x1b[13;4H"); /* set cursor */
    expect_output_sequence("\x1b[?25h");      /* show cursor */
    expect_empty_output();

    child_set_cursor(28, 11);
    expect_hide_cursor();
    expect_output_sequence("\x1b[12;29H");    /* set cursor */
    expect_output_sequence("\x1b[?25h");      /* show cursor */

    child_string_request(REQ_WRITE_CONSOLE, L"AB\r\n");
    skip_hide_cursor();
    expect_output_sequence("AB\r\n");
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();

    child_set_output_mode(ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    child_set_cursor(28, 12);
    skip_hide_cursor();
    expect_output_sequence("\x1b[28C");       /* move cursor to the end of the line */
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();

    child_string_request(REQ_WRITE_CONSOLE, L"ab");
    skip_hide_cursor();
    expect_output_sequence("ab");
    skip_sequence("\b");
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();
    test_cursor_pos(29, 12);

    child_string_request(REQ_WRITE_CONSOLE, L"c");
    skip_hide_cursor();
    expect_output_sequence("\r\n");
    expect_output_sequence("c");
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();
    test_cursor_pos(1, 13);

    child_set_cursor(28, 14);
    skip_hide_cursor();
    expect_output_sequence("\x1b[15;29H");    /* set cursor */
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();

    child_string_request(REQ_WRITE_CONSOLE, L"x");
    skip_hide_cursor();
    expect_output_sequence("x");
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();
    test_cursor_pos(29, 14);

    child_string_request(REQ_WRITE_CONSOLE, L"y");
    skip_hide_cursor();
    expect_output_sequence("y");
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();
    test_cursor_pos(29, 14);

    child_string_request(REQ_WRITE_CONSOLE, L"\b");
    skip_hide_cursor();
    expect_output_sequence("\b");
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();
    test_cursor_pos(28, 14);

    child_string_request(REQ_WRITE_CONSOLE, L"z");
    skip_hide_cursor();
    expect_output_sequence("z");
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();
    test_cursor_pos(29, 14);

    child_string_request(REQ_WRITE_CONSOLE, L"w");
    skip_hide_cursor();
    expect_output_sequence("w");
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();
    test_cursor_pos(29, 14);

    child_string_request(REQ_WRITE_CONSOLE, L"X");
    skip_hide_cursor();
    expect_output_sequence("\r\n");
    expect_output_sequence("X");
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();
    test_cursor_pos(1, 15);

    child_set_output_mode(ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);

    child_set_cursor(28, 20);
    skip_hide_cursor();
    expect_output_sequence("\x1b[21;29H");    /* set cursor */
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();

    child_string_request(REQ_WRITE_CONSOLE, L"ab");
    skip_hide_cursor();
    expect_output_sequence("ab");
    expect_output_sequence("\r\n");
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();
    test_cursor_pos(0, 21);

    child_string_request(REQ_WRITE_CONSOLE, L"c");
    skip_hide_cursor();
    expect_output_sequence("c");
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();
    test_cursor_pos(1, 21);

    child_set_cursor(28, 22);
    skip_hide_cursor();
    expect_output_sequence("\x1b[23;29H");    /* set cursor */
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();

    child_string_request(REQ_WRITE_CONSOLE, L"x");
    skip_hide_cursor();
    expect_output_sequence("x");
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();
    test_cursor_pos(29, 22);

    child_string_request(REQ_WRITE_CONSOLE, L"y");
    skip_hide_cursor();
    expect_output_sequence("y");
    expect_output_sequence("\r\n");
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();
    test_cursor_pos(0, 23);

    child_string_request(REQ_WRITE_CONSOLE, L"z");
    skip_hide_cursor();
    expect_output_sequence("z");
    skip_sequence("\x1b[?25h");               /* show cursor */
    expect_empty_output();
    test_cursor_pos(1, 23);

    return TRUE;
}

static BOOL test_tty_output(void)
{
    CHAR_INFO char_info_buf[2048], char_info;
    HANDLE sb, sb2;
    unsigned int i;

    /* simple write chars */
    child_write_characters(L"child", 3, 4);
    expect_hide_cursor();
    expect_output_sequence("\x1b[5;4H");   /* set cursor */
    expect_output_sequence("child");
    expect_output_sequence("\x1b[H");      /* set cursor */
    expect_output_sequence("\x1b[?25h");   /* show cursor */
    expect_empty_output();

    /* wrapped write chars */
    child_write_characters(L"bound", 28, 6);
    expect_hide_cursor();
    expect_output_sequence("\x1b[7;1H");   /* set cursor */
    expect_output_sequence("                            bo\r\nund");
    expect_erase_line(27);
    expect_output_sequence("\x1b[H");      /* set cursor */
    expect_output_sequence("\x1b[?25h");   /* show cursor */
    expect_empty_output();

    /* fill line 4 with a few simple writes */
    child_write_characters(L"xxx", 13, 4);
    expect_hide_cursor();
    expect_output_sequence("\x1b[5;14H");  /* set cursor */
    expect_output_sequence("xxx");
    expect_output_sequence("\x1b[H");      /* set cursor */
    expect_output_sequence("\x1b[?25h");   /* show cursor */
    expect_empty_output();

    /* write one char at the end of row */
    child_write_characters(L"y", 29, 4);
    expect_hide_cursor();
    expect_output_sequence("\x1b[5;30H");  /* set cursor */
    expect_output_sequence("y");
    expect_output_sequence("\x1b[H");      /* set cursor */
    expect_output_sequence("\x1b[?25h");   /* show cursor */
    expect_empty_output();

    /* wrapped write chars */
    child_write_characters(L"zz", 29, 4);
    expect_hide_cursor();
    expect_output_sequence("\x1b[5;1H");   /* set cursor */
    expect_output_sequence("   child     xxx             z");
    expect_output_sequence("\r\nz");
    expect_erase_line(29);
    expect_output_sequence("\x1b[H");      /* set cursor */
    expect_output_sequence("\x1b[?25h");   /* show cursor */
    expect_empty_output();

    /* trailing spaces */
    child_write_characters(L"child        ", 3, 4);
    expect_hide_cursor();
    expect_output_sequence("\x1b[5;4H");   /* set cursor */
    expect_output_sequence("child        ");
    expect_output_sequence("\x1b[H");      /* set cursor */
    expect_output_sequence("\x1b[?25h");   /* show cursor */
    expect_empty_output();

    child_set_cursor(2, 3);
    expect_hide_cursor();
    expect_output_sequence("\x1b[4;3H");   /* set cursor */
    expect_output_sequence("\x1b[?25h");   /* show cursor */
    expect_empty_output();

    child_string_request(REQ_SET_TITLE, L"new title");
    fetch_console_output();
    skip_sequence("\x1b[?25l");            /* hide cursor */
    expect_output_sequence("\x1b]0;new title\x07"); /* set title */
    skip_sequence("\x1b[?25h");            /* show cursor */
    expect_empty_output();

    for (i = 0; i < ARRAY_SIZE(char_info_buf); i++)
    {
        char_info_buf[i].Char.UnicodeChar = '0' + i % 10;
        char_info_buf[i].Attributes = 0;
    }

    child_write_output(char_info_buf, /* size */ 7, 8, /* coord */ 1, 2,
                       /* region */ 3, 7, 5, 9, /* out region */ 3, 7, 5, 9);
    expect_hide_cursor();
    expect_output_sequence("\x1b[30m");    /* foreground black */
    expect_output_sequence("\x1b[8;4H");   /* set cursor */
    expect_output_sequence("567");
    expect_output_sequence("\x1b[9;4H");   /* set cursor */
    expect_output_sequence("234");
    expect_output_sequence("\x1b[10;4H");  /* set cursor */
    expect_output_sequence("901");
    expect_output_sequence("\x1b[4;3H");   /* set cursor */
    expect_output_sequence("\x1b[?25h");   /* show cursor */
    expect_empty_output();

    child_write_output(char_info_buf, /* size */ 2, 3, /* coord */ 1, 2,
                       /* region */ 3, 8, 15, 19, /* out region */ 3, 8, 3, 8);
    expect_hide_cursor();
    if (skip_sequence("\x1b[m"))           /* default attr */
        expect_output_sequence("\x1b[30m");/* foreground black */
    expect_output_sequence("\x1b[9;4H");   /* set cursor */
    expect_output_sequence("5");
    expect_output_sequence("\x1b[4;3H");   /* set cursor */
    expect_output_sequence("\x1b[?25h");   /* show cursor */
    expect_empty_output();

    child_write_output(char_info_buf, /* size */ 3, 4, /* coord */ 1, 2,
                       /* region */ 3, 8, 15, 19, /* out region */ 3, 8, 4, 9);
    expect_hide_cursor();
    if (skip_sequence("\x1b[m"))           /* default attr */
        expect_output_sequence("\x1b[30m");/* foreground black */
    expect_output_sequence("\x1b[9;4H");   /* set cursor */
    expect_output_sequence("78");
    expect_output_sequence("\x1b[10;4H");  /* set cursor */
    expect_output_sequence("01");
    expect_output_sequence("\x1b[4;3H");   /* set cursor */
    expect_output_sequence("\x1b[?25h");   /* show cursor */
    expect_empty_output();

    child_write_output(char_info_buf, /* size */ 7, 8, /* coord */ 2, 3,
                       /* region */ 28, 38, 31, 60, /* out region */ 28, 38, 29, 39);
    expect_hide_cursor();
    if (skip_sequence("\x1b[m"))           /* default attr */
        expect_output_sequence("\x1b[30m");/* foreground black */
    expect_output_sequence("\x1b[39;29H"); /* set cursor */
    expect_output_sequence("34");
    expect_output_sequence("\x1b[40;29H"); /* set cursor */
    expect_output_sequence("01");
    expect_output_sequence("\x1b[4;3H");   /* set cursor */
    expect_output_sequence("\x1b[?25h");   /* show cursor */
    expect_empty_output();

    child_write_output(char_info_buf, /* size */ 7, 8, /* coord */ 1, 2,
                       /* region */ 0, 7, 5, 9, /* out region */ 0, 7, 5, 9);
    expect_hide_cursor();
    if (skip_sequence("\x1b[m"))           /* default attr */
        expect_output_sequence("\x1b[30m");/* foreground black */
    expect_output_sequence("\x1b[8;1H");   /* set cursor */
    expect_output_sequence("567890\r\n");
    expect_output_sequence("234567\r\n");
    expect_output_sequence("901234");
    expect_output_sequence("\x1b[4;3H");   /* set cursor */
    expect_output_sequence("\x1b[?25h");   /* show cursor */
    expect_empty_output();

    child_scroll(/* scroll rect */ 0, 7, 2, 8, /* destination */ 2, 8, /* fill */ 'x');
    expect_hide_cursor();
    if (skip_sequence("\x1b[m"))           /* default attr */
        expect_output_sequence("\x1b[30m");/* foreground black */
    expect_output_sequence("\x1b[8;1H");   /* set cursor */
    expect_output_sequence("xxx89\r\n");
    expect_output_sequence("xx567\r\n");
    expect_output_sequence("90234");
    expect_output_sequence("\x1b[4;3H");   /* set cursor */
    expect_output_sequence("\x1b[?25h");   /* show cursor */
    expect_empty_output();

    child_write_characters(L"xxx", 3, 10);
    expect_hide_cursor();
    expect_output_sequence("\x1b[m");      /* default attributes */
    expect_output_sequence("\x1b[11;4H");  /* set cursor */
    expect_output_sequence("xxx");
    expect_output_sequence("\x1b[4;3H");   /* set cursor */
    expect_output_sequence("\x1b[?25h");   /* show cursor */
    expect_empty_output();

    /* test attributes */
    for (i = 0; i < 0x100 - 0xff; i++)
    {
        unsigned int expect;
        char expect_buf[16];
        char_info.Char.UnicodeChar = 'a';
        char_info.Attributes = i;
        child_write_output(&char_info, /* size */ 1, 1, /* coord */ 0, 0,
                           /* region */ 12, 3, 12, 3, /* out region */ 12, 3, 12, 3);
        expect_hide_cursor();
        if (i != 0x190 && i && ((i & 0xff) != 8)) expect_output_sequence_ctx(i, "\x1b[m");
        if ((i & 0x0f) != 7)
        {
            expect = 30;
            if (i & FOREGROUND_BLUE)  expect += 4;
            if (i & FOREGROUND_GREEN) expect += 2;
            if (i & FOREGROUND_RED)   expect += 1;
            if (i & FOREGROUND_INTENSITY) expect += 60;
            sprintf(expect_buf, "\x1b[%um", expect);
            expect_output_sequence_ctx(i, expect_buf);
        }
        if (i & 0xf0)
        {
            expect = 40;
            if (i & BACKGROUND_BLUE)  expect += 4;
            if (i & BACKGROUND_GREEN) expect += 2;
            if (i & BACKGROUND_RED)   expect += 1;
            if (i & BACKGROUND_INTENSITY) expect += 60;
            sprintf(expect_buf, "\x1b[%um", expect);
            expect_output_sequence_ctx(i, expect_buf);
        }
        if (!skip_sequence("\x1b[10C"))
            expect_output_sequence_ctx(i, "\x1b[4;13H"); /* set cursor */
        expect_output_sequence("a");
        if (!skip_sequence("\x1b[11D"))
            expect_output_sequence("\x1b[4;3H"); /* set cursor */
        expect_output_sequence("\x1b[?25h");     /* show cursor */
        expect_empty_output();
    }

    char_info_buf[0].Attributes = FOREGROUND_GREEN;
    char_info_buf[1].Attributes = FOREGROUND_GREEN | BACKGROUND_RED;
    char_info_buf[2].Attributes = BACKGROUND_RED;
    child_write_output(char_info_buf, /* size */ 7, 8, /* coord */ 0, 0,
                       /* region */ 7, 0, 9, 0, /* out region */ 7, 0, 9, 0);
    expect_hide_cursor();
    skip_sequence("\x1b[m");               /* default attr */
    expect_output_sequence("\x1b[32m");    /* foreground black */
    expect_output_sequence("\x1b[1;8H");   /* set cursor */
    expect_output_sequence("0");
    expect_output_sequence("\x1b[41m");    /* background red */
    expect_output_sequence("1");
    expect_output_sequence("\x1b[30m");    /* foreground black */
    expect_output_sequence("2");
    expect_output_sequence("\x1b[4;3H");   /* set cursor */
    expect_output_sequence("\x1b[?25h");   /* show cursor */
    expect_empty_output();

    child_fill_character('i', 5, 15, 16);
    expect_hide_cursor();
    expect_output_sequence("\x1b[m");      /* default attributes */
    expect_output_sequence("\x1b[17;16H"); /* set cursor */
    expect_output_sequence("iiiii");
    expect_output_sequence("\x1b[4;3H");   /* set cursor */
    expect_output_sequence("\x1b[?25h");   /* show cursor */
    expect_empty_output();

    if (!test_write_console()) return FALSE;

    sb = child_create_screen_buffer();
    child_set_active(sb);
    expect_hide_cursor();
    expect_output_sequence("\x1b[H");      /* set cursor */
    for (i = 0; i < 40; i++)
    {
        expect_erase_line(30);
        if (i != 39) expect_output_sequence("\r\n");
    }
    expect_output_sequence("\x1b[H");      /* set cursor */
    expect_output_sequence("\x1b[?25h");   /* show cursor */
    expect_empty_output();

    child_write_characters(L"new sb", 0, 0);
    skip_hide_cursor();
    expect_output_sequence("new sb");
    ok(skip_sequence("\x1b[H") || skip_sequence("\r"), "expected set cursor\n");
    skip_sequence("\x1b[?25h");            /* show cursor */
    expect_empty_output();

    sb2 = child_create_screen_buffer();
    child_set_active(sb2);
    expect_hide_cursor();
    for (i = 0; i < 40; i++)
    {
        expect_erase_line(30);
        if (i != 39) expect_output_sequence("\r\n");
    }
    expect_output_sequence("\x1b[H");      /* set cursor */
    expect_output_sequence("\x1b[?25h");   /* show cursor */
    expect_empty_output();

    child_set_active(sb);
    expect_hide_cursor();
    expect_output_sequence("new sb");
    expect_erase_line(24);
    expect_output_sequence("\r\n");
    for (i = 1; i < 40; i++)
    {
        expect_erase_line(30);
        if (i != 39) expect_output_sequence("\r\n");
    }
    expect_output_sequence("\x1b[H");      /* set cursor */
    expect_output_sequence("\x1b[?25h");   /* show cursor */
    expect_empty_output();

    return TRUE;
}

static void write_console_pipe(const char *buf)
{
    DWORD written;
    BOOL res;
    res = WriteFile(console_pipe, buf, strlen(buf), &written, NULL);
    ok(res, "WriteFile failed: %lu\n", GetLastError());
}

static void test_read_console(void)
{
    child_set_input_mode(child_pipe, ENABLE_PROCESSED_INPUT);

    child_read_console(child_pipe, 100);
    write_console_pipe("abc");
    expect_empty_output();
    child_expect_read_result(child_pipe, L"abc");
    expect_empty_output();

    child_read_console(child_pipe, 1);
    write_console_pipe("xyz");
    child_expect_read_result(child_pipe, L"x");
    child_read_console(child_pipe, 100);
    child_expect_read_result(child_pipe, L"yz");
    expect_empty_output();

    child_set_input_cp(932);

    child_read_console_a(child_pipe, 2);
    write_console_pipe("\xe3\x81\x81");
    child_expect_read_result_a(child_pipe, "\x82\x9f");
    expect_empty_output();

    child_read_console_a(child_pipe, 1);
    write_console_pipe("\xe3\x81\x81""a");
    child_expect_read_result_a(child_pipe, "\x82\xcc");
    child_read_console_a(child_pipe, 1);
    child_expect_read_result_a(child_pipe, "a");
    expect_empty_output();

    child_read_console_a(child_pipe, 2);
    write_console_pipe("a\xe3\x81\x81""b");
    child_expect_read_result_a(child_pipe, "a\x82\xcc");
    child_read_console_a(child_pipe, 1);
    child_expect_read_result_a(child_pipe, "b");
    expect_empty_output();

    child_read_console_file(child_pipe, 2);
    write_console_pipe("\xe3\x81\x81");
    child_expect_read_result_a(child_pipe, "\x82\x9f");
    expect_empty_output();

    child_read_console_file(child_pipe, 1);
    write_console_pipe("\xe3\x81\x81""a");
    child_expect_read_result_a(child_pipe, "\x82\xcc");
    child_read_console_file(child_pipe, 1);
    child_expect_read_result_a(child_pipe, "a");
    expect_empty_output();

    child_read_console_file(child_pipe, 2);
    write_console_pipe("a\xe3\x81\x81""b");
    child_expect_read_result_a(child_pipe, "a\x82\xcc");
    child_read_console_file(child_pipe, 1);
    child_expect_read_result_a(child_pipe, "b");
    expect_empty_output();

    child_set_input_cp(437);

    child_set_input_mode(child_pipe, ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT |
                         ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT | ENABLE_INSERT_MODE |
                         ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS | ENABLE_AUTO_POSITION);

    child_read_console(child_pipe, 100);
    write_console_pipe("xyz");
    skip_hide_cursor();
    expect_output_sequence("xyz");
    skip_sequence("\x1b[?25h"); /* show cursor */
    write_console_pipe("ab\r\n");
    child_expect_read_result(child_pipe, L"xyzab\r\n");
    skip_hide_cursor();
    expect_output_sequence("ab\r\n");
    skip_sequence("\x1b[?25h"); /* show cursor */
    expect_key_input('\r', VK_RETURN, 0, FALSE);
    expect_key_pressed('\n', VK_RETURN, LEFT_CTRL_PRESSED);
    expect_empty_output();
}

static void test_read_console_control(void)
{
    CONSOLE_SCREEN_BUFFER_INFO info1, info2;
    char ctrl;
    char buf[16];
    WCHAR bufw[16];

    child_set_input_mode(child_pipe, ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT |
                         ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT | ENABLE_INSERT_MODE |
                         ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS | ENABLE_AUTO_POSITION);

    /* test simple behavior */
    for (ctrl = 0; ctrl < ' '; ctrl++)
    {
        DWORD state;
        SHORT cc;

        /* don't play with fire */
        if (ctrl == 0 || ctrl == 3 || ctrl == '\n' || ctrl == 27) continue;

        /* Simulate initial characters
         * Note: as ReadConsole with CONTROL structure will need to back up the cursor
         * up to the length of the initial characters, all the following tests ensure that
         * this backup doesn't imply backing up to the previous line.
         * This is the "regular" behavior when using completion.
         */
        child_string_request(REQ_WRITE_CONSOLE, L"\rabc");
        skip_sequence("\x1b[25l");  /* broken hide cursor */
        skip_sequence("\x1b[?25l"); /* hide cursor */
        skip_sequence("\x1b[H");
        skip_sequence("\r");
        expect_output_sequence("abc");
        skip_sequence("\x1b[?25h"); /* show cursor */

        fetch_child_sb_info(&info1);
        child_read_console_control(child_pipe, 100, 1ul << ctrl, L"abc");
        strcpy(buf, "def."); buf[3] = ctrl;
        write_console_pipe(buf);
        wcscpy(bufw, L"abcdef."); bufw[6] = (WCHAR)ctrl;
        /* ^H requires special handling */
        cc = (ctrl == 8) ? 0x0200 : VkKeyScanA(ctrl);
        if (cc == -1) cc = 0;
        state = 0;
        if (cc & 0x0100) state |= SHIFT_PRESSED;
        if (cc & 0x0200) state |= LEFT_CTRL_PRESSED;
        child_expect_read_control_result(child_pipe, bufw, state);
        skip_sequence("\x1b[?25l"); /* hide cursor */
        expect_output_sequence("def");
        skip_sequence("\x1b[?25h"); /* show cursor */
        fetch_child_sb_info(&info2);
        ok(info1.dwCursorPosition.X + 3 == info2.dwCursorPosition.X,
           "Bad x-position: expected %u => %u but got => %u instead\n",
           info1.dwCursorPosition.X, info1.dwCursorPosition.X + 3, info2.dwCursorPosition.X);
        ok(info1.dwCursorPosition.Y == info2.dwCursorPosition.Y, "Cursor shouldn't have changed line\n");
    }

    /* test two different control characters in input */
    fetch_child_sb_info(&info1);
    child_read_console_control(child_pipe, 100, 1ul << '\x01', L"abc");
    write_console_pipe("d\x02""ef\x01");
    child_expect_read_control_result(child_pipe, L"abcd\x02""ef\x01", LEFT_CTRL_PRESSED);
    skip_sequence("\x1b[?25l"); /* hide cursor */
    expect_output_sequence("d^Bef");
    skip_sequence("\x1b[?25h"); /* show cursor */
    fetch_child_sb_info(&info2);
    ok(info1.dwCursorPosition.X + 5 == info2.dwCursorPosition.X,
       "Bad x-position: expected %u => %u but got => %u instead\n",
       info1.dwCursorPosition.X, info1.dwCursorPosition.X + 5, info2.dwCursorPosition.X);
    ok(info1.dwCursorPosition.Y == info2.dwCursorPosition.Y, "Cursor shouldn't have changed line\n");

    /* test that ctrl character not in mask is handled as without ctrl mask */
    child_read_console_control(child_pipe, 100, 1ul << '\x01', L"abc");
    write_console_pipe("d\ref\x01");
    child_expect_read_control_result(child_pipe, L"abcd\r\n", 0);
    skip_sequence("\x1b[?25l"); /* hide cursor */
    expect_output_sequence("d\r\n");
    skip_sequence("\x1b[?25h"); /* show cursor */
    expect_empty_output();
    /* see note above... ditto */
    child_string_request(REQ_WRITE_CONSOLE, L"abc");
    skip_sequence("\x1b[?25l"); /* hide cursor */
    expect_output_sequence("abc");
    skip_sequence("\x1b[?25h"); /* show cursor */

    child_read_console_control(child_pipe, 100, 1ul << '\x01', L"abc");
    child_expect_read_control_result(child_pipe, L"abcef\x01", LEFT_CTRL_PRESSED);
    skip_sequence("\x1b[?25l"); /* hide cursor */
    expect_output_sequence("ef");
    skip_sequence("\x1b[?25h"); /* show cursor */

    /* test when output buffer becomes full before control event */
    child_read_console_control(child_pipe, 4, 1ul << '\x01', L"abc");
    write_console_pipe("def\x01");
    child_expect_read_control_result(child_pipe, L"abcd", LEFT_CTRL_PRESSED);
    skip_sequence("\x1b[?25l"); /* hide cursor */
    expect_output_sequence("def");
    skip_sequence("\x1b[?25h"); /* show cursor */
    expect_empty_output();
    child_read_console_control(child_pipe, 20, 1ul << '\x01', L"abc");
    child_expect_read_control_result(child_pipe, L"ef\x01", 0);

    /* TODO: add tests:
     * - when initial characters go back to previous line
     * - edition inside initial characters can occur
     */
    expect_empty_output();
}

static void test_tty_input(void)
{
    INPUT_RECORD ir;
    unsigned int i;
    char buf[8];

    static const struct
    {
        const char *str;
        unsigned int vk;
        unsigned int ctrl;
    } escape_test[] = {
        { "\x1b[A",          VK_UP,       0 },
        { "\x1b[B",          VK_DOWN,     0 },
        { "\x1b[C",          VK_RIGHT,    0 },
        { "\x1b[D",          VK_LEFT,     0 },
        { "\x1b[H",          VK_HOME,     0 },
        { "\x1b[F",          VK_END,      0 },
        { "\x1b[2~",         VK_INSERT,   0 },
        { "\x1b[3~",         VK_DELETE,   0 },
        { "\x1b[5~",         VK_PRIOR,    0 },
        { "\x1b[6~",         VK_NEXT,     0 },
        { "\x1b[15~",        VK_F5,       0 },
        { "\x1b[17~",        VK_F6,       0 },
        { "\x1b[18~",        VK_F7,       0 },
        { "\x1b[19~",        VK_F8,       0 },
        { "\x1b[20~",        VK_F9,       0 },
        { "\x1b[21~",        VK_F10,      0 },
        /* 0x10 */
        { "\x1b[23~",        VK_F11,      0 },
        { "\x1b[24~",        VK_F12,      0 },
        { "\x1bOP",          VK_F1,       0 },
        { "\x1bOQ",          VK_F2,       0 },
        { "\x1bOR",          VK_F3,       0 },
        { "\x1bOS",          VK_F4,       0 },
        { "\x1b[1;1A",       VK_UP,       0 },
        { "\x1b[1;2A",       VK_UP,       SHIFT_PRESSED },
        { "\x1b[1;3A",       VK_UP,       LEFT_ALT_PRESSED },
        { "\x1b[1;4A",       VK_UP,       SHIFT_PRESSED | LEFT_ALT_PRESSED },
        { "\x1b[1;5A",       VK_UP,       LEFT_CTRL_PRESSED },
        { "\x1b[1;6A",       VK_UP,       SHIFT_PRESSED | LEFT_CTRL_PRESSED },
        { "\x1b[1;7A",       VK_UP,       LEFT_ALT_PRESSED  | LEFT_CTRL_PRESSED },
        { "\x1b[1;8A",       VK_UP,       SHIFT_PRESSED | LEFT_ALT_PRESSED  | LEFT_CTRL_PRESSED },
        { "\x1b[1;9A",       VK_UP,       0 },
        { "\x1b[1;10A",      VK_UP,       SHIFT_PRESSED },
        /* 0x20 */
        { "\x1b[1;11A",      VK_UP,       LEFT_ALT_PRESSED },
        { "\x1b[1;12A",      VK_UP,       SHIFT_PRESSED | LEFT_ALT_PRESSED },
        { "\x1b[1;13A",      VK_UP,       LEFT_CTRL_PRESSED },
        { "\x1b[1;14A",      VK_UP,       SHIFT_PRESSED | LEFT_CTRL_PRESSED },
        { "\x1b[1;15A",      VK_UP,       LEFT_ALT_PRESSED  | LEFT_CTRL_PRESSED },
        { "\x1b[1;16A",      VK_UP,       SHIFT_PRESSED | LEFT_ALT_PRESSED  | LEFT_CTRL_PRESSED },
        { "\x1b[1;2P",       VK_F1,       SHIFT_PRESSED },
        { "\x1b[2;3~",       VK_INSERT,   LEFT_ALT_PRESSED },
        { "\x1b[2;3;5;6~",   VK_INSERT,   0 },
        { "\x1b[6;2;3;5;1~", VK_NEXT,     0 },
    };
    static const struct
    {
        const char *str;
        WCHAR ch;
        unsigned int ctrl;
    } escape_char_test[] = {
        { "\x1b[Z",          0x9,    SHIFT_PRESSED },
        { "\xe4\xb8\x80",    0x4e00, 0 },
        { "\x1b\x1b",        0x1b,   LEFT_ALT_PRESSED },
        { "\x1b""1",         '1',    LEFT_ALT_PRESSED },
        { "\x1b""x",         'x',    LEFT_ALT_PRESSED },
        { "\x1b""[",         '[',    LEFT_ALT_PRESSED },
        { "\x7f",            '\b',   0 },
    };

    write_console_pipe("x");
    if (!get_input_key_vt())
    {
        skip("Skipping tests on settings that don't have VT mapping for 'x'\n");
        get_input_key_vt();
        return;
    }
    get_input_key_vt();

    write_console_pipe("aBCd");
    expect_char_key('a');
    expect_char_key('B');
    expect_char_key('C');
    expect_char_key('d');

    for (i = 1; i < 0x7f; i++)
    {
        if (i == 3 || i == '\n' || i == 0x1b || i == 0x1f) continue;
        buf[0] = i;
        buf[1] = 0;
        write_console_pipe(buf);
        if (i == 8)
            expect_key_pressed('\b', 'H', LEFT_CTRL_PRESSED);
        else if (i == 0x7f)
            expect_char_key(8);
        else
            expect_char_key(i);
    }

    write_console_pipe("\r\n");
    expect_key_pressed('\r', VK_RETURN, 0);
    expect_key_pressed('\n', VK_RETURN, LEFT_CTRL_PRESSED);

    write_console_pipe("\xc4\x85");
    if (get_key_input(VK_MENU, &ir))
    {
        expect_key_input(0x105, 'A', TRUE, LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED);
        expect_key_input(0x105, 'A', FALSE, LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED);
        expect_key_input(0, VK_MENU, FALSE, ENHANCED_KEY);
    }
    else
    {
        expect_key_input(0x105, 0, TRUE, 0);
        expect_key_input(0x105, 0, FALSE, 0);
    }

    for (i = 0; i < ARRAY_SIZE(escape_test); i++)
    {
        write_console_pipe(escape_test[i].str);
        expect_key_pressed_ctx(i, 0, escape_test[i].vk, escape_test[i].ctrl);
    }

    for (i = 0; i < ARRAY_SIZE(escape_char_test); i++)
    {
        write_console_pipe(escape_char_test[i].str);
        expect_char_key_ctrl(escape_char_test[i].ch, escape_char_test[i].ctrl);
    }

    for (i = 0x80; i < 0x100; i += 11)
    {
        buf[0] = i;
        buf[1] = 0;
        write_console_pipe(buf);
        expect_empty_output();
    }
}

static void child_process(HANDLE pipe)
{
    HANDLE output, input;
    DWORD size, count;
    char buf[4096];
    BOOL ret;

    output = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(output != INVALID_HANDLE_VALUE, "could not open console output\n");

    input = CreateFileA("CONIN$", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(output != INVALID_HANDLE_VALUE, "could not open console output\n");

    while(ReadFile(pipe, buf, sizeof(buf), &size, NULL))
    {
        const struct pseudoconsole_req *req = (void *)buf;
        switch (req->type)
        {
        case REQ_CREATE_SCREEN_BUFFER:
            {
                HANDLE handle;
                SetLastError(0xdeadbeef);
                handle = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                                   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                                   CONSOLE_TEXTMODE_BUFFER, NULL);
                ok(handle != INVALID_HANDLE_VALUE, "CreateConsoleScreenBuffer failed: %lu\n", GetLastError());
                ret = WriteFile(pipe, &handle, sizeof(handle), &count, NULL);
                ok(ret, "WriteFile failed: %lu\n", GetLastError());
                break;
            }

        case REQ_GET_INPUT:
            {
                INPUT_RECORD record;
                ret = ReadConsoleInputW(input, &record, 1, &count);
                ok(ret, "ReadConsoleInputW failed: %lu\n", GetLastError());
                ok(count == 1, "count = %lu\n", count);
                ret = WriteFile(pipe, &record, sizeof(record), &count, NULL);
                ok(ret, "WriteFile failed: %lu\n", GetLastError());
                break;
            }

        case REQ_GET_SB_INFO:
            {
                CONSOLE_SCREEN_BUFFER_INFO info;
                ret = GetConsoleScreenBufferInfo(output, &info);
                ok(ret, "GetConsoleScreenBufferInfo failed: %lu\n", GetLastError());
                ret = WriteFile(pipe, &info, sizeof(info), &count, NULL);
                ok(ret, "WriteFile failed: %lu\n", GetLastError());
                break;
            }

        case REQ_READ_CONSOLE:
            ret = ReadConsoleW(input, buf, req->u.size, &count, NULL );
            ok(ret, "ReadConsoleW failed: %lu\n", GetLastError());
            ret = WriteFile(pipe, buf, count * sizeof(WCHAR), NULL, NULL);
            ok(ret, "WriteFile failed: %lu\n", GetLastError());
            break;

        case REQ_READ_CONSOLE_A:
            count = req->u.size;
            memset(buf, 0xcc, sizeof(buf));
            ret = ReadConsoleA(input, buf, count, &count, NULL );
            ok(ret, "ReadConsoleA failed: %lu\n", GetLastError());
            ret = WriteFile(pipe, buf, count, NULL, NULL);
            ok(ret, "WriteFile failed: %lu\n", GetLastError());
            break;

        case REQ_READ_CONSOLE_FILE:
            count = req->u.size;
            memset(buf, 0xcc, sizeof(buf));
            ret = ReadFile(input, buf, count, &count, NULL );
            ok(ret, "ReadFile failed: %lu\n", GetLastError());
            ret = WriteFile(pipe, buf, count, NULL, NULL);
            ok(ret, "WriteFile failed: %lu\n", GetLastError());
            break;

        case REQ_READ_CONSOLE_CONTROL:
            {
                CONSOLE_READCONSOLE_CONTROL crc;
                WCHAR result[1024];
                WCHAR *ptr = (void *)((char *)result + sizeof(DWORD));

                count = req->u.control.size;
                memset(result, 0xcc, sizeof(result));
                crc.nLength = sizeof(crc);
                crc.dwCtrlWakeupMask = req->u.control.mask;
                crc.nInitialChars = wcslen(req->u.control.initial);
                crc.dwControlKeyState = 0xa5;
                memcpy(ptr, req->u.control.initial, crc.nInitialChars * sizeof(WCHAR));
                ret = ReadConsoleW(input, ptr, count, &count, &crc);
                ok(ret, "ReadConsoleW failed: %lu\n", GetLastError());
                *(DWORD *)result = crc.dwControlKeyState;
                ret = WriteFile(pipe, result, sizeof(DWORD) + count * sizeof(WCHAR), NULL, NULL);
                ok(ret, "WriteFile failed: %lu\n", GetLastError());
            }
            break;

        case REQ_SCROLL:
            ret = ScrollConsoleScreenBufferW(output, &req->u.scroll.rect, NULL, req->u.scroll.dst, &req->u.scroll.fill);
            ok(ret, "ScrollConsoleScreenBuffer failed: %lu\n", GetLastError());
            break;

        case REQ_FILL_CHAR:
            ret = FillConsoleOutputCharacterW(output, req->u.fill.ch, req->u.fill.count, req->u.fill.coord, &count);
            ok(ret, "FillConsoleOutputCharacter failed: %lu\n", GetLastError());
            ok(count == req->u.fill.count, "count = %lu, expected %lu\n", count, req->u.fill.count);
            break;

        case REQ_SET_ACTIVE:
            output = req->u.handle;
            ret = SetConsoleActiveScreenBuffer(output);
            ok(ret, "SetConsoleActiveScreenBuffer failed: %lu\n", GetLastError());
            break;

        case REQ_SET_CURSOR:
            ret = SetConsoleCursorPosition(output, req->u.coord);
            ok(ret, "SetConsoleCursorPosition failed: %lu\n", GetLastError());
            break;

        case REQ_SET_INPUT_CP:
            ret = SetConsoleCP(req->u.cp);
            ok(ret, "SetConsoleCP failed: %lu\n", GetLastError());
            break;

        case REQ_SET_INPUT_MODE:
            ret = SetConsoleMode(input, req->u.mode);
            ok(ret, "SetConsoleMode failed: %lu\n", GetLastError());
            break;

        case REQ_SET_OUTPUT_MODE:
            ret = SetConsoleMode(output, req->u.mode);
            ok(ret, "SetConsoleMode failed: %lu\n", GetLastError());
            break;

        case REQ_SET_TITLE:
            ret = SetConsoleTitleW(req->u.string);
            ok(ret, "SetConsoleTitleW failed: %lu\n", GetLastError());
            break;

        case REQ_WRITE_CHARACTERS:
            ret = WriteConsoleOutputCharacterW(output, req->u.write_characters.buf,
                                               req->u.write_characters.len,
                                               req->u.write_characters.coord, &count);
            ok(ret, "WriteConsoleOutputCharacterW failed: %lu\n", GetLastError());
            break;

        case REQ_WRITE_CONSOLE:
            ret = WriteConsoleW(output, req->u.string, lstrlenW(req->u.string), NULL, NULL);
            ok(ret, "SetConsoleTitleW failed: %lu\n", GetLastError());
            break;

        case REQ_WRITE_OUTPUT:
            {
                SMALL_RECT region = req->u.write_output.region;
                ret = WriteConsoleOutputW(output, req->u.write_output.buf, req->u.write_output.size, req->u.write_output.coord, &region);
                ok(ret, "WriteConsoleOutput failed: %lu\n", GetLastError());
                ret = WriteFile(pipe, &region, sizeof(region), &count, NULL);
                ok(ret, "WriteFile failed: %lu\n", GetLastError());
                break;
            }

        default:
            ok(0, "unexpected request type %u\n", req->type);
        };
    }
    ok(GetLastError() == ERROR_BROKEN_PIPE, "ReadFile failed: %lu\n", GetLastError());
    CloseHandle(output);
    CloseHandle(input);
}

static void run_child(HANDLE console, HANDLE pipe, PROCESS_INFORMATION *info)
{
    STARTUPINFOEXA startup = {{ sizeof(startup) }};
    char **argv, cmdline[MAX_PATH];
    SIZE_T size;
    BOOL ret;

    InitializeProcThreadAttributeList(NULL, 1, 0, &size);
    startup.lpAttributeList = HeapAlloc(GetProcessHeap(), 0, size);
    InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &size);
    UpdateProcThreadAttribute(startup.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, console,
                              sizeof(console), NULL, NULL);

    winetest_get_mainargs(&argv);
    sprintf(cmdline, "\"%s\" %s child %p", argv[0], argv[1], pipe);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, EXTENDED_STARTUPINFO_PRESENT, NULL, NULL,
                         &startup.StartupInfo, info);
    ok(ret, "CreateProcessW failed: %lu\n", GetLastError());

    HeapFree(GetProcessHeap(), 0, startup.lpAttributeList);
}

static HPCON create_pseudo_console(HANDLE *console_pipe_end, PROCESS_INFORMATION *process_info)
{
    SECURITY_ATTRIBUTES sec_attr = { sizeof(sec_attr), NULL, TRUE };
    HANDLE child_pipe_end;
    COORD size = { 30, 40 };
    DWORD read_mode;
    HPCON console;
    HRESULT hres;
    BOOL r;

    console_pipe = CreateNamedPipeW(L"\\\\.\\pipe\\pseudoconsoleconn", PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                    PIPE_WAIT | PIPE_TYPE_BYTE, 1, 4096, 4096, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(console_pipe != INVALID_HANDLE_VALUE, "CreateNamedPipeW failed: %lu\n", GetLastError());

    *console_pipe_end = CreateFileW(L"\\\\.\\pipe\\pseudoconsoleconn", GENERIC_READ | GENERIC_WRITE,
                                    0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    ok(*console_pipe_end != INVALID_HANDLE_VALUE, "CreateFile failed: %lu\n", GetLastError());

    child_pipe = CreateNamedPipeW(L"\\\\.\\pipe\\pseudoconsoleserver", PIPE_ACCESS_DUPLEX,
                                  PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 1, 5000, 6000,
                                  NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(child_pipe != INVALID_HANDLE_VALUE, "CreateNamedPipeW failed: %lu\n", GetLastError());

    child_pipe_end = CreateFileW(L"\\\\.\\pipe\\pseudoconsoleserver", GENERIC_READ | GENERIC_WRITE, 0,
                                 &sec_attr, OPEN_EXISTING, 0, NULL);
    ok(child_pipe_end != INVALID_HANDLE_VALUE, "CreateFile failed: %lu\n", GetLastError());

    read_mode = PIPE_READMODE_MESSAGE;
    r = SetNamedPipeHandleState(child_pipe_end, &read_mode, NULL, NULL);
    ok(r, "SetNamedPipeHandleState failed: %lu\n", GetLastError());

    hres = pCreatePseudoConsole(size, *console_pipe_end, *console_pipe_end, 0, &console);
    ok(hres == S_OK, "CreatePseudoConsole failed: %08lx\n", hres);

    run_child(console, child_pipe_end, process_info);
    CloseHandle(child_pipe_end);
    return console;
}

static void test_pseudoconsole(void)
{
    PROCESS_INFORMATION process_info;
    HANDLE console_pipe_end;
    BOOL broken_version;
    HPCON console;

    console = create_pseudo_console(&console_pipe_end, &process_info);

    child_string_request(REQ_SET_TITLE, L"test title");
    expect_output_sequence("\x1b[2J");   /* erase display */
    skip_hide_cursor();
    expect_output_sequence("\x1b[m");    /* default attributes */
    expect_output_sequence("\x1b[H");    /* set cursor */
    skip_sequence("\x1b[H");             /* some windows versions emit it twice */
    expect_output_sequence("\x1b]0;test title"); /* set title */
    broken_version = skip_byte(0);       /* some win versions emit nullbyte */
    expect_output_sequence("\x07");
    skip_sequence("\x1b[?25h");          /* show cursor */
    expect_empty_output();

    if (!broken_version)
    {
        if (test_tty_output())
        {
            test_read_console_control();
            test_read_console();
            test_tty_input();
        }
        else
            win_skip("Partially supported windows version. Skipping rest of the tests\n");
    }
    else win_skip("Skipping tty tests on broken Windows version\n");

    CloseHandle(child_pipe);
    wait_child_process(&process_info);

    /* native sometimes clears the screen here */
    if (skip_sequence("\x1b[25l"))
    {
        unsigned int i;
        skip_sequence("\x1b[H");
        for (i = 0; i < 40; i++)
        {
            expect_output_sequence("\x1b[K");
            if (i != 39) expect_output_sequence("\r\n");
        }
        skip_sequence("\x1b[H\x1b[?25h");
        /* native sometimes redraws the screen afterwards */
        if (skip_sequence("\x1b[25l"))
        {
            unsigned int line_feed = 0;
            /* not checking exact output, depends too heavily on previous tests */
            for (i = 0; i < console_output_count - 2; i++)
                if (!memcmp(console_output + i, "\r\n", 2)) line_feed++;
            ok(line_feed == 39, "Wrong number of line feeds %u\n", line_feed);
            console_output_count = 0;
        }
    }
    expect_empty_output();

    pClosePseudoConsole(console);
    CloseHandle(console_pipe_end);
    CloseHandle(console_pipe);
}

START_TEST(tty)
{
    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    char **argv;
    int argc;

    argc = winetest_get_mainargs(&argv);
    if (argc > 3)
    {
        HANDLE pipe;
        DWORD mode;
        sscanf(argv[3], "%p", &pipe);
        /* if std output is console, silence debug output so it does not interfere with tests */
        if (GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &mode))
            winetest_debug = 0;
        child_process(pipe);
        return;
    }

    pCreatePseudoConsole = (void *)GetProcAddress(kernel32, "CreatePseudoConsole");
    pClosePseudoConsole  = (void *)GetProcAddress(kernel32, "ClosePseudoConsole");
    if (!pCreatePseudoConsole)
    {
        win_skip("CreatePseudoConsole is not available\n");
        return;
    }

    test_pseudoconsole();
}
