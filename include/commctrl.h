/*
 * Common controls definitions
 */

#ifndef __WINE_COMMCTRL_H
#define __WINE_COMMCTRL_H

#include "windows.h"

void WINAPI InitCommonControls(void);

typedef struct tagINITCOMMONCONTROLSEX {
    DWORD dwSize;
    DWORD dwICC;
} INITCOMMONCONTROLSEX, *LPINITCOMMONCONTROLSEX;

BOOL32 WINAPI InitCommonControlsEx(LPINITCOMMONCONTROLSEX);

#define ICC_LISTVIEW_CLASSES   0x00000001  /* listview, header */
#define ICC_TREEVIEW_CLASSES   0x00000002  /* treeview, tooltips */
#define ICC_BAR_CLASSES        0x00000004  /* toolbar, statusbar, trackbar, tooltips */
#define ICC_TAB_CLASSES        0x00000008  /* tab, tooltips */
#define ICC_UPDOWN_CLASS       0x00000010  /* updown */
#define ICC_PROGRESS_CLASS     0x00000020  /* progress */
#define ICC_HOTKEY_CLASS       0x00000040  /* hotkey */
#define ICC_ANIMATE_CLASS      0x00000080  /* animate */
#define ICC_WIN95_CLASSES      0x000000FF
#define ICC_DATE_CLASSES       0x00000100  /* month picker, date picker, time picker, updown */
#define ICC_USEREX_CLASSES     0x00000200  /* comboex */
#define ICC_COOL_CLASSES       0x00000400  /* rebar (coolbar) */
#define ICC_INTERNET_CLASSES   0x00000800  /* IP address, ... */
#define ICC_PAGESCROLLER_CLASS 0x00001000  /* page scroller */
#define ICC_NATIVEFNTCTL_CLASS 0x00002000  /* native font control ???*/


/* common control styles */

#define CCS_TOP             0x00000001L
#define CCS_NOMOVEY         0x00000002L
#define CCS_BOTTOM          0x00000003L
#define CCS_NORESIZE        0x00000004L
#define CCS_NOPARENTALIGN   0x00000008L
#define CCS_ADJUSTABLE      0x00000020L
#define CCS_NODIVIDER       0x00000040L
#define CCS_VERT            0x00000080L
#define CCS_LEFT            (CCS_VERT|CCS_TOP)
#define CCS_RIGHT           (CCS_VERT|CCS_BOTTOM)
#define CCS_NOMOVEX         (CCS_VERT|CCS_NOMOVEY)


/* common control shared messages */
#define CCM_FIRST           0x2000

#define CCM_SETBKCOLOR      (CCM_FIRST+1)     /* lParam = bkColor */


/* common notification codes */
#define NM_FIRST             (0U-0U)
#define NM_LAST              (0U-99U)

#define NM_OUTOFMEMORY       (NM_FIRST-1)
#define NM_CLICK             (NM_FIRST-2)
#define NM_DBLCLK            (NM_FIRST-3)
#define NM_RETURN            (NM_FIRST-4)
#define NM_RCLICK            (NM_FIRST-5)
#define NM_RDBLCLK           (NM_FIRST-6)
#define NM_SETFOCUS          (NM_FIRST-7)
#define NM_KILLFOCUS         (NM_FIRST-8)
#define NM_CUSTOMDRAW        (NM_FIRST-12)
#define NM_HOVER             (NM_FIRST-13)
#define NM_NCHITTEST         (NM_FIRST-14)
#define NM_KEYDOWN           (NM_FIRST-15)
#define NM_RELEASEDCAPTURE   (NM_FIRST-16)
#define NM_SETCURSOR         (NM_FIRST-17)
#define NM_CHAR              (NM_FIRST-18)
#define NM_TOOLTIPSCREATED   (NM_FIRST-19)


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
#define SB_ISSIMPLE           (WM_USER+14)
#define SB_SETICON            (WM_USER+15)
#define SB_GETICON            (WM_USER+20)
#define SB_SETBKCOLOR         CCM_SETBKCOLOR   /* lParam = bkColor */

#define SBT_NOBORDERS         0x0100
#define SBT_POPOUT            0x0200
#define SBT_RTLREADING        0x0400  /* not supported */
#define SBT_OWNERDRAW         0x1000

#define SBARS_SIZEGRIP        0x0100

