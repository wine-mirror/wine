/*
 * DOS Virtual Machine
 *
 * Copyright 1998 Ove Kåven
 */

#ifdef linux

/* force dosmod at high addresses */
asm(".org 0x110000");

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/vm86.h>
#include <sys/syscall.h>

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
 
/* fprintf(stderr,"main is at %08lx, file is %s, fd=%d\n",(unsigned long)&main,argv[0],mfd); */
 if (mfd<0) return 1;
/* Map in our DOS image at the start of the process address space */
 img=mmap(NULL,0x110000,PROT_EXEC|PROT_READ|PROT_WRITE,MAP_FIXED|MAP_SHARED,mfd,0);
 if (img==(void*)-1) {
  fprintf(stderr,"DOS memory map failed, error=%s\n",strerror(errno));
  return 1;
 }
/* fprintf(stderr,"Successfully mapped DOS memory, entering vm86 loop\n"); */
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
