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

static const char *debugstr_wm_ime( UINT msg )
{
    switch (msg)
    {
    case WM_IME_STARTCOMPOSITION: return "WM_IME_STARTCOMPOSITION";
    case WM_IME_ENDCOMPOSITION: return "WM_IME_ENDCOMPOSITION";
    case WM_IME_COMPOSITION: return "WM_IME_COMPOSITION";
    case WM_IME_SETCONTEXT: return "WM_IME_SETCONTEXT";
    case WM_IME_NOTIFY: return "WM_IME_NOTIFY";
    case WM_IME_CONTROL: return "WM_IME_CONTROL";
    case WM_IME_COMPOSITIONFULL: return "WM_IME_COMPOSITIONFULL";
    case WM_IME_SELECT: return "WM_IME_SELECT";
    case WM_IME_CHAR: return "WM_IME_CHAR";
    case WM_IME_REQUEST: return "WM_IME_REQUEST";
    case WM_IME_KEYDOWN: return "WM_IME_KEYDOWN";
    case WM_IME_KEYUP: return "WM_IME_KEYUP";
    default: return wine_dbg_sprintf( "%#x", msg );
    }
}

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

#define check_member_wstr_( file, line, val, exp, member )                                         \
    ok_(file, line)( !wcscmp( (val).member, (exp).member ), "got " #member " %s\n",                \
                     debugstr_w((val).member) )
#define check_member_wstr( val, exp, member )                                                      \
    check_member_wstr_( __FILE__, __LINE__, val, exp, member )

#define check_member_str_( file, line, val, exp, member )                                          \
    ok_(file, line)( !strcmp( (val).member, (exp).member ), "got " #member " %s\n",                \
                     debugstr_a((val).member) )
#define check_member_str( val, exp, member )                                                       \
    check_member_str_( __FILE__, __LINE__, val, exp, member )

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

#define check_composition_string( a, b ) check_composition_string_( __LINE__, a, b )
static void check_composition_string_( int line, COMPOSITIONSTRING *string, const COMPOSITIONSTRING *expect )
{
    check_member_( __FILE__, line, *string, *expect, "%lu", dwSize );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwCompReadAttrLen );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwCompReadAttrOffset );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwCompReadClauseLen );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwCompReadClauseOffset );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwCompReadStrLen );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwCompReadStrOffset );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwCompAttrLen );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwCompAttrOffset );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwCompClauseLen );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwCompClauseOffset );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwCompStrLen );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwCompStrOffset );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwCursorPos );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwDeltaStart );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwResultReadClauseLen );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwResultReadClauseOffset );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwResultReadStrLen );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwResultReadStrOffset );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwResultClauseLen );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwResultClauseOffset );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwResultStrLen );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwResultStrOffset );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwPrivateSize );
    check_member_( __FILE__, line, *string, *expect, "%lu", dwPrivateOffset );
}

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

#define check_composition_form( a, b ) check_composition_form_( __LINE__, a, b )
static void check_composition_form_( int line, COMPOSITIONFORM *form, const COMPOSITIONFORM *expect )
{
    check_member_( __FILE__, line, *form, *expect, "%#lx", dwStyle );
    check_member_point_( __FILE__, line, *form, *expect, ptCurrentPos );
    check_member_rect_( __FILE__, line, *form, *expect, rcArea );
}

#define check_logfont_w( a, b ) check_logfont_w_( __LINE__, a, b )
static void check_logfont_w_( int line, LOGFONTW *font, const LOGFONTW *expect )
{
    check_member_( __FILE__, line, *font, *expect, "%lu", lfHeight );
    check_member_( __FILE__, line, *font, *expect, "%lu", lfWidth );
    check_member_( __FILE__, line, *font, *expect, "%lu", lfEscapement );
    check_member_( __FILE__, line, *font, *expect, "%lu", lfOrientation );
    check_member_( __FILE__, line, *font, *expect, "%lu", lfWeight );
    check_member_( __FILE__, line, *font, *expect, "%u", lfItalic );
    check_member_( __FILE__, line, *font, *expect, "%u", lfUnderline );
    check_member_( __FILE__, line, *font, *expect, "%u", lfStrikeOut );
    check_member_( __FILE__, line, *font, *expect, "%u", lfCharSet );
    check_member_( __FILE__, line, *font, *expect, "%u", lfOutPrecision );
    check_member_( __FILE__, line, *font, *expect, "%u", lfClipPrecision );
    check_member_( __FILE__, line, *font, *expect, "%u", lfQuality );
    check_member_( __FILE__, line, *font, *expect, "%u", lfPitchAndFamily );
    check_member_wstr_( __FILE__, line, *font, *expect, lfFaceName );
}

#define check_logfont_a( a, b ) check_logfont_a_( __LINE__, a, b )
static void check_logfont_a_( int line, LOGFONTA *font, const LOGFONTA *expect )
{
    check_member_( __FILE__, line, *font, *expect, "%lu", lfHeight );
    check_member_( __FILE__, line, *font, *expect, "%lu", lfWidth );
    check_member_( __FILE__, line, *font, *expect, "%lu", lfEscapement );
    check_member_( __FILE__, line, *font, *expect, "%lu", lfOrientation );
    check_member_( __FILE__, line, *font, *expect, "%lu", lfWeight );
    check_member_( __FILE__, line, *font, *expect, "%u", lfItalic );
    check_member_( __FILE__, line, *font, *expect, "%u", lfUnderline );
    check_member_( __FILE__, line, *font, *expect, "%u", lfStrikeOut );
    check_member_( __FILE__, line, *font, *expect, "%u", lfCharSet );
    check_member_( __FILE__, line, *font, *expect, "%u", lfOutPrecision );
    check_member_( __FILE__, line, *font, *expect, "%u", lfClipPrecision );
    check_member_( __FILE__, line, *font, *expect, "%u", lfQuality );
    check_member_( __FILE__, line, *font, *expect, "%u", lfPitchAndFamily );
    check_member_str_( __FILE__, line, *font, *expect, lfFaceName );
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

#define process_messages() process_messages_(0)
static void process_messages_(HWND hwnd)
{
    MSG msg;

    while (PeekMessageA( &msg, hwnd, 0, 0, PM_REMOVE ))
    {
        TranslateMessage( &msg );
        DispatchMessageA( &msg );
    }
}

/* try to make sure pending X events have been processed before continuing */
#define flush_events() flush_events_( 100, 200 )
static void flush_events_( int min_timeout, int max_timeout )
{
    DWORD time = GetTickCount() + max_timeout;
    MSG msg;

    while (max_timeout > 0)
    {
        if (MsgWaitForMultipleObjects( 0, NULL, FALSE, min_timeout, QS_ALLINPUT ) == WAIT_TIMEOUT) break;
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            DispatchMessageA( &msg );
        }
        max_timeout = time - GetTickCount();
    }
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
static BOOL todo_ImeSetCompositionString;
DEFINE_EXPECT( ImeSetCompositionString );
static BOOL todo_IME_DLL_PROCESS_ATTACH;
DEFINE_EXPECT( IME_DLL_PROCESS_ATTACH );
static BOOL todo_IME_DLL_PROCESS_DETACH;
DEFINE_EXPECT( IME_DLL_PROCESS_DETACH );

static IMEINFO ime_info;
static UINT ime_count;
static WCHAR ime_path[MAX_PATH];
static HIMC default_himc;
static HKL default_hkl, wineime_hkl;
static HKL expect_ime = (HKL)(int)0xe020047f;

enum ime_function
{
    IME_SELECT = 1,
    IME_NOTIFY,
    IME_PROCESS_KEY,
    IME_TO_ASCII_EX,
    IME_SET_ACTIVE_CONTEXT,
    MSG_IME_UI,
    MSG_TEST_WIN,
};

struct ime_call
{
    HKL hkl;
    HIMC himc;
    enum ime_function func;

    WCHAR comp[16];
    WCHAR result[16];

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
            LPARAM lparam;
        } process_key;
        struct
        {
            UINT vkey;
            UINT vsc;
            UINT flags;
        } to_ascii_ex;
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
    BOOL todo_value;
    BOOL todo_himc;
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
    if (!expected->flaky_himc && (ret = (UINT_PTR)expected->himc - (UINT_PTR)received->himc))
    {
        /* on some Wine configurations the IME UI doesn't get an HIMC */
        if (!winetest_platform_is_wine || !expected->todo_himc) goto done;
        else todo_wine ok( 0, "got himc %p\n", received->himc );
    }

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
        if ((ret = expected->process_key.lparam - received->process_key.lparam)) goto done;
        break;
    case IME_TO_ASCII_EX:
        if ((ret = expected->to_ascii_ex.vkey - received->to_ascii_ex.vkey)) goto done;
        if ((ret = expected->to_ascii_ex.vsc - received->to_ascii_ex.vsc)) goto done;
        if ((ret = expected->to_ascii_ex.flags - received->to_ascii_ex.flags)) goto done;
        break;
    case IME_SET_ACTIVE_CONTEXT:
        if ((ret = expected->set_active_context.flag - received->set_active_context.flag)) goto done;
        break;
    case MSG_IME_UI:
    case MSG_TEST_WIN:
        if ((ret = expected->message.msg - received->message.msg)) goto done;
        if ((ret = (expected->message.wparam - received->message.wparam))) goto done;
        if ((ret = (expected->message.lparam - received->message.lparam))) goto done;
        if ((ret = wcscmp( expected->comp, received->comp ))) goto done;
        if ((ret = wcscmp( expected->result, received->result ))) goto done;
        break;
    }

done:
    if (ret && broken( expected->broken )) return ret;

    switch (received->func)
    {
    case IME_SELECT:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "got hkl %p, himc %p, IME_SELECT select %u\n", received->hkl, received->himc, received->select );
        return ret;
    case IME_NOTIFY:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "got hkl %p, himc %p, IME_NOTIFY action %#x, index %#x, value %#x\n",
                         received->hkl, received->himc, received->notify.action, received->notify.index,
                         received->notify.value );
        return ret;
    case IME_PROCESS_KEY:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "got hkl %p, himc %p, IME_PROCESS_KEY vkey %#x, lparam %#Ix\n",
                         received->hkl, received->himc, received->process_key.vkey, received->process_key.lparam );
        return ret;
    case IME_TO_ASCII_EX:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "got hkl %p, himc %p, IME_TO_ASCII_EX vkey %#x, vsc %#x, flags %#x\n",
                         received->hkl, received->himc, received->to_ascii_ex.vkey, received->to_ascii_ex.vsc,
                         received->to_ascii_ex.flags );
        return ret;
    case IME_SET_ACTIVE_CONTEXT:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "got hkl %p, himc %p, IME_SET_ACTIVE_CONTEXT flag %u\n", received->hkl, received->himc,
                         received->set_active_context.flag );
        return ret;
    case MSG_IME_UI:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "got hkl %p, himc %p, MSG_IME_UI msg %s, wparam %#Ix, lparam %#Ix\n", received->hkl,
                         received->himc, debugstr_wm_ime(received->message.msg), received->message.wparam, received->message.lparam );
        return ret;
    case MSG_TEST_WIN:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "got hkl %p, himc %p, MSG_TEST_WIN msg %s, wparam %#Ix, lparam %#Ix, comp %s, result %s\n", received->hkl,
                         received->himc, debugstr_wm_ime(received->message.msg), received->message.wparam, received->message.lparam,
                         debugstr_w(received->comp), debugstr_w(received->result) );
        return ret;
    }

    switch (expected->func)
    {
    case IME_SELECT:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "hkl %p, himc %p, IME_SELECT select %u\n", expected->hkl, expected->himc, expected->select );
        break;
    case IME_NOTIFY:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "hkl %p, himc %p, IME_NOTIFY action %#x, index %#x, value %#x\n",
                         expected->hkl, expected->himc, expected->notify.action, expected->notify.index,
                         expected->notify.value );
        break;
    case IME_PROCESS_KEY:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "hkl %p, himc %p, IME_PROCESS_KEY vkey %#x, lparam %#Ix\n",
                         expected->hkl, expected->himc, expected->process_key.vkey, expected->process_key.lparam );
        break;
    case IME_TO_ASCII_EX:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "hkl %p, himc %p, IME_TO_ASCII_EX vkey %#x, vsc %#x, flags %#x\n",
                         expected->hkl, expected->himc, expected->to_ascii_ex.vkey, expected->to_ascii_ex.vsc,
                         expected->to_ascii_ex.flags );
        break;
    case IME_SET_ACTIVE_CONTEXT:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "hkl %p, himc %p, IME_SET_ACTIVE_CONTEXT flag %u\n", expected->hkl, expected->himc,
                         expected->set_active_context.flag );
        break;
    case MSG_IME_UI:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "hkl %p, himc %p, MSG_IME_UI msg %s, wparam %#Ix, lparam %#Ix\n", expected->hkl,
                         expected->himc, debugstr_wm_ime(expected->message.msg), expected->message.wparam, expected->message.lparam );
        break;
    case MSG_TEST_WIN:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "hkl %p, himc %p, MSG_TEST_WIN msg %s, wparam %#Ix, lparam %#Ix, comp %s, result %s\n", expected->hkl,
                         expected->himc, debugstr_wm_ime(expected->message.msg), expected->message.wparam, expected->message.lparam,
                         debugstr_w(expected->comp), debugstr_w(expected->result) );
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
        if (ret && expected->todo && expected->func &&
            !strcmp( winetest_platform, "wine" ))
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

static BOOL check_WM_SHOWWINDOW;
static BOOL ignore_WM_IME_NOTIFY;
static BOOL ignore_WM_IME_REQUEST;

