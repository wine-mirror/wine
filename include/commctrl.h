/*
 * Common controls definitions
 */

#ifndef __WINE_COMMCTRL_H
#define __WINE_COMMCTRL_H

#include "windows.h"
#include "imagelist.h"

#ifdef __cplusplus
extern "C" {
#endif

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
#define NM_FIRST                (0U-  0U)
#define NM_LAST                 (0U- 99U)
#define NM_OUTOFMEMORY          (NM_FIRST-1)
#define NM_CLICK                (NM_FIRST-2)
#define NM_DBLCLK               (NM_FIRST-3)
#define NM_RETURN               (NM_FIRST-4)
#define NM_RCLICK               (NM_FIRST-5)
#define NM_RDBLCLK              (NM_FIRST-6)
#define NM_SETFOCUS             (NM_FIRST-7)
#define NM_KILLFOCUS            (NM_FIRST-8)
#define NM_CUSTOMDRAW           (NM_FIRST-12)
#define NM_HOVER                (NM_FIRST-13)
#define NM_NCHITTEST            (NM_FIRST-14)
#define NM_KEYDOWN              (NM_FIRST-15)
#define NM_RELEASEDCAPTURE      (NM_FIRST-16)
#define NM_SETCURSOR            (NM_FIRST-17)
#define NM_CHAR                 (NM_FIRST-18)
#define NM_TOOLTIPSCREATED      (NM_FIRST-19)


/* callback constants */
#define LPSTR_TEXTCALLBACK32A    ((LPSTR)-1L)
#define LPSTR_TEXTCALLBACK32W    ((LPWSTR)-1L)
#define LPSTR_TEXTCALLBACK WINELIB_NAME_AW(LPSTR_TEXTCALLBACK)

#define I_IMAGECALLBACK          (-1)
#define I_INDENTCALLBACK         (-1)
#define I_CHILDRENCALLBACK       (-1)


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


#ifndef CCSIZEOF_STRUCT
#define CCSIZEOF_STRUCT(name, member) \
    (((INT32)((LPBYTE)(&((name*)0)->member)-((LPBYTE)((name*)0))))+ \
    sizeof(((name*)0)->member))
#endif


/* This is only for Winelib applications. DON't use it wine itself!!! */
#define SNDMSG WINELIB_NAME_AW(SendMessage)



/* Custom Draw messages */

#define CDRF_DODEFAULT          0x0
#define CDRF_NEWFONT            0x00000002
#define CDRF_SKIPDEFAULT        0x00000004
#define CDRF_NOTIFYPOSTPAINT    0x00000010
#define CDRF_NOTIFYITEMDRAW     0x00000020
#define CDRF_NOTIFYSUBITEMDRAW  0x00000020
#define CDRF_NOTIFYPOSTERASE    0x00000040
/* #define CDRF_NOTIFYITEMERASE    0x00000080          obsolete ? */


/* drawstage flags */

#define CDDS_PREPAINT           1
#define CDDS_POSTPAINT          2
#define CDDS_PREERASE           3
#define CDDS_POSTERASE          4

#define CDDS_ITEM				0x00010000
#define CDDS_ITEMPREPAINT		(CDDS_ITEM | CDDS_PREPAINT)
#define CDDS_ITEMPOSTPAINT		(CDDS_ITEM | CDDS_POSTPAINT)
#define CDDS_ITEMPREERASE		(CDDS_ITEM | CDDS_PREERASE)
#define CDDS_ITEMPOSTERASE		(CDDS_ITEM | CDDS_POSTERASE)
#define CDDS_SUBITEM            0x00020000

/* itemState flags */

#define CDIS_SELECTED	 	0x0001
#define CDIS_GRAYED			0x0002
#define CDIS_DISABLED		0x0004
#define CDIS_CHECKED		0x0008
#define CDIS_FOCUS			0x0010
#define CDIS_DEFAULT		0x0020
#define CDIS_HOT			0x0040
#define CDIS_MARKED         0x0080
#define CDIS_INDETERMINATE  0x0100


typedef struct tagNMCUSTOMDRAWINFO
{
	NMHDR	hdr;
	DWORD	dwDrawStage;
	HDC32	hdc;
	RECT32	rc;
	DWORD	dwItemSpec; 
	UINT32	uItemState;
	LPARAM	lItemlParam;
} NMCUSTOMDRAW, *LPNMCUSTOMDRAW;

typedef struct tagNMTTCUSTOMDRAW
{
    NMCUSTOMDRAW nmcd;
    UINT32       uDrawFlags;
} NMTTCUSTOMDRAW, *LPNMTTCUSTOMDRAW;




/* StatusWindow */

#define STATUSCLASSNAME16	"msctls_statusbar"
#define STATUSCLASSNAME32A	"msctls_statusbar32"
/* Does not work. gcc creates 4 byte wide strings.
 * #define STATUSCLASSNAME32W	L"msctls_statusbar32"
 */
static const WCHAR	_scn32w[] = {
'm','s','c','t','l','s','_','s','t','a','t','u','s','b','a','r','3','2',0
};
#define STATUSCLASSNAME32W	_scn32w
#define STATUSCLASSNAME		WINELIB_NAME_AW(STATUSCLASSNAME)

#define SBT_NOBORDERS		0x0100
#define SBT_POPOUT		0x0200
#define SBT_RTLREADING		0x0400  /* not supported */
#define SBT_TOOLTIPS		0x0800
#define SBT_OWNERDRAW		0x1000

#define SBARS_SIZEGRIP		0x0100

#define SB_SETTEXT32A		(WM_USER+1)
#define SB_SETTEXT32W		(WM_USER+11)
#define SB_SETTEXT		WINELIB_NAME_AW(SB_SETTEXT)
#define SB_GETTEXT32A		(WM_USER+2)
#define SB_GETTEXT32W		(WM_USER+13)
#define SB_GETTEXT		WINELIB_NAME_AW(SB_GETTEXT)
#define SB_GETTEXTLENGTH32A	(WM_USER+3)
#define SB_GETTEXTLENGTH32W	(WM_USER+12)
#define SB_GETTEXTLENGTH	WINELIB_NAME_AW(SB_GETTEXTLENGTH)
#define SB_SETPARTS		(WM_USER+4)
#define SB_GETPARTS		(WM_USER+6)
#define SB_GETBORDERS		(WM_USER+7)
#define SB_SETMINHEIGHT		(WM_USER+8)
#define SB_SIMPLE		(WM_USER+9)
#define SB_GETRECT		(WM_USER+10)
#define SB_ISSIMPLE		(WM_USER+14)
#define SB_SETICON		(WM_USER+15)
#define SB_SETTIPTEXT32A	(WM_USER+16)
#define SB_SETTIPTEXT32W	(WM_USER+17)
#define SB_SETTIPTEXT		WINELIB_NAME_AW(SB_SETTIPTEXT)
#define SB_GETTIPTEXT32A	(WM_USER+18)
#define SB_GETTIPTEXT32W	(WM_USER+19)
#define SB_GETTIPTEXT		WINELIB_NAME_AW(SB_GETTIPTEXT)
#define SB_GETICON		(WM_USER+20)
#define SB_SETBKCOLOR		CCM_SETBKCOLOR   /* lParam = bkColor */
#define SB_GETUNICODEFORMAT	CCM_GETUNICODEFORMAT
#define SB_SETUNICODEFORMAT	CCM_SETUNICODEFORMAT

#define SBN_FIRST		(0U-880U)
#define SBN_LAST		(0U-899U)
#define SBN_SIMPLEMODECHANGE	(SBN_FIRST-0)

HWND32 WINAPI CreateStatusWindow32A (INT32, LPCSTR, HWND32, UINT32);
HWND32 WINAPI CreateStatusWindow32W (INT32, LPCWSTR, HWND32, UINT32);
#define CreateStatusWindow WINELIB_NAME_AW(CreateStatusWindow)
VOID WINAPI DrawStatusText32A (HDC32, LPRECT32, LPCSTR, UINT32);
VOID WINAPI DrawStatusText32W (HDC32, LPRECT32, LPCWSTR, UINT32);
#define DrawStatusText WINELIB_NAME_AW(DrawStatusText)
VOID WINAPI MenuHelp (UINT32, WPARAM32, LPARAM, HMENU32,
                      HINSTANCE32, HWND32, LPUINT32);

/**************************************************************************
 *  Drag List control
 */

typedef struct tagDRAGLISTINFO
{
    UINT32  uNotification;
    HWND32  hWnd;
    POINT32 ptCursor;
} DRAGLISTINFO, *LPDRAGLISTINFO;

#define DL_BEGINDRAG            (WM_USER+133)
#define DL_DRAGGING             (WM_USER+134)
#define DL_DROPPED              (WM_USER+135)
#define DL_CANCELDRAG           (WM_USER+136)

#define DL_CURSORSET            0
#define DL_STOPCURSOR           1
#define DL_COPYCURSOR           2
#define DL_MOVECURSOR           3

#define DRAGLISTMSGSTRING       TEXT("commctrl_DragListMsg")

BOOL32 WINAPI MakeDragList (HWND32);
VOID   WINAPI DrawInsert (HWND32, HWND32, INT32);
INT32  WINAPI LBItemFromPt (HWND32, POINT32, BOOL32);

  
/* UpDown */

#define UPDOWN_CLASS16          "msctls_updown"
#define UPDOWN_CLASS32A         "msctls_updown32"
#define UPDOWN_CLASS32W         L"msctls_updown32"
#define UPDOWN_CLASS            WINELIB_NAME_AW(UPDOWN_CLASS)

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
#define UDN_LAST           (0U-740)
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
#if !defined(__WINE__) || !defined(__WINE_IMAGELIST_C)
struct _IMAGELIST;
typedef struct _IMAGELIST *HIMAGELIST;
#endif */  /* __WINE__ */

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
BOOL32     WINAPI ImageList_EndDrag(VOID);
COLORREF   WINAPI ImageList_GetBkColor(HIMAGELIST);
HIMAGELIST WINAPI ImageList_GetDragImage(POINT32*,POINT32*);
HICON32    WINAPI ImageList_GetIcon(HIMAGELIST,INT32,UINT32);
BOOL32     WINAPI ImageList_GetIconSize(HIMAGELIST,INT32*,INT32*);
INT32      WINAPI ImageList_GetImageCount(HIMAGELIST);
BOOL32     WINAPI ImageList_GetImageInfo(HIMAGELIST,INT32,IMAGEINFO*);
BOOL32     WINAPI ImageList_GetImageRect(HIMAGELIST,INT32,LPRECT32);
HIMAGELIST WINAPI ImageList_LoadImage32A(HINSTANCE32,LPCSTR,INT32,INT32,
                                         COLORREF,UINT32,UINT32);
HIMAGELIST WINAPI ImageList_LoadImage32W(HINSTANCE32,LPCWSTR,INT32,INT32,
                                         COLORREF,UINT32,UINT32);
#define    ImageList_LoadImage WINELIB_NAME_AW(ImageList_LoadImage)
HIMAGELIST WINAPI ImageList_Merge(HIMAGELIST,INT32,HIMAGELIST,INT32,INT32,INT32);
#ifdef __IStream_INTREFACE_DEFINED__
HIMAGELIST WINAPI ImageList_Read(LPSTREAM32);
#endif
BOOL32     WINAPI ImageList_Remove(HIMAGELIST,INT32);
BOOL32     WINAPI ImageList_Replace(HIMAGELIST,INT32,HBITMAP32,HBITMAP32);
INT32      WINAPI ImageList_ReplaceIcon(HIMAGELIST,INT32,HICON32);
COLORREF   WINAPI ImageList_SetBkColor(HIMAGELIST,COLORREF);
BOOL32     WINAPI ImageList_SetDragCursorImage(HIMAGELIST,INT32,INT32,INT32);

BOOL32     WINAPI ImageList_SetIconSize(HIMAGELIST,INT32,INT32);
BOOL32     WINAPI ImageList_SetImageCount(HIMAGELIST,INT32);
BOOL32     WINAPI ImageList_SetOverlayImage(HIMAGELIST,INT32,INT32);
#ifdef __IStream_INTREFACE_DEFINED__
BOOL32     WINAPI ImageList_Write(HIMAGELIST, LPSTREAM32);
#endif

#ifndef __WINE__
#define ImageList_AddIcon(himl,hicon) ImageList_ReplaceIcon(himl,-1,hicon)
#endif
#define ImageList_ExtractIcon(hi,himl,i) ImageList_GetIcon(himl,i,0)
#define ImageList_LoadBitmap(hi,lpbmp,cx,cGrow,crMask) \
  ImageList_LoadImage(hi,lpbmp,cx,cGrow,crMask,IMAGE_BITMAP,0)
#define ImageList_RemoveAll(himl) ImageList_Remove(himl,-1)


/* Flat Scrollbar control */

#define FLATSB_CLASS16        "flatsb_class"
#define FLATSB_CLASS32A       "flatsb_class32"
#define FLATSB_CLASS32W       L"flatsb_class32"
#define FLATSB_CLASS          WINELIB_NAME_AW(FLATSB_CLASS)

BOOL32  WINAPI FlatSB_EnableScrollBar(HWND32, INT32, UINT32);
BOOL32  WINAPI FlatSB_ShowScrollBar(HWND32, INT32, BOOL32);
BOOL32  WINAPI FlatSB_GetScrollRange(HWND32, INT32, LPINT32, LPINT32);
BOOL32  WINAPI FlatSB_GetScrollInfo(HWND32, INT32, LPSCROLLINFO);
INT32   WINAPI FlatSB_GetScrollPos(HWND32, INT32);
BOOL32  WINAPI FlatSB_GetScrollProp(HWND32, INT32, LPINT32);
INT32   WINAPI FlatSB_SetScrollPos(HWND32, INT32, INT32, BOOL32);
INT32   WINAPI FlatSB_SetScrollInfo(HWND32, INT32, LPSCROLLINFO, BOOL32);
INT32   WINAPI FlatSB_SetScrollRange(HWND32, INT32, INT32, INT32, BOOL32);
BOOL32  WINAPI FlatSB_SetScrollProp(HWND32, UINT32, INT32, BOOL32);
BOOL32  WINAPI InitializeFlatSB(HWND32);
HRESULT WINAPI UninitializeFlatSB(HWND32);


/* Header control */

#define WC_HEADER16		"SysHeader" 
#define WC_HEADER32A		"SysHeader32" 
#define WC_HEADER32W		L"SysHeader32" 
#define WC_HEADER		WINELIB_NAME_AW(WC_HEADER)
 
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
#define HDM_INSERTITEM		WINELIB_NAME_AW(HDM_INSERTITEM)
#define HDM_DELETEITEM          (HDM_FIRST+2)
#define HDM_GETITEM32A          (HDM_FIRST+3)
#define HDM_GETITEM32W          (HDM_FIRST+11)
#define HDM_GETITEM		WINELIB_NAME_AW(HDM_GETITEM)
#define HDM_SETITEM32A          (HDM_FIRST+4)
#define HDM_SETITEM32W          (HDM_FIRST+12)
#define HDM_SETITEM		WINELIB_NAME_AW(HDM_SETITEM)
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
#define HDM_GETUNICODEFORMAT    CCM_GETUNICODEFORMAT
#define HDM_SETUNICODEFORMAT    CCM_SETUNICODEFORMAT

#define HDN_FIRST               (0U-300U)
#define HDN_LAST                (0U-399U)
#define HDN_ITEMCHANGING32A     (HDN_FIRST-0)
#define HDN_ITEMCHANGING32W     (HDN_FIRST-20)
#define HDN_ITEMCHANGING WINELIB_NAME_AW(HDN_ITEMCHANGING)
#define HDN_ITEMCHANGED32A      (HDN_FIRST-1)
#define HDN_ITEMCHANGED32W      (HDN_FIRST-21)
#define HDN_ITEMCHANGED WINELIB_NAME_AW(HDN_ITEMCHANGED)
#define HDN_ITEMCLICK32A        (HDN_FIRST-2)
#define HDN_ITEMCLICK32W        (HDN_FIRST-22)
#define HDN_ITEMCLICK WINELIB_NAME_AW(HDN_ITEMCLICK)
#define HDN_ITEMDBLCLICK32A     (HDN_FIRST-3)
#define HDN_ITEMDBLCLICK32W     (HDN_FIRST-23)
#define HDN_ITEMDBLCLICK WINELIB_NAME_AW(HDN_ITEMDBLCLICK)
#define HDN_DIVIDERDBLCLICK32A  (HDN_FIRST-5)
#define HDN_DIVIDERDBLCLICK32W  (HDN_FIRST-25)
#define HDN_DIVIDERDBLCLICK WINELIB_NAME_AW(HDN_DIVIDERDBLCLICK)
#define HDN_BEGINTRACK32A       (HDN_FIRST-6)
#define HDN_BEGINTRACK32W       (HDN_FIRST-26)
#define HDN_BEGINTRACK WINELIB_NAME_AW(HDN_BEGINTRACK)
#define HDN_ENDTRACK32A         (HDN_FIRST-7)
#define HDN_ENDTRACK32W         (HDN_FIRST-27)
#define HDN_ENDTRACK WINELIB_NAME_AW(HDN_ENDTRACK)
#define HDN_TRACK32A            (HDN_FIRST-8)
#define HDN_TRACK32W            (HDN_FIRST-28)
#define HDN_TRACK WINELIB_NAME_AW(HDN_TRACK)
#define HDN_GETDISPINFO32A      (HDN_FIRST-9)
#define HDN_GETDISPINFO32W      (HDN_FIRST-29)
#define HDN_GETDISPINFO WINELIB_NAME_AW(HDN_GETDISPINFO)
#define HDN_BEGINDRACK          (HDN_FIRST-10)
#define HDN_ENDDRACK            (HDN_FIRST-11)

typedef struct _HD_LAYOUT
{
    RECT32      *prc;
    WINDOWPOS32 *pwpos;
} HDLAYOUT, *LPHDLAYOUT;

#define HD_LAYOUT   HDLAYOUT

typedef struct _HD_ITEM32A
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
} HDITEM32A, *LPHDITEM32A;

