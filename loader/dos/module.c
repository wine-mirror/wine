/*
 * DOS (MZ) loader
 *
 * Copyright 1998 Ove Kåven
 *
 * This code hasn't been completely cleaned up yet.
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
#include <sys/mman.h>
#include <sys/vm86.h>
#include "windows.h"
#include "winbase.h"
#include "module.h"
#include "task.h"
#include "ldt.h"
#include "process.h"
#include "miscemu.h"
#include "debug.h"
#include "dosexe.h"

/* define this to try mapping through /proc/pid/mem instead of a temp file,
   but Linus doesn't like mmapping /proc/pid/mem, so it doesn't work for me */
#undef MZ_MAPSELF

#define BIOS_DATA_SEGMENT 0x40
#define BIOS_SEGMENT BIOSSEG /* BIOSSEG is defined to 0xf000 in sys/vm86.h */
#define STUB_SEGMENT BIOS_SEGMENT
#define START_OFFSET 0
#define PSP_SIZE 0x10

#define SEG16(ptr,seg) ((LPVOID)((BYTE*)ptr+((DWORD)(seg)<<4)))
#define SEGPTR16(ptr,segptr) ((LPVOID)((BYTE*)ptr+((DWORD)SELECTOROF(segptr)<<4)+OFFSETOF(segptr)))

#define STUB(x) (0x90CF00CD|(x<<8)) /* INT x; IRET; NOP */

static void MZ_InitSystem( LPVOID lpImage )
{
 SEGPTR*isr=lpImage;
 DWORD*stub=SEG16(lpImage,STUB_SEGMENT);
 int x;
 
 /* initialize ISR table to make it point to INT stubs in BIOSSEG;
    Linux only sends INTs performed from or destined to BIOSSEG at us,
    if the int_revectored table is empty.
    This allows DOS programs to hook interrupts, as well as use their
    familiar retf tricks to call them... */
 /* (note that the hooking might not actually work since DOS3Call stuff
     isn't fully integrated with the DOS VM yet...) */
 TRACE(module,"Initializing interrupt vector table\n");
 for (x=0; x<256; x++) isr[x]=PTR_SEG_OFF_TO_SEGPTR(STUB_SEGMENT,x*4);
 TRACE(module,"Initializing interrupt stub table\n");
 for (x=0; x<256; x++) stub[x]=STUB(x);
}

static void MZ_InitPSP( LPVOID lpPSP, LPCSTR cmdline, LPCSTR env )
{
 PDB*psp=lpPSP;
 const char*cmd=cmdline?strchr(cmdline,' '):NULL;

 psp->int20=0x20CD; /* int 20 */
 /* some programs use this to calculate how much memory they need */
 psp->nextParagraph=0x9FFF;
 /* copy parameters */
 if (cmd) {
#if 0
  /* command.com doesn't do this */
  while (*cmd == ' ') cmd++;
#endif
  psp->cmdLine[0]=strlen(cmd);
  strcpy(psp->cmdLine+1,cmd);
  psp->cmdLine[psp->cmdLine[0]+1]='\r';
 } else psp->cmdLine[1]='\r';
 /* FIXME: integrate the memory stuff from Wine (msdos/dosmem.c) */
 /* FIXME: integrate the PDB stuff from Wine (loader/task.c) */
}

