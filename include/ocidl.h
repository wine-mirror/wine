/*
 * Copyright (C) 1999 Francis Beaudet
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

#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif

#ifndef __WINE_OCIDL_H
#define __WINE_OCIDL_H

#if defined(__WINESRC__) && !defined(INITGUID) && !defined(__WINE_INCLUDE_OCIDL)
#error DO NOT INCLUDE DIRECTLY
#endif

#define __WINE_INCLUDE_OLEIDL
#include "oleidl.h"
#undef __WINE_INCLUDE_OLEIDL

#define __WINE_INCLUDE_OAIDL
#include "oaidl.h"
#undef __WINE_INCLUDE_OAIDL

#include "wine/obj_olefont.h"
#include "wine/obj_picture.h"

#include "wine/obj_control.h"
#include "wine/obj_connection.h"
#include "wine/obj_property.h"
#include "wine/obj_oleundo.h"

#endif /* __WINE_OCIDL_H */
