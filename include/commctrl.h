/*
 * Common controls definitions
 */

#ifndef __WINE_COMMCTRL_H
#define __WINE_COMMCTRL_H

#include "windows.h"

BOOL32 WINAPI ShowHideMenuCtl (HWND32, UINT32, LPINT32);
VOID WINAPI GetEffectiveClientRect (HWND32, LPRECT32, LPINT32);
VOID WINAPI InitCommonControls (VOID);

typedef struct tagINITCOMMONCONTROLSEX {
    DWORD dwSize;
    DWORD dwICC;
} INITCOMMONCONTROLSEX, *LPINITCOMMONCONTROLSEX;

BOOL32 WINAPI InitCommonControlsEx (LPINITCOMMONCONTROLSEX);

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
#define CCM_FIRST            0x2000

#define CCM_SETBKCOLOR       (CCM_FIRST+1)     /* lParam = bkColor */

#define CCM_SETUNICODEFORMAT (CCM_FIRST+5)
#define CCM_GETUNICODEFORMAT (CCM_FIRST+6)


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


/* callback constants */
#define LPSTR_TEXTCALLBACK32A    ((LPSTR)-1L)
#define LPSTR_TEXTCALLBACK32W    ((LPWSTR)-1L)
#define LPSTR_TEXTCALLBACK WINELIB_NAME_AW(LPSTR_TEXTCALLBACK)


/* owner drawn types */
#define ODT_HEADER      100
#define ODT_TAB         101
#define ODT_LISTVIEW    102


/* StatusWindow */

#define STATUSCLASSNAME16     "msctls_statusbar"
#define STATUSCLASSNAME32A    "msctls_statusbar32"
#define STATUSCLASSNAME32W   L"msctls_statusbar32"
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

HWND32     WINAPI CreateStatusWindow32A(INT32,LPCSTR,HWND32,UINT32);
HWND32     WINAPI CreateStatusWindow32W(INT32,LPCWSTR,HWND32,UINT32);
#define    CreateStatusWindow WINELIB_NAME_AW(CreateStatusWindow)
HWND32     WINAPI CreateUpDownControl(DWORD,INT32,INT32,INT32,INT32,
                                      HWND32,INT32,HINSTANCE32,HWND32,
                                      INT32,INT32,INT32);
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
INT32      WINAPI ImageList_AddIcon (HIMAGELIST, HICON32);
INT32      WINAPI ImageList_AddMasked(HIMAGELIST,HBITMAP32,COLORREF);
BOOL32     WINAPI ImageList_BeginDrag(HIMAGELIST,INT32,INT32,INT32);
BOOL32     WINAPI ImageList_Copy(HIMAGELIST,INT32,HIMAGELIST,INT32,INT32);
HIMAGELIST WINAPI ImageList_Create(INT32,INT32,UINT32,INT32,INT32);
BOOL32     WINAPI ImageList_Destroy(HIMAGELIST);
BOOL32     WINAPI ImageList_DragEnter(HWND32,INT32,INT32);
BOOL32     WINAPI ImageList_DragLeave(HWND32); 
BOOL32     WINAPI ImageList_DragMove(INT32,INT32);
BOOL32     WINAPI ImageList_DragShowNolock (BOOL32);
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
#ifdef __IStream_INTREFACE_DEFINED__
HIMAGELIST WINAPI ImageList_Read (LPSTREAM32);
#endif
BOOL32     WINAPI ImageList_Remove(HIMAGELIST,INT32);
BOOL32     WINAPI ImageList_Replace(HIMAGELIST,INT32,HBITMAP32,HBITMAP32);
INT32      WINAPI ImageList_ReplaceIcon(HIMAGELIST,INT32,HICON32);
COLORREF   WINAPI ImageList_SetBkColor(HIMAGELIST,COLORREF);
BOOL32     WINAPI ImageList_SetDragCursorImage(HIMAGELIST,INT32,INT32,INT32);

BOOL32     WINAPI ImageList_SetIconSize (HIMAGELIST,INT32,INT32);
BOOL32     WINAPI ImageList_SetImageCount (HIMAGELIST,INT32);
BOOL32     WINAPI ImageList_SetOverlayImage(HIMAGELIST,INT32,INT32);
#ifdef __IStream_INTREFACE_DEFINED__
BOOL32     WINAPI ImageList_Write (HIMAGELIST, LPSTREAM32);
#endif

#ifndef __WINE__
#define ImageList_AddIcon(himl,hicon) ImageList_ReplaceIcon(himl,-1,hicon)
#endif
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


