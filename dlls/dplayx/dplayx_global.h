
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

/* This is a hack to ensure synchronization during application spawn */
#if !defined( WORKING_PROCESS_SUSPEND )

DWORD DPLAYX_AquireSemaphoreHack( void );
DWORD DPLAYX_ReleaseSemaphoreHack( void );

#endif


#endif /* __WINE_DPLAYX_GLOBAL */
