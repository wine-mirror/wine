/*
 * DOS EXE loader
 *
 * Copyright 1998 Ove Kåven
 */

#ifdef linux

#include "wintypes.h"

typedef struct _DOSTASK {
 LPVOID img;
 unsigned img_ofs;
 WORD psp_seg,load_seg;
 WORD init_cs,init_ip,init_ss,init_sp;
 WORD xms_seg;
 WORD dpmi_seg,dpmi_sel,dpmi_flag;
 HMODULE16 hModule;
 char mm_name[128];
 int mm_fd;
 int read_pipe,write_pipe;
 pid_t task;
} DOSTASK, *LPDOSTASK;

#define MZ_SUPPORTED

extern int MZ_InitTask( LPDOSTASK lpDosTask );
extern int MZ_InitMemory( LPDOSTASK lpDosTask, NE_MODULE *pModule );
extern void MZ_KillModule( LPDOSTASK lpDosTask );

#endif /* linux */

extern HINSTANCE16 MZ_CreateProcess( LPCSTR name, LPCSTR cmdline, LPCSTR env,
                                     LPSTARTUPINFO32A startup, LPPROCESS_INFORMATION info );
extern int DOSVM_Enter( PCONTEXT context );
