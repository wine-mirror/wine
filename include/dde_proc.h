/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      dde_proc.h
 * Purpose :  DDE signals and processes functionality for DDE
 ***************************************************************************
 */
#ifndef __WINE_DDE_PROC_H
#define __WINE_DDE_PROC_H

#ifdef CONFIG_IPC

#include <setjmp.h>
#include "windef.h"
#include "windows.h"
#define DDE_PROCS 64
#define DDE_WINDOWS 64
struct _dde_proc {
    int msg;			/* message queue for this process */
    int shmid;			/* first shared memory block id. */
    int sem;			/* semaphore for fragment allocation */
    int pid;
} ;
typedef struct _dde_proc *dde_proc;

extern sigjmp_buf env_wait_x;
enum stop_wait_op {		/* The action to be taken upon SIGUSR2 */
    CONT,			/* Don't do anything */
    STOP_WAIT_ACK,		/* Use siglongjmp to stop wait_ack() */
    STOP_WAIT_X			/* siglongjmp to stop MSG_WaitXEvent() */
};

typedef struct {
    WORD  proc_idx;		/* index into wine's process table  */
    HWND16  wnd;		/* Window on the local proccess */
} WND_DATA;
extern enum stop_wait_op stop_wait_op;
extern int had_SIGUSR2;

extern int curr_proc_idx;
void stop_wait(int a);		   /* signal handler for SIGUSR2
				      (interrupts "select" system call) */
void dde_proc_init(dde_proc proc); /* init proc array */
void dde_proc_done(dde_proc proc); /* delete a proc entry */
void dde_proc_refresh(dde_proc proc); /* delete entry, if old junk */
void dde_proc_add(dde_proc proc);  /* Add current proc to proc array */
void dde_msg_setup(int *msg_ptr);
int  dde_reschedule();
void dde_wnd_setup();		   /* setup Data structure of DDE windows */

/* Send ack. to hnd indicating that posted/sent msg. got to destination*/
void dde_proc_send_ack(HWND16 wnd, BOOL val);
BOOL DDE_PostMessage( MSG16 *msg);
BOOL DDE_SendMessage( MSG16 *msg);
int DDE_GetRemoteMessage();
void DDE_DestroyWindow(HWND16 hwnd); /* delete DDE info regarding hwnd */
void DDE_TestDDE(HWND16 hwnd);	   /* do we have dde handling in the window ?*/

#endif  /* CONFIG_IPC */

#endif /* __WINE_DDE_PROC_H */
