/* 
 * COMMDLG - Common Wine Dialog ... :-)
 */

#ifndef __WINE_COMMDLG_H
#define __WINE_COMMDLG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "wintypes.h"		/* needed for CHOOSEFONT structure */

#define OFN_READONLY                 0x00000001
#define OFN_OVERWRITEPROMPT          0x00000002
#define OFN_HIDEREADONLY             0x00000004
#define OFN_NOCHANGEDIR              0x00000008
#define OFN_SHOWHELP                 0x00000010
#define OFN_ENABLEHOOK               0x00000020
#define OFN_ENABLETEMPLATE           0x00000040
#define OFN_ENABLETEMPLATEHANDLE     0x00000080
#define OFN_NOVALIDATE               0x00000100
#define OFN_ALLOWMULTISELECT         0x00000200
#define OFN_EXTENSIONDIFFERENT       0x00000400
#define OFN_PATHMUSTEXIST            0x00000800
#define OFN_FILEMUSTEXIST            0x00001000
#define OFN_CREATEPROMPT             0x00002000
#define OFN_SHAREAWARE               0x00004000
#define OFN_NOREADONLYRETURN         0x00008000
#define OFN_NOTESTFILECREATE         0x00010000

/*      OFN_?                        0x00020000 */

#define OFN_NOLONGNAMES              0x00040000
#define OFN_EXPLORER                 0x00080000
#define OFN_NODEREFERENCELINKS       0x00100000
#define OFN_LONGNAMES                0x00200000

/* WINE internal flags */
#define OFN_UNICODE		     0x40000000	/*to differ between 32W/A hook*/
#define OFN_WINE32		     0x80000000	/* comdlg32 */

#define OFN_SHAREFALLTHROUGH     2
#define OFN_SHARENOWARN          1
#define OFN_SHAREWARN            0

typedef struct {
	DWORD		lStructSize;
	HWND16		hwndOwner;
	HINSTANCE16	hInstance;
	SEGPTR	        lpstrFilter;
	SEGPTR          lpstrCustomFilter;
	DWORD		nMaxCustFilter;
	DWORD		nFilterIndex;
	SEGPTR          lpstrFile;
	DWORD		nMaxFile;
	SEGPTR		lpstrFileTitle;
	DWORD		nMaxFileTitle;
	SEGPTR 		lpstrInitialDir;
	SEGPTR 		lpstrTitle;
	DWORD		Flags;
	UINT16		nFileOffset;
	UINT16		nFileExtension;
	SEGPTR		lpstrDefExt;
	LPARAM 		lCustData;
        WNDPROC16       lpfnHook;
	SEGPTR 		lpTemplateName;
}   OPENFILENAME16,*LPOPENFILENAME16;

typedef struct {
	DWORD		lStructSize;
	HWND32		hwndOwner;
	HINSTANCE32	hInstance;
	LPCSTR		lpstrFilter;
	LPSTR		lpstrCustomFilter;
	DWORD		nMaxCustFilter;
	DWORD		nFilterIndex;
	LPSTR		lpstrFile;
	DWORD		nMaxFile;
	LPSTR		lpstrFileTitle;
	DWORD		nMaxFileTitle;
	LPCSTR		lpstrInitialDir;
	LPCSTR		lpstrTitle;
	DWORD		Flags;
	WORD		nFileOffset;
	WORD		nFileExtension;
	LPCSTR		lpstrDefExt;
	LPARAM		lCustData;
	WNDPROC32	lpfnHook;
	LPCSTR		lpTemplateName;
} OPENFILENAME32A,*LPOPENFILENAME32A;

typedef struct {
	DWORD		lStructSize;
	HWND32		hwndOwner;
	HINSTANCE32	hInstance;
	LPCWSTR		lpstrFilter;
	LPWSTR		lpstrCustomFilter;
	DWORD		nMaxCustFilter;
	DWORD		nFilterIndex;
	LPWSTR		lpstrFile;
	DWORD		nMaxFile;
	LPWSTR		lpstrFileTitle;
	DWORD		nMaxFileTitle;
	LPCWSTR		lpstrInitialDir;
	LPCWSTR		lpstrTitle;
	DWORD		Flags;
	WORD		nFileOffset;
	WORD		nFileExtension;
	LPCWSTR		lpstrDefExt;
	LPARAM		lCustData;
	WNDPROC32	lpfnHook;
	LPCWSTR		lpTemplateName;
} OPENFILENAME32W,*LPOPENFILENAME32W;

