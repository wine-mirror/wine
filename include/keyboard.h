/*
 * KEYBOARD driver interface
 *
 * Copyright 1998 Ulrich Weigand
 */

#ifndef __WINE_KEYBOARD_H
#define __WINE_KEYBOARD_H

#include "dinput.h"
#include "windef.h"

#include "pshpack1.h"
typedef struct _KBINFO
{
    BYTE Begin_First_Range;
    BYTE End_First_Range;
    BYTE Begin_Second_Range;
    BYTE End_Second_Range;
    WORD StateSize;
} KBINFO, *LPKBINFO;
#include "poppack.h"

typedef VOID (CALLBACK *LPKEYBD_EVENT_PROC)(BYTE,BYTE,DWORD,DWORD);

WORD WINAPI KEYBOARD_Inquire(LPKBINFO kbInfo);
VOID WINAPI KEYBOARD_Enable(LPKEYBD_EVENT_PROC lpKeybEventProc,
                            LPBYTE lpKeyState);
VOID WINAPI KEYBOARD_Disable(VOID);

/* Wine internals */

typedef struct tagKEYBOARD_DRIVER {
  void   (*pInit)(void);
  WORD   (*pVkKeyScan)(CHAR);
  UINT16 (*pMapVirtualKey)(UINT16, UINT16);
  INT16  (*pGetKeyNameText)(LONG, LPSTR, INT16);
  INT16  (*pToAscii)(UINT16, UINT16, LPBYTE, LPVOID, UINT16);
  BOOL   (*pGetBeepActive)(void);
  void   (*pSetBeepActive)(BOOL);
  void   (*pBeep)(void);
  BOOL   (*pGetDIState)(DWORD, LPVOID);
  BOOL   (*pGetDIData)(BYTE *, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);
} KEYBOARD_DRIVER;

extern KEYBOARD_DRIVER *KEYBOARD_Driver;

extern BOOL KEYBOARD_GetBeepActive(void);
extern void KEYBOARD_SetBeepActive(BOOL bActivate);
extern void KEYBOARD_Beep(void);

extern void KEYBOARD_SendEvent(BYTE bVk, BYTE bScan, DWORD dwFlags, DWORD posX, DWORD posY, DWORD time);

#define WINE_KEYBDEVENT_MAGIC  ( ('K'<<24)|('E'<<16)|('Y'<<8)|'B' )
typedef struct _WINE_KEYBDEVENT
{
    DWORD magic;
    DWORD posX;
    DWORD posY;
    DWORD time;

} WINE_KEYBDEVENT;

#endif /* __WINE_KEYBOARD_H */

