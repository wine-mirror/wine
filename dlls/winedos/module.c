/*
 * DOS (MZ) loader
 *
 * Copyright 1998 Ove Kåven
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Note: This code hasn't been completely cleaned up yet.
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include "windef.h"
#include "wine/winbase16.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "task.h"
#include "file.h"
#include "miscemu.h"
#include "wine/debug.h"
#include "dosexe.h"
#include "dosvm.h"
#include "vga.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);

static BOOL DOSVM_isdosexe;

/**********************************************************************
 *          DOSVM_IsWin16
 * 
 * Return TRUE if we are in Windows process.
 */
BOOL DOSVM_IsWin16(void)
{
  return DOSVM_isdosexe ? FALSE : TRUE;
}

#ifdef MZ_SUPPORTED

#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif

/* define this to try mapping through /proc/pid/mem instead of a temp file,
   but Linus doesn't like mmapping /proc/pid/mem, so it doesn't work for me */
#undef MZ_MAPSELF

#define BIOS_DATA_SEGMENT 0x40
#define PSP_SIZE 0x10

#define SEG16(ptr,seg) ((LPVOID)((BYTE*)ptr+((DWORD)(seg)<<4)))
#define SEGPTR16(ptr,segptr) ((LPVOID)((BYTE*)ptr+((DWORD)SELECTOROF(segptr)<<4)+OFFSETOF(segptr)))

/* structures for EXEC */

typedef struct {
  WORD env_seg;
  DWORD cmdline WINE_PACKED;
  DWORD fcb1 WINE_PACKED;
  DWORD fcb2 WINE_PACKED;
  WORD init_sp;
  WORD init_ss;
  WORD init_ip;
  WORD init_cs;
} ExecBlock;

typedef struct {
  WORD load_seg;
  WORD rel_seg;
} OverlayBlock;

/* global variables */

pid_t dosvm_pid;

static WORD init_cs,init_ip,init_ss,init_sp;
static HANDLE dosvm_thread, loop_thread;
static DWORD dosvm_tid, loop_tid;

static void MZ_Launch(void);
static BOOL MZ_InitTask(void);

static void MZ_CreatePSP( LPVOID lpPSP, WORD env, WORD par )
{
  PDB16*psp=lpPSP;

  psp->int20=0x20CD; /* int 20 */
  /* some programs use this to calculate how much memory they need */
  psp->nextParagraph=0x9FFF; /* FIXME: use a real value */
  /* FIXME: dispatcher */
  psp->savedint22 = DOSVM_GetRMHandler(0x22);
  psp->savedint23 = DOSVM_GetRMHandler(0x23);
  psp->savedint24 = DOSVM_GetRMHandler(0x24);
  psp->parentPSP=par;
  psp->environment=env;
  /* FIXME: more PSP stuff */
}