#define Header_GetItemCount(hwndHD) \
  (INT32)SendMessage32A((hwndHD),HDM_GETITEMCOUNT,0,0L)
#define Header_InsertItem(hwndHD,i,phdi) \
  (INT32)SendMessage32A((hwndHD),HDM_INSERTITEM,(WPARAM32)(INT32)(i),\
			(LPARAM)(const HD_ITEMA*)(phdi))
#define Header_DeleteItem(hwndHD,i) \
  (BOOL32)SendMessage32A((hwndHD),HDM_DELETEITEM,(WPARAM32)(INT32)(i),0L)
#define Header_GetItem(hwndHD,i,phdi) \
  (BOOL32)SendMessage32A((hwndHD),HDM_GETITEM,(WPARAM32)(INT32)(i),(LPARAM)(HD_ITEMA*)(phdi))
#define Header_SetItem(hwndHD,i,phdi) \
  (BOOL32)SendMessage32A((hwndHD),HDM_SETITEM,(WPARAM32)(INT32)(i),(LPARAM)(const HD_ITEMA*)(phdi))
#define Header_Layout(hwndHD,playout) \
  (BOOL32)SendMessage32A((hwndHD),HDM_LAYOUT,0,(LPARAM)(LPHDLAYOUT)(playout))
#define Header_GetItemRect(hwnd,iItem,lprc) \
  (BOOL32)SendMessage32A((hwnd),HDM_GETITEMRECT,(WPARAM32)iItem,(LPARAM)lprc)
#define Header_SetImageList(hwnd,himl) \
  (HIMAGELIST)SendMessage32A((hwnd),HDM_SETIMAGELIST,0,(LPARAM)himl)
#define Header_GetImageList(hwnd) \
  (HIMAGELIST)SendMessage32A((hwnd),HDM_GETIMAGELIST,0,0)
#define Header_OrderToIndex(hwnd,i) \
  (INT32)SendMessage32A((hwnd),HDM_ORDERTOINDEX,(WPARAM32)i,0)
#define Header_CreateDragImage(hwnd,i) \
  (HIMAGELIST)SendMessage32A((hwnd),HDM_CREATEDRAGIMAGE,(WPARAM32)i,0)
#define Header_GetOrderArray(hwnd,iCount,lpi) \
  (BOOL32)SendMessage32A((hwnd),HDM_GETORDERARRAY,(WPARAM32)iCount,(LPARAM)lpi)
#define Header_SetOrderArray(hwnd,iCount,lpi) \
  (BOOL32)SendMessage32A((hwnd),HDM_SETORDERARRAY,(WPARAM32)iCount,(LPARAM)lpi)
#define Header_SetHotDivider(hwnd,fPos,dw) \
  (INT32)SendMessage32A((hwnd),HDM_SETHOTDIVIDER,(WPARAM32)fPos,(LPARAM)dw)
#define Header_SetUnicodeFormat(hwnd,fUnicode) \
  (BOOL32)SendMessage32A((hwnd),HDM_SETUNICODEFORMAT,(WPARAM32)(fUnicode),0)
#define Header_GetUnicodeFormat(hwnd) \
  (BOOL32)SendMessage32A((hwnd),HDM_GETUNICODEFORMAT,0,0)


/* Toolbar */

#define TOOLBARCLASSNAME16        "ToolbarWindow" 
#define TOOLBARCLASSNAME32W       L"ToolbarWindow32" 
#define TOOLBARCLASSNAME32A       "ToolbarWindow32" 
#define TOOLBARCLASSNAME WINELIB_NAME_AW(TOOLBARCLASSNAME)

 
#define CMB_MASKED              0x02 
 
#define TBSTATE_CHECKED         0x01 
#define TBSTATE_PRESSED         0x02 
#define TBSTATE_ENABLED         0x04 
#define TBSTATE_HIDDEN          0x08 
#define TBSTATE_INDETERMINATE   0x10 
#define TBSTATE_WRAP            0x20 
#define TBSTATE_ELLIPSES        0x40
#define TBSTATE_MARKED          0x80 
 
 
#define TBSTYLE_BUTTON          0x00 
#define TBSTYLE_SEP             0x01 
#define TBSTYLE_CHECK           0x02 
#define TBSTYLE_GROUP           0x04 
#define TBSTYLE_CHECKGROUP      (TBSTYLE_GROUP | TBSTYLE_CHECK) 
#define TBSTYLE_DROPDOWN        0x08 
 
