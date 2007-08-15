/* Test Key event to Key message translation
 *
 * Copyright 2003 Rein Klazes
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

/* test whether the right type of messages:
 * WM_KEYUP/DOWN vs WM_SYSKEYUP/DOWN  are sent in case of combined
 * keystrokes.
 *
 * For instance <ALT>-X can be accompished by
 * the sequence ALT-KEY-DOWN, X-KEY-DOWN, ALT-KEY-UP, X-KEY-UP
 * but also X-KEY-DOWN, ALT-KEY-DOWN, X-KEY-UP, ALT-KEY-UP
 * Whether a KEY or a SYSKEY message is sent is not always clear, it is
 * also not the same in WINNT as in WIN9X */

/* NOTE that there will be test failures under WIN9X
 * No applications are known to me that rely on this
 * so I don't fix it */

/* TODO:
 * 1. extend it to the wm_command and wm_syscommand notifications
 * 2. add some more tests with special cases like dead keys or right (alt) key
 * 3. there is some adapted code from input.c in here. Should really
 *    make that code exactly the same.
 * 4. resolve the win9x case when there is a need or the testing frame work
 *    offers a nice way.
 * 5. The test app creates a window, the user should not take the focus
 *    away during its short existence. I could do something to prevent that
 *    if it is a problem.
 *
 */

#define _WIN32_WINNT 0x401

#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"

#include "wine/test.h"

/* globals */
static HWND hWndTest;
static long timetag = 0x10000000;

static UINT (WINAPI *pSendInput) (UINT, INPUT*, size_t);

#define MAXKEYEVENTS 6
#define MAXKEYMESSAGES MAXKEYEVENTS /* assuming a key event generates one
                                       and only one message */

/* keyboard message names, sorted as their value */
static const char *MSGNAME[]={"WM_KEYDOWN", "WM_KEYUP", "WM_CHAR","WM_DEADCHAR",
    "WM_SYSKEYDOWN", "WM_SYSKEYUP", "WM_SYSCHAR", "WM_SYSDEADCHAR" ,"WM_KEYLAST"};

/* keyevents, add more as needed */
typedef enum KEVtag
{  ALTDOWN = 1, ALTUP, XDOWN, XUP, SHIFTDOWN, SHIFTUP, CTRLDOWN, CTRLUP } KEV;
/* matching VK's */
static const int GETVKEY[]={0, VK_MENU, VK_MENU, 'X', 'X', VK_SHIFT, VK_SHIFT, VK_CONTROL, VK_CONTROL};
/* matching scan codes */
static const int GETSCAN[]={0, 0x38, 0x38, 0x2D, 0x2D, 0x2A, 0x2A, 0x1D, 0x1D };
/* matching updown events */
static const int GETUPDOWN[]={0, 0, KEYEVENTF_KEYUP, 0, KEYEVENTF_KEYUP, 0, KEYEVENTF_KEYUP, 0, KEYEVENTF_KEYUP};
/* matching descripts */
static const char *getdesc[]={"", "+alt","-alt","+X","-X","+shift","-shift","+ctrl","-ctrl"};

/* The MSVC headers ignore our NONAMELESSUNION requests so we have to define our own type */
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

#define ADDTOINPUTS(kev) \
inputs[evtctr].type = INPUT_KEYBOARD; \
    ((TEST_INPUT*)inputs)[evtctr].u.ki.wVk = GETVKEY[ kev]; \
    ((TEST_INPUT*)inputs)[evtctr].u.ki.wScan = GETSCAN[ kev]; \
    ((TEST_INPUT*)inputs)[evtctr].u.ki.dwFlags = GETUPDOWN[ kev]; \
    ((TEST_INPUT*)inputs)[evtctr].u.ki.dwExtraInfo = 0; \
    ((TEST_INPUT*)inputs)[evtctr].u.ki.time = ++timetag; \
    if( kev) evtctr++;

typedef struct {
    UINT    message;
    WPARAM  wParam;
    LPARAM  lParam;
} KMSG;

/*******************************************
 * add new test sets here
 * the software will make all combinations of the
 * keyevent defined here
 */
