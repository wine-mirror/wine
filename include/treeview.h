/*
 * Treeview class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_TREEVIEW_H
#define __WINE_TREEVIEW_H

typedef struct tagTREEVIEW_INFO
{
    UINT32  uDummy;  /* this is just a dummy to keep the compiler happy */

} TREEVIEW_INFO;


extern void TREEVIEW_Register (void);

#endif  /* __WINE_TREEVIEW_H */
