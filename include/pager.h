/*
 * Pager class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_PAGER_H
#define __WINE_PAGER_H


typedef struct tagPAGER_INFO
{
    UINT32  uDummy;  /* this is just a dummy to keep the compiler happy */

} PAGER_INFO;


extern void PAGER_Register (void);

#endif  /* __WINE_PAGER_H */