static const struct {
    int nrkev;
    KEV keydwn[MAXKEYEVENTS];
    KEV keyup[MAXKEYEVENTS];
} testkeyset[]= {
    { 2, { ALTDOWN, XDOWN }, { ALTUP, XUP}},
    { 3, { ALTDOWN, XDOWN , SHIFTDOWN}, { ALTUP, XUP, SHIFTUP}},
    { 3, { ALTDOWN, XDOWN , CTRLDOWN}, { ALTUP, XUP, CTRLUP}},
    { 3, { SHIFTDOWN, XDOWN , CTRLDOWN}, { SHIFTUP, XUP, CTRLUP}},
    { 0 } /* mark the end */
};

/**********************adapted from input.c **********************************/

static BYTE InputKeyStateTable[256];
static BYTE AsyncKeyStateTable[256];
static BYTE TrackSysKey = 0; /* determine whether ALT key up will cause a WM_SYSKEYUP
                         or a WM_KEYUP message */
typedef union
{
    struct
    {
	unsigned long count : 16;
	unsigned long code : 8;
	unsigned long extended : 1;
	unsigned long unused : 2;
	unsigned long win_internal : 2;
	unsigned long context : 1;
	unsigned long previous : 1;
	unsigned long transition : 1;
    } lp1;
    unsigned long lp2;
} KEYLP;