DECL_WINELIB_TYPE_AW(OPENFILENAME);
DECL_WINELIB_TYPE_AW(LPOPENFILENAME);

typedef struct {
	DWORD		lStructSize;
	HWND16		hwndOwner;
	HWND16		hInstance;
	COLORREF	rgbResult;
	COLORREF       *lpCustColors;
	DWORD 		Flags;
	LPARAM		lCustData;
        WNDPROC16       lpfnHook;
	SEGPTR 		lpTemplateName;
} CHOOSECOLOR;
typedef CHOOSECOLOR *LPCHOOSECOLOR;

#define CC_RGBINIT               0x00000001
#define CC_FULLOPEN              0x00000002
#define CC_PREVENTFULLOPEN       0x00000004
#define CC_SHOWHELP              0x00000008
#define CC_ENABLEHOOK            0x00000010
#define CC_ENABLETEMPLATE        0x00000020
#define CC_ENABLETEMPLATEHANDLE  0x00000040

typedef struct {
	DWORD		lStructSize; 			/* size of this struct 0x20 */
	HWND16		hwndOwner; 				/* handle to owner's window */
	HINSTANCE16	hInstance; 				/* instance handle of.EXE that  */
										/*	contains cust. dlg. template */
	DWORD		Flags;                  /* one or more of the FR_?? */
	SEGPTR		lpstrFindWhat;          /* ptr. to search string    */
	SEGPTR		lpstrReplaceWith;       /* ptr. to replace string   */
	UINT16		wFindWhatLen;           /* size of find buffer      */
	UINT16 		wReplaceWithLen;        /* size of replace buffer   */
	LPARAM 		lCustData;              /* data passed to hook fn.  */
        WNDPROC16       lpfnHook;
	SEGPTR 		lpTemplateName;         /* custom template name     */
	} FINDREPLACE16, *LPFINDREPLACE16;

typedef struct {
	DWORD		lStructSize;
	HWND32		hwndOwner;
	HINSTANCE32	hInstance;

	DWORD		Flags;
	LPSTR		lpstrFindWhat;
	LPSTR		lpstrReplaceWith;
	WORD		wFindWhatLen;
	WORD 		wReplaceWithLen;
	LPARAM 		lCustData;
        WNDPROC32       lpfnHook;
	LPCSTR 		lpTemplateName;
	} FINDREPLACE32A, *LPFINDREPLACE32A;

typedef struct {
	DWORD		lStructSize;
	HWND32		hwndOwner;
	HINSTANCE32	hInstance;

	DWORD		Flags;
	LPWSTR		lpstrFindWhat;
	LPWSTR		lpstrReplaceWith;
	WORD		wFindWhatLen;
	WORD 		wReplaceWithLen;
	LPARAM 		lCustData;
        WNDPROC32       lpfnHook;
	LPCWSTR		lpTemplateName;
	} FINDREPLACE32W, *LPFINDREPLACE32W;
	
DECL_WINELIB_TYPE_AW(FINDREPLACE);
DECL_WINELIB_TYPE_AW(LPFINDREPLACE);
	
#define FR_DOWN                         0x00000001
#define FR_WHOLEWORD                    0x00000002
#define FR_MATCHCASE                    0x00000004
#define FR_FINDNEXT                     0x00000008
#define FR_REPLACE                      0x00000010
#define FR_REPLACEALL                   0x00000020
#define FR_DIALOGTERM                   0x00000040
#define FR_SHOWHELP                     0x00000080
#define FR_ENABLEHOOK                   0x00000100
#define FR_ENABLETEMPLATE               0x00000200
#define FR_NOUPDOWN                     0x00000400
#define FR_NOMATCHCASE                  0x00000800
#define FR_NOWHOLEWORD                  0x00001000
#define FR_ENABLETEMPLATEHANDLE         0x00002000
#define FR_HIDEUPDOWN                   0x00004000
#define FR_HIDEMATCHCASE                0x00008000
#define FR_HIDEWHOLEWORD                0x00010000


#pragma pack(1)

