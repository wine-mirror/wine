/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      dde_proc.c
 * Purpose :  DDE signals and processes functionality for DDE
 ***************************************************************************
 */
#ifdef CONFIG_IPC

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define msgbuf mymsg
#endif

#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/msg.h>
#include "wintypes.h"
#include "win.h"
#include "shm_semaph.h"
#include "shm_main_blk.h"
#include "dde_proc.h"
#include "dde_mem.h"
#include "dde.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"

int curr_proc_idx= -1;

enum stop_wait_op stop_wait_op=CONT;
int had_SIGUSR2 = 0;
sigjmp_buf env_get_ack;
sigjmp_buf env_wait_x;

#define IDX_TO_HWND(idx)  (0xfffe - (idx))
#define HWND_TO_IDX(wnd)  (0xfffe - (wnd))
#define DDE_WIN_INFO(win) ( main_block->windows[HWND_TO_IDX(win)] )
#define DDE_WIN2PROC(win) ( DDE_WIN_INFO(win).proc_idx )
#define DDE_IsRemoteWindow(win)	 (  (win)<0xffff && (win)>=(0xffff-DDE_PROCS))
#define DDE_SEND 1
#define DDE_POST 2
#define DDE_ACK	 3
#define DDE_MSG_SIZE   sizeof(MSG16)
#define FREE_WND (WORD)(-2)
#define DELETED_WND (WORD)(-3)
#if defined(DEBUG_MSG) || defined(DEBUG_RUNTIME)
static char *msg_type[4]={"********", "DDE_SEND", "DDE_POST", "DDE_ACK"};
#endif

struct msg_dat {
	struct msgbuf dat;
	char filler[DDE_MSG_SIZE];
} ;

typedef struct fifo_element {
	int value;
	struct fifo_element *next;
} fifo_element;

struct fifo {
	fifo_element *first;	   /* first element in the fifo or NULL */
	fifo_element *last;	   /* last element in the fifo or NULL */
};
static struct fifo fifo = {NULL,NULL};

void dde_proc_delete(int proc_idx);

void dde_proc_add_fifo(int val)
{
  fifo_element *created;

  created= (fifo_element*) xmalloc( sizeof(fifo_element) );
  created->value = val;
  created->next = NULL;
  
  if (fifo.first==NULL) 
     fifo.first= created;
  else 
     fifo.last->next= created;
  fifo.last = created;
}

/* get an item from the fifo, and return it.
 * If fifo is empty, return -1
 */
int dde_proc_shift_fifo()
{
  int val;
  fifo_element *deleted;
  
  if (fifo.first == NULL)
     return -1;
  
  deleted= fifo.first;
  val= deleted->value;
  fifo.first= deleted->next;
  if (fifo.first == NULL)
     fifo.last= NULL;

  free(deleted);
  return val;
}

static void print_dde_message(char *desc, MSG16 *msg);

/* This should be run only when main_block is first allocated.	*/
void dde_proc_init(dde_proc proc)
{
  int proc_num;

  for (proc_num=0 ; proc_num<DDE_PROCS ; proc_num++, proc++) {
     proc->msg=-1;
     proc->sem=-1;
     proc->shmid=-1;
     proc->pid=-1;
  }
}

/* add current process to the list of processes */
void dde_proc_add(dde_proc procs)
{
  dde_proc proc;
  int proc_idx;
  dprintf_dde(stddeb,"dde_proc_add(..)\n");
  shm_write_wait(main_block->sem);

  /* find free proc_idx and allocate it */
  for (proc_idx=0, proc=procs ; proc_idx<DDE_PROCS ; proc_idx++, proc++)
     if (proc->pid==-1)
	break;			   /* found! */

  if (proc_idx<DDE_PROCS) {	   /* got here beacuse a free was found ? */
     dde_msg_setup(&proc->msg);
     proc->pid=getpid();
     curr_proc_idx=proc_idx;
     shm_sem_init(&proc->sem);
  }
  else	{
     fflush(stdout);
     fprintf(stderr,"dde_proc_add: Can't allocate process\n");
  }
  shm_write_signal(main_block->sem);
}