#define SBN_FIRST             (0U-880U)
#define SBN_LAST              (0U-899U)
#define SBN_SIMPLEMODECHANGE  (SBN_FIRST-0)

void WINAPI MenuHelp (UINT32, WPARAM32, LPARAM, HMENU32,
                      HINSTANCE32, HWND32, LPUINT32);


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
#define UDM_SETRANGE32     (WM_USER+111)
#define UDM_GETRANGE32     (WM_USER+112)


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
#define PBM_SETRANGE32      (WM_USER+6)
#define PBM_GETRANGE        (WM_USER+7)
#define PBM_GETPOS          (WM_USER+8)
#define PBM_SETBARCOLOR     (WM_USER+9)
#define PBM_SETBKCOLOR      CCM_SETBKCOLOR

#define PBS_SMOOTH          0x01
#define PBS_VERTICAL        0x04

typedef struct
{
  INT32 iLow;
  INT32 iHigh;
} PBRANGE, *PPBRANGE;

 
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

#define CLR_NONE         0xFFFFFFFF
#define CLR_DEFAULT      0xFF000000
#define CLR_HILIGHT      CLR_DEFAULT

#define ILC_MASK         0x0001
#define ILC_COLOR        0x0000
#define ILC_COLORDDB     0x00FE
#define ILC_COLOR4       0x0004
#define ILC_COLOR8       0x0008
#define ILC_COLOR16      0x0010
#define ILC_COLOR24      0x0018
#define ILC_COLOR32      0x0020
#define ILC_PALETTE      0x0800  /* no longer supported by M$ */

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

#define ILCF_MOVE        (0x00000000)
#define ILCF_SWAP        (0x00000001)


typedef struct _IMAGEINFO
{
    HBITMAP32 hbmImage;
    HBITMAP32 hbmMask;
    INT32     Unused1;
    INT32     Unused2;
    RECT32    rcImage;
} IMAGEINFO;


typedef struct _IMAGELISTDRAWPARAMS
{
    DWORD       cbSize;
    HIMAGELIST  himl;
    INT32       i;
    HDC32       hdcDst;
    INT32       x;
    INT32       y;
    INT32       cx;
    INT32       cy;
    INT32       xBitmap;  /* x offest from the upperleft of bitmap */
    INT32       yBitmap;  /* y offset from the upperleft of bitmap */
    COLORREF    rgbBk;
    COLORREF    rgbFg;
    UINT32      fStyle;
    DWORD       dwRop;
} IMAGELISTDRAWPARAMS, *LPIMAGELISTDRAWPARAMS;

 
INT32      WINAPI ImageList_Add(HIMAGELIST,HBITMAP32,HBITMAP32);
INT32      WINAPI ImageList_AddMasked(HIMAGELIST,HBITMAP32,COLORREF);
BOOL32     WINAPI ImageList_BeginDrag(HIMAGELIST,INT32,INT32,INT32);
BOOL32     WINAPI ImageList_Copy(HIMAGELIST,INT32,HIMAGELIST,INT32,INT32);
HIMAGELIST WINAPI ImageList_Create(INT32,INT32,UINT32,INT32,INT32);
BOOL32     WINAPI ImageList_Destroy(HIMAGELIST);
BOOL32     WINAPI ImageList_DragEnter(HWND32,INT32,INT32);
BOOL32     WINAPI ImageList_DragLeave(HWND32); 
BOOL32     WINAPI ImageList_DragMove(INT32,INT32);
BOOL32     WINAPI ImageList_DragShowNolock (BOOL32 bShow);
BOOL32     WINAPI ImageList_Draw(HIMAGELIST,INT32,HDC32,INT32,INT32,UINT32);
BOOL32     WINAPI ImageList_DrawEx(HIMAGELIST,INT32,HDC32,INT32,INT32,INT32,
                                   INT32,COLORREF,COLORREF,UINT32);