static int MZ_LoadImage( HFILE16 hFile, LPCSTR cmdline, LPCSTR env,
                         LPDOSTASK lpDosTask, NE_MODULE *pModule )
{
 IMAGE_DOS_HEADER mz_header;
 DWORD image_start,image_size,min_size,max_size,avail;
 BYTE*psp_start,*load_start;
 int x;
 SEGPTR reloc;

 if ((_hread16(hFile,&mz_header,sizeof(mz_header)) != sizeof(mz_header)) ||
     (mz_header.e_magic != IMAGE_DOS_SIGNATURE))
     return 11; /* invalid exe */
 /* calculate load size */
 image_start=mz_header.e_cparhdr<<4;
 image_size=mz_header.e_cp<<9; /* pages are 512 bytes */
 if ((mz_header.e_cblp!=0)&&(mz_header.e_cblp!=4)) image_size-=512-mz_header.e_cblp;
 image_size-=image_start;
 min_size=image_size+((DWORD)mz_header.e_minalloc<<4)+(PSP_SIZE<<4);
 max_size=image_size+((DWORD)mz_header.e_maxalloc<<4)+(PSP_SIZE<<4);

 /* allocate 1MB+64K shared memory */
 lpDosTask->img_ofs=START_OFFSET;
#ifdef MZ_MAPSELF
 lpDosTask->img=VirtualAlloc(NULL,0x110000,MEM_COMMIT,PAGE_READWRITE);
 /* make sure mmap accepts it */
 ((char*)lpDosTask->img)[0x10FFFF]=0;
#else
 tmpnam(lpDosTask->mm_name);
/* strcpy(lpDosTask->mm_name,"/tmp/mydosimage"); */
 lpDosTask->mm_fd=open(lpDosTask->mm_name,O_RDWR|O_CREAT /* |O_TRUNC */,S_IRUSR|S_IWUSR);
 if (lpDosTask->mm_fd<0) ERR(module,"file %s could not be opened\n",lpDosTask->mm_name);
 /* expand file to 1MB+64K */
 lseek(lpDosTask->mm_fd,0x110000-1,SEEK_SET);
 x=0; write(lpDosTask->mm_fd,&x,1);
 /* map it in */
 lpDosTask->img=mmap(NULL,0x110000-START_OFFSET,PROT_READ|PROT_WRITE,MAP_SHARED,lpDosTask->mm_fd,0);
#endif
 if (lpDosTask->img==(LPVOID)-1) {
  ERR(module,"could not map shared memory, error=%s\n",strerror(errno));
  return 0;
 }
 TRACE(module,"DOS VM86 image mapped at %08lx\n",(DWORD)lpDosTask->img);
 pModule->dos_image=lpDosTask->img;

 /* initialize the memory */
 MZ_InitSystem(lpDosTask->img);
 TRACE(module,"Initializing DOS memory structures\n");
 DOSMEM_Init(lpDosTask->hModule);

 /* FIXME: allocate memory for environment variables */

 /* allocate memory for the executable */
 TRACE(module,"Allocating DOS memory (min=%ld, max=%ld)\n",min_size,max_size);
 avail=DOSMEM_Available(lpDosTask->hModule);
 if (avail<min_size) {
  ERR(module, "insufficient DOS memory\n");
  return 0;
 }
 if (avail>max_size) avail=max_size;
 psp_start=DOSMEM_GetBlock(lpDosTask->hModule,avail,&lpDosTask->psp_seg);
 if (!psp_start) {
  ERR(module, "error allocating DOS memory\n");
  return 0;
 }
 lpDosTask->load_seg=lpDosTask->psp_seg+PSP_SIZE;
 load_start=psp_start+(PSP_SIZE<<4);
 MZ_InitPSP(psp_start, cmdline, env);

 /* load executable image */
 TRACE(module,"loading DOS EXE image size, %08lx bytes\n",image_size);
 _llseek16(hFile,image_start,FILE_BEGIN);
 if ((_hread16(hFile,load_start,image_size)) != image_size)
  return 11; /* invalid exe */

 /* load relocation table */
 TRACE(module,"loading DOS EXE relocation table, %d entries\n",mz_header.e_lfarlc);
 /* FIXME: is this too slow without read buffering? */
 _llseek16(hFile,mz_header.e_lfarlc,FILE_BEGIN);
 for (x=0; x<mz_header.e_crlc; x++) {
  if (_lread16(hFile,&reloc,sizeof(reloc)) != sizeof(reloc))
   return 11; /* invalid exe */
  *(WORD*)SEGPTR16(load_start,reloc)+=lpDosTask->load_seg;
 }

 /* initialize vm86 struct */
 memset(&lpDosTask->VM86,0,sizeof(lpDosTask->VM86));
 lpDosTask->VM86.regs.cs=lpDosTask->load_seg+mz_header.e_cs;
 lpDosTask->VM86.regs.eip=mz_header.e_ip;
 lpDosTask->VM86.regs.ss=lpDosTask->load_seg+mz_header.e_ss;
 lpDosTask->VM86.regs.esp=mz_header.e_sp;
 lpDosTask->VM86.regs.ds=lpDosTask->psp_seg;
 lpDosTask->VM86.regs.es=lpDosTask->psp_seg;
 /* hmm, what else do we need? */

 return 32;
}

int MZ_InitTask( LPDOSTASK lpDosTask )
{
 int read_fd[2],write_fd[2];
 pid_t child;
 char *fname,*farg,arg[16],fproc[64];

 /* create read pipe */
 if (pipe(read_fd)<0) return 0;
 if (pipe(write_fd)<0) {
  close(read_fd[0]); close(read_fd[1]); return 0;
 }
 lpDosTask->read_pipe=read_fd[0];
 lpDosTask->write_pipe=write_fd[1];
 lpDosTask->fn=VM86_ENTER;
 lpDosTask->state=1;
 /* if we have a mapping file, use it */
 fname=lpDosTask->mm_name; farg=NULL;
 if (!fname[0]) {
  /* otherwise, map our own memory image */
  sprintf(fproc,"/proc/%d/mem",getpid());
  sprintf(arg,"%ld",(unsigned long)lpDosTask->img);
  fname=fproc; farg=arg;
 }

 TRACE(module,"Preparing to load DOS VM support module: forking\n");
 if ((child=fork())<0) {
  close(write_fd[0]); close(write_fd[1]);
  close(read_fd[0]); close(read_fd[1]); return 0;
 }
 if (child!=0) {
  /* parent process */
  int ret;

  close(read_fd[1]); close(write_fd[0]);
  lpDosTask->task=child;
  /* wait for child process to signal readiness */
  do {
   if (read(lpDosTask->read_pipe,&ret,sizeof(ret))!=sizeof(ret)) {
    if ((errno==EINTR)||(errno==EAGAIN)) continue;
    /* failure */
    ERR(module,"dosmod has failed to initialize\n");
    return 0;
   }
  } while (0);
  /* all systems are now go */
 } else {
  /* child process */
  close(read_fd[0]); close(write_fd[1]);
  /* put our pipes somewhere dosmod can find them */
  dup2(write_fd[0],0);      /* stdin */
  dup2(read_fd[1],1);       /* stdout */
  /* now load dosmod */
  execlp("dosmod",fname,farg,NULL);
  execl("dosmod",fname,farg,NULL);
  execl("loader/dos/dosmod",fname,farg,NULL);
  /* if failure, exit */
  ERR(module,"Failed to spawn dosmod, error=%s\n",strerror(errno));
  exit(1);
 }
 return 32;
}