typedef struct 
{
	DWORD			lStructSize;
	HWND16			hwndOwner;          /* caller's window handle   */
	HDC16          	        hDC;                /* printer DC/IC or NULL    */
	SEGPTR                  lpLogFont;          /* ptr. to a LOGFONT struct */
	short			iPointSize;         /* 10 * size in points of selected font */
	DWORD			Flags WINE_PACKED;  /* enum. type flags         */
	COLORREF		rgbColors;          /* returned text color      */
	LPARAM	                lCustData;          /* data passed to hook fn.  */
        WNDPROC16               lpfnHook;
	SEGPTR			lpTemplateName;     /* custom template name     */
	HINSTANCE16		hInstance;          /* instance handle of.EXE that   */
							/* contains cust. dlg. template  */
	SEGPTR			lpszStyle WINE_PACKED;  /* return the style field here   */
							/* must be LF_FACESIZE or bigger */
	UINT16			nFontType;          	/* same value reported to the    */
						    	/* EnumFonts callback with the   */
							/* extra FONTTYPE_ bits added    */
	short			nSizeMin WINE_PACKED;   /* minimum pt size allowed & */
	short			nSizeMax WINE_PACKED;   /* max pt size allowed if    */
							/* CF_LIMITSIZE is used      */
} CHOOSEFONT, *LPCHOOSEFONT;

typedef struct
{
	UINT32  	lStructSize; 
	HWND32 		hwndOwner; 
	HDC32  		hDC; 
	LPLOGFONT32A    lpLogFont; 
	INT32		iPointSize; 
	UINT32		Flags; 
	COLORREF	rgbColors; 
	LPARAM		lCustData; 
	WNDPROC32 	lpfnHook; 
	LPCSTR		lpTemplateName; 
	HINSTANCE32	hInstance; 
	LPSTR		lpszStyle; 
	UINT16		nFontType; 
	UINT16	___MISSING_ALIGNMENT__; 
	INT32   	nSizeMin; 
	INT32		nSizeMax; 
} CHOOSEFONT32A, *PCHOOSEFONT32A;

#pragma pack(4)

#define CF_SCREENFONTS               0x00000001
#define CF_PRINTERFONTS              0x00000002
#define CF_BOTH                      (CF_SCREENFONTS | CF_PRINTERFONTS)
#define CF_SHOWHELP                  0x00000004L
#define CF_ENABLEHOOK                0x00000008L
#define CF_ENABLETEMPLATE            0x00000010L
#define CF_ENABLETEMPLATEHANDLE      0x00000020L
#define CF_INITTOLOGFONTSTRUCT       0x00000040L
#define CF_USESTYLE                  0x00000080L
#define CF_EFFECTS                   0x00000100L
#define CF_APPLY                     0x00000200L
#define CF_ANSIONLY                  0x00000400L
#define CF_NOVECTORFONTS             0x00000800L
#define CF_NOOEMFONTS                CF_NOVECTORFONTS
#define CF_NOSIMULATIONS             0x00001000L
#define CF_LIMITSIZE                 0x00002000L
#define CF_FIXEDPITCHONLY            0x00004000L
#define CF_WYSIWYG                   0x00008000L /* use with CF_SCREENFONTS & CF_PRINTERFONTS */
#define CF_FORCEFONTEXIST            0x00010000L
#define CF_SCALABLEONLY              0x00020000L
#define CF_TTONLY                    0x00040000L
#define CF_NOFACESEL                 0x00080000L
#define CF_NOSTYLESEL                0x00100000L
#define CF_NOSIZESEL                 0x00200000L

#define SIMULATED_FONTTYPE      0x8000
#define PRINTER_FONTTYPE        0x4000
#define SCREEN_FONTTYPE         0x2000
#define BOLD_FONTTYPE           0x0100
#define ITALIC_FONTTYPE         0x0200
#define REGULAR_FONTTYPE        0x0400

#define WM_CHOOSEFONT_GETLOGFONT        (WM_USER + 1)

#define LBSELCHSTRING  "commdlg_LBSelChangedNotify"
#define SHAREVISTRING  "commdlg_ShareViolation"
#define FILEOKSTRING   "commdlg_FileNameOK"
#define COLOROKSTRING  "commdlg_ColorOK"
#define SETRGBSTRING   "commdlg_SetRGBColor"
#define FINDMSGSTRING  "commdlg_FindReplace"
#define HELPMSGSTRING  "commdlg_help"

#define CD_LBSELNOITEMS -1
#define CD_LBSELCHANGE   0
#define CD_LBSELSUB      1
#define CD_LBSELADD      2