typedef struct _HD_ITEM32W
{
    UINT32    mask;
    INT32     cxy;
    LPWSTR    pszText;
    HBITMAP32 hbm;
    INT32     cchTextMax;
    INT32     fmt;
    LPARAM    lParam;
    INT32     iImage;
    INT32     iOrder;
} HDITEM32W, *LPHDITEM32W;

#define HDITEM   WINELIB_NAME_AW(HDITEM)
#define LPHDITEM WINELIB_NAME_AW(LPHDITEM)
#define HD_ITEM  HDITEM

#define HDITEM_V1_SIZE32A CCSIZEOF_STRUCT(HDITEM32A, lParam)
#define HDITEM_V1_SIZE32W CCSIZEOF_STRUCT(HDITEM32W, lParam)
#define HDITEM_V1_SIZE WINELIB_NAME_AW(HDITEM_V1_SIZE)

typedef struct _HD_HITTESTINFO
{
    POINT32 pt;
    UINT32  flags;
    INT32   iItem;
} HDHITTESTINFO, *LPHDHITTESTINFO;

#define HD_HITTESTINFO   HDHITTESTINFO

typedef struct tagNMHEADER32A
{
    NMHDR     hdr;
    INT32     iItem;
    INT32     iButton;
    HDITEM32A *pitem;
} NMHEADER32A, *LPNMHEADER32A;

typedef struct tagNMHEADER32W
{
    NMHDR     hdr;
    INT32     iItem;
    INT32     iButton;
    HDITEM32W *pitem;
} NMHEADER32W, *LPNMHEADER32W;

#define NMHEADER		WINELIB_NAME_AW(NMHEADER)
#define LPNMHEADER		WINELIB_NAME_AW(LPNMHEADER)

typedef struct tagNMHDDISPINFO32A
{
    NMHDR     hdr;
    INT32     iItem;
    UINT32    mask;
    LPSTR     pszText;
    INT32     cchTextMax;
    INT32     iImage;
    LPARAM    lParam;
} NMHDDISPINFO32A, *LPNMHDDISPINFO32A;

typedef struct tagNMHDDISPINFO32W
{
    NMHDR     hdr;
    INT32     iItem;
    UINT32    mask;
    LPWSTR    pszText;
    INT32     cchTextMax;
    INT32     iImage;
    LPARAM    lParam;
} NMHDDISPINFO32W, *LPNMHDDISPINFO32W;

#define NMHDDISPINFO		WINELIB_NAME_AW(NMHDDISPINFO)
#define LPNMHDDISPINFO		WINELIB_NAME_AW(LPNMHDDISPINFO)

#define Header_GetItemCount(hwndHD) \
  (INT32)SendMessage32A((hwndHD),HDM_GETITEMCOUNT,0,0L)
#define Header_InsertItem32A(hwndHD,i,phdi) \
  (INT32)SendMessage32A((hwndHD),HDM_INSERTITEM32A,(WPARAM32)(INT32)(i),(LPARAM)(const HDITEM32A*)(phdi))
#define Header_InsertItem32W(hwndHD,i,phdi) \
  (INT32)SendMessage32W((hwndHD),HDM_INSERTITEM32W,(WPARAM32)(INT32)(i),(LPARAM)(const HDITEM32W*)(phdi))
#define Header_InsertItem WINELIB_NAME_AW(Header_InsertItem)
#define Header_DeleteItem(hwndHD,i) \
  (BOOL32)SendMessage32A((hwndHD),HDM_DELETEITEM,(WPARAM32)(INT32)(i),0L)
#define Header_GetItem32A(hwndHD,i,phdi) \
  (BOOL32)SendMessage32A((hwndHD),HDM_GETITEM32A,(WPARAM32)(INT32)(i),(LPARAM)(HDITEM32A*)(phdi))
#define Header_GetItem32W(hwndHD,i,phdi) \
  (BOOL32)SendMessage32W((hwndHD),HDM_GETITEM32W,(WPARAM32)(INT32)(i),(LPARAM)(HDITEM32W*)(phdi))
#define Header_GetItem WINELIB_NAME_AW(Header_GetItem)
#define Header_SetItem32A(hwndHD,i,phdi) \
  (BOOL32)SendMessage32A((hwndHD),HDM_SETITEM32A,(WPARAM32)(INT32)(i),(LPARAM)(const HDITEM32A*)(phdi))
#define Header_SetItem32W(hwndHD,i,phdi) \
  (BOOL32)SendMessage32W((hwndHD),HDM_SETITEM32W,(WPARAM32)(INT32)(i),(LPARAM)(const HDITEM32W*)(phdi))
#define Header_SetItem WINELIB_NAME_AW(Header_SetItem)
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

#define TOOLBARCLASSNAME16      "ToolbarWindow" 
#define TOOLBARCLASSNAME32W     L"ToolbarWindow32" 
#define TOOLBARCLASSNAME32A     "ToolbarWindow32" 
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

#define TBBF_LARGE		0x0001 

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

#define TBN_FIRST               (0U-700U)
#define TBN_LAST                (0U-720U)
#define TBN_GETBUTTONINFO32A    (TBN_FIRST-0)
#define TBN_GETBUTTONINFO32W    (TBN_FIRST-20)
#define TBN_GETBUTTONINFO WINELIB_NAME_AW(TBN_GETBUTTONINFO)
#define TBN_GETINFOTIP32A       (TBN_FIRST-18)
#define TBN_GETINFOTIP32W       (TBN_FIRST-19)
#define TBN_GETINFOTIP WINELIB_NAME_AW(TBN_GETINFOTIP)


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
#define IDB_STD_SMALL_COLOR     0
#define IDB_STD_LARGE_COLOR     1
#define IDB_VIEW_SMALL_COLOR    4
#define IDB_VIEW_LARGE_COLOR    5
#define IDB_HIST_SMALL_COLOR    8
#define IDB_HIST_LARGE_COLOR    9

#define STD_CUT                 0
#define STD_COPY                1
#define STD_PASTE               2
#define STD_UNDO                3
#define STD_REDOW               4
#define STD_DELETE              5
#define STD_FILENEW             6
#define STD_FILEOPEN            7
#define STD_FILESAVE            8
#define STD_PRINTPRE            9
#define STD_PROPERTIES          10
#define STD_HELP                11
#define STD_FIND                12
#define STD_REPLACE             13
#define STD_PRINT               14

#define VIEW_LARGEICONS         0
#define VIEW_SMALLICONS         1
#define VIEW_LIST               2
#define VIEW_DETAILS            3
#define VIEW_SORTNAME           4
#define VIEW_SORTSIZE           5
#define VIEW_SORTDATE           6
#define VIEW_SORTTYPE           7
#define VIEW_PARENTFOLDER       8
#define VIEW_NETCONNECT         9
#define VIEW_NETDISCONNECT      10
#define VIEW_NEWFOLDER          11

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

#define TBSAVEPARAMS   WINELIB_NAME_AW(TBSAVEPARAMS)
#define LPTBSAVEPARAMS WINELIB_NAME_AW(LPTBSAVEPARAMS)

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

#define TBBUTTONINFO   WINELIB_NAME_AW(TBBUTTONINFO)
#define LPTBBUTTONINFO WINELIB_NAME_AW(LPTBBUTTONINFO)

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

#define NMTBGETINFOTIP   WINELIB_NAME_AW(NMTBGETINFOFTIP)
#define LPNMTBGETINFOTIP WINELIB_NAME_AW(LPNMTBGETINFOTIP)

typedef struct
{
	HINSTANCE32 hInstOld;
	UINT32      nIDOld;
	HINSTANCE32 hInstNew;
	UINT32      nIDNew;
	INT32       nButtons;
} TBREPLACEBITMAP, *LPTBREPLACEBITMAP;

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

#define TOOLTIPS_CLASS16        "tooltips_class"
#define TOOLTIPS_CLASS32A       "tooltips_class32"
#define TOOLTIPS_CLASS32W       L"tooltips_class32"
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

#define TTN_NEEDTEXT TTN_GETDISPINFO
#define TTN_NEEDTEXTA TTN_GETDISPINFO32A
#define TTN_NEEDTEXTW TTN_GETDISPINFO32W

typedef struct tagTOOLINFOA {
    UINT32 cbSize;
    UINT32 uFlags;
    HWND32 hwnd;
    UINT32 uId;
    RECT32 rect;
    HINSTANCE32 hinst;
    LPSTR lpszText;
    LPARAM lParam;
} TTTOOLINFO32A, *LPTOOLINFO32A, *PTOOLINFO32A, *LPTTTOOLINFO32A;

typedef struct tagTOOLINFOW {
    UINT32 cbSize;
    UINT32 uFlags;
    HWND32 hwnd;
    UINT32 uId;
    RECT32 rect;
    HINSTANCE32 hinst;
    LPWSTR lpszText;
    LPARAM lParam;
} TTTOOLINFO32W, *LPTOOLINFO32W, *PTOOLINFO32W, *LPTTTOOLINFO32W;

#define TTTOOLINFO WINELIB_NAME_AW(TTTOOLINFO)
#define TOOLINFO WINELIB_NAME_AW(TTTOOLINFO)
#define PTOOLINFO WINELIB_NAME_AW(PTOOLINFO)
#define LPTTTOOLINFO WINELIB_NAME_AW(LPTTTOOLINFO)
#define LPTOOLINFO WINELIB_NAME_AW(LPTOOLINFO)

#define TTTOOLINFO_V1_SIZE32A CCSIZEOF_STRUCT(TTTOOLINFO32A, lpszText)
#define TTTOOLINFO_V1_SIZE32W CCSIZEOF_STRUCT(TTTOOLINFO32W, lpszText)
#define TTTOOLINFO_V1_SIZE WINELIB_NAME_AW(TTTOOLINFO_V1_SIZE)

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

#define NMTTDISPINFO_V1_SIZE32A CCSIZEOF_STRUCT(NMTTDISPINFO32A, uFlags)
#define NMTTDISPINFO_V1_SIZE32W CCSIZEOF_STRUCT(NMTTDISPINFO32W, uFlags)
#define NMTTDISPINFO_V1_SIZE WINELIB_NAME_AW(NMTTDISPINFO_V1_SIZE)

#define TOOLTIPTEXTW    NMTTDISPINFO32W
#define TOOLTIPTEXTA    NMTTDISPINFO32A
#define TOOLTIPTEXT     NMTTDISPINFO
#define LPTOOLTIPTEXTW  LPNMTTDISPINFOW
#define LPTOOLTIPTEXTA  LPNMTTDISPINFOA
#define LPTOOLTIPTEXT   LPNMTTDISPINFO


/* Rebar control */

#define REBARCLASSNAME16        "ReBarWindow"
#define REBARCLASSNAME32A       "ReBarWindow32"
#define REBARCLASSNAME32W       L"ReBarWindow32"
#define REBARCLASSNAME          WINELIB_NAME_AW(REBARCLASSNAME)