static BOOL ignore_message( UINT msg, WPARAM wparam )
{
    switch (msg)
    {
    case WM_IME_NOTIFY:
        if (ignore_WM_IME_NOTIFY) return TRUE;
        return wparam > IMN_PRIVATE;
    case WM_IME_REQUEST:
        if (ignore_WM_IME_REQUEST) return TRUE;
        return FALSE;
    case WM_IME_STARTCOMPOSITION:
    case WM_IME_ENDCOMPOSITION:
    case WM_IME_COMPOSITION:
    case WM_IME_SETCONTEXT:
    case WM_IME_CONTROL:
    case WM_IME_COMPOSITIONFULL:
    case WM_IME_SELECT:
    case WM_IME_CHAR:
    case 0x287:
    case WM_IME_KEYDOWN:
    case WM_IME_KEYUP:
        return FALSE;
    case WM_SHOWWINDOW:
        return !check_WM_SHOWWINDOW;
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

    if (ignore_message( msg, wparam )) return DefWindowProcW( hwnd, msg, wparam, lparam );

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

    if (ignore_message( msg, wparam )) return DefWindowProcW( hwnd, msg, wparam, lparam );

    if (msg == WM_IME_COMPOSITION)
    {
        ImmGetCompositionStringW( call.himc, GCS_COMPSTR, call.comp, sizeof(call.comp) );
        ImmGetCompositionStringW( call.himc, GCS_RESULTSTR, call.result, sizeof(call.result) );
    }

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

static void test_SCS_SETSTR(void)
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

struct test_cross_thread_himc_params
{
    HWND hwnd;
    HANDLE event;
    HIMC himc[2];
    INPUTCONTEXT *contexts[2];
};

static DWORD WINAPI test_cross_thread_himc_thread( void *arg )
{
    CANDIDATEFORM candidate = {.dwIndex = 1, .dwStyle = CFS_CANDIDATEPOS};
    struct test_cross_thread_himc_params *params = arg;
    COMPOSITIONFORM composition = {0};
    INPUTCONTEXT *contexts[2];
    HIMC himc[2], tmp_himc;
    LOGFONTW fontW = {0};
    HWND hwnd, tmp_hwnd;
    POINT pos = {0};
    MSG msg;

    hwnd = CreateWindowW( test_class.lpszClassName, NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 100, 100, NULL, NULL, NULL, NULL );
    ok_ne( NULL, hwnd, HWND, "%p" );
    himc[0] = ImmGetContext( hwnd );
    ok_ne( NULL, himc[0], HIMC, "%p" );
    contexts[0] = ImmLockIMC( himc[0] );
    ok_ne( NULL, contexts[0], INPUTCONTEXT *, "%p" );
    contexts[0]->hWnd = hwnd;

    tmp_hwnd = CreateWindowW( L"static", NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              100, 100, 100, 100, NULL, NULL, NULL, NULL );
    tmp_himc = ImmGetContext( tmp_hwnd );
    ok_eq( himc[0], tmp_himc, HIMC, "%p" );
    ok_ret( 1, ImmReleaseContext( tmp_hwnd, tmp_himc ) );
    ok_ret( 1, DestroyWindow( tmp_hwnd ) );

    himc[1] = ImmCreateContext();
    ok_ne( NULL, himc[1], HIMC, "%p" );
    contexts[1] = ImmLockIMC( himc[1] );
    ok_ne( NULL, contexts[1], INPUTCONTEXT *, "%p" );
    contexts[1]->hWnd = hwnd;

    ok_ret( 1, ImmSetOpenStatus( himc[0], 0xdeadbeef ) );
    ok_ret( 1, ImmSetOpenStatus( himc[1], 0xfeedcafe ) );
    ok_ret( 1, ImmSetCompositionWindow( himc[0], &composition ) );
    ok_ret( 1, ImmSetCompositionWindow( himc[1], &composition ) );
    ok_ret( 1, ImmSetCandidateWindow( himc[0], &candidate ) );
    ok_ret( 1, ImmSetCandidateWindow( himc[1], &candidate ) );
    ok_ret( 1, ImmSetStatusWindowPos( himc[0], &pos ) );
    ok_ret( 1, ImmSetStatusWindowPos( himc[1], &pos ) );
    ok_ret( 1, ImmSetCompositionFontW( himc[0], &fontW ) );
    ok_ret( 1, ImmSetCompositionFontW( himc[1], &fontW ) );

    params->hwnd = hwnd;
    params->himc[0] = himc[0];
    params->himc[1] = himc[1];
    params->contexts[0] = contexts[0];
    params->contexts[1] = contexts[1];
    SetEvent( params->event );

    while (GetMessageW( &msg, 0, 0, 0 ))
    {
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    }

    ok_ret( 1, ImmUnlockIMC( himc[0] ) );
    ok_ret( 1, ImmUnlockIMC( himc[1] ) );

    ok_ret( 1, ImmDestroyContext( himc[1] ) );
    ok_ret( 1, ImmReleaseContext( hwnd, himc[0] ) );
    ok_ret( 0, DestroyWindow( hwnd ) );

    return 1;
}

static void test_cross_thread_himc(void)
{
    static const WCHAR comp_string[] = L"CompString";
    RECONVERTSTRING reconv = {.dwSize = sizeof(RECONVERTSTRING)};
    struct test_cross_thread_himc_params params;
    COMPOSITIONFORM composition = {0};
    DWORD tid, conversion, sentence;
    IMECHARPOSITION char_pos = {0};
    CANDIDATEFORM candidate = {0};
    COMPOSITIONSTRING *string;
    HIMC himc[2], tmp_himc;
    INPUTCONTEXT *tmp_ctx;
    LOGFONTW fontW = {0};
    LOGFONTA fontA = {0};
    char buffer[512];
    POINT pos = {0};
    HANDLE thread;
    BYTE *dst;
    UINT ret;

    himc[0] = ImmGetContext( hwnd );
    ok_ne( NULL, himc[0], HIMC, "%p" );
    ok_ne( NULL, ImmLockIMC( himc[0] ), INPUTCONTEXT *, "%p" );
    ok_ret( 1, ImmUnlockIMC( himc[0] ) );

    params.event = CreateEventW(NULL, TRUE, FALSE, NULL);
    ok_ne( NULL, params.event, HANDLE, "%p" );
    thread = CreateThread( NULL, 0, test_cross_thread_himc_thread, &params, 0, &tid );
    ok_ne( NULL, thread, HANDLE, "%p" );
    WaitForSingleObject( params.event, INFINITE );

    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    tmp_himc = ImmGetContext( params.hwnd );
    ok_ne( himc[0], tmp_himc, HIMC, "%p" );
    ok_eq( params.himc[0], tmp_himc, HIMC, "%p" );
    ok_ret( 1, ImmReleaseContext( params.hwnd, tmp_himc ) );

    himc[1] = ImmCreateContext();
    ok_ne( NULL, himc[1], HIMC, "%p" );
    tmp_ctx = ImmLockIMC( himc[1] );
    ok_ne( NULL, tmp_ctx, INPUTCONTEXT *, "%p" );

    tmp_ctx->hCompStr = ImmReSizeIMCC( tmp_ctx->hCompStr, 512 );
    ok_ne( NULL, tmp_ctx->hCompStr, HIMCC, "%p" );
    string = ImmLockIMCC( tmp_ctx->hCompStr );
    ok_ne( NULL, string, COMPOSITIONSTRING *, "%p" );
    string->dwSize = sizeof(COMPOSITIONSTRING);
    string->dwCompStrLen = wcslen( comp_string );
    string->dwCompStrOffset = string->dwSize;
    dst = (BYTE *)string + string->dwCompStrOffset;
    memcpy( dst, comp_string, string->dwCompStrLen * sizeof(WCHAR) );
    string->dwSize += string->dwCompStrLen * sizeof(WCHAR);

    string->dwCompClauseLen = 2 * sizeof(DWORD);
    string->dwCompClauseOffset = string->dwSize;
    dst = (BYTE *)string + string->dwCompClauseOffset;
    *(DWORD *)(dst + 0 * sizeof(DWORD)) = 0;
    *(DWORD *)(dst + 1 * sizeof(DWORD)) = string->dwCompStrLen;
    string->dwSize += 2 * sizeof(DWORD);

    string->dwCompAttrLen = string->dwCompStrLen;
    string->dwCompAttrOffset = string->dwSize;
    dst = (BYTE *)string + string->dwCompAttrOffset;
    memset( dst, ATTR_INPUT, string->dwCompStrLen );
    string->dwSize += string->dwCompStrLen;
    ok_ret( 0, ImmUnlockIMCC( tmp_ctx->hCompStr ) );

    ok_ret( 1, ImmUnlockIMC( himc[1] ) );

    /* ImmLockIMC should succeed with cross thread HIMC */

    tmp_ctx = ImmLockIMC( params.himc[0] );
    ok_eq( params.contexts[0], tmp_ctx, INPUTCONTEXT *, "%p" );
    ret = ImmGetIMCLockCount( params.himc[0] );
    ok( ret >= 2, "got ret %u\n", ret );

    tmp_ctx->hCompStr = ImmReSizeIMCC( tmp_ctx->hCompStr, 512 );
    ok_ne( NULL, tmp_ctx->hCompStr, HIMCC, "%p" );
    string = ImmLockIMCC( tmp_ctx->hCompStr );
    ok_ne( NULL, string, COMPOSITIONSTRING *, "%p" );
    string->dwSize = sizeof(COMPOSITIONSTRING);
    string->dwCompStrLen = wcslen( comp_string );
    string->dwCompStrOffset = string->dwSize;
    dst = (BYTE *)string + string->dwCompStrOffset;
    memcpy( dst, comp_string, string->dwCompStrLen * sizeof(WCHAR) );
    string->dwSize += string->dwCompStrLen * sizeof(WCHAR);

    string->dwCompClauseLen = 2 * sizeof(DWORD);
    string->dwCompClauseOffset = string->dwSize;
    dst = (BYTE *)string + string->dwCompClauseOffset;
    *(DWORD *)(dst + 0 * sizeof(DWORD)) = 0;
    *(DWORD *)(dst + 1 * sizeof(DWORD)) = string->dwCompStrLen;
    string->dwSize += 2 * sizeof(DWORD);

    string->dwCompAttrLen = string->dwCompStrLen;
    string->dwCompAttrOffset = string->dwSize;
    dst = (BYTE *)string + string->dwCompAttrOffset;
    memset( dst, ATTR_INPUT, string->dwCompStrLen );
    string->dwSize += string->dwCompStrLen;
    ok_ret( 0, ImmUnlockIMCC( tmp_ctx->hCompStr ) );

    ok_ret( 1, ImmUnlockIMC( params.himc[0] ) );

    tmp_ctx = ImmLockIMC( params.himc[1] );
    ok_eq( params.contexts[1], tmp_ctx, INPUTCONTEXT *, "%p" );
    ret = ImmGetIMCLockCount( params.himc[1] );
    ok( ret >= 2, "got ret %u\n", ret );
    ok_ret( 1, ImmUnlockIMC( params.himc[1] ) );

    /* ImmSetActiveContext should succeed with cross thread HIMC */

    SET_ENABLE( WM_IME_SETCONTEXT_DEACTIVATE, TRUE );
    SET_ENABLE( WM_IME_SETCONTEXT_ACTIVATE, TRUE );

    SET_EXPECT( WM_IME_SETCONTEXT_ACTIVATE );
    ok_ret( 1, ImmSetActiveContext( hwnd, params.himc[0], TRUE ) );
    CHECK_CALLED( WM_IME_SETCONTEXT_ACTIVATE );

    SET_EXPECT( WM_IME_SETCONTEXT_DEACTIVATE );
    ok_ret( 1, ImmSetActiveContext( hwnd, params.himc[0], FALSE ) );
    CHECK_CALLED( WM_IME_SETCONTEXT_DEACTIVATE );

    SET_ENABLE( WM_IME_SETCONTEXT_DEACTIVATE, FALSE );
    SET_ENABLE( WM_IME_SETCONTEXT_ACTIVATE, FALSE );

    /* ImmSetOpenStatus should fail with cross thread HIMC */

    ok_ret( 1, ImmSetOpenStatus( himc[1], 0xdeadbeef ) );
    ok_ret( (int)0xdeadbeef, ImmGetOpenStatus( himc[1] ) );

    ok_ret( 0, ImmSetOpenStatus( params.himc[0], TRUE ) );
    ok_ret( 0, ImmSetOpenStatus( params.himc[1], TRUE ) );
    ok_ret( (int)0xdeadbeef, ImmGetOpenStatus( params.himc[0] ) );
    ok_ret( (int)0xfeedcafe, ImmGetOpenStatus( params.himc[1] ) );
    ok_ret( 0, ImmSetOpenStatus( params.himc[0], FALSE ) );
    ok_ret( (int)0xdeadbeef, ImmGetOpenStatus( params.himc[0] ) );

    /* ImmSetConversionStatus should fail with cross thread HIMC */

    ok_ret( 1, ImmGetConversionStatus( himc[1], &conversion, &sentence ) );
    ok_ret( 1, ImmSetConversionStatus( himc[1], conversion, sentence ) );

    ok_ret( 1, ImmGetConversionStatus( params.himc[0], &conversion, &sentence ) );
    ok_ret( 1, ImmGetConversionStatus( params.himc[1], &conversion, &sentence ) );
    ok_ret( 0, ImmSetConversionStatus( params.himc[0], conversion, sentence ) );
    ok_ret( 0, ImmSetConversionStatus( params.himc[1], conversion, sentence ) );

    /* ImmSetCompositionFont(W|A) should fail with cross thread HIMC */

    ok_ret( 1, ImmSetCompositionFontA( himc[1], &fontA ) );
    ok_ret( 1, ImmGetCompositionFontA( himc[1], &fontA ) );
    ok_ret( 1, ImmSetCompositionFontW( himc[1], &fontW ) );
    ok_ret( 1, ImmGetCompositionFontW( himc[1], &fontW ) );

    ok_ret( 0, ImmSetCompositionFontA( params.himc[0], &fontA ) );
    ok_ret( 0, ImmSetCompositionFontA( params.himc[1], &fontA ) );
    ok_ret( 1, ImmGetCompositionFontA( params.himc[0], &fontA ) );
    ok_ret( 1, ImmGetCompositionFontA( params.himc[1], &fontA ) );
    ok_ret( 0, ImmSetCompositionFontW( params.himc[0], &fontW ) );
    ok_ret( 0, ImmSetCompositionFontW( params.himc[1], &fontW ) );
    ok_ret( 1, ImmGetCompositionFontW( params.himc[0], &fontW ) );
    ok_ret( 1, ImmGetCompositionFontW( params.himc[1], &fontW ) );

    /* ImmRequestMessage(W|A) should fail with cross thread HIMC */

    ok_ret( 0, ImmRequestMessageW( himc[1], IMR_COMPOSITIONFONT, (LPARAM)&fontW ) );
    ok_ret( 0, ImmRequestMessageA( himc[1], IMR_COMPOSITIONFONT, (LPARAM)&fontA ) );

    ok_ret( 0, ImmRequestMessageW( params.himc[0], IMR_COMPOSITIONFONT, (LPARAM)&fontW ) );
    ok_ret( 0, ImmRequestMessageA( params.himc[0], IMR_COMPOSITIONFONT, (LPARAM)&fontA ) );
    ok_ret( 0, ImmRequestMessageW( params.himc[1], IMR_COMPOSITIONFONT, (LPARAM)&fontW ) );
    ok_ret( 0, ImmRequestMessageA( params.himc[1], IMR_COMPOSITIONFONT, (LPARAM)&fontA ) );

    ok_seq( empty_sequence );

    /* ImmSetCompositionString(W|A) should fail with cross thread HIMC */

    ok_ret( 10, ImmGetCompositionStringA( himc[1], GCS_COMPSTR, buffer, sizeof(buffer) ) );
    ok_ret( 20, ImmGetCompositionStringW( himc[1], GCS_COMPSTR, buffer, sizeof(buffer) ) );
    ok_ret( 1, ImmSetCompositionStringA( himc[1], SCS_SETSTR, "a", 2, NULL, 0 ) );
    ok_ret( 1, ImmSetCompositionStringW( himc[1], SCS_SETSTR, L"a", 4, NULL, 0 ) );

    ok_ret( 0, ImmSetCompositionStringA( params.himc[0], SCS_SETSTR, "a", 2, NULL, 0 ) );
    ok_ret( 0, ImmSetCompositionStringA( params.himc[1], SCS_SETSTR, "a", 2, NULL, 0 ) );
    ok_ret( 0, ImmSetCompositionStringW( params.himc[0], SCS_SETSTR, L"a", 4, NULL, 0 ) );
    ok_ret( 0, ImmSetCompositionStringW( params.himc[1], SCS_SETSTR, L"a", 4, NULL, 0 ) );
    ok_ret( 10, ImmGetCompositionStringA( params.himc[0], GCS_COMPSTR, buffer, sizeof(buffer) ) );
    ok_ret( 0, ImmGetCompositionStringA( params.himc[1], GCS_COMPSTR, buffer, sizeof(buffer) ) );
    ok_ret( 20, ImmGetCompositionStringW( params.himc[0], GCS_COMPSTR, buffer, sizeof(buffer) ) );
    ok_ret( 0, ImmGetCompositionStringW( params.himc[1], GCS_COMPSTR, buffer, sizeof(buffer) ) );

    /* ImmRequestMessage(W|A) should fail with cross thread HIMC */

    ok_ret( 0, ImmRequestMessageW( himc[1], IMR_RECONVERTSTRING, 0 ) );
    ok_ret( 0, ImmRequestMessageW( himc[1], IMR_RECONVERTSTRING, (LPARAM)&reconv ) );
    ok_ret( 0, ImmRequestMessageA( himc[1], IMR_RECONVERTSTRING, 0 ) );
    ok_ret( 0, ImmRequestMessageA( himc[1], IMR_RECONVERTSTRING, (LPARAM)&reconv ) );

    ok_ret( 0, ImmRequestMessageW( params.himc[0], IMR_RECONVERTSTRING, 0 ) );
    ok_ret( 0, ImmRequestMessageW( params.himc[0], IMR_RECONVERTSTRING, (LPARAM)&reconv ) );
    ok_ret( 0, ImmRequestMessageA( params.himc[0], IMR_RECONVERTSTRING, 0 ) );
    ok_ret( 0, ImmRequestMessageA( params.himc[0], IMR_RECONVERTSTRING, (LPARAM)&reconv ) );
    ok_ret( 0, ImmRequestMessageW( params.himc[1], IMR_RECONVERTSTRING, 0 ) );
    ok_ret( 0, ImmRequestMessageW( params.himc[1], IMR_RECONVERTSTRING, (LPARAM)&reconv ) );
    ok_ret( 0, ImmRequestMessageA( params.himc[1], IMR_RECONVERTSTRING, 0 ) );
    ok_ret( 0, ImmRequestMessageA( params.himc[1], IMR_RECONVERTSTRING, (LPARAM)&reconv ) );

    ok_ret( 0, ImmRequestMessageW( himc[1], IMR_DOCUMENTFEED, 0 ) );
    ok_ret( 0, ImmRequestMessageW( himc[1], IMR_DOCUMENTFEED, (LPARAM)&reconv ) );
    ok_ret( 0, ImmRequestMessageA( himc[1], IMR_DOCUMENTFEED, 0 ) );
    ok_ret( 0, ImmRequestMessageA( himc[1], IMR_DOCUMENTFEED, (LPARAM)&reconv ) );

    ok_ret( 0, ImmRequestMessageW( params.himc[0], IMR_DOCUMENTFEED, 0 ) );
    ok_ret( 0, ImmRequestMessageW( params.himc[0], IMR_DOCUMENTFEED, (LPARAM)&reconv ) );
    ok_ret( 0, ImmRequestMessageA( params.himc[0], IMR_DOCUMENTFEED, 0 ) );
    ok_ret( 0, ImmRequestMessageA( params.himc[0], IMR_DOCUMENTFEED, (LPARAM)&reconv ) );
    ok_ret( 0, ImmRequestMessageW( params.himc[1], IMR_DOCUMENTFEED, 0 ) );
    ok_ret( 0, ImmRequestMessageW( params.himc[1], IMR_DOCUMENTFEED, (LPARAM)&reconv ) );
    ok_ret( 0, ImmRequestMessageA( params.himc[1], IMR_DOCUMENTFEED, 0 ) );
    ok_ret( 0, ImmRequestMessageA( params.himc[1], IMR_DOCUMENTFEED, (LPARAM)&reconv ) );

    ok_ret( 0, ImmRequestMessageW( himc[1], IMR_CONFIRMRECONVERTSTRING, (LPARAM)&reconv ) );
    ok_ret( 0, ImmRequestMessageA( himc[1], IMR_CONFIRMRECONVERTSTRING, (LPARAM)&reconv ) );

    ok_ret( 0, ImmRequestMessageW( params.himc[0], IMR_CONFIRMRECONVERTSTRING, (LPARAM)&reconv ) );
    ok_ret( 0, ImmRequestMessageA( params.himc[0], IMR_CONFIRMRECONVERTSTRING, (LPARAM)&reconv ) );
    ok_ret( 0, ImmRequestMessageW( params.himc[1], IMR_CONFIRMRECONVERTSTRING, (LPARAM)&reconv ) );
    ok_ret( 0, ImmRequestMessageA( params.himc[1], IMR_CONFIRMRECONVERTSTRING, (LPARAM)&reconv ) );

    ok_seq( empty_sequence );

    /* ImmSetCompositionWindow should fail with cross thread HIMC */

    ok_ret( 1, ImmSetCompositionWindow( himc[1], &composition ) );
    ok_ret( 1, ImmGetCompositionWindow( himc[1], &composition ) );

    ok_ret( 0, ImmSetCompositionWindow( params.himc[0], &composition ) );
    ok_ret( 0, ImmSetCompositionWindow( params.himc[1], &composition ) );
    ok_ret( 1, ImmGetCompositionWindow( params.himc[0], &composition ) );
    ok_ret( 1, ImmGetCompositionWindow( params.himc[1], &composition ) );

    /* ImmRequestMessage(W|A) should fail with cross thread HIMC */

    ok_ret( 0, ImmRequestMessageW( himc[1], IMR_COMPOSITIONWINDOW, (LPARAM)&composition ) );
    ok_ret( 0, ImmRequestMessageA( himc[1], IMR_COMPOSITIONWINDOW, (LPARAM)&composition ) );

    ok_ret( 0, ImmRequestMessageW( params.himc[0], IMR_COMPOSITIONWINDOW, (LPARAM)&composition ) );
    ok_ret( 0, ImmRequestMessageA( params.himc[0], IMR_COMPOSITIONWINDOW, (LPARAM)&composition ) );
    ok_ret( 0, ImmRequestMessageW( params.himc[1], IMR_COMPOSITIONWINDOW, (LPARAM)&composition ) );
    ok_ret( 0, ImmRequestMessageA( params.himc[1], IMR_COMPOSITIONWINDOW, (LPARAM)&composition ) );

    ok_seq( empty_sequence );

    /* ImmSetCandidateWindow should fail with cross thread HIMC */

    ok_ret( 1, ImmSetCandidateWindow( himc[1], &candidate ) );
    ok_ret( 1, ImmGetCandidateWindow( himc[1], 0, &candidate ) );

    ok_ret( 1, ImmGetCandidateWindow( params.himc[0], 1, &candidate ) );
    ok_ret( 1, ImmGetCandidateWindow( params.himc[1], 1, &candidate ) );
    ok_ret( 0, ImmSetCandidateWindow( params.himc[0], &candidate ) );
    ok_ret( 0, ImmSetCandidateWindow( params.himc[1], &candidate ) );

    /* ImmRequestMessage(W|A) should fail with cross thread HIMC */

    candidate.dwIndex = -1;
    ok_ret( 0, ImmRequestMessageW( himc[1], IMR_CANDIDATEWINDOW, (LPARAM)&candidate ) );
    ok_ret( 0, ImmRequestMessageA( himc[1], IMR_CANDIDATEWINDOW, (LPARAM)&candidate ) );

    candidate.dwIndex = 0;
    ok_ret( 0, ImmRequestMessageW( himc[1], IMR_CANDIDATEWINDOW, (LPARAM)&candidate ) );
    ok_ret( 0, ImmRequestMessageA( himc[1], IMR_CANDIDATEWINDOW, (LPARAM)&candidate ) );

    ok_ret( 0, ImmRequestMessageW( params.himc[0], IMR_CANDIDATEWINDOW, (LPARAM)&candidate ) );
    ok_ret( 0, ImmRequestMessageA( params.himc[0], IMR_CANDIDATEWINDOW, (LPARAM)&candidate ) );
    ok_ret( 0, ImmRequestMessageW( params.himc[1], IMR_CANDIDATEWINDOW, (LPARAM)&candidate ) );
    ok_ret( 0, ImmRequestMessageA( params.himc[1], IMR_CANDIDATEWINDOW, (LPARAM)&candidate ) );

    ok_seq( empty_sequence );

    /* ImmSetStatusWindowPos should fail with cross thread HIMC */

    ok_ret( 1, ImmSetStatusWindowPos( himc[1], &pos ) );
    ok_ret( 1, ImmGetStatusWindowPos( himc[1], &pos ) );

    ok_ret( 0, ImmSetStatusWindowPos( params.himc[0], &pos ) );
    ok_ret( 0, ImmSetStatusWindowPos( params.himc[1], &pos ) );
    ok_ret( 1, ImmGetStatusWindowPos( params.himc[0], &pos ) );
    ok_ret( 1, ImmGetStatusWindowPos( params.himc[1], &pos ) );

    /* ImmRequestMessage(W|A) should fail with cross thread HIMC */

    ok_ret( 0, ImmRequestMessageW( himc[1], IMR_QUERYCHARPOSITION, (LPARAM)&char_pos ) );
    ok_ret( 0, ImmRequestMessageA( himc[1], IMR_QUERYCHARPOSITION, (LPARAM)&char_pos ) );

    ok_ret( 0, ImmRequestMessageW( params.himc[0], IMR_QUERYCHARPOSITION, (LPARAM)&char_pos ) );
    ok_ret( 0, ImmRequestMessageA( params.himc[0], IMR_QUERYCHARPOSITION, (LPARAM)&char_pos ) );
    ok_ret( 0, ImmRequestMessageW( params.himc[1], IMR_QUERYCHARPOSITION, (LPARAM)&char_pos ) );
    ok_ret( 0, ImmRequestMessageA( params.himc[1], IMR_QUERYCHARPOSITION, (LPARAM)&char_pos ) );

    ok_seq( empty_sequence );

    /* ImmGenerateMessage should fail with cross thread HIMC */

    ok_ret( 1, ImmGenerateMessage( himc[1] ) );

    ok_ret( 0, ImmGenerateMessage( params.himc[0] ) );
    ok_ret( 0, ImmGenerateMessage( params.himc[1] ) );

    /* ImmAssociateContext should fail with cross thread HWND or HIMC */

    tmp_himc = ImmAssociateContext( hwnd, params.himc[0] );
    ok_eq( NULL, tmp_himc, HIMC, "%p" );
    tmp_himc = ImmGetContext( hwnd );
    ok_eq( himc[0], tmp_himc, HIMC, "%p" );
    ok_ret( 1, ImmReleaseContext( hwnd, tmp_himc ) );

    tmp_himc = ImmAssociateContext( hwnd, params.himc[1] );
    ok_eq( NULL, tmp_himc, HIMC, "%p" );
    tmp_himc = ImmGetContext( hwnd );
    ok_eq( himc[0], tmp_himc, HIMC, "%p" );
    ok_ret( 1, ImmReleaseContext( hwnd, tmp_himc ) );

    tmp_himc = ImmAssociateContext( params.hwnd, params.himc[1] );
    ok_eq( NULL, tmp_himc, HIMC, "%p" );
    tmp_himc = ImmGetContext( params.hwnd );
    ok_eq( params.himc[0], tmp_himc, HIMC, "%p" );
    ok_ret( 1, ImmReleaseContext( params.hwnd, tmp_himc ) );

    /* ImmAssociateContext should succeed with cross thread HWND and NULL HIMC */

    tmp_himc = ImmAssociateContext( params.hwnd, NULL );
    ok_eq( params.himc[0], tmp_himc, HIMC, "%p" );
    tmp_himc = ImmGetContext( params.hwnd );
    ok_eq( NULL, tmp_himc, HIMC, "%p" );

    /* ImmReleaseContext / ImmDestroyContext should fail with cross thread HIMC */

    ok_ret( 1, ImmReleaseContext( params.hwnd, params.himc[0] ) );
    ok_ret( 0, ImmDestroyContext( params.himc[1] ) );

    /* ImmGetContext should fail with another process HWND */

    tmp_himc = ImmGetContext( GetDesktopWindow() );
    ok_eq( NULL, tmp_himc, HIMC, "%p" );

    ok_ret( 0, SendMessageW( params.hwnd, WM_CLOSE, 0, 0 ) );
    ok_ret( 1, PostThreadMessageW( tid, WM_QUIT, 1, 0 ) );
    ok_ret( 0, WaitForSingleObject( thread, 5000 ) );
    ok_ret( 1, CloseHandle( thread ) );
    ok_ret( 1, CloseHandle( params.event ) );

    ok_ret( 1, ImmReleaseContext( hwnd, himc[0] ) );
    ok_ret( 1, ImmDestroyContext( himc[1] ) );
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

static BOOL WINAPI ime_ImeProcessKey( HIMC himc, UINT vkey, LPARAM lparam, BYTE *state )
{
    struct ime_call call =
    {
        .hkl = GetKeyboardLayout( 0 ), .himc = himc,
        .func = IME_PROCESS_KEY, .process_key = {.vkey = vkey, .lparam = lparam}
    };
    ime_trace( "himc %p, vkey %u, lparam %#Ix, state %p\n",
               himc, vkey, lparam, state );
    ime_calls[ime_call_count++] = call;
    return LOWORD(lparam);
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
    CHECK_EXPECT( ImeSetCompositionString );

    ok_eq( expect_ime, GetKeyboardLayout( 0 ), HKL, "%p" );
    ok_ne( default_himc, himc, HIMC, "%p" );

    if (ime_info.fdwProperty & IME_PROP_UNICODE)
    {
        switch (index)
        {
        case SCS_SETSTR:
            todo_wine_if( todo_ImeSetCompositionString )
            ok_eq( 22, comp_len, UINT, "%#x" );
            ok_wcs( L"CompString", comp );
            break;
        case SCS_CHANGECLAUSE:
        {
            const UINT *clause = comp;
            ok_eq( 8, comp_len, UINT, "%#x" );
            ok_eq( 0, clause[0], UINT, "%#x" );
            todo_wine_if( todo_ImeSetCompositionString )
            ok_eq( 1, clause[1], UINT, "%#x");
            break;
        }
        case SCS_CHANGEATTR:
        {
            const BYTE *attr = comp;
            todo_wine_if( todo_ImeSetCompositionString && comp_len != 4 )
            ok_eq( 4, comp_len, UINT, "%#x" );
            todo_wine_if( todo_ImeSetCompositionString && attr[0] != 0xcd )
            ok_eq( 0xcd, attr[0], UINT, "%#x" );
            todo_wine_if( todo_ImeSetCompositionString )
            ok_eq( 0xcd, attr[1], UINT, "%#x" );
            break;
        }
        default:
            ok( 0, "unexpected index %#lx\n", index );
            break;
        }
    }
    else
    {
        switch (index)
        {
        case SCS_SETSTR:
            todo_wine_if( todo_ImeSetCompositionString )
            ok_eq( 11, comp_len, UINT, "%#x" );
            ok_str( "CompString", comp );
            break;
        case SCS_CHANGECLAUSE:
        {
            const UINT *clause = comp;
            ok_eq( 8, comp_len, UINT, "%#x" );
            todo_wine_if( todo_ImeSetCompositionString )
            ok_eq( 0, clause[0], UINT, "%#x" );
            todo_wine_if( todo_ImeSetCompositionString )
            ok_eq( 1, clause[1], UINT, "%#x");
            break;
        }
        case SCS_CHANGEATTR:
        {
            const BYTE *attr = comp;
            todo_wine_if( todo_ImeSetCompositionString && comp_len != 4 )
            ok_eq( 4, comp_len, UINT, "%#x" );
            todo_wine_if( todo_ImeSetCompositionString )
            ok_eq( 0xcd, attr[0], UINT, "%#x" );
            todo_wine_if( todo_ImeSetCompositionString )
            ok_eq( 0xcd, attr[1], UINT, "%#x" );
            break;
        }
        default:
            ok( 0, "unexpected index %#lx\n", index );
            break;
        }
    }

    ok_eq( NULL, read, const void *, "%p" );
    ok_eq( 0, read_len, UINT, "%#x" );

    return TRUE;
}

static UINT WINAPI ime_ImeToAsciiEx( UINT vkey, UINT vsc, BYTE *state, TRANSMSGLIST *msgs, UINT flags, HIMC himc )
{
    struct ime_call call =
    {
        .hkl = GetKeyboardLayout( 0 ), .himc = himc,
        .func = IME_TO_ASCII_EX, .to_ascii_ex = {.vkey = vkey, .vsc = vsc, .flags = flags}
    };
    INPUTCONTEXT *ctx;
    UINT count = 0;

    ime_trace( "vkey %#x, vsc %#x, state %p, msgs %p, flags %#x, himc %p\n",
           vkey, vsc, state, msgs, flags, himc );
    ime_calls[ime_call_count++] = call;

    ok_ne( NULL, msgs, TRANSMSGLIST *, "%p" );
    todo_wine ok_eq( 256, msgs->uMsgCount, UINT, "%u" );

    ctx = ImmLockIMC( himc );
    ok_ret( VK_PROCESSKEY, ImmGetVirtualKey( ctx->hWnd ) );

    if (vsc & 0x200)
    {
        msgs->TransMsg[0].message = WM_IME_STARTCOMPOSITION;
        msgs->TransMsg[0].wParam = 1;
        msgs->TransMsg[0].lParam = 0;
        count++;
        msgs->TransMsg[1].message = WM_IME_ENDCOMPOSITION;
        msgs->TransMsg[1].wParam = 1;
        msgs->TransMsg[1].lParam = 0;
        count++;
    }

    if (vsc & 0x400)
    {
        TRANSMSG *msgs;

        ctx = ImmLockIMC( himc );
        ok_ne( NULL, ctx, INPUTCONTEXT *, "%p" );

        ok_ne( NULL, ctx->hMsgBuf, HIMCC, "%p" );
        ok_eq( 0, ctx->dwNumMsgBuf, UINT, "%u" );

        ctx->hMsgBuf = ImmReSizeIMCC( ctx->hMsgBuf, 64 * sizeof(*msgs) );
        ok_ne( NULL, ctx->hMsgBuf, HIMCC, "%p" );

        msgs = ImmLockIMCC( ctx->hMsgBuf );
        ok_ne( NULL, msgs, TRANSMSG *, "%p" );

        msgs[ctx->dwNumMsgBuf].message = WM_IME_STARTCOMPOSITION;
        msgs[ctx->dwNumMsgBuf].wParam = 2;
        msgs[ctx->dwNumMsgBuf].lParam = 0;
        ctx->dwNumMsgBuf++;
        msgs[ctx->dwNumMsgBuf].message = WM_IME_ENDCOMPOSITION;
        msgs[ctx->dwNumMsgBuf].wParam = 2;
        msgs[ctx->dwNumMsgBuf].lParam = 0;
        ctx->dwNumMsgBuf++;

        ok_ret( 0, ImmUnlockIMCC( ctx->hMsgBuf ) );
    }

    ok_ret( 1, ImmUnlockIMC( himc ) );

    if (vsc & 0x800) count = ~0;
    return count;
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
        if (HandleToUlong( hkl ) != wcstoul( buffer, NULL, 16 )) continue;
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

    ime_info.fdwProperty = 0;

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

    if (!(hkl = wineime_hkl)) goto cleanup;

    todo_ImeInquire = TRUE;
    todo_ImeDestroy = TRUE;
    todo_IME_DLL_PROCESS_ATTACH = TRUE;
    todo_IME_DLL_PROCESS_DETACH = TRUE;
    ok_ret( 1, ImmIsIME( hkl ) );
    todo_IME_DLL_PROCESS_ATTACH = FALSE;
    todo_IME_DLL_PROCESS_DETACH = FALSE;
    todo_ImeInquire = FALSE;
    todo_ImeDestroy = FALSE;

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

    /* IME_PROP_COMPLETE_ON_UNSELECT seems to be sometimes set on CJK locales IMEs, sometimes not */
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

    if (!(hkl = wineime_hkl)) goto cleanup;

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

    if (!(hkl = wineime_hkl)) goto cleanup;

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

    if (!(hkl = wineime_hkl)) goto cleanup;

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

    if (!(hkl = wineime_hkl)) goto cleanup;

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

    if (!(hkl = wineime_hkl)) goto cleanup;

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

    if (!(hkl = wineime_hkl)) goto cleanup;

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

    if (!(hkl = wineime_hkl)) goto cleanup;

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

    if (!(hkl = wineime_hkl)) goto cleanup;

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

cleanup:
    SET_ENABLE( ImeUnregisterWord, FALSE );

    winetest_pop_context();
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
            .func = MSG_TEST_WIN, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETCONVERSIONMODE},
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
            .func = MSG_TEST_WIN, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETSENTENCEMODE},
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
            .func = MSG_TEST_WIN, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETCONVERSIONMODE},
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
            .func = MSG_TEST_WIN, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETSENTENCEMODE},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETSENTENCEMODE},
        },
        {0},
    };
    DWORD old_conversion, old_sentence, conversion, sentence;
    HKL hkl;
    INPUTCONTEXT *ctx;
    HWND hwnd;

    ok_ret( 0, ImmGetConversionStatus( 0, &old_conversion, &old_sentence ) );
    ok_ret( 1, ImmGetConversionStatus( default_himc, &old_conversion, &old_sentence ) );

    ctx = ImmLockIMC( default_himc );
    ok_ne( NULL, ctx, INPUTCONTEXT *, "%p" );
    ok_eq( old_conversion, ctx->fdwConversion, UINT, "%#x" );
    ok_eq( old_sentence, ctx->fdwSentence, UINT, "%#x" );

    hwnd = CreateWindowW( test_class.lpszClassName, NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
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

    if (!(hkl = wineime_hkl)) goto cleanup;

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

    ok_ret( 1, ImmSetConversionStatus( default_himc, 0xdeadbeef, 0xfeedcafe ) );
    ok_seq( empty_sequence );

    ok_ret( 1, ImmGetConversionStatus( default_himc, &conversion, NULL ) );
    ok_eq( 0xdeadbeef, conversion, UINT, "%#x" );
    ok_eq( 0xdeadbeef, ctx->fdwConversion, UINT, "%#x" );
    ok_eq( 0xfeedcafe, ctx->fdwSentence, UINT, "%#x" );

    ctx->hWnd = 0;
    ok_seq( empty_sequence );
    ok_ret( 1, ImmSetConversionStatus( default_himc, 0, 0xfeedcafe ) );
    ok_seq( set_conversion_status_1_seq );

    ok_ret( 1, ImmGetConversionStatus( default_himc, &conversion, &sentence ) );
    ok_eq( 0, conversion, UINT, "%#x" );
    ok_eq( 0xfeedcafe, sentence, UINT, "%#x" );
    ok_eq( 0, ctx->fdwConversion, UINT, "%#x" );
    ok_eq( 0xfeedcafe, ctx->fdwSentence, UINT, "%#x" );

    ctx->hWnd = hwnd;
    ok_seq( empty_sequence );
    ok_ret( 1, ImmSetConversionStatus( default_himc, ~0, ~0 ) );
    ok_seq( set_conversion_status_2_seq );

    ok_ret( 1, ImmGetConversionStatus( default_himc, NULL, &sentence ) );
    ok_eq( ~0, sentence, UINT, "%#x" );
    ok_eq( ~0, ctx->fdwConversion, UINT, "%#x" );
    ok_eq( ~0, ctx->fdwSentence, UINT, "%#x" );

    ok_ret( 1, ImmSetConversionStatus( default_himc, ~0, ~0 ) );
    ok_seq( empty_sequence );

    ok_ret( 1, ImmGetConversionStatus( default_himc, &conversion, &sentence ) );
    ok_eq( ~0, conversion, UINT, "%#x" );
    ok_eq( ~0, sentence, UINT, "%#x" );
    ok_eq( ~0, ctx->fdwConversion, UINT, "%#x" );
    ok_eq( ~0, ctx->fdwSentence, UINT, "%#x" );

    /* status is cached and some bits are kept from the previous active IME */
    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    todo_wine ok_eq( 0x200, ctx->fdwConversion, UINT, "%#x" );
    ok_eq( old_sentence, ctx->fdwSentence, UINT, "%#x" );
    ok_ret( 1, ImmActivateLayout( hkl ) );
    ok_eq( ~0, ctx->fdwConversion, UINT, "%#x" );
    ok_eq( ~0, ctx->fdwSentence, UINT, "%#x" );
    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    todo_wine ok_eq( 0x200, ctx->fdwConversion, UINT, "%#x" );
    ok_eq( old_sentence, ctx->fdwSentence, UINT, "%#x" );

    ok_ret( 1, ImmFreeLayout( hkl ) );

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
            .func = MSG_TEST_WIN, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETOPENSTATUS},
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
        },
        {0},
    };
    const struct ime_call set_open_status_2_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_NOTIFY, .notify = {.action = NI_CONTEXTUPDATED, .index = 0, .value = IMC_SETOPENSTATUS},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_TEST_WIN, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETOPENSTATUS},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETOPENSTATUS},
        },
        {0},
    };
    HKL hkl;
    struct ime_windows ime_windows = {0};
    DWORD old_status, status;
    INPUTCONTEXT *ctx;
    HIMC himc;
    HWND hwnd;

    ok_ret( 0, ImmGetOpenStatus( 0 ) );
    old_status = ImmGetOpenStatus( default_himc );

    ctx = ImmLockIMC( default_himc );
    ok_ne( NULL, ctx, INPUTCONTEXT *, "%p" );
    ok_eq( old_status, ctx->fOpen, UINT, "%#x" );

    hwnd = CreateWindowW( test_class.lpszClassName, NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
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

    if (!(hkl = wineime_hkl)) goto cleanup;

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

    ok_ret( 1, ImmSetOpenStatus( default_himc, 0xdeadbeef ) );
    ok_seq( empty_sequence );

    himc = ImmCreateContext();
    ok_ne( NULL, himc, HIMC, "%p" );
    ok_ret( 1, EnumThreadWindows( GetCurrentThreadId(), enum_thread_ime_windows, (LPARAM)&ime_windows ) );
    ok_ne( NULL, ime_windows.ime_hwnd, HWND, "%p" );
    ok_ne( NULL, ime_windows.ime_ui_hwnd, HWND, "%p" );
    ok_eq( default_himc, (HIMC)GetWindowLongPtrW( ime_windows.ime_ui_hwnd, IMMGWL_IMC ), HIMC, "%p" );
    ok_ret( 1, ImmSetOpenStatus( himc, TRUE ) );
    ok_eq( default_himc, (HIMC)GetWindowLongPtrW( ime_windows.ime_ui_hwnd, IMMGWL_IMC ), HIMC, "%p" );
    ok_ret( 1, ImmSetOpenStatus( himc, FALSE ) );
    ok_eq( default_himc, (HIMC)GetWindowLongPtrW( ime_windows.ime_ui_hwnd, IMMGWL_IMC ), HIMC, "%p" );
    ok_ret( 1, ImmDestroyContext( himc ) );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    status = ImmGetOpenStatus( default_himc );
    ok( status == 0xdeadbeef || status == 0 /* MS Korean IME */, "got status %#lx\n", status );
    ok_eq( status, ctx->fOpen, UINT, "%#x" );

    ctx->hWnd = 0;
    ok_ret( 1, ImmSetOpenStatus( default_himc, 0xfeedcafe ) );
    ok_seq( set_open_status_1_seq );

    status = ImmGetOpenStatus( default_himc );
    ok_eq( 0xfeedcafe, status, UINT, "%#x" );
    ok_eq( 0xfeedcafe, ctx->fOpen, UINT, "%#x" );

    ctx->hWnd = hwnd;
    ok_seq( empty_sequence );
    ok_ret( 1, ImmSetOpenStatus( default_himc, ~0 ) );
    ok_seq( set_open_status_2_seq );

    status = ImmGetOpenStatus( default_himc );
    ok_eq( ~0, status, UINT, "%#x" );
    ok_eq( ~0, ctx->fOpen, UINT, "%#x" );

    ok_ret( 1, ImmSetOpenStatus( default_himc, ~0 ) );
    ok_seq( empty_sequence );

    status = ImmGetOpenStatus( default_himc );
    ok_eq( ~0, status, UINT, "%#x" );
    ok_eq( ~0, ctx->fOpen, UINT, "%#x" );

    /* status is cached between IME activations */

    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    status = ImmGetOpenStatus( default_himc );
    ok_eq( old_status, status, UINT, "%#x" );
    ok_eq( old_status, ctx->fOpen, UINT, "%#x" );
    ok_ret( 1, ImmActivateLayout( hkl ) );
    status = ImmGetOpenStatus( default_himc );
    todo_wine ok_eq( 1, status, UINT, "%#x" );
    todo_wine ok_eq( 1, ctx->fOpen, UINT, "%#x" );
    ok_ret( 1, ImmSetOpenStatus( default_himc, 0 ) );
    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    status = ImmGetOpenStatus( default_himc );
    ok_eq( old_status, status, UINT, "%#x" );
    ok_eq( old_status, ctx->fOpen, UINT, "%#x" );

    ok_ret( 1, ImmFreeLayout( hkl ) );

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
    const struct ime_call process_key_0[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_PROCESS_KEY, .process_key = {.vkey = 'A', .lparam = MAKELONG(0, 0x1e)},
        },
        {0},
    };
    const struct ime_call process_key_1[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_PROCESS_KEY, .process_key = {.vkey = 'A', .lparam = MAKELONG(1, 0x1e)},
        },
        {0},
    };
    const struct ime_call process_key_2[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = IME_PROCESS_KEY, .process_key = {.vkey = 'A', .lparam = MAKELONG(2, 0x1e)},
        },
        {0},
    };
    HKL hkl;
    UINT_PTR ret;
    HIMC himc;
    HWND hwnd;

    hwnd = CreateWindowW( L"static", NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 100, 100, NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );
    process_messages();

    ok_ret( 0, ImmProcessKey( hwnd, default_hkl, 'A', MAKELONG(1, 0x1e), 0 ) );
    ok_seq( empty_sequence );

    ime_info.fdwProperty = IME_PROP_END_UNLOAD | IME_PROP_UNICODE;

    if (!(hkl = wineime_hkl)) goto cleanup;

    ok_ret( 1, ImmActivateLayout( hkl ) );
    ok_ret( 1, ImmLoadIME( hkl ) );
    process_messages();
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    ok_ret( 0, ImmProcessKey( 0, hkl, 'A', MAKELONG(1, 0x1e), 0 ) );
    ok_seq( empty_sequence );

    ok_ret( 0, ImmProcessKey( hwnd, hkl, 'A', MAKELONG(0, 0x1e), 0 ) );
    ok_seq( process_key_0 );
    ret = ImmProcessKey( hwnd, hkl, 'A', MAKELONG(1, 0x1e), 0 );
    todo_wine
    ok_ret( 2, ret );
    ok_seq( process_key_1 );
    ok_ret( 2, ImmProcessKey( hwnd, hkl, 'A', MAKELONG(2, 0x1e), 0 ) );
    ok_seq( process_key_2 );

    ok_eq( hkl, GetKeyboardLayout( 0 ), HKL, "%p" );
    ok_ret( 0, ImmProcessKey( hwnd, default_hkl, 'A', MAKELONG(1, 0x1e), 0 ) );
    ok_seq( empty_sequence );
    ok_eq( hkl, GetKeyboardLayout( 0 ), HKL, "%p" );

    himc = ImmCreateContext();
    ok_ne( NULL, himc, HIMC, "%p" );
    ok_ret( 'A', ImmGetVirtualKey( hwnd ) );
    ok_eq( default_himc, ImmAssociateContext( hwnd, himc ), HIMC, "%p" );
    todo_wine ok_ret( VK_PROCESSKEY, ImmGetVirtualKey( hwnd ) );
    ok_eq( himc, ImmAssociateContext( hwnd, default_himc ), HIMC, "%p" );
    ok_ret( 'A', ImmGetVirtualKey( hwnd ) );
    ImmDestroyContext( himc );

    ok_ret( 0, ImmTranslateMessage( hwnd, WM_KEYUP, 'A', 0 ) );
    ok_ret( VK_PROCESSKEY, ImmGetVirtualKey( hwnd ) );

    ok_ret( 1, ImmActivateLayout( default_hkl ) );

    ok_ret( 1, ImmFreeLayout( hkl ) );
    process_messages();
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

