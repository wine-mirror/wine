/*
 * Message queues definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_QUEUE_H
#define __WINE_QUEUE_H

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "thread.h"

#define WH_NB_HOOKS (WH_MAXHOOK-WH_MINHOOK+1)

/* Per-queue data for the message queue
 * Note that we currently only store the current values for
 * Active, Capture and Focus windows currently.
 * It might be necessary to store a pointer to the system message queue
 * as well since windows 9x maintains per thread system message queues
 */
typedef struct tagPERQUEUEDATA      
{
  HWND    hWndFocus;              /* Focus window */
  HWND    hWndActive;             /* Active window */
  HWND    hWndCapture;            /* Capture window */
  INT16     nCaptureHT;             /* Capture info (hit-test) */
  ULONG     ulRefCount;             /* Reference count */
  CRITICAL_SECTION cSection;        /* Critical section for thread safe access */
} PERQUEUEDATA;

struct received_message_info;

/* Message queue */
typedef struct tagMESSAGEQUEUE
{
  HQUEUE16  self;                   /* Handle to self (was: reserved) */
  TEB*      teb;                    /* Thread owning queue */
  HANDLE    server_queue;           /* Handle to server-side queue */
  struct received_message_info *receive_info; /* Info about message being currently received */

  DWORD     magic;                  /* magic number should be QUEUE_MAGIC */
  DWORD     lockCount;              /* reference counter */

  DWORD     GetMessageTimeVal;      /* Value for GetMessageTime */
  DWORD     GetMessagePosVal;       /* Value for GetMessagePos */
  DWORD     GetMessageExtraInfoVal; /* Value for GetMessageExtraInfo */

  HANDLE16  hCurHook;               /* Current hook */
  HANDLE16  hooks[WH_NB_HOOKS];     /* Task hooks list */

  PERQUEUEDATA *pQData;             /* pointer to (shared) PERQUEUEDATA structure */
  
} MESSAGEQUEUE;


#define QUEUE_MAGIC        0xD46E80AF

/* Per queue data management methods */
HWND PERQDATA_GetFocusWnd( PERQUEUEDATA *pQData );
HWND PERQDATA_SetFocusWnd( PERQUEUEDATA *pQData, HWND hWndFocus );
HWND PERQDATA_GetActiveWnd( PERQUEUEDATA *pQData );
HWND PERQDATA_SetActiveWnd( PERQUEUEDATA *pQData, HWND hWndActive );
HWND PERQDATA_GetCaptureWnd( INT *hittest );
HWND PERQDATA_SetCaptureWnd( HWND hWndCapture, INT hittest );

/* Message queue management methods */
extern MESSAGEQUEUE *QUEUE_Current(void);
extern MESSAGEQUEUE *QUEUE_Lock( HQUEUE16 hQueue );
extern void QUEUE_Unlock( MESSAGEQUEUE *queue );
extern void QUEUE_DeleteMsgQueue(void);

#endif  /* __WINE_QUEUE_H */
