/*
 * Common controls definitions
 */

#ifndef __WINE_COMMCTRL_H
#define __WINE_COMMCTRL_H

#include "windows.h"
#include "imagelist.h"

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
#define CCM_SETCOLORSCHEME   (CCM_FIRST+2)
#define CCM_GETCOLORSCHEME   (CCM_FIRST+3)
#define CCM_GETDROPTARGET    (CCM_FIRST+4)
#define CCM_SETUNICODEFORMAT (CCM_FIRST+5)
#define CCM_GETUNICODEFORMAT (CCM_FIRST+6)


/* common notification codes (WM_NOTIFY)*/
#define NM_FIRST                (0U-  0U)       // generic to all controls
#define NM_LAST                 (0U- 99U)

#define LVN_FIRST               (0U-100U)       // listview
#define LVN_LAST                (0U-199U)

#define HDN_FIRST               (0U-300U)       // header
#define HDN_LAST                (0U-399U)

#define TVN_FIRST               (0U-400U)       // treeview
#define TVN_LAST                (0U-499U)

#define TTN_FIRST               (0U-520U)       // tooltips
#define TTN_LAST                (0U-549U)

#define TCN_FIRST               (0U-550U)       // tab control
#define TCN_LAST                (0U-580U)

// Shell reserved               (0U-580U) -  (0U-589U)

#define CDN_FIRST               (0U-601U)       // common dialog (new)
#define CDN_LAST                (0U-699U)

#define TBN_FIRST               (0U-700U)       // toolbar
#define TBN_LAST                (0U-720U)

#define UDN_FIRST               (0U-721)        // updown
#define UDN_LAST                (0U-740)

#define MCN_FIRST               (0U-750U)       // monthcal
#define MCN_LAST                (0U-759U)

#define DTN_FIRST               (0U-760U)       // datetimepick
#define DTN_LAST                (0U-799U)

#define CBEN_FIRST              (0U-800U)       // combo box ex
#define CBEN_LAST               (0U-830U)

#define RBN_FIRST               (0U-831U)       // rebar
#define RBN_LAST                (0U-859U)

#define IPN_FIRST               (0U-860U)       // internet address
#define IPN_LAST                (0U-879U)       // internet address

#define SBN_FIRST               (0U-880U)       // status bar
#define SBN_LAST                (0U-899U)

#define PGN_FIRST               (0U-900U)       // Pager Control
#define PGN_LAST                (0U-950U)


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

#define HDN_ITEMCHANGINGA       (HDN_FIRST-0)
#define HDN_ITEMCHANGINGW       (HDN_FIRST-20)
#define HDN_ITEMCHANGEDA        (HDN_FIRST-1)
#define HDN_ITEMCHANGEDW        (HDN_FIRST-21)
#define HDN_ITEMCLICKA          (HDN_FIRST-2)
#define HDN_ITEMCLICKW          (HDN_FIRST-22)
#define HDN_ITEMDBLCLICKA       (HDN_FIRST-3)
#define HDN_ITEMDBLCLICKW       (HDN_FIRST-23)
#define HDN_DIVIDERDBLCLICKA    (HDN_FIRST-5)
#define HDN_DIVIDERDBLCLICKW    (HDN_FIRST-25)
#define HDN_BEGINTRACKA         (HDN_FIRST-6)
#define HDN_BEGINTRACKW         (HDN_FIRST-26)
#define HDN_ENDTRACKA           (HDN_FIRST-7)
#define HDN_ENDTRACKW           (HDN_FIRST-27)
#define HDN_TRACKA              (HDN_FIRST-8)
#define HDN_TRACKW              (HDN_FIRST-28)
#define HDN_GETDISPINFOA        (HDN_FIRST-9)
#define HDN_GETDISPINFOW        (HDN_FIRST-29)
#define HDN_BEGINDRAG           (HDN_FIRST-10)
#define HDN_ENDDRAG             (HDN_FIRST-11)


#define HDN_ITEMCHANGING         HDN_ITEMCHANGINGA
#define HDN_ITEMCHANGED          HDN_ITEMCHANGEDA
#define HDN_ITEMCLICK            HDN_ITEMCLICKA
#define HDN_ITEMDBLCLICK         HDN_ITEMDBLCLICKA
#define HDN_DIVIDERDBLCLICK      HDN_DIVIDERDBLCLICKA
#define HDN_BEGINTRACK           HDN_BEGINTRACKA
#define HDN_ENDTRACK             HDN_ENDTRACKA
#define HDN_TRACK                HDN_TRACKA
#define HDN_GETDISPINFO          HDN_GETDISPINFOA

