#ifndef __INCLUDE_WINUSER_H
#define __INCLUDE_WINUSER_H

#ifndef RC_INVOKED
#include <stdarg.h>
#endif
#include "wintypes.h"
#include "wingdi.h"
#include "wine/winestring.h"

#pragma pack(1)

/* flags for HIGHCONTRAST dwFlags field */
#define HCF_HIGHCONTRASTON  0x00000001
#define HCF_AVAILABLE       0x00000002
#define HCF_HOTKEYACTIVE    0x00000004
#define HCF_CONFIRMHOTKEY   0x00000008
#define HCF_HOTKEYSOUND     0x00000010
#define HCF_INDICATOR       0x00000020
#define HCF_HOTKEYAVAILABLE 0x00000040

typedef struct tagHIGHCONTRAST32A
{
    UINT32  cbSize;
    DWORD   dwFlags;
    LPSTR   lpszDefaultScheme;
} HIGHCONTRAST32A, *LPHIGHCONTRAST32A;

typedef struct tagHIGHCONTRAST32W
{
    UINT32  cbSize;
    DWORD   dwFlags;
    LPWSTR  lpszDefaultScheme;
} HIGHCONTRAST32W, *LPHIGHCONTRAST32W;

DECL_WINELIB_TYPE_AW(HIGHCONTRAST)
DECL_WINELIB_TYPE_AW(LPHIGHCONTRAST)

typedef struct
{
    UINT32  message;
    UINT32  paramL;
    UINT32  paramH;
    DWORD   time;
    HWND32  hwnd;
} EVENTMSG32, *LPEVENTMSG32;

DECL_WINELIB_TYPE(EVENTMSG)
DECL_WINELIB_TYPE(LPEVENTMSG)

    /* Mouse hook structure */

typedef struct
{
    POINT32 pt;
    HWND32  hwnd;
    UINT32  wHitTestCode;
    DWORD   dwExtraInfo;
} MOUSEHOOKSTRUCT32, *PMOUSEHOOKSTRUCT32, *LPMOUSEHOOKSTRUCT32;

DECL_WINELIB_TYPE(MOUSEHOOKSTRUCT)
DECL_WINELIB_TYPE(PMOUSEHOOKSTRUCT)
DECL_WINELIB_TYPE(LPMOUSEHOOKSTRUCT)

    /* Hardware hook structure */

typedef struct
{
    HWND32    hWnd;
    UINT32    wMessage;
    WPARAM32  wParam;
    LPARAM    lParam;
} HARDWAREHOOKSTRUCT32, *LPHARDWAREHOOKSTRUCT32;

DECL_WINELIB_TYPE(HARDWAREHOOKSTRUCT)
DECL_WINELIB_TYPE(LPHARDWAREHOOKSTRUCT)

  /* Debug hook structure */

typedef struct
{
    DWORD       idThread;
    DWORD       idThreadInstaller;
    LPARAM      lParam;
    WPARAM32    wParam;
    INT32       code;
} DEBUGHOOKINFO32, *LPDEBUGHOOKINFO32;

DECL_WINELIB_TYPE(DEBUGHOOKINFO)
DECL_WINELIB_TYPE(LPDEBUGHOOKINFO)

  /***** Dialogs *****/
#ifdef FSHIFT
/* Gcc on Solaris has a version of this that we don't care about.  */
#undef FSHIFT
#endif

#define	FVIRTKEY	TRUE          /* Assumed to be == TRUE */
#define	FNOINVERT	0x02
#define	FSHIFT		0x04
#define	FCONTROL	0x08
#define	FALT		0x10


typedef struct tagANIMATIONINFO
{
       UINT32          cbSize;
       INT32           iMinAnimate;
} ANIMATIONINFO, *LPANIMATIONINFO;

typedef struct tagNMHDR
{
    HWND32  hwndFrom;
    UINT32  idFrom;
    UINT32  code;
} NMHDR, *LPNMHDR;

typedef struct
{
	UINT32	cbSize;
	INT32	iTabLength;
	INT32	iLeftMargin;
	INT32	iRightMargin;
	UINT32	uiLengthDrawn;
} DRAWTEXTPARAMS,*LPDRAWTEXTPARAMS;

#define WM_USER             0x0400

typedef struct
{
    UINT32   length;
    UINT32   flags;
    UINT32   showCmd;
    POINT32  ptMinPosition WINE_PACKED;
    POINT32  ptMaxPosition WINE_PACKED;
    RECT32   rcNormalPosition WINE_PACKED;
} WINDOWPLACEMENT32, *LPWINDOWPLACEMENT32;

DECL_WINELIB_TYPE(WINDOWPLACEMENT)
DECL_WINELIB_TYPE(LPWINDOWPLACEMENT)

  /* WINDOWPLACEMENT flags */
#define WPF_SETMINPOSITION      0x0001
#define WPF_RESTORETOMAXIMIZED  0x0002

/***** Dialogs *****/

  /* cbWndExtra bytes for dialog class */
#define DLGWINDOWEXTRA      30

/* Button control styles */
#define BS_PUSHBUTTON          0x00000000L
#define BS_DEFPUSHBUTTON       0x00000001L
#define BS_CHECKBOX            0x00000002L
#define BS_AUTOCHECKBOX        0x00000003L
#define BS_RADIOBUTTON         0x00000004L
#define BS_3STATE              0x00000005L
#define BS_AUTO3STATE          0x00000006L
#define BS_GROUPBOX            0x00000007L
#define BS_USERBUTTON          0x00000008L
#define BS_AUTORADIOBUTTON     0x00000009L
#define BS_OWNERDRAW           0x0000000BL
#define BS_LEFTTEXT            0x00000020L

  /* Dialog styles */
#define DS_ABSALIGN		0x0001
#define DS_SYSMODAL		0x0002
#define DS_3DLOOK		0x0004	/* win95 */
#define DS_FIXEDSYS		0x0008	/* win95 */
#define DS_NOFAILCREATE		0x0010	/* win95 */
#define DS_LOCALEDIT		0x0020
#define DS_SETFONT		0x0040
#define DS_MODALFRAME		0x0080
#define DS_NOIDLEMSG		0x0100
#define DS_SETFOREGROUND	0x0200	/* win95 */
#define DS_CONTROL		0x0400	/* win95 */
#define DS_CENTER		0x0800	/* win95 */
#define DS_CENTERMOUSE		0x1000	/* win95 */
#define DS_CONTEXTHELP		0x2000	/* win95 */


  /* Dialog messages */
#define DM_GETDEFID         (WM_USER+0)
#define DM_SETDEFID         (WM_USER+1)

#define DC_HASDEFID         0x534b

/* Owner draw control types */
#define ODT_MENU        1
#define ODT_LISTBOX     2
#define ODT_COMBOBOX    3
#define ODT_BUTTON      4

/* Owner draw actions */
#define ODA_DRAWENTIRE  0x0001
#define ODA_SELECT      0x0002
#define ODA_FOCUS       0x0004

/* Owner draw state */
#define ODS_SELECTED    0x0001
#define ODS_GRAYED      0x0002
#define ODS_DISABLED    0x0004
#define ODS_CHECKED     0x0008
#define ODS_FOCUS       0x0010

/* Edit control styles */
#define ES_LEFT         0x00000000
#define ES_CENTER       0x00000001
#define ES_RIGHT        0x00000002
#define ES_MULTILINE    0x00000004
#define ES_UPPERCASE    0x00000008
#define ES_LOWERCASE    0x00000010
#define ES_PASSWORD     0x00000020
#define ES_AUTOVSCROLL  0x00000040
#define ES_AUTOHSCROLL  0x00000080
#define ES_NOHIDESEL    0x00000100
#define ES_OEMCONVERT   0x00000400
#define ES_READONLY     0x00000800
#define ES_WANTRETURN   0x00001000
#define ES_NUMBER       0x00002000

/* OEM Resource Ordinal Numbers */
#define OBM_CLOSE           32754
#define OBM_UPARROW         32753
#define OBM_DNARROW         32752
#define OBM_RGARROW         32751
#define OBM_LFARROW         32750
#define OBM_REDUCE          32749
#define OBM_ZOOM            32748
#define OBM_RESTORE         32747
#define OBM_REDUCED         32746
#define OBM_ZOOMD           32745
#define OBM_RESTORED        32744
#define OBM_UPARROWD        32743
#define OBM_DNARROWD        32742
#define OBM_RGARROWD        32741
#define OBM_LFARROWD        32740
#define OBM_MNARROW         32739
#define OBM_COMBO           32738
#define OBM_UPARROWI        32737
#define OBM_DNARROWI        32736
#define OBM_RGARROWI        32735
#define OBM_LFARROWI        32734

#define OBM_FOLDER          32733
#define OBM_FOLDER2         32732
#define OBM_FLOPPY          32731
#define OBM_HDISK           32730
#define OBM_CDROM           32729
#define OBM_TRTYPE          32728

/* Wine extension, I think.  */
#define OBM_RADIOCHECK      32727

#define OBM_OLD_CLOSE       32767
#define OBM_SIZE            32766
#define OBM_OLD_UPARROW     32765
#define OBM_OLD_DNARROW     32764
#define OBM_OLD_RGARROW     32763
#define OBM_OLD_LFARROW     32762
#define OBM_BTSIZE          32761
#define OBM_CHECK           32760
#define OBM_CHECKBOXES      32759
#define OBM_BTNCORNERS      32758
#define OBM_OLD_REDUCE      32757
#define OBM_OLD_ZOOM        32756
#define OBM_OLD_RESTORE     32755

#define OCR_BUMMER	    100
#define OCR_DRAGOBJECT	    101

#define OCR_NORMAL          32512
#define OCR_IBEAM           32513
#define OCR_WAIT            32514
#define OCR_CROSS           32515
#define OCR_UP              32516
#define OCR_SIZE            32640
#define OCR_ICON            32641
#define OCR_SIZENWSE        32642
#define OCR_SIZENESW        32643
#define OCR_SIZEWE          32644
#define OCR_SIZENS          32645
#define OCR_SIZEALL         32646
#define OCR_ICOCUR          32647
#define OCR_NO              32648
#define OCR_APPSTARTING     32650
#define OCR_HELP            32651  /* only defined in wine */

#define OIC_SAMPLE          32512
#define OIC_HAND            32513
#define OIC_QUES            32514
#define OIC_BANG            32515
#define OIC_NOTE            32516
#define OIC_PORTRAIT        32517
#define OIC_LANDSCAPE       32518
#define OIC_WINEICON        32519

/* Edit control messages */
#define EM_GETSEL32                0x00b0
#define EM_GETSEL                  WINELIB_NAME(EM_GETSEL)
#define EM_SETSEL32                0x00b1
#define EM_SETSEL                  WINELIB_NAME(EM_SETSEL)
#define EM_GETRECT32               0x00b2
#define EM_GETRECT                 WINELIB_NAME(EM_GETRECT)
#define EM_SETRECT32               0x00b3
#define EM_SETRECT                 WINELIB_NAME(EM_SETRECT)
#define EM_SETRECTNP32             0x00b4
#define EM_SETRECTNP               WINELIB_NAME(EM_SETRECTNP)
#define EM_SCROLL32                0x00b5
#define EM_SCROLL                  WINELIB_NAME(EM_SCROLL)
#define EM_LINESCROLL32            0x00b6
#define EM_LINESCROLL              WINELIB_NAME(EM_LINESCROLL)
#define EM_SCROLLCARET32           0x00b7
#define EM_SCROLLCARET             WINELIB_NAME(EM_SCROLLCARET)
#define EM_GETMODIFY32             0x00b8
#define EM_GETMODIFY               WINELIB_NAME(EM_GETMODIFY)
#define EM_SETMODIFY32             0x00b9
#define EM_SETMODIFY               WINELIB_NAME(EM_SETMODIFY)
#define EM_GETLINECOUNT32          0x00ba
#define EM_GETLINECOUNT            WINELIB_NAME(EM_GETLINECOUNT)
#define EM_LINEINDEX32             0x00bb
#define EM_LINEINDEX               WINELIB_NAME(EM_LINEINDEX)
#define EM_SETHANDLE32             0x00bc
#define EM_SETHANDLE               WINELIB_NAME(EM_SETHANDLE)
#define EM_GETHANDLE32             0x00bd
#define EM_GETHANDLE               WINELIB_NAME(EM_GETHANDLE)
#define EM_GETTHUMB32              0x00be
#define EM_GETTHUMB                WINELIB_NAME(EM_GETTHUMB)
/* FIXME : missing from specs 0x00bf and 0x00c0 */
#define EM_LINELENGTH32            0x00c1
#define EM_LINELENGTH              WINELIB_NAME(EM_LINELENGTH)
#define EM_REPLACESEL32            0x00c2
#define EM_REPLACESEL              WINELIB_NAME(EM_REPLACESEL)
/* FIXME : missing from specs 0x00c3 */
#define EM_GETLINE32               0x00c4
#define EM_GETLINE                 WINELIB_NAME(EM_GETLINE)
#define EM_LIMITTEXT32             0x00c5
#define EM_LIMITTEXT               WINELIB_NAME(EM_LIMITTEXT)
#define EM_CANUNDO32               0x00c6
#define EM_CANUNDO                 WINELIB_NAME(EM_CANUNDO)
#define EM_UNDO32                  0x00c7
#define EM_UNDO                    WINELIB_NAME(EM_UNDO)
#define EM_FMTLINES32              0x00c8
#define EM_FMTLINES                WINELIB_NAME(EM_FMTLINES)
#define EM_LINEFROMCHAR32          0x00c9
#define EM_LINEFROMCHAR            WINELIB_NAME(EM_LINEFROMCHAR)
/* FIXME : missing from specs 0x00ca */
#define EM_SETTABSTOPS32           0x00cb
#define EM_SETTABSTOPS             WINELIB_NAME(EM_SETTABSTOPS)
#define EM_SETPASSWORDCHAR32       0x00cc
#define EM_SETPASSWORDCHAR         WINELIB_NAME(EM_SETPASSWORDCHAR)
#define EM_EMPTYUNDOBUFFER32       0x00cd
#define EM_EMPTYUNDOBUFFER         WINELIB_NAME(EM_EMPTYUNDOBUFFER)
#define EM_GETFIRSTVISIBLELINE32   0x00ce
#define EM_GETFIRSTVISIBLELINE     WINELIB_NAME(EM_GETFIRSTVISIBLELINE)
#define EM_SETREADONLY32           0x00cf
#define EM_SETREADONLY             WINELIB_NAME(EM_SETREADONLY)
#define EM_SETWORDBREAKPROC32      0x00d0
#define EM_SETWORDBREAKPROC        WINELIB_NAME(EM_SETWORDBREAKPROC)
#define EM_GETWORDBREAKPROC32      0x00d1
#define EM_GETWORDBREAKPROC        WINELIB_NAME(EM_GETWORDBREAKPROC)
#define EM_GETPASSWORDCHAR32       0x00d2
#define EM_GETPASSWORDCHAR         WINELIB_NAME(EM_GETPASSWORDCHAR)
#define EM_SETMARGINS32            0x00d3
#define EM_SETMARGINS              WINELIB_NAME(EM_SETMARGINS)
#define EM_GETMARGINS32            0x00d4
#define EM_GETMARGINS              WINELIB_NAME(EM_GETMARGINS)
#define EM_GETLIMITTEXT32          0x00d5
#define EM_GETLIMITTEXT            WINELIB_NAME(EM_GETLIMITTEXT)
#define EM_POSFROMCHAR32           0x00d6
#define EM_POSFROMCHAR             WINELIB_NAME(EM_POSFROMCHAR)
#define EM_CHARFROMPOS32           0x00d7
#define EM_CHARFROMPOS             WINELIB_NAME(EM_CHARFROMPOS)
/* a name change since win95 */
#define EM_SETLIMITTEXT32          EM_LIMITTEXT32
#define EM_SETLIMITTEXT            WINELIB_NAME(EM_SETLIMITTEXT)

/* EDITWORDBREAKPROC code values */
#define WB_LEFT         0
#define WB_RIGHT        1
#define WB_ISDELIMITER  2

/* Edit control notification codes */
#define EN_SETFOCUS     0x0100
#define EN_KILLFOCUS    0x0200
#define EN_CHANGE       0x0300
#define EN_UPDATE       0x0400
#define EN_ERRSPACE     0x0500
#define EN_MAXTEXT      0x0501
#define EN_HSCROLL      0x0601
#define EN_VSCROLL      0x0602

/* New since win95 : EM_SETMARGIN parameters */
#define EC_LEFTMARGIN	0x0001
#define EC_RIGHTMARGIN	0x0002
#define EC_USEFONTINFO	0xffff


/* Messages */

  /* WM_GETDLGCODE values */


#define WM_NULL                 0x0000
#define WM_CREATE               0x0001
#define WM_DESTROY              0x0002
#define WM_MOVE                 0x0003
#define WM_SIZEWAIT             0x0004
#define WM_SIZE                 0x0005
#define WM_ACTIVATE             0x0006
#define WM_SETFOCUS             0x0007
#define WM_KILLFOCUS            0x0008
#define WM_SETVISIBLE           0x0009
#define WM_ENABLE               0x000a
#define WM_SETREDRAW            0x000b
#define WM_SETTEXT              0x000c
#define WM_GETTEXT              0x000d
#define WM_GETTEXTLENGTH        0x000e
#define WM_PAINT                0x000f
#define WM_CLOSE                0x0010
#define WM_QUERYENDSESSION      0x0011
#define WM_QUIT                 0x0012
#define WM_QUERYOPEN            0x0013
#define WM_ERASEBKGND           0x0014
#define WM_SYSCOLORCHANGE       0x0015
#define WM_ENDSESSION           0x0016
#define WM_SYSTEMERROR          0x0017
#define WM_SHOWWINDOW           0x0018
#define WM_CTLCOLOR             0x0019
#define WM_WININICHANGE         0x001a
#define WM_SETTINGCHANGE        WM_WININICHANGE
#define WM_DEVMODECHANGE        0x001b
#define WM_ACTIVATEAPP          0x001c
#define WM_FONTCHANGE           0x001d
#define WM_TIMECHANGE           0x001e
#define WM_CANCELMODE           0x001f
#define WM_SETCURSOR            0x0020
#define WM_MOUSEACTIVATE        0x0021
#define WM_CHILDACTIVATE        0x0022
#define WM_QUEUESYNC            0x0023
#define WM_GETMINMAXINFO        0x0024

#define WM_PAINTICON            0x0026
#define WM_ICONERASEBKGND       0x0027
#define WM_NEXTDLGCTL           0x0028
#define WM_ALTTABACTIVE         0x0029
#define WM_SPOOLERSTATUS        0x002a
#define WM_DRAWITEM             0x002b
#define WM_MEASUREITEM          0x002c
#define WM_DELETEITEM           0x002d
#define WM_VKEYTOITEM           0x002e
#define WM_CHARTOITEM           0x002f
#define WM_SETFONT              0x0030
#define WM_GETFONT              0x0031
#define WM_SETHOTKEY            0x0032
#define WM_GETHOTKEY            0x0033
#define WM_FILESYSCHANGE        0x0034
#define WM_ISACTIVEICON         0x0035
#define WM_QUERYPARKICON        0x0036
#define WM_QUERYDRAGICON        0x0037
#define WM_QUERYSAVESTATE       0x0038
#define WM_COMPAREITEM          0x0039
#define WM_TESTING              0x003a

#define WM_OTHERWINDOWCREATED	0x003c
#define WM_OTHERWINDOWDESTROYED	0x003d
#define WM_ACTIVATESHELLWINDOW	0x003e

#define WM_COMPACTING		0x0041

#define WM_COMMNOTIFY		0x0044
#define WM_WINDOWPOSCHANGING 	0x0046
#define WM_WINDOWPOSCHANGED 	0x0047
#define WM_POWER		0x0048

  /* Win32 4.0 messages */
#define WM_COPYDATA		0x004a
#define WM_CANCELJOURNAL	0x004b
#define WM_NOTIFY		0x004e
#define WM_HELP			0x0053
#define WM_NOTIFYFORMAT		0x0055

#define WM_CONTEXTMENU		0x007b
#define WM_STYLECHANGING 	0x007c
#define WM_STYLECHANGED		0x007d
#define WM_DISPLAYCHANGE        0x007e
#define WM_GETICON		0x007f
#define WM_SETICON		0x0080

  /* Non-client system messages */
#define WM_NCCREATE         0x0081
#define WM_NCDESTROY        0x0082
#define WM_NCCALCSIZE       0x0083
#define WM_NCHITTEST        0x0084
#define WM_NCPAINT          0x0085
#define WM_NCACTIVATE       0x0086

#define WM_GETDLGCODE	    0x0087
#define WM_SYNCPAINT	    0x0088
#define WM_SYNCTASK	    0x0089

  /* Non-client mouse messages */
#define WM_NCMOUSEMOVE      0x00a0
#define WM_NCLBUTTONDOWN    0x00a1
#define WM_NCLBUTTONUP      0x00a2
#define WM_NCLBUTTONDBLCLK  0x00a3
#define WM_NCRBUTTONDOWN    0x00a4
#define WM_NCRBUTTONUP      0x00a5
#define WM_NCRBUTTONDBLCLK  0x00a6
#define WM_NCMBUTTONDOWN    0x00a7
#define WM_NCMBUTTONUP      0x00a8
#define WM_NCMBUTTONDBLCLK  0x00a9

  /* Keyboard messages */
#define WM_KEYDOWN          0x0100
#define WM_KEYUP            0x0101
#define WM_CHAR             0x0102
#define WM_DEADCHAR         0x0103
#define WM_SYSKEYDOWN       0x0104
#define WM_SYSKEYUP         0x0105
#define WM_SYSCHAR          0x0106
#define WM_SYSDEADCHAR      0x0107
#define WM_KEYFIRST         WM_KEYDOWN
#define WM_KEYLAST          0x0108

#define WM_INITDIALOG       0x0110 
#define WM_COMMAND          0x0111
#define WM_SYSCOMMAND       0x0112
#define WM_TIMER	    0x0113
#define WM_SYSTIMER	    0x0118

  /* scroll messages */
#define WM_HSCROLL          0x0114
#define WM_VSCROLL          0x0115

/* Menu messages */
#define WM_INITMENU         0x0116
#define WM_INITMENUPOPUP    0x0117

#define WM_MENUSELECT       0x011F
#define WM_MENUCHAR         0x0120
#define WM_ENTERIDLE        0x0121

#define WM_LBTRACKPOINT     0x0131

  /* Win32 CTLCOLOR messages */
#define WM_CTLCOLORMSGBOX    0x0132
#define WM_CTLCOLOREDIT      0x0133
#define WM_CTLCOLORLISTBOX   0x0134
#define WM_CTLCOLORBTN       0x0135
#define WM_CTLCOLORDLG       0x0136
#define WM_CTLCOLORSCROLLBAR 0x0137
#define WM_CTLCOLORSTATIC    0x0138

  /* Mouse messages */
#define WM_MOUSEMOVE	    0x0200
#define WM_LBUTTONDOWN	    0x0201
#define WM_LBUTTONUP	    0x0202
#define WM_LBUTTONDBLCLK    0x0203
#define WM_RBUTTONDOWN	    0x0204
#define WM_RBUTTONUP	    0x0205
#define WM_RBUTTONDBLCLK    0x0206
#define WM_MBUTTONDOWN	    0x0207
#define WM_MBUTTONUP	    0x0208
#define WM_MBUTTONDBLCLK    0x0209
#define WM_MOUSEFIRST	    WM_MOUSEMOVE
#define WM_MOUSELAST	    WM_MBUTTONDBLCLK

#define WM_PARENTNOTIFY     0x0210
#define WM_ENTERMENULOOP    0x0211
#define WM_EXITMENULOOP     0x0212
#define WM_NEXTMENU	    0x0213

  /* Win32 4.0 messages */
#define WM_SIZING	    0x0214
#define WM_CAPTURECHANGED   0x0215
#define WM_MOVING	    0x0216

  /* MDI messages */
#define WM_MDICREATE	    0x0220
#define WM_MDIDESTROY	    0x0221
#define WM_MDIACTIVATE	    0x0222
#define WM_MDIRESTORE	    0x0223
#define WM_MDINEXT	    0x0224
#define WM_MDIMAXIMIZE	    0x0225
#define WM_MDITILE	    0x0226
#define WM_MDICASCADE	    0x0227
#define WM_MDIICONARRANGE   0x0228
#define WM_MDIGETACTIVE     0x0229
#define WM_MDIREFRESHMENU   0x0234

  /* D&D messages */
