/*
 * USER definitions
 *
 * Copyright 1993 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_USER_H
#define __WINE_USER_H

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>

#include <local.h>

extern WORD USER_HeapSel;

#define USER_HEAP_ALLOC(size) \
            ((HANDLE)(ULONG_PTR)LOCAL_Alloc( USER_HeapSel, LMEM_FIXED, (size) ))
#define USER_HEAP_REALLOC(handle,size) \
            ((HANDLE)(ULONG_PTR)LOCAL_ReAlloc( USER_HeapSel, LOWORD(handle), (size), LMEM_FIXED ))
#define USER_HEAP_FREE(handle) \
            LOCAL_Free( USER_HeapSel, LOWORD(handle) )
#define USER_HEAP_LIN_ADDR(handle)  \
         ((handle) ? MapSL(MAKESEGPTR(USER_HeapSel, LOWORD(handle))) : NULL)

#define GET_WORD(ptr)  (*(WORD *)(ptr))
#define GET_DWORD(ptr) (*(DWORD *)(ptr))

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
    WM_WINE_ENABLEWINDOW,
    WM_WINE_SETACTIVEWINDOW,
    WM_WINE_KEYBOARD_LL_HOOK,
    WM_WINE_MOUSE_LL_HOOK
};

/* internal SendInput codes (FIXME) */
#define WINE_INTERNAL_INPUT_MOUSE    (16+INPUT_MOUSE)
#define WINE_INTERNAL_INPUT_KEYBOARD (16+INPUT_KEYBOARD)

typedef struct tagUSER_DRIVER {
    /* keyboard functions */
    void   (*pInitKeyboard)(LPBYTE);
    SHORT  (*pVkKeyScanEx)(WCHAR, HKL);
    UINT   (*pMapVirtualKeyEx)(UINT, UINT, HKL);
    INT    (*pGetKeyNameText)(LONG, LPWSTR, INT);
    INT    (*pToUnicodeEx)(UINT, UINT, LPBYTE, LPWSTR, int, UINT, HKL);
    UINT   (*pGetKeyboardLayoutList)(INT, HKL *);
    HKL    (*pGetKeyboardLayout)(DWORD);
    BOOL   (*pGetKeyboardLayoutName)(LPWSTR);
    HKL    (*pLoadKeyboardLayout)(LPCWSTR, UINT);
    HKL    (*pActivateKeyboardLayout)(HKL, UINT);
    BOOL   (*pUnloadKeyboardLayout)(HKL);
    void   (*pBeep)(void);
    /* mouse functions */
    void   (*pInitMouse)(LPBYTE);
    void   (*pSetCursor)(struct tagCURSORICONINFO *);
    void   (*pGetCursorPos)(LPPOINT);
    void   (*pSetCursorPos)(INT,INT);
    /* screen saver functions */
    BOOL   (*pGetScreenSaveActive)(void);
    void   (*pSetScreenSaveActive)(BOOL);
    /* clipboard functions */
    void   (*pAcquireClipboard)(HWND);                     /* Acquire selection */
    BOOL   (*pCountClipboardFormats)(void);                /* Count available clipboard formats */
    void   (*pEmptyClipboard)(void);                       /* Empty clipboard data */
    BOOL   (*pEndClipboardUpdate)(void);                   /* End clipboard update */
    BOOL   (*pEnumClipboardFormats)(UINT);                 /* Enumerate clipboard formats */
    BOOL   (*pGetClipboardData)(UINT, HANDLE16*, HANDLE*); /* Get specified selection data */
    BOOL   (*pGetClipboardFormatName)(UINT, LPSTR, UINT);  /* Get a clipboard format name */
    BOOL   (*pIsClipboardFormatAvailable)(UINT);           /* Check if specified format is available */
    INT    (*pRegisterClipboardFormat)(LPCSTR);            /* Register a clipboard format */
    void   (*pResetSelectionOwner)(HWND, BOOL);
    BOOL   (*pSetClipboardData)(UINT, HANDLE16, HANDLE);   /* Set specified selection data */
    /* display modes */
    LONG   (*pChangeDisplaySettingsExW)(LPCWSTR,LPDEVMODEW,HWND,DWORD,LPVOID);
    BOOL   (*pEnumDisplaySettingsExW)(LPCWSTR,DWORD,LPDEVMODEW,DWORD);
    /* windowing functions */
    BOOL   (*pCreateWindow)(HWND,CREATESTRUCTA*,BOOL);
    BOOL   (*pDestroyWindow)(HWND);
    BOOL   (*pGetDC)(HWND,HDC,HRGN,DWORD);
    void   (*pForceWindowRaise)(HWND);
    DWORD  (*pMsgWaitForMultipleObjectsEx)(DWORD,const HANDLE*,DWORD,DWORD,DWORD);
    void   (*pReleaseDC)(HWND,HDC);
    INT    (*pScrollWindowEx)(HWND,INT,INT,const RECT*,const RECT*,HRGN,LPRECT,UINT);
    void   (*pSetFocus)(HWND);
    HWND   (*pSetParent)(HWND,HWND);
    BOOL   (*pSetWindowPos)(WINDOWPOS *);
    int    (*pSetWindowRgn)(HWND,HRGN,BOOL);
    void   (*pSetWindowIcon)(HWND,UINT,HICON);
    void   (*pSetWindowStyle)(HWND,DWORD);
    BOOL   (*pSetWindowText)(HWND,LPCWSTR);
    BOOL   (*pShowWindow)(HWND,INT);
    void   (*pSysCommandSizeMove)(HWND,WPARAM);
} USER_DRIVER;

extern USER_DRIVER USER_Driver;

extern HMODULE user32_module;

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

/* hook.c */
extern LRESULT HOOK_CallHooks( INT id, INT code, WPARAM wparam, LPARAM lparam, BOOL unicode );
extern BOOL HOOK_IsHooked( INT id );

/* input.c */
extern BYTE InputKeyStateTable[256];
extern BYTE AsyncKeyStateTable[256];

/* syscolor.c */
extern void SYSCOLOR_Init(void);
extern HPEN SYSCOLOR_GetPen( INT index );

/* sysmetrics.c */
extern void SYSMETRICS_Init(void);
extern INT SYSMETRICS_Set( INT index, INT value );

/* sysparams.c */
extern void SYSPARAMS_GetDoubleClickSize( INT *width, INT *height );
extern INT SYSPARAMS_GetMouseButtonSwap( void );

extern HPALETTE WINAPI SelectPalette( HDC hDC, HPALETTE hPal, BOOL bForceBackground );

extern BOOL CLIPBOARD_ReleaseOwner(void);

extern DWORD USER16_AlertableWait;

/* HANDLE16 <-> HANDLE conversions */
#define HCURSOR_16(h32)    (LOWORD(h32))
#define HICON_16(h32)      (LOWORD(h32))
#define HINSTANCE_16(h32)  (LOWORD(h32))

#define HCURSOR_32(h16)    ((HCURSOR)(ULONG_PTR)(h16))
#define HICON_32(h16)      ((HICON)(ULONG_PTR)(h16))
#define HINSTANCE_32(h16)  ((HINSTANCE)(ULONG_PTR)(h16))
#define HMODULE_32(h16)    ((HMODULE)(ULONG_PTR)(h16))

#endif  /* __WINE_USER_H */
