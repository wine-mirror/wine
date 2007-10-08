/*
 * Unit tests for DDE functions
 *
 * Copyright (c) 2004 Dmitry Timoshkov
 * Copyright (c) 2007 James Hawkins
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
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "dde.h"
#include "ddeml.h"
#include "winerror.h"

#include "wine/test.h"

static const WCHAR TEST_DDE_SERVICE[] = {'T','e','s','t','D','D','E','S','e','r','v','i','c','e',0};

static char exec_cmdA[] = "ANSI dde command";
static WCHAR exec_cmdW[] = {'u','n','i','c','o','d','e',' ','d','d','e',' ','c','o','m','m','a','n','d',0};

static WNDPROC old_dde_client_wndproc;

static void create_dde_window(HWND *hwnd, LPCSTR name, WNDPROC wndproc)
{
    WNDCLASSA wcA;

    memset(&wcA, 0, sizeof(wcA));
    wcA.lpfnWndProc = wndproc;
    wcA.lpszClassName = name;
    wcA.hInstance = GetModuleHandleA(0);
    assert(RegisterClassA(&wcA));

    *hwnd = CreateWindowExA(0, name, NULL, WS_POPUP,
                            500, 500, CW_USEDEFAULT, CW_USEDEFAULT,
                            GetDesktopWindow(), 0, GetModuleHandleA(0), NULL);
    assert(*hwnd);
}

static LRESULT WINAPI dde_server_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    UINT_PTR lo, hi;
    char str[MAX_PATH], *ptr;
    HGLOBAL hglobal;
    DDEDATA *data;
    DDEPOKE *poke;
    DWORD size;

    static int msg_index = 0;
    static HWND client = 0;
    static BOOL executed = FALSE;

    if (msg < WM_DDE_FIRST || msg > WM_DDE_LAST)
        return DefWindowProcA(hwnd, msg, wparam, lparam);

    msg_index++;

    switch (msg)
    {
    case WM_DDE_INITIATE:
    {
        client = (HWND)wparam;
        ok(msg_index == 1, "Expected 1, got %d\n", msg_index);

        GlobalGetAtomNameA(LOWORD(lparam), str, MAX_PATH);
        ok(!lstrcmpA(str, "TestDDEService"), "Expected TestDDEService, got %s\n", str);

        GlobalGetAtomNameA(HIWORD(lparam), str, MAX_PATH);
        ok(!lstrcmpA(str, "TestDDETopic"), "Expected TestDDETopic, got %s\n", str);

        SendMessageA(client, WM_DDE_ACK, (WPARAM)hwnd, lparam);

        break;
    }

    case WM_DDE_REQUEST:
    {
        ok((msg_index >= 2 && msg_index <= 4) ||
           (msg_index >= 7 && msg_index <= 8),
           "Expected 2, 3, 4, 7 or 8, got %d\n", msg_index);
        ok(wparam == (WPARAM)client, "Expected client hwnd, got %08lx\n", wparam);
        ok(LOWORD(lparam) == CF_TEXT, "Expected CF_TEXT, got %d\n", LOWORD(lparam));

        GlobalGetAtomNameA(HIWORD(lparam), str, MAX_PATH);
        if (msg_index < 8)
            ok(!lstrcmpA(str, "request"), "Expected request, got %s\n", str);
        else
            ok(!lstrcmpA(str, "executed"), "Expected executed, got %s\n", str);

        if (msg_index == 8)
        {
            if (executed)
                lstrcpyA(str, "command executed\r\n");
            else
                lstrcpyA(str, "command not executed\r\n");
        }
        else
            lstrcpyA(str, "requested data\r\n");

        size = sizeof(DDEDATA) + lstrlenA(str) + 1;
        hglobal = GlobalAlloc(GMEM_MOVEABLE, size);
        ok(hglobal != NULL, "Expected non-NULL hglobal\n");

        data = GlobalLock(hglobal);
        ZeroMemory(data, size);

        /* setting fResponse to FALSE at this point destroys
         * the internal messaging state of native dde
         */
        data->fResponse = TRUE;

        if (msg_index == 2)
            data->fRelease = TRUE;
        else if (msg_index == 3)
            data->fAckReq = TRUE;

        data->cfFormat = CF_TEXT;
        lstrcpyA((LPSTR)data->Value, str);
        GlobalUnlock(hglobal);

        lparam = PackDDElParam(WM_DDE_ACK, (UINT)hglobal, HIWORD(lparam));
        PostMessageA(client, WM_DDE_DATA, (WPARAM)hwnd, lparam);

        break;
    }

    case WM_DDE_POKE:
    {
        ok(msg_index == 5 || msg_index == 6, "Expected 5 or 6, got %d\n", msg_index);
        ok(wparam == (WPARAM)client, "Expected client hwnd, got %08lx\n", wparam);

        UnpackDDElParam(WM_DDE_POKE, lparam, &lo, &hi);

        GlobalGetAtomNameA(hi, str, MAX_PATH);
        ok(!lstrcmpA(str, "poker"), "Expected poker, got %s\n", str);

        poke = GlobalLock((HGLOBAL)lo);
        ok(poke != NULL, "Expected non-NULL poke\n");
        ok(poke->unused == 0, "Expected 0, got %d\n", poke->unused);
        todo_wine
        {
            ok(poke->fRelease == TRUE, "Expected TRUE, got %d\n", poke->fRelease);
        }
        ok(poke->fReserved == 0, "Expected 0, got %d\n", poke->fReserved);
        ok(poke->cfFormat == CF_TEXT, "Expected CF_TEXT, got %d\n", poke->cfFormat);

        if (msg_index == 5)
            ok(lstrcmpA((LPSTR)poke->Value, "poke data\r\n"),
               "Expected 'poke data\\r\\n', got %s\n", poke->Value);
        else
            ok(!lstrcmpA((LPSTR)poke->Value, "poke data\r\n"),
               "Expected 'poke data\\r\\n', got %s\n", poke->Value);

        GlobalUnlock((HGLOBAL)lo);

        lparam = PackDDElParam(WM_DDE_ACK, DDE_FACK, hi);
        PostMessageA(client, WM_DDE_ACK, (WPARAM)hwnd, lparam);

        break;
    }

    case WM_DDE_EXECUTE:
    {
        ok(msg_index == 7, "Expected 7, got %d\n", msg_index);
        ok(wparam == (WPARAM)client, "Expected client hwnd, got %08lx\n", wparam);

        ptr = GlobalLock((HGLOBAL)lparam);
        ok(!lstrcmpA(ptr, "[Command(Var)]"), "Expected [Command(Var)], got %s\n", ptr);
        GlobalUnlock((HGLOBAL)lparam);

        executed = TRUE;

        lparam = ReuseDDElParam(lparam, WM_DDE_EXECUTE, WM_DDE_ACK, DDE_FACK, HIWORD(lparam));
        PostMessageA(client, WM_DDE_ACK, (WPARAM)hwnd, lparam);

        break;
    }

    case WM_DDE_TERMINATE:
    {
        ok(msg_index == 9, "Expected 9, got %d\n", msg_index);
        ok(wparam == (WPARAM)client, "Expected client hwnd, got %08lx\n", wparam);
        ok(lparam == 0, "Expected 0, got %08lx\n", lparam);

        PostMessageA(client, WM_DDE_TERMINATE, (WPARAM)hwnd, 0);

        break;
    }

    default:
        ok(FALSE, "Unhandled msg: %08x\n", msg);
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void test_msg_server(HANDLE hproc)
{
    MSG msg;
    HWND hwnd;

    create_dde_window(&hwnd, "dde_server", dde_server_wndproc);

    do
    {
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    } while (WaitForSingleObject(hproc, 500) == WAIT_TIMEOUT);

   DestroyWindow(hwnd);
}

