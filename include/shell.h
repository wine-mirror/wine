/*
 * 				Shell Library definitions
 */
#ifndef __WINE_SHELL_H
#define __WINE_SHELL_H

#include "wintypes.h"
#include "windef.h"


/****************************************************************************
* shell 16
*/
extern void SHELL_LoadRegistry(void);
extern void SHELL_SaveRegistry(void);
extern void SHELL_Init(void);

/* global functions used from shell32 */
extern HINSTANCE SHELL_FindExecutable(LPCSTR,LPCSTR ,LPSTR);
extern HGLOBAL16 WINAPI InternalExtractIcon16(HINSTANCE16,LPCSTR,UINT16,WORD);

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
	POINT		ptMousePos;   
	BOOL		fInNonClientArea;
        BOOL          fWideChar;
	/* memory block with filenames follows */     
} DROPFILESTRUCT, *LPDROPFILESTRUCT; 


void        WINAPI DragAcceptFiles16(HWND16 hWnd, BOOL16 b);
void        WINAPI DragAcceptFiles(HWND hWnd, BOOL b);
UINT16      WINAPI DragQueryFile16(HDROP16 hDrop, WORD wFile, LPSTR lpszFile, WORD wLength);
UINT      WINAPI DragQueryFileA(HDROP hDrop, UINT lFile, LPSTR lpszFile, UINT lLength);
UINT      WINAPI DragQueryFileW(HDROP hDrop, UINT lFile, LPWSTR lpszFile, UINT lLength);
#define     DragQueryFile WINELIB_NAME_AW(DragQueryFile)
void        WINAPI DragFinish(HDROP h);
void        WINAPI DragFinish16(HDROP16 h);
BOOL      WINAPI DragQueryPoint(HDROP hDrop, POINT *p);
BOOL16      WINAPI DragQueryPoint16(HDROP16 hDrop, POINT16 *p);