#define RBS_TOOLTIPS            0x0100
#define RBS_VARHEIGHT           0x0200
#define RBS_BANDBORDERS         0x0400
#define RBS_FIXEDORDER          0x0800
#define RBS_REGISTERDROP        0x1000
#define RBS_AUTOSIZE            0x2000
#define RBS_VERTICALGRIPPER     0x4000
#define RBS_DBLCLKTOGGLE        0x8000

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

#define RBBS_BREAK              0x00000001
#define RBBS_FIXEDSIZE          0x00000002
#define RBBS_CHILDEDGE          0x00000004
#define RBBS_HIDDEN             0x00000008
#define RBBS_NOVERT             0x00000010
#define RBBS_FIXEDBMP           0x00000020
#define RBBS_VARIABLEHEIGHT     0x00000040
#define RBBS_GRIPPERALWAYS      0x00000080
#define RBBS_NOGRIPPER          0x00000100

#define RBNM_ID                 0x00000001
#define RBNM_STYLE              0x00000002
#define RBNM_LPARAM             0x00000004

#define RBHT_NOWHERE            0x0001
#define RBHT_CAPTION            0x0002
#define RBHT_CLIENT             0x0003
#define RBHT_GRABBER            0x0004

#define RB_INSERTBAND32A        (WM_USER+1)
#define RB_INSERTBAND32W        (WM_USER+10)
#define RB_INSERTBAND           WINELIB_NAME_AW(RB_INSERTBAND)
#define RB_DELETEBAND           (WM_USER+2)
#define RB_GETBARINFO           (WM_USER+3)
#define RB_SETBARINFO           (WM_USER+4)
#define RB_GETBANDINFO32        (WM_USER+5)   /* just for compatibility */
#define RB_SETBANDINFO32A       (WM_USER+6)
#define RB_SETBANDINFO32W       (WM_USER+11)
#define RB_SETBANDINFO          WINELIB_NAME_AW(RB_SETBANDINFO)
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
#define RB_GETBANDINFO          WINELIB_NAME_AW(RB_GETBANDINFO)
#define RB_MINIMIZEBAND         (WM_USER+30)
#define RB_MAXIMIZEBAND         (WM_USER+31)
#define RB_GETBANDBORDERS       (WM_USER+34)
#define RB_SHOWBAND             (WM_USER+35)
#define RB_SETPALETTE           (WM_USER+37)
#define RB_GETPALETTE           (WM_USER+38)
#define RB_MOVEBAND             (WM_USER+39)
#define RB_GETDROPTARGET        CCM_GETDROPTARGET
#define RB_SETCOLORSCHEME       CCM_SETCOLORSCHEME
#define RB_GETCOLORSCHEME       CCM_GETCOLORSCHEME
#define RB_SETUNICODEFORMAT     CCM_SETUNICODEFORMAT
#define RB_GETUNICODEFORMAT     CCM_GETUNICODEFORMAT

#define RBN_FIRST               (0U-831U)
#define RBN_LAST                (0U-859U)
#define RBN_HEIGHTCHANGE        (RBN_FIRST-0)
#define RBN_GETOBJECT           (RBN_FIRST-1)
#define RBN_LAYOUTCHANGED       (RBN_FIRST-2)
#define RBN_AUTOSIZE            (RBN_FIRST-3)
#define RBN_BEGINDRAG           (RBN_FIRST-4)
#define RBN_ENDDRAG             (RBN_FIRST-5)
#define RBN_DELETINGBAND        (RBN_FIRST-6)
#define RBN_DELETEDBAND         (RBN_FIRST-7)
#define RBN_CHILDSIZE           (RBN_FIRST-8)

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

typedef REBARBANDINFO32A const *LPCREBARBANDINFO32A;

typedef struct tagREBARBANDINFOW
{
    UINT32    cbSize;
    UINT32    fMask;
    UINT32    fStyle;
    COLORREF  clrFore;
    COLORREF  clrBack;
    LPWSTR    lpText;
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
} REBARBANDINFO32W, *LPREBARBANDINFO32W;

typedef REBARBANDINFO32W const *LPCREBARBANDINFO32W;

#define REBARBANDINFO    WINELIB_NAME_AW(REBARBANDINFO)
#define LPREBARBANDINFO  WINELIB_NAME_AW(LPREBARBANDINFO)
#define LPCREBARBANDINFO WINELIB_NAME_AW(LPCREBARBANDINFO)

#define REBARBANDINFO_V3_SIZE32A CCSIZEOF_STRUCT(REBARBANDINFO32A, wID)
#define REBARBANDINFO_V3_SIZE32W CCSIZEOF_STRUCT(REBARBANDINFO32W, wID)
#define REBARBANDINFO_V3_SIZE WINELIB_NAME_AW(REBARBANDINFO_V3_SIZE)

typedef struct tagNMREBARCHILDSIZE
{
    NMHDR  hdr;
    UINT32 uBand;
    UINT32 wID;
    RECT32 rcChild;
    RECT32 rcBand;
} NMREBARCHILDSIZE, *LPNMREBARCHILDSIZE;

typedef struct tagNMREBAR
{
    NMHDR  hdr;
    DWORD  dwMask;
    UINT32 uBand;
    UINT32 fStyle;
    UINT32 wID;
    LPARAM lParam;
} NMREBAR, *LPNMREBAR;

typedef struct tagNMRBAUTOSIZE
{
    NMHDR hdr;
    BOOL32 fChanged;
    RECT32 rcTarget;
    RECT32 rcActual;
} NMRBAUTOSIZE, *LPNMRBAUTOSIZE;

typedef struct _RB_HITTESTINFO
{
    POINT32 pt;
    UINT32  flags;
    INT32   iBand;
} RBHITTESTINFO, *LPRBHITTESTINFO;


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

/* only used with PGN_SCROLL */
#define PGF_SCROLLUP            1
#define PGF_SCROLLDOWN          2
#define PGF_SCROLLLEFT          4
#define PGF_SCROLLRIGHT         8

#define PGK_SHIFT               1
#define PGK_CONTROL             2
#define PGK_MENU                4

/* only used with PGN_CALCSIZE */
#define PGF_CALCWIDTH           1
#define PGF_CALCHEIGHT          2

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

#define PGN_FIRST               (0U-900U)
#define PGN_LAST                (0U-950U)
#define PGN_SCROLL              (PGN_FIRST-1)
#define PGN_CALCSIZE            (PGN_FIRST-2)

typedef struct
{
    NMHDR hdr;
    WORD  fwKeys;
    RECT32 rcParent;
    INT32  iDir;
    INT32  iXpos;
    INT32  iYpos;
    INT32  iScroll;
} NMPGSCROLL, *LPNMPGSCROLL;

typedef struct
{
    NMHDR hdr;
    DWORD dwFlag;
    INT32 iWidth;
    INT32 iHeight;
} NMPGCALCSIZE, *LPNMPGCALCSIZE;


/* Treeview control */

#define WC_TREEVIEW32A          "SysTreeView32"
#define WC_TREEVIEW32W          L"SysTreeView32"
#define WC_TREEVIEW             WINELIB_NAME_AW(WC_TREEVIEW)

#define TVSIL_NORMAL            0
#define TVSIL_STATE             2

#define TV_FIRST                0x1100
#define TVM_INSERTITEM32A       (TV_FIRST+0)
#define TVM_INSERTITEM32W       (TV_FIRST+50)
#define TVM_INSERTITEM          WINELIB_NAME_AW(TVM_INSERTITEM)
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
#define TVM_GETITEM32A          (TV_FIRST+12)
#define TVM_GETITEM32W          (TV_FIRST+62)
#define TVM_GETITEM             WINELIB_NAME_AW(TVM_GETITEM)
#define TVM_SETITEM32A          (TV_FIRST+13)
#define TVM_SETITEM32W          (TV_FIRST+63)
#define TVM_SETITEM             WINELIB_NAME_AW(TVM_SETITEM)
#define TVM_EDITLABEL32A        (TV_FIRST+14)
#define TVM_EDITLABEL32W        (TV_FIRST+65)
#define TVM_EDITLABEL           WINELIB_NAME_AW(TVM_EDITLABEL)
#define TVM_GETEDITCONTROL      (TV_FIRST+15)
#define TVM_GETVISIBLECOUNT     (TV_FIRST+16)
#define TVM_HITTEST             (TV_FIRST+17)
#define TVM_CREATEDRAGIMAGE     (TV_FIRST+18)
#define TVM_SORTCHILDREN        (TV_FIRST+19)
#define TVM_ENSUREVISIBLE       (TV_FIRST+20)
#define TVM_SORTCHILDRENCB      (TV_FIRST+21)
#define TVM_ENDEDITLABELNOW     (TV_FIRST+22)
#define TVM_GETISEARCHSTRING32A (TV_FIRST+23)
#define TVM_GETISEARCHSTRING32W (TV_FIRST+64)
#define TVM_GETISEARCHSTRING    WINELIB_NAME_AW(TVM_GETISEARCHSTRING)
#define TVM_SETTOOLTIPS         (TV_FIRST+24)
#define TVM_GETTOOLTIPS         (TV_FIRST+25)
#define TVM_SETINSERTMARK       (TV_FIRST+26)
#define TVM_SETITEMHEIGHT       (TV_FIRST+27)
#define TVM_GETITEMHEIGHT       (TV_FIRST+28)
#define TVM_SETBKCOLOR          (TV_FIRST+29)
#define TVM_SETTEXTCOLOR        (TV_FIRST+30)
#define TVM_GETBKCOLOR          (TV_FIRST+31)
#define TVM_GETTEXTCOLOR        (TV_FIRST+32)
#define TVM_SETSCROLLTIME       (TV_FIRST+33)
#define TVM_GETSCROLLTIME       (TV_FIRST+34)
#define TVM_UNKNOWN35           (TV_FIRST+35)
#define TVM_UNKNOWN36           (TV_FIRST+36)
#define TVM_SETINSERTMARKCOLOR  (TV_FIRST+37)
#define TVM_GETINSERTMARKCOLOR  (TV_FIRST+38)
#define TVM_SETUNICODEFORMAT    CCM_SETUNICODEFORMAT
#define TVM_GETUNICODEFORMAT    CCM_GETUNICODEFORMAT

#define TVN_FIRST               (0U-400U)
#define TVN_LAST                (0U-499U)

#define TVN_SELCHANGING         (TVN_FIRST-1)
#define TVN_SELCHANGED          (TVN_FIRST-2)
#define TVN_GETDISPINFO         (TVN_FIRST-3)
#define TVN_SETDISPINFO         (TVN_FIRST-4)
#define TVN_ITEMEXPANDING       (TVN_FIRST-5)
#define TVN_ITEMEXPANDED        (TVN_FIRST-6)
#define TVN_BEGINDRAG           (TVN_FIRST-7)
#define TVN_BEGINRDRAG          (TVN_FIRST-8)
#define TVN_DELETEITEM          (TVN_FIRST-9)
#define TVN_BEGINLABELEDIT      (TVN_FIRST-10)
#define TVN_ENDLABELEDIT        (TVN_FIRST-11)
#define TVN_KEYDOWN             (TVN_FIRST-12)
#define TVN_GETINFOTIPA         (TVN_FIRST-13)
#define TVN_GETINFOTIPW         (TVN_FIRST-14)
#define TVN_SINGLEEXPAND        (TVN_FIRST-15)


#define TVN_SELCHANGINGW        (TVN_FIRST-50)
#define TVN_SELCHANGEDW         (TVN_FIRST-51)
#define TVN_GETDISPINFOW        (TVN_FIRST-52)
#define TVN_SETDISPINFOW        (TVN_FIRST-53)
#define TVN_ITEMEXPANDINGW      (TVN_FIRST-54)
#define TVN_ITEMEXPANDEDW       (TVN_FIRST-55)
#define TVN_BEGINDRAGW          (TVN_FIRST-56)
#define TVN_BEGINRDRAGW         (TVN_FIRST-57)
#define TVN_DELETEITEMW         (TVN_FIRST-58)
#define TVN_BEGINLABELEDITW     (TVN_FIRST-59)
#define TVN_ENDLABELEDITW       (TVN_FIRST-60)



#define TVIF_TEXT             0x0001
#define TVIF_IMAGE            0x0002
#define TVIF_PARAM            0x0004
#define TVIF_STATE            0x0008
#define TVIF_HANDLE           0x0010
#define TVIF_SELECTEDIMAGE    0x0020
#define TVIF_CHILDREN         0x0040
#define TVIF_INTEGRAL         0x0080
#define TVIF_DI_SETITEM		  0x1000

#define TVI_ROOT              0xffff0000      /* -65536 */
#define TVI_FIRST             0xffff0001      /* -65535 */
#define TVI_LAST              0xffff0002      /* -65534 */
#define TVI_SORT              0xffff0003      /* -65533 */

#define TVIS_FOCUSED          0x0001
#define TVIS_SELECTED         0x0002
#define TVIS_CUT              0x0004
#define TVIS_DROPHILITED      0x0008
#define TVIS_BOLD             0x0010
#define TVIS_EXPANDED         0x0020
#define TVIS_EXPANDEDONCE     0x0040
#define TVIS_OVERLAYMASK      0x0f00
#define TVIS_STATEIMAGEMASK   0xf000
#define TVIS_USERMASK         0xf000

#define TVHT_NOWHERE          0x0001
#define TVHT_ONITEMICON       0x0002
#define TVHT_ONITEMLABEL      0x0004
#define TVHT_ONITEMINDENT     0x0008
#define TVHT_ONITEMBUTTON     0x0010
#define TVHT_ONITEMRIGHT      0x0020
#define TVHT_ONITEMSTATEICON  0x0040
#define TVHT_ONITEM           0x0046
#define TVHT_ABOVE            0x0100
#define TVHT_BELOW            0x0200
#define TVHT_TORIGHT          0x0400
#define TVHT_TOLEFT           0x0800

#define TVS_HASBUTTONS        0x0001
#define TVS_HASLINES          0x0002
#define TVS_LINESATROOT       0x0004
#define TVS_EDITLABELS        0x0008
#define TVS_DISABLEDRAGDROP   0x0010
#define TVS_SHOWSELALWAYS     0x0020
#define TVS_RTLREADING        0x0040
#define TVS_NOTOOLTIPS        0x0080
#define TVS_CHECKBOXES        0x0100
#define TVS_TRACKSELECT       0x0200
#define TVS_SINGLEEXPAND 	  0x0400
#define TVS_INFOTIP      	  0x0800
#define TVS_FULLROWSELECT	  0x1000
#define TVS_NOSCROLL     	  0x2000
#define TVS_NONEVENHEIGHT	  0x4000

