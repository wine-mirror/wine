/*
 * DOS EXE loader
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
 */

#ifndef __WINE_DOSEXE_H
#define __WINE_DOSEXE_H

#include "wine/windef16.h"
#include "winbase.h"   /* for LPSTARTUPINFO32A */
#include "winnt.h"     /* for PCONTEXT */
#include "wincon.h"    /* for MOUSE_EVENT_RECORD */
#include "miscemu.h"

struct _DOSEVENT;
struct _STACK16FRAME;

/* amount of space reserved for relay stack */
#define DOSVM_RELAY_DATA_SIZE 4096

/* various real-mode code stubs */
struct DPMI_segments
{
    WORD wrap_seg;
    WORD xms_seg;
    WORD dpmi_seg;
    WORD dpmi_sel;
    WORD int48_sel;
    WORD int16_sel;
    WORD relay_code_sel;
    WORD relay_data_sel;
};

/* 48-bit segmented pointers for DOS DPMI32 */
typedef struct {
  WORD  selector;
  DWORD offset;
} SEGPTR48, FARPROC48;

typedef void (*DOSRELAY)(CONTEXT86*,void*);
typedef void (WINAPI *RMCBPROC)(CONTEXT86*);
typedef void (WINAPI *INTPROC)(CONTEXT86*);

#define DOS_PRIORITY_REALTIME 0  /* IRQ0 */
#define DOS_PRIORITY_KEYBOARD 1  /* IRQ1 */
#define DOS_PRIORITY_VGA      2  /* IRQ9 */
#define DOS_PRIORITY_MOUSE    5  /* IRQ12 */
#define DOS_PRIORITY_SERIAL   10 /* IRQ4 */

extern WORD DOSVM_psp;     /* psp of current DOS task */
extern WORD DOSVM_retval;  /* return value of previous DOS task */
extern struct DPMI_segments *DOSVM_dpmi_segments;

#if defined(linux) && defined(__i386__) && defined(HAVE_SYS_VM86_H)
# define MZ_SUPPORTED
#endif /* linux-i386 */

#define V86_FLAG 0x00020000

#define BIOS_DATA ((void *)0x400)

#define ADD_LOWORD(dw,val)  ((dw) = ((dw) & 0xffff0000) | LOWORD((DWORD)(dw)+(val)))

/* module.c */
extern void WINAPI MZ_LoadImage( LPCSTR filename, HANDLE hFile );
extern BOOL WINAPI MZ_Exec( CONTEXT86 *context, LPCSTR filename, BYTE func, LPVOID paramblk );
extern void WINAPI MZ_Exit( CONTEXT86 *context, BOOL cs_psp, WORD retval );
extern BOOL WINAPI MZ_Current( void );
extern void WINAPI MZ_AllocDPMITask( void );
extern void WINAPI MZ_RunInThread( PAPCFUNC proc, ULONG_PTR arg );
extern BOOL DOSVM_IsWin16(void);

/* dosvm.c */
extern INT WINAPI DOSVM_Enter( CONTEXT86 *context );
extern void WINAPI DOSVM_Wait( INT read_pipe, HANDLE hObject );
extern DWORD WINAPI DOSVM_Loop( HANDLE hThread );
extern void WINAPI DOSVM_QueueEvent( INT irq, INT priority, DOSRELAY relay, LPVOID data );
extern void WINAPI DOSVM_PIC_ioport_out( WORD port, BYTE val );
extern void WINAPI DOSVM_SetTimer( UINT ticks );
extern UINT WINAPI DOSVM_GetTimer( void );
extern void DOSVM_RealModeInterrupt( BYTE intnum, CONTEXT86 *context );

/* devices.c */
extern void DOSDEV_InstallDOSDevices(void);
extern DWORD DOSDEV_Console(void);
extern DWORD DOSDEV_FindCharDevice(char*name);
extern int DOSDEV_Peek(DWORD dev, BYTE*data);
extern int DOSDEV_Read(DWORD dev, DWORD buf, int buflen);
extern int DOSDEV_Write(DWORD dev, DWORD buf, int buflen, int verify);
extern int DOSDEV_IoctlRead(DWORD dev, DWORD buf, int buflen);
extern int DOSDEV_IoctlWrite(DWORD dev, DWORD buf, int buflen);
extern struct _DOS_LISTOFLISTS * DOSMEM_LOL();

/* dma.c */
extern int DMA_Transfer(int channel,int reqlength,void* buffer);
extern void DMA_ioport_out( WORD port, BYTE val );
extern BYTE DMA_ioport_in( WORD port );

/* fpu.c */
extern void WINAPI DOSVM_Int34Handler(CONTEXT86*);
extern void WINAPI DOSVM_Int35Handler(CONTEXT86*);
extern void WINAPI DOSVM_Int36Handler(CONTEXT86*);
extern void WINAPI DOSVM_Int37Handler(CONTEXT86*);
extern void WINAPI DOSVM_Int38Handler(CONTEXT86*);
extern void WINAPI DOSVM_Int39Handler(CONTEXT86*);
extern void WINAPI DOSVM_Int3aHandler(CONTEXT86*);
extern void WINAPI DOSVM_Int3bHandler(CONTEXT86*);
extern void WINAPI DOSVM_Int3cHandler(CONTEXT86*);
extern void WINAPI DOSVM_Int3dHandler(CONTEXT86*);
extern void WINAPI DOSVM_Int3eHandler(CONTEXT86*);

