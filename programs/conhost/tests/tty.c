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

    o.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    ret = ReadFile(console_pipe, console_output + console_output_count,
                   sizeof(console_output) - console_output_count, NULL, &o);
    if (!ret)
    {
        ok_(__FILE__,line)(GetLastError() == ERROR_IO_PENDING, "read failed: %u\n", GetLastError());
        if (GetLastError() != ERROR_IO_PENDING) return;
        WaitForSingleObject(o.hEvent, 1000);
    }
    ret = GetOverlappedResult(console_pipe, &o, &count, FALSE);
    if (!ret && GetLastError() == ERROR_IO_PENDING)
        CancelIoEx(console_pipe, &o);

    ok_(__FILE__,line)(ret, "Read file failed: %u\n", GetLastError());
    CloseHandle(o.hEvent);
    if (ret) console_output_count += count;
}

#define expect_empty_output() expect_empty_output_(__LINE__)
static void expect_empty_output_(unsigned int line)
{
    DWORD avail;
    BOOL ret;

    ret = PeekNamedPipe(console_pipe, NULL, 0, NULL, &avail, NULL);
    ok_(__FILE__,line)(ret, "PeekNamedPipe failed: %u\n", GetLastError());
    ok_(__FILE__,line)(!avail, "avail = %u\n", avail);
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

#define skip_hide_cursor() skip_hide_cursor_(__LINE__)
static BOOL skip_hide_cursor_(unsigned int line)
{
    if (!console_output_count) fetch_console_output_(line);
    return skip_sequence_(line, "\x1b[25l") || broken(skip_sequence_(line, "\x1b[?25l"));
}

enum req_type
{
    REQ_SET_TITLE,
};

struct pseudoconsole_req
{
    enum req_type type;
    union
    {
        WCHAR string[1];
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
    ok(ret, "WriteFile failed: %u\n", GetLastError());
}

static void child_process(HANDLE pipe)
{
    HANDLE output, input;
    char buf[4096];
    DWORD size;
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
        case REQ_SET_TITLE:
            ret = SetConsoleTitleW(req->u.string);
            ok(ret, "SetConsoleTitleW failed: %u\n", GetLastError());
            break;

        default:
            ok(0, "unexpected request type %u\n", req->type);
        };
    }
    ok(GetLastError() == ERROR_BROKEN_PIPE, "ReadFile failed: %u\n", GetLastError());
    CloseHandle(output);
    CloseHandle(input);
}

static HANDLE run_child(HANDLE console, HANDLE pipe)
{
    STARTUPINFOEXA startup = {{ sizeof(startup) }};
    char **argv, cmdline[MAX_PATH];
    PROCESS_INFORMATION info;
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
                         &startup.StartupInfo, &info);
    ok(ret, "CreateProcessW failed: %u\n", GetLastError());

    CloseHandle(info.hThread);
    HeapFree(GetProcessHeap(), 0, startup.lpAttributeList);
    return info.hProcess;
}

static HPCON create_pseudo_console(HANDLE *console_pipe_end, HANDLE *child_process)
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
    ok(console_pipe != INVALID_HANDLE_VALUE, "CreateNamedPipeW failed: %u\n", GetLastError());

    *console_pipe_end = CreateFileW(L"\\\\.\\pipe\\pseudoconsoleconn", GENERIC_READ | GENERIC_WRITE,
                                    0, &sec_attr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    ok(*console_pipe_end != INVALID_HANDLE_VALUE, "CreateFile failed: %u\n", GetLastError());

    child_pipe = CreateNamedPipeW(L"\\\\.\\pipe\\pseudoconsoleserver", PIPE_ACCESS_DUPLEX,
                                  PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 1, 5000, 6000,
                                  NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(child_pipe != INVALID_HANDLE_VALUE, "CreateNamedPipeW failed: %u\n", GetLastError());

    child_pipe_end = CreateFileW(L"\\\\.\\pipe\\pseudoconsoleserver", GENERIC_READ | GENERIC_WRITE, 0,
                                 &sec_attr, OPEN_EXISTING, 0, NULL);
    ok(child_pipe_end != INVALID_HANDLE_VALUE, "CreateFile failed: %u\n", GetLastError());

    read_mode = PIPE_READMODE_MESSAGE;
    r = SetNamedPipeHandleState(child_pipe_end, &read_mode, NULL, NULL);
    ok(r, "SetNamedPipeHandleState failed: %u\n", GetLastError());

    hres = pCreatePseudoConsole(size, *console_pipe_end, *console_pipe_end, 0, &console);
    ok(hres == S_OK, "CreatePseudoConsole failed: %08x\n", hres);

    *child_process = run_child(console, child_pipe_end);
    CloseHandle(child_pipe_end);
    return console;
}

static void test_pseudoconsole(void)
{
    HANDLE console_pipe_end, child_process;
    HPCON console;

    console = create_pseudo_console(&console_pipe_end, &child_process);

    child_string_request(REQ_SET_TITLE, L"test title");
    expect_output_sequence("\x1b[2J");   /* erase display */
    skip_hide_cursor();
    expect_output_sequence("\x1b[m");    /* default attributes */
    expect_output_sequence("\x1b[H");    /* set cursor */
    skip_sequence("\x1b[H");             /* some windows versions emit it twice */
    expect_output_sequence("\x1b]0;test title"); /* set title */
    skip_byte(0);                        /* some win versions emit nullbyte */
    expect_output_sequence("\x07");
    skip_sequence("\x1b[?25h");          /* show cursor */
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
        sscanf(argv[3], "%p", &pipe);
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