/* wait for dde - acknowledge message - or timout */
static BOOL32 get_ack()
{
    struct timeval timeout;
    int size;
    struct msg_dat ack_buff;

    /* timeout after exactly one seconf */
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    sigsetjmp(env_get_ack, 1);
    /* get here after normal execution, or after siglongjmp */

    do {			   /* loop to wait for DDE_ACK */
       had_SIGUSR2=0;
       stop_wait_op=CONT;	   /*  sensitive code: disallow siglongjmp */
       size= msgrcv( main_block->proc[curr_proc_idx].msg , &ack_buff.dat,
		     1, DDE_ACK, IPC_NOWAIT);
       if (size>=0) {
	  dprintf_msg(stddeb,"get_ack: received DDE_ACK message\n");
	  return TRUE;
       }
       if (DDE_GetRemoteMessage()) {
	     had_SIGUSR2=1;  /* might have recieved SIGUSR2 */
       }
       stop_wait_op=STOP_WAIT_ACK; /* allow siglongjmp */

    } while (had_SIGUSR2);	   /* loop if SIGUSR2 was recieved */

    /* siglongjmp should be enabled at this moment */
    select( 0, NULL, NULL, NULL, &timeout );
    stop_wait_op=CONT;		   /* disallow further siglongjmp */

    /* timeout !! (otherwise there would have been a siglongjmp) */
    return FALSE;
}

/* Transfer one message to a given process */
static BOOL32 DDE_DoOneMessage (int proc_idx, int size, struct msgbuf *msgbuf)
{
  dde_proc proc= &main_block->proc[ proc_idx ];


  if (proc_idx == curr_proc_idx)
     return FALSE;

  if (kill(proc->pid,0) < 0) {
     /* pid does not exist, or not our */
     dde_proc_delete(proc_idx);
     return FALSE;
  }

  if (debugging_dde) {
     MSG16 *msg=(MSG16*) &msgbuf->mtext;
     char *title;
     if (msgbuf->mtype==DDE_SEND)
	title="sending dde:";
     else if (msgbuf->mtype==DDE_POST)
	title="posting dde:";
     else
	title=NULL;
     if (title)
	 print_dde_message(title, msg);
     else
       fprintf(stddeb,"Unknown message type=0x%lx\n",msgbuf->mtype);
  }
  dprintf_msg(stddeb,
	      "DDE_DoOneMessage: to proc_idx=%d (pid=%d), queue=%u\n",
	      proc_idx, proc->pid, (unsigned)proc->msg);
  if ( proc->msg != -1) {
     dprintf_msg(stddeb, "DDE_DoOneMessage: doing...(type=%s)\n",
		 msg_type[msgbuf->mtype]);
     size=msgsnd (proc->msg, msgbuf, size, 0);

     if (size<0) {
	 fflush(stdout);
	 perror("msgsnd");
     }
     kill(proc->pid,SIGUSR2);	   /* tell the process there is a message */

     dprintf_msg(stddeb,"DDE_DoOneMessage: "
		 "Trying to get acknowledgment from msg queue=%d\n",
		 proc->msg);
     Yield16();			/* force task switch, and */
				/* acknowledgment sending */
     if (get_ack()) {
	return TRUE;
     } else {
	fflush(stdout);
	fprintf(stderr,"get_ack: DDE_DoOneMessage: timeout\n");
	return FALSE;
     }
  }
  else {
     dprintf_msg(stddeb,"DDE_DoOneMessage: message not sent, "
		 "target has no message queue\n");
     return FALSE;
  }
}