/****************************************************************************
* NOTIFYICONDATA 
*/
typedef struct _NOTIFYICONDATA {
	DWORD cbSize;
	HWND hWnd;
	UINT uID;
	UINT uFlags;
	UINT uCallbackMessage;
	HICON hIcon;
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

DWORD WINAPI SHGetPathFromIDListA (LPCITEMIDLIST pidl,LPSTR pszPath);
DWORD WINAPI SHGetPathFromIDListW (LPCITEMIDLIST pidl,LPWSTR pszPath);
#define  SHGetPathFromIDList WINELIB_NAME_AW(SHGetPathFromIDList)

/****************************************************************************
* SHFILEINFO API 
*/
typedef struct tagSHFILEINFOA {
	HICON	hIcon;			/* icon */
	int	iIcon;			/* icon index */
	DWORD	dwAttributes;		/* SFGAO_ flags */
	CHAR	szDisplayName[MAX_PATH];/* display name (or path) */
	CHAR	szTypeName[80];		/* type name */
} SHFILEINFOA;

typedef struct tagSHFILEINFOW {
	HICON	hIcon;			/* icon */
	int	iIcon;			/* icon index */
	DWORD	dwAttributes;		/* SFGAO_ flags */
	WCHAR	szDisplayName[MAX_PATH];/* display name (or path) */
	WCHAR	szTypeName[80];		/* type name */
} SHFILEINFOW;

DECL_WINELIB_TYPE_AW(SHFILEINFO)

DWORD    WINAPI SHGetFileInfoA(LPCSTR,DWORD,SHFILEINFOA*,UINT,UINT);
DWORD    WINAPI SHGetFileInfoW(LPCWSTR,DWORD,SHFILEINFOW*,UINT,UINT);
#define  SHGetFileInfo WINELIB_NAME_AW(SHGetFileInfo)

/****************************************************************************
* SHFILEOPSTRUCT API 
*/
typedef struct _SHFILEOPSTRUCTA
{ HWND          hwnd;
  UINT          wFunc;
  LPCSTR          pFrom;
  LPCSTR          pTo;
  FILEOP_FLAGS    fFlags;
  BOOL          fAnyOperationsAborted;
  LPVOID          hNameMappings;
  LPCSTR          lpszProgressTitle;
} SHFILEOPSTRUCTA, *LPSHFILEOPSTRUCTA;

typedef struct _SHFILEOPSTRUCTW
{ HWND          hwnd;
  UINT          wFunc;
  LPCWSTR         pFrom;
  LPCWSTR         pTo;
  FILEOP_FLAGS    fFlags;
  BOOL          fAnyOperationsAborted;
  LPVOID          hNameMappings;
  LPCWSTR         lpszProgressTitle;
} SHFILEOPSTRUCTW, *LPSHFILEOPSTRUCTW;

#define  SHFILEOPSTRUCT WINELIB_NAME_AW(SHFILEOPSTRUCT)
#define  LPSHFILEOPSTRUCT WINELIB_NAME_AW(LPSHFILEOPSTRUCT)

DWORD WINAPI SHFileOperationA (LPSHFILEOPSTRUCTA lpFileOp);  
DWORD WINAPI SHFileOperationW (LPSHFILEOPSTRUCTW lpFileOp);
#define  SHFileOperation WINELIB_NAME_AW(SHFileOperation)

DWORD WINAPI SHFileOperationAW(DWORD x);

/****************************************************************************
* APPBARDATA 
*/
typedef struct _AppBarData {
	DWORD	cbSize;
	HWND	hWnd;
	UINT	uCallbackMessage;
	UINT	uEdge;
	RECT	rc;
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

DWORD WINAPI SHChangeNotifyRegister(HWND hwnd,LONG events1,LONG events2,DWORD msg,int count,IDSTRUCT *idlist);
DWORD WINAPI SHChangeNotifyDeregister(LONG x1);

/****************************************************************************
* SHAddToRecentDocs API
*/
#define SHARD_PIDL      0x00000001L
#define SHARD_PATH      0x00000002L

DWORD WINAPI SHAddToRecentDocs(UINT uFlags, LPCVOID pv);

/****************************************************************************
* SHGetSpecialFolderLocation API
*/
HRESULT WINAPI SHGetSpecialFolderLocation(HWND, INT, LPITEMIDLIST *);
/****************************************************************************
*  string and path functions
*/
BOOL WINAPI PathIsRootA(LPCSTR x);
BOOL WINAPI PathIsRootW(LPCWSTR x);
#define  PathIsRoot WINELIB_NAME_AW(PathIsRoot)
BOOL WINAPI PathIsRootAW(LPCVOID x);

LPSTR  WINAPI PathAddBackslashA(LPSTR path);	
LPWSTR WINAPI PathAddBackslashW(LPWSTR path);	
#define  PathAddBackslash WINELIB_NAME_AW(PathAddBackslash)
LPVOID  WINAPI PathAddBackslashAW(LPVOID path);	

BOOL  WINAPI PathQualifyA(LPCSTR path);	
BOOL WINAPI PathQualifyW(LPCWSTR path);	
#define  PathQualify WINELIB_NAME_AW(PathQualify)
BOOL  WINAPI PathQualifyAW(LPCVOID path);	

LPSTR  WINAPI PathQuoteSpacesA(LPCSTR path);	
LPWSTR WINAPI PathQuoteSpacesW(LPCWSTR path);	
#define  PathQuoteSpaces WINELIB_NAME_AW(PathQuoteSpaces)
LPVOID  WINAPI PathQuoteSpacesAW(LPCVOID path);	

LPSTR  WINAPI PathCombineA(LPSTR szDest, LPCSTR lpszDir, LPCSTR lpszFile);
LPWSTR WINAPI PathCombineW(LPWSTR szDest, LPCWSTR lpszDir, LPCWSTR lpszFile);
#define  PathCombine WINELIB_NAME_AW(PathCombine)
LPVOID WINAPI PathCombineAW(LPVOID szDest, LPCVOID lpszDir, LPCVOID lpszFile);

LPCSTR WINAPI PathFindExtensionA(LPCSTR path);
LPCWSTR WINAPI PathFindExtensionW(LPCWSTR path);
#define  PathFindExtension WINELIB_NAME_AW(PathFindExtension)
LPCVOID WINAPI PathFindExtensionAW(LPCVOID path); 

LPCSTR WINAPI PathGetExtensionA(LPCSTR path, DWORD y, DWORD x);
LPCWSTR WINAPI PathGetExtensionW(LPCWSTR path, DWORD y, DWORD x);
#define  PathGetExtension WINELIB_NAME_AW(PathGetExtension)
LPCVOID WINAPI PathGetExtensionAW(LPCVOID path, DWORD y, DWORD x); 

LPCSTR WINAPI PathFindFilenameA(LPCSTR path);
LPCWSTR WINAPI PathFindFilenameW(LPCWSTR path);
#define  PathFindFilename WINELIB_NAME_AW(PathFindFilename)
LPCVOID WINAPI PathFindFilenameAW(LPCVOID path); 

BOOL WINAPI PathMatchSpecA(LPCSTR x, LPCSTR y);
BOOL WINAPI PathMatchSpecW(LPCWSTR x, LPCWSTR y);
#define  PathMatchSpec WINELIB_NAME_AW(PathMatchSpec)
BOOL WINAPI PathMatchSpecAW(LPVOID x, LPVOID y);

LPSTR WINAPI PathRemoveBlanksA(LPSTR str);
LPWSTR WINAPI PathRemoveBlanksW(LPWSTR str);
#define  PathRemoveBlanks WINELIB_NAME_AW(PathRemoveBlanks)
LPVOID WINAPI PathRemoveBlanksAW(LPVOID str);

BOOL WINAPI PathIsRelativeA(LPCSTR str);
BOOL WINAPI PathIsRelativeW(LPCWSTR str);
#define  PathIsRelative WINELIB_NAME_AW(PathIsRelative)
BOOL WINAPI PathIsRelativeAW(LPCVOID str);

BOOL WINAPI PathIsUNCA(LPCSTR str);
BOOL WINAPI PathIsUNCW(LPCWSTR str);
#define  PathIsUNC WINELIB_NAME_AW(PathIsUNC)
BOOL WINAPI PathIsUNCAW(LPCVOID str);

BOOL WINAPI PathFindOnPathA(LPSTR sFile, LPCSTR sOtherDirs);
BOOL WINAPI PathFindOnPathW(LPWSTR sFile, LPCWSTR sOtherDirs);
#define PathFindOnPath WINELIB_NAME_AW(PathFindOnPath)
BOOL WINAPI PathFindOnPathAW(LPVOID sFile, LPCVOID sOtherDirs);

LPSTR WINAPI StrFormatByteSizeA ( DWORD dw, LPSTR pszBuf, UINT cchBuf );
LPWSTR WINAPI StrFormatByteSizeW ( DWORD dw, LPWSTR pszBuf, UINT cchBuf );
#define  StrFormatByteSize WINELIB_NAME_AW(StrFormatByteSize)

/****************************************************************************
*  other functions
*/
HICON16 WINAPI ExtractIconEx16 ( LPCSTR, INT16, HICON16 *, HICON16 *, UINT16 );
HICON WINAPI ExtractIconExA( LPCSTR, INT, HICON *, HICON *, UINT );
HICON WINAPI ExtractIconExW( LPCWSTR, INT, HICON *, HICON *, UINT );
#define  ExtractIconEx WINELIB_NAME_AW(ExtractIconEx)
HICON WINAPI ExtractIconExAW(LPCVOID, INT, HICON *, HICON *, UINT );

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
#define CSIDL_ALTSTARTUP	0x001d
#define CSIDL_COMMON_ALTSTARTUP	0x001e
#define CSIDL_COMMON_FAVORITES  0x001f
#define CSIDL_INTERNET_CACHE	0x0020
#define CSIDL_COOKIES		0x0021
#define CSIDL_HISTORY		0x0022

#endif  /* __WINE_SHELL_H */
