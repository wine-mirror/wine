/*
 * NetBIOS interrupt handling
 *
 * Copyright 1995 Alexandre Julliard
 */

#include "wine.h"
#include "miscemu.h"
#include "stddebug.h"
/* #define DEBUG_INT */
#include "debug.h"
#include "registers.h"


/***********************************************************************
 *           do_int5c
 */
int do_int5c(struct sigcontext_struct * context)
{
    dprintf_int(stddeb,"NetBiosCall: AX %04x, BX %04x, CX %04x, DX %04x, "
           "SI %04x, DI %04x, DS %04x, ES %04x\n",
           AX, BX, CX, DX, SI, DI, DS, ES);
    return 0;
}


/***********************************************************************
 *           NetBIOSCall  (KERNEL.103)
 */
void NetBIOSCall( struct sigcontext_struct context )
{
    do_int5c( &context );
}
