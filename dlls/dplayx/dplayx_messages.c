/* DirectPlay & DirectPlayLobby messaging implementation
 *
 * Copyright 2000 - Peter Hunnisett
 *
 * <presently under construction - contact hunnise@nortelnetworks.com>
 *
 */

#include "winbase.h"
#include "debugtools.h"

#include "dplayx_messages.h"

DEFAULT_DEBUG_CHANNEL(dplay)


static DWORD CALLBACK DPLAYX_MSG_ThreadMain( LPVOID lpContext );

/* Create the message reception thread to allow the application to receive
 * asynchronous message reception
 */
DWORD CreateMessageReceptionThread( HANDLE hNotifyEvent )
{
  DWORD dwMsgThreadId;

  if( !DuplicateHandle( 0, hNotifyEvent, 0, NULL, 0, FALSE, 0 ) )
  {
    ERR( "Unable to duplicate event handle\n" );
    return 0;
  }

  /* FIXME: Should most likely store that thread handle */
  CreateThread( NULL,                  /* Security attribs */
                0,                     /* Stack */ 
                DPLAYX_MSG_ThreadMain, /* Msg reception function */
                (LPVOID)hNotifyEvent,  /* Msg reception function parameter */
                0,                     /* Flags */
                &dwMsgThreadId         /* Updated with thread id */
              );
 
  return dwMsgThreadId;
}
static DWORD CALLBACK DPLAYX_MSG_ThreadMain( LPVOID lpContext )
{
  HANDLE hMsgEvent = (HANDLE)lpContext;

  for ( ;; )
  {
    FIXME( "Ho Hum. Msg thread with nothing to do on handle %u\n", hMsgEvent );

    SleepEx( 10000, FALSE );  /* 10 secs */
  }

  CloseHandle( hMsgEvent );
}

