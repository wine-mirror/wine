/*
 * Kernel Service Thread API
 *
 * Copyright 1999 Ulrich Weigand
 */

#ifndef __WINE_SERVICES_H
#define __WINE_SERVICES_H

#include "winbase.h"

BOOL SERVICE_Init( void );

void SERVICE_Exit( void );

HANDLE SERVICE_AddObject( HANDLE object,
                          PAPCFUNC callback, ULONG_PTR callback_arg );

HANDLE SERVICE_AddTimer( LONG rate,
                         PAPCFUNC callback, ULONG_PTR callback_arg );

BOOL SERVICE_Delete( HANDLE service );

BOOL SERVICE_Enable( HANDLE service );

BOOL SERVICE_Disable( HANDLE service );


#endif /* __WINE_SERVICES_H */

