/*
 * Kernel Service Thread API
 *
 * Copyright 1999 Ulrich Weigand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_SERVICES_H
#define __WINE_SERVICES_H

#include "winbase.h"

HANDLE SERVICE_AddObject( HANDLE object,
                          PAPCFUNC callback, ULONG_PTR callback_arg );

HANDLE SERVICE_AddTimer( LONG rate,
                         PAPCFUNC callback, ULONG_PTR callback_arg );

BOOL SERVICE_Delete( HANDLE service );

BOOL SERVICE_Enable( HANDLE service );

BOOL SERVICE_Disable( HANDLE service );


#endif /* __WINE_SERVICES_H */

