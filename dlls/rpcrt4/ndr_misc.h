/*
 * NDR definitions
 *
 * Copyright 2001 Ove Kåven, TransGaming Technologies
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

#ifndef __WINE_NDR_MISC_H
#define __WINE_NDR_MISC_H

#include <stdarg.h>

#define FORMAT_STRING_PARANOIA 20
#define TYPE_FORMAT_STRING_SIZE (5 + FORMAT_STRING_PARANOIA)
#define PROC_FORMAT_STRING_SIZE (9 + FORMAT_STRING_PARANOIA) 

typedef struct _MIDL_TYPE_FORMAT_STRING
{
  short Pad;
  unsigned char Format[TYPE_FORMAT_STRING_SIZE];
} MIDL_TYPE_FORMAT_STRING;

typedef struct _MIDL_PROC_FORMAT_STRING
{
  short Pad;
  unsigned char Format[PROC_FORMAT_STRING_SIZE];
} MIDL_PROC_FORMAT_STRING;

struct IPSFactoryBuffer;

LONG_PTR RPCRT4_NdrClientCall2(PMIDL_STUB_DESC pStubDesc,
			       PFORMAT_STRING pFormat, va_list args );

HRESULT RPCRT4_GetPSFactory(REFIID riid, struct IPSFactoryBuffer **ppPS);

#endif  /* __WINE_NDR_MISC_H */
