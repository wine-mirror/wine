/*
 * TTY driver definitions
 */

#ifndef __WINE_TTYDRV_H
#define __WINE_TTYDRV_H

#include "wintypes.h"
#include "wine/winuser16.h"

struct tagCLASS;
struct tagDC;
struct tagDESKTOP;
struct tagWND;

/* TTY GDI driver */

typedef struct {
} TTYDRV_PDEVICE;

extern BOOL32 TTYDRV_GDI_Initialize(void);
extern void TTDRV_GDI_Finalize(void);
extern BOOL32 TTYDRV_GDI_CreateDC(struct tagDC *dc, LPCSTR driver, LPCSTR device, LPCSTR output, const DEVMODE16 *initData);
extern BOOL32 TTYDRV_GDI_DeleteDC(struct tagDC *dc);
extern INT32 TTYDRV_GDI_Escape(struct tagDC *dc, INT32 nEscape, INT32 cbInput, SEGPTR lpInData, SEGPTR lpOutData);

/* TTY clipboard driver */

extern struct _CLIPBOARD_DRIVER TTYDRV_CLIPBOARD_Driver;

extern void TTYDRV_CLIPBOARD_EmptyClipboard(void);
extern void TTYDRV_CLIPBOARD_SetClipboardData(UINT32 wFormat);
extern BOOL32 TTYDRV_CLIPBOARD_RequestSelection(void);
extern void TTYDRV_CLIPBOARD_ResetOwner(struct tagWND *pWnd, BOOL32 bFooBar);

/* TTY desktop driver */

extern struct _DESKTOP_DRIVER TTYDRV_DESKTOP_Driver;

extern void TTYDRV_DESKTOP_Initialize(struct tagDESKTOP *pDesktop);
extern void TTYDRV_DESKTOP_Finalize(struct tagDESKTOP *pDesktop);
extern int TTYDRV_DESKTOP_GetScreenWidth(struct tagDESKTOP *pDesktop);
extern int TTYDRV_DESKTOP_GetScreenHeight(struct tagDESKTOP *pDesktop);
extern int TTYDRV_DESKTOP_GetScreenDepth(struct tagDESKTOP *pDesktop);

/* TTY event driver */

extern struct _EVENT_DRIVER TTYDRV_EVENT_Driver;

extern BOOL32 TTYDRV_EVENT_Init(void);
extern void TTYDRV_EVENT_AddIO(int fd, unsigned flag);
extern void TTYDRV_EVENT_DeleteIO(int fd, unsigned flag);
extern BOOL32 TTYDRV_EVENT_WaitNetEvent(BOOL32 sleep, BOOL32 peek);
extern void TTYDRV_EVENT_Synchronize(void);
extern BOOL32 TTYDRV_EVENT_CheckFocus(void);
extern BOOL32 TTYDRV_EVENT_QueryPointer(DWORD *posX, DWORD *posY, DWORD *state);
extern void TTYDRV_EVENT_DummyMotionNotify(void);
extern BOOL32 TTYDRV_EVENT_Pending(void);
extern BOOL16 TTYDRV_EVENT_IsUserIdle(void);
extern void TTYDRV_EVENT_WakeUp(void);

/* TTY keyboard driver */

extern struct _KEYBOARD_DRIVER TTYDRV_KEYBOARD_Driver;

extern void TTYDRV_KEYBOARD_Init(void);
extern WORD TTYDRV_KEYBOARD_VkKeyScan(CHAR cChar);
extern UINT16 TTYDRV_KEYBOARD_MapVirtualKey(UINT16 wCode, UINT16 wMapType);
extern INT16 TTYDRV_KEYBOARD_GetKeyNameText(LONG lParam, LPSTR lpBuffer, INT16 nSize);
extern INT16 TTYDRV_KEYBOARD_ToAscii(UINT16 virtKey, UINT16 scanCode, LPBYTE lpKeyState, LPVOID lpChar, UINT16 flags);

/* TTY main driver */

extern void TTYDRV_MAIN_Finalize(void);
extern void TTYDRV_MAIN_Initialize(void);
extern void TTYDRV_MAIN_ParseOptions(int *argc, char *argv[]);
extern void TTYDRV_MAIN_Create(void);
extern void TTYDRV_MAIN_SaveSetup(void);
extern void TTYDRV_MAIN_RestoreSetup(void);

/* TTY monitor driver */

extern struct tagMONITOR_DRIVER TTYDRV_MONITOR_Driver;

typedef struct _TTYDRV_MONITOR_DATA {
  int      width;
  int      height;
  int      depth;
} TTYDRV_MONITOR_DATA;

struct tagMONITOR;

extern void TTYDRV_MONITOR_Initialize(struct tagMONITOR *pMonitor);
extern void  TTYDRV_MONITOR_Finalize(struct tagMONITOR *pMonitor);
extern int TTYDRV_MONITOR_GetWidth(struct tagMONITOR *pMonitor);
extern int TTYDRV_MONITOR_GetHeight(struct tagMONITOR *pMonitor);
extern int TTYDRV_MONITOR_GetDepth(struct tagMONITOR *pMonitor);

/* TTY mouse driver */

extern struct _MOUSE_DRIVER TTYDRV_MOUSE_Driver;

extern void TTYDRV_MOUSE_SetCursor(CURSORICONINFO *lpCursor);
extern void TTYDRV_MOUSE_MoveCursor(WORD wAbsX, WORD wAbsY);

/* TTY windows driver */

extern struct _WND_DRIVER TTYDRV_WND_Driver;

extern void TTYDRV_WND_Initialize(struct tagWND *wndPtr);
extern void TTYDRV_WND_Finalize(struct tagWND *wndPtr);
extern BOOL32 TTYDRV_WND_CreateDesktopWindow(struct tagWND *wndPtr, struct tagCLASS *classPtr, BOOL32 bUnicode);
extern BOOL32 TTYDRV_WND_CreateWindow(struct tagWND *wndPtr, struct tagCLASS *classPtr, CREATESTRUCT32A *cs, BOOL32 bUnicode);
extern BOOL32 TTYDRV_WND_DestroyWindow(struct tagWND *pWnd);
extern struct tagWND *TTYDRV_WND_SetParent(struct tagWND *wndPtr, struct tagWND *pWndParent);
extern void TTYDRV_WND_ForceWindowRaise(struct tagWND *pWnd);
extern void TTYDRV_WND_SetWindowPos(struct tagWND *wndPtr, const WINDOWPOS32 *winpos, BOOL32 bSMC_SETXPOS);
extern void TTYDRV_WND_SetText(struct tagWND *wndPtr, LPCSTR text);
extern void TTYDRV_WND_SetFocus(struct tagWND *wndPtr);
extern void TTYDRV_WND_PreSizeMove(struct tagWND *wndPtr);
extern void TTYDRV_WND_PostSizeMove(struct tagWND *wndPtr);
extern void TTYDRV_WND_ScrollWindow(struct tagWND *wndPtr, struct tagDC *dcPtr, INT32 dx, INT32 dy, const RECT32 *clipRect, BOOL32 bUpdate);
extern void TTYDRV_WND_SetDrawable(struct tagWND *wndPtr, struct tagDC *dc, WORD flags, BOOL32 bSetClipOrigin);
extern BOOL32 TTYDRV_WND_IsSelfClipping(struct tagWND *wndPtr);

#endif /* !defined(__WINE_TTYDRV_H) */
