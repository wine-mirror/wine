/*
 *  Common Dialog Boxes interface (32 bit)
 *
 * Copyright 1998 Bertho A. Stultiens
 */

#ifndef _WINE_DLL_CDLG_H
#define _WINE_DLL_CDLG_H

#include "dlgs.h"

/*---------------- 16-bit ----------------*/
extern HINSTANCE16	COMMDLG_hInstance;
extern HINSTANCE	COMMDLG_hInstance32;

/*---------------- 32-bit ----------------*/

/* Common dialogs implementation globals */
#define COMDLG32_Atom	((ATOM)0xa000)	/* MS uses this one to identify props */

extern HINSTANCE	COMDLG32_hInstance;

void	COMDLG32_SetCommDlgExtendedError(DWORD err);
LPVOID	COMDLG32_AllocMem(int size);



/* Find/Replace local definitions */

#define FR_WINE_UNICODE		0x80000000
#define FR_WINE_REPLACE		0x40000000

typedef struct {
	FINDREPLACEA	fr;	/* Internally used structure */
        union {
		FINDREPLACEA	*fra;	/* Reference to the user supplied structure */
		FINDREPLACEW	*frw;
        } user_fr;
} COMDLG32_FR_Data;

#define PD32_PRINT_TITLE        7000

#define PD32_VALUE_UREADABLE                  1104
#define PD32_INVALID_PAGE_RANGE               1105
#define PD32_FROM_NOT_ABOVE_TO                1106
#define PD32_MARGINS_OVERLAP                  1107
#define PD32_NR_OF_COPIES_EMPTY               1108
#define PD32_TOO_LARGE_COPIES                 1109
#define PD32_PRINT_ERROR                      1110
#define PD32_NO_DEFAULT_PRINTER               1111
#define PD32_CANT_FIND_PRINTER                1112
#define PD32_OUT_OF_MEMORY                    1113
#define PD32_GENERIC_ERROR                    1114
#define PD32_DRIVER_UNKNOWN                   1115

#define PD32_PRINTER_STATUS_READY             1536
#define PD32_PRINTER_STATUS_PAUSED            1537
#define PD32_PRINTER_STATUS_ERROR             1538 
#define PD32_PRINTER_STATUS_PENDING_DELETION  1539
#define PD32_PRINTER_STATUS_PAPER_JAM         1540
#define PD32_PRINTER_STATUS_PAPER_OUT         1541
#define PD32_PRINTER_STATUS_MANUAL_FEED       1542
#define PD32_PRINTER_STATUS_PAPER_PROBLEM     1543
#define PD32_PRINTER_STATUS_OFFLINE           1544
#define PD32_PRINTER_STATUS_IO_ACTIVE         1545
#define PD32_PRINTER_STATUS_BUSY              1546
#define PD32_PRINTER_STATUS_PRINTING          1547
#define PD32_PRINTER_STATUS_OUTPUT_BIN_FULL   1548
#define PD32_PRINTER_STATUS_NOT_AVAILABLE     1549
#define PD32_PRINTER_STATUS_WAITING           1550
#define PD32_PRINTER_STATUS_PROCESSING        1551
#define PD32_PRINTER_STATUS_INITIALIZING      1552
#define PD32_PRINTER_STATUS_WARMING_UP        1553
#define PD32_PRINTER_STATUS_TONER_LOW         1554
#define PD32_PRINTER_STATUS_NO_TONER          1555
#define PD32_PRINTER_STATUS_PAGE_PUNT         1556
#define PD32_PRINTER_STATUS_USER_INTERVENTION 1557
#define PD32_PRINTER_STATUS_OUT_OF_MEMORY     1558
#define PD32_PRINTER_STATUS_DOOR_OPEN         1559
#define PD32_PRINTER_STATUS_SERVER_UNKNOWN    1560
#define PD32_PRINTER_STATUS_POWER_SAVE        1561

