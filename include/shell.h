/*
 * 				Shell Library definitions
 */
#ifndef __WINE_SHELL_H
#define __WINE_SHELL_H

#include "windows.h"
#include "winreg.h"
#include "imagelist.h"


/****************************************************************************
* shell 16
*/
extern void SHELL_LoadRegistry(void);
extern void SHELL_SaveRegistry(void);
extern void SHELL_Init(void);

/* global functions used from shell32 */
extern HINSTANCE32 SHELL_FindExecutable(LPCSTR,LPCSTR ,LPSTR);
extern HGLOBAL16 WINAPI InternalExtractIcon(HINSTANCE16,LPCSTR,UINT16,WORD);

/****************************************************************************
* shell 32
*/
/****************************************************************************
* common return codes 
*/
#define SHELL_ERROR_SUCCESS           0L
#define SHELL_ERROR_BADDB             1L
#define SHELL_ERROR_BADKEY            2L
#define SHELL_ERROR_CANTOPEN          3L
#define SHELL_ERROR_CANTREAD          4L
#define SHELL_ERROR_CANTWRITE         5L
#define SHELL_ERROR_OUTOFMEMORY       6L
#define SHELL_ERROR_INVALID_PARAMETER 7L
#define SHELL_ERROR_ACCESS_DENIED     8L

/****************************************************************************
* common shell file structures 
*/
#define FO_MOVE           0x0001
#define FO_COPY           0x0002
#define FO_DELETE         0x0003
#define FO_RENAME         0x0004

#define FOF_MULTIDESTFILES         0x0001
#define FOF_CONFIRMMOUSE           0x0002
#define FOF_SILENT                 0x0004  
#define FOF_RENAMEONCOLLISION      0x0008
#define FOF_NOCONFIRMATION         0x0010  
#define FOF_WANTMAPPINGHANDLE      0x0020  
#define FOF_ALLOWUNDO              0x0040
#define FOF_FILESONLY              0x0080  
#define FOF_SIMPLEPROGRESS         0x0100  
#define FOF_NOCONFIRMMKDIR         0x0200  
#define FOF_NOERRORUI              0x0400  

typedef WORD FILEOP_FLAGS;

#define PO_DELETE       0x0013  
#define PO_RENAME       0x0014  
#define PO_PORTCHANGE   0x0020  

typedef WORD PRINTEROP_FLAGS;

/******************************
* DRAG&DROP API
*/
typedef struct { 	   /* structure for dropped files */ 
	WORD		wSize;
	POINT16		ptMousePos;   
	BOOL16		fInNonClientArea;
	/* memory block with filenames follows */     
} DROPFILESTRUCT16, *LPDROPFILESTRUCT16; 

typedef struct { 	   /* structure for dropped files */ 
	DWORD		lSize;
	POINT32		ptMousePos;   
	BOOL32		fInNonClientArea;
        BOOL32          fWideChar;
	/* memory block with filenames follows */     
} DROPFILESTRUCT32, *LPDROPFILESTRUCT32; 

DECL_WINELIB_TYPE(DROPFILESTRUCT)
DECL_WINELIB_TYPE(LPDROPFILESTRUCT)

void        WINAPI DragAcceptFiles16(HWND16 hWnd, BOOL16 b);
void        WINAPI DragAcceptFiles32(HWND32 hWnd, BOOL32 b);
#define     DragAcceptFiles WINELIB_NAME(DragAcceptFiles)
UINT16      WINAPI DragQueryFile16(HDROP16 hDrop, WORD wFile, LPSTR lpszFile, WORD wLength);
UINT32      WINAPI DragQueryFile32A(HDROP32 hDrop, UINT32 lFile, LPSTR lpszFile, UINT32 lLength);
UINT32      WINAPI DragQueryFile32W(HDROP32 hDrop, UINT32 lFile, LPWSTR lpszFile, UINT32 lLength);
#define     DragQueryFile WINELIB_NAME_AW(DragQueryFile)
void        WINAPI DragFinish32(HDROP32 h);
void        WINAPI DragFinish16(HDROP16 h);
#define     DragFinish WINELIB_NAME(DragFinish)
BOOL32      WINAPI DragQueryPoint32(HDROP32 hDrop, POINT32 *p);
BOOL16      WINAPI DragQueryPoint16(HDROP16 hDrop, POINT16 *p);
#define     DragQueryPoint WINELIB_NAME(DragQueryPoint)


