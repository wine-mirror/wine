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
#include "windows.h"
#include "winbase.h"
#include "winnt.h"
#include "sig_context.h"
#include "msdos.h"
#include "miscemu.h"
#include "debugger.h"
#include "debug.h"
#include "module.h"
#include "task.h"
#include "ldt.h"
#include "dosexe.h"
#include "dosmod.h"

void (*ctx_debug_call)(int sig,CONTEXT*ctx)=NULL;
BOOL32 (*instr_emu_call)(SIGCONTEXT*ctx)=NULL;

#ifdef MZ_SUPPORTED

#include <sys/mman.h>
#include <sys/vm86.h>

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
 WORD*stack=(WORD*)(V86BASE(context)+(((DWORD)SS_reg(context))<<4)+SP_reg(context));

 *(--stack)=FL_reg(context);
 *(--stack)=CS_reg(context);
 *(--stack)=IP_reg(context);
 SP_reg(context)-=6;
 CS_reg(context)=SELECTOROF(handler);
 IP_reg(context)=OFFSETOF(handler);
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
  if (instr_emu_call) ret=instr_emu_call(&sigcontext);
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

 switch (VM86_TYPE(fn)) {
  case VM86_SIGNAL:
   TRACE(int,"DOS module caught signal %d\n",sig);
   if (sig==SIGALRM) {
    DOSVM_SimulateInt(8,&context,lpDosTask);
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
   break;
  case VM86_PICRETURN:
   printf("Trapped due to pending PIC request\n"); break;
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
 return ret;
}

int DOSVM_Enter( PCONTEXT context )
{
 TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
 NE_MODULE *pModule = NE_GetPtr( pTask->hModule );
 LPDOSTASK lpDosTask;
 struct vm86plus_struct VM86;
 int stat,len,sig;
 fd_set readfds,exceptfds;

 GlobalUnlock16( GetCurrentTask() );
 if (!pModule) {
  ERR(module,"No task is currently active!\n");
  return -1;
 }
 if (!pModule->lpDosTask) {
  /* no VM86 (dosmod) task is currently running, start one */
  if ((lpDosTask = calloc(1, sizeof(DOSTASK))) == NULL)
    return 0;
  lpDosTask->hModule=pModule->self;
  stat=MZ_InitMemory(lpDosTask,pModule);
  if (stat>=32) stat=MZ_InitTask(lpDosTask);
  if (stat<32) {
   free(lpDosTask);
   return -1;
  }
  pModule->lpDosTask = lpDosTask;
  pModule->dos_image = lpDosTask->img;
  /* Note: we're leaving it running after this, in case we need it again,
     as this minimizes the overhead of starting it up every time...
     it will be killed automatically when the current task terminates */
 } else lpDosTask=pModule->lpDosTask;

 if (context) {
#define CP(x,y) VM86.regs.x = y##_reg(context)
  CV;
#undef CP
 } else {
/* initial setup */
  memset(&VM86,0,sizeof(VM86));
  VM86.regs.cs=lpDosTask->init_cs;
  VM86.regs.eip=lpDosTask->init_ip;
  VM86.regs.ss=lpDosTask->init_ss;
  VM86.regs.esp=lpDosTask->init_sp;
  VM86.regs.ds=lpDosTask->psp_seg;
  VM86.regs.es=lpDosTask->psp_seg;
  /* hmm, what else do we need? */
 }

 /* main exchange loop */
 do {
  stat = VM86_ENTER;
  errno = 0;
  /* transmit VM86 structure to dosmod task */
  if (write(lpDosTask->write_pipe,&stat,sizeof(stat))!=sizeof(stat)) {
   ERR(module,"dosmod sync lost, errno=%d\n",errno);
   return -1;
  }
  if (write(lpDosTask->write_pipe,&VM86,sizeof(VM86))!=sizeof(VM86)) {
   ERR(module,"dosmod sync lost, errno=%d\n",errno);
   return -1;
  }
  /* wait for response, with async events enabled */
  FD_ZERO(&readfds);
  FD_ZERO(&exceptfds);
  SIGNAL_MaskAsyncEvents(FALSE);
  do {
   FD_SET(lpDosTask->read_pipe,&readfds);
   FD_SET(lpDosTask->read_pipe,&exceptfds);
   select(lpDosTask->read_pipe+1,&readfds,NULL,&exceptfds,NULL);
  } while (!(FD_ISSET(lpDosTask->read_pipe,&readfds)||
             FD_ISSET(lpDosTask->read_pipe,&exceptfds)));
  SIGNAL_MaskAsyncEvents(TRUE);
  /* read response (with async events disabled to avoid some strange problems) */
  do {
   if ((len=read(lpDosTask->read_pipe,&stat,sizeof(stat)))!=sizeof(stat)) {
    if (((errno==EINTR)||(errno==EAGAIN))&&(len<=0)) {
     WARN(module,"rereading dosmod return code due to errno=%d, result=%d\n",errno,len);
     continue;
    }
    ERR(module,"dosmod sync lost reading return code, errno=%d, result=%d\n",errno,len);
    return -1;
   }
  } while (0);
  TRACE(module,"dosmod return code=%d\n",stat);
  do {
   if ((len=read(lpDosTask->read_pipe,&VM86,sizeof(VM86)))!=sizeof(VM86)) {
    if (((errno==EINTR)||(errno==EAGAIN))&&(len<=0)) {
     WARN(module,"rereading dosmod VM86 structure due to errno=%d, result=%d\n",errno,len);
     continue;
    }
    ERR(module,"dosmod sync lost reading VM86 structure, errno=%d, result=%d\n",errno,len);
    return -1;
   }
  } while (0);
  if ((stat&0xff)==DOSMOD_SIGNAL) {
   do {
    if ((len=read(lpDosTask->read_pipe,&sig,sizeof(sig)))!=sizeof(sig)) {
     if (((errno==EINTR)||(errno==EAGAIN))&&(len<=0)) {
      WARN(module,"rereading dosmod signal due to errno=%d, result=%d\n",errno,len);
      continue;
     }
     ERR(module,"dosmod sync lost reading signal, errno=%d, result=%d\n",errno,len);
     return -1;
    }
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

void MZ_Tick( WORD handle )
{
 /* find the DOS task that has the right system_timer handle... */
 /* should usually be the current, so let's just be lazy... */
 TDB *pTask = (TDB*)GlobalLock16( GetCurrentTask() );
 NE_MODULE *pModule = pTask ? NE_GetPtr( pTask->hModule ) : NULL;
 LPDOSTASK lpDosTask = pModule ? pModule->lpDosTask : NULL;

 GlobalUnlock16( GetCurrentTask() );
 if (lpDosTask&&(lpDosTask->system_timer==handle)) {
  /* BIOS timer tick */
  (*((DWORD*)(((BYTE*)(lpDosTask->img))+0x46c)))++;
 }
}

#else /* !MZ_SUPPORTED */

int DOSVM_Enter( PCONTEXT context )
{
 ERR(module,"DOS realmode not supported on this architecture!\n");
 return -1;
}

void MZ_Tick( WORD handle ) {}
void DOSVM_SetTimer( unsigned ticks ) {}
unsigned DOSVM_GetTimer( void ) { return 0; }

#endif
