/*
 * DOS Virtual Machine
 *
 * Copyright 1998 Ove Kåven
 *
 * This code hasn't been completely cleaned up yet.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "wine/winbase16.h"
#include "wine/exception.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnt.h"
#include "wincon.h"

#include "callback.h"
#include "msdos.h"
#include "file.h"
#include "miscemu.h"
#include "dosexe.h"
#include "dosmod.h"
#include "stackframe.h"
#include "debugtools.h"

DECLARE_DEBUG_CHANNEL(int);
DECLARE_DEBUG_CHANNEL(module);
DECLARE_DEBUG_CHANNEL(relay);

#ifdef MZ_SUPPORTED

#ifdef HAVE_SYS_VM86_H
# include <sys/vm86.h>
#endif
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif

#define IF_CLR(ctx)     ((ctx)->EFlags &= ~VIF_MASK)
#define IF_SET(ctx)     ((ctx)->EFlags |= VIF_MASK)
#define IF_ENABLED(ctx) ((ctx)->EFlags & VIF_MASK)
#define SET_PEND(ctx)   ((ctx)->EFlags |= VIP_MASK)
#define CLR_PEND(ctx)   ((ctx)->EFlags &= ~VIP_MASK)
#define IS_PEND(ctx)    ((ctx)->EFlags & VIP_MASK)

#undef TRY_PICRETURN

typedef struct _DOSEVENT {
  int irq,priority;
  void (*relay)(CONTEXT86*,void*);
  void *data;
  struct _DOSEVENT *next;
} DOSEVENT, *LPDOSEVENT;

static struct _DOSEVENT *pending_event, *current_event;
static int sig_sent, entered;

/* from module.c */
extern int read_pipe, write_pipe;
extern HANDLE hReadPipe;
extern pid_t dosmod_pid;

static void do_exception( int signal, CONTEXT86 *context )
{
    EXCEPTION_RECORD rec;
    if ((signal == SIGTRAP) || (signal == SIGHUP))
    {
        rec.ExceptionCode  = EXCEPTION_BREAKPOINT;
        rec.ExceptionFlags = EXCEPTION_CONTINUABLE;
    }
    else
    {
        rec.ExceptionCode  = EXCEPTION_ILLEGAL_INSTRUCTION;  /* generic error */
        rec.ExceptionFlags = EH_NONCONTINUABLE;
    }
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)context->Eip;
    rec.NumberParameters = 0;
    EXC_RtlRaiseException( &rec, context );
}

static void DOSVM_Dump( int fn, int sig, struct vm86plus_struct*VM86 )
{
 BYTE*inst;
 int x;

 switch (VM86_TYPE(fn)) {
  case VM86_SIGNAL:
   printf("Trapped signal %d\n",sig); break;
  case VM86_UNKNOWN:
   printf("Trapped unhandled GPF\n"); break;
  case VM86_INTx:
   printf("Trapped INT %02x\n",VM86_ARG(fn)); break;
  case VM86_STI:
   printf("Trapped STI\n"); break;
  case VM86_PICRETURN:
   printf("Trapped due to pending PIC request\n"); break;
  case VM86_TRAP:
   printf("Trapped debug request\n"); break;
  default:
   printf("Trapped unknown VM86 type %d arg %d\n",VM86_TYPE(fn),VM86_ARG(fn)); break;
 }
#define REGS VM86->regs
 fprintf(stderr,"AX=%04lX CX=%04lX DX=%04lX BX=%04lX\n",REGS.eax,REGS.ecx,REGS.edx,REGS.ebx);
 fprintf(stderr,"SI=%04lX DI=%04lX SP=%04lX BP=%04lX\n",REGS.esi,REGS.edi,REGS.esp,REGS.ebp);
 fprintf(stderr,"CS=%04X DS=%04X ES=%04X SS=%04X\n",REGS.cs,REGS.ds,REGS.es,REGS.ss);
 fprintf(stderr,"IP=%04lX EFLAGS=%08lX\n",REGS.eip,REGS.eflags);

 inst = PTR_REAL_TO_LIN( REGS.cs, REGS.eip );
#undef REGS
 printf("Opcodes:");
 for (x=0; x<8; x++) printf(" %02x",inst[x]);
 printf("\n");
}

