#ifndef __WINE_RAS_H
#define __WINE_RAS_H

#include "windef.h"
#include "lmcons.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "pshpack4.h"

#define RAS_MaxCallbackNumber RAS_MaxPhoneNumber
#define RAS_MaxDeviceName     128
#define RAS_MaxDeviceType     16
#define RAS_MaxEntryName      256
#define RAS_MaxPhoneNumber    128

DECLARE_HANDLE(HRASCONN);

typedef struct tagRASCONNA {
    DWORD    dwSize;
    HRASCONN hRasConn;
    CHAR     szEntryName[ RAS_MaxEntryName + 1 ];
    CHAR     szDeviceType[ RAS_MaxDeviceType + 1 ];
    CHAR     szDeviceName[ RAS_MaxDeviceName + 1 ];
    CHAR     szPhonebook[ MAX_PATH ];
    DWORD    dwSubEntry;
} RASCONNA,*LPRASCONNA;

typedef struct tagRASCONNW {
    DWORD    dwSize;
    HRASCONN hRasConn;
    WCHAR    szEntryName[ RAS_MaxEntryName + 1 ];
    WCHAR    szDeviceType[ RAS_MaxDeviceType + 1 ];
    WCHAR    szDeviceName[ RAS_MaxDeviceName + 1 ];
    WCHAR    szPhonebook[ MAX_PATH ];
    DWORD    dwSubEntry;
} RASCONNW,*LPRASCONNW;

DECL_WINELIB_TYPE_AW(RASCONN)
DECL_WINELIB_TYPE_AW(LPRASCONN)

typedef struct tagRASENTRYNAMEA {
    DWORD dwSize;
    CHAR  szEntryName[ RAS_MaxEntryName + 1 ];
} RASENTRYNAMEA, *LPRASENTRYNAMEA;

typedef struct tagRASENTRYNAMEW {
    DWORD dwSize;
    WCHAR szEntryName[ RAS_MaxEntryName + 1 ];
} RASENTRYNAMEW, *LPRASENTRYNAMEW;

DECL_WINELIB_TYPE_AW(RASENTRYNAME)
DECL_WINELIB_TYPE_AW(LPRASENTRYNAME)

typedef struct tagRASDIALPARAMSA {
    DWORD dwSize;
    CHAR szEntryName[ RAS_MaxEntryName + 1 ];
    CHAR szPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    CHAR szCallbackNumber[ RAS_MaxCallbackNumber + 1 ];
    CHAR szUserName[ UNLEN + 1 ];
    CHAR szPassword[ PWLEN + 1 ];
    CHAR szDomain[ DNLEN + 1 ];
    DWORD dwSubEntry;
    DWORD dwCallbackId;
} RASDIALPARAMSA, *LPRASDIALPARAMSA;

typedef struct tagRASDIALPARAMSW {
    DWORD dwSize;
    WCHAR szEntryName[ RAS_MaxEntryName + 1 ];
    WCHAR szPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    WCHAR szCallbackNumber[ RAS_MaxCallbackNumber + 1 ];
    WCHAR szUserName[ UNLEN + 1 ];
    WCHAR szPassword[ PWLEN + 1 ];
    WCHAR szDomain[ DNLEN + 1 ];
    DWORD dwSubEntry;
    DWORD dwCallbackId;
} RASDIALPARAMSW, *LPRASDIALPARAMSW;

DECL_WINELIB_TYPE_AW(RASDIALPARAMS)
DECL_WINELIB_TYPE_AW(LPRASDIALPARAMS)


DWORD WINAPI RasEnumConnectionsA(LPRASCONNA,LPDWORD,LPDWORD);
DWORD WINAPI RasEnumConnectionsW(LPRASCONNW,LPDWORD,LPDWORD);
#define      RasEnumConnections WINELIB_NAME_AW(RasEnumConnections)
DWORD WINAPI RasEnumEntriesA(LPCSTR,LPCSTR,LPRASENTRYNAMEA,LPDWORD,LPDWORD); 
DWORD WINAPI RasEnumEntriesW(LPCWSTR,LPCWSTR,LPRASENTRYNAMEW,LPDWORD,LPDWORD); 
#define      RasEnumEntries WINELIB_NAME_AW(RasEnumEntries)
DWORD WINAPI RasGetEntryDialParamsA(LPCSTR,LPRASDIALPARAMSA,LPBOOL); 
DWORD WINAPI RasGetEntryDialParamsW(LPCWSTR,LPRASDIALPARAMSW,LPBOOL); 
#define      RasGetEntryDialParams WINELIB_NAME_AW(RasGetEntryDialParams)
DWORD WINAPI RasHangUpA(HRASCONN);
DWORD WINAPI RasHangUpW(HRASCONN);
#define      RasHangUp WINELIB_NAME_AW(RasHangUp)

#include "poppack.h"
#ifdef __cplusplus
}
#endif

#endif