static void init_function_pointers(void)
{
    HMODULE hdll = GetModuleHandleA("user32");

#define GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hdll, #func); \
    if(!p ## func) \
      trace("GetProcAddress(%s) failed\n", #func);

    GET_PROC(SendInput)

#undef GET_PROC
}

static int KbdMessage( KEV kev, WPARAM *pwParam, LPARAM *plParam )
{
    UINT message;
    int VKey = GETVKEY[kev];
    KEYLP keylp;

    keylp.lp2 = 0;

    keylp.lp1.count = 1;
    keylp.lp1.code = GETSCAN[kev];
    keylp.lp1.extended = 0 ;/*  FIXME (ki->dwFlags & KEYEVENTF_EXTENDEDKEY) != 0; */
    keylp.lp1.win_internal = 0;

    if (GETUPDOWN[kev] & KEYEVENTF_KEYUP )
    {
        message = WM_KEYUP;
        if( (InputKeyStateTable[VK_MENU] & 0x80) && (
                (VKey == VK_MENU) || (VKey == VK_CONTROL) ||
                 !(InputKeyStateTable[VK_CONTROL] & 0x80))) {
            if(  TrackSysKey == VK_MENU || /* <ALT>-down/<ALT>-up sequence */
                    (VKey != VK_MENU)) /* <ALT>-down...<something else>-up */
                message = WM_SYSKEYUP;
                TrackSysKey = 0;
        }
        InputKeyStateTable[VKey] &= ~0x80;
        keylp.lp1.previous = 1;
        keylp.lp1.transition = 1;
    }
    else
    {
        keylp.lp1.previous = (InputKeyStateTable[VKey] & 0x80) != 0;
        keylp.lp1.transition = 0;
        if (!(InputKeyStateTable[VKey] & 0x80)) InputKeyStateTable[VKey] ^= 0x01;
        InputKeyStateTable[VKey] |= 0x80;
        AsyncKeyStateTable[VKey] |= 0x80;

        message = WM_KEYDOWN;
        if( (InputKeyStateTable[VK_MENU] & 0x80) &&
                !(InputKeyStateTable[VK_CONTROL] & 0x80)) {
            message = WM_SYSKEYDOWN;
            TrackSysKey = VKey;
        }
    }

    keylp.lp1.context = (InputKeyStateTable[VK_MENU] & 0x80) != 0; /* 1 if alt */

    if( plParam) *plParam = keylp.lp2;
    if( pwParam) *pwParam = VKey;
    return message;
}

/****************************** end copy input.c ****************************/

/*
 * . prepare the keyevents for SendInputs
 * . calculate the "expected" messages
 * . Send the events to our window
 * . retrieve the messages from the input queue
 * . verify
 */
static void do_test( HWND hwnd, int seqnr, const KEV td[] )
{
    INPUT inputs[MAXKEYEVENTS];
    KMSG expmsg[MAXKEYEVENTS];
    MSG msg;
    char buf[100];
    UINT evtctr=0;
    int kmctr, i;

    buf[0]='\0';
    TrackSysKey=0; /* see input.c */
    for( i = 0; i < MAXKEYEVENTS; i++) {
        ADDTOINPUTS(td[i])
        strcat(buf, getdesc[td[i]]);
        if(td[i])
            expmsg[i].message = KbdMessage(td[i], &(expmsg[i].wParam), &(expmsg[i].lParam)); /* see queue_kbd_event() */
        else
            expmsg[i].message = 0;
    }
    for( kmctr = 0; kmctr < MAXKEYEVENTS && expmsg[kmctr].message; kmctr++)
        ;
    assert( evtctr <= MAXKEYEVENTS );
    assert( evtctr == pSendInput(evtctr, &inputs[0], sizeof(INPUT)));
    i = 0;
    if (winetest_debug > 1)
        trace("======== key stroke sequence #%d: %s =============\n",
            seqnr + 1, buf);
    while( PeekMessage(&msg,hwnd,WM_KEYFIRST,WM_KEYLAST,PM_REMOVE) ) {
        if (winetest_debug > 1)
            trace("message[%d] %-15s wParam %04lx lParam %08lx time %x\n", i,
                  MSGNAME[msg.message - WM_KEYFIRST], msg.wParam, msg.lParam, msg.time);
        if( i < kmctr ) {
            ok( msg.message == expmsg[i].message &&
                    msg.wParam == expmsg[i].wParam &&
                    msg.lParam == expmsg[i].lParam,
                    "wrong message! expected:\n"
                    "message[%d] %-15s wParam %04lx lParam %08lx\n",i,
                    MSGNAME[(expmsg[i]).message - WM_KEYFIRST],
                    expmsg[i].wParam, expmsg[i].lParam );
        }
        i++;
    }
    if (winetest_debug > 1)
        trace("%d messages retrieved\n", i);
    ok( i == kmctr, "message count is wrong: got %d expected: %d\n", i, kmctr);
}

/* test all combinations of the specified key events */
static void TestASet( HWND hWnd, int nrkev, const KEV kevdwn[], const KEV kevup[] )
{
    int i,j,k,l,m,n;
    static int count=0;
    KEV kbuf[MAXKEYEVENTS];
    assert( nrkev==2 || nrkev==3);
    for(i=0;i<MAXKEYEVENTS;i++) kbuf[i]=0;
    /* two keys involved gives 4 test cases */
    if(nrkev==2) {
        for(i=0;i<nrkev;i++) {
            for(j=0;j<nrkev;j++) {
                kbuf[0] = kevdwn[i];
                kbuf[1] = kevdwn[1-i];
                kbuf[2] = kevup[j];
                kbuf[3] = kevup[1-j];
                do_test( hWnd, count++, kbuf);
            }
        }
    }
    /* three keys involved gives 36 test cases */
    if(nrkev==3){
        for(i=0;i<nrkev;i++){
            for(j=0;j<nrkev;j++){
                if(j==i) continue;
                for(k=0;k<nrkev;k++){
                    if(k==i || k==j) continue;
                    for(l=0;l<nrkev;l++){
                        for(m=0;m<nrkev;m++){
                            if(m==l) continue;
                            for(n=0;n<nrkev;n++){
                                if(n==l ||n==m) continue;
                                kbuf[0] = kevdwn[i];
                                kbuf[1] = kevdwn[j];
                                kbuf[2] = kevdwn[k];
                                kbuf[3] = kevup[l];
                                kbuf[4] = kevup[m];
                                kbuf[5] = kevup[n];
                                do_test( hWnd, count++, kbuf);
                            }
                        }
                    }
                }
            }
        }
    }
}

/* test each set specified in the global testkeyset array */
static void TestSysKeys( HWND hWnd)
{
    int i;
    for(i=0; testkeyset[i].nrkev;i++)
        TestASet( hWnd, testkeyset[i].nrkev, testkeyset[i].keydwn,
                testkeyset[i].keyup);
}

static LRESULT CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam,
        LPARAM lParam )
{
    switch (msg) {
        case WM_USER:
            SetFocus(hWnd);
            /* window has focus, now do the test */
            if( hWnd == hWndTest) TestSysKeys( hWnd);
            /* finished :-) */
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;

        default:
            return( DefWindowProcA( hWnd, msg, wParam, lParam ) );
    }

    return 0;
}

static void test_Input_whitebox(void)
{
    MSG msg;
    WNDCLASSA  wclass;
    HANDLE hInstance = GetModuleHandleA( NULL );

    wclass.lpszClassName = "InputSysKeyTestClass";
    wclass.style         = CS_HREDRAW | CS_VREDRAW;
    wclass.lpfnWndProc   = WndProc;
    wclass.hInstance     = hInstance;
    wclass.hIcon         = LoadIconA( 0, (LPSTR)IDI_APPLICATION );
    wclass.hCursor       = LoadCursorA( NULL, IDC_ARROW);
    wclass.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1);
    wclass.lpszMenuName = 0;
    wclass.cbClsExtra    = 0;
    wclass.cbWndExtra    = 0;
    assert (RegisterClassA( &wclass ));
    /* create the test window that will receive the keystrokes */
    assert ( hWndTest = CreateWindowA( wclass.lpszClassName, "InputSysKeyTest",
                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 100, 100,
                NULL, NULL, hInstance, NULL) );
    ShowWindow( hWndTest, SW_SHOW);
    UpdateWindow( hWndTest);

    /* flush pending messages */
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    SendMessageA(hWndTest, WM_USER, 0, 0);
    DestroyWindow(hWndTest);
}