cleanup:
    ok_ret( 1, DestroyWindow( hwnd ) );
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
            .todo = TRUE, .broken = TRUE, /* broken after SetForegroundWindow(GetDesktopWindow()) as in d3d8:device  */
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
            .todo = TRUE, .broken = TRUE, /* broken after SetForegroundWindow(GetDesktopWindow()) as in d3d8:device  */
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
    HKL hkl;
    struct ime_windows ime_windows = {0};
    HIMC himc;
    HWND hwnd;
    UINT ret;

    SET_ENABLE( ImeInquire, TRUE );
    SET_ENABLE( ImeDestroy, TRUE );

    ok_ret( 1, ImmActivateLayout( default_hkl ) );

    ime_info.fdwProperty = IME_PROP_END_UNLOAD | IME_PROP_UNICODE;

    if (!(hkl = wineime_hkl)) goto cleanup;

    /* ActivateKeyboardLayout doesn't call ImeInquire / ImeDestroy */

    ok_seq( empty_sequence );
    ok_eq( default_hkl, ActivateKeyboardLayout( hkl, 0 ), HKL, "%p" );
    ok_eq( hkl, GetKeyboardLayout( 0 ), HKL, "%p" );
    ok_eq( hkl, ActivateKeyboardLayout( default_hkl, 0 ), HKL, "%p" );
    ok_seq( empty_sequence );
    ok_eq( default_hkl, GetKeyboardLayout( 0 ), HKL, "%p" );


    /* ImmActivateLayout changes active HKL */

    SET_EXPECT( ImeInquire );
    ok_ret( 1, ImmActivateLayout( hkl ) );
    ok_seq( activate_seq );
    CHECK_CALLED( ImeInquire );
    ok_ret( 1, ImmLoadIME( hkl ) );

    ok_eq( hkl, GetKeyboardLayout( 0 ), HKL, "%p" );

    ok_ret( 1, ImmActivateLayout( hkl ) );
    ok_seq( empty_sequence );

    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    ok_seq( deactivate_seq );

    ok_eq( default_hkl, GetKeyboardLayout( 0 ), HKL, "%p" );


    /* ImmActivateLayout leaks the IME, we need to free it manually */

    SET_EXPECT( ImeDestroy );
    ret = ImmFreeLayout( hkl );
    ok( ret, "ImmFreeLayout returned %u\n", ret );
    CHECK_CALLED( ImeDestroy );
    ok_seq( empty_sequence );


    /* when there's a window, ActivateKeyboardLayout calls ImeInquire */

    hwnd = CreateWindowW( L"static", NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 100, 100, NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );
    process_messages();
    ok_seq( empty_sequence );

    himc = ImmCreateContext();
    ok( !!himc, "got himc %p\n", himc );
    ok_seq( empty_sequence );

    SET_EXPECT( ImeInquire );
    ok_eq( default_hkl, ActivateKeyboardLayout( hkl, 0 ), HKL, "%p" );
    CHECK_CALLED( ImeInquire );
    activate_with_window_seq[1].himc = himc;
    ok_seq( activate_with_window_seq );
    ok_ret( 1, ImmLoadIME( hkl ) );

    ok_eq( hkl, GetKeyboardLayout( 0 ), HKL, "%p" );

    /* FIXME: ignore spurious VK_CONTROL ImeProcessKey / ImeToAsciiEx calls */
    process_messages();
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    ok_eq( hkl, ActivateKeyboardLayout( default_hkl, 0 ), HKL, "%p" );
    deactivate_with_window_seq[1].himc = himc;
    deactivate_with_window_seq[5].himc = himc;
    ok_seq( deactivate_with_window_seq );

    ok_eq( default_hkl, GetKeyboardLayout( 0 ), HKL, "%p" );

    ok_ret( 1, ImmDestroyContext( himc ) );
    ok_seq( empty_sequence );


    ok_eq( default_hkl, ActivateKeyboardLayout( hkl, 0 ), HKL, "%p" );
    ok_eq( hkl, GetKeyboardLayout( 0 ), HKL, "%p" );

    ok_ret( 1, EnumThreadWindows( GetCurrentThreadId(), enum_thread_ime_windows, (LPARAM)&ime_windows ) );
    ok( !!ime_windows.ime_hwnd, "missing IME window\n" );
    ok( !!ime_windows.ime_ui_hwnd, "missing IME UI window\n" );
    ok_ret( (UINT_PTR)ime_windows.ime_hwnd, (UINT_PTR)GetParent( ime_windows.ime_ui_hwnd ) );

    ok_eq( hkl, ActivateKeyboardLayout( default_hkl, 0 ), HKL, "%p" );
    ok_eq( default_hkl, GetKeyboardLayout( 0 ), HKL, "%p" );
    process_messages();


    SET_EXPECT( ImeDestroy );
    ok_ret( 1, ImmFreeLayout( hkl ) );
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
            .todo = TRUE, .broken = TRUE, /* broken after SetForegroundWindow(GetDesktopWindow()) as in d3d8:device  */
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
            .todo = TRUE, .broken = TRUE, /* broken after SetForegroundWindow(GetDesktopWindow()) as in d3d8:device  */
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
    HKL hkl;
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

    if (!(hkl = wineime_hkl)) goto cleanup;

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

    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    deactivate_seq[1].himc = himc[0];
    deactivate_seq[5].himc = himc[0];
    ok_seq( deactivate_seq );

    ok_eq( default_hkl, GetKeyboardLayout( 0 ), HKL, "%p" );

    ok_ret( 1, ImmFreeLayout( hkl ) );
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
    HKL hkl;
    UINT_PTR ret;
    HWND hwnd;

    ime_info.fdwProperty = IME_PROP_END_UNLOAD | IME_PROP_UNICODE;

    if (!(hkl = wineime_hkl)) return;

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

    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

    ok_ret( 1, ImmFreeLayout( hkl ) );
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
    struct ime_call deactivate_2_seq[] =
    {
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
    HKL hkl;
    struct ime_windows ime_windows = {0};
    INPUTCONTEXT *ctx;
    HIMC himc;
    HWND hwnd;

    ime_info.fdwProperty = IME_PROP_END_UNLOAD | IME_PROP_UNICODE;

    if (!(hkl = wineime_hkl)) return;

    hwnd = CreateWindowW( L"static", NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 100, 100, NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );

    ok_ret( 1, ImmActivateLayout( hkl ) );
    ok_ret( 1, ImmLoadIME( hkl ) );
    process_messages();
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    ok_ret( 1, EnumThreadWindows( GetCurrentThreadId(), enum_thread_ime_windows, (LPARAM)&ime_windows ) );
    ok_ne( NULL, ime_windows.ime_hwnd, HWND, "%p" );
    ok_ne( NULL, ime_windows.ime_ui_hwnd, HWND, "%p" );
    ok_ret( 0, IsWindowVisible( ime_windows.ime_ui_hwnd ) );

    SetLastError( 0xdeadbeef );
    ok_ret( 1, ImmSetActiveContext( hwnd, default_himc, TRUE ) );
    ok_seq( activate_0_seq );
    ok_ret( 0, GetLastError() );
    ok_ret( 0, IsWindowVisible( ime_windows.ime_ui_hwnd ) );
    ok_ret( 1, ImmSetActiveContext( hwnd, default_himc, TRUE ) );
    ok_seq( activate_0_seq );
    ok_ret( 1, ImmSetActiveContext( hwnd, default_himc, FALSE ) );
    ok_seq( deactivate_0_seq );

    himc = ImmCreateContext();
    ok_ne( NULL, himc, HIMC, "%p" );
    ctx = ImmLockIMC( himc );
    ok_ne( NULL, ctx, INPUTCONTEXT *, "%p" );
    ok_eq( 0, ctx->hWnd, HWND, "%p" );

    ok_ret( 1, ImmSetActiveContext( hwnd, himc, FALSE ) );
    deactivate_1_seq[3].himc = himc;
    deactivate_1_seq[4].himc = himc;
    ok_seq( deactivate_1_seq );
    ok_ret( 1, ImmSetActiveContext( hwnd, himc, TRUE ) );
    activate_1_seq[0].himc = himc;
    ok_seq( activate_1_seq );

    ctx->hWnd = (HWND)0xdeadbeef;
    ok_ret( 1, ImmSetActiveContext( hwnd, himc, FALSE ) );
    ok_eq( (HWND)0xdeadbeef, ctx->hWnd, HWND, "%p" );
    deactivate_2_seq[0].himc = himc;
    ok_seq( deactivate_2_seq );

    ctx->hWnd = 0;
    ok_ret( 1, ImmSetActiveContext( hwnd, himc, TRUE ) );
    ok_eq( hwnd, ctx->hWnd, HWND, "%p" );
    activate_1_seq[0].himc = himc;
    ok_seq( activate_1_seq );

    ok_eq( default_himc, (HIMC)GetWindowLongPtrW( ime_windows.ime_ui_hwnd, IMMGWL_IMC ), HIMC, "%p" );
    ok_ret( 1, ImmSetActiveContext( hwnd, himc, TRUE ) );
    ok_ret( 0, IsWindowVisible( ime_windows.ime_ui_hwnd ) );
    ok_eq( default_himc, (HIMC)GetWindowLongPtrW( ime_windows.ime_ui_hwnd, IMMGWL_IMC ), HIMC, "%p" );

    ctx->hWnd = 0;
    ok_eq( default_himc, ImmAssociateContext( hwnd, himc ), HIMC, "%p" );
    ok_eq( himc, (HIMC)GetWindowLongPtrW( ime_windows.ime_ui_hwnd, IMMGWL_IMC ), HIMC, "%p" );
    ok_ret( 0, IsWindowVisible( ime_windows.ime_ui_hwnd ) );
    ok_eq( hwnd, ctx->hWnd, HWND, "%p" );

    ctx->hWnd = (HWND)0xdeadbeef;
    ok_eq( himc, ImmGetContext( hwnd ), HIMC, "%p" );
    ok_eq( (HWND)0xdeadbeef, ctx->hWnd, HWND, "%p" );
    ok_ret( 1, ImmReleaseContext( hwnd, himc ) );

    ok_ret( 1, ImmUnlockIMC( himc ) );
    ok_ret( 1, ImmDestroyContext( himc ) );

    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

    ok_ret( 1, ImmFreeLayout( hkl ) );
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
    HKL hkl;
    COMPOSITIONFORM comp_form = {0};
    IMECHARPOSITION char_pos = {0};
    RECONVERTSTRING reconv = {0};
    CANDIDATEFORM cand_form = {0};
    LOGFONTW log_font = {0};
    INPUTCONTEXT *ctx;
    HIMC himc;
    HWND hwnd;

    if (!(hkl = wineime_hkl)) return;

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

    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

    ok_ret( 1, ImmFreeLayout( hkl ) );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;
}

