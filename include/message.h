/*
 * Message queues definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef MESSAGE_H
#define MESSAGE_H

#include "windows.h"

  /* Message as stored in the queue (contains the extraInfo field) */
typedef struct tagQMSG
{
    DWORD   extraInfo;  /* Only in 3.1 */
    MSG     msg;
} QMSG;


typedef struct tagMESSAGEQUEUE
{
  WORD      next;
  HTASK     hTask;                  /* hTask owning the queue                */
  WORD      msgSize;                /* Size of messages in the queue         */
  WORD      msgCount;               /* Number of waiting messages            */
  WORD      nextMessage;            /* Next message to be retrieved          */
  WORD      nextFreeMessage;        /* Next available slot in the queue      */
  WORD      queueSize;              /* Size of the queue                     */
  DWORD     GetMessageTimeVal;      /* Value returned by GetMessageTime      */
  DWORD     GetMessagePosVal;       /* Value returned by GetMessagePos       */
  DWORD     GetMessageExtraInfoVal; /* Value returned by GetMessageExtraInfo */
  LPARAM    lParam;                 /* Next four values set by SendMessage   */
  WPARAM    wParam;
  UINT      msg;
  HWND      hWnd;
  WORD      wPostQMsg;              /* PostQuitMessage flag                  */
  WORD      wExitCode;              /* PostQuitMessage exit code             */
  WORD      InSendMessageHandle;    /* Handle of task that sent a message    */
  WORD      wPaintCount;            /* Number of WM_PAINT needed             */
  WORD      wTimerCount;            /* Number of timers for this application */
  WORD      tempStatus;             /* State reset by GetQueueStatus         */
  WORD      status;                 /* Queue state                           */
  QMSG      messages[1];            /* Queue messages                        */
} MESSAGEQUEUE;


extern void MSG_IncPaintCount( HANDLE hQueue );
extern void MSG_DecPaintCount( HANDLE hQueue );
extern void MSG_IncTimerCount( HANDLE hQueue );
extern void MSG_DecTimerCount( HANDLE hQueue );
extern void MSG_Synchronize();
extern BOOL MSG_WaitXEvent( LONG maxWait );
extern BOOL MSG_CreateSysMsgQueue( int size );
extern void hardware_event( WORD message, WORD wParam, LONG lParam,
			    int xPos, int yPos, DWORD time, DWORD extraInfo );
extern BOOL MSG_GetHardwareMessage( LPMSG msg );
extern BOOL MSG_InternalGetMessage( SEGPTR msg, HWND hwnd, HWND hwndOwner,
				    short code, WORD flags, BOOL sendIdle );

#endif  /* MESSAGE_H */
