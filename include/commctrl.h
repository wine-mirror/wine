/*
 * Common controls definitions
 */

#ifndef __WINE_COMMCTRL_H
#define __WINE_COMMCTRL_H

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "imagelist.h"
#include "prsht.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI ShowHideMenuCtl (HWND, UINT, LPINT);
VOID WINAPI GetEffectiveClientRect (HWND, LPRECT, LPINT);
VOID WINAPI InitCommonControls (VOID);

typedef struct tagINITCOMMONCONTROLSEX {
    DWORD dwSize;
    DWORD dwICC;
} INITCOMMONCONTROLSEX, *LPINITCOMMONCONTROLSEX;

BOOL WINAPI InitCommonControlsEx (LPINITCOMMONCONTROLSEX);

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
#define NM_UPDOWN                NMUPDOWN

/* callback constants */
#define LPSTR_TEXTCALLBACKA    ((LPSTR)-1L)
#define LPSTR_TEXTCALLBACKW    ((LPWSTR)-1L)
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
    HWND hwndToolTips;
} NMTOOLTIPSCREATED, *LPNMTOOLTIPSCREATED;


#ifndef CCSIZEOF_STRUCT
#define CCSIZEOF_STRUCT(name, member) \
    (((INT)((LPBYTE)(&((name*)0)->member)-((LPBYTE)((name*)0))))+ \
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
#define CDRF_NOTIFYITEMERASE    0x00000080      /*  obsolete ??? */


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
	HDC	hdc;
	RECT	rc;
	DWORD	dwItemSpec; 
	UINT	uItemState;
	LPARAM	lItemlParam;
} NMCUSTOMDRAW, *LPNMCUSTOMDRAW;

typedef struct tagNMTTCUSTOMDRAW
{
    NMCUSTOMDRAW nmcd;
    UINT       uDrawFlags;
} NMTTCUSTOMDRAW, *LPNMTTCUSTOMDRAW;




/* StatusWindow */

#define STATUSCLASSNAME16	"msctls_statusbar"
#define STATUSCLASSNAMEA	"msctls_statusbar32"
/* Does not work. gcc creates 4 byte wide strings.
 * #define STATUSCLASSNAME32W	L"msctls_statusbar32"
 */
static const WCHAR	_scn32w[] = {
'm','s','c','t','l','s','_','s','t','a','t','u','s','b','a','r','3','2',0
};
#define STATUSCLASSNAMEW	_scn32w
#define STATUSCLASSNAME		WINELIB_NAME_AW(STATUSCLASSNAME)

#define SBT_NOBORDERS		0x0100
#define SBT_POPOUT		0x0200
#define SBT_RTLREADING		0x0400  /* not supported */
#define SBT_TOOLTIPS		0x0800
#define SBT_OWNERDRAW		0x1000

#define SBARS_SIZEGRIP		0x0100

#define SB_SETTEXTA		(WM_USER+1)
#define SB_SETTEXTW		(WM_USER+11)
#define SB_SETTEXT		WINELIB_NAME_AW(SB_SETTEXT)
#define SB_GETTEXTA		(WM_USER+2)
#define SB_GETTEXTW		(WM_USER+13)
#define SB_GETTEXT		WINELIB_NAME_AW(SB_GETTEXT)
#define SB_GETTEXTLENGTHA	(WM_USER+3)
#define SB_GETTEXTLENGTHW	(WM_USER+12)
#define SB_GETTEXTLENGTH	WINELIB_NAME_AW(SB_GETTEXTLENGTH)
#define SB_SETPARTS		(WM_USER+4)
#define SB_GETPARTS		(WM_USER+6)
#define SB_GETBORDERS		(WM_USER+7)
#define SB_SETMINHEIGHT		(WM_USER+8)
#define SB_SIMPLE		(WM_USER+9)
#define SB_GETRECT		(WM_USER+10)
#define SB_ISSIMPLE		(WM_USER+14)
#define SB_SETICON		(WM_USER+15)
#define SB_SETTIPTEXTA	(WM_USER+16)
#define SB_SETTIPTEXTW	(WM_USER+17)
#define SB_SETTIPTEXT		WINELIB_NAME_AW(SB_SETTIPTEXT)
#define SB_GETTIPTEXTA	(WM_USER+18)
#define SB_GETTIPTEXTW	(WM_USER+19)
#define SB_GETTIPTEXT		WINELIB_NAME_AW(SB_GETTIPTEXT)
#define SB_GETICON		(WM_USER+20)
#define SB_SETBKCOLOR		CCM_SETBKCOLOR   /* lParam = bkColor */
#define SB_GETUNICODEFORMAT	CCM_GETUNICODEFORMAT
#define SB_SETUNICODEFORMAT	CCM_SETUNICODEFORMAT

#define SBN_FIRST		(0U-880U)
#define SBN_LAST		(0U-899U)
#define SBN_SIMPLEMODECHANGE	(SBN_FIRST-0)

HWND WINAPI CreateStatusWindowA (INT, LPCSTR, HWND, UINT);
HWND WINAPI CreateStatusWindowW (INT, LPCWSTR, HWND, UINT);
#define CreateStatusWindow WINELIB_NAME_AW(CreateStatusWindow)
VOID WINAPI DrawStatusTextA (HDC, LPRECT, LPCSTR, UINT);
VOID WINAPI DrawStatusTextW (HDC, LPRECT, LPCWSTR, UINT);
#define DrawStatusText WINELIB_NAME_AW(DrawStatusText)
VOID WINAPI MenuHelp (UINT, WPARAM, LPARAM, HMENU,
                      HINSTANCE, HWND, LPUINT);

/**************************************************************************
 *  Drag List control
 */

