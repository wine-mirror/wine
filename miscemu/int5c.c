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
 *           NetBIOSCall  (KERNEL.103)
 *
 * Also handler for interrupt 5c.
 */
void NetBIOSCall( struct sigcontext_struct sigcontext )
{
#define context (&sigcontext)
    INT_BARF( 0x5c );
#undef context
}