static void test_ImmGetCandidateList( BOOL unicode )
{
    char buffer[512], expect_bufW[512] = {0}, expect_bufA[512] = {0};
    CANDIDATELIST *cand_list = (CANDIDATELIST *)buffer, *expect_listW, *expect_listA;
    HKL hkl;
    CANDIDATEINFO *cand_info;
    INPUTCONTEXT *ctx;
    HIMC himc;
    HWND hwnd;

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

    if (!(hkl = wineime_hkl)) goto cleanup;

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

    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

    ok_ret( 1, ImmFreeLayout( hkl ) );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

cleanup:
    winetest_pop_context();
}

static void test_ImmGetCandidateListCount( BOOL unicode )
{
    HKL hkl;
    CANDIDATEINFO *cand_info;
    INPUTCONTEXT *ctx;
    DWORD count;
    HIMC himc;
    HWND hwnd;

    winetest_push_context( unicode ? "unicode" : "ansi" );

    /* IME_PROP_END_UNLOAD for the IME to unload / reload. */
    ime_info.fdwProperty = IME_PROP_END_UNLOAD;
    if (unicode) ime_info.fdwProperty |= IME_PROP_UNICODE;

    if (!(hkl = wineime_hkl)) goto cleanup;

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

    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

    ok_ret( 1, ImmFreeLayout( hkl ) );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

cleanup:
    winetest_pop_context();
}

