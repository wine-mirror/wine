/*
 * DOS (MZ) loader
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
#include <sys/time.h>
#include "windef.h"
#include "wine/winbase16.h"
#include "winerror.h"
#include "module.h"
#include "peexe.h"
#include "neexe.h"
#include "task.h"
#include "selectors.h"
#include "file.h"
#include "ldt.h"
#include "process.h"
#include "miscemu.h"
#include "debug.h"
#include "dosexe.h"
#include "dosmod.h"
#include "options.h"

#ifdef MZ_SUPPORTED

#include <sys/mman.h>

/* define this to try mapping through /proc/pid/mem instead of a temp file,
   but Linus doesn't like mmapping /proc/pid/mem, so it doesn't work for me */
#undef MZ_MAPSELF

#define BIOS_DATA_SEGMENT 0x40
#define START_OFFSET 0
#define PSP_SIZE 0x10

#define SEG16(ptr,seg) ((LPVOID)((BYTE*)ptr+((DWORD)(seg)<<4)))
#define SEGPTR16(ptr,segptr) ((LPVOID)((BYTE*)ptr+((DWORD)SELECTOROF(segptr)<<4)+OFFSETOF(segptr)))

static void MZ_InitPSP( LPVOID lpPSP, LPCSTR cmdline, WORD env )
{
 PDB16*psp=lpPSP;
 const char*cmd=cmdline?strchr(cmdline,' '):NULL;

 psp->int20=0x20CD; /* int 20 */
 /* some programs use this to calculate how much memory they need */
 psp->nextParagraph=0x9FFF;
 psp->environment=env;
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

/* default INT 08 handler: increases timer tick counter but not much more */
static char int08[]={
 0xCD,0x1C,           /* int $0x1c */
 0x50,                /* pushw %ax */
 0x1E,                /* pushw %ds */
 0xB8,0x40,0x00,      /* movw $0x40,%ax */
 0x8E,0xD8,           /* movw %ax,%ds */
#if 0
 0x83,0x06,0x6C,0x00,0x01, /* addw $1,(0x6c) */
 0x83,0x16,0x6E,0x00,0x00, /* adcw $0,(0x6e) */
#else
 0x66,0xFF,0x06,0x6C,0x00, /* incl (0x6c) */
#endif
 0xB0,0x20,           /* movb $0x20,%al */
 0xE6,0x20,           /* outb %al,$0x20 */
 0x1F,                /* popw %ax */
 0x58,                /* popw %ax */
 0xCF                 /* iret */
};

static void MZ_InitHandlers( LPDOSTASK lpDosTask )
{
 WORD seg;
 LPBYTE start=DOSMEM_GetBlock(lpDosTask->hModule,sizeof(int08),&seg);
 memcpy(start,int08,sizeof(int08));
/* INT 08: point it at our tick-incrementing handler */
 ((SEGPTR*)(lpDosTask->img))[0x08]=PTR_SEG_OFF_TO_SEGPTR(seg,0);
/* INT 1C: just point it to IRET, we don't want to handle it ourselves */
 ((SEGPTR*)(lpDosTask->img))[0x1C]=PTR_SEG_OFF_TO_SEGPTR(seg,sizeof(int08)-1);
}

static char enter_xms[]={
/* XMS hookable entry point */
 0xEB,0x03,           /* jmp entry */
 0x90,0x90,0x90,      /* nop;nop;nop */
                      /* entry: */
/* real entry point */
/* for simplicity, we'll just use the same hook as DPMI below */
 0xCD,0x31,           /* int $0x31 */
 0xCB                 /* lret */
};

static void MZ_InitXMS( LPDOSTASK lpDosTask )
{
 LPBYTE start=DOSMEM_GetBlock(lpDosTask->hModule,sizeof(enter_xms),&(lpDosTask->xms_seg));
 memcpy(start,enter_xms,sizeof(enter_xms));
}

static char enter_pm[]={
 0x50,                /* pushw %ax */
 0x52,                /* pushw %dx */
 0x55,                /* pushw %bp */
 0x89,0xE5,           /* movw %sp,%bp */
/* get return CS */
 0x8B,0x56,0x08,      /* movw 8(%bp),%dx */
/* just call int 31 here to get into protected mode... */
/* it'll check whether it was called from dpmi_seg... */
 0xCD,0x31,           /* int $0x31 */
/* we are now in the context of a 16-bit relay call */
/* need to fixup our stack;
 * 16-bit relay return address will be lost, but we won't worry quite yet */
 0x8E,0xD0,           /* movw %ax,%ss */
 0x66,0x0F,0xB7,0xE5, /* movzwl %bp,%esp */
/* set return CS */
 0x89,0x56,0x08,      /* movw %dx,8(%bp) */
 0x5D,                /* popw %bp */
 0x5A,                /* popw %dx */
 0x58,                /* popw %ax */
 0xCB                 /* lret */
};

static void MZ_InitDPMI( LPDOSTASK lpDosTask )
{
 unsigned size=sizeof(enter_pm);
 LPBYTE start=DOSMEM_GetBlock(lpDosTask->hModule,size,&(lpDosTask->dpmi_seg));
 
 lpDosTask->dpmi_sel = SELECTOR_AllocBlock( start, size, SEGMENT_CODE, FALSE, FALSE );

 memcpy(start,enter_pm,sizeof(enter_pm));
}

static WORD MZ_InitEnvironment( LPDOSTASK lpDosTask, LPCSTR env, LPCSTR name )
{
 unsigned sz=0;
 WORD seg;
 LPSTR envblk;

 if (env) {
  /* get size of environment block */
  while (env[sz++]) sz+=strlen(env+sz)+1;
 } else sz++;
 /* allocate it */
 envblk=DOSMEM_GetBlock(lpDosTask->hModule,sz+sizeof(WORD)+strlen(name)+1,&seg);
 /* fill it */
 if (env) {
  memcpy(envblk,env,sz);
 } else envblk[0]=0;
 /* DOS 3.x: the block contains 1 additional string */
 *(WORD*)(envblk+sz)=1;
 /* being the program name itself */
 strcpy(envblk+sz+sizeof(WORD),name);
 return seg;
}

static BOOL MZ_InitMemory( LPDOSTASK lpDosTask, NE_MODULE *pModule )
{
 int x;

 if (lpDosTask->img_ofs) return TRUE; /* already allocated */

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
  return FALSE;
 }
 TRACE(module,"DOS VM86 image mapped at %08lx\n",(DWORD)lpDosTask->img);
 pModule->dos_image=lpDosTask->img;

 /* initialize the memory */
 TRACE(module,"Initializing DOS memory structures\n");
 DOSMEM_Init(lpDosTask->hModule);
 MZ_InitHandlers(lpDosTask);
 MZ_InitXMS(lpDosTask);
 MZ_InitDPMI(lpDosTask);
 return TRUE;
}

