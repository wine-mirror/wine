/*
 * TTY driver definitions
 */

#ifndef __WINE_TTYDRV_H
#define __WINE_TTYDRV_H

#include "clipboard.h"
#include "keyboard.h"
#include "message.h"

/* TTY clipboard driver */

extern CLIPBOARD_DRIVER TTYDRV_CLIPBOARD_Driver;

extern void TTYDRV_CLIPBOARD_EmptyClipboard();
extern void TTYDRV_CLIPBOARD_SetClipboardData(UINT32 wFormat);
extern BOOL32 TTYDRV_CLIPBOARD_RequestSelection();
extern void TTYDRV_CLIPBOARD_ResetOwner(WND *pWnd, BOOL32 bFooBar);

/* TTY event driver */

extern EVENT_DRIVER TTYDRV_EVENT_Driver;

extern BOOL32 TTYDRV_EVENT_Init(void);
extern void TTYDRV_EVENT_AddIO(int fd, unsigned flag);
extern void TTYDRV_EVENT_DeleteIO(int fd, unsigned flag);
extern BOOL32 TTYDRV_EVENT_WaitNetEvent(BOOL32 sleep, BOOL32 peek);
extern void TTYDRV_EVENT_Synchronize(void);
extern BOOL32 TTYDRV_EVENT_CheckFocus( void );
extern BOOL32 TTYDRV_EVENT_QueryPointer(DWORD *posX, DWORD *posY, DWORD *state);
extern void TTYDRV_EVENT_DummyMotionNotify(void);
extern BOOL32 TTYDRV_EVENT_Pending(void);
extern BOOL16 TTYDRV_EVENT_IsUserIdle(void);

/* TTY keyboard driver */

extern KEYBOARD_DRIVER TTYDRV_KEYBOARD_Driver;

extern void TTYDRV_KEYBOARD_Init(void);
extern WORD TTYDRV_KEYBOARD_VkKeyScan(CHAR cChar);
extern UINT16 TTYDRV_KEYBOARD_MapVirtualKey(UINT16 wCode, UINT16 wMapType);
extern INT16 TTYDRV_KEYBOARD_GetKeyNameText(LONG lParam, LPSTR lpBuffer, INT16 nSize);
extern INT16 TTYDRV_KEYBOARD_ToAscii(UINT16 virtKey, UINT16 scanCode, LPBYTE lpKeyState, LPVOID lpChar, UINT16 flags);

#endif /* !defined(__WINE_TTYDRV_H) */
