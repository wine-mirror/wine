/*
 * Pager class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_PAGER_H
#define __WINE_PAGER_H


typedef struct tagPAGER_INFO
{
    HWND32   hwndChild;
    COLORREF clrBk;
    INT32    nBorder;
    INT32    nButtonSize;
    INT32    nPos;
    BOOL32   bForward;

    INT32    nChildSize;

} PAGER_INFO;


extern VOID PAGER_Register (VOID);
extern VOID PAGER_Unregister (VOID);

#endif  /* __WINE_PAGER_H */
