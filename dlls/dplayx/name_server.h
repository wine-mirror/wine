
#ifndef __WINE_DPLAYX_NAMESERVER
#define __WINE_DPLAYX_NAMESERVER

#include "dplay.h"

void NS_SetLocalComputerAsNameServer( LPCDPSESSIONDESC2 lpsd );
void NS_SendSessionRequestBroadcast( LPVOID lpNSInfo );

BOOL NS_InitializeSessionCache( LPVOID* lplpNSInfo );
void NS_DeleteSessionCache( LPVOID lpNSInfo );

void NS_ResetSessionEnumeration( LPVOID lpNSInfo );
LPDPSESSIONDESC2 NS_WalkSessions( LPVOID lpNSInfo );

#endif /* __WINE_DPLAYX_NAMESERVER */
