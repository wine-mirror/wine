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
struct DIDEVICEOBJECTDATA;

typedef VOID CALLBACK (*LPMOUSE_EVENT_PROC)(DWORD,DWORD,DWORD,DWORD,DWORD);

struct tagWND;

typedef struct tagUSER_DRIVER {
    /* event functions */
    void   (*pUserRepaintDisable)(BOOL);
    /* keyboard functions */
    void   (*pInitKeyboard)(void);
    WORD   (*pVkKeyScan)(CHAR);
    UINT16 (*pMapVirtualKey)(UINT16, UINT16);
    INT16  (*pGetKeyNameText)(LONG, LPSTR, INT16);
    INT    (*pToUnicode)(UINT, UINT, LPBYTE, LPWSTR, int, UINT);
    BOOL   (*pGetBeepActive)(void);
    void   (*pSetBeepActive)(BOOL);
    void   (*pBeep)(void);
    BOOL   (*pGetDIState)(DWORD, LPVOID);
    BOOL   (*pGetDIData)(BYTE *, DWORD, struct DIDEVICEOBJECTDATA *, LPDWORD, DWORD);
    /* mouse functions */
    void   (*pInitMouse)(LPMOUSE_EVENT_PROC);
    void   (*pSetCursor)(struct tagCURSORICONINFO *);
    void   (*pMoveCursor)(WORD, WORD);
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
    void   (*pResetSelectionOwner)(struct tagWND *, BOOL);

    /* windowing functions */
    BOOL   (*pCreateWindow)(HWND);
    BOOL   (*pDestroyWindow)(HWND);
    BOOL   (*pGetDC)(HWND,HDC,HRGN,DWORD);
    BOOL   (*pEnableWindow)(HWND,BOOL);
    void   (*pSetFocus)(HWND);
    HWND   (*pSetParent)(HWND,HWND);
    BOOL   (*pSetWindowPos)(WINDOWPOS *);
    BOOL   (*pSetWindowRgn)(HWND,HRGN,BOOL);
    HICON  (*pSetWindowIcon)(HWND,HICON,BOOL);
    BOOL   (*pSetWindowText)(HWND,LPCWSTR);
    BOOL   (*pIsSingleWindow)(void);
} USER_DRIVER;

extern USER_DRIVER USER_Driver;

WORD WINAPI UserSignalProc( UINT uCode, DWORD dwThreadOrProcessID,
                            DWORD dwFlags, HMODULE16 hModule );

VOID WINAPI MOUSE_Enable(LPMOUSE_EVENT_PROC lpMouseEventProc);
VOID WINAPI MOUSE_Disable(VOID);

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
