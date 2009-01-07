/* File generated automatically from tools/winapi/tests.dat; do not edit! */
/* This file can be copied, modified and distributed without restriction. */

/*
 * Unit tests for data structure packing
 */

#define WINVER 0x0501
#define _WIN32_IE 0x0501
#define _WIN32_WINNT 0x0501

#define WINE_NOWINSOCK

#include "windows.h"

#include "wine/test.h"

/***********************************************************************
 * Compatibility macros
 */

#define DWORD_PTR UINT_PTR
#define LONG_PTR INT_PTR
#define ULONG_PTR UINT_PTR

/***********************************************************************
 * Windows API extension
 */

#if defined(_MSC_VER) && (_MSC_VER >= 1300) && defined(__cplusplus)
# define _TYPE_ALIGNMENT(type) __alignof(type)
#elif defined(__GNUC__)
# define _TYPE_ALIGNMENT(type) __alignof__(type)
#else
/*
 * FIXME: May not be possible without a compiler extension
 *        (if type is not just a name that is, otherwise the normal
 *         TYPE_ALIGNMENT can be used)
 */
#endif

#if defined(TYPE_ALIGNMENT) && defined(_MSC_VER) && _MSC_VER >= 800 && !defined(__cplusplus)
#pragma warning(disable:4116)
#endif

#if !defined(TYPE_ALIGNMENT) && defined(_TYPE_ALIGNMENT)
# define TYPE_ALIGNMENT _TYPE_ALIGNMENT
#endif

/***********************************************************************
 * Test helper macros
 */

#ifdef FIELD_ALIGNMENT
# define TEST_FIELD_ALIGNMENT(type, field, align) \
   ok(_TYPE_ALIGNMENT(((type*)0)->field) == align, \
       "FIELD_ALIGNMENT(" #type ", " #field ") == %d (expected " #align ")\n", \
           (int)_TYPE_ALIGNMENT(((type*)0)->field))
#else
# define TEST_FIELD_ALIGNMENT(type, field, align) do { } while (0)
#endif

#define TEST_FIELD_OFFSET(type, field, offset) \
    ok(FIELD_OFFSET(type, field) == offset, \
        "FIELD_OFFSET(" #type ", " #field ") == %ld (expected " #offset ")\n", \
             (long int)FIELD_OFFSET(type, field))

#ifdef _TYPE_ALIGNMENT
#define TEST__TYPE_ALIGNMENT(type, align) \
    ok(_TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")\n", (int)_TYPE_ALIGNMENT(type))
#else
# define TEST__TYPE_ALIGNMENT(type, align) do { } while (0)
#endif

#ifdef TYPE_ALIGNMENT
#define TEST_TYPE_ALIGNMENT(type, align) \
    ok(TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")\n", (int)TYPE_ALIGNMENT(type))
#else
# define TEST_TYPE_ALIGNMENT(type, align) do { } while (0)
#endif