static void test_ImmGetCandidateWindow(void)
{
    HKL hkl;
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
    HWND hwnd;

    ime_info.fdwProperty = IME_PROP_END_UNLOAD | IME_PROP_UNICODE;

    if (!(hkl = wineime_hkl)) return;

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
    ok_ret( 1, ImmGetCandidateWindow( default_himc, 4, &cand_form ) );
    ok_seq( empty_sequence );

    ok_ret( 0, ImmGetCandidateWindow( himc, 0, &cand_form ) );
    ok_seq( empty_sequence );

    ok_seq( empty_sequence );
    ok( !!ctx, "ImmLockIMC failed, error %lu\n", GetLastError() );
    ctx->cfCandForm[0] = expect_form;

    ok_ret( 1, ImmGetCandidateWindow( himc, 0, &cand_form ) );
    check_candidate_form( &cand_form, &expect_form );
    ok_seq( empty_sequence );

    ok_seq( empty_sequence );
    ok( !!ctx, "ImmLockIMC failed, error %lu\n", GetLastError() );
    ctx->cfCandForm[0].dwIndex = -1;

    ok_ret( 0, ImmGetCandidateWindow( himc, 0, &cand_form ) );
    ok_seq( empty_sequence );

    ok_ret( 1, ImmUnlockIMC( himc ) );
    ok_ret( 1, ImmDestroyContext( himc ) );

    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

    ok_ret( 1, ImmFreeLayout( hkl ) );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;
}

static void test_ImmGetCompositionString( BOOL unicode )
{
    static COMPOSITIONSTRING expect_string_empty = {.dwSize = sizeof(COMPOSITIONSTRING)};
    static COMPOSITIONSTRING expect_stringA =
    {
        .dwSize = 176,
        .dwCompReadAttrLen = 8,
        .dwCompReadAttrOffset = 116,
        .dwCompReadClauseLen = 8,
        .dwCompReadClauseOffset = 108,
        .dwCompReadStrLen = 8,
        .dwCompReadStrOffset = 100,
        .dwCompAttrLen = 4,
        .dwCompAttrOffset = 136,
        .dwCompClauseLen = 8,
        .dwCompClauseOffset = 128,
        .dwCompStrLen = 4,
        .dwCompStrOffset = 124,
        .dwCursorPos = 3,
        .dwDeltaStart = 1,
        .dwResultReadClauseLen = 8,
        .dwResultReadClauseOffset = 150,
        .dwResultReadStrLen = 10,
        .dwResultReadStrOffset = 140,
        .dwResultClauseLen = 8,
        .dwResultClauseOffset = 164,
        .dwResultStrLen = 6,
        .dwResultStrOffset = 158,
        .dwPrivateSize = 4,
        .dwPrivateOffset = 172,
    };
    static const COMPOSITIONSTRING expect_stringW =
    {
        .dwSize = 204,
        .dwCompReadAttrLen = 8,
        .dwCompReadAttrOffset = 124,
        .dwCompReadClauseLen = 8,
        .dwCompReadClauseOffset = 116,
        .dwCompReadStrLen = 8,
        .dwCompReadStrOffset = 100,
        .dwCompAttrLen = 4,
        .dwCompAttrOffset = 148,
        .dwCompClauseLen = 8,
        .dwCompClauseOffset = 140,
        .dwCompStrLen = 4,
        .dwCompStrOffset = 132,
        .dwCursorPos = 3,
        .dwDeltaStart = 1,
        .dwResultReadClauseLen = 8,
        .dwResultReadClauseOffset = 172,
        .dwResultReadStrLen = 10,
        .dwResultReadStrOffset = 152,
        .dwResultClauseLen = 8,
        .dwResultClauseOffset = 192,
        .dwResultStrLen = 6,
        .dwResultStrOffset = 180,
        .dwPrivateSize = 4,
        .dwPrivateOffset = 200,
    };
    static const UINT gcs_indexes[] =
    {
        GCS_COMPREADSTR,
        GCS_COMPREADATTR,
        GCS_COMPREADCLAUSE,
        GCS_COMPSTR,
        GCS_COMPATTR,
        GCS_COMPCLAUSE,
        GCS_CURSORPOS,
        GCS_DELTASTART,
        GCS_RESULTREADSTR,
        GCS_RESULTREADCLAUSE,
        GCS_RESULTSTR,
        GCS_RESULTCLAUSE,
    };
    static const UINT scs_indexes[] =
    {
        SCS_SETSTR,
        SCS_CHANGEATTR,
        SCS_CHANGECLAUSE,
    };
    static const UINT expect_retW[ARRAY_SIZE(gcs_indexes)] = {16, 8, 8, 8, 4, 8, 3, 1, 20, 8, 12, 8};
    static const UINT expect_retA[ARRAY_SIZE(gcs_indexes)] = {8, 8, 8, 4, 4, 8, 3, 1, 10, 8, 6, 8};
    HKL hkl;
    COMPOSITIONSTRING *string;
    char buffer[1024];
    INPUTCONTEXT *old_ctx, *ctx;
    const void *str;
    HIMCC old_himcc;
    UINT i, len;
    BYTE *dst;
    HIMC himc;
    HWND hwnd;

    SET_ENABLE( ImeSetCompositionString, TRUE );

    winetest_push_context( unicode ? "unicode" : "ansi" );

    /* IME_PROP_END_UNLOAD for the IME to unload / reload. */
    ime_info.fdwProperty = IME_PROP_END_UNLOAD;
    if (unicode) ime_info.fdwProperty |= IME_PROP_UNICODE;

    if (!(hkl = wineime_hkl)) goto cleanup;

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

    memset( buffer, 0xcd, sizeof(buffer) );
    todo_wine ok_ret( -2, ImmGetCompositionStringW( default_himc, GCS_COMPSTR | GCS_COMPATTR, buffer, sizeof(buffer) ) );
    memset( buffer, 0xcd, sizeof(buffer) );
    todo_wine ok_ret( -2, ImmGetCompositionStringA( default_himc, GCS_COMPSTR | GCS_COMPATTR, buffer, sizeof(buffer) ) );

    for (i = 0; i < ARRAY_SIZE(gcs_indexes); ++i)
    {
        memset( buffer, 0xcd, sizeof(buffer) );
        ok_ret( 0, ImmGetCompositionStringW( default_himc, gcs_indexes[i], buffer, sizeof(buffer) ) );
        memset( buffer, 0xcd, sizeof(buffer) );
        ok_ret( 0, ImmGetCompositionStringA( default_himc, gcs_indexes[i], buffer, sizeof(buffer) ) );

        memset( buffer, 0xcd, sizeof(buffer) );
        ok_ret( 0, ImmGetCompositionStringW( himc, gcs_indexes[i], buffer, sizeof(buffer) ) );
        memset( buffer, 0xcd, sizeof(buffer) );
        ok_ret( 0, ImmGetCompositionStringA( himc, gcs_indexes[i], buffer, sizeof(buffer) ) );
    }

    ctx->hCompStr = ImmReSizeIMCC( ctx->hCompStr, unicode ? expect_stringW.dwSize : expect_stringA.dwSize );
    string = ImmLockIMCC( ctx->hCompStr );
    ok( !!string, "ImmLockIMCC failed, error %lu\n", GetLastError() );
    check_composition_string( string, &expect_string_empty );

    string->dwCursorPos = 3;
    string->dwDeltaStart = 1;

    if (unicode) str = L"ReadComp";
    else str = "ReadComp";
    len = unicode ? wcslen( str ) : strlen( str );

    string->dwCompReadStrLen = len;
    string->dwCompReadStrOffset = string->dwSize;
    dst = (BYTE *)string + string->dwCompReadStrOffset;
    memcpy( dst, str, len * (unicode ? sizeof(WCHAR) : 1) );
    string->dwSize += len * (unicode ? sizeof(WCHAR) : 1);

    string->dwCompReadClauseLen = 2 * sizeof(DWORD);
    string->dwCompReadClauseOffset = string->dwSize;
    dst = (BYTE *)string + string->dwCompReadClauseOffset;
    *(DWORD *)(dst + 0 * sizeof(DWORD)) = 0;
    *(DWORD *)(dst + 1 * sizeof(DWORD)) = len;
    string->dwSize += 2 * sizeof(DWORD);

    string->dwCompReadAttrLen = len;
    string->dwCompReadAttrOffset = string->dwSize;
    dst = (BYTE *)string + string->dwCompReadAttrOffset;
    memset( dst, ATTR_INPUT, len );
    string->dwSize += len;

    if (unicode) str = L"Comp";
    else str = "Comp";
    len = unicode ? wcslen( str ) : strlen( str );

    string->dwCompStrLen = len;
    string->dwCompStrOffset = string->dwSize;
    dst = (BYTE *)string + string->dwCompStrOffset;
    memcpy( dst, str, len * (unicode ? sizeof(WCHAR) : 1) );
    string->dwSize += len * (unicode ? sizeof(WCHAR) : 1);

    string->dwCompClauseLen = 2 * sizeof(DWORD);
    string->dwCompClauseOffset = string->dwSize;
    dst = (BYTE *)string + string->dwCompClauseOffset;
    *(DWORD *)(dst + 0 * sizeof(DWORD)) = 0;
    *(DWORD *)(dst + 1 * sizeof(DWORD)) = len;
    string->dwSize += 2 * sizeof(DWORD);

    string->dwCompAttrLen = len;
    string->dwCompAttrOffset = string->dwSize;
    dst = (BYTE *)string + string->dwCompAttrOffset;
    memset( dst, ATTR_INPUT, len );
    string->dwSize += len;

    if (unicode) str = L"ReadResult";
    else str = "ReadResult";
    len = unicode ? wcslen( str ) : strlen( str );

    string->dwResultReadStrLen = len;
    string->dwResultReadStrOffset = string->dwSize;
    dst = (BYTE *)string + string->dwResultReadStrOffset;
    memcpy( dst, str, len * (unicode ? sizeof(WCHAR) : 1) );
    string->dwSize += len * (unicode ? sizeof(WCHAR) : 1);

    string->dwResultReadClauseLen = 2 * sizeof(DWORD);
    string->dwResultReadClauseOffset = string->dwSize;
    dst = (BYTE *)string + string->dwResultReadClauseOffset;
    *(DWORD *)(dst + 0 * sizeof(DWORD)) = 0;
    *(DWORD *)(dst + 1 * sizeof(DWORD)) = len;
    string->dwSize += 2 * sizeof(DWORD);

    if (unicode) str = L"Result";
    else str = "Result";
    len = unicode ? wcslen( str ) : strlen( str );

    string->dwResultStrLen = len;
    string->dwResultStrOffset = string->dwSize;
    dst = (BYTE *)string + string->dwResultStrOffset;
    memcpy( dst, str, len * (unicode ? sizeof(WCHAR) : 1) );
    string->dwSize += len * (unicode ? sizeof(WCHAR) : 1);

    string->dwResultClauseLen = 2 * sizeof(DWORD);
    string->dwResultClauseOffset = string->dwSize;
    dst = (BYTE *)string + string->dwResultClauseOffset;
    *(DWORD *)(dst + 0 * sizeof(DWORD)) = 0;
    *(DWORD *)(dst + 1 * sizeof(DWORD)) = len;
    string->dwSize += 2 * sizeof(DWORD);

    string->dwPrivateSize = 4;
    string->dwPrivateOffset = string->dwSize;
    dst = (BYTE *)string + string->dwPrivateOffset;
    memset( dst, 0xa5, string->dwPrivateSize );
    string->dwSize += 4;

    check_composition_string( string, unicode ? &expect_stringW : &expect_stringA );
    ok_ret( 0, ImmUnlockIMCC( ctx->hCompStr ) );
    old_himcc = ctx->hCompStr;

    for (i = 0; i < ARRAY_SIZE(gcs_indexes); ++i)
    {
        UINT_PTR expect;

        winetest_push_context( "%u", i );

        memset( buffer, 0xcd, sizeof(buffer) );
        expect = expect_retW[i];
        ok_ret( expect, ImmGetCompositionStringW( himc, gcs_indexes[i], buffer, sizeof(buffer) ) );
        memset( buffer + expect, 0, 4 );

        if (i == 0) ok_wcs( L"ReadComp", (WCHAR *)buffer );
        else if (i == 3) ok_wcs( L"Comp", (WCHAR *)buffer );
        else if (i == 8) ok_wcs( L"ReadResult", (WCHAR *)buffer );
        else if (i == 10) ok_wcs( L"Result", (WCHAR *)buffer );
        else if (i != 6 && i != 7) ok_wcs( L"", (WCHAR *)buffer );

        memset( buffer, 0xcd, sizeof(buffer) );
        expect = expect_retA[i];
        ok_ret( expect, ImmGetCompositionStringA( himc, gcs_indexes[i], buffer, sizeof(buffer) ) );
        memset( buffer + expect, 0, 4 );

        if (i == 0) ok_str( "ReadComp", (char *)buffer );
        else if (i == 3) ok_str( "Comp", (char *)buffer );
        else if (i == 8) ok_str( "ReadResult", (char *)buffer );
        else if (i == 10) ok_str( "Result", (char *)buffer );
        else if (i != 6 && i != 7) ok_str( "", (char *)buffer );

        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(gcs_indexes); ++i)
    {
        winetest_push_context( "%u", i );
        ok_ret( 0, ImmSetCompositionStringW( himc, gcs_indexes[i], buffer, sizeof(buffer), NULL, 0 ) );
        ok_ret( 0, ImmSetCompositionStringA( himc, gcs_indexes[i], buffer, sizeof(buffer), NULL, 0 ) );
        winetest_pop_context();
    }
    ok_ret( 0, ImmSetCompositionStringW( himc, SCS_SETSTR | SCS_CHANGEATTR, buffer, sizeof(buffer), NULL, 0 ) );
    ok_ret( 0, ImmSetCompositionStringA( himc, SCS_SETSTR | SCS_CHANGEATTR, buffer, sizeof(buffer), NULL, 0 ) );
    ok_ret( 0, ImmSetCompositionStringW( himc, SCS_CHANGECLAUSE | SCS_CHANGEATTR, buffer, sizeof(buffer), NULL, 0 ) );
    ok_ret( 0, ImmSetCompositionStringA( himc, SCS_CHANGECLAUSE | SCS_CHANGEATTR, buffer, sizeof(buffer), NULL, 0 ) );

    for (i = 0; i < ARRAY_SIZE(scs_indexes); ++i)
    {
        winetest_push_context( "%u", i );

        if (scs_indexes[i] == SCS_CHANGECLAUSE)
        {
            memset( buffer, 0, sizeof(buffer) );
            *((DWORD *)buffer + 1) = 1;
            len = 2 * sizeof(DWORD);
        }
        else if (scs_indexes[i] == SCS_CHANGEATTR)
        {
            memset( buffer, 0xcd, sizeof(buffer) );
            len = expect_stringW.dwCompAttrLen;
        }
        else if (scs_indexes[i] == SCS_SETSTR)
        {
            wcscpy( (WCHAR *)buffer, L"CompString" );
            len = 11 * sizeof(WCHAR);
        }

        todo_ImeSetCompositionString = !unicode;
        SET_EXPECT( ImeSetCompositionString );
        ok_ret( 1, ImmSetCompositionStringW( himc, scs_indexes[i], buffer, len, NULL, 0 ) );
        CHECK_CALLED( ImeSetCompositionString );
        todo_ImeSetCompositionString = FALSE;
        ok_seq( empty_sequence );

        string = ImmLockIMCC( ctx->hCompStr );
        ok_ne( NULL, string, COMPOSITIONSTRING *, "%p" );
        check_composition_string( string, unicode ? &expect_stringW : &expect_stringA );
        ok_ret( 0, ImmUnlockIMCC( ctx->hCompStr ) );

        if (scs_indexes[i] == SCS_CHANGECLAUSE)
        {
            memset( buffer, 0, sizeof(buffer) );
            *((DWORD *)buffer + 1) = 1;
            len = 2 * sizeof(DWORD);
        }
        else if (scs_indexes[i] == SCS_CHANGEATTR)
        {
            memset( buffer, 0xcd, sizeof(buffer) );
            len = expect_stringA.dwCompAttrLen;
        }
        else if (scs_indexes[i] == SCS_SETSTR)
        {
            strcpy( buffer, "CompString" );
            len = 11;
        }

        todo_ImeSetCompositionString = unicode;
        SET_EXPECT( ImeSetCompositionString );
        ok_ret( 1, ImmSetCompositionStringA( himc, scs_indexes[i], buffer, len, NULL, 0 ) );
        CHECK_CALLED( ImeSetCompositionString );
        todo_ImeSetCompositionString = FALSE;
        ok_seq( empty_sequence );

        string = ImmLockIMCC( ctx->hCompStr );
        ok_ne( NULL, string, COMPOSITIONSTRING *, "%p" );
        check_composition_string( string, unicode ? &expect_stringW : &expect_stringA );
        ok_ret( 0, ImmUnlockIMCC( ctx->hCompStr ) );

        winetest_pop_context();
    }
    ok_seq( empty_sequence );

    old_ctx = ctx;
    ok_ret( 1, ImmUnlockIMC( himc ) );

    /* composition strings are kept between IME selections */
    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    ctx = ImmLockIMC( himc );
    ok_eq( old_ctx, ctx, INPUTCONTEXT *, "%p" );
    ok_eq( old_himcc, ctx->hCompStr, HIMCC, "%p" );
    string = ImmLockIMCC( ctx->hCompStr );
    ok_ne( NULL, string, COMPOSITIONSTRING *, "%p" );
    *string = expect_string_empty;
    ok_ret( 0, ImmUnlockIMCC( ctx->hCompStr ) );
    ok_ret( 1, ImmActivateLayout( hkl ) );
    ok_eq( old_himcc, ctx->hCompStr, HIMCC, "%p" );
    check_composition_string( string, &expect_string_empty );
    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    ok_eq( old_himcc, ctx->hCompStr, HIMCC, "%p" );
    check_composition_string( string, &expect_string_empty );

    ok_ret( 1, ImmUnlockIMC( himc ) );
    ok_ret( 1, ImmDestroyContext( himc ) );

    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

    ok_ret( 1, ImmFreeLayout( hkl ) );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

cleanup:
    winetest_pop_context();
    SET_ENABLE( ImeSetCompositionString, FALSE );
}

