/*
 * Date and time picker class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_DATETIME_H
#define __WINE_DATETIME_H

typedef struct tagDATETIME_INFO
{
    DWORD dwDummy;  /* just to keep the compiler happy ;-) */


} DATETIME_INFO, *LPDATETIME_INFO;


extern VOID DATETIME_Register (VOID);
extern VOID DATETIME_Unregister (VOID);

#endif  /* __WINE_DATETIME_H */