/****************************************************************************
* NOTIFYICONDATA 
*/
typedef struct _NOTIFYICONDATA {
	DWORD cbSize;
	HWND32 hWnd;
	UINT32 uID;
	UINT32 uFlags;
	UINT32 uCallbackMessage;
	HICON32 hIcon;
	CHAR szTip[64];
} NOTIFYICONDATA, *PNOTIFYICONDATA;

/****************************************************************************
* SHITEMID, ITEMIDLIST, PIDL API 
*/
#pragma pack(1)
typedef struct 
{ WORD	cb;	/* nr of bytes in this item */
  BYTE	abID[1];/* first byte in this item */
} SHITEMID,*LPSHITEMID;

typedef struct 
{ SHITEMID mkid; /* first itemid in list */
} ITEMIDLIST,*LPITEMIDLIST,*LPCITEMIDLIST;
#pragma pack(4)

DWORD WINAPI SHGetPathFromIDList32A (LPCITEMIDLIST pidl,LPSTR pszPath);
DWORD WINAPI SHGetPathFromIDList32W (LPCITEMIDLIST pidl,LPWSTR pszPath);
#define  SHGetPathFromIDList WINELIB_NAME_AW(SHGetPathFromIDList)

/****************************************************************************
* SHFILEINFO API 
*/
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

DECL_WINELIB_TYPE_AW(SHFILEINFO)

DWORD    WINAPI SHGetFileInfo32A(LPCSTR,DWORD,SHFILEINFO32A*,UINT32,UINT32);
DWORD    WINAPI SHGetFileInfo32W(LPCWSTR,DWORD,SHFILEINFO32W*,UINT32,UINT32);
#define  SHGetFileInfo WINELIB_NAME_AW(SHGetFileInfo)

/****************************************************************************
* SHFILEOPSTRUCT API 
*/
typedef struct _SHFILEOPSTRUCTA
{ HWND32          hwnd;
  UINT32          wFunc;
  LPCSTR          pFrom;
  LPCSTR          pTo;
  FILEOP_FLAGS    fFlags;
  BOOL32          fAnyOperationsAborted;
  LPVOID          hNameMappings;
  LPCSTR          lpszProgressTitle;
} SHFILEOPSTRUCT32A, *LPSHFILEOPSTRUCT32A;

typedef struct _SHFILEOPSTRUCTW
{ HWND32          hwnd;
  UINT32          wFunc;
  LPCWSTR         pFrom;
  LPCWSTR         pTo;
  FILEOP_FLAGS    fFlags;
  BOOL32          fAnyOperationsAborted;
  LPVOID          hNameMappings;
  LPCWSTR         lpszProgressTitle;
} SHFILEOPSTRUCT32W, *LPSHFILEOPSTRUCT32W;

#define  SHFILEOPSTRUCT WINELIB_NAME_AW(SHFILEOPSTRUCT)
#define  LPSHFILEOPSTRUCT WINELIB_NAME_AW(LPSHFILEOPSTRUCT)

DWORD WINAPI SHFileOperation32A (LPSHFILEOPSTRUCT32A lpFileOp);  
DWORD WINAPI SHFileOperation32W (LPSHFILEOPSTRUCT32W lpFileOp);
#define  SHFileOperation WINELIB_NAME_AW(SHFileOperation)

DWORD WINAPI SHFileOperation32(DWORD x);

/****************************************************************************
* APPBARDATA 
*/
typedef struct _AppBarData {
	DWORD	cbSize;
	HWND32	hWnd;
	UINT32	uCallbackMessage;
	UINT32	uEdge;
	RECT32	rc;
	LPARAM	lParam;
} APPBARDATA, *PAPPBARDATA;

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