#define TEST_TYPE_SIZE(type, size) \
    ok(sizeof(type) == size, "sizeof(" #type ") == %d (expected " #size ")\n", ((int) sizeof(type)))

/***********************************************************************
 * Test macros
 */

#define TEST_FIELD(type, field, field_offset, field_size, field_align) \
  TEST_TYPE_SIZE((((type*)0)->field), field_size); \
  TEST_FIELD_ALIGNMENT(type, field, field_align); \
  TEST_FIELD_OFFSET(type, field, field_offset)

#define TEST_TYPE(type, size, align) \
  TEST_TYPE_ALIGNMENT(type, align); \
  TEST_TYPE_SIZE(type, size)

#define TEST_TYPE_POINTER(type, size, align) \
    TEST__TYPE_ALIGNMENT(*(type)0, align); \
    TEST_TYPE_SIZE(*(type)0, size)

#define TEST_TYPE_SIGNED(type) \
    ok((type) -1 < 0, "(" #type ") -1 < 0\n");

#define TEST_TYPE_UNSIGNED(type) \
     ok((type) -1 > 0, "(" #type ") -1 > 0\n");

static void test_pack_ACCESSTIMEOUT(void)
{
    /* ACCESSTIMEOUT (pack 4) */
    TEST_TYPE(ACCESSTIMEOUT, 12, 4);
    TEST_FIELD(ACCESSTIMEOUT, cbSize, 0, 4, 4);
    TEST_FIELD(ACCESSTIMEOUT, dwFlags, 4, 4, 4);
    TEST_FIELD(ACCESSTIMEOUT, iTimeOutMSec, 8, 4, 4);
}

static void test_pack_ANIMATIONINFO(void)
{
    /* ANIMATIONINFO (pack 4) */
    TEST_TYPE(ANIMATIONINFO, 8, 4);
    TEST_FIELD(ANIMATIONINFO, cbSize, 0, 4, 4);
    TEST_FIELD(ANIMATIONINFO, iMinAnimate, 4, 4, 4);
}

static void test_pack_CBTACTIVATESTRUCT(void)
{
    /* CBTACTIVATESTRUCT (pack 4) */
    TEST_TYPE(CBTACTIVATESTRUCT, 8, 4);
    TEST_FIELD(CBTACTIVATESTRUCT, fMouse, 0, 4, 4);
    TEST_FIELD(CBTACTIVATESTRUCT, hWndActive, 4, 4, 4);
}

static void test_pack_CBT_CREATEWNDA(void)
{
    /* CBT_CREATEWNDA (pack 4) */
    TEST_TYPE(CBT_CREATEWNDA, 8, 4);
    TEST_FIELD(CBT_CREATEWNDA, lpcs, 0, 4, 4);
    TEST_FIELD(CBT_CREATEWNDA, hwndInsertAfter, 4, 4, 4);
}

static void test_pack_CBT_CREATEWNDW(void)
{
    /* CBT_CREATEWNDW (pack 4) */
    TEST_TYPE(CBT_CREATEWNDW, 8, 4);
    TEST_FIELD(CBT_CREATEWNDW, lpcs, 0, 4, 4);
    TEST_FIELD(CBT_CREATEWNDW, hwndInsertAfter, 4, 4, 4);
}

static void test_pack_CLIENTCREATESTRUCT(void)
{
    /* CLIENTCREATESTRUCT (pack 4) */
    TEST_TYPE(CLIENTCREATESTRUCT, 8, 4);
    TEST_FIELD(CLIENTCREATESTRUCT, hWindowMenu, 0, 4, 4);
    TEST_FIELD(CLIENTCREATESTRUCT, idFirstChild, 4, 4, 4);
}

static void test_pack_COMBOBOXINFO(void)
{
    /* COMBOBOXINFO (pack 4) */
    TEST_TYPE(COMBOBOXINFO, 52, 4);
    TEST_FIELD(COMBOBOXINFO, cbSize, 0, 4, 4);
    TEST_FIELD(COMBOBOXINFO, rcItem, 4, 16, 4);
    TEST_FIELD(COMBOBOXINFO, rcButton, 20, 16, 4);
    TEST_FIELD(COMBOBOXINFO, stateButton, 36, 4, 4);
    TEST_FIELD(COMBOBOXINFO, hwndCombo, 40, 4, 4);
    TEST_FIELD(COMBOBOXINFO, hwndItem, 44, 4, 4);
    TEST_FIELD(COMBOBOXINFO, hwndList, 48, 4, 4);
}

static void test_pack_COMPAREITEMSTRUCT(void)
{
    /* COMPAREITEMSTRUCT (pack 4) */
    TEST_TYPE(COMPAREITEMSTRUCT, 32, 4);
    TEST_FIELD(COMPAREITEMSTRUCT, CtlType, 0, 4, 4);
    TEST_FIELD(COMPAREITEMSTRUCT, CtlID, 4, 4, 4);
    TEST_FIELD(COMPAREITEMSTRUCT, hwndItem, 8, 4, 4);
    TEST_FIELD(COMPAREITEMSTRUCT, itemID1, 12, 4, 4);
    TEST_FIELD(COMPAREITEMSTRUCT, itemData1, 16, 4, 4);
    TEST_FIELD(COMPAREITEMSTRUCT, itemID2, 20, 4, 4);
    TEST_FIELD(COMPAREITEMSTRUCT, itemData2, 24, 4, 4);
    TEST_FIELD(COMPAREITEMSTRUCT, dwLocaleId, 28, 4, 4);
}

static void test_pack_COPYDATASTRUCT(void)
{
    /* COPYDATASTRUCT (pack 4) */
    TEST_TYPE(COPYDATASTRUCT, 12, 4);
    TEST_FIELD(COPYDATASTRUCT, dwData, 0, 4, 4);
    TEST_FIELD(COPYDATASTRUCT, cbData, 4, 4, 4);
    TEST_FIELD(COPYDATASTRUCT, lpData, 8, 4, 4);
}

static void test_pack_CREATESTRUCTA(void)
{
    /* CREATESTRUCTA (pack 4) */
    TEST_TYPE(CREATESTRUCTA, 48, 4);
    TEST_FIELD(CREATESTRUCTA, lpCreateParams, 0, 4, 4);
    TEST_FIELD(CREATESTRUCTA, hInstance, 4, 4, 4);
    TEST_FIELD(CREATESTRUCTA, hMenu, 8, 4, 4);
    TEST_FIELD(CREATESTRUCTA, hwndParent, 12, 4, 4);
    TEST_FIELD(CREATESTRUCTA, cy, 16, 4, 4);
    TEST_FIELD(CREATESTRUCTA, cx, 20, 4, 4);
    TEST_FIELD(CREATESTRUCTA, y, 24, 4, 4);
    TEST_FIELD(CREATESTRUCTA, x, 28, 4, 4);
    TEST_FIELD(CREATESTRUCTA, style, 32, 4, 4);
    TEST_FIELD(CREATESTRUCTA, lpszName, 36, 4, 4);
    TEST_FIELD(CREATESTRUCTA, lpszClass, 40, 4, 4);
    TEST_FIELD(CREATESTRUCTA, dwExStyle, 44, 4, 4);
}

static void test_pack_CREATESTRUCTW(void)
{
    /* CREATESTRUCTW (pack 4) */
    TEST_TYPE(CREATESTRUCTW, 48, 4);
    TEST_FIELD(CREATESTRUCTW, lpCreateParams, 0, 4, 4);
    TEST_FIELD(CREATESTRUCTW, hInstance, 4, 4, 4);
    TEST_FIELD(CREATESTRUCTW, hMenu, 8, 4, 4);
    TEST_FIELD(CREATESTRUCTW, hwndParent, 12, 4, 4);
    TEST_FIELD(CREATESTRUCTW, cy, 16, 4, 4);
    TEST_FIELD(CREATESTRUCTW, cx, 20, 4, 4);
    TEST_FIELD(CREATESTRUCTW, y, 24, 4, 4);
    TEST_FIELD(CREATESTRUCTW, x, 28, 4, 4);
    TEST_FIELD(CREATESTRUCTW, style, 32, 4, 4);
    TEST_FIELD(CREATESTRUCTW, lpszName, 36, 4, 4);
    TEST_FIELD(CREATESTRUCTW, lpszClass, 40, 4, 4);
    TEST_FIELD(CREATESTRUCTW, dwExStyle, 44, 4, 4);
}

static void test_pack_CURSORINFO(void)
{
    /* CURSORINFO (pack 4) */
    TEST_TYPE(CURSORINFO, 20, 4);
    TEST_FIELD(CURSORINFO, cbSize, 0, 4, 4);
    TEST_FIELD(CURSORINFO, flags, 4, 4, 4);
    TEST_FIELD(CURSORINFO, hCursor, 8, 4, 4);
    TEST_FIELD(CURSORINFO, ptScreenPos, 12, 8, 4);
}

static void test_pack_CWPRETSTRUCT(void)
{
    /* CWPRETSTRUCT (pack 4) */
    TEST_TYPE(CWPRETSTRUCT, 20, 4);
    TEST_FIELD(CWPRETSTRUCT, lResult, 0, 4, 4);
    TEST_FIELD(CWPRETSTRUCT, lParam, 4, 4, 4);
    TEST_FIELD(CWPRETSTRUCT, wParam, 8, 4, 4);
    TEST_FIELD(CWPRETSTRUCT, message, 12, 4, 4);
    TEST_FIELD(CWPRETSTRUCT, hwnd, 16, 4, 4);
}

static void test_pack_CWPSTRUCT(void)
{
    /* CWPSTRUCT (pack 4) */
    TEST_TYPE(CWPSTRUCT, 16, 4);
    TEST_FIELD(CWPSTRUCT, lParam, 0, 4, 4);
    TEST_FIELD(CWPSTRUCT, wParam, 4, 4, 4);
    TEST_FIELD(CWPSTRUCT, message, 8, 4, 4);
    TEST_FIELD(CWPSTRUCT, hwnd, 12, 4, 4);
}

static void test_pack_DEBUGHOOKINFO(void)
{
    /* DEBUGHOOKINFO (pack 4) */
    TEST_TYPE(DEBUGHOOKINFO, 20, 4);
    TEST_FIELD(DEBUGHOOKINFO, idThread, 0, 4, 4);
    TEST_FIELD(DEBUGHOOKINFO, idThreadInstaller, 4, 4, 4);
    TEST_FIELD(DEBUGHOOKINFO, lParam, 8, 4, 4);
    TEST_FIELD(DEBUGHOOKINFO, wParam, 12, 4, 4);
    TEST_FIELD(DEBUGHOOKINFO, code, 16, 4, 4);
}

static void test_pack_DELETEITEMSTRUCT(void)
{
    /* DELETEITEMSTRUCT (pack 4) */
    TEST_TYPE(DELETEITEMSTRUCT, 20, 4);
    TEST_FIELD(DELETEITEMSTRUCT, CtlType, 0, 4, 4);
    TEST_FIELD(DELETEITEMSTRUCT, CtlID, 4, 4, 4);
    TEST_FIELD(DELETEITEMSTRUCT, itemID, 8, 4, 4);
    TEST_FIELD(DELETEITEMSTRUCT, hwndItem, 12, 4, 4);
    TEST_FIELD(DELETEITEMSTRUCT, itemData, 16, 4, 4);
}

static void test_pack_DESKTOPENUMPROCA(void)
{
    /* DESKTOPENUMPROCA */
    TEST_TYPE(DESKTOPENUMPROCA, 4, 4);
}

static void test_pack_DESKTOPENUMPROCW(void)
{
    /* DESKTOPENUMPROCW */
    TEST_TYPE(DESKTOPENUMPROCW, 4, 4);
}

static void test_pack_DLGITEMTEMPLATE(void)
{
    /* DLGITEMTEMPLATE (pack 2) */
    TEST_TYPE(DLGITEMTEMPLATE, 18, 2);
    TEST_FIELD(DLGITEMTEMPLATE, style, 0, 4, 2);
    TEST_FIELD(DLGITEMTEMPLATE, dwExtendedStyle, 4, 4, 2);
    TEST_FIELD(DLGITEMTEMPLATE, x, 8, 2, 2);
    TEST_FIELD(DLGITEMTEMPLATE, y, 10, 2, 2);
    TEST_FIELD(DLGITEMTEMPLATE, cx, 12, 2, 2);
    TEST_FIELD(DLGITEMTEMPLATE, cy, 14, 2, 2);
    TEST_FIELD(DLGITEMTEMPLATE, id, 16, 2, 2);
}

static void test_pack_DLGPROC(void)
{
    /* DLGPROC */
    TEST_TYPE(DLGPROC, 4, 4);
}

static void test_pack_DLGTEMPLATE(void)
{
    /* DLGTEMPLATE (pack 2) */
    TEST_TYPE(DLGTEMPLATE, 18, 2);
    TEST_FIELD(DLGTEMPLATE, style, 0, 4, 2);
    TEST_FIELD(DLGTEMPLATE, dwExtendedStyle, 4, 4, 2);
    TEST_FIELD(DLGTEMPLATE, cdit, 8, 2, 2);
    TEST_FIELD(DLGTEMPLATE, x, 10, 2, 2);
    TEST_FIELD(DLGTEMPLATE, y, 12, 2, 2);
    TEST_FIELD(DLGTEMPLATE, cx, 14, 2, 2);
    TEST_FIELD(DLGTEMPLATE, cy, 16, 2, 2);
}

static void test_pack_DRAWITEMSTRUCT(void)
{
    /* DRAWITEMSTRUCT (pack 4) */
    TEST_TYPE(DRAWITEMSTRUCT, 48, 4);
    TEST_FIELD(DRAWITEMSTRUCT, CtlType, 0, 4, 4);
    TEST_FIELD(DRAWITEMSTRUCT, CtlID, 4, 4, 4);
    TEST_FIELD(DRAWITEMSTRUCT, itemID, 8, 4, 4);
    TEST_FIELD(DRAWITEMSTRUCT, itemAction, 12, 4, 4);
    TEST_FIELD(DRAWITEMSTRUCT, itemState, 16, 4, 4);
    TEST_FIELD(DRAWITEMSTRUCT, hwndItem, 20, 4, 4);
    TEST_FIELD(DRAWITEMSTRUCT, hDC, 24, 4, 4);
    TEST_FIELD(DRAWITEMSTRUCT, rcItem, 28, 16, 4);
    TEST_FIELD(DRAWITEMSTRUCT, itemData, 44, 4, 4);
}

static void test_pack_DRAWSTATEPROC(void)
{
    /* DRAWSTATEPROC */
    TEST_TYPE(DRAWSTATEPROC, 4, 4);
}

static void test_pack_DRAWTEXTPARAMS(void)
{
    /* DRAWTEXTPARAMS (pack 4) */
    TEST_TYPE(DRAWTEXTPARAMS, 20, 4);
    TEST_FIELD(DRAWTEXTPARAMS, cbSize, 0, 4, 4);
    TEST_FIELD(DRAWTEXTPARAMS, iTabLength, 4, 4, 4);
    TEST_FIELD(DRAWTEXTPARAMS, iLeftMargin, 8, 4, 4);
    TEST_FIELD(DRAWTEXTPARAMS, iRightMargin, 12, 4, 4);
    TEST_FIELD(DRAWTEXTPARAMS, uiLengthDrawn, 16, 4, 4);
}

static void test_pack_EDITWORDBREAKPROCA(void)
{
    /* EDITWORDBREAKPROCA */
    TEST_TYPE(EDITWORDBREAKPROCA, 4, 4);
}

static void test_pack_EDITWORDBREAKPROCW(void)
{
    /* EDITWORDBREAKPROCW */
    TEST_TYPE(EDITWORDBREAKPROCW, 4, 4);
}

static void test_pack_EVENTMSG(void)
{
    /* EVENTMSG (pack 4) */
    TEST_TYPE(EVENTMSG, 20, 4);
    TEST_FIELD(EVENTMSG, message, 0, 4, 4);
    TEST_FIELD(EVENTMSG, paramL, 4, 4, 4);
    TEST_FIELD(EVENTMSG, paramH, 8, 4, 4);
    TEST_FIELD(EVENTMSG, time, 12, 4, 4);
    TEST_FIELD(EVENTMSG, hwnd, 16, 4, 4);
}

static void test_pack_FILTERKEYS(void)
{
    /* FILTERKEYS (pack 4) */
    TEST_TYPE(FILTERKEYS, 24, 4);
    TEST_FIELD(FILTERKEYS, cbSize, 0, 4, 4);
    TEST_FIELD(FILTERKEYS, dwFlags, 4, 4, 4);
    TEST_FIELD(FILTERKEYS, iWaitMSec, 8, 4, 4);
    TEST_FIELD(FILTERKEYS, iDelayMSec, 12, 4, 4);
    TEST_FIELD(FILTERKEYS, iRepeatMSec, 16, 4, 4);
    TEST_FIELD(FILTERKEYS, iBounceMSec, 20, 4, 4);
}

static void test_pack_FLASHWINFO(void)
{
    /* FLASHWINFO (pack 4) */
    TEST_TYPE(FLASHWINFO, 20, 4);
    TEST_FIELD(FLASHWINFO, cbSize, 0, 4, 4);
    TEST_FIELD(FLASHWINFO, hwnd, 4, 4, 4);
    TEST_FIELD(FLASHWINFO, dwFlags, 8, 4, 4);
    TEST_FIELD(FLASHWINFO, uCount, 12, 4, 4);
    TEST_FIELD(FLASHWINFO, dwTimeout, 16, 4, 4);
}

static void test_pack_GRAYSTRINGPROC(void)
{
    /* GRAYSTRINGPROC */
    TEST_TYPE(GRAYSTRINGPROC, 4, 4);
}

static void test_pack_GUITHREADINFO(void)
{
    /* GUITHREADINFO (pack 4) */
    TEST_TYPE(GUITHREADINFO, 48, 4);
    TEST_FIELD(GUITHREADINFO, cbSize, 0, 4, 4);
    TEST_FIELD(GUITHREADINFO, flags, 4, 4, 4);
    TEST_FIELD(GUITHREADINFO, hwndActive, 8, 4, 4);
    TEST_FIELD(GUITHREADINFO, hwndFocus, 12, 4, 4);
    TEST_FIELD(GUITHREADINFO, hwndCapture, 16, 4, 4);
    TEST_FIELD(GUITHREADINFO, hwndMenuOwner, 20, 4, 4);
    TEST_FIELD(GUITHREADINFO, hwndMoveSize, 24, 4, 4);
    TEST_FIELD(GUITHREADINFO, hwndCaret, 28, 4, 4);
    TEST_FIELD(GUITHREADINFO, rcCaret, 32, 16, 4);
}

static void test_pack_HARDWAREHOOKSTRUCT(void)
{
    /* HARDWAREHOOKSTRUCT (pack 4) */
    TEST_TYPE(HARDWAREHOOKSTRUCT, 16, 4);
    TEST_FIELD(HARDWAREHOOKSTRUCT, hwnd, 0, 4, 4);
    TEST_FIELD(HARDWAREHOOKSTRUCT, message, 4, 4, 4);
    TEST_FIELD(HARDWAREHOOKSTRUCT, wParam, 8, 4, 4);
    TEST_FIELD(HARDWAREHOOKSTRUCT, lParam, 12, 4, 4);
}

static void test_pack_HARDWAREINPUT(void)
{
    /* HARDWAREINPUT (pack 4) */
    TEST_TYPE(HARDWAREINPUT, 8, 4);
    TEST_FIELD(HARDWAREINPUT, uMsg, 0, 4, 4);
    TEST_FIELD(HARDWAREINPUT, wParamL, 4, 2, 2);
    TEST_FIELD(HARDWAREINPUT, wParamH, 6, 2, 2);
}

static void test_pack_HDEVNOTIFY(void)
{
    /* HDEVNOTIFY */
    TEST_TYPE(HDEVNOTIFY, 4, 4);
}

static void test_pack_HDWP(void)
{
    /* HDWP */
    TEST_TYPE(HDWP, 4, 4);
}

static void test_pack_HELPINFO(void)
{
    /* HELPINFO (pack 4) */
    TEST_TYPE(HELPINFO, 28, 4);
    TEST_FIELD(HELPINFO, cbSize, 0, 4, 4);
    TEST_FIELD(HELPINFO, iContextType, 4, 4, 4);
    TEST_FIELD(HELPINFO, iCtrlId, 8, 4, 4);
    TEST_FIELD(HELPINFO, hItemHandle, 12, 4, 4);
    TEST_FIELD(HELPINFO, dwContextId, 16, 4, 4);
    TEST_FIELD(HELPINFO, MousePos, 20, 8, 4);
}

static void test_pack_HELPWININFOA(void)
{
    /* HELPWININFOA (pack 4) */
    TEST_TYPE(HELPWININFOA, 28, 4);
    TEST_FIELD(HELPWININFOA, wStructSize, 0, 4, 4);
    TEST_FIELD(HELPWININFOA, x, 4, 4, 4);
    TEST_FIELD(HELPWININFOA, y, 8, 4, 4);
    TEST_FIELD(HELPWININFOA, dx, 12, 4, 4);
    TEST_FIELD(HELPWININFOA, dy, 16, 4, 4);
    TEST_FIELD(HELPWININFOA, wMax, 20, 4, 4);
    TEST_FIELD(HELPWININFOA, rgchMember, 24, 2, 1);
}

static void test_pack_HELPWININFOW(void)
{
    /* HELPWININFOW (pack 4) */
    TEST_TYPE(HELPWININFOW, 28, 4);
    TEST_FIELD(HELPWININFOW, wStructSize, 0, 4, 4);
    TEST_FIELD(HELPWININFOW, x, 4, 4, 4);
    TEST_FIELD(HELPWININFOW, y, 8, 4, 4);
    TEST_FIELD(HELPWININFOW, dx, 12, 4, 4);
    TEST_FIELD(HELPWININFOW, dy, 16, 4, 4);
    TEST_FIELD(HELPWININFOW, wMax, 20, 4, 4);
    TEST_FIELD(HELPWININFOW, rgchMember, 24, 4, 2);
}

static void test_pack_HIGHCONTRASTA(void)
{
    /* HIGHCONTRASTA (pack 4) */
    TEST_TYPE(HIGHCONTRASTA, 12, 4);
    TEST_FIELD(HIGHCONTRASTA, cbSize, 0, 4, 4);
    TEST_FIELD(HIGHCONTRASTA, dwFlags, 4, 4, 4);
    TEST_FIELD(HIGHCONTRASTA, lpszDefaultScheme, 8, 4, 4);
}

static void test_pack_HIGHCONTRASTW(void)
{
    /* HIGHCONTRASTW (pack 4) */
    TEST_TYPE(HIGHCONTRASTW, 12, 4);
    TEST_FIELD(HIGHCONTRASTW, cbSize, 0, 4, 4);
    TEST_FIELD(HIGHCONTRASTW, dwFlags, 4, 4, 4);
    TEST_FIELD(HIGHCONTRASTW, lpszDefaultScheme, 8, 4, 4);
}

static void test_pack_HOOKPROC(void)
{
    /* HOOKPROC */
    TEST_TYPE(HOOKPROC, 4, 4);
}

static void test_pack_ICONINFO(void)
{
    /* ICONINFO (pack 4) */
    TEST_TYPE(ICONINFO, 20, 4);
    TEST_FIELD(ICONINFO, fIcon, 0, 4, 4);
    TEST_FIELD(ICONINFO, xHotspot, 4, 4, 4);
    TEST_FIELD(ICONINFO, yHotspot, 8, 4, 4);
    TEST_FIELD(ICONINFO, hbmMask, 12, 4, 4);
    TEST_FIELD(ICONINFO, hbmColor, 16, 4, 4);
}

static void test_pack_ICONMETRICSA(void)
{
    /* ICONMETRICSA (pack 4) */
    TEST_TYPE(ICONMETRICSA, 76, 4);
    TEST_FIELD(ICONMETRICSA, cbSize, 0, 4, 4);
    TEST_FIELD(ICONMETRICSA, iHorzSpacing, 4, 4, 4);
    TEST_FIELD(ICONMETRICSA, iVertSpacing, 8, 4, 4);
    TEST_FIELD(ICONMETRICSA, iTitleWrap, 12, 4, 4);
    TEST_FIELD(ICONMETRICSA, lfFont, 16, 60, 4);
}

static void test_pack_ICONMETRICSW(void)
{
    /* ICONMETRICSW (pack 4) */
    TEST_TYPE(ICONMETRICSW, 108, 4);
    TEST_FIELD(ICONMETRICSW, cbSize, 0, 4, 4);
    TEST_FIELD(ICONMETRICSW, iHorzSpacing, 4, 4, 4);
    TEST_FIELD(ICONMETRICSW, iVertSpacing, 8, 4, 4);
    TEST_FIELD(ICONMETRICSW, iTitleWrap, 12, 4, 4);
    TEST_FIELD(ICONMETRICSW, lfFont, 16, 92, 4);
}

static void test_pack_INPUT(void)
{
    /* INPUT (pack 4) */
    TEST_FIELD(INPUT, type, 0, 4, 4);
}

static void test_pack_KBDLLHOOKSTRUCT(void)
{
    /* KBDLLHOOKSTRUCT (pack 4) */
    TEST_TYPE(KBDLLHOOKSTRUCT, 20, 4);
    TEST_FIELD(KBDLLHOOKSTRUCT, vkCode, 0, 4, 4);
    TEST_FIELD(KBDLLHOOKSTRUCT, scanCode, 4, 4, 4);
    TEST_FIELD(KBDLLHOOKSTRUCT, flags, 8, 4, 4);
    TEST_FIELD(KBDLLHOOKSTRUCT, time, 12, 4, 4);
    TEST_FIELD(KBDLLHOOKSTRUCT, dwExtraInfo, 16, 4, 4);
}

static void test_pack_KEYBDINPUT(void)
{
    /* KEYBDINPUT (pack 4) */
    TEST_TYPE(KEYBDINPUT, 16, 4);
    TEST_FIELD(KEYBDINPUT, wVk, 0, 2, 2);
    TEST_FIELD(KEYBDINPUT, wScan, 2, 2, 2);
    TEST_FIELD(KEYBDINPUT, dwFlags, 4, 4, 4);
    TEST_FIELD(KEYBDINPUT, time, 8, 4, 4);
    TEST_FIELD(KEYBDINPUT, dwExtraInfo, 12, 4, 4);
}

static void test_pack_LPACCESSTIMEOUT(void)
{
    /* LPACCESSTIMEOUT */
    TEST_TYPE(LPACCESSTIMEOUT, 4, 4);
    TEST_TYPE_POINTER(LPACCESSTIMEOUT, 12, 4);
}

static void test_pack_LPANIMATIONINFO(void)
{
    /* LPANIMATIONINFO */
    TEST_TYPE(LPANIMATIONINFO, 4, 4);
    TEST_TYPE_POINTER(LPANIMATIONINFO, 8, 4);
}

static void test_pack_LPCBTACTIVATESTRUCT(void)
{
    /* LPCBTACTIVATESTRUCT */
    TEST_TYPE(LPCBTACTIVATESTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPCBTACTIVATESTRUCT, 8, 4);
}

static void test_pack_LPCBT_CREATEWNDA(void)
{
    /* LPCBT_CREATEWNDA */
    TEST_TYPE(LPCBT_CREATEWNDA, 4, 4);
    TEST_TYPE_POINTER(LPCBT_CREATEWNDA, 8, 4);
}

static void test_pack_LPCBT_CREATEWNDW(void)
{
    /* LPCBT_CREATEWNDW */
    TEST_TYPE(LPCBT_CREATEWNDW, 4, 4);
    TEST_TYPE_POINTER(LPCBT_CREATEWNDW, 8, 4);
}

static void test_pack_LPCDLGTEMPLATEA(void)
{
    /* LPCDLGTEMPLATEA */
    TEST_TYPE(LPCDLGTEMPLATEA, 4, 4);
    TEST_TYPE_POINTER(LPCDLGTEMPLATEA, 18, 2);
}

static void test_pack_LPCDLGTEMPLATEW(void)
{
    /* LPCDLGTEMPLATEW */
    TEST_TYPE(LPCDLGTEMPLATEW, 4, 4);
    TEST_TYPE_POINTER(LPCDLGTEMPLATEW, 18, 2);
}

static void test_pack_LPCLIENTCREATESTRUCT(void)
{
    /* LPCLIENTCREATESTRUCT */
    TEST_TYPE(LPCLIENTCREATESTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPCLIENTCREATESTRUCT, 8, 4);
}

static void test_pack_LPCMENUINFO(void)
{
    /* LPCMENUINFO */
    TEST_TYPE(LPCMENUINFO, 4, 4);
    TEST_TYPE_POINTER(LPCMENUINFO, 28, 4);
}

static void test_pack_LPCMENUITEMINFOA(void)
{
    /* LPCMENUITEMINFOA */
    TEST_TYPE(LPCMENUITEMINFOA, 4, 4);
    TEST_TYPE_POINTER(LPCMENUITEMINFOA, 48, 4);
}

static void test_pack_LPCMENUITEMINFOW(void)
{
    /* LPCMENUITEMINFOW */
    TEST_TYPE(LPCMENUITEMINFOW, 4, 4);
    TEST_TYPE_POINTER(LPCMENUITEMINFOW, 48, 4);
}

static void test_pack_LPCOMBOBOXINFO(void)
{
    /* LPCOMBOBOXINFO */
    TEST_TYPE(LPCOMBOBOXINFO, 4, 4);
    TEST_TYPE_POINTER(LPCOMBOBOXINFO, 52, 4);
}

static void test_pack_LPCOMPAREITEMSTRUCT(void)
{
    /* LPCOMPAREITEMSTRUCT */
    TEST_TYPE(LPCOMPAREITEMSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPCOMPAREITEMSTRUCT, 32, 4);
}

static void test_pack_LPCREATESTRUCTA(void)
{
    /* LPCREATESTRUCTA */
    TEST_TYPE(LPCREATESTRUCTA, 4, 4);
    TEST_TYPE_POINTER(LPCREATESTRUCTA, 48, 4);
}

static void test_pack_LPCREATESTRUCTW(void)
{
    /* LPCREATESTRUCTW */
    TEST_TYPE(LPCREATESTRUCTW, 4, 4);
    TEST_TYPE_POINTER(LPCREATESTRUCTW, 48, 4);
}

static void test_pack_LPCSCROLLINFO(void)
{
    /* LPCSCROLLINFO */
    TEST_TYPE(LPCSCROLLINFO, 4, 4);
    TEST_TYPE_POINTER(LPCSCROLLINFO, 28, 4);
}

static void test_pack_LPCURSORINFO(void)
{
    /* LPCURSORINFO */
    TEST_TYPE(LPCURSORINFO, 4, 4);
    TEST_TYPE_POINTER(LPCURSORINFO, 20, 4);
}

static void test_pack_LPCWPRETSTRUCT(void)
{
    /* LPCWPRETSTRUCT */
    TEST_TYPE(LPCWPRETSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPCWPRETSTRUCT, 20, 4);
}

static void test_pack_LPCWPSTRUCT(void)
{
    /* LPCWPSTRUCT */
    TEST_TYPE(LPCWPSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPCWPSTRUCT, 16, 4);
}

static void test_pack_LPDEBUGHOOKINFO(void)
{
    /* LPDEBUGHOOKINFO */
    TEST_TYPE(LPDEBUGHOOKINFO, 4, 4);
    TEST_TYPE_POINTER(LPDEBUGHOOKINFO, 20, 4);
}

static void test_pack_LPDELETEITEMSTRUCT(void)
{
    /* LPDELETEITEMSTRUCT */
    TEST_TYPE(LPDELETEITEMSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPDELETEITEMSTRUCT, 20, 4);
}

static void test_pack_LPDLGITEMTEMPLATEA(void)
{
    /* LPDLGITEMTEMPLATEA */
    TEST_TYPE(LPDLGITEMTEMPLATEA, 4, 4);
    TEST_TYPE_POINTER(LPDLGITEMTEMPLATEA, 18, 2);
}

static void test_pack_LPDLGITEMTEMPLATEW(void)
{
    /* LPDLGITEMTEMPLATEW */
    TEST_TYPE(LPDLGITEMTEMPLATEW, 4, 4);
    TEST_TYPE_POINTER(LPDLGITEMTEMPLATEW, 18, 2);
}

static void test_pack_LPDLGTEMPLATEA(void)
{
    /* LPDLGTEMPLATEA */
    TEST_TYPE(LPDLGTEMPLATEA, 4, 4);
    TEST_TYPE_POINTER(LPDLGTEMPLATEA, 18, 2);
}

static void test_pack_LPDLGTEMPLATEW(void)
{
    /* LPDLGTEMPLATEW */
    TEST_TYPE(LPDLGTEMPLATEW, 4, 4);
    TEST_TYPE_POINTER(LPDLGTEMPLATEW, 18, 2);
}

static void test_pack_LPDRAWITEMSTRUCT(void)
{
    /* LPDRAWITEMSTRUCT */
    TEST_TYPE(LPDRAWITEMSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPDRAWITEMSTRUCT, 48, 4);
}

static void test_pack_LPDRAWTEXTPARAMS(void)
{
    /* LPDRAWTEXTPARAMS */
    TEST_TYPE(LPDRAWTEXTPARAMS, 4, 4);
    TEST_TYPE_POINTER(LPDRAWTEXTPARAMS, 20, 4);
}

static void test_pack_LPEVENTMSG(void)
{
    /* LPEVENTMSG */
    TEST_TYPE(LPEVENTMSG, 4, 4);
    TEST_TYPE_POINTER(LPEVENTMSG, 20, 4);
}

static void test_pack_LPFILTERKEYS(void)
{
    /* LPFILTERKEYS */
    TEST_TYPE(LPFILTERKEYS, 4, 4);
    TEST_TYPE_POINTER(LPFILTERKEYS, 24, 4);
}

static void test_pack_LPGUITHREADINFO(void)
{
    /* LPGUITHREADINFO */
    TEST_TYPE(LPGUITHREADINFO, 4, 4);
    TEST_TYPE_POINTER(LPGUITHREADINFO, 48, 4);
}

static void test_pack_LPHARDWAREHOOKSTRUCT(void)
{
    /* LPHARDWAREHOOKSTRUCT */
    TEST_TYPE(LPHARDWAREHOOKSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPHARDWAREHOOKSTRUCT, 16, 4);
}

static void test_pack_LPHARDWAREINPUT(void)
{
    /* LPHARDWAREINPUT */
    TEST_TYPE(LPHARDWAREINPUT, 4, 4);
    TEST_TYPE_POINTER(LPHARDWAREINPUT, 8, 4);
}

static void test_pack_LPHELPINFO(void)
{
    /* LPHELPINFO */
    TEST_TYPE(LPHELPINFO, 4, 4);
    TEST_TYPE_POINTER(LPHELPINFO, 28, 4);
}

static void test_pack_LPHELPWININFOA(void)
{
    /* LPHELPWININFOA */
    TEST_TYPE(LPHELPWININFOA, 4, 4);
    TEST_TYPE_POINTER(LPHELPWININFOA, 28, 4);
}

static void test_pack_LPHELPWININFOW(void)
{
    /* LPHELPWININFOW */
    TEST_TYPE(LPHELPWININFOW, 4, 4);
    TEST_TYPE_POINTER(LPHELPWININFOW, 28, 4);
}

static void test_pack_LPHIGHCONTRASTA(void)
{
    /* LPHIGHCONTRASTA */
    TEST_TYPE(LPHIGHCONTRASTA, 4, 4);
    TEST_TYPE_POINTER(LPHIGHCONTRASTA, 12, 4);
}

static void test_pack_LPHIGHCONTRASTW(void)
{
    /* LPHIGHCONTRASTW */
    TEST_TYPE(LPHIGHCONTRASTW, 4, 4);
    TEST_TYPE_POINTER(LPHIGHCONTRASTW, 12, 4);
}

static void test_pack_LPICONMETRICSA(void)
{
    /* LPICONMETRICSA */
    TEST_TYPE(LPICONMETRICSA, 4, 4);
    TEST_TYPE_POINTER(LPICONMETRICSA, 76, 4);
}

static void test_pack_LPICONMETRICSW(void)
{
    /* LPICONMETRICSW */
    TEST_TYPE(LPICONMETRICSW, 4, 4);
    TEST_TYPE_POINTER(LPICONMETRICSW, 108, 4);
}

static void test_pack_LPINPUT(void)
{
    /* LPINPUT */
    TEST_TYPE(LPINPUT, 4, 4);
}

static void test_pack_LPKBDLLHOOKSTRUCT(void)
{
    /* LPKBDLLHOOKSTRUCT */
    TEST_TYPE(LPKBDLLHOOKSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPKBDLLHOOKSTRUCT, 20, 4);
}

static void test_pack_LPKEYBDINPUT(void)
{
    /* LPKEYBDINPUT */
    TEST_TYPE(LPKEYBDINPUT, 4, 4);
    TEST_TYPE_POINTER(LPKEYBDINPUT, 16, 4);
}

static void test_pack_LPMDICREATESTRUCTA(void)
{
    /* LPMDICREATESTRUCTA */
    TEST_TYPE(LPMDICREATESTRUCTA, 4, 4);
    TEST_TYPE_POINTER(LPMDICREATESTRUCTA, 36, 4);
}

static void test_pack_LPMDICREATESTRUCTW(void)
{
    /* LPMDICREATESTRUCTW */
    TEST_TYPE(LPMDICREATESTRUCTW, 4, 4);
    TEST_TYPE_POINTER(LPMDICREATESTRUCTW, 36, 4);
}

static void test_pack_LPMDINEXTMENU(void)
{
    /* LPMDINEXTMENU */
    TEST_TYPE(LPMDINEXTMENU, 4, 4);
    TEST_TYPE_POINTER(LPMDINEXTMENU, 12, 4);
}

static void test_pack_LPMEASUREITEMSTRUCT(void)
{
    /* LPMEASUREITEMSTRUCT */
    TEST_TYPE(LPMEASUREITEMSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPMEASUREITEMSTRUCT, 24, 4);
}

static void test_pack_LPMENUINFO(void)
{
    /* LPMENUINFO */
    TEST_TYPE(LPMENUINFO, 4, 4);
    TEST_TYPE_POINTER(LPMENUINFO, 28, 4);
}

static void test_pack_LPMENUITEMINFOA(void)
{
    /* LPMENUITEMINFOA */
    TEST_TYPE(LPMENUITEMINFOA, 4, 4);
    TEST_TYPE_POINTER(LPMENUITEMINFOA, 48, 4);
}

static void test_pack_LPMENUITEMINFOW(void)
{
    /* LPMENUITEMINFOW */
    TEST_TYPE(LPMENUITEMINFOW, 4, 4);
    TEST_TYPE_POINTER(LPMENUITEMINFOW, 48, 4);
}

static void test_pack_LPMINIMIZEDMETRICS(void)
{
    /* LPMINIMIZEDMETRICS */
    TEST_TYPE(LPMINIMIZEDMETRICS, 4, 4);
    TEST_TYPE_POINTER(LPMINIMIZEDMETRICS, 20, 4);
}

static void test_pack_LPMINMAXINFO(void)
{
    /* LPMINMAXINFO */
    TEST_TYPE(LPMINMAXINFO, 4, 4);
    TEST_TYPE_POINTER(LPMINMAXINFO, 40, 4);
}

static void test_pack_LPMONITORINFO(void)
{
    /* LPMONITORINFO */
    TEST_TYPE(LPMONITORINFO, 4, 4);
    TEST_TYPE_POINTER(LPMONITORINFO, 40, 4);
}

static void test_pack_LPMONITORINFOEXA(void)
{
    /* LPMONITORINFOEXA */
    TEST_TYPE(LPMONITORINFOEXA, 4, 4);
    TEST_TYPE_POINTER(LPMONITORINFOEXA, 72, 4);
}

static void test_pack_LPMONITORINFOEXW(void)
{
    /* LPMONITORINFOEXW */
    TEST_TYPE(LPMONITORINFOEXW, 4, 4);
    TEST_TYPE_POINTER(LPMONITORINFOEXW, 104, 4);
}

static void test_pack_LPMOUSEHOOKSTRUCT(void)
{
    /* LPMOUSEHOOKSTRUCT */
    TEST_TYPE(LPMOUSEHOOKSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPMOUSEHOOKSTRUCT, 20, 4);
}

static void test_pack_LPMOUSEINPUT(void)
{
    /* LPMOUSEINPUT */
    TEST_TYPE(LPMOUSEINPUT, 4, 4);
    TEST_TYPE_POINTER(LPMOUSEINPUT, 24, 4);
}

static void test_pack_LPMOUSEKEYS(void)
{
    /* LPMOUSEKEYS */
    TEST_TYPE(LPMOUSEKEYS, 4, 4);
    TEST_TYPE_POINTER(LPMOUSEKEYS, 28, 4);
}

static void test_pack_LPMSG(void)
{
    /* LPMSG */
    TEST_TYPE(LPMSG, 4, 4);
    TEST_TYPE_POINTER(LPMSG, 28, 4);
}

static void test_pack_LPMSGBOXPARAMSA(void)
{
    /* LPMSGBOXPARAMSA */
    TEST_TYPE(LPMSGBOXPARAMSA, 4, 4);
    TEST_TYPE_POINTER(LPMSGBOXPARAMSA, 40, 4);
}

static void test_pack_LPMSGBOXPARAMSW(void)
{
    /* LPMSGBOXPARAMSW */
    TEST_TYPE(LPMSGBOXPARAMSW, 4, 4);
    TEST_TYPE_POINTER(LPMSGBOXPARAMSW, 40, 4);
}

static void test_pack_LPMSLLHOOKSTRUCT(void)
{
    /* LPMSLLHOOKSTRUCT */
    TEST_TYPE(LPMSLLHOOKSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPMSLLHOOKSTRUCT, 24, 4);
}

static void test_pack_LPMULTIKEYHELPA(void)
{
    /* LPMULTIKEYHELPA */
    TEST_TYPE(LPMULTIKEYHELPA, 4, 4);
    TEST_TYPE_POINTER(LPMULTIKEYHELPA, 8, 4);
}

static void test_pack_LPMULTIKEYHELPW(void)
{
    /* LPMULTIKEYHELPW */
    TEST_TYPE(LPMULTIKEYHELPW, 4, 4);
    TEST_TYPE_POINTER(LPMULTIKEYHELPW, 8, 4);
}

static void test_pack_LPNCCALCSIZE_PARAMS(void)
{
    /* LPNCCALCSIZE_PARAMS */
    TEST_TYPE(LPNCCALCSIZE_PARAMS, 4, 4);
    TEST_TYPE_POINTER(LPNCCALCSIZE_PARAMS, 52, 4);
}

static void test_pack_LPNMHDR(void)
{
    /* LPNMHDR */
    TEST_TYPE(LPNMHDR, 4, 4);
    TEST_TYPE_POINTER(LPNMHDR, 12, 4);
}

static void test_pack_LPNONCLIENTMETRICSA(void)
{
    /* LPNONCLIENTMETRICSA */
    TEST_TYPE(LPNONCLIENTMETRICSA, 4, 4);
    TEST_TYPE_POINTER(LPNONCLIENTMETRICSA, 340, 4);
}

static void test_pack_LPNONCLIENTMETRICSW(void)
{
    /* LPNONCLIENTMETRICSW */
    TEST_TYPE(LPNONCLIENTMETRICSW, 4, 4);
    TEST_TYPE_POINTER(LPNONCLIENTMETRICSW, 500, 4);
}

static void test_pack_LPPAINTSTRUCT(void)
{
    /* LPPAINTSTRUCT */
    TEST_TYPE(LPPAINTSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPPAINTSTRUCT, 64, 4);
}

static void test_pack_LPSCROLLINFO(void)
{
    /* LPSCROLLINFO */
    TEST_TYPE(LPSCROLLINFO, 4, 4);
    TEST_TYPE_POINTER(LPSCROLLINFO, 28, 4);
}

static void test_pack_LPSERIALKEYSA(void)
{
    /* LPSERIALKEYSA */
    TEST_TYPE(LPSERIALKEYSA, 4, 4);
    TEST_TYPE_POINTER(LPSERIALKEYSA, 28, 4);
}

static void test_pack_LPSERIALKEYSW(void)
{
    /* LPSERIALKEYSW */
    TEST_TYPE(LPSERIALKEYSW, 4, 4);
    TEST_TYPE_POINTER(LPSERIALKEYSW, 28, 4);
}

static void test_pack_LPSOUNDSENTRYA(void)
{
    /* LPSOUNDSENTRYA */
    TEST_TYPE(LPSOUNDSENTRYA, 4, 4);
    TEST_TYPE_POINTER(LPSOUNDSENTRYA, 48, 4);
}

static void test_pack_LPSOUNDSENTRYW(void)
{
    /* LPSOUNDSENTRYW */
    TEST_TYPE(LPSOUNDSENTRYW, 4, 4);
    TEST_TYPE_POINTER(LPSOUNDSENTRYW, 48, 4);
}

static void test_pack_LPSTICKYKEYS(void)
{
    /* LPSTICKYKEYS */
    TEST_TYPE(LPSTICKYKEYS, 4, 4);
    TEST_TYPE_POINTER(LPSTICKYKEYS, 8, 4);
}

static void test_pack_LPSTYLESTRUCT(void)
{
    /* LPSTYLESTRUCT */
    TEST_TYPE(LPSTYLESTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPSTYLESTRUCT, 8, 4);
}

static void test_pack_LPTITLEBARINFO(void)
{
    /* LPTITLEBARINFO */
    TEST_TYPE(LPTITLEBARINFO, 4, 4);
    TEST_TYPE_POINTER(LPTITLEBARINFO, 44, 4);
}

static void test_pack_LPTOGGLEKEYS(void)
{
    /* LPTOGGLEKEYS */
    TEST_TYPE(LPTOGGLEKEYS, 4, 4);
    TEST_TYPE_POINTER(LPTOGGLEKEYS, 8, 4);
}

static void test_pack_LPTPMPARAMS(void)
{
    /* LPTPMPARAMS */
    TEST_TYPE(LPTPMPARAMS, 4, 4);
    TEST_TYPE_POINTER(LPTPMPARAMS, 20, 4);
}

static void test_pack_LPTRACKMOUSEEVENT(void)
{
    /* LPTRACKMOUSEEVENT */
    TEST_TYPE(LPTRACKMOUSEEVENT, 4, 4);
    TEST_TYPE_POINTER(LPTRACKMOUSEEVENT, 16, 4);
}

static void test_pack_LPWINDOWINFO(void)
{
    /* LPWINDOWINFO */
    TEST_TYPE(LPWINDOWINFO, 4, 4);
    TEST_TYPE_POINTER(LPWINDOWINFO, 60, 4);
}

static void test_pack_LPWINDOWPLACEMENT(void)
{
    /* LPWINDOWPLACEMENT */
    TEST_TYPE(LPWINDOWPLACEMENT, 4, 4);
    TEST_TYPE_POINTER(LPWINDOWPLACEMENT, 44, 4);
}

static void test_pack_LPWINDOWPOS(void)
{
    /* LPWINDOWPOS */
    TEST_TYPE(LPWINDOWPOS, 4, 4);
    TEST_TYPE_POINTER(LPWINDOWPOS, 28, 4);
}

static void test_pack_LPWNDCLASSA(void)
{
    /* LPWNDCLASSA */
    TEST_TYPE(LPWNDCLASSA, 4, 4);
    TEST_TYPE_POINTER(LPWNDCLASSA, 40, 4);
}

static void test_pack_LPWNDCLASSEXA(void)
{
    /* LPWNDCLASSEXA */
    TEST_TYPE(LPWNDCLASSEXA, 4, 4);
    TEST_TYPE_POINTER(LPWNDCLASSEXA, 48, 4);
}

static void test_pack_LPWNDCLASSEXW(void)
{
    /* LPWNDCLASSEXW */
    TEST_TYPE(LPWNDCLASSEXW, 4, 4);
    TEST_TYPE_POINTER(LPWNDCLASSEXW, 48, 4);
}

static void test_pack_LPWNDCLASSW(void)
{
    /* LPWNDCLASSW */
    TEST_TYPE(LPWNDCLASSW, 4, 4);
    TEST_TYPE_POINTER(LPWNDCLASSW, 40, 4);
}

static void test_pack_MDICREATESTRUCTA(void)
{
    /* MDICREATESTRUCTA (pack 4) */
    TEST_TYPE(MDICREATESTRUCTA, 36, 4);
    TEST_FIELD(MDICREATESTRUCTA, szClass, 0, 4, 4);
    TEST_FIELD(MDICREATESTRUCTA, szTitle, 4, 4, 4);
    TEST_FIELD(MDICREATESTRUCTA, hOwner, 8, 4, 4);
    TEST_FIELD(MDICREATESTRUCTA, x, 12, 4, 4);
    TEST_FIELD(MDICREATESTRUCTA, y, 16, 4, 4);
    TEST_FIELD(MDICREATESTRUCTA, cx, 20, 4, 4);
    TEST_FIELD(MDICREATESTRUCTA, cy, 24, 4, 4);
    TEST_FIELD(MDICREATESTRUCTA, style, 28, 4, 4);
    TEST_FIELD(MDICREATESTRUCTA, lParam, 32, 4, 4);
}

static void test_pack_MDICREATESTRUCTW(void)
{
    /* MDICREATESTRUCTW (pack 4) */
    TEST_TYPE(MDICREATESTRUCTW, 36, 4);
    TEST_FIELD(MDICREATESTRUCTW, szClass, 0, 4, 4);
    TEST_FIELD(MDICREATESTRUCTW, szTitle, 4, 4, 4);
    TEST_FIELD(MDICREATESTRUCTW, hOwner, 8, 4, 4);
    TEST_FIELD(MDICREATESTRUCTW, x, 12, 4, 4);
    TEST_FIELD(MDICREATESTRUCTW, y, 16, 4, 4);
    TEST_FIELD(MDICREATESTRUCTW, cx, 20, 4, 4);
    TEST_FIELD(MDICREATESTRUCTW, cy, 24, 4, 4);
    TEST_FIELD(MDICREATESTRUCTW, style, 28, 4, 4);
    TEST_FIELD(MDICREATESTRUCTW, lParam, 32, 4, 4);
}

static void test_pack_MDINEXTMENU(void)
{
    /* MDINEXTMENU (pack 4) */
    TEST_TYPE(MDINEXTMENU, 12, 4);
    TEST_FIELD(MDINEXTMENU, hmenuIn, 0, 4, 4);
    TEST_FIELD(MDINEXTMENU, hmenuNext, 4, 4, 4);
    TEST_FIELD(MDINEXTMENU, hwndNext, 8, 4, 4);
}

static void test_pack_MEASUREITEMSTRUCT(void)
{
    /* MEASUREITEMSTRUCT (pack 4) */
    TEST_TYPE(MEASUREITEMSTRUCT, 24, 4);
    TEST_FIELD(MEASUREITEMSTRUCT, CtlType, 0, 4, 4);
    TEST_FIELD(MEASUREITEMSTRUCT, CtlID, 4, 4, 4);
    TEST_FIELD(MEASUREITEMSTRUCT, itemID, 8, 4, 4);
    TEST_FIELD(MEASUREITEMSTRUCT, itemWidth, 12, 4, 4);
    TEST_FIELD(MEASUREITEMSTRUCT, itemHeight, 16, 4, 4);
    TEST_FIELD(MEASUREITEMSTRUCT, itemData, 20, 4, 4);
}

static void test_pack_MENUINFO(void)
{
    /* MENUINFO (pack 4) */
    TEST_TYPE(MENUINFO, 28, 4);
    TEST_FIELD(MENUINFO, cbSize, 0, 4, 4);
    TEST_FIELD(MENUINFO, fMask, 4, 4, 4);
    TEST_FIELD(MENUINFO, dwStyle, 8, 4, 4);
    TEST_FIELD(MENUINFO, cyMax, 12, 4, 4);
    TEST_FIELD(MENUINFO, hbrBack, 16, 4, 4);
    TEST_FIELD(MENUINFO, dwContextHelpID, 20, 4, 4);
    TEST_FIELD(MENUINFO, dwMenuData, 24, 4, 4);
}

static void test_pack_MENUITEMINFOA(void)
{
    /* MENUITEMINFOA (pack 4) */
    TEST_TYPE(MENUITEMINFOA, 48, 4);
    TEST_FIELD(MENUITEMINFOA, cbSize, 0, 4, 4);
    TEST_FIELD(MENUITEMINFOA, fMask, 4, 4, 4);
    TEST_FIELD(MENUITEMINFOA, fType, 8, 4, 4);
    TEST_FIELD(MENUITEMINFOA, fState, 12, 4, 4);
    TEST_FIELD(MENUITEMINFOA, wID, 16, 4, 4);
    TEST_FIELD(MENUITEMINFOA, hSubMenu, 20, 4, 4);
    TEST_FIELD(MENUITEMINFOA, hbmpChecked, 24, 4, 4);
    TEST_FIELD(MENUITEMINFOA, hbmpUnchecked, 28, 4, 4);
    TEST_FIELD(MENUITEMINFOA, dwItemData, 32, 4, 4);
    TEST_FIELD(MENUITEMINFOA, dwTypeData, 36, 4, 4);
    TEST_FIELD(MENUITEMINFOA, cch, 40, 4, 4);
    TEST_FIELD(MENUITEMINFOA, hbmpItem, 44, 4, 4);
}

static void test_pack_MENUITEMINFOW(void)
{
    /* MENUITEMINFOW (pack 4) */
    TEST_TYPE(MENUITEMINFOW, 48, 4);
    TEST_FIELD(MENUITEMINFOW, cbSize, 0, 4, 4);
    TEST_FIELD(MENUITEMINFOW, fMask, 4, 4, 4);
    TEST_FIELD(MENUITEMINFOW, fType, 8, 4, 4);
    TEST_FIELD(MENUITEMINFOW, fState, 12, 4, 4);
    TEST_FIELD(MENUITEMINFOW, wID, 16, 4, 4);
    TEST_FIELD(MENUITEMINFOW, hSubMenu, 20, 4, 4);
    TEST_FIELD(MENUITEMINFOW, hbmpChecked, 24, 4, 4);
    TEST_FIELD(MENUITEMINFOW, hbmpUnchecked, 28, 4, 4);
    TEST_FIELD(MENUITEMINFOW, dwItemData, 32, 4, 4);
    TEST_FIELD(MENUITEMINFOW, dwTypeData, 36, 4, 4);
    TEST_FIELD(MENUITEMINFOW, cch, 40, 4, 4);
    TEST_FIELD(MENUITEMINFOW, hbmpItem, 44, 4, 4);
}

static void test_pack_MENUITEMTEMPLATE(void)
{
    /* MENUITEMTEMPLATE (pack 4) */
    TEST_TYPE(MENUITEMTEMPLATE, 6, 2);
    TEST_FIELD(MENUITEMTEMPLATE, mtOption, 0, 2, 2);
    TEST_FIELD(MENUITEMTEMPLATE, mtID, 2, 2, 2);
    TEST_FIELD(MENUITEMTEMPLATE, mtString, 4, 2, 2);
}

static void test_pack_MENUITEMTEMPLATEHEADER(void)
{
    /* MENUITEMTEMPLATEHEADER (pack 4) */
    TEST_TYPE(MENUITEMTEMPLATEHEADER, 4, 2);
    TEST_FIELD(MENUITEMTEMPLATEHEADER, versionNumber, 0, 2, 2);
    TEST_FIELD(MENUITEMTEMPLATEHEADER, offset, 2, 2, 2);
}

static void test_pack_MINIMIZEDMETRICS(void)
{
    /* MINIMIZEDMETRICS (pack 4) */
    TEST_TYPE(MINIMIZEDMETRICS, 20, 4);
    TEST_FIELD(MINIMIZEDMETRICS, cbSize, 0, 4, 4);
    TEST_FIELD(MINIMIZEDMETRICS, iWidth, 4, 4, 4);
    TEST_FIELD(MINIMIZEDMETRICS, iHorzGap, 8, 4, 4);
    TEST_FIELD(MINIMIZEDMETRICS, iVertGap, 12, 4, 4);
    TEST_FIELD(MINIMIZEDMETRICS, iArrange, 16, 4, 4);
}

static void test_pack_MINMAXINFO(void)
{
    /* MINMAXINFO (pack 4) */
    TEST_TYPE(MINMAXINFO, 40, 4);
    TEST_FIELD(MINMAXINFO, ptReserved, 0, 8, 4);
    TEST_FIELD(MINMAXINFO, ptMaxSize, 8, 8, 4);
    TEST_FIELD(MINMAXINFO, ptMaxPosition, 16, 8, 4);
    TEST_FIELD(MINMAXINFO, ptMinTrackSize, 24, 8, 4);
    TEST_FIELD(MINMAXINFO, ptMaxTrackSize, 32, 8, 4);
}

static void test_pack_MONITORENUMPROC(void)
{
    /* MONITORENUMPROC */
    TEST_TYPE(MONITORENUMPROC, 4, 4);
}

static void test_pack_MONITORINFO(void)
{
    /* MONITORINFO (pack 4) */
    TEST_TYPE(MONITORINFO, 40, 4);
    TEST_FIELD(MONITORINFO, cbSize, 0, 4, 4);
    TEST_FIELD(MONITORINFO, rcMonitor, 4, 16, 4);
    TEST_FIELD(MONITORINFO, rcWork, 20, 16, 4);
    TEST_FIELD(MONITORINFO, dwFlags, 36, 4, 4);
}

static void test_pack_MONITORINFOEXA(void)
{
    /* MONITORINFOEXA (pack 4) */
    TEST_TYPE(MONITORINFOEXA, 72, 4);
    TEST_FIELD(MONITORINFOEXA, cbSize, 0, 4, 4);
    TEST_FIELD(MONITORINFOEXA, rcMonitor, 4, 16, 4);
    TEST_FIELD(MONITORINFOEXA, rcWork, 20, 16, 4);
    TEST_FIELD(MONITORINFOEXA, dwFlags, 36, 4, 4);
    TEST_FIELD(MONITORINFOEXA, szDevice, 40, 32, 1);
}

static void test_pack_MONITORINFOEXW(void)
{
    /* MONITORINFOEXW (pack 4) */
    TEST_TYPE(MONITORINFOEXW, 104, 4);
    TEST_FIELD(MONITORINFOEXW, cbSize, 0, 4, 4);
    TEST_FIELD(MONITORINFOEXW, rcMonitor, 4, 16, 4);
    TEST_FIELD(MONITORINFOEXW, rcWork, 20, 16, 4);
    TEST_FIELD(MONITORINFOEXW, dwFlags, 36, 4, 4);
    TEST_FIELD(MONITORINFOEXW, szDevice, 40, 64, 2);
}

static void test_pack_MOUSEHOOKSTRUCT(void)
{
    /* MOUSEHOOKSTRUCT (pack 4) */
    TEST_TYPE(MOUSEHOOKSTRUCT, 20, 4);
    TEST_FIELD(MOUSEHOOKSTRUCT, pt, 0, 8, 4);
    TEST_FIELD(MOUSEHOOKSTRUCT, hwnd, 8, 4, 4);
    TEST_FIELD(MOUSEHOOKSTRUCT, wHitTestCode, 12, 4, 4);
    TEST_FIELD(MOUSEHOOKSTRUCT, dwExtraInfo, 16, 4, 4);
}

static void test_pack_MOUSEINPUT(void)
{
    /* MOUSEINPUT (pack 4) */
    TEST_TYPE(MOUSEINPUT, 24, 4);
    TEST_FIELD(MOUSEINPUT, dx, 0, 4, 4);
    TEST_FIELD(MOUSEINPUT, dy, 4, 4, 4);
    TEST_FIELD(MOUSEINPUT, mouseData, 8, 4, 4);
    TEST_FIELD(MOUSEINPUT, dwFlags, 12, 4, 4);
    TEST_FIELD(MOUSEINPUT, time, 16, 4, 4);
    TEST_FIELD(MOUSEINPUT, dwExtraInfo, 20, 4, 4);
}

static void test_pack_MOUSEKEYS(void)
{
    /* MOUSEKEYS (pack 4) */
    TEST_TYPE(MOUSEKEYS, 28, 4);
    TEST_FIELD(MOUSEKEYS, cbSize, 0, 4, 4);
    TEST_FIELD(MOUSEKEYS, dwFlags, 4, 4, 4);
    TEST_FIELD(MOUSEKEYS, iMaxSpeed, 8, 4, 4);
    TEST_FIELD(MOUSEKEYS, iTimeToMaxSpeed, 12, 4, 4);
    TEST_FIELD(MOUSEKEYS, iCtrlSpeed, 16, 4, 4);
    TEST_FIELD(MOUSEKEYS, dwReserved1, 20, 4, 4);
    TEST_FIELD(MOUSEKEYS, dwReserved2, 24, 4, 4);
}

static void test_pack_MSG(void)
{
    /* MSG (pack 4) */
    TEST_TYPE(MSG, 28, 4);
    TEST_FIELD(MSG, hwnd, 0, 4, 4);
    TEST_FIELD(MSG, message, 4, 4, 4);
    TEST_FIELD(MSG, wParam, 8, 4, 4);
    TEST_FIELD(MSG, lParam, 12, 4, 4);
    TEST_FIELD(MSG, time, 16, 4, 4);
    TEST_FIELD(MSG, pt, 20, 8, 4);
}

static void test_pack_MSGBOXCALLBACK(void)
{
    /* MSGBOXCALLBACK */
    TEST_TYPE(MSGBOXCALLBACK, 4, 4);
}

static void test_pack_MSGBOXPARAMSA(void)
{
    /* MSGBOXPARAMSA (pack 4) */
    TEST_TYPE(MSGBOXPARAMSA, 40, 4);
    TEST_FIELD(MSGBOXPARAMSA, cbSize, 0, 4, 4);
    TEST_FIELD(MSGBOXPARAMSA, hwndOwner, 4, 4, 4);
    TEST_FIELD(MSGBOXPARAMSA, hInstance, 8, 4, 4);
    TEST_FIELD(MSGBOXPARAMSA, lpszText, 12, 4, 4);
    TEST_FIELD(MSGBOXPARAMSA, lpszCaption, 16, 4, 4);
    TEST_FIELD(MSGBOXPARAMSA, dwStyle, 20, 4, 4);
    TEST_FIELD(MSGBOXPARAMSA, lpszIcon, 24, 4, 4);
    TEST_FIELD(MSGBOXPARAMSA, dwContextHelpId, 28, 4, 4);
    TEST_FIELD(MSGBOXPARAMSA, lpfnMsgBoxCallback, 32, 4, 4);
    TEST_FIELD(MSGBOXPARAMSA, dwLanguageId, 36, 4, 4);
}

static void test_pack_MSGBOXPARAMSW(void)
{
    /* MSGBOXPARAMSW (pack 4) */
    TEST_TYPE(MSGBOXPARAMSW, 40, 4);
    TEST_FIELD(MSGBOXPARAMSW, cbSize, 0, 4, 4);
    TEST_FIELD(MSGBOXPARAMSW, hwndOwner, 4, 4, 4);
    TEST_FIELD(MSGBOXPARAMSW, hInstance, 8, 4, 4);
    TEST_FIELD(MSGBOXPARAMSW, lpszText, 12, 4, 4);
    TEST_FIELD(MSGBOXPARAMSW, lpszCaption, 16, 4, 4);
    TEST_FIELD(MSGBOXPARAMSW, dwStyle, 20, 4, 4);
    TEST_FIELD(MSGBOXPARAMSW, lpszIcon, 24, 4, 4);
    TEST_FIELD(MSGBOXPARAMSW, dwContextHelpId, 28, 4, 4);
    TEST_FIELD(MSGBOXPARAMSW, lpfnMsgBoxCallback, 32, 4, 4);
    TEST_FIELD(MSGBOXPARAMSW, dwLanguageId, 36, 4, 4);
}

static void test_pack_MSLLHOOKSTRUCT(void)
{
    /* MSLLHOOKSTRUCT (pack 4) */
    TEST_TYPE(MSLLHOOKSTRUCT, 24, 4);
    TEST_FIELD(MSLLHOOKSTRUCT, pt, 0, 8, 4);
    TEST_FIELD(MSLLHOOKSTRUCT, mouseData, 8, 4, 4);
    TEST_FIELD(MSLLHOOKSTRUCT, flags, 12, 4, 4);
    TEST_FIELD(MSLLHOOKSTRUCT, time, 16, 4, 4);
    TEST_FIELD(MSLLHOOKSTRUCT, dwExtraInfo, 20, 4, 4);
}

static void test_pack_MULTIKEYHELPA(void)
{
    /* MULTIKEYHELPA (pack 4) */
    TEST_TYPE(MULTIKEYHELPA, 8, 4);
    TEST_FIELD(MULTIKEYHELPA, mkSize, 0, 4, 4);
    TEST_FIELD(MULTIKEYHELPA, mkKeylist, 4, 1, 1);
    TEST_FIELD(MULTIKEYHELPA, szKeyphrase, 5, 1, 1);
}

static void test_pack_MULTIKEYHELPW(void)
{
    /* MULTIKEYHELPW (pack 4) */
    TEST_TYPE(MULTIKEYHELPW, 8, 4);
    TEST_FIELD(MULTIKEYHELPW, mkSize, 0, 4, 4);
    TEST_FIELD(MULTIKEYHELPW, mkKeylist, 4, 2, 2);
    TEST_FIELD(MULTIKEYHELPW, szKeyphrase, 6, 2, 2);
}

static void test_pack_NAMEENUMPROCA(void)
{
    /* NAMEENUMPROCA */
    TEST_TYPE(NAMEENUMPROCA, 4, 4);
}

static void test_pack_NAMEENUMPROCW(void)
{
    /* NAMEENUMPROCW */
    TEST_TYPE(NAMEENUMPROCW, 4, 4);
}

static void test_pack_NCCALCSIZE_PARAMS(void)
{
    /* NCCALCSIZE_PARAMS (pack 4) */
    TEST_TYPE(NCCALCSIZE_PARAMS, 52, 4);
    TEST_FIELD(NCCALCSIZE_PARAMS, rgrc, 0, 48, 4);
    TEST_FIELD(NCCALCSIZE_PARAMS, lppos, 48, 4, 4);
}

static void test_pack_NMHDR(void)
{
    /* NMHDR (pack 4) */
    TEST_TYPE(NMHDR, 12, 4);
    TEST_FIELD(NMHDR, hwndFrom, 0, 4, 4);
    TEST_FIELD(NMHDR, idFrom, 4, 4, 4);
    TEST_FIELD(NMHDR, code, 8, 4, 4);
}

static void test_pack_NONCLIENTMETRICSA(void)
{
    /* NONCLIENTMETRICSA (pack 4) */
    TEST_TYPE(NONCLIENTMETRICSA, 340, 4);
    TEST_FIELD(NONCLIENTMETRICSA, cbSize, 0, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSA, iBorderWidth, 4, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSA, iScrollWidth, 8, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSA, iScrollHeight, 12, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSA, iCaptionWidth, 16, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSA, iCaptionHeight, 20, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSA, lfCaptionFont, 24, 60, 4);
    TEST_FIELD(NONCLIENTMETRICSA, iSmCaptionWidth, 84, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSA, iSmCaptionHeight, 88, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSA, lfSmCaptionFont, 92, 60, 4);
    TEST_FIELD(NONCLIENTMETRICSA, iMenuWidth, 152, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSA, iMenuHeight, 156, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSA, lfMenuFont, 160, 60, 4);
    TEST_FIELD(NONCLIENTMETRICSA, lfStatusFont, 220, 60, 4);
    TEST_FIELD(NONCLIENTMETRICSA, lfMessageFont, 280, 60, 4);
}

static void test_pack_NONCLIENTMETRICSW(void)
{
    /* NONCLIENTMETRICSW (pack 4) */
    TEST_TYPE(NONCLIENTMETRICSW, 500, 4);
    TEST_FIELD(NONCLIENTMETRICSW, cbSize, 0, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSW, iBorderWidth, 4, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSW, iScrollWidth, 8, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSW, iScrollHeight, 12, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSW, iCaptionWidth, 16, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSW, iCaptionHeight, 20, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSW, lfCaptionFont, 24, 92, 4);
    TEST_FIELD(NONCLIENTMETRICSW, iSmCaptionWidth, 116, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSW, iSmCaptionHeight, 120, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSW, lfSmCaptionFont, 124, 92, 4);
    TEST_FIELD(NONCLIENTMETRICSW, iMenuWidth, 216, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSW, iMenuHeight, 220, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSW, lfMenuFont, 224, 92, 4);
    TEST_FIELD(NONCLIENTMETRICSW, lfStatusFont, 316, 92, 4);
    TEST_FIELD(NONCLIENTMETRICSW, lfMessageFont, 408, 92, 4);
}

static void test_pack_PAINTSTRUCT(void)
{
    /* PAINTSTRUCT (pack 4) */
    TEST_TYPE(PAINTSTRUCT, 64, 4);
    TEST_FIELD(PAINTSTRUCT, hdc, 0, 4, 4);
    TEST_FIELD(PAINTSTRUCT, fErase, 4, 4, 4);
    TEST_FIELD(PAINTSTRUCT, rcPaint, 8, 16, 4);
    TEST_FIELD(PAINTSTRUCT, fRestore, 24, 4, 4);
    TEST_FIELD(PAINTSTRUCT, fIncUpdate, 28, 4, 4);
    TEST_FIELD(PAINTSTRUCT, rgbReserved, 32, 32, 1);
}

static void test_pack_PCOMBOBOXINFO(void)
{
    /* PCOMBOBOXINFO */
    TEST_TYPE(PCOMBOBOXINFO, 4, 4);
    TEST_TYPE_POINTER(PCOMBOBOXINFO, 52, 4);
}

static void test_pack_PCOMPAREITEMSTRUCT(void)
{
    /* PCOMPAREITEMSTRUCT */
    TEST_TYPE(PCOMPAREITEMSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PCOMPAREITEMSTRUCT, 32, 4);
}

static void test_pack_PCOPYDATASTRUCT(void)
{
    /* PCOPYDATASTRUCT */
    TEST_TYPE(PCOPYDATASTRUCT, 4, 4);
    TEST_TYPE_POINTER(PCOPYDATASTRUCT, 12, 4);
}

static void test_pack_PCURSORINFO(void)
{
    /* PCURSORINFO */
    TEST_TYPE(PCURSORINFO, 4, 4);
    TEST_TYPE_POINTER(PCURSORINFO, 20, 4);
}

static void test_pack_PCWPRETSTRUCT(void)
{
    /* PCWPRETSTRUCT */
    TEST_TYPE(PCWPRETSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PCWPRETSTRUCT, 20, 4);
}

static void test_pack_PCWPSTRUCT(void)
{
    /* PCWPSTRUCT */
    TEST_TYPE(PCWPSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PCWPSTRUCT, 16, 4);
}

static void test_pack_PDEBUGHOOKINFO(void)
{
    /* PDEBUGHOOKINFO */
    TEST_TYPE(PDEBUGHOOKINFO, 4, 4);
    TEST_TYPE_POINTER(PDEBUGHOOKINFO, 20, 4);
}

static void test_pack_PDELETEITEMSTRUCT(void)
{
    /* PDELETEITEMSTRUCT */
    TEST_TYPE(PDELETEITEMSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PDELETEITEMSTRUCT, 20, 4);
}

static void test_pack_PDLGITEMTEMPLATEA(void)
{
    /* PDLGITEMTEMPLATEA */
    TEST_TYPE(PDLGITEMTEMPLATEA, 4, 4);
    TEST_TYPE_POINTER(PDLGITEMTEMPLATEA, 18, 2);
}

static void test_pack_PDLGITEMTEMPLATEW(void)
{
    /* PDLGITEMTEMPLATEW */
    TEST_TYPE(PDLGITEMTEMPLATEW, 4, 4);
    TEST_TYPE_POINTER(PDLGITEMTEMPLATEW, 18, 2);
}

static void test_pack_PDRAWITEMSTRUCT(void)
{
    /* PDRAWITEMSTRUCT */
    TEST_TYPE(PDRAWITEMSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PDRAWITEMSTRUCT, 48, 4);
}

static void test_pack_PEVENTMSG(void)
{
    /* PEVENTMSG */
    TEST_TYPE(PEVENTMSG, 4, 4);
    TEST_TYPE_POINTER(PEVENTMSG, 20, 4);
}

static void test_pack_PFLASHWINFO(void)
{
    /* PFLASHWINFO */
    TEST_TYPE(PFLASHWINFO, 4, 4);
    TEST_TYPE_POINTER(PFLASHWINFO, 20, 4);
}

static void test_pack_PGUITHREADINFO(void)
{
    /* PGUITHREADINFO */
    TEST_TYPE(PGUITHREADINFO, 4, 4);
    TEST_TYPE_POINTER(PGUITHREADINFO, 48, 4);
}

static void test_pack_PHARDWAREHOOKSTRUCT(void)
{
    /* PHARDWAREHOOKSTRUCT */
    TEST_TYPE(PHARDWAREHOOKSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PHARDWAREHOOKSTRUCT, 16, 4);
}

static void test_pack_PHARDWAREINPUT(void)
{
    /* PHARDWAREINPUT */
    TEST_TYPE(PHARDWAREINPUT, 4, 4);
    TEST_TYPE_POINTER(PHARDWAREINPUT, 8, 4);
}

static void test_pack_PHDEVNOTIFY(void)
{
    /* PHDEVNOTIFY */
    TEST_TYPE(PHDEVNOTIFY, 4, 4);
    TEST_TYPE_POINTER(PHDEVNOTIFY, 4, 4);
}

static void test_pack_PHELPWININFOA(void)
{
    /* PHELPWININFOA */
    TEST_TYPE(PHELPWININFOA, 4, 4);
    TEST_TYPE_POINTER(PHELPWININFOA, 28, 4);
}

static void test_pack_PHELPWININFOW(void)
{
    /* PHELPWININFOW */
    TEST_TYPE(PHELPWININFOW, 4, 4);
    TEST_TYPE_POINTER(PHELPWININFOW, 28, 4);
}

static void test_pack_PICONINFO(void)
{
    /* PICONINFO */
    TEST_TYPE(PICONINFO, 4, 4);
    TEST_TYPE_POINTER(PICONINFO, 20, 4);
}

static void test_pack_PICONMETRICSA(void)
{
    /* PICONMETRICSA */
    TEST_TYPE(PICONMETRICSA, 4, 4);
    TEST_TYPE_POINTER(PICONMETRICSA, 76, 4);
}

static void test_pack_PICONMETRICSW(void)
{
    /* PICONMETRICSW */
    TEST_TYPE(PICONMETRICSW, 4, 4);
    TEST_TYPE_POINTER(PICONMETRICSW, 108, 4);
}

static void test_pack_PINPUT(void)
{
    /* PINPUT */
    TEST_TYPE(PINPUT, 4, 4);
}

static void test_pack_PKBDLLHOOKSTRUCT(void)
{
    /* PKBDLLHOOKSTRUCT */
    TEST_TYPE(PKBDLLHOOKSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PKBDLLHOOKSTRUCT, 20, 4);
}

static void test_pack_PKEYBDINPUT(void)
{
    /* PKEYBDINPUT */
    TEST_TYPE(PKEYBDINPUT, 4, 4);
    TEST_TYPE_POINTER(PKEYBDINPUT, 16, 4);
}

static void test_pack_PMDINEXTMENU(void)
{
    /* PMDINEXTMENU */
    TEST_TYPE(PMDINEXTMENU, 4, 4);
    TEST_TYPE_POINTER(PMDINEXTMENU, 12, 4);
}

static void test_pack_PMEASUREITEMSTRUCT(void)
{
    /* PMEASUREITEMSTRUCT */
    TEST_TYPE(PMEASUREITEMSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PMEASUREITEMSTRUCT, 24, 4);
}

static void test_pack_PMENUITEMTEMPLATE(void)
{
    /* PMENUITEMTEMPLATE */
    TEST_TYPE(PMENUITEMTEMPLATE, 4, 4);
    TEST_TYPE_POINTER(PMENUITEMTEMPLATE, 6, 2);
}

static void test_pack_PMENUITEMTEMPLATEHEADER(void)
{
    /* PMENUITEMTEMPLATEHEADER */
    TEST_TYPE(PMENUITEMTEMPLATEHEADER, 4, 4);
    TEST_TYPE_POINTER(PMENUITEMTEMPLATEHEADER, 4, 2);
}

static void test_pack_PMINIMIZEDMETRICS(void)
{
    /* PMINIMIZEDMETRICS */
    TEST_TYPE(PMINIMIZEDMETRICS, 4, 4);
    TEST_TYPE_POINTER(PMINIMIZEDMETRICS, 20, 4);
}

static void test_pack_PMINMAXINFO(void)
{
    /* PMINMAXINFO */
    TEST_TYPE(PMINMAXINFO, 4, 4);
    TEST_TYPE_POINTER(PMINMAXINFO, 40, 4);
}

static void test_pack_PMOUSEHOOKSTRUCT(void)
{
    /* PMOUSEHOOKSTRUCT */
    TEST_TYPE(PMOUSEHOOKSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PMOUSEHOOKSTRUCT, 20, 4);
}

static void test_pack_PMOUSEINPUT(void)
{
    /* PMOUSEINPUT */
    TEST_TYPE(PMOUSEINPUT, 4, 4);
    TEST_TYPE_POINTER(PMOUSEINPUT, 24, 4);
}

static void test_pack_PMSG(void)
{
    /* PMSG */
    TEST_TYPE(PMSG, 4, 4);
    TEST_TYPE_POINTER(PMSG, 28, 4);
}

static void test_pack_PMSGBOXPARAMSA(void)
{
    /* PMSGBOXPARAMSA */
    TEST_TYPE(PMSGBOXPARAMSA, 4, 4);
    TEST_TYPE_POINTER(PMSGBOXPARAMSA, 40, 4);
}

static void test_pack_PMSGBOXPARAMSW(void)
{
    /* PMSGBOXPARAMSW */
    TEST_TYPE(PMSGBOXPARAMSW, 4, 4);
    TEST_TYPE_POINTER(PMSGBOXPARAMSW, 40, 4);
}

static void test_pack_PMSLLHOOKSTRUCT(void)
{
    /* PMSLLHOOKSTRUCT */
    TEST_TYPE(PMSLLHOOKSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PMSLLHOOKSTRUCT, 24, 4);
}

static void test_pack_PMULTIKEYHELPA(void)
{
    /* PMULTIKEYHELPA */
    TEST_TYPE(PMULTIKEYHELPA, 4, 4);
    TEST_TYPE_POINTER(PMULTIKEYHELPA, 8, 4);
}

static void test_pack_PMULTIKEYHELPW(void)
{
    /* PMULTIKEYHELPW */
    TEST_TYPE(PMULTIKEYHELPW, 4, 4);
    TEST_TYPE_POINTER(PMULTIKEYHELPW, 8, 4);
}

static void test_pack_PNONCLIENTMETRICSA(void)
{
    /* PNONCLIENTMETRICSA */
    TEST_TYPE(PNONCLIENTMETRICSA, 4, 4);
    TEST_TYPE_POINTER(PNONCLIENTMETRICSA, 340, 4);
}

static void test_pack_PNONCLIENTMETRICSW(void)
{
    /* PNONCLIENTMETRICSW */
    TEST_TYPE(PNONCLIENTMETRICSW, 4, 4);
    TEST_TYPE_POINTER(PNONCLIENTMETRICSW, 500, 4);
}

static void test_pack_PPAINTSTRUCT(void)
{
    /* PPAINTSTRUCT */
    TEST_TYPE(PPAINTSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PPAINTSTRUCT, 64, 4);
}

static void test_pack_PROPENUMPROCA(void)
{
    /* PROPENUMPROCA */
    TEST_TYPE(PROPENUMPROCA, 4, 4);
}

static void test_pack_PROPENUMPROCEXA(void)
{
    /* PROPENUMPROCEXA */
    TEST_TYPE(PROPENUMPROCEXA, 4, 4);
}

static void test_pack_PROPENUMPROCEXW(void)
{
    /* PROPENUMPROCEXW */
    TEST_TYPE(PROPENUMPROCEXW, 4, 4);
}

static void test_pack_PROPENUMPROCW(void)
{
    /* PROPENUMPROCW */
    TEST_TYPE(PROPENUMPROCW, 4, 4);
}

static void test_pack_PTITLEBARINFO(void)
{
    /* PTITLEBARINFO */
    TEST_TYPE(PTITLEBARINFO, 4, 4);
    TEST_TYPE_POINTER(PTITLEBARINFO, 44, 4);
}

static void test_pack_PUSEROBJECTFLAGS(void)
{
    /* PUSEROBJECTFLAGS */
    TEST_TYPE(PUSEROBJECTFLAGS, 4, 4);
    TEST_TYPE_POINTER(PUSEROBJECTFLAGS, 12, 4);
}

static void test_pack_PWINDOWINFO(void)
{
    /* PWINDOWINFO */
    TEST_TYPE(PWINDOWINFO, 4, 4);
    TEST_TYPE_POINTER(PWINDOWINFO, 60, 4);
}

static void test_pack_PWINDOWPLACEMENT(void)
{
    /* PWINDOWPLACEMENT */
    TEST_TYPE(PWINDOWPLACEMENT, 4, 4);
    TEST_TYPE_POINTER(PWINDOWPLACEMENT, 44, 4);
}

static void test_pack_PWINDOWPOS(void)
{
    /* PWINDOWPOS */
    TEST_TYPE(PWINDOWPOS, 4, 4);
    TEST_TYPE_POINTER(PWINDOWPOS, 28, 4);
}

static void test_pack_PWNDCLASSA(void)
{
    /* PWNDCLASSA */
    TEST_TYPE(PWNDCLASSA, 4, 4);
    TEST_TYPE_POINTER(PWNDCLASSA, 40, 4);
}

static void test_pack_PWNDCLASSEXA(void)
{
    /* PWNDCLASSEXA */
    TEST_TYPE(PWNDCLASSEXA, 4, 4);
    TEST_TYPE_POINTER(PWNDCLASSEXA, 48, 4);
}

static void test_pack_PWNDCLASSEXW(void)
{
    /* PWNDCLASSEXW */
    TEST_TYPE(PWNDCLASSEXW, 4, 4);
    TEST_TYPE_POINTER(PWNDCLASSEXW, 48, 4);
}

static void test_pack_PWNDCLASSW(void)
{
    /* PWNDCLASSW */
    TEST_TYPE(PWNDCLASSW, 4, 4);
    TEST_TYPE_POINTER(PWNDCLASSW, 40, 4);
}

static void test_pack_SCROLLINFO(void)
{
    /* SCROLLINFO (pack 4) */
    TEST_TYPE(SCROLLINFO, 28, 4);
    TEST_FIELD(SCROLLINFO, cbSize, 0, 4, 4);
    TEST_FIELD(SCROLLINFO, fMask, 4, 4, 4);
    TEST_FIELD(SCROLLINFO, nMin, 8, 4, 4);
    TEST_FIELD(SCROLLINFO, nMax, 12, 4, 4);
    TEST_FIELD(SCROLLINFO, nPage, 16, 4, 4);
    TEST_FIELD(SCROLLINFO, nPos, 20, 4, 4);
    TEST_FIELD(SCROLLINFO, nTrackPos, 24, 4, 4);
}

static void test_pack_SENDASYNCPROC(void)
{
    /* SENDASYNCPROC */
    TEST_TYPE(SENDASYNCPROC, 4, 4);
}

static void test_pack_SERIALKEYSA(void)
{
    /* SERIALKEYSA (pack 4) */
    TEST_TYPE(SERIALKEYSA, 28, 4);
    TEST_FIELD(SERIALKEYSA, cbSize, 0, 4, 4);
    TEST_FIELD(SERIALKEYSA, dwFlags, 4, 4, 4);
    TEST_FIELD(SERIALKEYSA, lpszActivePort, 8, 4, 4);
    TEST_FIELD(SERIALKEYSA, lpszPort, 12, 4, 4);
    TEST_FIELD(SERIALKEYSA, iBaudRate, 16, 4, 4);
    TEST_FIELD(SERIALKEYSA, iPortState, 20, 4, 4);
    TEST_FIELD(SERIALKEYSA, iActive, 24, 4, 4);
}

static void test_pack_SERIALKEYSW(void)
{
    /* SERIALKEYSW (pack 4) */
    TEST_TYPE(SERIALKEYSW, 28, 4);
    TEST_FIELD(SERIALKEYSW, cbSize, 0, 4, 4);
    TEST_FIELD(SERIALKEYSW, dwFlags, 4, 4, 4);
    TEST_FIELD(SERIALKEYSW, lpszActivePort, 8, 4, 4);
    TEST_FIELD(SERIALKEYSW, lpszPort, 12, 4, 4);
    TEST_FIELD(SERIALKEYSW, iBaudRate, 16, 4, 4);
    TEST_FIELD(SERIALKEYSW, iPortState, 20, 4, 4);
    TEST_FIELD(SERIALKEYSW, iActive, 24, 4, 4);
}

static void test_pack_SOUNDSENTRYA(void)
{
    /* SOUNDSENTRYA (pack 4) */
    TEST_TYPE(SOUNDSENTRYA, 48, 4);
    TEST_FIELD(SOUNDSENTRYA, cbSize, 0, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, dwFlags, 4, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, iFSTextEffect, 8, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, iFSTextEffectMSec, 12, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, iFSTextEffectColorBits, 16, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, iFSGrafEffect, 20, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, iFSGrafEffectMSec, 24, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, iFSGrafEffectColor, 28, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, iWindowsEffect, 32, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, iWindowsEffectMSec, 36, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, lpszWindowsEffectDLL, 40, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, iWindowsEffectOrdinal, 44, 4, 4);
}

static void test_pack_SOUNDSENTRYW(void)
{
    /* SOUNDSENTRYW (pack 4) */
    TEST_TYPE(SOUNDSENTRYW, 48, 4);
    TEST_FIELD(SOUNDSENTRYW, cbSize, 0, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, dwFlags, 4, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, iFSTextEffect, 8, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, iFSTextEffectMSec, 12, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, iFSTextEffectColorBits, 16, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, iFSGrafEffect, 20, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, iFSGrafEffectMSec, 24, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, iFSGrafEffectColor, 28, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, iWindowsEffect, 32, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, iWindowsEffectMSec, 36, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, lpszWindowsEffectDLL, 40, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, iWindowsEffectOrdinal, 44, 4, 4);
}

static void test_pack_STICKYKEYS(void)
{
    /* STICKYKEYS (pack 4) */
    TEST_TYPE(STICKYKEYS, 8, 4);
    TEST_FIELD(STICKYKEYS, cbSize, 0, 4, 4);
    TEST_FIELD(STICKYKEYS, dwFlags, 4, 4, 4);
}

static void test_pack_STYLESTRUCT(void)
{
    /* STYLESTRUCT (pack 4) */
    TEST_TYPE(STYLESTRUCT, 8, 4);
    TEST_FIELD(STYLESTRUCT, styleOld, 0, 4, 4);
    TEST_FIELD(STYLESTRUCT, styleNew, 4, 4, 4);
}

static void test_pack_TIMERPROC(void)
{
    /* TIMERPROC */
    TEST_TYPE(TIMERPROC, 4, 4);
}

static void test_pack_TITLEBARINFO(void)
{
    /* TITLEBARINFO (pack 4) */
    TEST_TYPE(TITLEBARINFO, 44, 4);
    TEST_FIELD(TITLEBARINFO, cbSize, 0, 4, 4);
    TEST_FIELD(TITLEBARINFO, rcTitleBar, 4, 16, 4);
    TEST_FIELD(TITLEBARINFO, rgstate, 20, 24, 4);
}

static void test_pack_TOGGLEKEYS(void)
{
    /* TOGGLEKEYS (pack 4) */
    TEST_TYPE(TOGGLEKEYS, 8, 4);
    TEST_FIELD(TOGGLEKEYS, cbSize, 0, 4, 4);
    TEST_FIELD(TOGGLEKEYS, dwFlags, 4, 4, 4);
}

static void test_pack_TPMPARAMS(void)
{
    /* TPMPARAMS (pack 4) */
    TEST_TYPE(TPMPARAMS, 20, 4);
    TEST_FIELD(TPMPARAMS, cbSize, 0, 4, 4);
    TEST_FIELD(TPMPARAMS, rcExclude, 4, 16, 4);
}

static void test_pack_TRACKMOUSEEVENT(void)
{
    /* TRACKMOUSEEVENT (pack 4) */
    TEST_TYPE(TRACKMOUSEEVENT, 16, 4);
    TEST_FIELD(TRACKMOUSEEVENT, cbSize, 0, 4, 4);
    TEST_FIELD(TRACKMOUSEEVENT, dwFlags, 4, 4, 4);
    TEST_FIELD(TRACKMOUSEEVENT, hwndTrack, 8, 4, 4);
    TEST_FIELD(TRACKMOUSEEVENT, dwHoverTime, 12, 4, 4);
}

static void test_pack_USEROBJECTFLAGS(void)
{
    /* USEROBJECTFLAGS (pack 4) */
    TEST_TYPE(USEROBJECTFLAGS, 12, 4);
    TEST_FIELD(USEROBJECTFLAGS, fInherit, 0, 4, 4);
    TEST_FIELD(USEROBJECTFLAGS, fReserved, 4, 4, 4);
    TEST_FIELD(USEROBJECTFLAGS, dwFlags, 8, 4, 4);
}

static void test_pack_WINDOWINFO(void)
{
    /* WINDOWINFO (pack 4) */
    TEST_TYPE(WINDOWINFO, 60, 4);
    TEST_FIELD(WINDOWINFO, cbSize, 0, 4, 4);
    TEST_FIELD(WINDOWINFO, rcWindow, 4, 16, 4);
    TEST_FIELD(WINDOWINFO, rcClient, 20, 16, 4);
    TEST_FIELD(WINDOWINFO, dwStyle, 36, 4, 4);
    TEST_FIELD(WINDOWINFO, dwExStyle, 40, 4, 4);
    TEST_FIELD(WINDOWINFO, dwWindowStatus, 44, 4, 4);
    TEST_FIELD(WINDOWINFO, cxWindowBorders, 48, 4, 4);
    TEST_FIELD(WINDOWINFO, cyWindowBorders, 52, 4, 4);
    TEST_FIELD(WINDOWINFO, atomWindowType, 56, 2, 2);
    TEST_FIELD(WINDOWINFO, wCreatorVersion, 58, 2, 2);
}

static void test_pack_WINDOWPLACEMENT(void)
{
    /* WINDOWPLACEMENT (pack 4) */
    TEST_TYPE(WINDOWPLACEMENT, 44, 4);
    TEST_FIELD(WINDOWPLACEMENT, length, 0, 4, 4);
    TEST_FIELD(WINDOWPLACEMENT, flags, 4, 4, 4);
    TEST_FIELD(WINDOWPLACEMENT, showCmd, 8, 4, 4);
    TEST_FIELD(WINDOWPLACEMENT, ptMinPosition, 12, 8, 4);
    TEST_FIELD(WINDOWPLACEMENT, ptMaxPosition, 20, 8, 4);
    TEST_FIELD(WINDOWPLACEMENT, rcNormalPosition, 28, 16, 4);
}

static void test_pack_WINDOWPOS(void)
{
    /* WINDOWPOS (pack 4) */
    TEST_TYPE(WINDOWPOS, 28, 4);
    TEST_FIELD(WINDOWPOS, hwnd, 0, 4, 4);
    TEST_FIELD(WINDOWPOS, hwndInsertAfter, 4, 4, 4);
    TEST_FIELD(WINDOWPOS, x, 8, 4, 4);
    TEST_FIELD(WINDOWPOS, y, 12, 4, 4);
    TEST_FIELD(WINDOWPOS, cx, 16, 4, 4);
    TEST_FIELD(WINDOWPOS, cy, 20, 4, 4);
    TEST_FIELD(WINDOWPOS, flags, 24, 4, 4);
}

static void test_pack_WINEVENTPROC(void)
{
    /* WINEVENTPROC */
    TEST_TYPE(WINEVENTPROC, 4, 4);
}

static void test_pack_WINSTAENUMPROCA(void)
{
    /* WINSTAENUMPROCA */
    TEST_TYPE(WINSTAENUMPROCA, 4, 4);
}

static void test_pack_WINSTAENUMPROCW(void)
{
    /* WINSTAENUMPROCW */
    TEST_TYPE(WINSTAENUMPROCW, 4, 4);
}

static void test_pack_WNDCLASSA(void)
{
    /* WNDCLASSA (pack 4) */
    TEST_TYPE(WNDCLASSA, 40, 4);
    TEST_FIELD(WNDCLASSA, style, 0, 4, 4);
    TEST_FIELD(WNDCLASSA, lpfnWndProc, 4, 4, 4);
    TEST_FIELD(WNDCLASSA, cbClsExtra, 8, 4, 4);
    TEST_FIELD(WNDCLASSA, cbWndExtra, 12, 4, 4);
    TEST_FIELD(WNDCLASSA, hInstance, 16, 4, 4);
    TEST_FIELD(WNDCLASSA, hIcon, 20, 4, 4);
    TEST_FIELD(WNDCLASSA, hCursor, 24, 4, 4);
    TEST_FIELD(WNDCLASSA, hbrBackground, 28, 4, 4);
    TEST_FIELD(WNDCLASSA, lpszMenuName, 32, 4, 4);
    TEST_FIELD(WNDCLASSA, lpszClassName, 36, 4, 4);
}

static void test_pack_WNDCLASSEXA(void)
{
    /* WNDCLASSEXA (pack 4) */
    TEST_TYPE(WNDCLASSEXA, 48, 4);
    TEST_FIELD(WNDCLASSEXA, cbSize, 0, 4, 4);
    TEST_FIELD(WNDCLASSEXA, style, 4, 4, 4);
    TEST_FIELD(WNDCLASSEXA, lpfnWndProc, 8, 4, 4);
    TEST_FIELD(WNDCLASSEXA, cbClsExtra, 12, 4, 4);
    TEST_FIELD(WNDCLASSEXA, cbWndExtra, 16, 4, 4);
    TEST_FIELD(WNDCLASSEXA, hInstance, 20, 4, 4);
    TEST_FIELD(WNDCLASSEXA, hIcon, 24, 4, 4);
    TEST_FIELD(WNDCLASSEXA, hCursor, 28, 4, 4);
    TEST_FIELD(WNDCLASSEXA, hbrBackground, 32, 4, 4);
    TEST_FIELD(WNDCLASSEXA, lpszMenuName, 36, 4, 4);
    TEST_FIELD(WNDCLASSEXA, lpszClassName, 40, 4, 4);
    TEST_FIELD(WNDCLASSEXA, hIconSm, 44, 4, 4);
}

static void test_pack_WNDCLASSEXW(void)
{
    /* WNDCLASSEXW (pack 4) */
    TEST_TYPE(WNDCLASSEXW, 48, 4);
    TEST_FIELD(WNDCLASSEXW, cbSize, 0, 4, 4);
    TEST_FIELD(WNDCLASSEXW, style, 4, 4, 4);
    TEST_FIELD(WNDCLASSEXW, lpfnWndProc, 8, 4, 4);
    TEST_FIELD(WNDCLASSEXW, cbClsExtra, 12, 4, 4);
    TEST_FIELD(WNDCLASSEXW, cbWndExtra, 16, 4, 4);
    TEST_FIELD(WNDCLASSEXW, hInstance, 20, 4, 4);
    TEST_FIELD(WNDCLASSEXW, hIcon, 24, 4, 4);
    TEST_FIELD(WNDCLASSEXW, hCursor, 28, 4, 4);
    TEST_FIELD(WNDCLASSEXW, hbrBackground, 32, 4, 4);
    TEST_FIELD(WNDCLASSEXW, lpszMenuName, 36, 4, 4);
    TEST_FIELD(WNDCLASSEXW, lpszClassName, 40, 4, 4);
    TEST_FIELD(WNDCLASSEXW, hIconSm, 44, 4, 4);
}

static void test_pack_WNDCLASSW(void)
{
    /* WNDCLASSW (pack 4) */
    TEST_TYPE(WNDCLASSW, 40, 4);
    TEST_FIELD(WNDCLASSW, style, 0, 4, 4);
    TEST_FIELD(WNDCLASSW, lpfnWndProc, 4, 4, 4);
    TEST_FIELD(WNDCLASSW, cbClsExtra, 8, 4, 4);
    TEST_FIELD(WNDCLASSW, cbWndExtra, 12, 4, 4);
    TEST_FIELD(WNDCLASSW, hInstance, 16, 4, 4);
    TEST_FIELD(WNDCLASSW, hIcon, 20, 4, 4);
    TEST_FIELD(WNDCLASSW, hCursor, 24, 4, 4);
    TEST_FIELD(WNDCLASSW, hbrBackground, 28, 4, 4);
    TEST_FIELD(WNDCLASSW, lpszMenuName, 32, 4, 4);
    TEST_FIELD(WNDCLASSW, lpszClassName, 36, 4, 4);
}

static void test_pack_WNDENUMPROC(void)
{
    /* WNDENUMPROC */
    TEST_TYPE(WNDENUMPROC, 4, 4);
}

static void test_pack_WNDPROC(void)
{
    /* WNDPROC */
    TEST_TYPE(WNDPROC, 4, 4);
}

static void test_pack(void)
{
    test_pack_ACCESSTIMEOUT();
    test_pack_ANIMATIONINFO();
    test_pack_CBTACTIVATESTRUCT();
    test_pack_CBT_CREATEWNDA();
    test_pack_CBT_CREATEWNDW();
    test_pack_CLIENTCREATESTRUCT();
    test_pack_COMBOBOXINFO();
    test_pack_COMPAREITEMSTRUCT();
    test_pack_COPYDATASTRUCT();
    test_pack_CREATESTRUCTA();
    test_pack_CREATESTRUCTW();
    test_pack_CURSORINFO();
    test_pack_CWPRETSTRUCT();
    test_pack_CWPSTRUCT();
    test_pack_DEBUGHOOKINFO();
    test_pack_DELETEITEMSTRUCT();
    test_pack_DESKTOPENUMPROCA();
    test_pack_DESKTOPENUMPROCW();
    test_pack_DLGITEMTEMPLATE();
    test_pack_DLGPROC();
    test_pack_DLGTEMPLATE();
    test_pack_DRAWITEMSTRUCT();
    test_pack_DRAWSTATEPROC();
    test_pack_DRAWTEXTPARAMS();
    test_pack_EDITWORDBREAKPROCA();
    test_pack_EDITWORDBREAKPROCW();
    test_pack_EVENTMSG();
    test_pack_FILTERKEYS();
    test_pack_FLASHWINFO();
    test_pack_GRAYSTRINGPROC();
    test_pack_GUITHREADINFO();
    test_pack_HARDWAREHOOKSTRUCT();
    test_pack_HARDWAREINPUT();
    test_pack_HDEVNOTIFY();
    test_pack_HDWP();
    test_pack_HELPINFO();
    test_pack_HELPWININFOA();
    test_pack_HELPWININFOW();
    test_pack_HIGHCONTRASTA();
    test_pack_HIGHCONTRASTW();
    test_pack_HOOKPROC();
    test_pack_ICONINFO();
    test_pack_ICONMETRICSA();
    test_pack_ICONMETRICSW();
    test_pack_INPUT();
    test_pack_KBDLLHOOKSTRUCT();
    test_pack_KEYBDINPUT();
    test_pack_LPACCESSTIMEOUT();
    test_pack_LPANIMATIONINFO();
    test_pack_LPCBTACTIVATESTRUCT();
    test_pack_LPCBT_CREATEWNDA();
    test_pack_LPCBT_CREATEWNDW();
    test_pack_LPCDLGTEMPLATEA();
    test_pack_LPCDLGTEMPLATEW();
    test_pack_LPCLIENTCREATESTRUCT();
    test_pack_LPCMENUINFO();
    test_pack_LPCMENUITEMINFOA();
    test_pack_LPCMENUITEMINFOW();
    test_pack_LPCOMBOBOXINFO();
    test_pack_LPCOMPAREITEMSTRUCT();
    test_pack_LPCREATESTRUCTA();
    test_pack_LPCREATESTRUCTW();
    test_pack_LPCSCROLLINFO();
    test_pack_LPCURSORINFO();
    test_pack_LPCWPRETSTRUCT();
    test_pack_LPCWPSTRUCT();
    test_pack_LPDEBUGHOOKINFO();
    test_pack_LPDELETEITEMSTRUCT();
    test_pack_LPDLGITEMTEMPLATEA();
    test_pack_LPDLGITEMTEMPLATEW();
    test_pack_LPDLGTEMPLATEA();
    test_pack_LPDLGTEMPLATEW();
    test_pack_LPDRAWITEMSTRUCT();
    test_pack_LPDRAWTEXTPARAMS();
    test_pack_LPEVENTMSG();
    test_pack_LPFILTERKEYS();
    test_pack_LPGUITHREADINFO();
    test_pack_LPHARDWAREHOOKSTRUCT();
    test_pack_LPHARDWAREINPUT();
    test_pack_LPHELPINFO();
    test_pack_LPHELPWININFOA();
    test_pack_LPHELPWININFOW();
    test_pack_LPHIGHCONTRASTA();
    test_pack_LPHIGHCONTRASTW();
    test_pack_LPICONMETRICSA();
    test_pack_LPICONMETRICSW();
    test_pack_LPINPUT();
    test_pack_LPKBDLLHOOKSTRUCT();
    test_pack_LPKEYBDINPUT();
    test_pack_LPMDICREATESTRUCTA();
    test_pack_LPMDICREATESTRUCTW();
    test_pack_LPMDINEXTMENU();
    test_pack_LPMEASUREITEMSTRUCT();
    test_pack_LPMENUINFO();
    test_pack_LPMENUITEMINFOA();
    test_pack_LPMENUITEMINFOW();
    test_pack_LPMINIMIZEDMETRICS();
    test_pack_LPMINMAXINFO();
    test_pack_LPMONITORINFO();
    test_pack_LPMONITORINFOEXA();
    test_pack_LPMONITORINFOEXW();
    test_pack_LPMOUSEHOOKSTRUCT();
    test_pack_LPMOUSEINPUT();
    test_pack_LPMOUSEKEYS();
    test_pack_LPMSG();
    test_pack_LPMSGBOXPARAMSA();
    test_pack_LPMSGBOXPARAMSW();
    test_pack_LPMSLLHOOKSTRUCT();
    test_pack_LPMULTIKEYHELPA();
    test_pack_LPMULTIKEYHELPW();
    test_pack_LPNCCALCSIZE_PARAMS();
    test_pack_LPNMHDR();
    test_pack_LPNONCLIENTMETRICSA();
    test_pack_LPNONCLIENTMETRICSW();
    test_pack_LPPAINTSTRUCT();
    test_pack_LPSCROLLINFO();
    test_pack_LPSERIALKEYSA();
    test_pack_LPSERIALKEYSW();
    test_pack_LPSOUNDSENTRYA();
    test_pack_LPSOUNDSENTRYW();
    test_pack_LPSTICKYKEYS();
    test_pack_LPSTYLESTRUCT();
    test_pack_LPTITLEBARINFO();
    test_pack_LPTOGGLEKEYS();
    test_pack_LPTPMPARAMS();
    test_pack_LPTRACKMOUSEEVENT();
    test_pack_LPWINDOWINFO();
    test_pack_LPWINDOWPLACEMENT();
    test_pack_LPWINDOWPOS();
    test_pack_LPWNDCLASSA();
    test_pack_LPWNDCLASSEXA();
    test_pack_LPWNDCLASSEXW();
    test_pack_LPWNDCLASSW();
    test_pack_MDICREATESTRUCTA();
    test_pack_MDICREATESTRUCTW();
    test_pack_MDINEXTMENU();
    test_pack_MEASUREITEMSTRUCT();
    test_pack_MENUINFO();
    test_pack_MENUITEMINFOA();
    test_pack_MENUITEMINFOW();
    test_pack_MENUITEMTEMPLATE();
    test_pack_MENUITEMTEMPLATEHEADER();
    test_pack_MINIMIZEDMETRICS();
    test_pack_MINMAXINFO();
    test_pack_MONITORENUMPROC();
    test_pack_MONITORINFO();
    test_pack_MONITORINFOEXA();
    test_pack_MONITORINFOEXW();
    test_pack_MOUSEHOOKSTRUCT();
    test_pack_MOUSEINPUT();
    test_pack_MOUSEKEYS();
    test_pack_MSG();
    test_pack_MSGBOXCALLBACK();
    test_pack_MSGBOXPARAMSA();
    test_pack_MSGBOXPARAMSW();
    test_pack_MSLLHOOKSTRUCT();
    test_pack_MULTIKEYHELPA();
    test_pack_MULTIKEYHELPW();
    test_pack_NAMEENUMPROCA();
    test_pack_NAMEENUMPROCW();
    test_pack_NCCALCSIZE_PARAMS();
    test_pack_NMHDR();
    test_pack_NONCLIENTMETRICSA();
    test_pack_NONCLIENTMETRICSW();
    test_pack_PAINTSTRUCT();
    test_pack_PCOMBOBOXINFO();
    test_pack_PCOMPAREITEMSTRUCT();
    test_pack_PCOPYDATASTRUCT();
    test_pack_PCURSORINFO();
    test_pack_PCWPRETSTRUCT();
    test_pack_PCWPSTRUCT();
    test_pack_PDEBUGHOOKINFO();
    test_pack_PDELETEITEMSTRUCT();
    test_pack_PDLGITEMTEMPLATEA();
    test_pack_PDLGITEMTEMPLATEW();
    test_pack_PDRAWITEMSTRUCT();
    test_pack_PEVENTMSG();
    test_pack_PFLASHWINFO();
    test_pack_PGUITHREADINFO();
    test_pack_PHARDWAREHOOKSTRUCT();
    test_pack_PHARDWAREINPUT();
    test_pack_PHDEVNOTIFY();
    test_pack_PHELPWININFOA();
    test_pack_PHELPWININFOW();
    test_pack_PICONINFO();
    test_pack_PICONMETRICSA();
    test_pack_PICONMETRICSW();
    test_pack_PINPUT();
    test_pack_PKBDLLHOOKSTRUCT();
    test_pack_PKEYBDINPUT();
    test_pack_PMDINEXTMENU();
    test_pack_PMEASUREITEMSTRUCT();
    test_pack_PMENUITEMTEMPLATE();
    test_pack_PMENUITEMTEMPLATEHEADER();
    test_pack_PMINIMIZEDMETRICS();
    test_pack_PMINMAXINFO();
    test_pack_PMOUSEHOOKSTRUCT();
    test_pack_PMOUSEINPUT();
    test_pack_PMSG();
    test_pack_PMSGBOXPARAMSA();
    test_pack_PMSGBOXPARAMSW();
    test_pack_PMSLLHOOKSTRUCT();
    test_pack_PMULTIKEYHELPA();
    test_pack_PMULTIKEYHELPW();
    test_pack_PNONCLIENTMETRICSA();
    test_pack_PNONCLIENTMETRICSW();
    test_pack_PPAINTSTRUCT();
    test_pack_PROPENUMPROCA();
    test_pack_PROPENUMPROCEXA();
    test_pack_PROPENUMPROCEXW();
    test_pack_PROPENUMPROCW();
    test_pack_PTITLEBARINFO();
    test_pack_PUSEROBJECTFLAGS();
    test_pack_PWINDOWINFO();
    test_pack_PWINDOWPLACEMENT();
    test_pack_PWINDOWPOS();
    test_pack_PWNDCLASSA();
    test_pack_PWNDCLASSEXA();
    test_pack_PWNDCLASSEXW();
    test_pack_PWNDCLASSW();
    test_pack_SCROLLINFO();
    test_pack_SENDASYNCPROC();
    test_pack_SERIALKEYSA();
    test_pack_SERIALKEYSW();
    test_pack_SOUNDSENTRYA();
    test_pack_SOUNDSENTRYW();
    test_pack_STICKYKEYS();
    test_pack_STYLESTRUCT();
    test_pack_TIMERPROC();
    test_pack_TITLEBARINFO();
    test_pack_TOGGLEKEYS();
    test_pack_TPMPARAMS();
    test_pack_TRACKMOUSEEVENT();
    test_pack_USEROBJECTFLAGS();
    test_pack_WINDOWINFO();
    test_pack_WINDOWPLACEMENT();
    test_pack_WINDOWPOS();
    test_pack_WINEVENTPROC();
    test_pack_WINSTAENUMPROCA();
    test_pack_WINSTAENUMPROCW();
    test_pack_WNDCLASSA();
    test_pack_WNDCLASSEXA();
    test_pack_WNDCLASSEXW();
    test_pack_WNDCLASSW();
    test_pack_WNDENUMPROC();
    test_pack_WNDPROC();
}

START_TEST(generated)
{
    test_pack();
}