#define TBSTYLE_TOOLTIPS        0x0100 
#define TBSTYLE_WRAPABLE        0x0200 
#define TBSTYLE_ALTDRAG         0x0400 
#define TBSTYLE_FLAT            0x0800 
#define TBSTYLE_LIST            0x1000 
#define TBSTYLE_CUSTOMERASE     0x2000 
 
#define TB_ENABLEBUTTON          (WM_USER+1)
#define TB_CHECKBUTTON           (WM_USER+2)
#define TB_PRESSBUTTON           (WM_USER+3)
#define TB_HIDEBUTTON            (WM_USER+4)
#define TB_INDETERMINATE         (WM_USER+5)
#define TB_ISBUTTONENABLED       (WM_USER+9) 
#define TB_ISBUTTONCHECKED       (WM_USER+10) 
#define TB_ISBUTTONPRESSED       (WM_USER+11) 
#define TB_ISBUTTONHIDDEN        (WM_USER+12) 
#define TB_ISBUTTONINDETERMINATE (WM_USER+13)
#define TB_ISBUTTONHIGHLIGHTED   (WM_USER+14)
#define TB_SETSTATE              (WM_USER+17)
#define TB_GETSTATE              (WM_USER+18)
#define TB_ADDBITMAP             (WM_USER+19)
#define TB_ADDBUTTONS32A         (WM_USER+20)
#define TB_ADDBUTTONS32W         (WM_USER+68)
#define TB_ADDBUTTONS WINELIB_NAME_AW(TB_ADDBUTTONS)
#define TB_HITTEST               (WM_USER+69)
#define TB_INSERTBUTTON32A       (WM_USER+21)
#define TB_INSERTBUTTON32W       (WM_USER+67)
#define TB_INSERTBUTTON WINELIB_NAME_AW(TB_INSERTBUTTON)
#define TB_DELETEBUTTON          (WM_USER+22)
#define TB_GETBUTTON             (WM_USER+23)
#define TB_BUTTONCOUNT           (WM_USER+24)
#define TB_COMMANDTOINDEX        (WM_USER+25)
#define TB_SAVERESTORE32A        (WM_USER+26)
#define TB_SAVERESTORE32W        (WM_USER+76)
#define TB_SAVERESTORE WINELIB_NAME_AW(TB_SAVERESTORE)
#define TB_CUSTOMIZE             (WM_USER+27)
#define TB_ADDSTRING32A          (WM_USER+28) 
#define TB_ADDSTRING32W          (WM_USER+77) 
#define TB_ADDSTRING WINELIB_NAME_AW(TB_ADDSTRING)
#define TB_GETITEMRECT           (WM_USER+29)
#define TB_BUTTONSTRUCTSIZE      (WM_USER+30)
#define TB_SETBUTTONSIZE         (WM_USER+31)
#define TB_SETBITMAPSIZE         (WM_USER+32)
#define TB_AUTOSIZE              (WM_USER+33)
#define TB_GETTOOLTIPS           (WM_USER+35)
#define TB_SETTOOLTIPS           (WM_USER+36)
#define TB_SETPARENT             (WM_USER+37)
#define TB_SETROWS               (WM_USER+39)
#define TB_GETROWS               (WM_USER+40)
#define TB_GETBITMAPFLAGS        (WM_USER+41)
#define TB_SETCMDID              (WM_USER+42)
#define TB_CHANGEBITMAP          (WM_USER+43)
#define TB_GETBITMAP             (WM_USER+44)
#define TB_GETBUTTONTEXT32A      (WM_USER+45)
#define TB_GETBUTTONTEXT32W      (WM_USER+75)
#define TB_GETBUTTONTEXT WINELIB_NAME_AW(TB_GETBUTTONTEXT)
#define TB_REPLACEBITMAP         (WM_USER+46)
#define TB_SETINDENT             (WM_USER+47)
#define TB_SETIMAGELIST          (WM_USER+48)
#define TB_GETIMAGELIST          (WM_USER+49)
#define TB_LOADIMAGES            (WM_USER+50)
#define TB_GETRECT               (WM_USER+51) /* wParam is the Cmd instead of index */
#define TB_SETHOTIMAGELIST       (WM_USER+52)
#define TB_GETHOTIMAGELIST       (WM_USER+53)
#define TB_SETDISABLEDIMAGELIST  (WM_USER+54)
#define TB_GETDISABLEDIMAGELIST  (WM_USER+55)
#define TB_SETSTYLE              (WM_USER+56)
#define TB_GETSTYLE              (WM_USER+57)
#define TB_GETBUTTONSIZE         (WM_USER+58)
#define TB_SETBUTTONWIDTH        (WM_USER+59)
#define TB_SETMAXTEXTROWS        (WM_USER+60)
#define TB_GETTEXTROWS           (WM_USER+61)
#define TB_GETOBJECT             (WM_USER+62)
#define TB_SETDRAWTEXTFLAGS      (WM_USER+70)
#define TB_GETHOTITEM            (WM_USER+71)
#define TB_SETHOTITEM            (WM_USER+72)
#define TB_SETANCHORHIGHLIGHT    (WM_USER+73)
#define TB_GETANCHORHIGHLIGHT    (WM_USER+74)
#define TB_MAPACCELERATOR32A     (WM_USER+78)
#define TB_MAPACCELERATOR32W     (WM_USER+90)
#define TB_MAPACCELERATOR WINELIB_NAME_AW(TB_MAPACCELERATOR)
#define TB_GETINSERTMARK         (WM_USER+79)
#define TB_SETINSERTMARK         (WM_USER+80)
#define TB_INSERTMARKHITTEST     (WM_USER+81)
#define TB_MOVEBUTTON            (WM_USER+82)
#define TB_GETMAXSIZE            (WM_USER+83)
#define TB_SETEXTENDEDSTYLE      (WM_USER+84)
#define TB_GETEXTENDEDSTYLE      (WM_USER+85)
#define TB_GETPADDING            (WM_USER+86)
#define TB_SETPADDING            (WM_USER+87)
#define TB_SETINSERTMARKCOLOR    (WM_USER+88)
#define TB_GETINSERTMARKCOLOR    (WM_USER+89)