#define WM_DROPOBJECT	    0x022A
#define WM_QUERYDROPOBJECT  0x022B
#define WM_BEGINDRAG	    0x022C
#define WM_DRAGLOOP	    0x022D
#define WM_DRAGSELECT	    0x022E
#define WM_DRAGMOVE	    0x022F
#define WM_MDISETMENU	    0x0230

#define WM_ENTERSIZEMOVE    0x0231
#define WM_EXITSIZEMOVE     0x0232
#define WM_DROPFILES	    0x0233

#define WM_CUT               0x0300
#define WM_COPY              0x0301
#define WM_PASTE             0x0302
#define WM_CLEAR             0x0303
#define WM_UNDO              0x0304
#define WM_RENDERFORMAT      0x0305
#define WM_RENDERALLFORMATS  0x0306
#define WM_DESTROYCLIPBOARD  0x0307
#define WM_DRAWCLIPBOARD     0x0308
#define WM_PAINTCLIPBOARD    0x0309
#define WM_VSCROLLCLIPBOARD  0x030A
#define WM_SIZECLIPBOARD     0x030B
#define WM_ASKCBFORMATNAME   0x030C
#define WM_CHANGECBCHAIN     0x030D
#define WM_HSCROLLCLIPBOARD  0x030E
#define WM_QUERYNEWPALETTE   0x030F
#define WM_PALETTEISCHANGING 0x0310
#define WM_PALETTECHANGED    0x0311
#define WM_HOTKEY	     0x0312

#define WM_PRINT             0x0317
#define WM_PRINTCLIENT       0x0318

  /* FIXME: This does not belong to any libwine interface header */
  /* MFC messages [370-37f] */

#define WM_QUERYAFXWNDPROC  0x0360
#define WM_SIZEPARENT       0x0361
#define WM_SETMESSAGESTRING 0x0362
#define WM_IDLEUPDATECMDUI  0x0363 
#define WM_INITIALUPDATE    0x0364
#define WM_COMMANDHELP      0x0365
#define WM_HELPHITTEST      0x0366
#define WM_EXITHELPMODE     0x0367
#define WM_RECALCPARENT     0x0368
#define WM_SIZECHILD        0x0369
#define WM_KICKIDLE         0x036A 
#define WM_QUERYCENTERWND   0x036B
#define WM_DISABLEMODAL     0x036C
#define WM_FLOATSTATUS      0x036D 
#define WM_ACTIVATETOPLEVEL 0x036E 
#define WM_QUERY3DCONTROLS  0x036F 
#define WM_SOCKET_NOTIFY    0x0373
#define WM_SOCKET_DEAD      0x0374
#define WM_POPMESSAGESTRING 0x0375
#define WM_OCC_LOADFROMSTREAM           0x0376
#define WM_OCC_LOADFROMSTORAGE          0x0377
#define WM_OCC_INITNEW                  0x0378
#define WM_OCC_LOADFROMSTREAM_EX        0x037A
#define WM_OCC_LOADFROMSTORAGE_EX       0x037B
#define WM_QUEUE_SENTINEL   0x0379

/* end of MFC messages */

  /* FIXME: This does not belong to any libwine interface header */
#define WM_COALESCE_FIRST    0x0390
#define WM_COALESCE_LAST     0x039F



#define DLGC_WANTARROWS      0x0001
#define DLGC_WANTTAB         0x0002
#define DLGC_WANTALLKEYS     0x0004
#define DLGC_WANTMESSAGE     0x0004
#define DLGC_HASSETSEL       0x0008
#define DLGC_DEFPUSHBUTTON   0x0010
#define DLGC_UNDEFPUSHBUTTON 0x0020
#define DLGC_RADIOBUTTON     0x0040
#define DLGC_WANTCHARS       0x0080
#define DLGC_STATIC          0x0100
#define DLGC_BUTTON          0x2000

/* Standard dialog button IDs */
#define IDOK                1
#define IDCANCEL            2
#define IDABORT             3
#define IDRETRY             4
#define IDIGNORE            5
#define IDYES               6
#define IDNO                7
#define IDCLOSE             8
#define IDHELP              9      

/****** Window classes ******/

typedef struct
{
    LPVOID      lpCreateParams;
    HINSTANCE32 hInstance;
    HMENU32     hMenu;
    HWND32      hwndParent;
    INT32       cy;
    INT32       cx;
    INT32       y;
    INT32       x;
    LONG        style;
    LPCSTR      lpszName;
    LPCSTR      lpszClass;
    DWORD       dwExStyle;
} CREATESTRUCT32A, *LPCREATESTRUCT32A;

typedef struct
{
    LPVOID      lpCreateParams;
    HINSTANCE32 hInstance;
    HMENU32     hMenu;
    HWND32      hwndParent;
    INT32       cy;
    INT32       cx;
    INT32       y;
    INT32       x;
    LONG        style;
    LPCWSTR     lpszName;
    LPCWSTR     lpszClass;
    DWORD       dwExStyle;
} CREATESTRUCT32W, *LPCREATESTRUCT32W;

DECL_WINELIB_TYPE_AW(CREATESTRUCT)
DECL_WINELIB_TYPE_AW(LPCREATESTRUCT)

typedef struct 
{
    HMENU32   hWindowMenu;
    UINT32    idFirstChild;
} CLIENTCREATESTRUCT32, *LPCLIENTCREATESTRUCT32;

DECL_WINELIB_TYPE(CLIENTCREATESTRUCT)
DECL_WINELIB_TYPE(LPCLIENTCREATESTRUCT)

typedef struct
{
    LPCSTR       szClass;
    LPCSTR       szTitle;
    HINSTANCE32  hOwner;
    INT32        x;
    INT32        y;
    INT32        cx;
    INT32        cy;
    DWORD        style;
    LPARAM       lParam;
} MDICREATESTRUCT32A, *LPMDICREATESTRUCT32A;

typedef struct
{
    LPCWSTR      szClass;
    LPCWSTR      szTitle;
    HINSTANCE32  hOwner;
    INT32        x;
    INT32        y;
    INT32        cx;
    INT32        cy;
    DWORD        style;
    LPARAM       lParam;
} MDICREATESTRUCT32W, *LPMDICREATESTRUCT32W;

DECL_WINELIB_TYPE_AW(MDICREATESTRUCT)
DECL_WINELIB_TYPE_AW(LPMDICREATESTRUCT)

#define MDITILE_VERTICAL     0x0000   
#define MDITILE_HORIZONTAL   0x0001
#define MDITILE_SKIPDISABLED 0x0002

#define MDIS_ALLCHILDSTYLES  0x0001

typedef struct {
    DWORD   styleOld;
    DWORD   styleNew;
} STYLESTRUCT, *LPSTYLESTRUCT;

  /* Offsets for GetWindowLong() and GetWindowWord() */
#define GWL_USERDATA        (-21)
#define GWL_EXSTYLE         (-20)
#define GWL_STYLE           (-16)
#define GWW_ID              (-12)
#define GWL_ID              GWW_ID
#define GWW_HWNDPARENT      (-8)
#define GWL_HWNDPARENT      GWW_HWNDPARENT
#define GWW_HINSTANCE       (-6)
#define GWL_HINSTANCE       GWW_HINSTANCE
#define GWL_WNDPROC         (-4)
#define DWL_MSGRESULT	    0
#define DWL_DLGPROC	    4
#define DWL_USER	    8

  /* GetWindow() constants */
#define GW_HWNDFIRST	0
#define GW_HWNDLAST	1
#define GW_HWNDNEXT	2
#define GW_HWNDPREV	3
#define GW_OWNER	4
#define GW_CHILD	5

  /* WM_GETMINMAXINFO struct */
typedef struct
{
    POINT32   ptReserved;
    POINT32   ptMaxSize;
    POINT32   ptMaxPosition;
    POINT32   ptMinTrackSize;
    POINT32   ptMaxTrackSize;
} MINMAXINFO32, *PMINMAXINFO32, *LPMINMAXINFO32;

DECL_WINELIB_TYPE(MINMAXINFO)
DECL_WINELIB_TYPE(PMINMAXINFO)
DECL_WINELIB_TYPE(LPMINMAXINFO)

  /* RedrawWindow() flags */
#define RDW_INVALIDATE       0x0001
#define RDW_INTERNALPAINT    0x0002
#define RDW_ERASE            0x0004
#define RDW_VALIDATE         0x0008
#define RDW_NOINTERNALPAINT  0x0010
#define RDW_NOERASE          0x0020
#define RDW_NOCHILDREN       0x0040
#define RDW_ALLCHILDREN      0x0080
#define RDW_UPDATENOW        0x0100
#define RDW_ERASENOW         0x0200
#define RDW_FRAME            0x0400
#define RDW_NOFRAME          0x0800

/* debug flags */
#define DBGFILL_ALLOC  0xfd
#define DBGFILL_FREE   0xfb
#define DBGFILL_BUFFER 0xf9
#define DBGFILL_STACK  0xf7

  /* WM_WINDOWPOSCHANGING/CHANGED struct */
typedef struct
{
    HWND32  hwnd;
    HWND32  hwndInsertAfter;
    INT32   x;
    INT32   y;
    INT32   cx;
    INT32   cy;
    UINT32  flags;
} WINDOWPOS32, *PWINDOWPOS32, *LPWINDOWPOS32;

DECL_WINELIB_TYPE(WINDOWPOS)
DECL_WINELIB_TYPE(PWINDOWPOS)
DECL_WINELIB_TYPE(LPWINDOWPOS)

  /* WM_MOUSEACTIVATE return values */
#define MA_ACTIVATE             1
#define MA_ACTIVATEANDEAT       2
#define MA_NOACTIVATE           3
#define MA_NOACTIVATEANDEAT     4

  /* WM_ACTIVATE wParam values */
#define WA_INACTIVE             0
#define WA_ACTIVE               1
#define WA_CLICKACTIVE          2

  /* WM_NCCALCSIZE parameter structure */
typedef struct
{
    RECT32       rgrc[3];
    WINDOWPOS32 *lppos;
} NCCALCSIZE_PARAMS32, *LPNCCALCSIZE_PARAMS32;

DECL_WINELIB_TYPE(NCCALCSIZE_PARAMS)
DECL_WINELIB_TYPE(LPNCCALCSIZE_PARAMS)

  /* WM_NCCALCSIZE return flags */
#define WVR_ALIGNTOP        0x0010
#define WVR_ALIGNLEFT       0x0020
#define WVR_ALIGNBOTTOM     0x0040
#define WVR_ALIGNRIGHT      0x0080
#define WVR_HREDRAW         0x0100
#define WVR_VREDRAW         0x0200
#define WVR_REDRAW          (WVR_HREDRAW | WVR_VREDRAW)
#define WVR_VALIDRECTS      0x0400

  /* WM_NCHITTEST return codes */
#define HTERROR             (-2)
#define HTTRANSPARENT       (-1)
#define HTNOWHERE           0
#define HTCLIENT            1
#define HTCAPTION           2
#define HTSYSMENU           3
#define HTSIZE              4
#define HTMENU              5
#define HTHSCROLL           6
#define HTVSCROLL           7
#define HTMINBUTTON         8
#define HTMAXBUTTON         9
#define HTLEFT              10
#define HTRIGHT             11
#define HTTOP               12
#define HTTOPLEFT           13
#define HTTOPRIGHT          14
#define HTBOTTOM            15
#define HTBOTTOMLEFT        16
#define HTBOTTOMRIGHT       17
#define HTBORDER            18
#define HTGROWBOX           HTSIZE
#define HTREDUCE            HTMINBUTTON
#define HTZOOM              HTMAXBUTTON
#define HTOBJECT            19
#define HTCLOSE             20
#define HTHELP              21
#define HTSIZEFIRST         HTLEFT
#define HTSIZELAST          HTBOTTOMRIGHT

  /* WM_SYSCOMMAND parameters */
#ifdef SC_SIZE /* at least HP-UX: already defined in /usr/include/sys/signal.h */
#undef SC_SIZE
#endif
#define SC_SIZE         0xf000
#define SC_MOVE         0xf010
#define SC_MINIMIZE     0xf020
#define SC_MAXIMIZE     0xf030
#define SC_NEXTWINDOW   0xf040
#define SC_PREVWINDOW   0xf050
#define SC_CLOSE        0xf060
#define SC_VSCROLL      0xf070
#define SC_HSCROLL      0xf080
#define SC_MOUSEMENU    0xf090
#define SC_KEYMENU      0xf100
#define SC_ARRANGE      0xf110
#define SC_RESTORE      0xf120
#define SC_TASKLIST     0xf130
#define SC_SCREENSAVE   0xf140
#define SC_HOTKEY       0xf150

#define CS_VREDRAW          0x0001
#define CS_HREDRAW          0x0002
#define CS_KEYCVTWINDOW     0x0004
#define CS_DBLCLKS          0x0008
#define CS_OWNDC            0x0020
#define CS_CLASSDC          0x0040
#define CS_PARENTDC         0x0080
#define CS_NOKEYCVT         0x0100
#define CS_NOCLOSE          0x0200
#define CS_SAVEBITS         0x0800
#define CS_BYTEALIGNCLIENT  0x1000
#define CS_BYTEALIGNWINDOW  0x2000
#define CS_GLOBALCLASS      0x4000

  /* Offsets for GetClassLong() and GetClassWord() */
#define GCL_MENUNAME        (-8)
#define GCW_HBRBACKGROUND   (-10)
#define GCL_HBRBACKGROUND   GCW_HBRBACKGROUND
#define GCW_HCURSOR         (-12)
#define GCL_HCURSOR         GCW_HCURSOR
#define GCW_HICON           (-14)
#define GCL_HICON           GCW_HICON
#define GCW_HMODULE         (-16)
#define GCL_HMODULE         GCW_HMODULE
#define GCW_CBWNDEXTRA      (-18)
#define GCL_CBWNDEXTRA      GCW_CBWNDEXTRA
#define GCW_CBCLSEXTRA      (-20)
#define GCL_CBCLSEXTRA      GCW_CBCLSEXTRA
#define GCL_WNDPROC         (-24)
#define GCW_STYLE           (-26)
#define GCL_STYLE           GCW_STYLE
#define GCW_ATOM            (-32)
#define GCW_HICONSM         (-34)
#define GCL_HICONSM         GCW_HICONSM

#ifndef NOWINOFFSETS
#define GCW_HBRBACKGROUND (-10)
#endif


/***** Window hooks *****/

  /* Hook values */
#define WH_MIN		    (-1)
#define WH_MSGFILTER	    (-1)
#define WH_JOURNALRECORD    0
#define WH_JOURNALPLAYBACK  1
#define WH_KEYBOARD	    2
#define WH_GETMESSAGE	    3
#define WH_CALLWNDPROC	    4
#define WH_CBT		    5
#define WH_SYSMSGFILTER	    6
#define WH_MOUSE	    7
#define WH_HARDWARE	    8
#define WH_DEBUG	    9
#define WH_SHELL            10
#define WH_FOREGROUNDIDLE   11
#define WH_CALLWNDPROCRET   12
#define WH_MAX              12

#define WH_MINHOOK          WH_MIN
#define WH_MAXHOOK          WH_MAX
#define WH_NB_HOOKS         (WH_MAXHOOK-WH_MINHOOK+1)

  /* Hook action codes */
#define HC_ACTION           0
#define HC_GETNEXT          1
#define HC_SKIP             2
#define HC_NOREMOVE         3
#define HC_NOREM            HC_NOREMOVE
#define HC_SYSMODALON       4
#define HC_SYSMODALOFF      5

  /* CallMsgFilter() values */
#define MSGF_DIALOGBOX      0
#define MSGF_MESSAGEBOX     1
#define MSGF_MENU           2
#define MSGF_MOVE           3
#define MSGF_SIZE           4
#define MSGF_SCROLLBAR      5
#define MSGF_NEXTWINDOW     6
#define MSGF_MAINLOOP       8
#define MSGF_USER        4096

typedef struct
{
    UINT32      style;
    WNDPROC32   lpfnWndProc;
    INT32       cbClsExtra;
    INT32       cbWndExtra;
    HINSTANCE32 hInstance;
    HICON32     hIcon;
    HCURSOR32   hCursor;
    HBRUSH32    hbrBackground;
    LPCSTR      lpszMenuName;
    LPCSTR      lpszClassName;
} WNDCLASS32A, *LPWNDCLASS32A;

typedef struct
{
    UINT32      style;
    WNDPROC32   lpfnWndProc;
    INT32       cbClsExtra;
    INT32       cbWndExtra;
    HINSTANCE32 hInstance;
    HICON32     hIcon;
    HCURSOR32   hCursor;
    HBRUSH32    hbrBackground;
    LPCWSTR     lpszMenuName;
    LPCWSTR     lpszClassName;
} WNDCLASS32W, *LPWNDCLASS32W;

DECL_WINELIB_TYPE_AW(WNDCLASS)
DECL_WINELIB_TYPE_AW(LPWNDCLASS)

typedef struct {
    DWORD dwData;
    DWORD cbData;
    LPVOID lpData;
} COPYDATASTRUCT, *PCOPYDATASTRUCT, *LPCOPYDATASTRUCT;

typedef struct {
    HMENU32 hmenuIn;
    HMENU32 hmenuNext;
    HWND32  hwndNext;
} MDINEXTMENU, *PMDINEXTMENU, *LPMDINEXTMENU;

/* WinHelp internal structure */
typedef struct {
	WORD size;
	WORD command;
	LONG data;
	LONG reserved;
	WORD ofsFilename;
	WORD ofsData;
} WINHELP,*LPWINHELP;

typedef struct
{
    UINT16  mkSize;
    BYTE    mkKeyList;
    BYTE    szKeyPhrase[1];
} MULTIKEYHELP, *LPMULTIKEYHELP;

typedef struct {
	WORD wStructSize;
	WORD x;
	WORD y;
	WORD dx;
	WORD dy;
	WORD wMax;
	char rgchMember[2];
} HELPWININFO, *LPHELPWININFO;

#define HELP_CONTEXT        0x0001
#define HELP_QUIT           0x0002
#define HELP_INDEX          0x0003
#define HELP_CONTENTS       0x0003
#define HELP_HELPONHELP     0x0004
#define HELP_SETINDEX       0x0005
#define HELP_SETCONTENTS    0x0005
#define HELP_CONTEXTPOPUP   0x0008
#define HELP_FORCEFILE      0x0009
#define HELP_KEY            0x0101
#define HELP_COMMAND        0x0102
#define HELP_PARTIALKEY     0x0105
#define HELP_MULTIKEY       0x0201
#define HELP_SETWINPOS      0x0203
#define HELP_CONTEXTMENU    0x000a
#define HELP_FINDER	    0x000b
#define HELP_WM_HELP	    0x000c
#define HELP_SETPOPUP_POS   0x000d

#define HELP_TCARD	    0x8000
#define HELP_TCARD_DATA	    0x0010
#define HELP_TCARD_OTHER_CALLER 0x0011


     /* ChangeDisplaySettings return codes */

#define DISP_CHANGE_SUCCESSFUL 0
#define DISP_CHANGE_RESTART    1
#define DISP_CHANGE_FAILED     (-1)
#define DISP_CHANGE_BADMODE    (-2)
#define DISP_CHANGE_NOTUPDATED (-3)
#define DISP_CHANGE_BADFLAGS   (-4)

/* flags to FormatMessage */
#define	FORMAT_MESSAGE_ALLOCATE_BUFFER	0x00000100
#define	FORMAT_MESSAGE_IGNORE_INSERTS	0x00000200
#define	FORMAT_MESSAGE_FROM_STRING	0x00000400
#define	FORMAT_MESSAGE_FROM_HMODULE	0x00000800
#define	FORMAT_MESSAGE_FROM_SYSTEM	0x00001000
#define	FORMAT_MESSAGE_ARGUMENT_ARRAY	0x00002000
#define	FORMAT_MESSAGE_MAX_WIDTH_MASK	0x000000FF

typedef struct
{
    UINT32      cbSize;
    UINT32      style;
    WNDPROC32   lpfnWndProc;
    INT32       cbClsExtra;
    INT32       cbWndExtra;
    HINSTANCE32 hInstance;
    HICON32     hIcon;
    HCURSOR32   hCursor;
    HBRUSH32    hbrBackground;
    LPCSTR      lpszMenuName;
    LPCSTR      lpszClassName;
    HICON32     hIconSm;
} WNDCLASSEX32A, *LPWNDCLASSEX32A;

typedef struct
{
    UINT32      cbSize;
    UINT32      style;
    WNDPROC32   lpfnWndProc;
    INT32       cbClsExtra;
    INT32       cbWndExtra;
    HINSTANCE32 hInstance;
    HICON32     hIcon;
    HCURSOR32   hCursor;
    HBRUSH32    hbrBackground;
    LPCWSTR     lpszMenuName;
    LPCWSTR     lpszClassName;
    HICON32     hIconSm;
} WNDCLASSEX32W, *LPWNDCLASSEX32W;

DECL_WINELIB_TYPE_AW(WNDCLASSEX)
DECL_WINELIB_TYPE_AW(LPWNDCLASSEX)

typedef struct
{
    HWND32    hwnd;
    UINT32    message;
    WPARAM32  wParam;
    LPARAM    lParam;
    DWORD     time;
    POINT32   pt;
} MSG32, *LPMSG32;

DECL_WINELIB_TYPE(MSG)
DECL_WINELIB_TYPE(LPMSG)
	
/* Cursors / Icons */

typedef struct {
	BOOL32		fIcon;
	DWORD		xHotspot;
	DWORD		yHotspot;
	HBITMAP32	hbmMask;
	HBITMAP32	hbmColor;
} ICONINFO32,*LPICONINFO32;

DECL_WINELIB_TYPE(ICONINFO);
DECL_WINELIB_TYPE(LPICONINFO);

/* this is the 6 byte accel struct used in Win32 when presented to the user */
typedef struct
{
    BYTE   fVirt;
    BYTE   pad0;
    WORD   key;
    WORD   cmd;
} ACCEL32, *LPACCEL32;

/* this is the 8 byte accel struct used in Win32 resources (internal only) */
typedef struct
{
    BYTE   fVirt;
    BYTE   pad0;
    WORD   key;
    WORD   cmd;
    WORD   pad1;
} PE_ACCEL, *LPPE_ACCEL;

DECL_WINELIB_TYPE(ACCEL)
DECL_WINELIB_TYPE(LPACCEL)

/* Flags for TrackPopupMenu */
#define TPM_LEFTBUTTON    0x0000
#define TPM_RIGHTBUTTON   0x0002
#define TPM_LEFTALIGN     0x0000
#define TPM_CENTERALIGN   0x0004
#define TPM_RIGHTALIGN    0x0008
#define TPM_TOPALIGN      0x0000
#define TPM_VCENTERALIGN  0x0010
#define TPM_BOTTOMALIGN   0x0020
#define TPM_HORIZONTAL    0x0000
#define TPM_VERTICAL      0x0040
#define TPM_NONOTIFY      0x0080
#define TPM_RETURNCMD     0x0100

typedef struct 
{
    UINT32   cbSize;
    RECT32   rcExclude;
} TPMPARAMS, *LPTPMPARAMS;

/* FIXME: not sure this one is correct */
typedef struct {
  UINT32    cbSize;
  UINT32    fMask;
  UINT32    fType;
  UINT32    fState;
  UINT32    wID;
  HMENU32   hSubMenu;
  HBITMAP32 hbmpChecked;
  HBITMAP32 hbmpUnchecked;
  DWORD     dwItemData;
  LPSTR     dwTypeData;
  UINT32    cch;
} MENUITEMINFO32A, *LPMENUITEMINFO32A;

typedef struct {
  UINT32    cbSize;
  UINT32    fMask;
  UINT32    fType;
  UINT32    fState;
  UINT32    wID;
  HMENU32   hSubMenu;
  HBITMAP32 hbmpChecked;
  HBITMAP32 hbmpUnchecked;
  DWORD     dwItemData;
  LPWSTR    dwTypeData;
  UINT32    cch;
} MENUITEMINFO32W, *LPMENUITEMINFO32W;

DECL_WINELIB_TYPE_AW(MENUITEMINFO)
DECL_WINELIB_TYPE_AW(LPMENUITEMINFO)

/* Field specifiers for MENUITEMINFO[AW] type.  */
#define MIIM_STATE       0x00000001
#define MIIM_ID          0x00000002
#define MIIM_SUBMENU     0x00000004
#define MIIM_CHECKMARKS  0x00000008
#define MIIM_TYPE        0x00000010
#define MIIM_DATA        0x00000020