static void empty_message_queue(void) {
    MSG msg;
    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

struct transition_s {
    WORD wVk;
    BYTE before_state;
    BOOL _todo_wine;
};

struct sendinput_test_s {
    WORD wVk;
    DWORD dwFlags;
    struct transition_s expected_transitions[MAXKEYEVENTS+1];
    UINT expected_messages[MAXKEYMESSAGES+1];
} sendinput_test[] = {
    /* test ALT+F */
    {VK_LMENU, 0, {{VK_MENU, 0x00, 0}, {VK_LMENU, 0x00, 0}, {0}},
        {WM_SYSKEYDOWN, 0}},
    {'F', 0, {{'F', 0x00, 0}, {0}},
        {WM_SYSKEYDOWN, WM_SYSCHAR, WM_SYSCOMMAND, 0}},
    {'F', KEYEVENTF_KEYUP, {{'F', 0x80, 0}, {0}}, {WM_SYSKEYUP, 0}},
    {VK_LMENU, KEYEVENTF_KEYUP, {{VK_MENU, 0x80, 0}, {VK_LMENU, 0x80, 0}, {0}},
        {WM_KEYUP, 0}},

    /* test CTRL+O */
    {VK_LCONTROL, 0, {{VK_CONTROL, 0x00, 0}, {VK_LCONTROL, 0x00, 0}, {0}},
        {WM_KEYDOWN, 0}},
    {'O', 0, {{'O', 0x00, 0}, {0}}, {WM_KEYDOWN, WM_CHAR, 0}},
    {'O', KEYEVENTF_KEYUP, {{'O', 0x80, 0}, {0}}, {WM_KEYUP, 0}},
    {VK_LCONTROL, KEYEVENTF_KEYUP,
        {{VK_CONTROL, 0x80, 0}, {VK_LCONTROL, 0x80, 0}, {0}}, {WM_KEYUP, 0}},

    /* test ALT+CTRL+X */
    {VK_LMENU, 0, {{VK_MENU, 0x00, 0}, {VK_LMENU, 0x00, 0}, {0}},
        {WM_SYSKEYDOWN, 0}},
    {VK_LCONTROL, 0, {{VK_CONTROL, 0x00, 0}, {VK_LCONTROL, 0x00, 0}, {0}},
        {WM_KEYDOWN, 0}},
    {'X', 0, {{'X', 0x00, 0}, {0}}, {WM_KEYDOWN, 0}},
    {'X', KEYEVENTF_KEYUP, {{'X', 0x80, 0}, {0}}, {WM_KEYUP, 0}},
    {VK_LCONTROL, KEYEVENTF_KEYUP,
        {{VK_CONTROL, 0x80, 0}, {VK_LCONTROL, 0x80, 0}, {0}},
        {WM_SYSKEYUP, 0}},
    {VK_LMENU, KEYEVENTF_KEYUP, {{VK_MENU, 0x80, 0}, {VK_LMENU, 0x80, 0}, {0}},
        {WM_KEYUP, 0}},

    /* test SHIFT+A */
    {VK_LSHIFT, 0,
        {{VK_SHIFT, 0x00, 0}, {VK_LSHIFT, 0x00, 0}, {0}}, {WM_KEYDOWN, 0}},
    {'A', 0, {{'A', 0x00, 0}, {0}}, {WM_KEYDOWN, WM_CHAR, 0}},
    {'A', KEYEVENTF_KEYUP, {{'A', 0x80, 0}, {0}}, {WM_KEYUP, 0}},
    {VK_LSHIFT, KEYEVENTF_KEYUP,
        {{VK_SHIFT, 0x80, 0}, {VK_LSHIFT, 0x80, 0}, {0}}, {WM_KEYUP, 0}},

    {0, 0, {{0}}, {0}} /* end */
};

static struct sendinput_test_s *pTest = sendinput_test;
static UINT *pMsg = sendinput_test[0].expected_messages;

/* Verify that only specified key state transitions occur */
static void compare_and_check(int id, BYTE *ks1, BYTE *ks2, struct transition_s *t) {
    int i;
    while (t->wVk) {
        int matched = ((ks1[t->wVk]&0x80) == (t->before_state&0x80)
                       && (ks2[t->wVk]&0x80) == (~t->before_state&0x80));
        if (t->_todo_wine) {
            todo_wine {
                ok(matched, "%02d: %02x from %02x -> %02x "
                   "instead of %02x -> %02x\n", id, t->wVk,
                   ks1[t->wVk]&0x80, ks2[t->wVk]&0x80, t->before_state,
                   ~t->before_state&0x80);
            }
        } else {
            ok(matched, "%02d: %02x from %02x -> %02x "
               "instead of %02x -> %02x\n", id, t->wVk,
               ks1[t->wVk]&0x80, ks2[t->wVk]&0x80, t->before_state,
               ~t->before_state&0x80);
        }
        ks2[t->wVk] = ks1[t->wVk]; /* clear the match */
        t++;
    }
    for (i = 0; i < 256; i++)
        ok(ks2[i] == ks1[i], "%02d: %02x from %02x -> %02x unexpected\n",
           id, i, ks1[i], ks2[i]);
}

/* WndProc2 checks that we get at least the messages specified */
static LRESULT CALLBACK WndProc2(HWND hWnd, UINT Msg, WPARAM wParam,
                                   LPARAM lParam)
{
    if (pTest->wVk != 0) { /* not end */
        while(pTest->wVk != 0 && *pMsg == 0) {
            pTest++;
            pMsg = pTest->expected_messages;
        }
        if (Msg == *pMsg)
            pMsg++;
    }
    return DefWindowProc(hWnd, Msg, wParam, lParam);
}

static void test_Input_blackbox(void)
{
    TEST_INPUT i;
    int ii;
    BYTE ks1[256], ks2[256];
    LONG_PTR prevWndProc;
    HWND window;

    if (!pSendInput)
    {
        skip("SendInput is not available\n");
        return;
    }

    window = CreateWindow("Static", NULL, WS_POPUP|WS_HSCROLL|WS_VSCROLL
        |WS_VISIBLE, 0, 0, 200, 60, NULL, NULL,
        NULL, NULL);
    ok(window != NULL, "error: %d\n", (int) GetLastError());

    /* must process all initial messages, otherwise X11DRV_KeymapNotify unsets
     * key state set by SendInput(). */
    empty_message_queue();

    prevWndProc = SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR) WndProc2);
    ok(prevWndProc != 0 || (prevWndProc == 0 && GetLastError() == 0),
       "error: %d\n", (int) GetLastError());

    i.type = INPUT_KEYBOARD;
    i.u.ki.wScan = 0;
    i.u.ki.time = 0;
    i.u.ki.dwExtraInfo = 0;

    for (ii = 0; ii < sizeof(sendinput_test)/sizeof(struct sendinput_test_s)-1;
         ii++) {
        GetKeyboardState(ks1);
        i.u.ki.dwFlags = sendinput_test[ii].dwFlags;
        i.u.ki.wVk = sendinput_test[ii].wVk;
        pSendInput(1, (INPUT*)&i, sizeof(TEST_INPUT));
        empty_message_queue();
        GetKeyboardState(ks2);
        compare_and_check(ii, ks1, ks2,
                          sendinput_test[ii].expected_transitions);
    }

    /* *pMsg should be 0 and (++pTest)->wVk should be 0 */
    if (pTest->wVk && *pMsg == 0) pTest++;
    while(pTest->wVk && pTest->expected_messages[0] == 0) {
        ++pTest;
    }
    ok(*pMsg == 0 && pTest->wVk == 0,
       "not enough messages found; looking for %x\n", *pMsg);
    DestroyWindow(window);
}