typedef struct
{
    DWORD            lStructSize;
    HWND16           hwndOwner;
    HGLOBAL16        hDevMode;
    HGLOBAL16        hDevNames;
    HDC16            hDC;
    DWORD            Flags;
    WORD             nFromPage;
    WORD             nToPage;
    WORD             nMinPage;
    WORD             nMaxPage;
    WORD             nCopies;
    HINSTANCE16      hInstance;
    LPARAM           lCustData;
    WNDPROC16        lpfnPrintHook;
    WNDPROC16        lpfnSetupHook;
    SEGPTR           lpPrintTemplateName;
    SEGPTR           lpSetupTemplateName;
    HGLOBAL16        hPrintTemplate;
    HGLOBAL16        hSetupTemplate;
} PRINTDLG16, *LPPRINTDLG16;

typedef UINT32 (CALLBACK *LPPRINTHOOKPROC) (HWND32, UINT32, WPARAM32, LPARAM);
typedef UINT32 (CALLBACK *LPSETUPHOOKPROC) (HWND32, UINT32, WPARAM32, LPARAM);

typedef struct
{
    DWORD            lStructSize;
    HWND32           hwndOwner;
    HGLOBAL32        hDevMode;
    HGLOBAL32        hDevNames;
    HDC32            hDC;
    DWORD            Flags;
    WORD             nFromPage;
    WORD             nToPage;
    WORD             nMinPage;
    WORD             nMaxPage;
    WORD             nCopies;
    HINSTANCE32      hInstance;
    LPARAM           lCustData;
    LPPRINTHOOKPROC  lpfnPrintHook;
    LPSETUPHOOKPROC  lpfnSetupHook;
    LPCSTR           lpPrintTemplateName;
    LPCSTR           lpSetupTemplateName;
    HGLOBAL32        hPrintTemplate;
    HGLOBAL32        hSetupTemplate;
} PRINTDLG32A, *LPPRINTDLG32A;

typedef struct
{
    DWORD            lStructSize;
    HWND32           hwndOwner;
    HGLOBAL32        hDevMode;
    HGLOBAL32        hDevNames;
    HDC32            hDC;
    DWORD            Flags;
    WORD             nFromPage;
    WORD             nToPage;
    WORD             nMinPage;
    WORD             nMaxPage;
    WORD             nCopies;
    HINSTANCE32      hInstance;
    LPARAM           lCustData;
    LPPRINTHOOKPROC  lpfnPrintHook;
    LPSETUPHOOKPROC  lpfnSetupHook;
    LPCWSTR          lpPrintTemplateName;
    LPCWSTR          lpSetupTemplateName;
    HGLOBAL32        hPrintTemplate;
    HGLOBAL32        hSetupTemplate;
} PRINTDLG32W, *LPPRINTDLG32W;

DECL_WINELIB_TYPE_AW(PRINTDLG);
DECL_WINELIB_TYPE_AW(LPPRINTDLG);

#define PD_ALLPAGES                  0x00000000
#define PD_SELECTION                 0x00000001
#define PD_PAGENUMS                  0x00000002
#define PD_NOSELECTION               0x00000004
#define PD_NOPAGENUMS                0x00000008
#define PD_COLLATE                   0x00000010
#define PD_PRINTTOFILE               0x00000020
#define PD_PRINTSETUP                0x00000040
#define PD_NOWARNING                 0x00000080
#define PD_RETURNDC                  0x00000100
#define PD_RETURNIC                  0x00000200
#define PD_RETURNDEFAULT             0x00000400
#define PD_SHOWHELP                  0x00000800
#define PD_ENABLEPRINTHOOK           0x00001000
#define PD_ENABLESETUPHOOK           0x00002000
#define PD_ENABLEPRINTTEMPLATE       0x00004000
#define PD_ENABLESETUPTEMPLATE       0x00008000
#define PD_ENABLEPRINTTEMPLATEHANDLE 0x00010000
#define PD_ENABLESETUPTEMPLATEHANDLE 0x00020000
#define PD_USEDEVMODECOPIES          0x00040000
#define PD_DISABLEPRINTTOFILE        0x00080000
#define PD_HIDEPRINTTOFILE           0x00100000

typedef struct {
	UINT16 	wDriverOffset;
	UINT16 	wDeviceOffset;
	UINT16 	wOutputOffset;
	UINT16 	wDefault;
	} DEVNAMES;
typedef DEVNAMES * LPDEVNAMES;

#define DN_DEFAULTPRN      0x0001


