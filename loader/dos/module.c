/*
 * DOS (MZ) loader
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
#ifdef MZ_USESYSV
#include <sys/ipc.h>
#include <sys/shm.h>
#else
#include <sys/mman.h>
#endif
#include <sys/vm86.h>
#ifdef MZ_USESYSV
#include <linux/mm.h> /* FIXME: where else should I fetch the PAGE_SIZE define? */
#endif
#include "windows.h"
#include "winbase.h"
#include "module.h"
#include "task.h"
#include "ldt.h"
#include "process.h"
#include "miscemu.h"
#include "debug.h"
#include "dosexe.h"

#define BIOS_DATA_SEGMENT 0x40
#define BIOS_SEGMENT BIOSSEG /* BIOSSEG is defined to 0xf000 in sys/vm86.h */
#define STUB_SEGMENT BIOS_SEGMENT
#ifdef MZ_USESYSV
/* it might be that SYSV supports START_OFFSET 0 after all, haven't checked */
#define START_OFFSET PAGE_SIZE
#else
#define START_OFFSET 0
#endif
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
  while (*cmd == ' ') cmd++;
  psp->cmdLine[0]=strlen(cmd);
  strcpy(psp->cmdLine+1,cmd);
  psp->cmdLine[psp->cmdLine[0]+1]='\r';
 } else psp->cmdLine[1]='\r';
 /* FIXME: integrate the memory stuff from Wine (msdos/dosmem.c) */
 /* FIXME: integrate the PDB stuff from Wine (loader/task.c) */
}

static int MZ_LoadImage( HFILE16 hFile, LPCSTR cmdline, LPCSTR env, LPDOSTASK lpDosTask )
{
 IMAGE_DOS_HEADER mz_header;
 DWORD image_start,image_size,min_size,max_size,avail;
 WORD psp_seg,load_seg;
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
#ifdef MZ_USESYSV
 lpDosTask->key=ftok(".",'d'); /* FIXME: this is from my IPC intro doc */
 lpDosTask->shm_id=shmget(lpDosTask->key,0x110000-START_OFFSET,IPC_CREAT|SHM_R|SHM_W);
 lpDosTask->img=shmat(lpDosTask->shm_id,NULL,0);
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
 psp_start=DOSMEM_GetBlock(lpDosTask->hModule,avail,&psp_seg);
 if (!psp_start) {
  ERR(module, "error allocating DOS memory\n");
  return 0;
 }
 load_seg=psp_seg+PSP_SIZE;
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
  *(WORD*)SEGPTR16(load_start,reloc)+=load_seg;
 }

 /* initialize vm86 struct */
 memset(&lpDosTask->VM86,0,sizeof(lpDosTask->VM86));
 lpDosTask->VM86.regs.cs=load_seg+mz_header.e_cs;
 lpDosTask->VM86.regs.eip=mz_header.e_ip;
 lpDosTask->VM86.regs.ss=load_seg+mz_header.e_ss;
 lpDosTask->VM86.regs.esp=mz_header.e_sp;
 lpDosTask->VM86.regs.ds=psp_seg;
 lpDosTask->VM86.regs.es=psp_seg;
 /* hmm, what else do we need? */

 return 32;
}

static int MZ_InitTask( LPDOSTASK lpDosTask )
{
 int read_fd[2],write_fd[2];
 pid_t child;

 /* create read pipe */
 if (pipe(read_fd)<0) return 0;
 if (pipe(write_fd)<0) {
  close(read_fd[0]); close(read_fd[1]); return 0;
 }
 lpDosTask->read_pipe=read_fd[0];
 lpDosTask->write_pipe=write_fd[1];
 lpDosTask->fn=VM86_ENTER;
 lpDosTask->state=1;
 TRACE(module,"Preparing to load DOS EXE support module: forking\n");
 if ((child=fork())<0) {
  close(write_fd[0]); close(write_fd[1]);
  close(read_fd[0]); close(read_fd[1]); return 0;
 }
 if (child!=0) {
  /* parent process */
  close(read_fd[1]); close(write_fd[0]);
  lpDosTask->task=child;
 } else {
  /* child process */
  close(read_fd[0]); close(write_fd[1]);
  /* put our pipes somewhere dosmod can find them */
  dup2(write_fd[0],0);      /* stdin */
  dup2(read_fd[1],1);       /* stdout */
  /* now load dosmod */
  execlp("dosmod",lpDosTask->mm_name,NULL);
  execl("dosmod",lpDosTask->mm_name,NULL);
  execl("loader/dos/dosmod",lpDosTask->mm_name,NULL);
  /* if failure, exit */
  ERR(module,"Failed to spawn dosmod, error=%s\n",strerror(errno));
  exit(1);
 }
 return 32;
}

HINSTANCE16 MZ_LoadModule( LPCSTR name, LPCSTR cmdline, 
                           LPCSTR env, UINT16 show_cmd )
{
 LPDOSTASK lpDosTask;
 HMODULE16 hModule;
 HINSTANCE16 hInstance;
 NE_MODULE *pModule;
 HFILE16 hFile;
 OFSTRUCT ofs;
 PROCESS_INFORMATION info;
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
 err = MZ_LoadImage( hFile, cmdline, env, lpDosTask );
 _lclose16(hFile);
 if (err<32) {
  if (lpDosTask->img!=NULL) munmap(lpDosTask->img,0x110000-START_OFFSET);
  if (lpDosTask->mm_fd>=0) close(lpDosTask->mm_fd);
  if (lpDosTask->mm_name[0]!=0) unlink(lpDosTask->mm_name);
  return err;
 }
 MZ_InitTask( lpDosTask );

 hInstance = NE_CreateInstance(pModule, NULL, (cmdline == NULL));
 PROCESS_Create( pModule, cmdline, env, hInstance, 0, show_cmd, &info );
 /* we don't need the handles for now */
 CloseHandle( info.hThread );
 CloseHandle( info.hProcess );
 return hInstance;
}

void MZ_KillModule( LPDOSTASK lpDosTask )
{
 munmap(lpDosTask->img,0x110000-START_OFFSET);
 close(lpDosTask->mm_fd);
 unlink(lpDosTask->mm_name);
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

#endif /* linux */