#define TVS_SHAREDIMAGELISTS  0x0000
#define TVS_PRIVATEIMAGELISTS 0x0400


#define TVE_COLLAPSE          0x0001
#define TVE_EXPAND            0x0002
#define TVE_TOGGLE            0x0003
#define TVE_EXPANDPARTIAL     0x4000
#define TVE_COLLAPSERESET     0x8000

#define TVGN_ROOT             0
#define TVGN_NEXT             1
#define TVGN_PREVIOUS         2
#define TVGN_PARENT           3
#define TVGN_CHILD            4
#define TVGN_FIRSTVISIBLE     5
#define TVGN_NEXTVISIBLE      6
#define TVGN_PREVIOUSVISIBLE  7
#define TVGN_DROPHILITE       8
#define TVGN_CARET            9
#define TVGN_LASTVISIBLE      10

#define TVC_UNKNOWN           0x00
#define TVC_BYMOUSE           0x01
#define TVC_BYKEYBOARD        0x02


typedef HANDLE32 HTREEITEM;

typedef struct {
      UINT32 mask;
      HTREEITEM hItem;
      UINT32 state;
      UINT32 stateMask;
      LPSTR  pszText;
      INT32  cchTextMax;
      INT32  iImage;
      INT32  iSelectedImage;
      INT32  cChildren;
      LPARAM lParam;
} TVITEM32A, *LPTVITEM32A;

typedef struct {
      UINT32 mask;
      HTREEITEM hItem;
      UINT32 state;
      UINT32 stateMask;
      LPWSTR pszText;
      INT32 cchTextMax;
      INT32 iImage;
      INT32 iSelectedImage;
      INT32 cChildren;
      LPARAM lParam;
} TVITEM32W, *LPTVITEM32W;

#define TV_ITEM     WINELIB_NAME(TV_ITEM)
#define LPTV_ITEM   WINELIB_NAME(LPTV_ITEM)

typedef struct {
      UINT32 mask;
      HTREEITEM hItem;
      UINT32 state;
      UINT32 stateMask;
      LPSTR  pszText;
      INT32  cchTextMax;
      INT32  iImage;
      INT32  iSelectedImage;
      INT32  cChildren;
      LPARAM lParam;
      INT32 iIntegral;
} TVITEMEX32A, *LPTVITEMEX32A;

typedef struct {
      UINT32 mask;
      HTREEITEM hItem;
      UINT32 state;
      UINT32 stateMask;
      LPWSTR pszText;
      INT32 cchTextMax;
      INT32 iImage;
      INT32 iSelectedImage;
      INT32 cChildren;
      LPARAM lParam;
      INT32 iIntegral;
} TVITEMEX32W, *LPTV_ITEMEX32W;

#define TV_ITEMEX   WINELIB_NAME(TV_ITEM)
#define LPTV_ITEMEX WINELIB_NAME(LPTV_ITEM)


typedef struct tagTVINSERTSTRUCT32A {
        HTREEITEM hParent;
        HTREEITEM hInsertAfter;
        union {
           TVITEMEX32A itemex;
           TVITEM32A   item;
        } DUMMYUNIONNAME;
} TVINSERTSTRUCT32A, *LPTVINSERTSTRUCT32A;

typedef struct tagTVINSERTSTRUCT32W {
        HTREEITEM hParent;
        HTREEITEM hInsertAfter;
        union {
           TVITEMEX32W itemex;
           TVITEM32W   item;
        } DUMMYUNIONNAME;
} TVINSERTSTRUCT32W, *LPTVINSERTSTRUCT32W;

#define TV_INSERTSTRUCT   WINELIB_NAME(TVINSERTSTRUCT)
#define LPTV_INSERTSTRUCT WINELIB_NAME(LPTVINSERTSTRUCT)

#define TVINSERTSTRUCT_V1_SIZE32A CCSIZEOF_STRUCT(TVINSERTSTRUCT32A, item)
#define TVINSERTSTRUCT_V1_SIZE32W CCSIZEOF_STRUCT(TVINSERTSTRUCT32W, item)
#define TVINSERTSTRUCT_V1_SIZE    WINELIB_NAME_AW(TVINSERTSTRUCT_V1_SIZE)




typedef struct tagNMTREEVIEW32A {
	NMHDR	hdr;
	UINT32	action;
	TVITEM32A	itemOld;
	TVITEM32A	itemNew;
	POINT32	ptDrag;
} NMTREEVIEW32A, *LPNMTREEVIEW32A;

typedef struct tagNMTREEVIEW32W {
	NMHDR	hdr;
	UINT32	action;
	TVITEM32W	itemOld;
	TVITEM32W	itemNew;
	POINT32	ptDrag;
} NMTREEVIEW32W, *LPNMTREEVIEW32W;

#define NMTREEVIEW     WINELIB_NAME_AW(NMTREEVIEW)
#define LPNMTREEVIEW   WINELIB_NAME_AW(LPNMTREEVIEW)

typedef struct tagTVDISPINFO32A {
	NMHDR	hdr;
	TVITEM32A	item;
} NMTVDISPINFO32A, *LPNMTVDISPINFO32A;

typedef struct tagTVDISPINFO32W {
	NMHDR	hdr;
	TVITEM32W	item;
} NMTVDISPINFO32W, *LPNMTVDISPINFO32W;

#define NMTVDISPINFO            WINELIB_NAME_AW(NMTVDISPINFO)
#define LPNMTVDISPINFO          WINELIB_NAME_AW(LPNMTVDISPINFO)
#define TV_DISPINFO             NMTVDISPINFO

typedef INT32 (CALLBACK *PFNTVCOMPARE)(LPARAM, LPARAM, LPARAM);

typedef struct tagTVSORTCB
{
	HTREEITEM hParent;
	PFNTVCOMPARE lpfnCompare;
	LPARAM lParam;
} TVSORTCB, *LPTVSORTCB;

#define TV_SORTCB TVSORTCB
#define LPTV_SORTCB LPTVSORTCB

typedef struct tagTVHITTESTINFO {
        POINT32 pt;
        UINT32 flags;
        HTREEITEM hItem;
} TVHITTESTINFO, *LPTVHITTESTINFO;

#define TV_HITTESTINFO TVHITTESTINFO


/* Custom Draw Treeview */

#define NMTVCUSTOMDRAW_V3_SIZE CCSIZEOF_STRUCT(NMTVCUSTOMDRAW, clrTextBk)

#define TVCDRF_NOIMAGES     0x00010000

typedef struct tagNMTVCUSTOMDRAW
{
    NMCUSTOMDRAW nmcd;
    COLORREF     clrText;
    COLORREF     clrTextBk;
    INT32 iLevel;                 /* IE>0x0400 */
} NMTVCUSTOMDRAW, *LPNMTVCUSTOMDRAW;

/* Treeview tooltips */

typedef struct tagNMTVGETINFOTIP32A
{
    NMHDR hdr;
    LPSTR pszText;
    INT32 cchTextMax;
    HTREEITEM hItem;
    LPARAM lParam;
} NMTVGETINFOTIP32A, *LPNMTVGETINFOTIP32A;

typedef struct tagNMTVGETINFOTIP32W
{
    NMHDR hdr;
    LPWSTR pszText;
    INT32 cchTextMax;
    HTREEITEM hItem;
    LPARAM lParam;
} NMTVGETINFOTIP32W, *LPNMTVGETINFOTIP32W;


#define NMTVGETINFOTIP          WINELIB_NAME(NMTVGETINFOTIP)





#define TreeView_InsertItem32A(hwnd, phdi) \
  (INT32)SendMessage32A((hwnd), TVM_INSERTITEM32A, 0, \
                            (LPARAM)(LPTVINSERTSTRUCT32A)(phdi))
#define TreeView_DeleteItem(hwnd, hItem) \
  (BOOL32)SendMessage32A((hwnd), TVM_DELETEITEM, 0, (LPARAM)(HTREEITEM)(hItem))
#define TreeView_DeleteAllItems(hwnd) \
  (BOOL32)SendMessage32A((hwnd), TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT)
#define TreeView_Expand(hwnd, hitem, code) \
 (BOOL32)SendMessage32A((hwnd), TVM_EXPAND, (WPARAM)code, \
	(LPARAM)(HTREEITEM)(hitem))

#define TreeView_GetItemRect(hwnd, hitem, prc, code) \
 (*(HTREEITEM *)prc = (hitem), (BOOL)SendMessage32A((hwnd), \
			TVM_GETITEMRECT, (WPARAM32)(code), (LPARAM)(RECT *)(prc)))

#define TreeView_GetCount(hwnd) \
    (UINT32)SendMessage32A((hwnd), TVM_GETCOUNT, 0, 0)
#define TreeView_GetIndent(hwnd) \
    (UINT32)SendMessage32A((hwnd), TVM_GETINDENT, 0, 0)
#define TreeView_SetIndent(hwnd, indent) \
    (BOOL32)SendMessage32A((hwnd), TVM_SETINDENT, (WPARAM)indent, 0)

#define TreeView_GetImageList(hwnd, iImage) \
    (HIMAGELIST)SendMessage32A((hwnd), TVM_GETIMAGELIST, iImage, 0)

#define TreeView_SetImageList(hwnd, himl, iImage) \
    (HIMAGELIST)SendMessage32A((hwnd), TVM_SETIMAGELIST, iImage, \
 (LPARAM)(UINT32)(HIMAGELIST)(himl))

#define TreeView_GetNextItem(hwnd, hitem, code) \
    (HTREEITEM)SendMessage32A((hwnd), TVM_GETNEXTITEM, (WPARAM)code,\
(LPARAM)(HTREEITEM) (hitem))

#define TreeView_GetChild(hwnd, hitem) \
	 	 TreeView_GetNextItem(hwnd, hitem , TVGN_CHILD)
#define TreeView_GetNextSibling(hwnd, hitem) \
		TreeView_GetNextItem(hwnd, hitem , TVGN_NEXT)
#define TreeView_GetPrevSibling(hwnd, hitem) \
		TreeView_GetNextItem(hwnd, hitem , TVGN_PREVIOUS)
#define TreeView_GetParent(hwnd, hitem) \
		TreeView_GetNextItem(hwnd, hitem , TVGN_PARENT)
#define TreeView_GetFirstVisible(hwnd)  \
		TreeView_GetNextItem(hwnd, NULL, TVGN_FIRSTVISIBLE)
#define TreeView_GetLastVisible(hwnd)   \
		TreeView_GetNextItem(hwnd, NULL, TVGN_LASTVISIBLE)
#define TreeView_GetNextVisible(hwnd, hitem) \
		TreeView_GetNextItem(hwnd, hitem , TVGN_NEXTVISIBLE)
#define TreeView_GetPrevVisible(hwnd, hitem) \
		TreeView_GetNextItem(hwnd, hitem , TVGN_PREVIOUSVISIBLE)
#define TreeView_GetSelection(hwnd) \
		TreeView_GetNextItem(hwnd, NULL, TVGN_CARET)
#define TreeView_GetDropHilight(hwnd) \
		TreeView_GetNextItem(hwnd, NULL, TVGN_DROPHILITE)
#define TreeView_GetRoot(hwnd) \
		TreeView_GetNextItem(hwnd, NULL, TVGN_ROOT)
#define TreeView_GetLastVisible(hwnd) \
		TreeView_GetNextItem(hwnd, NULL, TVGN_LASTVISIBLE)


#define TreeView_Select(hwnd, hitem, code) \
 (UINT32)SendMessage32A((hwnd), TVM_SELECTITEM, (WPARAM32)code, \
(LPARAM)(UINT32)(hitem))


#define TreeView_SelectItem(hwnd, hitem) \
		TreeView_Select(hwnd, hitem, TVGN_CARET)
#define TreeView_SelectDropTarget(hwnd, hitem) \
        	TreeView_Select(hwnd, hitem, TVGN_DROPHILITE)
/* FIXME
#define TreeView_SelectSetFirstVisible(hwnd, hitem)  \ 
		TreeView_Select(hwnd, hitem, TVGN_FIRSTVISIBLE)
*/

#define TreeView_GetItem32A(hwnd, pitem) \
 (BOOL32)SendMessage32A((hwnd), TVM_GETITEM32A, 0, (LPARAM) (TVITEM32A *)(pitem))

#define TreeView_SetItem32A(hwnd, pitem) \
 (BOOL32)SendMessage32A((hwnd), TVM_SETITEM32A, 0, (LPARAM)(const TVITEM32A *)(pitem)) 

#define TreeView_EditLabel(hwnd, hitem) \
    (HWND)SendMessage32A((hwnd), TVM_EDITLABEL, 0, (LPARAM)(HTREEITEM)(hitem))


#define TreeView_GetEditControl(hwnd) \
    (HWND)SendMessage32A((hwnd), TVM_GETEDITCONTROL, 0, 0)

#define TreeView_GetVisibleCount(hwnd) \
    (UINT32)SendMessage32A((hwnd), TVM_GETVISIBLECOUNT, 0, 0)

#define TreeView_HitTest(hwnd, lpht) \
    (HTREEITEM)SendMessage32A((hwnd), TVM_HITTEST, 0,\
(LPARAM)(LPTVHITTESTINFO)(lpht))

#define TreeView_CreateDragImage(hwnd, hitem) \
    (HIMAGELIST)SendMessage32A((hwnd), TVM_CREATEDRAGIMAGE, 0,\
(LPARAM)(HTREEITEM)(hitem))

#define TreeView_SortChildren(hwnd, hitem, recurse) \
    (BOOL32)SendMessage32A((hwnd), TVM_SORTCHILDREN, (WPARAM32)recurse,\
(LPARAM)(HTREEITEM)(hitem))

#define TreeView_EnsureVisible(hwnd, hitem) \
    (BOOL32)SendMessage32A((hwnd), TVM_ENSUREVISIBLE, 0, (LPARAM)(UINT32)(hitem))

#define TreeView_SortChildrenCB(hwnd, psort, recurse) \
    (BOOL32)SendMessage32A((hwnd), TVM_SORTCHILDRENCB, (WPARAM32)recurse, \
    (LPARAM)(LPTV_SORTCB)(psort))

#define TreeView_EndEditLabelNow(hwnd, fCancel) \
    (BOOL32)SendMessage32A((hwnd), TVM_ENDEDITLABELNOW, (WPARAM32)fCancel, 0)

#define TreeView_GetISearchString(hwndTV, lpsz) \
    (BOOL32)SendMessage32A((hwndTV), TVM_GETISEARCHSTRING, 0, \
							(LPARAM)(LPTSTR)lpsz)