/* DrawState defines ... */
typedef BOOL32 (CALLBACK *DRAWSTATEPROC32)(HDC32,LPARAM,WPARAM32,INT32,INT32);
DECL_WINELIB_TYPE(DRAWSTATEPROC)

/* WM_H/VSCROLL commands */
#define SB_LINEUP           0
#define SB_LINELEFT         0
#define SB_LINEDOWN         1
#define SB_LINERIGHT        1
#define SB_PAGEUP           2
#define SB_PAGELEFT         2
#define SB_PAGEDOWN         3
#define SB_PAGERIGHT        3
#define SB_THUMBPOSITION    4
#define SB_THUMBTRACK       5
#define SB_TOP              6
#define SB_LEFT             6
#define SB_BOTTOM           7
#define SB_RIGHT            7
#define SB_ENDSCROLL        8

/* Scroll bar selection constants */
#define SB_HORZ             0
#define SB_VERT             1
#define SB_CTL              2
#define SB_BOTH             3

/* Scrollbar styles */
#define SBS_HORZ                    0x0000L
#define SBS_VERT                    0x0001L
#define SBS_TOPALIGN                0x0002L
#define SBS_LEFTALIGN               0x0002L
#define SBS_BOTTOMALIGN             0x0004L
#define SBS_RIGHTALIGN              0x0004L
#define SBS_SIZEBOXTOPLEFTALIGN     0x0002L
#define SBS_SIZEBOXBOTTOMRIGHTALIGN 0x0004L
#define SBS_SIZEBOX                 0x0008L
#define SBS_SIZEGRIP                0x0010L

/* EnableScrollBar() flags */
#define ESB_ENABLE_BOTH     0x0000
#define ESB_DISABLE_BOTH    0x0003

#define ESB_DISABLE_LEFT    0x0001
#define ESB_DISABLE_RIGHT   0x0002

#define ESB_DISABLE_UP      0x0001
#define ESB_DISABLE_DOWN    0x0002

#define ESB_DISABLE_LTUP    ESB_DISABLE_LEFT
#define ESB_DISABLE_RTDN    ESB_DISABLE_RIGHT

/* Win32 button control messages */
#define BM_GETCHECK32          0x00f0
#define BM_SETCHECK32          0x00f1
#define BM_GETSTATE32          0x00f2
#define BM_SETSTATE32          0x00f3
#define BM_SETSTYLE32          0x00f4
#define BM_CLICK32             0x00f5
#define BM_GETIMAGE32          0x00f6
#define BM_SETIMAGE32          0x00f7
/* Winelib button control messages */
#define BM_GETCHECK            WINELIB_NAME(BM_GETCHECK)
#define BM_SETCHECK            WINELIB_NAME(BM_SETCHECK)
#define BM_GETSTATE            WINELIB_NAME(BM_GETSTATE)
#define BM_SETSTATE            WINELIB_NAME(BM_SETSTATE)
#define BM_SETSTYLE            WINELIB_NAME(BM_SETSTYLE)
#define BM_CLICK               WINELIB_NAME(BM_CLICK)
#define BM_GETIMAGE            WINELIB_NAME(BM_GETIMAGE)
#define BM_SETIMAGE            WINELIB_NAME(BM_SETIMAGE)

/* Button notification codes */
#define BN_CLICKED             0
#define BN_PAINT               1
#define BN_HILITE              2
#define BN_UNHILITE            3
#define BN_DISABLE             4
#define BN_DOUBLECLICKED       5

/* Static Control Styles */
#define SS_LEFT             0x00000000L
#define SS_CENTER           0x00000001L
#define SS_RIGHT            0x00000002L
#define SS_ICON             0x00000003L
#define SS_BLACKRECT        0x00000004L
#define SS_GRAYRECT         0x00000005L
#define SS_WHITERECT        0x00000006L
#define SS_BLACKFRAME       0x00000007L
#define SS_GRAYFRAME        0x00000008L
#define SS_WHITEFRAME       0x00000009L

#define SS_SIMPLE           0x0000000BL
#define SS_LEFTNOWORDWRAP   0x0000000CL

#define SS_OWNERDRAW        0x0000000DL
#define SS_BITMAP           0x0000000EL
#define SS_ENHMETAFILE      0x0000000FL

#define SS_ETCHEDHORZ       0x00000010L
#define SS_ETCHEDVERT       0x00000011L
#define SS_ETCHEDFRAME      0x00000012L
#define SS_TYPEMASK         0x0000001FL

#define SS_NOPREFIX         0x00000080L
#define SS_NOTIFY           0x00000100L
#define SS_CENTERIMAGE      0x00000200L
#define SS_RIGHTJUST        0x00000400L
#define SS_REALSIZEIMAGE    0x00000800L
#define SS_SUNKEN           0x00001000L

/* Static Control Messages */
#define STM_SETICON32       0x0170
#define STM_SETICON	    WINELIB_NAME(STM_SETICON)
#define STM_GETICON32       0x0171
#define STM_GETICON	    WINELIB_NAME(STM_GETICON)
#define STM_SETIMAGE        0x0172
#define STM_GETIMAGE        0x0173

/* Scrollbar messages */
#define SBM_SETPOS32             0x00e0
#define SBM_SETPOS               WINELIB_NAME(SBM_SETPOS)
#define SBM_GETPOS32             0x00e1
#define SBM_GETPOS               WINELIB_NAME(SBM_GETPOS)
#define SBM_SETRANGE32           0x00e2
#define SBM_SETRANGE             WINELIB_NAME(SBM_SETRANGE)
#define SBM_GETRANGE32           0x00e3
#define SBM_GETRANGE             WINELIB_NAME(SBM_GETRANGE)
#define SBM_ENABLE_ARROWS32      0x00e4
#define SBM_ENABLE_ARROWS        WINELIB_NAME(SBM_ENABLE_ARROWS)
#define SBM_SETRANGEREDRAW32     0x00e6
#define SBM_SETRANGEREDRAW       WINELIB_NAME(SBM_SETRANGEREDRAW)
#define SBM_SETSCROLLINFO32      0x00e9
#define SBM_SETSCROLLINFO        WINELIB_NAME(SBM_SETSCROLLINFO)
#define SBM_GETSCROLLINFO32      0x00ea
#define SBM_GETSCROLLINFO        WINELIB_NAME(SBM_GETSCROLLINFO)

/* Scrollbar info */
typedef struct
{
    UINT32    cbSize;
    UINT32    fMask;
    INT32     nMin;
    INT32     nMax;
    UINT32    nPage;
    INT32     nPos;
    INT32     nTrackPos;
} SCROLLINFO, *LPSCROLLINFO;
 
/* GetScrollInfo() flags */ 
#define SIF_RANGE           0x0001
#define SIF_PAGE            0x0002
#define SIF_POS             0x0004
#define SIF_DISABLENOSCROLL 0x0008
#define SIF_TRACKPOS        0x0010
#define SIF_ALL             (SIF_RANGE | SIF_PAGE | SIF_POS | SIF_TRACKPOS)

/* Listbox styles */
#define LBS_NOTIFY               0x0001
#define LBS_SORT                 0x0002
#define LBS_NOREDRAW             0x0004
#define LBS_MULTIPLESEL          0x0008
#define LBS_OWNERDRAWFIXED       0x0010
#define LBS_OWNERDRAWVARIABLE    0x0020
#define LBS_HASSTRINGS           0x0040
#define LBS_USETABSTOPS          0x0080
#define LBS_NOINTEGRALHEIGHT     0x0100
#define LBS_MULTICOLUMN          0x0200
#define LBS_WANTKEYBOARDINPUT    0x0400
#define LBS_EXTENDEDSEL          0x0800
#define LBS_DISABLENOSCROLL      0x1000
#define LBS_NODATA               0x2000
#define LBS_NOSEL                0x4000
#define LBS_STANDARD  (LBS_NOTIFY | LBS_SORT | WS_VSCROLL | WS_BORDER)

/* Listbox messages */
#define LB_ADDSTRING32           0x0180
#define LB_ADDSTRING             WINELIB_NAME(LB_ADDSTRING)
#define LB_INSERTSTRING32        0x0181
#define LB_INSERTSTRING          WINELIB_NAME(LB_INSERTSTRING)
#define LB_DELETESTRING32        0x0182
#define LB_DELETESTRING          WINELIB_NAME(LB_DELETESTRING)
#define LB_SELITEMRANGEEX32      0x0183
#define LB_SELITEMRANGEEX        WINELIB_NAME(LB_SELITEMRANGEEX)
#define LB_RESETCONTENT32        0x0184
#define LB_RESETCONTENT          WINELIB_NAME(LB_RESETCONTENT)
#define LB_SETSEL32              0x0185
#define LB_SETSEL                WINELIB_NAME(LB_SETSEL)
#define LB_SETCURSEL32           0x0186
#define LB_SETCURSEL             WINELIB_NAME(LB_SETCURSEL)
#define LB_GETSEL32              0x0187
#define LB_GETSEL                WINELIB_NAME(LB_GETSEL)
#define LB_GETCURSEL32           0x0188
#define LB_GETCURSEL             WINELIB_NAME(LB_GETCURSEL)
#define LB_GETTEXT32             0x0189
#define LB_GETTEXT               WINELIB_NAME(LB_GETTEXT)
#define LB_GETTEXTLEN32          0x018a
#define LB_GETTEXTLEN            WINELIB_NAME(LB_GETTEXTLEN)
#define LB_GETCOUNT32            0x018b
#define LB_GETCOUNT              WINELIB_NAME(LB_GETCOUNT)
#define LB_SELECTSTRING32        0x018c
#define LB_SELECTSTRING          WINELIB_NAME(LB_SELECTSTRING)
#define LB_DIR32                 0x018d
#define LB_DIR                   WINELIB_NAME(LB_DIR)
#define LB_GETTOPINDEX32         0x018e
#define LB_GETTOPINDEX           WINELIB_NAME(LB_GETTOPINDEX)
#define LB_FINDSTRING32          0x018f
#define LB_FINDSTRING            WINELIB_NAME(LB_FINDSTRING)
#define LB_GETSELCOUNT32         0x0190
#define LB_GETSELCOUNT           WINELIB_NAME(LB_GETSELCOUNT)
#define LB_GETSELITEMS32         0x0191
#define LB_GETSELITEMS           WINELIB_NAME(LB_GETSELITEMS)
#define LB_SETTABSTOPS32         0x0192
#define LB_SETTABSTOPS           WINELIB_NAME(LB_SETTABSTOPS)
#define LB_GETHORIZONTALEXTENT32 0x0193
#define LB_GETHORIZONTALEXTENT   WINELIB_NAME(LB_GETHORIZONTALEXTENT)
#define LB_SETHORIZONTALEXTENT32 0x0194
#define LB_SETHORIZONTALEXTENT   WINELIB_NAME(LB_SETHORIZONTALEXTENT)
#define LB_SETCOLUMNWIDTH32      0x0195
#define LB_SETCOLUMNWIDTH        WINELIB_NAME(LB_SETCOLUMNWIDTH)
#define LB_ADDFILE32             0x0196
#define LB_ADDFILE               WINELIB_NAME(LB_ADDFILE)
#define LB_SETTOPINDEX32         0x0197
#define LB_SETTOPINDEX           WINELIB_NAME(LB_SETTOPINDEX)
#define LB_GETITEMRECT32         0x0198
#define LB_GETITEMRECT           WINELIB_NAME(LB_GETITEMRECT)
#define LB_GETITEMDATA32         0x0199
#define LB_GETITEMDATA           WINELIB_NAME(LB_GETITEMDATA)
#define LB_SETITEMDATA32         0x019a
#define LB_SETITEMDATA           WINELIB_NAME(LB_SETITEMDATA)
#define LB_SELITEMRANGE32        0x019b
#define LB_SELITEMRANGE          WINELIB_NAME(LB_SELITEMRANGE)
#define LB_SETANCHORINDEX32      0x019c
#define LB_SETANCHORINDEX        WINELIB_NAME(LB_SETANCHORINDEX)
#define LB_GETANCHORINDEX32      0x019d
#define LB_GETANCHORINDEX        WINELIB_NAME(LB_GETANCHORINDEX)
#define LB_SETCARETINDEX32       0x019e
#define LB_SETCARETINDEX         WINELIB_NAME(LB_SETCARETINDEX)
#define LB_GETCARETINDEX32       0x019f
#define LB_GETCARETINDEX         WINELIB_NAME(LB_GETCARETINDEX)
#define LB_SETITEMHEIGHT32       0x01a0
#define LB_SETITEMHEIGHT         WINELIB_NAME(LB_SETITEMHEIGHT)
#define LB_GETITEMHEIGHT32       0x01a1
#define LB_GETITEMHEIGHT         WINELIB_NAME(LB_GETITEMHEIGHT)
#define LB_FINDSTRINGEXACT32     0x01a2
#define LB_FINDSTRINGEXACT       WINELIB_NAME(LB_FINDSTRINGEXACT)
#define LB_CARETON32             0x01a3
#define LB_CARETON               WINELIB_NAME(LB_CARETON)
#define LB_CARETOFF32            0x01a4
#define LB_CARETOFF              WINELIB_NAME(LB_CARETOFF)
#define LB_SETLOCALE32           0x01a5
#define LB_SETLOCALE             WINELIB_NAME(LB_SETLOCALE)
#define LB_GETLOCALE32           0x01a6
#define LB_GETLOCALE             WINELIB_NAME(LB_GETLOCALE)
#define LB_SETCOUNT32            0x01a7
#define LB_SETCOUNT              WINELIB_NAME(LB_SETCOUNT)
#define LB_INITSTORAGE32         0x01a8
#define LB_INITSTORAGE           WINELIB_NAME(LB_INITSTORAGE)
#define LB_ITEMFROMPOINT32       0x01a9
#define LB_ITEMFROMPOINT         WINELIB_NAME(LB_ITEMFROMPOINT)

/* Listbox notification codes */
#define LBN_ERRSPACE        (-2)
#define LBN_SELCHANGE       1
#define LBN_DBLCLK          2
#define LBN_SELCANCEL       3
#define LBN_SETFOCUS        4
#define LBN_KILLFOCUS       5

/* Listbox message return values */
#define LB_OKAY             0
#define LB_ERR              (-1)
#define LB_ERRSPACE         (-2)

#define LB_CTLCODE          0L

/* Combo box styles */
#define CBS_SIMPLE            0x0001L
#define CBS_DROPDOWN          0x0002L
#define CBS_DROPDOWNLIST      0x0003L
#define CBS_OWNERDRAWFIXED    0x0010L
#define CBS_OWNERDRAWVARIABLE 0x0020L
#define CBS_AUTOHSCROLL       0x0040L
#define CBS_OEMCONVERT        0x0080L
#define CBS_SORT              0x0100L
#define CBS_HASSTRINGS        0x0200L
#define CBS_NOINTEGRALHEIGHT  0x0400L
#define CBS_DISABLENOSCROLL   0x0800L

#define CBS_UPPERCASE	      0x2000L
#define CBS_LOWERCASE	      0x4000L


/* Combo box messages */
#define CB_GETEDITSEL32            0x0140
#define CB_GETEDITSEL              WINELIB_NAME(CB_GETEDITSEL)
#define CB_LIMITTEXT32             0x0141
#define CB_LIMITTEXT               WINELIB_NAME(CB_LIMITTEXT)
#define CB_SETEDITSEL32            0x0142
#define CB_SETEDITSEL              WINELIB_NAME(CB_SETEDITSEL)
#define CB_ADDSTRING32             0x0143
#define CB_ADDSTRING               WINELIB_NAME(CB_ADDSTRING)
#define CB_DELETESTRING32          0x0144
#define CB_DELETESTRING            WINELIB_NAME(CB_DELETESTRING)
#define CB_DIR32                   0x0145
#define CB_DIR                     WINELIB_NAME(CB_DIR)
#define CB_GETCOUNT32              0x0146
#define CB_GETCOUNT                WINELIB_NAME(CB_GETCOUNT)
#define CB_GETCURSEL32             0x0147
#define CB_GETCURSEL               WINELIB_NAME(CB_GETCURSEL)
#define CB_GETLBTEXT32             0x0148
#define CB_GETLBTEXT               WINELIB_NAME(CB_GETLBTEXT)
#define CB_GETLBTEXTLEN32          0x0149
#define CB_GETLBTEXTLEN            WINELIB_NAME(CB_GETLBTEXTLEN)
#define CB_INSERTSTRING32          0x014a
#define CB_INSERTSTRING            WINELIB_NAME(CB_INSERTSTRING)
#define CB_RESETCONTENT32          0x014b
#define CB_RESETCONTENT            WINELIB_NAME(CB_RESETCONTENT)
#define CB_FINDSTRING32            0x014c
#define CB_FINDSTRING              WINELIB_NAME(CB_FINDSTRING)
#define CB_SELECTSTRING32          0x014d
#define CB_SELECTSTRING            WINELIB_NAME(CB_SELECTSTRING)
#define CB_SETCURSEL32             0x014e
#define CB_SETCURSEL               WINELIB_NAME(CB_SETCURSEL)
#define CB_SHOWDROPDOWN32          0x014f
#define CB_SHOWDROPDOWN            WINELIB_NAME(CB_SHOWDROPDOWN)
#define CB_GETITEMDATA32           0x0150
#define CB_GETITEMDATA             WINELIB_NAME(CB_GETITEMDATA)
#define CB_SETITEMDATA32           0x0151
#define CB_SETITEMDATA             WINELIB_NAME(CB_SETITEMDATA)
#define CB_GETDROPPEDCONTROLRECT32 0x0152
#define CB_GETDROPPEDCONTROLRECT   WINELIB_NAME(CB_GETDROPPEDCONTROLRECT)
#define CB_SETITEMHEIGHT32         0x0153
#define CB_SETITEMHEIGHT           WINELIB_NAME(CB_SETITEMHEIGHT)
#define CB_GETITEMHEIGHT32         0x0154
#define CB_GETITEMHEIGHT           WINELIB_NAME(CB_GETITEMHEIGHT)
#define CB_SETEXTENDEDUI32         0x0155
#define CB_SETEXTENDEDUI           WINELIB_NAME(CB_SETEXTENDEDUI)
#define CB_GETEXTENDEDUI32         0x0156
#define CB_GETEXTENDEDUI           WINELIB_NAME(CB_GETEXTENDEDUI)
#define CB_GETDROPPEDSTATE32       0x0157
#define CB_GETDROPPEDSTATE         WINELIB_NAME(CB_GETDROPPEDSTATE)
#define CB_FINDSTRINGEXACT32       0x0158
#define CB_FINDSTRINGEXACT         WINELIB_NAME(CB_FINDSTRINGEXACT)
#define CB_SETLOCALE32             0x0159
#define CB_SETLOCALE               WINELIB_NAME(CB_SETLOCALE)
#define CB_GETLOCALE32             0x015a
#define CB_GETLOCALE               WINELIB_NAME(CB_GETLOCALE)
#define CB_GETTOPINDEX32           0x015b
#define CB_GETTOPINDEX             WINELIB_NAME(CB_GETTOPINDEX)
#define CB_SETTOPINDEX32           0x015c
#define CB_SETTOPINDEX             WINELIB_NAME(CB_SETTOPINDEX)
#define CB_GETHORIZONTALEXTENT32   0x015d
#define CB_GETHORIZONTALEXTENT     WINELIB_NAME(CB_GETHORIZONTALEXTENT)
#define CB_SETHORIZONTALEXTENT32   0x015e
#define CB_SETHORIZONTALEXTENT     WINELIB_NAME(CB_SETHORIZONTALEXTENT)
#define CB_GETDROPPEDWIDTH32       0x015f
#define CB_GETDROPPEDWIDTH         WINELIB_NAME(CB_GETDROPPEDWIDTH)
#define CB_SETDROPPEDWIDTH32       0x0160
#define CB_SETDROPPEDWIDTH         WINELIB_NAME(CB_SETDROPPEDWIDTH)
#define CB_INITSTORAGE32           0x0161
#define CB_INITSTORAGE             WINELIB_NAME(CB_INITSTORAGE)

/* Combo box notification codes */
#define CBN_ERRSPACE        (-1)
#define CBN_SELCHANGE       1
#define CBN_DBLCLK          2
#define CBN_SETFOCUS        3
#define CBN_KILLFOCUS       4
#define CBN_EDITCHANGE      5
#define CBN_EDITUPDATE      6
#define CBN_DROPDOWN        7
#define CBN_CLOSEUP         8
#define CBN_SELENDOK        9
#define CBN_SELENDCANCEL    10

/* Combo box message return values */
#define CB_OKAY             0
#define CB_ERR              (-1)
#define CB_ERRSPACE         (-2)

#define MB_OK			0x00000000
#define MB_OKCANCEL		0x00000001
#define MB_ABORTRETRYIGNORE	0x00000002
#define MB_YESNOCANCEL		0x00000003
#define MB_YESNO		0x00000004
#define MB_RETRYCANCEL		0x00000005
#define MB_TYPEMASK		0x0000000F

#define MB_ICONHAND		0x00000010
#define MB_ICONQUESTION		0x00000020
#define MB_ICONEXCLAMATION	0x00000030
#define MB_ICONASTERISK		0x00000040
#define	MB_USERICON		0x00000080
#define MB_ICONMASK		0x000000F0

#define MB_ICONINFORMATION	MB_ICONASTERISK
#define MB_ICONSTOP		MB_ICONHAND
#define MB_ICONWARNING		MB_ICONEXCLAMATION
#define MB_ICONERROR		MB_ICONHAND

#define MB_DEFBUTTON1		0x00000000
#define MB_DEFBUTTON2		0x00000100
#define MB_DEFBUTTON3		0x00000200
#define MB_DEFBUTTON4		0x00000300
#define MB_DEFMASK		0x00000F00

#define MB_APPLMODAL		0x00000000
#define MB_SYSTEMMODAL		0x00001000
#define MB_TASKMODAL		0x00002000
#define MB_MODEMASK		0x00003000

#define MB_HELP			0x00004000
#define MB_NOFOCUS		0x00008000
#define MB_MISCMASK		0x0000C000

#define MB_SETFOREGROUND	0x00010000
#define MB_DEFAULT_DESKTOP_ONLY	0x00020000
#define MB_SERVICE_NOTIFICATION	0x00040000
#define MB_TOPMOST		0x00040000
#define MB_RIGHT		0x00080000
#define MB_RTLREADING		0x00100000

#define	HELPINFO_WINDOW		0x0001
#define	HELPINFO_MENUITEM	0x0002
typedef struct			/* Structure pointed to by lParam of WM_HELP */
{
    UINT32	cbSize;		/* Size in bytes of this struct  */
    INT32	iContextType;	/* Either HELPINFO_WINDOW or HELPINFO_MENUITEM */
    INT32	iCtrlId;	/* Control Id or a Menu item Id. */
    HANDLE32	hItemHandle;	/* hWnd of control or hMenu.     */
    DWORD	dwContextId;	/* Context Id associated with this item */
    POINT32	MousePos;	/* Mouse Position in screen co-ordinates */
}  HELPINFO,*LPHELPINFO;

typedef void (CALLBACK *MSGBOXCALLBACK)(LPHELPINFO lpHelpInfo);

typedef struct
{
    UINT32	cbSize;
    HWND32	hwndOwner;
    HINSTANCE32	hInstance;
    LPCSTR	lpszText;
    LPCSTR	lpszCaption;
    DWORD	dwStyle;
    LPCSTR	lpszIcon;
    DWORD	dwContextHelpId;
    MSGBOXCALLBACK	lpfnMsgBoxCallback;
    DWORD	dwLanguageId;
} MSGBOXPARAMS32A,*LPMSGBOXPARAMS32A;

typedef struct
{
    UINT32	cbSize;
    HWND32	hwndOwner;
    HINSTANCE32	hInstance;
    LPCWSTR	lpszText;
    LPCWSTR	lpszCaption;
    DWORD	dwStyle;
    LPCWSTR	lpszIcon;
    DWORD	dwContextHelpId;
    MSGBOXCALLBACK	lpfnMsgBoxCallback;
    DWORD	dwLanguageId;
} MSGBOXPARAMS32W,*LPMSGBOXPARAMS32W;

DECL_WINELIB_TYPE_AW(MSGBOXPARAMS)
DECL_WINELIB_TYPE_AW(LPMSGBOXPARAMS)

typedef struct _numberfmt32a {
    UINT32 NumDigits;
    UINT32 LeadingZero;
    UINT32 Grouping;
    LPCSTR lpDecimalSep;
    LPCSTR lpThousandSep;
    UINT32 NegativeOrder;
} NUMBERFMT32A;