/* himem.c */
extern void DOSVM_InitSegments(void);
extern LPVOID DOSVM_AllocUMB(DWORD);
extern LPVOID DOSVM_AllocCodeUMB(DWORD, WORD *, WORD *);
extern LPVOID DOSVM_AllocDataUMB(DWORD, WORD *, WORD *);

/* int09.c */
extern void WINAPI DOSVM_Int09Handler(CONTEXT86*);
extern void WINAPI DOSVM_Int09SendScan(BYTE scan,BYTE ascii);
extern BYTE WINAPI DOSVM_Int09ReadScan(BYTE*ascii);

/* int10.c */
extern void WINAPI DOSVM_Int10Handler(CONTEXT86*);
extern void WINAPI DOSVM_PutChar(BYTE ascii);

/* int11.c */
extern void WINAPI DOSVM_Int11Handler(CONTEXT86*);

/* int12.c */
extern void WINAPI DOSVM_Int12Handler(CONTEXT86*);

/* int13.c */
extern void WINAPI DOSVM_Int13Handler(CONTEXT86*);

/* int15.c */
extern void WINAPI DOSVM_Int15Handler(CONTEXT86*);

/* int1a.c */
extern void WINAPI DOSVM_Int1aHandler(CONTEXT86*);

/* int16.c */
extern void WINAPI DOSVM_Int16Handler(CONTEXT86*);
extern int WINAPI DOSVM_Int16ReadChar(BYTE*ascii,BYTE*scan,BOOL peek);
extern int WINAPI DOSVM_Int16AddChar(BYTE ascii,BYTE scan);

/* int17.c */
extern void WINAPI DOSVM_Int17Handler(CONTEXT86*);

/* int19.c */
extern void WINAPI DOSVM_Int19Handler(CONTEXT86*);

/* int20.c */
extern void WINAPI DOSVM_Int20Handler(CONTEXT86*);

/* int21.c */
extern void WINAPI DOSVM_Int21Handler(CONTEXT86*);

/* int25.c */
BOOL DOSVM_RawRead( BYTE, DWORD, DWORD, BYTE *, BOOL );
void WINAPI DOSVM_Int25Handler( CONTEXT86 * );

/* int26.c */
BOOL DOSVM_RawWrite( BYTE, DWORD, DWORD, BYTE *, BOOL );
void WINAPI DOSVM_Int26Handler( CONTEXT86 * );

/* int29.c */
extern void WINAPI DOSVM_Int29Handler(CONTEXT86*);

/* int2a.c */
extern void WINAPI DOSVM_Int2aHandler(CONTEXT86*);

/* int2f.c */
extern void WINAPI DOSVM_Int2fHandler(CONTEXT86*);

/* int31.c */
extern void WINAPI DOSVM_Int31Handler(CONTEXT86*);
extern void WINAPI DOSVM_RawModeSwitchHandler(CONTEXT86*);
extern BOOL DOSVM_IsDos32(void);
extern FARPROC16 WINAPI DPMI_AllocInternalRMCB(RMCBPROC);
extern void WINAPI DPMI_FreeInternalRMCB(FARPROC16);
extern int DPMI_CallRMProc(CONTEXT86*,LPWORD,int,int);

/* int33.c */
extern void WINAPI DOSVM_Int33Handler(CONTEXT86*);
extern void WINAPI DOSVM_Int33Message(UINT,WPARAM,LPARAM);
extern void WINAPI DOSVM_Int33Console(MOUSE_EVENT_RECORD*);

/* int41.c */
extern void WINAPI DOSVM_Int41Handler(CONTEXT86*);

/* int4b.c */
extern void WINAPI DOSVM_Int4bHandler(CONTEXT86*);

/* int5c.c */
extern void WINAPI DOSVM_Int5cHandler(CONTEXT86*);

/* int67.c */
extern void WINAPI DOSVM_Int67Handler(CONTEXT86*);
extern void WINAPI EMS_Ioctl_Handler(CONTEXT86*);

/* interrupts.c */
extern FARPROC16 DOSVM_GetRMHandler( BYTE intnum );
extern void DOSVM_SetRMHandler( BYTE intnum, FARPROC16 handler );
extern FARPROC16 DOSVM_GetPMHandler16( BYTE intnum );
extern void DOSVM_SetPMHandler16( BYTE intnum, FARPROC16 handler );
extern FARPROC48 DOSVM_GetPMHandler48( BYTE intnum );
extern void DOSVM_SetPMHandler48( BYTE intnum, FARPROC48 handler );
extern INTPROC DOSVM_GetBuiltinHandler( BYTE intnum );

/* relay.c */
void DOSVM_RelayHandler( CONTEXT86 * );
void DOSVM_SaveCallFrame( CONTEXT86 *, struct _STACK16FRAME * );
void DOSVM_RestoreCallFrame( CONTEXT86 *, struct _STACK16FRAME * );
void DOSVM_BuildCallFrame( CONTEXT86 *, DOSRELAY, LPVOID );

/* soundblaster.c */
extern void SB_ioport_out( WORD port, BYTE val );
extern BYTE SB_ioport_in( WORD port );

/* xms.c */
extern void WINAPI XMS_Handler(CONTEXT86*);

#endif /* __WINE_DOSEXE_H */
