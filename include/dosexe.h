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
 HMODULE16 hModule;
 struct vm86plus_struct VM86;
 int fn, state;
#ifdef MZ_USESYSV
 /* SYSV IPC is not quite supported yet... */
 key_t shm_key;
 int shm_id;
#else
 char mm_name[128];
 int mm_fd;
#endif
 int read_pipe,write_pipe;
 pid_t task;
} DOSTASK, *LPDOSTASK;

extern HINSTANCE16 MZ_LoadModule( LPCSTR name, LPCSTR cmdline, LPCSTR env, UINT16 show_cmd );
extern int MZ_RunModule( LPDOSTASK lpDosTask );
extern void MZ_KillModule( LPDOSTASK lpDosTask );
extern int DOSVM_Process( LPDOSTASK lpDosTask );

#endif /* linux */