typedef struct _numberfmt32w {
    UINT32 NumDigits;
    UINT32 LeadingZero;
    UINT32 Grouping;
    LPCWSTR lpDecimalSep;
    LPCWSTR lpThousandSep;
    UINT32 NegativeOrder;
} NUMBERFMT32W;

#define MONITOR_DEFAULTTONULL       0x00000000
#define MONITOR_DEFAULTTOPRIMARY    0x00000001
#define MONITOR_DEFAULTTONEAREST    0x00000002

#define MONITORINFOF_PRIMARY        0x00000001

typedef struct tagMONITORINFO
{
    DWORD   cbSize;
    RECT32  rcMonitor;
    RECT32  rcWork;
    DWORD   dwFlags;
} MONITORINFO, *LPMONITORINFO;

typedef struct tagMONITORINFOEX32A
{
    MONITORINFO dummy;
    CHAR        szDevice[CCHDEVICENAME];
} MONITORINFOEX32A, *LPMONITORINFOEX32A;

typedef struct tagMONITORINFOEX32W
{
    MONITORINFO dummy;
    WCHAR       szDevice[CCHDEVICENAME];
} MONITORINFOEX32W, *LPMONITORINFOEX32W;

DECL_WINELIB_TYPE_AW(MONITORINFOEX)
DECL_WINELIB_TYPE_AW(LPMONITORINFOEX)

typedef BOOL32  (CALLBACK *MONITORENUMPROC)(HMONITOR,HDC32,LPRECT32,LPARAM);

typedef struct tagDLGTEMPLATE
{
    DWORD style;
    DWORD dwExtendedStyle;
    WORD cdit;
    short x;
    short y;
    short cx;
    short cy;
}DLGTEMPLATE, *LPDLGTEMPLATE;
typedef const DLGTEMPLATE *LPCDLGTEMPLATE;
/* Fixme: use this instaed of LPCVOID for CreateDialogIndirectParam and DialogBoxIndirectParam*/
typedef struct tagDLGITEMTEMPLATE
{
    DWORD style;
    DWORD dwExtendedStyle;
    WORD cdit;
    short x;
    short y;
    short cx;
    short cy;
    WORD id;
}DLGITEMTEMPLATE, *LPDLGITEMTEMPLATE;

typedef const DLGITEMTEMPLATE *LPCDLGITEMTEMPLATE;

  /* CBT hook values */
#define HCBT_MOVESIZE	    0
#define HCBT_MINMAX	    1
#define HCBT_QS 	    2
#define HCBT_CREATEWND	    3
#define HCBT_DESTROYWND	    4
#define HCBT_ACTIVATE	    5
#define HCBT_CLICKSKIPPED   6
#define HCBT_KEYSKIPPED     7
#define HCBT_SYSCOMMAND	    8
#define HCBT_SETFOCUS	    9

  /* CBT hook structures */

typedef struct
{
    CREATESTRUCT32A *lpcs;
    HWND32           hwndInsertAfter;
} CBT_CREATEWND32A, *LPCBT_CREATEWND32A;

typedef struct
{
    CREATESTRUCT32W *lpcs;
    HWND32           hwndInsertAfter;
} CBT_CREATEWND32W, *LPCBT_CREATEWND32W;

DECL_WINELIB_TYPE_AW(CBT_CREATEWND)
DECL_WINELIB_TYPE_AW(LPCBT_CREATEWND)

typedef struct
{
    BOOL32    fMouse;
    HWND32    hWndActive;
} CBTACTIVATESTRUCT32, *LPCBTACTIVATESTRUCT32;

DECL_WINELIB_TYPE(CBTACTIVATESTRUCT)
DECL_WINELIB_TYPE(LPCBTACTIVATESTRUCT)

/* modifiers for RegisterHotKey */
#define	MOD_ALT		0x0001
#define	MOD_CONTROL	0x0002
#define	MOD_SHIFT	0x0004
#define	MOD_WIN		0x0008

/* ids for RegisterHotKey */
#define	IDHOT_SNAPWINDOW	(-1)    /* SHIFT-PRINTSCRN  */
#define	IDHOT_SNAPDESKTOP	(-2)    /* PRINTSCRN        */

  /* keybd_event flags */
#define KEYEVENTF_EXTENDEDKEY        0x0001
#define KEYEVENTF_KEYUP              0x0002
#define KEYEVENTF_WINE_FORCEEXTENDED 0x8000

  /* mouse_event flags */
#define MOUSEEVENTF_MOVE        0x0001
#define MOUSEEVENTF_LEFTDOWN    0x0002
#define MOUSEEVENTF_LEFTUP      0x0004
#define MOUSEEVENTF_RIGHTDOWN   0x0008
#define MOUSEEVENTF_RIGHTUP     0x0010
#define MOUSEEVENTF_MIDDLEDOWN  0x0020
#define MOUSEEVENTF_MIDDLEUP    0x0040
#define MOUSEEVENTF_ABSOLUTE    0x8000

/* ExitWindows() flags */
#define EW_RESTARTWINDOWS   0x0042
#define EW_REBOOTSYSTEM     0x0043
#define EW_EXITANDEXECAPP   0x0044

/* ExitWindowsEx() flags */
#define EWX_LOGOFF           0
#define EWX_SHUTDOWN         1
#define EWX_REBOOT           2
#define EWX_FORCE            4
#define EWX_POWEROFF         8

/* SetLastErrorEx types */
#define	SLE_ERROR	0x00000001
#define	SLE_MINORERROR	0x00000002
#define	SLE_WARNING	0x00000003

/* Predefined resources */
#define IDI_APPLICATION32A MAKEINTRESOURCE32A(32512)
#define IDI_APPLICATION32W MAKEINTRESOURCE32W(32512)
#define IDI_APPLICATION    WINELIB_NAME_AW(IDI_APPLICATION)
#define IDI_HAND32A        MAKEINTRESOURCE32A(32513)
#define IDI_HAND32W        MAKEINTRESOURCE32W(32513)
#define IDI_HAND           WINELIB_NAME_AW(IDI_HAND)
#define IDI_QUESTION32A    MAKEINTRESOURCE32A(32514)
#define IDI_QUESTION32W    MAKEINTRESOURCE32W(32514)
#define IDI_QUESTION       WINELIB_NAME_AW(IDI_QUESTION)
#define IDI_EXCLAMATION32A MAKEINTRESOURCE32A(32515)
#define IDI_EXCLAMATION32W MAKEINTRESOURCE32W(32515)
#define IDI_EXCLAMATION    WINELIB_NAME_AW(IDI_EXCLAMATION)
#define IDI_ASTERISK32A    MAKEINTRESOURCE32A(32516)
#define IDI_ASTERISK32W    MAKEINTRESOURCE32W(32516)
#define IDI_ASTERISK       WINELIB_NAME_AW(IDI_ASTERISK)

#define IDC_BUMMER32A      MAKEINTRESOURCE32A(100)
#define IDC_BUMMER32W      MAKEINTRESOURCE32W(100)
#define IDC_BUMMER         WINELIB_NAME_AW(IDC_BUMMER)
#define IDC_ARROW32A       MAKEINTRESOURCE32A(32512)
#define IDC_ARROW32W       MAKEINTRESOURCE32W(32512)
#define IDC_ARROW          WINELIB_NAME_AW(IDC_ARROW)
#define IDC_IBEAM32A       MAKEINTRESOURCE32A(32513)
#define IDC_IBEAM32W       MAKEINTRESOURCE32W(32513)
#define IDC_IBEAM          WINELIB_NAME_AW(IDC_IBEAM)
#define IDC_WAIT32A        MAKEINTRESOURCE32A(32514)
#define IDC_WAIT32W        MAKEINTRESOURCE32W(32514)
#define IDC_WAIT           WINELIB_NAME_AW(IDC_WAIT)
#define IDC_CROSS32A       MAKEINTRESOURCE32A(32515)
#define IDC_CROSS32W       MAKEINTRESOURCE32W(32515)
#define IDC_CROSS          WINELIB_NAME_AW(IDC_CROSS)
#define IDC_UPARROW32A     MAKEINTRESOURCE32A(32516)
#define IDC_UPARROW32W     MAKEINTRESOURCE32W(32516)
#define IDC_UPARROW        WINELIB_NAME_AW(IDC_UPARROW)
#define IDC_SIZE32A        MAKEINTRESOURCE32A(32640)
#define IDC_SIZE32W        MAKEINTRESOURCE32W(32640)
#define IDC_SIZE           WINELIB_NAME_AW(IDC_SIZE)
#define IDC_ICON32A        MAKEINTRESOURCE32A(32641)
#define IDC_ICON32W        MAKEINTRESOURCE32W(32641)
#define IDC_ICON           WINELIB_NAME_AW(IDC_ICON)
#define IDC_SIZENWSE32A    MAKEINTRESOURCE32A(32642)
#define IDC_SIZENWSE32W    MAKEINTRESOURCE32W(32642)
#define IDC_SIZENWSE       WINELIB_NAME_AW(IDC_SIZENWSE)
#define IDC_SIZENESW32A    MAKEINTRESOURCE32A(32643)
#define IDC_SIZENESW32W    MAKEINTRESOURCE32W(32643)
#define IDC_SIZENESW       WINELIB_NAME_AW(IDC_SIZENESW)
#define IDC_SIZEWE32A      MAKEINTRESOURCE32A(32644)
#define IDC_SIZEWE32W      MAKEINTRESOURCE32W(32644)
#define IDC_SIZEWE         WINELIB_NAME_AW(IDC_SIZEWE)
#define IDC_SIZENS32A      MAKEINTRESOURCE32A(32645)
#define IDC_SIZENS32W      MAKEINTRESOURCE32W(32645)
#define IDC_SIZENS         WINELIB_NAME_AW(IDC_SIZENS)
#define IDC_SIZEALL32A     MAKEINTRESOURCE32A(32646)
#define IDC_SIZEALL32W     MAKEINTRESOURCE32W(32646)
#define IDC_SIZEALL        WINELIB_NAME_AW(IDC_SIZEALL)
#define IDC_NO32A          MAKEINTRESOURCE32A(32648)
#define IDC_NO32W          MAKEINTRESOURCE32W(32648)
#define IDC_NO             WINELIB_NAME_AW(IDC_NO)
#define IDC_APPSTARTING32A MAKEINTRESOURCE32A(32650)
#define IDC_APPSTARTING32W MAKEINTRESOURCE32W(32650)
#define IDC_APPSTARTING    WINELIB_NAME_AW(IDC_APPSTARTING)
#define IDC_HELP32A        MAKEINTRESOURCE32A(32651)
#define IDC_HELP32W        MAKEINTRESOURCE32W(32651)
#define IDC_HELP           WINELIB_NAME_AW(IDC_HELP)

/* SystemParametersInfo */
/* defines below are for all win versions */
#define SPI_GETBEEP               1
#define SPI_SETBEEP               2
#define SPI_GETMOUSE              3
#define SPI_SETMOUSE              4
#define SPI_GETBORDER             5
#define SPI_SETBORDER             6
#define SPI_GETKEYBOARDSPEED      10
#define SPI_SETKEYBOARDSPEED      11
#define SPI_LANGDRIVER            12
#define SPI_ICONHORIZONTALSPACING 13
#define SPI_GETSCREENSAVETIMEOUT  14
#define SPI_SETSCREENSAVETIMEOUT  15
#define SPI_GETSCREENSAVEACTIVE   16
#define SPI_SETSCREENSAVEACTIVE   17
#define SPI_GETGRIDGRANULARITY    18
#define SPI_SETGRIDGRANULARITY    19
#define SPI_SETDESKWALLPAPER      20
#define SPI_SETDESKPATTERN        21
#define SPI_GETKEYBOARDDELAY      22
#define SPI_SETKEYBOARDDELAY      23
#define SPI_ICONVERTICALSPACING   24
#define SPI_GETICONTITLEWRAP      25
#define SPI_SETICONTITLEWRAP      26
#define SPI_GETMENUDROPALIGNMENT  27
#define SPI_SETMENUDROPALIGNMENT  28
#define SPI_SETDOUBLECLKWIDTH     29
#define SPI_SETDOUBLECLKHEIGHT    30
#define SPI_GETICONTITLELOGFONT   31
#define SPI_SETDOUBLECLICKTIME    32
#define SPI_SETMOUSEBUTTONSWAP    33
#define SPI_SETICONTITLELOGFONT   34
#define SPI_GETFASTTASKSWITCH     35
#define SPI_SETFASTTASKSWITCH     36
#define SPI_SETDRAGFULLWINDOWS    37
#define SPI_GETDRAGFULLWINDOWS	  38

#define SPI_GETFILTERKEYS         50
#define SPI_SETFILTERKEYS         51
#define SPI_GETTOGGLEKEYS         52
#define SPI_SETTOGGLEKEYS         53
#define SPI_GETMOUSEKEYS          54
#define SPI_SETMOUSEKEYS          55
#define SPI_GETSHOWSOUNDS         56
#define SPI_SETSHOWSOUNDS         57
#define SPI_GETSTICKYKEYS         58
#define SPI_SETSTICKYKEYS         59
#define SPI_GETACCESSTIMEOUT      60
#define SPI_SETACCESSTIMEOUT      61

#define SPI_GETSOUNDSENTRY        64
#define SPI_SETSOUNDSENTRY        65

/* defines below are for all win versions WINVER >= 0x0400 */
#define SPI_SETDRAGFULLWINDOWS    37
#define SPI_GETDRAGFULLWINDOWS    38
#define SPI_GETNONCLIENTMETRICS   41
#define SPI_SETNONCLIENTMETRICS   42
#define SPI_GETMINIMIZEDMETRICS   43
#define SPI_SETMINIMIZEDMETRICS   44
#define SPI_GETICONMETRICS        45
#define SPI_SETICONMETRICS        46
#define SPI_SETWORKAREA           47
#define SPI_GETWORKAREA           48
#define SPI_SETPENWINDOWS         49

#define SPI_GETSERIALKEYS         62
#define SPI_SETSERIALKEYS         63
#define SPI_GETHIGHCONTRAST       66
#define SPI_SETHIGHCONTRAST       67
#define SPI_GETKEYBOARDPREF       68
#define SPI_SETKEYBOARDPREF       69
#define SPI_GETSCREENREADER       70
#define SPI_SETSCREENREADER       71
#define SPI_GETANIMATION          72
#define SPI_SETANIMATION          73
#define SPI_GETFONTSMOOTHING      74
#define SPI_SETFONTSMOOTHING      75
#define SPI_SETDRAGWIDTH          76
#define SPI_SETDRAGHEIGHT         77
#define SPI_SETHANDHELD           78
#define SPI_GETLOWPOWERTIMEOUT    79
#define SPI_GETPOWEROFFTIMEOUT    80
#define SPI_SETLOWPOWERTIMEOUT    81
#define SPI_SETPOWEROFFTIMEOUT    82
#define SPI_GETLOWPOWERACTIVE     83
#define SPI_GETPOWEROFFACTIVE     84
#define SPI_SETLOWPOWERACTIVE     85
#define SPI_SETPOWEROFFACTIVE     86
#define SPI_SETCURSORS            87
#define SPI_SETICONS              88
#define SPI_GETDEFAULTINPUTLANG   89
#define SPI_SETDEFAULTINPUTLANG   90
#define SPI_SETLANGTOGGLE         91
#define SPI_GETWINDOWSEXTENSION   92
#define SPI_SETMOUSETRAILS        93
#define SPI_GETMOUSETRAILS        94
#define SPI_SETSCREENSAVERRUNNING 97
#define SPI_SCREENSAVERRUNNING    SPI_SETSCREENSAVERRUNNING

/* defines below are for all win versions (_WIN32_WINNT >= 0x0400) ||
 *                                        (_WIN32_WINDOWS > 0x0400) */
#define SPI_GETMOUSEHOVERWIDTH    98
#define SPI_SETMOUSEHOVERWIDTH    99
#define SPI_GETMOUSEHOVERHEIGHT   100
#define SPI_SETMOUSEHOVERHEIGHT   101
#define SPI_GETMOUSEHOVERTIME     102
#define SPI_SETMOUSEHOVERTIME     103
#define SPI_GETWHEELSCROLLLINES   104
#define SPI_SETWHEELSCROLLLINES   105

#define SPI_GETSHOWIMEUI          110
#define SPI_SETSHOWIMEUI          111

/* defines below are for all win versions WINVER >= 0x0500 */
#define SPI_GETMOUSESPEED         112
#define SPI_SETMOUSESPEED         113
#define SPI_GETSCREENSAVERRUNNING 114

#define SPI_GETACTIVEWINDOWTRACKING    0x1000
#define SPI_SETACTIVEWINDOWTRACKING    0x1001
#define SPI_GETMENUANIMATION           0x1002
#define SPI_SETMENUANIMATION           0x1003
#define SPI_GETCOMBOBOXANIMATION       0x1004
#define SPI_SETCOMBOBOXANIMATION       0x1005
#define SPI_GETLISTBOXSMOOTHSCROLLING  0x1006
#define SPI_SETLISTBOXSMOOTHSCROLLING  0x1007
#define SPI_GETGRADIENTCAPTIONS        0x1008
#define SPI_SETGRADIENTCAPTIONS        0x1009
#define SPI_GETMENUUNDERLINES          0x100A
#define SPI_SETMENUUNDERLINES          0x100B
#define SPI_GETACTIVEWNDTRKZORDER      0x100C
#define SPI_SETACTIVEWNDTRKZORDER      0x100D
#define SPI_GETHOTTRACKING             0x100E
#define SPI_SETHOTTRACKING             0x100F
#define SPI_GETFOREGROUNDLOCKTIMEOUT   0x2000
#define SPI_SETFOREGROUNDLOCKTIMEOUT   0x2001
#define SPI_GETACTIVEWNDTRKTIMEOUT     0x2002
#define SPI_SETACTIVEWNDTRKTIMEOUT     0x2003
#define SPI_GETFOREGROUNDFLASHCOUNT    0x2004
#define SPI_SETFOREGROUNDFLASHCOUNT    0x2005

/* SystemParametersInfo flags */

#define SPIF_UPDATEINIFILE              1
#define SPIF_SENDWININICHANGE           2
#define SPIF_SENDCHANGE                 SPIF_SENDWININICHANGE

/* Window Styles */
#define WS_OVERLAPPED    0x00000000L
#define WS_POPUP         0x80000000L
#define WS_CHILD         0x40000000L
#define WS_MINIMIZE      0x20000000L
#define WS_VISIBLE       0x10000000L
#define WS_DISABLED      0x08000000L
#define WS_CLIPSIBLINGS  0x04000000L
#define WS_CLIPCHILDREN  0x02000000L
#define WS_MAXIMIZE      0x01000000L
#define WS_CAPTION       0x00C00000L
#define WS_BORDER        0x00800000L
#define WS_DLGFRAME      0x00400000L
#define WS_VSCROLL       0x00200000L
#define WS_HSCROLL       0x00100000L
#define WS_SYSMENU       0x00080000L
#define WS_THICKFRAME    0x00040000L
#define WS_GROUP         0x00020000L
#define WS_TABSTOP       0x00010000L
#define WS_MINIMIZEBOX   0x00020000L
#define WS_MAXIMIZEBOX   0x00010000L
#define WS_TILED         WS_OVERLAPPED
#define WS_ICONIC        WS_MINIMIZE
#define WS_SIZEBOX       WS_THICKFRAME
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME| WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_POPUPWINDOW (WS_POPUP | WS_BORDER | WS_SYSMENU)
#define WS_CHILDWINDOW (WS_CHILD)
#define WS_TILEDWINDOW (WS_OVERLAPPEDWINDOW)

/* Window extended styles */
#define WS_EX_DLGMODALFRAME    0x00000001L
#define WS_EX_DRAGDETECT       0x00000002L
#define WS_EX_NOPARENTNOTIFY   0x00000004L
#define WS_EX_TOPMOST          0x00000008L
#define WS_EX_ACCEPTFILES      0x00000010L
#define WS_EX_TRANSPARENT      0x00000020L

/* New Win95/WinNT4 styles */
#define WS_EX_MDICHILD         0x00000040L
#define WS_EX_TOOLWINDOW       0x00000080L
#define WS_EX_WINDOWEDGE       0x00000100L
#define WS_EX_CLIENTEDGE       0x00000200L
#define WS_EX_CONTEXTHELP      0x00000400L
#define WS_EX_RIGHT            0x00001000L
#define WS_EX_LEFT             0x00000000L
#define WS_EX_RTLREADING       0x00002000L
#define WS_EX_LTRREADING       0x00000000L
#define WS_EX_LEFTSCROLLBAR    0x00004000L
#define WS_EX_RIGHTSCROLLBAR   0x00000000L
#define WS_EX_CONTROLPARENT    0x00010000L
#define WS_EX_STATICEDGE       0x00020000L
#define WS_EX_APPWINDOW        0x00040000L

#define WS_EX_OVERLAPPEDWINDOW (WS_EX_WINDOWEDGE|WS_EX_CLIENTEDGE)
#define WS_EX_PALETTEWINDOW    (WS_EX_WINDOWEDGE|WS_EX_TOOLWINDOW|WS_EX_TOPMOST)

/* Window scrolling */
#define SW_SCROLLCHILDREN      0x0001
#define SW_INVALIDATE          0x0002
#define SW_ERASE               0x0004

/* CreateWindow() coordinates */
#define CW_USEDEFAULT32 ((INT32)0x80000000)
#define CW_USEDEFAULT   WINELIB_NAME(CW_USEDEFAULT)

/* ChildWindowFromPointEx Flags */
#define CWP_ALL                0x0000
#define CWP_SKIPINVISIBLE      0x0001
#define CWP_SKIPDISABLED       0x0002
#define CWP_SKIPTRANSPARENT    0x0004

  /* PeekMessage() options */
#define PM_NOREMOVE	0x0000
#define PM_REMOVE	0x0001
#define PM_NOYIELD	0x0002

#define WM_SHOWWINDOW       0x0018

/* WM_SHOWWINDOW wParam codes */
#define SW_PARENTCLOSING    1
#define SW_OTHERMAXIMIZED   2
#define SW_PARENTOPENING    3
#define SW_OTHERRESTORED    4

  /* ShowWindow() codes */
#define SW_HIDE             0
#define SW_SHOWNORMAL       1
#define SW_NORMAL           1
#define SW_SHOWMINIMIZED    2
#define SW_SHOWMAXIMIZED    3
#define SW_MAXIMIZE         3
#define SW_SHOWNOACTIVATE   4
#define SW_SHOW             5
#define SW_MINIMIZE         6
#define SW_SHOWMINNOACTIVE  7
#define SW_SHOWNA           8
#define SW_RESTORE          9
#define SW_SHOWDEFAULT	    10
#define SW_MAX		    10
#define SW_NORMALNA	    0xCC	/* undoc. flag in MinMaximize */

  /* WM_SIZE message wParam values */
#define SIZE_RESTORED        0
#define SIZE_MINIMIZED       1
#define SIZE_MAXIMIZED       2
#define SIZE_MAXSHOW         3
#define SIZE_MAXHIDE         4
#define SIZENORMAL           SIZE_RESTORED
#define SIZEICONIC           SIZE_MINIMIZED
#define SIZEFULLSCREEN       SIZE_MAXIMIZED
#define SIZEZOOMSHOW         SIZE_MAXSHOW
#define SIZEZOOMHIDE         SIZE_MAXHIDE

/* SetWindowPos() and WINDOWPOS flags */
#define SWP_NOSIZE          0x0001
#define SWP_NOMOVE          0x0002
#define SWP_NOZORDER        0x0004
#define SWP_NOREDRAW        0x0008
#define SWP_NOACTIVATE      0x0010
#define SWP_FRAMECHANGED    0x0020  /* The frame changed: send WM_NCCALCSIZE */
#define SWP_SHOWWINDOW      0x0040
#define SWP_HIDEWINDOW      0x0080
#define SWP_NOCOPYBITS      0x0100
#define SWP_NOOWNERZORDER   0x0200  /* Don't do owner Z ordering */

#define SWP_DRAWFRAME       SWP_FRAMECHANGED
#define SWP_NOREPOSITION    SWP_NOOWNERZORDER

#define SWP_NOSENDCHANGING  0x0400
#define SWP_DEFERERASE      0x2000

#define HWND_DESKTOP        ((HWND32)0)
#define HWND_BROADCAST      ((HWND32)0xffff)

