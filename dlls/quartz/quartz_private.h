/*
 * Copyright (C) Hidenori TAKESHIMA
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

#ifndef	QUARTZ_PRIVATE_H
#define	QUARTZ_PRIVATE_H

typedef HRESULT (*QUARTZ_pCreateIUnknown)(IUnknown* punkOuter,void** ppobj);

void* QUARTZ_AllocObj( DWORD dwSize );
void QUARTZ_FreeObj( void* pobj );
void* QUARTZ_AllocMem( DWORD dwSize );
void QUARTZ_FreeMem( void* pMem );
void* QUARTZ_ReallocMem( void* pMem, DWORD dwSize );

#define QUARTZ_TIMEUNITS		((LONGLONG)10000000)

/* undocument APIs. */
LONG WINAPI QUARTZ_AmpFactorToDB( LONG amp );
LONG WINAPI QUARTZ_DBToAmpFactor( LONG dB );


#endif	/* QUARTZ_PRIVATE_H */

