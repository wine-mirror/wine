/*
 * DOS Virtual Machine
 *
 * Copyright 1998 Ove Kåven
 */

#ifdef linux

/* apparently ELF images are usually loaded high anyway */
#ifndef __ELF__
/* if not, force dosmod at high addresses */
asm(".org 0x110000");
#endif __ELF__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/vm86.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

/* FIXME: hack because libc vm86 may be the old syscall version */
static __inline__ int vm86plus( int func, struct vm86plus_struct *ptr )
{
    int res;
#ifdef __PIC__
    __asm__ __volatile__( "pushl %%ebx\n\t"
                          "movl %2,%%ebx\n\t"
                          "int $0x80\n\t"
                          "popl %%ebx"
                          : "=a" (res)
                          : "0" (SYS_vm86),
                            "g" (func),
                            "c" (ptr) );
#else
    __asm__ __volatile__("int $0x80"
                         : "=a" (res)
                         : "0" (SYS_vm86),
                           "b" (func),
                           "c" (ptr) );
#endif  /* __PIC__ */
    if (res >= 0) return res;
    errno = -res;
    return -1;
}

int main(int argc,char**argv)
{
 int mfd=open(argv[0],O_RDWR);
 void*img;
 struct vm86plus_struct VM86;
 int func,ret;
 off_t fofs=0;
 pid_t ppid=getppid();
 
/* fprintf(stderr,"main is at %08lx, file is %s, fd=%d\n",(unsigned long)&main,argv[0],mfd); */
 if (mfd<0) return 1;
/* Map in our DOS image at the start of the process address space */
 if (argv[1]) {
  /* Ulrich Weigand suggested mapping in the DOS image directly from the Wine
     address space */
  fofs=atol(argv[1]);
  /* linux currently only allows mapping a process memory if it's being ptraced */
  /* Linus doesn't like it, so this probably won't work in the future */
  /* it doesn't even work for me right now */
  ptrace(PTRACE_ATTACH,ppid,0,0);
  kill(ppid,SIGSTOP);
  waitpid(ppid,NULL,0);
 }
 img=mmap(NULL,0x110000,PROT_EXEC|PROT_READ|PROT_WRITE,MAP_FIXED|MAP_SHARED,mfd,fofs);
 if (argv[1]) {
  ptrace(PTRACE_DETACH,ppid,0,0);
  kill(ppid,SIGCONT);
 }
 if (img==(void*)-1) {
  fprintf(stderr,"DOS memory map failed, error=%s\n",strerror(errno));
  fprintf(stderr,"in attempt to map %s, offset %08lX, length 110000, to offset 0\n",argv[0],fofs);
  return 1;
 }
/* fprintf(stderr,"Successfully mapped DOS memory, entering vm86 loop\n"); */
/* report back to the main program that we're ready */
 ret=0;
 write(1,&ret,sizeof(ret));
/* context exchange loop */
 do {
  if (read(0,&func,sizeof(func))!=sizeof(func)) return 1;
  if (read(0,&VM86,sizeof(VM86))!=sizeof(VM86)) return 1;
  if (func<0) break;
  ret=vm86plus(func,&VM86);
  if (write(1,&ret,sizeof(ret))!=sizeof(ret)) return 1;
  if (write(1,&VM86,sizeof(VM86))!=sizeof(VM86)) return 1;
 } while (1);
 return 0;
}

#else /* !linux */
int main(void) {return 1;}
#endif
