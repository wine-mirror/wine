/*
 * Status codes returned by MAPI
 *
 * Copyright (C) 2002 Aric Stewart
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

#ifndef MAPICODE_H
#define MAPICODE_H

#include <winerror.h>

#define MAKE_MAPI_SCODE(sev,fac,code) \
    ( (((ULONG)(sev)<<31) | ((ULONG)(fac)<<16) | ((ULONG)(code))) )

#define MAKE_MAPI_E( err ) (MAKE_MAPI_SCODE(1, FACILITY_ITF, err ))

#define MAPI_E_NOT_INITIALIZED              MAKE_MAPI_E( 0x605)


#endif