#define CDERR_DIALOGFAILURE   0xFFFF
#define CDERR_GENERALCODES     0x0000
#define CDERR_STRUCTSIZE       0x0001
#define CDERR_INITIALIZATION   0x0002
#define CDERR_NOTEMPLATE       0x0003
#define CDERR_NOHINSTANCE      0x0004
#define CDERR_LOADSTRFAILURE   0x0005
#define CDERR_FINDRESFAILURE   0x0006
#define CDERR_LOADRESFAILURE   0x0007
#define CDERR_LOCKRESFAILURE   0x0008
#define CDERR_MEMALLOCFAILURE  0x0009
#define CDERR_MEMLOCKFAILURE   0x000A
#define CDERR_NOHOOK           0x000B
#define CDERR_REGISTERMSGFAIL  0x000C

BOOL16  WINAPI ChooseColor(LPCHOOSECOLOR lpChCol);
DWORD   WINAPI CommDlgExtendedError(void);
HWND16  WINAPI FindText16( SEGPTR find);
HWND32  WINAPI FindText32A(LPFINDREPLACE32A lpFind);
HWND32  WINAPI FindText32W(LPFINDREPLACE32W lpFind);
#define FindText WINELIB_NAME_AW(FindText)
INT16   WINAPI GetFileTitle16(LPCSTR lpFile, LPSTR lpTitle, UINT16 cbBuf);
INT16   WINAPI GetFileTitle32A(LPCSTR lpFile, LPSTR lpTitle, UINT32 cbBuf);
INT16   WINAPI GetFileTitle32W(LPCWSTR lpFile, LPWSTR lpTitle, UINT32 cbBuf);
#define GetFileTitle WINELIB_NAME_AW(GetFileTitle)
BOOL16  WINAPI GetOpenFileName16(SEGPTR ofn);
BOOL32  WINAPI GetOpenFileName32A(LPOPENFILENAME32A ofn);
BOOL32  WINAPI GetOpenFileName32W(LPOPENFILENAME32W ofn);
#define GetOpenFileName WINELIB_NAME_AW(GetOpenFileName)
BOOL16  WINAPI GetSaveFileName16(SEGPTR ofn);
BOOL32  WINAPI GetSaveFileName32A(LPOPENFILENAME32A ofn);
BOOL32  WINAPI GetSaveFileName32W(LPOPENFILENAME32W ofn);
#define GetSaveFileName WINELIB_NAME_AW(GetSaveFileName)
BOOL16  WINAPI PrintDlg( SEGPTR print);
HWND16  WINAPI ReplaceText16( SEGPTR find);
HWND32  WINAPI ReplaceText32A( LPFINDREPLACE32A lpFind);
HWND32  WINAPI ReplaceText32W( LPFINDREPLACE32W lpFind);
#define ReplaceText WINELIB_NAME_AW(ReplaceText)
BOOL16  WINAPI ChooseFont(LPCHOOSEFONT lpChFont);
LRESULT WINAPI FileOpenDlgProc(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam, LPARAM lParam);
LRESULT WINAPI FileSaveDlgProc(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam, LPARAM lParam);
LRESULT WINAPI ColorDlgProc(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam, LPARAM lParam);
LRESULT WINAPI FindTextDlgProc16(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam, LPARAM lParam);
LRESULT WINAPI FindTextDlgProc32A(HWND32 hWnd, UINT32 wMsg, WPARAM32 wParam, LPARAM lParam);
LRESULT WINAPI FindTextDlgProc32W(HWND32 hWnd, UINT32 wMsg, WPARAM32 wParam, LPARAM lParam);
#define FindTextDlgProc WINELIB_NAME_AW(FindTextDlgProc)
LRESULT WINAPI ReplaceTextDlgProc16(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam, LPARAM lParam);
LRESULT WINAPI ReplaceTextDlgProc32A(HWND32 hWnd, UINT32 wMsg, WPARAM32 wParam, LPARAM lParam);
LRESULT WINAPI ReplaceTextDlgProc32W(HWND32 hWnd, UINT32 wMsg, WPARAM32 wParam, LPARAM lParam);
#define ReplaceTextProc WINELIB_NAME_AW(ReplaceTextDlgProc)
LRESULT WINAPI PrintDlgProc(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam, LPARAM lParam);
LRESULT WINAPI PrintSetupDlgProc(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam, LPARAM lParam);
LRESULT WINAPI FormatCharDlgProc(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam, LPARAM lParam);

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_COMMDLG_H */
