/*
 * USER definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_USER_H
#define __WINE_USER_H

#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "local.h"

extern WORD USER_HeapSel;

#define USER_HEAP_ALLOC(size) \
            LOCAL_Alloc( USER_HeapSel, LMEM_FIXED, (size) )
#define USER_HEAP_REALLOC(handle,size) \
            LOCAL_ReAlloc( USER_HeapSel, (handle), (size), LMEM_FIXED )
#define USER_HEAP_FREE(handle) \
            LOCAL_Free( USER_HeapSel, (handle) )
#define USER_HEAP_LIN_ADDR(handle)  \
         ((handle) ? MapSL(MAKESEGPTR(USER_HeapSel, (handle))) : NULL)

#define USUD_LOCALALLOC        0x0001
#define USUD_LOCALFREE         0x0002
#define USUD_LOCALCOMPACT      0x0003
#define USUD_LOCALHEAP         0x0004
#define USUD_FIRSTCLASS        0x0005

struct tagCURSORICONINFO;

/* internal messages codes */
enum wine_internal_message
{
    WM_WINE_DESTROYWINDOW = 0x80000000,
    WM_WINE_SETWINDOWPOS,
    WM_WINE_SHOWWINDOW,
    WM_WINE_SETPARENT,
    WM_WINE_SETWINDOWLONG,
    WM_WINE_ENABLEWINDOW
};

/* internal SendInput codes (FIXME) */
#define WINE_INTERNAL_INPUT_MOUSE    (16+INPUT_MOUSE)
#define WINE_INTERNAL_INPUT_KEYBOARD (16+INPUT_KEYBOARD)

typedef struct tagUSER_DRIVER {
    /* keyboard functions */
    void   (*pInitKeyboard)(LPBYTE);
    WORD   (*pVkKeyScan)(CHAR);
    UINT   (*pMapVirtualKey)(UINT,UINT);
    INT    (*pGetKeyNameText)(LONG,LPSTR,INT);
    INT    (*pToUnicode)(UINT, UINT, LPBYTE, LPWSTR, int, UINT);
    void   (*pBeep)(void);
    /* mouse functions */
    void   (*pInitMouse)(LPBYTE);
    void   (*pSetCursor)(struct tagCURSORICONINFO *);
    void   (*pGetCursorPos)(LPPOINT);
    void   (*pSetCursorPos)(INT,INT);
    /* screen saver functions */
    BOOL   (*pGetScreenSaveActive)(void);
    void   (*pSetScreenSaveActive)(BOOL);
    int    (*pGetScreenSaveTimeout)(void);
    void   (*pSetScreenSaveTimeout)(int);
    /* resource functions */
    HANDLE (*pLoadOEMResource)(WORD,WORD);
    /* clipboard functions */
    void   (*pAcquireClipboard)(void);            /* Acquire selection */
    void   (*pReleaseClipboard)(void);            /* Release selection */
    void   (*pSetClipboardData)(UINT);            /* Set specified selection data */
    BOOL   (*pGetClipboardData)(UINT);            /* Get specified selection data */
    BOOL   (*pIsClipboardFormatAvailable)(UINT);  /* Check if specified format is available */
    BOOL   (*pRegisterClipboardFormat)(LPCSTR);   /* Register a clipboard format */
    BOOL   (*pIsSelectionOwner)(void);            /* Check if we own the selection */
    void   (*pResetSelectionOwner)(HWND, BOOL);

    /* windowing functions */
    BOOL   (*pCreateWindow)(HWND,CREATESTRUCTA*,BOOL);
    BOOL   (*pDestroyWindow)(HWND);
    BOOL   (*pGetDC)(HWND,HDC,HRGN,DWORD);
    void   (*pForceWindowRaise)(HWND);
    DWORD  (*pMsgWaitForMultipleObjectsEx)(DWORD,const HANDLE*,DWORD,DWORD,DWORD);
    BOOL   (*pScrollDC)(HDC,INT,INT,const RECT*,const RECT*,HRGN,LPRECT);
    INT    (*pScrollWindowEx)(HWND,INT,INT,const RECT*,const RECT*,HRGN,LPRECT,UINT);
    void   (*pSetFocus)(HWND);
    HWND   (*pSetParent)(HWND,HWND);
    BOOL   (*pSetWindowPos)(WINDOWPOS *);
    int    (*pSetWindowRgn)(HWND,HRGN,BOOL);
    HICON  (*pSetWindowIcon)(HWND,HICON,BOOL);
    void   (*pSetWindowStyle)(HWND,DWORD);
    BOOL   (*pSetWindowText)(HWND,LPCWSTR);
    BOOL   (*pShowWindow)(HWND,INT);
    void   (*pSysCommandSizeMove)(HWND,WPARAM);
} USER_DRIVER;

extern USER_DRIVER USER_Driver;

WORD WINAPI UserSignalProc( UINT uCode, DWORD dwThreadOrProcessID,
                            DWORD dwFlags, HMODULE16 hModule );

/* user lock */
extern void USER_Lock(void);
extern void USER_Unlock(void);
extern void USER_CheckNotLock(void);

extern BOOL USER_IsExitingThread( DWORD tid );

/* Wine look */

typedef enum
{
    WIN31_LOOK,
    WIN95_LOOK,
    WIN98_LOOK
} WINE_LOOK;

extern WINE_LOOK TWEAK_WineLook;

/* gray brush cache */
extern HBRUSH CACHE_GetPattern55AABrush(void);

#endif  /* __WINE_USER_H */
