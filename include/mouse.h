/*
 * MOUSE driver interface
 *
 * Copyright 1998 Ulrich Weigand
 */

#ifndef __WINE_MOUSE_H
#define __WINE_MOUSE_H

#include "windef.h"
#include "user.h"

/* Wine internals */

#define WINE_MOUSEEVENT_MAGIC  ( ('M'<<24)|('A'<<16)|('U'<<8)|'S' )
typedef struct _WINE_MOUSEEVENT
{
    DWORD magic;
    DWORD keyState;
    DWORD time;
    HWND hWnd;
} WINE_MOUSEEVENT;

#endif /* __WINE_MOUSE_H */

