#include <stdio.h>
#include <stdlib.h>
#include "registers.h"
#include "wine.h"
#include "miscemu.h"
#include "stddebug.h"
/* #define DEBUG_INT */
#include "debug.h"

typedef struct {
	WORD	accessed    : 1;
	WORD	read_write  : 1;
	WORD	conf_exp    : 1;
	WORD	code        : 1;
	WORD	xsystem     : 1;
	WORD	dpl         : 2;
	WORD	present     : 1;
	WORD	dummy	    : 8;
	} ACCESS;
typedef ACCESS *LPACCESS;

typedef struct {
	WORD	Limit;
	WORD	addr_lo;
	BYTE	addr_hi;
	ACCESS	access; 
	WORD	reserved;
	} DESCRIPTOR;
typedef DESCRIPTOR *LPDESCRIPTOR;

HANDLE DPMI_GetNewSelector(WORD selcount);
BOOL DPMI_FreeSelector(HANDLE pmSel);
BOOL DPMI_SetDescriptor(HANDLE pmSel, LPDESCRIPTOR lpDesc);

/*************************************************************************/

int do_int31(struct sigcontext_struct *context)
{
	LPDESCRIPTOR lpDesc;
	dprintf_int(stddeb,"do_int31 // context->sc_eax=%08lX\n",
		context->sc_eax);
	switch(context->sc_eax)
	{
	case 0x0000:
		context->sc_eax = DPMI_GetNewSelector(context->sc_ecx);
		break;
	case 0x0001:
		context->sc_eax = DPMI_FreeSelector(context->sc_ebx);
		break;
	case 0x000C:
		lpDesc = (LPDESCRIPTOR) MAKELONG(context->sc_edi,
                                                 context->sc_es);
		context->sc_eax = DPMI_SetDescriptor(context->sc_ebx, lpDesc);
		break;
	default:
		IntBarf(0x31, context);
	};
	return 1;
}


/*************************************************************************/


HANDLE DPMI_GetNewSelector(WORD selcount)
{
	LPSTR	ptr;
	HANDLE 	pmSel;
	dprintf_int(stddeb,"DPMI_GetNewSelector(%d); !\n", selcount);
	pmSel = GlobalAlloc(GMEM_FIXED, 4096);
	ptr = GlobalLock(pmSel);
	dprintf_int(stddeb,"DPMI_GetNewSelector() return %04X !\n", pmSel);
	return pmSel;
}


BOOL DPMI_FreeSelector(HANDLE pmSel)
{
	dprintf_int(stddeb,"DPMI_FreeSelector(%04X); !\n", pmSel);
	GlobalFree(pmSel);
	return 0;
}

BOOL DPMI_SetDescriptor(HANDLE pmSel, LPDESCRIPTOR lpDesc)
{
	dprintf_int(stdnimp,"DPMI_SetDescriptor(%04X, %p); !\n", 
		pmSel, lpDesc);
	dprintf_int(stdnimp,"DPMI lpDesc->Limit=%u \n", lpDesc->Limit);
	dprintf_int(stdnimp,"DPMI lpDesc->addr_lo=%04X \n", lpDesc->addr_lo);
	dprintf_int(stdnimp,"DPMI lpDesc->addr_hi=%02X \n", lpDesc->addr_hi);
	dprintf_int(stdnimp,"DPMI lpDesc->access.accessed=%u \n", 
		lpDesc->access.accessed);
	dprintf_int(stdnimp,"DPMI lpDesc->access.read_write=%u \n", 
		lpDesc->access.read_write);
	dprintf_int(stdnimp,"DPMI lpDesc->access.conf_exp=%u \n", 
		lpDesc->access.conf_exp);
	dprintf_int(stdnimp,"DPMI lpDesc->access.code=%u \n", 
		lpDesc->access.code);
	dprintf_int(stdnimp,"DPMI lpDesc->access.xsystem=%u \n", 
		lpDesc->access.xsystem);
	dprintf_int(stdnimp,"DPMI lpDesc->access.dpl=%u \n", 
		lpDesc->access.dpl);
	dprintf_int(stdnimp,"DPMI lpDesc->access.present=%u \n", 
		lpDesc->access.present);
	dprintf_int(stdnimp,"DPMI lpDesc->reserved=%04X \n", lpDesc->reserved);
	return FALSE;
}

