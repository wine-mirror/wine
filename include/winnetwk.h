#ifndef _WINNETWK_H_
#define _WINNETWK_H_

#include "wintypes.h"


typedef struct {
	DWORD	dwScope;
	DWORD	dwType;
	DWORD	dwDisplayType;
	DWORD	dwUsage;
	LPSTR	lpLocalName;
	LPSTR	lpRemoteName;
	LPSTR	lpComment ;
	LPSTR	lpProvider;
} NETRESOURCE32A,*LPNETRESOURCE32A;

typedef struct {
	DWORD	dwScope;
	DWORD	dwType;
	DWORD	dwDisplayType;
	DWORD	dwUsage;
	LPWSTR	lpLocalName;
	LPWSTR	lpRemoteName;
	LPWSTR	lpComment ;
	LPWSTR	lpProvider;
} NETRESOURCE32W,*LPNETRESOURCE32W;

DECL_WINELIB_TYPE_AW(NETRESOURCE)
DECL_WINELIB_TYPE_AW(LPNETRESOURCE)

typedef struct {
    DWORD cbStructure;       /* size of this structure in bytes */
    HWND32 hwndOwner;          /* owner window for the dialog */
    LPNETRESOURCE32A lpConnRes;/* Requested Resource info    */
    DWORD dwFlags;           /* flags (see below) */
    DWORD dwDevNum;          /* number of devices connected to */
} CONNECTDLGSTRUCT32A, *LPCONNECTDLGSTRUCT32A;
typedef struct {
    DWORD cbStructure;       /* size of this structure in bytes */
    HWND32 hwndOwner;          /* owner window for the dialog */
    LPNETRESOURCE32W lpConnRes;/* Requested Resource info    */
    DWORD dwFlags;           /* flags (see below) */
    DWORD dwDevNum;          /* number of devices connected to */
} CONNECTDLGSTRUCT32W, *LPCONNECTDLGSTRUCT32W;

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
  

UINT32      WINAPI WNetAddConnection2_32A(LPNETRESOURCE32A,LPCSTR,LPCSTR,DWORD);
UINT32      WINAPI WNetAddConnection2_32W(LPNETRESOURCE32W,LPCWSTR,LPCWSTR,DWORD);
#define     WNetAddConnection2 WINELIB_NAME_AW(WNetAddConnection2_)
UINT32      WINAPI WNetAddConnection3_32A(HWND32,LPNETRESOURCE32A,LPCSTR,LPCSTR,DWORD);
UINT32      WINAPI WNetAddConnection3_32W(HWND32,LPNETRESOURCE32W,LPCWSTR,LPCWSTR,DWORD);
#define     WNetAddConnection3 WINELIB_NAME_AW(WNetAddConnection3_)
UINT32      WINAPI WNetConnectionDialog1_32(HWND32,DWORD);
UINT32      WINAPI WNetConnectionDialog1_32A(LPCONNECTDLGSTRUCT32A);
UINT32      WINAPI WNetConnectionDialog1_32W(LPCONNECTDLGSTRUCT32W);
#define     WNetConnectionDialog1 WINELIB_NAME_AW(WNetConnectionDialog1_)
UINT32      WINAPI MultinetGetErrorText32A(DWORD,DWORD,DWORD);
UINT32      WINAPI MultinetGetErrorText32W(DWORD,DWORD,DWORD);
#define     MultinetGetErrorText WINELIB_NAME_AW(MultinetGetErrorText_)


#endif /* _WINNETWK_H_ */
