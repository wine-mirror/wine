/*
 * TTY driver definitions
 */

#ifndef __WINE_TTYDRV_H
#define __WINE_TTYDRV_H

#include "windef.h"
#include "wingdi.h"
#include "dinput.h"
#include "wine/winuser16.h"
#include "wine/wingdi16.h"

struct tagBITMAPOBJ;
struct tagCLASS;
struct tagDC;
struct tagDESKTOP;
struct tagPALETTEOBJ;
struct tagWND;
struct tagCURSORICONINFO;
struct tagCREATESTRUCTA;
struct tagWINDOWPOS;

/**************************************************************************
 * TTY GDI driver
 */

extern struct tagGDI_DRIVER TTYDRV_GDI_Driver;

extern BOOL TTYDRV_GDI_Initialize(void);
extern void TTYDRV_GDI_Finalize(void);

/* TTY GDI bitmap driver */

extern HBITMAP TTYDRV_BITMAP_CreateDIBSection(struct tagDC *dc, BITMAPINFO *bmi, UINT usage, LPVOID *bits, HANDLE section, DWORD offset);
extern HBITMAP16 TTYDRV_BITMAP_CreateDIBSection16(struct tagDC *dc, BITMAPINFO *bmi, UINT16 usage, SEGPTR *bits, HANDLE section, DWORD offset);

extern INT TTYDRV_BITMAP_SetDIBits(struct tagBITMAPOBJ *bmp, struct tagDC *dc, UINT startscan, UINT lines, LPCVOID bits, const BITMAPINFO *info, UINT coloruse, HBITMAP hbitmap);
extern INT TTYDRV_BITMAP_GetDIBits(struct tagBITMAPOBJ *bmp, struct tagDC *dc, UINT startscan, UINT lines, LPVOID bits, BITMAPINFO *info, UINT coloruse, HBITMAP hbitmap);
extern void TTYDRV_BITMAP_DeleteDIBSection(struct tagBITMAPOBJ *bmp);

typedef struct {
  int dummy;
} TTYDRV_PDEVICE;

extern BOOL TTYDRV_DC_CreateDC(struct tagDC *dc, LPCSTR driver, LPCSTR device, LPCSTR output, const DEVMODEA *initData);
extern BOOL TTYDRV_DC_DeleteDC(struct tagDC *dc);
extern INT TTYDRV_DC_Escape(struct tagDC *dc, INT nEscape, INT cbInput, SEGPTR lpInData, SEGPTR lpOutData);

/* TTY GDI palette driver */

extern struct tagPALETTE_DRIVER TTYDRV_PALETTE_Driver;

extern BOOL TTYDRV_PALETTE_Initialize(void);
extern void TTYDRV_PALETTE_Finalize(void);

extern int TTYDRV_PALETTE_SetMapping(struct tagPALETTEOBJ *palPtr, UINT uStart, UINT uNum, BOOL mapOnly);
extern int TTYDRV_PALETTE_UpdateMapping(struct tagPALETTEOBJ *palPtr);
extern BOOL TTYDRV_PALETTE_IsDark(int pixel);

/**************************************************************************
 * TTY USER driver
 */

extern struct tagUSER_DRIVER TTYDRV_USER_Driver;

extern BOOL TTYDRV_USER_Initialize(void);
extern void TTYDRV_USER_Finalize(void);
extern void TTYDRV_USER_BeginDebugging(void);
extern void TTYDRV_USER_EndDebugging(void);

/* TTY clipboard driver */

extern struct tagCLIPBOARD_DRIVER TTYDRV_CLIPBOARD_Driver;

extern void TTYDRV_CLIPBOARD_Empty(void);
extern void TTYDRV_CLIPBOARD_SetData(UINT wFormat);
extern BOOL TTYDRV_CLIPBOARD_GetData(UINT wFormat);
extern void TTYDRV_CLIPBOARD_ResetOwner(struct tagWND *pWnd, BOOL bFooBar);

/* TTY desktop driver */

extern struct tagDESKTOP_DRIVER TTYDRV_DESKTOP_Driver;

extern void TTYDRV_DESKTOP_Initialize(struct tagDESKTOP *pDesktop);
extern void TTYDRV_DESKTOP_Finalize(struct tagDESKTOP *pDesktop);
extern int TTYDRV_DESKTOP_GetScreenWidth(struct tagDESKTOP *pDesktop);
extern int TTYDRV_DESKTOP_GetScreenHeight(struct tagDESKTOP *pDesktop);
extern int TTYDRV_DESKTOP_GetScreenDepth(struct tagDESKTOP *pDesktop);

/* TTY event driver */

extern struct tagEVENT_DRIVER TTYDRV_EVENT_Driver;

extern BOOL TTYDRV_EVENT_Init(void);
extern void TTYDRV_EVENT_Synchronize(BOOL bProcessEvents);
extern BOOL TTYDRV_EVENT_CheckFocus(void);
extern BOOL TTYDRV_EVENT_QueryPointer(DWORD *posX, DWORD *posY, DWORD *state);
extern void TTYDRV_EVENT_UserRepaintDisable(BOOL bDisable);