/****************************************************************************
* SHChangeNotifyRegister API
*/
typedef struct
{ LPITEMIDLIST pidl;
  DWORD unknown;
} IDSTRUCT;

DWORD WINAPI SHChangeNotifyRegister(HWND32 hwnd,LONG events1,LONG events2,DWORD msg,int count,IDSTRUCT *idlist);
DWORD WINAPI SHChangeNotifyDeregister(LONG x1);

/****************************************************************************
* SHAddToRecentDocs API
*/
#define SHARD_PIDL      0x00000001L
#define SHARD_PATH      0x00000002L

DWORD WINAPI SHAddToRecentDocs(UINT32 uFlags, LPCVOID pv);

/****************************************************************************
* SHGetSpecialFolderLocation API
*/
HRESULT WINAPI SHGetSpecialFolderLocation(HWND32, INT32, LPITEMIDLIST *);
/****************************************************************************
*  string and path functions
*/
BOOL32 WINAPI PathIsRoot32A(LPCSTR x);
BOOL32 WINAPI PathIsRoot32W(LPCWSTR x);
#define  PathIsRoot WINELIB_NAME_AW(PathIsRoot)
BOOL32 WINAPI PathIsRoot32AW(LPCVOID x);

LPSTR  WINAPI PathAddBackslash32A(LPSTR path);	
LPWSTR WINAPI PathAddBackslash32W(LPWSTR path);	
#define  PathAddBackslash WINELIB_NAME_AW(PathAddBackslash)
LPVOID  WINAPI PathAddBackslash32AW(LPVOID path);	

LPSTR  WINAPI PathQuoteSpaces32A(LPCSTR path);	
LPWSTR WINAPI PathQuoteSpaces32W(LPCWSTR path);	
#define  PathQuoteSpaces WINELIB_NAME_AW(PathQuoteSpaces)
LPVOID  WINAPI PathQuoteSpaces32AW(LPCVOID path);	

LPSTR  WINAPI PathCombine32A(LPSTR szDest, LPCSTR lpszDir, LPCSTR lpszFile);
LPWSTR WINAPI PathCombine32W(LPWSTR szDest, LPCWSTR lpszDir, LPCWSTR lpszFile);
#define  PathCombine WINELIB_NAME_AW(PathCombine)
LPVOID WINAPI PathCombine32AW(LPVOID szDest, LPCVOID lpszDir, LPCVOID lpszFile);

LPCSTR WINAPI PathFindExtension32A(LPCSTR path);
LPCWSTR WINAPI PathFindExtension32W(LPCWSTR path);
#define  PathFindExtension WINELIB_NAME_AW(PathFindExtension)
LPCVOID WINAPI PathFindExtension32AW(LPCVOID path); 

LPCSTR WINAPI PathGetExtension32A(LPCSTR path, DWORD y, DWORD x);
LPCWSTR WINAPI PathGetExtension32W(LPCWSTR path, DWORD y, DWORD x);
#define  PathGetExtension WINELIB_NAME_AW(PathGetExtension)
LPCVOID WINAPI PathGetExtension32AW(LPCVOID path, DWORD y, DWORD x); 

LPCSTR WINAPI PathFindFilename32A(LPCSTR path);
LPCWSTR WINAPI PathFindFilename32W(LPCWSTR path);
#define  PathFindFilename WINELIB_NAME_AW(PathFindFilename)
LPCVOID WINAPI PathFindFilename32AW(LPCVOID path); 

BOOL32 WINAPI PathMatchSpec32A(LPCSTR x, LPCSTR y);
BOOL32 WINAPI PathMatchSpec32W(LPCWSTR x, LPCWSTR y);
#define  PathMatchSpec WINELIB_NAME_AW(PathMatchSpec)
BOOL32 WINAPI PathMatchSpec32AW(LPVOID x, LPVOID y);