static int DOSVM_SimulateInt( int vect, CONTEXT86 *context, BOOL inwine )
{
  FARPROC16 handler=INT_GetRMHandler(vect);

  /* check for our real-mode hooks */
  if (vect==0x31) {
    if (context->SegCs==DOSMEM_wrap_seg) {
      /* exit from real-mode wrapper */
      return -1;
    }
    /* we could probably move some other dodgy stuff here too from dpmi.c */
  }
  /* check if the call is from our fake BIOS interrupt stubs */
  if ((context->SegCs==0xf000) && !inwine) {
    if (vect != (context->Eip/4)) {
      TRACE_(int)("something fishy going on here (interrupt stub is %02lx)\n", context->Eip/4);
    }
    TRACE_(int)("builtin interrupt %02x has been branched to\n", vect);
    INT_RealModeInterrupt(vect, context);
  }
  /* check if the call goes to an unhooked interrupt */
  else if (SELECTOROF(handler)==0xf000) {
    /* if so, call it directly */
    TRACE_(int)("builtin interrupt %02x has been invoked (through vector %02x)\n", OFFSETOF(handler)/4, vect);
    INT_RealModeInterrupt(OFFSETOF(handler)/4, context);
  }
  /* the interrupt is hooked, simulate interrupt in DOS space */
  else {
    WORD*stack= PTR_REAL_TO_LIN( context->SegSs, context->Esp );
    WORD flag=LOWORD(context->EFlags);

    if (IF_ENABLED(context)) flag|=IF_MASK;
    else flag&=~IF_MASK;

    *(--stack)=flag;
    *(--stack)=context->SegCs;
    *(--stack)=LOWORD(context->Eip);
    context->Esp-=6;
    context->SegCs=SELECTOROF(handler);
    context->Eip=OFFSETOF(handler);
    IF_CLR(context);
  }
  return 0;
}

#define SHOULD_PEND(x) \
  (x && ((!current_event) || (x->priority < current_event->priority)))

static void DOSVM_SendQueuedEvent(CONTEXT86 *context)
{
  LPDOSEVENT event = pending_event;

  if (SHOULD_PEND(event)) {
    /* remove from "pending" list */
    pending_event = event->next;
    /* process event */
    if (event->irq>=0) {
      /* it's an IRQ, move it to "current" list */
      event->next = current_event;
      current_event = event;
      TRACE_(int)("dispatching IRQ %d\n",event->irq);
      /* note that if DOSVM_SimulateInt calls an internal interrupt directly,
       * current_event might be cleared (and event freed) in this very call! */
      DOSVM_SimulateInt((event->irq<8)?(event->irq+8):(event->irq-8+0x70),context,TRUE);
    } else {
      /* callback event */
      TRACE_(int)("dispatching callback event\n");
      (*event->relay)(context,event->data);
      free(event);
    }
  }
  if (!SHOULD_PEND(pending_event)) {
    TRACE_(int)("clearing Pending flag\n");
    CLR_PEND(context);
  }
}

static void DOSVM_SendQueuedEvents(CONTEXT86 *context)
{
  /* we will send all queued events as long as interrupts are enabled,
   * but IRQ events will disable interrupts again */
  while (IS_PEND(context) && IF_ENABLED(context))
    DOSVM_SendQueuedEvent(context);
}

