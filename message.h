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
    MSG     msg;
    DWORD   extraInfo __attribute__ ((packed));  /* Only in 3.1 */
} QMSG;


typedef struct tagMESSAGEQUEUE
{
  WORD      next;
  WORD      hTask;                  /* hTask owning the queue                */
  WORD      msgSize;                /* Size of messages in the queue         */
  WORD      msgCount;               /* Number of waiting messages            */
  WORD      nextMessage;            /* Next message to be retrieved          */
  WORD      nextFreeMessage;        /* Next available slot in the queue      */
  WORD      queueSize;              /* Size of the queue                     */
  DWORD     GetMessageTimeVal;      /* Value returned by GetMessageTime      */
  DWORD     GetMessagePosVal;       /* Value returned by GetMessagePos       */
  WORD      GetMessageExtraInfoVal; /* Value returned by GetMessageExtraInfo */
  DWORD     lParam;                 /* Next four values set by SetMessage    */
  WORD      wParam;
  WORD      msg;
  WORD      hWnd;
  WORD      wPostQMsg;              /* PostQuitMessage flag                  */
  WORD      wExitCode;              /* PostQuitMessage exit code             */
  WORD      InSendMessageHandle;    /* Handle of task that sent a message    */
  WORD      tempStatus;             /* State reset by GetQueueStatus         */
  WORD      status;                 /* Queue state                           */
  QMSG      messages[1];            /* Queue messages                        */
} MESSAGEQUEUE;

#endif  /* MESSAGE_H */
