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
#define PD32_PRINT_ALL_X_PAGES  7001
#define PD32_INVALID_PAGE_RANGE 7003

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
extern HRESULT (WINAPI *COMDLG32_StrRetToBufW)(LPSTRRET,LPITEMIDLIST,LPVOID,DWORD);
extern HRESULT (WINAPI *COMDLG32_StrRetToBufA)(LPSTRRET,LPITEMIDLIST,LPVOID,DWORD);

/* PATH */
extern BOOL (WINAPI *COMDLG32_PathIsRootA)(LPCSTR x);
extern LPCSTR (WINAPI *COMDLG32_PathFindFilenameA)(LPCSTR path);
extern DWORD (WINAPI *COMDLG32_PathRemoveFileSpecA)(LPSTR fn);
extern BOOL (WINAPI *COMDLG32_PathMatchSpecW)(LPCWSTR x, LPCWSTR y);
extern LPSTR (WINAPI *COMDLG32_PathAddBackslashA)(LPSTR path);
#endif

