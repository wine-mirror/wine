/*
 * Common controls definitions
 */

#ifndef __WINE_COMMCTRL_H
#define __WINE_COMMCTRL_H

#include "windows.h"

void WINAPI InitCommonControls(void);

/* StatusWindow */

#define STATUSCLASSNAME16     "msctls_statusbar"
#define STATUSCLASSNAME32A    "msctls_statusbar32"
#define STATUSCLASSNAME32W   L"msctls_statusbar32"       /*FIXME*/
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

/* UpDown */

#define UPDOWN_CLASS16        "msctls_updown"
#define UPDOWN_CLASS32A       "msctls_updown32"
#define UPDOWN_CLASS32W      L"msctls_updown32"   /*FIXME*/
#define UPDOWN_CLASS          WINELIB_NAME_AW(UPDOWN_CLASS)

typedef struct tagUDACCEL
{
  UINT32 nSec;
  UINT32 nInc;
} UDACCEL;

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
#define UDN_DELTAPOS       (UDN_FIRST-1)

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

/* Progress Bar */

#define PROGRESS_CLASS32A   "msctls_progress32"
#define PROGRESS_CLASS32W  L"msctls_progress32"
#define PROGRESS_CLASS16    "msctls_progress"

#define PROGRESS_CLASS      WINELIB_NAME_AW(PROGRESS_CLASS)

#define PBM_SETRANGE        (WM_USER+1)
#define PBM_SETPOS          (WM_USER+2)
#define PBM_DELTAPOS        (WM_USER+3)
#define PBM_SETSTEP         (WM_USER+4)
#define PBM_STEPIT          (WM_USER+5)
 
/* Functions prototypes */

HWND16     WINAPI CreateStatusWindow16(INT16,LPCSTR,HWND16,UINT16);
HWND32     WINAPI CreateStatusWindow32A(INT32,LPCSTR,HWND32,UINT32);
HWND32     WINAPI CreateStatusWindow32W(INT32,LPCWSTR,HWND32,UINT32);
#define    CreateStatusWindow WINELIB_NAME_AW(CreateStatusWindow)
HWND32     WINAPI CreateUpDownControl(DWORD,INT32,INT32,INT32,INT32,
                                      HWND32,INT32,HINSTANCE32,HWND32,
                                      INT32,INT32,INT32);
VOID       WINAPI DrawStatusText16(HDC16,LPRECT16,LPCSTR,UINT16);
VOID       WINAPI DrawStatusText32A(HDC32,LPRECT32,LPCSTR,UINT32);
VOID       WINAPI DrawStatusText32W(HDC32,LPRECT32,LPCWSTR,UINT32);
#define    DrawStatusText WINELIB_NAME_AW(DrawStatusText)


/* ImageList */

#if defined(__WINE__) && defined(__WINE_IMAGELIST_C)
#else
struct _IMAGELIST;
typedef struct _IMAGELIST *HIMAGELIST;
#endif  /* __WINE__ */

#define CLR_NONE      0xFFFFFFFF
#define CLR_DEFAULT   0x00000000
#define CLR_HILIGHT   CLR_DEFAULT

#define ILC_MASK       0x0001
#define ILC_COLOR      0x0000
#define ILC_COLORDDB   0x00FE
#define ILC_COLOR4     0x0004
#define ILC_COLOR8     0x0008
#define ILC_COLOR16    0x0010
#define ILC_COLOR24    0x0018
#define ILC_COLOR32    0x0020
#define ILC_PALETTE    0x0800

#define ILD_NORMAL       0x0000
#define ILD_TRANSPARENT  0x0001
#define ILD_BLEND25      0x0002
#define ILD_BLEND50      0x0004
#define ILD_MASK         0x0010
#define ILD_IMAGE        0x0020
#define ILD_ROP          0x0040
#define ILD_OVERLAYMASK  0x0F00

#define ILD_SELECTED     ILD_BLEND50
#define ILD_FOCUS        ILD_BLEND25
#define ILD_BLEND        ILD_BLEND50

#define INDEXTOOVERLAYMASK(i)  ((i)<<8)

typedef struct _IMAGEINFO
{
    HBITMAP32 hbmImage;
    HBITMAP32 hbmMask;
    INT32     Unused1;
    INT32     Unused2;
    RECT32    rcImage;
} IMAGEINFO;


INT32      WINAPI ImageList_Add(HIMAGELIST,HBITMAP32,HBITMAP32);
INT32      WINAPI ImageList_AddMasked(HIMAGELIST,HBITMAP32,COLORREF);

HIMAGELIST WINAPI ImageList_Create(INT32,INT32,UINT32,INT32,INT32);
BOOL32     WINAPI ImageList_Destroy(HIMAGELIST);

BOOL32     WINAPI ImageList_Draw(HIMAGELIST,INT32,HDC32,INT32,INT32,UINT32);

COLORREF   WINAPI ImageList_GetBkColor(HIMAGELIST);

BOOL32     WINAPI ImageList_GetIconSize(HIMAGELIST,INT32*,INT32*);
INT32      WINAPI ImageList_GetImageCount(HIMAGELIST);
BOOL32     WINAPI ImageList_GetImageInfo(HIMAGELIST,INT32,IMAGEINFO*);

HIMAGELIST WINAPI ImageList_LoadImage32A(HINSTANCE32,LPCSTR,INT32,INT32,
                                         COLORREF,UINT32,UINT32);
HIMAGELIST WINAPI ImageList_LoadImage32W(HINSTANCE32,LPCWSTR,INT32,INT32,
                                         COLORREF,UINT32,UINT32);
#define    ImageList_LoadImage WINELIB_NAME_AW(ImageList_LoadImage)
HIMAGELIST WINAPI ImageList_Merge(HIMAGELIST,INT32,HIMAGELIST,INT32,INT32,INT32);

BOOL32     WINAPI ImageList_Replace(HIMAGELIST,INT32,HBITMAP32,HBITMAP32);
INT32      WINAPI ImageList_ReplaceIcon(HIMAGELIST,INT32,HICON32);

COLORREF   WINAPI ImageList_SetBkColor(HIMAGELIST,COLORREF);

BOOL32     WINAPI ImageList_SetOverlayImage(HIMAGELIST,INT32,INT32);

#define ImageList_AddIcon(himl,hicon) ImageList_ReplaceIcon(himl,-1,hicon)
#define ImageList_LoadBitmap(hi,lpbmp,cx,cGrow,crMask) \
  ImageList_LoadImage(hi,lpbmp,cx,cGrow,crMask,IMAGE_BITMAP,0)

#endif  /* __WINE_COMMCTRL_H */
