/*
 * DOS EXE loader
 *
 * Copyright 1998 Ove Kåven
 */

#ifndef __WINE_DOSEXE_H
#define __WINE_DOSEXE_H

#include <sys/types.h>
#include "windows.h"
#include "winnt.h"
#include "wintypes.h"

typedef struct _DOSTASK {
 LPVOID img;
 unsigned img_ofs;
 WORD psp_seg,load_seg;
 WORD init_cs,init_ip,init_ss,init_sp;
 WORD xms_seg;
 WORD dpmi_seg,dpmi_sel,dpmi_flag;
 DWORD wrap_ofs,call_ofs;
 HMODULE16 hModule;
 char mm_name[128];
 int mm_fd;
 int read_pipe,write_pipe;
 pid_t task;
} DOSTASK, *LPDOSTASK;

#ifdef linux

#define MZ_SUPPORTED

struct _NE_MODULE;

extern int MZ_InitTask( LPDOSTASK lpDosTask );
extern int MZ_InitMemory( LPDOSTASK lpDosTask, struct _NE_MODULE *pModule );
extern void MZ_KillModule( LPDOSTASK lpDosTask );
extern LPDOSTASK MZ_AllocDPMITask( HMODULE16 hModule );

#endif /* linux */

extern void (*ctx_debug_call)( int sig, CONTEXT* );

extern HINSTANCE16 MZ_CreateProcess( LPCSTR name, LPCSTR cmdline, LPCSTR env,
                                     LPSTARTUPINFO32A startup, LPPROCESS_INFORMATION info );
extern int DOSVM_Enter( PCONTEXT context );

#endif /* __WINE_DOSEXE_H */
