
#ifndef __WINE_DPLAYX_GLOBAL
#define __WINE_DPLAYX_GLOBAL

#include "dplay.h"

BOOL DPLAYX_ConstructData(void);
BOOL DPLAYX_DestructData(void);

HRESULT DPLAYX_GetConnectionSettingsA ( DWORD dwAppID, 
                                        LPVOID lpData, 
                                        LPDWORD lpdwDataSize,
                                        LPBOOL  lpbSendHaveReadMessage );
HRESULT DPLAYX_GetConnectionSettingsW ( DWORD dwAppID, 
                                        LPVOID lpData,
                                        LPDWORD lpdwDataSize, 
                                        LPBOOL  lpbSendHaveReadMessage );

HRESULT DPLAYX_SetConnectionSettingsA ( DWORD dwFlags, 
                                        DWORD dwAppID, 
                                        LPDPLCONNECTION lpConn );
HRESULT DPLAYX_SetConnectionSettingsW ( DWORD dwFlags, 
                                        DWORD dwAppID, 
                                        LPDPLCONNECTION lpConn );

BOOL DPLAYX_CreateLobbyApplication( DWORD dwAppID, HANDLE hReceiveEvent );
BOOL DPLAYX_DestroyLobbyApplication( DWORD dwAppID );

BOOL DPLAYX_WaitForConnectionSettings( BOOL bWait );
BOOL DPLAYX_AnyLobbiesWaitingForConnSettings(void);

LPDPSESSIONDESC2 DPLAYX_CopyAndAllocateLocalSession( UINT* index );
BOOL DPLAYX_CopyLocalSession( UINT* index, LPDPSESSIONDESC2 lpsd );
void DPLAYX_SetLocalSession( LPCDPSESSIONDESC2 lpsd );

/* Convert a DP or DPL HRESULT code into a string for human consumption */
LPCSTR DPLAYX_HresultToString( HRESULT hr );

#endif /* __WINE_DPLAYX_GLOBAL */