static BOOL MZ_LoadImage( HFILE hFile, OFSTRUCT *ofs, LPCSTR cmdline,
                          LPCSTR env, LPDOSTASK lpDosTask, NE_MODULE *pModule )
{
 IMAGE_DOS_HEADER mz_header;
 DWORD image_start,image_size,min_size,max_size,avail;
 BYTE*psp_start,*load_start;
 int x,old_com=0;
 SEGPTR reloc;
 WORD env_seg;

 _llseek(hFile,0,FILE_BEGIN);
 if ((_lread(hFile,&mz_header,sizeof(mz_header)) != sizeof(mz_header)) ||
     (mz_header.e_magic != IMAGE_DOS_SIGNATURE)) {
  old_com=1; /* assume .COM file */
  image_start=0;
  image_size=GetFileSize(hFile,NULL);
  min_size=0x10000; max_size=0x100000;
  mz_header.e_crlc=0;
  mz_header.e_ss=0; mz_header.e_sp=0xFFFE;
  mz_header.e_cs=0; mz_header.e_ip=0x100;
 } else {
  /* calculate load size */
  image_start=mz_header.e_cparhdr<<4;
  image_size=mz_header.e_cp<<9; /* pages are 512 bytes */
  if ((mz_header.e_cblp!=0)&&(mz_header.e_cblp!=4)) image_size-=512-mz_header.e_cblp;
  image_size-=image_start;
  min_size=image_size+((DWORD)mz_header.e_minalloc<<4)+(PSP_SIZE<<4);
  max_size=image_size+((DWORD)mz_header.e_maxalloc<<4)+(PSP_SIZE<<4);
 }

 MZ_InitMemory(lpDosTask,pModule);

 /* allocate environment block */
 env_seg=MZ_InitEnvironment(lpDosTask,env,ofs->szPathName);

 /* allocate memory for the executable */
 TRACE(module,"Allocating DOS memory (min=%ld, max=%ld)\n",min_size,max_size);
 avail=DOSMEM_Available(lpDosTask->hModule);
 if (avail<min_size) {
  ERR(module, "insufficient DOS memory\n");
  SetLastError(ERROR_NOT_ENOUGH_MEMORY);
  return FALSE;
 }
 if (avail>max_size) avail=max_size;
 psp_start=DOSMEM_GetBlock(lpDosTask->hModule,avail,&lpDosTask->psp_seg);
 if (!psp_start) {
  ERR(module, "error allocating DOS memory\n");
  SetLastError(ERROR_NOT_ENOUGH_MEMORY);
  return FALSE;
 }
 lpDosTask->load_seg=lpDosTask->psp_seg+(old_com?0:PSP_SIZE);
 load_start=psp_start+(PSP_SIZE<<4);
 MZ_InitPSP(psp_start, cmdline, env_seg);

 /* load executable image */
 TRACE(module,"loading DOS %s image, %08lx bytes\n",old_com?"COM":"EXE",image_size);
 _llseek(hFile,image_start,FILE_BEGIN);
 if ((_lread(hFile,load_start,image_size)) != image_size) {
  SetLastError(ERROR_BAD_FORMAT);
  return FALSE;
 }

 if (mz_header.e_crlc) {
  /* load relocation table */
  TRACE(module,"loading DOS EXE relocation table, %d entries\n",mz_header.e_crlc);
  /* FIXME: is this too slow without read buffering? */
  _llseek(hFile,mz_header.e_lfarlc,FILE_BEGIN);
  for (x=0; x<mz_header.e_crlc; x++) {
   if (_lread(hFile,&reloc,sizeof(reloc)) != sizeof(reloc)) {
    SetLastError(ERROR_BAD_FORMAT);
    return FALSE;
   }
   *(WORD*)SEGPTR16(load_start,reloc)+=lpDosTask->load_seg;
  }
 }

 lpDosTask->init_cs=lpDosTask->load_seg+mz_header.e_cs;
 lpDosTask->init_ip=mz_header.e_ip;
 lpDosTask->init_ss=lpDosTask->load_seg+mz_header.e_ss;
 lpDosTask->init_sp=mz_header.e_sp;

 TRACE(module,"entry point: %04x:%04x\n",lpDosTask->init_cs,lpDosTask->init_ip);
 return TRUE;
}