void DOSVM_QueueEvent( int irq, int priority, void (*relay)(CONTEXT86*,void*), void *data)
{
  LPDOSEVENT event, cur, prev;

  if (entered) {
    event = malloc(sizeof(DOSEVENT));
    if (!event) {
      ERR_(int)("out of memory allocating event entry\n");
      return;
    }
    event->irq = irq; event->priority = priority;
    event->relay = relay; event->data = data;

    /* insert event into linked list, in order *after*
     * all earlier events of higher or equal priority */
    cur = pending_event; prev = NULL;
    while (cur && cur->priority<=priority) {
      prev = cur;
      cur = cur->next;
    }
    event->next = cur;
    if (prev) prev->next = event;
    else pending_event = event;
    
    /* get dosmod's attention to the new event, if necessary */
    if (!sig_sent) {
      TRACE_(int)("new event queued, signalling dosmod\n");
      kill(dosmod_pid,SIGUSR2);
      sig_sent++;
    } else {
      TRACE_(int)("new event queued\n");
    }
  } else {
    /* DOS subsystem not running */
    /* (this probably means that we're running a win16 app
     *  which uses DPMI to thunk down to DOS services) */
    if (irq<0) {
      /* callback event, perform it with dummy context */
      CONTEXT86 context;
      memset(&context,0,sizeof(context));
      (*relay)(&context,data);
    } else {
      ERR_(int)("IRQ without DOS task: should not happen");
    }
  }
}

#define CV do { CP(eax,Eax); CP(ecx,Ecx); CP(edx,Edx); CP(ebx,Ebx); \
           CP(esi,Esi); CP(edi,Edi); CP(esp,Esp); CP(ebp,Ebp); \
           CP(cs,SegCs); CP(ds,SegDs); CP(es,SegEs); \
           CP(ss,SegSs); CP(fs,SegFs); CP(gs,SegGs); \
           CP(eip,Eip); CP(eflags,EFlags); } while(0)

static int DOSVM_Process( int fn, int sig, struct vm86plus_struct*VM86 )
{
 CONTEXT86 context;
 int ret=0;

#define CP(x,y) context.y = VM86->regs.x
  CV;
#undef CP
 if (VM86_TYPE(fn)==VM86_UNKNOWN) {
  ret=INSTR_EmulateInstruction(&context);
#define CP(x,y) VM86->regs.x = context.y
  CV;
#undef CP
  if (ret) return 0;
  ret=0;
 }
#ifdef TRY_PICRETURN
 if (VM86->vm86plus.force_return_for_pic) {
   SET_PEND(&context);
 }
#else
 /* linux doesn't preserve pending flag on return */
 if (SHOULD_PEND(pending_event)) {
   SET_PEND(&context);
 }
#endif

 switch (VM86_TYPE(fn)) {
  case VM86_SIGNAL:
   TRACE_(int)("DOS module caught signal %d\n",sig);
   if ((sig==SIGALRM) || (sig==SIGUSR2)) {
     if (sig==SIGALRM) {
       sig_sent++;
       DOSVM_QueueEvent(0,DOS_PRIORITY_REALTIME,NULL,NULL);
     }
     if (pending_event) {
       TRACE_(int)("setting Pending flag, interrupts are currently %s\n",
                 IF_ENABLED(&context) ? "enabled" : "disabled");
       SET_PEND(&context);
       DOSVM_SendQueuedEvents(&context);
     } else {
       TRACE_(int)("no events are pending, clearing Pending flag\n");
       CLR_PEND(&context);
     }
     sig_sent--;
   }
   else if ((sig==SIGHUP) || (sig==SIGILL) || (sig==SIGSEGV)) {
       do_exception( sig, &context );
   } else {
    DOSVM_Dump(fn,sig,VM86);
    ret=-1;
   }
   break;
  case VM86_UNKNOWN: /* unhandled GPF */
   DOSVM_Dump(fn,sig,VM86);
   do_exception( SIGSEGV, &context );
   break;
  case VM86_INTx:
   if (TRACE_ON(relay))
    DPRINTF("Call DOS int 0x%02x (EAX=%08lx) ret=%04lx:%04lx\n",VM86_ARG(fn),context.Eax,context.SegCs,context.Eip);
   ret=DOSVM_SimulateInt(VM86_ARG(fn),&context,FALSE);
   if (TRACE_ON(relay))
    DPRINTF("Ret  DOS int 0x%02x (EAX=%08lx) ret=%04lx:%04lx\n",VM86_ARG(fn),context.Eax,context.SegCs,context.Eip);
   break;
  case VM86_STI:
    IF_SET(&context);
  /* case VM86_PICRETURN: */
    TRACE_(int)("DOS task enabled interrupts %s events pending, sending events\n", IS_PEND(&context)?"with":"without");
    DOSVM_SendQueuedEvents(&context);
    break;
  case VM86_TRAP:
   do_exception( SIGTRAP, &context );
   break;
  default:
   DOSVM_Dump(fn,sig,VM86);
   ret=-1;
 }

#define CP(x,y) VM86->regs.x = context.y
 CV;
#undef CP
#ifdef TRY_PICRETURN
 VM86->vm86plus.force_return_for_pic = IS_PEND(&context) ? 1 : 0;
 CLR_PEND(&context);
#endif
 return ret;
}

