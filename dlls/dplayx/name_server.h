
#ifndef __WINE_DPLAYX_NAMESERVER
#define __WINE_DPLAYX_NAMESERVER

#include "dplay.h"
#include "dplaysp.h"
#include "dplayx_messages.h"
#include "dplay_global.h"

void NS_SetLocalComputerAsNameServer( LPCDPSESSIONDESC2 lpsd );
void NS_SetRemoteComputerAsNameServer( LPVOID lpNSAddrHdr,
                                       DWORD dwHdrSize,
                                       LPDPMSG_ENUMSESSIONSREPLY lpMsg,
                                       LPVOID lpNSInfo );
LPVOID NS_GetNSAddr( LPVOID lpNSInfo );

void NS_ReplyToEnumSessionsRequest( LPVOID lpMsg, 
                                    LPDPSP_REPLYDATA lpReplyData,
                                    IDirectPlay2Impl* lpDP );

HRESULT NS_SendSessionRequestBroadcast( LPCGUID lpcGuid,
                                        DWORD dwFlags,
                                        LPSPINITDATA lpSpData );
                                        

BOOL NS_InitializeSessionCache( LPVOID* lplpNSInfo );
void NS_DeleteSessionCache( LPVOID lpNSInfo );
void NS_InvalidateSessionCache( LPVOID lpNSInfo );


void NS_ResetSessionEnumeration( LPVOID lpNSInfo );
LPDPSESSIONDESC2 NS_WalkSessions( LPVOID lpNSInfo );
void NS_PruneSessionCache( LPVOID lpNSInfo );

#endif /* __WINE_DPLAYX_NAMESERVER */
