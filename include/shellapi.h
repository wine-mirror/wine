#ifndef _WINE_SHELLAPI_H
#define _WINE_SHELLAPI_H

#include "windef.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#include "pshpack1.h"

/******************************************
 * DragObject
 */

typedef struct _DRAGINFOA 
{	UINT uSize;
	POINT pt;
	BOOL fNC;
	LPSTR   lpFileList;
	DWORD grfKeyState;
} DRAGINFOA, * LPDRAGINFOA;

typedef struct _DRAGINFOW 
{	UINT uSize;
	POINT pt;
	BOOL fNC;
	LPWSTR  lpFileList;
	DWORD grfKeyState;
} DRAGINFOW, LPDRAGINFOW;

void	WINAPI DragAcceptFiles(HWND hWnd, BOOL b);
void	WINAPI DragAcceptFiles16(HWND16 hWnd, BOOL16 b);

UINT16	WINAPI DragQueryFile16(HDROP16 hDrop, WORD wFile, LPSTR lpszFile, WORD wLength);
UINT	WINAPI DragQueryFileA(HDROP hDrop, UINT lFile, LPSTR lpszFile, UINT lLength);
UINT	WINAPI DragQueryFileW(HDROP hDrop, UINT lFile, LPWSTR lpszFile, UINT lLength);
#define     DragQueryFile WINELIB_NAME_AW(DragQueryFile)

void	WINAPI DragFinish16(HDROP16 h);
void	WINAPI DragFinish(HDROP h);

BOOL16	WINAPI DragQueryPoint16(HDROP16 hDrop, POINT16 *p);
BOOL	WINAPI DragQueryPoint(HDROP hDrop, POINT *p);

#define NIF_MESSAGE             0x00000001
#define NIF_ICON                0x00000002
#define NIF_TIP                 0x00000004

#define NIM_ADD                 0x00000000
#define NIM_MODIFY              0x00000001
#define NIM_DELETE              0x00000002



/******************************************
 * Application Bar
 */
#define ABM_NEW			0x00000000
#define ABM_REMOVE		0x00000001
#define ABM_QUERYPOS		0x00000002
#define ABM_SETPOS		0x00000003
#define ABM_GETSTATE		0x00000004
#define ABM_GETTASKBARPOS	0x00000005
#define ABM_ACTIVATE		0x00000006
#define ABM_GETAUTOHIDEBAR	0x00000007
#define ABM_SETAUTOHIDEBAR	0x00000008
#define ABM_WINDOWPOSCHANGED	0x00000009

#define ABN_STATECHANGE		0x00000000
#define ABN_POSCHANGED		0x00000001
#define ABN_FULLSCREENAPP	0x00000002
#define ABN_WINDOWARRANGE	0x00000003

#define ABS_AUTOHIDE		0x00000001
#define ABS_ALWAYSONTOP		0x00000002

#define ABE_LEFT		0
#define ABE_TOP			1
#define ABE_RIGHT		2
#define ABE_BOTTOM		3

typedef struct _AppBarData 
{	DWORD	cbSize;
	HWND	hWnd;
	UINT	uCallbackMessage;
	UINT	uEdge;
	RECT	rc;
	LPARAM	lParam;
} APPBARDATA, *PAPPBARDATA;


/******************************************
 * SHGetFileInfo
 */

#define SHGFI_LARGEICON         0x000000000     /* get large icon */
#define SHGFI_SMALLICON         0x000000001     /* get small icon */
#define SHGFI_OPENICON          0x000000002     /* get open icon */
#define SHGFI_SHELLICONSIZE     0x000000004     /* get shell size icon */
#define SHGFI_PIDL              0x000000008     /* pszPath is a pidl */
#define SHGFI_USEFILEATTRIBUTES 0x000000010     /* use passed dwFileAttribute */
#define SHGFI_UNKNOWN1          0x000000020
#define SHGFI_UNKNOWN2          0x000000040
#define SHGFI_UNKNOWN3          0x000000080
#define SHGFI_ICON              0x000000100     /* get icon */
#define SHGFI_DISPLAYNAME       0x000000200     /* get display name */
#define SHGFI_TYPENAME          0x000000400     /* get type name */
#define SHGFI_ATTRIBUTES        0x000000800     /* get attributes */
#define SHGFI_ICONLOCATION      0x000001000     /* get icon location */
#define SHGFI_EXETYPE           0x000002000     /* return exe type */
#define SHGFI_SYSICONINDEX      0x000004000     /* get system icon index */
#define SHGFI_LINKOVERLAY       0x000008000     /* put a link overlay on icon */
#define SHGFI_SELECTED          0x000010000     /* show icon in selected state */
#define SHGFI_ATTR_SPECIFIED    0x000020000     /* get only specified attributes */

typedef struct tagSHFILEINFOA 
{	HICON	hIcon;			/* icon */
	int	iIcon;			/* icon index */
	DWORD	dwAttributes;		/* SFGAO_ flags */
	CHAR	szDisplayName[MAX_PATH];/* display name (or path) */
	CHAR	szTypeName[80];		/* type name */
} SHFILEINFOA;