typedef struct tagDRAGLISTINFO
{
    UINT  uNotification;
    HWND  hWnd;
    POINT ptCursor;
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

BOOL WINAPI MakeDragList (HWND);
VOID   WINAPI DrawInsert (HWND, HWND, INT);
INT  WINAPI LBItemFromPt (HWND, POINT, BOOL);

  
/* UpDown */

#define UPDOWN_CLASS16          "msctls_updown"
#define UPDOWN_CLASSA         "msctls_updown32"
#define UPDOWN_CLASSW         L"msctls_updown32"
#define UPDOWN_CLASS            WINELIB_NAME_AW(UPDOWN_CLASS)

typedef struct tagUDACCEL
{
    UINT nSec;
    UINT nInc;
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

HWND WINAPI CreateUpDownControl (DWORD, INT, INT, INT, INT,
                                   HWND, INT, HINSTANCE, HWND,
                                   INT, INT, INT);

/* Progress Bar */

#define PROGRESS_CLASSA   "msctls_progress32"
#define PROGRESS_CLASSW  L"msctls_progress32"
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
    INT iLow;
    INT iHigh;
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
    HBITMAP hbmImage;
    HBITMAP hbmMask;
    INT     Unused1;
    INT     Unused2;
    RECT    rcImage;
} IMAGEINFO;


typedef struct _IMAGELISTDRAWPARAMS
{
    DWORD       cbSize;
    HIMAGELIST  himl;
    INT       i;
    HDC       hdcDst;
    INT       x;
    INT       y;
    INT       cx;
    INT       cy;
    INT       xBitmap;  /* x offest from the upperleft of bitmap */
    INT       yBitmap;  /* y offset from the upperleft of bitmap */
    COLORREF    rgbBk;
    COLORREF    rgbFg;
    UINT      fStyle;
    DWORD       dwRop;
} IMAGELISTDRAWPARAMS, *LPIMAGELISTDRAWPARAMS;

 
INT      WINAPI ImageList_Add(HIMAGELIST,HBITMAP,HBITMAP);
INT      WINAPI ImageList_AddIcon (HIMAGELIST, HICON);
INT      WINAPI ImageList_AddMasked(HIMAGELIST,HBITMAP,COLORREF);
BOOL     WINAPI ImageList_BeginDrag(HIMAGELIST,INT,INT,INT);
BOOL     WINAPI ImageList_Copy(HIMAGELIST,INT,HIMAGELIST,INT,INT);
HIMAGELIST WINAPI ImageList_Create(INT,INT,UINT,INT,INT);
BOOL     WINAPI ImageList_Destroy(HIMAGELIST);
BOOL     WINAPI ImageList_DragEnter(HWND,INT,INT);
BOOL     WINAPI ImageList_DragLeave(HWND); 
BOOL     WINAPI ImageList_DragMove(INT,INT);
BOOL     WINAPI ImageList_DragShowNolock (BOOL);
BOOL     WINAPI ImageList_Draw(HIMAGELIST,INT,HDC,INT,INT,UINT);
BOOL     WINAPI ImageList_DrawEx(HIMAGELIST,INT,HDC,INT,INT,INT,
                                   INT,COLORREF,COLORREF,UINT);
BOOL     WINAPI ImageList_DrawIndirect(IMAGELISTDRAWPARAMS*); 
HIMAGELIST WINAPI ImageList_Duplicate(HIMAGELIST);
BOOL     WINAPI ImageList_EndDrag(VOID);
COLORREF   WINAPI ImageList_GetBkColor(HIMAGELIST);
HIMAGELIST WINAPI ImageList_GetDragImage(POINT*,POINT*);
HICON    WINAPI ImageList_GetIcon(HIMAGELIST,INT,UINT);
BOOL     WINAPI ImageList_GetIconSize(HIMAGELIST,INT*,INT*);
INT      WINAPI ImageList_GetImageCount(HIMAGELIST);
BOOL     WINAPI ImageList_GetImageInfo(HIMAGELIST,INT,IMAGEINFO*);
BOOL     WINAPI ImageList_GetImageRect(HIMAGELIST,INT,LPRECT);
HIMAGELIST WINAPI ImageList_LoadImageA(HINSTANCE,LPCSTR,INT,INT,
                                         COLORREF,UINT,UINT);
HIMAGELIST WINAPI ImageList_LoadImageW(HINSTANCE,LPCWSTR,INT,INT,
                                         COLORREF,UINT,UINT);
#define    ImageList_LoadImage WINELIB_NAME_AW(ImageList_LoadImage)
HIMAGELIST WINAPI ImageList_Merge(HIMAGELIST,INT,HIMAGELIST,INT,INT,INT);
#ifdef __IStream_INTREFACE_DEFINED__
HIMAGELIST WINAPI ImageList_Read(LPSTREAM);
#endif
BOOL     WINAPI ImageList_Remove(HIMAGELIST,INT);
BOOL     WINAPI ImageList_Replace(HIMAGELIST,INT,HBITMAP,HBITMAP);
INT      WINAPI ImageList_ReplaceIcon(HIMAGELIST,INT,HICON);
COLORREF   WINAPI ImageList_SetBkColor(HIMAGELIST,COLORREF);
BOOL     WINAPI ImageList_SetDragCursorImage(HIMAGELIST,INT,INT,INT);

BOOL     WINAPI ImageList_SetIconSize(HIMAGELIST,INT,INT);
BOOL     WINAPI ImageList_SetImageCount(HIMAGELIST,INT);
BOOL     WINAPI ImageList_SetOverlayImage(HIMAGELIST,INT,INT);
#ifdef __IStream_INTREFACE_DEFINED__
BOOL     WINAPI ImageList_Write(HIMAGELIST, LPSTREAM);
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
#define FLATSB_CLASSA       "flatsb_class32"
#define FLATSB_CLASSW       L"flatsb_class32"
#define FLATSB_CLASS          WINELIB_NAME_AW(FLATSB_CLASS)

#define WSB_PROP_CYVSCROLL     0x00000001L
#define WSB_PROP_CXHSCROLL     0x00000002L
#define WSB_PROP_CYHSCROLL     0x00000004L
#define WSB_PROP_CXVSCROLL     0x00000008L
#define WSB_PROP_CXHTHUMB      0x00000010L
#define WSB_PROP_CYVTHUMB      0x00000020L
#define WSB_PROP_VBKGCOLOR     0x00000040L
#define WSB_PROP_HBKGCOLOR     0x00000080L
#define WSB_PROP_VSTYLE        0x00000100L
#define WSB_PROP_HSTYLE        0x00000200L
#define WSB_PROP_WINSTYLE      0x00000400L
#define WSB_PROP_PALETTE       0x00000800L
#define WSB_PROP_MASK          0x00000FFFL

#define FSB_REGULAR_MODE       0
#define FSB_ENCARTA_MODE       1
#define FSB_FLAT_MODE          2


BOOL  WINAPI FlatSB_EnableScrollBar(HWND, INT, UINT);
BOOL  WINAPI FlatSB_ShowScrollBar(HWND, INT, BOOL);
BOOL  WINAPI FlatSB_GetScrollRange(HWND, INT, LPINT, LPINT);
BOOL  WINAPI FlatSB_GetScrollInfo(HWND, INT, LPSCROLLINFO);
INT   WINAPI FlatSB_GetScrollPos(HWND, INT);
BOOL  WINAPI FlatSB_GetScrollProp(HWND, INT, LPINT);
INT   WINAPI FlatSB_SetScrollPos(HWND, INT, INT, BOOL);
INT   WINAPI FlatSB_SetScrollInfo(HWND, INT, LPSCROLLINFO, BOOL);
INT   WINAPI FlatSB_SetScrollRange(HWND, INT, INT, INT, BOOL);
BOOL  WINAPI FlatSB_SetScrollProp(HWND, UINT, INT, BOOL);
BOOL  WINAPI InitializeFlatSB(HWND);
HRESULT WINAPI UninitializeFlatSB(HWND);


/* Header control */

#define WC_HEADER16		"SysHeader" 
#define WC_HEADERA		"SysHeader32" 
#define WC_HEADERW		L"SysHeader32" 
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
#define HDM_INSERTITEMA       (HDM_FIRST+1)
#define HDM_INSERTITEMW       (HDM_FIRST+10)
#define HDM_INSERTITEM		WINELIB_NAME_AW(HDM_INSERTITEM)
#define HDM_DELETEITEM          (HDM_FIRST+2)
#define HDM_GETITEMA          (HDM_FIRST+3)
#define HDM_GETITEMW          (HDM_FIRST+11)
#define HDM_GETITEM		WINELIB_NAME_AW(HDM_GETITEM)
#define HDM_SETITEMA          (HDM_FIRST+4)
#define HDM_SETITEMW          (HDM_FIRST+12)
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
#define HDN_ITEMCHANGINGA     (HDN_FIRST-0)
#define HDN_ITEMCHANGINGW     (HDN_FIRST-20)
#define HDN_ITEMCHANGING WINELIB_NAME_AW(HDN_ITEMCHANGING)
#define HDN_ITEMCHANGEDA      (HDN_FIRST-1)
#define HDN_ITEMCHANGEDW      (HDN_FIRST-21)
#define HDN_ITEMCHANGED WINELIB_NAME_AW(HDN_ITEMCHANGED)
#define HDN_ITEMCLICKA        (HDN_FIRST-2)
#define HDN_ITEMCLICKW        (HDN_FIRST-22)
#define HDN_ITEMCLICK WINELIB_NAME_AW(HDN_ITEMCLICK)
#define HDN_ITEMDBLCLICKA     (HDN_FIRST-3)
#define HDN_ITEMDBLCLICKW     (HDN_FIRST-23)
#define HDN_ITEMDBLCLICK WINELIB_NAME_AW(HDN_ITEMDBLCLICK)
#define HDN_DIVIDERDBLCLICKA  (HDN_FIRST-5)
#define HDN_DIVIDERDBLCLICKW  (HDN_FIRST-25)
#define HDN_DIVIDERDBLCLICK WINELIB_NAME_AW(HDN_DIVIDERDBLCLICK)
#define HDN_BEGINTRACKA       (HDN_FIRST-6)
#define HDN_BEGINTRACKW       (HDN_FIRST-26)
#define HDN_BEGINTRACK WINELIB_NAME_AW(HDN_BEGINTRACK)
#define HDN_ENDTRACKA         (HDN_FIRST-7)
#define HDN_ENDTRACKW         (HDN_FIRST-27)
#define HDN_ENDTRACK WINELIB_NAME_AW(HDN_ENDTRACK)
#define HDN_TRACKA            (HDN_FIRST-8)
#define HDN_TRACKW            (HDN_FIRST-28)
#define HDN_TRACK WINELIB_NAME_AW(HDN_TRACK)
#define HDN_GETDISPINFOA      (HDN_FIRST-9)
#define HDN_GETDISPINFOW      (HDN_FIRST-29)
#define HDN_GETDISPINFO WINELIB_NAME_AW(HDN_GETDISPINFO)
#define HDN_BEGINDRACK          (HDN_FIRST-10)
#define HDN_ENDDRACK            (HDN_FIRST-11)

typedef struct _HD_LAYOUT
{
    RECT      *prc;
    WINDOWPOS *pwpos;
} HDLAYOUT, *LPHDLAYOUT;

#define HD_LAYOUT   HDLAYOUT

typedef struct _HD_ITEMA
{
    UINT    mask;
    INT     cxy;
    LPSTR     pszText;
    HBITMAP hbm;
    INT     cchTextMax;
    INT     fmt;
    LPARAM    lParam;
    INT     iImage;
    INT     iOrder;
} HDITEMA, *LPHDITEMA;

typedef struct _HD_ITEMW
{
    UINT    mask;
    INT     cxy;
    LPWSTR    pszText;
    HBITMAP hbm;
    INT     cchTextMax;
    INT     fmt;
    LPARAM    lParam;
    INT     iImage;
    INT     iOrder;
} HDITEMW, *LPHDITEMW;

#define HDITEM   WINELIB_NAME_AW(HDITEM)
#define LPHDITEM WINELIB_NAME_AW(LPHDITEM)
#define HD_ITEM  HDITEM

#define HDITEM_V1_SIZEA CCSIZEOF_STRUCT(HDITEMA, lParam)
#define HDITEM_V1_SIZEW CCSIZEOF_STRUCT(HDITEMW, lParam)
#define HDITEM_V1_SIZE WINELIB_NAME_AW(HDITEM_V1_SIZE)

typedef struct _HD_HITTESTINFO
{
    POINT pt;
    UINT  flags;
    INT   iItem;
} HDHITTESTINFO, *LPHDHITTESTINFO;

#define HD_HITTESTINFO   HDHITTESTINFO

typedef struct tagNMHEADERA
{
    NMHDR     hdr;
    INT     iItem;
    INT     iButton;
    HDITEMA *pitem;
} NMHEADERA, *LPNMHEADERA;

typedef struct tagNMHEADERW
{
    NMHDR     hdr;
    INT     iItem;
    INT     iButton;
    HDITEMW *pitem;
} NMHEADERW, *LPNMHEADERW;

#define NMHEADER		WINELIB_NAME_AW(NMHEADER)
#define LPNMHEADER		WINELIB_NAME_AW(LPNMHEADER)
#define HD_NOTIFY               NMHEADER

typedef struct tagNMHDDISPINFOA
{
    NMHDR     hdr;
    INT     iItem;
    UINT    mask;
    LPSTR     pszText;
    INT     cchTextMax;
    INT     iImage;
    LPARAM    lParam;
} NMHDDISPINFOA, *LPNMHDDISPINFOA;

typedef struct tagNMHDDISPINFOW
{
    NMHDR     hdr;
    INT     iItem;
    UINT    mask;
    LPWSTR    pszText;
    INT     cchTextMax;
    INT     iImage;
    LPARAM    lParam;
} NMHDDISPINFOW, *LPNMHDDISPINFOW;

#define NMHDDISPINFO		WINELIB_NAME_AW(NMHDDISPINFO)
#define LPNMHDDISPINFO		WINELIB_NAME_AW(LPNMHDDISPINFO)

#define Header_GetItemCount(hwndHD) \
  (INT)SendMessageA((hwndHD),HDM_GETITEMCOUNT,0,0L)
#define Header_InsertItemA(hwndHD,i,phdi) \
  (INT)SendMessageA((hwndHD),HDM_INSERTITEMA,(WPARAM)(INT)(i),(LPARAM)(const HDITEMA*)(phdi))
#define Header_InsertItemW(hwndHD,i,phdi) \
  (INT)SendMessageW((hwndHD),HDM_INSERTITEMW,(WPARAM)(INT)(i),(LPARAM)(const HDITEMW*)(phdi))
#define Header_InsertItem WINELIB_NAME_AW(Header_InsertItem)
#define Header_DeleteItem(hwndHD,i) \
  (BOOL)SendMessageA((hwndHD),HDM_DELETEITEM,(WPARAM)(INT)(i),0L)
#define Header_GetItemA(hwndHD,i,phdi) \
  (BOOL)SendMessageA((hwndHD),HDM_GETITEMA,(WPARAM)(INT)(i),(LPARAM)(HDITEMA*)(phdi))
#define Header_GetItemW(hwndHD,i,phdi) \
  (BOOL)SendMessageW((hwndHD),HDM_GETITEMW,(WPARAM)(INT)(i),(LPARAM)(HDITEMW*)(phdi))
#define Header_GetItem WINELIB_NAME_AW(Header_GetItem)
#define Header_SetItemA(hwndHD,i,phdi) \
  (BOOL)SendMessageA((hwndHD),HDM_SETITEMA,(WPARAM)(INT)(i),(LPARAM)(const HDITEMA*)(phdi))
#define Header_SetItemW(hwndHD,i,phdi) \
  (BOOL)SendMessageW((hwndHD),HDM_SETITEMW,(WPARAM)(INT)(i),(LPARAM)(const HDITEMW*)(phdi))
#define Header_SetItem WINELIB_NAME_AW(Header_SetItem)
#define Header_Layout(hwndHD,playout) \
  (BOOL)SendMessageA((hwndHD),HDM_LAYOUT,0,(LPARAM)(LPHDLAYOUT)(playout))
#define Header_GetItemRect(hwnd,iItem,lprc) \
  (BOOL)SendMessageA((hwnd),HDM_GETITEMRECT,(WPARAM)iItem,(LPARAM)lprc)
#define Header_SetImageList(hwnd,himl) \
  (HIMAGELIST)SendMessageA((hwnd),HDM_SETIMAGELIST,0,(LPARAM)himl)
#define Header_GetImageList(hwnd) \
  (HIMAGELIST)SendMessageA((hwnd),HDM_GETIMAGELIST,0,0)
#define Header_OrderToIndex(hwnd,i) \
  (INT)SendMessageA((hwnd),HDM_ORDERTOINDEX,(WPARAM)i,0)
#define Header_CreateDragImage(hwnd,i) \
  (HIMAGELIST)SendMessageA((hwnd),HDM_CREATEDRAGIMAGE,(WPARAM)i,0)
#define Header_GetOrderArray(hwnd,iCount,lpi) \
  (BOOL)SendMessageA((hwnd),HDM_GETORDERARRAY,(WPARAM)iCount,(LPARAM)lpi)
#define Header_SetOrderArray(hwnd,iCount,lpi) \
  (BOOL)SendMessageA((hwnd),HDM_SETORDERARRAY,(WPARAM)iCount,(LPARAM)lpi)
#define Header_SetHotDivider(hwnd,fPos,dw) \
  (INT)SendMessageA((hwnd),HDM_SETHOTDIVIDER,(WPARAM)fPos,(LPARAM)dw)
#define Header_SetUnicodeFormat(hwnd,fUnicode) \
  (BOOL)SendMessageA((hwnd),HDM_SETUNICODEFORMAT,(WPARAM)(fUnicode),0)
#define Header_GetUnicodeFormat(hwnd) \
  (BOOL)SendMessageA((hwnd),HDM_GETUNICODEFORMAT,0,0)


/* Toolbar */

#define TOOLBARCLASSNAME16      "ToolbarWindow" 
#define TOOLBARCLASSNAMEW     "ToolbarWindow32" 
#define TOOLBARCLASSNAMEA     "ToolbarWindow32" 
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
#define TB_ADDBUTTONSA         (WM_USER+20)
#define TB_ADDBUTTONSW         (WM_USER+68)
#define TB_ADDBUTTONS WINELIB_NAME_AW(TB_ADDBUTTONS)
#define TB_HITTEST               (WM_USER+69)
#define TB_INSERTBUTTONA       (WM_USER+21)
#define TB_INSERTBUTTONW       (WM_USER+67)
#define TB_INSERTBUTTON WINELIB_NAME_AW(TB_INSERTBUTTON)
#define TB_DELETEBUTTON          (WM_USER+22)
#define TB_GETBUTTON             (WM_USER+23)
#define TB_BUTTONCOUNT           (WM_USER+24)
#define TB_COMMANDTOINDEX        (WM_USER+25)
#define TB_SAVERESTOREA        (WM_USER+26)
#define TB_SAVERESTOREW        (WM_USER+76)
#define TB_SAVERESTORE WINELIB_NAME_AW(TB_SAVERESTORE)
#define TB_CUSTOMIZE             (WM_USER+27)
#define TB_ADDSTRINGA          (WM_USER+28) 
#define TB_ADDSTRINGW          (WM_USER+77) 
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
#define TB_GETBUTTONTEXTA      (WM_USER+45)
#define TB_GETBUTTONTEXTW      (WM_USER+75)
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
#define TB_GETBUTTONINFOW      (WM_USER+63)
#define TB_GETBUTTONINFOA      (WM_USER+65)
#define TB_GETBUTTONINFO WINELIB_NAME_AW(TB_GETBUTTONINFO)
#define TB_SETBUTTONINFOW      (WM_USER+64)
#define TB_SETBUTTONINFOA      (WM_USER+66)
#define TB_SETBUTTONINFO WINELIB_NAME_AW(TB_SETBUTTONINFO)
#define TB_SETDRAWTEXTFLAGS      (WM_USER+70)
#define TB_GETHOTITEM            (WM_USER+71)
#define TB_SETHOTITEM            (WM_USER+72)
#define TB_SETANCHORHIGHLIGHT    (WM_USER+73)
#define TB_GETANCHORHIGHLIGHT    (WM_USER+74)
#define TB_MAPACCELERATORA     (WM_USER+78)
#define TB_MAPACCELERATORW     (WM_USER+90)
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
#define TBN_GETBUTTONINFOA    (TBN_FIRST-0)
#define TBN_GETBUTTONINFOW    (TBN_FIRST-20)
#define TBN_GETBUTTONINFO WINELIB_NAME_AW(TBN_GETBUTTONINFO)
#define TBN_BEGINDRAG		(TBN_FIRST-1)
#define TBN_ENDDRAG		(TBN_FIRST-2)
#define TBN_BEGINADJUST		(TBN_FIRST-3)
#define TBN_ENDADJUST		(TBN_FIRST-4)
#define TBN_RESET		(TBN_FIRST-5)
#define TBN_QUERYINSERT		(TBN_FIRST-6)
#define TBN_QUERYDELETE		(TBN_FIRST-7)
#define TBN_TOOLBARCHANGE	(TBN_FIRST-8)
#define TBN_CUSTHELP		(TBN_FIRST-9)
#define TBN_DROPDOWN		(TBN_FIRST-10)
#define TBN_GETOBJECT		(TBN_FIRST-12)
#define TBN_HOTITEMCHANGE	(TBN_FIRST-13)
#define TBN_DRAGOUT		(TBN_FIRST-14)
#define TBN_DELETINGBUTTON	(TBN_FIRST-15)
#define TBN_GETDISPINFOA	(TBN_FIRST-16)
#define TBN_GETDISPINFOW	(TBN_FIRST-17)
#define TBN_GETDISPINFO		WINELIB_NAME_AW(TBN_GETDISPINFO)
#define TBN_GETINFOTIPA       (TBN_FIRST-18)
#define TBN_GETINFOTIPW       (TBN_FIRST-19)
#define TBN_GETINFOTIP WINELIB_NAME_AW(TBN_GETINFOTIP)


/* This is just for old CreateToolbar. */
/* Don't use it in new programs. */
typedef struct _OLDTBBUTTON {
    INT iBitmap;
    INT idCommand;
    BYTE  fsState;
    BYTE  fsStyle;
    BYTE  bReserved[2];
    DWORD dwData;
} OLDTBBUTTON, *POLDTBBUTTON, *LPOLDTBBUTTON;
typedef const OLDTBBUTTON *LPCOLDTBBUTTON;


typedef struct _TBBUTTON {
    INT iBitmap;
    INT idCommand;
    BYTE  fsState;
    BYTE  fsStyle;
    BYTE  bReserved[2];
    DWORD dwData;
    INT iString;
} TBBUTTON, *PTBBUTTON, *LPTBBUTTON;
typedef const TBBUTTON *LPCTBBUTTON;

typedef struct tagNMTOOLBAR {
        NMHDR           hdr;
        int             iItem;
        TBBUTTON        tbButton;
        int             cchText;
        LPWSTR          pszText;
        RECT            rcButton; /*Version 5.80*/
} NMTOOLBARW, * LPNMTOOLBARW;

typedef struct _COLORMAP {
    COLORREF from;
    COLORREF to;
} COLORMAP, *LPCOLORMAP;

typedef struct tagTBADDBITMAP {
    HINSTANCE hInst;
    UINT      nID;
} TBADDBITMAP, *LPTBADDBITMAP;

#define HINST_COMMCTRL         ((HINSTANCE)-1)
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
} TBSAVEPARAMSA, *LPTBSAVEPARAMSA;

