/*
 * AILinux ConPTY resize behavior probe (Windows and Wine).
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * This probe never copies system DLLs or modifies its Wine prefix.
 */
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0a00
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE ProcThreadAttributeValue(ProcThreadAttributePseudoConsole, FALSE, TRUE, FALSE)
#endif

typedef HRESULT (WINAPI *create_pty_fn)(COORD,HANDLE,HANDLE,DWORD,HPCON *);
typedef HRESULT (WINAPI *resize_pty_fn)(HPCON,COORD);
typedef VOID (WINAPI *close_pty_fn)(HPCON);

static int run_child(void)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    HANDLE conout = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    BOOL ok;
    DWORD error;
    if (conout == INVALID_HANDLE_VALUE) return 100 + GetLastError() % 100;
    ok = GetConsoleScreenBufferInfo(conout, &info);
    error = GetLastError();
    CloseHandle(conout);
    if (!ok) return 100 + error % 100;
    /* The parent must have resized to 100 columns, 40 rows. */
    return info.dwSize.X == 100 && info.dwSize.Y == 40 ? 0 : 12;
}

static void handles_close(HANDLE *h)
{
    if (*h && *h != INVALID_HANDLE_VALUE) CloseHandle(*h);
    *h = NULL;
}

int wmain(int argc, WCHAR **argv)
{
    STARTUPINFOEXW startup;
    PROCESS_INFORMATION pi = {0};
    HANDLE input_r = NULL, input_w = NULL, output_r = NULL, output_w = NULL;
    HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
    create_pty_fn create_pty = (create_pty_fn)(void *)GetProcAddress(kernel, "CreatePseudoConsole");
    resize_pty_fn resize_pty = (resize_pty_fn)(void *)GetProcAddress(kernel, "ResizePseudoConsole");
    close_pty_fn close_pty = (close_pty_fn)(void *)GetProcAddress(kernel, "ClosePseudoConsole");
    HRESULT hr;
    HPCON pty = NULL;
    COORD initial = {80,25}, desired = {100,40}, zero = {0,40}, negative = {-1,40}, intermediate = {120,50};
    SIZE_T attr_size = 0;
    WCHAR exe[MAX_PATH], cmd[MAX_PATH + 48];
    DWORD code = 255;
    BOOL started = FALSE;
    BOOL attrs_initialized = FALSE;
    BOOL stress = argc > 1 && !lstrcmpiW(argv[1], L"--stress");
    BOOL close_only = argc > 1 && !lstrcmpiW(argv[1], L"--close-only");
    DWORD i;
    int result = 1;

    if (argc > 1 && !lstrcmpiW(argv[1], L"--child")) return run_child();
    if (!create_pty || !resize_pty || !close_pty)
    {
        puts("CONPTY_API_EXPORT_MISSING");
        return 2;
    }
    if (!CreatePipe(&input_r, &input_w, NULL, 0) ||
        !CreatePipe(&output_r, &output_w, NULL, 0))
    {
        printf("CONPTY_PIPE_FAILED=%lu\n", GetLastError());
        goto cleanup;
    }
    hr = create_pty(initial, input_r, output_w, 0, &pty);
    printf("CREATE_PSEUDOCONSOLE=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr) || !pty) goto cleanup;

    if (close_only)
    {
        puts("CONPTY_CLOSE_ONLY_READY");
        result = 0;
        goto cleanup;
    }
    if (stress)
    {
        for (i = 0; i < 256; i++)
        {
            COORD size = { (SHORT)(80 + i % 77), (SHORT)(25 + i % 42) };
            hr = resize_pty(pty, size);
            if (FAILED(hr))
            {
                printf("CONPTY_STRESS_WRITE_FAILED=%lu hresult=0x%08lx\n",
                       (unsigned long)i, (unsigned long)hr);
                goto cleanup;
            }
        }
        printf("CONPTY_STRESS_WRITES=%lu\n", (unsigned long)i);
    }

    hr = resize_pty(pty, zero);
    printf("RESIZE_ZERO_HRESULT=0x%08lx\n", (unsigned long)hr);
    /* Reference winconpty accepts zero size at its API boundary. */
    if (hr != S_OK) goto cleanup;
    hr = resize_pty(pty, negative);
    printf("RESIZE_NEGATIVE_HRESULT=0x%08lx\n", (unsigned long)hr);
    if (hr != E_INVALIDARG) goto cleanup;

    hr = resize_pty(pty, intermediate);
    printf("RESIZE_INTERMEDIATE_HRESULT=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr)) goto cleanup;

    hr = resize_pty(pty, desired);
    printf("RESIZE_VALID_HRESULT=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr)) goto cleanup;
    Sleep(stress ? 750 : 250);

    InitializeProcThreadAttributeList(NULL, 1, 0, &attr_size);
    ZeroMemory(&startup, sizeof(startup));
    startup.StartupInfo.cb = sizeof(startup);
    startup.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, attr_size);
    if (!startup.lpAttributeList) goto cleanup;
    if (!InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &attr_size))
        goto cleanup;
    attrs_initialized = TRUE;
    if (!UpdateProcThreadAttribute(startup.lpAttributeList, 0,
                                   PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                                   pty, sizeof(pty), NULL, NULL))
        goto cleanup;
    if (!GetModuleFileNameW(NULL, exe, MAX_PATH)) goto cleanup;
    if (swprintf(cmd, sizeof(cmd) / sizeof(cmd[0]), L"\"%s\" --child", exe) <= 0)
        goto cleanup;
    started = CreateProcessW(exe, cmd, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT,
                             NULL, NULL, &startup.StartupInfo, &pi);
    if (!started)
    {
        printf("CONPTY_CHILD_START_FAILED=%lu\n", GetLastError());
        goto cleanup;
    }
    if (WaitForSingleObject(pi.hProcess, 15000) != WAIT_OBJECT_0)
    {
        puts("CONPTY_CHILD_TIMEOUT");
        TerminateProcess(pi.hProcess, 90);
        goto cleanup;
    }
    if (!GetExitCodeProcess(pi.hProcess, &code)) goto cleanup;
    printf("CHILD_CONSOLE_RESIZE_EXIT=%lu\n", code);
    if (code) goto cleanup;
    puts(stress ? "CONPTY_STRESS_PASS" : "CONPTY_RESIZE_BEHAVIOR_PASS");
    result = 0;

cleanup:
    if (startup.lpAttributeList)
    {
        if (attrs_initialized) DeleteProcThreadAttributeList(startup.lpAttributeList);
        HeapFree(GetProcessHeap(), 0, startup.lpAttributeList);
    }
    handles_close(&pi.hThread);
    handles_close(&pi.hProcess);
    if (pty)
    {
        ULONGLONG before_close = GetTickCount64();
        close_pty(pty);
        printf("CONPTY_CLOSE_MS=%llu\n", (unsigned long long)(GetTickCount64() - before_close));
    }
    handles_close(&input_r);
    handles_close(&input_w);
    handles_close(&output_r);
    handles_close(&output_w);
    if (result) puts("CONPTY_RESIZE_BEHAVIOR_FAIL");
    return result;
}
