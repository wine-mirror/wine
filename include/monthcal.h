/*
 * Month calendar class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_MONTHCAL_H
#define __WINE_MONTHCAL_H

#include "windef.h"

typedef struct tagMONTHCAL_INFO
{
    DWORD dwDummy;  /* just to keep the compiler happy ;-) */


} MONTHCAL_INFO, *LPMONTHCAL_INFO;


extern VOID MONTHCAL_Register (VOID);
extern VOID MONTHCAL_Unregister (VOID);

#endif  /* __WINE_MONTHCAL_H */
