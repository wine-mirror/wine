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

#ifndef __WINE_OLEIDL_H
#define __WINE_OLEIDL_H

#if defined(__WINESRC__) && !defined(INITGUID) && !defined(__WINE_INCLUDE_OLEIDL)
#error DO NOT INCLUDE DIRECTLY
#endif

#include "wine/obj_inplace.h"
#include "wine/obj_cache.h"
#include "wine/obj_oleobj.h"
#include "wine/obj_oleview.h"
#include "wine/obj_errorinfo.h"
#include "wine/obj_dragdrop.h"

#endif /* __WINE_OLEIDL_H */
