/*
 * Message queues definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_QUEUE_H
#define __WINE_QUEUE_H

#include "windows.h"

  /* Message as stored in the queue (contains the extraInfo field) */
typedef struct tagQMSG
{
    DWORD   extraInfo;  /* Only in 3.1 */
    MSG16   msg;
} QMSG;

typedef struct
{
  LRESULT   lResult;
  BOOL16    bPending;
} QSMCTRL;

#pragma pack(1)

typedef struct tagMESSAGEQUEUE
{
  HQUEUE16  next;                   /* 00 Next queue */
  HTASK16   hTask;                  /* 02 hTask owning the queue */
  WORD      msgSize;                /* 04 Size of messages in the queue */
  WORD      msgCount;               /* 06 Number of waiting messages */
  WORD      nextMessage;            /* 08 Next message to be retrieved */
  WORD      nextFreeMessage;        /* 0a Next available slot in the queue */
  WORD      queueSize;              /* 0c Size of the queue */
  DWORD     GetMessageTimeVal WINE_PACKED;  /* 0e Value for GetMessageTime */
  DWORD     GetMessagePosVal WINE_PACKED;   /* 12 Value for GetMessagePos */
  HQUEUE16  self;                   /* 16 Handle to self (was: reserved) */
  DWORD     GetMessageExtraInfoVal; /* 18 Value for GetMessageExtraInfo */
  WORD      wParamHigh;             /* 1c High word of wParam (was: reserved)*/
  LPARAM    lParam WINE_PACKED;     /* 1e Next 4 values set by SendMessage */
  WPARAM16  wParam;                 /* 22 */
  UINT16    msg;                    /* 24 */
  HWND16    hWnd;                   /* 26 */
  DWORD     SendMessageReturn;      /* 28 Return value for SendMessage */
  WORD      wPostQMsg;              /* 2c PostQuitMessage flag */
  WORD      wExitCode;              /* 2e PostQuitMessage exit code */
  WORD      flags;                  /* 30 Queue flags */
  QSMCTRL*  smResultInit;           /* 32 1st LRESULT ptr - was: reserved */
  WORD      wWinVersion;            /* 36 Expected Windows version */
  HQUEUE16  InSendMessageHandle;    /* 38 Queue of task that sent a message */
  HTASK16   hSendingTask;           /* 3a Handle of task that sent a message */
  HTASK16   hPrevSendingTask;       /* 3c Handle of previous sender */
  WORD      wPaintCount;            /* 3e Number of WM_PAINT needed */
  WORD      wTimerCount;            /* 40 Number of timers for this task */
  WORD      changeBits;             /* 42 Changed wake-up bits */
  WORD      wakeBits;               /* 44 Queue wake-up bits */
  WORD      wakeMask;               /* 46 Queue wake-up mask */
  QSMCTRL*  smResultCurrent;        /* 48 ptrs to SendMessage() LRESULT - point to */
  WORD      SendMsgReturnPtr[1];    /*    values on stack */
  HANDLE16  hCurHook;               /* 4e Current hook */
  HANDLE16  hooks[WH_NB_HOOKS];     /* 50 Task hooks list */
  QSMCTRL*  smResult;               /* 6c 3rd LRESULT ptr - was: reserved */
  QMSG      messages[1];            /* 70 Queue messages */
} MESSAGEQUEUE;

#pragma pack(4)

/* Extra (undocumented) queue wake bits - see "Undoc. Windows" */
#define QS_SMRESULT      0x8000  /* Queue has a SendMessage() result */
#define QS_SMPARAMSFREE  0x4000  /* SendMessage() parameters are available */

/* Queue flags */
#define QUEUE_FLAG_XEVENT  0x0001
#define QUEUE_SM_WIN32     0x0002  /* Currently sent message is Win32 */
#define QUEUE_SM_UNICODE   0x0004  /* Currently sent message is Unicode */

extern void QUEUE_DumpQueue( HQUEUE16 hQueue );
extern void QUEUE_WalkQueues(void);
extern BOOL32 QUEUE_IsExitingQueue( HQUEUE16 hQueue );
extern void QUEUE_SetExitingQueue( HQUEUE16 hQueue );
extern MESSAGEQUEUE *QUEUE_GetSysQueue(void);
extern void QUEUE_SetWakeBit( MESSAGEQUEUE *queue, WORD bit );
extern void QUEUE_ClearWakeBit( MESSAGEQUEUE *queue, WORD bit );
extern void QUEUE_ReceiveMessage( MESSAGEQUEUE *queue );
extern void QUEUE_WaitBits( WORD bits );
extern void QUEUE_IncPaintCount( HQUEUE16 hQueue );
extern void QUEUE_DecPaintCount( HQUEUE16 hQueue );
extern void QUEUE_IncTimerCount( HQUEUE16 hQueue );
extern void QUEUE_DecTimerCount( HQUEUE16 hQueue );
extern BOOL32 QUEUE_CreateSysMsgQueue( int size );
extern BOOL32 QUEUE_DeleteMsgQueue( HQUEUE16 hQueue );
extern HTASK16 QUEUE_GetQueueTask( HQUEUE16 hQueue );
extern BOOL32 QUEUE_AddMsg( HQUEUE16 hQueue, MSG16 * msg, DWORD extraInfo );
extern int QUEUE_FindMsg( MESSAGEQUEUE * msgQueue, HWND32 hwnd,
                          int first, int last );
extern void QUEUE_RemoveMsg( MESSAGEQUEUE * msgQueue, int pos );
extern void hardware_event( WORD message, WORD wParam, LONG lParam,
			    int xPos, int yPos, DWORD time, DWORD extraInfo );

#endif  /* __WINE_QUEUE_H */