#define TB_SETCOLORSCHEME        CCM_SETCOLORSCHEME
#define TB_GETCOLORSCHEME        CCM_GETCOLORSCHEME

#define TB_SETUNICODEFORMAT      CCM_SETUNICODEFORMAT
#define TB_GETUNICODEFORMAT      CCM_GETUNICODEFORMAT


/* This is just for old CreateToolbar. */
/* Don't use it in new programs. */
typedef struct _OLDTBBUTTON {
    INT32 iBitmap;
    INT32 idCommand;
    BYTE  fsState;
    BYTE  fsStyle;
    BYTE  bReserved[2];
    DWORD dwData;
} OLDTBBUTTON, *POLDTBBUTTON, *LPOLDTBBUTTON;
typedef const OLDTBBUTTON *LPCOLDTBBUTTON;


typedef struct _TBBUTTON {
    INT32 iBitmap;
    INT32 idCommand;
    BYTE  fsState;
    BYTE  fsStyle;
    BYTE  bReserved[2];
    DWORD dwData;
    INT32 iString;
} TBBUTTON, *PTBBUTTON, *LPTBBUTTON;
typedef const TBBUTTON *LPCTBBUTTON;


typedef struct _COLORMAP {
    COLORREF from;
    COLORREF to;
} COLORMAP, *LPCOLORMAP;

typedef struct tagTBADDBITMAP {
    HINSTANCE32 hInst;
    UINT32      nID;
} TBADDBITMAP, *LPTBADDBITMAP;

#define HINST_COMMCTRL         ((HINSTANCE32)-1)


typedef struct tagTBSAVEPARAMSA {
    HKEY   hkr;
    LPCSTR pszSubKey;
    LPCSTR pszValueName;
} TBSAVEPARAMS32A, *LPTBSAVEPARAMS32A;

typedef struct tagTBSAVEPARAMSW {
    HKEY   hkr;
    LPCWSTR pszSubKey;
    LPCWSTR pszValueName;
} TBSAVEPARAMSA32W, *LPTBSAVEPARAMSA32W;

#define TBSAVEPARAMS   WINELIB_NAMEAW(TBSAVEPARAMS)
#define LPTBSAVEPARAMS WINELIB_NAMEAW(LPTBSAVEPARAMS)


HWND32 WINAPI
CreateToolbar(HWND32, DWORD, UINT32, INT32, HINSTANCE32,
              UINT32, LPCOLDTBBUTTON, INT32); 
 
HWND32 WINAPI
CreateToolbarEx(HWND32, DWORD, UINT32, INT32,
                HINSTANCE32, UINT32, LPCTBBUTTON, 
                INT32, INT32, INT32, INT32, INT32, UINT32); 

