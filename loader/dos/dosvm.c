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
#include <sys/types.h>
#include <sys/stat.h>
#include "windows.h"
#include "winbase.h"
#include "winnt.h"
#include "msdos.h"
#include "miscemu.h"
#include "debug.h"
#include "module.h"
#include "task.h"
#include "ldt.h"
#include "dosexe.h"

#ifdef MZ_SUPPORTED

static void DOSVM_Dump( LPDOSTASK lpDosTask)
{
 unsigned iofs;
 BYTE*inst;
 int x;

 switch (VM86_TYPE(lpDosTask->fn)) {
  case VM86_SIGNAL:
   printf("Trapped signal\n"); break;
  case VM86_UNKNOWN:
   printf("Trapped unhandled GPF\n"); break;
  case VM86_INTx:
   printf("Trapped INT %02x\n",VM86_ARG(lpDosTask->fn)); break;
  case VM86_STI:
   printf("Trapped STI\n"); break;
  case VM86_PICRETURN:
   printf("Trapped due to pending PIC request\n"); break;
  case VM86_TRAP:
   printf("Trapped debug request\n"); break;
 }
#define REGS lpDosTask->VM86.regs
 fprintf(stderr,"AX=%04lX CX=%04lX DX=%04lX BX=%04lX\n",REGS.eax,REGS.ebx,REGS.ecx,REGS.edx);
 fprintf(stderr,"SI=%04lX DI=%04lX SP=%04lX BP=%04lX\n",REGS.esi,REGS.edi,REGS.esp,REGS.ebp);
 fprintf(stderr,"CS=%04X DS=%04X ES=%04X SS=%04X\n",REGS.cs,REGS.ds,REGS.es,REGS.ss);
 fprintf(stderr,"EIP=%04lX EFLAGS=%08lX\n",REGS.eip,REGS.eflags);

 iofs=((DWORD)REGS.cs<<4)+REGS.eip;
#undef REGS
 inst=(BYTE*)lpDosTask->img+iofs;
 printf("Opcodes:");
 for (x=0; x<8; x++) printf(" %02x",inst[x]);
 printf("\n");

 exit(0);
}

static int DOSVM_Int(int vect, PCONTEXT context )
{
 /* moved to INT_RealModeInterrupt in msdos/interrupts.c */
 INT_RealModeInterrupt(vect,context);
 return 0;
}

#define CV CP(eax,Eax); CP(ecx,Ecx); CP(edx,Edx); CP(ebx,Ebx); \
           CP(esi,Esi); CP(edi,Edi); CP(esp,Esp); CP(ebp,Ebp); \
           CP(cs,SegCs); CP(ds,SegDs); CP(es,SegEs); \
           CP(ss,SegSs); CP(fs,SegFs); CP(gs,SegGs); \
           CP(eip,Eip); CP(eflags,EFlags)

int DOSVM_Process( LPDOSTASK lpDosTask )
{
 CONTEXT context;
 int ret=0;

#define CP(x,y) context.y = lpDosTask->VM86.regs.x
 CV;
#undef CP
 (void*)V86BASE(&context)=lpDosTask->img;

 switch (VM86_TYPE(lpDosTask->fn)) {
  case VM86_SIGNAL:
   printf("Trapped signal\n");
   ret=-1; break;
  case VM86_UNKNOWN: /* unhandled GPF */
   DOSVM_Dump(lpDosTask);
   ctx_debug(SIGSEGV,&context);
   break;
  case VM86_INTx:
   TRACE(int,"DOS EXE calls INT %02x with AX=%04lx\n",VM86_ARG(lpDosTask->fn),context.Eax);
   ret=DOSVM_Int(VM86_ARG(lpDosTask->fn),&context); break;
  case VM86_STI:
   break;
  case VM86_PICRETURN:
   printf("Trapped due to pending PIC request\n"); break;
  case VM86_TRAP:
   ctx_debug(SIGTRAP,&context);
   break;
  default:
   DOSVM_Dump(lpDosTask);
 }

 lpDosTask->fn=VM86_ENTER;
#define CP(x,y) lpDosTask->VM86.regs.x = context.y
 CV;
#undef CP
 return ret;
}

int DOSVM_Enter( PCONTEXT context )
{
 TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
 NE_MODULE *pModule = NE_GetPtr( pTask->hModule );
 LPDOSTASK lpDosTask;
 int stat;

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
#define CP(x,y) lpDosTask->VM86.regs.x = context->y
  CV;
#undef CP
 }

 /* main loop */
 while ((stat = MZ_RunModule(lpDosTask)) >= 0)
  if (stat > 0 && DOSVM_Process(lpDosTask) < 0)
   break;

 if (context) {
#define CP(x,y) context->y = lpDosTask->VM86.regs.x
  CV;
#undef CP
 }
 return 0;
}

#else /* !MZ_SUPPORTED */

int DOSVM_Enter( PCONTEXT context )
{
 ERR(module,"DOS realmode not supported on this architecture!\n");
 return -1;
}

#endif