HINSTANCE16 MZ_CreateProcess( LPCSTR name, LPCSTR cmdline, LPCSTR env, 
                              LPSTARTUPINFO32A startup, LPPROCESS_INFORMATION info )
{
 LPDOSTASK lpDosTask;
 HMODULE16 hModule;
 HINSTANCE16 hInstance;
 NE_MODULE *pModule;
 HFILE16 hFile;
 OFSTRUCT ofs;
 int err;

 if ((lpDosTask = calloc(1, sizeof(DOSTASK))) == NULL)
  return 0;

 if ((hFile = OpenFile16( name, &ofs, OF_READ )) == HFILE_ERROR16)
  return 2; /* File not found */

 if ((hModule = MODULE_CreateDummyModule(&ofs)) < 32)
  return hModule;

 lpDosTask->hModule = hModule;

 pModule = (NE_MODULE *)GlobalLock16(hModule);
 pModule->lpDosTask = lpDosTask;
 
 lpDosTask->img=NULL; lpDosTask->mm_name[0]=0; lpDosTask->mm_fd=-1;
 err = MZ_LoadImage( hFile, cmdline, env, lpDosTask, pModule );
 _lclose16(hFile);
 pModule->dos_image = lpDosTask->img;
 if (err<32) {
  if (lpDosTask->mm_name[0]!=0) {
   if (lpDosTask->img!=NULL) munmap(lpDosTask->img,0x110000-START_OFFSET);
   if (lpDosTask->mm_fd>=0) close(lpDosTask->mm_fd);
   unlink(lpDosTask->mm_name);
  } else
   if (lpDosTask->img!=NULL) VirtualFree(lpDosTask->img,0x110000,MEM_RELEASE);
  return err;
 }
 err = MZ_InitTask( lpDosTask );
 if (lpDosTask->mm_name[0]!=0) {
  /* we unlink the temp file here to avoid leaving a mess in /tmp
     if/when Wine crashes; the mapping still remains open, though */
  unlink(lpDosTask->mm_name);
 }
 if (err<32) {
  MZ_KillModule( lpDosTask );
  /* FIXME: cleanup hModule */
  return err;
 }

 hInstance = NE_CreateInstance(pModule, NULL, (cmdline == NULL));
 PROCESS_Create( pModule, cmdline, env, hInstance, 0, startup, info );
 return hInstance;
}

void MZ_KillModule( LPDOSTASK lpDosTask )
{
 if (lpDosTask->mm_name[0]!=0) {
  munmap(lpDosTask->img,0x110000-START_OFFSET);
  close(lpDosTask->mm_fd);
 } else VirtualFree(lpDosTask->img,0x110000,MEM_RELEASE);
 close(lpDosTask->read_pipe);
 close(lpDosTask->write_pipe);
 kill(lpDosTask->task,SIGTERM);
}

int MZ_RunModule( LPDOSTASK lpDosTask )
{
/* transmit the current context structure to the DOS task */
 if (lpDosTask->state==1) {
  if (write(lpDosTask->write_pipe,&lpDosTask->fn,sizeof(lpDosTask->fn))!=sizeof(lpDosTask->fn))
   return -1;
  if (write(lpDosTask->write_pipe,&lpDosTask->VM86,sizeof(lpDosTask->VM86))!=sizeof(lpDosTask->VM86))
   return -1;
  lpDosTask->state=2;
 }
/* wait for another context structure to appear (this may block) */
 if (lpDosTask->state==2) {
  if (read(lpDosTask->read_pipe,&lpDosTask->fn,sizeof(lpDosTask->fn))!=sizeof(lpDosTask->fn)) {
   if ((errno==EINTR)||(errno==EAGAIN)) return 0;
   return -1;
  }
  lpDosTask->state=3;
 }
 if (lpDosTask->state==3) {
  if (read(lpDosTask->read_pipe,&lpDosTask->VM86,sizeof(lpDosTask->VM86))!=sizeof(lpDosTask->VM86)) {
   if ((errno==EINTR)||(errno==EAGAIN)) return 0;
   return -1;
  }
/* got one */
  lpDosTask->state=1;
  return 1;
 }
 return 0;
}

#else /* !linux */

HINSTANCE16 MZ_CreateProcess( LPCSTR name, LPCSTR cmdline, LPCSTR env, 
                              LPSTARTUPINFO32A startup, LPPROCESS_INFORMATION info )
{
 WARN(module,"DOS executables not supported on this architecture\n");
 return (HMODULE16)11;  /* invalid exe */
}

#endif
