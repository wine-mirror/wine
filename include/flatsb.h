/*
 * Date and time picker class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_FLATSB_H
#define __WINE_FLATSB_H

typedef struct tagFLATSB_INFO
{
    DWORD dwDummy;  /* just to keep the compiler happy ;-) */

} FLATSB_INFO, *LPFLATSB_INFO;


extern VOID FLATSB_Register (VOID);
extern VOID FLATSB_Unregister (VOID);

#endif  /* __WINE_FLATSB_H */
