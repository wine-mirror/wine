/*
 * TTY driver definitions
 */

#ifndef __WINE_TTYDRV_H
#define __WINE_TTYDRV_H

#include "config.h"

#undef ERR
#ifdef HAVE_NCURSES_H
# include <ncurses.h>
#elif defined(HAVE_CURSES_H)
# include <curses.h>
#endif

#include "windef.h"
#include "wingdi.h"
#include "dinput.h"
#include "wine/winuser16.h"
#include "wine/wingdi16.h"

#include "keyboard.h"

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
#ifdef HAVE_LIBCURSES
  WINDOW *window;
#endif /* defined(HAVE_LIBCURSES) */
  int cellWidth;
  int cellHeight;
} TTYDRV_PDEVICE;

extern BOOL TTYDRV_DC_Arc(struct tagDC *dc, INT left, INT top, INT right, INT bottom, INT xstart, INT ystart, INT xend, INT yend);
extern BOOL TTYDRV_DC_CreateBitmap(HBITMAP hbitmap);
extern BOOL TTYDRV_DC_CreateDC(struct tagDC *dc, LPCSTR driver, LPCSTR device, LPCSTR output, const DEVMODEA *initData);
extern BOOL TTYDRV_DC_DeleteDC(struct tagDC *dc);
extern BOOL TTYDRV_DC_DeleteObject(HGDIOBJ handle);
extern BOOL TTYDRV_DC_BitBlt(struct tagDC *dcDst, INT xDst, INT yDst, INT width, INT height, struct tagDC *dcSrc, INT xSrc, INT ySrc, DWORD rop);
extern BOOL TTYDRV_DC_Chord(struct tagDC *dc, INT left, INT top, INT right, INT bottom, INT xstart, INT ystart, INT xend, INT yend);
extern BOOL TTYDRV_DC_Ellipse(struct tagDC *dc, INT left, INT top, INT right, INT bottom);
extern INT TTYDRV_DC_Escape(struct tagDC *dc, INT nEscape, INT cbInput, SEGPTR lpInData, SEGPTR lpOutData);
extern BOOL TTYDRV_DC_ExtFloodFill(struct tagDC *dc, INT x, INT y, COLORREF color, UINT fillType);
extern BOOL TTYDRV_DC_ExtTextOut(struct tagDC *dc, INT x, INT y, UINT flags, const RECT *lpRect, LPCSTR str, UINT count, const INT *lpDx);
extern BOOL TTYDRV_DC_GetCharWidth(struct tagDC *dc, UINT firstChar, UINT lastChar, LPINT buffer);
extern COLORREF TTYDRV_DC_GetPixel(struct tagDC *dc, INT x, INT y);

extern BOOL TTYDRV_DC_GetTextExtentPoint(struct tagDC *dc, LPCSTR str, INT count, LPSIZE size);
extern BOOL TTYDRV_DC_GetTextMetrics(struct tagDC *dc, TEXTMETRICA *metrics);
extern BOOL TTYDRV_DC_LineTo(struct tagDC *dc, INT x, INT y);
extern HANDLE TTYDRV_DC_LoadOEMResource(WORD resid, WORD type);
extern BOOL TTYDRV_DC_PaintRgn(struct tagDC *dc, HRGN hrgn);
extern BOOL TTYDRV_DC_PatBlt(struct tagDC *dc, INT left, INT top, INT width, INT height, DWORD rop);
extern BOOL TTYDRV_DC_Pie(struct tagDC *dc, INT left, INT top, INT right, INT bottom, INT xstart, INT ystart, INT xend, INT yend);
extern BOOL TTYDRV_DC_PolyBezier(struct tagDC *dc, const POINT* BezierPoints, DWORD count);
extern BOOL TTYDRV_DC_Polygon(struct tagDC *dc, const POINT* pt, INT count);
extern BOOL TTYDRV_DC_Polyline(struct tagDC *dc, const POINT* pt, INT count);
extern BOOL TTYDRV_DC_PolyPolygon(struct tagDC *dc, const POINT* pt, const INT* counts, UINT polygons);
extern BOOL TTYDRV_DC_PolyPolyline(struct tagDC *dc, const POINT* pt, const DWORD* counts, DWORD polylines);
extern BOOL TTYDRV_DC_Rectangle(struct tagDC *dc, INT left, INT top, INT right, INT bottom);
extern BOOL TTYDRV_DC_RoundRect(struct tagDC *dc, INT left, INT top, INT right, INT bottom, INT ell_width, INT ell_height);
extern void TTYDRV_DC_SetDeviceClipping(struct tagDC *dc);
extern HGDIOBJ TTYDRV_DC_SelectObject(struct tagDC *dc, HGDIOBJ handle);
extern COLORREF TTYDRV_DC_SetBkColor(struct tagDC *dc, COLORREF color);
extern COLORREF TTYDRV_DC_SetPixel(struct tagDC *dc, INT x, INT y, COLORREF color);
extern COLORREF TTYDRV_DC_SetTextColor(struct tagDC *dc, COLORREF color);
extern BOOL TTYDRV_DC_StretchBlt(struct tagDC *dcDst, INT xDst, INT yDst, INT widthDst, INT heightDst, struct tagDC *dcSrc, INT xSrc, INT ySrc, INT widthSrc, INT heightSrc, DWORD rop);

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

