/*
 * AILinux ConPTY console control signals and VT smoke probe.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * Run the same PE binaries in Windows and a separate Wine test prefix.
 */
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0a00
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE ProcThreadAttributeValue(ProcThreadAttributePseudoConsole, FALSE, TRUE, FALSE)
#endif

typedef HRESULT (WINAPI *create_pty_fn)(COORD,HANDLE,HANDLE,DWORD,HPCON *);
typedef VOID (WINAPI *close_pty_fn)(HPCON);

#define READY_MARK "AILINUX_CONPTY_SIGNAL_READY"
#define CTRL_MARK "AILINUX_CONPTY_CONTROL_RECEIVED"
#define VT_MARK "AILINUX_CONPTY_VT_OUTPUT"
#define CAPACITY 32768

static HANDLE signal_event;
static DWORD expected_signal;
static volatile LONG received_signal;

static BOOL WINAPI signal_handler(DWORD type)
{
    if (type != expected_signal) return FALSE;
    InterlockedIncrement(&received_signal);
    SetEvent(signal_event);
    return TRUE;
}

static void close_handle(HANDLE *handle)
{
    if (*handle && *handle != INVALID_HANDLE_VALUE) CloseHandle(*handle);
    *handle = NULL;
}

static BOOL emit(HANDLE output, const char *s)
{
    DWORD n = 0, len = (DWORD)strlen(s);
    return WriteConsoleA(output, s, len, &n, NULL) && n == len;
}

static int run_child(const wchar_t *mode)
{
    HANDLE conout = INVALID_HANDLE_VALUE, conin = INVALID_HANDLE_VALUE;
    DWORD mode_flags;
    int result = 1;

    conout = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (conout == INVALID_HANDLE_VALUE) return 11;
    conin = CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (conin == INVALID_HANDLE_VALUE) { result = 12; goto done; }

    if (!wcscmp(mode, L"--child-vt"))
    {
        if (!GetConsoleMode(conout, &mode_flags)) { result = 13; goto done; }
        if (!SetConsoleMode(conout, mode_flags | ENABLE_PROCESSED_OUTPUT |
                           ENABLE_VIRTUAL_TERMINAL_PROCESSING))
        {
            result = 14;
            goto done;
        }
        if (!emit(conout, "\x1b[32m" VT_MARK "\x1b[0m\r\n"))
        {
            result = 15;
            goto done;
        }
        result = 0;
        goto done;
    }

    expected_signal = !wcscmp(mode, L"--child-break") ? CTRL_BREAK_EVENT : CTRL_C_EVENT;
    signal_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!signal_event) { result = 16; goto done; }
    if (!GetConsoleMode(conin, &mode_flags) ||
        !SetConsoleMode(conin, mode_flags | ENABLE_PROCESSED_INPUT) ||
        !SetConsoleCtrlHandler(signal_handler, TRUE))
    {
        result = 17;
        goto done;
    }
    if (!emit(conout, READY_MARK "\r\n")) { result = 18; goto unhook; }

    if (expected_signal == CTRL_BREAK_EVENT)
    {
        if (!GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, 0))
        {
            printf("CONPTY_BREAK_GENERATE_ERROR=%lu\n", (unsigned long)GetLastError());
            result = 19;
            goto unhook;
        }
    }
    if (WaitForSingleObject(signal_event, 8000) != WAIT_OBJECT_0)
    {
        emit(conout, "AILINUX_CONPTY_SIGNAL_HANDLER_TIMEOUT\r\n");
        printf("CONPTY_CHILD_SIGNAL_TIMEOUT event=%lu\n",
               (unsigned long)expected_signal);
        result = 20;
        goto unhook;
    }
    if (InterlockedCompareExchange(&received_signal, 0, 0) != 1)
    {
        result = 21;
        goto unhook;
    }
    if (!emit(conout, CTRL_MARK "\r\n")) { result = 22; goto unhook; }
    /* The host must consume the output before this child closes CONOUT$.
     * Otherwise a fast exit can race the ConPTY output thread and discard
     * the final reply. Ack uses an actual CR, not LF, in canonical mode. */
    {
        char ack[32] = {0};
        DWORD n = 0;
        if (!ReadConsoleA(conin, ack, sizeof(ack) - 1, &n, NULL))
        {
            result = 23;
            goto unhook;
        }
        if (n < 3 || memcmp(ack, "ack", 3))
        {
            result = 24;
            goto unhook;
        }
    }
    /* Sentinel proves the child completed its control-handler handshake. */
    result = 77;