static void DOSVM_ProcessConsole(void)
{
  INPUT_RECORD msg;
  DWORD res;
  BYTE scan;

  if (ReadConsoleInputA(GetStdHandle(STD_INPUT_HANDLE),&msg,1,&res)) {
    switch (msg.EventType) {
    case KEY_EVENT:
      scan = msg.Event.KeyEvent.wVirtualScanCode;
      if (!msg.Event.KeyEvent.bKeyDown) scan |= 0x80;

      /* check whether extended bit is set,
       * and if so, queue the extension prefix */
      if (msg.Event.KeyEvent.dwControlKeyState & ENHANCED_KEY) {
        INT_Int09SendScan(0xE0,0);
      }
      INT_Int09SendScan(scan,msg.Event.KeyEvent.uChar.AsciiChar);
      break;
    default:
      FIXME_(int)("unhandled console event: %d\n", msg.EventType);
    }
  }
}

static void DOSVM_ProcessMessage(MSG *msg)
{
  BYTE scan = 0;

  TRACE_(int)("got message %04x, wparam=%08x, lparam=%08lx\n",msg->message,msg->wParam,msg->lParam);
  if ((msg->message>=WM_MOUSEFIRST)&&
      (msg->message<=WM_MOUSELAST)) {
    INT_Int33Message(msg->message,msg->wParam,msg->lParam);
  } else {
    switch (msg->message) {
    case WM_KEYUP:
      scan = 0x80;
    case WM_KEYDOWN:
      scan |= (msg->lParam >> 16) & 0x7f;

      /* check whether extended bit is set,
       * and if so, queue the extension prefix */
      if (msg->lParam & 0x1000000) {
	/* FIXME: some keys (function keys) have
	 * extended bit set even when they shouldn't,
	 * should check for them */
	INT_Int09SendScan(0xE0,0);
      }
      INT_Int09SendScan(scan,0);
      break;
    }
  }
}

