
#ifndef __WINE_DPLAYX_GLOBAL
#define __WINE_DPLAYX_GLOBAL

#include "dplay.h"

void DPLAYX_ConstructData(void);
void DPLAYX_DestructData(void);

HRESULT DPLAYX_GetConnectionSettingsA ( DWORD dwAppID, LPVOID lpData, LPDWORD lpdwDataSize );
HRESULT DPLAYX_GetConnectionSettingsW ( DWORD dwAppID, LPVOID lpData, LPDWORD lpdwDataSize );

HRESULT DPLAYX_SetConnectionSettingsA ( DWORD dwFlags, DWORD dwAppID, LPDPLCONNECTION lpConn );
HRESULT DPLAYX_SetConnectionSettingsW ( DWORD dwFlags, DWORD dwAppID, LPDPLCONNECTION lpConn );

BOOL DPLAYX_CreateLobbyApplication( DWORD dwAppID, HANDLE hReceiveEvent );
BOOL DPLAYX_DestroyLobbyApplication( DWORD dwAppID );

/* Convert a DP or DPL HRESULT code into a string for human consumption */
LPCSTR DPLAYX_HresultToString( HRESULT hr );

#endif /* __WINE_DPLAYX_GLOBAL */