unhook:
    SetConsoleCtrlHandler(signal_handler, FALSE);
done:
    close_handle(&signal_event);
    close_handle(&conin);
    close_handle(&conout);
    return result;
}

static BOOL read_output(HANDLE output, char *buffer, DWORD *got, DWORD capacity)
{
    DWORD available, received;
    if (!PeekNamedPipe(output, NULL, 0, NULL, &available, NULL)) return FALSE;
    if (!available) return TRUE;
    if (available > capacity - *got - 1) available = capacity - *got - 1;
    if (!available) return FALSE;
    if (!ReadFile(output, buffer + *got, available, &received, NULL)) return FALSE;
    *got += received;
    buffer[*got] = 0;
    return TRUE;
}

int wmain(int argc, wchar_t **argv)
{
    const wchar_t *mode;
    const char *expected;
    STARTUPINFOEXW startup = {0};
    PROCESS_INFORMATION pi = {0};
    HANDLE in_r = NULL, in_w = NULL, out_r = NULL, out_w = NULL;
    HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
    create_pty_fn create_pty = (create_pty_fn)(void *)GetProcAddress(kernel, "CreatePseudoConsole");
    close_pty_fn close_pty = (close_pty_fn)(void *)GetProcAddress(kernel, "ClosePseudoConsole");
    HPCON pty = NULL;
    SIZE_T attr_size = 0;
    BOOL attrs_initialized = FALSE, ready = FALSE;
    DWORD written, exitcode = 999, got = 0;
    ULONGLONG deadline;
    WCHAR filename[MAX_PATH], cmd[MAX_PATH + 80];
    char output[CAPACITY] = {0};
    int rc = 1;

    if (argc > 1 && !wcsncmp(argv[1], L"--child-", 8)) return run_child(argv[1]);
    if (argc != 2 ||
        (wcscmp(argv[1], L"--ctrl-c") && wcscmp(argv[1], L"--ctrl-break") &&
         wcscmp(argv[1], L"--vt")))
    {
        puts("USAGE: conpty-control-probe.exe --ctrl-c|--ctrl-break|--vt");
        return 2;
    }
    mode = argv[1];
    expected = !wcscmp(mode, L"--vt") ? VT_MARK : CTRL_MARK;
    if (!create_pty || !close_pty) { puts("CONPTY_CONTROL_EXPORT_MISSING"); return 3; }
    if (!CreatePipe(&in_r, &in_w, NULL, 0) || !CreatePipe(&out_r, &out_w, NULL, 0))
        goto cleanup;
    {
        COORD dimensions = {100, 40};
        HRESULT hr = create_pty(dimensions, in_r, out_w, 0, &pty);
        if (FAILED(hr) || !pty)
        {
            printf("CONPTY_CONTROL_CREATE_HRESULT=0x%08lx\n", (unsigned long)hr);
            goto cleanup;
        }
    }
    close_handle(&in_r);
    close_handle(&out_w);
    InitializeProcThreadAttributeList(NULL, 1, 0, &attr_size);
    startup.StartupInfo.cb = sizeof(startup);
    startup.lpAttributeList = HeapAlloc(GetProcessHeap(), 0, attr_size);
    if (!startup.lpAttributeList) goto cleanup;
    if (!InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &attr_size))
        goto cleanup;
    attrs_initialized = TRUE;
    if (!UpdateProcThreadAttribute(startup.lpAttributeList, 0,
        PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, pty, sizeof(pty), NULL, NULL)) goto cleanup;
    if (!GetModuleFileNameW(NULL, filename, MAX_PATH)) goto cleanup;
    if (swprintf(cmd, sizeof(cmd) / sizeof(cmd[0]), L"\"%ls\" %ls",
                 filename, !wcscmp(mode, L"--ctrl-c") ? L"--child-c" :
                 !wcscmp(mode, L"--ctrl-break") ? L"--child-break" : L"--child-vt") <= 0) goto cleanup;
    if (!CreateProcessW(filename, cmd, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT,
                        NULL, NULL, &startup.StartupInfo, &pi)) goto cleanup;

    deadline = GetTickCount64() + 7000;
    while (GetTickCount64() < deadline)
    {
        if (!read_output(out_r, output, &got, sizeof(output))) goto cleanup;
        if (strstr(output, READY_MARK)) { ready = TRUE; break; }
        if (!wcscmp(mode, L"--vt")) break;
        if (WaitForSingleObject(pi.hProcess, 0) == WAIT_OBJECT_0) break;
        Sleep(20);
    }
    if (!wcscmp(mode, L"--ctrl-c"))
    {
        const char key = 3; /* Ctrl+C via processed ConPTY input */
        if (!ready) { puts("CONPTY_CTRL_C_NOT_READY"); goto cleanup; }
        if (!WriteFile(in_w, &key, 1, &written, NULL) || written != 1)
        {
            puts("CONPTY_CTRL_C_WRITE_FAILED");
            goto cleanup;
        }
    }
    if (wcscmp(mode, L"--vt"))
    {
        /* Drain asynchronously while child is alive. Wait for its explicit
         * output marker before sending the input acknowledgement. */
        deadline = GetTickCount64() + 9000;
        while (GetTickCount64() < deadline)
        {
            if (!read_output(out_r, output, &got, sizeof(output))) break;
            if (strstr(output, expected)) break;
            if (WaitForSingleObject(pi.hProcess, 0) == WAIT_OBJECT_0) break;
            Sleep(15);
        }
        if (strstr(output, expected))
        {
            const char ack[] = "ack\r";
            if (!WriteFile(in_w, ack, sizeof(ack) - 1, &written, NULL) ||
                written != sizeof(ack) - 1)
            {
                puts("CONPTY_CONTROL_ACK_WRITE_FAILED");
                goto cleanup;
            }
        }
    }
    if (WaitForSingleObject(pi.hProcess, 10500) != WAIT_OBJECT_0)
    {
        puts("CONPTY_CONTROL_CHILD_TIMEOUT");
        TerminateProcess(pi.hProcess, 90);
        goto cleanup;
    }
    if (!GetExitCodeProcess(pi.hProcess, &exitcode)) goto cleanup;
    printf("CONPTY_CONTROL_CHILD_EXIT=%lu\n", (unsigned long)exitcode);
    if (exitcode != (!wcscmp(mode, L"--vt") ? 0u : 77u))
    {
        printf("CONPTY_CONTROL_CHILD_FAILED mode=%ls exit=%lu expected=%lu\n",
               mode, (unsigned long)exitcode,
               (unsigned long)(!wcscmp(mode, L"--vt") ? 0u : 77u));
        goto cleanup;
    }
    deadline = GetTickCount64() + 1600;
    while (GetTickCount64() < deadline)
    {
        if (!read_output(out_r, output, &got, sizeof(output))) break;
        if (strstr(output, expected)) break;
        Sleep(20);
    }
    printf("CONPTY_CONTROL_OUTPUT_BYTES=%lu\n", (unsigned long)got);
    if (!strstr(output, expected))
    {
        printf("CONPTY_CONTROL_MARKER_MISSING output=%.180s\n", output);
        goto cleanup;
    }
    if (!wcscmp(mode, L"--vt"))
    {
        if (!strstr(output, "\x1b[") ) { puts("CONPTY_VT_ESCAPE_MISSING"); goto cleanup; }
        puts("CONPTY_VT_PASS");
    }
    else if (!wcscmp(mode, L"--ctrl-c")) puts("CONPTY_CTRL_C_PASS");
    else puts("CONPTY_CTRL_BREAK_PASS");
    rc = 0;

cleanup:
    if (startup.lpAttributeList)
    {
        if (attrs_initialized) DeleteProcThreadAttributeList(startup.lpAttributeList);
        HeapFree(GetProcessHeap(), 0, startup.lpAttributeList);
    }
    close_handle(&pi.hThread);
    close_handle(&pi.hProcess);
    if (pty) close_pty(pty);
    close_handle(&in_r);
    close_handle(&in_w);
    close_handle(&out_r);
    close_handle(&out_w);
    if (rc) puts("CONPTY_CONTROL_FAIL");
    return rc;
}
