/*
 * VxD emulation
 *
 * Copyright 1995 Anand Kumria
 */

#include <stdio.h>
#include "windows.h"
#include "msdos.h"
#include "miscemu.h"
#include "wine.h"
#include "stddebug.h"
/* #define DEBUG_VXD */
#include "debug.h"


/***********************************************************************
 *           VXD_PageFile
 */
void VXD_PageFile( SIGCONTEXT context )
{
    /* taken from Ralf Brown's Interrupt List */

    dprintf_vxd(stddeb,"VxD PageFile called ...\n");

    switch(AX_reg(&context)) 
    {
    case 0x00: /* get version, is this windows version? */
	dprintf_vxd(stddeb,"VxD PageFile: returning version\n");
        AX_reg(&context) = (WINVERSION >> 8) | ((WINVERSION << 8) & 0xff00);
	RESET_CFLAG(&context);
	break;

    case 0x01: /* get swap file info */
	dprintf_vxd(stddeb,"VxD PageFile: returning swap file info\n");
	AX_reg(&context) = 0x00; /* paging disabled */
	ECX_reg(&context) = 0;   /* maximum size of paging file */	
	/* FIXME: do I touch DS:SI or DS:DI? */
	RESET_CFLAG(&context);
	break;

    case 0x02: /* delete permanent swap on exit */
	dprintf_vxd(stddeb,"VxD PageFile: supposed to delete swap\n");
	RESET_CFLAG(&context);
	break;

    case 0x03: /* current temporary swap file size */
	dprintf_vxd(stddeb,"VxD PageFile: what is current temp. swap size\n");
	RESET_CFLAG(&context);
	break;

    case 0x04: /* read or write?? INTERRUP.D */
    case 0x05: /* cancel?? INTERRUP.D */
    case 0x06: /* test I/O valid INTERRUP.D */
    default:
	INT_BARF( &context, 0x2f);
	break;
    }
}


/***********************************************************************
 *           VXD_Shell
 */
void VXD_Shell( SIGCONTEXT context )
{
    dprintf_vxd(stddeb,"VxD Shell called ...\n");

    switch (DX_reg(&context)) /* Ralf Brown says EDX, but I use DX instead */
    {
    case 0x0000:
	dprintf_vxd(stddeb,"VxD Shell: returning version\n");
        AX_reg(&context) = (WINVERSION >> 8) | ((WINVERSION << 8) & 0xff00);
	EBX_reg(&context) = 1; /* system VM Handle */
	break;

    case 0x0001:
    case 0x0002:
    case 0x0003:
    case 0x0004:
    case 0x0005:
	dprintf_vxd(stddeb,"VxD Shell: EDX = %08lx\n",EDX_reg(&context));
	INT_BARF( &context, 0x2f);
	break;

    case 0x0006: /* SHELL_Get_VM_State */
	dprintf_vxd(stddeb,"VxD Shell: returning VM state\n");
	/* Actually we don't, not yet. We have to return a structure
         * and I am not to sure how to set it up and return it yet,
         * so for now let's do nothing. I can (hopefully) get this
         * by the next release
         */
	/* RESET_CFLAG(&context); */
	break;

    case 0x0007:
    case 0x0008:
    case 0x0009:
    case 0x000A:
    case 0x000B:
    case 0x000C:
    case 0x000D:
    case 0x000E:
    case 0x000F:
    case 0x0010:
    case 0x0011:
    case 0x0012:
    case 0x0013:
    case 0x0014:
    case 0x0015:
    case 0x0016:
    default:
 	dprintf_vxd(stddeb,"VxD Shell: EDX = %08lx\n",EDX_reg(&context)); 
	INT_BARF( &context, 0x2f);
	break;
    }
}


/***********************************************************************
 *           VXD_Comm
 */
void VXD_Comm( SIGCONTEXT context )
{
    dprintf_vxd(stddeb,"VxD Comm called ...\n");

    switch (AX_reg(&context))
    {
    case 0x0000: /* get version */
	dprintf_vxd(stddeb,"VxD Comm: returning version\n");
        AX_reg(&context) = (WINVERSION >> 8) | ((WINVERSION << 8) & 0xff00);
	RESET_CFLAG(&context);
	break;

    case 0x0001: /* set port global */
    case 0x0002: /* get focus */
    case 0x0003: /* virtualise port */
    default:
        INT_BARF( &context, 0x2f);
    }
}
