/*
 * DOS Virtual Machine
 *
 * Copyright 1998 Ove Kåven
 *
 * This code hasn't been completely cleaned up yet.
 */

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
#include "winuser.h"
#include "winnt.h"
#include "sig_context.h"
#include "msdos.h"
#include "file.h"
#include "miscemu.h"
#include "debugger.h"
#include "module.h"
#include "task.h"
#include "ldt.h"
#include "dosexe.h"
#include "dosmod.h"
#include "debug.h"

#ifdef MZ_SUPPORTED

#include <sys/mman.h>
#include <sys/vm86.h>

#define IF_CLR(ctx) EFL_reg(ctx) &= ~VIF_MASK
#define IF_ENABLED(ctx) (EFL_reg(ctx) & VIF_MASK)
#define SET_PEND(ctx) EFL_reg(ctx) |= VIP_MASK
#define CLR_PEND(ctx) EFL_reg(ctx) &= ~VIP_MASK
#define IS_PEND(ctx) (EFL_reg(ctx) & VIP_MASK)

#undef TRY_PICRETURN

static void DOSVM_Dump( LPDOSTASK lpDosTask, int fn, int sig,
                        struct vm86plus_struct*VM86 )
{
 unsigned iofs;
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

 iofs=((DWORD)REGS.cs<<4)+REGS.eip;
#undef REGS
 inst=(BYTE*)lpDosTask->img+iofs;
 printf("Opcodes:");
 for (x=0; x<8; x++) printf(" %02x",inst[x]);
 printf("\n");
}

static int DOSVM_Int( int vect, PCONTEXT context, LPDOSTASK lpDosTask )
{
 extern UINT16 DPMI_wrap_seg;

 if (vect==0x31) {
  if (CS_reg(context)==DPMI_wrap_seg) {
   /* exit from real-mode wrapper */
   return -1;
  }
  /* we could probably move some other dodgy stuff here too from dpmi.c */
 }
 INT_RealModeInterrupt(vect,context);
 return 0;
}

static void DOSVM_SimulateInt( int vect, PCONTEXT context, LPDOSTASK lpDosTask )
{
  FARPROC16 handler=INT_GetRMHandler(vect);

  if (SELECTOROF(handler)==0xf000) {
    /* if internal interrupt, call it directly */
    INT_RealModeInterrupt(vect,context);
  } else {
    WORD*stack=(WORD*)(V86BASE(context)+(((DWORD)SS_reg(context))<<4)+SP_reg(context));
    WORD flag=FL_reg(context);

    if (IF_ENABLED(context)) flag|=IF_MASK;
    else flag&=~IF_MASK;

    *(--stack)=flag;
    *(--stack)=CS_reg(context);
    *(--stack)=IP_reg(context);
    SP_reg(context)-=6;
    CS_reg(context)=SELECTOROF(handler);
    IP_reg(context)=OFFSETOF(handler);
    IF_CLR(context);
  }
}

#define SHOULD_PEND(x) \
  (x && ((!lpDosTask->current) || (x->priority < lpDosTask->current->priority)))

static void DOSVM_SendQueuedEvent(PCONTEXT context, LPDOSTASK lpDosTask)
{
  LPDOSEVENT event = lpDosTask->pending;

  if (SHOULD_PEND(event)) {
    /* remove from "pending" list */
    lpDosTask->pending = event->next;
    /* process event */
    if (event->irq>=0) {
      /* it's an IRQ, move it to "current" list */
      event->next = lpDosTask->current;
      lpDosTask->current = event;
      TRACE(int,"dispatching IRQ %d\n",event->irq);
      /* note that if DOSVM_SimulateInt calls an internal interrupt directly,
       * lpDosTask->current might be cleared (and event freed) in this very call! */
      DOSVM_SimulateInt((event->irq<8)?(event->irq+8):(event->irq-8+0x70),context,lpDosTask);
    } else {
      /* callback event */
      TRACE(int,"dispatching callback event\n");
      (*event->relay)(lpDosTask,context,event->data);
      free(event);
    }
  }
  if (!SHOULD_PEND(lpDosTask->pending)) {
    TRACE(int,"clearing Pending flag\n");
    CLR_PEND(context);
  }
}

