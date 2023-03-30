/*
 * Unit tests for imm32
 *
 * Copyright (c) 2008 Michael Jung
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
#include <stddef.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"

#include "wine/test.h"
#include "objbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "imm.h"
#include "immdev.h"

#include "ime_test.h"

static const char *debugstr_ok( const char *cond )
{
    int c, n = 0;
    /* skip possible casts */
    while ((c = *cond++))
    {
        if (c == '(') n++;
        if (!n) break;
        if (c == ')') n--;
    }
    if (!strchr( cond - 1, '(' )) return wine_dbg_sprintf( "got %s", cond - 1 );
    return wine_dbg_sprintf( "%.*s returned", (int)strcspn( cond - 1, "( " ), cond - 1 );
}

#define ok_eq( e, r, t, f, ... )                                                                   \
    do                                                                                             \
    {                                                                                              \
        t v = (r);                                                                                 \
        ok( v == (e), "%s " f "\n", debugstr_ok( #r ), v, ##__VA_ARGS__ );                         \
    } while (0)
#define ok_ne( e, r, t, f, ... )                                                                   \
    do                                                                                             \
    {                                                                                              \
        t v = (r);                                                                                 \
        ok( v != (e), "%s " f "\n", debugstr_ok( #r ), v, ##__VA_ARGS__ );                         \
    } while (0)
#define ok_wcs( e, r )                                                                             \
    do                                                                                             \
    {                                                                                              \
        const WCHAR *v = (r);                                                                      \
        ok( !wcscmp( v, (e) ), "%s %s\n", debugstr_ok(#r), debugstr_w(v) );                        \
    } while (0)
#define ok_str( e, r )                                                                             \
    do                                                                                             \
    {                                                                                              \
        const char *v = (r);                                                                       \
        ok( !strcmp( v, (e) ), "%s %s\n", debugstr_ok(#r), debugstr_a(v) );                        \
    } while (0)
#define ok_ret( e, r ) ok_eq( e, r, UINT_PTR, "%Iu, error %ld", GetLastError() )

BOOL WINAPI ImmSetActiveContext(HWND, HIMC, BOOL);

static BOOL (WINAPI *pImmAssociateContextEx)(HWND,HIMC,DWORD);
static UINT (WINAPI *pNtUserAssociateInputContext)(HWND,HIMC,ULONG);
static BOOL (WINAPI *pImmIsUIMessageA)(HWND,UINT,WPARAM,LPARAM);
static UINT (WINAPI *pSendInput) (UINT, INPUT*, size_t);

extern BOOL WINAPI ImmFreeLayout(HKL);
extern BOOL WINAPI ImmLoadIME(HKL);
extern BOOL WINAPI ImmActivateLayout(HKL);

#define check_member_( file, line, val, exp, fmt, member )                                         \
    ok_(file, line)( (val).member == (exp).member, "got " #member " " fmt "\n", (val).member )
#define check_member( val, exp, fmt, member )                                                      \
    check_member_( __FILE__, __LINE__, val, exp, fmt, member )

#define check_member_point_( file, line, val, exp, member )                                        \
    ok_(file, line)( !memcmp( &(val).member, &(exp).member, sizeof(POINT) ),                       \
                     "got " #member " %s\n", wine_dbgstr_point( &(val).member ) )
#define check_member_point( val, exp, member )                                                     \
    check_member_point_( __FILE__, __LINE__, val, exp, member )

#define check_member_rect_( file, line, val, exp, member )                                         \
    ok_(file, line)( !memcmp( &(val).member, &(exp).member, sizeof(RECT) ),                        \
                     "got " #member " %s\n", wine_dbgstr_rect( &(val).member ) )
#define check_member_rect( val, exp, member )                                                      \
    check_member_rect_( __FILE__, __LINE__, val, exp, member )

#define check_candidate_list( a, b ) check_candidate_list_( __LINE__, a, b, TRUE )
static void check_candidate_list_( int line, CANDIDATELIST *list, const CANDIDATELIST *expect, BOOL unicode )
{
    UINT i;

    check_member_( __FILE__, line, *list, *expect, "%lu", dwSize );
    check_member_( __FILE__, line, *list, *expect, "%lu", dwStyle );
    check_member_( __FILE__, line, *list, *expect, "%lu", dwCount );
    check_member_( __FILE__, line, *list, *expect, "%lu", dwSelection );
    check_member_( __FILE__, line, *list, *expect, "%lu", dwPageStart );
    check_member_( __FILE__, line, *list, *expect, "%lu", dwPageSize );
    for (i = 0; i < list->dwCount && i < expect->dwCount; ++i)
    {
        void *list_str = (BYTE *)list + list->dwOffset[i], *expect_str = (BYTE *)expect + expect->dwOffset[i];
        check_member_( __FILE__, line, *list, *expect, "%lu", dwOffset[i] );
        if (unicode) ok_( __FILE__, line )( !wcscmp( list_str, expect_str ), "got %s\n", debugstr_w(list_str) );
        else ok_( __FILE__, line )( !strcmp( list_str, expect_str ), "got %s\n", debugstr_a(list_str) );
    }
}

#define check_candidate_form( a, b ) check_candidate_form_( __LINE__, a, b )
static void check_candidate_form_( int line, CANDIDATEFORM *form, const CANDIDATEFORM *expect )
{
    check_member_( __FILE__, line, *form, *expect, "%#lx", dwIndex );
    check_member_( __FILE__, line, *form, *expect, "%#lx", dwStyle );
    check_member_point_( __FILE__, line, *form, *expect, ptCurrentPos );
    check_member_rect_( __FILE__, line, *form, *expect, rcArea );
}

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE, enabled_ ## func = FALSE

#define SET_EXPECT(func) \
        expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        if (enabled_ ## func) {\
            ok(expect_ ##func, "unexpected call " #func "\n"); \
            called_ ## func = TRUE; \
        } \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define SET_ENABLE(func, val) \
    enabled_ ## func = (val)

DEFINE_EXPECT(WM_IME_SETCONTEXT_DEACTIVATE);
DEFINE_EXPECT(WM_IME_SETCONTEXT_ACTIVATE);

static void process_messages(void)
{
    MSG msg;

    while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE ))
    {
        TranslateMessage( &msg );
        DispatchMessageA( &msg );
    }
}

/*
 * msgspy - record and analyse message traces sent to a certain window
 */
typedef struct _msgs {
    CWPSTRUCT    msg;
    BOOL         post;
} imm_msgs;

static struct _msg_spy {
    HWND         hwnd;
    HHOOK        get_msg_hook;
    HHOOK        call_wnd_proc_hook;
    imm_msgs     msgs[64];
    unsigned int i_msg;
} msg_spy;

typedef struct
{
    DWORD type;
    union
    {
        MOUSEINPUT      mi;
        KEYBDINPUT      ki;
        HARDWAREINPUT   hi;
    } u;
} TEST_INPUT;

static UINT (WINAPI *pSendInput) (UINT, INPUT*, size_t);

static LRESULT CALLBACK get_msg_filter(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (HC_ACTION == nCode) {
        MSG *msg = (MSG*)lParam;

        if ((msg->hwnd == msg_spy.hwnd || msg_spy.hwnd == NULL) &&
            (msg_spy.i_msg < ARRAY_SIZE(msg_spy.msgs)))
        {
            msg_spy.msgs[msg_spy.i_msg].msg.hwnd    = msg->hwnd;
            msg_spy.msgs[msg_spy.i_msg].msg.message = msg->message;
            msg_spy.msgs[msg_spy.i_msg].msg.wParam  = msg->wParam;
            msg_spy.msgs[msg_spy.i_msg].msg.lParam  = msg->lParam;
            msg_spy.msgs[msg_spy.i_msg].post = TRUE;
            msg_spy.i_msg++;
        }
    }

    return CallNextHookEx(msg_spy.get_msg_hook, nCode, wParam, lParam);
}

static LRESULT CALLBACK call_wnd_proc_filter(int nCode, WPARAM wParam,
                                             LPARAM lParam)
{
    if (HC_ACTION == nCode) {
        CWPSTRUCT *cwp = (CWPSTRUCT*)lParam;

        if (((cwp->hwnd == msg_spy.hwnd || msg_spy.hwnd == NULL)) &&
            (msg_spy.i_msg < ARRAY_SIZE(msg_spy.msgs)))
        {
            memcpy(&msg_spy.msgs[msg_spy.i_msg].msg, cwp, sizeof(msg_spy.msgs[0].msg));
            msg_spy.msgs[msg_spy.i_msg].post = FALSE;
            msg_spy.i_msg++;
        }
    }

    return CallNextHookEx(msg_spy.call_wnd_proc_hook, nCode, wParam, lParam);
}

static void msg_spy_pump_msg_queue(void) {
    MSG msg;

    while(PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return;
}

static void msg_spy_flush_msgs(void) {
    msg_spy_pump_msg_queue();
    msg_spy.i_msg = 0;
}

static imm_msgs* msg_spy_find_next_msg(UINT message, UINT *start) {
    UINT i;

    msg_spy_pump_msg_queue();

    if (msg_spy.i_msg >= ARRAY_SIZE(msg_spy.msgs))
        fprintf(stdout, "%s:%d: msg_spy: message buffer overflow!\n",
                __FILE__, __LINE__);

    for (i = *start; i < msg_spy.i_msg; i++)
        if (msg_spy.msgs[i].msg.message == message)
        {
            *start = i+1;
            return &msg_spy.msgs[i];
        }

    return NULL;
}

static imm_msgs* msg_spy_find_msg(UINT message) {
    UINT i = 0;

    return msg_spy_find_next_msg(message, &i);
}

static void msg_spy_init(HWND hwnd) {
    msg_spy.hwnd = hwnd;
    msg_spy.get_msg_hook =
            SetWindowsHookExW(WH_GETMESSAGE, get_msg_filter, GetModuleHandleW(NULL),
                              GetCurrentThreadId());
    msg_spy.call_wnd_proc_hook =
            SetWindowsHookExW(WH_CALLWNDPROC, call_wnd_proc_filter,
                              GetModuleHandleW(NULL), GetCurrentThreadId());
    msg_spy.i_msg = 0;

    msg_spy_flush_msgs();
}

static void msg_spy_cleanup(void) {
    if (msg_spy.get_msg_hook)
        UnhookWindowsHookEx(msg_spy.get_msg_hook);
    if (msg_spy.call_wnd_proc_hook)
        UnhookWindowsHookEx(msg_spy.call_wnd_proc_hook);
    memset(&msg_spy, 0, sizeof(msg_spy));
}

/*
 * imm32 test cases - Issue some IMM commands on a dummy window and analyse the
 * messages being sent to this window in response.
 */
static const char wndcls[] = "winetest_imm32_wndcls";
static enum { PHASE_UNKNOWN, FIRST_WINDOW, SECOND_WINDOW,
              CREATE_CANCEL, NCCREATE_CANCEL, IME_DISABLED } test_phase;
static HWND hwnd, child;

static HWND get_ime_window(void);

static void load_resource( const WCHAR *name, WCHAR *filename )
{
    static WCHAR path[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathW( ARRAY_SIZE(path), path );
    GetTempFileNameW( path, name, 0, filename );

    file = CreateFileW( filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "failed to create %s, error %lu\n", debugstr_w(filename), GetLastError() );

    res = FindResourceW( NULL, name, L"TESTDLL" );
    ok( res != 0, "couldn't find resource\n" );
    ptr = LockResource( LoadResource( GetModuleHandleW( NULL ), res ) );
    WriteFile( file, ptr, SizeofResource( GetModuleHandleW( NULL ), res ), &written, NULL );
    ok( written == SizeofResource( GetModuleHandleW( NULL ), res ), "couldn't write resource\n" );
    CloseHandle( file );
}

static LRESULT WINAPI wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND default_ime_wnd;
    switch (msg)
    {
        case WM_IME_SETCONTEXT:
            if (wParam) CHECK_EXPECT(WM_IME_SETCONTEXT_ACTIVATE);
            else CHECK_EXPECT(WM_IME_SETCONTEXT_DEACTIVATE);
            ok(lParam == ISC_SHOWUIALL || !lParam, "lParam = %Ix\n", lParam);
            return TRUE;
        case WM_NCCREATE:
            default_ime_wnd = get_ime_window();
            switch(test_phase) {
                case FIRST_WINDOW:
                case IME_DISABLED:
                    ok(!default_ime_wnd, "expected no IME windows\n");
                    break;
                case SECOND_WINDOW:
                    ok(default_ime_wnd != NULL, "expected IME window existence\n");
                    break;
                default:
                    break; /* do nothing */
            }
            if (test_phase == NCCREATE_CANCEL)
                return FALSE;
            return TRUE;
        case WM_NCCALCSIZE:
            default_ime_wnd = get_ime_window();
            switch(test_phase) {
                case FIRST_WINDOW:
                case SECOND_WINDOW:
                case CREATE_CANCEL:
                    ok(default_ime_wnd != NULL, "expected IME window existence\n");
                    break;
                case IME_DISABLED:
                    ok(!default_ime_wnd, "expected no IME windows\n");
                    break;
                default:
                    break; /* do nothing */
            }
            break;
        case WM_CREATE:
            default_ime_wnd = get_ime_window();
            switch(test_phase) {
                case FIRST_WINDOW:
                case SECOND_WINDOW:
                case CREATE_CANCEL:
                    ok(default_ime_wnd != NULL, "expected IME window existence\n");
                    break;
                case IME_DISABLED:
                    ok(!default_ime_wnd, "expected no IME windows\n");
                    break;
                default:
                    break; /* do nothing */
            }
            if (test_phase == CREATE_CANCEL)
                return -1;
            return TRUE;
    }

    return DefWindowProcA(hWnd,msg,wParam,lParam);
}

static BOOL is_ime_enabled(void)
{
    HIMC himc;
    HWND wnd;

    wnd = CreateWindowA("static", "static", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ok(wnd != NULL, "CreateWindow failed\n");

    himc = ImmGetContext(wnd);
    if (!himc)
    {
        DestroyWindow(wnd);
        return FALSE;
    }

    ImmReleaseContext(wnd, himc);
    DestroyWindow(wnd);
    return TRUE;
}

static BOOL init(void) {
    WNDCLASSEXA wc;
    HMODULE hmod,huser;

    hmod = GetModuleHandleA("imm32.dll");
    huser = GetModuleHandleA("user32");
    pImmAssociateContextEx = (void*)GetProcAddress(hmod, "ImmAssociateContextEx");
    pImmIsUIMessageA = (void*)GetProcAddress(hmod, "ImmIsUIMessageA");
    pSendInput = (void*)GetProcAddress(huser, "SendInput");
    pNtUserAssociateInputContext = (void*)GetProcAddress(GetModuleHandleW(L"win32u.dll"),
                                                         "NtUserAssociateInputContext");

    wc.cbSize        = sizeof(WNDCLASSEXA);
    wc.style         = 0;
    wc.lpfnWndProc   = wndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = GetModuleHandleA(NULL);
    wc.hIcon         = LoadIconA(NULL, (LPCSTR)IDI_APPLICATION);
    wc.hCursor       = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = wndcls;
    wc.hIconSm       = LoadIconA(NULL, (LPCSTR)IDI_APPLICATION);

    if (!RegisterClassExA(&wc))
        return FALSE;

    hwnd = CreateWindowExA(WS_EX_CLIENTEDGE, wndcls, "Wine imm32.dll test",
                           WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                           240, 120, NULL, NULL, GetModuleHandleW(NULL), NULL);
    if (!hwnd)
        return FALSE;

    child = CreateWindowA("edit", "edit", WS_CHILD | WS_VISIBLE, 0, 0, 50, 50, hwnd, 0, 0, 0);
    if (!child)
        return FALSE;

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    msg_spy_init(hwnd);

    return TRUE;
}

static void cleanup(void) {
    msg_spy_cleanup();
    if (hwnd)
        DestroyWindow(hwnd);
    UnregisterClassA(wndcls, GetModuleHandleW(NULL));
}

static void test_ImmNotifyIME(void) {
    static const char string[] = "wine";
    char resstr[16] = "";
    HIMC imc;
    BOOL ret;

    imc = ImmGetContext(hwnd);
    msg_spy_flush_msgs();

    ret = ImmNotifyIME(imc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
    ok(broken(!ret) ||
       ret, /* Vista+ */
       "Canceling an empty composition string should succeed.\n");
    ok(!msg_spy_find_msg(WM_IME_COMPOSITION), "Windows does not post "
       "WM_IME_COMPOSITION in response to NI_COMPOSITIONSTR / CPS_CANCEL, if "
       "the composition string being canceled is empty.\n");

    ImmSetCompositionStringA(imc, SCS_SETSTR, string, sizeof(string), NULL, 0);
    msg_spy_flush_msgs();

    ImmNotifyIME(imc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
    msg_spy_flush_msgs();

    /* behavior differs between win9x and NT */
    ret = ImmGetCompositionStringA(imc, GCS_COMPSTR, resstr, sizeof(resstr));
    ok(!ret, "After being cancelled the composition string is empty.\n");

    msg_spy_flush_msgs();

    ret = ImmNotifyIME(imc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
    ok(broken(!ret) ||
       ret, /* Vista+ */
       "Canceling an empty composition string should succeed.\n");
    ok(!msg_spy_find_msg(WM_IME_COMPOSITION), "Windows does not post "
       "WM_IME_COMPOSITION in response to NI_COMPOSITIONSTR / CPS_CANCEL, if "
       "the composition string being canceled is empty.\n");

    msg_spy_flush_msgs();
    ImmReleaseContext(hwnd, imc);

    imc = ImmCreateContext();
    ImmDestroyContext(imc);

    SetLastError(0xdeadbeef);
    ret = ImmNotifyIME((HIMC)0xdeadcafe, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
    ok (ret == 0, "Bad IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08x!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmNotifyIME(0x00000000, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
    ok (ret == 0, "NULL IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_SUCCESS, "wrong last error %08x!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmNotifyIME(imc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
    ok (ret == 0, "Destroyed IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08x!\n", ret);

}

static struct {
    WNDPROC old_wnd_proc;
    BOOL catch_result_str;
    BOOL catch_ime_char;
    DWORD start;
    DWORD timer_id;
} ime_composition_test;

static LRESULT WINAPI test_ime_wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_IME_COMPOSITION:
        if ((lParam & GCS_RESULTSTR) && !ime_composition_test.catch_result_str) {
            HWND hwndIme;
            WCHAR wstring[20];
            HIMC imc;
            LONG size;
            LRESULT ret;

            hwndIme = ImmGetDefaultIMEWnd(hWnd);
            ok(hwndIme != NULL, "expected IME window existence\n");

            ok(!ime_composition_test.catch_ime_char, "WM_IME_CHAR is sent\n");
            ret = CallWindowProcA(ime_composition_test.old_wnd_proc,
                                  hWnd, msg, wParam, lParam);
            ok(ime_composition_test.catch_ime_char, "WM_IME_CHAR isn't sent\n");

            ime_composition_test.catch_ime_char = FALSE;
            SendMessageA(hwndIme, msg, wParam, lParam);
            ok(!ime_composition_test.catch_ime_char, "WM_IME_CHAR is sent\n");

            imc = ImmGetContext(hWnd);
            size = ImmGetCompositionStringW(imc, GCS_RESULTSTR,
                                            wstring, sizeof(wstring));
            ok(size > 0, "ImmGetCompositionString(GCS_RESULTSTR) is %ld\n", size);
            ImmReleaseContext(hwnd, imc);

            ime_composition_test.catch_result_str = TRUE;
            return ret;
        }
        break;
    case WM_IME_CHAR:
        if (!ime_composition_test.catch_result_str)
            ime_composition_test.catch_ime_char = TRUE;
        break;
    case WM_TIMER:
        if (wParam == ime_composition_test.timer_id) {
            HWND parent = GetParent(hWnd);
            char title[64];
            int left = 20 - (GetTickCount() - ime_composition_test.start) / 1000;
            wsprintfA(title, "%sLeft %d sec. - IME composition test",
                      ime_composition_test.catch_result_str ? "[*] " : "", left);
            SetWindowTextA(parent, title);
            if (left <= 0)
                DestroyWindow(parent);
            else
                SetTimer(hWnd, wParam, 100, NULL);
            return TRUE;
        }
        break;
    }
    return CallWindowProcA(ime_composition_test.old_wnd_proc,
                           hWnd, msg, wParam, lParam);
}

static void test_ImmGetCompositionString(void)
{
    HIMC imc;
    static const WCHAR string[] = {'w','i','n','e',0x65e5,0x672c,0x8a9e};
    char cstring[20];
    WCHAR wstring[20];
    LONG len;
    LONG alen,wlen;
    BOOL ret;
    DWORD prop;

    imc = ImmGetContext(hwnd);
    ret = ImmSetCompositionStringW(imc, SCS_SETSTR, string, sizeof(string), NULL,0);
    if (!ret) {
        win_skip("Composition isn't supported\n");
        ImmReleaseContext(hwnd, imc);
        return;
    }
    msg_spy_flush_msgs();

    alen = ImmGetCompositionStringA(imc, GCS_COMPSTR, cstring, 20);
    wlen = ImmGetCompositionStringW(imc, GCS_COMPSTR, wstring, 20);
    /* windows machines without any IME installed just return 0 above */
    if( alen && wlen)
    {
        len = ImmGetCompositionStringW(imc, GCS_COMPATTR, NULL, 0);
        ok(len*sizeof(WCHAR)==wlen,"GCS_COMPATTR(W) not returning correct count\n");
        len = ImmGetCompositionStringA(imc, GCS_COMPATTR, NULL, 0);
        ok(len==alen,"GCS_COMPATTR(A) not returning correct count\n");

        /* Get strings with exactly matching buffer sizes. */
        memset(wstring, 0x1a, sizeof(wstring));
        memset(cstring, 0x1a, sizeof(cstring));

        len = ImmGetCompositionStringA(imc, GCS_COMPSTR, cstring, alen);
        ok(len == alen, "Unexpected length %ld.\n", len);
        ok(cstring[alen] == 0x1a, "Unexpected buffer contents.\n");

        len = ImmGetCompositionStringW(imc, GCS_COMPSTR, wstring, wlen);
        ok(len == wlen, "Unexpected length %ld.\n", len);
        ok(wstring[wlen/sizeof(WCHAR)] == 0x1a1a, "Unexpected buffer contents.\n");

        /* Get strings with exactly smaller buffer sizes. */
        memset(wstring, 0x1a, sizeof(wstring));
        memset(cstring, 0x1a, sizeof(cstring));

        /* Returns 0 but still fills buffer. */
        len = ImmGetCompositionStringA(imc, GCS_COMPSTR, cstring, alen - 1);
        ok(!len, "Unexpected length %ld.\n", len);
        ok(cstring[0] == 'w', "Unexpected buffer contents %s.\n", cstring);

        len = ImmGetCompositionStringW(imc, GCS_COMPSTR, wstring, wlen - 1);
        ok(len == wlen - 1, "Unexpected length %ld.\n", len);
        ok(!memcmp(wstring, string, wlen - 1), "Unexpected buffer contents.\n");

        /* Get the size of the required output buffer. */
        memset(wstring, 0x1a, sizeof(wstring));
        memset(cstring, 0x1a, sizeof(cstring));

        len = ImmGetCompositionStringA(imc, GCS_COMPSTR, cstring, 0);
        ok(len == alen, "Unexpected length %ld.\n", len);
        ok(cstring[0] == 0x1a, "Unexpected buffer contents %s.\n", cstring);

        len = ImmGetCompositionStringW(imc, GCS_COMPSTR, wstring, 0);
        ok(len == wlen, "Unexpected length %ld.\n", len);
        ok(wstring[0] == 0x1a1a, "Unexpected buffer contents.\n");
    }
    else
        win_skip("Composition string isn't available\n");

    ImmReleaseContext(hwnd, imc);

    /* Test composition results input by IMM API */
    prop = ImmGetProperty(GetKeyboardLayout(0), IGP_SETCOMPSTR);
    if (!(prop & SCS_CAP_COMPSTR)) {
        /* Wine's IME doesn't support SCS_SETSTR in ImmSetCompositionString */
        skip("This IME doesn't support SCS_SETSTR\n");
    }
    else {
        ime_composition_test.old_wnd_proc =
            (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC,
                                       (LONG_PTR)test_ime_wnd_proc);
        imc = ImmGetContext(hwnd);
        msg_spy_flush_msgs();

        ret = ImmSetCompositionStringW(imc, SCS_SETSTR,
                                       string, sizeof(string), NULL,0);
        ok(ret, "ImmSetCompositionStringW failed\n");
        wlen = ImmGetCompositionStringW(imc, GCS_COMPSTR,
                                        wstring, sizeof(wstring));
        if (wlen > 0) {
            ret = ImmNotifyIME(imc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
            ok(ret, "ImmNotifyIME(CPS_COMPLETE) failed\n");
            msg_spy_flush_msgs();
            ok(ime_composition_test.catch_result_str,
               "WM_IME_COMPOSITION(GCS_RESULTSTR) isn't sent\n");
        }
        else
            win_skip("Composition string isn't available\n");
        ImmReleaseContext(hwnd, imc);
        SetWindowLongPtrA(hwnd, GWLP_WNDPROC,
                          (LONG_PTR)ime_composition_test.old_wnd_proc);
        msg_spy_flush_msgs();
    }

    /* Test composition results input by hand */
    memset(&ime_composition_test, 0, sizeof(ime_composition_test));
    if (winetest_interactive) {
        HWND hwndMain, hwndChild;
        MSG msg;
        const DWORD MY_TIMER = 0xcaffe;

        hwndMain = CreateWindowExA(WS_EX_CLIENTEDGE, wndcls,
                                   "IME composition test",
                                   WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                   CW_USEDEFAULT, CW_USEDEFAULT, 320, 160,
                                   NULL, NULL, GetModuleHandleA(NULL), NULL);
        hwndChild = CreateWindowExA(0, "static",
                                    "Input a DBCS character here using IME.",
                                    WS_CHILD | WS_VISIBLE,
                                    0, 0, 320, 100, hwndMain, NULL,
                                    GetModuleHandleA(NULL), NULL);

        ime_composition_test.old_wnd_proc =
            (WNDPROC)SetWindowLongPtrA(hwndChild, GWLP_WNDPROC,
                                       (LONG_PTR)test_ime_wnd_proc);

        SetFocus(hwndChild);

        ime_composition_test.timer_id = MY_TIMER;
        ime_composition_test.start = GetTickCount();
        SetTimer(hwndChild, ime_composition_test.timer_id, 100, NULL);
        while (GetMessageA(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
            if (!IsWindow(hwndMain))
                break;
        }
        if (!ime_composition_test.catch_result_str)
            skip("WM_IME_COMPOSITION(GCS_RESULTSTR) isn't tested\n");
        msg_spy_flush_msgs();
    }
}

static void test_ImmSetCompositionString(void)
{
    HIMC imc;
    BOOL ret;

    SetLastError(0xdeadbeef);
    imc = ImmGetContext(hwnd);
    ok(imc != 0, "ImmGetContext() failed. Last error: %lu\n", GetLastError());
    if (!imc)
        return;

    ret = ImmSetCompositionStringW(imc, SCS_SETSTR, NULL, 0, NULL, 0);
    ok(broken(!ret) ||
       ret, /* Vista+ */
       "ImmSetCompositionStringW() failed.\n");

    ret = ImmSetCompositionStringW(imc, SCS_SETSTR | SCS_CHANGEATTR,
        NULL, 0, NULL, 0);
    ok(!ret, "ImmSetCompositionStringW() succeeded.\n");

    ret = ImmSetCompositionStringW(imc, SCS_SETSTR | SCS_CHANGECLAUSE,
        NULL, 0, NULL, 0);
    ok(!ret, "ImmSetCompositionStringW() succeeded.\n");

    ret = ImmSetCompositionStringW(imc, SCS_CHANGEATTR | SCS_CHANGECLAUSE,
        NULL, 0, NULL, 0);
    ok(!ret, "ImmSetCompositionStringW() succeeded.\n");

    ret = ImmSetCompositionStringW(imc, SCS_SETSTR | SCS_CHANGEATTR | SCS_CHANGECLAUSE,
        NULL, 0, NULL, 0);
    ok(!ret, "ImmSetCompositionStringW() succeeded.\n");

    ImmReleaseContext(hwnd, imc);
}

static void test_ImmIME(void)
{
    HIMC imc;

    imc = ImmGetContext(hwnd);
    if (imc)
    {
        BOOL rc;
        rc = ImmConfigureIMEA(imc, NULL, IME_CONFIG_REGISTERWORD, NULL);
        ok (rc == 0, "ImmConfigureIMEA did not fail\n");
        rc = ImmConfigureIMEW(imc, NULL, IME_CONFIG_REGISTERWORD, NULL);
        ok (rc == 0, "ImmConfigureIMEW did not fail\n");
    }
    ImmReleaseContext(hwnd,imc);
}

static void test_ImmAssociateContextEx(void)
{
    HIMC imc;
    BOOL rc;

    if (!pImmAssociateContextEx) return;

    imc = ImmGetContext(hwnd);
    if (imc)
    {
        HIMC retimc, newimc;
        HWND focus;

        SET_ENABLE(WM_IME_SETCONTEXT_ACTIVATE, TRUE);
        SET_ENABLE(WM_IME_SETCONTEXT_DEACTIVATE, TRUE);

        ok(GetActiveWindow() == hwnd, "hwnd is not active\n");
        newimc = ImmCreateContext();
        ok(newimc != imc, "handles should not be the same\n");
        rc = pImmAssociateContextEx(NULL, NULL, 0);
        ok(!rc, "ImmAssociateContextEx succeeded\n");
        SET_EXPECT(WM_IME_SETCONTEXT_DEACTIVATE);
        SET_EXPECT(WM_IME_SETCONTEXT_ACTIVATE);
        rc = pImmAssociateContextEx(hwnd, NULL, 0);
        CHECK_CALLED(WM_IME_SETCONTEXT_DEACTIVATE);
        CHECK_CALLED(WM_IME_SETCONTEXT_ACTIVATE);
        ok(rc, "ImmAssociateContextEx failed\n");
        rc = pImmAssociateContextEx(NULL, imc, 0);
        ok(!rc, "ImmAssociateContextEx succeeded\n");

        SET_EXPECT(WM_IME_SETCONTEXT_DEACTIVATE);
        SET_EXPECT(WM_IME_SETCONTEXT_ACTIVATE);
        rc = pImmAssociateContextEx(hwnd, imc, 0);
        CHECK_CALLED(WM_IME_SETCONTEXT_DEACTIVATE);
        CHECK_CALLED(WM_IME_SETCONTEXT_ACTIVATE);
        ok(rc, "ImmAssociateContextEx failed\n");
        retimc = ImmGetContext(hwnd);
        ok(retimc == imc, "handles should be the same\n");
        ImmReleaseContext(hwnd,retimc);

        rc = pImmAssociateContextEx(hwnd, imc, 0);
        ok(rc, "ImmAssociateContextEx failed\n");

        SET_EXPECT(WM_IME_SETCONTEXT_DEACTIVATE);
        SET_EXPECT(WM_IME_SETCONTEXT_ACTIVATE);
        rc = pImmAssociateContextEx(hwnd, newimc, 0);
        CHECK_CALLED(WM_IME_SETCONTEXT_DEACTIVATE);
        CHECK_CALLED(WM_IME_SETCONTEXT_ACTIVATE);
        ok(rc, "ImmAssociateContextEx failed\n");
        retimc = ImmGetContext(hwnd);
        ok(retimc == newimc, "handles should be the same\n");
        ImmReleaseContext(hwnd,retimc);

        focus = CreateWindowA("button", "button", 0, 0, 0, 0, 0, 0, 0, 0, 0);
        ok(focus != NULL, "CreateWindow failed\n");
        SET_EXPECT(WM_IME_SETCONTEXT_DEACTIVATE);
        SetFocus(focus);
        CHECK_CALLED(WM_IME_SETCONTEXT_DEACTIVATE);
        rc = pImmAssociateContextEx(hwnd, imc, 0);
        ok(rc, "ImmAssociateContextEx failed\n");
        SET_EXPECT(WM_IME_SETCONTEXT_ACTIVATE);
        DestroyWindow(focus);
        CHECK_CALLED(WM_IME_SETCONTEXT_ACTIVATE);

        SET_EXPECT(WM_IME_SETCONTEXT_DEACTIVATE);
        SetFocus(child);
        CHECK_CALLED(WM_IME_SETCONTEXT_DEACTIVATE);
        rc = pImmAssociateContextEx(hwnd, newimc, 0);
        ok(rc, "ImmAssociateContextEx failed\n");
        SET_EXPECT(WM_IME_SETCONTEXT_ACTIVATE);
        SetFocus(hwnd);
        CHECK_CALLED(WM_IME_SETCONTEXT_ACTIVATE);

        SET_EXPECT(WM_IME_SETCONTEXT_DEACTIVATE);
        SET_EXPECT(WM_IME_SETCONTEXT_ACTIVATE);
        rc = pImmAssociateContextEx(hwnd, NULL, IACE_DEFAULT);
        CHECK_CALLED(WM_IME_SETCONTEXT_DEACTIVATE);
        CHECK_CALLED(WM_IME_SETCONTEXT_ACTIVATE);
        ok(rc, "ImmAssociateContextEx failed\n");

        SET_ENABLE(WM_IME_SETCONTEXT_ACTIVATE, FALSE);
        SET_ENABLE(WM_IME_SETCONTEXT_DEACTIVATE, FALSE);
    }
    ImmReleaseContext(hwnd,imc);
}

/* similar to above, but using NtUserAssociateInputContext */
static void test_NtUserAssociateInputContext(void)
{
    HIMC imc;
    UINT rc;

    if (!pNtUserAssociateInputContext)
    {
        win_skip("NtUserAssociateInputContext not available\n");
        return;
    }

    imc = ImmGetContext(hwnd);
    if (imc)
    {
        HIMC retimc, newimc;
        HWND focus;

        SET_ENABLE(WM_IME_SETCONTEXT_ACTIVATE, TRUE);
        SET_ENABLE(WM_IME_SETCONTEXT_DEACTIVATE, TRUE);

        ok(GetActiveWindow() == hwnd, "hwnd is not active\n");
        newimc = ImmCreateContext();
        ok(newimc != imc, "handles should not be the same\n");
        rc = pNtUserAssociateInputContext(NULL, NULL, 0);
        ok(rc == 2, "NtUserAssociateInputContext returned %x\n", rc);
        rc = pNtUserAssociateInputContext(hwnd, NULL, 0);
        ok(rc == 1, "NtUserAssociateInputContext returned %x\n", rc);
        rc = pNtUserAssociateInputContext(NULL, imc, 0);
        ok(rc == 2, "NtUserAssociateInputContext returned %x\n", rc);

        rc = pNtUserAssociateInputContext(hwnd, imc, 0);
        ok(rc == 1, "NtUserAssociateInputContext returned %x\n", rc);
        retimc = ImmGetContext(hwnd);
        ok(retimc == imc, "handles should be the same\n");
        ImmReleaseContext(hwnd,retimc);

        rc = pNtUserAssociateInputContext(hwnd, imc, 0);
        ok(rc == 0, "NtUserAssociateInputContext returned %x\n", rc);

        rc = pNtUserAssociateInputContext(hwnd, newimc, 0);
        ok(rc == 1, "NtUserAssociateInputContext returned %x\n", rc);
        retimc = ImmGetContext(hwnd);
        ok(retimc == newimc, "handles should be the same\n");
        ImmReleaseContext(hwnd,retimc);

        focus = CreateWindowA("button", "button", 0, 0, 0, 0, 0, 0, 0, 0, 0);
        ok(focus != NULL, "CreateWindow failed\n");
        SET_EXPECT(WM_IME_SETCONTEXT_DEACTIVATE);
        SetFocus(focus);
        CHECK_CALLED(WM_IME_SETCONTEXT_DEACTIVATE);
        rc = pNtUserAssociateInputContext(hwnd, imc, 0);
        ok(rc == 0, "NtUserAssociateInputContext returned %x\n", rc);
        SET_EXPECT(WM_IME_SETCONTEXT_ACTIVATE);
        DestroyWindow(focus);
        CHECK_CALLED(WM_IME_SETCONTEXT_ACTIVATE);

        SET_EXPECT(WM_IME_SETCONTEXT_DEACTIVATE);
        SetFocus(child);
        CHECK_CALLED(WM_IME_SETCONTEXT_DEACTIVATE);
        rc = pNtUserAssociateInputContext(hwnd, newimc, 0);
        ok(rc == 0, "NtUserAssociateInputContext returned %x\n", rc);
        SET_EXPECT(WM_IME_SETCONTEXT_ACTIVATE);
        SetFocus(hwnd);
        CHECK_CALLED(WM_IME_SETCONTEXT_ACTIVATE);

        rc = pNtUserAssociateInputContext(hwnd, NULL, IACE_DEFAULT);
        ok(rc == 1, "NtUserAssociateInputContext returned %x\n", rc);

        SET_ENABLE(WM_IME_SETCONTEXT_ACTIVATE, FALSE);
        SET_ENABLE(WM_IME_SETCONTEXT_DEACTIVATE, FALSE);
    }
    ImmReleaseContext(hwnd,imc);
}

typedef struct _igc_threadinfo {
    HWND hwnd;
    HANDLE event;
    HIMC himc;
    HIMC u_himc;
} igc_threadinfo;


static DWORD WINAPI ImmGetContextThreadFunc( LPVOID lpParam)
{
    HIMC h1,h2;
    HWND hwnd2;
    COMPOSITIONFORM cf;
    CANDIDATEFORM cdf;
    POINT pt;
    MSG msg;

    igc_threadinfo *info= (igc_threadinfo*)lpParam;
    info->hwnd = CreateWindowExA(WS_EX_CLIENTEDGE, wndcls, "Wine imm32.dll test",
                                 WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                 240, 120, NULL, NULL, GetModuleHandleW(NULL), NULL);

    h1 = ImmGetContext(hwnd);
    ok(info->himc == h1, "hwnd context changed in new thread\n");
    h2 = ImmGetContext(info->hwnd);
    ok(h2 != h1, "new hwnd in new thread should have different context\n");
    info->himc = h2;
    ImmReleaseContext(hwnd,h1);

    hwnd2 = CreateWindowExA(WS_EX_CLIENTEDGE, wndcls, "Wine imm32.dll test",
                            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                            240, 120, NULL, NULL, GetModuleHandleW(NULL), NULL);
    h1 = ImmGetContext(hwnd2);

    ok(h1 == h2, "Windows in same thread should have same default context\n");
    ImmReleaseContext(hwnd2,h1);
    ImmReleaseContext(info->hwnd,h2);
    DestroyWindow(hwnd2);

    /* priming for later tests */
    ImmSetCompositionWindow(h1, &cf);
    ImmSetStatusWindowPos(h1, &pt);
    info->u_himc = ImmCreateContext();
    ImmSetOpenStatus(info->u_himc, TRUE);
    cdf.dwIndex = 0;
    cdf.dwStyle = CFS_CANDIDATEPOS;
    cdf.ptCurrentPos.x = 0;
    cdf.ptCurrentPos.y = 0;
    ImmSetCandidateWindow(info->u_himc, &cdf);

    SetEvent(info->event);

    while(GetMessageW(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 1;
}

static void test_ImmThreads(void)
{
    HIMC himc, otherHimc, h1;
    igc_threadinfo threadinfo;
    HANDLE hThread;
    DWORD dwThreadId;
    BOOL rc;
    LOGFONTA lf;
    COMPOSITIONFORM cf;
    CANDIDATEFORM cdf;
    DWORD status, sentence;
    POINT pt;

    himc = ImmGetContext(hwnd);
    threadinfo.event = CreateEventA(NULL, TRUE, FALSE, NULL);
    threadinfo.himc = himc;
    hThread = CreateThread(NULL, 0, ImmGetContextThreadFunc, &threadinfo, 0, &dwThreadId );
    WaitForSingleObject(threadinfo.event, INFINITE);

    otherHimc = ImmGetContext(threadinfo.hwnd);

    ok(himc != otherHimc, "Windows from other threads should have different himc\n");
    ok(otherHimc == threadinfo.himc, "Context from other thread should not change in main thread\n");

    SET_ENABLE(WM_IME_SETCONTEXT_DEACTIVATE, TRUE);
    SET_ENABLE(WM_IME_SETCONTEXT_ACTIVATE, TRUE);
    SET_EXPECT(WM_IME_SETCONTEXT_ACTIVATE);
    rc = ImmSetActiveContext(hwnd, otherHimc, TRUE);
    ok(rc, "ImmSetActiveContext failed\n");
    CHECK_CALLED(WM_IME_SETCONTEXT_ACTIVATE);
    SET_EXPECT(WM_IME_SETCONTEXT_DEACTIVATE);
    rc = ImmSetActiveContext(hwnd, otherHimc, FALSE);
    ok(rc, "ImmSetActiveContext failed\n");
    CHECK_CALLED(WM_IME_SETCONTEXT_DEACTIVATE);
    SET_ENABLE(WM_IME_SETCONTEXT_DEACTIVATE, FALSE);
    SET_ENABLE(WM_IME_SETCONTEXT_ACTIVATE, FALSE);

    h1 = ImmAssociateContext(hwnd,otherHimc);
    ok(h1 == NULL, "Should fail to be able to Associate a default context from a different thread\n");
    h1 = ImmGetContext(hwnd);
    ok(h1 == himc, "Context for window should remain unchanged\n");
    ImmReleaseContext(hwnd,h1);

    h1 = ImmAssociateContext(hwnd, threadinfo.u_himc);
    ok (h1 == NULL, "Should fail to associate a context from a different thread\n");
    h1 = ImmGetContext(hwnd);
    ok(h1 == himc, "Context for window should remain unchanged\n");
    ImmReleaseContext(hwnd,h1);

    h1 = ImmAssociateContext(threadinfo.hwnd, threadinfo.u_himc);
    ok (h1 == NULL, "Should fail to associate a context from a different thread into a window from that thread.\n");
    h1 = ImmGetContext(threadinfo.hwnd);
    ok(h1 == threadinfo.himc, "Context for window should remain unchanged\n");
    ImmReleaseContext(threadinfo.hwnd,h1);

    /* OpenStatus */
    rc = ImmSetOpenStatus(himc, TRUE);
    ok(rc != 0, "ImmSetOpenStatus failed\n");
    rc = ImmGetOpenStatus(himc);
    ok(rc != 0, "ImmGetOpenStatus failed\n");
    rc = ImmSetOpenStatus(himc, FALSE);
    ok(rc != 0, "ImmSetOpenStatus failed\n");
    rc = ImmGetOpenStatus(himc);
    ok(rc == 0, "ImmGetOpenStatus failed\n");

    rc = ImmSetOpenStatus(otherHimc, TRUE);
    ok(rc == 0, "ImmSetOpenStatus should fail\n");
    rc = ImmSetOpenStatus(threadinfo.u_himc, TRUE);
    ok(rc == 0, "ImmSetOpenStatus should fail\n");
    rc = ImmGetOpenStatus(otherHimc);
    ok(rc == 0, "ImmGetOpenStatus failed\n");
    rc = ImmGetOpenStatus(threadinfo.u_himc);
    ok (rc == 1 || broken(rc == 0), "ImmGetOpenStatus should return 1\n");
    rc = ImmSetOpenStatus(otherHimc, FALSE);
    ok(rc == 0, "ImmSetOpenStatus should fail\n");
    rc = ImmGetOpenStatus(otherHimc);
    ok(rc == 0, "ImmGetOpenStatus failed\n");

    /* CompositionFont */
    rc = ImmGetCompositionFontA(himc, &lf);
    ok(rc != 0, "ImmGetCompositionFont failed\n");
    rc = ImmSetCompositionFontA(himc, &lf);
    ok(rc != 0, "ImmSetCompositionFont failed\n");

    rc = ImmGetCompositionFontA(otherHimc, &lf);
    ok(rc != 0 || broken(rc == 0), "ImmGetCompositionFont failed\n");
    rc = ImmGetCompositionFontA(threadinfo.u_himc, &lf);
    ok(rc != 0 || broken(rc == 0), "ImmGetCompositionFont user himc failed\n");
    rc = ImmSetCompositionFontA(otherHimc, &lf);
    ok(rc == 0, "ImmSetCompositionFont should fail\n");
    rc = ImmSetCompositionFontA(threadinfo.u_himc, &lf);
    ok(rc == 0, "ImmSetCompositionFont should fail\n");

    /* CompositionString */
    rc = ImmSetCompositionStringA(himc, SCS_SETSTR, "a", 2, NULL, 0);
    ok(rc, "failed.\n");
    rc = ImmSetCompositionStringA(otherHimc, SCS_SETSTR, "a", 2, NULL, 0);
    ok(!rc, "should fail.\n");
    rc = ImmSetCompositionStringA(threadinfo.u_himc, SCS_SETSTR, "a", 2, NULL, 0);
    ok(!rc, "should fail.\n");

    /* CompositionWindow */
    rc = ImmSetCompositionWindow(himc, &cf);
    ok(rc != 0, "ImmSetCompositionWindow failed\n");
    rc = ImmGetCompositionWindow(himc, &cf);
    ok(rc != 0, "ImmGetCompositionWindow failed\n");

    rc = ImmSetCompositionWindow(otherHimc, &cf);
    ok(rc == 0, "ImmSetCompositionWindow should fail\n");
    rc = ImmSetCompositionWindow(threadinfo.u_himc, &cf);
    ok(rc == 0, "ImmSetCompositionWindow should fail\n");
    rc = ImmGetCompositionWindow(otherHimc, &cf);
    ok(rc != 0 || broken(rc == 0), "ImmGetCompositionWindow failed\n");
    rc = ImmGetCompositionWindow(threadinfo.u_himc, &cf);
    ok(rc != 0 || broken(rc == 0), "ImmGetCompositionWindow failed\n");

    /* ConversionStatus */
    rc = ImmGetConversionStatus(himc, &status, &sentence);
    ok(rc != 0, "ImmGetConversionStatus failed\n");
    rc = ImmSetConversionStatus(himc, status, sentence);
    ok(rc != 0, "ImmSetConversionStatus failed\n");

    rc = ImmGetConversionStatus(otherHimc, &status, &sentence);
    ok(rc != 0 || broken(rc == 0), "ImmGetConversionStatus failed\n");
    rc = ImmGetConversionStatus(threadinfo.u_himc, &status, &sentence);
    ok(rc != 0 || broken(rc == 0), "ImmGetConversionStatus failed\n");
    rc = ImmSetConversionStatus(otherHimc, status, sentence);
    ok(rc == 0, "ImmSetConversionStatus should fail\n");
    rc = ImmSetConversionStatus(threadinfo.u_himc, status, sentence);
    ok(rc == 0, "ImmSetConversionStatus should fail\n");

    /* StatusWindowPos */
    rc = ImmSetStatusWindowPos(himc, &pt);
    ok(rc != 0, "ImmSetStatusWindowPos failed\n");
    rc = ImmGetStatusWindowPos(himc, &pt);
    ok(rc != 0, "ImmGetStatusWindowPos failed\n");

    rc = ImmSetStatusWindowPos(otherHimc, &pt);
    ok(rc == 0, "ImmSetStatusWindowPos should fail\n");
    rc = ImmSetStatusWindowPos(threadinfo.u_himc, &pt);
    ok(rc == 0, "ImmSetStatusWindowPos should fail\n");
    rc = ImmGetStatusWindowPos(otherHimc, &pt);
    ok(rc != 0 || broken(rc == 0), "ImmGetStatusWindowPos failed\n");
    rc = ImmGetStatusWindowPos(threadinfo.u_himc, &pt);
    ok(rc != 0 || broken(rc == 0), "ImmGetStatusWindowPos failed\n");

    h1 = ImmAssociateContext(threadinfo.hwnd, NULL);
    ok (h1 == otherHimc, "ImmAssociateContext cross thread with NULL should work\n");
    h1 = ImmGetContext(threadinfo.hwnd);
    ok (h1 == NULL, "CrossThread window context should be NULL\n");
    h1 = ImmAssociateContext(threadinfo.hwnd, h1);
    ok (h1 == NULL, "Resetting cross thread context should fail\n");
    h1 = ImmGetContext(threadinfo.hwnd);
    ok (h1 == NULL, "CrossThread window context should still be NULL\n");

    rc = ImmDestroyContext(threadinfo.u_himc);
    ok (rc == 0, "ImmDestroyContext Cross Thread should fail\n");

    /* Candidate Window */
    rc = ImmGetCandidateWindow(himc, 0, &cdf);
    ok (rc == 0, "ImmGetCandidateWindow should fail\n");
    cdf.dwIndex = 0;
    cdf.dwStyle = CFS_CANDIDATEPOS;
    cdf.ptCurrentPos.x = 0;
    cdf.ptCurrentPos.y = 0;
    rc = ImmSetCandidateWindow(himc, &cdf);
    ok (rc == 1, "ImmSetCandidateWindow should succeed\n");
    rc = ImmGetCandidateWindow(himc, 0, &cdf);
    ok (rc == 1, "ImmGetCandidateWindow should succeed\n");

    rc = ImmGetCandidateWindow(otherHimc, 0, &cdf);
    ok (rc == 0, "ImmGetCandidateWindow should fail\n");
    rc = ImmSetCandidateWindow(otherHimc, &cdf);
    ok (rc == 0, "ImmSetCandidateWindow should fail\n");
    rc = ImmGetCandidateWindow(threadinfo.u_himc, 0, &cdf);
    ok (rc == 1 || broken( rc == 0), "ImmGetCandidateWindow should succeed\n");
    rc = ImmSetCandidateWindow(threadinfo.u_himc, &cdf);
    ok (rc == 0, "ImmSetCandidateWindow should fail\n");

    ImmReleaseContext(threadinfo.hwnd,otherHimc);
    ImmReleaseContext(hwnd,himc);

    SendMessageA(threadinfo.hwnd, WM_CLOSE, 0, 0);
    rc = PostThreadMessageA(dwThreadId, WM_QUIT, 1, 0);
    ok(rc == 1, "PostThreadMessage should succeed\n");
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

    himc = ImmGetContext(GetDesktopWindow());
    ok(himc == NULL, "Should not be able to get himc from other process window\n");
}

static void test_ImmIsUIMessage(void)
{
    struct test
    {
        UINT msg;
        BOOL ret;
    };

    static const struct test tests[] =
    {
        { WM_MOUSEMOVE,            FALSE },
        { WM_IME_STARTCOMPOSITION, TRUE  },
        { WM_IME_ENDCOMPOSITION,   TRUE  },
        { WM_IME_COMPOSITION,      TRUE  },
        { WM_IME_SETCONTEXT,       TRUE  },
        { WM_IME_NOTIFY,           TRUE  },
        { WM_IME_CONTROL,          FALSE },
        { WM_IME_COMPOSITIONFULL,  TRUE  },
        { WM_IME_SELECT,           TRUE  },
        { WM_IME_CHAR,             FALSE },
        { 0x287 /* FIXME */,       TRUE  },
        { WM_IME_REQUEST,          FALSE },
        { WM_IME_KEYDOWN,          FALSE },
        { WM_IME_KEYUP,            FALSE },
        { 0, FALSE } /* mark the end */
    };

    UINT WM_MSIME_SERVICE = RegisterWindowMessageA("MSIMEService");
    UINT WM_MSIME_RECONVERTOPTIONS = RegisterWindowMessageA("MSIMEReconvertOptions");
    UINT WM_MSIME_MOUSE = RegisterWindowMessageA("MSIMEMouseOperation");
    UINT WM_MSIME_RECONVERTREQUEST = RegisterWindowMessageA("MSIMEReconvertRequest");
    UINT WM_MSIME_RECONVERT = RegisterWindowMessageA("MSIMEReconvert");
    UINT WM_MSIME_QUERYPOSITION = RegisterWindowMessageA("MSIMEQueryPosition");
    UINT WM_MSIME_DOCUMENTFEED = RegisterWindowMessageA("MSIMEDocumentFeed");

    const struct test *test;
    BOOL ret;

    if (!pImmIsUIMessageA) return;

    for (test = tests; test->msg; test++)
    {
        msg_spy_flush_msgs();
        ret = pImmIsUIMessageA(NULL, test->msg, 0, 0);
        ok(ret == test->ret, "ImmIsUIMessageA returned %x for %x\n", ret, test->msg);
        ok(!msg_spy_find_msg(test->msg), "Windows does not send 0x%x for NULL hwnd\n", test->msg);

        ret = pImmIsUIMessageA(hwnd, test->msg, 0, 0);
        ok(ret == test->ret, "ImmIsUIMessageA returned %x for %x\n", ret, test->msg);
        if (ret)
            ok(msg_spy_find_msg(test->msg) != NULL, "Windows does send 0x%x\n", test->msg);
        else
            ok(!msg_spy_find_msg(test->msg), "Windows does not send 0x%x\n", test->msg);
    }

    ret = pImmIsUIMessageA(NULL, WM_MSIME_SERVICE, 0, 0);
    ok(!ret, "ImmIsUIMessageA returned TRUE for WM_MSIME_SERVICE\n");
    ret = pImmIsUIMessageA(NULL, WM_MSIME_RECONVERTOPTIONS, 0, 0);
    ok(!ret, "ImmIsUIMessageA returned TRUE for WM_MSIME_RECONVERTOPTIONS\n");
    ret = pImmIsUIMessageA(NULL, WM_MSIME_MOUSE, 0, 0);
    ok(!ret, "ImmIsUIMessageA returned TRUE for WM_MSIME_MOUSE\n");
    ret = pImmIsUIMessageA(NULL, WM_MSIME_RECONVERTREQUEST, 0, 0);
    ok(!ret, "ImmIsUIMessageA returned TRUE for WM_MSIME_RECONVERTREQUEST\n");
    ret = pImmIsUIMessageA(NULL, WM_MSIME_RECONVERT, 0, 0);
    ok(!ret, "ImmIsUIMessageA returned TRUE for WM_MSIME_RECONVERT\n");
    ret = pImmIsUIMessageA(NULL, WM_MSIME_QUERYPOSITION, 0, 0);
    ok(!ret, "ImmIsUIMessageA returned TRUE for WM_MSIME_QUERYPOSITION\n");
    ret = pImmIsUIMessageA(NULL, WM_MSIME_DOCUMENTFEED, 0, 0);
    ok(!ret, "ImmIsUIMessageA returned TRUE for WM_MSIME_DOCUMENTFEED\n");
}

static void test_ImmGetContext(void)
{
    HIMC himc;
    DWORD err;

    SetLastError(0xdeadbeef);
    himc = ImmGetContext((HWND)0xffffffff);
    err = GetLastError();
    ok(himc == NULL, "ImmGetContext succeeded\n");
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "got %lu\n", err);

    himc = ImmGetContext(hwnd);
    ok(himc != NULL, "ImmGetContext failed\n");
    ok(ImmReleaseContext(hwnd, himc), "ImmReleaseContext failed\n");
}

static LRESULT (WINAPI *old_imm_wnd_proc)(HWND, UINT, WPARAM, LPARAM);
static LRESULT WINAPI imm_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    ok(msg != WM_DESTROY, "got WM_DESTROY message\n");
    return old_imm_wnd_proc(hwnd, msg, wparam, lparam);
}

static HWND thread_ime_wnd;
static DWORD WINAPI test_ImmGetDefaultIMEWnd_thread(void *arg)
{
    CreateWindowA("static", "static", WS_POPUP, 0, 0, 1, 1, NULL, NULL, NULL, NULL);

    thread_ime_wnd = ImmGetDefaultIMEWnd(0);
    ok(thread_ime_wnd != 0, "ImmGetDefaultIMEWnd returned NULL\n");
    old_imm_wnd_proc = (void*)SetWindowLongPtrW(thread_ime_wnd, GWLP_WNDPROC, (LONG_PTR)imm_wnd_proc);
    return 0;
}

static void test_ImmDefaultHwnd(void)
{
    HIMC imc1, imc2, imc3;
    HWND def1, def3;
    HANDLE thread;
    HWND hwnd;
    char title[16];
    LONG style;

    hwnd = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "Wine imm32.dll test",
                           WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                           240, 120, NULL, NULL, GetModuleHandleW(NULL), NULL);

    ShowWindow(hwnd, SW_SHOWNORMAL);

    imc1 = ImmGetContext(hwnd);
    if (!imc1)
    {
        win_skip("IME support not implemented\n");
        return;
    }

    def1 = ImmGetDefaultIMEWnd(hwnd);

    GetWindowTextA(def1, title, sizeof(title));
    ok(!strcmp(title, "Default IME"), "got %s\n", title);
    style = GetWindowLongA(def1, GWL_STYLE);
    ok(style == (WS_DISABLED | WS_POPUP | WS_CLIPSIBLINGS), "got %08lx\n", style);
    style = GetWindowLongA(def1, GWL_EXSTYLE);
    ok(style == 0, "got %08lx\n", style);

    imc2 = ImmCreateContext();
    ImmSetOpenStatus(imc2, TRUE);

    imc3 = ImmGetContext(hwnd);
    def3 = ImmGetDefaultIMEWnd(hwnd);

    ok(def3 == def1, "Default IME window should not change\n");
    ok(imc1 == imc3, "IME context should not change\n");
    ImmSetOpenStatus(imc2, FALSE);

    thread = CreateThread(NULL, 0, test_ImmGetDefaultIMEWnd_thread, NULL, 0, NULL);
    WaitForSingleObject(thread, INFINITE);
    ok(thread_ime_wnd != def1, "thread_ime_wnd == def1\n");
    ok(!IsWindow(thread_ime_wnd), "thread_ime_wnd was not destroyed\n");
    CloseHandle(thread);

    ImmReleaseContext(hwnd, imc1);
    ImmReleaseContext(hwnd, imc3);
    ImmDestroyContext(imc2);
    DestroyWindow(hwnd);
}

static BOOL CALLBACK is_ime_window_proc(HWND hWnd, LPARAM param)
{
    WCHAR class_nameW[16];
    HWND *ime_window = (HWND *)param;
    if (GetClassNameW(hWnd, class_nameW, ARRAY_SIZE(class_nameW)) && !lstrcmpW(class_nameW, L"IME"))
    {
        *ime_window = hWnd;
        return FALSE;
    }
    return TRUE;
}

static HWND get_ime_window(void)
{
    HWND ime_window = NULL;
    EnumThreadWindows(GetCurrentThreadId(), is_ime_window_proc, (LPARAM)&ime_window);
    return ime_window;
}

struct testcase_ime_window {
    BOOL visible;
    BOOL top_level_window;
};

static DWORD WINAPI test_default_ime_window_cb(void *arg)
{
    struct testcase_ime_window *testcase = (struct testcase_ime_window *)arg;
    DWORD visible = testcase->visible ? WS_VISIBLE : 0;
    HWND hwnd1, hwnd2, default_ime_wnd, ime_wnd;

    ok(!get_ime_window(), "Expected no IME windows\n");
    if (testcase->top_level_window) {
        test_phase = FIRST_WINDOW;
        hwnd1 = CreateWindowExA(WS_EX_CLIENTEDGE, wndcls, "Wine imm32.dll test",
                                WS_OVERLAPPEDWINDOW | visible,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                240, 120, NULL, NULL, GetModuleHandleW(NULL), NULL);
    }
    else {
        hwnd1 = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "Wine imm32.dll test",
                                WS_CHILD | visible,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                240, 24, hwnd, NULL, GetModuleHandleW(NULL), NULL);
    }
    ime_wnd = get_ime_window();
    ok(ime_wnd != NULL, "Expected IME window existence\n");
    default_ime_wnd = ImmGetDefaultIMEWnd(hwnd1);
    ok(ime_wnd == default_ime_wnd, "Expected %p, got %p\n", ime_wnd, default_ime_wnd);

    test_phase = SECOND_WINDOW;
    hwnd2 = CreateWindowExA(WS_EX_CLIENTEDGE, wndcls, "Wine imm32.dll test",
                            WS_OVERLAPPEDWINDOW | visible,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            240, 120, NULL, NULL, GetModuleHandleW(NULL), NULL);
    DestroyWindow(hwnd2);
    ok(IsWindow(ime_wnd) ||
       broken(!testcase->visible /* Vista */)  ||
       broken(!testcase->top_level_window /* Vista */) ,
       "Expected IME window existence\n");
    DestroyWindow(hwnd1);
    flaky
    ok(!IsWindow(ime_wnd), "Expected no IME windows\n");
    return 1;
}

static DWORD WINAPI test_default_ime_window_cancel_cb(void *arg)
{
    struct testcase_ime_window *testcase = (struct testcase_ime_window *)arg;
    DWORD visible = testcase->visible ? WS_VISIBLE : 0;
    HWND hwnd1, hwnd2, default_ime_wnd, ime_wnd;

    ok(!get_ime_window(), "Expected no IME windows\n");
    test_phase = NCCREATE_CANCEL;
    hwnd1 = CreateWindowExA(WS_EX_CLIENTEDGE, wndcls, "Wine imm32.dll test",
                            WS_OVERLAPPEDWINDOW | visible,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            240, 120, NULL, NULL, GetModuleHandleW(NULL), NULL);
    ok(hwnd1 == NULL, "creation succeeded, got %p\n", hwnd1);
    ok(!get_ime_window(), "Expected no IME windows\n");

    test_phase = CREATE_CANCEL;
    hwnd1 = CreateWindowExA(WS_EX_CLIENTEDGE, wndcls, "Wine imm32.dll test",
                            WS_OVERLAPPEDWINDOW | visible,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            240, 120, NULL, NULL, GetModuleHandleW(NULL), NULL);
    ok(hwnd1 == NULL, "creation succeeded, got %p\n", hwnd1);
    ok(!get_ime_window(), "Expected no IME windows\n");

    test_phase = FIRST_WINDOW;
    hwnd2 = CreateWindowExA(WS_EX_CLIENTEDGE, wndcls, "Wine imm32.dll test",
                            WS_OVERLAPPEDWINDOW | visible,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            240, 120, NULL, NULL, GetModuleHandleW(NULL), NULL);
    ime_wnd = get_ime_window();
    ok(ime_wnd != NULL, "Expected IME window existence\n");
    default_ime_wnd = ImmGetDefaultIMEWnd(hwnd2);
    ok(ime_wnd == default_ime_wnd, "Expected %p, got %p\n", ime_wnd, default_ime_wnd);

    DestroyWindow(hwnd2);
    ok(!IsWindow(ime_wnd), "Expected no IME windows\n");
    return 1;
}

static DWORD WINAPI test_default_ime_disabled_cb(void *arg)
{
    HWND hWnd, default_ime_wnd;

    ok(!get_ime_window(), "Expected no IME windows\n");
    ImmDisableIME(GetCurrentThreadId());
    test_phase = IME_DISABLED;
    hWnd = CreateWindowExA(WS_EX_CLIENTEDGE, wndcls, "Wine imm32.dll test",
                            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            240, 120, NULL, NULL, GetModuleHandleW(NULL), NULL);
    default_ime_wnd = ImmGetDefaultIMEWnd(hWnd);
    ok(!default_ime_wnd, "Expected no IME windows\n");
    DestroyWindow(hWnd);
    return 1;
}

static DWORD WINAPI test_default_ime_with_message_only_window_cb(void *arg)
{
    HWND hwnd1, hwnd2, default_ime_wnd;

    /* Message-only window doesn't create associated IME window. */
    test_phase = PHASE_UNKNOWN;
    hwnd1 = CreateWindowA(wndcls, "Wine imm32.dll test",
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          240, 120, HWND_MESSAGE, NULL, GetModuleHandleW(NULL), NULL);
    default_ime_wnd = ImmGetDefaultIMEWnd(hwnd1);
    ok(!default_ime_wnd, "Expected no IME windows, got %p\n", default_ime_wnd);

    /* Setting message-only window as owner at creation,
       doesn't create associated IME window. */
    hwnd2 = CreateWindowA(wndcls, "Wine imm32.dll test",
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          240, 120, hwnd1, NULL, GetModuleHandleW(NULL), NULL);
    default_ime_wnd = ImmGetDefaultIMEWnd(hwnd2);
    todo_wine ok(!default_ime_wnd || broken(IsWindow(default_ime_wnd)), "Expected no IME windows, got %p\n", default_ime_wnd);

    DestroyWindow(hwnd2);
    DestroyWindow(hwnd1);

    /* Making window message-only after creation,
       doesn't disassociate IME window. */
    hwnd1 = CreateWindowA(wndcls, "Wine imm32.dll test",
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          240, 120, NULL, NULL, GetModuleHandleW(NULL), NULL);
    default_ime_wnd = ImmGetDefaultIMEWnd(hwnd1);
    ok(IsWindow(default_ime_wnd), "Expected IME window existence\n");
    SetParent(hwnd1, HWND_MESSAGE);
    default_ime_wnd = ImmGetDefaultIMEWnd(hwnd1);
    ok(IsWindow(default_ime_wnd), "Expected IME window existence\n");
    DestroyWindow(hwnd1);
    return 1;
}

static void test_default_ime_window_creation(void)
{
    HANDLE thread;
    size_t i;
    struct testcase_ime_window testcases[] = {
        /* visible, top-level window */
        { TRUE,  TRUE  },
        { FALSE, TRUE  },
        { TRUE,  FALSE },
        { FALSE, FALSE }
    };

    for (i = 0; i < ARRAY_SIZE(testcases); i++)
    {
        thread = CreateThread(NULL, 0, test_default_ime_window_cb, &testcases[i], 0, NULL);
        ok(thread != NULL, "CreateThread failed with error %lu\n", GetLastError());
        while (MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT) == WAIT_OBJECT_0 + 1)
        {
            MSG msg;
            while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }
        CloseHandle(thread);

        if (testcases[i].top_level_window)
        {
            thread = CreateThread(NULL, 0, test_default_ime_window_cancel_cb, &testcases[i], 0, NULL);
            ok(thread != NULL, "CreateThread failed with error %lu\n", GetLastError());
            WaitForSingleObject(thread, INFINITE);
            CloseHandle(thread);
        }
    }

    thread = CreateThread(NULL, 0, test_default_ime_disabled_cb, NULL, 0, NULL);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    thread = CreateThread(NULL, 0, test_default_ime_with_message_only_window_cb, NULL, 0, NULL);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    test_phase = PHASE_UNKNOWN;
}

static void test_ImmGetIMCLockCount(void)
{
    HIMC imc;
    DWORD count, ret, i;
    INPUTCONTEXT *ic;

    imc = ImmCreateContext();
    ImmDestroyContext(imc);
    SetLastError(0xdeadbeef);
    count = ImmGetIMCLockCount((HIMC)0xdeadcafe);
    ok(count == 0, "Invalid IMC should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    count = ImmGetIMCLockCount(0x00000000);
    ok(count == 0, "NULL IMC should return 0\n");
    ret = GetLastError();
    ok(ret == 0xdeadbeef, "Last Error should remain unchanged: %08lx\n",ret);
    count = ImmGetIMCLockCount(imc);
    ok(count == 0, "Destroyed IMC should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);

    imc = ImmCreateContext();
    count = ImmGetIMCLockCount(imc);
    ok(count == 0, "expect 0, returned %ld\n", count);
    ic = ImmLockIMC(imc);
    ok(ic != NULL, "ImmLockIMC failed!\n");
    count = ImmGetIMCLockCount(imc);
    ok(count == 1, "expect 1, returned %ld\n", count);
    ret = ImmUnlockIMC(imc);
    ok(ret == TRUE, "expect TRUE, ret %ld\n", ret);
    count = ImmGetIMCLockCount(imc);
    ok(count == 0, "expect 0, returned %ld\n", count);
    ret = ImmUnlockIMC(imc);
    ok(ret == TRUE, "expect TRUE, ret %ld\n", ret);
    count = ImmGetIMCLockCount(imc);
    ok(count == 0, "expect 0, returned %ld\n", count);

    for (i = 0; i < GMEM_LOCKCOUNT * 2; i++)
    {
        ic = ImmLockIMC(imc);
        ok(ic != NULL, "ImmLockIMC failed!\n");
    }
    count = ImmGetIMCLockCount(imc);
    todo_wine ok(count == GMEM_LOCKCOUNT, "expect GMEM_LOCKCOUNT, returned %ld\n", count);

    for (i = 0; i < GMEM_LOCKCOUNT - 1; i++)
        ImmUnlockIMC(imc);
    count = ImmGetIMCLockCount(imc);
    todo_wine ok(count == 1, "expect 1, returned %ld\n", count);
    ImmUnlockIMC(imc);
    count = ImmGetIMCLockCount(imc);
    todo_wine ok(count == 0, "expect 0, returned %ld\n", count);

    ImmDestroyContext(imc);
}

static void test_ImmGetIMCCLockCount(void)
{
    HIMCC imcc;
    DWORD count, g_count, i;
    BOOL ret;
    VOID *p;

    imcc = ImmCreateIMCC(sizeof(CANDIDATEINFO));
    count = ImmGetIMCCLockCount(imcc);
    ok(count == 0, "expect 0, returned %ld\n", count);
    ImmLockIMCC(imcc);
    count = ImmGetIMCCLockCount(imcc);
    ok(count == 1, "expect 1, returned %ld\n", count);
    ret = ImmUnlockIMCC(imcc);
    ok(ret == FALSE, "expect FALSE, ret %d\n", ret);
    count = ImmGetIMCCLockCount(imcc);
    ok(count == 0, "expect 0, returned %ld\n", count);
    ret = ImmUnlockIMCC(imcc);
    ok(ret == FALSE, "expect FALSE, ret %d\n", ret);
    count = ImmGetIMCCLockCount(imcc);
    ok(count == 0, "expect 0, returned %ld\n", count);

    p = ImmLockIMCC(imcc);
    ok(GlobalHandle(p) == imcc, "expect %p, returned %p\n", imcc, GlobalHandle(p));

    for (i = 0; i < GMEM_LOCKCOUNT * 2; i++)
    {
        ImmLockIMCC(imcc);
        count = ImmGetIMCCLockCount(imcc);
        g_count = GlobalFlags(imcc) & GMEM_LOCKCOUNT;
        ok(count == g_count, "count %ld, g_count %ld\n", count, g_count);
    }
    count = ImmGetIMCCLockCount(imcc);
    ok(count == GMEM_LOCKCOUNT, "expect GMEM_LOCKCOUNT, returned %ld\n", count);

    for (i = 0; i < GMEM_LOCKCOUNT - 1; i++)
        GlobalUnlock(imcc);
    count = ImmGetIMCCLockCount(imcc);
    ok(count == 1, "expect 1, returned %ld\n", count);
    GlobalUnlock(imcc);
    count = ImmGetIMCCLockCount(imcc);
    ok(count == 0, "expect 0, returned %ld\n", count);

    ImmDestroyIMCC(imcc);
}

static void test_ImmDestroyContext(void)
{
    HIMC imc;
    DWORD ret, count;
    INPUTCONTEXT *ic;

    imc = ImmCreateContext();
    count = ImmGetIMCLockCount(imc);
    ok(count == 0, "expect 0, returned %ld\n", count);
    ic = ImmLockIMC(imc);
    ok(ic != NULL, "ImmLockIMC failed!\n");
    count = ImmGetIMCLockCount(imc);
    ok(count == 1, "expect 1, returned %ld\n", count);
    ret = ImmDestroyContext(imc);
    ok(ret == TRUE, "Destroy a locked IMC should success!\n");
    ic = ImmLockIMC(imc);
    ok(ic == NULL, "Lock a destroyed IMC should fail!\n");
    ret = ImmUnlockIMC(imc);
    ok(ret == FALSE, "Unlock a destroyed IMC should fail!\n");
    count = ImmGetIMCLockCount(imc);
    ok(count == 0, "Get lock count of a destroyed IMC should return 0!\n");
    SetLastError(0xdeadbeef);
    ret = ImmDestroyContext(imc);
    ok(ret == FALSE, "Destroy a destroyed IMC should fail!\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
}

static void test_ImmDestroyIMCC(void)
{
    HIMCC imcc;
    DWORD ret, count, size;
    VOID *p;

    imcc = ImmCreateIMCC(sizeof(CANDIDATEINFO));
    count = ImmGetIMCCLockCount(imcc);
    ok(count == 0, "expect 0, returned %ld\n", count);
    p = ImmLockIMCC(imcc);
    ok(p != NULL, "ImmLockIMCC failed!\n");
    count = ImmGetIMCCLockCount(imcc);
    ok(count == 1, "expect 1, returned %ld\n", count);
    size = ImmGetIMCCSize(imcc);
    ok(size == sizeof(CANDIDATEINFO), "returned %ld\n", size);
    p = ImmDestroyIMCC(imcc);
    ok(p == NULL, "Destroy a locked IMCC should success!\n");
    p = ImmLockIMCC(imcc);
    ok(p == NULL, "Lock a destroyed IMCC should fail!\n");
    ret = ImmUnlockIMCC(imcc);
    ok(ret == FALSE, "Unlock a destroyed IMCC should return FALSE!\n");
    count = ImmGetIMCCLockCount(imcc);
    ok(count == 0, "Get lock count of a destroyed IMCC should return 0!\n");
    size = ImmGetIMCCSize(imcc);
    ok(size == 0, "Get size of a destroyed IMCC should return 0!\n");
    SetLastError(0xdeadbeef);
    p = ImmDestroyIMCC(imcc);
    ok(p != NULL, "returned NULL\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
}

static void test_ImmMessages(void)
{
    CANDIDATEFORM cf;
    imm_msgs *msg;
    HWND defwnd;
    HIMC imc;
    UINT idx = 0;

    LPINPUTCONTEXT lpIMC;
    LPTRANSMSG lpTransMsg;

    HWND hwnd = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "Wine imm32.dll test",
                                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                240, 120, NULL, NULL, GetModuleHandleA(NULL), NULL);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    defwnd = ImmGetDefaultIMEWnd(hwnd);
    imc = ImmGetContext(hwnd);

    ImmSetOpenStatus(imc, TRUE);
    msg_spy_flush_msgs();
    SendMessageA(defwnd, WM_IME_CONTROL, IMC_GETCANDIDATEPOS, (LPARAM)&cf );
    do
    {
        msg = msg_spy_find_next_msg(WM_IME_CONTROL,&idx);
        if (msg) ok(!msg->post, "Message should not be posted\n");
    } while (msg);
    msg_spy_flush_msgs();

    lpIMC = ImmLockIMC(imc);
    lpIMC->hMsgBuf = ImmReSizeIMCC(lpIMC->hMsgBuf, (lpIMC->dwNumMsgBuf + 1) * sizeof(TRANSMSG));
    lpTransMsg = ImmLockIMCC(lpIMC->hMsgBuf);
    lpTransMsg += lpIMC->dwNumMsgBuf;
    lpTransMsg->message = WM_IME_STARTCOMPOSITION;
    lpTransMsg->wParam = 0;
    lpTransMsg->lParam = 0;
    ImmUnlockIMCC(lpIMC->hMsgBuf);
    lpIMC->dwNumMsgBuf++;
    ImmUnlockIMC(imc);
    ImmGenerateMessage(imc);
    idx = 0;
    do
    {
        msg = msg_spy_find_next_msg(WM_IME_STARTCOMPOSITION, &idx);
        if (msg) ok(!msg->post, "Message should not be posted\n");
    } while (msg);
    msg_spy_flush_msgs();

    lpIMC = ImmLockIMC(imc);
    lpIMC->hMsgBuf = ImmReSizeIMCC(lpIMC->hMsgBuf, (lpIMC->dwNumMsgBuf + 1) * sizeof(TRANSMSG));
    lpTransMsg = ImmLockIMCC(lpIMC->hMsgBuf);
    lpTransMsg += lpIMC->dwNumMsgBuf;
    lpTransMsg->message = WM_IME_COMPOSITION;
    lpTransMsg->wParam = 0;
    lpTransMsg->lParam = 0;
    ImmUnlockIMCC(lpIMC->hMsgBuf);
    lpIMC->dwNumMsgBuf++;
    ImmUnlockIMC(imc);
    ImmGenerateMessage(imc);
    idx = 0;
    do
    {
        msg = msg_spy_find_next_msg(WM_IME_COMPOSITION, &idx);
        if (msg) ok(!msg->post, "Message should not be posted\n");
    } while (msg);
    msg_spy_flush_msgs();

    lpIMC = ImmLockIMC(imc);
    lpIMC->hMsgBuf = ImmReSizeIMCC(lpIMC->hMsgBuf, (lpIMC->dwNumMsgBuf + 1) * sizeof(TRANSMSG));
    lpTransMsg = ImmLockIMCC(lpIMC->hMsgBuf);
    lpTransMsg += lpIMC->dwNumMsgBuf;
    lpTransMsg->message = WM_IME_ENDCOMPOSITION;
    lpTransMsg->wParam = 0;
    lpTransMsg->lParam = 0;
    ImmUnlockIMCC(lpIMC->hMsgBuf);
    lpIMC->dwNumMsgBuf++;
    ImmUnlockIMC(imc);
    ImmGenerateMessage(imc);
    idx = 0;
    do
    {
        msg = msg_spy_find_next_msg(WM_IME_ENDCOMPOSITION, &idx);
        if (msg) ok(!msg->post, "Message should not be posted\n");
    } while (msg);
    msg_spy_flush_msgs();

    ImmSetOpenStatus(imc, FALSE);
    ImmReleaseContext(hwnd, imc);
    DestroyWindow(hwnd);
}

static LRESULT CALLBACK processkey_wnd_proc( HWND hWnd, UINT msg, WPARAM wParam,
        LPARAM lParam )
{
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static void test_ime_processkey(void)
{
    MSG msg;
    WNDCLASSW wclass;
    HANDLE hInstance = GetModuleHandleW(NULL);
    TEST_INPUT inputs[2];
    HIMC imc;
    INT rc;
    HWND hWndTest;

    wclass.lpszClassName = L"ProcessKeyTestClass";
    wclass.style         = CS_HREDRAW | CS_VREDRAW;
    wclass.lpfnWndProc   = processkey_wnd_proc;
    wclass.hInstance     = hInstance;
    wclass.hIcon         = LoadIconW(0, (LPCWSTR)IDI_APPLICATION);
    wclass.hCursor       = LoadCursorW( NULL, (LPCWSTR)IDC_ARROW);
    wclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wclass.lpszMenuName  = 0;
    wclass.cbClsExtra    = 0;
    wclass.cbWndExtra    = 0;
    if(!RegisterClassW(&wclass)){
        win_skip("Failed to register window.\n");
        return;
    }

    /* create the test window that will receive the keystrokes */
    hWndTest = CreateWindowW(wclass.lpszClassName, L"ProcessKey",
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 100, 100,
                             NULL, NULL, hInstance, NULL);

    ShowWindow(hWndTest, SW_SHOW);
    SetWindowPos(hWndTest, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    SetForegroundWindow(hWndTest);
    UpdateWindow(hWndTest);

    imc = ImmGetContext(hWndTest);
    if (!imc)
    {
        win_skip("IME not supported\n");
        DestroyWindow(hWndTest);
        return;
    }

    rc = ImmSetOpenStatus(imc, TRUE);
    if (rc != TRUE)
    {
        win_skip("Unable to open IME\n");
        ImmReleaseContext(hWndTest, imc);
        DestroyWindow(hWndTest);
        return;
    }

    /* flush pending messages */
    while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageW(&msg);

    SetFocus(hWndTest);

    /* init input data that never changes */
    inputs[1].type = inputs[0].type = INPUT_KEYBOARD;
    inputs[1].u.ki.dwExtraInfo = inputs[0].u.ki.dwExtraInfo = 0;
    inputs[1].u.ki.time = inputs[0].u.ki.time = 0;

    /* Pressing a key */
    inputs[0].u.ki.wVk = 0x41;
    inputs[0].u.ki.wScan = 0x1e;
    inputs[0].u.ki.dwFlags = 0x0;

    pSendInput(1, (INPUT*)inputs, sizeof(INPUT));

    while(PeekMessageW(&msg, hWndTest, 0, 0, PM_NOREMOVE)) {
        if(msg.message != WM_KEYDOWN)
            PeekMessageW(&msg, hWndTest, 0, 0, PM_REMOVE);
        else
        {
            ok(msg.wParam != VK_PROCESSKEY,"Incorrect ProcessKey Found\n");
            PeekMessageW(&msg, hWndTest, 0, 0, PM_REMOVE);
            if(msg.wParam == VK_PROCESSKEY)
                trace("ProcessKey was correctly found\n");
        }
        TranslateMessage(&msg);
        /* test calling TranslateMessage multiple times */
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    inputs[0].u.ki.wVk = 0x41;
    inputs[0].u.ki.wScan = 0x1e;
    inputs[0].u.ki.dwFlags = KEYEVENTF_KEYUP;

    pSendInput(1, (INPUT*)inputs, sizeof(INPUT));

    while(PeekMessageW(&msg, hWndTest, 0, 0, PM_NOREMOVE)) {
        if(msg.message != WM_KEYUP)
            PeekMessageW(&msg, hWndTest, 0, 0, PM_REMOVE);
        else
        {
            ok(msg.wParam != VK_PROCESSKEY,"Incorrect ProcessKey Found\n");
            PeekMessageW(&msg, hWndTest, 0, 0, PM_REMOVE);
            ok(msg.wParam != VK_PROCESSKEY,"ProcessKey should still not be Found\n");
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    ImmReleaseContext(hWndTest, imc);
    ImmSetOpenStatus(imc, FALSE);
    DestroyWindow(hWndTest);
}

static void test_InvalidIMC(void)
{
    HIMC imc_destroy;
    HIMC imc_null = 0x00000000;
    HIMC imc_bad = (HIMC)0xdeadcafe;

    HIMC imc1, imc2, oldimc;
    DWORD ret;
    DWORD count;
    CHAR buffer[1000];
    INPUTCONTEXT *ic;
    LOGFONTA lf;
    BOOL r;

    memset(&lf, 0, sizeof(lf));

    imc_destroy = ImmCreateContext();
    ret = ImmDestroyContext(imc_destroy);
    ok(ret == TRUE, "Destroy an IMC should success!\n");

    /* Test associating destroyed imc */
    imc1 = ImmGetContext(hwnd);
    SetLastError(0xdeadbeef);
    oldimc = ImmAssociateContext(hwnd, imc_destroy);
    ok(!oldimc, "Associating to a destroyed imc should fail!\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    imc2 = ImmGetContext(hwnd);
    ok(imc1 == imc2, "imc should not changed! imc1 %p, imc2 %p\n", imc1, imc2);

    SET_ENABLE(WM_IME_SETCONTEXT_DEACTIVATE, TRUE);
    SET_ENABLE(WM_IME_SETCONTEXT_ACTIVATE, TRUE);
    r = ImmSetActiveContext(hwnd, imc_destroy, TRUE);
    ok(!r, "ImmSetActiveContext succeeded\n");
    SET_EXPECT(WM_IME_SETCONTEXT_DEACTIVATE);
    r = ImmSetActiveContext(hwnd, imc_destroy, FALSE);
    ok(r, "ImmSetActiveContext failed\n");
    CHECK_CALLED(WM_IME_SETCONTEXT_DEACTIVATE);
    SET_ENABLE(WM_IME_SETCONTEXT_ACTIVATE, FALSE);
    SET_ENABLE(WM_IME_SETCONTEXT_DEACTIVATE, FALSE);

    /* Test associating NULL imc, which is different from an invalid imc */
    oldimc = ImmAssociateContext(hwnd, imc_null);
    ok(oldimc != NULL, "Associating to NULL imc should success!\n");
    imc2 = ImmGetContext(hwnd);
    ok(!imc2, "expect NULL, returned %p\n", imc2);
    oldimc = ImmAssociateContext(hwnd, imc1);
    ok(!oldimc, "expect NULL, returned %p\n", oldimc);
    imc2 = ImmGetContext(hwnd);
    ok(imc2 == imc1, "imc should not changed! imc2 %p, imc1 %p\n", imc2, imc1);

    /* Test associating invalid imc */
    imc1 = ImmGetContext(hwnd);
    SetLastError(0xdeadbeef);
    oldimc = ImmAssociateContext(hwnd, imc_bad);
    ok(!oldimc, "Associating to a destroyed imc should fail!\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    imc2 = ImmGetContext(hwnd);
    ok(imc1 == imc2, "imc should not changed! imc1 %p, imc2 %p\n", imc1, imc2);


    /* Test ImmGetCandidateListA */
    SetLastError(0xdeadbeef);
    ret = ImmGetCandidateListA(imc_bad, 0, NULL, 0);
    ok(ret == 0, "Bad IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmGetCandidateListA(imc_null, 0, NULL, 0);
    ok(ret == 0, "NULL IME should return 0\n");
    ret = GetLastError();
    ok(ret == 0xdeadbeef, "last error should remain unchanged %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmGetCandidateListA(imc_destroy, 0, NULL, 0);
    ok(ret == 0, "Destroyed IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);

    /* Test ImmGetCandidateListCountA*/
    SetLastError(0xdeadbeef);
    ret = ImmGetCandidateListCountA(imc_bad,&count);
    ok(ret == 0, "Bad IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmGetCandidateListCountA(imc_null,&count);
    ok(ret == 0, "NULL IME should return 0\n");
    ret = GetLastError();
    ok(ret == 0xdeadbeef, "last error should remain unchanged %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmGetCandidateListCountA(imc_destroy,&count);
    ok(ret == 0, "Destroyed IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);

    /* Test ImmGetCandidateWindow */
    SetLastError(0xdeadbeef);
    ret = ImmGetCandidateWindow(imc_bad, 0, (LPCANDIDATEFORM)buffer);
    ok(ret == 0, "Bad IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmGetCandidateWindow(imc_null, 0, (LPCANDIDATEFORM)buffer);
    ok(ret == 0, "NULL IME should return 0\n");
    ret = GetLastError();
    ok(ret == 0xdeadbeef, "last error should remain unchanged %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmGetCandidateWindow(imc_destroy, 0, (LPCANDIDATEFORM)buffer);
    ok(ret == 0, "Destroyed IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);

    /* Test ImmGetCompositionFontA */
    SetLastError(0xdeadbeef);
    ret = ImmGetCompositionFontA(imc_bad, (LPLOGFONTA)buffer);
    ok(ret == 0, "Bad IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmGetCompositionFontA(imc_null, (LPLOGFONTA)buffer);
    ok(ret == 0, "NULL IME should return 0\n");
    ret = GetLastError();
    ok(ret == 0xdeadbeef, "last error should remain unchanged %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmGetCompositionFontA(imc_destroy, (LPLOGFONTA)buffer);
    ok(ret == 0, "Destroyed IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);

    /* Test ImmGetCompositionWindow */
    SetLastError(0xdeadbeef);
    ret = ImmGetCompositionWindow(imc_bad, (LPCOMPOSITIONFORM)buffer);
    ok(ret == 0, "Bad IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmGetCompositionWindow(imc_null, (LPCOMPOSITIONFORM)buffer);
    ok(ret == 0, "NULL IME should return 0\n");
    ret = GetLastError();
    ok(ret == 0xdeadbeef, "last error should remain unchanged %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmGetCompositionWindow(imc_destroy, (LPCOMPOSITIONFORM)buffer);
    ok(ret == 0, "Destroyed IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);

    /* Test ImmGetCompositionStringA */
    SetLastError(0xdeadbeef);
    ret = ImmGetCompositionStringA(imc_bad, GCS_COMPSTR, NULL, 0);
    ok(ret == 0, "Bad IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmGetCompositionStringA(imc_null, GCS_COMPSTR, NULL, 0);
    ok(ret == 0, "NULL IME should return 0\n");
    ret = GetLastError();
    ok(ret == 0xdeadbeef, "last error should remain unchanged %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmGetCompositionStringA(imc_destroy, GCS_COMPSTR, NULL, 0);
    ok(ret == 0, "Destroyed IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);

    /* Test ImmSetOpenStatus */
    SetLastError(0xdeadbeef);
    ret = ImmSetOpenStatus(imc_bad, 1);
    ok(ret == 0, "Bad IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmSetOpenStatus(imc_null, 1);
    ok(ret == 0, "NULL IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmSetOpenStatus(imc_destroy, 1);
    ok(ret == 0, "Destroyed IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);

    /* Test ImmGetOpenStatus */
    SetLastError(0xdeadbeef);
    ret = ImmGetOpenStatus(imc_bad);
    ok(ret == 0, "Bad IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmGetOpenStatus(imc_null);
    ok(ret == 0, "NULL IME should return 0\n");
    ret = GetLastError();
    ok(ret == 0xdeadbeef, "last error should remain unchanged %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmGetOpenStatus(imc_destroy);
    ok(ret == 0, "Destroyed IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);

    /* Test ImmGetStatusWindowPos */
    SetLastError(0xdeadbeef);
    ret = ImmGetStatusWindowPos(imc_bad, NULL);
    ok(ret == 0, "Bad IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmGetStatusWindowPos(imc_null, NULL);
    ok(ret == 0, "NULL IME should return 0\n");
    ret = GetLastError();
    ok(ret == 0xdeadbeef, "last error should remain unchanged %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmGetStatusWindowPos(imc_destroy, NULL);
    ok(ret == 0, "Destroyed IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);

    /* Test ImmRequestMessageA */
    SetLastError(0xdeadbeef);
    ret = ImmRequestMessageA(imc_bad, WM_CHAR, 0);
    ok(ret == 0, "Bad IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmRequestMessageA(imc_null, WM_CHAR, 0);
    ok(ret == 0, "NULL IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmRequestMessageA(imc_destroy, WM_CHAR, 0);
    ok(ret == 0, "Destroyed IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);

    /* Test ImmSetCompositionFontA */
    SetLastError(0xdeadbeef);
    ret = ImmSetCompositionFontA(imc_bad, &lf);
    ok(ret == 0, "Bad IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmSetCompositionFontA(imc_null, &lf);
    ok(ret == 0, "NULL IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmSetCompositionFontA(imc_destroy, &lf);
    ok(ret == 0, "Destroyed IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);

    /* Test ImmSetCompositionWindow */
    SetLastError(0xdeadbeef);
    ret = ImmSetCompositionWindow(imc_bad, NULL);
    ok(ret == 0, "Bad IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmSetCompositionWindow(imc_null, NULL);
    ok(ret == 0, "NULL IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmSetCompositionWindow(imc_destroy, NULL);
    ok(ret == 0, "Destroyed IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);

    /* Test ImmSetConversionStatus */
    SetLastError(0xdeadbeef);
    ret = ImmSetConversionStatus(imc_bad, 0, 0);
    ok(ret == 0, "Bad IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmSetConversionStatus(imc_null, 0, 0);
    ok(ret == 0, "NULL IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmSetConversionStatus(imc_destroy, 0, 0);
    ok(ret == 0, "Destroyed IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);

    /* Test ImmSetStatusWindowPos */
    SetLastError(0xdeadbeef);
    ret = ImmSetStatusWindowPos(imc_bad, 0);
    ok(ret == 0, "Bad IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmSetStatusWindowPos(imc_null, 0);
    ok(ret == 0, "NULL IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmSetStatusWindowPos(imc_destroy, 0);
    ok(ret == 0, "Destroyed IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);

    /* Test ImmGetImeMenuItemsA */
    SetLastError(0xdeadbeef);
    ret = ImmGetImeMenuItemsA(imc_bad, 0, 0, NULL, NULL, 0);
    ok(ret == 0, "Bad IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmGetImeMenuItemsA(imc_null, 0, 0, NULL, NULL, 0);
    ok(ret == 0, "NULL IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmGetImeMenuItemsA(imc_destroy, 0, 0, NULL, NULL, 0);
    ok(ret == 0, "Destroyed IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);

    /* Test ImmLockIMC */
    SetLastError(0xdeadbeef);
    ic = ImmLockIMC(imc_bad);
    ok(ic == 0, "Bad IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ic = ImmLockIMC(imc_null);
    ok(ic == 0, "NULL IME should return 0\n");
    ret = GetLastError();
    ok(ret == 0xdeadbeef, "last error should remain unchanged %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ic = ImmLockIMC(imc_destroy);
    ok(ic == 0, "Destroyed IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);

    /* Test ImmUnlockIMC */
    SetLastError(0xdeadbeef);
    ret = ImmUnlockIMC(imc_bad);
    ok(ret == 0, "Bad IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmUnlockIMC(imc_null);
    ok(ret == 0, "NULL IME should return 0\n");
    ret = GetLastError();
    ok(ret == 0xdeadbeef, "last error should remain unchanged %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmUnlockIMC(imc_destroy);
    ok(ret == 0, "Destroyed IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);

    /* Test ImmGenerateMessage */
    SetLastError(0xdeadbeef);
    ret = ImmGenerateMessage(imc_bad);
    ok(ret == 0, "Bad IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmGenerateMessage(imc_null);
    ok(ret == 0, "NULL IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
    SetLastError(0xdeadbeef);
    ret = ImmGenerateMessage(imc_destroy);
    ok(ret == 0, "Destroyed IME should return 0\n");
    ret = GetLastError();
    ok(ret == ERROR_INVALID_HANDLE, "wrong last error %08lx!\n", ret);
}

#define test_apttype(apttype) _test_apttype(apttype, __LINE__)
static void _test_apttype(APTTYPE apttype, unsigned int line)
{
    APTTYPEQUALIFIER qualifier;
    HRESULT hr, hr_expected;
    APTTYPE type;

    hr = CoGetApartmentType(&type, &qualifier);
    hr_expected = (apttype == -1 ? CO_E_NOTINITIALIZED : S_OK);
    ok_(__FILE__, line)(hr == hr_expected, "CoGetApartmentType returned %lx\n", hr);
    if (FAILED(hr))
        return;

    ok_(__FILE__, line)(type == apttype, "type %x\n", type);
    ok_(__FILE__, line)(!qualifier, "qualifier %x\n", qualifier);
}

static DWORD WINAPI com_initialization_thread(void *arg)
{
    HRESULT hr;
    BOOL r;

    test_apttype(-1);
    ImmDisableIME(GetCurrentThreadId());
    r = ImmSetActiveContext(NULL, NULL, TRUE);
    ok(r, "ImmSetActiveContext failed\n");
    test_apttype(APTTYPE_MAINSTA);
    hr = CoInitialize(NULL);
    ok(hr == S_OK, "CoInitialize returned %lx\n", hr);
    CoUninitialize();
    test_apttype(-1);

    /* Changes IMM behavior so it no longer initialized COM */
    r = ImmSetActiveContext(NULL, NULL, TRUE);
    ok(r, "ImmSetActiveContext failed\n");
    test_apttype(APTTYPE_MAINSTA);
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    ok(hr == S_OK, "CoInitialize returned %lx\n", hr);
    test_apttype(APTTYPE_MTA);
    CoUninitialize();
    test_apttype(-1);
    r = ImmSetActiveContext(NULL, NULL, TRUE);
    ok(r, "ImmSetActiveContext failed\n");
    test_apttype(-1);
    return 0;
}

static void test_com_initialization(void)
{
    APTTYPEQUALIFIER qualifier;
    HANDLE thread;
    APTTYPE type;
    HRESULT hr;
    HWND wnd;
    BOOL r;

    thread = CreateThread(NULL, 0, com_initialization_thread, NULL, 0, NULL);
    ok(thread != NULL, "CreateThread failed\n");
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    test_apttype(-1);
    r = ImmSetActiveContext(NULL, (HIMC)0xdeadbeef, TRUE);
    ok(!r, "ImmSetActiveContext succeeded\n");
    test_apttype(-1);

    r = ImmSetActiveContext(NULL, NULL, TRUE);
    ok(r, "ImmSetActiveContext failed\n");
    test_apttype(APTTYPE_MAINSTA);

    /* Force default IME window destruction */
    wnd = CreateWindowA("static", "static", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ok(wnd != NULL, "CreateWindow failed\n");
    DestroyWindow(wnd);
    test_apttype(-1);

    r = ImmSetActiveContext(NULL, NULL, TRUE);
    ok(r, "ImmSetActiveContext failed\n");
    test_apttype(APTTYPE_MAINSTA);
    hr = CoInitialize(NULL);
    ok(hr == S_OK, "CoInitialize returned %lx\n", hr);
    CoUninitialize();
    test_apttype(-1);

    /* Test with default IME window created */
    wnd = CreateWindowA("static", "static", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ok(wnd != NULL, "CreateWindow failed\n");
    test_apttype(-1);
    r = ImmSetActiveContext(NULL, NULL, TRUE);
    ok(r, "ImmSetActiveContext failed\n");
    test_apttype(APTTYPE_MAINSTA);
    hr = CoInitialize(NULL);
    ok(hr == S_OK, "CoInitialize returned %lx\n", hr);
    CoUninitialize();
    test_apttype(APTTYPE_MAINSTA);
    DestroyWindow(wnd);
    test_apttype(-1);

    wnd = CreateWindowA("static", "static", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ok(wnd != NULL, "CreateWindow failed\n");
    r = ImmSetActiveContext(NULL, NULL, TRUE);
    CoUninitialize();
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    ok(hr == S_OK, "CoInitialize returned %lx\n", hr);
    test_apttype(APTTYPE_MTA);
    DestroyWindow(wnd);

    hr = CoGetApartmentType(&type, &qualifier);
    ok(hr == CO_E_NOTINITIALIZED || broken(hr == S_OK) /* w10v22H2 */,
       "CoGetApartmentType returned %#lx\n", hr);
    test_apttype(hr == S_OK ? APTTYPE_MTA : -1);

    wnd = CreateWindowA("static", "static", WS_POPUP, 0, 0, 100, 100, 0, 0, 0, 0);
    ok(wnd != NULL, "CreateWindow failed\n");
    test_apttype(hr == S_OK ? APTTYPE_MTA : -1);
    ShowWindow(wnd, SW_SHOW);
    test_apttype(hr == S_OK ? APTTYPE_MTA : APTTYPE_MAINSTA);
    DestroyWindow(wnd);
    test_apttype(-1);
}

static DWORD WINAPI disable_ime_thread(void *arg)
{
    HWND h, def;
    MSG msg;
    BOOL r;

    h = CreateWindowA("static", "static", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ok(h != NULL, "CreateWindow failed\n");
    def = ImmGetDefaultIMEWnd(h);
    ok(def != NULL, "ImmGetDefaultIMEWnd returned NULL\n");

    r = ImmDisableIME(arg ? GetCurrentThreadId() : 0);
    ok(r, "ImmDisableIME failed\n");

    if (arg)
    {
        def = ImmGetDefaultIMEWnd(h);
        todo_wine ok(def != NULL, "ImmGetDefaultIMEWnd returned NULL\n");
        while(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);
    }
    def = ImmGetDefaultIMEWnd(h);
    ok(!def, "ImmGetDefaultIMEWnd returned %p\n", def);
    return 0;
}

static DWORD WINAPI check_not_disabled_ime_thread(void *arg)
{
    HWND def, hwnd;

    WaitForSingleObject(arg, INFINITE);
    hwnd = CreateWindowA("static", "static", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ok(hwnd != NULL, "CreateWindow failed\n");
    def = ImmGetDefaultIMEWnd(hwnd);
    ok(def != NULL, "ImmGetDefaultIMEWnd returned %p\n", def);
    return 0;
}

static DWORD WINAPI disable_ime_process(void *arg)
{
    BOOL r = ImmDisableIME(-1);
    ok(r, "ImmDisableIME failed\n");
    return 0;
}

static void test_ImmDisableIME(void)
{
    HANDLE thread, event;
    DWORD tid;
    HWND def, def2;
    BOOL r;

    def = ImmGetDefaultIMEWnd(hwnd);
    ok(def != NULL, "ImmGetDefaultIMEWnd(hwnd) returned NULL\n");

    event = CreateEventW(NULL, TRUE, FALSE, FALSE);
    thread = CreateThread(NULL, 0, check_not_disabled_ime_thread, event, 0, &tid);
    ok(thread != NULL, "CreateThread failed\n");
    r = ImmDisableIME(tid);
    ok(!r, "ImmDisableIME(tid) succeeded\n");
    SetEvent(event);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    CloseHandle(event);

    thread = CreateThread(NULL, 0, disable_ime_thread, 0, 0, NULL);
    ok(thread != NULL, "CreateThread failed\n");
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    thread = CreateThread(NULL, 0, disable_ime_thread, (void*)1, 0, NULL);
    ok(thread != NULL, "CreateThread failed\n");
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    msg_spy_pump_msg_queue();
    thread = CreateThread(NULL, 0, disable_ime_process, 0, 0, NULL);
    ok(thread != NULL, "CreateThread failed\n");
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    ok(IsWindow(def), "not a window\n");
    def2 = ImmGetDefaultIMEWnd(hwnd);
    ok(def2 == def, "ImmGetDefaultIMEWnd(hwnd) returned %p\n", def2);
    ok(IsWindow(def), "not a window\n");
    msg_spy_pump_msg_queue();
    ok(!IsWindow(def), "window is still valid\n");
    def = ImmGetDefaultIMEWnd(hwnd);
    ok(!def, "ImmGetDefaultIMEWnd(hwnd) returned %p\n", def);

    r = ImmDisableIME(-1);
    ok(r, "ImmDisableIME(-1) failed\n");
    def = ImmGetDefaultIMEWnd(hwnd);
    ok(!def, "ImmGetDefaultIMEWnd(hwnd) returned %p\n", def);
}

#define ime_trace( msg, ... ) if (winetest_debug > 1) trace( "%04lx:%s " msg, GetCurrentThreadId(), __func__, ## __VA_ARGS__ )

static BOOL ImeSelect_init_status;
static BOOL todo_ImeInquire;
DEFINE_EXPECT( ImeInquire );
static BOOL todo_ImeDestroy;
DEFINE_EXPECT( ImeDestroy );
DEFINE_EXPECT( ImeEscape );
DEFINE_EXPECT( ImeEnumRegisterWord );
DEFINE_EXPECT( ImeRegisterWord );
DEFINE_EXPECT( ImeGetRegisterWordStyle );
DEFINE_EXPECT( ImeUnregisterWord );
static BOOL todo_IME_DLL_PROCESS_ATTACH;
DEFINE_EXPECT( IME_DLL_PROCESS_ATTACH );
static BOOL todo_IME_DLL_PROCESS_DETACH;
DEFINE_EXPECT( IME_DLL_PROCESS_DETACH );

static IMEINFO ime_info;
static UINT ime_count;
static WCHAR ime_path[MAX_PATH];
static HIMC default_himc;
static HKL default_hkl;
static HKL expect_ime = (HKL)(int)0xe020047f;

enum ime_function
{
    IME_SELECT = 1,
    IME_NOTIFY,
    IME_PROCESS_KEY,
    IME_SET_ACTIVE_CONTEXT,
    MSG_IME_UI,
    MSG_TEST_WIN,
};

struct ime_call
{
    HKL hkl;
    HIMC himc;
    enum ime_function func;

    union
    {
        int select;
        struct
        {
            int action;
            int index;
            int value;
        } notify;
        struct
        {
            WORD vkey;
            LPARAM key_data;
        } process_key;
        struct
        {
            int flag;
        } set_active_context;
        struct
        {
            UINT msg;
            WPARAM wparam;
            LPARAM lparam;
        } message;
    };

    BOOL todo;
    BOOL broken;
    BOOL flaky_himc;
};

struct ime_call empty_sequence[] = {{0}};
static struct ime_call ime_calls[1024];
static ULONG ime_call_count;

#define ok_call( a, b ) ok_call_( __FILE__, __LINE__, a, b )
static int ok_call_( const char *file, int line, const struct ime_call *expected, const struct ime_call *received )
{
    int ret;

    if ((ret = expected->func - received->func)) goto done;
    /* Wine doesn't allocate HIMC in a deterministic order, ignore them when they are enumerated */
    if (expected->flaky_himc && (ret = !!(UINT_PTR)expected->himc - !!(UINT_PTR)received->himc)) goto done;
    if (!expected->flaky_himc && (ret = (UINT_PTR)expected->himc - (UINT_PTR)received->himc)) goto done;
    if ((ret = (UINT)(UINT_PTR)expected->hkl - (UINT)(UINT_PTR)received->hkl)) goto done;
    switch (expected->func)
    {
    case IME_SELECT:
        if ((ret = expected->select - received->select)) goto done;
        break;
    case IME_NOTIFY:
        if ((ret = expected->notify.action - received->notify.action)) goto done;
        if ((ret = expected->notify.index - received->notify.index)) goto done;
        if ((ret = expected->notify.value - received->notify.value)) goto done;
        break;
    case IME_PROCESS_KEY:
        if ((ret = expected->process_key.vkey - received->process_key.vkey)) goto done;
        if ((ret = expected->process_key.key_data - received->process_key.key_data)) goto done;
        break;
    case IME_SET_ACTIVE_CONTEXT:
        if ((ret = expected->set_active_context.flag - received->set_active_context.flag)) goto done;
        break;
    case MSG_IME_UI:
    case MSG_TEST_WIN:
        if ((ret = expected->message.msg - received->message.msg)) goto done;
        if ((ret = (expected->message.wparam - received->message.wparam))) goto done;
        if ((ret = (expected->message.lparam - received->message.lparam))) goto done;
        break;
    }

done:
    if (ret && broken( expected->broken )) return ret;

    switch (received->func)
    {
    case IME_SELECT:
        todo_wine_if( expected->todo )
        ok_(file, line)( !ret, "got hkl %p, himc %p, IME_SELECT select %u\n", received->hkl, received->himc, received->select );
        return ret;
    case IME_NOTIFY:
        todo_wine_if( expected->todo )
        ok_(file, line)( !ret, "got hkl %p, himc %p, IME_NOTIFY action %#x, index %#x, value %#x\n",
                         received->hkl, received->himc, received->notify.action, received->notify.index,
                         received->notify.value );
        return ret;
    case IME_PROCESS_KEY:
        todo_wine_if( expected->todo )
        ok_(file, line)( !ret, "got hkl %p, himc %p, IME_PROCESS_KEY vkey %#x, key_data %#Ix\n",
                         received->hkl, received->himc, received->process_key.vkey, received->process_key.key_data );
        return ret;
    case IME_SET_ACTIVE_CONTEXT:
        todo_wine_if( expected->todo )
        ok_(file, line)( !ret, "got hkl %p, himc %p, IME_SET_ACTIVE_CONTEXT flag %u\n", received->hkl, received->himc,
                         received->set_active_context.flag );
        return ret;
    case MSG_IME_UI:
        todo_wine_if( expected->todo )
        ok_(file, line)( !ret, "got hkl %p, himc %p, MSG_IME_UI msg %#x, wparam %#Ix, lparam %#Ix\n", received->hkl,
                         received->himc, received->message.msg, received->message.wparam, received->message.lparam );
        return ret;
    case MSG_TEST_WIN:
        ok_(file, line)( !ret, "got hkl %p, himc %p, MSG_TEST_WIN msg %#x, wparam %#Ix, lparam %#Ix\n", received->hkl,
                         received->himc, received->message.msg, received->message.wparam, received->message.lparam );
        return ret;
    }

    switch (expected->func)
    {
    case IME_SELECT:
        todo_wine_if( expected->todo )
        ok_(file, line)( !ret, "hkl %p, himc %p, IME_SELECT select %u\n", expected->hkl, expected->himc, expected->select );
        break;
    case IME_NOTIFY:
        todo_wine_if( expected->todo )
        ok_(file, line)( !ret, "hkl %p, himc %p, IME_NOTIFY action %#x, index %#x, value %#x\n",
                         expected->hkl, expected->himc, expected->notify.action, expected->notify.index,
                         expected->notify.value );
        break;
    case IME_PROCESS_KEY:
        todo_wine_if( expected->todo )
        ok_(file, line)( !ret, "hkl %p, himc %p, IME_PROCESS_KEY vkey %#x, key_data %#Ix\n",
                         expected->hkl, expected->himc, expected->process_key.vkey, expected->process_key.key_data );
        break;
    case IME_SET_ACTIVE_CONTEXT:
        todo_wine_if( expected->todo )
        ok_(file, line)( !ret, "hkl %p, himc %p, IME_SET_ACTIVE_CONTEXT flag %u\n", expected->hkl, expected->himc,
                         expected->set_active_context.flag );
        break;
    case MSG_IME_UI:
        todo_wine_if( expected->todo )
        ok_(file, line)( !ret, "hkl %p, himc %p, MSG_IME_UI msg %#x, wparam %#Ix, lparam %#Ix\n", expected->hkl,
                         expected->himc, expected->message.msg, expected->message.wparam, expected->message.lparam );
        break;
    case MSG_TEST_WIN:
        ok_(file, line)( !ret, "hkl %p, himc %p, MSG_TEST_WIN msg %#x, wparam %#Ix, lparam %#Ix\n", expected->hkl,
                         expected->himc, expected->message.msg, expected->message.wparam, expected->message.lparam );
        break;
    }

    return 0;
}

#define ok_seq( a ) ok_seq_( __FILE__, __LINE__, a, #a )
static void ok_seq_( const char *file, int line, const struct ime_call *expected, const char *context )
{
    const struct ime_call *received = ime_calls;
    UINT i = 0, ret;

    while (expected->func || received->func)
    {
        winetest_push_context( "%u%s%s", i++, !expected->func ? " (spurious)" : "",
                               !received->func ? " (missing)" : "" );
        ret = ok_call_( file, line, expected, received );
        if (ret && expected->todo && !strcmp( winetest_platform, "wine" ))
            expected++;
        else if (ret && broken(expected->broken))
            expected++;
        else
        {
            if (expected->func) expected++;
            if (received->func) received++;
        }
        winetest_pop_context();
    }

    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;
}

static BOOL ignore_message( UINT msg )
{
    switch (msg)
    {
    case WM_IME_STARTCOMPOSITION:
    case WM_IME_ENDCOMPOSITION:
    case WM_IME_COMPOSITION:
    case WM_IME_SETCONTEXT:
    case WM_IME_NOTIFY:
    case WM_IME_CONTROL:
    case WM_IME_COMPOSITIONFULL:
    case WM_IME_SELECT:
    case WM_IME_CHAR:
    case 0x287:
    case WM_IME_REQUEST:
    case WM_IME_KEYDOWN:
    case WM_IME_KEYUP:
        return FALSE;
    default:
        return TRUE;
    }
}

static LRESULT CALLBACK ime_ui_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    struct ime_call call =
    {
        .hkl = GetKeyboardLayout( 0 ), .himc = (HIMC)GetWindowLongPtrW( hwnd, IMMGWL_IMC ),
        .func = MSG_IME_UI, .message = {.msg = msg, .wparam = wparam, .lparam = lparam}
    };
    LONG_PTR ptr;

    ime_trace( "hwnd %p, msg %#x, wparam %#Ix, lparam %#Ix\n", hwnd, msg, wparam, lparam );

    if (ignore_message( msg )) return DefWindowProcW( hwnd, msg, wparam, lparam );

    ptr = GetWindowLongPtrW( hwnd, IMMGWL_PRIVATE );
    ok( !ptr, "got IMMGWL_PRIVATE %#Ix\n", ptr );

    ime_calls[ime_call_count++] = call;
    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static LRESULT CALLBACK test_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    struct ime_call call =
    {
        .hkl = GetKeyboardLayout( 0 ), .himc = ImmGetContext( hwnd ),
        .func = MSG_TEST_WIN, .message = {.msg = msg, .wparam = wparam, .lparam = lparam}
    };

    ime_trace( "hwnd %p, msg %#x, wparam %#Ix, lparam %#Ix\n", hwnd, msg, wparam, lparam );

    if (ignore_message( msg )) return DefWindowProcW( hwnd, msg, wparam, lparam );

    ime_calls[ime_call_count++] = call;
    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static WNDCLASSEXW ime_ui_class =
{
    .cbSize = sizeof(WNDCLASSEXW),
    .style = CS_IME,
    .lpfnWndProc = ime_ui_window_proc,
    .cbWndExtra = 2 * sizeof(LONG_PTR),
    .lpszClassName = L"WineTestIME",
};

static WNDCLASSEXW test_class =
{
    .cbSize = sizeof(WNDCLASSEXW),
    .lpfnWndProc = test_window_proc,
    .lpszClassName = L"WineTest",
};

static BOOL WINAPI ime_ImeConfigure( HKL hkl, HWND hwnd, DWORD mode, void *data )
{
    ime_trace( "hkl %p, hwnd %p, mode %lu, data %p\n", hkl, hwnd, mode, data );
    ok( 0, "unexpected call\n" );
    return FALSE;
}

static DWORD WINAPI ime_ImeConversionList( HIMC himc, const WCHAR *source, CANDIDATELIST *dest,
                                           DWORD dest_len, UINT flag )
{
    ime_trace( "himc %p, source %s, dest %p, dest_len %lu, flag %#x\n",
               himc, debugstr_w(source), dest, dest_len, flag );
    ok( 0, "unexpected call\n" );
    return 0;
}

static BOOL WINAPI ime_ImeDestroy( UINT force )
{
    ime_trace( "force %u\n", force );

    todo_wine_if( todo_ImeDestroy )
    CHECK_EXPECT( ImeDestroy );

    ok( !force, "got force %u\n", force );

    return TRUE;
}

static UINT WINAPI ime_ImeEnumRegisterWord( REGISTERWORDENUMPROCW proc, const WCHAR *reading, DWORD style,
                                            const WCHAR *string, void *data )
{
    ime_trace( "proc %p, reading %s, style %lu, string %s, data %p\n",
               proc, debugstr_w(reading), style, debugstr_w(string), data );

    ok_eq( default_hkl, GetKeyboardLayout( 0 ), HKL, "%p" );
    CHECK_EXPECT( ImeEnumRegisterWord );

    if (!style)
    {
        ok_eq( 0, reading, const void *, "%p" );
        ok_eq( 0, string, const void *, "%p" );
    }
    else if (ime_info.fdwProperty & IME_PROP_UNICODE)
    {
        ok_eq( 0xdeadbeef, style, UINT, "%#x" );
        ok_wcs( L"Reading", reading );
        ok_wcs( L"String", string );
    }
    else
    {
        ok_eq( 0xdeadbeef, style, UINT, "%#x" );
        ok_str( "Reading", (char *)reading );
        ok_str( "String", (char *)string );
    }

    if (style) return proc( reading, style, string, data );
    return 0;
}

static LRESULT WINAPI ime_ImeEscape( HIMC himc, UINT escape, void *data )
{
    ime_trace( "himc %p, escape %#x, data %p\n", himc, escape, data );

    ok_eq( default_hkl, GetKeyboardLayout( 0 ), HKL, "%p" );
    CHECK_EXPECT( ImeEscape );

    switch (escape)
    {
    case IME_ESC_SET_EUDC_DICTIONARY:
        if (!data) return 4;
        if (ime_info.fdwProperty & IME_PROP_UNICODE)
            ok_wcs( L"EscapeIme", data );
        else
            ok_str( "EscapeIme", data );
        /* fallthrough */
    case IME_ESC_QUERY_SUPPORT:
    case IME_ESC_SEQUENCE_TO_INTERNAL:
    case IME_ESC_GET_EUDC_DICTIONARY:
    case IME_ESC_MAX_KEY:
    case IME_ESC_IME_NAME:
    case IME_ESC_HANJA_MODE:
    case IME_ESC_GETHELPFILENAME:
        if (!data) return 4;
        if (ime_info.fdwProperty & IME_PROP_UNICODE) wcscpy( data, L"ImeEscape" );
        else strcpy( data, "ImeEscape" );
        return 4;
    }

    ok_eq( 0xdeadbeef, escape, UINT, "%#x" );
    ok_eq( NULL, data, void *, "%p" );

    return TRUE;
}

static DWORD WINAPI ime_ImeGetImeMenuItems( HIMC himc, DWORD flags, DWORD type, IMEMENUITEMINFOW *parent,
                                            IMEMENUITEMINFOW *menu, DWORD size )
{
    ime_trace( "himc %p, flags %#lx, type %lu, parent %p, menu %p, size %#lx\n",
               himc, flags, type, parent, menu, size );
    ok( 0, "unexpected call\n" );
    return 0;
}

static UINT WINAPI ime_ImeGetRegisterWordStyle( UINT item, STYLEBUFW *style )
{
    ime_trace( "item %u, style %p\n", item, style );

    ok_eq( default_hkl, GetKeyboardLayout( 0 ), HKL, "%p" );
    CHECK_EXPECT( ImeGetRegisterWordStyle );

    if (!style)
        ok_eq( 16, item, UINT, "%u" );
    else if (ime_info.fdwProperty & IME_PROP_UNICODE)
    {
        STYLEBUFW *styleW = style;
        styleW->dwStyle = 0xdeadbeef;
        wcscpy( styleW->szDescription, L"StyleDescription" );
    }
    else
    {
        STYLEBUFA *styleA = (STYLEBUFA *)style;
        styleA->dwStyle = 0xdeadbeef;
        strcpy( styleA->szDescription, "StyleDescription" );
    }

    return 0xdeadbeef;
}

static BOOL WINAPI ime_ImeInquire( IMEINFO *info, WCHAR *ui_class, DWORD flags )
{
    ime_trace( "info %p, ui_class %p, flags %#lx\n", info, ui_class, flags );

    todo_wine_if( todo_ImeInquire )
    CHECK_EXPECT( ImeInquire );

    ok( !!info, "got info %p\n", info );
    ok( !!ui_class, "got ui_class %p\n", ui_class );
    ok( !flags, "got flags %#lx\n", flags );

    *info = ime_info;

    if (ime_info.fdwProperty & IME_PROP_UNICODE)
        wcscpy( ui_class, ime_ui_class.lpszClassName );
    else
        WideCharToMultiByte( CP_ACP, 0, ime_ui_class.lpszClassName, -1,
                             (char *)ui_class, 17, NULL, NULL );

    return TRUE;
}

static BOOL WINAPI ime_ImeProcessKey( HIMC himc, UINT vkey, LPARAM key_data, BYTE *key_state )
{
    struct ime_call call =
    {
        .hkl = GetKeyboardLayout( 0 ), .himc = himc,
        .func = IME_PROCESS_KEY, .process_key = {.vkey = vkey, .key_data = key_data}
    };
    ime_trace( "himc %p, vkey %u, key_data %#Ix, key_state %p\n",
               himc, vkey, key_data, key_state );
    ime_calls[ime_call_count++] = call;
    return TRUE;
}

static BOOL WINAPI ime_ImeRegisterWord( const WCHAR *reading, DWORD style, const WCHAR *string )
{
    ime_trace( "reading %s, style %lu, string %s\n", debugstr_w(reading), style, debugstr_w(string) );

    ok_eq( default_hkl, GetKeyboardLayout( 0 ), HKL, "%p" );
    CHECK_EXPECT( ImeRegisterWord );

    if (style) ok_eq( 0xdeadbeef, style, UINT, "%#x" );
    if (ime_info.fdwProperty & IME_PROP_UNICODE)
    {
        if (reading) ok_wcs( L"Reading", reading );
        if (string) ok_wcs( L"String", string );
    }
    else
    {
        if (reading) ok_str( "Reading", (char *)reading );
        if (string) ok_str( "String", (char *)string );
    }

    return FALSE;
}

static BOOL WINAPI ime_ImeSelect( HIMC himc, BOOL select )
{
    struct ime_call call =
    {
        .hkl = GetKeyboardLayout( 0 ), .himc = himc,
        .func = IME_SELECT, .select = select
    };
    INPUTCONTEXT *ctx;

    ime_trace( "himc %p, select %d\n", himc, select );
    ime_calls[ime_call_count++] = call;

    if (ImeSelect_init_status && select)
    {
        ctx = ImmLockIMC( himc );
        ok_ne( NULL, ctx, INPUTCONTEXT *, "%p" );
        ctx->fOpen = ~0;
        ctx->fdwConversion = ~0;
        ctx->fdwSentence = ~0;
        ImmUnlockIMC( himc );
    }

    return TRUE;
}

static BOOL WINAPI ime_ImeSetActiveContext( HIMC himc, BOOL flag )
{
    struct ime_call call =
    {
        .hkl = GetKeyboardLayout( 0 ), .himc = himc,
        .func = IME_SET_ACTIVE_CONTEXT, .set_active_context = {.flag = flag}
    };
    ime_trace( "himc %p, flag %#x\n", himc, flag );
    ime_calls[ime_call_count++] = call;
    return TRUE;
}

static BOOL WINAPI ime_ImeSetCompositionString( HIMC himc, DWORD index, const void *comp, DWORD comp_len,
                                                const void *read, DWORD read_len )
{
    ime_trace( "himc %p, index %lu, comp %p, comp_len %lu, read %p, read_len %lu\n",
               himc, index, comp, comp_len, read, read_len );
    ok( 0, "unexpected call\n" );
    return FALSE;
}

static UINT WINAPI ime_ImeToAsciiEx( UINT vkey, UINT scan_code, BYTE *key_state, TRANSMSGLIST *msgs, UINT state, HIMC himc )
{
    ime_trace( "vkey %u, scan_code %u, key_state %p, msgs %p, state %u, himc %p\n",
           vkey, scan_code, key_state, msgs, state, himc );
    return 0;
}

static BOOL WINAPI ime_ImeUnregisterWord( const WCHAR *reading, DWORD style, const WCHAR *string )
{
    ime_trace( "reading %s, style %lu, string %s\n", debugstr_w(reading), style, debugstr_w(string) );

    ok_eq( default_hkl, GetKeyboardLayout( 0 ), HKL, "%p" );
    CHECK_EXPECT( ImeUnregisterWord );

    if (style) ok_eq( 0xdeadbeef, style, UINT, "%#x" );
    if (ime_info.fdwProperty & IME_PROP_UNICODE)
    {
        if (reading) ok_wcs( L"Reading", reading );
        if (string) ok_wcs( L"String", string );
    }
    else
    {
        if (reading) ok_str( "Reading", (char *)reading );
        if (string) ok_str( "String", (char *)string );
    }

    return FALSE;
}

static BOOL WINAPI ime_NotifyIME( HIMC himc, DWORD action, DWORD index, DWORD value )
{
    struct ime_call call =
    {
        .hkl = GetKeyboardLayout( 0 ), .himc = himc,
        .func = IME_NOTIFY, .notify = {.action = action, .index = index, .value = value}
    };
    ime_trace( "himc %p, action %#lx, index %lu, value %lu\n", himc, action, index, value );
    ime_calls[ime_call_count++] = call;
    return FALSE;
}

static BOOL WINAPI ime_DllMain( HINSTANCE instance, DWORD reason, LPVOID reserved )
{
    ime_trace( "instance %p, reason %lu, reserved %p.\n", instance, reason, reserved );

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( instance );
        ime_ui_class.hInstance = instance;
        RegisterClassExW( &ime_ui_class );
        todo_wine_if(todo_IME_DLL_PROCESS_ATTACH)
        CHECK_EXPECT( IME_DLL_PROCESS_ATTACH );
        break;

    case DLL_PROCESS_DETACH:
        UnregisterClassW( ime_ui_class.lpszClassName, instance );
        todo_wine_if(todo_IME_DLL_PROCESS_DETACH)
        CHECK_EXPECT( IME_DLL_PROCESS_DETACH );
        break;
    }

    return TRUE;
}

static struct ime_functions ime_functions =
{
    ime_ImeConfigure,
    ime_ImeConversionList,
    ime_ImeDestroy,
    ime_ImeEnumRegisterWord,
    ime_ImeEscape,
    ime_ImeGetImeMenuItems,
    ime_ImeGetRegisterWordStyle,
    ime_ImeInquire,
    ime_ImeProcessKey,
    ime_ImeRegisterWord,
    ime_ImeSelect,
    ime_ImeSetActiveContext,
    ime_ImeSetCompositionString,
    ime_ImeToAsciiEx,
    ime_ImeUnregisterWord,
    ime_NotifyIME,
    ime_DllMain,
};

static HKL ime_install(void)
{
    WCHAR buffer[MAX_PATH];
    HMODULE module;
    DWORD len, ret;
    HKEY hkey;
    HKL hkl;

    /* IME module gets cached and won't reload from disk as soon as a window has
     * loaded it. To workaround the issue we load the module first as a DLL,
     * set its function pointers from the test, and later when the cached IME
     * gets loaded, read the function pointers from the separately loaded DLL.
     */

    load_resource( L"ime_wrapper.dll", buffer );

    SetLastError( 0xdeadbeef );
    ret = MoveFileW( buffer, L"c:\\windows\\system32\\winetest_ime.dll" );
    if (!ret)
    {
        ok( GetLastError() == ERROR_ACCESS_DENIED, "got error %lu\n", GetLastError() );
        win_skip( "Failed to copy DLL to system directory\n" );
        return 0;
    }

    module = LoadLibraryW( L"c:\\windows\\system32\\winetest_ime.dll" );
    ok( !!module, "LoadLibraryW failed, error %lu\n", GetLastError() );
    *(struct ime_functions *)GetProcAddress( module, "ime_functions" ) = ime_functions;

    /* install the actual IME module, it will lookup the functions from the DLL */
    load_resource( L"ime_wrapper.dll", buffer );

    SetLastError( 0xdeadbeef );
    swprintf( ime_path, ARRAY_SIZE(ime_path), L"c:\\windows\\system32\\wine%04x.ime", ime_count++ );
    ret = MoveFileW( buffer, ime_path );
    todo_wine_if( GetLastError() == ERROR_ALREADY_EXISTS )
    ok( ret || broken( !ret ) /* sometimes still in use */,
        "MoveFileW failed, error %lu\n", GetLastError() );

    hkl = ImmInstallIMEW( ime_path, L"WineTest IME" );
    ok( hkl == expect_ime, "ImmInstallIMEW returned %p, error %lu\n", hkl, GetLastError() );

    swprintf( buffer, ARRAY_SIZE(buffer), L"System\\CurrentControlSet\\Control\\Keyboard Layouts\\%08x", hkl );
    ret = RegOpenKeyW( HKEY_LOCAL_MACHINE, buffer, &hkey );
    ok( !ret, "RegOpenKeyW returned %#lx, error %lu\n", ret, GetLastError() );

    len = sizeof(buffer);
    memset( buffer, 0xcd, sizeof(buffer) );
    ret = RegQueryValueExW( hkey, L"Ime File", NULL, NULL, (BYTE *)buffer, &len );
    ok( !ret, "RegQueryValueExW returned %#lx, error %lu\n", ret, GetLastError() );
    ok( !wcsicmp( buffer, wcsrchr( ime_path, '\\' ) + 1 ), "got Ime File %s\n", debugstr_w(buffer) );

    len = sizeof(buffer);
    memset( buffer, 0xcd, sizeof(buffer) );
    ret = RegQueryValueExW( hkey, L"Layout Text", NULL, NULL, (BYTE *)buffer, &len );
    ok( !ret, "RegQueryValueExW returned %#lx, error %lu\n", ret, GetLastError() );
    ok( !wcscmp( buffer, L"WineTest IME" ), "got Layout Text %s\n", debugstr_w(buffer) );

    len = sizeof(buffer);
    memset( buffer, 0, sizeof(buffer) );
    ret = RegQueryValueExW( hkey, L"Layout File", NULL, NULL, (BYTE *)buffer, &len );
    todo_wine
    ok( !ret, "RegQueryValueExW returned %#lx, error %lu\n", ret, GetLastError() );
    todo_wine
    ok( !wcscmp( buffer, L"kbdus.dll" ), "got Layout File %s\n", debugstr_w(buffer) );

    ret = RegCloseKey( hkey );
    ok( !ret, "RegCloseKey returned %#lx, error %lu\n", ret, GetLastError() );

    return hkl;
}

static void ime_cleanup( HKL hkl, BOOL free )
{
    HMODULE module = GetModuleHandleW( L"winetest_ime.dll" );
    WCHAR buffer[MAX_PATH], value[MAX_PATH];
    DWORD i, buffer_len, value_len, ret;
    HKEY hkey;

    ret = UnloadKeyboardLayout( hkl );
    todo_wine
    ok( ret, "UnloadKeyboardLayout failed, error %lu\n", GetLastError() );

    if (free) ok_ret( 1, ImmFreeLayout( hkl ) );

    swprintf( buffer, ARRAY_SIZE(buffer), L"System\\CurrentControlSet\\Control\\Keyboard Layouts\\%08x", hkl );
    ret = RegDeleteKeyW( HKEY_LOCAL_MACHINE, buffer );
    ok( !ret, "RegDeleteKeyW returned %#lx, error %lu\n", ret, GetLastError() );

    ret = RegOpenKeyW( HKEY_CURRENT_USER, L"Keyboard Layout\\Preload", &hkey );
    ok( !ret, "RegOpenKeyW returned %#lx, error %lu\n", ret, GetLastError() );

    value_len = ARRAY_SIZE(value);
    buffer_len = sizeof(buffer);
    for (i = 0; !RegEnumValueW( hkey, i, value, &value_len, NULL, NULL, (void *)buffer, &buffer_len ); i++)
    {
        value_len = ARRAY_SIZE(value);
        buffer_len = sizeof(buffer);
        if (hkl != UlongToHandle( wcstoul( buffer, NULL, 16 ) )) continue;
        ret = RegDeleteValueW( hkey, value );
        ok( !ret, "RegDeleteValueW returned %#lx, error %lu\n", ret, GetLastError() );
    }

    ret = RegCloseKey( hkey );
    ok( !ret, "RegCloseKey returned %#lx, error %lu\n", ret, GetLastError() );

    ret = DeleteFileW( ime_path );
    todo_wine_if( GetLastError() == ERROR_ACCESS_DENIED )
    ok( ret || broken( !ret ) /* sometimes still in use */,
        "DeleteFileW failed, error %lu\n", GetLastError() );

    ret = FreeLibrary( module );
    ok( ret, "FreeLibrary failed, error %lu\n", GetLastError() );

    ret = DeleteFileW( L"c:\\windows\\system32\\winetest_ime.dll" );
    ok( ret, "DeleteFileW failed, error %lu\n", GetLastError() );
}

static BOOL CALLBACK enum_get_context( HIMC himc, LPARAM lparam )
{
    ime_trace( "himc %p\n", himc );
    *(HIMC *)lparam = himc;
    return TRUE;
}

static BOOL CALLBACK enum_find_context( HIMC himc, LPARAM lparam )
{
    ime_trace( "himc %p\n", himc );
    if (lparam && lparam == (LPARAM)himc) return FALSE;
    return TRUE;
}

static void test_ImmEnumInputContext(void)
{
    HIMC himc;

    ok_ret( 1, ImmEnumInputContext( 0, enum_get_context, (LPARAM)&default_himc ) );
    ok_ret( 1, ImmEnumInputContext( -1, enum_find_context, 0 ) );
    ok_ret( 1, ImmEnumInputContext( GetCurrentThreadId(), enum_find_context, 0 ) );

    todo_wine
    ok_ret( 0, ImmEnumInputContext( 1, enum_find_context, 0 ) );
    todo_wine
    ok_ret( 0, ImmEnumInputContext( GetCurrentProcessId(), enum_find_context, 0 ) );

    himc = ImmCreateContext();
    ok_ne( NULL, himc, HIMC, "%p" );
    ok_ret( 0, ImmEnumInputContext( GetCurrentThreadId(), enum_find_context, (LPARAM)himc ) );
    ok_ret( 1, ImmDestroyContext( himc ) );
    ok_ret( 1, ImmEnumInputContext( GetCurrentThreadId(), enum_find_context, (LPARAM)himc ) );
}

static void test_ImmInstallIME(void)
{
    UINT ret;
    HKL hkl;

    SET_ENABLE( IME_DLL_PROCESS_ATTACH, TRUE );
    SET_ENABLE( ImeInquire, TRUE );
    SET_ENABLE( ImeDestroy, TRUE );
    SET_ENABLE( IME_DLL_PROCESS_DETACH, TRUE );

    /* IME_PROP_END_UNLOAD for the IME to unload / reload. */
    ime_info.fdwProperty = IME_PROP_END_UNLOAD;

    if (!(hkl = ime_install())) goto cleanup;

    SET_EXPECT( IME_DLL_PROCESS_ATTACH );
    SET_EXPECT( ImeInquire );
    ret = ImmLoadIME( hkl );
    ok( ret, "ImmLoadIME returned %#x\n", ret );
    CHECK_CALLED( IME_DLL_PROCESS_ATTACH );
    CHECK_CALLED( ImeInquire );

    ret = ImmLoadIME( hkl );
    ok( ret, "ImmLoadIME returned %#x\n", ret );

    SET_EXPECT( ImeDestroy );
    SET_EXPECT( IME_DLL_PROCESS_DETACH );
    ret = ImmFreeLayout( hkl );
    ok( ret, "ImmFreeLayout returned %#x\n", ret );
    CHECK_CALLED( ImeDestroy );
    CHECK_CALLED( IME_DLL_PROCESS_DETACH );

    ret = ImmFreeLayout( hkl );
    ok( ret, "ImmFreeLayout returned %#x\n", ret );

    ime_cleanup( hkl, FALSE );

    ime_info.fdwProperty = 0;

    if (!(hkl = ime_install())) goto cleanup;

    SET_EXPECT( IME_DLL_PROCESS_ATTACH );
    SET_EXPECT( ImeInquire );
    ret = ImmLoadIME( hkl );
    ok( ret, "ImmLoadIME returned %#x\n", ret );
    CHECK_CALLED( IME_DLL_PROCESS_ATTACH );
    CHECK_CALLED( ImeInquire );

    ret = ImmLoadIME( hkl );
    ok( ret, "ImmLoadIME returned %#x\n", ret );

    SET_EXPECT( ImeDestroy );
    SET_EXPECT( IME_DLL_PROCESS_DETACH );
    ret = ImmFreeLayout( hkl );
    ok( ret, "ImmFreeLayout returned %#x\n", ret );
    CHECK_CALLED( ImeDestroy );
    CHECK_CALLED( IME_DLL_PROCESS_DETACH );

    ret = ImmFreeLayout( hkl );
    ok( ret, "ImmFreeLayout returned %#x\n", ret );

    ime_cleanup( hkl, FALSE );

cleanup:
    SET_ENABLE( IME_DLL_PROCESS_ATTACH, FALSE );
    SET_ENABLE( ImeInquire, FALSE );
    SET_ENABLE( ImeDestroy, FALSE );
    SET_ENABLE( IME_DLL_PROCESS_DETACH, FALSE );
}

static void test_ImmIsIME(void)
{
    HKL hkl = GetKeyboardLayout( 0 );

    SET_ENABLE( IME_DLL_PROCESS_ATTACH, TRUE );
    SET_ENABLE( ImeInquire, TRUE );
    SET_ENABLE( ImeDestroy, TRUE );
    SET_ENABLE( IME_DLL_PROCESS_DETACH, TRUE );

    SetLastError( 0xdeadbeef );
    ok_ret( 0, ImmIsIME( 0 ) );
    ok_ret( 0xdeadbeef, GetLastError() );
    ok_ret( 1, ImmIsIME( hkl ) );

    /* IME_PROP_END_UNLOAD for the IME to unload / reload. */
    ime_info.fdwProperty = IME_PROP_END_UNLOAD;

    if (!(hkl = ime_install())) goto cleanup;

    todo_ImeInquire = TRUE;
    todo_ImeDestroy = TRUE;
    todo_IME_DLL_PROCESS_ATTACH = TRUE;
    todo_IME_DLL_PROCESS_DETACH = TRUE;
    ok_ret( 1, ImmIsIME( hkl ) );
    todo_IME_DLL_PROCESS_ATTACH = FALSE;
    todo_IME_DLL_PROCESS_DETACH = FALSE;
    todo_ImeInquire = FALSE;
    todo_ImeDestroy = FALSE;

    ime_cleanup( hkl, FALSE );

cleanup:
    SET_ENABLE( IME_DLL_PROCESS_ATTACH, FALSE );
    SET_ENABLE( ImeInquire, FALSE );
    SET_ENABLE( ImeDestroy, FALSE );
    SET_ENABLE( IME_DLL_PROCESS_DETACH, FALSE );
}

static void test_ImmGetProperty(void)
{
    static const IMEINFO expect_ime_info =
    {
        .fdwProperty = IME_PROP_UNICODE | IME_PROP_AT_CARET,
    };
    static const IMEINFO expect_ime_info_0411 = /* MS Japanese IME */
    {
        .fdwProperty = IME_PROP_CANDLIST_START_FROM_1 | IME_PROP_UNICODE | IME_PROP_AT_CARET | 0xa,
        .fdwConversionCaps = IME_CMODE_NATIVE | IME_CMODE_FULLSHAPE | IME_CMODE_KATAKANA,
        .fdwSentenceCaps = IME_SMODE_PLAURALCLAUSE | IME_SMODE_CONVERSATION,
        .fdwSCSCaps = SCS_CAP_COMPSTR | SCS_CAP_SETRECONVERTSTRING | SCS_CAP_MAKEREAD,
        .fdwSelectCaps = SELECT_CAP_CONVERSION | SELECT_CAP_SENTENCE,
        .fdwUICaps = UI_CAP_ROT90,
    };
    static const IMEINFO expect_ime_info_0412 = /* MS Korean IME */
    {
        .fdwProperty = IME_PROP_CANDLIST_START_FROM_1 | IME_PROP_UNICODE | IME_PROP_AT_CARET | 0xa,
        .fdwConversionCaps = IME_CMODE_NATIVE | IME_CMODE_FULLSHAPE,
        .fdwSentenceCaps = IME_SMODE_NONE,
        .fdwSCSCaps = SCS_CAP_COMPSTR | SCS_CAP_SETRECONVERTSTRING,
        .fdwSelectCaps = SELECT_CAP_CONVERSION,
        .fdwUICaps = UI_CAP_ROT90,
    };
    static const IMEINFO expect_ime_info_0804 = /* MS Chinese IME */
    {
        .fdwProperty = IME_PROP_CANDLIST_START_FROM_1 | IME_PROP_UNICODE | IME_PROP_AT_CARET | 0xa,
        .fdwConversionCaps = IME_CMODE_NATIVE | IME_CMODE_FULLSHAPE,
        .fdwSentenceCaps = IME_SMODE_PLAURALCLAUSE,
        .fdwSCSCaps = SCS_CAP_COMPSTR | SCS_CAP_SETRECONVERTSTRING | SCS_CAP_MAKEREAD,
        .fdwUICaps = UI_CAP_ROT90,
    };
    HKL hkl = GetKeyboardLayout( 0 );
    const IMEINFO *expect;

    SET_ENABLE( ImeInquire, TRUE );
    SET_ENABLE( ImeDestroy, TRUE );

    SetLastError( 0xdeadbeef );
    ok_ret( 0, ImmGetProperty( 0, 0 ) );
    ok_ret( 0, ImmGetProperty( hkl, 0 ) );

    if (hkl == (HKL)0x04110411) expect = &expect_ime_info_0411;
    else if (hkl == (HKL)0x04120412) expect = &expect_ime_info_0412;
    else if (hkl == (HKL)0x08040804) expect = &expect_ime_info_0804;
    else expect = &expect_ime_info;

    /* IME_PROP_COMPLETE_ON_UNSELECT seems to be somtimes set on CJK locales IMEs, sometimes not */
    ok_ret( expect->fdwProperty,       ImmGetProperty( hkl, IGP_PROPERTY ) & ~IME_PROP_COMPLETE_ON_UNSELECT );
    todo_wine
    ok_ret( expect->fdwConversionCaps, ImmGetProperty( hkl, IGP_CONVERSION ) );
    todo_wine
    ok_ret( expect->fdwSentenceCaps,   ImmGetProperty( hkl, IGP_SENTENCE ) );
    ok_ret( expect->fdwSCSCaps,        ImmGetProperty( hkl, IGP_SETCOMPSTR ) );
    todo_wine
    ok_ret( expect->fdwSelectCaps,     ImmGetProperty( hkl, IGP_SELECT ) );
    ok_ret( IMEVER_0400,               ImmGetProperty( hkl, IGP_GETIMEVERSION ) );
    ok_ret( expect->fdwUICaps,         ImmGetProperty( hkl, IGP_UI ) );
    todo_wine
    ok_ret( 0xdeadbeef, GetLastError() );

    /* IME_PROP_END_UNLOAD for the IME to unload / reload. */
    ime_info.fdwProperty = IME_PROP_END_UNLOAD;

    if (!(hkl = ime_install())) goto cleanup;

    SET_EXPECT( ImeInquire );
    SET_EXPECT( ImeDestroy );
    ok_ret( 0, ImmGetProperty( hkl, 0 ) );
    CHECK_CALLED( ImeInquire );
    CHECK_CALLED( ImeDestroy );

    expect = &ime_info;
    todo_ImeInquire = TRUE;
    todo_ImeDestroy = TRUE;
    ok_ret( expect->fdwProperty,       ImmGetProperty( hkl, IGP_PROPERTY ) );
    ok_ret( expect->fdwConversionCaps, ImmGetProperty( hkl, IGP_CONVERSION ) );
    ok_ret( expect->fdwSentenceCaps,   ImmGetProperty( hkl, IGP_SENTENCE ) );
    ok_ret( expect->fdwSCSCaps,        ImmGetProperty( hkl, IGP_SETCOMPSTR ) );
    ok_ret( expect->fdwSelectCaps,     ImmGetProperty( hkl, IGP_SELECT ) );
    ok_ret( IMEVER_0400,               ImmGetProperty( hkl, IGP_GETIMEVERSION ) );
    ok_ret( expect->fdwUICaps,         ImmGetProperty( hkl, IGP_UI ) );
    todo_ImeInquire = FALSE;
    called_ImeInquire = FALSE;
    todo_ImeDestroy = FALSE;
    called_ImeDestroy = FALSE;

    ime_cleanup( hkl, FALSE );

cleanup:
    SET_ENABLE( ImeInquire, FALSE );
    SET_ENABLE( ImeDestroy, FALSE );
}

static void test_ImmGetDescription(void)
{
    HKL hkl = GetKeyboardLayout( 0 );
    WCHAR bufferW[MAX_PATH];
    char bufferA[MAX_PATH];
    DWORD ret;

    SET_ENABLE( IME_DLL_PROCESS_ATTACH, TRUE );
    SET_ENABLE( ImeInquire, TRUE );
    SET_ENABLE( ImeDestroy, TRUE );
    SET_ENABLE( IME_DLL_PROCESS_DETACH, TRUE );

    SetLastError( 0xdeadbeef );
    ret = ImmGetDescriptionW( NULL, NULL, 0 );
    ok( !ret, "ImmGetDescriptionW returned %lu\n", ret );
    ret = ImmGetDescriptionA( NULL, NULL, 0 );
    ok( !ret, "ImmGetDescriptionA returned %lu\n", ret );
    ret = ImmGetDescriptionW( NULL, NULL, 100 );
    ok( !ret, "ImmGetDescriptionW returned %lu\n", ret );
    ret = ImmGetDescriptionA( NULL, NULL, 100 );
    ok( !ret, "ImmGetDescriptionA returned %lu\n", ret );
    ret = ImmGetDescriptionW( hkl, bufferW, 100 );
    ok( !ret, "ImmGetDescriptionW returned %lu\n", ret );
    ret = ImmGetDescriptionA( hkl, bufferA, 100 );
    ok( !ret, "ImmGetDescriptionA returned %lu\n", ret );
    ret = GetLastError();
    ok( ret == 0xdeadbeef, "got error %lu\n", ret );

    if (!(hkl = ime_install())) goto cleanup;

    memset( bufferW, 0xcd, sizeof(bufferW) );
    ret = ImmGetDescriptionW( hkl, bufferW, 2 );
    ok( ret == 1, "ImmGetDescriptionW returned %lu\n", ret );
    ok( !wcscmp( bufferW, L"W" ), "got bufferW %s\n", debugstr_w(bufferW) );
    memset( bufferA, 0xcd, sizeof(bufferA) );
    ret = ImmGetDescriptionA( hkl, bufferA, 2 );
    ok( ret == 0, "ImmGetDescriptionA returned %lu\n", ret );
    ok( !strcmp( bufferA, "" ), "got bufferA %s\n", debugstr_a(bufferA) );

    memset( bufferW, 0xcd, sizeof(bufferW) );
    ret = ImmGetDescriptionW( hkl, bufferW, 11 );
    ok( ret == 10, "ImmGetDescriptionW returned %lu\n", ret );
    ok( !wcscmp( bufferW, L"WineTest I" ), "got bufferW %s\n", debugstr_w(bufferW) );
    memset( bufferA, 0xcd, sizeof(bufferA) );
    ret = ImmGetDescriptionA( hkl, bufferA, 11 );
    ok( ret == 0, "ImmGetDescriptionA returned %lu\n", ret );
    ok( !strcmp( bufferA, "" ), "got bufferA %s\n", debugstr_a(bufferA) );

    memset( bufferW, 0xcd, sizeof(bufferW) );
    ret = ImmGetDescriptionW( hkl, bufferW, 12 );
    ok( ret == 11, "ImmGetDescriptionW returned %lu\n", ret );
    ok( !wcscmp( bufferW, L"WineTest IM" ), "got bufferW %s\n", debugstr_w(bufferW) );
    memset( bufferA, 0xcd, sizeof(bufferA) );
    ret = ImmGetDescriptionA( hkl, bufferA, 12 );
    ok( ret == 12, "ImmGetDescriptionA returned %lu\n", ret );
    ok( !strcmp( bufferA, "WineTest IME" ), "got bufferA %s\n", debugstr_a(bufferA) );

    memset( bufferW, 0xcd, sizeof(bufferW) );
    ret = ImmGetDescriptionW( hkl, bufferW, 13 );
    ok( ret == 12, "ImmGetDescriptionW returned %lu\n", ret );
    ok( !wcscmp( bufferW, L"WineTest IME" ), "got bufferW %s\n", debugstr_w(bufferW) );
    memset( bufferA, 0xcd, sizeof(bufferA) );
    ret = ImmGetDescriptionA( hkl, bufferA, 13 );
    ok( ret == 12, "ImmGetDescriptionA returned %lu\n", ret );
    ok( !strcmp( bufferA, "WineTest IME" ), "got bufferA %s\n", debugstr_a(bufferA) );

    ime_cleanup( hkl, FALSE );

cleanup:
    SET_ENABLE( IME_DLL_PROCESS_ATTACH, FALSE );
    SET_ENABLE( ImeInquire, FALSE );
    SET_ENABLE( ImeDestroy, FALSE );
    SET_ENABLE( IME_DLL_PROCESS_DETACH, FALSE );
}

static void test_ImmGetIMEFileName(void)
{
    HKL hkl = GetKeyboardLayout( 0 );
    WCHAR bufferW[MAX_PATH], expectW[16];
    char bufferA[MAX_PATH], expectA[16];
    DWORD ret;

    SET_ENABLE( IME_DLL_PROCESS_ATTACH, TRUE );
    SET_ENABLE( ImeInquire, TRUE );
    SET_ENABLE( ImeDestroy, TRUE );
    SET_ENABLE( IME_DLL_PROCESS_DETACH, TRUE );

    SetLastError( 0xdeadbeef );
    ret = ImmGetIMEFileNameW( NULL, NULL, 0 );
    ok( !ret, "ImmGetIMEFileNameW returned %lu\n", ret );
    ret = ImmGetIMEFileNameA( NULL, NULL, 0 );
    ok( !ret, "ImmGetIMEFileNameA returned %lu\n", ret );
    ret = ImmGetIMEFileNameW( NULL, NULL, 100 );
    ok( !ret, "ImmGetIMEFileNameW returned %lu\n", ret );
    ret = ImmGetIMEFileNameA( NULL, NULL, 100 );
    ok( !ret, "ImmGetIMEFileNameA returned %lu\n", ret );
    ret = ImmGetIMEFileNameW( hkl, bufferW, 100 );
    ok( !ret, "ImmGetIMEFileNameW returned %lu\n", ret );
    ret = ImmGetIMEFileNameA( hkl, bufferA, 100 );
    ok( !ret, "ImmGetIMEFileNameA returned %lu\n", ret );
    ret = GetLastError();
    ok( ret == 0xdeadbeef, "got error %lu\n", ret );

    if (!(hkl = ime_install())) goto cleanup;

    memset( bufferW, 0xcd, sizeof(bufferW) );
    ret = ImmGetIMEFileNameW( hkl, bufferW, 2 );
    ok( ret == 1, "ImmGetIMEFileNameW returned %lu\n", ret );
    ok( !wcscmp( bufferW, L"W" ), "got bufferW %s\n", debugstr_w(bufferW) );
    memset( bufferA, 0xcd, sizeof(bufferA) );
    ret = ImmGetIMEFileNameA( hkl, bufferA, 2 );
    ok( ret == 0, "ImmGetIMEFileNameA returned %lu\n", ret );
    ok( !strcmp( bufferA, "" ), "got bufferA %s\n", debugstr_a(bufferA) );

    swprintf( expectW, ARRAY_SIZE(expectW), L"WINE%04X.I", ime_count - 1 );
    memset( bufferW, 0xcd, sizeof(bufferW) );
    ret = ImmGetIMEFileNameW( hkl, bufferW, 11 );
    ok( ret == 10, "ImmGetIMEFileNameW returned %lu\n", ret );
    ok( !wcscmp( bufferW, expectW ), "got bufferW %s\n", debugstr_w(bufferW) );
    memset( bufferA, 0xcd, sizeof(bufferA) );
    ret = ImmGetIMEFileNameA( hkl, bufferA, 11 );
    ok( ret == 0, "ImmGetIMEFileNameA returned %lu\n", ret );
    ok( !strcmp( bufferA, "" ), "got bufferA %s\n", debugstr_a(bufferA) );

    swprintf( expectW, ARRAY_SIZE(expectW), L"WINE%04X.IM", ime_count - 1 );
    memset( bufferW, 0xcd, sizeof(bufferW) );
    ret = ImmGetIMEFileNameW( hkl, bufferW, 12 );
    ok( ret == 11, "ImmGetIMEFileNameW returned %lu\n", ret );
    ok( !wcscmp( bufferW, expectW ), "got bufferW %s\n", debugstr_w(bufferW) );
    snprintf( expectA, ARRAY_SIZE(expectA), "WINE%04X.IME", ime_count - 1 );
    memset( bufferA, 0xcd, sizeof(bufferA) );
    ret = ImmGetIMEFileNameA( hkl, bufferA, 12 );
    ok( ret == 12, "ImmGetIMEFileNameA returned %lu\n", ret );
    ok( !strcmp( bufferA, expectA ), "got bufferA %s\n", debugstr_a(bufferA) );

    swprintf( expectW, ARRAY_SIZE(expectW), L"WINE%04X.IME", ime_count - 1 );
    memset( bufferW, 0xcd, sizeof(bufferW) );
    ret = ImmGetIMEFileNameW( hkl, bufferW, 13 );
    ok( ret == 12, "ImmGetIMEFileNameW returned %lu\n", ret );
    ok( !wcscmp( bufferW, expectW ), "got bufferW %s\n", debugstr_w(bufferW) );
    memset( bufferA, 0xcd, sizeof(bufferA) );
    ret = ImmGetIMEFileNameA( hkl, bufferA, 13 );
    ok( ret == 12, "ImmGetIMEFileNameA returned %lu\n", ret );
    ok( !strcmp( bufferA, expectA ), "got bufferA %s\n", debugstr_a(bufferA) );

    ime_cleanup( hkl, FALSE );

cleanup:
    SET_ENABLE( IME_DLL_PROCESS_ATTACH, FALSE );
    SET_ENABLE( ImeInquire, FALSE );
    SET_ENABLE( ImeDestroy, FALSE );
    SET_ENABLE( IME_DLL_PROCESS_DETACH, FALSE );
}

static void test_ImmEscape( BOOL unicode )
{
    HKL hkl = GetKeyboardLayout( 0 );
    DWORD i, codes[] =
    {
        IME_ESC_QUERY_SUPPORT,
        IME_ESC_SEQUENCE_TO_INTERNAL,
        IME_ESC_GET_EUDC_DICTIONARY,
        IME_ESC_SET_EUDC_DICTIONARY,
        IME_ESC_MAX_KEY,
        IME_ESC_IME_NAME,
        IME_ESC_HANJA_MODE,
        IME_ESC_GETHELPFILENAME,
    };
    WCHAR bufferW[512];
    char bufferA[512];

    SET_ENABLE( ImeEscape, TRUE );

    winetest_push_context( unicode ? "unicode" : "ansi" );

    ok_ret( 0, ImmEscapeW( hkl, 0, 0, NULL ) );
    ok_ret( 0, ImmEscapeA( hkl, 0, 0, NULL ) );

    /* IME_PROP_END_UNLOAD for the IME to unload / reload. */
    ime_info.fdwProperty = IME_PROP_END_UNLOAD;
    if (unicode) ime_info.fdwProperty |= IME_PROP_UNICODE;

    if (!(hkl = ime_install())) goto cleanup;

    for (i = 0; i < ARRAY_SIZE(codes); ++i)
    {
        winetest_push_context( "esc %#lx", codes[i] );

        SET_EXPECT( ImeEscape );
        ok_ret( 4, ImmEscapeW( hkl, 0, codes[i], NULL ) );
        CHECK_CALLED( ImeEscape );

        SET_EXPECT( ImeEscape );
        memset( bufferW, 0xcd, sizeof(bufferW) );
        if (codes[i] == IME_ESC_SET_EUDC_DICTIONARY) wcscpy( bufferW, L"EscapeIme" );
        ok_ret( 4, ImmEscapeW( hkl, 0, codes[i], bufferW ) );
        if (unicode || codes[i] == IME_ESC_GET_EUDC_DICTIONARY || codes[i] == IME_ESC_IME_NAME ||
            codes[i] == IME_ESC_GETHELPFILENAME)
        {
            ok_wcs( L"ImeEscape", bufferW );
            ok_eq( 0xcdcd, bufferW[10], WORD, "%#x" );
        }
        else if (codes[i] == IME_ESC_SET_EUDC_DICTIONARY)
        {
            ok_wcs( L"EscapeIme", bufferW );
            ok_eq( 0xcdcd, bufferW[10], WORD, "%#x" );
        }
        else if (codes[i] == IME_ESC_HANJA_MODE)
        {
            todo_wine
            ok_eq( 0xcdcd, bufferW[0], WORD, "%#x" );
        }
        else
        {
            ok( !memcmp( bufferW, "ImeEscape", 10 ), "got bufferW %s\n", debugstr_w(bufferW) );
            ok_eq( 0xcdcd, bufferW[5], WORD, "%#x" );
        }
        CHECK_CALLED( ImeEscape );

        SET_EXPECT( ImeEscape );
        ok_ret( 4, ImmEscapeA( hkl, 0, codes[i], NULL ) );
        CHECK_CALLED( ImeEscape );

        SET_EXPECT( ImeEscape );
        memset( bufferA, 0xcd, sizeof(bufferA) );
        if (codes[i] == IME_ESC_SET_EUDC_DICTIONARY) strcpy( bufferA, "EscapeIme" );
        ok_ret( 4, ImmEscapeA( hkl, 0, codes[i], bufferA ) );
        if (!unicode || codes[i] == IME_ESC_GET_EUDC_DICTIONARY || codes[i] == IME_ESC_IME_NAME ||
            codes[i] == IME_ESC_GETHELPFILENAME)
        {
            ok_str( "ImeEscape", bufferA );
            ok_eq( 0xcd, bufferA[10], BYTE, "%#x" );
        }
        else if (codes[i] == IME_ESC_SET_EUDC_DICTIONARY)
        {
            ok_str( "EscapeIme", bufferA );
            ok_eq( 0xcd, bufferA[10], BYTE, "%#x" );
        }
        else if (codes[i] == IME_ESC_HANJA_MODE)
        {
            todo_wine
            ok_eq( 0xcd, bufferA[0], BYTE, "%#x" );
        }
        else
        {
            ok( !memcmp( bufferA, L"ImeEscape", 10 * sizeof(WCHAR) ), "got bufferA %s\n", debugstr_a(bufferA) );
            ok_eq( 0xcd, bufferA[20], BYTE, "%#x" );
        }
        CHECK_CALLED( ImeEscape );

        winetest_pop_context();
    }

    ime_cleanup( hkl, FALSE );

cleanup:
    SET_ENABLE( ImeEscape, FALSE );

    winetest_pop_context();
}

static int CALLBACK enum_register_wordA( const char *reading, DWORD style, const char *string, void *user )
{
    ime_trace( "reading %s, style %#lx, string %s, user %p\n", debugstr_a(reading), style, debugstr_a(string), user );

    ok_eq( 0xdeadbeef, style, UINT, "%#x" );
    ok_str( "Reading", reading );
    ok_str( "String", string );

    return 0xdeadbeef;
}

static int CALLBACK enum_register_wordW( const WCHAR *reading, DWORD style, const WCHAR *string, void *user )
{
    ime_trace( "reading %s, style %#lx, string %s, user %p\n", debugstr_w(reading), style, debugstr_w(string), user );

    ok_eq( 0xdeadbeef, style, UINT, "%#x" );
    ok_wcs( L"Reading", reading );
    ok_wcs( L"String", string );

    return 0xdeadbeef;
}

static void test_ImmEnumRegisterWord( BOOL unicode )
{
    HKL hkl = GetKeyboardLayout( 0 );

    winetest_push_context( unicode ? "unicode" : "ansi" );

    SET_ENABLE( ImeEnumRegisterWord, TRUE );

    SetLastError( 0xdeadbeef );
    ok_ret( 0, ImmEnumRegisterWordW( NULL, enum_register_wordW, NULL, 0, NULL, NULL ) );
    ok_ret( 0, ImmEnumRegisterWordA( NULL, enum_register_wordA, NULL, 0, NULL, NULL ) );
    ok_ret( 0, ImmEnumRegisterWordW( hkl, enum_register_wordW, NULL, 0, NULL, NULL ) );
    ok_ret( 0, ImmEnumRegisterWordA( hkl, enum_register_wordA, NULL, 0, NULL, NULL ) );
    todo_wine
    ok_ret( 0xdeadbeef, GetLastError() );

    /* IME_PROP_END_UNLOAD for the IME to unload / reload. */
    ime_info.fdwProperty = IME_PROP_END_UNLOAD;
    if (unicode) ime_info.fdwProperty |= IME_PROP_UNICODE;

    if (!(hkl = ime_install())) goto cleanup;

    SET_EXPECT( ImeEnumRegisterWord );
    ok_ret( 0, ImmEnumRegisterWordW( hkl, enum_register_wordW, NULL, 0, NULL, NULL ) );
    CHECK_CALLED( ImeEnumRegisterWord );

    SET_EXPECT( ImeEnumRegisterWord );
    ok_ret( 0, ImmEnumRegisterWordA( hkl, enum_register_wordA, NULL, 0, NULL, NULL ) );
    CHECK_CALLED( ImeEnumRegisterWord );

    SET_EXPECT( ImeEnumRegisterWord );
    ok_ret( 0xdeadbeef, ImmEnumRegisterWordW( hkl, enum_register_wordW, L"Reading", 0xdeadbeef, L"String", NULL ) );
    CHECK_CALLED( ImeEnumRegisterWord );

    SET_EXPECT( ImeEnumRegisterWord );
    ok_ret( 0xdeadbeef, ImmEnumRegisterWordA( hkl, enum_register_wordA, "Reading", 0xdeadbeef, "String", NULL ) );
    CHECK_CALLED( ImeEnumRegisterWord );

    ime_cleanup( hkl, FALSE );

cleanup:
    SET_ENABLE( ImeEnumRegisterWord, FALSE );

    winetest_pop_context();
}

static void test_ImmRegisterWord( BOOL unicode )
{
    HKL hkl = GetKeyboardLayout( 0 );

    SET_ENABLE( ImeRegisterWord, TRUE );

    winetest_push_context( unicode ? "unicode" : "ansi" );

    SetLastError( 0xdeadbeef );
    ok_ret( 0, ImmRegisterWordW( NULL, NULL, 0, NULL ) );
    ok_ret( 0, ImmRegisterWordA( NULL, NULL, 0, NULL ) );
    ok_ret( 0, ImmRegisterWordW( hkl, NULL, 0, NULL ) );
    ok_ret( 0, ImmRegisterWordA( hkl, NULL, 0, NULL ) );
    todo_wine
    ok_ret( 0xdeadbeef, GetLastError() );

    /* IME_PROP_END_UNLOAD for the IME to unload / reload. */
    ime_info.fdwProperty = IME_PROP_END_UNLOAD;
    if (unicode) ime_info.fdwProperty |= IME_PROP_UNICODE;

    if (!(hkl = ime_install())) goto cleanup;

    SET_EXPECT( ImeRegisterWord );
    ok_ret( 0, ImmRegisterWordW( hkl, NULL, 0, NULL ) );
    CHECK_CALLED( ImeRegisterWord );

    SET_EXPECT( ImeRegisterWord );
    ok_ret( 0, ImmRegisterWordA( hkl, NULL, 0, NULL ) );
    CHECK_CALLED( ImeRegisterWord );

    SET_EXPECT( ImeRegisterWord );
    ok_ret( 0, ImmRegisterWordW( hkl, L"Reading", 0, NULL ) );
    CHECK_CALLED( ImeRegisterWord );

    SET_EXPECT( ImeRegisterWord );
    ok_ret( 0, ImmRegisterWordA( hkl, "Reading", 0, NULL ) );
    CHECK_CALLED( ImeRegisterWord );

    SET_EXPECT( ImeRegisterWord );
    ok_ret( 0, ImmRegisterWordW( hkl, NULL, 0xdeadbeef, NULL ) );
    CHECK_CALLED( ImeRegisterWord );

    SET_EXPECT( ImeRegisterWord );
    ok_ret( 0, ImmRegisterWordA( hkl, NULL, 0xdeadbeef, NULL ) );
    CHECK_CALLED( ImeRegisterWord );

    SET_EXPECT( ImeRegisterWord );
    ok_ret( 0, ImmRegisterWordW( hkl, NULL, 0, L"String" ) );
    CHECK_CALLED( ImeRegisterWord );

    SET_EXPECT( ImeRegisterWord );
    ok_ret( 0, ImmRegisterWordA( hkl, NULL, 0, "String" ) );
    CHECK_CALLED( ImeRegisterWord );

    ime_cleanup( hkl, FALSE );

cleanup:
    SET_ENABLE( ImeRegisterWord, FALSE );

    winetest_pop_context();
}

static void test_ImmGetRegisterWordStyle( BOOL unicode )
{
    HKL hkl = GetKeyboardLayout( 0 );
    STYLEBUFW styleW;
    STYLEBUFA styleA;

    winetest_push_context( unicode ? "unicode" : "ansi" );

    SET_ENABLE( ImeGetRegisterWordStyle, TRUE );

    SetLastError( 0xdeadbeef );
    ok_ret( 0, ImmGetRegisterWordStyleW( NULL, 0, &styleW ) );
    ok_ret( 0, ImmGetRegisterWordStyleA( NULL, 0, &styleA ) );
    ok_ret( 0, ImmGetRegisterWordStyleW( hkl, 0, &styleW ) );
    ok_ret( 0, ImmGetRegisterWordStyleA( hkl, 0, &styleA ) );
    todo_wine
    ok_ret( 0xdeadbeef, GetLastError() );

    /* IME_PROP_END_UNLOAD for the IME to unload / reload. */
    ime_info.fdwProperty = IME_PROP_END_UNLOAD;
    if (unicode) ime_info.fdwProperty |= IME_PROP_UNICODE;

    if (!(hkl = ime_install())) goto cleanup;

    if (!strcmp( winetest_platform, "wine" )) goto skip_null;

    SET_EXPECT( ImeGetRegisterWordStyle );
    ok_ret( 0xdeadbeef, ImmGetRegisterWordStyleW( hkl, 16, NULL ) );
    CHECK_CALLED( ImeGetRegisterWordStyle );

    SET_EXPECT( ImeGetRegisterWordStyle );
    ok_ret( 0xdeadbeef, ImmGetRegisterWordStyleA( hkl, 16, NULL ) );
    CHECK_CALLED( ImeGetRegisterWordStyle );

skip_null:
    SET_EXPECT( ImeGetRegisterWordStyle );
    memset( &styleW, 0xcd, sizeof(styleW) );
    ok_ret( 0xdeadbeef, ImmGetRegisterWordStyleW( hkl, 1, &styleW ) );
    if (ime_info.fdwProperty & IME_PROP_UNICODE)
    {
        ok_eq( 0xdeadbeef, styleW.dwStyle, UINT, "%#x" );
        ok_wcs( L"StyleDescription", styleW.szDescription );
    }
    else
    {
        todo_wine
        ok_eq( 0xcdcdcdcd, styleW.dwStyle, UINT, "%#x" );
        todo_wine
        ok_eq( 0xcdcd, styleW.szDescription[0], WORD, "%#x" );
    }
    CHECK_CALLED( ImeGetRegisterWordStyle );

    SET_EXPECT( ImeGetRegisterWordStyle );
    memset( &styleA, 0xcd, sizeof(styleA) );
    ok_ret( 0xdeadbeef, ImmGetRegisterWordStyleA( hkl, 1, &styleA ) );
    if (ime_info.fdwProperty & IME_PROP_UNICODE)
    {
        todo_wine
        ok_eq( 0xcdcdcdcd, styleA.dwStyle, UINT, "%#x" );
        todo_wine
        ok_eq( 0xcd, styleA.szDescription[0], BYTE, "%#x" );
    }
    else
    {
        ok_eq( 0xdeadbeef, styleA.dwStyle, UINT, "%#x" );
        ok_str( "StyleDescription", styleA.szDescription );
    }
    CHECK_CALLED( ImeGetRegisterWordStyle );

    ime_cleanup( hkl, FALSE );

cleanup:
    SET_ENABLE( ImeGetRegisterWordStyle, FALSE );

    winetest_pop_context();
}

static void test_ImmUnregisterWord( BOOL unicode )
{
    HKL hkl = GetKeyboardLayout( 0 );

    winetest_push_context( unicode ? "unicode" : "ansi" );

    SET_ENABLE( ImeUnregisterWord, TRUE );

    SetLastError( 0xdeadbeef );
    ok_ret( 0, ImmUnregisterWordW( NULL, NULL, 0, NULL ) );
    ok_ret( 0, ImmUnregisterWordA( NULL, NULL, 0, NULL ) );
    ok_ret( 0, ImmUnregisterWordW( hkl, NULL, 0, NULL ) );
    ok_ret( 0, ImmUnregisterWordA( hkl, NULL, 0, NULL ) );
    todo_wine
    ok_ret( 0xdeadbeef, GetLastError() );

    /* IME_PROP_END_UNLOAD for the IME to unload / reload. */
    ime_info.fdwProperty = IME_PROP_END_UNLOAD;
    if (unicode) ime_info.fdwProperty |= IME_PROP_UNICODE;

    if (!(hkl = ime_install())) goto cleanup;

    SET_EXPECT( ImeUnregisterWord );
    ok_ret( 0, ImmUnregisterWordW( hkl, NULL, 0, NULL ) );
    CHECK_CALLED( ImeUnregisterWord );

    SET_EXPECT( ImeUnregisterWord );
    ok_ret( 0, ImmUnregisterWordA( hkl, NULL, 0, NULL ) );
    CHECK_CALLED( ImeUnregisterWord );

    SET_EXPECT( ImeUnregisterWord );
    ok_ret( 0, ImmUnregisterWordW( hkl, L"Reading", 0, NULL ) );
    CHECK_CALLED( ImeUnregisterWord );

    SET_EXPECT( ImeUnregisterWord );
    ok_ret( 0, ImmUnregisterWordA( hkl, "Reading", 0, NULL ) );
    CHECK_CALLED( ImeUnregisterWord );

    SET_EXPECT( ImeUnregisterWord );
    ok_ret( 0, ImmUnregisterWordW( hkl, NULL, 0xdeadbeef, NULL ) );
    CHECK_CALLED( ImeUnregisterWord );

    SET_EXPECT( ImeUnregisterWord );
    ok_ret( 0, ImmUnregisterWordA( hkl, NULL, 0xdeadbeef, NULL ) );
    CHECK_CALLED( ImeUnregisterWord );

    SET_EXPECT( ImeUnregisterWord );
    ok_ret( 0, ImmUnregisterWordW( hkl, NULL, 0, L"String" ) );
    CHECK_CALLED( ImeUnregisterWord );

    SET_EXPECT( ImeUnregisterWord );
    ok_ret( 0, ImmUnregisterWordA( hkl, NULL, 0, "String" ) );
    CHECK_CALLED( ImeUnregisterWord );

    ime_cleanup( hkl, FALSE );

cleanup:
    SET_ENABLE( ImeUnregisterWord, FALSE );

    winetest_pop_context();
}

static void test_ImmSetConversionStatus(void)
{
    const struct ime_call set_conversion_status_0_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_NOTIFY, .notify = {.action = NI_CONTEXTUPDATED, .index = 0, .value = IMC_SETCONVERSIONMODE},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETCONVERSIONMODE},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_NOTIFY, .notify = {.action = NI_CONTEXTUPDATED, .index = 0, .value = IMC_SETSENTENCEMODE},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETSENTENCEMODE},
        },
        {0},
    };
    const struct ime_call set_conversion_status_1_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_NOTIFY, .notify = {.action = NI_CONTEXTUPDATED, .index = 0xdeadbeef, .value = IMC_SETCONVERSIONMODE},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETCONVERSIONMODE},
        },
        {0},
    };
    const struct ime_call set_conversion_status_2_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_NOTIFY, .notify = {.action = NI_CONTEXTUPDATED, .index = 0, .value = IMC_SETCONVERSIONMODE},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETCONVERSIONMODE},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_NOTIFY, .notify = {.action = NI_CONTEXTUPDATED, .index = 0xfeedcafe, .value = IMC_SETSENTENCEMODE},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETSENTENCEMODE},
        },
        {0},
    };
    DWORD old_conversion, old_sentence, conversion, sentence;
    HKL hkl, old_hkl = GetKeyboardLayout( 0 );
    INPUTCONTEXT *ctx;

    ok_ret( 0, ImmGetConversionStatus( 0, &old_conversion, &old_sentence ) );
    ok_ret( 1, ImmGetConversionStatus( default_himc, &old_conversion, &old_sentence ) );

    ctx = ImmLockIMC( default_himc );
    ok_ne( NULL, ctx, INPUTCONTEXT *, "%p" );
    ok_eq( old_conversion, ctx->fdwConversion, UINT, "%#x" );
    ok_eq( old_sentence, ctx->fdwSentence, UINT, "%#x" );

    hwnd = CreateWindowW( L"static", NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 100, 100, NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );
    process_messages();

    ok_ret( 1, ImmGetConversionStatus( default_himc, &conversion, &sentence ) );
    ok_eq( old_conversion, conversion, UINT, "%#x" );
    ok_eq( old_sentence, sentence, UINT, "%#x" );
    ok_eq( old_conversion, ctx->fdwConversion, UINT, "%#x" );
    ok_eq( old_sentence, ctx->fdwSentence, UINT, "%#x" );

    ok_ret( 1, ImmSetConversionStatus( default_himc, 0, 0 ) );
    ok_ret( 1, ImmGetConversionStatus( default_himc, &conversion, &sentence ) );
    ok_eq( 0, conversion, UINT, "%#x" );
    ok_eq( 0, sentence, UINT, "%#x" );
    ok_eq( 0, ctx->fdwConversion, UINT, "%#x" );
    ok_eq( 0, ctx->fdwSentence, UINT, "%#x" );

    ime_info.fdwProperty = IME_PROP_END_UNLOAD | IME_PROP_UNICODE;

    if (!(hkl = ime_install())) goto cleanup;

    ok_ret( 1, ImmActivateLayout( hkl ) );
    ok_ret( 1, ImmLoadIME( hkl ) );
    process_messages();
    /* initial values are dependent on both old and new IME */
    ok_ret( 1, ImmSetConversionStatus( default_himc, 0, 0 ) );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    ok_ret( 1, ImmGetConversionStatus( default_himc, &conversion, &sentence ) );
    ok_eq( 0, conversion, UINT, "%#x" );
    ok_eq( 0, sentence, UINT, "%#x" );
    ok_eq( 0, ctx->fdwConversion, UINT, "%#x" );
    ok_eq( 0, ctx->fdwSentence, UINT, "%#x" );

    ok_seq( empty_sequence );
    ok_ret( 1, ImmSetConversionStatus( default_himc, 0xdeadbeef, 0xfeedcafe ) );
    ok_seq( set_conversion_status_0_seq );

    ok_ret( 1, ImmGetConversionStatus( default_himc, &conversion, &sentence ) );
    ok_eq( 0xdeadbeef, conversion, UINT, "%#x" );
    ok_eq( 0xfeedcafe, sentence, UINT, "%#x" );
    ok_eq( 0xdeadbeef, ctx->fdwConversion, UINT, "%#x" );
    ok_eq( 0xfeedcafe, ctx->fdwSentence, UINT, "%#x" );

    ok_seq( empty_sequence );
    ok_ret( 1, ImmSetConversionStatus( default_himc, 0, 0xfeedcafe ) );
    ok_seq( set_conversion_status_1_seq );

    ok_ret( 1, ImmGetConversionStatus( default_himc, &conversion, &sentence ) );
    ok_eq( 0, conversion, UINT, "%#x" );
    ok_eq( 0xfeedcafe, sentence, UINT, "%#x" );
    ok_eq( 0, ctx->fdwConversion, UINT, "%#x" );
    ok_eq( 0xfeedcafe, ctx->fdwSentence, UINT, "%#x" );

    ok_seq( empty_sequence );
    ok_ret( 1, ImmSetConversionStatus( default_himc, ~0, ~0 ) );
    ok_seq( set_conversion_status_2_seq );

    ok_ret( 1, ImmGetConversionStatus( default_himc, &conversion, &sentence ) );
    ok_eq( ~0, conversion, UINT, "%#x" );
    ok_eq( ~0, sentence, UINT, "%#x" );
    ok_eq( ~0, ctx->fdwConversion, UINT, "%#x" );
    ok_eq( ~0, ctx->fdwSentence, UINT, "%#x" );

    /* status is cached and some bits are kept from the previous active IME */
    ok_ret( 1, ImmActivateLayout( old_hkl ) );
    todo_wine ok_eq( 0x200, ctx->fdwConversion, UINT, "%#x" );
    ok_eq( old_sentence, ctx->fdwSentence, UINT, "%#x" );
    ok_ret( 1, ImmActivateLayout( hkl ) );
    todo_wine ok_eq( ~0, ctx->fdwConversion, UINT, "%#x" );
    todo_wine ok_eq( ~0, ctx->fdwSentence, UINT, "%#x" );
    ok_ret( 1, ImmActivateLayout( old_hkl ) );
    todo_wine ok_eq( 0x200, ctx->fdwConversion, UINT, "%#x" );
    ok_eq( old_sentence, ctx->fdwSentence, UINT, "%#x" );

    ime_cleanup( hkl, TRUE );

cleanup:
    /* sanitize conversion status to some sane default */
    ok_ret( 1, ImmSetConversionStatus( default_himc, 0, 0 ) );
    ok_ret( 1, ImmGetConversionStatus( default_himc, &conversion, &sentence ) );
    ok_eq( 0, conversion, UINT, "%#x" );
    ok_eq( 0, sentence, UINT, "%#x" );
    ok_eq( 0, ctx->fdwConversion, UINT, "%#x" );
    ok_eq( 0, ctx->fdwSentence, UINT, "%#x" );

    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    ok_ret( 1, ImmUnlockIMC( default_himc ) );
}

static void test_ImmSetOpenStatus(void)
{
    const struct ime_call set_open_status_0_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_NOTIFY, .notify = {.action = NI_CONTEXTUPDATED, .index = 0, .value = IMC_SETOPENSTATUS},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETOPENSTATUS},
        },
        {0},
    };
    const struct ime_call set_open_status_1_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_NOTIFY, .notify = {.action = NI_CONTEXTUPDATED, .index = 0, .value = IMC_SETOPENSTATUS},
            .todo = TRUE,
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETOPENSTATUS},
            .todo = TRUE,
        },
        {0},
    };
    HKL hkl, old_hkl = GetKeyboardLayout( 0 );
    DWORD old_status, status;
    INPUTCONTEXT *ctx;

    ok_ret( 0, ImmGetOpenStatus( 0 ) );
    old_status = ImmGetOpenStatus( default_himc );

    ctx = ImmLockIMC( default_himc );
    ok_ne( NULL, ctx, INPUTCONTEXT *, "%p" );
    ok_eq( old_status, ctx->fOpen, UINT, "%#x" );

    hwnd = CreateWindowW( L"static", NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 100, 100, NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );
    process_messages();

    status = ImmGetOpenStatus( default_himc );
    ok_eq( old_status, status, UINT, "%#x" );
    ok_eq( old_status, ctx->fOpen, UINT, "%#x" );

    ok_ret( 1, ImmSetOpenStatus( default_himc, 0 ) );
    status = ImmGetOpenStatus( default_himc );
    ok_eq( 0, status, UINT, "%#x" );
    ok_eq( 0, ctx->fOpen, UINT, "%#x" );

    ime_info.fdwProperty = IME_PROP_END_UNLOAD | IME_PROP_UNICODE;

    if (!(hkl = ime_install())) goto cleanup;

    ok_ret( 1, ImmActivateLayout( hkl ) );
    ok_ret( 1, ImmLoadIME( hkl ) );
    process_messages();
    /* initial values are dependent on both old and new IME */
    ok_ret( 1, ImmSetOpenStatus( default_himc, 0 ) );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    status = ImmGetOpenStatus( default_himc );
    ok_eq( 0, status, UINT, "%#x" );
    ok_eq( 0, ctx->fOpen, UINT, "%#x" );

    ok_seq( empty_sequence );
    ok_ret( 1, ImmSetOpenStatus( default_himc, 0xdeadbeef ) );
    ok_seq( set_open_status_0_seq );

    status = ImmGetOpenStatus( default_himc );
    ok_eq( 0xdeadbeef, status, UINT, "%#x" );
    ok_eq( 0xdeadbeef, ctx->fOpen, UINT, "%#x" );

    ok_seq( empty_sequence );
    ok_ret( 1, ImmSetOpenStatus( default_himc, ~0 ) );
    ok_seq( set_open_status_1_seq );

    status = ImmGetOpenStatus( default_himc );
    todo_wine ok_eq( ~0, status, UINT, "%#x" );
    todo_wine ok_eq( ~0, ctx->fOpen, UINT, "%#x" );

    /* status is cached between IME activations */

    ok_ret( 1, ImmActivateLayout( old_hkl ) );
    status = ImmGetOpenStatus( default_himc );
    todo_wine ok_eq( old_status, status, UINT, "%#x" );
    todo_wine ok_eq( old_status, ctx->fOpen, UINT, "%#x" );
    ok_ret( 1, ImmActivateLayout( hkl ) );
    status = ImmGetOpenStatus( default_himc );
    todo_wine ok_eq( 1, status, UINT, "%#x" );
    todo_wine ok_eq( 1, ctx->fOpen, UINT, "%#x" );
    ok_ret( 1, ImmSetOpenStatus( default_himc, 0 ) );
    ok_ret( 1, ImmActivateLayout( old_hkl ) );
    status = ImmGetOpenStatus( default_himc );
    ok_eq( old_status, status, UINT, "%#x" );
    ok_eq( old_status, ctx->fOpen, UINT, "%#x" );

    ime_cleanup( hkl, TRUE );

cleanup:
    /* sanitize open status to some sane default */
    ok_ret( 1, ImmSetOpenStatus( default_himc, 0 ) );
    status = ImmGetOpenStatus( default_himc );
    ok_eq( 0, status, UINT, "%#x" );
    ok_eq( 0, ctx->fOpen, UINT, "%#x" );

    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    ok_ret( 1, ImmUnlockIMC( default_himc ) );
}

static void test_ImmProcessKey(void)
{
    const struct ime_call process_key_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_PROCESS_KEY, .process_key = {.vkey = 'A', .key_data = 0},
        },
        {0},
    };
    HKL hkl, old_hkl = GetKeyboardLayout( 0 );
    UINT_PTR ret;

    hwnd = CreateWindowW( L"static", NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 100, 100, NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );
    process_messages();

    ok_ret( 0, ImmProcessKey( hwnd, old_hkl, 'A', 0, 0 ) );
    ok_seq( empty_sequence );

    ime_info.fdwProperty = IME_PROP_END_UNLOAD | IME_PROP_UNICODE;

    if (!(hkl = ime_install())) goto cleanup;

    ok_ret( 1, ImmActivateLayout( hkl ) );
    ok_ret( 1, ImmLoadIME( hkl ) );
    process_messages();
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    ok_ret( 0, ImmProcessKey( 0, hkl, 'A', 0, 0 ) );
    ok_seq( empty_sequence );

    ret = ImmProcessKey( hwnd, hkl, 'A', 0, 0 );
    todo_wine
    ok_ret( 2, ret );
    ok_seq( process_key_seq );

    ok_eq( hkl, GetKeyboardLayout( 0 ), HKL, "%p" );
    ok_ret( 0, ImmProcessKey( hwnd, old_hkl, 'A', 0, 0 ) );
    ok_seq( empty_sequence );
    ok_eq( hkl, GetKeyboardLayout( 0 ), HKL, "%p" );

    ok_ret( 1, ImmActivateLayout( old_hkl ) );

    ime_cleanup( hkl, TRUE );
    process_messages();
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

cleanup:
    ok_ret( 1, DestroyWindow( hwnd ) );
}

struct ime_windows
{
    HWND ime_hwnd;
    HWND ime_ui_hwnd;
};

static BOOL CALLBACK enum_thread_ime_windows( HWND hwnd, LPARAM lparam )
{
    struct ime_windows *params = (void *)lparam;
    WCHAR buffer[256];
    UINT ret;

    ime_trace( "hwnd %p, lparam %#Ix\n", hwnd, lparam );

    ret = RealGetWindowClassW( hwnd, buffer, ARRAY_SIZE(buffer) );
    ok( ret, "RealGetWindowClassW returned %#x\n", ret );

    if (!wcscmp( buffer, L"IME" ))
    {
        ok( !params->ime_hwnd, "Found extra IME window %p\n", hwnd );
        params->ime_hwnd = hwnd;
    }
    if (!wcscmp( buffer, ime_ui_class.lpszClassName ))
    {
        ok( !params->ime_ui_hwnd, "Found extra IME UI window %p\n", hwnd );
        params->ime_ui_hwnd = hwnd;
    }

    return TRUE;
}

static void test_ImmActivateLayout(void)
{
    const struct ime_call activate_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_SELECT, .select = 1,
        },
        {0},
    };
    const struct ime_call deactivate_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_NOTIFY, .notify = {.action = NI_COMPOSITIONSTR, .index = CPS_CANCEL, .value = 0},
            .todo = TRUE,
        },
        {
            .hkl = default_hkl, .himc = default_himc,
            .func = IME_SELECT, .select = 0,
        },
        {0},
    };
    struct ime_call activate_with_window_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_SELECT, .select = 1,
            .flaky_himc = TRUE,
        },
        {
            .hkl = expect_ime, .himc = 0/*himc*/,
            .func = IME_SELECT, .select = 1,
            .flaky_himc = TRUE,
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_SELECT, .wparam = 1, .lparam = (LPARAM)expect_ime},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_OPENSTATUSWINDOW},
            .todo = TRUE,
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETOPENSTATUS},
            .todo = TRUE,
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETCONVERSIONMODE},
            .todo = TRUE,
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETSENTENCEMODE},
            .todo = TRUE,
        },
        {0},
    };
    struct ime_call deactivate_with_window_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_NOTIFY, .notify = {.action = NI_COMPOSITIONSTR, .index = CPS_CANCEL, .value = 0},
            .todo = TRUE, .flaky_himc = TRUE,
        },
        {
            .hkl = expect_ime, .himc = 0/*himc*/,
            .func = IME_NOTIFY, .notify = {.action = NI_COMPOSITIONSTR, .index = CPS_CANCEL, .value = 0},
            .todo = TRUE, .flaky_himc = TRUE,
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_CLOSESTATUSWINDOW},
            .todo = TRUE,
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_SELECT, .wparam = 0, .lparam = (LPARAM)expect_ime},
        },
        {
            .hkl = default_hkl, .himc = default_himc,
            .func = IME_SELECT, .select = 0,
            .flaky_himc = TRUE,
        },
        {
            .hkl = default_hkl, .himc = 0/*himc*/,
            .func = IME_SELECT, .select = 0,
            .flaky_himc = TRUE,
        },
        {0},
    };
    HKL hkl, old_hkl = GetKeyboardLayout( 0 );
    struct ime_windows ime_windows = {0};
    HIMC himc;
    UINT ret;

    SET_ENABLE( ImeInquire, TRUE );
    SET_ENABLE( ImeDestroy, TRUE );

    ok_ret( 1, ImmActivateLayout( old_hkl ) );

    ime_info.fdwProperty = IME_PROP_END_UNLOAD | IME_PROP_UNICODE;

    if (!(hkl = ime_install())) goto cleanup;

    /* ActivateKeyboardLayout doesn't call ImeInquire / ImeDestroy */

    ok_seq( empty_sequence );
    ok_eq( old_hkl, ActivateKeyboardLayout( hkl, 0 ), HKL, "%p" );
    ok_eq( hkl, GetKeyboardLayout( 0 ), HKL, "%p" );
    ok_eq( hkl, ActivateKeyboardLayout( old_hkl, 0 ), HKL, "%p" );
    ok_seq( empty_sequence );
    ok_eq( old_hkl, GetKeyboardLayout( 0 ), HKL, "%p" );


    /* ImmActivateLayout changes active HKL */

    SET_EXPECT( ImeInquire );
    ok_ret( 1, ImmActivateLayout( hkl ) );
    ok_seq( activate_seq );
    CHECK_CALLED( ImeInquire );
    ok_ret( 1, ImmLoadIME( hkl ) );

    ok_eq( hkl, GetKeyboardLayout( 0 ), HKL, "%p" );

    ok_ret( 1, ImmActivateLayout( hkl ) );
    ok_seq( empty_sequence );

    todo_ImeDestroy = TRUE; /* Wine doesn't leak the IME */
    ok_ret( 1, ImmActivateLayout( old_hkl ) );
    ok_seq( deactivate_seq );
    todo_ImeDestroy = FALSE;

    ok_eq( old_hkl, GetKeyboardLayout( 0 ), HKL, "%p" );

    ime_cleanup( hkl, FALSE );
    ok_seq( empty_sequence );


    /* ImmActivateLayout leaks the IME, we need to free it manually */

    SET_EXPECT( ImeDestroy );
    ret = ImmFreeLayout( hkl );
    ok( ret, "ImmFreeLayout returned %u\n", ret );
    CHECK_CALLED( ImeDestroy );
    ok_seq( empty_sequence );


    /* when there's a window, ActivateKeyboardLayout calls ImeInquire */

    if (!(hkl = ime_install())) goto cleanup;

    hwnd = CreateWindowW( L"static", NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 100, 100, NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );
    process_messages();
    ok_seq( empty_sequence );

    himc = ImmCreateContext();
    ok( !!himc, "got himc %p\n", himc );
    ok_seq( empty_sequence );

    SET_EXPECT( ImeInquire );
    ok_eq( old_hkl, ActivateKeyboardLayout( hkl, 0 ), HKL, "%p" );
    CHECK_CALLED( ImeInquire );
    activate_with_window_seq[1].himc = himc;
    ok_seq( activate_with_window_seq );
    todo_ImeInquire = TRUE;
    ok_ret( 1, ImmLoadIME( hkl ) );
    todo_ImeInquire = FALSE;

    ok_eq( hkl, GetKeyboardLayout( 0 ), HKL, "%p" );

    /* FIXME: ignore spurious VK_CONTROL ImeProcessKey / ImeToAsciiEx calls */
    process_messages();
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    todo_ImeDestroy = TRUE; /* Wine doesn't leak the IME */
    ok_eq( hkl, ActivateKeyboardLayout( old_hkl, 0 ), HKL, "%p" );
    todo_ImeDestroy = FALSE;
    deactivate_with_window_seq[1].himc = himc;
    deactivate_with_window_seq[5].himc = himc;
    ok_seq( deactivate_with_window_seq );

    ok_eq( old_hkl, GetKeyboardLayout( 0 ), HKL, "%p" );

    ok_ret( 1, ImmDestroyContext( himc ) );
    ok_seq( empty_sequence );


    todo_ImeInquire = TRUE;
    ok_eq( old_hkl, ActivateKeyboardLayout( hkl, 0 ), HKL, "%p" );
    todo_ImeInquire = FALSE;
    ok_eq( hkl, GetKeyboardLayout( 0 ), HKL, "%p" );

    ok_ret( 1, EnumThreadWindows( GetCurrentThreadId(), enum_thread_ime_windows, (LPARAM)&ime_windows ) );
    ok( !!ime_windows.ime_hwnd, "missing IME window\n" );
    ok( !!ime_windows.ime_ui_hwnd, "missing IME UI window\n" );
    ok_ret( (UINT_PTR)ime_windows.ime_hwnd, (UINT_PTR)GetParent( ime_windows.ime_ui_hwnd ) );

    todo_ImeDestroy = TRUE; /* Wine doesn't leak the IME */
    ok_eq( hkl, ActivateKeyboardLayout( old_hkl, 0 ), HKL, "%p" );
    todo_ImeDestroy = FALSE;
    ok_eq( old_hkl, GetKeyboardLayout( 0 ), HKL, "%p" );


    SET_EXPECT( ImeDestroy );
    ime_cleanup( hkl, TRUE );
    CHECK_CALLED( ImeDestroy );

    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;


cleanup:
    SET_ENABLE( ImeInquire, FALSE );
    SET_ENABLE( ImeDestroy, FALSE );
}

static void test_ImmCreateInputContext(void)
{
    struct ime_call activate_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_SELECT, .select = 1,
            .flaky_himc = TRUE,
        },
        {
            .hkl = expect_ime, .himc = 0/*himc[0]*/,
            .func = IME_SELECT, .select = 1,
            .flaky_himc = TRUE,
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_SELECT, .wparam = 1, .lparam = (LPARAM)expect_ime},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_OPENSTATUSWINDOW},
            .todo = TRUE,
        },
        {0},
    };
    struct ime_call select1_seq[] =
    {
        {
            .hkl = expect_ime, .himc = 0/*himc[0]*/,
            .func = IME_NOTIFY, .notify = {.action = NI_CONTEXTUPDATED, .index = 0, .value = IMC_SETOPENSTATUS},
            .todo = TRUE, .flaky_himc = TRUE, .broken = TRUE /* sometimes */,
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_NOTIFY, .notify = {.action = NI_CONTEXTUPDATED, .index = 0, .value = IMC_SETOPENSTATUS},
            .todo = TRUE, .flaky_himc = TRUE, .broken = TRUE /* sometimes */,
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETOPENSTATUS},
            .todo = TRUE, .broken = TRUE /* sometimes */,
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETCONVERSIONMODE},
            .todo = TRUE, .broken = TRUE /* sometimes */,
        },
        {
            .hkl = expect_ime, .himc = 0/*himc[1]*/,
            .func = IME_SELECT, .select = 1,
        },
        {0},
    };
    struct ime_call select0_seq[] =
    {
        {
            .hkl = expect_ime, .himc = 0/*himc[1]*/,
            .func = IME_SELECT, .select = 0,
        },
        {0},
    };
    struct ime_call deactivate_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_NOTIFY, .notify = {.action = NI_COMPOSITIONSTR, .index = CPS_CANCEL, .value = 0},
            .todo = TRUE, .flaky_himc = TRUE,
        },
        {
            .hkl = expect_ime, .himc = 0/*himc[0]*/,
            .func = IME_NOTIFY, .notify = {.action = NI_COMPOSITIONSTR, .index = CPS_CANCEL, .value = 0},
            .todo = TRUE, .flaky_himc = TRUE,
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_CLOSESTATUSWINDOW},
            .todo = TRUE,
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_SELECT, .wparam = 0, .lparam = (LPARAM)expect_ime},
        },
        {
            .hkl = default_hkl, .himc = default_himc,
            .func = IME_SELECT, .select = 0,
            .flaky_himc = TRUE,
        },
        {
            .hkl = default_hkl, .himc = 0/*himc[0]*/,
            .func = IME_SELECT, .select = 0,
            .flaky_himc = TRUE,
        },
        {0},
    };
    HKL hkl, old_hkl = GetKeyboardLayout( 0 );
    INPUTCONTEXT *ctx;
    HIMC himc[2];
    HWND hwnd;

    ctx = ImmLockIMC( default_himc );
    ok( !!ctx, "ImmLockIMC failed, error %lu\n", GetLastError() );
    ok_ret( 0, IsWindow( ctx->hWnd ) );
    ok_ret( 1, ImmUnlockIMC( default_himc ) );


    /* new input contexts cannot be locked before IME window has been created */

    himc[0] = ImmCreateContext();
    ok( !!himc[0], "ImmCreateContext failed, error %lu\n", GetLastError() );
    ctx = ImmLockIMC( himc[0] );
    todo_wine
    ok( !ctx, "ImmLockIMC failed, error %lu\n", GetLastError() );
    if (ctx) ImmUnlockIMCC( himc[0] );

    hwnd = CreateWindowW( L"static", NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 100, 100, NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );
    process_messages();

    ctx = ImmLockIMC( default_himc );
    ok( !!ctx, "ImmLockIMC failed, error %lu\n", GetLastError() );
    ok_ret( 1, ImmUnlockIMC( default_himc ) );

    ctx = ImmLockIMC( himc[0] );
    ok( !!ctx, "ImmLockIMC failed, error %lu\n", GetLastError() );
    ok_ret( 1, ImmUnlockIMC( himc[0] ) );


    ime_info.fdwProperty = IME_PROP_END_UNLOAD | IME_PROP_UNICODE;
    ime_info.dwPrivateDataSize = 123;

    if (!(hkl = ime_install())) goto cleanup;

    ok_ret( 1, ImmLoadIME( hkl ) );

    /* Activating the layout calls ImeSelect 1 on existing HIMC */

    ok_seq( empty_sequence );
    ok_ret( 1, ImmActivateLayout( hkl ) );
    activate_seq[1].himc = himc[0];
    ok_seq( activate_seq );

    ok_eq( hkl, GetKeyboardLayout( 0 ), HKL, "%p" );

    ctx = ImmLockIMC( default_himc );
    ok( !!ctx, "ImmLockIMC failed, error %lu\n", GetLastError() );
    ok_ret( 1, ImmUnlockIMC( default_himc ) );

    ctx = ImmLockIMC( himc[0] );
    ok( !!ctx, "ImmLockIMC failed, error %lu\n", GetLastError() );
    ok_ret( 1, ImmUnlockIMC( himc[0] ) );


    /* ImmLockIMC triggers the ImeSelect call, to allocate private data */

    himc[1] = ImmCreateContext();
    ok( !!himc[1], "ImmCreateContext failed, error %lu\n", GetLastError() );

    ok_seq( empty_sequence );
    ctx = ImmLockIMC( himc[1] );
    ok( !!ctx, "ImmLockIMC failed, error %lu\n", GetLastError() );
    select1_seq[0].himc = himc[0];
    select1_seq[4].himc = himc[1];
    ok_seq( select1_seq );

    ok_ret( 1, ImmUnlockIMC( himc[1] ) );

    ok_seq( empty_sequence );
    ok_ret( 1, ImmDestroyContext( himc[1] ) );
    select0_seq[0].himc = himc[1];
    ok_seq( select0_seq );


    /* Deactivating the layout calls ImeSelect 0 on existing HIMC */

    ok_ret( 1, ImmActivateLayout( old_hkl ) );
    deactivate_seq[1].himc = himc[0];
    deactivate_seq[5].himc = himc[0];
    ok_seq( deactivate_seq );

    ok_eq( old_hkl, GetKeyboardLayout( 0 ), HKL, "%p" );

    ime_cleanup( hkl, TRUE );
    ok_seq( empty_sequence );

cleanup:
    ok_ret( 1, ImmDestroyContext( himc[0] ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    ime_info.dwPrivateDataSize = 0;
}

static void test_DefWindowProc(void)
{
    const struct ime_call start_composition_seq[] =
    {
        {.hkl = expect_ime, .himc = default_himc, .func = MSG_IME_UI, .message = {.msg = WM_IME_STARTCOMPOSITION}},
        {0},
    };
    const struct ime_call end_composition_seq[] =
    {
        {.hkl = expect_ime, .himc = default_himc, .func = MSG_IME_UI, .message = {.msg = WM_IME_ENDCOMPOSITION}},
        {0},
    };
    const struct ime_call composition_seq[] =
    {
        {.hkl = expect_ime, .himc = default_himc, .func = MSG_IME_UI, .message = {.msg = WM_IME_COMPOSITION}},
        {0},
    };
    const struct ime_call set_context_seq[] =
    {
        {.hkl = expect_ime, .himc = default_himc, .func = MSG_IME_UI, .message = {.msg = WM_IME_SETCONTEXT}},
        {0},
    };
    const struct ime_call notify_seq[] =
    {
        {.hkl = expect_ime, .himc = default_himc, .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY}},
        {0},
    };
    HKL hkl, old_hkl = GetKeyboardLayout( 0 );
    UINT_PTR ret;

    ime_info.fdwProperty = IME_PROP_END_UNLOAD | IME_PROP_UNICODE;

    if (!(hkl = ime_install())) return;

    hwnd = CreateWindowW( L"static", NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 100, 100, NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );

    ok_ret( 1, ImmActivateLayout( hkl ) );
    ok_ret( 1, ImmLoadIME( hkl ) );
    process_messages();
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    ok_ret( 0, DefWindowProcW( hwnd, WM_IME_STARTCOMPOSITION, 0, 0 ) );
    ok_seq( start_composition_seq );
    ok_ret( 0, DefWindowProcW( hwnd, WM_IME_ENDCOMPOSITION, 0, 0 ) );
    ok_seq( end_composition_seq );
    ok_ret( 0, DefWindowProcW( hwnd, WM_IME_COMPOSITION, 0, 0 ) );
    ok_seq( composition_seq );
    ok_ret( 0, DefWindowProcW( hwnd, WM_IME_SETCONTEXT, 0, 0 ) );
    ok_seq( set_context_seq );
    ok_ret( 0, DefWindowProcW( hwnd, WM_IME_NOTIFY, 0, 0 ) );
    ok_seq( notify_seq );
    ok_ret( 0, DefWindowProcW( hwnd, WM_IME_CONTROL, 0, 0 ) );
    ok_seq( empty_sequence );
    ok_ret( 0, DefWindowProcW( hwnd, WM_IME_COMPOSITIONFULL, 0, 0 ) );
    ok_seq( empty_sequence );
    ok_ret( 0, DefWindowProcW( hwnd, WM_IME_SELECT, 0, 0 ) );
    ok_seq( empty_sequence );
    ok_ret( 0, DefWindowProcW( hwnd, WM_IME_CHAR, 0, 0 ) );
    ok_seq( empty_sequence );
    ok_ret( 0, DefWindowProcW( hwnd, 0x287, 0, 0 ) );
    ok_seq( empty_sequence );
    ok_ret( 0, DefWindowProcW( hwnd, WM_IME_REQUEST, 0, 0 ) );
    ok_seq( empty_sequence );
    ret = DefWindowProcW( hwnd, WM_IME_KEYDOWN, 0, 0 );
    todo_wine
    ok_ret( 0, ret );
    ok_seq( empty_sequence );
    ret = DefWindowProcW( hwnd, WM_IME_KEYUP, 0, 0 );
    todo_wine
    ok_ret( 0, ret );
    ok_seq( empty_sequence );

    ok_ret( 1, ImmActivateLayout( old_hkl ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

    ime_cleanup( hkl, TRUE );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;
}

static void test_ImmSetActiveContext(void)
{
    const struct ime_call activate_0_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_SET_ACTIVE_CONTEXT, .set_active_context = {.flag = 1}
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_SETCONTEXT, .wparam = 1, .lparam = ISC_SHOWUIALL}
        },
        {0},
    };
    const struct ime_call deactivate_0_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_SET_ACTIVE_CONTEXT, .set_active_context = {.flag = 0}
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_SETCONTEXT, .wparam = 0, .lparam = ISC_SHOWUIALL}
        },
        {0},
    };
    struct ime_call deactivate_1_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_NOTIFY, .notify = {.action = NI_CONTEXTUPDATED, .index = 0, .value = IMC_SETOPENSTATUS},
            .todo = TRUE, .flaky_himc = TRUE, .broken = TRUE /* sometimes */,
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETOPENSTATUS},
            .todo = TRUE, .broken = TRUE /* sometimes */,
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETCONVERSIONMODE},
            .todo = TRUE, .broken = TRUE /* sometimes */,
        },
        {
            .hkl = expect_ime, .himc = 0/*himc*/,
            .func = IME_SELECT, .select = 1,
        },
        {
            .hkl = expect_ime, .himc = 0/*himc*/,
            .func = IME_SET_ACTIVE_CONTEXT, .set_active_context = {.flag = 0}
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_SETCONTEXT, .wparam = 0, .lparam = ISC_SHOWUIALL}
        },
        {0},
    };
    struct ime_call activate_1_seq[] =
    {
        {
            .hkl = expect_ime, .himc = 0/*himc*/,
            .func = IME_SET_ACTIVE_CONTEXT, .set_active_context = {.flag = 1}
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_SETCONTEXT, .wparam = 1, .lparam = ISC_SHOWUIALL}
        },
        {0},
    };
    HKL hkl, old_hkl = GetKeyboardLayout( 0 );
    HIMC himc;

    ime_info.fdwProperty = IME_PROP_END_UNLOAD | IME_PROP_UNICODE;

    if (!(hkl = ime_install())) return;

    hwnd = CreateWindowW( L"static", NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 100, 100, NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );

    ok_ret( 1, ImmActivateLayout( hkl ) );
    ok_ret( 1, ImmLoadIME( hkl ) );
    process_messages();
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    SetLastError( 0xdeadbeef );
    ok_ret( 1, ImmSetActiveContext( hwnd, default_himc, TRUE ) );
    ok_seq( activate_0_seq );
    ok_ret( 0, GetLastError() );
    ok_ret( 1, ImmSetActiveContext( hwnd, default_himc, TRUE ) );
    ok_seq( activate_0_seq );
    ok_ret( 1, ImmSetActiveContext( hwnd, default_himc, FALSE ) );
    ok_seq( deactivate_0_seq );

    himc = ImmCreateContext();
    ok_ne( NULL, himc, HIMC, "%p" );
    ok_ret( 1, ImmSetActiveContext( hwnd, himc, FALSE ) );
    deactivate_1_seq[3].himc = himc;
    deactivate_1_seq[4].himc = himc;
    ok_seq( deactivate_1_seq );
    ok_ret( 1, ImmSetActiveContext( hwnd, himc, TRUE ) );
    activate_1_seq[0].himc = himc;
    ok_seq( activate_1_seq );
    ok_ret( 1, ImmDestroyContext( himc ) );

    ok_ret( 1, ImmActivateLayout( old_hkl ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

    ime_cleanup( hkl, TRUE );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;
}

static void test_ImmRequestMessage(void)
{
    struct ime_call composition_window_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_TEST_WIN, .message = {.msg = WM_IME_REQUEST, .wparam = IMR_COMPOSITIONWINDOW, .lparam = 0/*&comp_form*/}
        },
        {0},
    };
    struct ime_call candidate_window_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_TEST_WIN, .message = {.msg = WM_IME_REQUEST, .wparam = IMR_CANDIDATEWINDOW, .lparam = 0/*&cand_form*/}
        },
        {0},
    };
    struct ime_call composition_font_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_TEST_WIN, .message = {.msg = WM_IME_REQUEST, .wparam = IMR_COMPOSITIONFONT, .lparam = 0/*&log_font*/}
        },
        {0},
    };
    struct ime_call reconvert_string_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_TEST_WIN, .message = {.msg = WM_IME_REQUEST, .wparam = IMR_RECONVERTSTRING, .lparam = 0/*&reconv*/}
        },
        {0},
    };
    struct ime_call confirm_reconvert_string_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_TEST_WIN, .message = {.msg = WM_IME_REQUEST, .wparam = IMR_CONFIRMRECONVERTSTRING, .lparam = 0/*&reconv*/}
        },
        {0},
    };
    struct ime_call query_char_position_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_TEST_WIN, .message = {.msg = WM_IME_REQUEST, .wparam = IMR_QUERYCHARPOSITION, .lparam = 0/*&char_pos*/}
        },
        {0},
    };
    struct ime_call document_feed_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_TEST_WIN, .message = {.msg = WM_IME_REQUEST, .wparam = IMR_DOCUMENTFEED, .lparam = 0/*&reconv*/}
        },
        {0},
    };
    HKL hkl, old_hkl = GetKeyboardLayout( 0 );
    COMPOSITIONFORM comp_form = {0};
    IMECHARPOSITION char_pos = {0};
    RECONVERTSTRING reconv = {0};
    CANDIDATEFORM cand_form = {0};
    LOGFONTW log_font = {0};
    INPUTCONTEXT *ctx;
    HIMC himc;

    if (!(hkl = ime_install())) return;

    hwnd = CreateWindowW( test_class.lpszClassName, NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 100, 100, NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );

    ime_info.fdwProperty = IME_PROP_END_UNLOAD | IME_PROP_UNICODE;

    ok_ret( 1, ImmActivateLayout( hkl ) );
    ok_ret( 1, ImmLoadIME( hkl ) );
    himc = ImmCreateContext();
    ok_ne( NULL, himc, HIMC, "%p" );
    ctx = ImmLockIMC( himc );
    ok_ne( NULL, ctx, INPUTCONTEXT *, "%p" );
    process_messages();
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    ok_ret( 0, ImmRequestMessageW( default_himc, 0xdeadbeef, 0 ) );
    todo_wine ok_seq( empty_sequence );
    ok_ret( 0, ImmRequestMessageW( default_himc, IMR_COMPOSITIONWINDOW, (LPARAM)&comp_form ) );
    composition_window_seq[0].message.lparam = (LPARAM)&comp_form;
    ok_seq( composition_window_seq );
    ok_ret( 0, ImmRequestMessageW( default_himc, IMR_CANDIDATEWINDOW, (LPARAM)&cand_form ) );
    candidate_window_seq[0].message.lparam = (LPARAM)&cand_form;
    ok_seq( candidate_window_seq );
    ok_ret( 0, ImmRequestMessageW( default_himc, IMR_COMPOSITIONFONT, (LPARAM)&log_font ) );
    composition_font_seq[0].message.lparam = (LPARAM)&log_font;
    ok_seq( composition_font_seq );
    ok_ret( 0, ImmRequestMessageW( default_himc, IMR_RECONVERTSTRING, (LPARAM)&reconv ) );
    todo_wine ok_seq( empty_sequence );
    reconv.dwSize = sizeof(RECONVERTSTRING);
    ok_ret( 0, ImmRequestMessageW( default_himc, IMR_RECONVERTSTRING, (LPARAM)&reconv ) );
    reconvert_string_seq[0].message.lparam = (LPARAM)&reconv;
    ok_seq( reconvert_string_seq );
    ok_ret( 0, ImmRequestMessageW( default_himc, IMR_CONFIRMRECONVERTSTRING, (LPARAM)&reconv ) );
    confirm_reconvert_string_seq[0].message.lparam = (LPARAM)&reconv;
    ok_seq( confirm_reconvert_string_seq );
    ok_ret( 0, ImmRequestMessageW( default_himc, IMR_QUERYCHARPOSITION, (LPARAM)&char_pos ) );
    query_char_position_seq[0].message.lparam = (LPARAM)&char_pos;
    ok_seq( query_char_position_seq );
    ok_ret( 0, ImmRequestMessageW( default_himc, IMR_DOCUMENTFEED, (LPARAM)&reconv ) );
    document_feed_seq[0].message.lparam = (LPARAM)&reconv;
    ok_seq( document_feed_seq );

    ok_ret( 0, ImmRequestMessageW( himc, IMR_CANDIDATEWINDOW, (LPARAM)&cand_form ) );
    ok_seq( empty_sequence );
    ok_ret( 1, ImmSetActiveContext( hwnd, himc, TRUE ) );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;
    ok_ret( 0, ImmRequestMessageW( himc, IMR_CANDIDATEWINDOW, (LPARAM)&cand_form ) );
    candidate_window_seq[0].message.lparam = (LPARAM)&cand_form;
    ok_seq( candidate_window_seq );
    ok_ret( 0, ImmRequestMessageW( default_himc, IMR_CANDIDATEWINDOW, (LPARAM)&cand_form ) );
    ok_seq( candidate_window_seq );

    ok_ret( 1, ImmUnlockIMC( himc ) );
    ok_ret( 1, ImmDestroyContext( himc ) );

    ok_ret( 1, ImmActivateLayout( old_hkl ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

    ime_cleanup( hkl, TRUE );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;
}

static void test_ImmGetCandidateList( BOOL unicode )
{
    char buffer[512], expect_bufW[512] = {0}, expect_bufA[512] = {0};
    CANDIDATELIST *cand_list = (CANDIDATELIST *)buffer, *expect_listW, *expect_listA;
    HKL hkl, old_hkl = GetKeyboardLayout( 0 );
    CANDIDATEINFO *cand_info;
    INPUTCONTEXT *ctx;
    HIMC himc;

    expect_listW = (CANDIDATELIST *)expect_bufW;
    expect_listW->dwSize = offsetof(CANDIDATELIST, dwOffset[2]) + 32 * sizeof(WCHAR);
    expect_listW->dwStyle = 0xdeadbeef;
    expect_listW->dwCount = 2;
    expect_listW->dwSelection = 3;
    expect_listW->dwPageStart = 4;
    expect_listW->dwPageSize = 5;
    expect_listW->dwOffset[0] = offsetof(CANDIDATELIST, dwOffset[2]) + 2 * sizeof(WCHAR);
    expect_listW->dwOffset[1] = offsetof(CANDIDATELIST, dwOffset[2]) + 16 * sizeof(WCHAR);
    wcscpy( (WCHAR *)(expect_bufW + expect_listW->dwOffset[0]), L"Candidate 1" );
    wcscpy( (WCHAR *)(expect_bufW + expect_listW->dwOffset[1]), L"Candidate 2" );

    expect_listA = (CANDIDATELIST *)expect_bufA;
    expect_listA->dwSize = offsetof(CANDIDATELIST, dwOffset[2]) + 32;
    expect_listA->dwStyle = 0xdeadbeef;
    expect_listA->dwCount = 2;
    expect_listA->dwSelection = 3;
    expect_listA->dwPageStart = 4;
    expect_listA->dwPageSize = 5;
    expect_listA->dwOffset[0] = offsetof(CANDIDATELIST, dwOffset[2]) + 2;
    expect_listA->dwOffset[1] = offsetof(CANDIDATELIST, dwOffset[2]) + 16;
    strcpy( (char *)(expect_bufA + expect_listA->dwOffset[0]), "Candidate 1" );
    strcpy( (char *)(expect_bufA + expect_listA->dwOffset[1]), "Candidate 2" );

    winetest_push_context( unicode ? "unicode" : "ansi" );

    /* IME_PROP_END_UNLOAD for the IME to unload / reload. */
    ime_info.fdwProperty = IME_PROP_END_UNLOAD;
    if (unicode) ime_info.fdwProperty |= IME_PROP_UNICODE;

    if (!(hkl = ime_install())) goto cleanup;

    hwnd = CreateWindowW( test_class.lpszClassName, NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 100, 100, NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );

    ok_ret( 1, ImmActivateLayout( hkl ) );
    ok_ret( 1, ImmLoadIME( hkl ) );
    himc = ImmCreateContext();
    ok_ne( NULL, himc, HIMC, "%p" );
    ctx = ImmLockIMC( himc );
    ok_ne( NULL, ctx, INPUTCONTEXT *, "%p" );
    process_messages();
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    ok_ret( 0, ImmGetCandidateListW( default_himc, 0, NULL, 0 ) );
    ok_seq( empty_sequence );
    ok_ret( 0, ImmGetCandidateListW( default_himc, 1, NULL, 0 ) );
    ok_seq( empty_sequence );
    ok_ret( 0, ImmGetCandidateListW( default_himc, 0, cand_list, sizeof(buffer) ) );
    ok_seq( empty_sequence );

    ok_ret( 0, ImmGetCandidateListW( himc, 0, cand_list, sizeof(buffer) ) );
    ok_seq( empty_sequence );

    todo_wine ok_seq( empty_sequence );
    ctx->hCandInfo = ImmReSizeIMCC( ctx->hCandInfo, sizeof(*cand_info) + sizeof(buffer) );
    ok( !!ctx->hCandInfo, "ImmReSizeIMCC failed, error %lu\n", GetLastError() );

    cand_info = ImmLockIMCC( ctx->hCandInfo );
    ok( !!cand_info, "ImmLockIMCC failed, error %lu\n", GetLastError() );
    cand_info->dwCount = 1;
    cand_info->dwOffset[0] = sizeof(*cand_info);
    if (unicode) memcpy( cand_info + 1, expect_bufW, sizeof(expect_bufW) );
    else memcpy( cand_info + 1, expect_bufA, sizeof(expect_bufA) );
    ok_ret( 0, ImmUnlockIMCC( ctx->hCandInfo ) );

    ok_ret( (unicode ? 96 : 80), ImmGetCandidateListW( himc, 0, NULL, 0 ) );
    ok_seq( empty_sequence );
    ok_ret( 0, ImmGetCandidateListW( himc, 1, NULL, 0 ) );
    ok_seq( empty_sequence );
    memset( buffer, 0xcd, sizeof(buffer) );
    ok_ret( (unicode ? 96 : 80), ImmGetCandidateListW( himc, 0, cand_list, sizeof(buffer) ) );
    ok_seq( empty_sequence );

    if (!unicode)
    {
        expect_listW->dwSize = offsetof(CANDIDATELIST, dwOffset[2]) + 24 * sizeof(WCHAR);
        expect_listW->dwOffset[0] = offsetof(CANDIDATELIST, dwOffset[2]);
        expect_listW->dwOffset[1] = offsetof(CANDIDATELIST, dwOffset[2]) + 12 * sizeof(WCHAR);
        wcscpy( (WCHAR *)(expect_bufW + expect_listW->dwOffset[0]), L"Candidate 1" );
        wcscpy( (WCHAR *)(expect_bufW + expect_listW->dwOffset[1]), L"Candidate 2" );
    }
    check_candidate_list_( __LINE__, cand_list, expect_listW, TRUE );

    ok_ret( (unicode ? 56 : 64), ImmGetCandidateListA( himc, 0, NULL, 0 ) );
    ok_seq( empty_sequence );
    ok_ret( 0, ImmGetCandidateListA( himc, 1, NULL, 0 ) );
    ok_seq( empty_sequence );
    memset( buffer, 0xcd, sizeof(buffer) );
    ok_ret( (unicode ? 56 : 64), ImmGetCandidateListA( himc, 0, cand_list, sizeof(buffer) ) );
    ok_seq( empty_sequence );

    if (unicode)
    {
        expect_listA->dwSize = offsetof(CANDIDATELIST, dwOffset[2]) + 24;
        expect_listA->dwOffset[0] = offsetof(CANDIDATELIST, dwOffset[2]);
        expect_listA->dwOffset[1] = offsetof(CANDIDATELIST, dwOffset[2]) + 12;
        strcpy( (char *)(expect_bufA + expect_listA->dwOffset[0]), "Candidate 1" );
        strcpy( (char *)(expect_bufA + expect_listA->dwOffset[1]), "Candidate 2" );
    }
    check_candidate_list_( __LINE__, cand_list, expect_listA, FALSE );

    ok_ret( 1, ImmUnlockIMC( himc ) );
    ok_ret( 1, ImmDestroyContext( himc ) );

    ok_ret( 1, ImmActivateLayout( old_hkl ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

    ime_cleanup( hkl, TRUE );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

cleanup:
    winetest_pop_context();
}

static void test_ImmGetCandidateListCount( BOOL unicode )
{
    HKL hkl, old_hkl = GetKeyboardLayout( 0 );
    CANDIDATEINFO *cand_info;
    INPUTCONTEXT *ctx;
    DWORD count;
    HIMC himc;

    winetest_push_context( unicode ? "unicode" : "ansi" );

    /* IME_PROP_END_UNLOAD for the IME to unload / reload. */
    ime_info.fdwProperty = IME_PROP_END_UNLOAD;
    if (unicode) ime_info.fdwProperty |= IME_PROP_UNICODE;

    if (!(hkl = ime_install())) goto cleanup;

    hwnd = CreateWindowW( test_class.lpszClassName, NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 100, 100, NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );

    ok_ret( 1, ImmActivateLayout( hkl ) );
    ok_ret( 1, ImmLoadIME( hkl ) );
    himc = ImmCreateContext();
    ok_ne( NULL, himc, HIMC, "%p" );
    ctx = ImmLockIMC( himc );
    ok_ne( NULL, ctx, INPUTCONTEXT *, "%p" );
    process_messages();
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    ok_ret( 144, ImmGetCandidateListCountW( default_himc, &count ) );
    ok_eq( 0, count, UINT, "%u" );
    ok_seq( empty_sequence );
    ok_ret( 144, ImmGetCandidateListCountA( default_himc, &count ) );
    ok_eq( 0, count, UINT, "%u" );
    ok_seq( empty_sequence );

    ok_ret( 144, ImmGetCandidateListCountW( himc, &count ) );
    ok_eq( 0, count, UINT, "%u" );
    ok_seq( empty_sequence );
    ok_ret( 144, ImmGetCandidateListCountA( himc, &count ) );
    ok_eq( 0, count, UINT, "%u" );
    ok_seq( empty_sequence );

    cand_info = ImmLockIMCC( ctx->hCandInfo );
    ok( !!cand_info, "ImmLockIMCC failed, error %lu\n", GetLastError() );
    cand_info->dwCount = 1;
    ok_ret( 0, ImmUnlockIMCC( ctx->hCandInfo ) );

    todo_wine_if(!unicode)
    ok_ret( (unicode ? 144 : 172), ImmGetCandidateListCountW( himc, &count ) );
    ok_eq( 1, count, UINT, "%u" );
    ok_seq( empty_sequence );
    todo_wine_if(unicode)
    ok_ret( (unicode ? 172 : 144), ImmGetCandidateListCountA( himc, &count ) );
    ok_eq( 1, count, UINT, "%u" );
    ok_seq( empty_sequence );

    ok_ret( 1, ImmUnlockIMC( himc ) );
    ok_ret( 1, ImmDestroyContext( himc ) );

    ok_ret( 1, ImmActivateLayout( old_hkl ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

    ime_cleanup( hkl, TRUE );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

cleanup:
    winetest_pop_context();
}

static void test_ImmGetCandidateWindow(void)
{
    HKL hkl, old_hkl = GetKeyboardLayout( 0 );
    const CANDIDATEFORM expect_form =
    {
        .dwIndex = 0xdeadbeef,
        .dwStyle = 0xfeedcafe,
        .ptCurrentPos = {.x = 123, .y = 456},
        .rcArea = {.left = 1, .top = 2, .right = 3, .bottom = 4},
    };
    CANDIDATEFORM cand_form;
    INPUTCONTEXT *ctx;
    HIMC himc;

    ime_info.fdwProperty = IME_PROP_END_UNLOAD | IME_PROP_UNICODE;

    if (!(hkl = ime_install())) return;

    hwnd = CreateWindowW( test_class.lpszClassName, NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 100, 100, NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );

    ok_ret( 1, ImmActivateLayout( hkl ) );
    ok_ret( 1, ImmLoadIME( hkl ) );
    himc = ImmCreateContext();
    ok_ne( NULL, himc, HIMC, "%p" );
    ctx = ImmLockIMC( himc );
    ok_ne( NULL, ctx, INPUTCONTEXT *, "%p" );
    process_messages();
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    memset( &cand_form, 0xcd, sizeof(cand_form) );
    ok_ret( 0, ImmGetCandidateWindow( default_himc, 0, &cand_form ) );
    ok_eq( 0xcdcdcdcd, cand_form.dwIndex, UINT, "%u" );
    ok_ret( 0, ImmGetCandidateWindow( default_himc, 1, &cand_form ) );
    ok_eq( 0xcdcdcdcd, cand_form.dwIndex, UINT, "%u" );
    ok_ret( 0, ImmGetCandidateWindow( default_himc, 2, &cand_form ) );
    ok_eq( 0xcdcdcdcd, cand_form.dwIndex, UINT, "%u" );
    ok_ret( 0, ImmGetCandidateWindow( default_himc, 3, &cand_form ) );
    ok_eq( 0xcdcdcdcd, cand_form.dwIndex, UINT, "%u" );
    todo_wine ok_ret( 1, ImmGetCandidateWindow( default_himc, 4, &cand_form ) );
    ok_seq( empty_sequence );

    ok_ret( 0, ImmGetCandidateWindow( himc, 0, &cand_form ) );
    ok_seq( empty_sequence );

    todo_wine ok_seq( empty_sequence );
    ok( !!ctx, "ImmLockIMC failed, error %lu\n", GetLastError() );
    ctx->cfCandForm[0] = expect_form;

    todo_wine ok_ret( 1, ImmGetCandidateWindow( himc, 0, &cand_form ) );
    todo_wine check_candidate_form( &cand_form, &expect_form );
    ok_seq( empty_sequence );

    todo_wine ok_seq( empty_sequence );
    ok( !!ctx, "ImmLockIMC failed, error %lu\n", GetLastError() );
    ctx->cfCandForm[0].dwIndex = -1;

    ok_ret( 0, ImmGetCandidateWindow( himc, 0, &cand_form ) );
    ok_seq( empty_sequence );

    ok_ret( 1, ImmUnlockIMC( himc ) );
    ok_ret( 1, ImmDestroyContext( himc ) );

    ok_ret( 1, ImmActivateLayout( old_hkl ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

    ime_cleanup( hkl, TRUE );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;
}

START_TEST(imm32)
{
    default_hkl = GetKeyboardLayout( 0 );

    test_class.hInstance = GetModuleHandleW( NULL );
    RegisterClassExW( &test_class );

    if (!is_ime_enabled())
    {
        win_skip("IME support not implemented\n");
        return;
    }

    test_com_initialization();

    test_ImmEnumInputContext();

    test_ImmInstallIME();
    test_ImmGetDescription();
    test_ImmGetIMEFileName();
    test_ImmIsIME();
    test_ImmGetProperty();

    test_ImmEscape( FALSE );
    test_ImmEscape( TRUE );
    test_ImmEnumRegisterWord( FALSE );
    test_ImmEnumRegisterWord( TRUE );
    test_ImmRegisterWord( FALSE );
    test_ImmRegisterWord( TRUE );
    test_ImmGetRegisterWordStyle( FALSE );
    test_ImmGetRegisterWordStyle( TRUE );
    test_ImmUnregisterWord( FALSE );
    test_ImmUnregisterWord( TRUE );

    /* test these first to sanitize conversion / open statuses */
    test_ImmSetConversionStatus();
    test_ImmSetOpenStatus();
    ImeSelect_init_status = TRUE;

    test_ImmActivateLayout();
    test_ImmCreateInputContext();
    test_ImmProcessKey();
    test_DefWindowProc();
    test_ImmSetActiveContext();
    test_ImmRequestMessage();

    test_ImmGetCandidateList( TRUE );
    test_ImmGetCandidateList( FALSE );
    test_ImmGetCandidateListCount( TRUE );
    test_ImmGetCandidateListCount( FALSE );
    test_ImmGetCandidateWindow();

    if (init())
    {
        test_ImmNotifyIME();
        test_ImmGetCompositionString();
        test_ImmSetCompositionString();
        test_ImmIME();
        test_ImmAssociateContextEx();
        test_NtUserAssociateInputContext();
        test_ImmThreads();
        test_ImmIsUIMessage();
        test_ImmGetContext();
        test_ImmDefaultHwnd();
        test_default_ime_window_creation();
        test_ImmGetIMCLockCount();
        test_ImmGetIMCCLockCount();
        test_ImmDestroyContext();
        test_ImmDestroyIMCC();
        test_InvalidIMC();
        msg_spy_cleanup();
        /* Reinitialize the hooks to capture all windows */
        msg_spy_init(NULL);
        test_ImmMessages();
        msg_spy_cleanup();
        if (pSendInput)
            test_ime_processkey();
        else win_skip("SendInput is not available\n");

        /* there's no way of enabling IME - keep the test last */
        test_ImmDisableIME();
    }
    cleanup();

    UnregisterClassW( test_class.lpszClassName, test_class.hInstance );
}
