/*
 * DOS EXE loader
 *
 * Copyright 1998 Ove Kåven
 */

#ifdef linux

#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/vm86.h>
#include "wintypes.h"

typedef struct _DOSTASK {
 LPVOID img;
 unsigned img_ofs;
 WORD psp_seg,load_seg;
 HMODULE16 hModule;
 struct vm86plus_struct VM86;
 int fn, state;
 char mm_name[128];
 int mm_fd;
 int read_pipe,write_pipe;
 pid_t task;
} DOSTASK, *LPDOSTASK;

#define MZ_SUPPORTED

extern int MZ_InitTask( LPDOSTASK lpDosTask );
extern int MZ_InitMemory( LPDOSTASK lpDosTask, NE_MODULE *pModule );
extern int MZ_RunModule( LPDOSTASK lpDosTask );
extern void MZ_KillModule( LPDOSTASK lpDosTask );
extern int DOSVM_Process( LPDOSTASK lpDosTask );

#endif /* linux */

extern HINSTANCE16 MZ_CreateProcess( LPCSTR name, LPCSTR cmdline, LPCSTR env,
                                     LPSTARTUPINFO32A startup, LPPROCESS_INFORMATION info );
extern int DOSVM_Enter( PCONTEXT context );
