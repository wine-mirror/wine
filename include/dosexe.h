/*
 * DOS EXE loader
 *
 * Copyright 1998 Ove Kåven
 */

#ifndef __WINE_DOSEXE_H
#define __WINE_DOSEXE_H

#include <sys/types.h> /* pid_t */
#include "winbase.h"   /* for LPSTARTUPINFO32A */
#include "winnt.h"     /* for PCONTEXT */

typedef struct _DOSSYSTEM {
  int id;
  void *data;
  struct _DOSSYSTEM *next;
} DOSSYSTEM;

struct _DOSEVENT;

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
 HANDLE hReadPipe,hXPipe,hConInput,hConOutput;
 int read_pipe,write_pipe;
 pid_t task;
 int sig_sent;
 struct _DOSEVENT *pending,*current;
 DOSSYSTEM *sys;
} DOSTASK, *LPDOSTASK;

typedef struct _DOSEVENT {
  int irq,priority;
  void (*relay)(LPDOSTASK,CONTEXT86*,void*);
  void *data;
  struct _DOSEVENT *next;
} DOSEVENT, *LPDOSEVENT;

#define DOS_PRIORITY_REALTIME 0  /* IRQ0 */
#define DOS_PRIORITY_KEYBOARD 1  /* IRQ1 */
#define DOS_PRIORITY_VGA      2  /* IRQ9 */
#define DOS_PRIORITY_MOUSE    5  /* IRQ12 */
#define DOS_PRIORITY_SERIAL   10 /* IRQ4 */

#if defined(linux) && defined(__i386__)

#define MZ_SUPPORTED

extern BOOL MZ_InitTask( LPDOSTASK lpDosTask );
extern void MZ_KillModule( LPDOSTASK lpDosTask );
extern LPDOSTASK MZ_AllocDPMITask( HMODULE16 hModule );

#endif /* linux-i386 */

#define V86_FLAG 0x00020000

extern BOOL MZ_CreateProcess( HANDLE hFile, LPCSTR filename, LPCSTR cmdline, LPCSTR env, 
                              LPSECURITY_ATTRIBUTES psa, LPSECURITY_ATTRIBUTES tsa,
                              BOOL inherit, DWORD flags, LPSTARTUPINFOA startup, 
                              LPPROCESS_INFORMATION info );
extern LPDOSTASK MZ_Current( void );
extern int DOSVM_Enter( CONTEXT86 *context );
extern void DOSVM_Wait( int read_pipe, HANDLE hObject );
extern void DOSVM_QueueEvent( int irq, int priority, void (*relay)(LPDOSTASK,CONTEXT86*,void*), void *data );
extern void DOSVM_PIC_ioport_out( WORD port, BYTE val );
extern void DOSVM_SetTimer( unsigned ticks );
extern unsigned DOSVM_GetTimer( void );
extern void DOSVM_SetSystemData( int id, void *data );
extern void* DOSVM_GetSystemData( int id );

#endif /* __WINE_DOSEXE_H */