HBITMAP32 WINAPI
CreateMappedBitmap (HINSTANCE32, INT32, UINT32, LPCOLORMAP, INT32); 


/* Tool tips */

#define TOOLTIPS_CLASS16         "tooltips_class"
#define TOOLTIPS_CLASS32W       L"tooltips_class32"
#define TOOLTIPS_CLASS32A        "tooltips_class32"
#define TOOLTIPS_CLASS          WINELIB_NAME_AW(TOOLTIPS_CLASS)
 
#define TTS_ALWAYSTIP           0x01
#define TTS_NOPREFIX            0x02
 
#define TTF_IDISHWND            0x0001
#define TTF_CENTERTIP           0x0002
#define TTF_RTLREADING          0x0004
#define TTF_SUBCLASS            0x0010
#define TTF_TRACK               0x0020
#define TTF_ABSOLUTE            0x0080
#define TTF_TRANSPARENT         0x0100
#define TTF_DI_SETITEM          0x8000  /* valid only on the TTN_NEEDTEXT callback */


#define TTDT_AUTOMATIC          0
#define TTDT_RESHOW             1
#define TTDT_AUTOPOP            2
#define TTDT_INITIAL            3


#define TTM_ACTIVATE            (WM_USER+1)
#define TTM_SETDELAYTIME        (WM_USER+3)
#define TTM_ADDTOOL32A          (WM_USER+4)
#define TTM_ADDTOOL32W          (WM_USER+50)
#define TTM_ADDTOOL WINELIB_NAME_AW(TTM_ADDTOOL)
#define TTM_DELTOOL32A          (WM_USER+5)
#define TTM_DELTOOL32W          (WM_USER+51)
#define TTM_DELTOOL WINELIB_NAME_AW(TTM_DELTOOL)
#define TTM_NEWTOOLRECT32A      (WM_USER+6)
#define TTM_NEWTOOLRECT32W      (WM_USER+52)
#define TTM_NEWTOOLRECT WINELIB_NAME_AW(TTM_NEWTOOLRECT)
#define TTM_RELAYEVENT          (WM_USER+7)
#define TTM_GETTOOLINFO32A      (WM_USER+8)
#define TTM_GETTOOLINFO32W      (WM_USER+53)
#define TTM_GETTOOLINFO WINELIB_NAME_AW(TTM_GETTOOLINFO)
#define TTM_SETTOOLINFO32A      (WM_USER+9)
#define TTM_SETTOOLINFO32W      (WM_USER+54)
#define TTM_SETTOOLINFO WINELIB_NAME_AW(TTM_SETTOOLINFO)
#define TTM_HITTEST32A          (WM_USER+10)
#define TTM_HITTEST32W          (WM_USER+55)
#define TTM_HITTEST WINELIB_NAME_AW(TTM_HITTEST)
#define TTM_GETTEXT32A          (WM_USER+11)
#define TTM_GETTEXT32W          (WM_USER+56)
#define TTM_GETTEXT WINELIB_NAME_AW(TTM_GETTEXT)
#define TTM_UPDATETIPTEXT32A    (WM_USER+12)
#define TTM_UPDATETIPTEXT32W    (WM_USER+57)
#define TTM_UPDATETIPTEXT WINELIB_NAME_AW(TTM_UPDATETIPTEXT)
#define TTM_GETTOOLCOUNT        (WM_USER+13)
#define TTM_ENUMTOOLS32A        (WM_USER+14)
#define TTM_ENUMTOOLS32W        (WM_USER+58)
#define TTM_ENUMTOOLS WINELIB_NAME_AW(TTM_ENUMTOOLS)
#define TTM_GETCURRENTTOOL32A   (WM_USER+15)
#define TTM_GETCURRENTTOOL32W   (WM_USER+59)
#define TTM_GETCURRENTTOOL WINELIB_NAME_AW(TTM_GETCURRENTTOOL)
#define TTM_WINDOWFROMPOINT     (WM_USER+16)
#define TTM_TRACKACTIVATE       (WM_USER+17)
#define TTM_TRACKPOSITION       (WM_USER+18)
#define TTM_SETTIPBKCOLOR       (WM_USER+19)
#define TTM_SETTIPTEXTCOLOR     (WM_USER+20)
#define TTM_GETDELAYTIME        (WM_USER+21)
#define TTM_GETTIPBKCOLOR       (WM_USER+22)
#define TTM_GETTIPTEXTCOLOR     (WM_USER+23)
#define TTM_SETMAXTIPWIDTH      (WM_USER+24)
#define TTM_GETMAXTIPWIDTH      (WM_USER+25)
#define TTM_SETMARGIN           (WM_USER+26)
#define TTM_GETMARGIN           (WM_USER+27)
#define TTM_POP                 (WM_USER+28)
#define TTM_UPDATE              (WM_USER+29)