/* SetWindowPos() hwndInsertAfter field values */
#define HWND_TOP            ((HWND32)0)
#define HWND_BOTTOM         ((HWND32)1)
#define HWND_TOPMOST        ((HWND32)-1)
#define HWND_NOTOPMOST      ((HWND32)-2)

#define MF_INSERT          0x0000
#define MF_CHANGE          0x0080
#define MF_APPEND          0x0100
#define MF_DELETE          0x0200
#define MF_REMOVE          0x1000
#define MF_END             0x0080

#define MF_ENABLED         0x0000
#define MF_GRAYED          0x0001
#define MF_DISABLED        0x0002
#define MF_STRING          0x0000
#define MF_BITMAP          0x0004
#define MF_UNCHECKED       0x0000
#define MF_CHECKED         0x0008
#define MF_POPUP           0x0010
#define MF_MENUBARBREAK    0x0020
#define MF_MENUBREAK       0x0040
#define MF_UNHILITE        0x0000
#define MF_HILITE          0x0080
#define MF_OWNERDRAW       0x0100
#define MF_USECHECKBITMAPS 0x0200
#define MF_BYCOMMAND       0x0000
#define MF_BYPOSITION      0x0400
#define MF_SEPARATOR       0x0800
#define MF_DEFAULT         0x1000
#define MF_SYSMENU         0x2000
#define MF_HELP            0x4000
#define MF_RIGHTJUSTIFY    0x4000
#define MF_MOUSESELECT     0x8000

/* Flags for extended menu item types.  */
#define MFT_STRING         MF_STRING
#define MFT_BITMAP         MF_BITMAP
#define MFT_MENUBARBREAK   MF_MENUBARBREAK
#define MFT_MENUBREAK      MF_MENUBREAK
#define MFT_OWNERDRAW      MF_OWNERDRAW
#define MFT_RADIOCHECK     0x00000200L
#define MFT_SEPARATOR      MF_SEPARATOR
#define MFT_RIGHTORDER     0x00002000L
#define MFT_RIGHTJUSTIFY   MF_RIGHTJUSTIFY

/* Flags for extended menu item states.  */
#define MFS_GRAYED          0x00000003L
#define MFS_DISABLED        MFS_GRAYED
#define MFS_CHECKED         MF_CHECKED
#define MFS_HILITE          MF_HILITE
#define MFS_ENABLED         MF_ENABLED
#define MFS_UNCHECKED       MF_UNCHECKED
#define MFS_UNHILITE        MF_UNHILITE
#define MFS_DEFAULT         MF_DEFAULT

#define DT_TOP 0
#define DT_LEFT 0
#define DT_CENTER 1
#define DT_RIGHT 2
#define DT_VCENTER 4
#define DT_BOTTOM 8
#define DT_WORDBREAK 16
#define DT_SINGLELINE 32
#define DT_EXPANDTABS 64
#define DT_TABSTOP 128
#define DT_NOCLIP 256
#define DT_EXTERNALLEADING 512
#define DT_CALCRECT 1024
#define DT_NOPREFIX 2048
#define DT_INTERNAL 4096

/* DrawCaption()/DrawCaptionTemp() flags */
#define DC_ACTIVE		0x0001
#define DC_SMALLCAP		0x0002
#define DC_ICON			0x0004
#define DC_TEXT			0x0008
#define DC_INBUTTON		0x0010

/* DrawEdge() flags */
#define BDR_RAISEDOUTER    0x0001
#define BDR_SUNKENOUTER    0x0002
#define BDR_RAISEDINNER    0x0004
#define BDR_SUNKENINNER    0x0008

#define BDR_OUTER          0x0003
#define BDR_INNER          0x000c
#define BDR_RAISED         0x0005
#define BDR_SUNKEN         0x000a

#define EDGE_RAISED        (BDR_RAISEDOUTER | BDR_RAISEDINNER)
#define EDGE_SUNKEN        (BDR_SUNKENOUTER | BDR_SUNKENINNER)
#define EDGE_ETCHED        (BDR_SUNKENOUTER | BDR_RAISEDINNER)
#define EDGE_BUMP          (BDR_RAISEDOUTER | BDR_SUNKENINNER)

/* border flags */
#define BF_LEFT            0x0001
#define BF_TOP             0x0002
#define BF_RIGHT           0x0004
#define BF_BOTTOM          0x0008
#define BF_DIAGONAL        0x0010
#define BF_MIDDLE          0x0800  /* Fill in the middle */
#define BF_SOFT            0x1000  /* For softer buttons */
#define BF_ADJUST          0x2000  /* Calculate the space left over */
#define BF_FLAT            0x4000  /* For flat rather than 3D borders */
#define BF_MONO            0x8000  /* For monochrome borders */
#define BF_TOPLEFT         (BF_TOP | BF_LEFT)
#define BF_TOPRIGHT        (BF_TOP | BF_RIGHT)
#define BF_BOTTOMLEFT      (BF_BOTTOM | BF_LEFT)
#define BF_BOTTOMRIGHT     (BF_BOTTOM | BF_RIGHT)
#define BF_RECT            (BF_LEFT | BF_TOP | BF_RIGHT | BF_BOTTOM)
#define BF_DIAGONAL_ENDTOPRIGHT     (BF_DIAGONAL | BF_TOP | BF_RIGHT)
#define BF_DIAGONAL_ENDTOPLEFT      (BF_DIAGONAL | BF_TOP | BF_LEFT)
#define BF_DIAGONAL_ENDBOTTOMLEFT   (BF_DIAGONAL | BF_BOTTOM | BF_LEFT)
#define BF_DIAGONAL_ENDBOTTOMRIGHT  (BF_DIAGONAL | BF_BOTTOM | BF_RIGHT)

/* DrawFrameControl() uType's */

#define DFC_CAPTION             1
#define DFC_MENU                2
#define DFC_SCROLL              3
#define DFC_BUTTON              4

/* uState's */

#define DFCS_CAPTIONCLOSE       0x0000
#define DFCS_CAPTIONMIN         0x0001
#define DFCS_CAPTIONMAX         0x0002
#define DFCS_CAPTIONRESTORE     0x0003
#define DFCS_CAPTIONHELP        0x0004		/* Windows 95 only */

#define DFCS_MENUARROW          0x0000
#define DFCS_MENUCHECK          0x0001
#define DFCS_MENUBULLET         0x0002
#define DFCS_MENUARROWRIGHT     0x0004

#define DFCS_SCROLLUP            0x0000
#define DFCS_SCROLLDOWN          0x0001
#define DFCS_SCROLLLEFT          0x0002
#define DFCS_SCROLLRIGHT         0x0003
#define DFCS_SCROLLCOMBOBOX      0x0005
#define DFCS_SCROLLSIZEGRIP      0x0008
#define DFCS_SCROLLSIZEGRIPRIGHT 0x0010

#define DFCS_BUTTONCHECK        0x0000
#define DFCS_BUTTONRADIOIMAGE   0x0001
#define DFCS_BUTTONRADIOMASK    0x0002		/* to draw nonsquare button */
#define DFCS_BUTTONRADIO        0x0004
#define DFCS_BUTTON3STATE       0x0008
#define DFCS_BUTTONPUSH         0x0010

/* additional state of the control */

#define DFCS_INACTIVE           0x0100
#define DFCS_PUSHED             0x0200
#define DFCS_CHECKED            0x0400
#define DFCS_ADJUSTRECT         0x2000		/* exclude surrounding edge */
#define DFCS_FLAT               0x4000
#define DFCS_MONO               0x8000

/* Image type */
#define	DST_COMPLEX	0x0000
#define	DST_TEXT	0x0001
#define	DST_PREFIXTEXT	0x0002
#define	DST_ICON	0x0003
#define	DST_BITMAP	0x0004

/* State type */
#define	DSS_NORMAL	0x0000
#define	DSS_UNION	0x0010  /* Gray string appearance */
#define	DSS_DISABLED	0x0020
#define	DSS_DEFAULT	0x0040  /* Make it bold */
#define	DSS_MONO	0x0080
#define	DSS_RIGHT	0x8000

typedef struct
{
    UINT32      CtlType;
    UINT32      CtlID;
    UINT32      itemID;
    UINT32      itemAction;
    UINT32      itemState;
    HWND32      hwndItem;
    HDC32       hDC;
    RECT32      rcItem WINE_PACKED;
    DWORD       itemData WINE_PACKED;
} DRAWITEMSTRUCT32, *PDRAWITEMSTRUCT32, *LPDRAWITEMSTRUCT32;

DECL_WINELIB_TYPE(DRAWITEMSTRUCT)
DECL_WINELIB_TYPE(PDRAWITEMSTRUCT)
DECL_WINELIB_TYPE(LPDRAWITEMSTRUCT)

typedef struct
{
    UINT32      CtlType;
    UINT32      CtlID;
    UINT32      itemID;
    UINT32      itemWidth;
    UINT32      itemHeight;
    DWORD       itemData;
} MEASUREITEMSTRUCT32, *PMEASUREITEMSTRUCT32, *LPMEASUREITEMSTRUCT32;

DECL_WINELIB_TYPE(MEASUREITEMSTRUCT)
DECL_WINELIB_TYPE(PMEASUREITEMSTRUCT)
DECL_WINELIB_TYPE(LPMEASUREITEMSTRUCT)

typedef struct
{
    UINT32     CtlType;
    UINT32     CtlID;
    UINT32     itemID;
    HWND32     hwndItem;
    DWORD      itemData;
} DELETEITEMSTRUCT32, *LPDELETEITEMSTRUCT32;

DECL_WINELIB_TYPE(DELETEITEMSTRUCT)
DECL_WINELIB_TYPE(LPDELETEITEMSTRUCT)

typedef struct
{
    UINT32      CtlType;
    UINT32      CtlID;
    HWND32      hwndItem;
    UINT32      itemID1;
    DWORD       itemData1;
    UINT32      itemID2;
    DWORD       itemData2;
    DWORD       dwLocaleId;
} COMPAREITEMSTRUCT32, *LPCOMPAREITEMSTRUCT32;

DECL_WINELIB_TYPE(COMPAREITEMSTRUCT)
DECL_WINELIB_TYPE(LPCOMPAREITEMSTRUCT)

/* WM_KEYUP/DOWN/CHAR HIWORD(lParam) flags */
#define KF_EXTENDED         0x0100
#define KF_DLGMODE          0x0800
#define KF_MENUMODE         0x1000
#define KF_ALTDOWN          0x2000
#define KF_REPEAT           0x4000
#define KF_UP               0x8000

/* Virtual key codes */
#define VK_LBUTTON          0x01
#define VK_RBUTTON          0x02
#define VK_CANCEL           0x03
#define VK_MBUTTON          0x04
/*                          0x05-0x07  Undefined */
#define VK_BACK             0x08
#define VK_TAB              0x09
/*                          0x0A-0x0B  Undefined */
#define VK_CLEAR            0x0C
#define VK_RETURN           0x0D
/*                          0x0E-0x0F  Undefined */
#define VK_SHIFT            0x10
#define VK_CONTROL          0x11
#define VK_MENU             0x12
#define VK_PAUSE            0x13
#define VK_CAPITAL          0x14
/*                          0x15-0x19  Reserved for Kanji systems */
/*                          0x1A       Undefined */
#define VK_ESCAPE           0x1B
/*                          0x1C-0x1F  Reserved for Kanji systems */
#define VK_SPACE            0x20
#define VK_PRIOR            0x21
#define VK_NEXT             0x22
#define VK_END              0x23
#define VK_HOME             0x24
#define VK_LEFT             0x25
#define VK_UP               0x26
#define VK_RIGHT            0x27
#define VK_DOWN             0x28
#define VK_SELECT           0x29
#define VK_PRINT            0x2A /* OEM specific in Windows 3.1 SDK */
#define VK_EXECUTE          0x2B
#define VK_SNAPSHOT         0x2C
#define VK_INSERT           0x2D
#define VK_DELETE           0x2E
#define VK_HELP             0x2F
#define VK_0                0x30
#define VK_1                0x31
#define VK_2                0x32
#define VK_3                0x33
#define VK_4                0x34
#define VK_5                0x35
#define VK_6                0x36
#define VK_7                0x37
#define VK_8                0x38
#define VK_9                0x39
/*                          0x3A-0x40  Undefined */
#define VK_A                0x41
#define VK_B                0x42
#define VK_C                0x43
#define VK_D                0x44
#define VK_E                0x45
#define VK_F                0x46
#define VK_G                0x47
#define VK_H                0x48
#define VK_I                0x49
#define VK_J                0x4A
#define VK_K                0x4B
#define VK_L                0x4C
#define VK_M                0x4D
#define VK_N                0x4E
#define VK_O                0x4F
#define VK_P                0x50
#define VK_Q                0x51
#define VK_R                0x52
#define VK_S                0x53
#define VK_T                0x54
#define VK_U                0x55
#define VK_V                0x56
#define VK_W                0x57
#define VK_X                0x58
#define VK_Y                0x59
#define VK_Z                0x5A

#define VK_LWIN             0x5B
#define VK_RWIN             0x5C
#define VK_APPS             0x5D
/*                          0x5E-0x5F Unassigned */
#define VK_NUMPAD0          0x60
#define VK_NUMPAD1          0x61
#define VK_NUMPAD2          0x62
#define VK_NUMPAD3          0x63
#define VK_NUMPAD4          0x64
#define VK_NUMPAD5          0x65
#define VK_NUMPAD6          0x66
#define VK_NUMPAD7          0x67
#define VK_NUMPAD8          0x68
#define VK_NUMPAD9          0x69
#define VK_MULTIPLY         0x6A
#define VK_ADD              0x6B
#define VK_SEPARATOR        0x6C
#define VK_SUBTRACT         0x6D
#define VK_DECIMAL          0x6E
#define VK_DIVIDE           0x6F
#define VK_F1               0x70
#define VK_F2               0x71
#define VK_F3               0x72
#define VK_F4               0x73
#define VK_F5               0x74
#define VK_F6               0x75
#define VK_F7               0x76
#define VK_F8               0x77
#define VK_F9               0x78
#define VK_F10              0x79
#define VK_F11              0x7A
#define VK_F12              0x7B
#define VK_F13              0x7C
#define VK_F14              0x7D
#define VK_F15              0x7E
#define VK_F16              0x7F
#define VK_F17              0x80
#define VK_F18              0x81
#define VK_F19              0x82
#define VK_F20              0x83
#define VK_F21              0x84
#define VK_F22              0x85
#define VK_F23              0x86
#define VK_F24              0x87
/*                          0x88-0x8F  Unassigned */
#define VK_NUMLOCK          0x90
#define VK_SCROLL           0x91
/*                          0x92-0x9F  Unassigned */
/*
 * differencing between right and left shift/control/alt key.
 * Used only by GetAsyncKeyState() and GetKeyState().
 */
#define VK_LSHIFT           0xA0
#define VK_RSHIFT           0xA1
#define VK_LCONTROL         0xA2
#define VK_RCONTROL         0xA3
#define VK_LMENU            0xA4
#define VK_RMENU            0xA5
/*                          0xA6-0xB9  Unassigned */
#define VK_OEM_1            0xBA
#define VK_OEM_PLUS         0xBB
#define VK_OEM_COMMA        0xBC
#define VK_OEM_MINUS        0xBD
#define VK_OEM_PERIOD       0xBE
#define VK_OEM_2            0xBF
#define VK_OEM_3            0xC0
/*                          0xC1-0xDA  Unassigned */
#define VK_OEM_4            0xDB
#define VK_OEM_5            0xDC
#define VK_OEM_6            0xDD
#define VK_OEM_7            0xDE
/*                          0xDF-0xE4  OEM specific */

#define VK_PROCESSKEY       0xE5

/*                          0xE6       OEM specific */
/*                          0xE7-0xE8  Unassigned */
/*                          0xE9-0xF5  OEM specific */

#define VK_ATTN             0xF6
#define VK_CRSEL            0xF7
#define VK_EXSEL            0xF8
#define VK_EREOF            0xF9
#define VK_PLAY             0xFA
#define VK_ZOOM             0xFB
#define VK_NONAME           0xFC
#define VK_PA1              0xFD
#define VK_OEM_CLEAR        0xFE
  
  /* Key status flags for mouse events */
#define MK_LBUTTON	    0x0001
#define MK_RBUTTON	    0x0002
#define MK_SHIFT	    0x0004
#define MK_CONTROL	    0x0008
#define MK_MBUTTON	    0x0010

  /* Queue status flags */
#define QS_KEY		0x0001
#define QS_MOUSEMOVE	0x0002
#define QS_MOUSEBUTTON	0x0004
#define QS_MOUSE	(QS_MOUSEMOVE | QS_MOUSEBUTTON)
#define QS_POSTMESSAGE	0x0008
#define QS_TIMER	0x0010
#define QS_PAINT	0x0020
#define QS_SENDMESSAGE	0x0040
#define QS_ALLINPUT     0x007f

#define DDL_READWRITE	0x0000
#define DDL_READONLY	0x0001
#define DDL_HIDDEN	0x0002
#define DDL_SYSTEM	0x0004
#define DDL_DIRECTORY	0x0010
#define DDL_ARCHIVE	0x0020

#define DDL_POSTMSGS	0x2000
#define DDL_DRIVES	0x4000
#define DDL_EXCLUSIVE	0x8000

  /* Shell hook values */
#define HSHELL_WINDOWCREATED       1
#define HSHELL_WINDOWDESTROYED     2
#define HSHELL_ACTIVATESHELLWINDOW 3

/* Predefined Clipboard Formats */
#define CF_TEXT              1
#define CF_BITMAP            2
#define CF_METAFILEPICT      3
#define CF_SYLK              4
#define CF_DIF               5
#define CF_TIFF              6
#define CF_OEMTEXT           7
#define CF_DIB               8
#define CF_PALETTE           9
#define CF_PENDATA          10
#define CF_RIFF             11
#define CF_WAVE             12
#define CF_ENHMETAFILE      14

#define CF_OWNERDISPLAY     0x0080
#define CF_DSPTEXT          0x0081
#define CF_DSPBITMAP        0x0082
#define CF_DSPMETAFILEPICT  0x0083

/* "Private" formats don't get GlobalFree()'d */
#define CF_PRIVATEFIRST     0x0200
#define CF_PRIVATELAST      0x02FF

/* "GDIOBJ" formats do get DeleteObject()'d */
#define CF_GDIOBJFIRST      0x0300
#define CF_GDIOBJLAST       0x03FF

/* Clipboard command messages */
#define WM_CUT              0x0300
#define WM_COPY             0x0301
#define WM_PASTE            0x0302
#define WM_CLEAR            0x0303
#define WM_UNDO             0x0304

/* Clipboard owner messages */
#define WM_RENDERFORMAT     0x0305
#define WM_RENDERALLFORMATS 0x0306
#define WM_DESTROYCLIPBOARD 0x0307

/* Clipboard viewer messages */
#define WM_DRAWCLIPBOARD    0x0308
#define WM_PAINTCLIPBOARD   0x0309
#define WM_SIZECLIPBOARD    0x030B
#define WM_VSCROLLCLIPBOARD 0x030A
#define WM_HSCROLLCLIPBOARD 0x030E
#define WM_ASKCBFORMATNAME  0x030C
#define WM_CHANGECBCHAIN    0x030D



/* DragObject stuff */

typedef struct
{
    HWND16     hWnd;
    HANDLE16   hScope;
    WORD       wFlags;
    HANDLE16   hList;
    HANDLE16   hOfStruct;
    POINT16 pt WINE_PACKED;
    LONG       l WINE_PACKED;
} DRAGINFO, *LPDRAGINFO;

#define DRAGOBJ_PROGRAM		0x0001
#define DRAGOBJ_DATA		0x0002
#define DRAGOBJ_DIRECTORY	0x0004
#define DRAGOBJ_MULTIPLE	0x0008
#define DRAGOBJ_EXTERNAL	0x8000

#define DRAG_PRINT		0x544E5250
#define DRAG_FILE		0x454C4946

/* types of LoadImage */
#define IMAGE_BITMAP	0
#define IMAGE_ICON	1
#define IMAGE_CURSOR	2
#define IMAGE_ENHMETA	3

/* loadflags to LoadImage */
#define LR_DEFAULTCOLOR		0x0000
#define LR_MONOCHROME		0x0001
#define LR_COLOR		0x0002
#define LR_COPYRETURNORG	0x0004
#define LR_COPYDELETEORG	0x0008
#define LR_LOADFROMFILE		0x0010
#define LR_LOADTRANSPARENT	0x0020
#define LR_DEFAULTSIZE		0x0040
#define LR_VGA_COLOR		0x0080
#define LR_LOADMAP3DCOLORS	0x1000
#define	LR_CREATEDIBSECTION	0x2000
#define LR_COPYFROMRESOURCE	0x4000
#define LR_SHARED		0x8000

/* Flags for DrawIconEx.  */
#define DI_MASK                 1
#define DI_IMAGE                2
#define DI_NORMAL               (DI_MASK | DI_IMAGE)
#define DI_COMPAT               4
#define DI_DEFAULTSIZE          8

  /* misc messages */
#define WM_NULL             0x0000
#define WM_CPL_LAUNCH       (WM_USER + 1000)
#define WM_CPL_LAUNCHED     (WM_USER + 1001)

/* WM_NOTIFYFORMAT commands and return values */
#define NFR_ANSI	    1
#define NFR_UNICODE	    2
#define NF_QUERY	    3
#define NF_REQUERY	    4

#pragma pack(4)
#define     EnumTaskWindows32(handle,proc,lparam) \
            EnumThreadWindows(handle,proc,lparam)
#define     EnumTaskWindows WINELIB_NAME(EnumTaskWindows)
#define     OemToAnsi32A OemToChar32A
#define     OemToAnsi32W OemToChar32W
#define     OemToAnsi WINELIB_NAME_AW(OemToAnsi)
#define     OemToAnsiBuff32A OemToCharBuff32A
#define     OemToAnsiBuff32W OemToCharBuff32W
#define     OemToAnsiBuff WINELIB_NAME_AW(OemToAnsiBuff)
#define     AnsiToOem32A CharToOem32A
#define     AnsiToOem32W CharToOem32W
#define     AnsiToOem WINELIB_NAME_AW(AnsiToOem)
#define     AnsiToOemBuff32A CharToOemBuff32A
#define     AnsiToOemBuff32W CharToOemBuff32W
#define     AnsiToOemBuff WINELIB_NAME_AW(AnsiToOemBuff)
WORD        WINAPI CascadeWindows (HWND32, UINT32, const LPRECT32,
                                   UINT32, const HWND32 *);
