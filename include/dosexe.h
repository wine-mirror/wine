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

struct _DOSEVENT;

typedef struct _DOSTASK {
 WORD psp_seg;
 WORD dpmi_flag;
} DOSTASK, *LPDOSTASK;

#define DOS_PRIORITY_REALTIME 0  /* IRQ0 */
#define DOS_PRIORITY_KEYBOARD 1  /* IRQ1 */
#define DOS_PRIORITY_VGA      2  /* IRQ9 */
#define DOS_PRIORITY_MOUSE    5  /* IRQ12 */
#define DOS_PRIORITY_SERIAL   10 /* IRQ4 */

#if defined(linux) && defined(__i386__)
#define MZ_SUPPORTED
#endif /* linux-i386 */

#define V86_FLAG 0x00020000

extern BOOL MZ_LoadImage( HMODULE module, HANDLE hFile, LPCSTR filename );
extern LPDOSTASK MZ_Current( void );
extern LPDOSTASK MZ_AllocDPMITask( void );
extern int DOSVM_Enter( CONTEXT86 *context );
extern void DOSVM_Wait( int read_pipe, HANDLE hObject );
extern void DOSVM_QueueEvent( int irq, int priority, void (*relay)(CONTEXT86*,void*), void *data );
extern void DOSVM_PIC_ioport_out( WORD port, BYTE val );
extern void DOSVM_SetTimer( unsigned ticks );
extern unsigned DOSVM_GetTimer( void );

#endif /* __WINE_DOSEXE_H */