#define TreeView_SetItemHeight(hwnd,  iHeight) \
    (INT32)SendMessage32A((hwnd), TVM_SETITEMHEIGHT, (WPARAM32)iHeight, 0)
#define TreeView_GetItemHeight(hwnd) \
    (INT32)SendMessage32A((hwnd), TVM_GETITEMHEIGHT, 0, 0)

#define TreeView_SetBkColor(hwnd, clr) \
    (COLORREF)SendMessage32A((hwnd), TVM_SETBKCOLOR, 0, (LPARAM)clr)

#define TreeView_SetTextColor(hwnd, clr) \
    (COLORREF)SendMessage32A((hwnd), TVM_SETTEXTCOLOR, 0, (LPARAM)clr)

#define TreeView_GetBkColor(hwnd) \
    (COLORREF)SendMessage32A((hwnd), TVM_GETBKCOLOR, 0, 0)

#define TreeView_GetTextColor(hwnd) \
    (COLORREF)SendMessage32A((hwnd), TVM_GETTEXTCOLOR, 0, 0)

#define TreeView_SetScrollTime(hwnd, uTime) \
    (UINT32)SendMessage32A((hwnd), TVM_SETSCROLLTIME, uTime, 0)

#define TreeView_GetScrollTime(hwnd) \
    (UINT32)SendMessage32A((hwnd), TVM_GETSCROLLTIME, 0, 0)

#define TreeView_SetInsertMarkColor(hwnd, clr) \
    (COLORREF)SendMessage32A((hwnd), TVM_SETINSERTMARKCOLOR, 0, (LPARAM)clr)
#define TreeView_GetInsertMarkColor(hwnd) \
    (COLORREF)SendMessage32A((hwnd), TVM_GETINSERTMARKCOLOR, 0, 0)









/* Listview control */

#define WC_LISTVIEW32A          "SysListView32"
#define WC_LISTVIEW32W          L"SysListView32"
#define WC_LISTVIEW  WINELIB_NAME_AW(WC_LISTVIEW)

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

#define LVSIL_NORMAL            0
#define LVSIL_SMALL             1
#define LVSIL_STATE             2

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

#define LVNI_ALL		0x0000
#define LVNI_FOCUSED		0x0001
#define LVNI_SELECTED		0x0002
#define LVNI_CUT		0x0004
#define LVNI_DROPHILITED	0x0008

#define LVNI_ABOVE		0x0100
#define LVNI_BELOW		0x0200
#define LVNI_TOLEFT		0x0400
#define LVNI_TORIGHT		0x0800

#define LVHT_NOWHERE		0x0001
#define LVHT_ONITEMICON		0x0002
#define LVHT_ONITEMLABEL	0x0004
#define LVHT_ONITEMSTATEICON	0x0008
#define LVHT_ONITEM		(LVHT_ONITEMICON|LVHT_ONITEMLABEL|LVHT_ONITEMSTATEICON)

#define LVHT_ABOVE		0x0008
#define LVHT_BELOW		0x0010
#define LVHT_TORIGHT		0x0020
#define LVHT_TOLEFT		0x0040

#define LVM_FIRST               0x1000
#define LVM_GETBKCOLOR          (LVM_FIRST+0)
#define LVM_SETBKCOLOR          (LVM_FIRST+1)
#define LVM_GETIMAGELIST        (LVM_FIRST+2)
#define LVM_SETIMAGELIST        (LVM_FIRST+3)
#define LVM_GETITEMCOUNT        (LVM_FIRST+4)
#define LVM_GETITEM32A          (LVM_FIRST+5)
#define LVM_GETITEM32W          (LVM_FIRST+75)
#define LVM_GETITEM             WINELIB_NAME_AW(LVM_GETITEM)
#define LVM_SETITEM32A          (LVM_FIRST+6)
#define LVM_SETITEM32W          (LVM_FIRST+76)
#define LVM_SETITEM             WINELIB_NAME_AW(LVM_SETITEM)
#define LVM_INSERTITEM32A       (LVM_FIRST+7)
#define LVM_INSERTITEM32W       (LVM_FIRST+77)
#define LVM_INSERTITEM          WINELIB_NAME_AW(LVM_INSERTITEM)
#define LVM_DELETEITEM          (LVM_FIRST+8)
#define LVM_DELETEALLITEMS      (LVM_FIRST+9)
#define LVM_GETCALLBACKMASK     (LVM_FIRST+10)
#define LVM_SETCALLBACKMASK     (LVM_FIRST+11)
#define LVM_GETNEXTITEM         (LVM_FIRST+12)
#define LVM_FINDITEM32A         (LVM_FIRST+13)
#define LVM_FINDITEM32W         (LVM_FIRST+83)
#define LVM_FINDITEM            WINELIB_NAME_AW(LVM_FINDITEM)
#define LVM_GETITEMRECT         (LVM_FIRST+14)
#define LVM_SETITEMPOSITION     (LVM_FIRST+15)
#define LVM_GETITEMPOSITION     (LVM_FIRST+16)
#define LVM_GETSTRINGWIDTH32A   (LVM_FIRST+17)
#define LVM_GETSTRINGWIDTH32W   (LVM_FIRST+87)
#define LVM_GETSTRINGWIDTH      WINELIB_NAME_AW(LVM_GETSTRINGWIDTH)
#define LVM_HITTEST             (LVM_FIRST+18)
#define LVM_ENSUREVISIBLE       (LVM_FIRST+19)
#define LVM_SCROLL              (LVM_FIRST+20)
#define LVM_REDRAWITEMS         (LVM_FIRST+21)
#define LVM_ARRANGE             (LVM_FIRST+22)
#define LVM_EDITLABEL32A        (LVM_FIRST+23)
#define LVM_EDITLABEL32W        (LVM_FIRST+118)
#define LVM_EDITLABEL           WINELIB_NAME_AW(LVM_EDITLABEL)
#define LVM_GETEDITCONTROL      (LVM_FIRST+24)
#define LVM_GETCOLUMN32A        (LVM_FIRST+25)
#define LVM_GETCOLUMN32W        (LVM_FIRST+95)
#define LVM_GETCOLUMN           WINELIB_NAME_AW(LVM_GETCOLUMN)
#define LVM_SETCOLUMN32A        (LVM_FIRST+26)
#define LVM_SETCOLUMN32W        (LVM_FIRST+96)
#define LVM_SETCOLUMN           WINELIB_NAME_AW(LVM_SETCOLUMN)
#define LVM_INSERTCOLUMN32A     (LVM_FIRST+27)
#define LVM_INSERTCOLUMN32W     (LVM_FIRST+97)
#define LVM_INSERTCOLUMN        WINELIB_NAME_AW(LVM_INSERTCOLUMN)
#define LVM_DELETECOLUMN        (LVM_FIRST+28)
#define LVM_GETCOLUMNWIDTH      (LVM_FIRST+29)
#define LVM_SETCOLUMNWIDTH      (LVM_FIRST+30)
#define LVM_GETHEADER           (LVM_FIRST+31)

#define LVM_CREATEDRAGIMAGE     (LVM_FIRST+33)
#define LVM_GETVIEWRECT         (LVM_FIRST+34)
#define LVM_GETTEXTCOLOR        (LVM_FIRST+35)
#define LVM_SETTEXTCOLOR        (LVM_FIRST+36)
#define LVM_GETTEXTBKCOLOR      (LVM_FIRST+37)
#define LVM_SETTEXTBKCOLOR      (LVM_FIRST+38)
#define LVM_GETTOPINDEX         (LVM_FIRST+39)
#define LVM_GETCOUNTPERPAGE     (LVM_FIRST+40)
#define LVM_GETORIGIN           (LVM_FIRST+41)
#define LVM_UPDATE              (LVM_FIRST+42)
#define LVM_SETITEMSTATE        (LVM_FIRST+43)
#define LVM_GETITEMSTATE        (LVM_FIRST+44)
#define LVM_GETITEMTEXT32A      (LVM_FIRST+45)
#define LVM_GETITEMTEXT32W      (LVM_FIRST+115)
#define LVM_GETITEMTEXT         WINELIB_NAME_AW(LVM_GETITEMTEXT)
#define LVM_SETITEMTEXT32A      (LVM_FIRST+46)
#define LVM_SETITEMTEXT32W      (LVM_FIRST+116)
#define LVM_SETITEMTEXT         WINELIB_NAME_AW(LVM_SETITEMTEXT)
#define LVM_SETITEMCOUNT        (LVM_FIRST+47)
#define LVM_SORTITEMS           (LVM_FIRST+48)
#define LVM_SETITEMPOSITION32   (LVM_FIRST+49)
#define LVM_GETSELECTEDCOUNT    (LVM_FIRST+50)
#define LVM_GETITEMSPACING      (LVM_FIRST+51)
#define LVM_GETISEARCHSTRING32A (LVM_FIRST+52)
#define LVM_GETISEARCHSTRING32W (LVM_FIRST+117)
#define LVM_GETISEARCHSTRING    WINELIB_NAME_AW(LVM_GETISEARCHSTRING)
#define LVM_SETICONSPACING      (LVM_FIRST+53)
#define LVM_SETEXTENDEDLISTVIEWSTYLE (LVM_FIRST+54)
#define LVM_GETEXTENDEDLISTVIEWSTYLE (LVM_FIRST+55)
#define LVM_GETSUBITEMRECT      (LVM_FIRST+56)
#define LVM_SUBITEMHITTEST      (LVM_FIRST+57)
#define LVM_SETCOLUMNORDERARRAY (LVM_FIRST+58)
#define LVM_GETCOLUMNORDERARRAY (LVM_FIRST+59)
#define LVM_SETHOTITEM          (LVM_FIRST+60)
#define LVM_GETHOTITEM          (LVM_FIRST+61)
#define LVM_SETHOTCURSOR        (LVM_FIRST+62)
#define LVM_GETHOTCURSOR        (LVM_FIRST+63)
#define LVM_APPROXIMATEVIEWRECT (LVM_FIRST+64)
#define LVM_SETWORKAREAS        (LVM_FIRST+65)
#define LVM_GETSELECTIONMARK    (LVM_FIRST+66)
#define LVM_SETSELECTIONMARK    (LVM_FIRST+67)
#define LVM_SETBKIMAGE32A       (LVM_FIRST+68)
#define LVM_SETBKIMAGE32W       (LVM_FIRST+138)
#define LVM_SETBKIMAGE          WINELIB_NAME_AW(LVM_SETBKIMAGE)
#define LVM_GETBKIMAGE32A       (LVM_FIRST+69)
#define LVM_GETBKIMAGE32W       (LVM_FIRST+139)
#define LVM_GETBKIMAGE          WINELIB_NAME_AW(LVM_GETBKIMAGE)
#define LVM_GETWORKAREAS        (LVM_FIRST+70)
#define LVM_SETHOVERTIME        (LVM_FIRST+71)
#define LVM_GETHOVERTIME        (LVM_FIRST+72)
#define LVM_GETNUMBEROFWORKAREAS (LVM_FIRST+73)
#define LVM_SETTOOLTIPS         (LVM_FIRST+74)

#define LVM_GETTOOLTIPS         (LVM_FIRST+78)

#define LVN_FIRST               (0U-100U)
#define LVN_LAST                (0U-199U)
#define LVN_ITEMCHANGING        (LVN_FIRST-0)
#define LVN_ITEMCHANGED         (LVN_FIRST-1)
#define LVN_INSERTITEM          (LVN_FIRST-2)
#define LVN_DELETEITEM          (LVN_FIRST-3)
#define LVN_DELETEALLITEMS      (LVN_FIRST-4)
#define LVN_BEGINLABELEDIT32A   (LVN_FIRST-5)
#define LVN_BEGINLABELEDIT32W   (LVN_FIRST-75)
#define LVN_BEGINLABELEDIT WINELIB_NAME_AW(LVN_BEGINLABELEDIT)
#define LVN_ENDLABELEDIT32A     (LVN_FIRST-6)
#define LVN_ENDLABELEDIT32W     (LVN_FIRST-76)
#define LVN_ENDLABELEDIT WINELIB_NAME_AW(LVN_ENDLABELEDIT)
#define LVN_COLUMNCLICK         (LVN_FIRST-8)
#define LVN_BEGINDRAG           (LVN_FIRST-9)
#define LVN_BEGINRDRAG          (LVN_FIRST-11)
#define LVN_ODCACHEHINT         (LVN_FIRST-13)
#define LVN_ODFINDITEM32A       (LVN_FIRST-52)
#define LVN_ODFINDITEM32W       (LVN_FIRST-79)
#define LVN_ODFINDITEM WINELIB_NAME_AW(LVN_ODFINDITEM)
#define LVN_ITEMACTIVATE        (LVN_FIRST-14)
#define LVN_ODSTATECHANGED      (LVN_FIRST-15)
#define LVN_HOTTRACK            (LVN_FIRST-21)
#define LVN_GETDISPINFO32A      (LVN_FIRST-50)
#define LVN_GETDISPINFO32W      (LVN_FIRST-77)
#define LVN_GETDISPINFO WINELIB_NAME_AW(LVN_GETDISPINFO)
#define LVN_SETDISPINFO32A      (LVN_FIRST-51)
#define LVN_SETDISPINFO32W      (LVN_FIRST-78)
#define LVN_SETDISPINFO WINELIB_NAME_AW(LVN_SETDISPINFO)


typedef struct tagLVITEMA
{
    UINT32 mask;
    INT32  iItem;
    INT32  iSubItem;
    UINT32 state;
    UINT32 stateMask;
    LPSTR  pszText;
    INT32  cchTextMax;
    INT32  iImage;
    LPARAM lParam;
    INT32  iIndent;	/* (_WIN32_IE >= 0x0300) */
} LVITEM32A, *LPLVITEM32A;

typedef struct tagLVITEMW
{
    UINT32 mask;
    INT32  iItem;
    INT32  iSubItem;
    UINT32 state;
    UINT32 stateMask;
    LPWSTR pszText;
    INT32  cchTextMax;
    INT32  iImage;
    LPARAM lParam;
    INT32  iIndent;	/* (_WIN32_IE >= 0x0300) */
} LVITEM32W, *LPLVITEM32W;

#define LVITEM   WINELIB_NAME_AW(LVITEM)
#define LPLVITEM WINELIB_NAME_AW(LPLVITEM)

#define LVITEM_V1_SIZE32A CCSIZEOF_STRUCT(LVITEM32A, lParam)
#define LVITEM_V1_SIZE32W CCSIZEOF_STRUCT(LVITEM32W, lParam)
#define LVITEM_V1_SIZE WINELIB_NAME_AW(LVITEM_V1_SIZE)

