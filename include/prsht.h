#ifndef _WINE_PRSHT_H
#define _WINE_PRSHT_H


#ifdef __cplusplus
extern "C" {
#endif


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
#define DUMMYUNIONNAME
#define DUMMYUNIONNAME1
#define DUMMYUNIONNAME2
#define DUMMYUNIONNAME3
#define DUMMYUNIONNAME4
#define DUMMYUNIONNAME5
#else
#define DUMMYUNIONNAME   u
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
	 

#ifdef __cplusplus
}
#endif



#endif /* _WINE_PRSHT_H */