LPSTR WINAPI PathRemoveBlanks32A(LPSTR str);
LPWSTR WINAPI PathRemoveBlanks32W(LPWSTR str);
#define  PathRemoveBlanks WINELIB_NAME_AW(PathRemoveBlanks)
LPVOID WINAPI PathRemoveBlanks32AW(LPVOID str);

BOOL32 WINAPI PathIsRelative32A(LPCSTR str);
BOOL32 WINAPI PathIsRelative32W(LPCWSTR str);
#define  PathIsRelative WINELIB_NAME_AW(PathIsRelative)
BOOL32 WINAPI PathIsRelative32AW(LPCVOID str);

BOOL32 WINAPI PathIsUNC32A(LPCSTR str);
BOOL32 WINAPI PathIsUNC32W(LPCWSTR str);
#define  PathIsUNC WINELIB_NAME_AW(PathIsUNC)
BOOL32 WINAPI PathIsUNC32AW(LPCVOID str);

BOOL32 WINAPI PathFindOnPath32A(LPSTR sFile, LPCSTR sOtherDirs);
BOOL32 WINAPI PathFindOnPath32W(LPWSTR sFile, LPCWSTR sOtherDirs);
#define PathFindOnPath WINELIB_NAME_AW(PathFindOnPath)
BOOL32 WINAPI PathFindOnPath32AW(LPVOID sFile, LPCVOID sOtherDirs);

LPSTR WINAPI StrFormatByteSize32A ( DWORD dw, LPSTR pszBuf, UINT32 cchBuf );
LPWSTR WINAPI StrFormatByteSize32W ( DWORD dw, LPWSTR pszBuf, UINT32 cchBuf );
#define  StrFormatByteSize WINELIB_NAME_AW(StrFormatByteSize)

/****************************************************************************
*  other functions
*/
HICON16 WINAPI ExtractIconEx16 ( LPCSTR, INT16, HICON16 *, HICON16 *, UINT16 );
HICON32 WINAPI ExtractIconEx32A( LPCSTR, INT32, HICON32 *, HICON32 *, UINT32 );
HICON32 WINAPI ExtractIconEx32W( LPCWSTR, INT32, HICON32 *, HICON32 *, UINT32 );
#define  ExtractIconEx WINELIB_NAME_AW(ExtractIconEx)
HICON32 WINAPI ExtractIconEx32AW(LPCVOID, INT32, HICON32 *, HICON32 *, UINT32 );

LPVOID WINAPI SHAlloc(DWORD len);
DWORD WINAPI SHFree(LPVOID x);

#define SE_ERR_SHARE            26
#define SE_ERR_ASSOCINCOMPLETE  27
#define SE_ERR_DDETIMEOUT       28
#define SE_ERR_DDEFAIL          29
#define SE_ERR_DDEBUSY          30
#define SE_ERR_NOASSOC          31

#define	CSIDL_DESKTOP		0x0000
#define	CSIDL_PROGRAMS		0x0002
#define	CSIDL_CONTROLS		0x0003
#define	CSIDL_PRINTERS		0x0004
#define	CSIDL_PERSONAL		0x0005
#define	CSIDL_FAVORITES		0x0006
#define	CSIDL_STARTUP		0x0007
#define	CSIDL_RECENT		0x0008
#define	CSIDL_SENDTO		0x0009
#define	CSIDL_BITBUCKET		0x000a
#define	CSIDL_STARTMENU		0x000b
#define	CSIDL_DESKTOPDIRECTORY	0x0010
#define	CSIDL_DRIVES		0x0011
#define	CSIDL_NETWORK		0x0012
#define	CSIDL_NETHOOD		0x0013
#define	CSIDL_FONTS		0x0014
#define	CSIDL_TEMPLATES		0x0015
#define CSIDL_COMMON_STARTMENU	0x0016
#define CSIDL_COMMON_PROGRAMS	0X0017
#define CSIDL_COMMON_STARTUP	0x0018
#define CSIDL_COMMON_DESKTOPDIRECTORY	0x0019
#define CSIDL_APPDATA		0x001a
#define CSIDL_PRINTHOOD		0x001b


#endif  /* __WINE_SHELL_H */