/* TTY keyboard driver */

extern struct tagKEYBOARD_DRIVER TTYDRV_KEYBOARD_Driver;

extern void TTYDRV_KEYBOARD_Init(void);
extern WORD TTYDRV_KEYBOARD_VkKeyScan(CHAR cChar);
extern UINT16 TTYDRV_KEYBOARD_MapVirtualKey(UINT16 wCode, UINT16 wMapType);
extern INT16 TTYDRV_KEYBOARD_GetKeyNameText(LONG lParam, LPSTR lpBuffer, INT16 nSize);
extern INT16 TTYDRV_KEYBOARD_ToAscii(UINT16 virtKey, UINT16 scanCode, LPBYTE lpKeyState, LPVOID lpChar, UINT16 flags);
extern BOOL TTYDRV_KEYBOARD_GetBeepActive(void);
extern void TTYDRV_KEYBOARD_SetBeepActive(BOOL bActivate);
extern void TTYDRV_KEYBOARD_Beep(void);
extern BOOL TTYDRV_KEYBOARD_GetDIState(DWORD len, LPVOID ptr);
extern BOOL TTYDRV_KEYBOARD_GetDIData(BYTE *keystate, DWORD dodsize, LPDIDEVICEOBJECTDATA dod, LPDWORD entries, DWORD flags);

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
extern BOOL TTYDRV_MONITOR_IsSingleWindow(struct tagMONITOR *pMonitor);
extern int TTYDRV_MONITOR_GetWidth(struct tagMONITOR *pMonitor);
extern int TTYDRV_MONITOR_GetHeight(struct tagMONITOR *pMonitor);
extern int TTYDRV_MONITOR_GetDepth(struct tagMONITOR *pMonitor);
extern BOOL TTYDRV_MONITOR_GetScreenSaveActive(struct tagMONITOR *pMonitor);
extern void TTYDRV_MONITOR_SetScreenSaveActive(struct tagMONITOR *pMonitor, BOOL bActivate);
extern int TTYDRV_MONITOR_GetScreenSaveTimeout(struct tagMONITOR *pMonitor);
extern void TTYDRV_MONITOR_SetScreenSaveTimeout(struct tagMONITOR *pMonitor, int nTimeout);

/* TTY mouse driver */

extern struct tagMOUSE_DRIVER TTYDRV_MOUSE_Driver;

extern void TTYDRV_MOUSE_SetCursor(struct tagCURSORICONINFO *lpCursor);
extern void TTYDRV_MOUSE_MoveCursor(WORD wAbsX, WORD wAbsY);
extern BOOL TTYDRV_MOUSE_EnableWarpPointer(BOOL bEnable);

/* TTY windows driver */

extern struct tagWND_DRIVER TTYDRV_WND_Driver;

extern void TTYDRV_WND_Initialize(struct tagWND *wndPtr);
extern void TTYDRV_WND_Finalize(struct tagWND *wndPtr);
extern BOOL TTYDRV_WND_CreateDesktopWindow(struct tagWND *wndPtr, struct tagCLASS *classPtr, BOOL bUnicode);
extern BOOL TTYDRV_WND_CreateWindow(struct tagWND *wndPtr, struct tagCLASS *classPtr, struct tagCREATESTRUCTA *cs, BOOL bUnicode);
extern BOOL TTYDRV_WND_DestroyWindow(struct tagWND *pWnd);
extern struct tagWND *TTYDRV_WND_SetParent(struct tagWND *wndPtr, struct tagWND *pWndParent);
extern void TTYDRV_WND_ForceWindowRaise(struct tagWND *pWnd);
extern void TTYDRV_WND_SetWindowPos(struct tagWND *wndPtr, const struct tagWINDOWPOS *winpos, BOOL bSMC_SETXPOS);
extern void TTYDRV_WND_SetText(struct tagWND *wndPtr, LPCSTR text);
extern void TTYDRV_WND_SetFocus(struct tagWND *wndPtr);
extern void TTYDRV_WND_PreSizeMove(struct tagWND *wndPtr);
extern void TTYDRV_WND_PostSizeMove(struct tagWND *wndPtr);
extern void TTYDRV_WND_ScrollWindow(struct tagWND *wndPtr, struct tagDC *dcPtr, INT dx, INT dy, const RECT *clipRect, BOOL bUpdate);
extern void TTYDRV_WND_SetDrawable(struct tagWND *wndPtr, struct tagDC *dc, WORD flags, BOOL bSetClipOrigin);
extern BOOL TTYDRV_WND_SetHostAttr(struct tagWND *wndPtr, INT haKey, INT value);
extern BOOL TTYDRV_WND_IsSelfClipping(struct tagWND *wndPtr);

#endif /* !defined(__WINE_TTYDRV_H) */
