/*
 * DOS Virtual Machine
 *
 * Copyright 1998 Ove Kåven
 */

#ifdef linux
 
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
#include "msdos.h"
#include "miscemu.h"
#include "debug.h"
#include "module.h"
#include "ldt.h"
#include "dosexe.h"

void DOSVM_Dump( LPDOSTASK lpDosTask)
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

 iofs=((DWORD)REGS.cs<<4)+REGS.eip;
#undef REGS
 inst=(BYTE*)lpDosTask->img+iofs;
 printf("Opcodes:");
 for (x=0; x<8; x++) printf(" %02x",inst[x]);
 printf("\n");

 exit(0);
}

int DOSVM_Int(int vect, LPDOSTASK lpDosTask, PCONTEXT context )
{
 switch (vect) {
  case 0x20:
   return -1;
  case 0x21:
   if (AH_reg(context)==0x4c) return -1;
   DOS3Call(context);
   break;
 }
 return 0;
}

int DOSVM_Process( LPDOSTASK lpDosTask )
{
 CONTEXT context;
 int ret=0;

#define REGS lpDosTask->VM86.regs
 context.Eax=REGS.eax; context.Ecx=REGS.ecx; context.Edx=REGS.edx; context.Ebx=REGS.ebx;
 context.Esi=REGS.esi; context.Edi=REGS.edi; context.Esp=REGS.esp; context.Ebp=REGS.ebp;
 context.SegCs=REGS.cs; context.SegDs=REGS.ds; context.SegEs=REGS.es;
 context.SegSs=REGS.ss; context.SegFs=REGS.fs; context.SegGs=REGS.gs;
 context.Eip=REGS.eip; context.EFlags=REGS.eflags;
 (void*)V86BASE(&context)=lpDosTask->img;

 switch (VM86_TYPE(lpDosTask->fn)) {
  case VM86_SIGNAL:
   printf("Trapped signal\n");
   ret=-1; break;
  case VM86_UNKNOWN:
   DOSVM_Dump(lpDosTask);
   break;
  case VM86_INTx:
   TRACE(int,"DOS EXE calls INT %02x\n",VM86_ARG(lpDosTask->fn));
   ret=DOSVM_Int(VM86_ARG(lpDosTask->fn),lpDosTask,&context); break;
  case VM86_STI:
   break;
  case VM86_PICRETURN:
   printf("Trapped due to pending PIC request\n"); break;
  case VM86_TRAP:
   printf("Trapped debug request\n"); break;
  default:
   DOSVM_Dump(lpDosTask);
 }

 lpDosTask->fn=VM86_ENTER;
 REGS.eax=context.Eax; REGS.ecx=context.Ecx; REGS.edx=context.Edx; REGS.ebx=context.Ebx;
 REGS.esi=context.Esi; REGS.edi=context.Edi; REGS.esp=context.Esp; REGS.ebp=context.Ebp;
 REGS.cs=context.SegCs; REGS.ds=context.SegDs; REGS.es=context.SegEs;
 REGS.ss=context.SegSs; REGS.fs=context.SegFs; REGS.gs=context.SegGs;
 REGS.eip=context.Eip; REGS.eflags=context.EFlags;
#undef REGS
 return ret;
}

#endif /* linux */