void DOSVM_Wait( int read_pipe, HANDLE hObject )
{
  MSG msg;
  DWORD waitret;
  HANDLE objs[2];
  int objc;
  BOOL got_msg = FALSE;

  objs[0]=GetStdHandle(STD_INPUT_HANDLE);
  objs[1]=hObject;
  objc=hObject?2:1;
  do {
    /* check for messages (waste time before the response check below) */
    if (Callout.PeekMessageA)
    {
        while (Callout.PeekMessageA(&msg,0,0,0,PM_REMOVE|PM_NOYIELD)) {
            /* got a message */
            DOSVM_ProcessMessage(&msg);
            /* we don't need a TranslateMessage here */
            Callout.DispatchMessageA(&msg);
            got_msg = TRUE;
        }
    }
    if (!got_msg) {
      /* check for console input */
      INPUT_RECORD msg;
      DWORD num;
      if (PeekConsoleInputA(objs[0],&msg,1,&num) && num) {
        DOSVM_ProcessConsole();
        got_msg = TRUE;
      }
    }
    if (read_pipe == -1) {
      if (got_msg) break;
    } else {
      fd_set readfds;
      struct timeval timeout={0,0};
      /* quick check for response from dosmod
       * (faster than doing the full blocking wait, if data already available) */
      FD_ZERO(&readfds); FD_SET(read_pipe,&readfds);
      if (select(read_pipe+1,&readfds,NULL,NULL,&timeout)>0)
	break;
    }
    /* nothing yet, block while waiting for something to do */
    if (Callout.MsgWaitForMultipleObjects)
        waitret = Callout.MsgWaitForMultipleObjects(objc,objs,FALSE,INFINITE,QS_ALLINPUT);
    else
        waitret = WaitForMultipleObjects(objc,objs,FALSE,INFINITE);

    if (waitret==(DWORD)-1) {
      ERR_(module)("dosvm wait error=%ld\n",GetLastError());
    }
    if ((read_pipe != -1) && hObject) {
      if (waitret==(WAIT_OBJECT_0+1)) break;
    }
    if (waitret==WAIT_OBJECT_0) {
      DOSVM_ProcessConsole();
    }
  } while (TRUE);
}

int DOSVM_Enter( CONTEXT86 *context )
{
 struct vm86plus_struct VM86;
 int stat,len,sig;

 memset(&VM86, 0, sizeof(VM86));
#define CP(x,y) VM86.regs.x = context->y
  CV;
#undef CP
  if (VM86.regs.eflags & IF_MASK)
    VM86.regs.eflags |= VIF_MASK;

 /* main exchange loop */
 entered++;
 do {
  TRACE_(module)("thread is: %lx\n",GetCurrentThreadId());
  stat = VM86_ENTER;
  errno = 0;
  /* transmit VM86 structure to dosmod task */
  if (write(write_pipe,&stat,sizeof(stat))!=sizeof(stat)) {
   ERR_(module)("dosmod sync lost, errno=%d, fd=%d, pid=%d\n",errno,write_pipe,getpid());
   return -1;
  }
  if (write(write_pipe,&VM86,sizeof(VM86))!=sizeof(VM86)) {
   ERR_(module)("dosmod sync lost, errno=%d\n",errno);
   return -1;
  }
  /* wait for response, doing other things in the meantime */
  DOSVM_Wait(read_pipe, hReadPipe);
  /* read response */
  while (1) {
    if ((len=read(read_pipe,&stat,sizeof(stat)))==sizeof(stat)) break;
    if (((errno==EINTR)||(errno==EAGAIN))&&(len<=0)) {
     WARN_(module)("rereading dosmod return code due to errno=%d, result=%d\n",errno,len);
     continue;
    }
    ERR_(module)("dosmod sync lost reading return code, errno=%d, result=%d\n",errno,len);
    return -1;
  }
  TRACE_(module)("dosmod return code=%d\n",stat);
  if (stat==DOSMOD_LEFTIDLE) {
    continue;
  }
  while (1) {
    if ((len=read(read_pipe,&VM86,sizeof(VM86)))==sizeof(VM86)) break;
    if (((errno==EINTR)||(errno==EAGAIN))&&(len<=0)) {
     WARN_(module)("rereading dosmod VM86 structure due to errno=%d, result=%d\n",errno,len);
     continue;
    }
    ERR_(module)("dosmod sync lost reading VM86 structure, errno=%d, result=%d\n",errno,len);
    return -1;
  }
  if ((stat&0xff)==DOSMOD_SIGNAL) {
    while (1) {
      if ((len=read(read_pipe,&sig,sizeof(sig)))==sizeof(sig)) break;
      if (((errno==EINTR)||(errno==EAGAIN))&&(len<=0)) {
	WARN_(module)("rereading dosmod signal due to errno=%d, result=%d\n",errno,len);
	continue;
      }
      ERR_(module)("dosmod sync lost reading signal, errno=%d, result=%d\n",errno,len);
      return -1;
    } while (0);
  } else sig=0;
  /* got response */
 } while (DOSVM_Process(stat,sig,&VM86)>=0);
 entered--;

#define CP(x,y) context->y = VM86.regs.x
  CV;
#undef CP
 return 0;
}