/* Do some sort of premitive hash table */
static HWND16 HWND_Local2Remote(HWND16 orig)
{
  int dde_wnd_idx;
  int deleted_idx= -1;
  WND_DATA *tested;
  WND_DATA *deleted= NULL;
  int i;
  
  dde_wnd_idx= orig % DDE_WINDOWS;
  for ( i=0 ; i < DDE_WINDOWS ; i++, dde_wnd_idx++) {
    if (dde_wnd_idx >= DDE_WINDOWS)
      dde_wnd_idx -= DDE_WINDOWS; /* wrap-around */
    
    tested= &main_block->windows[ dde_wnd_idx ];
    if (tested->proc_idx == FREE_WND)
      break;
    
    if (deleted == NULL && tested->proc_idx == DELETED_WND) {
      deleted= tested;
      deleted_idx= dde_wnd_idx;
    } else if (tested->wnd == orig && tested->proc_idx == curr_proc_idx) {
      return IDX_TO_HWND(dde_wnd_idx);
    }
  }
  if (deleted != NULL)	{	/* deleted is preferable */
    /* free item, allocate it */
    deleted->proc_idx= curr_proc_idx;
    deleted->wnd = orig;
    return IDX_TO_HWND(deleted_idx);
  }
  if (tested->proc_idx == FREE_WND) {
    tested->proc_idx= curr_proc_idx;
    tested->wnd = orig;
    return IDX_TO_HWND(dde_wnd_idx);
  }

  fprintf(stderr,
	  "HWND_Local2Remote: Can't map any more windows to DDE windows\n");
  return 0;			
}

static BOOL32 DDE_DoMessage( MSG16 *msg, int type )
{
  int proc_idx;

  MSG16 *remote_message;
  struct msg_dat msg_dat;
  BOOL32 success;
  
  if (msg->wParam == 0)
      return FALSE;
  
  if (main_block==NULL) {
    if (msg->message >=  WM_DDE_FIRST && msg->message <= WM_DDE_LAST) 
      DDE_IPC_init();
    else 
      return FALSE;
  }


  if (msg->wParam == (HWND16)-1)
     return FALSE;

  if ( ! DDE_IsRemoteWindow(msg->hwnd) && msg->hwnd!= (HWND16)-1)
     return FALSE;

  dprintf_msg(stddeb, "%s: DDE_DoMessage(hwnd=0x%x,msg=0x%x,..)\n",
	      msg_type[type], (int)msg->hwnd,(int)msg->message);


  dprintf_msg(stddeb,
	      "DDE_DoMessage(hwnd=0x%x,msg=0x%x,..) // HWND_BROADCAST !\n",
	      (int)msg->hwnd,(int)msg->message);
  remote_message=(void*)&msg_dat.dat.mtext;
  
  memcpy(remote_message, msg, sizeof(*msg));
  remote_message->wParam= HWND_Local2Remote(msg->wParam);
  if (remote_message->wParam == 0)
    return FALSE;
  
  msg_dat.dat.mtype=type;

  if (msg->hwnd == (HWND16)-1) {
     success= FALSE;
     for ( proc_idx=0; proc_idx < DDE_PROCS ; proc_idx++) {
	if (proc_idx == curr_proc_idx)
	   continue;
	if (main_block->proc[ proc_idx ].msg != -1)
	   success|=DDE_DoOneMessage(proc_idx, DDE_MSG_SIZE, &msg_dat.dat);
     }
     return success;
  } else {
     return DDE_DoOneMessage(DDE_WIN2PROC(msg->hwnd), DDE_MSG_SIZE,
			     &msg_dat.dat);
  }
}

BOOL32 DDE_SendMessage( MSG16 *msg)
{
  return DDE_DoMessage(msg, DDE_SEND);
}

BOOL32 DDE_PostMessage( MSG16 *msg)
{
  return DDE_DoMessage(msg, DDE_POST);
}


void dde_proc_send_ack(HWND16 wnd, BOOL32 val) {
   int proc,msg;

   static struct msgbuf msg_ack={DDE_ACK,{'0'}};

   proc=DDE_WIN2PROC(wnd);
   msg=main_block->proc[proc].msg;
   dprintf_msg(stddeb,"DDE_GetRemoteMessage: sending ACK "
	       "to wnd=%4x, proc=%d,msg=%d, pid=%d\n",wnd,proc,msg,
	       main_block->proc[proc].pid
     );

   msg_ack.mtext[0]=val;
   msgsnd (msg, &msg_ack, 1, 0);
   kill(main_block->proc[proc].pid, SIGUSR2);
}