#define LVN_ITEMCHANGING        (LVN_FIRST-0)
#define LVN_ITEMCHANGED         (LVN_FIRST-1)
#define LVN_INSERTITEM          (LVN_FIRST-2)
#define LVN_DELETEITEM          (LVN_FIRST-3)
#define LVN_DELETEALLITEMS      (LVN_FIRST-4)
#define LVN_BEGINLABELEDITA     (LVN_FIRST-5)
#define LVN_BEGINLABELEDITW     (LVN_FIRST-75)
#define LVN_ENDLABELEDITA       (LVN_FIRST-6)
#define LVN_ENDLABELEDITW       (LVN_FIRST-76)
#define LVN_COLUMNCLICK         (LVN_FIRST-8)
#define LVN_BEGINDRAG           (LVN_FIRST-9)
#define LVN_BEGINRDRAG          (LVN_FIRST-11)
#define LVN_ODCACHEHINT         (LVN_FIRST-13)
#define LVN_ODFINDITEMA         (LVN_FIRST-52)
#define LVN_ODFINDITEMW         (LVN_FIRST-79)
#define LVN_ITEMACTIVATE        (LVN_FIRST-14)
#define LVN_ODSTATECHANGED      (LVN_FIRST-15)
#define LVN_HOTTRACK            (LVN_FIRST-21)
#define LVN_GETDISPINFOA        (LVN_FIRST-50)
#define LVN_GETDISPINFOW        (LVN_FIRST-77)
#define LVN_SETDISPINFOA        (LVN_FIRST-51)
#define LVN_SETDISPINFOW        (LVN_FIRST-78)

#define LVN_ODFINDITEM          LVN_ODFINDITEMA
#define LVN_BEGINLABELEDIT      LVN_BEGINLABELEDITA
#define LVN_ENDLABELEDIT        LVN_ENDLABELEDITA
#define LVN_GETDISPINFO         LVN_GETDISPINFOA
#define LVN_SETDISPINFO         LVN_SETDISPINFOA

/* callback constants */
#define LPSTR_TEXTCALLBACK32A    ((LPSTR)-1L)
#define LPSTR_TEXTCALLBACK32W    ((LPWSTR)-1L)
#define LPSTR_TEXTCALLBACK WINELIB_NAME_AW(LPSTR_TEXTCALLBACK)


/* owner drawn types */
#define ODT_HEADER      100
#define ODT_TAB         101
#define ODT_LISTVIEW    102

/* common notification structures */
typedef struct tagNMTOOLTIPSCREATED
{
    NMHDR  hdr;
    HWND32 hwndToolTips;
} NMTOOLTIPSCREATED, *LPNMTOOLTIPSCREATED;


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
#define SB_SETTIPTEXT32A      (WM_USER+16)
#define SB_SETTIPTEXT32W      (WM_USER+17)
#define SB_SETTIPTEXT WINELIB_NAME_AW(SB_SETTIPTEXT)
#define SB_GETTIPTEXT32A      (WM_USER+18)
#define SB_GETTIPTEXT32W      (WM_USER+19)
#define SB_GETTIPTEXT WINELIB_NAME_AW(SB_GETTIPTEXT)
#define SB_GETICON            (WM_USER+20)
#define SB_SETBKCOLOR         CCM_SETBKCOLOR   /* lParam = bkColor */

#define SBT_NOBORDERS         0x0100
#define SBT_POPOUT            0x0200
#define SBT_RTLREADING        0x0400  /* not supported */
#define SBT_TOOLTIPS          0x0800
#define SBT_OWNERDRAW         0x1000

#define SBARS_SIZEGRIP        0x0100

#define SBN_FIRST             (0U-880U)
#define SBN_LAST              (0U-899U)
#define SBN_SIMPLEMODECHANGE  (SBN_FIRST-0)