static void MZ_FillPSP( LPVOID lpPSP, LPBYTE cmdline, int length )
{
  PDB16      *psp = lpPSP;

  while(length > 0 && *cmdline != ' ') {
    length--;
    cmdline++;
  }

  /* command.com does not skip over multiple spaces */

  if(length > 126) {
    /*
     * FIXME: If length > 126 we should put truncated command line to
     *        PSP and store the entire command line in the environment 
     *        variable CMDLINE.
     */
    FIXME("Command line truncated! (length %d > maximum length 126)\n",
          length);
    length = 126;
  }

  psp->cmdLine[0] = length;
  if(length > 0)
    memmove(psp->cmdLine+1, cmdline, length);
  psp->cmdLine[length+1] = '\r';

  /* FIXME: more PSP stuff */
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

static void MZ_InitHandlers(void)
{
 WORD seg;
 LPBYTE start = DOSVM_AllocCodeUMB( sizeof(int08), &seg, 0 );
 memcpy(start,int08,sizeof(int08));
/* INT 08: point it at our tick-incrementing handler */
 ((SEGPTR*)0)[0x08]=MAKESEGPTR(seg,0);
/* INT 1C: just point it to IRET, we don't want to handle it ourselves */
 ((SEGPTR*)0)[0x1C]=MAKESEGPTR(seg,sizeof(int08)-1);
}

static WORD MZ_InitEnvironment( LPCSTR env, LPCSTR name )
{
 unsigned sz=0;
 WORD seg;
 LPSTR envblk;

 if (env) {
  /* get size of environment block */
  while (env[sz++]) sz+=strlen(env+sz)+1;
 } else sz++;
 /* allocate it */
 envblk=DOSMEM_GetBlock(sz+sizeof(WORD)+strlen(name)+1,&seg);
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

static BOOL MZ_InitMemory(void)
{
    /* initialize the memory */
    TRACE("Initializing DOS memory structures\n");
    DOSMEM_Init(TRUE);
    DOSDEV_InstallDOSDevices();

    MZ_InitHandlers();
    return TRUE;
}

static BOOL MZ_DoLoadImage( HANDLE hFile, LPCSTR filename, OverlayBlock *oblk )
{
  IMAGE_DOS_HEADER mz_header;
  DWORD image_start,image_size,min_size,max_size,avail;
  BYTE*psp_start,*load_start,*oldenv;
  int x, old_com=0, alloc;
  SEGPTR reloc;
  WORD env_seg, load_seg, rel_seg, oldpsp_seg;
  DWORD len;

  if (DOSVM_psp) {
    /* DOS process already running, inherit from it */
    PDB16* par_psp = (PDB16*)((DWORD)DOSVM_psp << 4);
    alloc=0;
    oldenv = (LPBYTE)((DWORD)par_psp->environment << 4);
    oldpsp_seg = DOSVM_psp;
  } else {
    /* allocate new DOS process, inheriting from Wine environment */
    alloc=1;
    oldenv = GetEnvironmentStringsA();
    oldpsp_seg = 0;
  }

 SetFilePointer(hFile,0,NULL,FILE_BEGIN);
 if (   !ReadFile(hFile,&mz_header,sizeof(mz_header),&len,NULL)
     || len != sizeof(mz_header)
     || mz_header.e_magic != IMAGE_DOS_SIGNATURE) {
  char *p = strrchr( filename, '.' );
  if (!p || strcasecmp( p, ".com" ))  /* check for .COM extension */
  {
      SetLastError(ERROR_BAD_FORMAT);
      goto load_error;
  }
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

  if (alloc) MZ_InitMemory();

  if (oblk) {
    /* load overlay into preallocated memory */
    load_seg=oblk->load_seg;
    rel_seg=oblk->rel_seg;
    load_start=(LPBYTE)((DWORD)load_seg<<4);
  } else {
    /* allocate environment block */
    env_seg=MZ_InitEnvironment(oldenv, filename);

    /* allocate memory for the executable */
    TRACE("Allocating DOS memory (min=%ld, max=%ld)\n",min_size,max_size);
    avail=DOSMEM_Available();
    if (avail<min_size) {
      ERR("insufficient DOS memory\n");
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      goto load_error;
    }
    if (avail>max_size) avail=max_size;
    psp_start=DOSMEM_GetBlock(avail,&DOSVM_psp);
    if (!psp_start) {
      ERR("error allocating DOS memory\n");
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      goto load_error;
    }
    load_seg=DOSVM_psp+(old_com?0:PSP_SIZE);
    rel_seg=load_seg;
    load_start=psp_start+(PSP_SIZE<<4);
    MZ_CreatePSP(psp_start, env_seg, oldpsp_seg);
  }

 /* load executable image */
 TRACE("loading DOS %s image, %08lx bytes\n",old_com?"COM":"EXE",image_size);
 SetFilePointer(hFile,image_start,NULL,FILE_BEGIN);
 if (!ReadFile(hFile,load_start,image_size,&len,NULL) || len != image_size) {
  SetLastError(ERROR_BAD_FORMAT);
  goto load_error;
 }

 if (mz_header.e_crlc) {
  /* load relocation table */
  TRACE("loading DOS EXE relocation table, %d entries\n",mz_header.e_crlc);
  /* FIXME: is this too slow without read buffering? */
  SetFilePointer(hFile,mz_header.e_lfarlc,NULL,FILE_BEGIN);
  for (x=0; x<mz_header.e_crlc; x++) {
   if (!ReadFile(hFile,&reloc,sizeof(reloc),&len,NULL) || len != sizeof(reloc)) {
    SetLastError(ERROR_BAD_FORMAT);
    goto load_error;
   }
   *(WORD*)SEGPTR16(load_start,reloc)+=rel_seg;
  }
 }

  if (!oblk) {
    init_cs = load_seg+mz_header.e_cs;
    init_ip = mz_header.e_ip;
    init_ss = load_seg+mz_header.e_ss;
    init_sp = mz_header.e_sp;

    TRACE("entry point: %04x:%04x\n",init_cs,init_ip);
  }

  if (alloc && !MZ_InitTask()) {
    SetLastError(ERROR_GEN_FAILURE);
    return FALSE;
  }

  return TRUE;

load_error:
  DOSVM_psp = oldpsp_seg;

  return FALSE;
}

/***********************************************************************
 *		LoadDosExe (WINEDOS.@)
 *
 * Called from Wine loader when a new real-mode DOS process is started.
 * Loads DOS program into memory and executes the program.
 */
void WINAPI MZ_LoadImage( LPCSTR filename, HANDLE hFile )
{
  DOSVM_isdosexe = TRUE;
  if (MZ_DoLoadImage( hFile, filename, NULL )) MZ_Launch();
}

/***********************************************************************
 *		MZ_Exec
 *
 * this may only be called from existing DOS processes
 */
BOOL WINAPI MZ_Exec( CONTEXT86 *context, LPCSTR filename, BYTE func, LPVOID paramblk )
{
  DWORD binType;
  STARTUPINFOA st;
  PROCESS_INFORMATION pe;
  HANDLE hFile;

  BOOL ret = FALSE;

  if(!GetBinaryTypeA(filename, &binType))   /* determine what kind of binary this is */
  {
    return FALSE; /* binary is not an executable */
  }

  /* handle non-dos executables */
  if(binType != SCS_DOS_BINARY)
  {
    if(func == 0) /* load and execute */
    {
      LPBYTE fullCmdLine;
      WORD fullCmdLength;
      LPBYTE psp_start = (LPBYTE)((DWORD)DOSVM_psp << 4);
      PDB16 *psp = (PDB16 *)psp_start;
      ExecBlock *blk = (ExecBlock *)paramblk;
      LPBYTE cmdline = PTR_REAL_TO_LIN(SELECTOROF(blk->cmdline),OFFSETOF(blk->cmdline));
      LPBYTE envblock = PTR_REAL_TO_LIN(psp->environment, 0);
      BYTE cmdLength = cmdline[0];

      /*
       * FIXME: If cmdLength == 126, PSP may contain truncated version
       *        of the full command line. In this case environment
       *        variable CMDLINE contains the entire command line.
       */

      fullCmdLength = (strlen(filename) + 1) + cmdLength + 1; /* filename + space + cmdline + terminating null character */

      fullCmdLine = HeapAlloc(GetProcessHeap(), 0, fullCmdLength);
      if(!fullCmdLine) return FALSE; /* return false on memory alloc failure */

      /* build the full command line from the executable file and the command line being passed in */
      snprintf(fullCmdLine, fullCmdLength, "%s ", filename); /* start off with the executable filename and a space */
      memcpy(fullCmdLine + strlen(fullCmdLine), cmdline + 1, cmdLength); /* append cmdline onto the end */
      fullCmdLine[fullCmdLength - 1] = 0; /* null terminate string */

      ZeroMemory (&st, sizeof(STARTUPINFOA));
      st.cb = sizeof(STARTUPINFOA);
      ret = CreateProcessA (NULL, fullCmdLine, NULL, NULL, TRUE, 0, envblock, NULL, &st, &pe);

      /* wait for the app to finish and clean up PROCESS_INFORMATION handles */
      if(ret)
      {
        WaitForSingleObject(pe.hProcess, INFINITE);  /* wait here until the child process is complete */
        CloseHandle(pe.hProcess);
        CloseHandle(pe.hThread);
      }

      HeapFree(GetProcessHeap(), 0, fullCmdLine);  /* free the memory we allocated */
    }
    else
    {
      FIXME("EXEC type of %d not implemented for non-dos executables\n", func);
      ret = FALSE;
    }

    return ret;
  } /* if(binType != SCS_DOS_BINARY) */


  /* handle dos executables */

  hFile = CreateFileA( filename, GENERIC_READ, FILE_SHARE_READ,
			     NULL, OPEN_EXISTING, 0, 0);
  if (hFile == INVALID_HANDLE_VALUE) return FALSE;

  switch (func) {
  case 0: /* load and execute */
  case 1: /* load but don't execute */
    {
      /* save current process's return SS:SP now */
      LPBYTE psp_start = (LPBYTE)((DWORD)DOSVM_psp << 4);
      PDB16 *psp = (PDB16 *)psp_start;
      psp->saveStack = (DWORD)MAKESEGPTR(context->SegSs, LOWORD(context->Esp));
    }
    ret = MZ_DoLoadImage( hFile, filename, NULL );
    if (ret) {
      /* MZ_LoadImage created a new PSP and loaded new values into it,
       * let's work on the new values now */
      LPBYTE psp_start = (LPBYTE)((DWORD)DOSVM_psp << 4);
      ExecBlock *blk = (ExecBlock *)paramblk;
      LPBYTE cmdline = PTR_REAL_TO_LIN(SELECTOROF(blk->cmdline),OFFSETOF(blk->cmdline));

      /* First character contains the length of the command line. */
      MZ_FillPSP(psp_start, cmdline + 1, cmdline[0]);

      /* the lame MS-DOS engineers decided that the return address should be in int22 */
      DOSVM_SetRMHandler(0x22, (FARPROC16)MAKESEGPTR(context->SegCs, LOWORD(context->Eip)));
      if (func) {
	/* don't execute, just return startup state */
	blk->init_cs = init_cs;
	blk->init_ip = init_ip;
	blk->init_ss = init_ss;
	blk->init_sp = init_sp;
      } else {
	/* execute by making us return to new process */
	context->SegCs = init_cs;
	context->Eip   = init_ip;
	context->SegSs = init_ss;
	context->Esp   = init_sp;
	context->SegDs = DOSVM_psp;
	context->SegEs = DOSVM_psp;
	context->Eax   = 0;
      }
    }
    break;
  case 3: /* load overlay */
    {
      OverlayBlock *blk = (OverlayBlock *)paramblk;
      ret = MZ_DoLoadImage( hFile, filename, blk );
    }
    break;
  default:
    FIXME("EXEC load type %d not implemented\n", func);
    SetLastError(ERROR_INVALID_FUNCTION);
    break;
  }
  CloseHandle(hFile);
  return ret;
}

/***********************************************************************
 *		MZ_AllocDPMITask
 */
void WINAPI MZ_AllocDPMITask( void )
{
  MZ_InitMemory();
  MZ_InitTask();
}

/***********************************************************************
 *		MZ_RunInThread
 */
void WINAPI MZ_RunInThread( PAPCFUNC proc, ULONG_PTR arg )
{
  if (loop_thread) {
    DOS_SPC spc;
    HANDLE event;

    spc.proc = proc;
    spc.arg = arg;
    event = CreateEventA(NULL, TRUE, FALSE, NULL);
    PostThreadMessageA(loop_tid, WM_USER, (WPARAM)event, (LPARAM)&spc);
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);
  } else
    proc(arg);
}

static DWORD WINAPI MZ_DOSVM( LPVOID lpExtra )
{
  CONTEXT context;
  DWORD ret;

  dosvm_pid = getpid();

  memset( &context, 0, sizeof(context) );
  context.SegCs  = init_cs;
  context.Eip    = init_ip;
  context.SegSs  = init_ss;
  context.Esp    = init_sp;
  context.SegDs  = DOSVM_psp;
  context.SegEs  = DOSVM_psp;
  context.EFlags = 0x00080000;  /* virtual interrupt flag */
  DOSVM_SetTimer(0x10000);
  ret = DOSVM_Enter( &context );

  dosvm_pid = 0;
  return ret;
}

static BOOL MZ_InitTask(void)
{
  if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
                       GetCurrentProcess(), &loop_thread,
                       0, FALSE, DUPLICATE_SAME_ACCESS))
    return FALSE;
  dosvm_thread = CreateThread(NULL, 0, MZ_DOSVM, NULL, CREATE_SUSPENDED, &dosvm_tid);
  if (!dosvm_thread) {
    CloseHandle(loop_thread);
    loop_thread = 0;
    return FALSE;
  }
  loop_tid = GetCurrentThreadId();
  return TRUE;
}