typedef struct tagTBSAVEPARAMSW {
    HKEY   hkr;
    LPCWSTR pszSubKey;
    LPCWSTR pszValueName;
} TBSAVEPARAMSAW, *LPTBSAVEPARAMSAW;

#define TBSAVEPARAMS   WINELIB_NAME_AW(TBSAVEPARAMS)
#define LPTBSAVEPARAMS WINELIB_NAME_AW(LPTBSAVEPARAMS)

typedef struct
{
    UINT cbSize;
    DWORD  dwMask;
    INT  idCommand;
    INT  iImage;
    BYTE   fsState;
    BYTE   fsStyle;
    WORD   cx;
    DWORD  lParam;
    LPSTR  pszText;
    INT  cchText;
} TBBUTTONINFOA, *LPTBBUTTONINFOA;

typedef struct
{
    UINT cbSize;
    DWORD  dwMask;
    INT  idCommand;
    INT  iImage;
    BYTE   fsState;
    BYTE   fsStyle;
    WORD   cx;
    DWORD  lParam;
    LPWSTR pszText;
    INT  cchText;
} TBBUTTONINFOW, *LPTBBUTTONINFOW;

#define TBBUTTONINFO   WINELIB_NAME_AW(TBBUTTONINFO)
#define LPTBBUTTONINFO WINELIB_NAME_AW(LPTBBUTTONINFO)

typedef struct tagNMTBGETINFOTIPA
{
    NMHDR  hdr;
    LPSTR  pszText;
    INT  cchTextMax;
    INT  iItem;
    LPARAM lParam;
} NMTBGETINFOTIPA, *LPNMTBGETINFOTIPA;

typedef struct tagNMTBGETINFOTIPW
{
    NMHDR  hdr;
    LPWSTR pszText;
    INT  cchTextMax;
    INT  iItem;
    LPARAM lParam;
} NMTBGETINFOTIPW, *LPNMTBGETINFOTIPW;

#define NMTBGETINFOTIP   WINELIB_NAME_AW(NMTBGETINFOFTIP)
#define LPNMTBGETINFOTIP WINELIB_NAME_AW(LPNMTBGETINFOTIP)

typedef struct
{
	HINSTANCE hInstOld;
	UINT      nIDOld;
	HINSTANCE hInstNew;
	UINT      nIDNew;
	INT       nButtons;
} TBREPLACEBITMAP, *LPTBREPLACEBITMAP;

HWND WINAPI
CreateToolbar(HWND, DWORD, UINT, INT, HINSTANCE,
              UINT, LPCOLDTBBUTTON, INT); 
 
HWND WINAPI
CreateToolbarEx(HWND, DWORD, UINT, INT,
                HINSTANCE, UINT, LPCTBBUTTON, 
                INT, INT, INT, INT, INT, UINT); 

HBITMAP WINAPI
CreateMappedBitmap (HINSTANCE, INT, UINT, LPCOLORMAP, INT); 


/* Tool tips */

#define TOOLTIPS_CLASS16        "tooltips_class"
#define TOOLTIPS_CLASSA       "tooltips_class32"
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
#define TTM_ADDTOOLA          (WM_USER+4)
#define TTM_ADDTOOLW          (WM_USER+50)
#define TTM_ADDTOOL WINELIB_NAME_AW(TTM_ADDTOOL)
#define TTM_DELTOOLA          (WM_USER+5)
#define TTM_DELTOOLW          (WM_USER+51)
#define TTM_DELTOOL WINELIB_NAME_AW(TTM_DELTOOL)
#define TTM_NEWTOOLRECTA      (WM_USER+6)
#define TTM_NEWTOOLRECTW      (WM_USER+52)
#define TTM_NEWTOOLRECT WINELIB_NAME_AW(TTM_NEWTOOLRECT)
#define TTM_RELAYEVENT          (WM_USER+7)
#define TTM_GETTOOLINFOA      (WM_USER+8)
#define TTM_GETTOOLINFOW      (WM_USER+53)
#define TTM_GETTOOLINFO WINELIB_NAME_AW(TTM_GETTOOLINFO)
#define TTM_SETTOOLINFOA      (WM_USER+9)
#define TTM_SETTOOLINFOW      (WM_USER+54)
#define TTM_SETTOOLINFO WINELIB_NAME_AW(TTM_SETTOOLINFO)
#define TTM_HITTESTA          (WM_USER+10)
#define TTM_HITTESTW          (WM_USER+55)
#define TTM_HITTEST WINELIB_NAME_AW(TTM_HITTEST)
#define TTM_GETTEXTA          (WM_USER+11)
#define TTM_GETTEXTW          (WM_USER+56)
#define TTM_GETTEXT WINELIB_NAME_AW(TTM_GETTEXT)
#define TTM_UPDATETIPTEXTA    (WM_USER+12)
#define TTM_UPDATETIPTEXTW    (WM_USER+57)
#define TTM_UPDATETIPTEXT WINELIB_NAME_AW(TTM_UPDATETIPTEXT)
#define TTM_GETTOOLCOUNT        (WM_USER+13)
#define TTM_ENUMTOOLSA        (WM_USER+14)
#define TTM_ENUMTOOLSW        (WM_USER+58)
#define TTM_ENUMTOOLS WINELIB_NAME_AW(TTM_ENUMTOOLS)
#define TTM_GETCURRENTTOOLA   (WM_USER+15)
#define TTM_GETCURRENTTOOLW   (WM_USER+59)
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
#define TTN_GETDISPINFOA      (TTN_FIRST-0)
#define TTN_GETDISPINFOW      (TTN_FIRST-10)
#define TTN_GETDISPINFO WINELIB_NAME_AW(TTN_GETDISPINFO)
#define TTN_SHOW                (TTN_FIRST-1)
#define TTN_POP                 (TTN_FIRST-2)

#define TTN_NEEDTEXT TTN_GETDISPINFO
#define TTN_NEEDTEXTA TTN_GETDISPINFOA
#define TTN_NEEDTEXTW TTN_GETDISPINFOW

typedef struct tagTOOLINFOA {
    UINT cbSize;
    UINT uFlags;
    HWND hwnd;
    UINT uId;
    RECT rect;
    HINSTANCE hinst;
    LPSTR lpszText;
    LPARAM lParam;
} TTTOOLINFOA, *LPTOOLINFOA, *PTOOLINFOA, *LPTTTOOLINFOA;

typedef struct tagTOOLINFOW {
    UINT cbSize;
    UINT uFlags;
    HWND hwnd;
    UINT uId;
    RECT rect;
    HINSTANCE hinst;
    LPWSTR lpszText;
    LPARAM lParam;
} TTTOOLINFOW, *LPTOOLINFOW, *PTOOLINFOW, *LPTTTOOLINFOW;

#define TTTOOLINFO WINELIB_NAME_AW(TTTOOLINFO)
#define TOOLINFO WINELIB_NAME_AW(TTTOOLINFO)
#define PTOOLINFO WINELIB_NAME_AW(PTOOLINFO)
#define LPTTTOOLINFO WINELIB_NAME_AW(LPTTTOOLINFO)
#define LPTOOLINFO WINELIB_NAME_AW(LPTOOLINFO)

#define TTTOOLINFO_V1_SIZEA CCSIZEOF_STRUCT(TTTOOLINFOA, lpszText)
#define TTTOOLINFO_V1_SIZEW CCSIZEOF_STRUCT(TTTOOLINFOW, lpszText)
#define TTTOOLINFO_V1_SIZE WINELIB_NAME_AW(TTTOOLINFO_V1_SIZE)

typedef struct _TT_HITTESTINFOA
{
    HWND        hwnd;
    POINT       pt;
    TTTOOLINFOA ti;
} TTHITTESTINFOA, *LPTTHITTESTINFOA;

typedef struct _TT_HITTESTINFOW
{
    HWND        hwnd;
    POINT       pt;
    TTTOOLINFOW ti;
} TTHITTESTINFOW, *LPTTHITTESTINFOW;

#define TTHITTESTINFO WINELIB_NAME_AW(TTHITTESTINFO)
#define LPTTHITTESTINFO WINELIB_NAME_AW(LPTTHITTESTINFO)

typedef struct tagNMTTDISPINFOA
{
    NMHDR hdr;
    LPSTR lpszText;
    CHAR  szText[80];
    HINSTANCE hinst;
    UINT      uFlags;
    LPARAM      lParam;
} NMTTDISPINFOA, *LPNMTTDISPINFOA;

typedef struct tagNMTTDISPINFOW
{
    NMHDR       hdr;
    LPWSTR      lpszText;
    WCHAR       szText[80];
    HINSTANCE hinst;
    UINT      uFlags;
    LPARAM      lParam;
} NMTTDISPINFOW, *LPNMTTDISPINFOW;

#define NMTTDISPINFO WINELIB_NAME_AW(NMTTDISPINFO)
#define LPNMTTDISPINFO WINELIB_NAME_AW(LPNMTTDISPINFO)

#define NMTTDISPINFO_V1_SIZEA CCSIZEOF_STRUCT(NMTTDISPINFOA, uFlags)
#define NMTTDISPINFO_V1_SIZEW CCSIZEOF_STRUCT(NMTTDISPINFOW, uFlags)
#define NMTTDISPINFO_V1_SIZE WINELIB_NAME_AW(NMTTDISPINFO_V1_SIZE)

#define TOOLTIPTEXTW    NMTTDISPINFOW
#define TOOLTIPTEXTA    NMTTDISPINFOA
#define TOOLTIPTEXT     NMTTDISPINFO
#define LPTOOLTIPTEXTW  LPNMTTDISPINFOW
#define LPTOOLTIPTEXTA  LPNMTTDISPINFOA
#define LPTOOLTIPTEXT   LPNMTTDISPINFO


/* Rebar control */

#define REBARCLASSNAME16        "ReBarWindow"
#define REBARCLASSNAMEA       "ReBarWindow32"
#define REBARCLASSNAMEW       L"ReBarWindow32"
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

#define RB_INSERTBANDA        (WM_USER+1)
#define RB_INSERTBANDW        (WM_USER+10)
#define RB_INSERTBAND           WINELIB_NAME_AW(RB_INSERTBAND)
#define RB_DELETEBAND           (WM_USER+2)
#define RB_GETBARINFO           (WM_USER+3)
#define RB_SETBARINFO           (WM_USER+4)
#define RB_GETBANDINFO        (WM_USER+5)   /* just for compatibility */
#define RB_SETBANDINFOA       (WM_USER+6)
#define RB_SETBANDINFOW       (WM_USER+11)
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
#define RB_GETBANDINFOW       (WM_USER+28)
#define RB_GETBANDINFOA       (WM_USER+29)
#define RB_GETBANDINFO16          WINELIB_NAME_AW(RB_GETBANDINFO16)
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
    UINT     cbSize;
    UINT     fMask;
    HIMAGELIST himl;
} REBARINFO, *LPREBARINFO;

typedef struct tagREBARBANDINFOA
{
    UINT    cbSize;
    UINT    fMask;
    UINT    fStyle;
    COLORREF  clrFore;
    COLORREF  clrBack;
    LPSTR     lpText;
    UINT    cch;
    INT     iImage;
    HWND    hwndChild;
    UINT    cxMinChild;
    UINT    cyMinChild;
    UINT    cx;
    HBITMAP hbmBack;
    UINT    wID;
    UINT    cyChild;
    UINT    cyMaxChild;
    UINT    cyIntegral;
    UINT    cxIdeal;
    LPARAM    lParam;
    UINT    cxHeader;
} REBARBANDINFOA, *LPREBARBANDINFOA;

typedef REBARBANDINFOA const *LPCREBARBANDINFOA;

typedef struct tagREBARBANDINFOW
{
    UINT    cbSize;
    UINT    fMask;
    UINT    fStyle;
    COLORREF  clrFore;
    COLORREF  clrBack;
    LPWSTR    lpText;
    UINT    cch;
    INT     iImage;
    HWND    hwndChild;
    UINT    cxMinChild;
    UINT    cyMinChild;
    UINT    cx;
    HBITMAP hbmBack;
    UINT    wID;
    UINT    cyChild;
    UINT    cyMaxChild;
    UINT    cyIntegral;
    UINT    cxIdeal;
    LPARAM    lParam;
    UINT    cxHeader;
} REBARBANDINFOW, *LPREBARBANDINFOW;

typedef REBARBANDINFOW const *LPCREBARBANDINFOW;

#define REBARBANDINFO    WINELIB_NAME_AW(REBARBANDINFO)
#define LPREBARBANDINFO  WINELIB_NAME_AW(LPREBARBANDINFO)
#define LPCREBARBANDINFO WINELIB_NAME_AW(LPCREBARBANDINFO)