typedef struct tagSHFILEINFOW 
{	HICON	hIcon;			/* icon */
	int	iIcon;			/* icon index */
	DWORD	dwAttributes;		/* SFGAO_ flags */
	WCHAR	szDisplayName[MAX_PATH];/* display name (or path) */
	WCHAR	szTypeName[80];		/* type name */
} SHFILEINFOW;

DECL_WINELIB_TYPE_AW(SHFILEINFO)

DWORD	WINAPI SHGetFileInfoA(LPCSTR,DWORD,SHFILEINFOA*,UINT,UINT);
DWORD	WINAPI SHGetFileInfoW(LPCWSTR,DWORD,SHFILEINFOW*,UINT,UINT);
#define  SHGetFileInfo WINELIB_NAME_AW(SHGetFileInfo)

/******************************************
 * SHSetFileInfo
 */

/******************************************
* SHFileOperation
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

DWORD	WINAPI SHFileOperationA (LPSHFILEOPSTRUCTA lpFileOp);  
DWORD	WINAPI SHFileOperationW (LPSHFILEOPSTRUCTW lpFileOp);
#define  SHFileOperation WINELIB_NAME_AW(SHFileOperation)

DWORD WINAPI SHFileOperationAW(DWORD x);

/******************************************
 * ShellExecute
 */
#define SE_ERR_SHARE            26
#define SE_ERR_ASSOCINCOMPLETE  27
#define SE_ERR_DDETIMEOUT       28
#define SE_ERR_DDEFAIL          29
#define SE_ERR_DDEBUSY          30
#define SE_ERR_NOASSOC          31

HINSTANCE16	WINAPI ShellExecute16(HWND16,LPCSTR,LPCSTR,LPCSTR,LPCSTR,INT16);
HINSTANCE	WINAPI ShellExecuteA(HWND,LPCSTR,LPCSTR,LPCSTR,LPCSTR,INT);
HINSTANCE	WINAPI ShellExecuteW(HWND,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,INT);
#define     ShellExecute WINELIB_NAME_AW(ShellExecute)

/******************************************
 * Tray Notification
 */
typedef struct _NOTIFYICONDATAA
{	DWORD cbSize;
	HWND hWnd;
	UINT uID;
	UINT uFlags;
	UINT uCallbackMessage;
	HICON hIcon;
	CHAR szTip[64];
} NOTIFYICONDATAA, *PNOTIFYICONDATAA;

typedef struct _NOTIFYICONDATAW
{	DWORD cbSize;
	HWND hWnd;
	UINT uID;
	UINT uFlags;
	UINT uCallbackMessage;
	HICON hIcon;
	WCHAR szTip[64];
} NOTIFYICONDATAW, *PNOTIFYICONDATAW;


/******************************************
 * Misc
 */

HICON16	WINAPI ExtractIcon16(HINSTANCE16,LPCSTR,UINT16);
HICON	WINAPI ExtractIconA(HINSTANCE,LPCSTR,UINT);
HICON	WINAPI ExtractIconW(HINSTANCE,LPCWSTR,UINT);
#define     ExtractIcon WINELIB_NAME_AW(ExtractIcon)

HICON16     WINAPI ExtractAssociatedIcon16(HINSTANCE16,LPSTR,LPWORD);
HICON     WINAPI ExtractAssociatedIconA(HINSTANCE,LPSTR,LPWORD);
HICON     WINAPI ExtractAssociatedIconW(HINSTANCE,LPWSTR,LPWORD);
#define     ExtractAssociatedIcon WINELIB_NAME_AW(ExtractAssociatedIcon)

HICON16 WINAPI ExtractIconEx16 ( LPCSTR, INT16, HICON16 *, HICON16 *, UINT16 );
HICON WINAPI ExtractIconExA( LPCSTR, INT, HICON *, HICON *, UINT );
HICON WINAPI ExtractIconExW( LPCWSTR, INT, HICON *, HICON *, UINT );
#define  ExtractIconEx WINELIB_NAME_AW(ExtractIconEx)
HICON WINAPI ExtractIconExAW(LPCVOID, INT, HICON *, HICON *, UINT );

HINSTANCE16 WINAPI FindExecutable16(LPCSTR,LPCSTR,LPSTR);
HINSTANCE WINAPI FindExecutableA(LPCSTR,LPCSTR,LPSTR);
HINSTANCE WINAPI FindExecutableW(LPCWSTR,LPCWSTR,LPWSTR);
#define     FindExecutable WINELIB_NAME_AW(FindExecutable)

BOOL16      WINAPI ShellAbout16(HWND16,LPCSTR,LPCSTR,HICON16);
BOOL      WINAPI ShellAboutA(HWND,LPCSTR,LPCSTR,HICON);
BOOL      WINAPI ShellAboutW(HWND,LPCWSTR,LPCWSTR,HICON);
#define     ShellAbout WINELIB_NAME_AW(ShellAbout)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#include "poppack.h"

#endif /* _WINE_SHELLAPI_H */