/* return true (non zero) if had a remote message */
#undef DDE_GetRemoteMessage

int DDE_GetRemoteMessage()
{
  static int nesting=0;		   /* to avoid infinite recursion */

  MSG16 *remote_message;
  int size;
  struct msg_dat msg_dat;
  BOOL32 was_sent;		   /* sent/received */
  BOOL32 passed;
  WND *wndPtr;

  if (curr_proc_idx==-1)	   /* do we have DDE initialized ? */
     return 0;

  if (nesting>10) {
     fflush(stdout);
     fprintf(stderr,"DDE_GetRemoteMessage: suspecting infinite recursion, exiting");
     return 0;
  }

  remote_message=(void*)&msg_dat.dat.mtext;

  /* test for SendMessage */
  size= msgrcv( main_block->proc[curr_proc_idx].msg , &msg_dat.dat,
		DDE_MSG_SIZE, DDE_SEND, IPC_NOWAIT);

  if (size==DDE_MSG_SIZE) {	   /* is this a correct message (if any) ?*/
     was_sent=TRUE;
     dprintf_msg(stddeb,
		 "DDE:receive sent message. msg=%04x wPar=%04x"
		 " lPar=%08lx\n",
		 remote_message->message, remote_message->wParam,
		 remote_message->lParam);
  } else {
     size= msgrcv( main_block->proc[curr_proc_idx].msg , &msg_dat.dat,
		   DDE_MSG_SIZE, DDE_POST, IPC_NOWAIT);

     if (size==DDE_MSG_SIZE) {	   /* is this a correct message (if any) ?*/
	was_sent=FALSE;
	dprintf_msg(stddeb,
		    "DDE:receive posted message. "
		    "msg=%04x wPar=%04x lPar=%08lx\n",
		    remote_message->message, remote_message->wParam,
		    remote_message->lParam);
     }
     else
	return 0;		   /* no DDE message found */
  }

  /* At this point we are sure that there is a DDE message,
   * was_sent is TRUE is the message was sent, and false if it was posted
   */

  nesting++;

  if (debugging_dde) {
     char *title;
     if (was_sent)
	title="receive sent dde:";
     else
	title="receive posted dde:";
     print_dde_message(title, remote_message);
  }

  if (remote_message->hwnd != (HWND16) -1 ) {
    HWND16 dde_window= DDE_WIN_INFO(remote_message->hwnd).wnd;
     /* we should know exactly where to send the message (locally)*/
     if (was_sent) {
	dprintf_dde(stddeb,
		    "SendMessage(wnd=0x%04x, msg=0x%04x, wPar=0x%04x,"
		    "lPar=0x%08x\n",
		    dde_window, remote_message->message,
		    remote_message->wParam, (int)remote_message->lParam);

	/* execute the recieved message */
	passed= SendMessage16(dde_window, remote_message->message,
			    remote_message->wParam, remote_message->lParam);

	/* Tell the sended, that the message is here */
	dde_proc_send_ack(remote_message->wParam, passed);
     }
     else {
	passed= PostMessage16(dde_window, remote_message->message,
			    remote_message->wParam, remote_message->lParam);
	if (passed == FALSE) {
	   /* Tell the sender, that the message is here, and failed */
	    dde_proc_send_ack(remote_message->wParam, FALSE);
	}
	else {
	   /* ack will be sent later, at the first peek/get message  */
	   dde_proc_add_fifo(remote_message->wParam);
	}
     }
     nesting--;
     return 1;
  }

  /* iterate through all the windows */
  for (wndPtr = WIN_FindWndPtr(GetTopWindow32(GetDesktopWindow32()));
       wndPtr != NULL;
       wndPtr = wndPtr->next)
  {
     if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION) {
	if (was_sent)
	   SendMessage16( wndPtr->hwndSelf, remote_message->message,
			remote_message->wParam, remote_message->lParam );
	else
	   PostMessage16( wndPtr->hwndSelf, remote_message->message,
                        remote_message->wParam, remote_message->lParam );
     } /* if */
  } /* for */

  /* replay with DDE_ACK after broadcasting in DDE_GetRemoteMessage */
  dde_proc_send_ack(remote_message->wParam, TRUE);

  nesting--;
  return 1;
}