LPDOSTASK MZ_AllocDPMITask( HMODULE16 hModule )
{
 LPDOSTASK lpDosTask = calloc(1, sizeof(DOSTASK));
 NE_MODULE *pModule;

 if (lpDosTask) {
  lpDosTask->hModule = hModule;

  pModule = (NE_MODULE *)GlobalLock16(hModule);
  pModule->lpDosTask = lpDosTask;
 
  lpDosTask->img=NULL; lpDosTask->mm_name[0]=0; lpDosTask->mm_fd=-1;

  MZ_InitMemory(lpDosTask, pModule);

  GlobalUnlock16(hModule);
 }
 return lpDosTask;
}

static void MZ_InitTimer( LPDOSTASK lpDosTask, int ver )
{
 if (ver<1) {
#if 0
  /* start simulated system 55Hz timer */
  lpDosTask->system_timer = CreateSystemTimer( 55, MZ_Tick );
  TRACE(module,"created 55Hz timer tick, handle=%d\n",lpDosTask->system_timer);
#endif
 } else {
  int func;
  struct timeval tim;

  /* start dosmod timer at 55Hz */
  func=DOSMOD_SET_TIMER;
  tim.tv_sec=0; tim.tv_usec=54925;
  write(lpDosTask->write_pipe,&func,sizeof(func));
  write(lpDosTask->write_pipe,&tim,sizeof(tim));
 }
}

BOOL MZ_InitTask( LPDOSTASK lpDosTask )
{
 int read_fd[2],write_fd[2];
 pid_t child;
 char *fname,*farg,arg[16],fproc[64],path[256],*fpath;

 if (!lpDosTask) return FALSE;
 /* create read pipe */
 if (pipe(read_fd)<0) return FALSE;
 if (pipe(write_fd)<0) {
  close(read_fd[0]); close(read_fd[1]); return FALSE;
 }
 lpDosTask->read_pipe=read_fd[0];
 lpDosTask->write_pipe=write_fd[1];
 /* if we have a mapping file, use it */
 fname=lpDosTask->mm_name; farg=NULL;
 if (!fname[0]) {
  /* otherwise, map our own memory image */
  sprintf(fproc,"/proc/%d/mem",getpid());
  sprintf(arg,"%ld",(unsigned long)lpDosTask->img);
  fname=fproc; farg=arg;
 }

 TRACE(module,"Loading DOS VM support module (hmodule=%04x)\n",lpDosTask->hModule);
 if ((child=fork())<0) {
  close(write_fd[0]); close(write_fd[1]);
  close(read_fd[0]); close(read_fd[1]); return FALSE;
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
    if (lpDosTask->mm_name[0]!=0) unlink(lpDosTask->mm_name);
    return FALSE;
   }
  } while (0);
  /* the child has now mmaped the temp file, it's now safe to unlink.
   * do it here to avoid leaving a mess in /tmp if/when Wine crashes... */
  if (lpDosTask->mm_name[0]!=0) unlink(lpDosTask->mm_name);
  /* start simulated system timer */
  MZ_InitTimer(lpDosTask,ret);
  /* all systems are now go */
 } else {
  /* child process */
  close(read_fd[0]); close(write_fd[1]);
  /* put our pipes somewhere dosmod can find them */
  dup2(write_fd[0],0);      /* stdin */
  dup2(read_fd[1],1);       /* stdout */
  /* enable signals */
  SIGNAL_MaskAsyncEvents(FALSE);
  /* now load dosmod */
  execlp("dosmod",fname,farg,NULL);
  execl("dosmod",fname,farg,NULL);
  /* hmm, they didn't install properly */
  execl("loader/dos/dosmod",fname,farg,NULL);
  /* last resort, try to find it through argv[0] */
  fpath=strrchr(strcpy(path,Options.argv0),'/');
  if (fpath) {
   strcpy(fpath,"/loader/dos/dosmod");
   execl(path,fname,farg,NULL);
  }
  /* if failure, exit */
  ERR(module,"Failed to spawn dosmod, error=%s\n",strerror(errno));
  exit(1);
 }
 return TRUE;
}