static void DOSVM_SendQueuedEvents(PCONTEXT context, LPDOSTASK lpDosTask)
{
  /* we will send all queued events as long as interrupts are enabled,
   * but IRQ events will disable interrupts again */
  while (IS_PEND(context) && IF_ENABLED(context))
    DOSVM_SendQueuedEvent(context,lpDosTask);
}

void DOSVM_QueueEvent( int irq, int priority, void (*relay)(LPDOSTASK,PCONTEXT,void*), void *data)
{
  TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
  NE_MODULE *pModule = NE_GetPtr( pTask->hModule );
  LPDOSEVENT event, cur, prev;

  GlobalUnlock16( GetCurrentTask() );
  if (pModule && pModule->lpDosTask) {
    event = malloc(sizeof(DOSEVENT));
    if (!event) {
      ERR(int,"out of memory allocating event entry\n");
      return;
    }
    event->irq = irq; event->priority = priority;
    event->relay = relay; event->data = data;

    /* insert event into linked list, in order *after*
     * all earlier events of higher or equal priority */
    cur = pModule->lpDosTask->pending; prev = NULL;
    while (cur && cur->priority<=priority) {
      prev = cur;
      cur = cur->next;
    }
    event->next = cur;
    if (prev) prev->next = event;
    else pModule->lpDosTask->pending = event;
    
    /* get dosmod's attention to the new event, except for irq==0 where we already have it */
    if (irq && !pModule->lpDosTask->sig_sent) {
      TRACE(int,"new event queued, signalling dosmod\n");
      kill(pModule->lpDosTask->task,SIGUSR2);
      pModule->lpDosTask->sig_sent++;
    } else {
      TRACE(int,"new event queued\n");
    }
  }
}

#define CV CP(eax,EAX); CP(ecx,ECX); CP(edx,EDX); CP(ebx,EBX); \
           CP(esi,ESI); CP(edi,EDI); CP(esp,ESP); CP(ebp,EBP); \
           CP(cs,CS); CP(ds,DS); CP(es,ES); \
           CP(ss,SS); CP(fs,FS); CP(gs,GS); \
           CP(eip,EIP); CP(eflags,EFL)

static int DOSVM_Process( LPDOSTASK lpDosTask, int fn, int sig,
                          struct vm86plus_struct*VM86 )
{
 SIGCONTEXT sigcontext;
 CONTEXT context;
 int ret=0;

 if (VM86_TYPE(fn)==VM86_UNKNOWN) {
  /* INSTR_EmulateInstruction needs a SIGCONTEXT, not a CONTEXT... */
#define CP(x,y) y##_sig(&sigcontext) = VM86->regs.x
  CV;
#undef CP
  if (fnINSTR_EmulateInstruction) ret=fnINSTR_EmulateInstruction(&sigcontext);
#define CP(x,y) VM86->regs.x = y##_sig(&sigcontext)
  CV;
#undef CP
  if (ret) return 0;
  ret=0;
 }
#define CP(x,y) y##_reg(&context) = VM86->regs.x
 CV;
#undef CP
 (void*)V86BASE(&context)=lpDosTask->img;
#ifdef TRY_PICRETURN
 if (VM86->vm86plus.force_return_for_pic) {
   SET_PEND(&context);
 }
#else
 /* linux doesn't preserve pending flag on return */
 if (SHOULD_PEND(lpDosTask->pending)) {
   SET_PEND(&context);
 }
#endif