static void MZ_Launch(void)
{
  TDB *pTask = GlobalLock16( GetCurrentTask() );
  BYTE *psp_start = PTR_REAL_TO_LIN( DOSVM_psp, 0 );
  LPSTR cmdline = GetCommandLineA();
  DWORD rv;
  SYSLEVEL *lock;

  MZ_FillPSP(psp_start, cmdline, cmdline ? strlen(cmdline) : 0);
  pTask->flags |= TDBF_WINOLDAP;

  /* DTA is set to PSP:0080h when a program is started. */
  pTask->dta = MAKESEGPTR( DOSVM_psp, 0x80 );

  GetpWin16Lock( &lock );
  _LeaveSysLevel( lock );

  ResumeThread(dosvm_thread);
  rv = DOSVM_Loop(dosvm_thread);

  CloseHandle(dosvm_thread);
  dosvm_thread = 0; dosvm_tid = 0;
  CloseHandle(loop_thread);
  loop_thread = 0; loop_tid = 0;

  VGA_Clean();
  ExitThread(rv);
}

/***********************************************************************
 *		MZ_Exit
 */
void WINAPI MZ_Exit( CONTEXT86 *context, BOOL cs_psp, WORD retval )
{
  if (DOSVM_psp) {
    WORD psp_seg = cs_psp ? context->SegCs : DOSVM_psp;
    LPBYTE psp_start = (LPBYTE)((DWORD)psp_seg << 4);
    PDB16 *psp = (PDB16 *)psp_start;
    WORD parpsp = psp->parentPSP; /* check for parent DOS process */
    if (parpsp) {
      /* retrieve parent's return address */
      FARPROC16 retaddr = DOSVM_GetRMHandler(0x22);
      /* restore interrupts */
      DOSVM_SetRMHandler(0x22, psp->savedint22);
      DOSVM_SetRMHandler(0x23, psp->savedint23);
      DOSVM_SetRMHandler(0x24, psp->savedint24);
      /* FIXME: deallocate file handles etc */
      /* free process's associated memory
       * FIXME: walk memory and deallocate all blocks owned by process */
      DOSMEM_FreeBlock( PTR_REAL_TO_LIN(psp->environment,0) );
      DOSMEM_FreeBlock( PTR_REAL_TO_LIN(DOSVM_psp,0) );
      /* switch to parent's PSP */
      DOSVM_psp = parpsp;
      psp_start = (LPBYTE)((DWORD)parpsp << 4);
      psp = (PDB16 *)psp_start;
      /* now return to parent */
      DOSVM_retval = retval;
      context->SegCs = SELECTOROF(retaddr);
      context->Eip   = OFFSETOF(retaddr);
      context->SegSs = SELECTOROF(psp->saveStack);
      context->Esp   = OFFSETOF(psp->saveStack);
      return;
    } else
      TRACE("killing DOS task\n");
  }
  ExitThread( retval );
}


