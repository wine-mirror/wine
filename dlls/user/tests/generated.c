/* File generated automatically from tools/winapi/test.dat; do not edit! */
/* This file can be copied, modified and distributed without restriction. */

/*
 * Unit tests for data structure packing
 */

#include <stdio.h>

#include "wine/test.h"
#include "winbase.h"
#include "winuser.h"

void test_pack(void)
{
    /* ACCESSTIMEOUT */
    ok(FIELD_OFFSET(ACCESSTIMEOUT, cbSize) == 0,
       "FIELD_OFFSET(ACCESSTIMEOUT, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(ACCESSTIMEOUT, cbSize)); /* UINT */
    ok(FIELD_OFFSET(ACCESSTIMEOUT, dwFlags) == 4,
       "FIELD_OFFSET(ACCESSTIMEOUT, dwFlags) == %ld (expected 4)",
       FIELD_OFFSET(ACCESSTIMEOUT, dwFlags)); /* DWORD */
    ok(FIELD_OFFSET(ACCESSTIMEOUT, iTimeOutMSec) == 8,
       "FIELD_OFFSET(ACCESSTIMEOUT, iTimeOutMSec) == %ld (expected 8)",
       FIELD_OFFSET(ACCESSTIMEOUT, iTimeOutMSec)); /* DWORD */
    ok(sizeof(ACCESSTIMEOUT) == 12, "sizeof(ACCESSTIMEOUT) == %d (expected 12)", sizeof(ACCESSTIMEOUT));

    /* ANIMATIONINFO */
    ok(FIELD_OFFSET(ANIMATIONINFO, cbSize) == 0,
       "FIELD_OFFSET(ANIMATIONINFO, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(ANIMATIONINFO, cbSize)); /* UINT */
    ok(FIELD_OFFSET(ANIMATIONINFO, iMinAnimate) == 4,
       "FIELD_OFFSET(ANIMATIONINFO, iMinAnimate) == %ld (expected 4)",
       FIELD_OFFSET(ANIMATIONINFO, iMinAnimate)); /* INT */
    ok(sizeof(ANIMATIONINFO) == 8, "sizeof(ANIMATIONINFO) == %d (expected 8)", sizeof(ANIMATIONINFO));

    /* CBTACTIVATESTRUCT */
    ok(FIELD_OFFSET(CBTACTIVATESTRUCT, fMouse) == 0,
       "FIELD_OFFSET(CBTACTIVATESTRUCT, fMouse) == %ld (expected 0)",
       FIELD_OFFSET(CBTACTIVATESTRUCT, fMouse)); /* BOOL */
    ok(FIELD_OFFSET(CBTACTIVATESTRUCT, hWndActive) == 4,
       "FIELD_OFFSET(CBTACTIVATESTRUCT, hWndActive) == %ld (expected 4)",
       FIELD_OFFSET(CBTACTIVATESTRUCT, hWndActive)); /* HWND */
    ok(sizeof(CBTACTIVATESTRUCT) == 8, "sizeof(CBTACTIVATESTRUCT) == %d (expected 8)", sizeof(CBTACTIVATESTRUCT));

    /* CBT_CREATEWNDA */
    ok(FIELD_OFFSET(CBT_CREATEWNDA, lpcs) == 0,
       "FIELD_OFFSET(CBT_CREATEWNDA, lpcs) == %ld (expected 0)",
       FIELD_OFFSET(CBT_CREATEWNDA, lpcs)); /* CREATESTRUCTA * */
    ok(FIELD_OFFSET(CBT_CREATEWNDA, hwndInsertAfter) == 4,
       "FIELD_OFFSET(CBT_CREATEWNDA, hwndInsertAfter) == %ld (expected 4)",
       FIELD_OFFSET(CBT_CREATEWNDA, hwndInsertAfter)); /* HWND */
    ok(sizeof(CBT_CREATEWNDA) == 8, "sizeof(CBT_CREATEWNDA) == %d (expected 8)", sizeof(CBT_CREATEWNDA));

    /* CBT_CREATEWNDW */
    ok(FIELD_OFFSET(CBT_CREATEWNDW, lpcs) == 0,
       "FIELD_OFFSET(CBT_CREATEWNDW, lpcs) == %ld (expected 0)",
       FIELD_OFFSET(CBT_CREATEWNDW, lpcs)); /* CREATESTRUCTW * */
    ok(FIELD_OFFSET(CBT_CREATEWNDW, hwndInsertAfter) == 4,
       "FIELD_OFFSET(CBT_CREATEWNDW, hwndInsertAfter) == %ld (expected 4)",
       FIELD_OFFSET(CBT_CREATEWNDW, hwndInsertAfter)); /* HWND */
    ok(sizeof(CBT_CREATEWNDW) == 8, "sizeof(CBT_CREATEWNDW) == %d (expected 8)", sizeof(CBT_CREATEWNDW));

    /* CLIENTCREATESTRUCT */
    ok(FIELD_OFFSET(CLIENTCREATESTRUCT, hWindowMenu) == 0,
       "FIELD_OFFSET(CLIENTCREATESTRUCT, hWindowMenu) == %ld (expected 0)",
       FIELD_OFFSET(CLIENTCREATESTRUCT, hWindowMenu)); /* HMENU */
    ok(FIELD_OFFSET(CLIENTCREATESTRUCT, idFirstChild) == 4,
       "FIELD_OFFSET(CLIENTCREATESTRUCT, idFirstChild) == %ld (expected 4)",
       FIELD_OFFSET(CLIENTCREATESTRUCT, idFirstChild)); /* UINT */
    ok(sizeof(CLIENTCREATESTRUCT) == 8, "sizeof(CLIENTCREATESTRUCT) == %d (expected 8)", sizeof(CLIENTCREATESTRUCT));

    /* COMPAREITEMSTRUCT */
    ok(FIELD_OFFSET(COMPAREITEMSTRUCT, CtlType) == 0,
       "FIELD_OFFSET(COMPAREITEMSTRUCT, CtlType) == %ld (expected 0)",
       FIELD_OFFSET(COMPAREITEMSTRUCT, CtlType)); /* UINT */
    ok(FIELD_OFFSET(COMPAREITEMSTRUCT, CtlID) == 4,
       "FIELD_OFFSET(COMPAREITEMSTRUCT, CtlID) == %ld (expected 4)",
       FIELD_OFFSET(COMPAREITEMSTRUCT, CtlID)); /* UINT */
    ok(FIELD_OFFSET(COMPAREITEMSTRUCT, hwndItem) == 8,
       "FIELD_OFFSET(COMPAREITEMSTRUCT, hwndItem) == %ld (expected 8)",
       FIELD_OFFSET(COMPAREITEMSTRUCT, hwndItem)); /* HWND */
    ok(FIELD_OFFSET(COMPAREITEMSTRUCT, itemID1) == 12,
       "FIELD_OFFSET(COMPAREITEMSTRUCT, itemID1) == %ld (expected 12)",
       FIELD_OFFSET(COMPAREITEMSTRUCT, itemID1)); /* UINT */
    ok(FIELD_OFFSET(COMPAREITEMSTRUCT, itemData1) == 16,
       "FIELD_OFFSET(COMPAREITEMSTRUCT, itemData1) == %ld (expected 16)",
       FIELD_OFFSET(COMPAREITEMSTRUCT, itemData1)); /* DWORD */
    ok(FIELD_OFFSET(COMPAREITEMSTRUCT, itemID2) == 20,
       "FIELD_OFFSET(COMPAREITEMSTRUCT, itemID2) == %ld (expected 20)",
       FIELD_OFFSET(COMPAREITEMSTRUCT, itemID2)); /* UINT */
    ok(FIELD_OFFSET(COMPAREITEMSTRUCT, itemData2) == 24,
       "FIELD_OFFSET(COMPAREITEMSTRUCT, itemData2) == %ld (expected 24)",
       FIELD_OFFSET(COMPAREITEMSTRUCT, itemData2)); /* DWORD */
    ok(FIELD_OFFSET(COMPAREITEMSTRUCT, dwLocaleId) == 28,
       "FIELD_OFFSET(COMPAREITEMSTRUCT, dwLocaleId) == %ld (expected 28)",
       FIELD_OFFSET(COMPAREITEMSTRUCT, dwLocaleId)); /* DWORD */
    ok(sizeof(COMPAREITEMSTRUCT) == 32, "sizeof(COMPAREITEMSTRUCT) == %d (expected 32)", sizeof(COMPAREITEMSTRUCT));

    /* COPYDATASTRUCT */
    ok(FIELD_OFFSET(COPYDATASTRUCT, dwData) == 0,
       "FIELD_OFFSET(COPYDATASTRUCT, dwData) == %ld (expected 0)",
       FIELD_OFFSET(COPYDATASTRUCT, dwData)); /* DWORD */
    ok(FIELD_OFFSET(COPYDATASTRUCT, cbData) == 4,
       "FIELD_OFFSET(COPYDATASTRUCT, cbData) == %ld (expected 4)",
       FIELD_OFFSET(COPYDATASTRUCT, cbData)); /* DWORD */
    ok(FIELD_OFFSET(COPYDATASTRUCT, lpData) == 8,
       "FIELD_OFFSET(COPYDATASTRUCT, lpData) == %ld (expected 8)",
       FIELD_OFFSET(COPYDATASTRUCT, lpData)); /* LPVOID */
    ok(sizeof(COPYDATASTRUCT) == 12, "sizeof(COPYDATASTRUCT) == %d (expected 12)", sizeof(COPYDATASTRUCT));

    /* CREATESTRUCTA */
    ok(FIELD_OFFSET(CREATESTRUCTA, lpCreateParams) == 0,
       "FIELD_OFFSET(CREATESTRUCTA, lpCreateParams) == %ld (expected 0)",
       FIELD_OFFSET(CREATESTRUCTA, lpCreateParams)); /* LPVOID */
    ok(FIELD_OFFSET(CREATESTRUCTA, hInstance) == 4,
       "FIELD_OFFSET(CREATESTRUCTA, hInstance) == %ld (expected 4)",
       FIELD_OFFSET(CREATESTRUCTA, hInstance)); /* HINSTANCE */
    ok(FIELD_OFFSET(CREATESTRUCTA, hMenu) == 8,
       "FIELD_OFFSET(CREATESTRUCTA, hMenu) == %ld (expected 8)",
       FIELD_OFFSET(CREATESTRUCTA, hMenu)); /* HMENU */
    ok(FIELD_OFFSET(CREATESTRUCTA, hwndParent) == 12,
       "FIELD_OFFSET(CREATESTRUCTA, hwndParent) == %ld (expected 12)",
       FIELD_OFFSET(CREATESTRUCTA, hwndParent)); /* HWND */
    ok(FIELD_OFFSET(CREATESTRUCTA, cy) == 16,
       "FIELD_OFFSET(CREATESTRUCTA, cy) == %ld (expected 16)",
       FIELD_OFFSET(CREATESTRUCTA, cy)); /* INT */
    ok(FIELD_OFFSET(CREATESTRUCTA, cx) == 20,
       "FIELD_OFFSET(CREATESTRUCTA, cx) == %ld (expected 20)",
       FIELD_OFFSET(CREATESTRUCTA, cx)); /* INT */
    ok(FIELD_OFFSET(CREATESTRUCTA, y) == 24,
       "FIELD_OFFSET(CREATESTRUCTA, y) == %ld (expected 24)",
       FIELD_OFFSET(CREATESTRUCTA, y)); /* INT */
    ok(FIELD_OFFSET(CREATESTRUCTA, x) == 28,
       "FIELD_OFFSET(CREATESTRUCTA, x) == %ld (expected 28)",
       FIELD_OFFSET(CREATESTRUCTA, x)); /* INT */
    ok(FIELD_OFFSET(CREATESTRUCTA, style) == 32,
       "FIELD_OFFSET(CREATESTRUCTA, style) == %ld (expected 32)",
       FIELD_OFFSET(CREATESTRUCTA, style)); /* LONG */
    ok(FIELD_OFFSET(CREATESTRUCTA, lpszName) == 36,
       "FIELD_OFFSET(CREATESTRUCTA, lpszName) == %ld (expected 36)",
       FIELD_OFFSET(CREATESTRUCTA, lpszName)); /* LPCSTR */
    ok(FIELD_OFFSET(CREATESTRUCTA, lpszClass) == 40,
       "FIELD_OFFSET(CREATESTRUCTA, lpszClass) == %ld (expected 40)",
       FIELD_OFFSET(CREATESTRUCTA, lpszClass)); /* LPCSTR */
    ok(FIELD_OFFSET(CREATESTRUCTA, dwExStyle) == 44,
       "FIELD_OFFSET(CREATESTRUCTA, dwExStyle) == %ld (expected 44)",
       FIELD_OFFSET(CREATESTRUCTA, dwExStyle)); /* DWORD */
    ok(sizeof(CREATESTRUCTA) == 48, "sizeof(CREATESTRUCTA) == %d (expected 48)", sizeof(CREATESTRUCTA));

    /* CREATESTRUCTW */
    ok(FIELD_OFFSET(CREATESTRUCTW, lpCreateParams) == 0,
       "FIELD_OFFSET(CREATESTRUCTW, lpCreateParams) == %ld (expected 0)",
       FIELD_OFFSET(CREATESTRUCTW, lpCreateParams)); /* LPVOID */
    ok(FIELD_OFFSET(CREATESTRUCTW, hInstance) == 4,
       "FIELD_OFFSET(CREATESTRUCTW, hInstance) == %ld (expected 4)",
       FIELD_OFFSET(CREATESTRUCTW, hInstance)); /* HINSTANCE */
    ok(FIELD_OFFSET(CREATESTRUCTW, hMenu) == 8,
       "FIELD_OFFSET(CREATESTRUCTW, hMenu) == %ld (expected 8)",
       FIELD_OFFSET(CREATESTRUCTW, hMenu)); /* HMENU */
    ok(FIELD_OFFSET(CREATESTRUCTW, hwndParent) == 12,
       "FIELD_OFFSET(CREATESTRUCTW, hwndParent) == %ld (expected 12)",
       FIELD_OFFSET(CREATESTRUCTW, hwndParent)); /* HWND */
    ok(FIELD_OFFSET(CREATESTRUCTW, cy) == 16,
       "FIELD_OFFSET(CREATESTRUCTW, cy) == %ld (expected 16)",
       FIELD_OFFSET(CREATESTRUCTW, cy)); /* INT */
    ok(FIELD_OFFSET(CREATESTRUCTW, cx) == 20,
       "FIELD_OFFSET(CREATESTRUCTW, cx) == %ld (expected 20)",
       FIELD_OFFSET(CREATESTRUCTW, cx)); /* INT */
    ok(FIELD_OFFSET(CREATESTRUCTW, y) == 24,
       "FIELD_OFFSET(CREATESTRUCTW, y) == %ld (expected 24)",
       FIELD_OFFSET(CREATESTRUCTW, y)); /* INT */
    ok(FIELD_OFFSET(CREATESTRUCTW, x) == 28,
       "FIELD_OFFSET(CREATESTRUCTW, x) == %ld (expected 28)",
       FIELD_OFFSET(CREATESTRUCTW, x)); /* INT */
    ok(FIELD_OFFSET(CREATESTRUCTW, style) == 32,
       "FIELD_OFFSET(CREATESTRUCTW, style) == %ld (expected 32)",
       FIELD_OFFSET(CREATESTRUCTW, style)); /* LONG */
    ok(FIELD_OFFSET(CREATESTRUCTW, lpszName) == 36,
       "FIELD_OFFSET(CREATESTRUCTW, lpszName) == %ld (expected 36)",
       FIELD_OFFSET(CREATESTRUCTW, lpszName)); /* LPCWSTR */
    ok(FIELD_OFFSET(CREATESTRUCTW, lpszClass) == 40,
       "FIELD_OFFSET(CREATESTRUCTW, lpszClass) == %ld (expected 40)",
       FIELD_OFFSET(CREATESTRUCTW, lpszClass)); /* LPCWSTR */
    ok(FIELD_OFFSET(CREATESTRUCTW, dwExStyle) == 44,
       "FIELD_OFFSET(CREATESTRUCTW, dwExStyle) == %ld (expected 44)",
       FIELD_OFFSET(CREATESTRUCTW, dwExStyle)); /* DWORD */
    ok(sizeof(CREATESTRUCTW) == 48, "sizeof(CREATESTRUCTW) == %d (expected 48)", sizeof(CREATESTRUCTW));

    /* CURSORINFO */
    ok(FIELD_OFFSET(CURSORINFO, cbSize) == 0,
       "FIELD_OFFSET(CURSORINFO, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(CURSORINFO, cbSize)); /* DWORD */
    ok(FIELD_OFFSET(CURSORINFO, flags) == 4,
       "FIELD_OFFSET(CURSORINFO, flags) == %ld (expected 4)",
       FIELD_OFFSET(CURSORINFO, flags)); /* DWORD */
    ok(FIELD_OFFSET(CURSORINFO, hCursor) == 8,
       "FIELD_OFFSET(CURSORINFO, hCursor) == %ld (expected 8)",
       FIELD_OFFSET(CURSORINFO, hCursor)); /* HCURSOR */
    ok(FIELD_OFFSET(CURSORINFO, ptScreenPos) == 12,
       "FIELD_OFFSET(CURSORINFO, ptScreenPos) == %ld (expected 12)",
       FIELD_OFFSET(CURSORINFO, ptScreenPos)); /* POINT */
    ok(sizeof(CURSORINFO) == 20, "sizeof(CURSORINFO) == %d (expected 20)", sizeof(CURSORINFO));

    /* CWPRETSTRUCT */
    ok(FIELD_OFFSET(CWPRETSTRUCT, lResult) == 0,
       "FIELD_OFFSET(CWPRETSTRUCT, lResult) == %ld (expected 0)",
       FIELD_OFFSET(CWPRETSTRUCT, lResult)); /* LRESULT */
    ok(FIELD_OFFSET(CWPRETSTRUCT, lParam) == 4,
       "FIELD_OFFSET(CWPRETSTRUCT, lParam) == %ld (expected 4)",
       FIELD_OFFSET(CWPRETSTRUCT, lParam)); /* LPARAM */
    ok(FIELD_OFFSET(CWPRETSTRUCT, wParam) == 8,
       "FIELD_OFFSET(CWPRETSTRUCT, wParam) == %ld (expected 8)",
       FIELD_OFFSET(CWPRETSTRUCT, wParam)); /* WPARAM */
    ok(FIELD_OFFSET(CWPRETSTRUCT, message) == 12,
       "FIELD_OFFSET(CWPRETSTRUCT, message) == %ld (expected 12)",
       FIELD_OFFSET(CWPRETSTRUCT, message)); /* DWORD */
    ok(FIELD_OFFSET(CWPRETSTRUCT, hwnd) == 16,
       "FIELD_OFFSET(CWPRETSTRUCT, hwnd) == %ld (expected 16)",
       FIELD_OFFSET(CWPRETSTRUCT, hwnd)); /* HWND */
    ok(sizeof(CWPRETSTRUCT) == 20, "sizeof(CWPRETSTRUCT) == %d (expected 20)", sizeof(CWPRETSTRUCT));

    /* CWPSTRUCT */
    ok(FIELD_OFFSET(CWPSTRUCT, lParam) == 0,
       "FIELD_OFFSET(CWPSTRUCT, lParam) == %ld (expected 0)",
       FIELD_OFFSET(CWPSTRUCT, lParam)); /* LPARAM */
    ok(FIELD_OFFSET(CWPSTRUCT, wParam) == 4,
       "FIELD_OFFSET(CWPSTRUCT, wParam) == %ld (expected 4)",
       FIELD_OFFSET(CWPSTRUCT, wParam)); /* WPARAM */
    ok(FIELD_OFFSET(CWPSTRUCT, message) == 8,
       "FIELD_OFFSET(CWPSTRUCT, message) == %ld (expected 8)",
       FIELD_OFFSET(CWPSTRUCT, message)); /* UINT */
    ok(FIELD_OFFSET(CWPSTRUCT, hwnd) == 12,
       "FIELD_OFFSET(CWPSTRUCT, hwnd) == %ld (expected 12)",
       FIELD_OFFSET(CWPSTRUCT, hwnd)); /* HWND */
    ok(sizeof(CWPSTRUCT) == 16, "sizeof(CWPSTRUCT) == %d (expected 16)", sizeof(CWPSTRUCT));

    /* DEBUGHOOKINFO */
    ok(FIELD_OFFSET(DEBUGHOOKINFO, idThread) == 0,
       "FIELD_OFFSET(DEBUGHOOKINFO, idThread) == %ld (expected 0)",
       FIELD_OFFSET(DEBUGHOOKINFO, idThread)); /* DWORD */
    ok(FIELD_OFFSET(DEBUGHOOKINFO, idThreadInstaller) == 4,
       "FIELD_OFFSET(DEBUGHOOKINFO, idThreadInstaller) == %ld (expected 4)",
       FIELD_OFFSET(DEBUGHOOKINFO, idThreadInstaller)); /* DWORD */
    ok(FIELD_OFFSET(DEBUGHOOKINFO, lParam) == 8,
       "FIELD_OFFSET(DEBUGHOOKINFO, lParam) == %ld (expected 8)",
       FIELD_OFFSET(DEBUGHOOKINFO, lParam)); /* LPARAM */
    ok(FIELD_OFFSET(DEBUGHOOKINFO, wParam) == 12,
       "FIELD_OFFSET(DEBUGHOOKINFO, wParam) == %ld (expected 12)",
       FIELD_OFFSET(DEBUGHOOKINFO, wParam)); /* WPARAM */
    ok(FIELD_OFFSET(DEBUGHOOKINFO, code) == 16,
       "FIELD_OFFSET(DEBUGHOOKINFO, code) == %ld (expected 16)",
       FIELD_OFFSET(DEBUGHOOKINFO, code)); /* INT */
    ok(sizeof(DEBUGHOOKINFO) == 20, "sizeof(DEBUGHOOKINFO) == %d (expected 20)", sizeof(DEBUGHOOKINFO));

    /* DELETEITEMSTRUCT */
    ok(FIELD_OFFSET(DELETEITEMSTRUCT, CtlType) == 0,
       "FIELD_OFFSET(DELETEITEMSTRUCT, CtlType) == %ld (expected 0)",
       FIELD_OFFSET(DELETEITEMSTRUCT, CtlType)); /* UINT */
    ok(FIELD_OFFSET(DELETEITEMSTRUCT, CtlID) == 4,
       "FIELD_OFFSET(DELETEITEMSTRUCT, CtlID) == %ld (expected 4)",
       FIELD_OFFSET(DELETEITEMSTRUCT, CtlID)); /* UINT */
    ok(FIELD_OFFSET(DELETEITEMSTRUCT, itemID) == 8,
       "FIELD_OFFSET(DELETEITEMSTRUCT, itemID) == %ld (expected 8)",
       FIELD_OFFSET(DELETEITEMSTRUCT, itemID)); /* UINT */
    ok(FIELD_OFFSET(DELETEITEMSTRUCT, hwndItem) == 12,
       "FIELD_OFFSET(DELETEITEMSTRUCT, hwndItem) == %ld (expected 12)",
       FIELD_OFFSET(DELETEITEMSTRUCT, hwndItem)); /* HWND */
    ok(FIELD_OFFSET(DELETEITEMSTRUCT, itemData) == 16,
       "FIELD_OFFSET(DELETEITEMSTRUCT, itemData) == %ld (expected 16)",
       FIELD_OFFSET(DELETEITEMSTRUCT, itemData)); /* DWORD */
    ok(sizeof(DELETEITEMSTRUCT) == 20, "sizeof(DELETEITEMSTRUCT) == %d (expected 20)", sizeof(DELETEITEMSTRUCT));

    /* DLGITEMTEMPLATE */
    ok(FIELD_OFFSET(DLGITEMTEMPLATE, style) == 0,
       "FIELD_OFFSET(DLGITEMTEMPLATE, style) == %ld (expected 0)",
       FIELD_OFFSET(DLGITEMTEMPLATE, style)); /* DWORD */
    ok(FIELD_OFFSET(DLGITEMTEMPLATE, dwExtendedStyle) == 4,
       "FIELD_OFFSET(DLGITEMTEMPLATE, dwExtendedStyle) == %ld (expected 4)",
       FIELD_OFFSET(DLGITEMTEMPLATE, dwExtendedStyle)); /* DWORD */
    ok(FIELD_OFFSET(DLGITEMTEMPLATE, x) == 8,
       "FIELD_OFFSET(DLGITEMTEMPLATE, x) == %ld (expected 8)",
       FIELD_OFFSET(DLGITEMTEMPLATE, x)); /* short */
    ok(FIELD_OFFSET(DLGITEMTEMPLATE, y) == 10,
       "FIELD_OFFSET(DLGITEMTEMPLATE, y) == %ld (expected 10)",
       FIELD_OFFSET(DLGITEMTEMPLATE, y)); /* short */
    ok(FIELD_OFFSET(DLGITEMTEMPLATE, cx) == 12,
       "FIELD_OFFSET(DLGITEMTEMPLATE, cx) == %ld (expected 12)",
       FIELD_OFFSET(DLGITEMTEMPLATE, cx)); /* short */
    ok(FIELD_OFFSET(DLGITEMTEMPLATE, cy) == 14,
       "FIELD_OFFSET(DLGITEMTEMPLATE, cy) == %ld (expected 14)",
       FIELD_OFFSET(DLGITEMTEMPLATE, cy)); /* short */
    ok(FIELD_OFFSET(DLGITEMTEMPLATE, id) == 16,
       "FIELD_OFFSET(DLGITEMTEMPLATE, id) == %ld (expected 16)",
       FIELD_OFFSET(DLGITEMTEMPLATE, id)); /* WORD */
    ok(sizeof(DLGITEMTEMPLATE) == 18, "sizeof(DLGITEMTEMPLATE) == %d (expected 18)", sizeof(DLGITEMTEMPLATE));

    /* DLGTEMPLATE */
    ok(FIELD_OFFSET(DLGTEMPLATE, style) == 0,
       "FIELD_OFFSET(DLGTEMPLATE, style) == %ld (expected 0)",
       FIELD_OFFSET(DLGTEMPLATE, style)); /* DWORD */
    ok(FIELD_OFFSET(DLGTEMPLATE, dwExtendedStyle) == 4,
       "FIELD_OFFSET(DLGTEMPLATE, dwExtendedStyle) == %ld (expected 4)",
       FIELD_OFFSET(DLGTEMPLATE, dwExtendedStyle)); /* DWORD */
    ok(FIELD_OFFSET(DLGTEMPLATE, cdit) == 8,
       "FIELD_OFFSET(DLGTEMPLATE, cdit) == %ld (expected 8)",
       FIELD_OFFSET(DLGTEMPLATE, cdit)); /* WORD */
    ok(FIELD_OFFSET(DLGTEMPLATE, x) == 10,
       "FIELD_OFFSET(DLGTEMPLATE, x) == %ld (expected 10)",
       FIELD_OFFSET(DLGTEMPLATE, x)); /* short */
    ok(FIELD_OFFSET(DLGTEMPLATE, y) == 12,
       "FIELD_OFFSET(DLGTEMPLATE, y) == %ld (expected 12)",
       FIELD_OFFSET(DLGTEMPLATE, y)); /* short */
    ok(FIELD_OFFSET(DLGTEMPLATE, cx) == 14,
       "FIELD_OFFSET(DLGTEMPLATE, cx) == %ld (expected 14)",
       FIELD_OFFSET(DLGTEMPLATE, cx)); /* short */
    ok(FIELD_OFFSET(DLGTEMPLATE, cy) == 16,
       "FIELD_OFFSET(DLGTEMPLATE, cy) == %ld (expected 16)",
       FIELD_OFFSET(DLGTEMPLATE, cy)); /* short */
    ok(sizeof(DLGTEMPLATE) == 18, "sizeof(DLGTEMPLATE) == %d (expected 18)", sizeof(DLGTEMPLATE));

    /* DRAWITEMSTRUCT */
    ok(FIELD_OFFSET(DRAWITEMSTRUCT, CtlType) == 0,
       "FIELD_OFFSET(DRAWITEMSTRUCT, CtlType) == %ld (expected 0)",
       FIELD_OFFSET(DRAWITEMSTRUCT, CtlType)); /* UINT */
    ok(FIELD_OFFSET(DRAWITEMSTRUCT, CtlID) == 4,
       "FIELD_OFFSET(DRAWITEMSTRUCT, CtlID) == %ld (expected 4)",
       FIELD_OFFSET(DRAWITEMSTRUCT, CtlID)); /* UINT */
    ok(FIELD_OFFSET(DRAWITEMSTRUCT, itemID) == 8,
       "FIELD_OFFSET(DRAWITEMSTRUCT, itemID) == %ld (expected 8)",
       FIELD_OFFSET(DRAWITEMSTRUCT, itemID)); /* UINT */
    ok(FIELD_OFFSET(DRAWITEMSTRUCT, itemAction) == 12,
       "FIELD_OFFSET(DRAWITEMSTRUCT, itemAction) == %ld (expected 12)",
       FIELD_OFFSET(DRAWITEMSTRUCT, itemAction)); /* UINT */
    ok(FIELD_OFFSET(DRAWITEMSTRUCT, itemState) == 16,
       "FIELD_OFFSET(DRAWITEMSTRUCT, itemState) == %ld (expected 16)",
       FIELD_OFFSET(DRAWITEMSTRUCT, itemState)); /* UINT */
    ok(FIELD_OFFSET(DRAWITEMSTRUCT, hwndItem) == 20,
       "FIELD_OFFSET(DRAWITEMSTRUCT, hwndItem) == %ld (expected 20)",
       FIELD_OFFSET(DRAWITEMSTRUCT, hwndItem)); /* HWND */
    ok(FIELD_OFFSET(DRAWITEMSTRUCT, hDC) == 24,
       "FIELD_OFFSET(DRAWITEMSTRUCT, hDC) == %ld (expected 24)",
       FIELD_OFFSET(DRAWITEMSTRUCT, hDC)); /* HDC */
    ok(FIELD_OFFSET(DRAWITEMSTRUCT, rcItem) == 28,
       "FIELD_OFFSET(DRAWITEMSTRUCT, rcItem) == %ld (expected 28)",
       FIELD_OFFSET(DRAWITEMSTRUCT, rcItem)); /* RECT */
    ok(FIELD_OFFSET(DRAWITEMSTRUCT, itemData) == 44,
       "FIELD_OFFSET(DRAWITEMSTRUCT, itemData) == %ld (expected 44)",
       FIELD_OFFSET(DRAWITEMSTRUCT, itemData)); /* DWORD */
    ok(sizeof(DRAWITEMSTRUCT) == 48, "sizeof(DRAWITEMSTRUCT) == %d (expected 48)", sizeof(DRAWITEMSTRUCT));

    /* DRAWTEXTPARAMS */
    ok(FIELD_OFFSET(DRAWTEXTPARAMS, cbSize) == 0,
       "FIELD_OFFSET(DRAWTEXTPARAMS, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(DRAWTEXTPARAMS, cbSize)); /* UINT */
    ok(FIELD_OFFSET(DRAWTEXTPARAMS, iTabLength) == 4,
       "FIELD_OFFSET(DRAWTEXTPARAMS, iTabLength) == %ld (expected 4)",
       FIELD_OFFSET(DRAWTEXTPARAMS, iTabLength)); /* INT */
    ok(FIELD_OFFSET(DRAWTEXTPARAMS, iLeftMargin) == 8,
       "FIELD_OFFSET(DRAWTEXTPARAMS, iLeftMargin) == %ld (expected 8)",
       FIELD_OFFSET(DRAWTEXTPARAMS, iLeftMargin)); /* INT */
    ok(FIELD_OFFSET(DRAWTEXTPARAMS, iRightMargin) == 12,
       "FIELD_OFFSET(DRAWTEXTPARAMS, iRightMargin) == %ld (expected 12)",
       FIELD_OFFSET(DRAWTEXTPARAMS, iRightMargin)); /* INT */
    ok(FIELD_OFFSET(DRAWTEXTPARAMS, uiLengthDrawn) == 16,
       "FIELD_OFFSET(DRAWTEXTPARAMS, uiLengthDrawn) == %ld (expected 16)",
       FIELD_OFFSET(DRAWTEXTPARAMS, uiLengthDrawn)); /* UINT */
    ok(sizeof(DRAWTEXTPARAMS) == 20, "sizeof(DRAWTEXTPARAMS) == %d (expected 20)", sizeof(DRAWTEXTPARAMS));

    /* EVENTMSG */
    ok(FIELD_OFFSET(EVENTMSG, message) == 0,
       "FIELD_OFFSET(EVENTMSG, message) == %ld (expected 0)",
       FIELD_OFFSET(EVENTMSG, message)); /* UINT */
    ok(FIELD_OFFSET(EVENTMSG, paramL) == 4,
       "FIELD_OFFSET(EVENTMSG, paramL) == %ld (expected 4)",
       FIELD_OFFSET(EVENTMSG, paramL)); /* UINT */
    ok(FIELD_OFFSET(EVENTMSG, paramH) == 8,
       "FIELD_OFFSET(EVENTMSG, paramH) == %ld (expected 8)",
       FIELD_OFFSET(EVENTMSG, paramH)); /* UINT */
    ok(FIELD_OFFSET(EVENTMSG, time) == 12,
       "FIELD_OFFSET(EVENTMSG, time) == %ld (expected 12)",
       FIELD_OFFSET(EVENTMSG, time)); /* DWORD */
    ok(FIELD_OFFSET(EVENTMSG, hwnd) == 16,
       "FIELD_OFFSET(EVENTMSG, hwnd) == %ld (expected 16)",
       FIELD_OFFSET(EVENTMSG, hwnd)); /* HWND */
    ok(sizeof(EVENTMSG) == 20, "sizeof(EVENTMSG) == %d (expected 20)", sizeof(EVENTMSG));

    /* FILTERKEYS */
    ok(FIELD_OFFSET(FILTERKEYS, cbSize) == 0,
       "FIELD_OFFSET(FILTERKEYS, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(FILTERKEYS, cbSize)); /* UINT */
    ok(FIELD_OFFSET(FILTERKEYS, dwFlags) == 4,
       "FIELD_OFFSET(FILTERKEYS, dwFlags) == %ld (expected 4)",
       FIELD_OFFSET(FILTERKEYS, dwFlags)); /* DWORD */
    ok(FIELD_OFFSET(FILTERKEYS, iWaitMSec) == 8,
       "FIELD_OFFSET(FILTERKEYS, iWaitMSec) == %ld (expected 8)",
       FIELD_OFFSET(FILTERKEYS, iWaitMSec)); /* DWORD */
    ok(FIELD_OFFSET(FILTERKEYS, iDelayMSec) == 12,
       "FIELD_OFFSET(FILTERKEYS, iDelayMSec) == %ld (expected 12)",
       FIELD_OFFSET(FILTERKEYS, iDelayMSec)); /* DWORD */
    ok(FIELD_OFFSET(FILTERKEYS, iRepeatMSec) == 16,
       "FIELD_OFFSET(FILTERKEYS, iRepeatMSec) == %ld (expected 16)",
       FIELD_OFFSET(FILTERKEYS, iRepeatMSec)); /* DWORD */
    ok(FIELD_OFFSET(FILTERKEYS, iBounceMSec) == 20,
       "FIELD_OFFSET(FILTERKEYS, iBounceMSec) == %ld (expected 20)",
       FIELD_OFFSET(FILTERKEYS, iBounceMSec)); /* DWORD */
    ok(sizeof(FILTERKEYS) == 24, "sizeof(FILTERKEYS) == %d (expected 24)", sizeof(FILTERKEYS));

    /* HARDWAREHOOKSTRUCT */
    ok(FIELD_OFFSET(HARDWAREHOOKSTRUCT, hWnd) == 0,
       "FIELD_OFFSET(HARDWAREHOOKSTRUCT, hWnd) == %ld (expected 0)",
       FIELD_OFFSET(HARDWAREHOOKSTRUCT, hWnd)); /* HWND */
    ok(FIELD_OFFSET(HARDWAREHOOKSTRUCT, wMessage) == 4,
       "FIELD_OFFSET(HARDWAREHOOKSTRUCT, wMessage) == %ld (expected 4)",
       FIELD_OFFSET(HARDWAREHOOKSTRUCT, wMessage)); /* UINT */
    ok(FIELD_OFFSET(HARDWAREHOOKSTRUCT, wParam) == 8,
       "FIELD_OFFSET(HARDWAREHOOKSTRUCT, wParam) == %ld (expected 8)",
       FIELD_OFFSET(HARDWAREHOOKSTRUCT, wParam)); /* WPARAM */
    ok(FIELD_OFFSET(HARDWAREHOOKSTRUCT, lParam) == 12,
       "FIELD_OFFSET(HARDWAREHOOKSTRUCT, lParam) == %ld (expected 12)",
       FIELD_OFFSET(HARDWAREHOOKSTRUCT, lParam)); /* LPARAM */
    ok(sizeof(HARDWAREHOOKSTRUCT) == 16, "sizeof(HARDWAREHOOKSTRUCT) == %d (expected 16)", sizeof(HARDWAREHOOKSTRUCT));

    /* HARDWAREINPUT */
    ok(FIELD_OFFSET(HARDWAREINPUT, uMsg) == 0,
       "FIELD_OFFSET(HARDWAREINPUT, uMsg) == %ld (expected 0)",
       FIELD_OFFSET(HARDWAREINPUT, uMsg)); /* DWORD */
    ok(FIELD_OFFSET(HARDWAREINPUT, wParamL) == 4,
       "FIELD_OFFSET(HARDWAREINPUT, wParamL) == %ld (expected 4)",
       FIELD_OFFSET(HARDWAREINPUT, wParamL)); /* WORD */
    ok(FIELD_OFFSET(HARDWAREINPUT, wParamH) == 6,
       "FIELD_OFFSET(HARDWAREINPUT, wParamH) == %ld (expected 6)",
       FIELD_OFFSET(HARDWAREINPUT, wParamH)); /* WORD */
    ok(sizeof(HARDWAREINPUT) == 8, "sizeof(HARDWAREINPUT) == %d (expected 8)", sizeof(HARDWAREINPUT));

    /* HELPINFO */
    ok(FIELD_OFFSET(HELPINFO, cbSize) == 0,
       "FIELD_OFFSET(HELPINFO, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(HELPINFO, cbSize)); /* UINT */
    ok(FIELD_OFFSET(HELPINFO, iContextType) == 4,
       "FIELD_OFFSET(HELPINFO, iContextType) == %ld (expected 4)",
       FIELD_OFFSET(HELPINFO, iContextType)); /* INT */
    ok(FIELD_OFFSET(HELPINFO, iCtrlId) == 8,
       "FIELD_OFFSET(HELPINFO, iCtrlId) == %ld (expected 8)",
       FIELD_OFFSET(HELPINFO, iCtrlId)); /* INT */
    ok(FIELD_OFFSET(HELPINFO, hItemHandle) == 12,
       "FIELD_OFFSET(HELPINFO, hItemHandle) == %ld (expected 12)",
       FIELD_OFFSET(HELPINFO, hItemHandle)); /* HANDLE */
    ok(FIELD_OFFSET(HELPINFO, dwContextId) == 16,
       "FIELD_OFFSET(HELPINFO, dwContextId) == %ld (expected 16)",
       FIELD_OFFSET(HELPINFO, dwContextId)); /* DWORD */
    ok(FIELD_OFFSET(HELPINFO, MousePos) == 20,
       "FIELD_OFFSET(HELPINFO, MousePos) == %ld (expected 20)",
       FIELD_OFFSET(HELPINFO, MousePos)); /* POINT */
    ok(sizeof(HELPINFO) == 28, "sizeof(HELPINFO) == %d (expected 28)", sizeof(HELPINFO));

    /* HELPWININFOA */
    ok(FIELD_OFFSET(HELPWININFOA, wStructSize) == 0,
       "FIELD_OFFSET(HELPWININFOA, wStructSize) == %ld (expected 0)",
       FIELD_OFFSET(HELPWININFOA, wStructSize)); /* int */
    ok(FIELD_OFFSET(HELPWININFOA, x) == 4,
       "FIELD_OFFSET(HELPWININFOA, x) == %ld (expected 4)",
       FIELD_OFFSET(HELPWININFOA, x)); /* int */
    ok(FIELD_OFFSET(HELPWININFOA, y) == 8,
       "FIELD_OFFSET(HELPWININFOA, y) == %ld (expected 8)",
       FIELD_OFFSET(HELPWININFOA, y)); /* int */
    ok(FIELD_OFFSET(HELPWININFOA, dx) == 12,
       "FIELD_OFFSET(HELPWININFOA, dx) == %ld (expected 12)",
       FIELD_OFFSET(HELPWININFOA, dx)); /* int */
    ok(FIELD_OFFSET(HELPWININFOA, dy) == 16,
       "FIELD_OFFSET(HELPWININFOA, dy) == %ld (expected 16)",
       FIELD_OFFSET(HELPWININFOA, dy)); /* int */
    ok(FIELD_OFFSET(HELPWININFOA, wMax) == 20,
       "FIELD_OFFSET(HELPWININFOA, wMax) == %ld (expected 20)",
       FIELD_OFFSET(HELPWININFOA, wMax)); /* int */
    ok(FIELD_OFFSET(HELPWININFOA, rgchMember) == 24,
       "FIELD_OFFSET(HELPWININFOA, rgchMember) == %ld (expected 24)",
       FIELD_OFFSET(HELPWININFOA, rgchMember)); /* CHAR[2] */
    ok(sizeof(HELPWININFOA) == 28, "sizeof(HELPWININFOA) == %d (expected 28)", sizeof(HELPWININFOA));

    /* HELPWININFOW */
    ok(FIELD_OFFSET(HELPWININFOW, wStructSize) == 0,
       "FIELD_OFFSET(HELPWININFOW, wStructSize) == %ld (expected 0)",
       FIELD_OFFSET(HELPWININFOW, wStructSize)); /* int */
    ok(FIELD_OFFSET(HELPWININFOW, x) == 4,
       "FIELD_OFFSET(HELPWININFOW, x) == %ld (expected 4)",
       FIELD_OFFSET(HELPWININFOW, x)); /* int */
    ok(FIELD_OFFSET(HELPWININFOW, y) == 8,
       "FIELD_OFFSET(HELPWININFOW, y) == %ld (expected 8)",
       FIELD_OFFSET(HELPWININFOW, y)); /* int */
    ok(FIELD_OFFSET(HELPWININFOW, dx) == 12,
       "FIELD_OFFSET(HELPWININFOW, dx) == %ld (expected 12)",
       FIELD_OFFSET(HELPWININFOW, dx)); /* int */
    ok(FIELD_OFFSET(HELPWININFOW, dy) == 16,
       "FIELD_OFFSET(HELPWININFOW, dy) == %ld (expected 16)",
       FIELD_OFFSET(HELPWININFOW, dy)); /* int */
    ok(FIELD_OFFSET(HELPWININFOW, wMax) == 20,
       "FIELD_OFFSET(HELPWININFOW, wMax) == %ld (expected 20)",
       FIELD_OFFSET(HELPWININFOW, wMax)); /* int */
    ok(FIELD_OFFSET(HELPWININFOW, rgchMember) == 24,
       "FIELD_OFFSET(HELPWININFOW, rgchMember) == %ld (expected 24)",
       FIELD_OFFSET(HELPWININFOW, rgchMember)); /* WCHAR[2] */
    ok(sizeof(HELPWININFOW) == 28, "sizeof(HELPWININFOW) == %d (expected 28)", sizeof(HELPWININFOW));

    /* HIGHCONTRASTA */
    ok(FIELD_OFFSET(HIGHCONTRASTA, cbSize) == 0,
       "FIELD_OFFSET(HIGHCONTRASTA, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(HIGHCONTRASTA, cbSize)); /* UINT */
    ok(FIELD_OFFSET(HIGHCONTRASTA, dwFlags) == 4,
       "FIELD_OFFSET(HIGHCONTRASTA, dwFlags) == %ld (expected 4)",
       FIELD_OFFSET(HIGHCONTRASTA, dwFlags)); /* DWORD */
    ok(FIELD_OFFSET(HIGHCONTRASTA, lpszDefaultScheme) == 8,
       "FIELD_OFFSET(HIGHCONTRASTA, lpszDefaultScheme) == %ld (expected 8)",
       FIELD_OFFSET(HIGHCONTRASTA, lpszDefaultScheme)); /* LPSTR */
    ok(sizeof(HIGHCONTRASTA) == 12, "sizeof(HIGHCONTRASTA) == %d (expected 12)", sizeof(HIGHCONTRASTA));

    /* HIGHCONTRASTW */
    ok(FIELD_OFFSET(HIGHCONTRASTW, cbSize) == 0,
       "FIELD_OFFSET(HIGHCONTRASTW, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(HIGHCONTRASTW, cbSize)); /* UINT */
    ok(FIELD_OFFSET(HIGHCONTRASTW, dwFlags) == 4,
       "FIELD_OFFSET(HIGHCONTRASTW, dwFlags) == %ld (expected 4)",
       FIELD_OFFSET(HIGHCONTRASTW, dwFlags)); /* DWORD */
    ok(FIELD_OFFSET(HIGHCONTRASTW, lpszDefaultScheme) == 8,
       "FIELD_OFFSET(HIGHCONTRASTW, lpszDefaultScheme) == %ld (expected 8)",
       FIELD_OFFSET(HIGHCONTRASTW, lpszDefaultScheme)); /* LPWSTR */
    ok(sizeof(HIGHCONTRASTW) == 12, "sizeof(HIGHCONTRASTW) == %d (expected 12)", sizeof(HIGHCONTRASTW));

    /* ICONINFO */
    ok(FIELD_OFFSET(ICONINFO, fIcon) == 0,
       "FIELD_OFFSET(ICONINFO, fIcon) == %ld (expected 0)",
       FIELD_OFFSET(ICONINFO, fIcon)); /* BOOL */
    ok(FIELD_OFFSET(ICONINFO, xHotspot) == 4,
       "FIELD_OFFSET(ICONINFO, xHotspot) == %ld (expected 4)",
       FIELD_OFFSET(ICONINFO, xHotspot)); /* DWORD */
    ok(FIELD_OFFSET(ICONINFO, yHotspot) == 8,
       "FIELD_OFFSET(ICONINFO, yHotspot) == %ld (expected 8)",
       FIELD_OFFSET(ICONINFO, yHotspot)); /* DWORD */
    ok(FIELD_OFFSET(ICONINFO, hbmMask) == 12,
       "FIELD_OFFSET(ICONINFO, hbmMask) == %ld (expected 12)",
       FIELD_OFFSET(ICONINFO, hbmMask)); /* HBITMAP */
    ok(FIELD_OFFSET(ICONINFO, hbmColor) == 16,
       "FIELD_OFFSET(ICONINFO, hbmColor) == %ld (expected 16)",
       FIELD_OFFSET(ICONINFO, hbmColor)); /* HBITMAP */
    ok(sizeof(ICONINFO) == 20, "sizeof(ICONINFO) == %d (expected 20)", sizeof(ICONINFO));

    /* KBDLLHOOKSTRUCT */
    ok(FIELD_OFFSET(KBDLLHOOKSTRUCT, vkCode) == 0,
       "FIELD_OFFSET(KBDLLHOOKSTRUCT, vkCode) == %ld (expected 0)",
       FIELD_OFFSET(KBDLLHOOKSTRUCT, vkCode)); /* DWORD */
    ok(FIELD_OFFSET(KBDLLHOOKSTRUCT, scanCode) == 4,
       "FIELD_OFFSET(KBDLLHOOKSTRUCT, scanCode) == %ld (expected 4)",
       FIELD_OFFSET(KBDLLHOOKSTRUCT, scanCode)); /* DWORD */
    ok(FIELD_OFFSET(KBDLLHOOKSTRUCT, flags) == 8,
       "FIELD_OFFSET(KBDLLHOOKSTRUCT, flags) == %ld (expected 8)",
       FIELD_OFFSET(KBDLLHOOKSTRUCT, flags)); /* DWORD */
    ok(FIELD_OFFSET(KBDLLHOOKSTRUCT, time) == 12,
       "FIELD_OFFSET(KBDLLHOOKSTRUCT, time) == %ld (expected 12)",
       FIELD_OFFSET(KBDLLHOOKSTRUCT, time)); /* DWORD */
    ok(FIELD_OFFSET(KBDLLHOOKSTRUCT, dwExtraInfo) == 16,
       "FIELD_OFFSET(KBDLLHOOKSTRUCT, dwExtraInfo) == %ld (expected 16)",
       FIELD_OFFSET(KBDLLHOOKSTRUCT, dwExtraInfo)); /* ULONG_PTR */
    ok(sizeof(KBDLLHOOKSTRUCT) == 20, "sizeof(KBDLLHOOKSTRUCT) == %d (expected 20)", sizeof(KBDLLHOOKSTRUCT));

    /* KEYBDINPUT */
    ok(FIELD_OFFSET(KEYBDINPUT, wVk) == 0,
       "FIELD_OFFSET(KEYBDINPUT, wVk) == %ld (expected 0)",
       FIELD_OFFSET(KEYBDINPUT, wVk)); /* WORD */
    ok(FIELD_OFFSET(KEYBDINPUT, wScan) == 2,
       "FIELD_OFFSET(KEYBDINPUT, wScan) == %ld (expected 2)",
       FIELD_OFFSET(KEYBDINPUT, wScan)); /* WORD */
    ok(FIELD_OFFSET(KEYBDINPUT, dwFlags) == 4,
       "FIELD_OFFSET(KEYBDINPUT, dwFlags) == %ld (expected 4)",
       FIELD_OFFSET(KEYBDINPUT, dwFlags)); /* DWORD */
    ok(FIELD_OFFSET(KEYBDINPUT, time) == 8,
       "FIELD_OFFSET(KEYBDINPUT, time) == %ld (expected 8)",
       FIELD_OFFSET(KEYBDINPUT, time)); /* DWORD */
    ok(FIELD_OFFSET(KEYBDINPUT, dwExtraInfo) == 12,
       "FIELD_OFFSET(KEYBDINPUT, dwExtraInfo) == %ld (expected 12)",
       FIELD_OFFSET(KEYBDINPUT, dwExtraInfo)); /* ULONG_PTR */
    ok(sizeof(KEYBDINPUT) == 16, "sizeof(KEYBDINPUT) == %d (expected 16)", sizeof(KEYBDINPUT));

    /* MDICREATESTRUCTA */
    ok(FIELD_OFFSET(MDICREATESTRUCTA, szClass) == 0,
       "FIELD_OFFSET(MDICREATESTRUCTA, szClass) == %ld (expected 0)",
       FIELD_OFFSET(MDICREATESTRUCTA, szClass)); /* LPCSTR */
    ok(FIELD_OFFSET(MDICREATESTRUCTA, szTitle) == 4,
       "FIELD_OFFSET(MDICREATESTRUCTA, szTitle) == %ld (expected 4)",
       FIELD_OFFSET(MDICREATESTRUCTA, szTitle)); /* LPCSTR */
    ok(FIELD_OFFSET(MDICREATESTRUCTA, hOwner) == 8,
       "FIELD_OFFSET(MDICREATESTRUCTA, hOwner) == %ld (expected 8)",
       FIELD_OFFSET(MDICREATESTRUCTA, hOwner)); /* HINSTANCE */
    ok(FIELD_OFFSET(MDICREATESTRUCTA, x) == 12,
       "FIELD_OFFSET(MDICREATESTRUCTA, x) == %ld (expected 12)",
       FIELD_OFFSET(MDICREATESTRUCTA, x)); /* INT */
    ok(FIELD_OFFSET(MDICREATESTRUCTA, y) == 16,
       "FIELD_OFFSET(MDICREATESTRUCTA, y) == %ld (expected 16)",
       FIELD_OFFSET(MDICREATESTRUCTA, y)); /* INT */
    ok(FIELD_OFFSET(MDICREATESTRUCTA, cx) == 20,
       "FIELD_OFFSET(MDICREATESTRUCTA, cx) == %ld (expected 20)",
       FIELD_OFFSET(MDICREATESTRUCTA, cx)); /* INT */
    ok(FIELD_OFFSET(MDICREATESTRUCTA, cy) == 24,
       "FIELD_OFFSET(MDICREATESTRUCTA, cy) == %ld (expected 24)",
       FIELD_OFFSET(MDICREATESTRUCTA, cy)); /* INT */
    ok(FIELD_OFFSET(MDICREATESTRUCTA, style) == 28,
       "FIELD_OFFSET(MDICREATESTRUCTA, style) == %ld (expected 28)",
       FIELD_OFFSET(MDICREATESTRUCTA, style)); /* DWORD */
    ok(FIELD_OFFSET(MDICREATESTRUCTA, lParam) == 32,
       "FIELD_OFFSET(MDICREATESTRUCTA, lParam) == %ld (expected 32)",
       FIELD_OFFSET(MDICREATESTRUCTA, lParam)); /* LPARAM */
    ok(sizeof(MDICREATESTRUCTA) == 36, "sizeof(MDICREATESTRUCTA) == %d (expected 36)", sizeof(MDICREATESTRUCTA));

    /* MDICREATESTRUCTW */
    ok(FIELD_OFFSET(MDICREATESTRUCTW, szClass) == 0,
       "FIELD_OFFSET(MDICREATESTRUCTW, szClass) == %ld (expected 0)",
       FIELD_OFFSET(MDICREATESTRUCTW, szClass)); /* LPCWSTR */
    ok(FIELD_OFFSET(MDICREATESTRUCTW, szTitle) == 4,
       "FIELD_OFFSET(MDICREATESTRUCTW, szTitle) == %ld (expected 4)",
       FIELD_OFFSET(MDICREATESTRUCTW, szTitle)); /* LPCWSTR */
    ok(FIELD_OFFSET(MDICREATESTRUCTW, hOwner) == 8,
       "FIELD_OFFSET(MDICREATESTRUCTW, hOwner) == %ld (expected 8)",
       FIELD_OFFSET(MDICREATESTRUCTW, hOwner)); /* HINSTANCE */
    ok(FIELD_OFFSET(MDICREATESTRUCTW, x) == 12,
       "FIELD_OFFSET(MDICREATESTRUCTW, x) == %ld (expected 12)",
       FIELD_OFFSET(MDICREATESTRUCTW, x)); /* INT */
    ok(FIELD_OFFSET(MDICREATESTRUCTW, y) == 16,
       "FIELD_OFFSET(MDICREATESTRUCTW, y) == %ld (expected 16)",
       FIELD_OFFSET(MDICREATESTRUCTW, y)); /* INT */
    ok(FIELD_OFFSET(MDICREATESTRUCTW, cx) == 20,
       "FIELD_OFFSET(MDICREATESTRUCTW, cx) == %ld (expected 20)",
       FIELD_OFFSET(MDICREATESTRUCTW, cx)); /* INT */
    ok(FIELD_OFFSET(MDICREATESTRUCTW, cy) == 24,
       "FIELD_OFFSET(MDICREATESTRUCTW, cy) == %ld (expected 24)",
       FIELD_OFFSET(MDICREATESTRUCTW, cy)); /* INT */
    ok(FIELD_OFFSET(MDICREATESTRUCTW, style) == 28,
       "FIELD_OFFSET(MDICREATESTRUCTW, style) == %ld (expected 28)",
       FIELD_OFFSET(MDICREATESTRUCTW, style)); /* DWORD */
    ok(FIELD_OFFSET(MDICREATESTRUCTW, lParam) == 32,
       "FIELD_OFFSET(MDICREATESTRUCTW, lParam) == %ld (expected 32)",
       FIELD_OFFSET(MDICREATESTRUCTW, lParam)); /* LPARAM */
    ok(sizeof(MDICREATESTRUCTW) == 36, "sizeof(MDICREATESTRUCTW) == %d (expected 36)", sizeof(MDICREATESTRUCTW));

    /* MDINEXTMENU */
    ok(FIELD_OFFSET(MDINEXTMENU, hmenuIn) == 0,
       "FIELD_OFFSET(MDINEXTMENU, hmenuIn) == %ld (expected 0)",
       FIELD_OFFSET(MDINEXTMENU, hmenuIn)); /* HMENU */
    ok(FIELD_OFFSET(MDINEXTMENU, hmenuNext) == 4,
       "FIELD_OFFSET(MDINEXTMENU, hmenuNext) == %ld (expected 4)",
       FIELD_OFFSET(MDINEXTMENU, hmenuNext)); /* HMENU */
    ok(FIELD_OFFSET(MDINEXTMENU, hwndNext) == 8,
       "FIELD_OFFSET(MDINEXTMENU, hwndNext) == %ld (expected 8)",
       FIELD_OFFSET(MDINEXTMENU, hwndNext)); /* HWND */
    ok(sizeof(MDINEXTMENU) == 12, "sizeof(MDINEXTMENU) == %d (expected 12)", sizeof(MDINEXTMENU));

    /* MEASUREITEMSTRUCT */
    ok(FIELD_OFFSET(MEASUREITEMSTRUCT, CtlType) == 0,
       "FIELD_OFFSET(MEASUREITEMSTRUCT, CtlType) == %ld (expected 0)",
       FIELD_OFFSET(MEASUREITEMSTRUCT, CtlType)); /* UINT */
    ok(FIELD_OFFSET(MEASUREITEMSTRUCT, CtlID) == 4,
       "FIELD_OFFSET(MEASUREITEMSTRUCT, CtlID) == %ld (expected 4)",
       FIELD_OFFSET(MEASUREITEMSTRUCT, CtlID)); /* UINT */
    ok(FIELD_OFFSET(MEASUREITEMSTRUCT, itemID) == 8,
       "FIELD_OFFSET(MEASUREITEMSTRUCT, itemID) == %ld (expected 8)",
       FIELD_OFFSET(MEASUREITEMSTRUCT, itemID)); /* UINT */
    ok(FIELD_OFFSET(MEASUREITEMSTRUCT, itemWidth) == 12,
       "FIELD_OFFSET(MEASUREITEMSTRUCT, itemWidth) == %ld (expected 12)",
       FIELD_OFFSET(MEASUREITEMSTRUCT, itemWidth)); /* UINT */
    ok(FIELD_OFFSET(MEASUREITEMSTRUCT, itemHeight) == 16,
       "FIELD_OFFSET(MEASUREITEMSTRUCT, itemHeight) == %ld (expected 16)",
       FIELD_OFFSET(MEASUREITEMSTRUCT, itemHeight)); /* UINT */
    ok(FIELD_OFFSET(MEASUREITEMSTRUCT, itemData) == 20,
       "FIELD_OFFSET(MEASUREITEMSTRUCT, itemData) == %ld (expected 20)",
       FIELD_OFFSET(MEASUREITEMSTRUCT, itemData)); /* DWORD */
    ok(sizeof(MEASUREITEMSTRUCT) == 24, "sizeof(MEASUREITEMSTRUCT) == %d (expected 24)", sizeof(MEASUREITEMSTRUCT));

    /* MENUINFO */
    ok(FIELD_OFFSET(MENUINFO, cbSize) == 0,
       "FIELD_OFFSET(MENUINFO, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(MENUINFO, cbSize)); /* DWORD */
    ok(FIELD_OFFSET(MENUINFO, fMask) == 4,
       "FIELD_OFFSET(MENUINFO, fMask) == %ld (expected 4)",
       FIELD_OFFSET(MENUINFO, fMask)); /* DWORD */
    ok(FIELD_OFFSET(MENUINFO, dwStyle) == 8,
       "FIELD_OFFSET(MENUINFO, dwStyle) == %ld (expected 8)",
       FIELD_OFFSET(MENUINFO, dwStyle)); /* DWORD */
    ok(FIELD_OFFSET(MENUINFO, cyMax) == 12,
       "FIELD_OFFSET(MENUINFO, cyMax) == %ld (expected 12)",
       FIELD_OFFSET(MENUINFO, cyMax)); /* UINT */
    ok(FIELD_OFFSET(MENUINFO, hbrBack) == 16,
       "FIELD_OFFSET(MENUINFO, hbrBack) == %ld (expected 16)",
       FIELD_OFFSET(MENUINFO, hbrBack)); /* HBRUSH */
    ok(FIELD_OFFSET(MENUINFO, dwContextHelpID) == 20,
       "FIELD_OFFSET(MENUINFO, dwContextHelpID) == %ld (expected 20)",
       FIELD_OFFSET(MENUINFO, dwContextHelpID)); /* DWORD */
    ok(FIELD_OFFSET(MENUINFO, dwMenuData) == 24,
       "FIELD_OFFSET(MENUINFO, dwMenuData) == %ld (expected 24)",
       FIELD_OFFSET(MENUINFO, dwMenuData)); /* DWORD */
    ok(sizeof(MENUINFO) == 28, "sizeof(MENUINFO) == %d (expected 28)", sizeof(MENUINFO));

    /* MENUITEMINFOA */
    ok(FIELD_OFFSET(MENUITEMINFOA, cbSize) == 0,
       "FIELD_OFFSET(MENUITEMINFOA, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(MENUITEMINFOA, cbSize)); /* UINT */
    ok(FIELD_OFFSET(MENUITEMINFOA, fMask) == 4,
       "FIELD_OFFSET(MENUITEMINFOA, fMask) == %ld (expected 4)",
       FIELD_OFFSET(MENUITEMINFOA, fMask)); /* UINT */
    ok(FIELD_OFFSET(MENUITEMINFOA, fType) == 8,
       "FIELD_OFFSET(MENUITEMINFOA, fType) == %ld (expected 8)",
       FIELD_OFFSET(MENUITEMINFOA, fType)); /* UINT */
    ok(FIELD_OFFSET(MENUITEMINFOA, fState) == 12,
       "FIELD_OFFSET(MENUITEMINFOA, fState) == %ld (expected 12)",
       FIELD_OFFSET(MENUITEMINFOA, fState)); /* UINT */
    ok(FIELD_OFFSET(MENUITEMINFOA, wID) == 16,
       "FIELD_OFFSET(MENUITEMINFOA, wID) == %ld (expected 16)",
       FIELD_OFFSET(MENUITEMINFOA, wID)); /* UINT */
    ok(FIELD_OFFSET(MENUITEMINFOA, hSubMenu) == 20,
       "FIELD_OFFSET(MENUITEMINFOA, hSubMenu) == %ld (expected 20)",
       FIELD_OFFSET(MENUITEMINFOA, hSubMenu)); /* HMENU */
    ok(FIELD_OFFSET(MENUITEMINFOA, hbmpChecked) == 24,
       "FIELD_OFFSET(MENUITEMINFOA, hbmpChecked) == %ld (expected 24)",
       FIELD_OFFSET(MENUITEMINFOA, hbmpChecked)); /* HBITMAP */
    ok(FIELD_OFFSET(MENUITEMINFOA, hbmpUnchecked) == 28,
       "FIELD_OFFSET(MENUITEMINFOA, hbmpUnchecked) == %ld (expected 28)",
       FIELD_OFFSET(MENUITEMINFOA, hbmpUnchecked)); /* HBITMAP */
    ok(FIELD_OFFSET(MENUITEMINFOA, dwItemData) == 32,
       "FIELD_OFFSET(MENUITEMINFOA, dwItemData) == %ld (expected 32)",
       FIELD_OFFSET(MENUITEMINFOA, dwItemData)); /* DWORD */
    ok(FIELD_OFFSET(MENUITEMINFOA, dwTypeData) == 36,
       "FIELD_OFFSET(MENUITEMINFOA, dwTypeData) == %ld (expected 36)",
       FIELD_OFFSET(MENUITEMINFOA, dwTypeData)); /* LPSTR */
    ok(FIELD_OFFSET(MENUITEMINFOA, cch) == 40,
       "FIELD_OFFSET(MENUITEMINFOA, cch) == %ld (expected 40)",
       FIELD_OFFSET(MENUITEMINFOA, cch)); /* UINT */
    ok(FIELD_OFFSET(MENUITEMINFOA, hbmpItem) == 44,
       "FIELD_OFFSET(MENUITEMINFOA, hbmpItem) == %ld (expected 44)",
       FIELD_OFFSET(MENUITEMINFOA, hbmpItem)); /* HBITMAP */
    ok(sizeof(MENUITEMINFOA) == 48, "sizeof(MENUITEMINFOA) == %d (expected 48)", sizeof(MENUITEMINFOA));

    /* MENUITEMINFOW */
    ok(FIELD_OFFSET(MENUITEMINFOW, cbSize) == 0,
       "FIELD_OFFSET(MENUITEMINFOW, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(MENUITEMINFOW, cbSize)); /* UINT */
    ok(FIELD_OFFSET(MENUITEMINFOW, fMask) == 4,
       "FIELD_OFFSET(MENUITEMINFOW, fMask) == %ld (expected 4)",
       FIELD_OFFSET(MENUITEMINFOW, fMask)); /* UINT */
    ok(FIELD_OFFSET(MENUITEMINFOW, fType) == 8,
       "FIELD_OFFSET(MENUITEMINFOW, fType) == %ld (expected 8)",
       FIELD_OFFSET(MENUITEMINFOW, fType)); /* UINT */
    ok(FIELD_OFFSET(MENUITEMINFOW, fState) == 12,
       "FIELD_OFFSET(MENUITEMINFOW, fState) == %ld (expected 12)",
       FIELD_OFFSET(MENUITEMINFOW, fState)); /* UINT */
    ok(FIELD_OFFSET(MENUITEMINFOW, wID) == 16,
       "FIELD_OFFSET(MENUITEMINFOW, wID) == %ld (expected 16)",
       FIELD_OFFSET(MENUITEMINFOW, wID)); /* UINT */
    ok(FIELD_OFFSET(MENUITEMINFOW, hSubMenu) == 20,
       "FIELD_OFFSET(MENUITEMINFOW, hSubMenu) == %ld (expected 20)",
       FIELD_OFFSET(MENUITEMINFOW, hSubMenu)); /* HMENU */
    ok(FIELD_OFFSET(MENUITEMINFOW, hbmpChecked) == 24,
       "FIELD_OFFSET(MENUITEMINFOW, hbmpChecked) == %ld (expected 24)",
       FIELD_OFFSET(MENUITEMINFOW, hbmpChecked)); /* HBITMAP */
    ok(FIELD_OFFSET(MENUITEMINFOW, hbmpUnchecked) == 28,
       "FIELD_OFFSET(MENUITEMINFOW, hbmpUnchecked) == %ld (expected 28)",
       FIELD_OFFSET(MENUITEMINFOW, hbmpUnchecked)); /* HBITMAP */
    ok(FIELD_OFFSET(MENUITEMINFOW, dwItemData) == 32,
       "FIELD_OFFSET(MENUITEMINFOW, dwItemData) == %ld (expected 32)",
       FIELD_OFFSET(MENUITEMINFOW, dwItemData)); /* DWORD */
    ok(FIELD_OFFSET(MENUITEMINFOW, dwTypeData) == 36,
       "FIELD_OFFSET(MENUITEMINFOW, dwTypeData) == %ld (expected 36)",
       FIELD_OFFSET(MENUITEMINFOW, dwTypeData)); /* LPWSTR */
    ok(FIELD_OFFSET(MENUITEMINFOW, cch) == 40,
       "FIELD_OFFSET(MENUITEMINFOW, cch) == %ld (expected 40)",
       FIELD_OFFSET(MENUITEMINFOW, cch)); /* UINT */
    ok(FIELD_OFFSET(MENUITEMINFOW, hbmpItem) == 44,
       "FIELD_OFFSET(MENUITEMINFOW, hbmpItem) == %ld (expected 44)",
       FIELD_OFFSET(MENUITEMINFOW, hbmpItem)); /* HBITMAP */
    ok(sizeof(MENUITEMINFOW) == 48, "sizeof(MENUITEMINFOW) == %d (expected 48)", sizeof(MENUITEMINFOW));

    /* MENUITEMTEMPLATEHEADER */
    ok(FIELD_OFFSET(MENUITEMTEMPLATEHEADER, versionNumber) == 0,
       "FIELD_OFFSET(MENUITEMTEMPLATEHEADER, versionNumber) == %ld (expected 0)",
       FIELD_OFFSET(MENUITEMTEMPLATEHEADER, versionNumber)); /* WORD */
    ok(FIELD_OFFSET(MENUITEMTEMPLATEHEADER, offset) == 2,
       "FIELD_OFFSET(MENUITEMTEMPLATEHEADER, offset) == %ld (expected 2)",
       FIELD_OFFSET(MENUITEMTEMPLATEHEADER, offset)); /* WORD */
    ok(sizeof(MENUITEMTEMPLATEHEADER) == 4, "sizeof(MENUITEMTEMPLATEHEADER) == %d (expected 4)", sizeof(MENUITEMTEMPLATEHEADER));

    /* MINIMIZEDMETRICS */
    ok(FIELD_OFFSET(MINIMIZEDMETRICS, cbSize) == 0,
       "FIELD_OFFSET(MINIMIZEDMETRICS, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(MINIMIZEDMETRICS, cbSize)); /* UINT */
    ok(FIELD_OFFSET(MINIMIZEDMETRICS, iWidth) == 4,
       "FIELD_OFFSET(MINIMIZEDMETRICS, iWidth) == %ld (expected 4)",
       FIELD_OFFSET(MINIMIZEDMETRICS, iWidth)); /* int */
    ok(FIELD_OFFSET(MINIMIZEDMETRICS, iHorzGap) == 8,
       "FIELD_OFFSET(MINIMIZEDMETRICS, iHorzGap) == %ld (expected 8)",
       FIELD_OFFSET(MINIMIZEDMETRICS, iHorzGap)); /* int */
    ok(FIELD_OFFSET(MINIMIZEDMETRICS, iVertGap) == 12,
       "FIELD_OFFSET(MINIMIZEDMETRICS, iVertGap) == %ld (expected 12)",
       FIELD_OFFSET(MINIMIZEDMETRICS, iVertGap)); /* int */
    ok(FIELD_OFFSET(MINIMIZEDMETRICS, iArrange) == 16,
       "FIELD_OFFSET(MINIMIZEDMETRICS, iArrange) == %ld (expected 16)",
       FIELD_OFFSET(MINIMIZEDMETRICS, iArrange)); /* int */
    ok(sizeof(MINIMIZEDMETRICS) == 20, "sizeof(MINIMIZEDMETRICS) == %d (expected 20)", sizeof(MINIMIZEDMETRICS));

    /* MINMAXINFO */
    ok(FIELD_OFFSET(MINMAXINFO, ptReserved) == 0,
       "FIELD_OFFSET(MINMAXINFO, ptReserved) == %ld (expected 0)",
       FIELD_OFFSET(MINMAXINFO, ptReserved)); /* POINT */
    ok(FIELD_OFFSET(MINMAXINFO, ptMaxSize) == 8,
       "FIELD_OFFSET(MINMAXINFO, ptMaxSize) == %ld (expected 8)",
       FIELD_OFFSET(MINMAXINFO, ptMaxSize)); /* POINT */
    ok(FIELD_OFFSET(MINMAXINFO, ptMaxPosition) == 16,
       "FIELD_OFFSET(MINMAXINFO, ptMaxPosition) == %ld (expected 16)",
       FIELD_OFFSET(MINMAXINFO, ptMaxPosition)); /* POINT */
    ok(FIELD_OFFSET(MINMAXINFO, ptMinTrackSize) == 24,
       "FIELD_OFFSET(MINMAXINFO, ptMinTrackSize) == %ld (expected 24)",
       FIELD_OFFSET(MINMAXINFO, ptMinTrackSize)); /* POINT */
    ok(FIELD_OFFSET(MINMAXINFO, ptMaxTrackSize) == 32,
       "FIELD_OFFSET(MINMAXINFO, ptMaxTrackSize) == %ld (expected 32)",
       FIELD_OFFSET(MINMAXINFO, ptMaxTrackSize)); /* POINT */
    ok(sizeof(MINMAXINFO) == 40, "sizeof(MINMAXINFO) == %d (expected 40)", sizeof(MINMAXINFO));

    /* MONITORINFO */
    ok(FIELD_OFFSET(MONITORINFO, cbSize) == 0,
       "FIELD_OFFSET(MONITORINFO, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(MONITORINFO, cbSize)); /* DWORD */
    ok(FIELD_OFFSET(MONITORINFO, rcMonitor) == 4,
       "FIELD_OFFSET(MONITORINFO, rcMonitor) == %ld (expected 4)",
       FIELD_OFFSET(MONITORINFO, rcMonitor)); /* RECT */
    ok(FIELD_OFFSET(MONITORINFO, rcWork) == 20,
       "FIELD_OFFSET(MONITORINFO, rcWork) == %ld (expected 20)",
       FIELD_OFFSET(MONITORINFO, rcWork)); /* RECT */
    ok(FIELD_OFFSET(MONITORINFO, dwFlags) == 36,
       "FIELD_OFFSET(MONITORINFO, dwFlags) == %ld (expected 36)",
       FIELD_OFFSET(MONITORINFO, dwFlags)); /* DWORD */
    ok(sizeof(MONITORINFO) == 40, "sizeof(MONITORINFO) == %d (expected 40)", sizeof(MONITORINFO));

    /* MOUSEHOOKSTRUCT */
    ok(FIELD_OFFSET(MOUSEHOOKSTRUCT, pt) == 0,
       "FIELD_OFFSET(MOUSEHOOKSTRUCT, pt) == %ld (expected 0)",
       FIELD_OFFSET(MOUSEHOOKSTRUCT, pt)); /* POINT */
    ok(FIELD_OFFSET(MOUSEHOOKSTRUCT, hwnd) == 8,
       "FIELD_OFFSET(MOUSEHOOKSTRUCT, hwnd) == %ld (expected 8)",
       FIELD_OFFSET(MOUSEHOOKSTRUCT, hwnd)); /* HWND */
    ok(FIELD_OFFSET(MOUSEHOOKSTRUCT, wHitTestCode) == 12,
       "FIELD_OFFSET(MOUSEHOOKSTRUCT, wHitTestCode) == %ld (expected 12)",
       FIELD_OFFSET(MOUSEHOOKSTRUCT, wHitTestCode)); /* UINT */
    ok(FIELD_OFFSET(MOUSEHOOKSTRUCT, dwExtraInfo) == 16,
       "FIELD_OFFSET(MOUSEHOOKSTRUCT, dwExtraInfo) == %ld (expected 16)",
       FIELD_OFFSET(MOUSEHOOKSTRUCT, dwExtraInfo)); /* DWORD */
    ok(sizeof(MOUSEHOOKSTRUCT) == 20, "sizeof(MOUSEHOOKSTRUCT) == %d (expected 20)", sizeof(MOUSEHOOKSTRUCT));

    /* MOUSEINPUT */
    ok(FIELD_OFFSET(MOUSEINPUT, dx) == 0,
       "FIELD_OFFSET(MOUSEINPUT, dx) == %ld (expected 0)",
       FIELD_OFFSET(MOUSEINPUT, dx)); /* LONG */
    ok(FIELD_OFFSET(MOUSEINPUT, dy) == 4,
       "FIELD_OFFSET(MOUSEINPUT, dy) == %ld (expected 4)",
       FIELD_OFFSET(MOUSEINPUT, dy)); /* LONG */
    ok(FIELD_OFFSET(MOUSEINPUT, mouseData) == 8,
       "FIELD_OFFSET(MOUSEINPUT, mouseData) == %ld (expected 8)",
       FIELD_OFFSET(MOUSEINPUT, mouseData)); /* DWORD */
    ok(FIELD_OFFSET(MOUSEINPUT, dwFlags) == 12,
       "FIELD_OFFSET(MOUSEINPUT, dwFlags) == %ld (expected 12)",
       FIELD_OFFSET(MOUSEINPUT, dwFlags)); /* DWORD */
    ok(FIELD_OFFSET(MOUSEINPUT, time) == 16,
       "FIELD_OFFSET(MOUSEINPUT, time) == %ld (expected 16)",
       FIELD_OFFSET(MOUSEINPUT, time)); /* DWORD */
    ok(FIELD_OFFSET(MOUSEINPUT, dwExtraInfo) == 20,
       "FIELD_OFFSET(MOUSEINPUT, dwExtraInfo) == %ld (expected 20)",
       FIELD_OFFSET(MOUSEINPUT, dwExtraInfo)); /* ULONG_PTR */
    ok(sizeof(MOUSEINPUT) == 24, "sizeof(MOUSEINPUT) == %d (expected 24)", sizeof(MOUSEINPUT));

    /* MOUSEKEYS */
    ok(FIELD_OFFSET(MOUSEKEYS, cbSize) == 0,
       "FIELD_OFFSET(MOUSEKEYS, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(MOUSEKEYS, cbSize)); /* UINT */
    ok(FIELD_OFFSET(MOUSEKEYS, dwFlags) == 4,
       "FIELD_OFFSET(MOUSEKEYS, dwFlags) == %ld (expected 4)",
       FIELD_OFFSET(MOUSEKEYS, dwFlags)); /* DWORD */
    ok(FIELD_OFFSET(MOUSEKEYS, iMaxSpeed) == 8,
       "FIELD_OFFSET(MOUSEKEYS, iMaxSpeed) == %ld (expected 8)",
       FIELD_OFFSET(MOUSEKEYS, iMaxSpeed)); /* DWORD */
    ok(FIELD_OFFSET(MOUSEKEYS, iTimeToMaxSpeed) == 12,
       "FIELD_OFFSET(MOUSEKEYS, iTimeToMaxSpeed) == %ld (expected 12)",
       FIELD_OFFSET(MOUSEKEYS, iTimeToMaxSpeed)); /* DWORD */
    ok(FIELD_OFFSET(MOUSEKEYS, iCtrlSpeed) == 16,
       "FIELD_OFFSET(MOUSEKEYS, iCtrlSpeed) == %ld (expected 16)",
       FIELD_OFFSET(MOUSEKEYS, iCtrlSpeed)); /* DWORD */
    ok(FIELD_OFFSET(MOUSEKEYS, dwReserved1) == 20,
       "FIELD_OFFSET(MOUSEKEYS, dwReserved1) == %ld (expected 20)",
       FIELD_OFFSET(MOUSEKEYS, dwReserved1)); /* DWORD */
    ok(FIELD_OFFSET(MOUSEKEYS, dwReserved2) == 24,
       "FIELD_OFFSET(MOUSEKEYS, dwReserved2) == %ld (expected 24)",
       FIELD_OFFSET(MOUSEKEYS, dwReserved2)); /* DWORD */
    ok(sizeof(MOUSEKEYS) == 28, "sizeof(MOUSEKEYS) == %d (expected 28)", sizeof(MOUSEKEYS));

    /* MSG */
    ok(FIELD_OFFSET(MSG, hwnd) == 0,
       "FIELD_OFFSET(MSG, hwnd) == %ld (expected 0)",
       FIELD_OFFSET(MSG, hwnd)); /* HWND */
    ok(FIELD_OFFSET(MSG, message) == 4,
       "FIELD_OFFSET(MSG, message) == %ld (expected 4)",
       FIELD_OFFSET(MSG, message)); /* UINT */
    ok(FIELD_OFFSET(MSG, wParam) == 8,
       "FIELD_OFFSET(MSG, wParam) == %ld (expected 8)",
       FIELD_OFFSET(MSG, wParam)); /* WPARAM */
    ok(FIELD_OFFSET(MSG, lParam) == 12,
       "FIELD_OFFSET(MSG, lParam) == %ld (expected 12)",
       FIELD_OFFSET(MSG, lParam)); /* LPARAM */
    ok(FIELD_OFFSET(MSG, time) == 16,
       "FIELD_OFFSET(MSG, time) == %ld (expected 16)",
       FIELD_OFFSET(MSG, time)); /* DWORD */
    ok(FIELD_OFFSET(MSG, pt) == 20,
       "FIELD_OFFSET(MSG, pt) == %ld (expected 20)",
       FIELD_OFFSET(MSG, pt)); /* POINT */
    ok(sizeof(MSG) == 28, "sizeof(MSG) == %d (expected 28)", sizeof(MSG));

    /* MSGBOXPARAMSA */
    ok(FIELD_OFFSET(MSGBOXPARAMSA, cbSize) == 0,
       "FIELD_OFFSET(MSGBOXPARAMSA, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(MSGBOXPARAMSA, cbSize)); /* UINT */
    ok(FIELD_OFFSET(MSGBOXPARAMSA, hwndOwner) == 4,
       "FIELD_OFFSET(MSGBOXPARAMSA, hwndOwner) == %ld (expected 4)",
       FIELD_OFFSET(MSGBOXPARAMSA, hwndOwner)); /* HWND */
    ok(FIELD_OFFSET(MSGBOXPARAMSA, hInstance) == 8,
       "FIELD_OFFSET(MSGBOXPARAMSA, hInstance) == %ld (expected 8)",
       FIELD_OFFSET(MSGBOXPARAMSA, hInstance)); /* HINSTANCE */
    ok(FIELD_OFFSET(MSGBOXPARAMSA, lpszText) == 12,
       "FIELD_OFFSET(MSGBOXPARAMSA, lpszText) == %ld (expected 12)",
       FIELD_OFFSET(MSGBOXPARAMSA, lpszText)); /* LPCSTR */
    ok(FIELD_OFFSET(MSGBOXPARAMSA, lpszCaption) == 16,
       "FIELD_OFFSET(MSGBOXPARAMSA, lpszCaption) == %ld (expected 16)",
       FIELD_OFFSET(MSGBOXPARAMSA, lpszCaption)); /* LPCSTR */
    ok(FIELD_OFFSET(MSGBOXPARAMSA, dwStyle) == 20,
       "FIELD_OFFSET(MSGBOXPARAMSA, dwStyle) == %ld (expected 20)",
       FIELD_OFFSET(MSGBOXPARAMSA, dwStyle)); /* DWORD */
    ok(FIELD_OFFSET(MSGBOXPARAMSA, lpszIcon) == 24,
       "FIELD_OFFSET(MSGBOXPARAMSA, lpszIcon) == %ld (expected 24)",
       FIELD_OFFSET(MSGBOXPARAMSA, lpszIcon)); /* LPCSTR */
    ok(FIELD_OFFSET(MSGBOXPARAMSA, dwContextHelpId) == 28,
       "FIELD_OFFSET(MSGBOXPARAMSA, dwContextHelpId) == %ld (expected 28)",
       FIELD_OFFSET(MSGBOXPARAMSA, dwContextHelpId)); /* DWORD */
    ok(FIELD_OFFSET(MSGBOXPARAMSA, lpfnMsgBoxCallback) == 32,
       "FIELD_OFFSET(MSGBOXPARAMSA, lpfnMsgBoxCallback) == %ld (expected 32)",
       FIELD_OFFSET(MSGBOXPARAMSA, lpfnMsgBoxCallback)); /* MSGBOXCALLBACK */
    ok(FIELD_OFFSET(MSGBOXPARAMSA, dwLanguageId) == 36,
       "FIELD_OFFSET(MSGBOXPARAMSA, dwLanguageId) == %ld (expected 36)",
       FIELD_OFFSET(MSGBOXPARAMSA, dwLanguageId)); /* DWORD */
    ok(sizeof(MSGBOXPARAMSA) == 40, "sizeof(MSGBOXPARAMSA) == %d (expected 40)", sizeof(MSGBOXPARAMSA));

    /* MSGBOXPARAMSW */
    ok(FIELD_OFFSET(MSGBOXPARAMSW, cbSize) == 0,
       "FIELD_OFFSET(MSGBOXPARAMSW, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(MSGBOXPARAMSW, cbSize)); /* UINT */
    ok(FIELD_OFFSET(MSGBOXPARAMSW, hwndOwner) == 4,
       "FIELD_OFFSET(MSGBOXPARAMSW, hwndOwner) == %ld (expected 4)",
       FIELD_OFFSET(MSGBOXPARAMSW, hwndOwner)); /* HWND */
    ok(FIELD_OFFSET(MSGBOXPARAMSW, hInstance) == 8,
       "FIELD_OFFSET(MSGBOXPARAMSW, hInstance) == %ld (expected 8)",
       FIELD_OFFSET(MSGBOXPARAMSW, hInstance)); /* HINSTANCE */
    ok(FIELD_OFFSET(MSGBOXPARAMSW, lpszText) == 12,
       "FIELD_OFFSET(MSGBOXPARAMSW, lpszText) == %ld (expected 12)",
       FIELD_OFFSET(MSGBOXPARAMSW, lpszText)); /* LPCWSTR */
    ok(FIELD_OFFSET(MSGBOXPARAMSW, lpszCaption) == 16,
       "FIELD_OFFSET(MSGBOXPARAMSW, lpszCaption) == %ld (expected 16)",
       FIELD_OFFSET(MSGBOXPARAMSW, lpszCaption)); /* LPCWSTR */
    ok(FIELD_OFFSET(MSGBOXPARAMSW, dwStyle) == 20,
       "FIELD_OFFSET(MSGBOXPARAMSW, dwStyle) == %ld (expected 20)",
       FIELD_OFFSET(MSGBOXPARAMSW, dwStyle)); /* DWORD */
    ok(FIELD_OFFSET(MSGBOXPARAMSW, lpszIcon) == 24,
       "FIELD_OFFSET(MSGBOXPARAMSW, lpszIcon) == %ld (expected 24)",
       FIELD_OFFSET(MSGBOXPARAMSW, lpszIcon)); /* LPCWSTR */
    ok(FIELD_OFFSET(MSGBOXPARAMSW, dwContextHelpId) == 28,
       "FIELD_OFFSET(MSGBOXPARAMSW, dwContextHelpId) == %ld (expected 28)",
       FIELD_OFFSET(MSGBOXPARAMSW, dwContextHelpId)); /* DWORD */
    ok(FIELD_OFFSET(MSGBOXPARAMSW, lpfnMsgBoxCallback) == 32,
       "FIELD_OFFSET(MSGBOXPARAMSW, lpfnMsgBoxCallback) == %ld (expected 32)",
       FIELD_OFFSET(MSGBOXPARAMSW, lpfnMsgBoxCallback)); /* MSGBOXCALLBACK */
    ok(FIELD_OFFSET(MSGBOXPARAMSW, dwLanguageId) == 36,
       "FIELD_OFFSET(MSGBOXPARAMSW, dwLanguageId) == %ld (expected 36)",
       FIELD_OFFSET(MSGBOXPARAMSW, dwLanguageId)); /* DWORD */
    ok(sizeof(MSGBOXPARAMSW) == 40, "sizeof(MSGBOXPARAMSW) == %d (expected 40)", sizeof(MSGBOXPARAMSW));

    /* MSLLHOOKSTRUCT */
    ok(FIELD_OFFSET(MSLLHOOKSTRUCT, pt) == 0,
       "FIELD_OFFSET(MSLLHOOKSTRUCT, pt) == %ld (expected 0)",
       FIELD_OFFSET(MSLLHOOKSTRUCT, pt)); /* POINT */
    ok(FIELD_OFFSET(MSLLHOOKSTRUCT, mouseData) == 8,
       "FIELD_OFFSET(MSLLHOOKSTRUCT, mouseData) == %ld (expected 8)",
       FIELD_OFFSET(MSLLHOOKSTRUCT, mouseData)); /* DWORD */
    ok(FIELD_OFFSET(MSLLHOOKSTRUCT, flags) == 12,
       "FIELD_OFFSET(MSLLHOOKSTRUCT, flags) == %ld (expected 12)",
       FIELD_OFFSET(MSLLHOOKSTRUCT, flags)); /* DWORD */
    ok(FIELD_OFFSET(MSLLHOOKSTRUCT, time) == 16,
       "FIELD_OFFSET(MSLLHOOKSTRUCT, time) == %ld (expected 16)",
       FIELD_OFFSET(MSLLHOOKSTRUCT, time)); /* DWORD */
    ok(FIELD_OFFSET(MSLLHOOKSTRUCT, dwExtraInfo) == 20,
       "FIELD_OFFSET(MSLLHOOKSTRUCT, dwExtraInfo) == %ld (expected 20)",
       FIELD_OFFSET(MSLLHOOKSTRUCT, dwExtraInfo)); /* ULONG_PTR */
    ok(sizeof(MSLLHOOKSTRUCT) == 24, "sizeof(MSLLHOOKSTRUCT) == %d (expected 24)", sizeof(MSLLHOOKSTRUCT));

    /* MULTIKEYHELPA */
    ok(FIELD_OFFSET(MULTIKEYHELPA, mkSize) == 0,
       "FIELD_OFFSET(MULTIKEYHELPA, mkSize) == %ld (expected 0)",
       FIELD_OFFSET(MULTIKEYHELPA, mkSize)); /* DWORD */
    ok(FIELD_OFFSET(MULTIKEYHELPA, mkKeyList) == 4,
       "FIELD_OFFSET(MULTIKEYHELPA, mkKeyList) == %ld (expected 4)",
       FIELD_OFFSET(MULTIKEYHELPA, mkKeyList)); /* CHAR */
    ok(FIELD_OFFSET(MULTIKEYHELPA, szKeyphrase) == 5,
       "FIELD_OFFSET(MULTIKEYHELPA, szKeyphrase) == %ld (expected 5)",
       FIELD_OFFSET(MULTIKEYHELPA, szKeyphrase)); /* CHAR[1] */
    ok(sizeof(MULTIKEYHELPA) == 8, "sizeof(MULTIKEYHELPA) == %d (expected 8)", sizeof(MULTIKEYHELPA));

    /* MULTIKEYHELPW */
    ok(FIELD_OFFSET(MULTIKEYHELPW, mkSize) == 0,
       "FIELD_OFFSET(MULTIKEYHELPW, mkSize) == %ld (expected 0)",
       FIELD_OFFSET(MULTIKEYHELPW, mkSize)); /* DWORD */
    ok(FIELD_OFFSET(MULTIKEYHELPW, mkKeyList) == 4,
       "FIELD_OFFSET(MULTIKEYHELPW, mkKeyList) == %ld (expected 4)",
       FIELD_OFFSET(MULTIKEYHELPW, mkKeyList)); /* WCHAR */
    ok(FIELD_OFFSET(MULTIKEYHELPW, szKeyphrase) == 6,
       "FIELD_OFFSET(MULTIKEYHELPW, szKeyphrase) == %ld (expected 6)",
       FIELD_OFFSET(MULTIKEYHELPW, szKeyphrase)); /* WCHAR[1] */
    ok(sizeof(MULTIKEYHELPW) == 8, "sizeof(MULTIKEYHELPW) == %d (expected 8)", sizeof(MULTIKEYHELPW));

    /* NCCALCSIZE_PARAMS */
    ok(FIELD_OFFSET(NCCALCSIZE_PARAMS, rgrc) == 0,
       "FIELD_OFFSET(NCCALCSIZE_PARAMS, rgrc) == %ld (expected 0)",
       FIELD_OFFSET(NCCALCSIZE_PARAMS, rgrc)); /* RECT[3] */
    ok(FIELD_OFFSET(NCCALCSIZE_PARAMS, lppos) == 48,
       "FIELD_OFFSET(NCCALCSIZE_PARAMS, lppos) == %ld (expected 48)",
       FIELD_OFFSET(NCCALCSIZE_PARAMS, lppos)); /* WINDOWPOS * */
    ok(sizeof(NCCALCSIZE_PARAMS) == 52, "sizeof(NCCALCSIZE_PARAMS) == %d (expected 52)", sizeof(NCCALCSIZE_PARAMS));

    /* NMHDR */
    ok(FIELD_OFFSET(NMHDR, hwndFrom) == 0,
       "FIELD_OFFSET(NMHDR, hwndFrom) == %ld (expected 0)",
       FIELD_OFFSET(NMHDR, hwndFrom)); /* HWND */
    ok(FIELD_OFFSET(NMHDR, idFrom) == 4,
       "FIELD_OFFSET(NMHDR, idFrom) == %ld (expected 4)",
       FIELD_OFFSET(NMHDR, idFrom)); /* UINT */
    ok(FIELD_OFFSET(NMHDR, code) == 8,
       "FIELD_OFFSET(NMHDR, code) == %ld (expected 8)",
       FIELD_OFFSET(NMHDR, code)); /* UINT */
    ok(sizeof(NMHDR) == 12, "sizeof(NMHDR) == %d (expected 12)", sizeof(NMHDR));

    /* PAINTSTRUCT */
    ok(FIELD_OFFSET(PAINTSTRUCT, hdc) == 0,
       "FIELD_OFFSET(PAINTSTRUCT, hdc) == %ld (expected 0)",
       FIELD_OFFSET(PAINTSTRUCT, hdc)); /* HDC */
    ok(FIELD_OFFSET(PAINTSTRUCT, fErase) == 4,
       "FIELD_OFFSET(PAINTSTRUCT, fErase) == %ld (expected 4)",
       FIELD_OFFSET(PAINTSTRUCT, fErase)); /* BOOL */
    ok(FIELD_OFFSET(PAINTSTRUCT, rcPaint) == 8,
       "FIELD_OFFSET(PAINTSTRUCT, rcPaint) == %ld (expected 8)",
       FIELD_OFFSET(PAINTSTRUCT, rcPaint)); /* RECT */
    ok(FIELD_OFFSET(PAINTSTRUCT, fRestore) == 24,
       "FIELD_OFFSET(PAINTSTRUCT, fRestore) == %ld (expected 24)",
       FIELD_OFFSET(PAINTSTRUCT, fRestore)); /* BOOL */
    ok(FIELD_OFFSET(PAINTSTRUCT, fIncUpdate) == 28,
       "FIELD_OFFSET(PAINTSTRUCT, fIncUpdate) == %ld (expected 28)",
       FIELD_OFFSET(PAINTSTRUCT, fIncUpdate)); /* BOOL */
    ok(FIELD_OFFSET(PAINTSTRUCT, rgbReserved) == 32,
       "FIELD_OFFSET(PAINTSTRUCT, rgbReserved) == %ld (expected 32)",
       FIELD_OFFSET(PAINTSTRUCT, rgbReserved)); /* BYTE[32] */
    ok(sizeof(PAINTSTRUCT) == 64, "sizeof(PAINTSTRUCT) == %d (expected 64)", sizeof(PAINTSTRUCT));

    /* SCROLLINFO */
    ok(FIELD_OFFSET(SCROLLINFO, cbSize) == 0,
       "FIELD_OFFSET(SCROLLINFO, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(SCROLLINFO, cbSize)); /* UINT */
    ok(FIELD_OFFSET(SCROLLINFO, fMask) == 4,
       "FIELD_OFFSET(SCROLLINFO, fMask) == %ld (expected 4)",
       FIELD_OFFSET(SCROLLINFO, fMask)); /* UINT */
    ok(FIELD_OFFSET(SCROLLINFO, nMin) == 8,
       "FIELD_OFFSET(SCROLLINFO, nMin) == %ld (expected 8)",
       FIELD_OFFSET(SCROLLINFO, nMin)); /* INT */
    ok(FIELD_OFFSET(SCROLLINFO, nMax) == 12,
       "FIELD_OFFSET(SCROLLINFO, nMax) == %ld (expected 12)",
       FIELD_OFFSET(SCROLLINFO, nMax)); /* INT */
    ok(FIELD_OFFSET(SCROLLINFO, nPage) == 16,
       "FIELD_OFFSET(SCROLLINFO, nPage) == %ld (expected 16)",
       FIELD_OFFSET(SCROLLINFO, nPage)); /* UINT */
    ok(FIELD_OFFSET(SCROLLINFO, nPos) == 20,
       "FIELD_OFFSET(SCROLLINFO, nPos) == %ld (expected 20)",
       FIELD_OFFSET(SCROLLINFO, nPos)); /* INT */
    ok(FIELD_OFFSET(SCROLLINFO, nTrackPos) == 24,
       "FIELD_OFFSET(SCROLLINFO, nTrackPos) == %ld (expected 24)",
       FIELD_OFFSET(SCROLLINFO, nTrackPos)); /* INT */
    ok(sizeof(SCROLLINFO) == 28, "sizeof(SCROLLINFO) == %d (expected 28)", sizeof(SCROLLINFO));

    /* SERIALKEYSA */
    ok(FIELD_OFFSET(SERIALKEYSA, cbSize) == 0,
       "FIELD_OFFSET(SERIALKEYSA, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(SERIALKEYSA, cbSize)); /* UINT */
    ok(FIELD_OFFSET(SERIALKEYSA, dwFlags) == 4,
       "FIELD_OFFSET(SERIALKEYSA, dwFlags) == %ld (expected 4)",
       FIELD_OFFSET(SERIALKEYSA, dwFlags)); /* DWORD */
    ok(FIELD_OFFSET(SERIALKEYSA, lpszActivePort) == 8,
       "FIELD_OFFSET(SERIALKEYSA, lpszActivePort) == %ld (expected 8)",
       FIELD_OFFSET(SERIALKEYSA, lpszActivePort)); /* LPSTR */
    ok(FIELD_OFFSET(SERIALKEYSA, lpszPort) == 12,
       "FIELD_OFFSET(SERIALKEYSA, lpszPort) == %ld (expected 12)",
       FIELD_OFFSET(SERIALKEYSA, lpszPort)); /* LPSTR */
    ok(FIELD_OFFSET(SERIALKEYSA, iBaudRate) == 16,
       "FIELD_OFFSET(SERIALKEYSA, iBaudRate) == %ld (expected 16)",
       FIELD_OFFSET(SERIALKEYSA, iBaudRate)); /* UINT */
    ok(FIELD_OFFSET(SERIALKEYSA, iPortState) == 20,
       "FIELD_OFFSET(SERIALKEYSA, iPortState) == %ld (expected 20)",
       FIELD_OFFSET(SERIALKEYSA, iPortState)); /* UINT */
    ok(FIELD_OFFSET(SERIALKEYSA, iActive) == 24,
       "FIELD_OFFSET(SERIALKEYSA, iActive) == %ld (expected 24)",
       FIELD_OFFSET(SERIALKEYSA, iActive)); /* UINT */
    ok(sizeof(SERIALKEYSA) == 28, "sizeof(SERIALKEYSA) == %d (expected 28)", sizeof(SERIALKEYSA));

    /* SERIALKEYSW */
    ok(FIELD_OFFSET(SERIALKEYSW, cbSize) == 0,
       "FIELD_OFFSET(SERIALKEYSW, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(SERIALKEYSW, cbSize)); /* UINT */
    ok(FIELD_OFFSET(SERIALKEYSW, dwFlags) == 4,
       "FIELD_OFFSET(SERIALKEYSW, dwFlags) == %ld (expected 4)",
       FIELD_OFFSET(SERIALKEYSW, dwFlags)); /* DWORD */
    ok(FIELD_OFFSET(SERIALKEYSW, lpszActivePort) == 8,
       "FIELD_OFFSET(SERIALKEYSW, lpszActivePort) == %ld (expected 8)",
       FIELD_OFFSET(SERIALKEYSW, lpszActivePort)); /* LPWSTR */
    ok(FIELD_OFFSET(SERIALKEYSW, lpszPort) == 12,
       "FIELD_OFFSET(SERIALKEYSW, lpszPort) == %ld (expected 12)",
       FIELD_OFFSET(SERIALKEYSW, lpszPort)); /* LPWSTR */
    ok(FIELD_OFFSET(SERIALKEYSW, iBaudRate) == 16,
       "FIELD_OFFSET(SERIALKEYSW, iBaudRate) == %ld (expected 16)",
       FIELD_OFFSET(SERIALKEYSW, iBaudRate)); /* UINT */
    ok(FIELD_OFFSET(SERIALKEYSW, iPortState) == 20,
       "FIELD_OFFSET(SERIALKEYSW, iPortState) == %ld (expected 20)",
       FIELD_OFFSET(SERIALKEYSW, iPortState)); /* UINT */
    ok(FIELD_OFFSET(SERIALKEYSW, iActive) == 24,
       "FIELD_OFFSET(SERIALKEYSW, iActive) == %ld (expected 24)",
       FIELD_OFFSET(SERIALKEYSW, iActive)); /* UINT */
    ok(sizeof(SERIALKEYSW) == 28, "sizeof(SERIALKEYSW) == %d (expected 28)", sizeof(SERIALKEYSW));

    /* SOUNDSENTRYA */
    ok(FIELD_OFFSET(SOUNDSENTRYA, cbSize) == 0,
       "FIELD_OFFSET(SOUNDSENTRYA, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(SOUNDSENTRYA, cbSize)); /* UINT */
    ok(FIELD_OFFSET(SOUNDSENTRYA, dwFlags) == 4,
       "FIELD_OFFSET(SOUNDSENTRYA, dwFlags) == %ld (expected 4)",
       FIELD_OFFSET(SOUNDSENTRYA, dwFlags)); /* DWORD */
    ok(FIELD_OFFSET(SOUNDSENTRYA, iFSTextEffect) == 8,
       "FIELD_OFFSET(SOUNDSENTRYA, iFSTextEffect) == %ld (expected 8)",
       FIELD_OFFSET(SOUNDSENTRYA, iFSTextEffect)); /* DWORD */
    ok(FIELD_OFFSET(SOUNDSENTRYA, iFSTextEffectMSec) == 12,
       "FIELD_OFFSET(SOUNDSENTRYA, iFSTextEffectMSec) == %ld (expected 12)",
       FIELD_OFFSET(SOUNDSENTRYA, iFSTextEffectMSec)); /* DWORD */
    ok(FIELD_OFFSET(SOUNDSENTRYA, iFSTextEffectColorBits) == 16,
       "FIELD_OFFSET(SOUNDSENTRYA, iFSTextEffectColorBits) == %ld (expected 16)",
       FIELD_OFFSET(SOUNDSENTRYA, iFSTextEffectColorBits)); /* DWORD */
    ok(FIELD_OFFSET(SOUNDSENTRYA, iFSGrafEffect) == 20,
       "FIELD_OFFSET(SOUNDSENTRYA, iFSGrafEffect) == %ld (expected 20)",
       FIELD_OFFSET(SOUNDSENTRYA, iFSGrafEffect)); /* DWORD */
    ok(FIELD_OFFSET(SOUNDSENTRYA, iFSGrafEffectMSec) == 24,
       "FIELD_OFFSET(SOUNDSENTRYA, iFSGrafEffectMSec) == %ld (expected 24)",
       FIELD_OFFSET(SOUNDSENTRYA, iFSGrafEffectMSec)); /* DWORD */
    ok(FIELD_OFFSET(SOUNDSENTRYA, iFSGrafEffectColor) == 28,
       "FIELD_OFFSET(SOUNDSENTRYA, iFSGrafEffectColor) == %ld (expected 28)",
       FIELD_OFFSET(SOUNDSENTRYA, iFSGrafEffectColor)); /* DWORD */
    ok(FIELD_OFFSET(SOUNDSENTRYA, iWindowsEffect) == 32,
       "FIELD_OFFSET(SOUNDSENTRYA, iWindowsEffect) == %ld (expected 32)",
       FIELD_OFFSET(SOUNDSENTRYA, iWindowsEffect)); /* DWORD */
    ok(FIELD_OFFSET(SOUNDSENTRYA, iWindowsEffectMSec) == 36,
       "FIELD_OFFSET(SOUNDSENTRYA, iWindowsEffectMSec) == %ld (expected 36)",
       FIELD_OFFSET(SOUNDSENTRYA, iWindowsEffectMSec)); /* DWORD */
    ok(FIELD_OFFSET(SOUNDSENTRYA, lpszWindowsEffectDLL) == 40,
       "FIELD_OFFSET(SOUNDSENTRYA, lpszWindowsEffectDLL) == %ld (expected 40)",
       FIELD_OFFSET(SOUNDSENTRYA, lpszWindowsEffectDLL)); /* LPSTR */
    ok(FIELD_OFFSET(SOUNDSENTRYA, iWindowsEffectOrdinal) == 44,
       "FIELD_OFFSET(SOUNDSENTRYA, iWindowsEffectOrdinal) == %ld (expected 44)",
       FIELD_OFFSET(SOUNDSENTRYA, iWindowsEffectOrdinal)); /* DWORD */
    ok(sizeof(SOUNDSENTRYA) == 48, "sizeof(SOUNDSENTRYA) == %d (expected 48)", sizeof(SOUNDSENTRYA));

    /* SOUNDSENTRYW */
    ok(FIELD_OFFSET(SOUNDSENTRYW, cbSize) == 0,
       "FIELD_OFFSET(SOUNDSENTRYW, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(SOUNDSENTRYW, cbSize)); /* UINT */
    ok(FIELD_OFFSET(SOUNDSENTRYW, dwFlags) == 4,
       "FIELD_OFFSET(SOUNDSENTRYW, dwFlags) == %ld (expected 4)",
       FIELD_OFFSET(SOUNDSENTRYW, dwFlags)); /* DWORD */
    ok(FIELD_OFFSET(SOUNDSENTRYW, iFSTextEffect) == 8,
       "FIELD_OFFSET(SOUNDSENTRYW, iFSTextEffect) == %ld (expected 8)",
       FIELD_OFFSET(SOUNDSENTRYW, iFSTextEffect)); /* DWORD */
    ok(FIELD_OFFSET(SOUNDSENTRYW, iFSTextEffectMSec) == 12,
       "FIELD_OFFSET(SOUNDSENTRYW, iFSTextEffectMSec) == %ld (expected 12)",
       FIELD_OFFSET(SOUNDSENTRYW, iFSTextEffectMSec)); /* DWORD */
    ok(FIELD_OFFSET(SOUNDSENTRYW, iFSTextEffectColorBits) == 16,
       "FIELD_OFFSET(SOUNDSENTRYW, iFSTextEffectColorBits) == %ld (expected 16)",
       FIELD_OFFSET(SOUNDSENTRYW, iFSTextEffectColorBits)); /* DWORD */
    ok(FIELD_OFFSET(SOUNDSENTRYW, iFSGrafEffect) == 20,
       "FIELD_OFFSET(SOUNDSENTRYW, iFSGrafEffect) == %ld (expected 20)",
       FIELD_OFFSET(SOUNDSENTRYW, iFSGrafEffect)); /* DWORD */
    ok(FIELD_OFFSET(SOUNDSENTRYW, iFSGrafEffectMSec) == 24,
       "FIELD_OFFSET(SOUNDSENTRYW, iFSGrafEffectMSec) == %ld (expected 24)",
       FIELD_OFFSET(SOUNDSENTRYW, iFSGrafEffectMSec)); /* DWORD */
    ok(FIELD_OFFSET(SOUNDSENTRYW, iFSGrafEffectColor) == 28,
       "FIELD_OFFSET(SOUNDSENTRYW, iFSGrafEffectColor) == %ld (expected 28)",
       FIELD_OFFSET(SOUNDSENTRYW, iFSGrafEffectColor)); /* DWORD */
    ok(FIELD_OFFSET(SOUNDSENTRYW, iWindowsEffect) == 32,
       "FIELD_OFFSET(SOUNDSENTRYW, iWindowsEffect) == %ld (expected 32)",
       FIELD_OFFSET(SOUNDSENTRYW, iWindowsEffect)); /* DWORD */
    ok(FIELD_OFFSET(SOUNDSENTRYW, iWindowsEffectMSec) == 36,
       "FIELD_OFFSET(SOUNDSENTRYW, iWindowsEffectMSec) == %ld (expected 36)",
       FIELD_OFFSET(SOUNDSENTRYW, iWindowsEffectMSec)); /* DWORD */
    ok(FIELD_OFFSET(SOUNDSENTRYW, lpszWindowsEffectDLL) == 40,
       "FIELD_OFFSET(SOUNDSENTRYW, lpszWindowsEffectDLL) == %ld (expected 40)",
       FIELD_OFFSET(SOUNDSENTRYW, lpszWindowsEffectDLL)); /* LPWSTR */
    ok(FIELD_OFFSET(SOUNDSENTRYW, iWindowsEffectOrdinal) == 44,
       "FIELD_OFFSET(SOUNDSENTRYW, iWindowsEffectOrdinal) == %ld (expected 44)",
       FIELD_OFFSET(SOUNDSENTRYW, iWindowsEffectOrdinal)); /* DWORD */
    ok(sizeof(SOUNDSENTRYW) == 48, "sizeof(SOUNDSENTRYW) == %d (expected 48)", sizeof(SOUNDSENTRYW));

    /* STICKYKEYS */
    ok(FIELD_OFFSET(STICKYKEYS, cbSize) == 0,
       "FIELD_OFFSET(STICKYKEYS, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(STICKYKEYS, cbSize)); /* DWORD */
    ok(FIELD_OFFSET(STICKYKEYS, dwFlags) == 4,
       "FIELD_OFFSET(STICKYKEYS, dwFlags) == %ld (expected 4)",
       FIELD_OFFSET(STICKYKEYS, dwFlags)); /* DWORD */
    ok(sizeof(STICKYKEYS) == 8, "sizeof(STICKYKEYS) == %d (expected 8)", sizeof(STICKYKEYS));

    /* STYLESTRUCT */
    ok(FIELD_OFFSET(STYLESTRUCT, styleOld) == 0,
       "FIELD_OFFSET(STYLESTRUCT, styleOld) == %ld (expected 0)",
       FIELD_OFFSET(STYLESTRUCT, styleOld)); /* DWORD */
    ok(FIELD_OFFSET(STYLESTRUCT, styleNew) == 4,
       "FIELD_OFFSET(STYLESTRUCT, styleNew) == %ld (expected 4)",
       FIELD_OFFSET(STYLESTRUCT, styleNew)); /* DWORD */
    ok(sizeof(STYLESTRUCT) == 8, "sizeof(STYLESTRUCT) == %d (expected 8)", sizeof(STYLESTRUCT));

    /* TOGGLEKEYS */
    ok(FIELD_OFFSET(TOGGLEKEYS, cbSize) == 0,
       "FIELD_OFFSET(TOGGLEKEYS, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(TOGGLEKEYS, cbSize)); /* DWORD */
    ok(FIELD_OFFSET(TOGGLEKEYS, dwFlags) == 4,
       "FIELD_OFFSET(TOGGLEKEYS, dwFlags) == %ld (expected 4)",
       FIELD_OFFSET(TOGGLEKEYS, dwFlags)); /* DWORD */
    ok(sizeof(TOGGLEKEYS) == 8, "sizeof(TOGGLEKEYS) == %d (expected 8)", sizeof(TOGGLEKEYS));

    /* TPMPARAMS */
    ok(FIELD_OFFSET(TPMPARAMS, cbSize) == 0,
       "FIELD_OFFSET(TPMPARAMS, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(TPMPARAMS, cbSize)); /* UINT */
    ok(FIELD_OFFSET(TPMPARAMS, rcExclude) == 4,
       "FIELD_OFFSET(TPMPARAMS, rcExclude) == %ld (expected 4)",
       FIELD_OFFSET(TPMPARAMS, rcExclude)); /* RECT */
    ok(sizeof(TPMPARAMS) == 20, "sizeof(TPMPARAMS) == %d (expected 20)", sizeof(TPMPARAMS));

    /* TRACKMOUSEEVENT */
    ok(FIELD_OFFSET(TRACKMOUSEEVENT, cbSize) == 0,
       "FIELD_OFFSET(TRACKMOUSEEVENT, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(TRACKMOUSEEVENT, cbSize)); /* DWORD */
    ok(FIELD_OFFSET(TRACKMOUSEEVENT, dwFlags) == 4,
       "FIELD_OFFSET(TRACKMOUSEEVENT, dwFlags) == %ld (expected 4)",
       FIELD_OFFSET(TRACKMOUSEEVENT, dwFlags)); /* DWORD */
    ok(FIELD_OFFSET(TRACKMOUSEEVENT, hwndTrack) == 8,
       "FIELD_OFFSET(TRACKMOUSEEVENT, hwndTrack) == %ld (expected 8)",
       FIELD_OFFSET(TRACKMOUSEEVENT, hwndTrack)); /* HWND */
    ok(FIELD_OFFSET(TRACKMOUSEEVENT, dwHoverTime) == 12,
       "FIELD_OFFSET(TRACKMOUSEEVENT, dwHoverTime) == %ld (expected 12)",
       FIELD_OFFSET(TRACKMOUSEEVENT, dwHoverTime)); /* DWORD */
    ok(sizeof(TRACKMOUSEEVENT) == 16, "sizeof(TRACKMOUSEEVENT) == %d (expected 16)", sizeof(TRACKMOUSEEVENT));

    /* WINDOWINFO */
    ok(FIELD_OFFSET(WINDOWINFO, cbSize) == 0,
       "FIELD_OFFSET(WINDOWINFO, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(WINDOWINFO, cbSize)); /* DWORD */
    ok(FIELD_OFFSET(WINDOWINFO, rcWindow) == 4,
       "FIELD_OFFSET(WINDOWINFO, rcWindow) == %ld (expected 4)",
       FIELD_OFFSET(WINDOWINFO, rcWindow)); /* RECT */
    ok(FIELD_OFFSET(WINDOWINFO, rcClient) == 20,
       "FIELD_OFFSET(WINDOWINFO, rcClient) == %ld (expected 20)",
       FIELD_OFFSET(WINDOWINFO, rcClient)); /* RECT */
    ok(FIELD_OFFSET(WINDOWINFO, dwStyle) == 36,
       "FIELD_OFFSET(WINDOWINFO, dwStyle) == %ld (expected 36)",
       FIELD_OFFSET(WINDOWINFO, dwStyle)); /* DWORD */
    ok(FIELD_OFFSET(WINDOWINFO, dwExStyle) == 40,
       "FIELD_OFFSET(WINDOWINFO, dwExStyle) == %ld (expected 40)",
       FIELD_OFFSET(WINDOWINFO, dwExStyle)); /* DWORD */
    ok(FIELD_OFFSET(WINDOWINFO, dwWindowStatus) == 44,
       "FIELD_OFFSET(WINDOWINFO, dwWindowStatus) == %ld (expected 44)",
       FIELD_OFFSET(WINDOWINFO, dwWindowStatus)); /* DWORD */
    ok(FIELD_OFFSET(WINDOWINFO, cxWindowBorders) == 48,
       "FIELD_OFFSET(WINDOWINFO, cxWindowBorders) == %ld (expected 48)",
       FIELD_OFFSET(WINDOWINFO, cxWindowBorders)); /* UINT */
    ok(FIELD_OFFSET(WINDOWINFO, cyWindowBorders) == 52,
       "FIELD_OFFSET(WINDOWINFO, cyWindowBorders) == %ld (expected 52)",
       FIELD_OFFSET(WINDOWINFO, cyWindowBorders)); /* UINT */
    ok(FIELD_OFFSET(WINDOWINFO, atomWindowType) == 56,
       "FIELD_OFFSET(WINDOWINFO, atomWindowType) == %ld (expected 56)",
       FIELD_OFFSET(WINDOWINFO, atomWindowType)); /* ATOM */
    ok(FIELD_OFFSET(WINDOWINFO, wCreatorVersion) == 58,
       "FIELD_OFFSET(WINDOWINFO, wCreatorVersion) == %ld (expected 58)",
       FIELD_OFFSET(WINDOWINFO, wCreatorVersion)); /* WORD */
    ok(sizeof(WINDOWINFO) == 60, "sizeof(WINDOWINFO) == %d (expected 60)", sizeof(WINDOWINFO));

    /* WINDOWPLACEMENT */
    ok(FIELD_OFFSET(WINDOWPLACEMENT, length) == 0,
       "FIELD_OFFSET(WINDOWPLACEMENT, length) == %ld (expected 0)",
       FIELD_OFFSET(WINDOWPLACEMENT, length)); /* UINT */
    ok(FIELD_OFFSET(WINDOWPLACEMENT, flags) == 4,
       "FIELD_OFFSET(WINDOWPLACEMENT, flags) == %ld (expected 4)",
       FIELD_OFFSET(WINDOWPLACEMENT, flags)); /* UINT */
    ok(FIELD_OFFSET(WINDOWPLACEMENT, showCmd) == 8,
       "FIELD_OFFSET(WINDOWPLACEMENT, showCmd) == %ld (expected 8)",
       FIELD_OFFSET(WINDOWPLACEMENT, showCmd)); /* UINT */
    ok(FIELD_OFFSET(WINDOWPLACEMENT, ptMinPosition) == 12,
       "FIELD_OFFSET(WINDOWPLACEMENT, ptMinPosition) == %ld (expected 12)",
       FIELD_OFFSET(WINDOWPLACEMENT, ptMinPosition)); /* POINT */
    ok(FIELD_OFFSET(WINDOWPLACEMENT, ptMaxPosition) == 20,
       "FIELD_OFFSET(WINDOWPLACEMENT, ptMaxPosition) == %ld (expected 20)",
       FIELD_OFFSET(WINDOWPLACEMENT, ptMaxPosition)); /* POINT */
    ok(FIELD_OFFSET(WINDOWPLACEMENT, rcNormalPosition) == 28,
       "FIELD_OFFSET(WINDOWPLACEMENT, rcNormalPosition) == %ld (expected 28)",
       FIELD_OFFSET(WINDOWPLACEMENT, rcNormalPosition)); /* RECT */
    ok(sizeof(WINDOWPLACEMENT) == 44, "sizeof(WINDOWPLACEMENT) == %d (expected 44)", sizeof(WINDOWPLACEMENT));

    /* WINDOWPOS */
    ok(FIELD_OFFSET(WINDOWPOS, hwnd) == 0,
       "FIELD_OFFSET(WINDOWPOS, hwnd) == %ld (expected 0)",
       FIELD_OFFSET(WINDOWPOS, hwnd)); /* HWND */
    ok(FIELD_OFFSET(WINDOWPOS, hwndInsertAfter) == 4,
       "FIELD_OFFSET(WINDOWPOS, hwndInsertAfter) == %ld (expected 4)",
       FIELD_OFFSET(WINDOWPOS, hwndInsertAfter)); /* HWND */
    ok(FIELD_OFFSET(WINDOWPOS, x) == 8,
       "FIELD_OFFSET(WINDOWPOS, x) == %ld (expected 8)",
       FIELD_OFFSET(WINDOWPOS, x)); /* INT */
    ok(FIELD_OFFSET(WINDOWPOS, y) == 12,
       "FIELD_OFFSET(WINDOWPOS, y) == %ld (expected 12)",
       FIELD_OFFSET(WINDOWPOS, y)); /* INT */
    ok(FIELD_OFFSET(WINDOWPOS, cx) == 16,
       "FIELD_OFFSET(WINDOWPOS, cx) == %ld (expected 16)",
       FIELD_OFFSET(WINDOWPOS, cx)); /* INT */
    ok(FIELD_OFFSET(WINDOWPOS, cy) == 20,
       "FIELD_OFFSET(WINDOWPOS, cy) == %ld (expected 20)",
       FIELD_OFFSET(WINDOWPOS, cy)); /* INT */
    ok(FIELD_OFFSET(WINDOWPOS, flags) == 24,
       "FIELD_OFFSET(WINDOWPOS, flags) == %ld (expected 24)",
       FIELD_OFFSET(WINDOWPOS, flags)); /* UINT */
    ok(sizeof(WINDOWPOS) == 28, "sizeof(WINDOWPOS) == %d (expected 28)", sizeof(WINDOWPOS));

    /* WNDCLASSA */
    ok(FIELD_OFFSET(WNDCLASSA, style) == 0,
       "FIELD_OFFSET(WNDCLASSA, style) == %ld (expected 0)",
       FIELD_OFFSET(WNDCLASSA, style)); /* UINT */
    ok(FIELD_OFFSET(WNDCLASSA, lpfnWndProc) == 4,
       "FIELD_OFFSET(WNDCLASSA, lpfnWndProc) == %ld (expected 4)",
       FIELD_OFFSET(WNDCLASSA, lpfnWndProc)); /* WNDPROC */
    ok(FIELD_OFFSET(WNDCLASSA, cbClsExtra) == 8,
       "FIELD_OFFSET(WNDCLASSA, cbClsExtra) == %ld (expected 8)",
       FIELD_OFFSET(WNDCLASSA, cbClsExtra)); /* INT */
    ok(FIELD_OFFSET(WNDCLASSA, cbWndExtra) == 12,
       "FIELD_OFFSET(WNDCLASSA, cbWndExtra) == %ld (expected 12)",
       FIELD_OFFSET(WNDCLASSA, cbWndExtra)); /* INT */
    ok(FIELD_OFFSET(WNDCLASSA, hInstance) == 16,
       "FIELD_OFFSET(WNDCLASSA, hInstance) == %ld (expected 16)",
       FIELD_OFFSET(WNDCLASSA, hInstance)); /* HINSTANCE */
    ok(FIELD_OFFSET(WNDCLASSA, hIcon) == 20,
       "FIELD_OFFSET(WNDCLASSA, hIcon) == %ld (expected 20)",
       FIELD_OFFSET(WNDCLASSA, hIcon)); /* HICON */
    ok(FIELD_OFFSET(WNDCLASSA, hCursor) == 24,
       "FIELD_OFFSET(WNDCLASSA, hCursor) == %ld (expected 24)",
       FIELD_OFFSET(WNDCLASSA, hCursor)); /* HCURSOR */
    ok(FIELD_OFFSET(WNDCLASSA, hbrBackground) == 28,
       "FIELD_OFFSET(WNDCLASSA, hbrBackground) == %ld (expected 28)",
       FIELD_OFFSET(WNDCLASSA, hbrBackground)); /* HBRUSH */
    ok(FIELD_OFFSET(WNDCLASSA, lpszMenuName) == 32,
       "FIELD_OFFSET(WNDCLASSA, lpszMenuName) == %ld (expected 32)",
       FIELD_OFFSET(WNDCLASSA, lpszMenuName)); /* LPCSTR */
    ok(FIELD_OFFSET(WNDCLASSA, lpszClassName) == 36,
       "FIELD_OFFSET(WNDCLASSA, lpszClassName) == %ld (expected 36)",
       FIELD_OFFSET(WNDCLASSA, lpszClassName)); /* LPCSTR */
    ok(sizeof(WNDCLASSA) == 40, "sizeof(WNDCLASSA) == %d (expected 40)", sizeof(WNDCLASSA));

    /* WNDCLASSEXA */
    ok(FIELD_OFFSET(WNDCLASSEXA, cbSize) == 0,
       "FIELD_OFFSET(WNDCLASSEXA, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(WNDCLASSEXA, cbSize)); /* UINT */
    ok(FIELD_OFFSET(WNDCLASSEXA, style) == 4,
       "FIELD_OFFSET(WNDCLASSEXA, style) == %ld (expected 4)",
       FIELD_OFFSET(WNDCLASSEXA, style)); /* UINT */
    ok(FIELD_OFFSET(WNDCLASSEXA, lpfnWndProc) == 8,
       "FIELD_OFFSET(WNDCLASSEXA, lpfnWndProc) == %ld (expected 8)",
       FIELD_OFFSET(WNDCLASSEXA, lpfnWndProc)); /* WNDPROC */
    ok(FIELD_OFFSET(WNDCLASSEXA, cbClsExtra) == 12,
       "FIELD_OFFSET(WNDCLASSEXA, cbClsExtra) == %ld (expected 12)",
       FIELD_OFFSET(WNDCLASSEXA, cbClsExtra)); /* INT */
    ok(FIELD_OFFSET(WNDCLASSEXA, cbWndExtra) == 16,
       "FIELD_OFFSET(WNDCLASSEXA, cbWndExtra) == %ld (expected 16)",
       FIELD_OFFSET(WNDCLASSEXA, cbWndExtra)); /* INT */
    ok(FIELD_OFFSET(WNDCLASSEXA, hInstance) == 20,
       "FIELD_OFFSET(WNDCLASSEXA, hInstance) == %ld (expected 20)",
       FIELD_OFFSET(WNDCLASSEXA, hInstance)); /* HINSTANCE */
    ok(FIELD_OFFSET(WNDCLASSEXA, hIcon) == 24,
       "FIELD_OFFSET(WNDCLASSEXA, hIcon) == %ld (expected 24)",
       FIELD_OFFSET(WNDCLASSEXA, hIcon)); /* HICON */
    ok(FIELD_OFFSET(WNDCLASSEXA, hCursor) == 28,
       "FIELD_OFFSET(WNDCLASSEXA, hCursor) == %ld (expected 28)",
       FIELD_OFFSET(WNDCLASSEXA, hCursor)); /* HCURSOR */
    ok(FIELD_OFFSET(WNDCLASSEXA, hbrBackground) == 32,
       "FIELD_OFFSET(WNDCLASSEXA, hbrBackground) == %ld (expected 32)",
       FIELD_OFFSET(WNDCLASSEXA, hbrBackground)); /* HBRUSH */
    ok(FIELD_OFFSET(WNDCLASSEXA, lpszMenuName) == 36,
       "FIELD_OFFSET(WNDCLASSEXA, lpszMenuName) == %ld (expected 36)",
       FIELD_OFFSET(WNDCLASSEXA, lpszMenuName)); /* LPCSTR */
    ok(FIELD_OFFSET(WNDCLASSEXA, lpszClassName) == 40,
       "FIELD_OFFSET(WNDCLASSEXA, lpszClassName) == %ld (expected 40)",
       FIELD_OFFSET(WNDCLASSEXA, lpszClassName)); /* LPCSTR */
    ok(FIELD_OFFSET(WNDCLASSEXA, hIconSm) == 44,
       "FIELD_OFFSET(WNDCLASSEXA, hIconSm) == %ld (expected 44)",
       FIELD_OFFSET(WNDCLASSEXA, hIconSm)); /* HICON */
    ok(sizeof(WNDCLASSEXA) == 48, "sizeof(WNDCLASSEXA) == %d (expected 48)", sizeof(WNDCLASSEXA));

    /* WNDCLASSEXW */
    ok(FIELD_OFFSET(WNDCLASSEXW, cbSize) == 0,
       "FIELD_OFFSET(WNDCLASSEXW, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(WNDCLASSEXW, cbSize)); /* UINT */
    ok(FIELD_OFFSET(WNDCLASSEXW, style) == 4,
       "FIELD_OFFSET(WNDCLASSEXW, style) == %ld (expected 4)",
       FIELD_OFFSET(WNDCLASSEXW, style)); /* UINT */
    ok(FIELD_OFFSET(WNDCLASSEXW, lpfnWndProc) == 8,
       "FIELD_OFFSET(WNDCLASSEXW, lpfnWndProc) == %ld (expected 8)",
       FIELD_OFFSET(WNDCLASSEXW, lpfnWndProc)); /* WNDPROC */
    ok(FIELD_OFFSET(WNDCLASSEXW, cbClsExtra) == 12,
       "FIELD_OFFSET(WNDCLASSEXW, cbClsExtra) == %ld (expected 12)",
       FIELD_OFFSET(WNDCLASSEXW, cbClsExtra)); /* INT */
    ok(FIELD_OFFSET(WNDCLASSEXW, cbWndExtra) == 16,
       "FIELD_OFFSET(WNDCLASSEXW, cbWndExtra) == %ld (expected 16)",
       FIELD_OFFSET(WNDCLASSEXW, cbWndExtra)); /* INT */
    ok(FIELD_OFFSET(WNDCLASSEXW, hInstance) == 20,
       "FIELD_OFFSET(WNDCLASSEXW, hInstance) == %ld (expected 20)",
       FIELD_OFFSET(WNDCLASSEXW, hInstance)); /* HINSTANCE */
    ok(FIELD_OFFSET(WNDCLASSEXW, hIcon) == 24,
       "FIELD_OFFSET(WNDCLASSEXW, hIcon) == %ld (expected 24)",
       FIELD_OFFSET(WNDCLASSEXW, hIcon)); /* HICON */
    ok(FIELD_OFFSET(WNDCLASSEXW, hCursor) == 28,
       "FIELD_OFFSET(WNDCLASSEXW, hCursor) == %ld (expected 28)",
       FIELD_OFFSET(WNDCLASSEXW, hCursor)); /* HCURSOR */
    ok(FIELD_OFFSET(WNDCLASSEXW, hbrBackground) == 32,
       "FIELD_OFFSET(WNDCLASSEXW, hbrBackground) == %ld (expected 32)",
       FIELD_OFFSET(WNDCLASSEXW, hbrBackground)); /* HBRUSH */
    ok(FIELD_OFFSET(WNDCLASSEXW, lpszMenuName) == 36,
       "FIELD_OFFSET(WNDCLASSEXW, lpszMenuName) == %ld (expected 36)",
       FIELD_OFFSET(WNDCLASSEXW, lpszMenuName)); /* LPCWSTR */
    ok(FIELD_OFFSET(WNDCLASSEXW, lpszClassName) == 40,
       "FIELD_OFFSET(WNDCLASSEXW, lpszClassName) == %ld (expected 40)",
       FIELD_OFFSET(WNDCLASSEXW, lpszClassName)); /* LPCWSTR */
    ok(FIELD_OFFSET(WNDCLASSEXW, hIconSm) == 44,
       "FIELD_OFFSET(WNDCLASSEXW, hIconSm) == %ld (expected 44)",
       FIELD_OFFSET(WNDCLASSEXW, hIconSm)); /* HICON */
    ok(sizeof(WNDCLASSEXW) == 48, "sizeof(WNDCLASSEXW) == %d (expected 48)", sizeof(WNDCLASSEXW));

    /* WNDCLASSW */
    ok(FIELD_OFFSET(WNDCLASSW, style) == 0,
       "FIELD_OFFSET(WNDCLASSW, style) == %ld (expected 0)",
       FIELD_OFFSET(WNDCLASSW, style)); /* UINT */
    ok(FIELD_OFFSET(WNDCLASSW, lpfnWndProc) == 4,
       "FIELD_OFFSET(WNDCLASSW, lpfnWndProc) == %ld (expected 4)",
       FIELD_OFFSET(WNDCLASSW, lpfnWndProc)); /* WNDPROC */
    ok(FIELD_OFFSET(WNDCLASSW, cbClsExtra) == 8,
       "FIELD_OFFSET(WNDCLASSW, cbClsExtra) == %ld (expected 8)",
       FIELD_OFFSET(WNDCLASSW, cbClsExtra)); /* INT */
    ok(FIELD_OFFSET(WNDCLASSW, cbWndExtra) == 12,
       "FIELD_OFFSET(WNDCLASSW, cbWndExtra) == %ld (expected 12)",
       FIELD_OFFSET(WNDCLASSW, cbWndExtra)); /* INT */
    ok(FIELD_OFFSET(WNDCLASSW, hInstance) == 16,
       "FIELD_OFFSET(WNDCLASSW, hInstance) == %ld (expected 16)",
       FIELD_OFFSET(WNDCLASSW, hInstance)); /* HINSTANCE */
    ok(FIELD_OFFSET(WNDCLASSW, hIcon) == 20,
       "FIELD_OFFSET(WNDCLASSW, hIcon) == %ld (expected 20)",
       FIELD_OFFSET(WNDCLASSW, hIcon)); /* HICON */
    ok(FIELD_OFFSET(WNDCLASSW, hCursor) == 24,
       "FIELD_OFFSET(WNDCLASSW, hCursor) == %ld (expected 24)",
       FIELD_OFFSET(WNDCLASSW, hCursor)); /* HCURSOR */
    ok(FIELD_OFFSET(WNDCLASSW, hbrBackground) == 28,
       "FIELD_OFFSET(WNDCLASSW, hbrBackground) == %ld (expected 28)",
       FIELD_OFFSET(WNDCLASSW, hbrBackground)); /* HBRUSH */
    ok(FIELD_OFFSET(WNDCLASSW, lpszMenuName) == 32,
       "FIELD_OFFSET(WNDCLASSW, lpszMenuName) == %ld (expected 32)",
       FIELD_OFFSET(WNDCLASSW, lpszMenuName)); /* LPCWSTR */
    ok(FIELD_OFFSET(WNDCLASSW, lpszClassName) == 36,
       "FIELD_OFFSET(WNDCLASSW, lpszClassName) == %ld (expected 36)",
       FIELD_OFFSET(WNDCLASSW, lpszClassName)); /* LPCWSTR */
    ok(sizeof(WNDCLASSW) == 40, "sizeof(WNDCLASSW) == %d (expected 40)", sizeof(WNDCLASSW));

}

START_TEST(generated)
{
    test_pack();
}