#define PD32_DEFAULT_PRINTER                  1582
#define PD32_NR_OF_DOCUMENTS_IN_QUEUE         1583
#define PD32_PRINT_ALL_X_PAGES                1584
#define PD32_MARGINS_IN_INCHES                1585
#define PD32_MARGINS_IN_MILIMETERS            1586
#define PD32_MILIMETERS                       1587

#include "commctrl.h"
#include "wine/undocshell.h"
#include "shellapi.h"

/* IMAGELIST */
extern BOOL	(WINAPI* COMDLG32_ImageList_Draw) (HIMAGELIST himl, int i, HDC hdcDest, int x, int y, UINT fStyle);

/* ITEMIDLIST */

extern LPITEMIDLIST (WINAPI *COMDLG32_PIDL_ILClone) (LPCITEMIDLIST);
extern LPITEMIDLIST (WINAPI *COMDLG32_PIDL_ILCombine)(LPCITEMIDLIST,LPCITEMIDLIST);
extern LPITEMIDLIST (WINAPI *COMDLG32_PIDL_ILGetNext)(LPITEMIDLIST);
extern BOOL (WINAPI *COMDLG32_PIDL_ILRemoveLastID)(LPCITEMIDLIST);
extern BOOL (WINAPI *COMDLG32_PIDL_ILIsEqual)(LPCITEMIDLIST, LPCITEMIDLIST);

/* SHELL */
extern BOOL (WINAPI *COMDLG32_SHGetPathFromIDListA) (LPCITEMIDLIST,LPSTR);
extern HRESULT (WINAPI *COMDLG32_SHGetSpecialFolderLocation)(HWND,INT,LPITEMIDLIST *);
extern DWORD (WINAPI *COMDLG32_SHGetDesktopFolder)(IShellFolder **);
extern DWORD (WINAPI *COMDLG32_SHGetFileInfoA)(LPCSTR,DWORD,SHFILEINFOA*,UINT,UINT);
extern LPVOID (WINAPI *COMDLG32_SHAlloc)(DWORD);
extern DWORD (WINAPI *COMDLG32_SHFree)(LPVOID);
extern HRESULT (WINAPI *COMDLG32_SHGetDataFromIDListA)(LPSHELLFOLDER psf, LPCITEMIDLIST pidl, int nFormat, LPVOID dest, int len);
extern BOOL (WINAPI *COMDLG32_SHGetFolderPathA)(HWND,int,HANDLE,DWORD,LPSTR);


/* PATH */
extern BOOL (WINAPI *COMDLG32_PathIsRootA)(LPCSTR x);
extern LPSTR (WINAPI *COMDLG32_PathFindFileNameA)(LPCSTR path);
extern DWORD (WINAPI *COMDLG32_PathRemoveFileSpecA)(LPSTR fn);
extern BOOL (WINAPI *COMDLG32_PathMatchSpecW)(LPCWSTR x, LPCWSTR y);
extern LPSTR (WINAPI *COMDLG32_PathAddBackslashA)(LPSTR path);
extern BOOL (WINAPI *COMDLG32_PathCanonicalizeA)(LPSTR pszBuf, LPCSTR pszPath);
extern int (WINAPI *COMDLG32_PathGetDriveNumberA)(LPCSTR lpszPath);
extern BOOL (WINAPI *COMDLG32_PathIsRelativeA) (LPCSTR lpszPath);
extern LPSTR (WINAPI *COMDLG32_PathFindNextComponentA)(LPCSTR pszPath);
extern LPWSTR (WINAPI *COMDLG32_PathAddBackslashW)(LPWSTR lpszPath);
extern LPVOID (WINAPI *COMDLG32_PathFindExtensionA)(LPCVOID lpszPath);
extern BOOL (WINAPI *COMDLG32_PathAddExtensionA)(LPSTR  pszPath,LPCSTR pszExtension);


extern BOOL  WINAPI GetFileDialog95A(LPOPENFILENAMEA ofn,UINT iDlgType);
extern BOOL  WINAPI GetFileDialog95W(LPOPENFILENAMEW ofn,UINT iDlgType);

/* 16 bit api */

#include "pshpack1.h"