BOOL MZ_CreateProcess( HFILE hFile, OFSTRUCT *ofs, LPCSTR cmdline, 
                       LPCSTR env, BOOL inherit, LPSTARTUPINFOA startup, 
                       LPPROCESS_INFORMATION info )
{
 LPDOSTASK lpDosTask = NULL; /* keep gcc from complaining */
 HMODULE16 hModule;
 PDB *pdb = PROCESS_Current();
 TDB *pTask = (TDB*)GlobalLock16( GetCurrentTask() );
 NE_MODULE *pModule = pTask ? NE_GetPtr( pTask->hModule ) : NULL;
 int alloc = !(pModule && pModule->dos_image);

 if (alloc && (lpDosTask = calloc(1, sizeof(DOSTASK))) == NULL) {
  SetLastError(ERROR_NOT_ENOUGH_MEMORY);
  return FALSE;
 }

 if ((!env)&&pdb) env = pdb->env_db->environ;
 if (alloc) {
  if ((hModule = MODULE_CreateDummyModule(ofs, NULL)) < 32) {
   SetLastError(hModule);
   return FALSE;
  }
  lpDosTask->hModule = hModule;

  pModule = (NE_MODULE *)GlobalLock16(hModule);
  pModule->lpDosTask = lpDosTask;
 
  lpDosTask->img=NULL; lpDosTask->mm_name[0]=0; lpDosTask->mm_fd=-1;
 } else lpDosTask=pModule->lpDosTask;
 if (!MZ_LoadImage( hFile, ofs, cmdline, env, lpDosTask, pModule )) {
  if (alloc) {
   if (lpDosTask->mm_name[0]!=0) {
    if (lpDosTask->img!=NULL) munmap(lpDosTask->img,0x110000-START_OFFSET);
    if (lpDosTask->mm_fd>=0) close(lpDosTask->mm_fd);
    unlink(lpDosTask->mm_name);
   } else
    if (lpDosTask->img!=NULL) VirtualFree(lpDosTask->img,0x110000,MEM_RELEASE);
  }
  return FALSE;
 }
 if (alloc) {
  pModule->dos_image = lpDosTask->img;
  if (!MZ_InitTask( lpDosTask )) {
   MZ_KillModule( lpDosTask );
   /* FIXME: cleanup hModule */
   SetLastError(ERROR_GEN_FAILURE);
   return FALSE;
  }
  if (!PROCESS_Create( pModule, cmdline, env, 0, 0, inherit, startup, info ))
   return FALSE;
 }
 return TRUE;
}

void MZ_KillModule( LPDOSTASK lpDosTask )
{
 TRACE(module,"killing DOS task\n");
#if 0
 SYSTEM_KillSystemTimer(lpDosTask->system_timer);
#endif
 if (lpDosTask->mm_name[0]!=0) {
  munmap(lpDosTask->img,0x110000-START_OFFSET);
  close(lpDosTask->mm_fd);
 } else VirtualFree(lpDosTask->img,0x110000,MEM_RELEASE);
 close(lpDosTask->read_pipe);
 close(lpDosTask->write_pipe);
 kill(lpDosTask->task,SIGTERM);
#if 0
 /* FIXME: this seems to crash */
 if (lpDosTask->dpmi_sel)
  UnMapLS(PTR_SEG_OFF_TO_SEGPTR(lpDosTask->dpmi_sel,0));
#endif
}

#else /* !MZ_SUPPORTED */

BOOL MZ_CreateProcess( HFILE hFile, OFSTRUCT *ofs, LPCSTR cmdline, 
                       LPCSTR env, BOOL inherit, LPSTARTUPINFOA startup, 
                       LPPROCESS_INFORMATION info )
{
 WARN(module,"DOS executables not supported on this architecture\n");
 SetLastError(ERROR_BAD_FORMAT);
 return FALSE;
}

#endif