#define REBARBANDINFO_V3_SIZEA CCSIZEOF_STRUCT(REBARBANDINFOA, wID)
#define REBARBANDINFO_V3_SIZEW CCSIZEOF_STRUCT(REBARBANDINFOW, wID)
#define REBARBANDINFO_V3_SIZE WINELIB_NAME_AW(REBARBANDINFO_V3_SIZE)

typedef struct tagNMREBARCHILDSIZE
{
    NMHDR  hdr;
    UINT uBand;
    UINT wID;
    RECT rcChild;
    RECT rcBand;
} NMREBARCHILDSIZE, *LPNMREBARCHILDSIZE;

typedef struct tagNMREBAR
{
    NMHDR  hdr;
    DWORD  dwMask;
    UINT uBand;
    UINT fStyle;
    UINT wID;
    LPARAM lParam;
} NMREBAR, *LPNMREBAR;

typedef struct tagNMRBAUTOSIZE
{
    NMHDR hdr;
    BOOL fChanged;
    RECT rcTarget;
    RECT rcActual;
} NMRBAUTOSIZE, *LPNMRBAUTOSIZE;

typedef struct _RB_HITTESTINFO
{
    POINT pt;
    UINT  flags;
    INT   iBand;
} RBHITTESTINFO, *LPRBHITTESTINFO;


/* Trackbar control */

#define TRACKBAR_CLASS16        "msctls_trackbar"
#define TRACKBAR_CLASSA       "msctls_trackbar32"
#define TRACKBAR_CLASSW       L"msctls_trackbar32"
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
#define TBS_REVERSED			0x0200

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

#define WC_PAGESCROLLERA      "SysPager"
#define WC_PAGESCROLLERW      L"SysPager"
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
    RECT rcParent;
    INT  iDir;
    INT  iXpos;
    INT  iYpos;
    INT  iScroll;
} NMPGSCROLL, *LPNMPGSCROLL;

typedef struct
{
    NMHDR hdr;
    DWORD dwFlag;
    INT iWidth;
    INT iHeight;
} NMPGCALCSIZE, *LPNMPGCALCSIZE;


/* Treeview control */

#define WC_TREEVIEWA          "SysTreeView32"
#define WC_TREEVIEWW          L"SysTreeView32"
#define WC_TREEVIEW             WINELIB_NAME_AW(WC_TREEVIEW)

#define TVSIL_NORMAL            0
#define TVSIL_STATE             2

#define TV_FIRST                0x1100
#define TVM_INSERTITEMA       (TV_FIRST+0)
#define TVM_INSERTITEMW       (TV_FIRST+50)
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
#define TVM_GETITEMA          (TV_FIRST+12)
#define TVM_GETITEMW          (TV_FIRST+62)
#define TVM_GETITEM             WINELIB_NAME_AW(TVM_GETITEM)
#define TVM_SETITEMA          (TV_FIRST+13)
#define TVM_SETITEMW          (TV_FIRST+63)
#define TVM_SETITEM             WINELIB_NAME_AW(TVM_SETITEM)
#define TVM_EDITLABELA        (TV_FIRST+14)
#define TVM_EDITLABELW        (TV_FIRST+65)
#define TVM_EDITLABEL           WINELIB_NAME_AW(TVM_EDITLABEL)
#define TVM_GETEDITCONTROL      (TV_FIRST+15)
#define TVM_GETVISIBLECOUNT     (TV_FIRST+16)
#define TVM_HITTEST             (TV_FIRST+17)
#define TVM_CREATEDRAGIMAGE     (TV_FIRST+18)
#define TVM_SORTCHILDREN        (TV_FIRST+19)
#define TVM_ENSUREVISIBLE       (TV_FIRST+20)
#define TVM_SORTCHILDRENCB      (TV_FIRST+21)
#define TVM_ENDEDITLABELNOW     (TV_FIRST+22)
#define TVM_GETISEARCHSTRINGA (TV_FIRST+23)
#define TVM_GETISEARCHSTRINGW (TV_FIRST+64)
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
#define TVM_GETITEMSTATE        (TV_FIRST+39)
#define TVM_SETLINECOLOR        (TV_FIRST+40)
#define TVM_GETLINECOLOR        (TV_FIRST+41)
#define TVM_SETUNICODEFORMAT    CCM_SETUNICODEFORMAT
#define TVM_GETUNICODEFORMAT    CCM_GETUNICODEFORMAT

#define TreeView_GetItemState(hwndTV, hti, mask) \
   (UINT)SendMessageA((hwndTV), TVM_GETITEMSTATE, (WPARAM)(hti), (LPARAM)(mask))
#define TreeView_GetCheckState(hwndTV, hti) \
   ((((UINT)(SendMessageA((hwndTV), TVM_GETITEMSTATE, (WPARAM)(hti),  \
                     TVIS_STATEIMAGEMASK))) >> 12) -1)
#define TreeView_SetLineColor(hwnd, clr) \
    (COLORREF)SendMessageA((hwnd), TVM_SETLINECOLOR, 0, (LPARAM)(clr))
#define TreeView_GetLineColor(hwnd) \
    (COLORREF)SendMessageA((hwnd), TVM_GETLINECOLOR, 0, 0)
#define TreeView_SetItemState(hwndTV, hti, data, _mask) \
{ TVITEM _TVi; \
  _TVi.mask = TVIF_STATE; \
  _TVi.hItem = hti; \
  _TVi.stateMask = _mask; \
  _TVi.state = data; \
  SendMessageA((hwndTV), TVM_SETITEM, 0, (LPARAM)(TV_ITEM *)&_TVi); \
}





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

#define TVI_ROOT              ((HTREEITEM)0xffff0000)     /* -65536 */
#define TVI_FIRST             ((HTREEITEM)0xffff0001)     /* -65535 */
#define TVI_LAST              ((HTREEITEM)0xffff0002)     /* -65534 */
#define TVI_SORT              ((HTREEITEM)0xffff0003)     /* -65533 */

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


typedef struct _TREEITEM *HTREEITEM;

typedef struct {
      UINT mask;
      HTREEITEM hItem;
      UINT state;
      UINT stateMask;
      LPSTR  pszText;
      INT  cchTextMax;
      INT  iImage;
      INT  iSelectedImage;
      INT  cChildren;
      LPARAM lParam;
} TVITEMA, *LPTVITEMA;

typedef struct {
      UINT mask;
      HTREEITEM hItem;
      UINT state;
      UINT stateMask;
      LPWSTR pszText;
      INT cchTextMax;
      INT iImage;
      INT iSelectedImage;
      INT cChildren;
      LPARAM lParam;
} TVITEMW, *LPTVITEMW;

#define TVITEM     WINELIB_NAME_AW(TVITEM)
#define LPTVITEM   WINELIB_NAME_AW(LPTVITEM)
#define TV_ITEM	    TVITEM

typedef struct {
      UINT mask;
      HTREEITEM hItem;
      UINT state;
      UINT stateMask;
      LPSTR  pszText;
      INT  cchTextMax;
      INT  iImage;
      INT  iSelectedImage;
      INT  cChildren;
      LPARAM lParam;
      INT iIntegral;
} TVITEMEXA, *LPTVITEMEXA;

typedef struct {
      UINT mask;
      HTREEITEM hItem;
      UINT state;
      UINT stateMask;
      LPWSTR pszText;
      INT cchTextMax;
      INT iImage;
      INT iSelectedImage;
      INT cChildren;
      LPARAM lParam;
      INT iIntegral;
} TVITEMEXW, *LPTVITEMEXW;

#define TVITEMEX   WINELIB_NAME_AW(TVITEMEX)
#define LPTVITEMEX WINELIB_NAME_AW(LPTVITEMEX)

typedef struct tagTVINSERTSTRUCTA {
        HTREEITEM hParent;
        HTREEITEM hInsertAfter;
        union {
           TVITEMEXA itemex;
           TVITEMA   item;
        } DUMMYUNIONNAME;
} TVINSERTSTRUCTA, *LPTVINSERTSTRUCTA;

typedef struct tagTVINSERTSTRUCTW {
        HTREEITEM hParent;
        HTREEITEM hInsertAfter;
        union {
           TVITEMEXW itemex;
           TVITEMW   item;
        } DUMMYUNIONNAME;
} TVINSERTSTRUCTW, *LPTVINSERTSTRUCTW;

#define TV_INSERTSTRUCT   WINELIB_NAME_AW(TVINSERTSTRUCT)
#define LPTV_INSERTSTRUCT WINELIB_NAME_AW(LPTVINSERTSTRUCT)

#define TVINSERTSTRUCT_V1_SIZEA CCSIZEOF_STRUCT(TVINSERTSTRUCTA, item)
#define TVINSERTSTRUCT_V1_SIZEW CCSIZEOF_STRUCT(TVINSERTSTRUCTW, item)
#define TVINSERTSTRUCT_V1_SIZE    WINELIB_NAME_AW(TVINSERTSTRUCT_V1_SIZE)



typedef struct tagNMTREEVIEWA {
	NMHDR	hdr;
	UINT	action;
	TVITEMA	itemOld;
	TVITEMA	itemNew;
	POINT	ptDrag;
} NMTREEVIEWA, *LPNMTREEVIEWA;

typedef struct tagNMTREEVIEWW {
	NMHDR	hdr;
	UINT	action;
	TVITEMW	itemOld;
	TVITEMW	itemNew;
	POINT	ptDrag;
} NMTREEVIEWW, *LPNMTREEVIEWW;

#define NMTREEVIEW     WINELIB_NAME_AW(NMTREEVIEW)
#define NM_TREEVIEW    WINELIB_NAME_AW(NMTREEVIEW)
#define LPNMTREEVIEW   WINELIB_NAME_AW(LPNMTREEVIEW)

typedef struct tagTVDISPINFOA {
	NMHDR	hdr;
	TVITEMA	item;
} NMTVDISPINFOA, *LPNMTVDISPINFOA;

typedef struct tagTVDISPINFOW {
	NMHDR	hdr;
	TVITEMW	item;
} NMTVDISPINFOW, *LPNMTVDISPINFOW;

#define NMTVDISPINFO            WINELIB_NAME_AW(NMTVDISPINFO)
#define LPNMTVDISPINFO          WINELIB_NAME_AW(LPNMTVDISPINFO)
#define TV_DISPINFO             NMTVDISPINFO

typedef INT (CALLBACK *PFNTVCOMPARE)(LPARAM, LPARAM, LPARAM);

typedef struct tagTVSORTCB
{
	HTREEITEM hParent;
	PFNTVCOMPARE lpfnCompare;
	LPARAM lParam;
} TVSORTCB, *LPTVSORTCB;

#define TV_SORTCB TVSORTCB
#define LPTV_SORTCB LPTVSORTCB

typedef struct tagTVHITTESTINFO {
        POINT pt;
        UINT flags;
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
    INT iLevel;                 /* IE>0x0400 */
} NMTVCUSTOMDRAW, *LPNMTVCUSTOMDRAW;

/* Treeview tooltips */

typedef struct tagNMTVGETINFOTIPA
{
    NMHDR hdr;
    LPSTR pszText;
    INT cchTextMax;
    HTREEITEM hItem;
    LPARAM lParam;
} NMTVGETINFOTIPA, *LPNMTVGETINFOTIPA;

typedef struct tagNMTVGETINFOTIPW
{
    NMHDR hdr;
    LPWSTR pszText;
    INT cchTextMax;
    HTREEITEM hItem;
    LPARAM lParam;
} NMTVGETINFOTIPW, *LPNMTVGETINFOTIPW;



#define TreeView_InsertItemA(hwnd, phdi) \
  (INT)SendMessageA((hwnd), TVM_INSERTITEMA, 0, \
                            (LPARAM)(LPTVINSERTSTRUCTA)(phdi))
#define TreeView_InsertItemW(hwnd,phdi) \
  (INT)SendMessageW((hwnd), TVM_INSERTITEMW, 0, \
                            (LPARAM)(LPTVINSERTSTRUCTW)(phdi))  
#define TreeView_DeleteItem(hwnd, hItem) \
  (BOOL)SendMessageA((hwnd), TVM_DELETEITEM, 0, (LPARAM)(HTREEITEM)(hItem))
#define TreeView_DeleteAllItems(hwnd) \
  (BOOL)SendMessageA((hwnd), TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT)
#define TreeView_Expand(hwnd, hitem, code) \
 (BOOL)SendMessageA((hwnd), TVM_EXPAND, (WPARAM)code, \
	(LPARAM)(HTREEITEM)(hitem))

#define TreeView_GetItemRect(hwnd, hitem, prc, code) \
 (*(HTREEITEM *)prc = (hitem), (BOOL)SendMessageA((hwnd), \
			TVM_GETITEMRECT, (WPARAM)(code), (LPARAM)(RECT *)(prc)))

#define TreeView_GetCount(hwnd) \
    (UINT)SendMessageA((hwnd), TVM_GETCOUNT, 0, 0)
#define TreeView_GetIndent(hwnd) \
    (UINT)SendMessageA((hwnd), TVM_GETINDENT, 0, 0)
#define TreeView_SetIndent(hwnd, indent) \
    (BOOL)SendMessageA((hwnd), TVM_SETINDENT, (WPARAM)indent, 0)

#define TreeView_GetImageList(hwnd, iImage) \
    (HIMAGELIST)SendMessageA((hwnd), TVM_GETIMAGELIST, iImage, 0)

#define TreeView_SetImageList(hwnd, himl, iImage) \
    (HIMAGELIST)SendMessageA((hwnd), TVM_SETIMAGELIST, iImage, \
 (LPARAM)(UINT)(HIMAGELIST)(himl))