typedef struct tagTOOLINFOA {
    UINT32 cbSize;
    UINT32 uFlags;
    HWND32 hwnd;
    UINT32 uId;
    RECT32 rect;
    HINSTANCE32 hinst;
    LPSTR lpszText;
    LPARAM lParam;
} TOOLINFOA, *PTOOLINFOA, *LPTOOLINFOA;

typedef struct tagTOOLINFOW {
    UINT32 cbSize;
    UINT32 uFlags;
    HWND32 hwnd;
    UINT32 uId;
    RECT32 rect;
    HINSTANCE32 hinst;
    LPWSTR lpszText;
    LPARAM lParam;
} TOOLINFOW, *PTOOLINFOW, *LPTOOLINFOW;





/* Rebar control */

#define REBARCLASSNAME16        "ReBarWindow"
#define REBARCLASSNAME32A       "ReBarWindow32"
#define REBARCLASSNAME32W       L"ReBarWindow32"
#define REBARCLASSNAME  WINELIB_NAME_AW(REBARCLASSNAME)


/* Trackbar control */

#define TRACKBAR_CLASS16        "msctls_trackbar"
#define TRACKBAR_CLASS32A       "msctls_trackbar32"
#define TRACKBAR_CLASS32W       L"msctls_trackbar32"
#define TRACKBAR_CLASS  WINELIB_NAME_AW(TRACKBAR_CLASS)

#define TBS_AUTOTICKS           0x0001
#define TBS_VERT                0x0002
#define TBS_HORZ                0x0000
#define TBS_TOP                 0x0004
#define TBS_BOTTOM              0x0000
#define TBS_LEFT                0x0004
#define TBS_RIGHT               0x0000
#define TBS_BOTH                0x0008
#define TBS_NOTICKS             0x0010
#define TBS_ENABLESELRANGE      0x0020
#define TBS_FIXEDLENGTH         0x0040
#define TBS_NOTHUMB             0x0080
#define TBS_TOOLTIPS            0x0100

#define TBM_GETPOS              (WM_USER)
#define TBM_GETRANGEMIN         (WM_USER+1)
#define TBM_GETRANGEMAX         (WM_USER+2)
#define TBM_GETTIC              (WM_USER+3)
#define TBM_SETTIC              (WM_USER+4)
#define TBM_SETPOS              (WM_USER+5)

#define TBM_GETSELSTART         (WM_USER+17)
#define TBM_GETSELEND           (WM_USER+18)
#define TBM_CLEARSEL            (WM_USER+19)
#define TBM_SETTICFREQ          (WM_USER+20)
#define TBM_SETPAGESIZE         (WM_USER+21)
#define TBM_GETPAGESIZE         (WM_USER+22)
#define TBM_SETLINESIZE         (WM_USER+23)
#define TBM_GETLINESIZE         (WM_USER+24)
#define TBM_GETTHUMBRECT        (WM_USER+25)
#define TBM_GETCHANNELRECT      (WM_USER+26)
#define TBM_SETTHUMBLENGTH      (WM_USER+27)
#define TBM_GETTHUMBLENGTH      (WM_USER+28)


/* Pager control */

#define WC_PAGESCROLLER32A      "SysPager"
#define WC_PAGESCROLLER32W      L"SysPager"
#define WC_PAGESCROLLER  WINELIB_NAME_AW(WC_PAGESCROLLER)


/* Treeview control */

#define WC_TREEVIEW32A          "SysTreeView32"
#define WC_TREEVIEW32W          L"SysTreeView32"
#define WC_TREEVIEW  WINELIB_NAME_AW(WC_TREEVIEW)

#define TV_FIRST      0x1100


/* Listview control */

#define WC_LISTVIEW32A          "SysListView32"
#define WC_LISTVIEW32W          L"SysListView32"
#define WC_LISTVIEW  WINELIB_NAME_AW(WC_LISTVIEW)

#define LVM_FIRST               0x1000

#define LVM_SETBKCOLOR          (LVM_FIRST+1)
#define LVM_SETIMAGELIST        (LVM_FIRST+3)



#endif  /* __WINE_COMMCTRL_H */