/***********************************************************************
 *		MZ_Current
 */
BOOL WINAPI MZ_Current( void )
{
  return (dosvm_pid != 0); /* FIXME: do a better check */
}

#else /* !MZ_SUPPORTED */

/***********************************************************************
 *		LoadDosExe (WINEDOS.@)
 */
void WINAPI MZ_LoadImage( LPCSTR filename, HANDLE hFile )
{
  WARN("DOS executables not supported on this platform\n");
  SetLastError(ERROR_BAD_FORMAT);
}

/***********************************************************************
 *		MZ_Exec
 */
BOOL WINAPI MZ_Exec( CONTEXT86 *context, LPCSTR filename, BYTE func, LPVOID paramblk )
{
  /* can't happen */
  SetLastError(ERROR_BAD_FORMAT);
  return FALSE;
}

/***********************************************************************
 *		MZ_AllocDPMITask
 */
void WINAPI MZ_AllocDPMITask( void )
{
    ERR("Actual real-mode calls not supported on this platform!\n");
}

/***********************************************************************
 *		MZ_RunInThread
 */
void WINAPI MZ_RunInThread( PAPCFUNC proc, ULONG_PTR arg )
{
    proc(arg);
}

/***********************************************************************
 *		MZ_Exit
 */
void WINAPI MZ_Exit( CONTEXT86 *context, BOOL cs_psp, WORD retval )
{
  ExitThread( retval );
}

/***********************************************************************
 *		MZ_Current
 */
BOOL WINAPI MZ_Current( void )
{
    return FALSE;
}

#endif /* !MZ_SUPPORTED */
