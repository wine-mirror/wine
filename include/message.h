/*
 * Message queues definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef MESSAGE_H
#define MESSAGE_H

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
  HANDLE    next;                   /* 00 Next queue */
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

extern void MSG_IncPaintCount( HANDLE hQueue );
extern void MSG_DecPaintCount( HANDLE hQueue );
extern void MSG_IncTimerCount( HANDLE hQueue );
extern void MSG_DecTimerCount( HANDLE hQueue );
extern void MSG_Synchronize();
extern BOOL MSG_WaitXEvent( LONG maxWait );
extern BOOL MSG_CreateSysMsgQueue( int size );
extern BOOL MSG_DeleteMsgQueue( HANDLE hQueue );
extern HTASK MSG_GetQueueTask( HANDLE hQueue );
extern void hardware_event( WORD message, WORD wParam, LONG lParam,
			    int xPos, int yPos, DWORD time, DWORD extraInfo );
extern BOOL MSG_GetHardwareMessage( LPMSG msg );
extern BOOL MSG_InternalGetMessage( SEGPTR msg, HWND hwnd, HWND hwndOwner,
				    short code, WORD flags, BOOL sendIdle );

#endif  /* MESSAGE_H */
