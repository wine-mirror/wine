#include <stdio.h>
#include <stdlib.h>
#include "registers.h"
#include "wine.h"

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
	printf("do_int31 // context->sc_eax=%04X\n", context->sc_eax);
	switch(context->sc_eax)
	{
	case 0x0000:
		context->sc_eax = DPMI_GetNewSelector(context->sc_ecx);
		break;
	case 0x0001:
		context->sc_eax = DPMI_FreeSelector(context->sc_ebx);
		break;
	case 0x000C:
		lpDesc = MAKELONG(context->sc_edi, context->sc_es);
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
	printf("DPMI_GetNewSelector(%d); !\n", selcount);
	pmSel = GlobalAlloc(GMEM_FIXED, 4096);
	ptr = GlobalLock(pmSel);
	printf("DPMI_GetNewSelector() return %04X !\n", pmSel);
	return pmSel;
}


BOOL DPMI_FreeSelector(HANDLE pmSel)
{
	printf("DPMI_FreeSelector(%04X); !\n", pmSel);
	GlobalFree(pmSel);
	return 0;
}

BOOL DPMI_SetDescriptor(HANDLE pmSel, LPDESCRIPTOR lpDesc)
{
	printf("DPMI_SetDescriptor(%04X, %08X); !\n", pmSel, lpDesc);
	printf("DPMI lpDesc->Limit=%u \n", lpDesc->Limit);
	printf("DPMI lpDesc->addr_lo=%04X \n", lpDesc->addr_lo);
	printf("DPMI lpDesc->addr_hi=%02X \n", lpDesc->addr_hi);
	printf("DPMI lpDesc->access.accessed=%u \n", lpDesc->access.accessed);
	printf("DPMI lpDesc->access.read_write=%u \n", lpDesc->access.read_write);
	printf("DPMI lpDesc->access.conf_exp=%u \n", lpDesc->access.conf_exp);
	printf("DPMI lpDesc->access.code=%u \n", lpDesc->access.code);
	printf("DPMI lpDesc->access.xsystem=%u \n", lpDesc->access.xsystem);
	printf("DPMI lpDesc->access.dpl=%u \n", lpDesc->access.dpl);
	printf("DPMI lpDesc->access.present=%u \n", lpDesc->access.present);
	printf("DPMI lpDesc->reserved=%04X \n", lpDesc->reserved);
	return FALSE;
}

