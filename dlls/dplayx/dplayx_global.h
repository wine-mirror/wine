
#ifndef __WINE_DPLAYX_GLOBAL
#define __WINE_DPLAYX_GLOBAL

#include "dplay.h"

BOOL DPLAYX_ConstructData(void);
BOOL DPLAYX_DestructData(void);

HRESULT DPLAYX_GetConnectionSettingsA ( DWORD dwAppID, 
                                        LPVOID lpData, 
                                        LPDWORD lpdwDataSize );
HRESULT DPLAYX_GetConnectionSettingsW ( DWORD dwAppID, 
                                        LPVOID lpData,
                                        LPDWORD lpdwDataSize );

HRESULT DPLAYX_SetConnectionSettingsA ( DWORD dwFlags, 
                                        DWORD dwAppID, 
                                        LPDPLCONNECTION lpConn );
HRESULT DPLAYX_SetConnectionSettingsW ( DWORD dwFlags, 
                                        DWORD dwAppID, 
                                        LPDPLCONNECTION lpConn );

BOOL DPLAYX_CreateLobbyApplication( DWORD dwAppID );
BOOL DPLAYX_DestroyLobbyApplication( DWORD dwAppID );

BOOL DPLAYX_WaitForConnectionSettings( BOOL bWait );
BOOL DPLAYX_AnyLobbiesWaitingForConnSettings(void);

BOOL DPLAYX_SetLobbyHandles( DWORD dwAppID, 
                             HANDLE hStart, HANDLE hDeath, HANDLE hConnRead );
BOOL DPLAYX_GetThisLobbyHandles( LPHANDLE lphStart, 
                                 LPHANDLE lphDeath,
                                 LPHANDLE lphConnRead, BOOL bClearSetHandles );

LPDPSESSIONDESC2 DPLAYX_CopyAndAllocateLocalSession( UINT* index );
BOOL DPLAYX_CopyLocalSession( UINT* index, LPDPSESSIONDESC2 lpsd );
void DPLAYX_SetLocalSession( LPCDPSESSIONDESC2 lpsd );

BOOL DPLAYX_SetLobbyMsgThreadId( DWORD dwAppId, DWORD dwThreadId );

/* FIXME: This should not be here */
LPVOID DPLAYX_PrivHeapAlloc( DWORD flags, DWORD size );
void DPLAYX_PrivHeapFree( LPVOID addr );

LPSTR DPLAYX_strdupA( DWORD flags, LPCSTR str );
LPWSTR DPLAYX_strdupW( DWORD flags, LPCWSTR str );
/* FIXME: End shared data alloc which should be local */


/* Convert a DP or DPL HRESULT code into a string for human consumption */
LPCSTR DPLAYX_HresultToString( HRESULT hr );

#endif /* __WINE_DPLAYX_GLOBAL */
