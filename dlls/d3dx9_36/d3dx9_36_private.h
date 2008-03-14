/*
 * Copyright (C) 2008 Tony Wasserka
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#ifndef __WINE_D3DX9_36_PRIVATE_H
#define __WINE_D3DX9_36_PRIVATE_H

#include <stdarg.h>

#define COBJMACROS
#include "wingdi.h"
#include "d3dx9.h"


typedef struct ID3DXFontImpl
{
  /* IUnknown fields */
  const ID3DXFontVtbl *lpVtbl;
  LONG ref;

  /* ID3DXFont fields */
} ID3DXFontImpl;


#endif /* __WINE_D3DX9_36_PRIVATE_H */
