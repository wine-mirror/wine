/*
 * Message queues definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_QUEUE_H
#define __WINE_QUEUE_H

#include "windows.h"

#ifndef WINELIB
#pragma pack(1)
#endif

  /* Message as stored in the queue (contains the extraInfo field) */
typedef struct tagQMSG
{
    DWORD   extraInfo;  /* Only in 3.1 */
    MSG     msg;
} QMSG;


typedef struct tagMESSAGEQUEUE
{
  HQUEUE    next;                   /* 00 Next queue */
  HTASK     hTask;                  /* 02 hTask owning the queue */
  WORD      msgSize;                /* 04 Size of messages in the queue */
  WORD      msgCount;               /* 06 Number of waiting messages */
  WORD      nextMessage;            /* 08 Next message to be retrieved */
  WORD      nextFreeMessage;        /* 0a Next available slot in the queue */
  WORD      queueSize;              /* 0c Size of the queue */
  DWORD     GetMessageTimeVal WINE_PACKED;  /* 0e Value for GetMessageTime */
  DWORD     GetMessagePosVal WINE_PACKED;   /* 12 Value for GetMessagePos */
  WORD      reserved1;              /* 16 Unknown */
  DWORD     GetMessageExtraInfoVal; /* 18 Value for GetMessageExtraInfo */
  WORD      reserved2;              /* 1c Unknown */
  LPARAM    lParam WINE_PACKED;     /* 1e Next 4 values set by SendMessage */
  WPARAM    wParam;                 /* 22 */
  UINT      msg;                    /* 24 */
  HWND      hWnd;                   /* 26 */
  DWORD     SendMessageReturn;      /* 28 Return value for SendMessage */
  WORD      wPostQMsg;              /* 2c PostQuitMessage flag */
  WORD      wExitCode;              /* 2e PostQuitMessage exit code */
  WORD      reserved3[3];           /* 30 Unknown */
  WORD      wWinVersion;            /* 36 Expected Windows version */
  HQUEUE    InSendMessageHandle;    /* 38 Queue of task that sent a message */
  HTASK     hSendingTask;           /* 3a Handle of task that sent a message */
  HTASK     hPrevSendingTask;       /* 3c Handle of previous sender */
  WORD      wPaintCount;            /* 3e Number of WM_PAINT needed */
  WORD      wTimerCount;            /* 40 Number of timers for this task */
  WORD      tempStatus;             /* 42 State reset by GetQueueStatus */
  WORD      status;                 /* 44 Queue state */
  WORD      wakeMask;               /* 46 Task wake-up mask */
  WORD      SendMsgReturnPtrs[3];   /* 48 Near ptr to return values (?) */
  HANDLE    hCurHook;               /* 4e Current hook */
  HANDLE    hooks[WH_NB_HOOKS];     /* 50 Task hooks list */
  WORD      reserved4[3];           /* 68 Unknown */
  QMSG      messages[1];            /* 6e Queue messages */
} MESSAGEQUEUE;

#ifndef WINELIB
#pragma pack(4)
#endif

extern void QUEUE_DumpQueue( HQUEUE hQueue );
extern void QUEUE_WalkQueues(void);
extern MESSAGEQUEUE *QUEUE_GetSysQueue(void);
extern void QUEUE_IncPaintCount( HQUEUE hQueue );
extern void QUEUE_DecPaintCount( HQUEUE hQueue );
extern void QUEUE_IncTimerCount( HQUEUE hQueue );
extern void QUEUE_DecTimerCount( HQUEUE hQueue );
extern BOOL QUEUE_CreateSysMsgQueue( int size );
extern BOOL QUEUE_DeleteMsgQueue( HQUEUE hQueue );
extern HTASK QUEUE_GetQueueTask( HQUEUE hQueue );
extern BOOL QUEUE_AddMsg( HQUEUE hQueue, MSG * msg, DWORD extraInfo );
extern int QUEUE_FindMsg( MESSAGEQUEUE * msgQueue, HWND hwnd,
                          int first, int last );
extern void QUEUE_RemoveMsg( MESSAGEQUEUE * msgQueue, int pos );
extern void hardware_event( WORD message, WORD wParam, LONG lParam,
			    int xPos, int yPos, DWORD time, DWORD extraInfo );

#endif  /* __WINE_QUEUE_H */