#define LV_ITEM LVITEM

typedef struct tagLVCOLUMNA
{
    UINT32 mask;
    INT32  fmt;
    INT32  cx;
    LPSTR  pszText;
    INT32  cchTextMax;
    INT32  iSubItem;
    INT32  iImage;  /* (_WIN32_IE >= 0x0300) */
    INT32  iOrder;  /* (_WIN32_IE >= 0x0300) */
} LVCOLUMN32A, *LPLVCOLUMN32A;

typedef struct tagLVCOLUMNW
{
    UINT32 mask;
    INT32  fmt;
    INT32  cx;
    LPWSTR pszText;
    INT32  cchTextMax;
    INT32  iSubItem;
    INT32  iImage;	/* (_WIN32_IE >= 0x0300) */
    INT32  iOrder;	/* (_WIN32_IE >= 0x0300) */
} LVCOLUMN32W, *LPLVCOLUMN32W;

#define LVCOLUMN   WINELIB_NAME_AW(LVCOLUMN)
#define LPLVCOLUMN WINELIB_NAME_AW(LPLVCOLUMN)

#define LVCOLUMN_V1_SIZE32A CCSIZEOF_STRUCT(LVCOLUMN32A, iSubItem)
#define LVCOLUMN_V1_SIZE32W CCSIZEOF_STRUCT(LVCOLUMN32W, iSubItem)
#define LVCOLUMN_V1_SIZE WINELIB_NAME_AW(LVCOLUMN_V1_SIZE)

#define LV_COLUMN       LVCOLUMN


typedef struct tagNMLISTVIEW
{
    NMHDR   hdr;
    INT32   iItem;
    INT32   iSubItem;
    UINT32  uNewState;
    UINT32  uOldState;
    UINT32  uChanged;
    POINT32 ptAction;
    LPARAM  lParam;
} NMLISTVIEW, *LPNMLISTVIEW;

#define LPNM_LISTVIEW   LPNMLISTVIEW
#define NM_LISTVIEW     NMLISTVIEW


typedef struct tagLVDISPINFO
{
    NMHDR     hdr;
    LVITEM32A item;
} NMLVDISPINFO32A, *LPNMLVDISPINFO32A;

typedef struct tagLVDISPINFOW
{
    NMHDR     hdr;
    LVITEM32W item;
} NMLVDISPINFO32W, *LPNMLVDISPINFO32W;

#define NMLVDISPINFO   WINELIB_NAME_AW(NMLVDISPINFO)
#define LPNMLVDISPINFO WINELIB_NAME_AW(LPNMLVDISPINFO)

#define LV_DISPINFO     NMLVDISPINFO


typedef struct tagLVHITTESTINFO
{
    POINT32 pt;
    UINT32  flags;
    INT32   iItem;
    INT32   iSubItem;
} LVHITTESTINFO, *LPLVHITTESTINFO;

#define LV_HITTESTINFO LVHITTESTINFO
#define _LV_HITTESTINFO tagLVHITTESTINFO
#define LVHITTESTINFO_V1_SIZE CCSIZEOF_STRUCT(LVHITTESTINFO,iItem)

typedef struct tagLVFINDINFO
{
	UINT32 flags;
	LPCSTR psz;
	LPARAM lParam;
	POINT32 pt;
	UINT32 vkDirection;
} LVFINDINFO, *LPLVFINDINFO;

#define LV_FINDINFO LVFINDINFO

typedef struct tagTCHITTESTINFO
{
	POINT32 pt;
	UINT32 flags;
} TCHITTESTINFO, *LPTCHITTESTINFO;

#define TC_HITTESTINFO TCHITTESTINFO

typedef INT32 (CALLBACK *PFNLVCOMPARE)(LPARAM, LPARAM, LPARAM);

#define ListView_SetBkColor(hwnd,clrBk) \
    (BOOL32)SendMessage32A((hwnd),LVM_SETBKCOLOR,0,(LPARAM)(COLORREF)(clrBk))
#define ListView_GetImageList(hwnd,iImageList) \
    (HIMAGELIST)SendMessage32A((hwnd),LVM_GETIMAGELIST,(WPARAM)(INT32)(iImageList),0L)
#define ListView_SetImageList(hwnd,himl,iImageList) \
    (HIMAGELIST)(UINT32)SendMessage32A((hwnd),LVM_SETIMAGELIST,(WPARAM32)(iImageList),(LPARAM)(UINT32)(HIMAGELIST)(himl))
#define ListView_GetItemCount(hwnd) \
    (INT32)SendMessage32A((hwnd),LVM_GETITEMCOUNT,0,0L)
#define ListView_GetItem32A(hwnd,pitem) \
    (BOOL32)SendMessage32A((hwnd),LVM_GETITEM32A,0,(LPARAM)(LVITEM32A *)(pitem))
#define ListView_GetItem32W(hwnd,pitem) \
    (BOOL32)SendMessage32W((hwnd),LVM_GETITEM32W,0,(LPARAM)(LVITEM32W *)(pitem))
#define ListView_GetItem WINELIB_NAME_AW(ListView_GetItem)
#define ListView_HitTest(hwnd,pinfo) \
    (INT32)SendMessage32A((hwnd),LVMHITTEST,0,(LPARAM)(LPLVHITTESTINFO)(pinfo))
#define ListView_InsertItem32A(hwnd,pitem) \
    (INT32)SendMessage32A((hwnd),LVM_INSERTITEM32A,0,(LPARAM)(const LVITEM32A *)(pitem))
#define ListView_InsertItem32W(hwnd,pitem) \
    (INT32)SendMessage32W((hwnd),LVM_INSERTITEM32W,0,(LPARAM)(const LVITEM32W *)(pitem))
#define ListView_InsertItem WINELIB_NAME_AW(ListView_InsertItem)
#define ListView_DeleteAllItems(hwnd) \
    (BOOL32)SendMessage32A((hwnd),LVM_DELETEALLITEMS,0,0L)
#define ListView_InsertColumn32A(hwnd,iCol,pcol) \
    (INT32)SendMessage32A((hwnd),LVM_INSERTCOLUMN32A,(WPARAM32)(INT32)(iCol),(LPARAM)(const LVCOLUMN32A *)(pcol))
#define ListView_InsertColumn32W(hwnd,iCol,pcol) \
    (INT32)SendMessage32W((hwnd),LVM_INSERTCOLUMN32W,(WPARAM32)(INT32)(iCol),(LPARAM)(const LVCOLUMN32W *)(pcol))
#define ListView_InsertColumn WINELIB_NAME_AW(ListView_InsertColumn)
#define ListView_SortItems(hwndLV,_pfnCompare,_lPrm) \
    (BOOL32)SendMessage32A((hwndLV),LVM_SORTITEMS,(WPARAM32)(LPARAM)_lPrm,(LPARAM)(PFNLVCOMPARE)_pfnCompare)
#define ListView_SetItemPosition(hwndLV, i, x, y) \
    (BOOL32)SendMessage32A((hwndLV),LVM_SETITEMPOSITION,(WPARAM32)(INT32)(i),MAKELPARAM((x),(y)))
#define ListView_GetSelectedCount(hwndLV) \
    (UINT32)SendMessage32A((hwndLV),LVM_GETSELECTEDCOUNT,0,0L)


/* Tab Control */

#define WC_TABCONTROL16		 "SysTabControl"
#define WC_TABCONTROL32A	 "SysTabControl32"
#define WC_TABCONTROL32W	L"SysTabControl32"

#define WC_TABCONTROL		WINELIB_NAME_AW(WC_TABCONTROL)

/* tab control styles */
#define TCS_SCROLLOPPOSITE      0x0001   /* assumes multiline tab */
#define TCS_BOTTOM              0x0002
#define TCS_RIGHT               0x0002
#define TCS_MULTISELECT         0x0004  /* allow multi-select in button mode */
#define TCS_FLATBUTTONS         0x0008
#define TCS_FORCEICONLEFT       0x0010
#define TCS_FORCELABELLEFT      0x0020
#define TCS_HOTTRACK            0x0040
#define TCS_VERTICAL            0x0080
#define TCS_TABS                0x0000
#define TCS_BUTTONS             0x0100
#define TCS_SINGLELINE          0x0000
#define TCS_MULTILINE           0x0200
#define TCS_RIGHTJUSTIFY        0x0000
#define TCS_FIXEDWIDTH          0x0400
#define TCS_RAGGEDRIGHT         0x0800
#define TCS_FOCUSONBUTTONDOWN   0x1000
#define TCS_OWNERDRAWFIXED      0x2000
#define TCS_TOOLTIPS            0x4000
#define TCS_FOCUSNEVER          0x8000
#define TCS_EX_FLATSEPARATORS   0x00000001  /* to be used with */
#define TCS_EX_REGISTERDROP     0x00000002  /* TCM_SETEXTENDEDSTYLE */


#define TCM_FIRST		0x1300

#define TCM_GETIMAGELIST        (TCM_FIRST + 2)
#define TCM_SETIMAGELIST        (TCM_FIRST + 3)
#define TCM_GETITEMCOUNT	(TCM_FIRST + 4)
#define TCM_GETITEM				WINELIB_NAME_AW(TCM_GETITEM)
#define TCM_GETITEM32A			(TCM_FIRST + 5)
#define TCM_GETITEM32W			(TCM_FIRST + 60)
#define TCM_SETITEM32A			(TCM_FIRST + 6)
#define TCM_SETITEM32W			(TCM_FIRST + 61)
#define TCM_SETITEM				WINELIB_NAME_AW(TCM_SETITEM)
#define TCM_INSERTITEM32A		(TCM_FIRST + 7)
#define TCM_INSERTITEM32W		(TCM_FIRST + 62)
#define TCM_INSERTITEM			WINELIB_NAME_AW(TCM_INSERTITEM)
#define TCM_DELETEITEM          (TCM_FIRST + 8)
#define TCM_DELETEALLITEMS      (TCM_FIRST + 9)
#define TCM_GETITEMRECT         (TCM_FIRST + 10)
#define TCM_GETCURSEL		(TCM_FIRST + 11)
#define TCM_SETCURSEL           (TCM_FIRST + 12)
#define TCM_HITTEST             (TCM_FIRST + 13)
#define TCM_SETITEMEXTRA	(TCM_FIRST + 14)
#define TCM_ADJUSTRECT          (TCM_FIRST + 40)
#define TCM_SETITEMSIZE         (TCM_FIRST + 41)
#define TCM_REMOVEIMAGE         (TCM_FIRST + 42)
#define TCM_SETPADDING          (TCM_FIRST + 43)
#define TCM_GETROWCOUNT         (TCM_FIRST + 44)
#define TCM_GETTOOLTIPS         (TCM_FIRST + 45)
#define TCM_SETTOOLTIPS         (TCM_FIRST + 46)
#define TCM_GETCURFOCUS         (TCM_FIRST + 47)
#define TCM_SETCURFOCUS         (TCM_FIRST + 48)
#define TCM_SETMINTTABWIDTH     (TCM_FIRST + 49)
#define TCM_DESELECTALL         (TCM_FIRST + 50)
#define TCM_HIGHLIGHTITEM		(TCM_FIRST + 51)
#define TCM_SETEXTENDEDSTYLE	(TCM_FIRST + 52)
#define TCM_GETEXTENDEDSTYLE	(TCM_FIRST + 53)
#define TCM_SETUNICODEFORMAT	CCM_SETUNICODEFORMAT
#define TCM_GETUNICODEFORMAT	CCM_GETUNICODEFORMAT


#define TCIF_TEXT		0x0001
#define TCIF_IMAGE		0x0002
#define TCIF_RTLREADING		0x0004
#define TCIF_PARAM		0x0008
#define TCIF_STATE		0x0010

#define TCIS_BUTTONPRESSED      0x0001
#define TCIS_HIGHLIGHTED 0x0002


/* constants for TCHITTESTINFO */

#define TCHT_NOWHERE      0x01
#define TCHT_ONITEMICON   0x02
#define TCHT_ONITEMLABEL  0x04
#define TCHT_ONITEM       (TCHT_ONITEMICON | TCHT_ONITEMLABEL)


typedef struct tagTCITEM32A {
    UINT32 mask;
    UINT32 dwState;
    UINT32 dwStateMask;
    LPSTR  pszText;
    INT32  cchTextMax;
    INT32  iImage;
    LPARAM lParam;
} TCITEM32A, *LPTCITEM32A;

typedef struct tagTCITEM32W
{
    UINT32 mask;
    DWORD  dwState;
    DWORD  dwStateMask;
    LPWSTR pszText;
    INT32  cchTextMax;
    INT32  iImage;
    LPARAM lParam;
} TCITEM32W, *LPTCITEM32W;

#define TCITEM   WINELIB_NAME_AW(TCITEM)
#define LPTCITEM WINELIB_NAME_AW(LPTCITEM)
#define TC_ITEM  TCITEM

#define TCN_FIRST               (0U-550U)
#define TCN_LAST                (0U-580U)
#define TCN_KEYDOWN             (TCN_FIRST - 0)
#define TCN_SELCHANGE		(TCN_FIRST - 1)
#define TCN_SELCHANGING         (TCN_FIRST - 2)
#define TCN_GETOBJECT      (TCN_FIRST - 3)


/* ComboBoxEx control */

#define WC_COMBOBOXEX32A        "ComboBoxEx32"
#define WC_COMBOBOXEX32W        L"ComboBoxEx32"
#define WC_COMBOBOXEX           WINELIB_NAME_AW(WC_COMBOBOXEX)

#define CBEM_INSERTITEM32A      (WM_USER+1)
#define CBEM_INSERTITEM32W      (WM_USER+11)
#define CBEM_INSERTITEM         WINELIB_NAME_AW(CBEM_INSERTITEM)
#define CBEM_SETIMAGELIST       (WM_USER+2)
#define CBEM_GETIMAGELIST       (WM_USER+3)
#define CBEM_GETITEM32A         (WM_USER+4)
#define CBEM_GETITEM32W         (WM_USER+13)
#define CBEM_GETITEM            WINELIB_NAME_AW(CBEM_GETITEM)
#define CBEM_SETITEM32A         (WM_USER+5)
#define CBEM_SETITEM32W         (WM_USER+12)
#define CBEM_SETITEM            WINELIB_NAME_AW(CBEM_SETITEM)
#define CBEM_GETCOMBOCONTROL    (WM_USER+6)
#define CBEM_GETEDITCONTROL     (WM_USER+7)
#define CBEM_SETEXSTYLE         (WM_USER+8)
#define CBEM_GETEXSTYLE         (WM_USER+9)
#define CBEM_GETEXTENDEDSTYLE   (WM_USER+9)
#define CBEM_SETEXTENDEDSTYLE   (WM_USER+14)
#define CBEM_HASEDITCHANGED     (WM_USER+10)
#define CBEM_SETUNICODEFORMAT   CCM_SETUNICODEFORMAT
#define CBEM_GETUNICODEFORMAT   CCM_GETUNICODEFORMAT