BOOL32     WINAPI ImageList_DrawIndirect(IMAGELISTDRAWPARAMS*); 
HIMAGELIST WINAPI ImageList_Duplicate(HIMAGELIST);
BOOL32     WINAPI ImageList_EndDrag (VOID);
COLORREF   WINAPI ImageList_GetBkColor(HIMAGELIST);
HIMAGELIST WINAPI ImageList_GetDragImage(POINT32*,POINT32*);
HICON32    WINAPI ImageList_GetIcon(HIMAGELIST,INT32,UINT32);
BOOL32     WINAPI ImageList_GetIconSize(HIMAGELIST,INT32*,INT32*);
INT32      WINAPI ImageList_GetImageCount(HIMAGELIST);
BOOL32     WINAPI ImageList_GetImageInfo(HIMAGELIST,INT32,IMAGEINFO*);
BOOL32     WINAPI ImageList_GetImageRect (HIMAGELIST,INT32,LPRECT32);
HIMAGELIST WINAPI ImageList_LoadImage32A(HINSTANCE32,LPCSTR,INT32,INT32,
                                         COLORREF,UINT32,UINT32);
HIMAGELIST WINAPI ImageList_LoadImage32W(HINSTANCE32,LPCWSTR,INT32,INT32,
                                         COLORREF,UINT32,UINT32);
#define    ImageList_LoadImage WINELIB_NAME_AW(ImageList_LoadImage)
HIMAGELIST WINAPI ImageList_Merge(HIMAGELIST,INT32,HIMAGELIST,INT32,INT32,INT32);

BOOL32     WINAPI ImageList_Remove(HIMAGELIST,INT32);
BOOL32     WINAPI ImageList_Replace(HIMAGELIST,INT32,HBITMAP32,HBITMAP32);
INT32      WINAPI ImageList_ReplaceIcon(HIMAGELIST,INT32,HICON32);

COLORREF   WINAPI ImageList_SetBkColor(HIMAGELIST,COLORREF);
BOOL32     WINAPI ImageList_SetDragCursorImage(HIMAGELIST,INT32,INT32,INT32);
BOOL32     WINAPI ImageList_SetIconSize (HIMAGELIST,INT32,INT32);
BOOL32     WINAPI ImageList_SetImageCount (HIMAGELIST,INT32);
BOOL32     WINAPI ImageList_SetOverlayImage(HIMAGELIST,INT32,INT32);

#define ImageList_AddIcon(himl,hicon) ImageList_ReplaceIcon(himl,-1,hicon)
#define ImageList_ExtractIcon(hi,himl,i) ImageList_GetIcon(himl,i,0)
#define ImageList_LoadBitmap(hi,lpbmp,cx,cGrow,crMask) \
  ImageList_LoadImage(hi,lpbmp,cx,cGrow,crMask,IMAGE_BITMAP,0)
#define ImageList_RemoveAll(himl) ImageList_Remove(himl,-1)


/* Header control */

#define WC_HEADER16    "SysHeader" 
#define WC_HEADER32A   "SysHeader32" 
#define WC_HEADER32W  L"SysHeader32" 

#define WC_HEADER     WINELIB_NAME_AW(WC_HEADER)
 
#define HDS_HORZ                0x0000 
#define HDS_BUTTONS             0x0002 
#define HDS_HOTTRACK            0x0004 
#define HDS_HIDDEN              0x0008 
#define HDS_DRAGDROP            0x0040 
#define HDS_FULLDRAG            0x0080 

#define HDI_WIDTH               0x0001
#define HDI_HEIGHT              HDI_WIDTH
#define HDI_TEXT                0x0002
#define HDI_FORMAT              0x0004
#define HDI_LPARAM              0x0008
#define HDI_BITMAP              0x0010
#define HDI_IMAGE               0x0020
#define HDI_DI_SETITEM          0x0040
#define HDI_ORDER               0x0080

#define HDF_LEFT                0x0000
#define HDF_RIGHT               0x0001
#define HDF_CENTER              0x0002
#define HDF_JUSTIFYMASK         0x0003

#define HDF_IMAGE               0x0800
#define HDF_BITMAP_ON_RIGHT     0x1000
#define HDF_BITMAP              0x2000
#define HDF_STRING              0x4000
#define HDF_OWNERDRAW           0x8000

#define HHT_NOWHERE             0x0001
#define HHT_ONHEADER            0x0002
#define HHT_ONDIVIDER           0x0004
#define HHT_ONDIVOPEN           0x0008
#define HHT_ABOVE               0x0100
#define HHT_BELOW               0x0200
#define HHT_TORIGHT             0x0400
#define HHT_TOLEFT              0x0800