static HDDEDATA CALLBACK client_ddeml_callback(UINT uType, UINT uFmt, HCONV hconv,
                                               HSZ hsz1, HSZ hsz2, HDDEDATA hdata,
                                               ULONG_PTR dwData1, ULONG_PTR dwData2)
{
    ok(FALSE, "Unhandled msg: %08x\n", uType);
    return 0;
}

static void test_ddeml_client(void)
{
    UINT ret;
    LPSTR str;
    DWORD size, res;
    HDDEDATA hdata, op;
    HSZ server, topic, item;
    DWORD client_pid;
    HCONV conversation;

    client_pid = 0;
    ret = DdeInitializeA(&client_pid, client_ddeml_callback, APPCMD_CLIENTONLY, 0);
    ok(ret == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", ret);

    /* FIXME: make these atoms global and check them in the server */

    server = DdeCreateStringHandleA(client_pid, "TestDDEService", CP_WINANSI);
    topic = DdeCreateStringHandleA(client_pid, "TestDDETopic", CP_WINANSI);

    DdeGetLastError(client_pid);
    conversation = DdeConnect(client_pid, server, topic, NULL);
    ok(conversation != NULL, "Expected non-NULL conversation\n");
    ret = DdeGetLastError(client_pid);
    ok(ret == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", ret);

    DdeFreeStringHandle(client_pid, server);

    item = DdeCreateStringHandleA(client_pid, "request", CP_WINANSI);

    /* XTYP_REQUEST, fRelease = TRUE */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    hdata = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_REQUEST, 500, &res);
    ret = DdeGetLastError(client_pid);
    ok(hdata == NULL, "Expected NULL hdata, got %p\n", hdata);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %08x\n", res);
    ok(ret == DMLERR_DATAACKTIMEOUT, "Expected DMLERR_DATAACKTIMEOUT, got %d\n", ret);

    /* XTYP_REQUEST, fAckReq = TRUE */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    hdata = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_REQUEST, 500, &res);
    ret = DdeGetLastError(client_pid);
    ok(hdata != NULL, "Expected non-NULL hdata\n");
    todo_wine
    {
        ok(res == DDE_FNOTPROCESSED, "Expected DDE_FNOTPROCESSED, got %d\n", res);
        ok(ret == DMLERR_MEMORY_ERROR, "Expected DMLERR_MEMORY_ERROR, got %d\n", ret);
    }

    str = (LPSTR)DdeAccessData(hdata, &size);
    ok(!lstrcmpA(str, "requested data\r\n"), "Expected 'requested data\\r\\n', got %s\n", str);
    ok(size == 19, "Expected 19, got %d\n", size);

    ret = DdeUnaccessData(hdata);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    /* XTYP_REQUEST, all params normal */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    hdata = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_REQUEST, 500, &res);
    ret = DdeGetLastError(client_pid);
    todo_wine
    {
        ok(hdata != NULL, "Expected non-NULL hdata\n");
        ok(res == DDE_FNOTPROCESSED, "Expected DDE_FNOTPROCESSED, got %d\n", res);
        ok(ret == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", ret);
    }

    str = (LPSTR)DdeAccessData(hdata, &size);
    todo_wine
    {
        ok(!lstrcmpA(str, "requested data\r\n"), "Expected 'requested data\\r\\n', got %s\n", str);
    }
    ok(size == 19, "Expected 19, got %d\n", size);

    ret = DdeUnaccessData(hdata);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    /* XTYP_REQUEST, no item */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    hdata = DdeClientTransaction(NULL, 0, conversation, 0, CF_TEXT, XTYP_REQUEST, 500, &res);
    ret = DdeGetLastError(client_pid);
    ok(hdata == NULL, "Expected NULL hdata, got %p\n", hdata);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %08x\n", res);
    todo_wine
    {
        ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);
    }

    DdeFreeStringHandle(client_pid, item);

    item = DdeCreateStringHandleA(client_pid, "poker", CP_WINANSI);

    lstrcpyA(str, "poke data\r\n");
    hdata = DdeCreateDataHandle(client_pid, (LPBYTE)str, lstrlenA(str) + 1,
                                0, item, CF_TEXT, 0);
    ok(hdata != NULL, "Expected non-NULL hdata\n");

    /* XTYP_POKE, no item */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction((LPBYTE)hdata, -1, conversation, 0, CF_TEXT, XTYP_POKE, 500, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    todo_wine
    {
        ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);
    }

    /* XTYP_POKE, no data */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, 0, CF_TEXT, XTYP_POKE, 500, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    todo_wine
    {
        ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);
    }

    /* XTYP_POKE, wrong size */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction((LPBYTE)hdata, 0, conversation, item, CF_TEXT, XTYP_POKE, 500, &res);
    ret = DdeGetLastError(client_pid);
    todo_wine
    {
        ok(op == (HDDEDATA)TRUE, "Expected TRUE, got %p\n", op);
        ok(res == DDE_FACK, "Expected DDE_FACK, got %d\n", res);
    }
    ok(ret == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", ret);

    /* XTYP_POKE, correct params */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction((LPBYTE)hdata, -1, conversation, item, CF_TEXT, XTYP_POKE, 500, &res);
    ret = DdeGetLastError(client_pid);
    todo_wine
    {
        ok(op == (HDDEDATA)TRUE, "Expected TRUE, got %p\n", op);
        ok(res == DDE_FACK, "Expected DDE_FACK, got %d\n", res);
        ok(ret == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", ret);
    }

    DdeFreeDataHandle(hdata);

    lstrcpyA(str, "[Command(Var)]");
    hdata = DdeCreateDataHandle(client_pid, (LPBYTE)str, lstrlenA(str) + 1,
                                0, NULL, CF_TEXT, 0);
    ok(hdata != NULL, "Expected non-NULL hdata\n");

    /* XTYP_EXECUTE, correct params */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction((LPBYTE)hdata, -1, conversation, NULL, 0, XTYP_EXECUTE, 5000, &res);
    ret = DdeGetLastError(client_pid);
    todo_wine
    {
        ok(op == (HDDEDATA)TRUE, "Expected TRUE, got %p\n", op);
        ok(res == DDE_FACK, "Expected DDE_FACK, got %d\n", res);
        ok(ret == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", ret);
    }

    /* XTYP_EXECUTE, no data */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, NULL, 0, XTYP_EXECUTE, 5000, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    todo_wine
    {
        ok(ret == DMLERR_MEMORY_ERROR, "Expected DMLERR_MEMORY_ERROR, got %d\n", ret);
    }

    /* XTYP_EXECUTE, no data, -1 size */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, -1, conversation, NULL, 0, XTYP_EXECUTE, 5000, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    todo_wine
    {
        ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);
    }

    DdeFreeStringHandle(client_pid, topic);
    DdeFreeDataHandle(hdata);

    item = DdeCreateStringHandleA(client_pid, "executed", CP_WINANSI);

    /* verify the execute */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    hdata = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_REQUEST, 500, &res);
    ret = DdeGetLastError(client_pid);
    todo_wine
    {
        ok(hdata != NULL, "Expected non-NULL hdata\n");
        ok(res == DDE_FNOTPROCESSED, "Expected DDE_FNOTPROCESSED, got %d\n", res);
        ok(ret == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", ret);
    }

    str = (LPSTR)DdeAccessData(hdata, &size);
    todo_wine
    {
        ok(!lstrcmpA(str, "command executed\r\n"), "Expected 'command executed\\r\\n', got %s\n", str);
        ok(size == 21, "Expected 21, got %d\n", size);
    }

    ret = DdeUnaccessData(hdata);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    /* invalid transactions */

    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_ADVREQ, 500, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    todo_wine
    {
        ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);
    }

    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_CONNECT, 500, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    todo_wine
    {
        ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);
    }

    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_CONNECT_CONFIRM, 500, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    todo_wine
    {
        ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);
    }

    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_DISCONNECT, 500, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    todo_wine
    {
        ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);
    }

    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_ERROR, 500, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    todo_wine
    {
        ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);
    }

    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_MONITOR, 500, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    todo_wine
    {
        ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);
    }

    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_REGISTER, 500, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    todo_wine
    {
        ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);
    }

    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_UNREGISTER, 500, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    todo_wine
    {
        ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);
    }

    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_WILDCONNECT, 500, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    todo_wine
    {
        ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);
    }

    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_XACT_COMPLETE, 500, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    todo_wine
    {
        ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);
    }

    DdeFreeStringHandle(client_pid, item);

    ret = DdeDisconnect(conversation);
    todo_wine
    {
        ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    }

    ret = DdeUninitialize(client_pid);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
}