#define CBEN_FIRST              (0U-800U)
#define CBEN_LAST               (0U-830U)


/* Hotkey control */

#define HOTKEY_CLASS16          "msctls_hotkey"
#define HOTKEY_CLASS32A         "msctls_hotkey32"
#define HOTKEY_CLASS32W         L"msctls_hotkey32"
#define HOTKEY_CLASS            WINELIB_NAME_AW(HOTKEY_CLASS)

#define HOTKEYF_SHIFT           0x01
#define HOTKEYF_CONTROL         0x02
#define HOTKEYF_ALT             0x04
#define HOTKEYF_EXT             0x08

#define HKCOMB_NONE             0x0001
#define HKCOMB_S                0x0002
#define HKCOMB_C                0x0004
#define HKCOMB_A                0x0008
#define HKCOMB_SC               0x0010
#define HKCOMB_SA               0x0020
#define HKCOMB_CA               0x0040
#define HKCOMB_SCA              0x0080

#define HKM_SETHOTKEY           (WM_USER+1)
#define HKM_GETHOTKEY           (WM_USER+2)
#define HKM_SETRULES            (WM_USER+3)


/* animate control */

#define ANIMATE_CLASS32A        "SysAnimate32"
#define ANIMATE_CLASS32W        L"SysAnimate32"
#define ANIMATE_CLASS           WINELIB_NAME_AW(ANIMATE_CLASS)

#define ACS_CENTER              0x0001
#define ACS_TRANSPARENT         0x0002
#define ACS_AUTOPLAY            0x0004
#define ACS_TIMER               0x0008  /* no threads, just timers */

#define ACM_OPEN32A             (WM_USER+100)
#define ACM_OPEN32W             (WM_USER+103)
#define ACM_OPEN                WINELIB_NAME_AW(ACM_OPEN)
#define ACM_PLAY                (WM_USER+101)
#define ACM_STOP                (WM_USER+102)

#define ACN_START               1
#define ACN_STOP                2

#define Animate_Create32A(hwndP,id,dwStyle,hInstance) \
    CreateWindow32A(ANIMATE_CLASS32A,NULL,dwStyle,0,0,0,0,hwndP,(HMENU32)(id),hInstance,NULL)
#define Animate_Create32W(hwndP,id,dwStyle,hInstance) \
    CreateWindow32W(ANIMATE_CLASS32W,NULL,dwStyle,0,0,0,0,hwndP,(HMENU32)(id),hInstance,NULL)
#define Animate_Create WINELIB_NAME_AW(Animate_Create)
#define Animate_Open32A(hwnd,szName) \
    (BOOL32)SendMessage32A(hwnd,ACM_OPEN32A,0,(LPARAM)(LPSTR)(szName))
#define Animate_Open32W(hwnd,szName) \
    (BOOL32)SendMessage32W(hwnd,ACM_OPEN32W,0,(LPARAM)(LPWSTR)(szName))
#define Animate_Open WINELIB_NAME_AW(Animate_Open)
#define Animate_OpenEx32A(hwnd,hInst,szName) \
    (BOOL32)SendMessage32A(hwnd,ACM_OPEN32A,(WPARAM32)hInst,(LPARAM)(LPSTR)(szName))
#define Animate_OpenEx32W(hwnd,hInst,szName) \
    (BOOL32)SendMessage32W(hwnd,ACM_OPEN32W,(WPARAM32)hInst,(LPARAM)(LPWSTR)(szName))
#define Animate_OpenEx WINELIB_NAME_AW(Animate_OpenEx)
#define Animate_Play(hwnd,from,to,rep) \
    (BOOL32)SendMessage32A(hwnd,ACM_PLAY,(WPARAM32)(UINT32)(rep),(LPARAM)MAKELONG(from,to))
#define Animate_Stop(hwnd) \
    (BOOL32)SendMessage32A(hwnd,ACM_STOP,0,0)
#define Animate_Close(hwnd) \
    (BOOL32)SendMessage32A(hwnd,ACM_OPEN32A,0,0)
#define Animate_Seek(hwnd,frame) \
    (BOOL32)SendMessage32A(hwnd,ACM_PLAY,1,(LPARAM)MAKELONG(frame,frame))


/**************************************************************************
 * IP Address control
 */

#define WC_IPADDRESS32A		"SysIPAddress32"
#define WC_IPADDRESS32W		L"SysIPAddress32"
#define WC_IPADDRESS		WINELIB_NAME_AW(WC_IPADDRESS)

#define IPM_CLEARADDRESS	(WM_USER+100)
#define IPM_SETADDRESS		(WM_USER+101)
#define IPM_GETADDRESS		(WM_USER+102)
#define IPM_SETRANGE		(WM_USER+103)
#define IPM_SETFOCUS		(WM_USER+104)
#define IPM_ISBLANK		(WM_USER+105)

#define IPN_FIRST               (0U-860U)
#define IPN_LAST                (0U-879U)
#define IPN_FIELDCHANGED    (IPN_FIRST-0)

typedef struct tagNMIPADDRESS
{
    NMHDR hdr;
    INT32 iField;
    INT32 iValue;
} NMIPADDRESS, *LPNMIPADDRESS;

#define MAKEIPRANGE(low,high) \
    ((LPARAM)(WORD)(((BYTE)(high)<<8)+(BYTE)(low)))
#define MAKEIPADDRESS(b1,b2,b3,b4) \
    ((LPARAM)(((DWORD)(b1)<<24)+((DWORD)(b2)<16)+((DWORD)(b3)<<8)+((DWORD)(b4))))

#define FIRST_IPADDRESS(x)	(((x)>>24)&0xff)
#define SECOND_IPADDRESS(x)	(((x)>>16)&0xff)
#define THIRD_IPADDRESS(x)	(((x)>>8)&0xff)
#define FOURTH_IPADDRESS(x)	((x)&0xff)


/**************************************************************************
 * Native Font control
 */

#define WC_NATIVEFONTCTL32A	"NativeFontCtl"
#define WC_NATIVEFONTCTL32W	L"NativeFontCtl"
#define WC_NATIVEFONTCTL	WINELIB_NAME_AW(WC_NATIVEFONTCTL)

#define NFS_EDIT		0x0001
#define NFS_STATIC		0x0002
#define NFS_LISTCOMBO		0x0004
#define NFS_BUTTON		0x0008
#define NFS_ALL			0x0010


/**************************************************************************
 * Month calendar control
 */

#define MONTHCAL_CLASS32A	"SysMonthCal32"
#define MONTHCAL_CLASS32W	L"SysMonthCal32"
#define MONTHCAL_CLASS		WINELIB_NAME_AW(MONTHCAL_CLASS)


/**************************************************************************
 * Date and time picker control
 */

#define DATETIMEPICK_CLASS32A	"SysDateTimePick32"
#define DATETIMEPICK_CLASS32W	L"SysDateTimePick32"
#define DATETIMEPICK_CLASS	WINELIB_NAME_AW(DATETIMEPICK_CLASS)

#define DTM_FIRST        0x1000

#define DTM_GETSYSTEMTIME	(DTM_FIRST+1)
#define DTM_SETSYSTEMTIME	(DTM_FIRST+2)
#define DTM_GETRANGE		(DTM_FIRST+3)
#define DTM_SETRANGE		(DTM_FIRST+4)
#define DTM_SETFORMAT32A	(DTM_FIRST+5)
#define DTM_SETFORMAT32W	(DTM_FIRST + 50)
#define DTM_SETFORMAT		WINELIB_NAME_AW(DTM_SETFORMAT)
#define DTM_SETMCCOLOR		(DTM_FIRST+6)
#define DTM_GETMCCOLOR		(DTM_FIRST+7)

#define DTM_GETMONTHCAL		(DTM_FIRST+8)

#define DTM_SETMCFONT		(DTM_FIRST+9)
#define DTM_GETMCFONT		(DTM_FIRST+10)




#define GDT_ERROR    -1
#define GDT_VALID    0
#define GDT_NONE     1

#define GDTR_MIN     0x0001
#define GDTR_MAX     0x0002





/*
 * Property sheet support (callback procs)
 */


#define WC_PROPSHEET32A      "SysPropertySheet"
#define WC_PROPSHEET32W      L"SysPropertySheet"
#define WC_PROPSHEET         WINELIB_NAME_AW(WC_PROPSHEET)

struct _PROPSHEETPAGE32A;  /** need to forward declare those structs **/
struct _PROPSHEETPAGE32W;
struct _PSP;
typedef struct _PSP *HPROPSHEETPAGE;


typedef UINT32 (CALLBACK *LPFNPSPCALLBACK32A)(HWND32, UINT32, struct _PROPSHEETPAGE32A*);
typedef UINT32 (CALLBACK *LPFNPSPCALLBACK32W)(HWND32, UINT32, struct _PROPSHEETPAGE32W*);
typedef INT32  (CALLBACK *PFNPROPSHEETCALLBACK32)(HWND32, UINT32, LPARAM);
typedef BOOL32 (CALLBACK *LPFNADDPROPSHEETPAGE)(HPROPSHEETPAGE, LPARAM);
typedef BOOL32 (CALLBACK *LPFNADDPROPSHEETPAGES)(LPVOID, LPFNADDPROPSHEETPAGE, LPARAM);

/* c++ likes nameless unions whereas c doesnt */
/* (used in property sheet structures)        */
#ifdef __cplusplus
#define DUMMYUNIONNAME1
#define DUMMYUNIONNAME2
#define DUMMYUNIONNAME3
#define DUMMYUNIONNAME4
#define DUMMYUNIONNAME5
#else
#define DUMMYUNIONNAME1  u1
#define DUMMYUNIONNAME2  u2
#define DUMMYUNIONNAME3  u3
#define DUMMYUNIONNAME4  u4
#define DUMMYUNIONNAME5  u5
#endif

/*
 * Property sheet support (structures)
 */
typedef struct _PROPSHEETPAGE32A
{
    DWORD              dwSize;
    DWORD              dwFlags;
    HINSTANCE32        hInstance;
    union 
    {
        LPCSTR           pszTemplate;
        LPCDLGTEMPLATE   pResource;
    }DUMMYUNIONNAME1;
    union
    {
        HICON32          hIcon;
        LPCSTR           pszIcon;
    }DUMMYUNIONNAME2;
    LPCSTR             pszTitle;
    DLGPROC32          pfnDlgProc;
    LPARAM             lParam;
    LPFNPSPCALLBACK32A pfnCallback;
    UINT32*            pcRefParent;
    LPCWSTR            pszHeaderTitle;
    LPCWSTR            pszHeaderSubTitle;
} PROPSHEETPAGE32A, *LPPROPSHEETPAGE32A;

typedef const PROPSHEETPAGE32A *LPCPROPSHEETPAGE32A;

typedef struct _PROPSHEETPAGE32W
{
    DWORD               dwSize;
    DWORD               dwFlags;
    HINSTANCE32         hInstance;
    union 
    {
        LPCWSTR          pszTemplate;
        LPCDLGTEMPLATE   pResource;
    }DUMMYUNIONNAME1;
    union
    {
        HICON32          hIcon;
        LPCWSTR          pszIcon;
    }DUMMYUNIONNAME2;
    LPCWSTR            pszTitle;
    DLGPROC32          pfnDlgProc;
    LPARAM             lParam;
    LPFNPSPCALLBACK32W pfnCallback;
    UINT32*            pcRefParent;
    LPCWSTR            pszHeaderTitle;
    LPCWSTR            pszHeaderSubTitle;
} PROPSHEETPAGE32W, *LPPROPSHEETPAGE32W;

typedef const PROPSHEETPAGE32W *LPCPROPSHEETPAGE32W;


typedef struct _PROPSHEETHEADER32A
{
    DWORD                    dwSize;
    DWORD                    dwFlags;
    HWND32                   hwndParent;
    HINSTANCE32              hInstance;
    union
    {
      HICON32                  hIcon;
      LPCSTR                   pszIcon;
    }DUMMYUNIONNAME1;
    LPCSTR                   pszCaption;
    UINT32                   nPages;
    union
    {
        UINT32                 nStartPage;
        LPCSTR                 pStartPage;
    }DUMMYUNIONNAME2;
    union
    {
        LPCPROPSHEETPAGE32A    ppsp;
        HPROPSHEETPAGE*        phpage;
    }DUMMYUNIONNAME3;
    PFNPROPSHEETCALLBACK32   pfnCallback;
    union
    {
        HBITMAP32              hbmWatermark;
        LPCSTR                 pszbmWatermark;
    }DUMMYUNIONNAME4;
    HPALETTE32               hplWatermark;
    union
    {
        HBITMAP32              hbmHeader;
        LPCSTR                 pszbmHeader;
    }DUMMYUNIONNAME5;
} PROPSHEETHEADER32A, *LPPROPSHEETHEADER32A;

typedef const PROPSHEETHEADER32A *LPCPROPSHEETHEADER32A;

typedef struct _PROPSHEETHEADER32W
{
    DWORD                    dwSize;
    DWORD                    dwFlags;
    HWND32                   hwndParent;
    HINSTANCE32              hInstance;
    union
    {
      HICON32                  hIcon;
      LPCSTR                   pszIcon;
    }DUMMYUNIONNAME1;
    LPCWSTR                  pszCaption;
    UINT32                   nPages;
    union
    {
        UINT32                 nStartPage;
        LPCWSTR                pStartPage;
    }DUMMYUNIONNAME2;
    union
    {
        LPCPROPSHEETPAGE32W    ppsp;
        HPROPSHEETPAGE*        phpage;
    }DUMMYUNIONNAME3;
    PFNPROPSHEETCALLBACK32   pfnCallback;
    union
    {
        HBITMAP32              hbmWatermark;
        LPCWSTR                pszbmWatermark;
    }DUMMYUNIONNAME4;
    HPALETTE32               hplWatermark;
    union
    {
        HBITMAP32              hbmHeader;
        LPCWSTR                pszbmHeader;
    }DUMMYUNIONNAME5;
} PROPSHEETHEADER32W, *LPPROPSHEETHEADER32W;

