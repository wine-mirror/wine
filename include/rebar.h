/*
 * Rebar class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_REBAR_H
#define __WINE_REBAR_H


typedef struct tagREBAR_INFO
{
    UINT32  uDummy;   /* this is just a dummy to keep the compiler happy */

} REBAR_INFO;


extern void REBAR_Register (void);

#endif  /* __WINE_REBAR_H */