typedef UINT16 (CALLBACK *LPOFNHOOKPROC16)(HWND16,UINT16,WPARAM16,LPARAM);

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
	LPOFNHOOKPROC16 lpfnHook;
	SEGPTR 		lpTemplateName;
}   OPENFILENAME16,*LPOPENFILENAME16;

typedef UINT16 (CALLBACK *LPCCHOOKPROC16) (HWND16, UINT16, WPARAM16, LPARAM);

typedef struct {
	DWORD		lStructSize;
	HWND16		hwndOwner;
	HWND16		hInstance;
	COLORREF	rgbResult;
	COLORREF       *lpCustColors;
	DWORD 		Flags;
	LPARAM		lCustData;
        LPCCHOOKPROC16  lpfnHook;
	SEGPTR 		lpTemplateName;
} CHOOSECOLOR16;
typedef CHOOSECOLOR16 *LPCHOOSECOLOR16;

typedef UINT16 (CALLBACK *LPFRHOOKPROC16)(HWND16,UINT16,WPARAM16,LPARAM);
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
        LPFRHOOKPROC16  lpfnHook;
	SEGPTR 		lpTemplateName;         /* custom template name     */
} FINDREPLACE16, *LPFINDREPLACE16;

typedef UINT16 (CALLBACK *LPCFHOOKPROC16)(HWND16,UINT16,WPARAM16,LPARAM);
typedef struct 
{
	DWORD			lStructSize;
	HWND16			hwndOwner;          /* caller's window handle   */
	HDC16          	        hDC;                /* printer DC/IC or NULL    */
	SEGPTR                  lpLogFont;          /* ptr. to a LOGFONT struct */
	short			iPointSize;         /* 10 * size in points of selected font */
	DWORD			Flags;  /* enum. type flags         */
	COLORREF		rgbColors;          /* returned text color      */
	LPARAM	                lCustData;          /* data passed to hook fn.  */
	LPCFHOOKPROC16          lpfnHook;
	SEGPTR			lpTemplateName;     /* custom template name     */
	HINSTANCE16		hInstance;          /* instance handle of.EXE that   */
							/* contains cust. dlg. template  */
	SEGPTR			lpszStyle;  /* return the style field here   */
							/* must be LF_FACESIZE or bigger */
	UINT16			nFontType;          	/* same value reported to the    */
						    	/* EnumFonts callback with the   */
							/* extra FONTTYPE_ bits added    */
	short			nSizeMin;   /* minimum pt size allowed & */
	short			nSizeMax;   /* max pt size allowed if    */
							/* CF_LIMITSIZE is used      */
} CHOOSEFONT16, *LPCHOOSEFONT16;


typedef UINT16 (CALLBACK *LPPRINTHOOKPROC16) (HWND16, UINT16, WPARAM16, LPARAM);
typedef UINT16 (CALLBACK *LPSETUPHOOKPROC16) (HWND16, UINT16, WPARAM16, LPARAM);
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
    LPPRINTHOOKPROC16 lpfnPrintHook;
    LPSETUPHOOKPROC16 lpfnSetupHook;
    SEGPTR           lpPrintTemplateName;
    SEGPTR           lpSetupTemplateName;
    HGLOBAL16        hPrintTemplate;
    HGLOBAL16        hSetupTemplate;
} PRINTDLG16, *LPPRINTDLG16;

BOOL16  WINAPI ChooseColor16(LPCHOOSECOLOR16 lpChCol);
HWND16  WINAPI FindText16( SEGPTR find);
INT16   WINAPI GetFileTitle16(LPCSTR lpFile, LPSTR lpTitle, UINT16 cbBuf);
BOOL16  WINAPI GetOpenFileName16(SEGPTR ofn);
BOOL16  WINAPI GetSaveFileName16(SEGPTR ofn);
BOOL16  WINAPI PrintDlg16( LPPRINTDLG16 print);
HWND16  WINAPI ReplaceText16( SEGPTR find);
BOOL16  WINAPI ChooseFont16(LPCHOOSEFONT16);

#include "poppack.h"

#endif /* _WINE_DLL_CDLG_H */