INT32       WINAPI CopyAcceleratorTable32A(HACCEL32,LPACCEL32,INT32);
INT32       WINAPI CopyAcceleratorTable32W(HACCEL32,LPACCEL32,INT32);
#define     CopyAcceleratorTable WINELIB_NAME_AW(CopyAcceleratorTable)
HICON32     WINAPI CreateIconIndirect(LPICONINFO32);
BOOL32      WINAPI DestroyAcceleratorTable(HACCEL32);
BOOL32      WINAPI EnumDisplayMonitors(HDC32,LPRECT32,MONITORENUMPROC,LPARAM);
INT32       WINAPI EnumPropsEx32A(HWND32,PROPENUMPROCEX32A,LPARAM);
INT32       WINAPI EnumPropsEx32W(HWND32,PROPENUMPROCEX32W,LPARAM);
#define     EnumPropsEx WINELIB_NAME_AW(EnumPropsEx)
BOOL32      WINAPI EnumThreadWindows(DWORD,WNDENUMPROC32,LPARAM);
BOOL32      WINAPI ExitWindowsEx(UINT32,DWORD);
BOOL32      WINAPI GetIconInfo32(HICON32,LPICONINFO32);
#define     GetIconInfo WINELIB_NAME(GetIconInfo)
DWORD       WINAPI GetMenuContextHelpId32(HMENU32);
#define     GetMenuContextHelpId WINELIB_NAME(GetMenuContextHelpId)
UINT32      WINAPI GetMenuDefaultItem32(HMENU32,UINT32,UINT32);
#define     GetMenuDefaultItem WINELIB_NAME(GetMenuDefaultItem)
BOOL32      WINAPI GetMenuItemInfo32A(HMENU32,UINT32,BOOL32,MENUITEMINFO32A*);
BOOL32      WINAPI GetMenuItemInfo32W(HMENU32,UINT32,BOOL32,MENUITEMINFO32W*);
#define     GetMenuItemInfo WINELIB_NAME_AW(GetMenuItemInfo)
BOOL32      WINAPI GetMonitorInfo32A(HMONITOR,LPMONITORINFO);
BOOL32      WINAPI GetMonitorInfo32W(HMONITOR,LPMONITORINFO);
#define     GetMonitorInfo WINELIB_NAME_AW(GetMonitorInfo)
INT32       WINAPI GetNumberFormat32A(LCID,DWORD,LPCSTR,const NUMBERFMT32A*,LPSTR,int);
INT32       WINAPI GetNumberFormat32W(LCID,DWORD,LPCWSTR,const NUMBERFMT32W*,LPWSTR,int);
#define     GetNumberFormat WINELIB_NAME_AW(GetNumberFormat);
DWORD       WINAPI GetWindowContextHelpId(HWND32);
DWORD       WINAPI GetWindowThreadProcessId(HWND32,LPDWORD);
BOOL32      WINAPI IsWindowUnicode(HWND32);
HKL32       WINAPI LoadKeyboardLayout32A(LPCSTR,UINT32);
HKL32       WINAPI LoadKeyboardLayout32W(LPCWSTR,UINT32);
#define     LoadKeyboardLayout WINELIB_NAME_AW(LoadKeyboardLayout)
INT32       WINAPI MessageBoxEx32A(HWND32,LPCSTR,LPCSTR,UINT32,WORD);
INT32       WINAPI MessageBoxEx32W(HWND32,LPCWSTR,LPCWSTR,UINT32,WORD);
#define     MessageBoxEx WINELIB_NAME_AW(MessageBoxEx)
HMONITOR    WINAPI MonitorFromPoint(POINT32,DWORD);
HMONITOR    WINAPI MonitorFromRect(LPRECT32,DWORD);
HMONITOR    WINAPI MonitorFromWindow(HWND32,DWORD);
DWORD       WINAPI MsgWaitForMultipleObjects(DWORD,HANDLE32*,BOOL32,DWORD,DWORD);
BOOL32      WINAPI PaintDesktop(HDC32);
BOOL32      WINAPI PostThreadMessage32A(DWORD, UINT32, WPARAM32, LPARAM);
BOOL32      WINAPI PostThreadMessage32W(DWORD, UINT32, WPARAM32, LPARAM);
#define     PostThreadMessage WINELIB_NAME_AW(PostThreadMessage)
UINT32      WINAPI ReuseDDElParam(UINT32,UINT32,UINT32,UINT32,UINT32);
BOOL32      WINAPI SendNotifyMessage32A(HWND32,UINT32,WPARAM32,LPARAM);
BOOL32      WINAPI SendNotifyMessage32W(HWND32,UINT32,WPARAM32,LPARAM);
#define     SendNotifyMessage WINELIB_NAME_AW(SendNotifyMessage)
VOID        WINAPI SetDebugErrorLevel(DWORD);
VOID        WINAPI SetLastErrorEx(DWORD,DWORD);
BOOL32      WINAPI SetMenuDefaultItem32(HMENU32,UINT32,UINT32);
#define     SetMenuDefaultItem WINELIB_NAME(SetMenuDefaultItem)
BOOL32      WINAPI SetMenuItemInfo32A(HMENU32,UINT32,BOOL32,const MENUITEMINFO32A*);
BOOL32      WINAPI SetMenuItemInfo32W(HMENU32,UINT32,BOOL32,const MENUITEMINFO32W*);
#define     SetMenuItemInfo WINELIB_NAME_AW(SetMenuItemInfo)
BOOL32      WINAPI SetWindowContextHelpId(HWND32,DWORD);
WORD        WINAPI TileWindows (HWND32, UINT32, const LPRECT32,
                                UINT32, const HWND32 *);
BOOL32      WINAPI TrackPopupMenuEx(HMENU32,UINT32,INT32,INT32,HWND32,
                                    LPTPMPARAMS);
UINT32      WINAPI UnpackDDElParam(UINT32,UINT32,UINT32*,UINT32*);
DWORD       WINAPI WaitForInputIdle(HANDLE32,DWORD);
VOID        WINAPI keybd_event(BYTE,BYTE,DWORD,DWORD);
VOID        WINAPI mouse_event(DWORD,DWORD,DWORD,DWORD,DWORD);

/* Declarations for functions that are the same in Win16 and Win32 */
VOID        WINAPI EndMenu(void);
DWORD       WINAPI GetDialogBaseUnits(void);
VOID        WINAPI GetKeyboardState(LPBYTE);
DWORD       WINAPI GetMenuCheckMarkDimensions(void);
LONG        WINAPI GetMessageExtraInfo(void);
DWORD       WINAPI GetMessagePos(void);
LONG        WINAPI GetMessageTime(void);
DWORD       WINAPI GetTickCount(void);
ATOM        WINAPI GlobalDeleteAtom(ATOM);
DWORD       WINAPI OemKeyScan(WORD);
VOID        WINAPI ReleaseCapture(void);
VOID        WINAPI SetKeyboardState(LPBYTE);
VOID        WINAPI WaitMessage(VOID);


/* Declarations for functions that change between Win16 and Win32 */

BOOL32      WINAPI AdjustWindowRect32(LPRECT32,DWORD,BOOL32);
#define     AdjustWindowRect WINELIB_NAME(AdjustWindowRect)
BOOL32      WINAPI AdjustWindowRectEx32(LPRECT32,DWORD,BOOL32,DWORD);
#define     AdjustWindowRectEx WINELIB_NAME(AdjustWindowRectEx)
#define     AnsiLower32A CharLower32A
#define     AnsiLower32W CharLower32W
#define     AnsiLower WINELIB_NAME_AW(AnsiLower)
#define     AnsiLowerBuff32A CharLowerBuff32A
#define     AnsiLowerBuff32W CharLowerBuff32W
#define     AnsiLowerBuff WINELIB_NAME_AW(AnsiLowerBuff)
#define     AnsiNext32A CharNext32A
#define     AnsiNext32W CharNext32W
#define     AnsiNext WINELIB_NAME_AW(AnsiNext)
#define     AnsiPrev32A CharPrev32A
#define     AnsiPrev32W CharPrev32W
#define     AnsiPrev WINELIB_NAME_AW(AnsiPrev)
#define     AnsiUpper32A CharUpper32A
#define     AnsiUpper32W CharUpper32W
#define     AnsiUpper WINELIB_NAME_AW(AnsiUpper)
#define     AnsiUpperBuff32A CharUpperBuff32A
#define     AnsiUpperBuff32W CharUpperBuff32W
#define     AnsiUpperBuff WINELIB_NAME_AW(AnsiUpperBuff)
BOOL32      WINAPI AnyPopup32(void);
#define     AnyPopup WINELIB_NAME(AnyPopup)
BOOL32      WINAPI AppendMenu32A(HMENU32,UINT32,UINT32,LPCSTR);
BOOL32      WINAPI AppendMenu32W(HMENU32,UINT32,UINT32,LPCWSTR);
#define     AppendMenu WINELIB_NAME_AW(AppendMenu)
UINT32      WINAPI ArrangeIconicWindows32(HWND32);
#define     ArrangeIconicWindows WINELIB_NAME(ArrangeIconicWindows)
HDWP32      WINAPI BeginDeferWindowPos32(INT32);
#define     BeginDeferWindowPos WINELIB_NAME(BeginDeferWindowPos)
HDC32       WINAPI BeginPaint32(HWND32,LPPAINTSTRUCT32);
#define     BeginPaint WINELIB_NAME(BeginPaint)
BOOL32      WINAPI BringWindowToTop32(HWND32);
#define     BringWindowToTop WINELIB_NAME(BringWindowToTop)
BOOL32      WINAPI CallMsgFilter32A(LPMSG32,INT32);
BOOL32      WINAPI CallMsgFilter32W(LPMSG32,INT32);
#define     CallMsgFilter WINELIB_NAME_AW(CallMsgFilter)
LRESULT     WINAPI CallNextHookEx32(HHOOK,INT32,WPARAM32,LPARAM);
#define     CallNextHookEx WINELIB_NAME(CallNextHookEx)
LRESULT     WINAPI CallWindowProc32A(WNDPROC32,HWND32,UINT32,WPARAM32,LPARAM);
LRESULT     WINAPI CallWindowProc32W(WNDPROC32,HWND32,UINT32,WPARAM32,LPARAM);
#define     CallWindowProc WINELIB_NAME_AW(CallWindowProc)
BOOL32      WINAPI ChangeClipboardChain32(HWND32,HWND32);
#define     ChangeClipboardChain WINELIB_NAME(ChangeClipboardChain)
BOOL32      WINAPI ChangeMenu32A(HMENU32,UINT32,LPCSTR,UINT32,UINT32);
BOOL32      WINAPI ChangeMenu32W(HMENU32,UINT32,LPCWSTR,UINT32,UINT32);
#define     ChangeMenu WINELIB_NAME_AW(ChangeMenu)
LPSTR       WINAPI CharLower32A(LPSTR);
LPWSTR      WINAPI CharLower32W(LPWSTR);
#define     CharLower WINELIB_NAME_AW(CharLower)
DWORD       WINAPI CharLowerBuff32A(LPSTR,DWORD);
DWORD       WINAPI CharLowerBuff32W(LPWSTR,DWORD);
#define     CharLowerBuff WINELIB_NAME_AW(CharLowerBuff)
LPSTR       WINAPI CharNext32A(LPCSTR);
LPWSTR      WINAPI CharNext32W(LPCWSTR);
#define     CharNext WINELIB_NAME_AW(CharNext)
LPSTR       WINAPI CharNextEx32A(WORD,LPCSTR,DWORD);
LPWSTR      WINAPI CharNextEx32W(WORD,LPCWSTR,DWORD);
#define     CharNextEx WINELIB_NAME_AW(CharNextEx)
LPSTR       WINAPI CharPrev32A(LPCSTR,LPCSTR);
LPWSTR      WINAPI CharPrev32W(LPCWSTR,LPCWSTR);
#define     CharPrev WINELIB_NAME_AW(CharPrev)
LPSTR       WINAPI CharPrevEx32A(WORD,LPCSTR,LPCSTR,DWORD);
LPWSTR      WINAPI CharPrevEx32W(WORD,LPCWSTR,LPCWSTR,DWORD);
#define     CharPrevEx WINELIB_NAME_AW(CharPrevEx)
LPSTR       WINAPI CharUpper32A(LPSTR);
LPWSTR      WINAPI CharUpper32W(LPWSTR);
#define     CharUpper WINELIB_NAME_AW(CharUpper)
DWORD       WINAPI CharUpperBuff32A(LPSTR,DWORD);
DWORD       WINAPI CharUpperBuff32W(LPWSTR,DWORD);
#define     CharUpperBuff WINELIB_NAME_AW(CharUpperBuff)
BOOL32      WINAPI CharToOem32A(LPCSTR,LPSTR);
BOOL32      WINAPI CharToOem32W(LPCWSTR,LPSTR);
#define     CharToOem WINELIB_NAME_AW(CharToOem)
BOOL32      WINAPI CharToOemBuff32A(LPCSTR,LPSTR,DWORD);
BOOL32      WINAPI CharToOemBuff32W(LPCWSTR,LPSTR,DWORD);
#define     CharToOemBuff WINELIB_NAME_AW(CharToOemBuff)
BOOL32      WINAPI CheckDlgButton32(HWND32,INT32,UINT32);
#define     CheckDlgButton WINELIB_NAME(CheckDlgButton)
DWORD       WINAPI CheckMenuItem32(HMENU32,UINT32,UINT32);
#define     CheckMenuItem WINELIB_NAME(CheckMenuItem)
BOOL32      WINAPI CheckMenuRadioItem32(HMENU32,UINT32,UINT32,UINT32,UINT32);
#define     CheckMenuRadioItem WINELIB_NAME(CheckMenuRadioItem)
BOOL32      WINAPI CheckRadioButton32(HWND32,UINT32,UINT32,UINT32);
#define     CheckRadioButton WINELIB_NAME(CheckRadioButton)
HWND32      WINAPI ChildWindowFromPoint32(HWND32,POINT32);
#define     ChildWindowFromPoint WINELIB_NAME(ChildWindowFromPoint)
HWND32      WINAPI ChildWindowFromPointEx32(HWND32,POINT32,UINT32);
#define     ChildWindowFromPointEx WINELIB_NAME(ChildWindowFromPointEx)
BOOL32      WINAPI ClearCommBreak32(INT32);
#define     ClearCommBreak WINELIB_NAME(ClearCommBreak)
BOOL32      WINAPI ClientToScreen32(HWND32,LPPOINT32);
#define     ClientToScreen WINELIB_NAME(ClientToScreen)
BOOL32      WINAPI ClipCursor32(const RECT32*);
#define     ClipCursor WINELIB_NAME(ClipCursor)
BOOL32      WINAPI CloseClipboard32(void);
#define     CloseClipboard WINELIB_NAME(CloseClipboard)
BOOL32      WINAPI CloseWindow32(HWND32);
#define     CloseWindow WINELIB_NAME(CloseWindow)
#define     CopyCursor32(cur) ((HCURSOR32)CopyIcon32((HICON32)(cur)))
#define     CopyCursor WINELIB_NAME(CopyCursor)
HICON32     WINAPI CopyIcon32(HICON32);
#define     CopyIcon WINELIB_NAME(CopyIcon)
HICON32     WINAPI CopyImage32(HANDLE32,UINT32,INT32,INT32,UINT32);
#define     CopyImage WINELIB_NAME(CopyImage)
BOOL32      WINAPI CopyRect32(RECT32*,const RECT32*);
#define     CopyRect WINELIB_NAME(CopyRect)
INT32       WINAPI CountClipboardFormats32(void);
#define     CountClipboardFormats WINELIB_NAME(CountClipboardFormats)
BOOL32      WINAPI CreateCaret32(HWND32,HBITMAP32,INT32,INT32);
#define     CreateCaret WINELIB_NAME(CreateCaret)
HCURSOR32   WINAPI CreateCursor32(HINSTANCE32,INT32,INT32,INT32,INT32,LPCVOID,LPCVOID);
#define     CreateCursor WINELIB_NAME(CreateCursor)
#define     CreateDialog32A(inst,ptr,hwnd,dlg) \
           CreateDialogParam32A(inst,ptr,hwnd,dlg,0)
#define     CreateDialog32W(inst,ptr,hwnd,dlg) \
           CreateDialogParam32W(inst,ptr,hwnd,dlg,0)
#define     CreateDialog WINELIB_NAME_AW(CreateDialog)
#define     CreateDialogIndirect32A(inst,ptr,hwnd,dlg) \
           CreateDialogIndirectParam32A(inst,ptr,hwnd,dlg,0)
#define     CreateDialogIndirect32W(inst,ptr,hwnd,dlg) \
           CreateDialogIndirectParam32W(inst,ptr,hwnd,dlg,0)
#define     CreateDialogIndirect WINELIB_NAME_AW(CreateDialogIndirect)
HWND32      WINAPI CreateDialogIndirectParam32A(HINSTANCE32,LPCVOID,HWND32,
                                                DLGPROC32,LPARAM);
HWND32      WINAPI CreateDialogIndirectParam32W(HINSTANCE32,LPCVOID,HWND32,
                                                DLGPROC32,LPARAM);
#define     CreateDialogIndirectParam WINELIB_NAME_AW(CreateDialogIndirectParam)
HWND32      WINAPI CreateDialogParam32A(HINSTANCE32,LPCSTR,HWND32,DLGPROC32,LPARAM);
HWND32      WINAPI CreateDialogParam32W(HINSTANCE32,LPCWSTR,HWND32,DLGPROC32,LPARAM);
#define     CreateDialogParam WINELIB_NAME_AW(CreateDialogParam)
HICON32     WINAPI CreateIcon32(HINSTANCE32,INT32,INT32,BYTE,BYTE,LPCVOID,LPCVOID);
#define     CreateIcon WINELIB_NAME(CreateIcon)
HICON32     WINAPI CreateIconFromResource32(LPBYTE,UINT32,BOOL32,DWORD);
#define     CreateIconFromResource WINELIB_NAME(CreateIconFromResource)
HICON32     WINAPI CreateIconFromResourceEx32(LPBYTE,UINT32,BOOL32,DWORD,INT32,INT32,UINT32);
#define     CreateIconFromResourceEx WINELIB_NAME(CreateIconFromResourceEx)
HMENU32     WINAPI CreateMenu32(void);
#define     CreateMenu WINELIB_NAME(CreateMenu)
HMENU32     WINAPI CreatePopupMenu32(void);
#define     CreatePopupMenu WINELIB_NAME(CreatePopupMenu)
#define     CreateWindow32A(className,titleName,style,x,y,width,height,\
                            parent,menu,instance,param) \
            CreateWindowEx32A(0,className,titleName,style,x,y,width,height,\
                              parent,menu,instance,param)
#define     CreateWindow32W(className,titleName,style,x,y,width,height,\
                            parent,menu,instance,param) \
            CreateWindowEx32W(0,className,titleName,style,x,y,width,height,\
                              parent,menu,instance,param)
#define     CreateWindow WINELIB_NAME_AW(CreateWindow)
HWND32      WINAPI CreateWindowEx32A(DWORD,LPCSTR,LPCSTR,DWORD,INT32,INT32,
                                INT32,INT32,HWND32,HMENU32,HINSTANCE32,LPVOID);
HWND32      WINAPI CreateWindowEx32W(DWORD,LPCWSTR,LPCWSTR,DWORD,INT32,INT32,
                                INT32,INT32,HWND32,HMENU32,HINSTANCE32,LPVOID);
#define     CreateWindowEx WINELIB_NAME_AW(CreateWindowEx)
HWND32      WINAPI CreateMDIWindow32A(LPCSTR,LPCSTR,DWORD,INT32,INT32,
                                INT32,INT32,HWND32,HINSTANCE32,LPARAM);
HWND32      WINAPI CreateMDIWindow32W(LPCWSTR,LPCWSTR,DWORD,INT32,INT32,
                                INT32,INT32,HWND32,HINSTANCE32,LPARAM);
#define     CreateMDIWindow WINELIB_NAME_AW(CreateMDIWindow)
LRESULT     WINAPI DefDlgProc32A(HWND32,UINT32,WPARAM32,LPARAM);
LRESULT     WINAPI DefDlgProc32W(HWND32,UINT32,WPARAM32,LPARAM);
#define     DefDlgProc WINELIB_NAME_AW(DefDlgProc)
HDWP32      WINAPI DeferWindowPos32(HDWP32,HWND32,HWND32,INT32,INT32,INT32,INT32,UINT32);
#define     DeferWindowPos WINELIB_NAME(DeferWindowPos)
LRESULT     WINAPI DefFrameProc32A(HWND32,HWND32,UINT32,WPARAM32,LPARAM);
LRESULT     WINAPI DefFrameProc32W(HWND32,HWND32,UINT32,WPARAM32,LPARAM);
#define     DefFrameProc WINELIB_NAME_AW(DefFrameProc)
#define     DefHookProc32(code,wparam,lparam,phhook) \
            CallNextHookEx32(*(phhook),code,wparam,lparam)
#define     DefHookProc WINELIB_NAME(DefHookProc)
LRESULT     WINAPI DefMDIChildProc32A(HWND32,UINT32,WPARAM32,LPARAM);
LRESULT     WINAPI DefMDIChildProc32W(HWND32,UINT32,WPARAM32,LPARAM);
#define     DefMDIChildProc WINELIB_NAME_AW(DefMDIChildProc)
LRESULT     WINAPI DefWindowProc32A(HWND32,UINT32,WPARAM32,LPARAM);
LRESULT     WINAPI DefWindowProc32W(HWND32,UINT32,WPARAM32,LPARAM);
#define     DefWindowProc WINELIB_NAME_AW(DefWindowProc)
BOOL32      WINAPI DeleteMenu32(HMENU32,UINT32,UINT32);
#define     DeleteMenu WINELIB_NAME(DeleteMenu)
BOOL32      WINAPI DestroyCaret32(void);
#define     DestroyCaret WINELIB_NAME(DestroyCaret)
BOOL32      WINAPI DestroyCursor32(HCURSOR32);
#define     DestroyCursor WINELIB_NAME(DestroyCursor)
BOOL32      WINAPI DestroyIcon32(HICON32);
#define     DestroyIcon WINELIB_NAME(DestroyIcon)
BOOL32      WINAPI DestroyMenu32(HMENU32);
#define     DestroyMenu WINELIB_NAME(DestroyMenu)
BOOL32      WINAPI DestroyWindow32(HWND32);
#define     DestroyWindow WINELIB_NAME(DestroyWindow)
#define     DialogBox32A(inst,template,owner,func) \
            DialogBoxParam32A(inst,template,owner,func,0)
#define     DialogBox32W(inst,template,owner,func) \
            DialogBoxParam32W(inst,template,owner,func,0)
#define     DialogBox WINELIB_NAME_AW(DialogBox)
#define     DialogBoxIndirect32A(inst,template,owner,func) \
            DialogBoxIndirectParam32A(inst,template,owner,func,0)
#define     DialogBoxIndirect32W(inst,template,owner,func) \
            DialogBoxIndirectParam32W(inst,template,owner,func,0)
#define     DialogBoxIndirect WINELIB_NAME_AW(DialogBoxIndirect)
INT32       WINAPI DialogBoxIndirectParam32A(HINSTANCE32,LPCVOID,HWND32,DLGPROC32,LPARAM);
INT32       WINAPI DialogBoxIndirectParam32W(HINSTANCE32,LPCVOID,HWND32,DLGPROC32,LPARAM);
#define     DialogBoxIndirectParam WINELIB_NAME_AW(DialogBoxIndirectParam)
INT32       WINAPI DialogBoxParam32A(HINSTANCE32,LPCSTR,HWND32,DLGPROC32,LPARAM);
INT32       WINAPI DialogBoxParam32W(HINSTANCE32,LPCWSTR,HWND32,DLGPROC32,LPARAM);
#define     DialogBoxParam WINELIB_NAME_AW(DialogBoxParam)
LONG        WINAPI DispatchMessage32A(const MSG32*);
LONG        WINAPI DispatchMessage32W(const MSG32*);
#define     DispatchMessage WINELIB_NAME_AW(DispatchMessage)
INT32       WINAPI DlgDirList32A(HWND32,LPSTR,INT32,INT32,UINT32);
INT32       WINAPI DlgDirList32W(HWND32,LPWSTR,INT32,INT32,UINT32);
#define     DlgDirList WINELIB_NAME_AW(DlgDirList)
INT32       WINAPI DlgDirListComboBox32A(HWND32,LPSTR,INT32,INT32,UINT32);
INT32       WINAPI DlgDirListComboBox32W(HWND32,LPWSTR,INT32,INT32,UINT32);
#define     DlgDirListComboBox WINELIB_NAME_AW(DlgDirListComboBox)
BOOL32      WINAPI DlgDirSelectComboBoxEx32A(HWND32,LPSTR,INT32,INT32);
BOOL32      WINAPI DlgDirSelectComboBoxEx32W(HWND32,LPWSTR,INT32,INT32);
#define     DlgDirSelectComboBoxEx WINELIB_NAME_AW(DlgDirSelectComboBoxEx)
BOOL32      WINAPI DlgDirSelectEx32A(HWND32,LPSTR,INT32,INT32);
BOOL32      WINAPI DlgDirSelectEx32W(HWND32,LPWSTR,INT32,INT32);
#define     DlgDirSelectEx WINELIB_NAME_AW(DlgDirSelectEx)
BOOL32      WINAPI DragDetect32(HWND32,POINT32);
#define     DragDetect WINELIB_NAME(DragDetect)
DWORD       WINAPI DragObject32(HWND32,HWND32,UINT32,DWORD,HCURSOR32);
#define     DragObject WINELIB_NAME(DragObject)
BOOL32      WINAPI DrawAnimatedRects32(HWND32,int,const RECT32*,const RECT32*);
#define     DrawAnimatedRects WINELIB_NAME(DrawAnimatedRects)
BOOL32      WINAPI DrawCaption32(HWND32,HDC32,const RECT32*,UINT32);
#define     DrawCaption WINELIB_NAME(DrawCaption)
BOOL32      WINAPI DrawCaptionTemp32A(HWND32,HDC32,const RECT32*,HFONT32,HICON32,LPCSTR,UINT32);
BOOL32      WINAPI DrawCaptionTemp32W(HWND32,HDC32,const RECT32*,HFONT32,HICON32,LPCWSTR,UINT32);
#define     DrawCaptionTemp WINELIB_NAME_AW(DrawCaptionTemp)
BOOL32      WINAPI DrawEdge32(HDC32,LPRECT32,UINT32,UINT32);
#define     DrawEdge WINELIB_NAME(DrawEdge)
void        WINAPI DrawFocusRect32(HDC32,const RECT32*);
#define     DrawFocusRect WINELIB_NAME(DrawFocusRect)
BOOL32      WINAPI DrawFrameControl32(HDC32,LPRECT32,UINT32,UINT32);
#define     DrawFrameControl WINELIB_NAME(DrawFrameControl)
BOOL32      WINAPI DrawIcon32(HDC32,INT32,INT32,HICON32);
#define     DrawIcon WINELIB_NAME(DrawIcon)
BOOL32      WINAPI DrawIconEx32(HDC32,INT32,INT32,HICON32,INT32,INT32,
				UINT32,HBRUSH32,UINT32);