int dde_reschedule()
{
    int ack_wnd;
	
    ack_wnd= dde_proc_shift_fifo();
    if (ack_wnd != -1) {
	dde_proc_send_ack(ack_wnd, TRUE);
	usleep(10000);		/* force unix task switch */
	return 1;
    }
    return 0;
}
void dde_msg_setup(int *msg_ptr)
{
  *msg_ptr= msgget (IPC_PRIVATE, IPC_CREAT | 0700);
  if (*msg_ptr==-1)
     perror("dde_msg_setup fails to get message queue");
}

/* do we have dde handling in the window ?
 * If we have, atom usage will make this instance of wine set up
 * it's IPC stuff.
 */
void DDE_TestDDE(HWND16 hwnd)	   
{
  static in_test = 0;
  if (in_test++) return;
  if (main_block != NULL) {
     in_test--;
     return;
  }
  dprintf_msg(stddeb,"DDE_TestDDE(0x%04x)\n", hwnd);
  if (hwnd==0)
      hwnd=-1;
  /* just send a message to see how things are going */
  SendMessage16( hwnd, WM_DDE_INITIATE, 0, 0);
  in_test--;
}

void dde_proc_delete(int proc_idx)
{
  dde_proc_done(&main_block->proc[proc_idx]);
}
void stop_wait(int a)
{

  had_SIGUSR2=1;
  switch(stop_wait_op) {
    case STOP_WAIT_ACK:
      siglongjmp(env_get_ack,1);
      break;  /* never reached */
    case STOP_WAIT_X:
      siglongjmp(env_wait_x,1);
      break;  /* never reached */
    case CONT:
      /* do nothing */
  }
}

static void print_dde_message(char *desc, MSG16 *msg)
{
/*    extern const char *MessageTypeNames[];*/
    extern int debug_last_handle_size;
    WORD wStatus,hWord;
    void *ptr;
    DDEACK *ddeack;
    DDEADVISE *ddeadvise;
    DDEDATA *ddedata;
    DDEPOKE *ddepoke;

    if (is_dde_handle(msg->lParam & 0xffff) )
	ptr=DDE_AttachHandle(msg->lParam&0xffff, NULL);
    else
	ptr =NULL;
    wStatus=LOWORD(msg->lParam);
    hWord=HIWORD(msg->lParam);

    fprintf(stddeb,"%s", desc);
    fprintf(stddeb,"%04x %04x==%s %04x %08lx ",
	    msg->hwnd, msg->message,"",/*MessageTypeNames[msg->message],*/
	    msg->wParam, msg->lParam);
    switch(msg->message) {
      case WM_DDE_INITIATE:
      case WM_DDE_REQUEST:
      case WM_DDE_EXECUTE:
      case WM_DDE_TERMINATE:
	/* nothing to do */
	break;
      case WM_DDE_ADVISE:
	/* DDEADVISE: hOptions in WM_DDE_ADVISE message */
	if (ptr) {
	   ddeadvise=ptr;
	   fprintf(stddeb,"fDeferUpd=%d,fAckReq=%d,cfFormat=0x%x",
		   ddeadvise->fDeferUpd, ddeadvise->fAckReq,
		   ddeadvise->cfFormat);
	} else
	   fprintf(stddeb,"NO-DATA");
	fprintf(stddeb," atom=0x%x",hWord);
	break;

      case WM_DDE_UNADVISE:
	fprintf(stddeb,"format=0x%x, atom=0x%x",wStatus,hWord);
	break;
      case WM_DDE_ACK:
	ddeack=(DDEACK*)&wStatus;
	fprintf(stddeb,"bAppReturnCode=%d,fBusy=%d,fAck=%d",
		ddeack->bAppReturnCode, ddeack->fBusy, ddeack->fAck);
	if (ddeack->fAck)
	   fprintf(stddeb,"(True)");
	else
	   fprintf(stddeb,"(False)");
	break;

      case WM_DDE_DATA:
	if (ptr) {
	   ddedata=ptr;
	   fprintf(stddeb,"fResponse=%d,fRelease=%d,"
		   "fAckReq=%d,cfFormat=0x%x,value=\"%.*s\"",
		   ddedata->fResponse, ddedata->fRelease,
		   ddedata->fAckReq, ddedata->cfFormat,
		   debug_last_handle_size- (int)sizeof(*ddedata)+1,
		   ddedata->Value);
	} else
	   fprintf(stddeb,"NO-DATA");
	fprintf(stddeb," atom=0x%04x",hWord);
	break;

      case WM_DDE_POKE:
	if (ptr) {
	   ddepoke=ptr;
	   fprintf(stddeb,"fRelease=%d,cfFormat=0x%x,value[0]='%c'",
		   ddepoke->fRelease, ddepoke->cfFormat, ddepoke->Value[0]);
	} else
	   fprintf(stddeb,"NO-DATA");
	fprintf(stddeb," atom=0x%04x",hWord);
	break;
    }
    fprintf(stddeb,"\n");
}

