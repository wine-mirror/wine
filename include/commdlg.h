/* 
 * COMMDLG - Common Wine Dialog ... :-)
 */

#ifndef COMMDLG_H
#define COMMDLG_H

#define RT_CURSOR           MAKEINTRESOURCE(1)
#define RT_BITMAP           MAKEINTRESOURCE(2)
#define RT_ICON             MAKEINTRESOURCE(3)
#define RT_MENU             MAKEINTRESOURCE(4)
#define RT_DIALOG           MAKEINTRESOURCE(5)
#define RT_STRING           MAKEINTRESOURCE(6)
#define RT_FONTDIR          MAKEINTRESOURCE(7)
#define RT_FONT             MAKEINTRESOURCE(8)
#define RT_ACCELERATOR      MAKEINTRESOURCE(9)
#define RT_RCDATA           MAKEINTRESOURCE(10)

#define RT_GROUP_CURSOR     MAKEINTRESOURCE(12)
#define RT_GROUP_ICON       MAKEINTRESOURCE(14)

#ifndef HGLOBAL
#define HGLOBAL     HANDLE
#endif

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

#define OFN_SHAREFALLTHROUGH     2
#define OFN_SHARENOWARN          1
#define OFN_SHAREWARN            0


typedef struct {
	DWORD		lStructSize;
	HWND		hwndOwner;
	HINSTANCE	hInstance;
	LPCSTR		lpstrFilter;
	LPSTR		lpstrCustomFilter;
	DWORD		nMaxCustFilter;
	DWORD		nFilterIndex;
	LPSTR		lpstrFile;
	DWORD		nMaxFile;
	LPSTR		lpstrFileTitle;
	DWORD		nMaxFileTitle;
	LPCSTR 		lpstrInitialDir;
	LPCSTR 		lpstrTitle;
	DWORD		Flags;
	UINT		nFileOffset;
	UINT		nFileExtension;
	LPCSTR		lpstrDefExt;
	LPARAM 		lCustData;
	UINT 		(CALLBACK *lpfnHook)(HWND, UINT, WPARAM, LPARAM);
	LPCSTR 		lpTemplateName;
	}   OPENFILENAME;
typedef OPENFILENAME * LPOPENFILENAME;


typedef struct {
	DWORD		lStructSize;
	HWND		hwndOwner;
	HWND		hInstance;
	COLORREF	rgbResult;
	COLORREF	FAR* lpCustColors;
	DWORD 		Flags;
	LPARAM		lCustData;
	UINT		(CALLBACK* lpfnHook)(HWND, UINT, WPARAM, LPARAM);
	LPCSTR 		lpTemplateName;
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
	HWND		hwndOwner; 				/* handle to owner's window */
	HINSTANCE	hInstance; 				/* instance handle of.EXE that  */
										/*	contains cust. dlg. template */
	DWORD		Flags;                  /* one or more of the FR_?? */
	LPSTR		lpstrFindWhat;          /* ptr. to search string    */
	LPSTR		lpstrReplaceWith;       /* ptr. to replace string   */
	UINT		wFindWhatLen;           /* size of find buffer      */
	UINT 		wReplaceWithLen;        /* size of replace buffer   */
	LPARAM 		lCustData;              /* data passed to hook fn.  */
	UINT		(CALLBACK* lpfnHook)(HWND, UINT, WPARAM, LPARAM);
	LPCSTR 		lpTemplateName;         /* custom template name     */
	} FINDREPLACE;
typedef FINDREPLACE *LPFINDREPLACE;

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


typedef struct {
	DWORD			lStructSize;
	HWND			hwndOwner;          /* caller's window handle   */
	HDC				hDC;                /* printer DC/IC or NULL    */
	LOGFONT FAR*	lpLogFont;          /* ptr. to a LOGFONT struct */
	short			iPointSize;         /* 10 * size in points of selected font */
	DWORD			Flags;              /* enum. type flags         */
	COLORREF		rgbColors;          /* returned text color      */
	LPARAM			lCustData;          /* data passed to hook fn.  */
	UINT (CALLBACK* lpfnHook)(HWND, UINT, WPARAM, LPARAM);
	LPCSTR			lpTemplateName;     /* custom template name     */
	HINSTANCE		hInstance;          /* instance handle of.EXE that   */
										/* contains cust. dlg. template  */
	LPSTR			lpszStyle;          /* return the style field here   */
										/* must be LF_FACESIZE or bigger */
	UINT			nFontType;          /* same value reported to the    */
										/* EnumFonts callback with the   */
										/* extra FONTTYPE_ bits added    */
	short			nSizeMin;           /* minimum pt size allowed & */
	short			nSizeMax;           /* max pt size allowed if    */
										/* CF_LIMITSIZE is used      */
	} CHOOSEFONT;
typedef CHOOSEFONT *LPCHOOSEFONT;


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

typedef struct {
	DWORD 		lStructSize;
	HWND 		hwndOwner;
	HGLOBAL		hDevMode;
	HGLOBAL		hDevNames;
	HDC			hDC;
	DWORD 		Flags;
	UINT		nFromPage;
	UINT		nToPage;
	UINT		nMinPage;
	UINT		nMaxPage;
	UINT		nCopies;
	HINSTANCE 	hInstance;
	LPARAM 		lCustData;
	UINT		(CALLBACK* lpfnPrintHook)(HWND, UINT, WPARAM, LPARAM);
	UINT		(CALLBACK* lpfnSetupHook)(HWND, UINT, WPARAM, LPARAM);
	LPCSTR 		lpPrintTemplateName;
	LPCSTR 		lpSetupTemplateName;
	HGLOBAL 	hPrintTemplate;
	HGLOBAL 	hSetupTemplate;
	} PRINTDLG;
typedef PRINTDLG * LPPRINTDLG;


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
	UINT 	wDriverOffset;
	UINT 	wDeviceOffset;
	UINT 	wOutputOffset;
	UINT 	wDefault;
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

/************************************************************************
*                 COMMDLG Resources placed in Wine SYSRES.DLL		    *
************************************************************************/

#define OBM_FOLDER			32520
#define OBM_FOLDER2			32521
#define OBM_FLOPPY			32522
#define OBM_HDISK			32523
#define OBM_CDROM			32524

#define OPENFILEDLG				3
#define SAVEFILEDLG				4
#define PRINTDLG				5
#define PRINTSETUPDLG			6
#define FONTDLG					7
#define COLORDLG				8
#define FINDDLG					9
#define REPLACEDLG				10



#endif 		/* #ifdef COMMDLG_H */