#define     DrawIconEx WINELIB_NAME(DrawIconEx)
BOOL32      WINAPI DrawMenuBar32(HWND32);
#define     DrawMenuBar WINELIB_NAME(DrawMenuBar)
BOOL32      WINAPI DrawState32A(HDC32,HBRUSH32,DRAWSTATEPROC32,LPARAM,WPARAM32,INT32,INT32,INT32,INT32,UINT32);
BOOL32      WINAPI DrawState32W(HDC32,HBRUSH32,DRAWSTATEPROC32,LPARAM,WPARAM32,INT32,INT32,INT32,INT32,UINT32);
#define     DrawState WINELIB_NAME_AW(DrawState)
INT32       WINAPI DrawText32A(HDC32,LPCSTR,INT32,LPRECT32,UINT32);
INT32       WINAPI DrawText32W(HDC32,LPCWSTR,INT32,LPRECT32,UINT32);
#define     DrawText WINELIB_NAME_AW(DrawText)
BOOL32      WINAPI EmptyClipboard32(void);
#define     EmptyClipboard WINELIB_NAME(EmptyClipboard)
UINT32      WINAPI EnableMenuItem32(HMENU32,UINT32,UINT32);
#define     EnableMenuItem WINELIB_NAME(EnableMenuItem)
BOOL32      WINAPI EnableScrollBar32(HWND32,INT32,UINT32);
#define     EnableScrollBar WINELIB_NAME(EnableScrollBar)
BOOL32      WINAPI EnableWindow32(HWND32,BOOL32);
#define     EnableWindow WINELIB_NAME(EnableWindow)
BOOL32      WINAPI EndDeferWindowPos32(HDWP32);
#define     EndDeferWindowPos WINELIB_NAME(EndDeferWindowPos)
BOOL32      WINAPI EndDialog32(HWND32,INT32);
#define     EndDialog WINELIB_NAME(EndDialog)
BOOL32      WINAPI EndPaint32(HWND32,const PAINTSTRUCT32*);
#define     EndPaint WINELIB_NAME(EndPaint)
BOOL32      WINAPI EnumChildWindows32(HWND32,WNDENUMPROC32,LPARAM);
#define     EnumChildWindows WINELIB_NAME(EnumChildWindows)
UINT32      WINAPI EnumClipboardFormats32(UINT32);
#define     EnumClipboardFormats WINELIB_NAME(EnumClipboardFormats)
INT32       WINAPI EnumProps32A(HWND32,PROPENUMPROC32A);
INT32       WINAPI EnumProps32W(HWND32,PROPENUMPROC32W);
#define     EnumProps WINELIB_NAME_AW(EnumProps)
BOOL32      WINAPI EnumWindows32(WNDENUMPROC32,LPARAM);
#define     EnumWindows WINELIB_NAME(EnumWindows)
BOOL32      WINAPI EqualRect32(const RECT32*,const RECT32*);
#define     EqualRect WINELIB_NAME(EqualRect)
BOOL32      WINAPI EscapeCommFunction32(INT32,UINT32);
#define     EscapeCommFunction WINELIB_NAME(EscapeCommFunction)
INT32       WINAPI ExcludeUpdateRgn32(HDC32,HWND32);
#define     ExcludeUpdateRgn WINELIB_NAME(ExcludeUpdateRgn)
#define     ExitWindows32(a,b) ExitWindowsEx(EWX_LOGOFF,0xffffffff)
#define     ExitWindows WINELIB_NAME(ExitWindows)
INT32       WINAPI FillRect32(HDC32,const RECT32*,HBRUSH32);
#define     FillRect WINELIB_NAME(FillRect)
HWND32      WINAPI FindWindow32A(LPCSTR,LPCSTR);
HWND32      WINAPI FindWindow32W(LPCWSTR,LPCWSTR);
#define     FindWindow WINELIB_NAME_AW(FindWindow)
HWND32      WINAPI FindWindowEx32A(HWND32,HWND32,LPCSTR,LPCSTR);
HWND32      WINAPI FindWindowEx32W(HWND32,HWND32,LPCWSTR,LPCWSTR);
#define     FindWindowEx WINELIB_NAME_AW(FindWindowEx)
BOOL32      WINAPI FlashWindow32(HWND32,BOOL32);
#define     FlashWindow WINELIB_NAME(FlashWindow)
INT32       WINAPI FrameRect32(HDC32,const RECT32*,HBRUSH32);
#define     FrameRect WINELIB_NAME(FrameRect)
HWND32      WINAPI GetActiveWindow32(void);
#define     GetActiveWindow WINELIB_NAME(GetActiveWindow)
DWORD       WINAPI GetAppCompatFlags32(HTASK32);
#define     GetAppCompatFlags WINELIB_NAME(GetAppCompatFlags)
WORD        WINAPI GetAsyncKeyState32(INT32);
#define     GetAsyncKeyState WINELIB_NAME(GetAsyncKeyState)
HWND32      WINAPI GetCapture32(void);
#define     GetCapture WINELIB_NAME(GetCapture)
UINT32      WINAPI GetCaretBlinkTime32(void);
#define     GetCaretBlinkTime WINELIB_NAME(GetCaretBlinkTime)
BOOL32      WINAPI GetCaretPos32(LPPOINT32);
#define     GetCaretPos WINELIB_NAME(GetCaretPos)
BOOL32      WINAPI GetClassInfo32A(HINSTANCE32,LPCSTR,WNDCLASS32A *);
BOOL32      WINAPI GetClassInfo32W(HINSTANCE32,LPCWSTR,WNDCLASS32W *);
#define     GetClassInfo WINELIB_NAME_AW(GetClassInfo)
BOOL32      WINAPI GetClassInfoEx32A(HINSTANCE32,LPCSTR,WNDCLASSEX32A *);
BOOL32      WINAPI GetClassInfoEx32W(HINSTANCE32,LPCWSTR,WNDCLASSEX32W *);
#define     GetClassInfoEx WINELIB_NAME_AW(GetClassInfoEx)
LONG        WINAPI GetClassLong32A(HWND32,INT32);
LONG        WINAPI GetClassLong32W(HWND32,INT32);
#define     GetClassLong WINELIB_NAME_AW(GetClassLong)
INT32       WINAPI GetClassName32A(HWND32,LPSTR,INT32);
INT32       WINAPI GetClassName32W(HWND32,LPWSTR,INT32);
#define     GetClassName WINELIB_NAME_AW(GetClassName)
WORD        WINAPI GetClassWord32(HWND32,INT32);
#define     GetClassWord WINELIB_NAME(GetClassWord)
void        WINAPI GetClientRect32(HWND32,LPRECT32);
#define     GetClientRect WINELIB_NAME(GetClientRect)
HANDLE32    WINAPI GetClipboardData32(UINT32);
#define     GetClipboardData WINELIB_NAME(GetClipboardData)
INT32       WINAPI GetClipboardFormatName32A(UINT32,LPSTR,INT32);
INT32       WINAPI GetClipboardFormatName32W(UINT32,LPWSTR,INT32);
#define     GetClipboardFormatName WINELIB_NAME_AW(GetClipboardFormatName)
HWND32      WINAPI GetClipboardOwner32(void);
#define     GetClipboardOwner WINELIB_NAME(GetClipboardOwner)
HWND32      WINAPI GetClipboardViewer32(void);
#define     GetClipboardViewer WINELIB_NAME(GetClipboardViewer)
void        WINAPI GetClipCursor32(LPRECT32);
#define     GetClipCursor WINELIB_NAME(GetClipCursor)
#define     GetCurrentTime32() GetTickCount()
#define     GetCurrentTime WINELIB_NAME(GetCurrentTime)
HCURSOR32   WINAPI GetCursor32(void);
#define     GetCursor WINELIB_NAME(GetCursor)
BOOL32      WINAPI GetCursorPos32(LPPOINT32);
#define     GetCursorPos WINELIB_NAME(GetCursorPos)
HDC32       WINAPI GetDC32(HWND32);
#define     GetDC WINELIB_NAME(GetDC)
HDC32       WINAPI GetDCEx32(HWND32,HRGN32,DWORD);
#define     GetDCEx WINELIB_NAME(GetDCEx)
HWND32      WINAPI GetDesktopWindow32(void);
#define     GetDesktopWindow WINELIB_NAME(GetDesktopWindow)
INT32       WINAPI GetDlgCtrlID32(HWND32);
#define     GetDlgCtrlID WINELIB_NAME(GetDlgCtrlID)
HWND32      WINAPI GetDlgItem32(HWND32,INT32);
#define     GetDlgItem WINELIB_NAME(GetDlgItem)
UINT32      WINAPI GetDlgItemInt32(HWND32,INT32,BOOL32*,BOOL32);
#define     GetDlgItemInt WINELIB_NAME(GetDlgItemInt)
INT32       WINAPI GetDlgItemText32A(HWND32,INT32,LPSTR,UINT32);
INT32       WINAPI GetDlgItemText32W(HWND32,INT32,LPWSTR,UINT32);
#define     GetDlgItemText WINELIB_NAME_AW(GetDlgItemText)
UINT32      WINAPI GetDoubleClickTime32(void);
#define     GetDoubleClickTime WINELIB_NAME(GetDoubleClickTime)
HWND32      WINAPI GetFocus32(void);
#define     GetFocus WINELIB_NAME(GetFocus)
HWND32      WINAPI GetForegroundWindow32(void);
#define     GetForegroundWindow WINELIB_NAME(GetForegroundWindow)
BOOL32      WINAPI GetInputState32(void);
#define     GetInputState WINELIB_NAME(GetInputState)
UINT32      WINAPI GetInternalWindowPos32(HWND32,LPRECT32,LPPOINT32);
#define     GetInternalWindowPos WINELIB_NAME(GetInternalWindowPos)
UINT32      WINAPI GetKBCodePage32(void);
#define     GetKBCodePage WINELIB_NAME(GetKBCodePage)
INT32       WINAPI GetKeyboardType32(INT32);
#define     GetKeyboardType WINELIB_NAME(GetKeyboardType)
INT32       WINAPI GetKeyNameText32A(LONG,LPSTR,INT32);
INT32       WINAPI GetKeyNameText32W(LONG,LPWSTR,INT32);
#define     GetKeyNameText WINELIB_NAME_AW(GetKeyNameText)
INT32       WINAPI GetKeyboardLayoutName32A(LPSTR);
INT32       WINAPI GetKeyboardLayoutName32W(LPWSTR);
#define     GetKeyboardLayoutName WINELIB_NAME_AW(GetKeyboardLayoutName)
INT16       WINAPI GetKeyState32(INT32);
#define     GetKeyState WINELIB_NAME(GetKeyState)
HWND32      WINAPI GetLastActivePopup32(HWND32);
#define     GetLastActivePopup WINELIB_NAME(GetLastActivePopup)
HMENU32     WINAPI GetMenu32(HWND32);
#define     GetMenu WINELIB_NAME(GetMenu)
INT32       WINAPI GetMenuItemCount32(HMENU32);
#define     GetMenuItemCount WINELIB_NAME(GetMenuItemCount)
UINT32      WINAPI GetMenuItemID32(HMENU32,INT32);
#define     GetMenuItemID WINELIB_NAME(GetMenuItemID)
BOOL32      WINAPI GetMenuItemRect32(HWND32,HMENU32,UINT32,LPRECT32);
#define     GetMenuItemRect WINELIB_NAME(GetMenuItemRect)
UINT32      WINAPI GetMenuState32(HMENU32,UINT32,UINT32);
#define     GetMenuState WINELIB_NAME(GetMenuState)
INT32       WINAPI GetMenuString32A(HMENU32,UINT32,LPSTR,INT32,UINT32);
INT32       WINAPI GetMenuString32W(HMENU32,UINT32,LPWSTR,INT32,UINT32);
#define     GetMenuString WINELIB_NAME_AW(GetMenuString)
BOOL32      WINAPI GetMessage32A(LPMSG32,HWND32,UINT32,UINT32);
BOOL32      WINAPI GetMessage32W(LPMSG32,HWND32,UINT32,UINT32);
#define     GetMessage WINELIB_NAME_AW(GetMessage)
HWND32      WINAPI GetNextDlgGroupItem32(HWND32,HWND32,BOOL32);
#define     GetNextDlgGroupItem WINELIB_NAME(GetNextDlgGroupItem)
HWND32      WINAPI GetNextDlgTabItem32(HWND32,HWND32,BOOL32);
#define     GetNextDlgTabItem WINELIB_NAME(GetNextDlgTabItem)
#define     GetNextWindow32 GetWindow32
#define     GetNextWindow WINELIB_NAME(GetNextWindow)
HWND32      WINAPI GetOpenClipboardWindow32(void);
#define     GetOpenClipboardWindow WINELIB_NAME(GetOpenClipboardWindow)
HWND32      WINAPI GetParent32(HWND32);
#define     GetParent WINELIB_NAME(GetParent)
INT32       WINAPI GetPriorityClipboardFormat32(UINT32*,INT32);
#define     GetPriorityClipboardFormat WINELIB_NAME(GetPriorityClipboardFormat)
HANDLE32    WINAPI GetProp32A(HWND32,LPCSTR);
HANDLE32    WINAPI GetProp32W(HWND32,LPCWSTR);
#define     GetProp WINELIB_NAME_AW(GetProp)
DWORD       WINAPI GetQueueStatus32(UINT32);
#define     GetQueueStatus WINELIB_NAME(GetQueueStatus)
BOOL32      WINAPI GetScrollInfo32(HWND32,INT32,LPSCROLLINFO);
#define     GetScrollInfo WINELIB_NAME(GetScrollInfo)
INT32       WINAPI GetScrollPos32(HWND32,INT32);
#define     GetScrollPos WINELIB_NAME(GetScrollPos)
BOOL32      WINAPI GetScrollRange32(HWND32,INT32,LPINT32,LPINT32);
#define     GetScrollRange WINELIB_NAME(GetScrollRange)
HWND32      WINAPI GetShellWindow32(void);
#define     GetShellWindow WINELIB_NAME(GetShellWindow)
HMENU32     WINAPI GetSubMenu32(HMENU32,INT32);
#define     GetSubMenu WINELIB_NAME(GetSubMenu)
COLORREF    WINAPI GetSysColor32(INT32);
#define     GetSysColor WINELIB_NAME(GetSysColor)
HBRUSH32    WINAPI GetSysColorBrush32(INT32);
#define     GetSysColorBrush WINELIB_NAME(GetSysColorBrush)
#define     GetSysModalWindow32() ((HWND32)0)
#define     GetSysModalWindow WINELIB_NAME(GetSysModalWindow)
HMENU32     WINAPI GetSystemMenu32(HWND32,BOOL32);
#define     GetSystemMenu WINELIB_NAME(GetSystemMenu)
INT32       WINAPI GetSystemMetrics32(INT32);
#define     GetSystemMetrics WINELIB_NAME(GetSystemMetrics)
DWORD       WINAPI GetTabbedTextExtent32A(HDC32,LPCSTR,INT32,INT32,const INT32*);
DWORD       WINAPI GetTabbedTextExtent32W(HDC32,LPCWSTR,INT32,INT32,const INT32*);
#define     GetTabbedTextExtent WINELIB_NAME_AW(GetTabbedTextExtent)
HWND32      WINAPI GetTopWindow32(HWND32);
#define     GetTopWindow WINELIB_NAME(GetTopWindow)
BOOL32      WINAPI GetUpdateRect32(HWND32,LPRECT32,BOOL32);
#define     GetUpdateRect WINELIB_NAME(GetUpdateRect)
INT32       WINAPI GetUpdateRgn32(HWND32,HRGN32,BOOL32);
#define     GetUpdateRgn WINELIB_NAME(GetUpdateRgn)
HWND32      WINAPI GetWindow32(HWND32,WORD);
#define     GetWindow WINELIB_NAME(GetWindow)
HDC32       WINAPI GetWindowDC32(HWND32);
#define     GetWindowDC WINELIB_NAME(GetWindowDC)
LONG        WINAPI GetWindowLong32A(HWND32,INT32);
LONG        WINAPI GetWindowLong32W(HWND32,INT32);
#define     GetWindowLong WINELIB_NAME_AW(GetWindowLong)
BOOL32      WINAPI GetWindowPlacement32(HWND32,LPWINDOWPLACEMENT32);
#define     GetWindowPlacement WINELIB_NAME(GetWindowPlacement)
BOOL32      WINAPI GetWindowRect32(HWND32,LPRECT32);
#define     GetWindowRect WINELIB_NAME(GetWindowRect)
INT32       WINAPI GetWindowRgn32(HWND32,HRGN32);
#define     GetWindowRgn WINELIB_NAME(GetWindowRgn)
#define     GetWindowTask32(hwnd) ((HTASK32)GetWindowThreadProcessId(hwnd,NULL))
#define     GetWindowTask WINELIB_NAME(GetWindowTask)
INT32       WINAPI GetWindowText32A(HWND32,LPSTR,INT32);
INT32       WINAPI GetWindowText32W(HWND32,LPWSTR,INT32);
#define     GetWindowText WINELIB_NAME_AW(GetWindowText)
INT32       WINAPI GetWindowTextLength32A(HWND32);
INT32       WINAPI GetWindowTextLength32W(HWND32);
#define     GetWindowTextLength WINELIB_NAME_AW(GetWindowTextLength)
WORD        WINAPI GetWindowWord32(HWND32,INT32);
#define     GetWindowWord WINELIB_NAME(GetWindowWord)
ATOM        WINAPI GlobalAddAtom32A(LPCSTR);
ATOM        WINAPI GlobalAddAtom32W(LPCWSTR);
#define     GlobalAddAtom WINELIB_NAME_AW(GlobalAddAtom)
ATOM        WINAPI GlobalFindAtom32A(LPCSTR);
ATOM        WINAPI GlobalFindAtom32W(LPCWSTR);
#define     GlobalFindAtom WINELIB_NAME_AW(GlobalFindAtom)
UINT32      WINAPI GlobalGetAtomName32A(ATOM,LPSTR,INT32);
UINT32      WINAPI GlobalGetAtomName32W(ATOM,LPWSTR,INT32);
#define     GlobalGetAtomName WINELIB_NAME_AW(GlobalGetAtomName)
BOOL32      WINAPI GrayString32A(HDC32,HBRUSH32,GRAYSTRINGPROC32,LPARAM,
                                 INT32,INT32,INT32,INT32,INT32);
BOOL32      WINAPI GrayString32W(HDC32,HBRUSH32,GRAYSTRINGPROC32,LPARAM,
                                 INT32,INT32,INT32,INT32,INT32);
