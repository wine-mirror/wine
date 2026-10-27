/*
 * ConPTY bidirectional input/output regression for genuine Windows and Wine.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * No Microsoft binaries or network access; a disposable standalone PE test.
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

#define INPUT_TEXT "ailinux-ping42\r"
#define OUTPUT_MARK "AILINUX_CONPTY_ECHO_OK"
#define READ_CAP 32768

static void close_handle(HANDLE *h)
{
    if (*h && *h != INVALID_HANDLE_VALUE) CloseHandle(*h);
    *h = NULL;
}

static int io_child(void)
{
    HANDLE conin, conout;
    char data[128];
    DWORD n = 0, written = 0;
    const char *ready = "AILINUX_CONPTY_CHILD_READY\r\n";
    const char *reply = OUTPUT_MARK "\r\n";
    conin = CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (conin == INVALID_HANDLE_VALUE) return 21;
    conout = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (conout == INVALID_HANDLE_VALUE)
    {
        CloseHandle(conin);
        return 22;
    }
    {
        DWORD original = 0, modified = 0, check = 0;
        if (!GetConsoleMode(conin, &original))
        {
            CloseHandle(conout);
            CloseHandle(conin);
            return 31;
        }
        modified = original ^ ENABLE_ECHO_INPUT;
        if (!SetConsoleMode(conin, modified) ||
            !GetConsoleMode(conin, &check) || check != modified ||
            !SetConsoleMode(conin, original))
        {
            CloseHandle(conout);
            CloseHandle(conin);
            return 32;
        }
    }
    if (!WriteConsoleA(conout, ready, (DWORD)strlen(ready), &written, NULL) ||
        written != strlen(ready))
    {
        CloseHandle(conout);
        CloseHandle(conin);
        return 23;
    }
    memset(data, 0, sizeof(data));
    if (!ReadConsoleA(conin, data, sizeof(data) - 1, &n, NULL))
    {
        CloseHandle(conout);
        CloseHandle(conin);
        return 100 + GetLastError() % 100;
    }
    CloseHandle(conin);
    if (n >= sizeof(data) || strncmp(data, "ailinux-ping42", 14))
    {
        CloseHandle(conout);
        return 25;
    }
    if (!WriteConsoleA(conout, reply, (DWORD)strlen(reply), &written, NULL) ||
        written != strlen(reply))
    {
        CloseHandle(conout);
        return 26;
    }
    CloseHandle(conout);
    return 0;
}

int wmain(int argc, wchar_t **argv)
{
    HANDLE input_r = NULL, input_w = NULL, output_r = NULL, output_w = NULL;
    PROCESS_INFORMATION pi = {0};
    STARTUPINFOEXW startup = {0};
    SIZE_T attrs_size = 0;
    BOOL attrs_inited = FALSE;
    HMODULE kernel;
    create_pty_fn create_pty;
    close_pty_fn close_pty;
    HPCON pty = NULL;
    HRESULT hr;
    WCHAR path[MAX_PATH], cmd[MAX_PATH + 48];
    DWORD written = 0, child_rc = 999, bytes;
    char buffer[READ_CAP + 1];
    DWORD got = 0;
    ULONGLONG end;
    int rc = 1;

    if (argc > 1 && !wcscmp(argv[1], L"--child")) return io_child();
    kernel = GetModuleHandleW(L"kernel32.dll");
    create_pty = (create_pty_fn)(void *)GetProcAddress(kernel, "CreatePseudoConsole");
    close_pty = (close_pty_fn)(void *)GetProcAddress(kernel, "ClosePseudoConsole");
    if (!create_pty || !close_pty)
    {
        puts("CONPTY_IO_MISSING_EXPORT");
        return 2;
    }
    if (!CreatePipe(&input_r, &input_w, NULL, 0) ||
        !CreatePipe(&output_r, &output_w, NULL, 0))
    {
        printf("CONPTY_IO_PIPE_FAILED=%lu\n", GetLastError());
        goto cleanup;
    }
    {
        COORD size = {100, 40};
        hr = create_pty(size, input_r, output_w, 0, &pty);
    }
    printf("CONPTY_IO_CREATED=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr) || !pty) goto cleanup;

    /* ConHost duplicates its own pipe endpoints; only the host-side ends
     * are required to remain open after CreatePseudoConsole succeeds. */
    close_handle(&input_r);
    close_handle(&output_w);

    InitializeProcThreadAttributeList(NULL, 1, 0, &attrs_size);
    startup.StartupInfo.cb = sizeof(startup);
    startup.lpAttributeList = HeapAlloc(GetProcessHeap(), 0, attrs_size);
    if (!startup.lpAttributeList) goto cleanup;
    if (!InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &attrs_size))
        goto cleanup;
    attrs_inited = TRUE;
    if (!UpdateProcThreadAttribute(startup.lpAttributeList, 0,
                                   PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                                   pty, sizeof(pty), NULL, NULL))
        goto cleanup;
    if (!GetModuleFileNameW(NULL, path, MAX_PATH)) goto cleanup;
    if (swprintf(cmd, sizeof(cmd) / sizeof(cmd[0]), L"\"%s\" --child", path) <= 0)
        goto cleanup;
    if (!CreateProcessW(path, cmd, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT,
                        NULL, NULL, &startup.StartupInfo, &pi))
    {
        printf("CONPTY_IO_CHILD_SPAWN_FAILED=%lu\n", GetLastError());
        goto cleanup;
    }
    Sleep(180);
    if (!WriteFile(input_w, INPUT_TEXT, sizeof(INPUT_TEXT) - 1, &written, NULL) ||
        written != sizeof(INPUT_TEXT) - 1)
    {
        printf("CONPTY_IO_INPUT_WRITE_FAILED=%lu\n", GetLastError());
        goto cleanup;
    }
    /* Keep the sending endpoint alive while the child reads. */
    if (WaitForSingleObject(pi.hProcess, 14000) != WAIT_OBJECT_0)
    {
        puts("CONPTY_IO_CHILD_TIMEOUT");
        TerminateProcess(pi.hProcess, 90);
        goto cleanup;
    }
    if (!GetExitCodeProcess(pi.hProcess, &child_rc)) goto cleanup;
    printf("CONPTY_IO_CHILD_EXIT=%lu\n", (unsigned long)child_rc);
    if (child_rc) goto cleanup;

    /* Collect the terminal bytes, including any ANSI/VT sequences. */
    end = GetTickCount64() + 1500;
    while (got < READ_CAP && GetTickCount64() < end)
    {
        DWORD available = 0, chunk;
        if (!PeekNamedPipe(output_r, NULL, 0, NULL, &available, NULL)) break;
        if (!available)
        {
            Sleep(15);
            continue;
        }
        chunk = available;
        if (chunk > READ_CAP - got) chunk = READ_CAP - got;
        if (!ReadFile(output_r, buffer + got, chunk, &bytes, NULL) || !bytes) break;
        got += bytes;
    }
    buffer[got] = 0;
    printf("CONPTY_IO_OUTPUT_BYTES=%lu\n", (unsigned long)got);
    if (!strstr(buffer, "AILINUX_CONPTY_CHILD_READY") ||
        !strstr(buffer, OUTPUT_MARK))
    {
        puts("CONPTY_IO_OUTPUT_MISSING");
        goto cleanup;
    }
    puts("CONPTY_IO_ROUNDTRIP_PASS");
    rc = 0;

cleanup:
    if (startup.lpAttributeList)
    {
        if (attrs_inited) DeleteProcThreadAttributeList(startup.lpAttributeList);
        HeapFree(GetProcessHeap(), 0, startup.lpAttributeList);
    }
    close_handle(&pi.hThread);
    close_handle(&pi.hProcess);
    if (pty) close_pty(pty);
    close_handle(&input_r);
    close_handle(&input_w);
    close_handle(&output_r);
    close_handle(&output_w);
    if (rc) puts("CONPTY_IO_ROUNDTRIP_FAIL");
    return rc;
}
