/*
 * Win32 file change notification functions
 *
 * Copyright 1998 Ulrich Weigand
 */

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "winbase.h"
#include "winerror.h"
#include "process.h"
#include "thread.h"
#include "heap.h"
#include "server.h"
#include "debug.h"

/* The change notification object */
typedef struct
{
    K32OBJ       header;
} CHANGE_OBJECT;


/****************************************************************************
 *		FindFirstChangeNotification32A (KERNEL32.248)
 */
HANDLE WINAPI FindFirstChangeNotificationA( LPCSTR lpPathName,
                                                BOOL bWatchSubtree,
                                                DWORD dwNotifyFilter ) 
{
    CHANGE_OBJECT *change;
    struct create_change_notification_request req;
    struct create_change_notification_reply reply;

    req.subtree = bWatchSubtree;
    req.filter  = dwNotifyFilter;
    CLIENT_SendRequest( REQ_CREATE_CHANGE_NOTIFICATION, -1, 1, &req, sizeof(req) );
    CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL );
    if (reply.handle == -1) return INVALID_HANDLE_VALUE;

    change = HeapAlloc( SystemHeap, 0, sizeof(CHANGE_OBJECT) );
    if (!change)
    {
        CLIENT_CloseHandle( reply.handle );
        return INVALID_HANDLE_VALUE;
    }
    change->header.type = K32OBJ_CHANGE;
    change->header.refcount = 1;
    return HANDLE_Alloc( PROCESS_Current(), &change->header, 
                         STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE /*FIXME*/,
                         FALSE, reply.handle );
}

/****************************************************************************
 *		FindFirstChangeNotification32W (KERNEL32.249)
 */
HANDLE WINAPI FindFirstChangeNotificationW( LPCWSTR lpPathName,
                                                BOOL bWatchSubtree,
                                                DWORD dwNotifyFilter) 
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, lpPathName );
    HANDLE ret = FindFirstChangeNotificationA( nameA, bWatchSubtree, 
                                                          dwNotifyFilter );
    if (nameA) HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}

/****************************************************************************
 *		FindNextChangeNotification (KERNEL32.252)
 */
BOOL WINAPI FindNextChangeNotification( HANDLE handle ) 
{
    if (HANDLE_GetServerHandle( PROCESS_Current(), handle,
                                K32OBJ_FILE, 0 ) == -1)
        return FALSE;
    /* FIXME: do something */
    return TRUE;
}

/****************************************************************************
 *		FindCloseChangeNotification (KERNEL32.247)
 */
BOOL WINAPI FindCloseChangeNotification( HANDLE handle) 
{
    return CloseHandle( handle );
}

