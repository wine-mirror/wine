/*
 * KEYBOARD driver interface
 *
 * Copyright 1998 Ulrich Weigand
 */

#ifndef __WINE_KEYBOARD_H
#define __WINE_KEYBOARD_H

#pragma pack(1)
typedef struct _KBINFO
{
    BYTE Begin_First_Range;
    BYTE End_First_Range;
    BYTE Begin_Second_Range;
    BYTE End_Second_Range;
    WORD StateSize;
} KBINFO, *LPKBINFO;
#pragma pack(4)

typedef VOID (CALLBACK *LPKEYBD_EVENT_PROC)(BYTE,BYTE,DWORD,DWORD);

WORD WINAPI KEYBOARD_Inquire(LPKBINFO kbInfo);
VOID WINAPI KEYBOARD_Enable(LPKEYBD_EVENT_PROC lpKeybEventProc,
                            LPBYTE lpKeyState);
VOID WINAPI KEYBOARD_Disable(VOID);

/* Wine internals */

extern void KEYBOARD_HandleEvent( WND *pWnd, XKeyEvent *event );
extern void KEYBOARD_UpdateState( void );

#define WINE_KEYBDEVENT_MAGIC  ( ('K'<<24)|('E'<<16)|('Y'<<8)|'B' )
typedef struct _WINE_KEYBDEVENT
{
    DWORD magic;
    DWORD posX;
    DWORD posY;
    DWORD time;

} WINE_KEYBDEVENT;

#endif /* __WINE_KEYBOARD_H */