#define HDM_FIRST               0x1200
#define HDM_GETITEMCOUNT        (HDM_FIRST+0)
#define HDM_INSERTITEM32A       (HDM_FIRST+1)
#define HDM_INSERTITEM32W       (HDM_FIRST+10)
#define HDM_INSERTITEM WINELIB_NAME_AW(HDM_INSERTITEM)
#define HDM_DELETEITEM          (HDM_FIRST+2)
#define HDM_GETITEM32A          (HDM_FIRST+3)
#define HDM_GETITEM32W          (HDM_FIRST+11)
#define HDM_GETITEM WINELIB_NAME_AW(HDM_GETITEM)
#define HDM_SETITEM32A          (HDM_FIRST+4)
#define HDM_SETITEM32W          (HDM_FIRST+12)
#define HDM_SETITEM WINELIB_NAME_AW(HDM_SETITEM)
#define HDM_LAYOUT              (HDM_FIRST+5)
#define HDM_HITTEST             (HDM_FIRST+6)
#define HDM_GETITEMRECT         (HDM_FIRST+7)
#define HDM_SETIMAGELIST        (HDM_FIRST+8)
#define HDM_GETIMAGELIST        (HDM_FIRST+9)


#define HDM_ORDERTOINDEX        (HDM_FIRST+15)
#define HDM_CREATEDRAGIMAGE     (HDM_FIRST+16)
#define HDM_GETORDERARRAY       (HDM_FIRST+17)
#define HDM_SETORDERARRAY       (HDM_FIRST+18)
#define HDM_SETHOTDIVIDER       (HDM_FIRST+19)

#define HDN_FIRST               (0U-300U)
#define HDN_LAST                (0U-399U)
#define HDN_ITEMCHANGING32A     (HDN_FIRST-0)
#define HDN_ITEMCHANGING32W     (HDN_FIRST-20)
#define HDN_ITEMCHANGED32A      (HDN_FIRST-1)
#define HDN_ITEMCHANGED32W      (HDN_FIRST-21)
#define HDN_ITEMCLICK32A        (HDN_FIRST-2)
#define HDN_ITEMCLICK32W        (HDN_FIRST-22)
#define HDN_ITEMDBLCLICK32A     (HDN_FIRST-3)
#define HDN_ITEMDBLCLICK32W     (HDN_FIRST-23)
#define HDN_DIVIDERDBLCLICK32A  (HDN_FIRST-5)
#define HDN_DIVIDERDBLCLICK32W  (HDN_FIRST-25)
#define HDN_BEGINTRACK32A       (HDN_FIRST-6)
#define HDN_BEGINTRACK32W       (HDN_FIRST-26)
#define HDN_ENDTRACK32A         (HDN_FIRST-7)
#define HDN_ENDTRACK32W         (HDN_FIRST-27)
#define HDN_TRACK32A            (HDN_FIRST-8)
#define HDN_TRACK32W            (HDN_FIRST-28)
#define HDN_GETDISPINFO32A      (HDN_FIRST-9)
#define HDN_GETDISPINFO32W      (HDN_FIRST-29)
#define HDN_BEGINDRACK          (HDN_FIRST-10)
#define HDN_ENDDRACK            (HDN_FIRST-11)


#define Header_Layout(hwndHD,playout) \
  (BOOL32)SendMessage32A((hwndHD),HDM_LAYOUT,0,(LPARAM)(LPHDLAYOUT)(playout))


typedef struct _HD_LAYOUT
{
    RECT32      *prc;
    WINDOWPOS32 *pwpos;
} HDLAYOUT, *LPHDLAYOUT;

#define HD_LAYOUT   HDLAYOUT


typedef struct _HD_ITEMA
{
    UINT32    mask;
    INT32     cxy;
    LPSTR     pszText;
    HBITMAP32 hbm;
    INT32     cchTextMax;
    INT32     fmt;
    LPARAM    lParam;
    INT32     iImage;
    INT32     iOrder;
} HD_ITEMA;


typedef struct _HD_HITTESTINFO
{
    POINT32 pt;
    UINT32  flags;
    INT32   iItem;
} HDHITTESTINFO, *LPHDHITTESTINFO;

#define HD_HITTESTINFO   HDHITTESTINFO


typedef struct tagNMHEADERA
{
    NMHDR    hdr;
    INT32    iItem;
    INT32    iButton;
    HD_ITEMA *pitem;
} NMHEADERA, *LPNMHEADERA;


#endif  /* __WINE_COMMCTRL_H */