static LRESULT WINAPI hook_dde_client_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    UINT_PTR lo, hi;

    trace("hook_dde_client_wndproc: %p %04x %08lx %08lx\n", hwnd, msg, wparam, lparam);

    switch (msg)
    {
    case WM_DDE_ACK:
        UnpackDDElParam(WM_DDE_ACK, lparam, &lo, &hi);
        trace("WM_DDE_ACK: status %04lx hglobal %p\n", lo, (HGLOBAL)hi);
        break;

    default:
        break;
    }
    return CallWindowProcA(old_dde_client_wndproc, hwnd, msg, wparam, lparam);
}

static LRESULT WINAPI dde_server_wndprocW(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    trace("dde_server_wndprocW: %p %04x %08lx %08lx\n", hwnd, msg, wparam, lparam);

    switch (msg)
    {
    case WM_DDE_INITIATE:
    {
        ATOM aService = GlobalAddAtomW(TEST_DDE_SERVICE);

        trace("server: got WM_DDE_INITIATE from %p with %08lx\n", (HWND)wparam, lparam);

        if (LOWORD(lparam) == aService)
        {
            ok(!IsWindowUnicode((HWND)wparam), "client should be an ANSI window\n");
            old_dde_client_wndproc = (WNDPROC)SetWindowLongPtrA((HWND)wparam, GWLP_WNDPROC, (ULONG_PTR)hook_dde_client_wndproc);
            trace("server: sending WM_DDE_ACK to %p\n", (HWND)wparam);
            SendMessageW((HWND)wparam, WM_DDE_ACK, (WPARAM)hwnd, MAKELPARAM(aService, 0));
        }
        else
            GlobalDeleteAtom(aService);
        return 0;
    }

    case WM_DDE_EXECUTE:
    {
        DDEACK ack;
        WORD status;
        LPCSTR cmd;
        UINT_PTR lo, hi;

        trace("server: got WM_DDE_EXECUTE from %p with %08lx\n", (HWND)wparam, lparam);

        UnpackDDElParam(WM_DDE_EXECUTE, lparam, &lo, &hi);
        trace("%08lx => lo %04lx hi %04lx\n", lparam, lo, hi);

        ack.bAppReturnCode = 0;
        ack.reserved = 0;
        ack.fBusy = 0;

        cmd = GlobalLock((HGLOBAL)hi);

        if (!cmd || (lstrcmpA(cmd, exec_cmdA) && lstrcmpW((LPCWSTR)cmd, exec_cmdW)))
        {
            trace("ignoring unknown WM_DDE_EXECUTE command\n");
            /* We have to send a negative acknowledge even if we don't
             * accept the command, otherwise Windows goes mad and next time
             * we send an acknowledge DDEML drops the connection.
             * Not sure how to call it: a bug or a feature.
             */
            ack.fAck = 0;
        }
        else
            ack.fAck = 1;
        GlobalUnlock((HGLOBAL)hi);

        trace("server: posting %s WM_DDE_ACK to %p\n", ack.fAck ? "POSITIVE" : "NEGATIVE", (HWND)wparam);

        status = *((WORD *)&ack);
        lparam = ReuseDDElParam(lparam, WM_DDE_EXECUTE, WM_DDE_ACK, status, hi);

        PostMessageW((HWND)wparam, WM_DDE_ACK, (WPARAM)hwnd, lparam);
        return 0;
    }

    case WM_DDE_TERMINATE:
    {
        DDEACK ack;
        WORD status;

        trace("server: got WM_DDE_TERMINATE from %p with %08lx\n", (HWND)wparam, lparam);

        ack.bAppReturnCode = 0;
        ack.reserved = 0;
        ack.fBusy = 0;
        ack.fAck = 1;

        trace("server: posting %s WM_DDE_ACK to %p\n", ack.fAck ? "POSITIVE" : "NEGATIVE", (HWND)wparam);

        status = *((WORD *)&ack);
        lparam = PackDDElParam(WM_DDE_ACK, status, 0);

        PostMessageW((HWND)wparam, WM_DDE_ACK, (WPARAM)hwnd, lparam);
        return 0;
    }

    default:
        break;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static LRESULT WINAPI dde_client_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static BOOL create_dde_windows(HWND *hwnd_client, HWND *hwnd_server)
{
    WNDCLASSA wcA;
    WNDCLASSW wcW;
    static const WCHAR server_class_name[] = {'d','d','e','_','s','e','r','v','e','r','_','w','i','n','d','o','w',0};
    static const char client_class_name[] = "dde_client_window";

    memset(&wcW, 0, sizeof(wcW));
    wcW.lpfnWndProc = dde_server_wndprocW;
    wcW.lpszClassName = server_class_name;
    wcW.hInstance = GetModuleHandleA(0);
    if (!RegisterClassW(&wcW)) return FALSE;

    memset(&wcA, 0, sizeof(wcA));
    wcA.lpfnWndProc = dde_client_wndproc;
    wcA.lpszClassName = client_class_name;
    wcA.hInstance = GetModuleHandleA(0);
    assert(RegisterClassA(&wcA));

    *hwnd_server = CreateWindowExW(0, server_class_name, NULL,
                                   WS_POPUP,
                                   100, 100, CW_USEDEFAULT, CW_USEDEFAULT,
                                   GetDesktopWindow(), 0,
                                   GetModuleHandleA(0), NULL);
    assert(*hwnd_server);

    *hwnd_client = CreateWindowExA(0, client_class_name, NULL,
                                   WS_POPUP,
                                   100, 100, CW_USEDEFAULT, CW_USEDEFAULT,
                                   GetDesktopWindow(), 0,
                                   GetModuleHandleA(0), NULL);
    assert(*hwnd_client);

    trace("server hwnd %p, client hwnd %p\n", *hwnd_server, *hwnd_client);

    ok(IsWindowUnicode(*hwnd_server), "server has to be a unicode window\n");
    ok(!IsWindowUnicode(*hwnd_client), "client has to be an ANSI window\n");

    return TRUE;
}

static HDDEDATA CALLBACK client_dde_callback(UINT uType, UINT uFmt, HCONV hconv,
                                     HSZ hsz1, HSZ hsz2, HDDEDATA hdata,
                                     ULONG_PTR dwData1, ULONG_PTR dwData2)
{
    static const char * const cmd_type[15] = {
        "XTYP_ERROR", "XTYP_ADVDATA", "XTYP_ADVREQ", "XTYP_ADVSTART",
        "XTYP_ADVSTOP", "XTYP_EXECUTE", "XTYP_CONNECT", "XTYP_CONNECT_CONFIRM",
        "XTYP_XACT_COMPLETE", "XTYP_POKE", "XTYP_REGISTER", "XTYP_REQUEST",
        "XTYP_DISCONNECT", "XTYP_UNREGISTER", "XTYP_WILDCONNECT" };
    UINT type;
    const char *cmd_name;

    type = (uType & XTYP_MASK) >> XTYP_SHIFT;
    cmd_name = (type >= 0 && type <= 14) ? cmd_type[type] : "unknown";

    trace("client_dde_callback: %04x (%s) %d %p %p %p %p %08lx %08lx\n",
          uType, cmd_name, uFmt, hconv, hsz1, hsz2, hdata, dwData1, dwData2);
    return 0;
}

static void test_dde_aw_transaction(void)
{
    HSZ hsz_server;
    DWORD dde_inst, ret, err;
    HCONV hconv;
    HWND hwnd_client, hwnd_server;
    CONVINFO info;
    HDDEDATA hdata;
    static char test_cmd[] = "test dde command";

    /* server: unicode, client: ansi */
    if (!create_dde_windows(&hwnd_client, &hwnd_server)) return;

    dde_inst = 0;
    ret = DdeInitializeA(&dde_inst, client_dde_callback, APPCMD_CLIENTONLY, 0);
    ok(ret == DMLERR_NO_ERROR, "DdeInitializeW failed with error %04x (%x)\n",
       ret, DdeGetLastError(dde_inst));

    hsz_server = DdeCreateStringHandleW(dde_inst, TEST_DDE_SERVICE, CP_WINUNICODE);

    hconv = DdeConnect(dde_inst, hsz_server, 0, NULL);
    ok(hconv != 0, "DdeConnect error %x\n", DdeGetLastError(dde_inst));
    err = DdeGetLastError(dde_inst);
    ok(err == DMLERR_NO_ERROR, "wrong dde error %x\n", err);

    info.cb = sizeof(info);
    ret = DdeQueryConvInfo(hconv, QID_SYNC, &info);
    ok(ret, "wrong info size %d, DdeQueryConvInfo error %x\n", ret, DdeGetLastError(dde_inst));
    /* should be CP_WINANSI since we used DdeInitializeA */
    ok(info.ConvCtxt.iCodePage == CP_WINANSI, "wrong iCodePage %d\n", info.ConvCtxt.iCodePage);
    ok(!info.hConvPartner, "unexpected info.hConvPartner: %p\n", info.hConvPartner);
todo_wine {
    ok((info.wStatus & DDE_FACK), "unexpected info.wStatus: %04x\n", info.wStatus);
}
    ok((info.wStatus & (ST_CONNECTED | ST_CLIENT)) == (ST_CONNECTED | ST_CLIENT), "unexpected info.wStatus: %04x\n", info.wStatus);
    ok(info.wConvst == XST_CONNECTED, "unexpected info.wConvst: %04x\n", info.wConvst);
    ok(info.wType == 0, "unexpected info.wType: %04x\n", info.wType);

    trace("hwnd %p, hwndPartner %p\n", info.hwnd, info.hwndPartner);

    trace("sending test client transaction command\n");
    ret = 0xdeadbeef;
    hdata = DdeClientTransaction((LPBYTE)test_cmd, strlen(test_cmd) + 1, hconv, (HSZ)0xdead, 0xbeef, XTYP_EXECUTE, 1000, &ret);
    ok(!hdata, "DdeClientTransaction succeeded\n");
    ok(ret == DDE_FNOTPROCESSED, "wrong status code %04x\n", ret);
    err = DdeGetLastError(dde_inst);
    ok(err == DMLERR_NOTPROCESSED, "wrong dde error %x\n", err);

    trace("sending ANSI client transaction command\n");
    ret = 0xdeadbeef;
    hdata = DdeClientTransaction((LPBYTE)exec_cmdA, lstrlenA(exec_cmdA) + 1, hconv, 0, 0, XTYP_EXECUTE, 1000, &ret);
    ok(hdata != 0, "DdeClientTransaction returned %p, error %x\n", hdata, DdeGetLastError(dde_inst));
    ok(ret == DDE_FACK, "wrong status code %04x\n", ret);

    err = DdeGetLastError(dde_inst);
    ok(err == DMLERR_NO_ERROR, "wrong dde error %x\n", err);

    trace("sending unicode client transaction command\n");
    ret = 0xdeadbeef;
    hdata = DdeClientTransaction((LPBYTE)exec_cmdW, (lstrlenW(exec_cmdW) + 1) * sizeof(WCHAR), hconv, 0, 0, XTYP_EXECUTE, 1000, &ret);
    ok(hdata != 0, "DdeClientTransaction returned %p, error %x\n", hdata, DdeGetLastError(dde_inst));
    ok(ret == DDE_FACK, "wrong status code %04x\n", ret);
    err = DdeGetLastError(dde_inst);
    ok(err == DMLERR_NO_ERROR, "wrong dde error %x\n", err);

    ok(DdeDisconnect(hconv), "DdeDisconnect error %x\n", DdeGetLastError(dde_inst));

    info.cb = sizeof(info);
    ret = DdeQueryConvInfo(hconv, QID_SYNC, &info);
    ok(!ret, "DdeQueryConvInfo should fail\n");
    err = DdeGetLastError(dde_inst);
todo_wine {
    ok(err == DMLERR_INVALIDPARAMETER, "wrong dde error %x\n", err);
}

    ok(DdeFreeStringHandle(dde_inst, hsz_server), "DdeFreeStringHandle error %x\n", DdeGetLastError(dde_inst));

    /* This call hangs on win2k SP4 and XP SP1.
    DdeUninitialize(dde_inst);*/

    DestroyWindow(hwnd_client);
    DestroyWindow(hwnd_server);
}

static void test_DdeCreateStringHandleW(DWORD dde_inst, int codepage)
{
    static const WCHAR dde_string[] = {'D','D','E',' ','S','t','r','i','n','g',0};
    HSZ str_handle;
    WCHAR bufW[256];
    char buf[256];
    ATOM atom;
    int ret;

    str_handle = DdeCreateStringHandleW(dde_inst, dde_string, codepage);
    ok(str_handle != 0, "DdeCreateStringHandleW failed with error %08x\n",
       DdeGetLastError(dde_inst));

    ret = DdeQueryStringW(dde_inst, str_handle, NULL, 0, codepage);
    if (codepage == CP_WINANSI)
        ok(ret == 1, "DdeQueryStringW returned wrong length %d\n", ret);
    else
        ok(ret == lstrlenW(dde_string), "DdeQueryStringW returned wrong length %d\n", ret);

    ret = DdeQueryStringW(dde_inst, str_handle, bufW, 256, codepage);
    if (codepage == CP_WINANSI)
    {
        ok(ret == 1, "DdeQueryStringW returned wrong length %d\n", ret);
        ok(!lstrcmpA("D", (LPCSTR)bufW), "DdeQueryStringW returned wrong string\n");
    }
    else
    {
        ok(ret == lstrlenW(dde_string), "DdeQueryStringW returned wrong length %d\n", ret);
        ok(!lstrcmpW(dde_string, bufW), "DdeQueryStringW returned wrong string\n");
    }

    ret = DdeQueryStringA(dde_inst, str_handle, buf, 256, CP_WINANSI);
    if (codepage == CP_WINANSI)
    {
        ok(ret == 1, "DdeQueryStringA returned wrong length %d\n", ret);
        ok(!lstrcmpA("D", buf), "DdeQueryStringW returned wrong string\n");
    }
    else
    {
        ok(ret == lstrlenA("DDE String"), "DdeQueryStringA returned wrong length %d\n", ret);
        ok(!lstrcmpA("DDE String", buf), "DdeQueryStringA returned wrong string %s\n", buf);
    }

    ret = DdeQueryStringA(dde_inst, str_handle, buf, 256, CP_WINUNICODE);
    if (codepage == CP_WINANSI)
    {
        ok(ret == 1, "DdeQueryStringA returned wrong length %d\n", ret);
        ok(!lstrcmpA("D", buf), "DdeQueryStringA returned wrong string %s\n", buf);
    }
    else
    {
        ok(ret == lstrlenA("DDE String"), "DdeQueryStringA returned wrong length %d\n", ret);
        ok(!lstrcmpW(dde_string, (LPCWSTR)buf), "DdeQueryStringW returned wrong string\n");
    }

    if (codepage == CP_WINANSI)
    {
        atom = FindAtomA((LPSTR)dde_string);
        ok(atom != 0, "Expected a valid atom\n");

        SetLastError(0xdeadbeef);
        atom = GlobalFindAtomA((LPSTR)dde_string);
        ok(atom == 0, "Expected 0, got %d\n", atom);
        ok(GetLastError() == ERROR_FILE_NOT_FOUND,
           "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    }
    else
    {
        atom = FindAtomW(dde_string);
        ok(atom != 0, "Expected a valid atom\n");

        SetLastError(0xdeadbeef);
        atom = GlobalFindAtomW(dde_string);
        ok(atom == 0, "Expected 0, got %d\n", atom);
        ok(GetLastError() == ERROR_FILE_NOT_FOUND,
           "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    }

    ok(DdeFreeStringHandle(dde_inst, str_handle), "DdeFreeStringHandle failed\n");
}

static void test_DdeCreateStringHandle(void)
{
    DWORD dde_inst, ret;

    dde_inst = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = DdeInitializeW(&dde_inst, client_ddeml_callback, APPCMD_CLIENTONLY, 0);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        trace("Skipping the DDE test on a Win9x platform\n");
        return;
    }

    ok(ret == DMLERR_INVALIDPARAMETER, "DdeInitializeW should fail, but got %04x instead\n", ret);
    ok(DdeGetLastError(dde_inst) == DMLERR_INVALIDPARAMETER, "expected DMLERR_INVALIDPARAMETER\n");

    dde_inst = 0;
    ret = DdeInitializeW(&dde_inst, client_ddeml_callback, APPCMD_CLIENTONLY, 0);
    ok(ret == DMLERR_NO_ERROR, "DdeInitializeW failed with error %04x (%08x)\n",
       ret, DdeGetLastError(dde_inst));

    test_DdeCreateStringHandleW(dde_inst, 0);
    test_DdeCreateStringHandleW(dde_inst, CP_WINUNICODE);
    test_DdeCreateStringHandleW(dde_inst, CP_WINANSI);

    ok(DdeUninitialize(dde_inst), "DdeUninitialize failed\n");
}

static void test_FreeDDElParam(void)
{
    HGLOBAL val, hglobal;
    BOOL ret;

    ret = FreeDDElParam(WM_DDE_INITIATE, (LPARAM)NULL);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    hglobal = GlobalAlloc(GMEM_DDESHARE, 100);
    ret = FreeDDElParam(WM_DDE_INITIATE, (LPARAM)hglobal);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    val = GlobalFree(hglobal);
    ok(val == NULL, "Expected NULL, got %p\n", val);

    hglobal = GlobalAlloc(GMEM_DDESHARE, 100);
    ret = FreeDDElParam(WM_DDE_ADVISE, (LPARAM)hglobal);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    val = GlobalFree(hglobal);
    ok(val == hglobal, "Expected hglobal, got %p\n", val);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    hglobal = GlobalAlloc(GMEM_DDESHARE, 100);
    ret = FreeDDElParam(WM_DDE_UNADVISE, (LPARAM)hglobal);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    val = GlobalFree(hglobal);
    ok(val == NULL, "Expected NULL, got %p\n", val);

    hglobal = GlobalAlloc(GMEM_DDESHARE, 100);
    ret = FreeDDElParam(WM_DDE_ACK, (LPARAM)hglobal);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    val = GlobalFree(hglobal);
    ok(val == hglobal, "Expected hglobal, got %p\n", val);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    hglobal = GlobalAlloc(GMEM_DDESHARE, 100);
    ret = FreeDDElParam(WM_DDE_DATA, (LPARAM)hglobal);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    val = GlobalFree(hglobal);
    ok(val == hglobal, "Expected hglobal, got %p\n", val);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    hglobal = GlobalAlloc(GMEM_DDESHARE, 100);
    ret = FreeDDElParam(WM_DDE_REQUEST, (LPARAM)hglobal);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    val = GlobalFree(hglobal);
    ok(val == NULL, "Expected NULL, got %p\n", val);

    hglobal = GlobalAlloc(GMEM_DDESHARE, 100);
    ret = FreeDDElParam(WM_DDE_POKE, (LPARAM)hglobal);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    val = GlobalFree(hglobal);
    ok(val == hglobal, "Expected hglobal, got %p\n", val);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    hglobal = GlobalAlloc(GMEM_DDESHARE, 100);
    ret = FreeDDElParam(WM_DDE_EXECUTE, (LPARAM)hglobal);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    val = GlobalFree(hglobal);
    ok(val == NULL, "Expected NULL, got %p\n", val);
}

static void test_PackDDElParam(void)
{
    UINT_PTR lo, hi, *ptr;
    HGLOBAL hglobal;
    LPARAM lparam;
    BOOL ret;

    lparam = PackDDElParam(WM_DDE_INITIATE, 0xcafe, 0xbeef);
    ok(lparam == 0xbeefcafe, "Expected 0xbeefcafe, got %08lx\n", lparam);
    ok(GlobalLock((HGLOBAL)lparam) == NULL,
       "Expected NULL, got %p\n", GlobalLock((HGLOBAL)lparam));
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    lo = hi = 0;
    ret = UnpackDDElParam(WM_DDE_INITIATE, lparam, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xcafe, "Expected 0xcafe, got %08lx\n", lo);
    ok(hi == 0xbeef, "Expected 0xbeef, got %08lx\n", hi);

    ret = FreeDDElParam(WM_DDE_INITIATE, lparam);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    lparam = PackDDElParam(WM_DDE_TERMINATE, 0xcafe, 0xbeef);
    ok(lparam == 0xbeefcafe, "Expected 0xbeefcafe, got %08lx\n", lparam);
    ok(GlobalLock((HGLOBAL)lparam) == NULL,
       "Expected NULL, got %p\n", GlobalLock((HGLOBAL)lparam));
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    lo = hi = 0;
    ret = UnpackDDElParam(WM_DDE_TERMINATE, lparam, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xcafe, "Expected 0xcafe, got %08lx\n", lo);
    ok(hi == 0xbeef, "Expected 0xbeef, got %08lx\n", hi);

    ret = FreeDDElParam(WM_DDE_TERMINATE, lparam);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    lparam = PackDDElParam(WM_DDE_ADVISE, 0xcafe, 0xbeef);
    ptr = GlobalLock((HGLOBAL)lparam);
    ok(ptr != NULL, "Expected non-NULL ptr\n");
    ok(ptr[0] == 0xcafe, "Expected 0xcafe, got %08lx\n", ptr[0]);
    ok(ptr[1] == 0xbeef, "Expected 0xbeef, got %08lx\n", ptr[1]);

    ret = GlobalUnlock((HGLOBAL)lparam);
    ok(ret == 1, "Expected 1, got %d\n", ret);

    lo = hi = 0;
    ret = UnpackDDElParam(WM_DDE_ADVISE, lparam, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xcafe, "Expected 0xcafe, got %08lx\n", lo);
    ok(hi == 0xbeef, "Expected 0xbeef, got %08lx\n", hi);

    ret = FreeDDElParam(WM_DDE_ADVISE, lparam);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    hglobal = GlobalFree((HGLOBAL)lparam);
    ok(hglobal == (HGLOBAL)lparam, "Expected lparam, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    lparam = PackDDElParam(WM_DDE_UNADVISE, 0xcafe, 0xbeef);
    ok(lparam == 0xbeefcafe, "Expected 0xbeefcafe, got %08lx\n", lparam);
    ok(GlobalLock((HGLOBAL)lparam) == NULL,
       "Expected NULL, got %p\n", GlobalLock((HGLOBAL)lparam));
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    lo = hi = 0;
    ret = UnpackDDElParam(WM_DDE_UNADVISE, lparam, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xcafe, "Expected 0xcafe, got %08lx\n", lo);
    ok(hi == 0xbeef, "Expected 0xbeef, got %08lx\n", hi);

    ret = FreeDDElParam(WM_DDE_UNADVISE, lparam);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    lparam = PackDDElParam(WM_DDE_ACK, 0xcafe, 0xbeef);
    ptr = GlobalLock((HGLOBAL)lparam);
    ok(ptr != NULL, "Expected non-NULL ptr\n");
    ok(ptr[0] == 0xcafe, "Expected 0xcafe, got %08lx\n", ptr[0]);
    ok(ptr[1] == 0xbeef, "Expected 0xbeef, got %08lx\n", ptr[1]);

    ret = GlobalUnlock((HGLOBAL)lparam);
    ok(ret == 1, "Expected 1, got %d\n", ret);

    lo = hi = 0;
    ret = UnpackDDElParam(WM_DDE_ACK, lparam, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xcafe, "Expected 0xcafe, got %08lx\n", lo);
    ok(hi == 0xbeef, "Expected 0xbeef, got %08lx\n", hi);

    ret = FreeDDElParam(WM_DDE_ACK, lparam);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    hglobal = GlobalFree((HGLOBAL)lparam);
    ok(hglobal == (HGLOBAL)lparam, "Expected lparam, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    lparam = PackDDElParam(WM_DDE_DATA, 0xcafe, 0xbeef);
    ptr = GlobalLock((HGLOBAL)lparam);
    ok(ptr != NULL, "Expected non-NULL ptr\n");
    ok(ptr[0] == 0xcafe, "Expected 0xcafe, got %08lx\n", ptr[0]);
    ok(ptr[1] == 0xbeef, "Expected 0xbeef, got %08lx\n", ptr[1]);

    ret = GlobalUnlock((HGLOBAL)lparam);
    ok(ret == 1, "Expected 1, got %d\n", ret);

    lo = hi = 0;
    ret = UnpackDDElParam(WM_DDE_DATA, lparam, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xcafe, "Expected 0xcafe, got %08lx\n", lo);
    ok(hi == 0xbeef, "Expected 0xbeef, got %08lx\n", hi);

    ret = FreeDDElParam(WM_DDE_DATA, lparam);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    hglobal = GlobalFree((HGLOBAL)lparam);
    ok(hglobal == (HGLOBAL)lparam, "Expected lparam, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    lparam = PackDDElParam(WM_DDE_REQUEST, 0xcafe, 0xbeef);
    ok(lparam == 0xbeefcafe, "Expected 0xbeefcafe, got %08lx\n", lparam);
    ok(GlobalLock((HGLOBAL)lparam) == NULL,
       "Expected NULL, got %p\n", GlobalLock((HGLOBAL)lparam));
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    lo = hi = 0;
    ret = UnpackDDElParam(WM_DDE_REQUEST, lparam, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xcafe, "Expected 0xcafe, got %08lx\n", lo);
    ok(hi == 0xbeef, "Expected 0xbeef, got %08lx\n", hi);

    ret = FreeDDElParam(WM_DDE_REQUEST, lparam);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    lparam = PackDDElParam(WM_DDE_POKE, 0xcafe, 0xbeef);
    ptr = GlobalLock((HGLOBAL)lparam);
    ok(ptr != NULL, "Expected non-NULL ptr\n");
    ok(ptr[0] == 0xcafe, "Expected 0xcafe, got %08lx\n", ptr[0]);
    ok(ptr[1] == 0xbeef, "Expected 0xbeef, got %08lx\n", ptr[1]);

    ret = GlobalUnlock((HGLOBAL)lparam);
    ok(ret == 1, "Expected 1, got %d\n", ret);

    lo = hi = 0;
    ret = UnpackDDElParam(WM_DDE_POKE, lparam, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xcafe, "Expected 0xcafe, got %08lx\n", lo);
    ok(hi == 0xbeef, "Expected 0xbeef, got %08lx\n", hi);

    ret = FreeDDElParam(WM_DDE_POKE, lparam);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    hglobal = GlobalFree((HGLOBAL)lparam);
    ok(hglobal == (HGLOBAL)lparam, "Expected lparam, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    lparam = PackDDElParam(WM_DDE_EXECUTE, 0xcafe, 0xbeef);
    ok(lparam == 0xbeef, "Expected 0xbeef, got %08lx\n", lparam);
    ok(GlobalLock((HGLOBAL)lparam) == NULL,
       "Expected NULL, got %p\n", GlobalLock((HGLOBAL)lparam));
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    lo = hi = 0;
    ret = UnpackDDElParam(WM_DDE_EXECUTE, lparam, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0, "Expected 0, got %08lx\n", lo);
    ok(hi == 0xbeef, "Expected 0xbeef, got %08lx\n", hi);

    ret = FreeDDElParam(WM_DDE_EXECUTE, lparam);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
}

static void test_UnpackDDElParam(void)
{
    UINT_PTR lo, hi, *ptr;
    HGLOBAL hglobal;
    BOOL ret;

    /* NULL lParam */
    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_INITIATE, (LPARAM)NULL, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0, "Expected 0, got %08lx\n", lo);
    ok(hi == 0, "Expected 0, got %08lx\n", hi);

    /* NULL lo */
    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_INITIATE, 0xcafebabe, NULL, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xdead, "Expected 0xdead, got %08lx\n", lo);
    ok(hi == 0xcafe, "Expected 0xcafe, got %08lx\n", hi);

    /* NULL hi */
    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_INITIATE, 0xcafebabe, &lo, NULL);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xbabe, "Expected 0xbabe, got %08lx\n", lo);
    ok(hi == 0xbeef, "Expected 0xbeef, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_INITIATE, 0xcafebabe, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xbabe, "Expected 0xbabe, got %08lx\n", lo);
    ok(hi == 0xcafe, "Expected 0xcafe, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_TERMINATE, 0xcafebabe, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xbabe, "Expected 0xbabe, got %08lx\n", lo);
    ok(hi == 0xcafe, "Expected 0xcafe, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_ADVISE, (LPARAM)NULL, &lo, &hi);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(lo == 0, "Expected 0, got %08lx\n", lo);
    ok(hi == 0, "Expected 0, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_ADVISE, 0xcafebabe, &lo, &hi);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(lo == 0, "Expected 0, got %08lx\n", lo);
    ok(hi == 0, "Expected 0, got %08lx\n", hi);

    hglobal = GlobalAlloc(GMEM_DDESHARE, 2);
    ptr = GlobalLock(hglobal);
    ptr[0] = 0xcafebabe;
    ptr[1] = 0xdeadbeef;
    GlobalUnlock(hglobal);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_ADVISE, (LPARAM)hglobal, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xcafebabe, "Expected 0xcafebabe, got %08lx\n", lo);
    ok(hi == 0xdeadbeef, "Expected 0xdeadbeef, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_UNADVISE, 0xcafebabe, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xbabe, "Expected 0xbabe, got %08lx\n", lo);
    ok(hi == 0xcafe, "Expected 0xcafe, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_ACK, 0xcafebabe, &lo, &hi);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(lo == 0, "Expected 0, got %08lx\n", lo);
    ok(hi == 0, "Expected 0, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_ACK, (LPARAM)hglobal, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xcafebabe, "Expected 0xcafebabe, got %08lx\n", lo);
    ok(hi == 0xdeadbeef, "Expected 0xdeadbeef, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_DATA, 0xcafebabe, &lo, &hi);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(lo == 0, "Expected 0, got %08lx\n", lo);
    ok(hi == 0, "Expected 0, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_DATA, (LPARAM)hglobal, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xcafebabe, "Expected 0xcafebabe, got %08lx\n", lo);
    ok(hi == 0xdeadbeef, "Expected 0xdeadbeef, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_REQUEST, 0xcafebabe, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xbabe, "Expected 0xbabe, got %08lx\n", lo);
    ok(hi == 0xcafe, "Expected 0xcafe, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_POKE, 0xcafebabe, &lo, &hi);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(lo == 0, "Expected 0, got %08lx\n", lo);
    ok(hi == 0, "Expected 0, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_POKE, (LPARAM)hglobal, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xcafebabe, "Expected 0xcafebabe, got %08lx\n", lo);
    ok(hi == 0xdeadbeef, "Expected 0xdeadbeef, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_EXECUTE, 0xcafebabe, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0, "Expected 0, got %08lx\n", lo);
    ok(hi == 0xcafebabe, "Expected 0xcafebabe, got %08lx\n", hi);
}

START_TEST(dde)
{
    int argc;
    char **argv;
    char buffer[MAX_PATH];
    STARTUPINFO startup;
    PROCESS_INFORMATION proc;

    argc = winetest_get_mainargs(&argv);
    if (argc == 3)
    {
        if (!lstrcmpA(argv[2], "ddeml"))
            test_ddeml_client();

        return;
    }

    ZeroMemory(&startup, sizeof(STARTUPINFO));
    sprintf(buffer, "%s dde ddeml", argv[0]);
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    CreateProcessA(NULL, buffer, NULL, NULL, FALSE,
                   0, NULL, NULL, &startup, &proc);

    test_msg_server(proc.hProcess);

    test_dde_aw_transaction();

    test_DdeCreateStringHandle();
    test_FreeDDElParam();
    test_PackDDElParam();
    test_UnpackDDElParam();
}