static void test_ImmSetCompositionWindow(void)
{
    struct ime_call set_composition_window_0_seq[] =
    {
        {
            .hkl = expect_ime, .himc = 0/*himc*/,
            .func = IME_NOTIFY, .notify = {.action = NI_CONTEXTUPDATED, .index = 0, .value = IMC_SETCOMPOSITIONWINDOW},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_TEST_WIN, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETCOMPOSITIONWINDOW},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETCOMPOSITIONWINDOW},
        },
        {0},
    };
    struct ime_call set_composition_window_1_seq[] =
    {
        {
            .hkl = expect_ime, .himc = 0/*himc*/,
            .func = IME_NOTIFY, .notify = {.action = NI_CONTEXTUPDATED, .index = 0, .value = IMC_SETCOMPOSITIONWINDOW},
        },
        {0},
    };
    COMPOSITIONFORM comp_form, expect_form =
    {
        .dwStyle = 0xfeedcafe,
        .ptCurrentPos = {.x = 123, .y = 456},
        .rcArea = {.left = 1, .top = 2, .right = 3, .bottom = 4},
    };
    struct ime_windows ime_windows = {0};
    INPUTCONTEXT *ctx;
    HIMC himc;
    HWND hwnd;
    HKL hkl;

    ime_info.fdwProperty = IME_PROP_END_UNLOAD | IME_PROP_UNICODE;

    if (!(hkl = wineime_hkl)) return;

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

    set_composition_window_0_seq[0].himc = himc;
    set_composition_window_1_seq[0].himc = himc;

    ok_ret( 1, EnumThreadWindows( GetCurrentThreadId(), enum_thread_ime_windows, (LPARAM)&ime_windows ) );
    ok_ne( NULL, ime_windows.ime_hwnd, HWND, "%p" );
    ok_ne( NULL, ime_windows.ime_ui_hwnd, HWND, "%p" );

    ctx->cfCompForm = expect_form;
    ctx->fdwInit = ~INIT_COMPFORM;
    memset( &comp_form, 0xcd, sizeof(comp_form) );
    ok_ret( 0, ImmGetCompositionWindow( himc, &comp_form ) );
    ok_eq( 0xcdcdcdcd, comp_form.dwStyle, UINT, "%#x" );
    ctx->fdwInit = INIT_COMPFORM;
    ok_ret( 1, ImmGetCompositionWindow( himc, &comp_form ) );
    check_composition_form( &comp_form, &expect_form );
    ok_seq( empty_sequence );
    ok_ret( 0, IsWindowVisible( ime_windows.ime_ui_hwnd ) );

    ok_ret( 0, ShowWindow( ime_windows.ime_ui_hwnd, SW_SHOWNOACTIVATE ) );
    process_messages();
    ok_seq( empty_sequence );
    check_WM_SHOWWINDOW = TRUE;

    ctx->hWnd = hwnd;
    ctx->fdwInit = 0;
    memset( &comp_form, 0xcd, sizeof(comp_form) );
    ok_ret( 1, ImmSetCompositionWindow( himc, &comp_form ) );
    process_messages();
    ok_seq( set_composition_window_0_seq );
    ok_eq( INIT_COMPFORM, ctx->fdwInit, UINT, "%u" );
    check_composition_form( &ctx->cfCompForm, &comp_form );
    ok_ret( 1, IsWindowVisible( ime_windows.ime_ui_hwnd ) );
    check_WM_SHOWWINDOW = FALSE;

    ShowWindow( ime_windows.ime_ui_hwnd, SW_HIDE );
    process_messages();
    ok_seq( empty_sequence );

    ok_ret( 1, ImmSetCompositionWindow( himc, &expect_form ) );
    ok_seq( set_composition_window_0_seq );
    check_composition_form( &ctx->cfCompForm, &expect_form );

    ctx->cfCompForm = expect_form;
    ok_ret( 1, ImmGetCompositionWindow( himc, &comp_form ) );
    check_composition_form( &comp_form, &expect_form );
    ok_seq( empty_sequence );

    ctx->hWnd = 0;
    memset( &comp_form, 0xcd, sizeof(comp_form) );
    ok_ret( 1, ImmSetCompositionWindow( himc, &comp_form ) );
    ok_seq( set_composition_window_1_seq );
    check_composition_form( &ctx->cfCompForm, &comp_form );

    ok_ret( 1, ImmUnlockIMC( himc ) );
    ok_ret( 1, ImmDestroyContext( himc ) );

    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

    ok_ret( 1, ImmFreeLayout( hkl ) );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;
}

static void test_ImmSetStatusWindowPos(void)
{
    struct ime_call set_status_window_pos_0_seq[] =
    {
        {
            .hkl = expect_ime, .himc = 0/*himc*/,
            .func = IME_NOTIFY, .notify = {.action = NI_CONTEXTUPDATED, .index = 0, .value = IMC_SETSTATUSWINDOWPOS},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_TEST_WIN, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETSTATUSWINDOWPOS},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETSTATUSWINDOWPOS},
        },
        {0},
    };
    struct ime_call set_status_window_pos_1_seq[] =
    {
        {
            .hkl = expect_ime, .himc = 0/*himc*/,
            .func = IME_NOTIFY, .notify = {.action = NI_CONTEXTUPDATED, .index = 0, .value = IMC_SETSTATUSWINDOWPOS},
        },
        {0},
    };
    INPUTCONTEXT *ctx;
    POINT pos;
    HIMC himc;
    HWND hwnd;
    HKL hkl;

    ime_info.fdwProperty = IME_PROP_END_UNLOAD | IME_PROP_UNICODE;

    if (!(hkl = wineime_hkl)) return;

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

    set_status_window_pos_0_seq[0].himc = himc;
    set_status_window_pos_1_seq[0].himc = himc;

    memset( &pos, 0xcd, sizeof(pos) );
    ctx->ptStatusWndPos.x = 0xdeadbeef;
    ctx->ptStatusWndPos.y = 0xfeedcafe;
    ctx->fdwInit = ~INIT_STATUSWNDPOS;
    ok_ret( 0, ImmGetStatusWindowPos( himc, &pos ) );
    ok_eq( 0xcdcdcdcd, pos.x, UINT, "%u" );
    ok_eq( 0xcdcdcdcd, pos.y, UINT, "%u" );
    ctx->fdwInit = INIT_STATUSWNDPOS;
    ok_ret( 1, ImmGetStatusWindowPos( himc, &pos ) );
    ok_eq( 0xdeadbeef, pos.x, UINT, "%u" );
    ok_eq( 0xfeedcafe, pos.y, UINT, "%u" );
    ok_seq( empty_sequence );

    pos.x = 123;
    pos.y = 456;
    ctx->hWnd = hwnd;
    ctx->fdwInit = 0;
    ok_ret( 1, ImmSetStatusWindowPos( himc, &pos ) );
    ok_seq( set_status_window_pos_0_seq );
    ok_eq( INIT_STATUSWNDPOS, ctx->fdwInit, UINT, "%u" );

    ok_ret( 1, ImmSetStatusWindowPos( himc, &pos ) );
    ok_seq( set_status_window_pos_0_seq );
    ok_ret( 1, ImmGetStatusWindowPos( himc, &pos ) );
    ok_eq( 123, pos.x, UINT, "%u" );
    ok_eq( 123, ctx->ptStatusWndPos.x, UINT, "%u" );
    ok_eq( 456, pos.y, UINT, "%u" );
    ok_eq( 456, ctx->ptStatusWndPos.y, UINT, "%u" );
    ok_seq( empty_sequence );

    ctx->hWnd = 0;
    ok_ret( 1, ImmSetStatusWindowPos( himc, &pos ) );
    ok_seq( set_status_window_pos_1_seq );

    ok_ret( 1, ImmUnlockIMC( himc ) );
    ok_ret( 1, ImmDestroyContext( himc ) );

    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

    ok_ret( 1, ImmFreeLayout( hkl ) );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;
}

static void test_ImmSetCompositionFont( BOOL unicode )
{
    struct ime_call set_composition_font_0_seq[] =
    {
        {
            .hkl = expect_ime, .himc = 0/*himc*/,
            .func = IME_NOTIFY, .notify = {.action = NI_CONTEXTUPDATED, .index = 0, .value = IMC_SETCOMPOSITIONFONT},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_TEST_WIN, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETCOMPOSITIONFONT},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETCOMPOSITIONFONT},
        },
        {0},
    };
    struct ime_call set_composition_font_1_seq[] =
    {
        {
            .hkl = expect_ime, .himc = 0/*himc*/,
            .func = IME_NOTIFY, .notify = {.action = NI_CONTEXTUPDATED, .index = 0, .value = IMC_SETCOMPOSITIONFONT},
        },
        {0},
    };
    LOGFONTW fontW, expect_fontW =
    {
        .lfHeight = 1,
        .lfWidth = 2,
        .lfEscapement = 3,
        .lfOrientation = 4,
        .lfWeight = 5,
        .lfItalic = 6,
        .lfUnderline = 7,
        .lfStrikeOut = 8,
        .lfCharSet = 8,
        .lfOutPrecision = 10,
        .lfClipPrecision = 11,
        .lfQuality = 12,
        .lfPitchAndFamily = 13,
        .lfFaceName = L"FontFace",
    };
    LOGFONTA fontA, expect_fontA =
    {
        .lfHeight = 1,
        .lfWidth = 2,
        .lfEscapement = 3,
        .lfOrientation = 4,
        .lfWeight = 5,
        .lfItalic = 6,
        .lfUnderline = 7,
        .lfStrikeOut = 8,
        .lfCharSet = 8,
        .lfOutPrecision = 10,
        .lfClipPrecision = 11,
        .lfQuality = 12,
        .lfPitchAndFamily = 13,
        .lfFaceName = "FontFace",
    };
    INPUTCONTEXT *ctx;
    HIMC himc;
    HWND hwnd;
    HKL hkl;

    winetest_push_context( unicode ? "unicode" : "ansi" );

    /* IME_PROP_END_UNLOAD for the IME to unload / reload. */
    ime_info.fdwProperty = IME_PROP_END_UNLOAD;
    if (unicode) ime_info.fdwProperty |= IME_PROP_UNICODE;

    if (!(hkl = wineime_hkl)) goto cleanup;

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

    set_composition_font_0_seq[0].himc = himc;
    set_composition_font_1_seq[0].himc = himc;

    memset( &fontW, 0xcd, sizeof(fontW) );
    memset( &fontA, 0xcd, sizeof(fontA) );

    if (unicode) ctx->lfFont.W = expect_fontW;
    else ctx->lfFont.A = expect_fontA;
    ctx->fdwInit = ~INIT_LOGFONT;
    ok_ret( 0, ImmGetCompositionFontW( himc, &fontW ) );
    ok_ret( 0, ImmGetCompositionFontA( himc, &fontA ) );
    ctx->fdwInit = INIT_LOGFONT;
    ok_ret( 1, ImmGetCompositionFontW( himc, &fontW ) );
    check_logfont_w( &fontW, &expect_fontW );
    ok_ret( 1, ImmGetCompositionFontA( himc, &fontA ) );
    check_logfont_a( &fontA, &expect_fontA );

    ctx->hWnd = hwnd;
    ctx->fdwInit = 0;
    memset( &ctx->lfFont, 0xcd, sizeof(ctx->lfFont) );
    ok_ret( 1, ImmSetCompositionFontW( himc, &expect_fontW ) );
    ok_eq( INIT_LOGFONT, ctx->fdwInit, UINT, "%u" );
    ok_seq( set_composition_font_0_seq );
    ok_ret( 1, ImmSetCompositionFontW( himc, &expect_fontW ) );
    ok_seq( set_composition_font_0_seq );
    if (unicode) check_logfont_w( &ctx->lfFont.W, &expect_fontW );
    else check_logfont_a( &ctx->lfFont.A, &expect_fontA );

    ok_ret( 1, ImmGetCompositionFontW( himc, &fontW ) );
    check_logfont_w( &fontW, &expect_fontW );
    ok_ret( 1, ImmGetCompositionFontA( himc, &fontA ) );
    check_logfont_a( &fontA, &expect_fontA );

    ctx->hWnd = hwnd;
    ctx->fdwInit = 0;
    memset( &ctx->lfFont, 0xcd, sizeof(ctx->lfFont) );
    ok_ret( 1, ImmSetCompositionFontA( himc, &expect_fontA ) );
    ok_eq( INIT_LOGFONT, ctx->fdwInit, UINT, "%u" );
    ok_seq( set_composition_font_0_seq );
    ok_ret( 1, ImmSetCompositionFontA( himc, &expect_fontA ) );
    ok_seq( set_composition_font_0_seq );
    if (unicode) check_logfont_w( &ctx->lfFont.W, &expect_fontW );
    else check_logfont_a( &ctx->lfFont.A, &expect_fontA );

    ok_ret( 1, ImmGetCompositionFontW( himc, &fontW ) );
    check_logfont_w( &fontW, &expect_fontW );
    ok_ret( 1, ImmGetCompositionFontA( himc, &fontA ) );
    check_logfont_a( &fontA, &expect_fontA );

    ctx->hWnd = 0;
    ok_ret( 1, ImmSetCompositionFontW( himc, &expect_fontW ) );
    ok_seq( set_composition_font_1_seq );
    ok_ret( 1, ImmSetCompositionFontA( himc, &expect_fontA ) );
    ok_seq( set_composition_font_1_seq );

    ok_ret( 1, ImmUnlockIMC( himc ) );
    ok_ret( 1, ImmDestroyContext( himc ) );

    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

    ok_ret( 1, ImmFreeLayout( hkl ) );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

cleanup:
    winetest_pop_context();
}

static void test_ImmSetCandidateWindow(void)
{
    struct ime_call set_candidate_window_0_seq[] =
    {
        {
            .hkl = expect_ime, .himc = 0/*himc*/,
            .func = IME_NOTIFY, .notify = {.action = NI_CONTEXTUPDATED, .index = 0, .value = IMC_SETCANDIDATEPOS},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_TEST_WIN, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETCANDIDATEPOS, .lparam = 4},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_NOTIFY, .wparam = IMN_SETCANDIDATEPOS, .lparam = 4},
        },
        {0},
    };
    struct ime_call set_candidate_window_1_seq[] =
    {
        {
            .hkl = expect_ime, .himc = 0/*himc*/,
            .func = IME_NOTIFY, .notify = {.action = NI_CONTEXTUPDATED, .index = 0, .value = IMC_SETCANDIDATEPOS},
        },
        {0},
    };
    CANDIDATEFORM cand_form, expect_form =
    {
        .dwIndex = 2, .dwStyle = 0xfeedcafe,
        .ptCurrentPos = {.x = 123, .y = 456},
        .rcArea = {.left = 1, .top = 2, .right = 3, .bottom = 4},
    };
    INPUTCONTEXT *ctx;
    HIMC himc;
    HWND hwnd;
    HKL hkl;

    ime_info.fdwProperty = IME_PROP_END_UNLOAD | IME_PROP_UNICODE;

    if (!(hkl = wineime_hkl)) return;

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

    set_candidate_window_0_seq[0].himc = himc;
    set_candidate_window_1_seq[0].himc = himc;

    ctx->cfCandForm[1] = expect_form;
    ctx->cfCandForm[2] = expect_form;
    ctx->fdwInit = 0;
    memset( &cand_form, 0xcd, sizeof(cand_form) );
    ok_ret( 0, ImmGetCandidateWindow( himc, 0, &cand_form ) );
    ok_eq( 0xcdcdcdcd, cand_form.dwStyle, UINT, "%#x" );
    ok_ret( 1, ImmGetCandidateWindow( himc, 1, &cand_form ) );
    check_candidate_form( &cand_form, &expect_form );
    ok_ret( 1, ImmGetCandidateWindow( himc, 2, &cand_form ) );
    check_candidate_form( &cand_form, &expect_form );
    ok_seq( empty_sequence );

    ctx->hWnd = hwnd;
    memset( &cand_form, 0xcd, sizeof(cand_form) );
    cand_form.dwIndex = 2;
    ok_ret( 1, ImmSetCandidateWindow( himc, &cand_form ) );
    ok_seq( set_candidate_window_0_seq );
    check_candidate_form( &ctx->cfCandForm[2], &cand_form );
    ok_eq( 0, ctx->fdwInit, UINT, "%u" );

    ok_ret( 1, ImmSetCandidateWindow( himc, &expect_form ) );
    ok_seq( set_candidate_window_0_seq );
    check_candidate_form( &ctx->cfCandForm[2], &expect_form );

    ctx->hWnd = 0;
    memset( &cand_form, 0xcd, sizeof(cand_form) );
    cand_form.dwIndex = 2;
    ok_ret( 1, ImmSetCandidateWindow( himc, &cand_form ) );
    ok_seq( set_candidate_window_1_seq );
    check_candidate_form( &ctx->cfCandForm[2], &cand_form );

    ok_ret( 1, ImmUnlockIMC( himc ) );
    ok_ret( 1, ImmDestroyContext( himc ) );

    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

    ok_ret( 1, ImmFreeLayout( hkl ) );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;
}