HWND32 WINAPI CreateStatusWindow32A (INT32, LPCSTR, HWND32, UINT32);
HWND32 WINAPI CreateStatusWindow32W (INT32, LPCWSTR, HWND32, UINT32);
#define    CreateStatusWindow WINELIB_NAME_AW(CreateStatusWindow)
VOID WINAPI DrawStatusText32A (HDC32, LPRECT32, LPCSTR, UINT32);
VOID WINAPI DrawStatusText32W (HDC32, LPRECT32, LPCWSTR, UINT32);
#define    DrawStatusText WINELIB_NAME_AW(DrawStatusText)
VOID WINAPI MenuHelp (UINT32, WPARAM32, LPARAM, HMENU32,
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

HWND32 WINAPI CreateUpDownControl (DWORD, INT32, INT32, INT32, INT32,
                                   HWND32, INT32, HINSTANCE32, HWND32,
                                   INT32, INT32, INT32);

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


/* ImageList */
/*
#if defined(__WINE__) && defined(__WINE_IMAGELIST_C)
#else
struct _IMAGELIST;
typedef struct _IMAGELIST *HIMAGELIST;
#endif*/  /* __WINE__ */

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

#define TBIF_IMAGE              0x00000001
#define TBIF_TEXT               0x00000002
#define TBIF_STATE              0x00000004
#define TBIF_STYLE              0x00000008
#define TBIF_LPARAM             0x00000010
#define TBIF_COMMAND            0x00000020
#define TBIF_SIZE               0x00000040
 
#define TBN_FIRST               (0U-700U)
#define TBN_LAST                (0U-720U)
#define TBN_GETBUTTONINFO32A    (TBN_FIRST-0)
#define TBN_GETBUTTONINFO32W    (TBN_FIRST-20)

#define TBN_GETINFOTIP32A       (TBN_FIRST-18)
#define TBN_GETINFOTIP32W       (TBN_FIRST-19)


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
#define TB_GETBUTTONINFO32W      (WM_USER+63)
#define TB_GETBUTTONINFO32A      (WM_USER+65)
#define TB_GETBUTTONINFO WINELIB_NAME_AW(TB_GETBUTTONINFO)
#define TB_SETBUTTONINFO32W      (WM_USER+64)
#define TB_SETBUTTONINFO32A      (WM_USER+66)
#define TB_SETBUTTONINFO WINELIB_NAME_AW(TB_SETBUTTONINFO)
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

typedef struct
{
    UINT32 cbSize;
    DWORD  dwMask;
    INT32  idCommand;
    INT32  iImage;
    BYTE   fsState;
    BYTE   fsStyle;
    WORD   cx;
    DWORD  lParam;
    LPSTR  pszText;
    INT32  cchText;
} TBBUTTONINFO32A, *LPTBBUTTONINFO32A;

typedef struct
{
    UINT32 cbSize;
    DWORD  dwMask;
    INT32  idCommand;
    INT32  iImage;
    BYTE   fsState;
    BYTE   fsStyle;
    WORD   cx;
    DWORD  lParam;
    LPWSTR pszText;
    INT32  cchText;
} TBBUTTONINFO32W, *LPTBBUTTONINFO32W;

#define TBBUTTONINFO   WINELIB_NAMEAW(TBBUTTONINFO)
#define LPTBBUTTONINFO WINELIB_NAMEAW(LPTBBUTTONINFO)

typedef struct tagNMTBGETINFOTIPA
{
    NMHDR  hdr;
    LPSTR  pszText;
    INT32  cchTextMax;
    INT32  iItem;
    LPARAM lParam;
} NMTBGETINFOTIP32A, *LPNMTBGETINFOTIP32A;

typedef struct tagNMTBGETINFOTIPW
{
    NMHDR  hdr;
    LPWSTR pszText;
    INT32  cchTextMax;
    INT32  iItem;
    LPARAM lParam;
} NMTBGETINFOTIP32W, *LPNMTBGETINFOTIP32W;

#define NMTBGETINFOTIP   WINELIB_NAMEAW(NMTBGETINFOFTIP)
#define LPNMTBGETINFOTIP WINELIB_NAMEAW(LPNMTBGETINFOTIP)


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

#define INFOTIPSIZE             1024
 
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


#define TTN_FIRST               (0U-520U)
#define TTN_LAST                (0U-549U)
#define TTN_GETDISPINFO32A      (TTN_FIRST-0)
#define TTN_GETDISPINFO32W      (TTN_FIRST-10)
#define TTN_GETDISPINFO WINELIB_NAME_AW(TTN_GETDISPINFO)
#define TTN_SHOW                (TTN_FIRST-1)
#define TTN_POP                 (TTN_FIRST-2)

typedef struct tagTOOLINFOA {
    UINT32 cbSize;
    UINT32 uFlags;
    HWND32 hwnd;
    UINT32 uId;
    RECT32 rect;
    HINSTANCE32 hinst;
    LPSTR lpszText;
    LPARAM lParam;
} TTTOOLINFO32A, *PTOOLINFO32A, *LPTTTOOLINFO32A;

typedef struct tagTOOLINFOW {
    UINT32 cbSize;
    UINT32 uFlags;
    HWND32 hwnd;
    UINT32 uId;
    RECT32 rect;
    HINSTANCE32 hinst;
    LPWSTR lpszText;
    LPARAM lParam;
} TTTOOLINFO32W, *PTOOLINFO32W, *LPTTTOOLINFO32W;

#define TTTOOLINFO WINELIB_NAME_AW(TTTOOLINFO)
#define PTOOLINFO WINELIB_NAME_AW(PTOOLINFO)
#define LPTTTOOLINFO WINELIB_NAME_AW(LPTTTOOLINFO)

typedef struct _TT_HITTESTINFOA
{
    HWND32        hwnd;
    POINT32       pt;
    TTTOOLINFO32A ti;
} TTHITTESTINFO32A, *LPTTHITTESTINFO32A;

typedef struct _TT_HITTESTINFOW
{
    HWND32        hwnd;
    POINT32       pt;
    TTTOOLINFO32W ti;
} TTHITTESTINFO32W, *LPTTHITTESTINFO32W;

#define TTHITTESTINFO WINELIB_NAME_AW(TTHITTESTINFO)
#define LPTTHITTESTINFO WINELIB_NAME_AW(LPTTHITTESTINFO)

typedef struct tagNMTTDISPINFOA
{
    NMHDR hdr;
    LPSTR lpszText;
    CHAR  szText[80];
    HINSTANCE32 hinst;
    UINT32      uFlags;
    LPARAM      lParam;
} NMTTDISPINFO32A, *LPNMTTDISPINFO32A;

typedef struct tagNMTTDISPINFOW
{
    NMHDR       hdr;
    LPWSTR      lpszText;
    WCHAR       szText[80];
    HINSTANCE32 hinst;
    UINT32      uFlags;
    LPARAM      lParam;
} NMTTDISPINFO32W, *LPNMTTDISPINFO32W;

#define NMTTDISPINFO WINELIB_NAME_AW(NMTTDISPINFO)
#define LPNMTTDISPINFO WINELIB_NAME_AW(LPNMTTDISPINFO)


/* Rebar control */

#define REBARCLASSNAME16        "ReBarWindow"
#define REBARCLASSNAME32A       "ReBarWindow32"
#define REBARCLASSNAME32W       L"ReBarWindow32"
#define REBARCLASSNAME  WINELIB_NAME_AW(REBARCLASSNAME)

#define RBIM_IMAGELIST          0x00000001

#define RBBIM_STYLE             0x00000001
#define RBBIM_COLORS            0x00000002
#define RBBIM_TEXT              0x00000004
#define RBBIM_IMAGE             0x00000008
#define RBBIM_CHILD             0x00000010
#define RBBIM_CHILDSIZE         0x00000020
#define RBBIM_SIZE              0x00000040
#define RBBIM_BACKGROUND        0x00000080
#define RBBIM_ID                0x00000100
#define RBBIM_IDEALSIZE         0x00000200
#define RBBIM_LPARAM            0x00000400
#define RBBIM_HEADERSIZE        0x00000800


#define RB_INSERTBAND32A        (WM_USER+1)
#define RB_INSERTBAND32W        (WM_USER+10)
#define RB_INSERTBANND WINELIB_NAME_AW(RB_INSERTBAND)
#define RB_DELETEBAND           (WM_USER+2)
#define RB_GETBARINFO           (WM_USER+3)
#define RB_SETBARINFO           (WM_USER+4)
#define RB_GETBANDINFO32        (WM_USER+5)   /* just for compatibility */
#define RB_SETBANDINFO32A       (WM_USER+6)
#define RB_SETBANDINFO32W       (WM_USER+11)
#define RB_SETBANDINFO WINELIB_NAME_AW(RB_SETBANDINFO)
#define RB_SETPARENT            (WM_USER+7)
#define RB_HITTEST              (WM_USER+8)
#define RB_GETRECT              (WM_USER+9)
#define RB_GETBANDCOUNT         (WM_USER+12)
#define RB_GETROWCOUNT          (WM_USER+13)
#define RB_GETROWHEIGHT         (WM_USER+14)
#define RB_IDTOINDEX            (WM_USER+16)
#define RB_GETTOOLTIPS          (WM_USER+17)
#define RB_SETTOOLTIPS          (WM_USER+18)
#define RB_SETBKCOLOR           (WM_USER+19)
#define RB_GETBKCOLOR           (WM_USER+20)
#define RB_SETTEXTCOLOR         (WM_USER+21)
#define RB_GETTEXTCOLOR         (WM_USER+22)
#define RB_SIZETORECT           (WM_USER+23)
#define RB_BEGINDRAG            (WM_USER+24)
#define RB_ENDDRAG              (WM_USER+25)
#define RB_DRAGMOVE             (WM_USER+26)
#define RB_GETBARHEIGHT         (WM_USER+27)
#define RB_GETBANDINFO32W       (WM_USER+28)
#define RB_GETBANDINFO32A       (WM_USER+29)
#define RB_GETBANDINFO WINELIB_NAME_AW(RB_GETBANDINFO)
#define RB_MINIMIZEBAND         (WM_USER+30)
#define RB_MAXIMIZEBAND         (WM_USER+31)

#define RB_GETBANDORDERS        (WM_USER+34)
#define RB_SHOWBAND             (WM_USER+35)

#define RB_SETPALETTE           (WM_USER+37)
#define RB_GETPALETTE           (WM_USER+38)
#define RB_MOVEBAND             (WM_USER+39)
#define RB_GETDROPTARGET        CCS_GETDROPTARGET
#define RB_SETCOLORSCHEME       CCS_SETCOLORSCHEME
#define RB_GETCOLORSCHEME       CCS_GETCOLORSCHEME
#define RB_SETUNICODEFORMAT     CCS_SETUNICODEFORMAT
#define RB_GETUNICODEFORMAT     CCS_GETUNICODEFORMAT


typedef struct tagREBARINFO
{
    UINT32     cbSize;
    UINT32     fMask;
    HIMAGELIST himl;
} REBARINFO, *LPREBARINFO;

typedef struct tagREBARBANDINFOA
{
    UINT32    cbSize;
    UINT32    fMask;
    UINT32    fStyle;
    COLORREF  clrFore;
    COLORREF  clrBack;
    LPSTR     lpText;
    UINT32    cch;
    INT32     iImage;
    HWND32    hwndChild;
    UINT32    cxMinChild;
    UINT32    cyMinChild;
    UINT32    cx;
    HBITMAP32 hbmBack;
    UINT32    wID;
    UINT32    cyChild;
    UINT32    cyMaxChild;
    UINT32    cyIntegral;
    UINT32    cxIdeal;
    LPARAM    lParam;
    UINT32    cxHeader;
} REBARBANDINFO32A, *LPREBARBANDINFO32A;





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

#define TBTS_TOP                0
#define TBTS_LEFT               1
#define TBTS_BOTTOM             2
#define TBTS_RIGHT              3

#define TB_LINEUP               0
#define TB_LINEDOWN             1
#define TB_PAGEUP               2
#define TB_PAGEDOWN             3
#define TB_THUMBPOSITION        4
#define TB_THUMBTRACK           5
#define TB_TOP                  6
#define TB_BOTTOM               7
#define TB_ENDTRACK             8

#define TBCD_TICS               0x0001
#define TBCD_THUMB              0x0002
#define TBCD_CHANNEL            0x0003

#define TBM_GETPOS              (WM_USER)
#define TBM_GETRANGEMIN         (WM_USER+1)
#define TBM_GETRANGEMAX         (WM_USER+2)
#define TBM_GETTIC              (WM_USER+3)
#define TBM_SETTIC              (WM_USER+4)
#define TBM_SETPOS              (WM_USER+5)
#define TBM_SETRANGE            (WM_USER+6)
#define TBM_SETRANGEMIN         (WM_USER+7)
#define TBM_SETRANGEMAX         (WM_USER+8)
#define TBM_CLEARTICS           (WM_USER+9)
#define TBM_SETSEL              (WM_USER+10)
#define TBM_SETSELSTART         (WM_USER+11)
#define TBM_SETSELEND           (WM_USER+12)
#define TBM_GETPTICS            (WM_USER+14)
#define TBM_GETTICPOS           (WM_USER+15)
#define TBM_GETNUMTICS          (WM_USER+16)
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
#define TBM_SETTOOLTIPS         (WM_USER+29)
#define TBM_GETTOOLTIPS         (WM_USER+30)
#define TBM_SETTIPSIDE          (WM_USER+31)
#define TBM_SETBUDDY            (WM_USER+32)
#define TBM_GETBUDDY            (WM_USER+33)
#define TBM_SETUNICODEFORMAT    CCM_SETUNICODEFORMAT
#define TBM_GETUNICODEFORMAT    CCM_GETUNICODEFORMAT


/* Pager control */

#define WC_PAGESCROLLER32A      "SysPager"
#define WC_PAGESCROLLER32W      L"SysPager"
#define WC_PAGESCROLLER  WINELIB_NAME_AW(WC_PAGESCROLLER)

#define PGS_VERT                0x00000000
#define PGS_HORZ                0x00000001
#define PGS_AUTOSCROLL          0x00000002
#define PGS_DRAGNDROP           0x00000004

#define PGF_INVISIBLE           0
#define PGF_NORMAL              1
#define PGF_GRAYED              2
#define PGF_DEPRESSED           4
#define PGF_HOT                 8

#define PGB_TOPORLEFT           0
#define PGB_BOTTOMORRIGHT       1

#define PGM_FIRST               0x1400

#define PGM_SETCHILD            (PGM_FIRST+1)
#define PGM_RECALCSIZE          (PGM_FIRST+2)
#define PGM_FORWARDMOUSE        (PGM_FIRST+3)
#define PGM_SETBKCOLOR          (PGM_FIRST+4)
#define PGM_GETBKCOLOR          (PGM_FIRST+5)
#define PGM_SETBORDER           (PGM_FIRST+6)
#define PGM_GETBORDER           (PGM_FIRST+7)
#define PGM_SETPOS              (PGM_FIRST+8)
#define PGM_GETPOS              (PGM_FIRST+9)
#define PGM_SETBUTTONSIZE       (PGM_FIRST+10)
#define PGM_GETBUTTONSIZE       (PGM_FIRST+11)
#define PGM_GETBUTTONSTATE      (PGM_FIRST+12)
#define PGM_GETDROPTARGET       CCM_GETDROPTARGET



/* Treeview control */

#define WC_TREEVIEW32A          "SysTreeView32"
#define WC_TREEVIEW32W          L"SysTreeView32"
#define WC_TREEVIEW  WINELIB_NAME_AW(WC_TREEVIEW)

#define TVSIL_NORMAL            0
#define TVSIL_STATE             2

#define TV_FIRST                0x1100

#define TVM_INSERTITEMA         (TV_FIRST+0)
#define TVM_INSERTITEMW         (TV_FIRST+50)
#define TVM_DELETEITEM          (TV_FIRST+1)
#define TVM_EXPAND              (TV_FIRST+2)
#define TVM_GETITEMRECT         (TV_FIRST+4)
#define TVM_GETCOUNT            (TV_FIRST+5)
#define TVM_GETINDENT           (TV_FIRST+6)
#define TVM_SETINDENT           (TV_FIRST+7)
#define TVM_GETIMAGELIST        (TV_FIRST+8)
#define TVM_SETIMAGELIST        (TV_FIRST+9)
#define TVM_GETNEXTITEM         (TV_FIRST+10)
#define TVM_SELECTITEM          (TV_FIRST+11)
#define TVM_GETITEMA            (TV_FIRST+12)
#define TVM_GETITEMW            (TV_FIRST+62)
#define TVM_SETITEMA            (TV_FIRST+13)
#define TVM_SETITEMW            (TV_FIRST+63)
#define TVM_EDITLABELA          (TV_FIRST+14)
#define TVM_EDITLABELW          (TV_FIRST+65)
#define TVM_GETEDITCONTROL      (TV_FIRST+15)
#define TVM_GETVISIBLECOUNT     (TV_FIRST+16)
#define TVM_HITTEST             (TV_FIRST+17)
#define TVM_CREATEDRAGIMAGE     (TV_FIRST+18)
#define TVM_SORTCHILDREN        (TV_FIRST+19)
#define TVM_ENSUREVISIBLE       (TV_FIRST+20)
#define TVM_SORTCHILDRENCB      (TV_FIRST+21)
#define TVM_ENDEDITLABELNOW     (TV_FIRST+22)
#define TVM_GETISEARCHSTRINGA   (TV_FIRST+23)
#define TVM_GETISEARCHSTRINGW   (TV_FIRST+64)
#define TVM_SETTOOLTIPS         (TV_FIRST+24)
#define TVM_GETTOOLTIPS         (TV_FIRST+25)

/* Listview control */

#define WC_LISTVIEW32A          "SysListView32"
#define WC_LISTVIEW32W          L"SysListView32"
#define WC_LISTVIEW  WINELIB_NAME_AW(WC_LISTVIEW)

#define LVM_FIRST               0x1000

#define LVM_SETBKCOLOR          (LVM_FIRST+1)
#define LVM_GETIMAGELIST        (LVM_FIRST+2)
#define LVM_SETIMAGELIST        (LVM_FIRST+3)
#define LVM_GETITEMCOUNT        (LVM_FIRST+4)
#define LVM_GETITEM             (LVM_FIRST+5)
#define LVM_INSERTITEM          (LVM_FIRST+7)
#define LVM_DELETEALLITEMS      (LVM_FIRST+9)
#define LVM_SETITEMPOSITION     (LVM_FIRST+15)
#define LVM_INSERTCOLUMN        (LVM_FIRST+27)
#define LVM_SORTITEMS           (LVM_FIRST+48)
#define LVM_GETSELECTEDCOUNT    (LVM_FIRST+50)

#define LVS_ICON                0x0000
#define LVS_REPORT              0x0001
#define LVS_SMALLICON           0x0002
#define LVS_LIST                0x0003
#define LVS_TYPEMASK            0x0003
#define LVS_SINGLESEL           0x0004
#define LVS_SHOWSELALWAYS       0x0008
#define LVS_SORTASCENDING       0x0010
#define LVS_SORTDESCENDING      0x0020
#define LVS_SHAREIMAGELISTS     0x0040
#define LVS_NOLABELWRAP         0x0080
#define LVS_AUTOARRANGE         0x0100
#define LVS_EDITLABELS          0x0200
#define LVS_OWNERDATA           0x1000
#define LVS_NOSCROLL            0x2000
#define LVS_TYPESTYLEMASK       0xfc00
#define LVS_ALIGNTOP            0x0000
#define LVS_ALIGNLEFT           0x0800
#define LVS_ALIGNMASK           0x0c00
#define LVS_OWNERDRAWFIXED      0x0400
#define LVS_NOCOLUMNHEADER      0x4000
#define LVS_NOSORTHEADER        0x8000

#define I_IMAGECALLBACK         (-1)
#define I_INDENTCALLBACK        (-1)
#define LV_ITEMA LVITEMA
#define LV_ITEMW LVITEMW

#define LV_ITEM LVITEM

#define LVITEMA_V1_SIZE CCSIZEOF_STRUCT(LVITEMA, lParam)
#define LVITEMW_V1_SIZE CCSIZEOF_STRUCT(LVITEMW, lParam)

typedef struct tagLVITEMA
{
    UINT32 mask;
    int iItem;
    int iSubItem;
    UINT32 state;
    UINT32 stateMask;
    LPSTR pszText;
    int cchTextMax;
    int iImage;
    LPARAM lParam;
    int iIndent;	//(_WIN32_IE >= 0x0300)
} LVITEMA, * LPLVITEMA;

typedef struct tagLVITEMW
{
    UINT32 mask;
    int iItem;
    int iSubItem;
    UINT32 state;
    UINT32 stateMask;
    LPWSTR pszText;
    int cchTextMax;
    int iImage;
    LPARAM lParam;
    int iIndent;	//(_WIN32_IE >= 0x0300)
} LVITEMW, * LPLVITEMW;

#define LVITEM    LVITEMA
#define LPLVITEM  LPLVITEMA
#define LVITEM_V1_SIZE LVITEMA_V1_SIZE

#define LV_COLUMNA      LVCOLUMNA
#define LV_COLUMNW      LVCOLUMNW
#define LV_COLUMN       LVCOLUMN
#define LVCOLUMNA_V1_SIZE CCSIZEOF_STRUCT(LVCOLUMNA, iSubItem)
#define LVCOLUMNW_V1_SIZE CCSIZEOF_STRUCT(LVCOLUMNW, iSubItem)

typedef struct tagLVCOLUMNA
{   UINT32 mask;
    int fmt;
    int cx;
    LPSTR pszText;
    int cchTextMax;
    int iSubItem;
    int iImage;  //(_WIN32_IE >= 0x0300)
    int iOrder;  //(_WIN32_IE >= 0x0300)
} LVCOLUMNA,* LPLVCOLUMNA;

typedef struct tagLVCOLUMNW
{   UINT32 mask;
    int fmt;
    int cx;
    LPWSTR pszText;
    int cchTextMax;
    int iSubItem;
    int iImage;	//(_WIN32_IE >= 0x0300)
    int iOrder;	//(_WIN32_IE >= 0x0300)
} LVCOLUMNW,* LPLVCOLUMNW;

#define  LVCOLUMN               LVCOLUMNA
#define  LPLVCOLUMN             LPLVCOLUMNA
#define LVCOLUMN_V1_SIZE LVCOLUMNA_V1_SIZE

#define LVCF_FMT                0x0001
#define LVCF_WIDTH              0x0002
#define LVCF_TEXT               0x0004
#define LVCF_SUBITEM            0x0008
#define LVCF_IMAGE              0x0010
#define LVCF_ORDER              0x0020


#define LVCFMT_LEFT             0x0000
#define LVCFMT_RIGHT            0x0001
#define LVCFMT_CENTER           0x0002
#define LVCFMT_JUSTIFYMASK      0x0003
#define LVCFMT_IMAGE            0x0800
#define LVCFMT_BITMAP_ON_RIGHT  0x1000
#define LVCFMT_COL_HAS_IMAGES   0x8000

#define SNDMSG SendMessage32A
#define ListView_GetImageList(hwnd, iImageList) (HIMAGELIST)SNDMSG((hwnd), LVM_GETIMAGELIST, (WPARAM)(INT)(iImageList), 0L)

#define LVSIL_NORMAL            0
#define LVSIL_SMALL             1
#define LVSIL_STATE             2


#define ListView_SetImageList(hwnd, himl, iImageList) (HIMAGELIST)(UINT32)SNDMSG((hwnd), LVM_SETIMAGELIST, (WPARAM32)(iImageList), (LPARAM)(UINT32)(HIMAGELIST)(himl))
#define ListView_GetItemCount(hwnd)(int)SNDMSG((hwnd), LVM_GETITEMCOUNT, 0, 0L)
#define ListView_GetItem(hwnd, pitem)(BOOL32)SNDMSG((hwnd), LVM_GETITEM, 0, (LPARAM)(LV_ITEM *)(pitem))
#define ListView_InsertItem(hwnd, pitem) (int)SNDMSG((hwnd), LVM_INSERTITEM, 0, (LPARAM)(const LV_ITEM *)(pitem))
#define ListView_DeleteAllItems(hwnd) (BOOL32)SNDMSG((hwnd), LVM_DELETEALLITEMS, 0, 0L)
#define ListView_InsertColumn(hwnd, iCol, pcol)(int)SNDMSG((hwnd), LVM_INSERTCOLUMN, (WPARAM32)(int)(iCol), (LPARAM)(const LV_COLUMN *)(pcol))
typedef int (CALLBACK *PFNLVCOMPARE)(LPARAM, LPARAM, LPARAM);
#define ListView_SortItems(hwndLV, _pfnCompare, _lPrm)(BOOL32)SNDMSG((hwndLV), LVM_SORTITEMS, (WPARAM32)(LPARAM)_lPrm,(LPARAM)(PFNLVCOMPARE)_pfnCompare)
#define ListView_SetItemPosition(hwndLV, i, x, y)(BOOL32)SNDMSG((hwndLV), LVM_SETITEMPOSITION, (WPARAM32)(int)(i), MAKELPARAM((x), (y)))
#define ListView_GetSelectedCount(hwndLV)(UINT32)SNDMSG((hwndLV), LVM_GETSELECTEDCOUNT, 0, 0L)

#define LVIF_TEXT               0x0001
#define LVIF_IMAGE              0x0002
#define LVIF_PARAM              0x0004
#define LVIF_STATE              0x0008
#define LVIF_INDENT             0x0010
#define LVIF_NORECOMPUTE        0x0800

#define LVIS_FOCUSED            0x0001
#define LVIS_SELECTED           0x0002
#define LVIS_CUT                0x0004
#define LVIS_DROPHILITED        0x0008
#define LVIS_ACTIVATING         0x0020

#define LVIS_OVERLAYMASK        0x0F00
#define LVIS_STATEIMAGEMASK     0xF000

#define LPNM_LISTVIEW   LPNMLISTVIEW
#define NM_LISTVIEW     NMLISTVIEW

typedef struct tagNMLISTVIEW
{   NMHDR   hdr;
    int     iItem;
    int     iSubItem;
    UINT32  uNewState;
    UINT32  uOldState;
    UINT32  uChanged;
    POINT32 ptAction;
    LPARAM  lParam;
} NMLISTVIEW,*LPNMLISTVIEW;


#define LV_DISPINFOA    NMLVDISPINFOA
#define LV_DISPINFOW    NMLVDISPINFOW

#define LV_DISPINFO     NMLVDISPINFO

typedef struct tagLVDISPINFO {
    NMHDR hdr;
    LVITEMA item;
} NMLVDISPINFOA, *LPNMLVDISPINFOA;

typedef struct tagLVDISPINFOW {
    NMHDR hdr;
    LVITEMW item;
} NMLVDISPINFOW, * LPNMLVDISPINFOW;

#define  NMLVDISPINFO           NMLVDISPINFOA


#endif  /* __WINE_COMMCTRL_H */
