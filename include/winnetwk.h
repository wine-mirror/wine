#ifndef _WINNETWK_H_
#define _WINNETWK_H_

#include "windef.h"


typedef struct {
	DWORD	dwScope;
	DWORD	dwType;
	DWORD	dwDisplayType;
	DWORD	dwUsage;
	LPSTR	lpLocalName;
	LPSTR	lpRemoteName;
	LPSTR	lpComment ;
	LPSTR	lpProvider;
} NETRESOURCEA,*LPNETRESOURCEA;

typedef struct {
	DWORD	dwScope;
	DWORD	dwType;
	DWORD	dwDisplayType;
	DWORD	dwUsage;
	LPWSTR	lpLocalName;
	LPWSTR	lpRemoteName;
	LPWSTR	lpComment ;
	LPWSTR	lpProvider;
} NETRESOURCEW,*LPNETRESOURCEW;

DECL_WINELIB_TYPE_AW(NETRESOURCE)
DECL_WINELIB_TYPE_AW(LPNETRESOURCE)

typedef struct {
    DWORD cbStructure;       /* size of this structure in bytes */
    HWND hwndOwner;          /* owner window for the dialog */
    LPNETRESOURCEA lpConnRes;/* Requested Resource info    */
    DWORD dwFlags;           /* flags (see below) */
    DWORD dwDevNum;          /* number of devices connected to */
} CONNECTDLGSTRUCTA, *LPCONNECTDLGSTRUCTA;
typedef struct {
    DWORD cbStructure;       /* size of this structure in bytes */
    HWND hwndOwner;          /* owner window for the dialog */
    LPNETRESOURCEW lpConnRes;/* Requested Resource info    */
    DWORD dwFlags;           /* flags (see below) */
    DWORD dwDevNum;          /* number of devices connected to */
} CONNECTDLGSTRUCTW, *LPCONNECTDLGSTRUCTW;

DECL_WINELIB_TYPE_AW(CONNECTDLGSTRUCT)
DECL_WINELIB_TYPE_AW(LPCONNECTDLGSTRUCT)


/**/
#define CONNDLG_RO_PATH     0x00000001 /* Resource path should be read-only    */
#define CONNDLG_CONN_POINT  0x00000002 /* Netware -style movable connection point enabled */
#define CONNDLG_USE_MRU     0x00000004 /* Use MRU combobox  */
#define CONNDLG_HIDE_BOX    0x00000008 /* Hide persistent connect checkbox  */
#define CONNDLG_PERSIST     0x00000010 /* Force persistent connection */
#define CONNDLG_NOT_PERSIST 0x00000020 /* Force connection NOT persistent */


typedef struct {
	DWORD	cbStructure;
	DWORD	dwFlags;
	DWORD	dwSpeed;
	DWORD	dwDelay;
	DWORD	dwOptDataSize;
} NETCONNECTINFOSTRUCT,*LPNETCONNECTINFOSTRUCT;
  

UINT      WINAPI WNetAddConnection2A(LPNETRESOURCEA,LPCSTR,LPCSTR,DWORD);
UINT      WINAPI WNetAddConnection2W(LPNETRESOURCEW,LPCWSTR,LPCWSTR,DWORD);
#define     WNetAddConnection2 WINELIB_NAME_AW(WNetAddConnection2_)
UINT      WINAPI WNetAddConnection3A(HWND,LPNETRESOURCEA,LPCSTR,LPCSTR,DWORD);
UINT      WINAPI WNetAddConnection3W(HWND,LPNETRESOURCEW,LPCWSTR,LPCWSTR,DWORD);
#define     WNetAddConnection3 WINELIB_NAME_AW(WNetAddConnection3_)
UINT      WINAPI WNetConnectionDialog1(HWND,DWORD);
UINT      WINAPI WNetConnectionDialog1A(LPCONNECTDLGSTRUCTA);
UINT      WINAPI WNetConnectionDialog1W(LPCONNECTDLGSTRUCTW);
#define     WNetConnectionDialog1 WINELIB_NAME_AW(WNetConnectionDialog1_)
UINT      WINAPI MultinetGetErrorTextA(DWORD,DWORD,DWORD);
UINT      WINAPI MultinetGetErrorTextW(DWORD,DWORD,DWORD);
#define     MultinetGetErrorText WINELIB_NAME_AW(MultinetGetErrorText_)

#define RESOURCETYPE_ANY             0x00000000
#define RESOURCETYPE_DISK            0x00000001
#define RESOURCETYPE_PRINT           0x00000002

#define CONNECT_UPDATE_PROFILE       0x00000001
#define CONNECT_UPDATE_RECENT        0x00000002
#define CONNECT_TEMPORARY            0x00000004
#define CONNECT_INTERACTIVE          0x00000008
#define CONNECT_PROMPT               0x00000010
#define CONNECT_NEED_DRIVE           0x00000020

#endif /* _WINNETWK_H_ */