static void test_keynames(void)
{
    int i, len;
    char buff[256];

    for (i = 0; i < 512; i++)
    {
        strcpy(buff, "----");
        len = GetKeyNameTextA(i << 16, buff, sizeof(buff));
        ok(len || !buff[0], "%d: Buffer is not zeroed\n", i);
    }
}

static POINT pt_old, pt_new;
static BOOL clipped;
#define STEP 20

static LRESULT CALLBACK hook_proc1( int code, WPARAM wparam, LPARAM lparam )
{
    MSLLHOOKSTRUCT *hook = (MSLLHOOKSTRUCT *)lparam;
    POINT pt, pt1;

    if (code == HC_ACTION)
    {
        /* This is our new cursor position */
        pt_new = hook->pt;
        /* Should return previous position */
        GetCursorPos(&pt);
        ok(pt.x == pt_old.x && pt.y == pt_old.y, "GetCursorPos: (%d,%d)\n", pt.x, pt.y);

        /* Should set new position until hook chain is finished. */
        pt.x = pt_old.x + STEP;
        pt.y = pt_old.y + STEP;
        SetCursorPos(pt.x, pt.y);
        GetCursorPos(&pt1);
        if (clipped)
            ok(pt1.x == pt_old.x && pt1.y == pt_old.y, "Wrong set pos: (%d,%d)\n", pt1.x, pt1.y);
        else
            ok(pt1.x == pt.x && pt1.y == pt.y, "Wrong set pos: (%d,%d)\n", pt1.x, pt1.y);
    }
    return CallNextHookEx( 0, code, wparam, lparam );
}