extern void TTYDRV_CLIPBOARD_Acquire(void);
extern void TTYDRV_CLIPBOARD_Release(void);
extern void TTYDRV_CLIPBOARD_SetData(UINT wFormat);
extern BOOL TTYDRV_CLIPBOARD_GetData(UINT wFormat);
extern BOOL TTYDRV_CLIPBOARD_IsFormatAvailable(UINT wFormat);
extern BOOL TTYDRV_CLIPBOARD_RegisterFormat( LPCSTR FormatName );
extern BOOL TTYDRV_CLIPBOARD_IsSelectionowner();
extern void TTYDRV_CLIPBOARD_ResetOwner(struct tagWND *pWnd, BOOL bFooBar);

/* TTY desktop driver */

extern struct tagDESKTOP_DRIVER TTYDRV_DESKTOP_Driver;

#ifdef HAVE_LIBCURSES
extern WINDOW *TTYDRV_DESKTOP_GetCursesRootWindow(struct tagDESKTOP *pDesktop);
#endif /* defined(HAVE_LIBCURSES) */

extern void TTYDRV_DESKTOP_Initialize(struct tagDESKTOP *pDesktop);
extern void TTYDRV_DESKTOP_Finalize(struct tagDESKTOP *pDesktop);
extern int TTYDRV_DESKTOP_GetScreenWidth(struct tagDESKTOP *pDesktop);
extern int TTYDRV_DESKTOP_GetScreenHeight(struct tagDESKTOP *pDesktop);
extern int TTYDRV_DESKTOP_GetScreenDepth(struct tagDESKTOP *pDesktop);

/* TTY event driver */

extern struct tagEVENT_DRIVER TTYDRV_EVENT_Driver;

extern BOOL TTYDRV_EVENT_Init(void);
extern void TTYDRV_EVENT_Synchronize(void);
extern BOOL TTYDRV_EVENT_CheckFocus(void);
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
extern void TTYDRV_KEYBOARD_GetKeyboardConfig(KEYBOARD_CONFIG *cfg);
extern void TTYDRV_KEYBOARD_SetKeyboardConfig(KEYBOARD_CONFIG *cfg, DWORD mask);

/* TTY monitor driver */

extern struct tagMONITOR_DRIVER TTYDRV_MONITOR_Driver;

typedef struct tagTTYDRV_MONITOR_DATA {
#ifdef HAVE_LIBCURSES
  WINDOW  *rootWindow;
#endif /* defined(HAVE_LIBCURSES) */
  int      cellWidth;
  int      cellHeight;
  int      width;
  int      height;
  int      depth;
} TTYDRV_MONITOR_DATA;

struct tagMONITOR;

#ifdef HAVE_LIBCURSES
extern WINDOW *TTYDRV_MONITOR_GetCursesRootWindow(struct tagMONITOR *pMonitor);
#endif /* defined(HAVE_LIBCURSES) */

extern INT TTYDRV_MONITOR_GetCellWidth(struct tagMONITOR *pMonitor);
extern INT TTYDRV_MONITOR_GetCellHeight(struct tagMONITOR *pMonitor);

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

extern void TTYDRV_MOUSE_Init();
extern void TTYDRV_MOUSE_SetCursor(struct tagCURSORICONINFO *lpCursor);
extern void TTYDRV_MOUSE_MoveCursor(WORD wAbsX, WORD wAbsY);
extern BOOL TTYDRV_MOUSE_EnableWarpPointer(BOOL bEnable);

/* TTY windows driver */

extern struct tagWND_DRIVER TTYDRV_WND_Driver;

typedef struct tagTTYDRV_WND_DATA {
#ifdef HAVE_LIBCURSES
  WINDOW *window;
#else /* defined(HAVE_LIBCURSES) */
  int dummy; /* FIXME: Remove later */
#endif /* defined(HAVE_LIBCURSES) */
} TTYDRV_WND_DATA;

#ifdef HAVE_LIBCURSES
WINDOW *TTYDRV_WND_GetCursesWindow(struct tagWND *wndPtr);
WINDOW *TTYDRV_WND_GetCursesRootWindow(struct tagWND *wndPtr);
#endif /* defined(HAVE_LIBCURSES) */

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