 switch (VM86_TYPE(fn)) {
  case VM86_SIGNAL:
   TRACE(int,"DOS module caught signal %d\n",sig);
   if ((sig==SIGALRM) || (sig==SIGUSR2)) {
     if (sig==SIGALRM) {
       DOSVM_QueueEvent(0,DOS_PRIORITY_REALTIME,NULL,NULL);
     }
     if (lpDosTask->pending) {
       TRACE(int,"setting Pending flag, interrupts are currently %s\n",
                 IF_ENABLED(&context) ? "enabled" : "disabled");
       SET_PEND(&context);
       DOSVM_SendQueuedEvents(&context,lpDosTask);
     } else {
       TRACE(int,"no events are pending, clearing Pending flag\n");
       CLR_PEND(&context);
     }
     if (sig==SIGUSR2) lpDosTask->sig_sent--;
   } else
   if (sig==SIGHUP) {
    if (ctx_debug_call) ctx_debug_call(SIGTRAP,&context);
   } else
   if ((sig==SIGILL)||(sig==SIGSEGV)) {
    if (ctx_debug_call) ctx_debug_call(SIGILL,&context);
   } else {
    DOSVM_Dump(lpDosTask,fn,sig,VM86);
    ret=-1;
   }
   break;
  case VM86_UNKNOWN: /* unhandled GPF */
   DOSVM_Dump(lpDosTask,fn,sig,VM86);
   if (ctx_debug_call) ctx_debug_call(SIGSEGV,&context); else ret=-1;
   break;
  case VM86_INTx:
   if (TRACE_ON(relay))
    DPRINTF("Call DOS int 0x%02x (EAX=%08lx) ret=%04lx:%04lx\n",VM86_ARG(fn),context.Eax,context.SegCs,context.Eip);
   ret=DOSVM_Int(VM86_ARG(fn),&context,lpDosTask);
   if (TRACE_ON(relay))
    DPRINTF("Ret  DOS int 0x%02x (EAX=%08lx) ret=%04lx:%04lx\n",VM86_ARG(fn),context.Eax,context.SegCs,context.Eip);
   break;
  case VM86_STI:
  case VM86_PICRETURN:
    TRACE(int,"DOS task enabled interrupts with events pending, sending events\n");
    DOSVM_SendQueuedEvents(&context,lpDosTask);
    break;
  case VM86_TRAP:
   if (ctx_debug_call) ctx_debug_call(SIGTRAP,&context);
   break;
  default:
   DOSVM_Dump(lpDosTask,fn,sig,VM86);
   ret=-1;
 }

#define CP(x,y) VM86->regs.x = y##_reg(&context)
 CV;
#undef CP
#ifdef TRY_PICRETURN
 VM86->vm86plus.force_return_for_pic = IS_PEND(&context) ? 1 : 0;
 CLR_PEND(&context);
#endif
 return ret;
}

void DOSVM_ProcessMessage(LPDOSTASK lpDosTask,MSG *msg)
{
  BYTE scan = 0;

  fprintf(stderr,"got message %04x, wparam=%08x, lparam=%08lx\n",msg->message,msg->wParam,msg->lParam);
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
	INT_Int09SendScan(0xE0);
      }
      INT_Int09SendScan(scan);
      break;
    }
  }
}