#define     GrayString WINELIB_NAME_AW(GrayString)
BOOL32      WINAPI HideCaret32(HWND32);
#define     HideCaret WINELIB_NAME(HideCaret)
BOOL32      WINAPI HiliteMenuItem32(HWND32,HMENU32,UINT32,UINT32);
#define     HiliteMenuItem WINELIB_NAME(HiliteMenuItem)
void        WINAPI InflateRect32(LPRECT32,INT32,INT32);
#define     InflateRect WINELIB_NAME(InflateRect)
BOOL32      WINAPI InSendMessage32(void);
#define     InSendMessage WINELIB_NAME(InSendMessage)
BOOL32      WINAPI InsertMenu32A(HMENU32,UINT32,UINT32,UINT32,LPCSTR);
BOOL32      WINAPI InsertMenu32W(HMENU32,UINT32,UINT32,UINT32,LPCWSTR);
#define     InsertMenu WINELIB_NAME_AW(InsertMenu)
BOOL32      WINAPI InsertMenuItem32A(HMENU32,UINT32,BOOL32,const MENUITEMINFO32A*);
BOOL32      WINAPI InsertMenuItem32W(HMENU32,UINT32,BOOL32,const MENUITEMINFO32W*);
#define     InsertMenuItem WINELIB_NAME_AW(InsertMenuItem)
BOOL32      WINAPI IntersectRect32(LPRECT32,const RECT32*,const RECT32*);
#define     IntersectRect WINELIB_NAME(IntersectRect)
void        WINAPI InvalidateRect32(HWND32,const RECT32*,BOOL32);
#define     InvalidateRect WINELIB_NAME(InvalidateRect)
void        WINAPI InvalidateRgn32(HWND32,HRGN32,BOOL32);
#define     InvalidateRgn WINELIB_NAME(InvalidateRgn)
void        WINAPI InvertRect32(HDC32,const RECT32*);
#define     InvertRect WINELIB_NAME(InvertRect)
BOOL32      WINAPI IsCharAlpha32A(CHAR);
BOOL32      WINAPI IsCharAlpha32W(WCHAR);
#define     IsCharAlpha WINELIB_NAME_AW(IsCharAlpha)
BOOL32      WINAPI IsCharAlphaNumeric32A(CHAR);
BOOL32      WINAPI IsCharAlphaNumeric32W(WCHAR);
#define     IsCharAlphaNumeric WINELIB_NAME_AW(IsCharAlphaNumeric)
BOOL32      WINAPI IsCharLower32A(CHAR);
BOOL32      WINAPI IsCharLower32W(WCHAR);
#define     IsCharLower WINELIB_NAME_AW(IsCharLower)
BOOL32      WINAPI IsCharUpper32A(CHAR);
BOOL32      WINAPI IsCharUpper32W(WCHAR);
#define     IsCharUpper WINELIB_NAME_AW(IsCharUpper)
BOOL32      WINAPI IsChild32(HWND32,HWND32);
#define     IsChild WINELIB_NAME(IsChild)
BOOL32      WINAPI IsClipboardFormatAvailable32(UINT32);
#define     IsClipboardFormatAvailable WINELIB_NAME(IsClipboardFormatAvailable)
BOOL32      WINAPI IsDialogMessage32A(HWND32,LPMSG32);
BOOL32      WINAPI IsDialogMessage32W(HWND32,LPMSG32);
#define     IsDialogMessage WINELIB_NAME_AW(IsDialogMessage)
UINT32      WINAPI IsDlgButtonChecked32(HWND32,UINT32);
#define     IsDlgButtonChecked WINELIB_NAME(IsDlgButtonChecked)
BOOL32      WINAPI IsIconic32(HWND32);
#define     IsIconic WINELIB_NAME(IsIconic)
BOOL32      WINAPI IsMenu32(HMENU32);
#define     IsMenu WINELIB_NAME(IsMenu)
BOOL32      WINAPI IsRectEmpty32(const RECT32*);
#define     IsRectEmpty WINELIB_NAME(IsRectEmpty)
BOOL32      WINAPI IsWindow32(HWND32);
#define     IsWindow WINELIB_NAME(IsWindow)
BOOL32      WINAPI IsWindowEnabled32(HWND32);
#define     IsWindowEnabled WINELIB_NAME(IsWindowEnabled)
BOOL32      WINAPI IsWindowVisible32(HWND32);
#define     IsWindowVisible WINELIB_NAME(IsWindowVisible)
BOOL32      WINAPI IsZoomed32(HWND32);
#define     IsZoomed WINELIB_NAME(IsZoomed)
BOOL32      WINAPI KillSystemTimer32(HWND32,UINT32);
#define     KillSystemTimer WINELIB_NAME(KillSystemTimer)
BOOL32      WINAPI KillTimer32(HWND32,UINT32);
#define     KillTimer WINELIB_NAME(KillTimer)
HACCEL32    WINAPI LoadAccelerators32A(HINSTANCE32,LPCSTR);
HACCEL32    WINAPI LoadAccelerators32W(HINSTANCE32,LPCWSTR);
#define     LoadAccelerators WINELIB_NAME_AW(LoadAccelerators)
HBITMAP32   WINAPI LoadBitmap32A(HANDLE32,LPCSTR);
HBITMAP32   WINAPI LoadBitmap32W(HANDLE32,LPCWSTR);
#define     LoadBitmap WINELIB_NAME_AW(LoadBitmap)
HCURSOR32   WINAPI LoadCursor32A(HINSTANCE32,LPCSTR);
HCURSOR32   WINAPI LoadCursor32W(HINSTANCE32,LPCWSTR);
#define     LoadCursor WINELIB_NAME_AW(LoadCursor)
HCURSOR32   WINAPI LoadCursorFromFile32A(LPCSTR);
HCURSOR32   WINAPI LoadCursorFromFile32W(LPCWSTR);
#define     LoadCursorFromFile WINELIB_NAME_AW(LoadCursorFromFile)
HICON32     WINAPI LoadIcon32A(HINSTANCE32,LPCSTR);
HICON32     WINAPI LoadIcon32W(HINSTANCE32,LPCWSTR);
#define     LoadIcon WINELIB_NAME_AW(LoadIcon)
HANDLE32    WINAPI LoadImage32A(HINSTANCE32,LPCSTR,UINT32,INT32,INT32,UINT32);
HANDLE32    WINAPI LoadImage32W(HINSTANCE32,LPCWSTR,UINT32,INT32,INT32,UINT32);
#define     LoadImage WINELIB_NAME_AW(LoadImage)
HMENU32     WINAPI LoadMenu32A(HINSTANCE32,LPCSTR);
HMENU32     WINAPI LoadMenu32W(HINSTANCE32,LPCWSTR);
#define     LoadMenu WINELIB_NAME_AW(LoadMenu)
HMENU32     WINAPI LoadMenuIndirect32A(LPCVOID);
HMENU32     WINAPI LoadMenuIndirect32W(LPCVOID);
#define     LoadMenuIndirect WINELIB_NAME_AW(LoadMenuIndirect)
INT32       WINAPI LoadString32A(HINSTANCE32,UINT32,LPSTR,INT32);
INT32       WINAPI LoadString32W(HINSTANCE32,UINT32,LPWSTR,INT32);
#define     LoadString WINELIB_NAME_AW(LoadString)
BOOL32      WINAPI LockWindowUpdate32(HWND32);
#define     LockWindowUpdate WINELIB_NAME(LockWindowUpdate)
INT32       WINAPI LookupIconIdFromDirectory32(LPBYTE,BOOL32);
#define     LookupIconIdFromDirectory WINELIB_NAME(LookupIconIdFromDirectory)
INT32       WINAPI LookupIconIdFromDirectoryEx32(LPBYTE,BOOL32,INT32,INT32,UINT32);
#define     LookupIconIdFromDirectoryEx WINELIB_NAME(LookupIconIdFromDirectoryEx)
UINT32      WINAPI MapVirtualKey32A(UINT32,UINT32);
UINT32      WINAPI MapVirtualKey32W(UINT32,UINT32);
#define     MapVirtualKey WINELIB_NAME_AW(MapVirtualKey)
UINT32      WINAPI MapVirtualKeyEx32A(UINT32,UINT32,HKL32);
#define     MapVirtualKeyEx WINELIB_NAME_AW(MapVirtualKeyEx)
void        WINAPI MapDialogRect32(HWND32,LPRECT32);
#define     MapDialogRect WINELIB_NAME(MapDialogRect)
void        WINAPI MapWindowPoints32(HWND32,HWND32,LPPOINT32,UINT32);
#define     MapWindowPoints WINELIB_NAME(MapWindowPoints)
BOOL32      WINAPI MessageBeep32(UINT32);
#define     MessageBeep WINELIB_NAME(MessageBeep)
INT32       WINAPI MessageBox32A(HWND32,LPCSTR,LPCSTR,UINT32);
INT32       WINAPI MessageBox32W(HWND32,LPCWSTR,LPCWSTR,UINT32);
#define     MessageBox WINELIB_NAME_AW(MessageBox)
INT32       WINAPI MessageBoxIndirect32A(LPMSGBOXPARAMS32A);
INT32       WINAPI MessageBoxIndirect32W(LPMSGBOXPARAMS32W);
#define     MessageBoxIndirect WINELIB_NAME_AW(MessageBoxIndirect)
BOOL32      WINAPI ModifyMenu32A(HMENU32,UINT32,UINT32,UINT32,LPCSTR);
BOOL32      WINAPI ModifyMenu32W(HMENU32,UINT32,UINT32,UINT32,LPCWSTR);
#define     ModifyMenu WINELIB_NAME_AW(ModifyMenu)
BOOL32      WINAPI MoveWindow32(HWND32,INT32,INT32,INT32,INT32,BOOL32);
#define     MoveWindow WINELIB_NAME(MoveWindow)
BOOL32      WINAPI OemToChar32A(LPCSTR,LPSTR);
BOOL32      WINAPI OemToChar32W(LPCSTR,LPWSTR);
#define     OemToChar WINELIB_NAME_AW(OemToChar)
BOOL32      WINAPI OemToCharBuff32A(LPCSTR,LPSTR,DWORD);
BOOL32      WINAPI OemToCharBuff32W(LPCSTR,LPWSTR,DWORD);
#define     OemToCharBuff WINELIB_NAME_AW(OemToCharBuff)
void        WINAPI OffsetRect32(LPRECT32,INT32,INT32);
#define     OffsetRect WINELIB_NAME(OffsetRect)
BOOL32      WINAPI OpenClipboard32(HWND32);
#define     OpenClipboard WINELIB_NAME(OpenClipboard)
BOOL32      WINAPI OpenIcon32(HWND32);
#define     OpenIcon WINELIB_NAME(OpenIcon)
BOOL32      WINAPI PeekMessage32A(LPMSG32,HWND32,UINT32,UINT32,UINT32);
BOOL32      WINAPI PeekMessage32W(LPMSG32,HWND32,UINT32,UINT32,UINT32);
#define     PeekMessage WINELIB_NAME_AW(PeekMessage)
#define     PostAppMessage32A(thread,msg,wparam,lparam) \
            PostThreadMessage32A((DWORD)(thread),msg,wparam,lparam)
#define     PostAppMessage32W(thread,msg,wparam,lparam) \
            PostThreadMessage32W((DWORD)(thread),msg,wparam,lparam)
#define     PostAppMessage WINELIB_NAME_AW(PostAppMessage)
BOOL32      WINAPI PostMessage32A(HWND32,UINT32,WPARAM32,LPARAM);
BOOL32      WINAPI PostMessage32W(HWND32,UINT32,WPARAM32,LPARAM);
#define     PostMessage WINELIB_NAME_AW(PostMessage)
void        WINAPI PostQuitMessage32(INT32);
#define     PostQuitMessage WINELIB_NAME(PostQuitMessage)
BOOL32      WINAPI PtInRect32(const RECT32*,POINT32);
#define     PtInRect WINELIB_NAME(PtInRect)
BOOL32      WINAPI RedrawWindow32(HWND32,const RECT32*,HRGN32,UINT32);
#define     RedrawWindow WINELIB_NAME(RedrawWindow)
ATOM        WINAPI RegisterClass32A(const WNDCLASS32A *);
ATOM        WINAPI RegisterClass32W(const WNDCLASS32W *);
#define     RegisterClass WINELIB_NAME_AW(RegisterClass)
ATOM        WINAPI RegisterClassEx32A(const WNDCLASSEX32A *);
ATOM        WINAPI RegisterClassEx32W(const WNDCLASSEX32W *);
#define     RegisterClassEx WINELIB_NAME_AW(RegisterClassEx)
UINT32      WINAPI RegisterClipboardFormat32A(LPCSTR);
UINT32      WINAPI RegisterClipboardFormat32W(LPCWSTR);
#define     RegisterClipboardFormat WINELIB_NAME_AW(RegisterClipboardFormat)
WORD        WINAPI RegisterWindowMessage32A(LPCSTR);
WORD        WINAPI RegisterWindowMessage32W(LPCWSTR);
#define     RegisterWindowMessage WINELIB_NAME_AW(RegisterWindowMessage)
INT32       WINAPI ReleaseDC32(HWND32,HDC32);
#define     ReleaseDC WINELIB_NAME(ReleaseDC)
BOOL32      WINAPI RemoveMenu32(HMENU32,UINT32,UINT32);
#define     RemoveMenu WINELIB_NAME(RemoveMenu)
HANDLE32    WINAPI RemoveProp32A(HWND32,LPCSTR);
HANDLE32    WINAPI RemoveProp32W(HWND32,LPCWSTR);
#define     RemoveProp WINELIB_NAME_AW(RemoveProp)
BOOL32      WINAPI ReplyMessage32(LRESULT);
#define     ReplyMessage WINELIB_NAME(ReplyMessage)
void        WINAPI ScreenToClient32(HWND32,LPPOINT32);
#define     ScreenToClient WINELIB_NAME(ScreenToClient)
VOID        WINAPI ScrollChildren32(HWND32,UINT32,WPARAM32,LPARAM);
#define     ScrollChildren WINELIB_NAME(ScrollChildren)
BOOL32      WINAPI ScrollDC32(HDC32,INT32,INT32,const RECT32*,const RECT32*,
                      HRGN32,LPRECT32);
#define     ScrollDC WINELIB_NAME(ScrollDC)
BOOL32      WINAPI ScrollWindow32(HWND32,INT32,INT32,const RECT32*,const RECT32*);
#define     ScrollWindow WINELIB_NAME(ScrollWindow)
INT32       WINAPI ScrollWindowEx32(HWND32,INT32,INT32,const RECT32*,
                                    const RECT32*,HRGN32,LPRECT32,UINT32);
#define     ScrollWindowEx WINELIB_NAME(ScrollWindowEx)
LRESULT     WINAPI SendDlgItemMessage32A(HWND32,INT32,UINT32,WPARAM32,LPARAM);
LRESULT     WINAPI SendDlgItemMessage32W(HWND32,INT32,UINT32,WPARAM32,LPARAM);
#define     SendDlgItemMessage WINELIB_NAME_AW(SendDlgItemMessage)
LRESULT     WINAPI SendMessage32A(HWND32,UINT32,WPARAM32,LPARAM);
LRESULT     WINAPI SendMessage32W(HWND32,UINT32,WPARAM32,LPARAM);
#define     SendMessage WINELIB_NAME_AW(SendMessage)
LRESULT     WINAPI SendMessageTimeout32A(HWND32,UINT32,WPARAM32,LPARAM,UINT32,
					 UINT32,LPDWORD);
LRESULT     WINAPI SendMessageTimeout32W(HWND32,UINT32,WPARAM32,LPARAM,UINT32,
					 UINT32,LPDWORD);
#define     SendMessageTimeout WINELIB_NAME_AW(SendMessageTimeout)
HWND32      WINAPI SetActiveWindow32(HWND32);
#define     SetActiveWindow WINELIB_NAME(SetActiveWindow)
HWND32      WINAPI SetCapture32(HWND32);
#define     SetCapture WINELIB_NAME(SetCapture)
BOOL32      WINAPI SetCaretBlinkTime32(UINT32);
#define     SetCaretBlinkTime WINELIB_NAME(SetCaretBlinkTime)
BOOL32      WINAPI SetCaretPos32(INT32,INT32);
#define     SetCaretPos WINELIB_NAME(SetCaretPos)
LONG        WINAPI SetClassLong32A(HWND32,INT32,LONG);
LONG        WINAPI SetClassLong32W(HWND32,INT32,LONG);
#define     SetClassLong WINELIB_NAME_AW(SetClassLong)
WORD        WINAPI SetClassWord32(HWND32,INT32,WORD);
#define     SetClassWord WINELIB_NAME(SetClassWord)
HANDLE32    WINAPI SetClipboardData32(UINT32,HANDLE32);
#define     SetClipboardData WINELIB_NAME(SetClipboardData)
HWND32      WINAPI SetClipboardViewer32(HWND32);
#define     SetClipboardViewer WINELIB_NAME(SetClipboardViewer)
BOOL32      WINAPI SetCommBreak32(INT32);
#define     SetCommBreak WINELIB_NAME(SetCommBreak)
HCURSOR32   WINAPI SetCursor32(HCURSOR32);
#define     SetCursor WINELIB_NAME(SetCursor)
BOOL32      WINAPI SetCursorPos32(INT32,INT32);
#define     SetCursorPos WINELIB_NAME(SetCursorPos)
BOOL32      WINAPI SetDeskWallPaper32(LPCSTR);
#define     SetDeskWallPaper WINELIB_NAME(SetDeskWallPaper)
void        WINAPI SetDlgItemInt32(HWND32,INT32,UINT32,BOOL32);
#define     SetDlgItemInt WINELIB_NAME(SetDlgItemInt)
BOOL32      WINAPI SetDlgItemText32A(HWND32,INT32,LPCSTR);
BOOL32      WINAPI SetDlgItemText32W(HWND32,INT32,LPCWSTR);
#define     SetDlgItemText WINELIB_NAME_AW(SetDlgItemText)
BOOL32      WINAPI SetDoubleClickTime32(UINT32);
#define     SetDoubleClickTime WINELIB_NAME(SetDoubleClickTime)
HWND32      WINAPI SetFocus32(HWND32);
#define     SetFocus WINELIB_NAME(SetFocus)
BOOL32      WINAPI SetForegroundWindow32(HWND32);
#define     SetForegroundWindow WINELIB_NAME(SetForegroundWindow)
void        WINAPI SetInternalWindowPos32(HWND32,UINT32,LPRECT32,LPPOINT32);
#define     SetInternalWindowPos WINELIB_NAME(SetInternalWindowPos)
BOOL32      WINAPI SetMenu32(HWND32,HMENU32);
#define     SetMenu WINELIB_NAME(SetMenu)
BOOL32      WINAPI SetMenuContextHelpId32(HMENU32,DWORD);
#define     SetMenuContextHelpId WINELIB_NAME(SetMenuContextHelpId)
BOOL32      WINAPI SetMenuItemBitmaps32(HMENU32,UINT32,UINT32,HBITMAP32,HBITMAP32);
#define     SetMenuItemBitmaps WINELIB_NAME(SetMenuItemBitmaps)
BOOL32      WINAPI SetMessageQueue32(INT32);
#define     SetMessageQueue WINELIB_NAME(SetMessageQueue)
HWND32      WINAPI SetParent32(HWND32,HWND32);
#define     SetParent WINELIB_NAME(SetParent)
BOOL32      WINAPI SetProp32A(HWND32,LPCSTR,HANDLE32);
BOOL32      WINAPI SetProp32W(HWND32,LPCWSTR,HANDLE32);
#define     SetProp WINELIB_NAME_AW(SetProp)
void        WINAPI SetRect32(LPRECT32,INT32,INT32,INT32,INT32);
#define     SetRect WINELIB_NAME(SetRect)
void        WINAPI SetRectEmpty32(LPRECT32);
#define     SetRectEmpty WINELIB_NAME(SetRectEmpty)
INT32       WINAPI SetScrollInfo32(HWND32,INT32,const SCROLLINFO*,BOOL32);
#define     SetScrollInfo WINELIB_NAME(SetScrollInfo)
INT32       WINAPI SetScrollPos32(HWND32,INT32,INT32,BOOL32);
#define     SetScrollPos WINELIB_NAME(SetScrollPos)
BOOL32      WINAPI SetScrollRange32(HWND32,INT32,INT32,INT32,BOOL32);
#define     SetScrollRange WINELIB_NAME(SetScrollRange)
BOOL32      WINAPI SetSysColors32(INT32,const INT32*,const COLORREF*);
#define     SetSysColors WINELIB_NAME(SetSysColors)
#define     SetSysModalWindow32(hwnd) ((HWND32)0)
#define     SetSysModalWindow WINELIB_NAME(SetSysModalWindow)
BOOL32      WINAPI SetSystemMenu32(HWND32,HMENU32);
#define     SetSystemMenu WINELIB_NAME(SetSystemMenu)
UINT32      WINAPI SetSystemTimer32(HWND32,UINT32,UINT32,TIMERPROC32);
#define     SetSystemTimer WINELIB_NAME(SetSystemTimer)
UINT32      WINAPI SetTimer32(HWND32,UINT32,UINT32,TIMERPROC32);
#define     SetTimer WINELIB_NAME(SetTimer)
LONG        WINAPI SetWindowLong32A(HWND32,INT32,LONG);
LONG        WINAPI SetWindowLong32W(HWND32,INT32,LONG);
#define     SetWindowLong WINELIB_NAME_AW(SetWindowLong)
BOOL32      WINAPI SetWindowPlacement32(HWND32,const WINDOWPLACEMENT32*);
#define     SetWindowPlacement WINELIB_NAME(SetWindowPlacement)
HHOOK       WINAPI SetWindowsHook32A(INT32,HOOKPROC32);
HHOOK       WINAPI SetWindowsHook32W(INT32,HOOKPROC32);
#define     SetWindowsHook WINELIB_NAME_AW(SetWindowsHook)
HHOOK       WINAPI SetWindowsHookEx32A(INT32,HOOKPROC32,HINSTANCE32,DWORD);
HHOOK       WINAPI SetWindowsHookEx32W(INT32,HOOKPROC32,HINSTANCE32,DWORD);
#define     SetWindowsHookEx WINELIB_NAME_AW(SetWindowsHookEx)
BOOL32      WINAPI SetWindowPos32(HWND32,HWND32,INT32,INT32,INT32,INT32,WORD);
#define     SetWindowPos WINELIB_NAME(SetWindowPos)
INT32       WINAPI SetWindowRgn32(HWND32,HRGN32,BOOL32);
#define     SetWindowRgn WINELIB_NAME(SetWindowRgn)
BOOL32      WINAPI SetWindowText32A(HWND32,LPCSTR);
BOOL32      WINAPI SetWindowText32W(HWND32,LPCWSTR);
#define     SetWindowText WINELIB_NAME_AW(SetWindowText)
WORD        WINAPI SetWindowWord32(HWND32,INT32,WORD);
#define     SetWindowWord WINELIB_NAME(SetWindowWord)
BOOL32      WINAPI ShowCaret32(HWND32);
#define     ShowCaret WINELIB_NAME(ShowCaret)
INT32       WINAPI ShowCursor32(BOOL32);
#define     ShowCursor WINELIB_NAME(ShowCursor)
BOOL32      WINAPI ShowScrollBar32(HWND32,INT32,BOOL32);
#define     ShowScrollBar WINELIB_NAME(ShowScrollBar)
BOOL32      WINAPI ShowOwnedPopups32(HWND32,BOOL32);
#define     ShowOwnedPopups WINELIB_NAME(ShowOwnedPopups)
BOOL32      WINAPI ShowWindow32(HWND32,INT32);
#define     ShowWindow WINELIB_NAME(ShowWindow)
BOOL32      WINAPI SubtractRect32(LPRECT32,const RECT32*,const RECT32*);
#define     SubtractRect WINELIB_NAME(SubtractRect)
BOOL32      WINAPI SwapMouseButton32(BOOL32);
#define     SwapMouseButton WINELIB_NAME(SwapMouseButton)
VOID        WINAPI SwitchToThisWindow32(HWND32,BOOL32);
#define     SwitchToThisWindow WINELIB_NAME(SwitchToThisWindow)
BOOL32      WINAPI SystemParametersInfo32A(UINT32,UINT32,LPVOID,UINT32);
BOOL32      WINAPI SystemParametersInfo32W(UINT32,UINT32,LPVOID,UINT32);
#define     SystemParametersInfo WINELIB_NAME_AW(SystemParametersInfo)
LONG        WINAPI TabbedTextOut32A(HDC32,INT32,INT32,LPCSTR,INT32,INT32,const INT32*,INT32);
LONG        WINAPI TabbedTextOut32W(HDC32,INT32,INT32,LPCWSTR,INT32,INT32,const INT32*,INT32);
#define     TabbedTextOut WINELIB_NAME_AW(TabbedTextOut)
INT32       WINAPI ToAscii32(UINT32,UINT32,LPBYTE,LPWORD,UINT32);
#define     ToAscii WINELIB_NAME(ToAscii)
BOOL32      WINAPI TrackPopupMenu32(HMENU32,UINT32,INT32,INT32,INT32,HWND32,const RECT32*);
#define     TrackPopupMenu WINELIB_NAME(TrackPopupMenu)
INT32       WINAPI TranslateAccelerator32(HWND32,HACCEL32,LPMSG32);
#define     TranslateAccelerator WINELIB_NAME(TranslateAccelerator)
BOOL32      WINAPI TranslateMDISysAccel32(HWND32,LPMSG32);
#define     TranslateMDISysAccel WINELIB_NAME(TranslateMDISysAccel)
BOOL32      WINAPI TranslateMessage32(const MSG32*);
#define     TranslateMessage WINELIB_NAME(TranslateMessage)
BOOL32      WINAPI UnhookWindowsHook32(INT32,HOOKPROC32);
#define     UnhookWindowsHook WINELIB_NAME(UnhookWindowsHook)
BOOL32      WINAPI UnhookWindowsHookEx32(HHOOK);
#define     UnhookWindowsHookEx WINELIB_NAME(UnhookWindowsHookEx)
BOOL32      WINAPI UnionRect32(LPRECT32,const RECT32*,const RECT32*);
#define     UnionRect WINELIB_NAME(UnionRect)
BOOL32      WINAPI UnregisterClass32A(LPCSTR,HINSTANCE32);
BOOL32      WINAPI UnregisterClass32W(LPCWSTR,HINSTANCE32);
#define     UnregisterClass WINELIB_NAME_AW(UnregisterClass)
VOID        WINAPI UpdateWindow32(HWND32);
#define     UpdateWindow WINELIB_NAME(UpdateWindow)
VOID        WINAPI ValidateRect32(HWND32,const RECT32*);
#define     ValidateRect WINELIB_NAME(ValidateRect)
VOID        WINAPI ValidateRgn32(HWND32,HRGN32);
#define     ValidateRgn WINELIB_NAME(ValidateRgn)
WORD        WINAPI VkKeyScan32A(CHAR);
WORD        WINAPI VkKeyScan32W(WCHAR);
#define     VkKeyScan WINELIB_NAME_AW(VkKeyScan)
HWND32      WINAPI WindowFromDC32(HDC32);
#define     WindowFromDC WINELIB_NAME(WindowFromDC)
HWND32      WINAPI WindowFromPoint32(POINT32);
#define     WindowFromPoint WINELIB_NAME(WindowFromPoint)
BOOL32      WINAPI WinHelp32A(HWND32,LPCSTR,UINT32,DWORD);
BOOL32      WINAPI WinHelp32W(HWND32,LPCWSTR,UINT32,DWORD);
#define     WinHelp WINELIB_NAME_AW(WinHelp)
UINT32      WINAPI WNetAddConnection32A(LPCSTR,LPCSTR,LPCSTR);
UINT32      WINAPI WNetAddConnection32W(LPCWSTR,LPCWSTR,LPCWSTR);
#define     WNetAddConnection WINELIB_NAME_AW(WNetAddConnection)
INT32       WINAPIV wsnprintf32A(LPSTR,UINT32,LPCSTR,...);
INT32       WINAPIV wsnprintf32W(LPWSTR,UINT32,LPCWSTR,...);
#define     wsnprintf WINELIB_NAME_AW(wsnprintf)
INT32       WINAPIV wsprintf32A(LPSTR,LPCSTR,...);
INT32       WINAPIV wsprintf32W(LPWSTR,LPCWSTR,...);
#define     wsprintf WINELIB_NAME_AW(wsprintf)
INT32       WINAPI wvsnprintf32A(LPSTR,UINT32,LPCSTR,va_list);
INT32       WINAPI wvsnprintf32W(LPWSTR,UINT32,LPCWSTR,va_list);
#define     wvsnprintf WINELIB_NAME_AW(wvsnprintf)
INT32       WINAPI wvsprintf32A(LPSTR,LPCSTR,va_list);
INT32       WINAPI wvsprintf32W(LPWSTR,LPCWSTR,va_list);
#define     wvsprintf WINELIB_NAME_AW(wvsprintf)

BOOL32      WINAPI RegisterShellHook(HWND16,UINT16);
/* NOTE: This is SYSTEM.3, not USER.182, which is also named KillSystemTimer */
WORD        WINAPI SYSTEM_KillSystemTimer( WORD );

/* Extra functions that don't exist in the Windows API */

HPEN32      WINAPI GetSysColorPen32(INT32);
INT32       WINAPI LoadMessage32A(HMODULE32,UINT32,WORD,LPSTR,INT32);
INT32       WINAPI LoadMessage32W(HMODULE32,UINT32,WORD,LPWSTR,INT32);

VOID        WINAPI ScreenSwitchEnable(WORD);

#endif /* __INCLUDE_WINUSER_H */
