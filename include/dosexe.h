/*
 * DOS EXE loader
 *
 * Copyright 1998 Ove Kåven
 */

#ifndef __WINE_DOSEXE_H
#define __WINE_DOSEXE_H

#include <sys/types.h> /* pid_t */
#include "winbase.h"   /* for LPSTARTUPINFO32A */
#include "sig_context.h"

typedef struct _DOSTASK {
 LPVOID img;
 unsigned img_ofs;
 WORD psp_seg,load_seg;
 WORD init_cs,init_ip,init_ss,init_sp;
 WORD xms_seg;
 WORD dpmi_seg,dpmi_sel,dpmi_flag;
 WORD system_timer;
 HMODULE16 hModule;
 char mm_name[128];
 int mm_fd;
 int read_pipe,write_pipe;
 pid_t task;
} DOSTASK, *LPDOSTASK;

#if defined(linux) && defined(__i386__)

#define MZ_SUPPORTED

extern BOOL MZ_InitTask( LPDOSTASK lpDosTask );
extern void MZ_KillModule( LPDOSTASK lpDosTask );
extern LPDOSTASK MZ_AllocDPMITask( HMODULE16 hModule );

#endif /* linux-i386 */

#define V86_FLAG 0x00020000

extern void MZ_Tick( WORD handle );

extern BOOL MZ_CreateProcess( HFILE hFile, OFSTRUCT *ofs, LPCSTR cmdline, LPCSTR env, 
                              LPSECURITY_ATTRIBUTES psa, LPSECURITY_ATTRIBUTES tsa,
                              BOOL inherit, LPSTARTUPINFOA startup, 
                              LPPROCESS_INFORMATION info );
extern int DOSVM_Enter( PCONTEXT context );
extern void DOSVM_SetTimer( unsigned ticks );
extern unsigned DOSVM_GetTimer( void );

#endif /* __WINE_DOSEXE_H */