int DOSVM_Enter( PCONTEXT context )
{
 TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
 NE_MODULE *pModule = NE_GetPtr( pTask->hModule );
 LPDOSTASK lpDosTask;
 struct vm86plus_struct VM86;
 int stat,len,sig;
 DWORD waitret;
 MSG msg;
 fd_set readfds;
 struct timeval timeout={0,0};

 GlobalUnlock16( GetCurrentTask() );
 if (!pModule) {
  ERR(module,"No task is currently active!\n");
  return -1;
 }
 if (!(lpDosTask=pModule->lpDosTask)) {
  /* MZ_CreateProcess or MZ_AllocDPMITask should have been called first */
  ERR(module,"dosmod has not been initialized!");
  return -1;
 }

 if (context) {
#define CP(x,y) VM86.regs.x = y##_reg(context)
  CV;
#undef CP
 } else {
/* initial setup */
  /* allocate standard DOS handles */
  FILE_InitProcessDosHandles(); 
  /* registers */
  memset(&VM86,0,sizeof(VM86));
  VM86.regs.cs=lpDosTask->init_cs;
  VM86.regs.eip=lpDosTask->init_ip;
  VM86.regs.ss=lpDosTask->init_ss;
  VM86.regs.esp=lpDosTask->init_sp;
  VM86.regs.ds=lpDosTask->psp_seg;
  VM86.regs.es=lpDosTask->psp_seg;
  VM86.regs.eflags=VIF_MASK;
  /* hmm, what else do we need? */
 }

 /* main exchange loop */
 do {
  stat = VM86_ENTER;
  errno = 0;
  /* transmit VM86 structure to dosmod task */
  if (write(lpDosTask->write_pipe,&stat,sizeof(stat))!=sizeof(stat)) {
   ERR(module,"dosmod sync lost, errno=%d, fd=%d, pid=%d\n",errno,lpDosTask->write_pipe,getpid());
   return -1;
  }
  if (write(lpDosTask->write_pipe,&VM86,sizeof(VM86))!=sizeof(VM86)) {
   ERR(module,"dosmod sync lost, errno=%d\n",errno);
   return -1;
  }
  do {
    /* check for messages (waste time before the response check below) */
    while (PeekMessageA(&msg,0,0,0,PM_REMOVE|PM_NOYIELD)) {
      /* got a message */
      DOSVM_ProcessMessage(lpDosTask,&msg);
      /* we don't need a TranslateMessage here */
      DispatchMessageA(&msg);
    }
    /* quick check for response from dosmod
     * (faster than doing the full blocking wait, if data already available) */
    FD_ZERO(&readfds); FD_SET(lpDosTask->read_pipe,&readfds);
    if (select(lpDosTask->read_pipe+1,&readfds,NULL,NULL,&timeout)>0)
      break;
    /* nothing yet, block while waiting for something to do */
    waitret=MsgWaitForMultipleObjects(1,&(lpDosTask->hReadPipe),FALSE,INFINITE,QS_ALLINPUT);
    if (waitret==(DWORD)-1) {
      ERR(module,"dosvm wait error=%ld\n",GetLastError());
    }
  } while (waitret!=WAIT_OBJECT_0);
  /* read response */
  while (1) {
    if ((len=read(lpDosTask->read_pipe,&stat,sizeof(stat)))==sizeof(stat)) break;
    if (((errno==EINTR)||(errno==EAGAIN))&&(len<=0)) {
     WARN(module,"rereading dosmod return code due to errno=%d, result=%d\n",errno,len);
     continue;
    }
    ERR(module,"dosmod sync lost reading return code, errno=%d, result=%d\n",errno,len);
    return -1;
  }
  TRACE(module,"dosmod return code=%d\n",stat);
  while (1) {
    if ((len=read(lpDosTask->read_pipe,&VM86,sizeof(VM86)))==sizeof(VM86)) break;
    if (((errno==EINTR)||(errno==EAGAIN))&&(len<=0)) {
     WARN(module,"rereading dosmod VM86 structure due to errno=%d, result=%d\n",errno,len);
     continue;
    }
    ERR(module,"dosmod sync lost reading VM86 structure, errno=%d, result=%d\n",errno,len);
    return -1;
  }
  if ((stat&0xff)==DOSMOD_SIGNAL) {
    while (1) {
      if ((len=read(lpDosTask->read_pipe,&sig,sizeof(sig)))==sizeof(sig)) break;
      if (((errno==EINTR)||(errno==EAGAIN))&&(len<=0)) {
	WARN(module,"rereading dosmod signal due to errno=%d, result=%d\n",errno,len);
	continue;
      }
      ERR(module,"dosmod sync lost reading signal, errno=%d, result=%d\n",errno,len);
      return -1;
    } while (0);
  } else sig=0;
  /* got response */
 } while (DOSVM_Process(lpDosTask,stat,sig,&VM86)>=0);

 if (context) {
#define CP(x,y) y##_reg(context) = VM86.regs.x
  CV;
#undef CP
 }
 return 0;
}

void DOSVM_PIC_ioport_out( WORD port, BYTE val)
{
  TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
  NE_MODULE *pModule = NE_GetPtr( pTask->hModule );
  LPDOSEVENT event;

  GlobalUnlock16( GetCurrentTask() );
  if (pModule && pModule->lpDosTask) {
    if ((port==0x20) && (val==0x20)) {
      if (pModule->lpDosTask->current) {
	/* EOI (End Of Interrupt) */
	TRACE(int,"received EOI for current IRQ, clearing\n");
	event = pModule->lpDosTask->current;
	pModule->lpDosTask->current = event->next;
	if (event->relay)
	(*event->relay)(pModule->lpDosTask,NULL,event->data);
	free(event);

	if (pModule->lpDosTask->pending &&
	    !pModule->lpDosTask->sig_sent) {
	  /* another event is pending, which we should probably
	   * be able to process now, so tell dosmod about it */
	  TRACE(int,"another event pending, signalling dosmod\n");
	  kill(pModule->lpDosTask->task,SIGUSR2);
	  pModule->lpDosTask->sig_sent++;
	}
      } else {
	WARN(int,"EOI without active IRQ\n");
      }
    } else {
      FIXME(int,"unrecognized PIC command %02x\n",val);
    }
  }
}

