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
struct tagKEYBOARD_CONFIG;
struct DIDEVICEOBJECTDATA;

#if defined(HAVE_LIBCURSES) || defined(HAVE_LIBNCURSES)
#define WINE_CURSES
#endif

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

#ifndef WINE_CURSES
typedef struct { int dummy; } WINDOW;
#endif

typedef struct {
  WINDOW *window;
  int cellWidth;
  int cellHeight;
} TTYDRV_PDEVICE;

typedef struct {
  int dummy; /* FIXME: Remove later */
} TTYDRV_PHYSBITMAP;

extern BOOL TTYDRV_DC_CreateBitmap(HBITMAP hbitmap);

extern BOOL TTYDRV_DC_Arc(struct tagDC *dc, INT left, INT top, INT right, INT bottom, INT xstart, INT ystart, INT xend, INT yend);
extern LONG TTYDRV_DC_BitmapBits(HBITMAP hbitmap, void *bits, LONG count, WORD flags);
extern BOOL TTYDRV_DC_CreateBitmap(HBITMAP hbitmap);
extern BOOL TTYDRV_DC_CreateDC(struct tagDC *dc, LPCSTR driver, LPCSTR device, LPCSTR output, const DEVMODEA *initData);
extern BOOL TTYDRV_DC_DeleteDC(struct tagDC *dc);
extern BOOL TTYDRV_DC_DeleteObject(HGDIOBJ handle);
extern BOOL TTYDRV_DC_BitBlt(struct tagDC *dcDst, INT xDst, INT yDst, INT width, INT height, struct tagDC *dcSrc, INT xSrc, INT ySrc, DWORD rop);
extern BOOL TTYDRV_DC_Chord(struct tagDC *dc, INT left, INT top, INT right, INT bottom, INT xstart, INT ystart, INT xend, INT yend);
extern BOOL TTYDRV_DC_Ellipse(struct tagDC *dc, INT left, INT top, INT right, INT bottom);
extern INT TTYDRV_DC_Escape(struct tagDC *dc, INT nEscape, INT cbInput, SEGPTR lpInData, SEGPTR lpOutData);
extern BOOL TTYDRV_DC_ExtFloodFill(struct tagDC *dc, INT x, INT y, COLORREF color, UINT fillType);
extern BOOL TTYDRV_DC_ExtTextOut(struct tagDC *dc, INT x, INT y, UINT flags, const RECT *lpRect, LPCWSTR str, UINT count, const INT *lpDx);
extern BOOL TTYDRV_DC_GetCharWidth(struct tagDC *dc, UINT firstChar, UINT lastChar, LPINT buffer);
extern COLORREF TTYDRV_DC_GetPixel(struct tagDC *dc, INT x, INT y);

extern BOOL TTYDRV_DC_GetTextExtentPoint(struct tagDC *dc, LPCWSTR str, INT count, LPSIZE size);
extern BOOL TTYDRV_DC_GetTextMetrics(struct tagDC *dc, TEXTMETRICA *metrics);
extern BOOL TTYDRV_DC_LineTo(struct tagDC *dc, INT x, INT y);
extern HANDLE TTYDRV_DC_LoadOEMResource(WORD resid, WORD type);
extern BOOL TTYDRV_DC_PaintRgn(struct tagDC *dc, HRGN hrgn);
extern BOOL TTYDRV_DC_PatBlt(struct tagDC *dc, INT left, INT top, INT width, INT height, DWORD rop);
extern BOOL TTYDRV_DC_Pie(struct tagDC *dc, INT left, INT top, INT right, INT bottom, INT xstart, INT ystart, INT xend, INT yend);
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
INT TTYDRV_DC_SetDIBitsToDevice(struct tagDC *dc, INT xDest, INT yDest, DWORD cx, DWORD cy, INT xSrc, INT ySrc, UINT startscan, UINT lines, LPCVOID bits, const BITMAPINFO *info, UINT coloruse);

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

extern int cell_width;
extern int cell_height;
static inline int TTYDRV_GetCellWidth(void)  { return cell_width; }
static inline int TTYDRV_GetCellHeight(void) { return cell_height; }

extern WINDOW *root_window;
static inline WINDOW *TTYDRV_GetRootWindow(void) { return root_window; }

extern BOOL TTYDRV_GetScreenSaveActive(void);
extern void TTYDRV_SetScreenSaveActive(BOOL bActivate);
extern int TTYDRV_GetScreenSaveTimeout(void);
extern void TTYDRV_SetScreenSaveTimeout(int nTimeout);
extern BOOL TTYDRV_IsSingleWindow(void);

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

/* TTY event driver */

extern void TTYDRV_EVENT_Synchronize(void);
extern BOOL TTYDRV_EVENT_CheckFocus(void);
extern void TTYDRV_EVENT_UserRepaintDisable(BOOL bDisable);

/* TTY keyboard driver */

extern void TTYDRV_KEYBOARD_Init(void);
extern WORD TTYDRV_KEYBOARD_VkKeyScan(CHAR cChar);
extern UINT16 TTYDRV_KEYBOARD_MapVirtualKey(UINT16 wCode, UINT16 wMapType);
extern INT16 TTYDRV_KEYBOARD_GetKeyNameText(LONG lParam, LPSTR lpBuffer, INT16 nSize);
extern INT16 TTYDRV_KEYBOARD_ToAscii(UINT16 virtKey, UINT16 scanCode, LPBYTE lpKeyState, LPVOID lpChar, UINT16 flags);
extern BOOL TTYDRV_KEYBOARD_GetBeepActive(void);
extern void TTYDRV_KEYBOARD_SetBeepActive(BOOL bActivate);
extern void TTYDRV_KEYBOARD_Beep(void);
extern BOOL TTYDRV_KEYBOARD_GetDIState(DWORD len, LPVOID ptr);
extern BOOL TTYDRV_KEYBOARD_GetDIData(BYTE *keystate, DWORD dodsize, struct DIDEVICEOBJECTDATA *dod, LPDWORD entries, DWORD flags);
extern void TTYDRV_KEYBOARD_GetKeyboardConfig(struct tagKEYBOARD_CONFIG *cfg);
extern void TTYDRV_KEYBOARD_SetKeyboardConfig(struct tagKEYBOARD_CONFIG *cfg, DWORD mask);

/* TTY mouse driver */

extern void TTYDRV_MOUSE_Init();
extern void TTYDRV_MOUSE_SetCursor(struct tagCURSORICONINFO *lpCursor);
extern void TTYDRV_MOUSE_MoveCursor(WORD wAbsX, WORD wAbsY);
extern LONG TTYDRV_MOUSE_EnableWarpPointer(BOOL bEnable);

/* TTY windows driver */

extern struct tagWND_DRIVER TTYDRV_WND_Driver;

typedef struct tagTTYDRV_WND_DATA {
  WINDOW *window;
} TTYDRV_WND_DATA;

WINDOW *TTYDRV_WND_GetCursesWindow(struct tagWND *wndPtr);

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
