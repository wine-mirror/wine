/* Native PE console control routing isolation, SPDX-License-Identifier: LGPL-2.1-or-later */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <wchar.h>
static HANDLE handled;
static BOOL WINAPI on_control(DWORD signal)
{
    if(signal != CTRL_C_EVENT && signal != CTRL_BREAK_EVENT) return FALSE;
    SetEvent(handled);
    return TRUE;
}
int wmain(int argc, wchar_t **argv)
{
    DWORD event_type;
    if(argc != 2 || (wcscmp(argv[1],L"--ctrl-c") && wcscmp(argv[1],L"--ctrl-break"))) return 3;
    event_type = !wcscmp(argv[1], L"--ctrl-c") ? CTRL_C_EVENT : CTRL_BREAK_EVENT;
    if (!GetConsoleWindow() && !AllocConsole())
    {
        printf("ALLOC_CONSOLE_FAILED=%lu\n",GetLastError());
        return 7;
    }
    handled = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!handled || !SetConsoleCtrlHandler(on_control, TRUE)) return 4;
    if(!GenerateConsoleCtrlEvent(event_type, 0))
    {
        printf("SIGNAL_GENERATE_FAILED=%lu\n",GetLastError());
        return 5;
    }
    if(WaitForSingleObject(handled, 3000) != WAIT_OBJECT_0)
    {
        puts("SIGNAL_HANDLER_NOT_DELIVERED");
        return 6;
    }
    puts(event_type == CTRL_C_EVENT ? "CTRL_C_DIRECT_PASS" : "CTRL_BREAK_DIRECT_PASS");
    SetConsoleCtrlHandler(on_control,FALSE);
    CloseHandle(handled);
    return 0;
}