void DOSVM_SetTimer( unsigned ticks )
{
 TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
 NE_MODULE *pModule = NE_GetPtr( pTask->hModule );
 int stat=DOSMOD_SET_TIMER;
 struct timeval tim;

 GlobalUnlock16( GetCurrentTask() );
 if (pModule&&pModule->lpDosTask) {
  /* the PC clocks ticks at 1193180 Hz */
  tim.tv_sec=0;
  tim.tv_usec=((unsigned long long)ticks*1000000)/1193180;
  /* sanity check */
  if (!tim.tv_usec) tim.tv_usec=1;

  if (write(pModule->lpDosTask->write_pipe,&stat,sizeof(stat))!=sizeof(stat)) {
   ERR(module,"dosmod sync lost, errno=%d\n",errno);
   return;
  }
  if (write(pModule->lpDosTask->write_pipe,&tim,sizeof(tim))!=sizeof(tim)) {
   ERR(module,"dosmod sync lost, errno=%d\n",errno);
   return;
  }
  /* there's no return */
 }
}

unsigned DOSVM_GetTimer( void )
{
 TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
 NE_MODULE *pModule = NE_GetPtr( pTask->hModule );
 int stat=DOSMOD_GET_TIMER;
 struct timeval tim;

 GlobalUnlock16( GetCurrentTask() );
 if (pModule&&pModule->lpDosTask) {
  if (write(pModule->lpDosTask->write_pipe,&stat,sizeof(stat))!=sizeof(stat)) {
   ERR(module,"dosmod sync lost, errno=%d\n",errno);
   return 0;
  }
  /* read response */
  if (read(pModule->lpDosTask->read_pipe,&tim,sizeof(tim))!=sizeof(tim)) {
   ERR(module,"dosmod sync lost, errno=%d\n",errno);
   return 0;
  }
  return ((unsigned long long)tim.tv_usec*1193180)/1000000;
 }
 return 0;
}

void DOSVM_SetSystemData( int id, void *data )
{
  TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
  NE_MODULE *pModule = NE_GetPtr( pTask->hModule );
  DOSSYSTEM *sys, *prev;

  GlobalUnlock16( GetCurrentTask() );
  if (pModule && pModule->lpDosTask) {
    sys = pModule->lpDosTask->sys;
    prev = NULL;
    while (sys && (sys->id != id)) {
      prev = sys;
      sys = sys->next;
    }
    if (sys) {
      free(sys->data);
      sys->data = data;
    } else {
      sys = malloc(sizeof(DOSSYSTEM));
      sys->id = id;
      sys->data = data;
      sys->next = NULL;
      if (prev) prev->next = sys;
      else pModule->lpDosTask->sys = sys;
    }
  } else free(data);
}

void* DOSVM_GetSystemData( int id )
{
  TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
  NE_MODULE *pModule = NE_GetPtr( pTask->hModule );
  DOSSYSTEM *sys;

  GlobalUnlock16( GetCurrentTask() );
  if (pModule && pModule->lpDosTask) {
    sys = pModule->lpDosTask->sys;
    while (sys && (sys->id != id))
      sys = sys->next;
    if (sys)
      return sys->data;
  }
  return NULL;
}

#else /* !MZ_SUPPORTED */

int DOSVM_Enter( PCONTEXT context )
{
 ERR(module,"DOS realmode not supported on this architecture!\n");
 return -1;
}

void DOSVM_PIC_ioport_out( WORD port, BYTE val) {}
void DOSVM_SetTimer( unsigned ticks ) {}
unsigned DOSVM_GetTimer( void ) { return 0; }
void DOSVM_SetSystemData( int id, void *data ) { free(data); }
void* DOSVM_GetSystemData( int id ) { return NULL; }

#endif