static LRESULT CALLBACK hook_proc2( int code, WPARAM wparam, LPARAM lparam )
{
    MSLLHOOKSTRUCT *hook = (MSLLHOOKSTRUCT *)lparam;
    POINT pt;

    if (code == HC_ACTION)
    {
        ok(hook->pt.x == pt_new.x && hook->pt.y == pt_new.y,
           "Wrong hook coords: (%d %d) != (%d,%d)\n", hook->pt.x, hook->pt.y, pt_new.x, pt_new.y);

        /* Should match position set above */
        GetCursorPos(&pt);
        if (clipped)
            ok(pt.x == pt_old.x && pt.y == pt_old.y, "GetCursorPos: (%d,%d)\n", pt.x, pt.y);
        else
            ok(pt.x == pt_old.x +STEP && pt.y == pt_old.y +STEP, "GetCursorPos: (%d,%d)\n", pt.x, pt.y);
    }
    return CallNextHookEx( 0, code, wparam, lparam );
}

static void test_mouse_ll_hook(void)
{
    HWND hwnd;
    HHOOK hook1, hook2;
    POINT pt_org, pt;
    RECT rc;

    GetCursorPos(&pt_org);
    hwnd = CreateWindow("static", "Title", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        10, 10, 200, 200, NULL, NULL, NULL, NULL);
    SetCursorPos(100, 100);

    hook2 = SetWindowsHookExA(WH_MOUSE_LL, hook_proc2, GetModuleHandleA(0), 0);
    hook1 = SetWindowsHookExA(WH_MOUSE_LL, hook_proc1, GetModuleHandleA(0), 0);

    GetCursorPos(&pt_old);
    mouse_event(MOUSEEVENTF_MOVE, -STEP,  0, 0, 0);
    GetCursorPos(&pt_old);
    ok(pt_old.x == pt_new.x && pt_old.y == pt_new.y, "Wrong new pos: (%d,%d)\n", pt_old.x, pt_old.y);
    mouse_event(MOUSEEVENTF_MOVE, +STEP,  0, 0, 0);
    GetCursorPos(&pt_old);
    ok(pt_old.x == pt_new.x && pt_old.y == pt_new.y, "Wrong new pos: (%d,%d)\n", pt_old.x, pt_old.y);
    mouse_event(MOUSEEVENTF_MOVE,  0, -STEP, 0, 0);
    GetCursorPos(&pt_old);
    ok(pt_old.x == pt_new.x && pt_old.y == pt_new.y, "Wrong new pos: (%d,%d)\n", pt_old.x, pt_old.y);
    mouse_event(MOUSEEVENTF_MOVE,  0, +STEP, 0, 0);
    GetCursorPos(&pt_old);
    ok(pt_old.x == pt_new.x && pt_old.y == pt_new.y, "Wrong new pos: (%d,%d)\n", pt_old.x, pt_old.y);

    SetRect(&rc, 50, 50, 151, 151);
    ClipCursor(&rc);
    clipped = TRUE;

    SetCursorPos(40, 40);
    GetCursorPos(&pt_old);
    ok(pt_old.x == 50 && pt_old.y == 50, "Wrong new pos: (%d,%d)\n", pt_new.x, pt_new.y);
    SetCursorPos(160, 160);
    GetCursorPos(&pt_old);
    ok(pt_old.x == 150 && pt_old.y == 150, "Wrong new pos: (%d,%d)\n", pt_new.x, pt_new.y);
    mouse_event(MOUSEEVENTF_MOVE, +STEP, +STEP, 0, 0);
    GetCursorPos(&pt_old);
    ok(pt_old.x == 150 && pt_old.y == 150, "Wrong new pos: (%d,%d)\n", pt_new.x, pt_new.y);

    clipped = FALSE;
    pt_new.x = pt_new.y = 150;
    ClipCursor(NULL);
    UnhookWindowsHookEx(hook1);

    /* Now check that mouse buttons do not change mouse position
       if we don't have MOUSEEVENTF_MOVE flag specified. */

    /* We reusing the same hook callback, so make it happy */
    pt_old.x = pt_new.x - STEP;
    pt_old.y = pt_new.y - STEP;
    mouse_event(MOUSEEVENTF_LEFTUP, 123, 456, 0, 0);
    GetCursorPos(&pt);
    ok(pt.x == pt_new.x && pt.y == pt_new.y, "Position changed: (%d,%d)\n", pt.x, pt.y);
    mouse_event(MOUSEEVENTF_RIGHTUP, 456, 123, 0, 0);
    GetCursorPos(&pt);
    ok(pt.x == pt_new.x && pt.y == pt_new.y, "Position changed: (%d,%d)\n", pt.x, pt.y);

    mouse_event(MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE, 123, 456, 0, 0);
    GetCursorPos(&pt);
    ok(pt.x == pt_new.x && pt.y == pt_new.y, "Position changed: (%d,%d)\n", pt.x, pt.y);
    mouse_event(MOUSEEVENTF_RIGHTUP | MOUSEEVENTF_ABSOLUTE, 456, 123, 0, 0);
    GetCursorPos(&pt);
    ok(pt.x == pt_new.x && pt.y == pt_new.y, "Position changed: (%d,%d)\n", pt.x, pt.y);

    UnhookWindowsHookEx(hook2);
    DestroyWindow(hwnd);
    SetCursorPos(pt_org.x, pt_org.y);
}

START_TEST(input)
{
    init_function_pointers();

    if (!pSendInput)
        skip("SendInput is not available\n");
    else
        test_Input_whitebox();

    test_Input_blackbox();
    test_keynames();
    test_mouse_ll_hook();
}