static void test_ImmGenerateMessage(void)
{
    const struct ime_call generate_sequence[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_TEST_WIN, .message = {.msg = WM_IME_COMPOSITION, .wparam = 0, .lparam = GCS_COMPSTR},
        },
        {
            .hkl = expect_ime, .himc = default_himc,
            .func = MSG_IME_UI, .message = {.msg = WM_IME_COMPOSITION, .wparam = 0, .lparam = GCS_COMPSTR},
        },
        {0},
    };
    TRANSMSG *msgs, *tmp_msgs;
    INPUTCONTEXT *ctx;
    HIMC himc;
    HWND hwnd;
    HKL hkl;

    ime_info.fdwProperty = IME_PROP_END_UNLOAD | IME_PROP_UNICODE;

    if (!(hkl = wineime_hkl)) return;

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

    todo_wine ok_ret( 4, ImmGetIMCCSize( ctx->hMsgBuf ) );
    ctx->hMsgBuf = ImmReSizeIMCC( ctx->hMsgBuf, sizeof(*msgs) );
    ok_ne( NULL, ctx->hMsgBuf, HIMCC, "%p" );

    msgs = ImmLockIMCC( ctx->hMsgBuf );
    ok_ne( NULL, msgs, TRANSMSG *, "%p" );
    msgs[0].message = WM_IME_COMPOSITION;
    msgs[0].wParam = 0;
    msgs[0].lParam = GCS_COMPSTR;
    ok_ret( 0, ImmUnlockIMCC( ctx->hMsgBuf ) );

    ctx->hWnd = 0;
    ctx->dwNumMsgBuf = 0;
    ok_ret( 1, ImmGenerateMessage( himc ) );
    ok_seq( empty_sequence );
    ok_ret( sizeof(*msgs), ImmGetIMCCSize( ctx->hMsgBuf ) );
    tmp_msgs = ImmLockIMCC( ctx->hMsgBuf );
    ok_eq( msgs, tmp_msgs, TRANSMSG *, "%p" );
    ok_ret( 0, ImmUnlockIMCC( ctx->hMsgBuf ) );

    ctx->dwNumMsgBuf = 1;
    ok_ret( 1, ImmGenerateMessage( himc ) );
    ok_seq( empty_sequence );
    ok_eq( 0, ctx->dwNumMsgBuf, UINT, "%u" );
    ok_ret( sizeof(*msgs), ImmGetIMCCSize( ctx->hMsgBuf ) );
    tmp_msgs = ImmLockIMCC( ctx->hMsgBuf );
    ok_eq( msgs, tmp_msgs, TRANSMSG *, "%p" );
    ok_ret( 0, ImmUnlockIMCC( ctx->hMsgBuf ) );

    ctx->hWnd = hwnd;
    ctx->dwNumMsgBuf = 0;
    ok_ret( 1, ImmGenerateMessage( himc ) );
    ok_seq( empty_sequence );
    ok_ret( sizeof(*msgs), ImmGetIMCCSize( ctx->hMsgBuf ) );
    tmp_msgs = ImmLockIMCC( ctx->hMsgBuf );
    ok_eq( msgs, tmp_msgs, TRANSMSG *, "%p" );
    ok_ret( 0, ImmUnlockIMCC( ctx->hMsgBuf ) );

    ctx->dwNumMsgBuf = 1;
    ok_ret( 1, ImmGenerateMessage( himc ) );
    ok_seq( generate_sequence );
    ok_eq( 0, ctx->dwNumMsgBuf, UINT, "%u" );
    ok_ret( sizeof(*msgs), ImmGetIMCCSize( ctx->hMsgBuf ) );
    tmp_msgs = ImmLockIMCC( ctx->hMsgBuf );
    ok_eq( msgs, tmp_msgs, TRANSMSG *, "%p" );
    ok_ret( 0, ImmUnlockIMCC( ctx->hMsgBuf ) );

    tmp_msgs = ImmLockIMCC( ctx->hMsgBuf );
    ok_eq( msgs, tmp_msgs, TRANSMSG *, "%p" );
    ok_ret( 0, ImmUnlockIMCC( ctx->hMsgBuf ) );

    ok_ret( 1, ImmUnlockIMC( himc ) );
    ok_ret( 1, ImmDestroyContext( himc ) );

    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

    ok_ret( 1, ImmFreeLayout( hkl ) );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;
}

static void test_ImmTranslateMessage( BOOL kbd_char_first )
{
    const struct ime_call process_key_seq[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc, .func = IME_PROCESS_KEY,
            .process_key = {.vkey = 'Q', .lparam = MAKELONG(2, 0x10)},
        },
        {
            .hkl = expect_ime, .himc = default_himc, .func = IME_PROCESS_KEY,
            .process_key = {.vkey = 'Q', .lparam = MAKELONG(2, 0xc010)},
        },
        {0},
    };
    const struct ime_call to_ascii_ex_0[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc, .func = IME_TO_ASCII_EX,
            .to_ascii_ex = {.vkey = kbd_char_first ? MAKELONG('Q', 'q') : 'Q', .vsc = 0x10},
        },
        {0},
    };
    const struct ime_call to_ascii_ex_1[] =
    {
        {
            .hkl = expect_ime, .himc = default_himc, .func = IME_PROCESS_KEY,
            .process_key = {.vkey = 'Q', .lparam = MAKELONG(2, 0xc010)},
        },
        {
            .hkl = expect_ime, .himc = default_himc, .func = IME_TO_ASCII_EX,
            /* FIXME what happened to kbd_char_first here!? */
            .to_ascii_ex = {.vkey = 'Q', .vsc = 0xc010},
            .todo_value = TRUE,
        },
        {0},
    };
    struct ime_call to_ascii_ex_2[] =
    {
        {
            .hkl = expect_ime, .himc = 0/*himc*/, .func = IME_PROCESS_KEY,
            .process_key = {.vkey = 'Q', .lparam = MAKELONG(2, 0x210)},
        },
        {
            .hkl = expect_ime, .himc = 0/*himc*/, .func = IME_TO_ASCII_EX,
            .to_ascii_ex = {.vkey = kbd_char_first ? MAKELONG('Q', 'q') : 'Q', .vsc = 0x210},
        },
        {
            .hkl = expect_ime, .himc = 0/*himc*/, .func = IME_PROCESS_KEY,
            .process_key = {.vkey = 'Q', .lparam = MAKELONG(2, 0x410)},
        },
        {
            .hkl = expect_ime, .himc = 0/*himc*/, .func = IME_TO_ASCII_EX,
            .to_ascii_ex = {.vkey = kbd_char_first ? MAKELONG('Q', 'q') : 'Q', .vsc = 0x410},
        },
        {0},
    };
    struct ime_call to_ascii_ex_3[] =
    {
        {
            .hkl = expect_ime, .himc = 0/*himc*/, .func = IME_PROCESS_KEY,
            .process_key = {.vkey = 'Q', .lparam = MAKELONG(2, 0xa10)},
        },
        {
            .hkl = expect_ime, .himc = 0/*himc*/, .func = IME_TO_ASCII_EX,
            .to_ascii_ex = {.vkey = kbd_char_first ? MAKELONG('Q', 'q') : 'Q', .vsc = 0xa10},
        },
        {
            .hkl = expect_ime, .himc = 0/*himc*/, .func = IME_PROCESS_KEY,
            .process_key = {.vkey = 'Q', .lparam = MAKELONG(2, 0xc10)},
        },
        {
            .hkl = expect_ime, .himc = 0/*himc*/, .func = IME_TO_ASCII_EX,
            .to_ascii_ex = {.vkey = kbd_char_first ? MAKELONG('Q', 'q') : 'Q', .vsc = 0xc10},
        },
        {0},
    };
    struct ime_call post_messages[] =
    {
        {.hkl = expect_ime, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_STARTCOMPOSITION, .wparam = 1}},
        {.hkl = expect_ime, .himc = 0/*himc*/, .func = MSG_IME_UI, .message = {.msg = WM_IME_STARTCOMPOSITION, .wparam = 1},
         .todo_himc = TRUE /* on some Wine configurations the IME UI doesn't get an HIMC */
        },
        {.hkl = expect_ime, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_ENDCOMPOSITION, .wparam = 1}},
        {.hkl = expect_ime, .himc = 0/*himc*/, .func = MSG_IME_UI, .message = {.msg = WM_IME_ENDCOMPOSITION, .wparam = 1},
         .todo_himc = TRUE /* on some Wine configurations the IME UI doesn't get an HIMC */
        },
        {0},
    };
    struct ime_call sent_messages[] =
    {
        {.hkl = expect_ime, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_STARTCOMPOSITION, .wparam = 2}},
        {.hkl = expect_ime, .himc = 0/*himc*/, .func = MSG_IME_UI, .message = {.msg = WM_IME_STARTCOMPOSITION, .wparam = 2},
         .todo_himc = TRUE /* on some Wine configurations the IME UI doesn't get an HIMC */
        },
        {.hkl = expect_ime, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_ENDCOMPOSITION, .wparam = 2}},
        {.hkl = expect_ime, .himc = 0/*himc*/, .func = MSG_IME_UI, .message = {.msg = WM_IME_ENDCOMPOSITION, .wparam = 2},
         .todo_himc = TRUE /* on some Wine configurations the IME UI doesn't get an HIMC */
        },
        {0},
    };
    HWND hwnd, other_hwnd;
    INPUTCONTEXT *ctx;
    HIMC himc;
    HKL hkl;
    UINT i;

    ime_info.fdwProperty = IME_PROP_END_UNLOAD | IME_PROP_UNICODE;
    if (kbd_char_first) ime_info.fdwProperty |= IME_PROP_KBD_CHAR_FIRST;

    winetest_push_context( kbd_char_first ? "kbd_char_first" : "default" );

    if (!(hkl = wineime_hkl)) goto cleanup;

    other_hwnd = CreateWindowW( test_class.lpszClassName, NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                100, 100, 100, 100, NULL, NULL, NULL, NULL );
    ok( !!other_hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );
    flush_events();

    hwnd = CreateWindowW( test_class.lpszClassName, NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 100, 100, NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );
    flush_events();

    ok_ret( 1, ImmActivateLayout( hkl ) );
    ok_ret( 1, ImmLoadIME( hkl ) );
    himc = ImmCreateContext();
    ok_ne( NULL, himc, HIMC, "%p" );
    ctx = ImmLockIMC( himc );
    ok_ne( NULL, ctx, INPUTCONTEXT *, "%p" );
    process_messages();
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    ok_ret( 2, ImmProcessKey( hwnd, expect_ime, 'Q', MAKELONG(2, 0x10), 0 ) );
    ok_ret( 2, ImmProcessKey( hwnd, expect_ime, 'Q', MAKELONG(2, 0xc010), 0 ) );

    ok_ret( 0, ImmTranslateMessage( hwnd, 0, 0, 0 ) );
    ok_ret( 'Q', ImmGetVirtualKey( hwnd ) );
    ok_seq( process_key_seq );

    ok_ret( 0, ImmTranslateMessage( hwnd, WM_KEYDOWN, 'Q', MAKELONG(2, 0x10) ) );
    ok_ret( VK_PROCESSKEY, ImmGetVirtualKey( hwnd ) );
    ok_seq( to_ascii_ex_0 );

    ok_ret( 0, ImmTranslateMessage( hwnd, WM_KEYUP, 'Q', MAKELONG(2, 0xc010) ) );
    ok_seq( empty_sequence );

    ok_ret( 2, ImmProcessKey( hwnd, expect_ime, 'Q', MAKELONG(2, 0xc010), 0 ) );
    ok_ret( 0, ImmTranslateMessage( hwnd, WM_KEYUP, 'Q', MAKELONG(2, 0xc010) ) );
    ok_ret( VK_PROCESSKEY, ImmGetVirtualKey( hwnd ) );
    ok_seq( to_ascii_ex_1 );

    ok_eq( default_himc, ImmAssociateContext( hwnd, himc ), HIMC, "%p" );
    ok_eq( default_himc, ImmAssociateContext( other_hwnd, himc ), HIMC, "%p" );
    for (i = 0; i < ARRAY_SIZE(to_ascii_ex_2); i++) to_ascii_ex_2[i].himc = himc;
    for (i = 0; i < ARRAY_SIZE(to_ascii_ex_3); i++) to_ascii_ex_3[i].himc = himc;
    for (i = 0; i < ARRAY_SIZE(post_messages); i++) post_messages[i].himc = himc;
    for (i = 0; i < ARRAY_SIZE(sent_messages); i++) sent_messages[i].himc = himc;
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    ctx->hWnd = hwnd;
    ok_ret( 2, ImmProcessKey( hwnd, expect_ime, 'Q', MAKELONG(2, 0x210), 0 ) );
    ok_ret( 1, ImmTranslateMessage( hwnd, WM_KEYUP, 'Q', MAKELONG(2, 0x210) ) );
    ok_ret( 2, ImmProcessKey( hwnd, expect_ime, 'Q', MAKELONG(2, 0x410), 0 ) );
    ok_ret( 0, ImmTranslateMessage( hwnd, WM_KEYUP, 'Q', MAKELONG(2, 0x410) ) );
    ok_ret( VK_PROCESSKEY, ImmGetVirtualKey( hwnd ) );
    ok_seq( to_ascii_ex_2 );
    process_messages();
    ok_seq( post_messages );
    ok_ret( 1, ImmGenerateMessage( himc ) );
    ok_seq( sent_messages );

    ok_ret( 2, ImmProcessKey( hwnd, expect_ime, 'Q', MAKELONG(2, 0xa10), 0 ) );
    ok_ret( 0, ImmTranslateMessage( hwnd, WM_KEYUP, 'Q', MAKELONG(2, 0xa10) ) );
    ok_ret( 2, ImmProcessKey( hwnd, expect_ime, 'Q', MAKELONG(2, 0xc10), 0 ) );
    ok_ret( 0, ImmTranslateMessage( hwnd, WM_KEYUP, 'Q', MAKELONG(2, 0xc10) ) );
    ok_ret( VK_PROCESSKEY, ImmGetVirtualKey( hwnd ) );
    ok_seq( to_ascii_ex_3 );
    process_messages();
    ok_seq( empty_sequence );
    ok_ret( 1, ImmGenerateMessage( himc ) );
    ok_seq( sent_messages );

    ctx->hWnd = 0;
    ok_ret( 2, ImmProcessKey( other_hwnd, expect_ime, 'Q', MAKELONG(2, 0x210), 0 ) );
    ok_ret( 1, ImmTranslateMessage( other_hwnd, WM_KEYUP, 'Q', MAKELONG(2, 0x210) ) );
    ok_ret( 2, ImmProcessKey( other_hwnd, expect_ime, 'Q', MAKELONG(2, 0x410), 0 ) );
    ok_ret( 0, ImmTranslateMessage( other_hwnd, WM_KEYUP, 'Q', MAKELONG(2, 0x410) ) );
    ok_ret( VK_PROCESSKEY, ImmGetVirtualKey( other_hwnd ) );
    ok_seq( to_ascii_ex_2 );
    process_messages_( hwnd );
    ok_seq( empty_sequence );
    process_messages_( other_hwnd );
    ok_seq( post_messages );
    ok_ret( 1, ImmGenerateMessage( himc ) );
    ok_seq( empty_sequence );

    ok_ret( 1, ImmUnlockIMC( himc ) );
    ok_ret( 1, ImmDestroyContext( himc ) );

    ok_ret( 1, ImmActivateLayout( default_hkl ) );
    ok_ret( 1, DestroyWindow( other_hwnd ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

    ok_ret( 1, ImmFreeLayout( hkl ) );
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

cleanup:
    winetest_pop_context();
}

static void test_ga_na_da(void)
{
    /* These sequences have some additional WM_IME_NOTIFY messages with unknown wparam > IMN_PRIVATE */
    struct ime_call complete_seq[] =
    {
        /* G */
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_STARTCOMPOSITION, .wparam = 0, .lparam = 0}},
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\u3131", .result = L"",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0x3131, .lparam = GCS_COMPSTR|GCS_COMPATTR|CS_INSERTCHAR|CS_NOMOVECARET},
        },

        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"", .result = L"",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0x1b, .lparam = GCS_CURSORPOS|GCS_DELTASTART|GCS_COMPSTR|GCS_COMPATTR|GCS_COMPCLAUSE|
                                                                             GCS_COMPREADSTR|GCS_COMPREADATTR|GCS_COMPREADCLAUSE},
        },
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_ENDCOMPOSITION, .wparam = 0, .lparam = 0}},

        /* G */
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_STARTCOMPOSITION, .wparam = 0, .lparam = 0}},
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\u3131", .result = L"",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0x3131, .lparam = GCS_COMPSTR|GCS_COMPATTR|CS_INSERTCHAR|CS_NOMOVECARET},
        },
        /* A */
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\uac00", .result = L"",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0xac00, .lparam = GCS_COMPSTR|GCS_COMPATTR|CS_INSERTCHAR|CS_NOMOVECARET},
        },
        /* N */
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\uac04", .result = L"",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0xac04, .lparam = GCS_COMPSTR|GCS_COMPATTR|CS_INSERTCHAR|CS_NOMOVECARET},
        },
        /* A */
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\ub098", .result = L"\uac00",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0xac00, .lparam = GCS_RESULTSTR},
        },
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_CHAR, .wparam = 0xac00, .lparam = 0x1}},
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\ub098", .result = L"\uac00",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0xb098, .lparam = GCS_COMPSTR|GCS_COMPATTR|CS_INSERTCHAR|CS_NOMOVECARET},
        },
        /* D */
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\ub09f", .result = L"",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0xb09f, .lparam = GCS_COMPSTR|GCS_COMPATTR|CS_INSERTCHAR|CS_NOMOVECARET},
        },
        /* A */
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\ub2e4", .result = L"\ub098",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0xb098, .lparam = GCS_RESULTSTR},
        },
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_CHAR, .wparam = 0xb098, .lparam = 0x1}},
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\ub2e4", .result = L"\ub098",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0xb2e4, .lparam = GCS_COMPSTR|GCS_COMPATTR|CS_INSERTCHAR|CS_NOMOVECARET},
        },
        /* RETURN */
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_ENDCOMPOSITION, .wparam = 0, .lparam = 0}},
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"", .result = L"\ub2e4",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0xb2e4, .lparam = GCS_RESULTSTR},
        },
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_CHAR, .wparam = 0xb2e4, .lparam = 0x1}},
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_KEYDOWN, .wparam = 0xd, .lparam = 0x1c0001}},
        {0},
    };
    struct ime_call partial_g_seq[] =
    {
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_STARTCOMPOSITION, .wparam = 0, .lparam = 0}},
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\u3131", .result = L"",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0x3131, .lparam = GCS_COMPSTR|GCS_COMPATTR|CS_INSERTCHAR|CS_NOMOVECARET},
        },
        {0},
    };
    struct ime_call partial_ga_seq[] =
    {
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\uac00", .result = L"",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0xac00, .lparam = GCS_COMPSTR|GCS_COMPATTR|CS_INSERTCHAR|CS_NOMOVECARET},
        },
        {0},
    };
    struct ime_call partial_n_seq[] =
    {
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\uac04", .result = L"",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0xac04, .lparam = GCS_COMPSTR|GCS_COMPATTR|CS_INSERTCHAR|CS_NOMOVECARET},
        },
        {0},
    };
    struct ime_call partial_na_seq[] =
    {
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\ub098", .result = L"\uac00",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0xac00, .lparam = GCS_RESULTSTR},
        },
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_CHAR, .wparam = 0xac00, .lparam = 0x1}},
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\ub098", .result = L"\uac00",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0xb098, .lparam = GCS_COMPSTR|GCS_COMPATTR|CS_INSERTCHAR|CS_NOMOVECARET},
        },
        {0},
    };
    struct ime_call partial_d_seq[] =
    {
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\ub09f", .result = L"",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0xb09f, .lparam = GCS_COMPSTR|GCS_COMPATTR|CS_INSERTCHAR|CS_NOMOVECARET},
        },
        {0},
    };
    struct ime_call partial_da_seq[] =
    {
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\ub2e4", .result = L"\ub098",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0xb098, .lparam = GCS_RESULTSTR},
        },
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_CHAR, .wparam = 0xb098, .lparam = 0x1}},
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\ub2e4", .result = L"\ub098",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0xb2e4, .lparam = GCS_COMPSTR|GCS_COMPATTR|CS_INSERTCHAR|CS_NOMOVECARET},
        },
        {0},
    };
    struct ime_call partial_return_seq[] =
    {
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_ENDCOMPOSITION, .wparam = 0, .lparam = 0}},
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"", .result = L"\ub2e4",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0xb2e4, .lparam = GCS_RESULTSTR},
        },
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_CHAR, .wparam = 0xb2e4, .lparam = 0x1}},
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_KEYDOWN, .wparam = 0xd, .lparam = 0x1c0001}},
        {0},
    };

    INPUTCONTEXT *ctx;
    HWND hwnd;
    HIMC himc;
    UINT i;

    /* this test doesn't work on Win32 / WoW64 */
    if (sizeof(void *) == 4 || default_hkl != (HKL)0x04120412 /* MS Korean IME */)
    {
        skip( "Got hkl %p, skipping Korean IME-specific test\n", default_hkl );
        process_messages();
        return;
    }

    hwnd = CreateWindowW( test_class.lpszClassName, NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 100, 100, NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );
    flush_events();

    himc = ImmCreateContext();
    ok_ne( NULL, himc, HIMC, "%p" );
    ctx = ImmLockIMC( himc );
    ok_ne( NULL, ctx, INPUTCONTEXT *, "%p" );
    ok_eq( default_himc, ImmAssociateContext( hwnd, himc ), HIMC, "%p" );
    ok_ret( 1, ImmSetOpenStatus( himc, TRUE ) );
    ok_ret( 1, ImmSetConversionStatus( himc, IME_CMODE_FULLSHAPE | IME_CMODE_NATIVE, IME_SMODE_PHRASEPREDICT ) );
    flush_events();
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    for (i = 0; i < ARRAY_SIZE(complete_seq); i++) complete_seq[i].himc = himc;
    for (i = 0; i < ARRAY_SIZE(partial_g_seq); i++) partial_g_seq[i].himc = himc;
    for (i = 0; i < ARRAY_SIZE(partial_ga_seq); i++) partial_ga_seq[i].himc = himc;
    for (i = 0; i < ARRAY_SIZE(partial_n_seq); i++) partial_n_seq[i].himc = himc;
    for (i = 0; i < ARRAY_SIZE(partial_na_seq); i++) partial_na_seq[i].himc = himc;
    for (i = 0; i < ARRAY_SIZE(partial_d_seq); i++) partial_d_seq[i].himc = himc;
    for (i = 0; i < ARRAY_SIZE(partial_da_seq); i++) partial_da_seq[i].himc = himc;
    for (i = 0; i < ARRAY_SIZE(partial_return_seq); i++) partial_return_seq[i].himc = himc;

    keybd_event( 'R', 0x13, 0, 0 );
    flush_events();
    keybd_event( 'R', 0x13, KEYEVENTF_KEYUP, 0 );

    keybd_event( VK_BACK, 0x0e, 0, 0 );
    flush_events();
    keybd_event( VK_BACK, 0x0e, KEYEVENTF_KEYUP, 0 );

    keybd_event( 'R', 0x13, 0, 0 );
    flush_events();
    keybd_event( 'R', 0x13, KEYEVENTF_KEYUP, 0 );

    keybd_event( 'K', 0x25, 0, 0 );
    flush_events();
    keybd_event( 'K', 0x25, KEYEVENTF_KEYUP, 0 );

    keybd_event( 'S', 0x1f, 0, 0 );
    flush_events();
    keybd_event( 'S', 0x1f, KEYEVENTF_KEYUP, 0 );

    keybd_event( 'K', 0x25, 0, 0 );
    flush_events();
    keybd_event( 'K', 0x25, KEYEVENTF_KEYUP, 0 );

    keybd_event( 'E', 0x12, 0, 0 );
    flush_events();
    keybd_event( 'E', 0x12, KEYEVENTF_KEYUP, 0 );

    keybd_event( 'K', 0x25, 0, 0 );
    flush_events();
    keybd_event( 'K', 0x25, KEYEVENTF_KEYUP, 0 );

    keybd_event( VK_RETURN, 0x1c, 0, 0 );
    flush_events();
    keybd_event( VK_RETURN, 0x1c, KEYEVENTF_KEYUP, 0 );

    flush_events();
    todo_wine ok_seq( complete_seq );


    /* Korean IME uses ImeProcessKey and posts messages */

    todo_wine ok_ret( 2, ImmProcessKey( hwnd, default_hkl, 'R', MAKELONG(1, 0x13), 0 ) );
    ok_ret( 0, ImmTranslateMessage( hwnd, WM_KEYDOWN, 'R', MAKELONG(1, 0x13) ) );
    ok_seq( empty_sequence );
    process_messages();
    todo_wine ok_seq( partial_g_seq );

    /* Korean IME doesn't eat WM_KEYUP */

    ok_ret( 0, ImmProcessKey( hwnd, default_hkl, 'R', MAKELONG(1, 0xc013), 0 ) );
    ok_ret( 0, ImmTranslateMessage( hwnd, WM_KEYUP, 'R', MAKELONG(1, 0xc013) ) );
    process_messages();
    ok_seq( empty_sequence );

    todo_wine ok_ret( 2, ImmProcessKey( hwnd, default_hkl, 'K', MAKELONG(1, 0x25), 0 ) );
    ok_ret( 0, ImmTranslateMessage( hwnd, WM_KEYDOWN, 'K', MAKELONG(1, 0x25) ) );
    process_messages();
    todo_wine ok_seq( partial_ga_seq );

    todo_wine ok_ret( 2, ImmProcessKey( hwnd, default_hkl, 'S', MAKELONG(1, 0x1f), 0 ) );
    ok_ret( 0, ImmTranslateMessage( hwnd, WM_KEYDOWN, 'S', MAKELONG(1, 0x1f) ) );
    process_messages();
    todo_wine ok_seq( partial_n_seq );

    todo_wine ok_ret( 2, ImmProcessKey( hwnd, default_hkl, 'K', MAKELONG(1, 0x25), 0 ) );
    ok_ret( 0, ImmTranslateMessage( hwnd, WM_KEYDOWN, 'K', MAKELONG(1, 0x25) ) );
    process_messages();
    todo_wine ok_seq( partial_na_seq );

    todo_wine ok_ret( 2, ImmProcessKey( hwnd, default_hkl, 'E', MAKELONG(1, 0x12), 0 ) );
    ok_ret( 0, ImmTranslateMessage( hwnd, WM_KEYDOWN, 'E', MAKELONG(1, 0x12) ) );
    process_messages();
    todo_wine ok_seq( partial_d_seq );

    todo_wine ok_ret( 2, ImmProcessKey( hwnd, default_hkl, 'K', MAKELONG(1, 0x25), 0 ) );
    ok_ret( 0, ImmTranslateMessage( hwnd, WM_KEYDOWN, 'K', MAKELONG(1, 0x25) ) );
    process_messages();
    todo_wine ok_seq( partial_da_seq );

    todo_wine ok_ret( 2, ImmProcessKey( hwnd, default_hkl, VK_RETURN, MAKELONG(1, 0x1c), 0 ) );
    ok_ret( 0, ImmTranslateMessage( hwnd, WM_KEYDOWN, VK_RETURN, MAKELONG(1, 0x1c) ) );
    process_messages();
    todo_wine ok_seq( partial_return_seq );


    ok_ret( 1, ImmSetConversionStatus( himc, 0, IME_SMODE_PHRASEPREDICT ) );
    ok_ret( 1, ImmSetOpenStatus( himc, FALSE ) );

    ok_ret( 1, ImmUnlockIMC( himc ) );
    ok_ret( 1, ImmDestroyContext( himc ) );

    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;
}

