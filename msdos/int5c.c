/*
 * NetBIOS interrupt handling
 *
 * Copyright 1995 Alexandre Julliard, Alex Korobka
 */

#include "ldt.h"
#include "miscemu.h"
#include "stddebug.h"
#include "debug.h"


/***********************************************************************
 *           NetBIOSCall  (KERNEL.103)
 *
 * Also handler for interrupt 5c. 
 */
void WINAPI NetBIOSCall( CONTEXT *context )
{
    BYTE* ptr;
    ptr = (BYTE*) PTR_SEG_OFF_TO_LIN(ES_reg(context),BX_reg(context));
    fprintf(stdnimp,"NetBIOSCall: command code %02x (ignored)\n",*ptr);
    AL_reg(context) = *(ptr+0x01) = 0xFB; /* NetBIOS emulator not found */
}