void DOSVM_PIC_ioport_out( WORD port, BYTE val)
{
    LPDOSEVENT event;

    if ((port==0x20) && (val==0x20)) {
      if (current_event) {
	/* EOI (End Of Interrupt) */
	TRACE_(int)("received EOI for current IRQ, clearing\n");
	event = current_event;
	current_event = event->next;
	if (event->relay)
	(*event->relay)(NULL,event->data);
	free(event);

	if (pending_event &&
	    !sig_sent) {
	  /* another event is pending, which we should probably
	   * be able to process now, so tell dosmod about it */
	  TRACE_(int)("another event pending, signalling dosmod\n");
	  kill(dosmod_pid,SIGUSR2);
	  sig_sent++;
	}
      } else {
	WARN_(int)("EOI without active IRQ\n");
      }
    } else {
      FIXME_(int)("unrecognized PIC command %02x\n",val);
    }
}

void DOSVM_SetTimer( unsigned ticks )
{
  int stat=DOSMOD_SET_TIMER;
  struct timeval tim;

  if (MZ_Current()) {
    /* the PC clocks ticks at 1193180 Hz */
    tim.tv_sec=0;
    tim.tv_usec=MulDiv(ticks,1000000,1193180);
    /* sanity check */
    if (!tim.tv_usec) tim.tv_usec=1;

    if (write(write_pipe,&stat,sizeof(stat))!=sizeof(stat)) {
      ERR_(module)("dosmod sync lost, errno=%d\n",errno);
      return;
    }
    if (write(write_pipe,&tim,sizeof(tim))!=sizeof(tim)) {
      ERR_(module)("dosmod sync lost, errno=%d\n",errno);
      return;
    }
    /* there's no return */
  }
}

unsigned DOSVM_GetTimer( void )
{
  int stat=DOSMOD_GET_TIMER;
  struct timeval tim;

  if (MZ_Current()) {
    if (write(write_pipe,&stat,sizeof(stat))!=sizeof(stat)) {
      ERR_(module)("dosmod sync lost, errno=%d\n",errno);
      return 0;
    }
    /* read response */
    while (1) {
      if (read(read_pipe,&tim,sizeof(tim))==sizeof(tim)) break;
      if ((errno==EINTR)||(errno==EAGAIN)) continue;
      ERR_(module)("dosmod sync lost, errno=%d\n",errno);
      return 0;
    }
    return MulDiv(tim.tv_usec,1193180,1000000);
  }
  return 0;
}

#else /* !MZ_SUPPORTED */

int DOSVM_Enter( CONTEXT86 *context )
{
 ERR_(module)("DOS realmode not supported on this architecture!\n");
 return -1;
}

void DOSVM_Wait( int read_pipe, HANDLE hObject) {}
void DOSVM_PIC_ioport_out( WORD port, BYTE val) {}
void DOSVM_SetTimer( unsigned ticks ) {}
unsigned DOSVM_GetTimer( void ) { return 0; }
void DOSVM_QueueEvent( int irq, int priority, void (*relay)(CONTEXT86*,void*), void *data)
{
  if (irq<0) {
    /* callback event, perform it with dummy context */
    CONTEXT86 context;
    memset(&context,0,sizeof(context));
    (*relay)(&context,data);
  } else {
    ERR_(int)("IRQ without DOS task: should not happen");
  }
}

#endif