static void test_nihongo_no(void)
{
    /* These sequences have some additional WM_IME_NOTIFY messages with wparam > IMN_PRIVATE */
    /* Some out-of-order WM_IME_REQUEST and WM_IME_NOTIFY messages are also ignored */
    struct ime_call complete_seq[] =
    {
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_STARTCOMPOSITION, .wparam = 0, .lparam = 0}},
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\uff4e", .result = L"",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0xff4e, .lparam = GCS_COMPSTR|GCS_COMPCLAUSE|GCS_COMPATTR|GCS_COMPREADSTR|GCS_DELTASTART|GCS_CURSORPOS},
        },
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\u306b", .result = L"",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0x306b, .lparam = GCS_COMPSTR|GCS_COMPCLAUSE|GCS_COMPATTR|GCS_COMPREADSTR|GCS_DELTASTART|GCS_CURSORPOS},
        },
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\u306b\uff48", .result = L"",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0x306b, .lparam = GCS_COMPSTR|GCS_COMPCLAUSE|GCS_COMPATTR|GCS_COMPREADSTR|GCS_DELTASTART|GCS_CURSORPOS},
        },
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\u306b\u307b", .result = L"",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0x306b, .lparam = GCS_COMPSTR|GCS_COMPCLAUSE|GCS_COMPATTR|GCS_COMPREADSTR|GCS_DELTASTART|GCS_CURSORPOS},
        },
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\u306b\u307b\uff4e", .result = L"",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0x306b, .lparam = GCS_COMPSTR|GCS_COMPCLAUSE|GCS_COMPATTR|GCS_COMPREADSTR|GCS_DELTASTART|GCS_CURSORPOS},
        },
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\u306b\u307b\u3093\uff47", .result = L"",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0x306b, .lparam = GCS_COMPSTR|GCS_COMPCLAUSE|GCS_COMPATTR|GCS_COMPREADSTR|GCS_DELTASTART|GCS_CURSORPOS},
        },
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\u306b\u307b\u3093\u3054", .result = L"",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0x306b, .lparam = GCS_COMPSTR|GCS_COMPCLAUSE|GCS_COMPATTR|GCS_COMPREADSTR|GCS_DELTASTART|GCS_CURSORPOS},
        },
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\u65e5\u672c\u8a9e", .result = L"",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0x65e5, .lparam = GCS_COMPSTR|GCS_COMPCLAUSE|GCS_COMPATTR|GCS_COMPREADSTR|GCS_DELTASTART|GCS_CURSORPOS},
        },
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"", .result = L"\u65e5\u672c\u8a9e",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0x65e5, .lparam = GCS_RESULTSTR|GCS_RESULTCLAUSE|GCS_RESULTREADSTR|GCS_RESULTREADCLAUSE},
        },
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_CHAR, .wparam = 0x65e5, .lparam = 0x1}},
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_CHAR, .wparam = 0x672c, .lparam = 0x1}},
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_CHAR, .wparam = 0x8a9e, .lparam = 0x1}},
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_ENDCOMPOSITION, .wparam = 0, .lparam = 0}},
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_STARTCOMPOSITION, .wparam = 0, .lparam = 0}},
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\uff4e", .result = L"",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0xff4e, .lparam = GCS_COMPSTR|GCS_COMPCLAUSE|GCS_COMPATTR|GCS_COMPREADSTR|GCS_DELTASTART|GCS_CURSORPOS},
        },
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"\u306e", .result = L"",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0x306e, .lparam = GCS_COMPSTR|GCS_COMPCLAUSE|GCS_COMPATTR|GCS_COMPREADSTR|GCS_DELTASTART|GCS_CURSORPOS},
        },
        {
            .hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .comp = L"", .result = L"\u306e",
            .message = {.msg = WM_IME_COMPOSITION, .wparam = 0x306e, .lparam = GCS_RESULTSTR|GCS_RESULTCLAUSE|GCS_RESULTREADSTR|GCS_RESULTREADCLAUSE},
        },
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_CHAR, .wparam = 0x306e, .lparam = 0x1}},
        {.hkl = default_hkl, .himc = 0/*himc*/, .func = MSG_TEST_WIN, .message = {.msg = WM_IME_ENDCOMPOSITION, .wparam = 0, .lparam = 0}},
        {0},
    };

    INPUTCONTEXT *ctx;
    HWND hwnd;
    HIMC himc;
    UINT i;

    /* this test doesn't work on Win32 / WoW64 */
    if (sizeof(void *) == 4 || default_hkl != (HKL)0x04110411 /* MS Japanese IME */)
    {
        skip( "Got hkl %p, skipping Japanese IME-specific test\n", default_hkl );
        return;
    }

    hwnd = CreateWindowW( test_class.lpszClassName, NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 100, 100, NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );
    flush_events();

    himc = ImmCreateContext();
    ok_ne( NULL, himc, HIMC, "%p" );
    ctx = ImmLockIMC( himc );
    ok_ne( NULL, ctx, INPUTCONTEXT *, "%p" );
    ok_eq( default_himc, ImmAssociateContext( hwnd, himc ), HIMC, "%p" );
    ok_ret( 1, ImmSetOpenStatus( himc, TRUE ) );
    ok_ret( 1, ImmSetConversionStatus( himc, IME_CMODE_FULLSHAPE | IME_CMODE_NATIVE, IME_SMODE_PHRASEPREDICT ) );
    flush_events();
    memset( ime_calls, 0, sizeof(ime_calls) );
    ime_call_count = 0;

    for (i = 0; i < ARRAY_SIZE(complete_seq); i++) complete_seq[i].himc = himc;
    ignore_WM_IME_REQUEST = TRUE;
    ignore_WM_IME_NOTIFY = TRUE;


    keybd_event( 'N', 0x31, 0, 0 );
    flush_events();
    keybd_event( 'N', 0x31, KEYEVENTF_KEYUP, 0 );

    keybd_event( 'I', 0x17, 0, 0 );
    flush_events();
    keybd_event( 'I', 0x17, KEYEVENTF_KEYUP, 0 );

    keybd_event( 'H', 0x23, 0, 0 );
    flush_events();
    keybd_event( 'H', 0x23, KEYEVENTF_KEYUP, 0 );

    keybd_event( 'O', 0x18, 0, 0 );
    flush_events();
    keybd_event( 'O', 0x18, KEYEVENTF_KEYUP, 0 );

    keybd_event( 'N', 0x31, 0, 0 );
    flush_events();
    keybd_event( 'N', 0x31, KEYEVENTF_KEYUP, 0 );

    keybd_event( 'G', 0x22, 0, 0 );
    flush_events();
    keybd_event( 'G', 0x22, KEYEVENTF_KEYUP, 0 );

    keybd_event( 'O', 0x18, 0, 0 );
    flush_events();
    keybd_event( 'O', 0x18, KEYEVENTF_KEYUP, 0 );

    keybd_event( VK_SPACE, 0x39, 0, 0 );
    flush_events();
    keybd_event( VK_SPACE, 0x39, KEYEVENTF_KEYUP, 0 );

    keybd_event( 'N', 0x31, 0, 0 );
    flush_events();
    keybd_event( 'N', 0x31, KEYEVENTF_KEYUP, 0 );

    keybd_event( 'O', 0x18, 0, 0 );
    flush_events();
    keybd_event( 'O', 0x18, KEYEVENTF_KEYUP, 0 );

    keybd_event( VK_RETURN, 0x1c, 0, 0 );
    flush_events();
    keybd_event( VK_RETURN, 0x1c, KEYEVENTF_KEYUP, 0 );

    flush_events();
    todo_wine ok_seq( complete_seq );

    ignore_WM_IME_REQUEST = FALSE;
    ignore_WM_IME_NOTIFY = FALSE;

    /* Japanese IME doesn't take input from ImmProcessKey */

    ok_ret( 0, ImmProcessKey( hwnd, default_hkl, 'N', MAKELONG(1, 0x31), 0 ) );
    ok_ret( 0, ImmTranslateMessage( hwnd, WM_KEYDOWN, 'N', MAKELONG(1, 0x31) ) );
    flush_events();
    ok_ret( 0, ImmProcessKey( hwnd, default_hkl, 'N', MAKELONG(1, 0xc031), 0 ) );
    ok_ret( 0, ImmTranslateMessage( hwnd, WM_KEYUP, 'N', MAKELONG(1, 0xc031) ) );
    flush_events();
    ok_seq( empty_sequence );


    ok_ret( 1, ImmSetConversionStatus( himc, 0, IME_SMODE_PHRASEPREDICT ) );
    ok_ret( 1, ImmSetOpenStatus( himc, FALSE ) );

    ok_ret( 1, ImmUnlockIMC( himc ) );
    ok_ret( 1, ImmDestroyContext( himc ) );

    ok_ret( 1, DestroyWindow( hwnd ) );
    process_messages();

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
    wineime_hkl = ime_install();

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

    test_ImmGetCompositionString( TRUE );
    test_ImmGetCompositionString( FALSE );
    test_ImmSetCompositionWindow();
    test_ImmSetStatusWindowPos();
    test_ImmSetCompositionFont( TRUE );
    test_ImmSetCompositionFont( FALSE );
    test_ImmSetCandidateWindow();

    test_ImmGenerateMessage();
    test_ImmTranslateMessage( FALSE );
    test_ImmTranslateMessage( TRUE );

    if (wineime_hkl) ime_cleanup( wineime_hkl, TRUE );

    test_ga_na_da();
    test_nihongo_no();

    if (init())
    {
        test_ImmNotifyIME();
        test_SCS_SETSTR();
        test_ImmIME();
        test_ImmAssociateContextEx();
        test_NtUserAssociateInputContext();
        test_cross_thread_himc();
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