#define TreeView_GetNextItem(hwnd, hitem, code) \
    (HTREEITEM)SendMessageA((hwnd), TVM_GETNEXTITEM, (WPARAM)code,\
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
 (UINT)SendMessageA((hwnd), TVM_SELECTITEM, (WPARAM)code, \
(LPARAM)(UINT)(hitem))


#define TreeView_SelectItem(hwnd, hitem) \
		TreeView_Select(hwnd, hitem, TVGN_CARET)
#define TreeView_SelectDropTarget(hwnd, hitem) \
        	TreeView_Select(hwnd, hitem, TVGN_DROPHILITE)
/* FIXME
#define TreeView_SelectSetFirstVisible(hwnd, hitem)  \ 
		TreeView_Select(hwnd, hitem, TVGN_FIRSTVISIBLE)
*/

#define TreeView_GetItemA(hwnd, pitem) \
 (BOOL)SendMessageA((hwnd), TVM_GETITEMA, 0, (LPARAM) (TVITEMA *)(pitem))

#define TreeView_SetItemA(hwnd, pitem) \
 (BOOL)SendMessageA((hwnd), TVM_SETITEMA, 0, (LPARAM)(const TVITEMA *)(pitem)) 

#define TreeView_EditLabel(hwnd, hitem) \
    (HWND)SendMessageA((hwnd), TVM_EDITLABEL, 0, (LPARAM)(HTREEITEM)(hitem))


#define TreeView_GetEditControl(hwnd) \
    (HWND)SendMessageA((hwnd), TVM_GETEDITCONTROL, 0, 0)

#define TreeView_GetVisibleCount(hwnd) \
    (UINT)SendMessageA((hwnd), TVM_GETVISIBLECOUNT, 0, 0)

#define TreeView_HitTest(hwnd, lpht) \
    (HTREEITEM)SendMessageA((hwnd), TVM_HITTEST, 0,\
(LPARAM)(LPTVHITTESTINFO)(lpht))

#define TreeView_CreateDragImage(hwnd, hitem) \
    (HIMAGELIST)SendMessageA((hwnd), TVM_CREATEDRAGIMAGE, 0,\
(LPARAM)(HTREEITEM)(hitem))

#define TreeView_SortChildren(hwnd, hitem, recurse) \
    (BOOL)SendMessageA((hwnd), TVM_SORTCHILDREN, (WPARAM)recurse,\
(LPARAM)(HTREEITEM)(hitem))

#define TreeView_EnsureVisible(hwnd, hitem) \
    (BOOL)SendMessageA((hwnd), TVM_ENSUREVISIBLE, 0, (LPARAM)(UINT)(hitem))

#define TreeView_SortChildrenCB(hwnd, psort, recurse) \
    (BOOL)SendMessageA((hwnd), TVM_SORTCHILDRENCB, (WPARAM)recurse, \
    (LPARAM)(LPTV_SORTCB)(psort))

#define TreeView_EndEditLabelNow(hwnd, fCancel) \
    (BOOL)SendMessageA((hwnd), TVM_ENDEDITLABELNOW, (WPARAM)fCancel, 0)

#define TreeView_GetISearchString(hwndTV, lpsz) \
    (BOOL)SendMessageA((hwndTV), TVM_GETISEARCHSTRING, 0, \
							(LPARAM)(LPTSTR)lpsz)

#define TreeView_SetItemHeight(hwnd,  iHeight) \
    (INT)SendMessageA((hwnd), TVM_SETITEMHEIGHT, (WPARAM)iHeight, 0)
#define TreeView_GetItemHeight(hwnd) \
    (INT)SendMessageA((hwnd), TVM_GETITEMHEIGHT, 0, 0)

#define TreeView_SetBkColor(hwnd, clr) \
    (COLORREF)SendMessageA((hwnd), TVM_SETBKCOLOR, 0, (LPARAM)clr)

#define TreeView_SetTextColor(hwnd, clr) \
    (COLORREF)SendMessageA((hwnd), TVM_SETTEXTCOLOR, 0, (LPARAM)clr)

#define TreeView_GetBkColor(hwnd) \
    (COLORREF)SendMessageA((hwnd), TVM_GETBKCOLOR, 0, 0)

#define TreeView_GetTextColor(hwnd) \
    (COLORREF)SendMessageA((hwnd), TVM_GETTEXTCOLOR, 0, 0)

#define TreeView_SetScrollTime(hwnd, uTime) \
    (UINT)SendMessageA((hwnd), TVM_SETSCROLLTIME, uTime, 0)

#define TreeView_GetScrollTime(hwnd) \
    (UINT)SendMessageA((hwnd), TVM_GETSCROLLTIME, 0, 0)

#define TreeView_SetInsertMarkColor(hwnd, clr) \
    (COLORREF)SendMessageA((hwnd), TVM_SETINSERTMARKCOLOR, 0, (LPARAM)clr)
#define TreeView_GetInsertMarkColor(hwnd) \
    (COLORREF)SendMessageA((hwnd), TVM_GETINSERTMARKCOLOR, 0, 0)









/* Listview control */

#define WC_LISTVIEWA          "SysListView32"
#define WC_LISTVIEWW          L"SysListView32"
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

#define LVFI_PARAM              0X0001
#define LVFI_STRING             0X0002
#define LVFI_PARTIAL            0X0008
#define LVFI_WRAP               0X0020  
#define LVFI_NEARESTXY          0X0040

#define LVIF_TEXT               0x0001
#define LVIF_IMAGE              0x0002
#define LVIF_PARAM              0x0004
#define LVIF_STATE              0x0008
#define LVIF_INDENT             0x0010
#define LVIF_NORECOMPUTE        0x0800
#define LVIF_DI_SETITEM         0x1000

#define LVIR_BOUNDS             0x0000
#define LVIR_LABEL              0x0002
#define LVIR_ICON               0x0001
#define LVIR_SELECTBOUNDS       0x0003 

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
#define LVM_GETITEMA          (LVM_FIRST+5)
#define LVM_GETITEMW          (LVM_FIRST+75)
#define LVM_GETITEM             WINELIB_NAME_AW(LVM_GETITEM)
#define LVM_SETITEMA          (LVM_FIRST+6)
#define LVM_SETITEMW          (LVM_FIRST+76)
#define LVM_SETITEM             WINELIB_NAME_AW(LVM_SETITEM)
#define LVM_INSERTITEMA       (LVM_FIRST+7)
#define LVM_INSERTITEMW       (LVM_FIRST+77)
#define LVM_INSERTITEM          WINELIB_NAME_AW(LVM_INSERTITEM)
#define LVM_DELETEITEM          (LVM_FIRST+8)
#define LVM_DELETEALLITEMS      (LVM_FIRST+9)
#define LVM_GETCALLBACKMASK     (LVM_FIRST+10)
#define LVM_SETCALLBACKMASK     (LVM_FIRST+11)
#define LVM_GETNEXTITEM         (LVM_FIRST+12)
#define LVM_FINDITEMA         (LVM_FIRST+13)
#define LVM_FINDITEMW         (LVM_FIRST+83)
#define LVM_FINDITEM            WINELIB_NAME_AW(LVM_FINDITEM)
#define LVM_GETITEMRECT         (LVM_FIRST+14)
#define LVM_SETITEMPOSITION     (LVM_FIRST+15)
#define LVM_GETITEMPOSITION     (LVM_FIRST+16)
#define LVM_GETSTRINGWIDTHA   (LVM_FIRST+17)
#define LVM_GETSTRINGWIDTHW   (LVM_FIRST+87)
#define LVM_GETSTRINGWIDTH      WINELIB_NAME_AW(LVM_GETSTRINGWIDTH)
#define LVM_HITTEST             (LVM_FIRST+18)
#define LVM_ENSUREVISIBLE       (LVM_FIRST+19)
#define LVM_SCROLL              (LVM_FIRST+20)
#define LVM_REDRAWITEMS         (LVM_FIRST+21)
#define LVM_ARRANGE             (LVM_FIRST+22)
#define LVM_EDITLABELA        (LVM_FIRST+23)
#define LVM_EDITLABELW        (LVM_FIRST+118)
#define LVM_EDITLABEL           WINELIB_NAME_AW(LVM_EDITLABEL)
#define LVM_GETEDITCONTROL      (LVM_FIRST+24)
#define LVM_GETCOLUMNA        (LVM_FIRST+25)
#define LVM_GETCOLUMNW        (LVM_FIRST+95)
#define LVM_GETCOLUMN           WINELIB_NAME_AW(LVM_GETCOLUMN)
#define LVM_SETCOLUMNA        (LVM_FIRST+26)
#define LVM_SETCOLUMNW        (LVM_FIRST+96)
#define LVM_SETCOLUMN           WINELIB_NAME_AW(LVM_SETCOLUMN)
#define LVM_INSERTCOLUMNA     (LVM_FIRST+27)
#define LVM_INSERTCOLUMNW     (LVM_FIRST+97)
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
#define LVM_GETITEMTEXTA      (LVM_FIRST+45)
#define LVM_GETITEMTEXTW      (LVM_FIRST+115)
#define LVM_GETITEMTEXT         WINELIB_NAME_AW(LVM_GETITEMTEXT)
#define LVM_SETITEMTEXTA      (LVM_FIRST+46)
#define LVM_SETITEMTEXTW      (LVM_FIRST+116)
#define LVM_SETITEMTEXT         WINELIB_NAME_AW(LVM_SETITEMTEXT)
#define LVM_SETITEMCOUNT        (LVM_FIRST+47)
#define LVM_SORTITEMS           (LVM_FIRST+48)
#define LVM_SETITEMPOSITION32   (LVM_FIRST+49)
#define LVM_GETSELECTEDCOUNT    (LVM_FIRST+50)
#define LVM_GETITEMSPACING      (LVM_FIRST+51)
#define LVM_GETISEARCHSTRINGA (LVM_FIRST+52)
#define LVM_GETISEARCHSTRINGW (LVM_FIRST+117)
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
#define LVM_SETBKIMAGEA       (LVM_FIRST+68)
#define LVM_SETBKIMAGEW       (LVM_FIRST+138)
#define LVM_SETBKIMAGE          WINELIB_NAME_AW(LVM_SETBKIMAGE)
#define LVM_GETBKIMAGEA       (LVM_FIRST+69)
#define LVM_GETBKIMAGEW       (LVM_FIRST+139)
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
#define LVN_BEGINLABELEDITA   (LVN_FIRST-5)
#define LVN_BEGINLABELEDITW   (LVN_FIRST-75)
#define LVN_BEGINLABELEDIT WINELIB_NAME_AW(LVN_BEGINLABELEDIT)
#define LVN_ENDLABELEDITA     (LVN_FIRST-6)
#define LVN_ENDLABELEDITW     (LVN_FIRST-76)
#define LVN_ENDLABELEDIT WINELIB_NAME_AW(LVN_ENDLABELEDIT)
#define LVN_COLUMNCLICK         (LVN_FIRST-8)
#define LVN_BEGINDRAG           (LVN_FIRST-9)
#define LVN_BEGINRDRAG          (LVN_FIRST-11)
#define LVN_ODCACHEHINT         (LVN_FIRST-13)
#define LVN_ODFINDITEMA       (LVN_FIRST-52)
#define LVN_ODFINDITEMW       (LVN_FIRST-79)
#define LVN_ODFINDITEM WINELIB_NAME_AW(LVN_ODFINDITEM)
#define LVN_ITEMACTIVATE        (LVN_FIRST-14)
#define LVN_ODSTATECHANGED      (LVN_FIRST-15)
#define LVN_HOTTRACK            (LVN_FIRST-21)
#define LVN_GETDISPINFOA      (LVN_FIRST-50)
#define LVN_GETDISPINFOW      (LVN_FIRST-77)
#define LVN_GETDISPINFO WINELIB_NAME_AW(LVN_GETDISPINFO)
#define LVN_SETDISPINFOA      (LVN_FIRST-51)
#define LVN_SETDISPINFOW      (LVN_FIRST-78)
#define LVN_SETDISPINFO WINELIB_NAME_AW(LVN_SETDISPINFO)

#define LVA_ALIGNLEFT           0x0000
#define LVA_DEFAULT             0x0001
#define LVA_ALIGNTOP            0x0002
#define LVA_SNAPTOGRID          0x0005

typedef struct tagLVITEMA
{
    UINT mask;
    INT  iItem;
    INT  iSubItem;
    UINT state;
    UINT stateMask;
    LPSTR  pszText;
    INT  cchTextMax;
    INT  iImage;
    LPARAM lParam;
    INT  iIndent;	/* (_WIN32_IE >= 0x0300) */
} LVITEMA, *LPLVITEMA;

typedef struct tagLVITEMW
{
    UINT mask;
    INT  iItem;
    INT  iSubItem;
    UINT state;
    UINT stateMask;
    LPWSTR pszText;
    INT  cchTextMax;
    INT  iImage;
    LPARAM lParam;
    INT  iIndent;	/* (_WIN32_IE >= 0x0300) */
} LVITEMW, *LPLVITEMW;

#define LVITEM   WINELIB_NAME_AW(LVITEM)
#define LPLVITEM WINELIB_NAME_AW(LPLVITEM)

#define LVITEM_V1_SIZEA CCSIZEOF_STRUCT(LVITEMA, lParam)
#define LVITEM_V1_SIZEW CCSIZEOF_STRUCT(LVITEMW, lParam)
#define LVITEM_V1_SIZE WINELIB_NAME_AW(LVITEM_V1_SIZE)

#define LV_ITEM LVITEM

typedef struct tagLVCOLUMNA
{
    UINT mask;
    INT  fmt;
    INT  cx;
    LPSTR  pszText;
    INT  cchTextMax;
    INT  iSubItem;
    INT  iImage;  /* (_WIN32_IE >= 0x0300) */
    INT  iOrder;  /* (_WIN32_IE >= 0x0300) */
} LVCOLUMNA, *LPLVCOLUMNA;

typedef struct tagLVCOLUMNW
{
    UINT mask;
    INT  fmt;
    INT  cx;
    LPWSTR pszText;
    INT  cchTextMax;
    INT  iSubItem;
    INT  iImage;	/* (_WIN32_IE >= 0x0300) */
    INT  iOrder;	/* (_WIN32_IE >= 0x0300) */
} LVCOLUMNW, *LPLVCOLUMNW;

#define LVCOLUMN   WINELIB_NAME_AW(LVCOLUMN)
#define LPLVCOLUMN WINELIB_NAME_AW(LPLVCOLUMN)

#define LVCOLUMN_V1_SIZEA CCSIZEOF_STRUCT(LVCOLUMNA, iSubItem)
#define LVCOLUMN_V1_SIZEW CCSIZEOF_STRUCT(LVCOLUMNW, iSubItem)
#define LVCOLUMN_V1_SIZE WINELIB_NAME_AW(LVCOLUMN_V1_SIZE)

#define LV_COLUMN       LVCOLUMN


typedef struct tagNMLISTVIEW
{
    NMHDR   hdr;
    INT   iItem;
    INT   iSubItem;
    UINT  uNewState;
    UINT  uOldState;
    UINT  uChanged;
    POINT ptAction;
    LPARAM  lParam;
} NMLISTVIEW, *LPNMLISTVIEW;

#define LPNM_LISTVIEW   LPNMLISTVIEW
#define NM_LISTVIEW     NMLISTVIEW


typedef struct tagLVDISPINFO
{
    NMHDR     hdr;
    LVITEMA item;
} NMLVDISPINFOA, *LPNMLVDISPINFOA;

typedef struct tagLVDISPINFOW
{
    NMHDR     hdr;
    LVITEMW item;
} NMLVDISPINFOW, *LPNMLVDISPINFOW;

#define NMLVDISPINFO   WINELIB_NAME_AW(NMLVDISPINFO)
#define LPNMLVDISPINFO WINELIB_NAME_AW(LPNMLVDISPINFO)

#define LV_DISPINFO     NMLVDISPINFO

#define LVN_KEYDOWN     (LVN_FIRST-55)

typedef struct tagLVKEYDOWN
{
  NMHDR hdr;
  WORD  wVKey;
  UINT flags;
} NMLVKEYDOWN, *LPNMLVKEYDOWN;

#define LV_KEYDOWN     NMLVKEYDOWN

typedef struct tagLVHITTESTINFO
{
    POINT pt;
    UINT  flags;
    INT   iItem;
    INT   iSubItem;
} LVHITTESTINFO, *LPLVHITTESTINFO;

#define LV_HITTESTINFO LVHITTESTINFO
#define _LV_HITTESTINFO tagLVHITTESTINFO
#define LVHITTESTINFO_V1_SIZE CCSIZEOF_STRUCT(LVHITTESTINFO,iItem)

typedef struct tagLVFINDINFO
{
	UINT flags;
	LPCSTR psz;
	LPARAM lParam;
	POINT pt;
	UINT vkDirection;
} LVFINDINFO, *LPLVFINDINFO;

#define LV_FINDINFO LVFINDINFO

typedef struct tagTCHITTESTINFO
{
	POINT pt;
	UINT flags;
} TCHITTESTINFO, *LPTCHITTESTINFO;

#define TC_HITTESTINFO TCHITTESTINFO

typedef INT (CALLBACK *PFNLVCOMPARE)(LPARAM, LPARAM, LPARAM);

#define ListView_GetNextItem(hwnd,nItem,flags) \
    (INT)SendMessageA((hwnd),LVM_GETNEXTITEM,(WPARAM)(INT)(nItem),(LPARAM)(MAKELPARAM(flags,0)))
#define ListView_FindItem(hwnd,nItem,plvfi) \
    (INT)SendMessageA((hwnd),LVM_FINDITEMA,(WPARAM)(INT)(nItem),(LPARAM)(LVFINDINFO*)(plvfi))
#define ListView_Arrange(hwnd,code) \
    (INT)SendMessageA((hwnd),LVM_ARRANGE,(WPARAM)(INT)(code),0L)
#define ListView_GetItemPosition(hwnd,i,ppt) \
    (INT)SendMessageA((hwnd),LVM_GETITEMPOSITION,(WPARAM)(INT)(i),(LPARAM)(LPPOINT)(ppt))
#define ListView_GetItemRect(hwnd,i,prc) \
    (INT)SendMessageA((hwnd),LVM_GETITEMRECT,(WPARAM)(INT)(i),(LPARAM)(LPRECT)(prc))
#define ListView_SetItemA(hwnd,pitem) \
    (INT)SendMessageA((hwnd),LVM_SETITEMA,0,(LPARAM)(const LVITEMA *)(pitem))
#define ListView_SetItemState(hwnd,i,pitem) \
    (BOOL)SendMessageA((hwnd),LVM_SETITEMSTATE,(WPARAM)(UINT)(i),(LPARAM)(LPLVITEMA)(pitem))
#define ListView_GetItemState(hwnd,i,mask) \
    (BOOL)SendMessageA((hwnd),LVM_GETITEMSTATE,(WPARAM)(UINT)(i),(LPARAM)(UINT)(mask))
#define ListView_GetCountPerPage(hwnd) \
    (BOOL)SendMessageW((hwnd),LVM_GETCOUNTPERPAGE,0,0L)
#define ListView_GetImageList(hwnd,iImageList) \
    (HIMAGELIST)SendMessageA((hwnd),LVM_GETIMAGELIST,(WPARAM)(INT)(iImageList),0L)
#define ListView_GetStringWidthA(hwnd,pstr) \
    (INT)SendMessageA((hwnd),LVM_GETSTRINGWIDTHA,0,(LPARAM)(LPCSTR)(pstr))
#define ListView_GetTopIndex(hwnd) \
    (BOOL)SendMessageA((hwnd),LVM_GETTOPINDEX,0,0L)
#define ListView_Scroll(hwnd,dx,dy) \
    (BOOL)SendMessageA((hwnd),LVM_SCROLL,(WPARAM)(INT)(dx),(LPARAM)(INT)(dy))
#define ListView_EnsureVisible(hwnd,i,fPartialOk) \
    (BOOL)SendMessageA((hwnd),LVM_ENSUREVISIBLE,(WPARAM)(INT)i,(LPARAM)(BOOL)fPartialOk)
#define ListView_SetBkColor(hwnd,clrBk) \
    (BOOL)SendMessageA((hwnd),LVM_SETBKCOLOR,0,(LPARAM)(COLORREF)(clrBk))
#define ListView_SetImageList(hwnd,himl,iImageList) \
    (HIMAGELIST)(UINT)SendMessageA((hwnd),LVM_SETIMAGELIST,(WPARAM)(iImageList),(LPARAM)(UINT)(HIMAGELIST)(himl))
#define ListView_GetItemCount(hwnd) \
    (INT)SendMessageA((hwnd),LVM_GETITEMCOUNT,0,0L)
#define ListView_GetItemA(hwnd,pitem) \
    (BOOL)SendMessageA((hwnd),LVM_GETITEMA,0,(LPARAM)(LVITEMA *)(pitem))
#define ListView_GetItemW(hwnd,pitem) \
    (BOOL)SendMessageW((hwnd),LVM_GETITEMW,0,(LPARAM)(LVITEMW *)(pitem))
#define ListView_GetItem WINELIB_NAME_AW(ListView_GetItem)
#define ListView_HitTest(hwnd,pinfo) \
    (INT)SendMessageA((hwnd),LVM_HITTEST,0,(LPARAM)(LPLVHITTESTINFO)(pinfo))
#define ListView_InsertItemA(hwnd,pitem) \
    (INT)SendMessageA((hwnd),LVM_INSERTITEMA,0,(LPARAM)(const LVITEMA *)(pitem))
#define ListView_InsertItemW(hwnd,pitem) \
    (INT)SendMessageW((hwnd),LVM_INSERTITEMW,0,(LPARAM)(const LVITEMW *)(pitem))
#define ListView_InsertItem WINELIB_NAME_AW(ListView_InsertItem)
#define ListView_DeleteAllItems(hwnd) \
    (BOOL)SendMessageA((hwnd),LVM_DELETEALLITEMS,0,0L)
#define ListView_InsertColumnA(hwnd,iCol,pcol) \
    (INT)SendMessageA((hwnd),LVM_INSERTCOLUMNA,(WPARAM)(INT)(iCol),(LPARAM)(const LVCOLUMNA *)(pcol))
#define ListView_InsertColumnW(hwnd,iCol,pcol) \
    (INT)SendMessageW((hwnd),LVM_INSERTCOLUMNW,(WPARAM)(INT)(iCol),(LPARAM)(const LVCOLUMNW *)(pcol))
#define ListView_InsertColumn WINELIB_NAME_AW(ListView_InsertColumn)
#define ListView_SortItems(hwndLV,_pfnCompare,_lPrm) \
    (BOOL)SendMessageA((hwndLV),LVM_SORTITEMS,(WPARAM)(LPARAM)_lPrm,(LPARAM)(PFNLVCOMPARE)_pfnCompare)
#define ListView_SetItemPosition(hwndLV, i, x, y) \
    (BOOL)SendMessageA((hwndLV),LVM_SETITEMPOSITION,(WPARAM)(INT)(i),MAKELPARAM((x),(y)))
#define ListView_GetSelectedCount(hwndLV) \
    (UINT)SendMessageA((hwndLV),LVM_GETSELECTEDCOUNT,0,0L)


/* Tab Control */

#define WC_TABCONTROL16		 "SysTabControl"
#define WC_TABCONTROLA	 "SysTabControl32"
#define WC_TABCONTROLW	L"SysTabControl32"

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
#define TCM_GETITEMA			(TCM_FIRST + 5)
#define TCM_GETITEMW			(TCM_FIRST + 60)
#define TCM_SETITEMA			(TCM_FIRST + 6)
#define TCM_SETITEMW			(TCM_FIRST + 61)
#define TCM_SETITEM				WINELIB_NAME_AW(TCM_SETITEM)
#define TCM_INSERTITEMA		(TCM_FIRST + 7)
#define TCM_INSERTITEMW		(TCM_FIRST + 62)
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

/* TabCtrl Macros */
#define TabCtrl_GetImageList(hwnd) \
    (HIMAGELIST)SendMessageA((hwnd), TCM_GETIMAGELIST, 0, 0L)
#define TabCtrl_SetImageList(hwnd, himl) \
    (HIMAGELIST)SendMessageA((hwnd), TCM_SETIMAGELIST, 0, (LPARAM)(UINT)(HIMAGELIST)(himl))
#define TabCtrl_GetItemCount(hwnd) \
    (int)SendMessageA((hwnd), TCM_GETITEMCOUNT, 0, 0L)
#define TabCtrl_GetItemA(hwnd, iItem, pitem) \
    (BOOL)SendMessageA((hwnd), TCM_GETITEM, (WPARAM)(int)iItem, (LPARAM)(TC_ITEM *)(pitem))
#define TabCtrl_GetItemW(hwnd, iItem, pitem) \
    (BOOL)SendMessageW((hwnd), TCM_GETITEM, (WPARAM)(int)iItem, (LPARAM)(TC_ITEM *)(pitem))
#define TabCtrl_GetItem WINELIB_NAME_AW(TabCtrl_GetItem)
#define TabCtrl_SetItemA(hwnd, iItem, pitem) \
    (BOOL)SendMessageA((hwnd), TCM_SETITEM, (WPARAM)(int)iItem, (LPARAM)(TC_ITEM *)(pitem))
#define TabCtrl_SetItemW(hwnd, iItem, pitem) \
    (BOOL)SendMessageW((hwnd), TCM_SETITEM, (WPARAM)(int)iItem, (LPARAM)(TC_ITEM *)(pitem))
#define TabCtrl_SetItem WINELIB_NAME_AW(TabCtrl_GetItem)
#define TabCtrl_InsertItemA(hwnd, iItem, pitem)   \
    (int)SendMessageA((hwnd), TCM_INSERTITEM, (WPARAM)(int)iItem, (LPARAM)(const TC_ITEM *)(pitem))
#define TabCtrl_InsertItemW(hwnd, iItem, pitem)   \
    (int)SendMessageW((hwnd), TCM_INSERTITEM, (WPARAM)(int)iItem, (LPARAM)(const TC_ITEM *)(pitem))
#define TabCtrl_InsertItem WINELIB_NAME_AW(TabCtrl_InsertItem)
#define TabCtrl_DeleteItem(hwnd, i) \
    (BOOL)SendMessageA((hwnd), TCM_DELETEITEM, (WPARAM)(int)(i), 0L)
#define TabCtrl_DeleteAllItems(hwnd) \
    (BOOL)SendMessageA((hwnd), TCM_DELETEALLITEMS, 0, 0L)
#define TabCtrl_GetItemRect(hwnd, i, prc) \
    (BOOL)SendMessageA((hwnd), TCM_GETITEMRECT, (WPARAM)(int)(i), (LPARAM)(RECT *)(prc))
#define TabCtrl_GetCurSel(hwnd) \
    (int)SendMessageA((hwnd), TCM_GETCURSEL, 0, 0)
#define TabCtrl_SetCurSel(hwnd, i) \
    (int)SendMessageA((hwnd), TCM_SETCURSEL, (WPARAM)i, 0)
#define TabCtrl_HitTest(hwndTC, pinfo) \
    (int)SendMessageA((hwndTC), TCM_HITTEST, 0, (LPARAM)(TC_HITTESTINFO *)(pinfo))
#define TabCtrl_SetItemExtra(hwndTC, cb) \
    (BOOL)SendMessageA((hwndTC), TCM_SETITEMEXTRA, (WPARAM)(cb), 0L)
#define TabCtrl_AdjustRect(hwnd, bLarger, prc) \
    (int)SendMessageA(hwnd, TCM_ADJUSTRECT, (WPARAM)(BOOL)bLarger, (LPARAM)(RECT *)prc)
#define TabCtrl_SetItemSize(hwnd, x, y) \
    (DWORD)SendMessageA((hwnd), TCM_SETITEMSIZE, 0, MAKELPARAM(x,y))
#define TabCtrl_RemoveImage(hwnd, i) \
    (void)SendMessageA((hwnd), TCM_REMOVEIMAGE, i, 0L)
#define TabCtrl_SetPadding(hwnd,  cx, cy) \
    (void)SendMessageA((hwnd), TCM_SETPADDING, 0, MAKELPARAM(cx, cy))
#define TabCtrl_GetRowCount(hwnd) \
    (int)SendMessageA((hwnd), TCM_GETROWCOUNT, 0, 0L)
#define TabCtrl_GetToolTips(hwnd) \
    (HWND)SendMessageA((hwnd), TCM_GETTOOLTIPS, 0, 0L)
#define TabCtrl_SetToolTips(hwnd, hwndTT) \
    (void)SendMessageA((hwnd), TCM_SETTOOLTIPS, (WPARAM)hwndTT, 0L)
#define TabCtrl_GetCurFocus(hwnd) \
    (int)SendMessageA((hwnd), TCM_GETCURFOCUS, 0, 0)
#define TabCtrl_SetCurFocus(hwnd, i) \
    SendMessageA((hwnd),TCM_SETCURFOCUS, i, 0)
#define TabCtrl_SetMinTabWidth(hwnd, x) \
    (int)SendMessageA((hwnd), TCM_SETMINTABWIDTH, 0, x)
#define TabCtrl_DeselectAll(hwnd, fExcludeFocus)\
    (void)SendMessageA((hwnd), TCM_DESELECTALL, fExcludeFocus, 0)


/* constants for TCHITTESTINFO */

#define TCHT_NOWHERE      0x01
#define TCHT_ONITEMICON   0x02
#define TCHT_ONITEMLABEL  0x04
#define TCHT_ONITEM       (TCHT_ONITEMICON | TCHT_ONITEMLABEL)


typedef struct tagTCITEMA {
    UINT mask;
    UINT dwState;
    UINT dwStateMask;
    LPSTR  pszText;
    INT  cchTextMax;
    INT  iImage;
    LPARAM lParam;
} TCITEMA, *LPTCITEMA;

typedef struct tagTCITEMW
{
    UINT mask;
    DWORD  dwState;
    DWORD  dwStateMask;
    LPWSTR pszText;
    INT  cchTextMax;
    INT  iImage;
    LPARAM lParam;
} TCITEMW, *LPTCITEMW;

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

#define WC_COMBOBOXEXA        "ComboBoxEx32"
#define WC_COMBOBOXEXW        L"ComboBoxEx32"
#define WC_COMBOBOXEX           WINELIB_NAME_AW(WC_COMBOBOXEX)

#define CBEM_INSERTITEMA      (WM_USER+1)
#define CBEM_INSERTITEMW      (WM_USER+11)
#define CBEM_INSERTITEM         WINELIB_NAME_AW(CBEM_INSERTITEM)
#define CBEM_SETIMAGELIST       (WM_USER+2)
#define CBEM_GETIMAGELIST       (WM_USER+3)
#define CBEM_GETITEMA         (WM_USER+4)
#define CBEM_GETITEMW         (WM_USER+13)
#define CBEM_GETITEM            WINELIB_NAME_AW(CBEM_GETITEM)
#define CBEM_SETITEMA         (WM_USER+5)
#define CBEM_SETITEMW         (WM_USER+12)
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
#define HOTKEY_CLASSA         "msctls_hotkey32"
#define HOTKEY_CLASSW         L"msctls_hotkey32"
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

#define ANIMATE_CLASSA        "SysAnimate32"
#define ANIMATE_CLASSW        L"SysAnimate32"
#define ANIMATE_CLASS           WINELIB_NAME_AW(ANIMATE_CLASS)

#define ACS_CENTER              0x0001
#define ACS_TRANSPARENT         0x0002
#define ACS_AUTOPLAY            0x0004
#define ACS_TIMER               0x0008  /* no threads, just timers */

#define ACM_OPENA             (WM_USER+100)
#define ACM_OPENW             (WM_USER+103)
#define ACM_OPEN                WINELIB_NAME_AW(ACM_OPEN)
#define ACM_PLAY                (WM_USER+101)
#define ACM_STOP                (WM_USER+102)

#define ACN_START               1
#define ACN_STOP                2

#define Animate_CreateA(hwndP,id,dwStyle,hInstance) \
    CreateWindowA(ANIMATE_CLASSA,NULL,dwStyle,0,0,0,0,hwndP,(HMENU)(id),hInstance,NULL)
#define Animate_CreateW(hwndP,id,dwStyle,hInstance) \
    CreateWindowW(ANIMATE_CLASSW,NULL,dwStyle,0,0,0,0,hwndP,(HMENU)(id),hInstance,NULL)
#define Animate_Create WINELIB_NAME_AW(Animate_Create)
#define Animate_OpenA(hwnd,szName) \
    (BOOL)SendMessageA(hwnd,ACM_OPENA,0,(LPARAM)(LPSTR)(szName))
#define Animate_OpenW(hwnd,szName) \
    (BOOL)SendMessageW(hwnd,ACM_OPENW,0,(LPARAM)(LPWSTR)(szName))
#define Animate_Open WINELIB_NAME_AW(Animate_Open)
#define Animate_OpenExA(hwnd,hInst,szName) \
    (BOOL)SendMessageA(hwnd,ACM_OPENA,(WPARAM)hInst,(LPARAM)(LPSTR)(szName))
#define Animate_OpenExW(hwnd,hInst,szName) \
    (BOOL)SendMessageW(hwnd,ACM_OPENW,(WPARAM)hInst,(LPARAM)(LPWSTR)(szName))
#define Animate_OpenEx WINELIB_NAME_AW(Animate_OpenEx)
#define Animate_Play(hwnd,from,to,rep) \
    (BOOL)SendMessageA(hwnd,ACM_PLAY,(WPARAM)(UINT)(rep),(LPARAM)MAKELONG(from,to))
#define Animate_Stop(hwnd) \
    (BOOL)SendMessageA(hwnd,ACM_STOP,0,0)
#define Animate_Close(hwnd) \
    (BOOL)SendMessageA(hwnd,ACM_OPENA,0,0)
#define Animate_Seek(hwnd,frame) \
    (BOOL)SendMessageA(hwnd,ACM_PLAY,1,(LPARAM)MAKELONG(frame,frame))


/**************************************************************************
 * IP Address control
 */

#define WC_IPADDRESSA		"SysIPAddress32"
#define WC_IPADDRESSW		L"SysIPAddress32"
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
    INT iField;
    INT iValue;
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

#define WC_NATIVEFONTCTLA	"NativeFontCtl"
#define WC_NATIVEFONTCTLW	L"NativeFontCtl"
#define WC_NATIVEFONTCTL	WINELIB_NAME_AW(WC_NATIVEFONTCTL)

#define NFS_EDIT		0x0001
#define NFS_STATIC		0x0002
#define NFS_LISTCOMBO		0x0004
#define NFS_BUTTON		0x0008
#define NFS_ALL			0x0010


/**************************************************************************
 * Month calendar control
 *
 */

#define MONTHCAL_CLASSA	"SysMonthCal32"
#define MONTHCAL_CLASSW	L"SysMonthCal32"
#define MONTHCAL_CLASS		WINELIB_NAME_AW(MONTHCAL_CLASS)

#define MCM_FIRST             0x1000
#define MCN_FIRST             (0U-750U)
#define MCN_LAST              (0U-759U)


#define MCM_GETCURSEL         (MCM_FIRST + 1)
#define MCM_SETCURSEL         (MCM_FIRST + 2)
#define MCM_GETMAXSELCOUNT    (MCM_FIRST + 3)
#define MCM_SETMAXSELCOUNT    (MCM_FIRST + 4)
#define MCM_GETSELRANGE       (MCM_FIRST + 5)
#define MCM_SETSELRANGE       (MCM_FIRST + 6)
#define MCM_GETMONTHRANGE     (MCM_FIRST + 7)
#define MCM_SETDAYSTATE       (MCM_FIRST + 8)
#define MCM_GETMINREQRECT     (MCM_FIRST + 9)
#define MCM_SETCOLOR          (MCM_FIRST + 10)
#define MCM_GETCOLOR          (MCM_FIRST + 11)
#define MCM_SETTODAY          (MCM_FIRST + 12)
#define MCM_GETTODAY          (MCM_FIRST + 13)
#define MCM_HITTEST           (MCM_FIRST + 14)
#define MCM_SETFIRSTDAYOFWEEK (MCM_FIRST + 15)
#define MCM_GETFIRSTDAYOFWEEK (MCM_FIRST + 16)
#define MCM_GETRANGE          (MCM_FIRST + 17)
#define MCM_SETRANGE          (MCM_FIRST + 18)
#define MCM_GETMONTHDELTA     (MCM_FIRST + 19)
#define MCM_SETMONTHDELTA     (MCM_FIRST + 20)
#define MCM_GETMAXTODAYWIDTH  (MCM_FIRST + 21)
#define MCM_GETUNICODEFORMAT   CCM_GETUNICODEFORMAT
#define MCM_SETUNICODEFORMAT   CCM_SETUNICODEFORMAT


/* Notifications */

#define MCN_SELCHANGE         (MCN_FIRST + 1)
#define MCN_GETDAYSTATE       (MCN_FIRST + 3)
#define MCN_SELECT            (MCN_FIRST + 4)

#define MCSC_BACKGROUND   0   
#define MCSC_TEXT         1   
#define MCSC_TITLEBK      2   
#define MCSC_TITLETEXT    3
#define MCSC_MONTHBK      4   
#define MCSC_TRAILINGTEXT 5   

#define MCS_DAYSTATE           0x0001
#define MCS_MULTISELECT        0x0002
#define MCS_WEEKNUMBERS        0x0004
#define MCS_NOTODAY            0x0010
#define MCS_NOTODAYCIRCLE      0x0008

#define MCHT_TITLE             0x00010000
#define MCHT_CALENDAR          0x00020000
#define MCHT_TODAYLINK         0x00030000

#define MCHT_NEXT              0x01000000   
#define MCHT_PREV              0x02000000  
#define MCHT_NOWHERE           0x00000000
#define MCHT_TITLEBK           (MCHT_TITLE)
#define MCHT_TITLEMONTH        (MCHT_TITLE | 0x0001)
#define MCHT_TITLEYEAR         (MCHT_TITLE | 0x0002)
#define MCHT_TITLEBTNNEXT      (MCHT_TITLE | MCHT_NEXT | 0x0003)
#define MCHT_TITLEBTNPREV      (MCHT_TITLE | MCHT_PREV | 0x0003)

#define MCHT_CALENDARBK        (MCHT_CALENDAR)
#define MCHT_CALENDARDATE      (MCHT_CALENDAR | 0x0001)
#define MCHT_CALENDARDATENEXT  (MCHT_CALENDARDATE | MCHT_NEXT)
#define MCHT_CALENDARDATEPREV  (MCHT_CALENDARDATE | MCHT_PREV)
#define MCHT_CALENDARDAY       (MCHT_CALENDAR | 0x0002)
#define MCHT_CALENDARWEEKNUM   (MCHT_CALENDAR | 0x0003)



#define GMR_VISIBLE     0      
#define GMR_DAYSTATE    1     


/*  Month calendar's structures */


typedef struct {
        UINT cbSize;
        POINT pt;
        UINT uHit;   
        SYSTEMTIME st;
} MCHITTESTINFO, *PMCHITTESTINFO;

typedef struct tagNMSELCHANGE
{
    NMHDR           nmhdr; 
    SYSTEMTIME      stSelStart;
    SYSTEMTIME      stSelEnd;
} NMSELCHANGE, *LPNMSELCHANGE;

typedef NMSELCHANGE NMSELECT, *LPNMSELECT;
typedef DWORD MONTHDAYSTATE, *LPMONTHDAYSTATE;

typedef struct tagNMDAYSTATE
{
    NMHDR           nmhdr;  
    SYSTEMTIME      stStart;
    int             cDayState;
    LPMONTHDAYSTATE prgDayState;
} NMDAYSTATE, *LPNMDAYSTATE;


/* macros */

#define MonthCal_GetCurSel(hmc, pst) \
		(BOOL)SendMessageA(hmc, MCM_GETCURSEL, 0, (LPARAM)(pst))
#define MonthCal_SetCurSel(hmc, pst)  \
		(BOOL)SendMessageA(hmc, MCM_SETCURSEL, 0, (LPARAM)(pst))
#define MonthCal_GetMaxSelCount(hmc) \
		(DWORD)SendMessageA(hmc, MCM_GETMAXSELCOUNT, 0, 0L)
#define MonthCal_SetMaxSelCount(hmc, n) \
		(BOOL)SendMessageA(hmc, MCM_SETMAXSELCOUNT, (WPARAM)(n), 0L)
#define MonthCal_GetSelRange(hmc, rgst) \
		SendMessageA(hmc, MCM_GETSELRANGE, 0, (LPARAM) (rgst))
#define MonthCal_SetSelRange(hmc, rgst) \
		SendMessageA(hmc, MCM_SETSELRANGE, 0, (LPARAM) (rgst))
#define MonthCal_GetMonthRange(hmc, gmr, rgst) \
		(DWORD)SendMessageA(hmc, MCM_GETMONTHRANGE, (WPARAM)(gmr), (LPARAM)(rgst))
#define MonthCal_SetDayState(hmc, cbds, rgds) \
		SendMessageA(hmc, MCM_SETDAYSTATE, (WPARAM)(cbds), (LPARAM)(rgds))
#define MonthCal_GetMinReqRect(hmc, prc) \
		SendMessageA(hmc, MCM_GETMINREQRECT, 0, (LPARAM)(prc))
#define MonthCal_SetColor(hmc, iColor, clr)\
		SendMessageA(hmc, MCM_SETCOLOR, iColor, clr
#define MonthCal_GetColor(hmc, iColor) \
		SendMessageA(hmc, MCM_SETCOLOR, iColor, 0)
#define MonthCal_GetToday(hmc, pst)\
		(BOOL)SendMessageA(hmc, MCM_GETTODAY, 0, (LPARAM)pst)
#define MonthCal_SetToday(hmc, pst)\
		SendMessageA(hmc, MCM_SETTODAY, 0, (LPARAM)pst)
#define MonthCal_HitTest(hmc, pinfo) \
        SendMessageA(hmc, MCM_HITTEST, 0, (LPARAM)(PMCHITTESTINFO)pinfo)
#define MonthCal_SetFirstDayOfWeek(hmc, iDay) \
        SendMessageA(hmc, MCM_SETFIRSTDAYOFWEEK, 0, iDay)
#define MonthCal_GetFirstDayOfWeek(hmc) \
        (DWORD)SendMessageA(hmc, MCM_GETFIRSTDAYOFWEEK, 0, 0)
#define MonthCal_GetRange(hmc, rgst) \
        (DWORD)SendMessageA(hmc, MCM_GETRANGE, 0, (LPARAM)(rgst))
#define MonthCal_SetRange(hmc, gd, rgst) \
        (BOOL)SendMessageA(hmc, MCM_SETRANGE, (WPARAM)(gd), (LPARAM)(rgst))
#define MonthCal_GetMonthDelta(hmc) \
        (int)SendMessageA(hmc, MCM_GETMONTHDELTA, 0, 0)
#define MonthCal_SetMonthDelta(hmc, n) \
        (int)SendMessageA(hmc, MCM_SETMONTHDELTA, n, 0)
#define MonthCal_GetMaxTodayWidth(hmc) \
        (DWORD)SendMessageA(hmc, MCM_GETMAXTODAYWIDTH, 0, 0)
#define MonthCal_SetUnicodeFormat(hwnd, fUnicode)  \
        (BOOL)SendMessageA((hwnd), MCM_SETUNICODEFORMAT, (WPARAM)(fUnicode), 0)
#define MonthCal_GetUnicodeFormat(hwnd)  \
        (BOOL)SendMessageA((hwnd), MCM_GETUNICODEFORMAT, 0, 0)


/**************************************************************************
 * Date and time picker control
 */

#define DATETIMEPICK_CLASSA	"SysDateTimePick32"
#define DATETIMEPICK_CLASSW	L"SysDateTimePick32"
#define DATETIMEPICK_CLASS	WINELIB_NAME_AW(DATETIMEPICK_CLASS)

#define DTM_FIRST        0x1000
#define DTN_FIRST       (0U-760U)     
#define DTN_LAST        (0U-799U)


#define DTM_GETSYSTEMTIME	(DTM_FIRST+1)
#define DTM_SETSYSTEMTIME	(DTM_FIRST+2)
#define DTM_GETRANGE		(DTM_FIRST+3)
#define DTM_SETRANGE		(DTM_FIRST+4)
#define DTM_SETFORMATA	    (DTM_FIRST+5)
#define DTM_SETFORMATW	    (DTM_FIRST + 50)
#define DTM_SETFORMAT		WINELIB_NAME_AW(DTM_SETFORMAT)
#define DTM_SETMCCOLOR		(DTM_FIRST+6)
#define DTM_GETMCCOLOR		(DTM_FIRST+7)
#define DTM_GETMONTHCAL		(DTM_FIRST+8)
#define DTM_SETMCFONT		(DTM_FIRST+9)
#define DTM_GETMCFONT		(DTM_FIRST+10)


/* Datetime Notifications */

#define DTN_DATETIMECHANGE  (DTN_FIRST + 1) 
#define DTN_USERSTRINGA     (DTN_FIRST + 2) 
#define DTN_WMKEYDOWNA      (DTN_FIRST + 3) 
#define DTN_FORMATA         (DTN_FIRST + 4) 
#define DTN_FORMATQUERYA    (DTN_FIRST + 5) 
#define DTN_DROPDOWN        (DTN_FIRST + 6)
#define DTN_CLOSEUP         (DTN_FIRST + 7) 
#define DTN_USERSTRINGW     (DTN_FIRST + 15)
#define DTN_WMKEYDOWNW      (DTN_FIRST + 16)
#define DTN_FORMATW         (DTN_FIRST + 17)
#define DTN_FORMATQUERYW    (DTN_FIRST + 18)


#define DTS_SHORTDATEFORMAT 0x0000 
#define DTS_UPDOWN          0x0001 
#define DTS_SHOWNONE        0x0002 
#define DTS_LONGDATEFORMAT  0x0004 
#define DTS_TIMEFORMAT      0x0009 
#define DTS_APPCANPARSE     0x0010 
#define DTS_RIGHTALIGN      0x0020 

typedef struct tagNMDATETIMECHANGE
{
    NMHDR       nmhdr;
    DWORD       dwFlags;    
    SYSTEMTIME  st;         
} NMDATETIMECHANGE, *LPNMDATETIMECHANGE;

typedef struct tagNMDATETIMESTRINGA
{
    NMHDR      nmhdr;
    LPCSTR     pszUserString; 
    SYSTEMTIME st;      
    DWORD      dwFlags;  
} NMDATETIMESTRINGA, *LPNMDATETIMESTRINGA;

typedef struct tagNMDATETIMESTRINGW
{
    NMHDR      nmhdr;
    LPCWSTR    pszUserString;
    SYSTEMTIME st;          
    DWORD      dwFlags;    
} NMDATETIMESTRINGW, *LPNMDATETIMESTRINGW;


typedef struct tagNMDATETIMEWMKEYDOWNA
{
    NMHDR      nmhdr;
    int        nVirtKey;  
    LPCSTR     pszFormat; 
    SYSTEMTIME st;       
} NMDATETIMEWMKEYDOWNA, *LPNMDATETIMEWMKEYDOWNA;

typedef struct tagNMDATETIMEWMKEYDOWNW
{
    NMHDR      nmhdr;
    int        nVirtKey;  
    LPCWSTR    pszFormat; 
    SYSTEMTIME st;       
} NMDATETIMEWMKEYDOWNW, *LPNMDATETIMEWMKEYDOWNW;



typedef struct tagNMDATETIMEFORMATA
{
    NMHDR nmhdr;
    LPCSTR  pszFormat;   
    SYSTEMTIME st;      
    LPCSTR pszDisplay;  
    CHAR szDisplay[64]; 
} NMDATETIMEFORMATA, *LPNMDATETIMEFORMATA;


typedef struct tagNMDATETIMEFORMATW
{
    NMHDR nmhdr;
    LPCWSTR pszFormat;  
    SYSTEMTIME st;     
    LPCWSTR pszDisplay;
    WCHAR szDisplay[64];
} NMDATETIMEFORMATW, *LPNMDATETIMEFORMATW;



typedef struct tagNMDATETIMEFORMATQUERYA
{
    NMHDR nmhdr;
    LPCSTR pszFormat; 
    SIZE szMax;       
} NMDATETIMEFORMATQUERYA, *LPNMDATETIMEFORMATQUERYA;

typedef struct tagNMDATETIMEFORMATQUERYW
{
    NMHDR nmhdr;
    LPCWSTR pszFormat; 
    SIZE szMax;        
} NMDATETIMEFORMATQUERYW, *LPNMDATETIMEFORMATQUERYW;


#define NMDATETIMESTRING WINELIB_NAME_AW(NMDATETIMESTRING)
#define NMDATETIMEWMKEYDOWN WINELIB_NAME_AW(NMDATETIMEWMKEYDOWN)
#define NMDATETIMEFORMAT WINELIB_NAME_AW(NMDATETIMEFORMAT)
#define NMDATETIMEFORMATQUERY WINELIB_NAME_AW(NMDATETIMEFORMATQUERY)





#define GDT_ERROR    -1
#define GDT_VALID    0
#define GDT_NONE     1

#define GDTR_MIN     0x0001
#define GDTR_MAX     0x0002


#define DateTime_GetSystemtime(hdp, pst)   \
  (DWORD)SendMessageA (hdp, DTM_GETSYSTEMTIME , 0, (LPARAM)(pst)) 
#define DateTime_SetSystemtime(hdp, gd, pst)   \
  (BOOL)SendMessageA (hdp, DTM_SETSYSTEMTIME, (LPARAM)(gd), (LPARAM)(pst))
#define DateTime_GetRange(hdp, rgst)  \
  (DWORD)SendMessageA (hdp, DTM_GETRANGE, 0, (LPARAM)(rgst)) 
#define DateTime_SetRange(hdp, gd, rgst) \
   (BOOL)SendMessageA (hdp, DTM_SETRANGE, (WPARAM)(gd), (LPARAM)(rgst))
#define DateTime_SetFormat WINELIB_NAME_AW(DateTime_SetFormat)
#define DateTime_SetFormatA(hdp, sz)  \
  (BOOL)SendMessageA (hdp, DTM_SETFORMAT, 0, (LPARAM)(sz))
#define DateTime_SetFormatW(hdp, sz)  \
  (BOOL)SendMessageW (hdp, DTM_SETFORMAT, 0, (LPARAM)(sz))
#define DateTime_GetMonthCalColor(hdp, iColor) \
  SendMessageA (hdp, DTM_GETMCCOLOR, iColor, 0)
#define DateTime_GetMonthCal(hdp)  \
  (HWND) SendMessageA (hdp, DTM_GETMONTHCAL, 0, 0)
#define DateTime_SetMonthCalFont(hdp, hfont, fRedraw) \
  SendMessageA (hdp, DTM_SETMCFONT, (WPARAM)hfont, (LPARAM)fRedraw)
#define DateTime_GetMonthCalFont(hdp) \
  SendMessageA (hdp, DTM_GETMCFONT, 0, 0)






/**************************************************************************
 *  UNDOCUMENTED functions
 */

/* private heap memory functions */

LPVOID WINAPI COMCTL32_Alloc (DWORD);
LPVOID WINAPI COMCTL32_ReAlloc (LPVOID, DWORD);
BOOL WINAPI COMCTL32_Free (LPVOID);
DWORD  WINAPI COMCTL32_GetSize (LPVOID);


INT  WINAPI Str_GetPtrA (LPCSTR, LPSTR, INT);
BOOL WINAPI Str_SetPtrA (LPSTR *, LPCSTR);
INT  WINAPI Str_GetPtrW (LPCWSTR, LPWSTR, INT);
BOOL WINAPI Str_SetPtrW (LPWSTR *, LPCWSTR);
#define Str_GetPtr WINELIB_NAME_AW(Str_GetPtr)
#define Str_SetPtr WINELIB_NAME_AW(Str_SetPtr)


/* Dynamic Storage Array */

typedef struct _DSA
{
    INT  nItemCount;
    LPVOID pData;
    INT  nMaxCount;
    INT  nItemSize;
    INT  nGrow;
} DSA, *HDSA;

HDSA   WINAPI DSA_Create (INT, INT);
BOOL WINAPI DSA_DeleteAllItems (const HDSA);
INT  WINAPI DSA_DeleteItem (const HDSA, INT);
BOOL WINAPI DSA_Destroy (const HDSA);
BOOL WINAPI DSA_GetItem (const HDSA, INT, LPVOID);
LPVOID WINAPI DSA_GetItemPtr (const HDSA, INT);
INT  WINAPI DSA_InsertItem (const HDSA, INT, LPVOID);
BOOL WINAPI DSA_SetItem (const HDSA, INT, LPVOID);

typedef INT (CALLBACK *DSAENUMPROC)(LPVOID, DWORD);
VOID   WINAPI DSA_EnumCallback (const HDSA, DSAENUMPROC, LPARAM);
BOOL WINAPI DSA_DestroyCallback (const HDSA, DSAENUMPROC, LPARAM);


/* Dynamic Pointer Array */

typedef struct _DPA
{
    INT    nItemCount;
    LPVOID   *ptrs; 
    HANDLE hHeap;
    INT    nGrow;
    INT    nMaxCount;
} DPA, *HDPA;

HDPA   WINAPI DPA_Create (INT);
HDPA   WINAPI DPA_CreateEx (INT, HANDLE);
BOOL WINAPI DPA_Destroy (const HDPA);
HDPA   WINAPI DPA_Clone (const HDPA, const HDPA);
LPVOID WINAPI DPA_GetPtr (const HDPA, INT);
INT  WINAPI DPA_GetPtrIndex (const HDPA, LPVOID);
BOOL WINAPI DPA_Grow (const HDPA, INT);
BOOL WINAPI DPA_SetPtr (const HDPA, INT, LPVOID);
INT  WINAPI DPA_InsertPtr (const HDPA, INT, LPVOID);
LPVOID WINAPI DPA_DeletePtr (const HDPA, INT);
BOOL WINAPI DPA_DeleteAllPtrs (const HDPA);

typedef INT (CALLBACK *PFNDPACOMPARE)(LPVOID, LPVOID, LPARAM);
BOOL WINAPI DPA_Sort (const HDPA, PFNDPACOMPARE, LPARAM);

#define DPAS_SORTED             0x0001
#define DPAS_INSERTBEFORE       0x0002
#define DPAS_INSERTAFTER        0x0004
 
INT  WINAPI DPA_Search (const HDPA, LPVOID, INT, PFNDPACOMPARE, LPARAM, UINT);

#define DPAM_SORT               0x0001

typedef PVOID(CALLBACK *PFNDPAMERGE)(DWORD,PVOID,PVOID,LPARAM);
BOOL WINAPI DPA_Merge (const HDPA, const HDPA, DWORD, PFNDPACOMPARE, PFNDPAMERGE, LPARAM);

typedef INT (CALLBACK *DPAENUMPROC)(LPVOID, DWORD);
VOID   WINAPI DPA_EnumCallback (const HDPA, DPAENUMPROC, LPARAM);
BOOL WINAPI DPA_DestroyCallback (const HDPA, DPAENUMPROC, LPARAM);


#define DPA_GetPtrCount(hdpa)  (*(INT*)(hdpa))
#define DPA_GetPtrPtr(hdpa)    (*((LPVOID**)((BYTE*)(hdpa)+sizeof(INT))))
#define DPA_FastGetPtr(hdpa,i) (DPA_GetPtrPtr(hdpa)[i])


/* notification helper functions */

LRESULT WINAPI COMCTL32_SendNotify (HWND, HWND, UINT, LPNMHDR);

/* type and functionality of last parameter is still unknown */
LRESULT WINAPI COMCTL32_SendNotifyEx (HWND, HWND, UINT, LPNMHDR, DWORD);

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_COMMCTRL_H */


