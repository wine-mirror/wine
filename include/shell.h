/*
 * 				Shell Library definitions
 */
#ifndef __WINE_SHELL_H
#define __WINE_SHELL_H

#include "windows.h"
#include "winreg.h"

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

extern void SHELL_LoadRegistry();
extern void SHELL_SaveRegistry();
extern void SHELL_Init();

#define SHELL_ERROR_SUCCESS           0L
#define SHELL_ERROR_BADDB             1L
#define SHELL_ERROR_BADKEY            2L
#define SHELL_ERROR_CANTOPEN          3L
#define SHELL_ERROR_CANTREAD          4L
#define SHELL_ERROR_CANTWRITE         5L
#define SHELL_ERROR_OUTOFMEMORY       6L
#define SHELL_ERROR_INVALID_PARAMETER 7L
#define SHELL_ERROR_ACCESS_DENIED     8L

typedef struct { 	   /* structure for dropped files */ 
	WORD		wSize;
	POINT16		ptMousePos;   
	BOOL16		fInNonClientArea;
	/* memory block with filenames follows */     
} DROPFILESTRUCT, *LPDROPFILESTRUCT; 

typedef struct tagSHFILEINFO32A {
	HICON32	hIcon;			/* icon */
	int	iIcon;			/* icon index */
	DWORD	dwAttributes;		/* SFGAO_ flags */
	CHAR	szDisplayName[MAX_PATH];/* display name (or path) */
	CHAR	szTypeName[80];		/* type name */
} SHFILEINFO32A;

typedef struct tagSHFILEINFO32W {
	HICON32	hIcon;			/* icon */
	int	iIcon;			/* icon index */
	DWORD	dwAttributes;		/* SFGAO_ flags */
	WCHAR	szDisplayName[MAX_PATH];/* display name (or path) */
	WCHAR	szTypeName[80];		/* type name */
} SHFILEINFO32W;

DECL_WINELIB_TYPE_AW(SHFILEINFO);

typedef struct _AppBarData {
	DWORD	cbSize;
	HWND32	hWnd;
	UINT32	uCallbackMessage;
	UINT32	uEdge;
	RECT32	rc;
	LPARAM	lParam;
} APPBARDATA, *PAPPBARDATA;

DECL_WINELIB_TYPE(APPBARDATA);

#define SHGFI_ICON              0x000000100     /* get icon */
#define SHGFI_DISPLAYNAME       0x000000200     /* get display name */
#define SHGFI_TYPENAME          0x000000400     /* get type name */
#define SHGFI_ATTRIBUTES        0x000000800     /* get attributes */
#define SHGFI_ICONLOCATION      0x000001000     /* get icon location */
#define SHGFI_EXETYPE           0x000002000     /* return exe type */
#define SHGFI_SYSICONINDEX      0x000004000     /* get system icon index */
#define SHGFI_LINKOVERLAY       0x000008000     /* put a link overlay on icon */
#define SHGFI_SELECTED          0x000010000     /* show icon in selected state */
#define SHGFI_LARGEICON         0x000000000     /* get large icon */
#define SHGFI_SMALLICON         0x000000001     /* get small icon */
#define SHGFI_OPENICON          0x000000002     /* get open icon */
#define SHGFI_SHELLICONSIZE     0x000000004     /* get shell size icon */
#define SHGFI_PIDL              0x000000008     /* pszPath is a pidl */
#define SHGFI_USEFILEATTRIBUTES 0x000000010     /* use passed dwFileAttribute */
DWORD    WINAPI SHGetFileInfo32A(LPCSTR,DWORD,SHFILEINFO32A*,UINT32,UINT32);
DWORD    WINAPI SHGetFileInfo32W(LPCWSTR,DWORD,SHFILEINFO32W*,UINT32,UINT32);
#define  SHGetFileInfo WINELIB_NAME_AW(SHGetFileInfo)

#define SE_ERR_SHARE            26
#define SE_ERR_ASSOCINCOMPLETE  27
#define SE_ERR_DDETIMEOUT       28
#define SE_ERR_DDEFAIL          29
#define SE_ERR_DDEBUSY          30
#define SE_ERR_NOASSOC          31

#endif  /* __WINE_SHELL_H */