void dde_proc_done(dde_proc proc)
{
  if (proc->msg != -1)
     msgctl(proc->msg, IPC_RMID, NULL);
  proc->msg=-1;
  proc->pid=-1;
  shm_delete_chain(&proc->shmid);
  shm_sem_done(&proc->sem);
}

/* delete entry, if old junk */
void dde_proc_refresh(dde_proc proc)
{
  if (proc->pid == -1)		  
     return;
  
  if (kill(proc->pid, 0) != -1)
     return;

  /* get here if entry non empty, and the process does not exist */
  dde_proc_done(proc);
}

void dde_wnd_setup()
{
  int i;

  for (i=0 ; i < DDE_WINDOWS ; i++)
    main_block->windows[i].proc_idx = FREE_WND;
}

static BOOL32 DDE_ProcHasWindows(int proc_idx)
{
  WND_DATA *tested;
  int i;
  
  for ( i=0 ; i < DDE_WINDOWS ; i++) {
    tested= &main_block->windows[ i ];
    
    if (tested->proc_idx == proc_idx) 
      return TRUE;
  }
  return FALSE;
}
/* Look for hwnd in the hash table of DDE windows,
 * Delete it from there. If there are no more windows for this
 * process, remove the process from the DDE data-structure.
 * If there are no more processes - delete the whole DDE struff.
 *
 * This is inefficient, but who cares for the efficiency of this rare
 * operation... 
 */
void DDE_DestroyWindow(HWND16 hwnd)
{
  int dde_wnd_idx;
  WND_DATA *tested;
  int i;
  
  if (main_block == NULL)
    return;
  
  dde_wnd_idx= hwnd % DDE_WINDOWS;
  
  for ( i=0 ; i < DDE_WINDOWS ; i++, dde_wnd_idx++) {
    if (dde_wnd_idx >= DDE_WINDOWS)
      dde_wnd_idx -= DDE_WINDOWS; /* wrap-around */
    
    tested= &main_block->windows[ dde_wnd_idx ];
    if (tested->proc_idx == FREE_WND)
      return;			/* No window will get deleted here */
    
    if (tested->wnd == hwnd && tested->proc_idx == curr_proc_idx) {
      dde_reschedule();
      tested->proc_idx= DELETED_WND;
      if (DDE_ProcHasWindows( curr_proc_idx ))
	return;
      while (dde_reschedule())	/* make sure there are no other */
				/* processes waiting for acknowledgment */
	;
      dde_proc_delete( curr_proc_idx );
      if (DDE_no_of_attached() == 1)
	shm_delete_all(-1);
      else {
	shmdt( (void *) main_block);
	main_block= NULL;
      }
      return;
    }
  }
}

#endif  /* CONFIG_IPC */