typedef const PROPSHEETHEADER32W *LPCPROPSHEETHEADER32W;


/*
 * Property sheet support (methods)
 */
INT32 WINAPI PropertySheet32A(LPCPROPSHEETHEADER32A);
INT32 WINAPI PropertySheet32W(LPCPROPSHEETHEADER32W);
#define PropertySheet WINELIB_NAME_AW(PropertySheet)
HPROPSHEETPAGE WINAPI CreatePropertySheetPage32A(LPCPROPSHEETPAGE32A);
HPROPSHEETPAGE WINAPI CreatePropertySheetPage32W(LPCPROPSHEETPAGE32W);
#define CreatePropertySheetPage WINELIB_NAME_AW(CreatePropertySheetPage)
BOOL32 WINAPI DestroyPropertySheetPage32(HPROPSHEETPAGE hPropPage);
#define DestroyPropertySheetPage WINELIB_NAME(DestroyPropertySheetPage)

/*
 * Property sheet support (UNICODE-WineLib)
 */

DECL_WINELIB_TYPE_AW(PROPSHEETPAGE) 
DECL_WINELIB_TYPE_AW(LPPROPSHEETPAGE) 
DECL_WINELIB_TYPE_AW(LPCPROPSHEETPAGE) 
DECL_WINELIB_TYPE_AW(PROPSHEETHEADER) 
DECL_WINELIB_TYPE_AW(LPPROPSHEETHEADER) 
DECL_WINELIB_TYPE_AW(LPCPROPSHEETHEADER) 
DECL_WINELIB_TYPE_AW(LPFNPSPCALLBACK) 
DECL_WINELIB_TYPE(PFNPROPSHEETCALLBACK) 


/*
 * Property sheet support (defines)
 */
#define PSP_DEFAULT             0x0000
#define PSP_DLGINDIRECT         0x0001
#define PSP_USEHICON            0x0002
#define PSP_USEICONID           0x0004
#define PSP_USETITLE            0x0008
#define PSP_RTLREADING          0x0010

#define PSP_HASHELP             0x0020
#define PSP_USEREFPARENT        0x0040
#define PSP_USECALLBACK         0x0080


#define PSPCB_RELEASE           1
#define PSPCB_CREATE            2

#define PSH_DEFAULT             0x0000
#define PSH_PROPTITLE           0x0001
#define PSH_USEHICON            0x0002
#define PSH_USEICONID           0x0004
#define PSH_PROPSHEETPAGE       0x0008
#define PSH_WIZARD              0x0020
#define PSH_USEPSTARTPAGE       0x0040
#define PSH_NOAPPLYNOW          0x0080
#define PSH_USECALLBACK         0x0100
#define PSH_HASHELP             0x0200
#define PSH_MODELESS            0x0400
#define PSH_RTLREADING          0x0800

#define PSCB_INITIALIZED  1
#define PSCB_PRECREATE    2

#define PSN_FIRST               (0U-200U)
#define PSN_LAST                (0U-299U)


#define PSN_SETACTIVE           (PSN_FIRST-0)
#define PSN_KILLACTIVE          (PSN_FIRST-1)
/* #define PSN_VALIDATE            (PSN_FIRST-1) */
#define PSN_APPLY               (PSN_FIRST-2)
#define PSN_RESET               (PSN_FIRST-3)
/* #define PSN_CANCEL              (PSN_FIRST-3) */
#define PSN_HELP                (PSN_FIRST-5)
#define PSN_WIZBACK             (PSN_FIRST-6)
#define PSN_WIZNEXT             (PSN_FIRST-7)
#define PSN_WIZFINISH           (PSN_FIRST-8)
#define PSN_QUERYCANCEL         (PSN_FIRST-9)

#define PSNRET_NOERROR              0
#define PSNRET_INVALID              1
#define PSNRET_INVALID_NOCHANGEPAGE 2
 

#define PSM_SETCURSEL           (WM_USER + 101)
#define PSM_REMOVEPAGE          (WM_USER + 102)
#define PSM_ADDPAGE             (WM_USER + 103)
#define PSM_CHANGED             (WM_USER + 104)
#define PSM_RESTARTWINDOWS      (WM_USER + 105)
#define PSM_REBOOTSYSTEM        (WM_USER + 106)
#define PSM_CANCELTOCLOSE       (WM_USER + 107)
#define PSM_QUERYSIBLINGS       (WM_USER + 108)
#define PSM_UNCHANGED           (WM_USER + 109)
#define PSM_APPLY               (WM_USER + 110)
#define PSM_SETTITLE32A         (WM_USER + 111)
#define PSM_SETTITLE32W         (WM_USER + 120)
#define PSM_SETTITLE WINELIB_NAME_AW(PSM_SETTITLE)
#define PSM_SETWIZBUTTONS       (WM_USER + 112)
#define PSM_PRESSBUTTON         (WM_USER + 113)
#define PSM_SETCURSELID         (WM_USER + 114)
#define PSM_SETFINISHTEXT32A    (WM_USER + 115)
#define PSM_SETFINISHTEXT32W    (WM_USER + 121)
#define PSM_SETFINISHTEXT WINELIB_NAME_AW(PSM_SETFINISHTEXT)
#define PSM_GETTABCONTROL       (WM_USER + 116)
#define PSM_ISDIALOGMESSAGE     (WM_USER + 117)
#define PSM_GETCURRENTPAGEHWND  (WM_USER + 118)

#define PSWIZB_BACK             0x00000001
#define PSWIZB_NEXT             0x00000002
#define PSWIZB_FINISH           0x00000004
#define PSWIZB_DISABLEDFINISH   0x00000008

#define PSBTN_BACK              0
#define PSBTN_NEXT              1
#define PSBTN_FINISH            2
#define PSBTN_OK                3
#define PSBTN_APPLYNOW          4
#define PSBTN_CANCEL            5
#define PSBTN_HELP              6
#define PSBTN_MAX               6

#define ID_PSRESTARTWINDOWS     0x2
#define ID_PSREBOOTSYSTEM       (ID_PSRESTARTWINDOWS | 0x1)


#define WIZ_CXDLG               276
#define WIZ_CYDLG               140

#define WIZ_CXBMP               80

#define WIZ_BODYX               92
#define WIZ_BODYCX              184

#define PROP_SM_CXDLG           212
#define PROP_SM_CYDLG           188

#define PROP_MED_CXDLG          227
#define PROP_MED_CYDLG          215

#define PROP_LG_CXDLG           252
#define PROP_LG_CYDLG           218

/*
 * Property sheet support (macros)
 */

#define PropSheet_SetCurSel(hDlg, hpage, index) \
	SeandMessage32A(hDlg, PSM_SETCURSEL, (WPARAM32)index, (LPARAM)hpage)
	 
#define PropSheet_RemovePage(hDlg, index, hpage) \
	SNDMSG(hDlg, PSM_REMOVEPAGE, index, (LPARAM)hpage)
	 
#define PropSheet_AddPage(hDlg, hpage) \
	SNDMSG(hDlg, PSM_ADDPAGE, 0, (LPARAM)hpage)
	 
#define PropSheet_Changed(hDlg, hwnd) \
	SNDMSG(hDlg, PSM_CHANGED, (WPARAM)hwnd, 0L)
	 
#define PropSheet_RestartWindows(hDlg) \
	SNDMSG(hDlg, PSM_RESTARTWINDOWS, 0, 0L)
	 
#define PropSheet_RebootSystem(hDlg) \
	SNDMSG(hDlg, PSM_REBOOTSYSTEM, 0, 0L)
	 
#define PropSheet_CancelToClose(hDlg) \
	PostMessage(hDlg, PSM_CANCELTOCLOSE, 0, 0L)
	 
#define PropSheet_QuerySiblings(hDlg, wParam, lParam) \
	SNDMSG(hDlg, PSM_QUERYSIBLINGS, wParam, lParam)
	 
#define PropSheet_UnChanged(hDlg, hwnd) \
	SNDMSG(hDlg, PSM_UNCHANGED, (WPARAM)hwnd, 0L)
	 
#define PropSheet_Apply(hDlg) \
	SNDMSG(hDlg, PSM_APPLY, 0, 0L)
	  
#define PropSheet_SetTitle(hDlg, wStyle, lpszText)\
	SNDMSG(hDlg, PSM_SETTITLE, wStyle, (LPARAM)(LPCTSTR)lpszText)
	 
#define PropSheet_SetWizButtons(hDlg, dwFlags) \
	PostMessage(hDlg, PSM_SETWIZBUTTONS, 0, (LPARAM)dwFlags)
	 
#define PropSheet_PressButton(hDlg, iButton) \
	PostMessage(hDlg, PSM_PRESSBUTTON, (WPARAM)iButton, 0)
	 
#define PropSheet_SetCurSelByID(hDlg, id) \
	SNDMSG(hDlg, PSM_SETCURSELID, 0, (LPARAM)id)

#define PropSheet_SetFinishText(hDlg, lpszText) \
	SNDMSG(hDlg, PSM_SETFINISHTEXT, 0, (LPARAM)lpszText)
	 
#define PropSheet_GetTabControl(hDlg) \
	(HWND)SNDMSG(hDlg, PSM_GETTABCONTROL, 0, 0)
	 
#define PropSheet_IsDialogMessage(hDlg, pMsg) \
	(BOOL)SNDMSG(hDlg, PSM_ISDIALOGMESSAGE, 0, (LPARAM)pMsg)
	 
#define PropSheet_GetCurrentPageHwnd(hDlg) \
	(HWND)SNDMSG(hDlg, PSM_GETCURRENTPAGEHWND, 0, 0L)
	 

/**************************************************************************
 *  UNDOCUMENTED functions
 */

/* private heap memory functions */

LPVOID WINAPI COMCTL32_Alloc (DWORD);
LPVOID WINAPI COMCTL32_ReAlloc (LPVOID, DWORD);
BOOL32 WINAPI COMCTL32_Free (LPVOID);
DWORD  WINAPI COMCTL32_GetSize (LPVOID);


INT32  WINAPI Str_GetPtr32A (LPCSTR, LPSTR, INT32);
BOOL32 WINAPI Str_SetPtr32A (LPSTR *, LPCSTR);
INT32  WINAPI Str_GetPtr32W (LPCWSTR, LPWSTR, INT32);
BOOL32 WINAPI Str_SetPtr32W (LPWSTR *, LPCWSTR);
#define Str_GetPtr WINELIB_NAME_AW(Str_GetPtr)
#define Str_SetPtr WINELIB_NAME_AW(Str_SetPtr)


/* Dynamic Storage Array */

typedef struct _DSA
{
    INT32  nItemCount;
    LPVOID pData;
    INT32  nMaxCount;
    INT32  nItemSize;
    INT32  nGrow;
} DSA, *HDSA;

HDSA   WINAPI DSA_Create (INT32, INT32);
BOOL32 WINAPI DSA_DeleteAllItems (const HDSA);
INT32  WINAPI DSA_DeleteItem (const HDSA, INT32);
BOOL32 WINAPI DSA_Destroy (const HDSA);
BOOL32 WINAPI DSA_GetItem (const HDSA, INT32, LPVOID);
LPVOID WINAPI DSA_GetItemPtr (const HDSA, INT32);
INT32  WINAPI DSA_InsertItem (const HDSA, INT32, LPVOID);
BOOL32 WINAPI DSA_SetItem (const HDSA, INT32, LPVOID);

typedef INT32 (CALLBACK *DSAENUMPROC)(LPVOID, DWORD);
VOID   WINAPI DSA_EnumCallback (const HDSA, DSAENUMPROC, LPARAM);
BOOL32 WINAPI DSA_DestroyCallback (const HDSA, DSAENUMPROC, LPARAM);


/* Dynamic Pointer Array */

typedef struct _DPA
{
    INT32    nItemCount;
    LPVOID   *ptrs; 
    HANDLE32 hHeap;
    INT32    nGrow;
    INT32    nMaxCount;
} DPA, *HDPA;

HDPA   WINAPI DPA_Create (INT32);
HDPA   WINAPI DPA_CreateEx (INT32, HANDLE32);
BOOL32 WINAPI DPA_Destroy (const HDPA);
HDPA   WINAPI DPA_Clone (const HDPA, const HDPA);
LPVOID WINAPI DPA_GetPtr (const HDPA, INT32);
INT32  WINAPI DPA_GetPtrIndex (const HDPA, LPVOID);
BOOL32 WINAPI DPA_Grow (const HDPA, INT32);
BOOL32 WINAPI DPA_SetPtr (const HDPA, INT32, LPVOID);
INT32  WINAPI DPA_InsertPtr (const HDPA, INT32, LPVOID);
LPVOID WINAPI DPA_DeletePtr (const HDPA, INT32);
BOOL32 WINAPI DPA_DeleteAllPtrs (const HDPA);

typedef INT32 (CALLBACK *PFNDPACOMPARE)(LPVOID, LPVOID, LPARAM);
BOOL32 WINAPI DPA_Sort (const HDPA, PFNDPACOMPARE, LPARAM);

#define DPAS_SORTED             0x0001
#define DPAS_INSERTBEFORE       0x0002
#define DPAS_INSERTAFTER        0x0004
 
INT32  WINAPI DPA_Search (const HDPA, LPVOID, INT32, PFNDPACOMPARE, LPARAM, UINT32);

#define DPAM_SORT               0x0001

BOOL32 WINAPI DPA_Merge (const HDPA, const HDPA, DWORD, PFNDPACOMPARE, LPVOID, LPARAM);

typedef INT32 (CALLBACK *DPAENUMPROC)(LPVOID, DWORD);
VOID   WINAPI DPA_EnumCallback (const HDPA, DPAENUMPROC, LPARAM);
BOOL32 WINAPI DPA_DestroyCallback (const HDPA, DPAENUMPROC, LPARAM);


#define DPA_GetPtrCount(hdpa)  (*(INT32*)(hdpa))
#define DPA_GetPtrPtr(hdpa)    (*((LPVOID**)((BYTE*)(hdpa)+sizeof(INT32))))
#define DPA_FastGetPtr(hdpa,i) (DPA_GetPtrPtr(hdpa)[i])


/* notification helper functions */

LRESULT WINAPI COMCTL32_SendNotify (HWND32, HWND32, UINT32, LPNMHDR);

/* type and functionality of last parameter is still unknown */
LRESULT WINAPI COMCTL32_SendNotifyEx (HWND32, HWND32, UINT32, LPNMHDR, DWORD);

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_COMMCTRL_H */


