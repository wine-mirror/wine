/*
 * Instance data declaration
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_INSTANCE_H
#define __WINE_INSTANCE_H

#include "wintypes.h"

  /* This structure is always located at offset 0 of the DGROUP segment */

typedef struct
{
    WORD null;        /* Always 0 */
    WORD old_sp;      /* Stack pointer; used by SwitchTaskTo() */
    WORD old_ss;      /* Stack segment; used by SwitchTaskTo() */
    WORD heap;        /* Pointer to the local heap information (if any) */
    WORD atomtable;   /* Pointer to the local atom table (if any) */ 
    WORD stacktop;    /* Top of the stack */
    WORD stackmin;    /* Lowest stack address used so far */
    WORD stackbottom; /* Bottom of the stack */
} INSTANCEDATA;

#endif /* __WINE_INSTANCE_H */
