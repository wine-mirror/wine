/*
 * USER definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_USER_H
#define __WINE_USER_H

#include "ldt.h"
#include "local.h"

extern WORD USER_HeapSel;

#define USER_HEAP_ALLOC(size) \
            LOCAL_Alloc( USER_HeapSel, LMEM_FIXED, (size) )
#define USER_HEAP_REALLOC(handle,size) \
            LOCAL_ReAlloc( USER_HeapSel, (handle), (size), LMEM_FIXED )
#define USER_HEAP_FREE(handle) \
            LOCAL_Free( USER_HeapSel, (handle) )
#define USER_HEAP_LIN_ADDR(handle)  \
         ((handle) ? PTR_SEG_OFF_TO_LIN(USER_HeapSel, (handle)) : NULL)
#define USER_HEAP_SEG_ADDR(handle)  \
         ((handle) ? PTR_SEG_OFF_TO_SEGPTR(USER_HeapSel, (handle)) : (SEGPTR)0)

#define USUD_LOCALALLOC        0x0001
#define USUD_LOCALFREE         0x0002
#define USUD_LOCALCOMPACT      0x0003
#define USUD_LOCALHEAP         0x0004
#define USUD_FIRSTCLASS        0x0005

struct tagCURSORICONINFO;
struct DIDEVICEOBJECTDATA;

#define WINE_KEYBOARD_CONFIG_AUTO_REPEAT 0x00000001
typedef struct tagKEYBOARD_CONFIG {
  BOOL auto_repeat;
} KEYBOARD_CONFIG;

typedef VOID CALLBACK (*LPMOUSE_EVENT_PROC)(DWORD,DWORD,DWORD,DWORD,DWORD);

typedef struct tagUSER_DRIVER {
    /* event functions */
    void   (*pSynchronize)(void);
    BOOL   (*pCheckFocus)(void);
    void   (*pUserRepaintDisable)(BOOL);
    /* keyboard functions */
    void   (*pInitKeyboard)(void);
    WORD   (*pVkKeyScan)(CHAR);
    UINT16 (*pMapVirtualKey)(UINT16, UINT16);
    INT16  (*pGetKeyNameText)(LONG, LPSTR, INT16);
    INT16  (*pToAscii)(UINT16, UINT16, LPBYTE, LPVOID, UINT16);
    BOOL   (*pGetBeepActive)(void);
    void   (*pSetBeepActive)(BOOL);
    void   (*pBeep)(void);
    BOOL   (*pGetDIState)(DWORD, LPVOID);
    BOOL   (*pGetDIData)(BYTE *, DWORD, struct DIDEVICEOBJECTDATA *, LPDWORD, DWORD);
    void   (*pGetKeyboardConfig)(KEYBOARD_CONFIG *);
    void   (*pSetKeyboardConfig)(KEYBOARD_CONFIG *, DWORD);
    /* mouse functions */
    void   (*pInitMouse)(LPMOUSE_EVENT_PROC);
    void   (*pSetCursor)(struct tagCURSORICONINFO *);
    void   (*pMoveCursor)(WORD, WORD);
    /* screen saver functions */
    BOOL   (*pGetScreenSaveActive)(void);
    void   (*pSetScreenSaveActive)(BOOL);
    int    (*pGetScreenSaveTimeout)(void);
    void   (*pSetScreenSaveTimeout)(int);
    /* windowing functions */
    BOOL   (*pIsSingleWindow)(void);
} USER_DRIVER;

extern USER_DRIVER *USER_Driver;

WORD WINAPI UserSignalProc( UINT uCode, DWORD dwThreadOrProcessID,
                            DWORD dwFlags, HMODULE16 hModule );

VOID WINAPI MOUSE_Enable(LPMOUSE_EVENT_PROC lpMouseEventProc);
VOID WINAPI MOUSE_Disable(VOID);

#endif  /* __WINE_USER_H */
