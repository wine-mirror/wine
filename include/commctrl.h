/*
 * Common controls definitions
 */

#ifndef __WINE_COMMCTRL_H
#define __WINE_COMMCTRL_H

#include "windows.h"

void WINAPI InitCommonControls(void);

/* StatusWindow */

#define STATUSCLASSNAME16  "msctls_statusbar"
#define STATUSCLASSNAME32A "msctls_statusbar32"
#define STATUSCLASSNAME32W "msctls_statusbar32"
#define STATUSCLASSNAME WINELIB_NAME_AW(STATUSCLASSNAME)

#define SB_SETTEXT32A         (WM_USER+1)
#define SB_SETTEXT32W         (WM_USER+11)
#define SB_SETTEXT WINELIB_NAME_AW(SB_SETTEXT)
#define SB_GETTEXT32A         (WM_USER+2)
#define SB_GETTEXT32W         (WM_USER+13)
#define SB_GETTEXT WINELIB_NAME_AW(SB_GETTEXT)
#define SB_GETTEXTLENGTH32A   (WM_USER+3)
#define SB_GETTEXTLENGTH32W   (WM_USER+12)
#define SB_GETTEXTLENGTH WINELIB_NAME_AW(SB_GETTEXTLENGTH)

#define SB_SETPARTS           (WM_USER+4)
#define SB_GETPARTS           (WM_USER+6)
#define SB_GETBORDERS         (WM_USER+7)
#define SB_SETMINHEIGHT       (WM_USER+8)
#define SB_SIMPLE             (WM_USER+9)
#define SB_GETRECT            (WM_USER+10)

#define SBT_NOBORDERS         0x0100
#define SBT_POPOUT            0x0200
#define SBT_RTLREADING        0x0400
#define SBT_OWNERDRAW         0x1000

#define CCS_BOTTOM            0x0003
#define SBARS_SIZEGRIP        0x0100

/* UpDown control */

#define UPDOWN_CLASS32A  "msctls_updown32"
#define UPDOWN_CLASS32W  "msctls_updown32"
#define UPDOWN_CLASS16   "msctls_updown"
#define UPDOWN_CLASS WINELIB_NAME_AW(UPDOWN_CLASS)

typedef struct tagUDACCEL
{
  UINT32 nSec;
  UINT32 nInc;
} UDACCEL;
 
typedef struct tagNM_UPDOWN
{
    NMHDR hdr;
    int iPos;
    int iDelta;
} NM_UPDOWN;

#define UD_MAXVAL          0x7fff
#define UD_MINVAL          0x8001

#define UDS_WRAP           0x0001
#define UDS_SETBUDDYINT    0x0002
#define UDS_ALIGNRIGHT     0x0004
#define UDS_ALIGNLEFT      0x0008
#define UDS_AUTOBUDDY      0x0010
#define UDS_ARROWKEYS      0x0020
#define UDS_HORZ           0x0040
#define UDS_NOTHOUSANDS    0x0080

#define UDN_FIRST          (0U-721)
#define UDN_DELTAPOS       (UDN_FIRST - 1)

#define UDM_SETRANGE       (WM_USER+101)
#define UDM_GETRANGE       (WM_USER+102)
#define UDM_SETPOS         (WM_USER+103)
#define UDM_GETPOS         (WM_USER+104)
#define UDM_SETBUDDY       (WM_USER+105)
#define UDM_GETBUDDY       (WM_USER+106)
#define UDM_SETACCEL       (WM_USER+107)
#define UDM_GETACCEL       (WM_USER+108)
#define UDM_SETBASE        (WM_USER+109)
#define UDM_GETBASE        (WM_USER+110)

/* Functions prototypes */

HWND32     WINAPI CreateStatusWindow32A(INT32,LPCSTR,HWND32,UINT32);
HWND32     WINAPI CreateStatusWindow32W(INT32,LPCWSTR,HWND32,UINT32);
#define    CreateStatusWindow WINELIB_NAME_AW(CreateStatusWindow)
HWND32     WINAPI CreateUpDownControl(DWORD,INT32,INT32,INT32,INT32,HWND32,
                                   INT32,HINSTANCE32,HWND32,INT32,INT32,INT32);
VOID       WINAPI DrawStatusText32A(HDC32,LPRECT32,LPCSTR,UINT32);
VOID       WINAPI DrawStatusText32W(HDC32,LPRECT32,LPCWSTR,UINT32);
#define    DrawStatusText WINELIB_NAME_AW(DrawStatusText)

#endif  /* __WINE_COMMCTRL_H */
