/*
 *  Common Dialog Boxes interface (32 bit)
 *
 * Copyright 1998 Bertho A. Stultiens
 */

#ifndef _WINE_DLL_CDLG_H
#define _WINE_DLL_CDLG_H

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

/* DPA */
extern HDPA	(WINAPI* COMDLG32_DPA_Create) (INT);  
extern LPVOID	(WINAPI* COMDLG32_DPA_GetPtr) (const HDPA, INT);   
extern LPVOID	(WINAPI* COMDLG32_DPA_DeleteAllPtrs) (const HDPA hdpa);
extern LPVOID	(WINAPI* COMDLG32_DPA_DeletePtr) (const HDPA hdpa, INT i);
extern INT	(WINAPI* COMDLG32_DPA_InsertPtr) (const HDPA, INT, LPVOID); 
extern BOOL	(WINAPI* COMDLG32_DPA_Destroy) (const HDPA); 

/* IMAGELIST */
extern HICON	(WINAPI* COMDLG32_ImageList_GetIcon) (HIMAGELIST, INT, UINT);
extern HIMAGELIST (WINAPI *COMDLG32_ImageList_LoadImageA) (HINSTANCE, LPCSTR, INT, INT, COLORREF, UINT, UINT);
extern BOOL	(WINAPI* COMDLG32_ImageList_Draw) (HIMAGELIST himl, int i, HDC hdcDest, int x, int y, UINT fStyle);
extern BOOL	(WINAPI* COMDLG32_ImageList_Destroy) (HIMAGELIST himl);

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
extern DWORD	(WINAPI *COMDLG32_SHGetFileInfoA)(LPCSTR,DWORD,SHFILEINFOA*,UINT,UINT);
extern DWORD (WINAPI *COMDLG32_SHFree)(LPVOID);

/* PATH */
extern BOOL (WINAPI *COMDLG32_PathIsRootA)(LPCSTR x);
extern LPCSTR (WINAPI *COMDLG32_PathFindFilenameA)(LPCSTR path);
extern DWORD (WINAPI *COMDLG32_PathRemoveFileSpecA)(LPSTR fn);
extern BOOL (WINAPI *COMDLG32_PathMatchSpecW)(LPCWSTR x, LPCWSTR y);
extern LPSTR (WINAPI *COMDLG32_PathAddBackslashA)(LPSTR path);
#endif

