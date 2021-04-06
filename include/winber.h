/*
 * Copyright 2021 Hans Leidekker for CodeWeavers
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
 */

#ifndef __WINE_WINBER_H
#define __WINE_WINBER_H

#define LBER_ERROR   (~0L)
#define LBER_DEFAULT (~0L)

typedef int ber_int_t;
typedef unsigned int ber_tag_t;
typedef unsigned int ber_len_t;

BerElement * CDECL ber_alloc_t( int );
void CDECL ber_bvfree( BERVAL * );
int CDECL ber_flatten( BerElement *, BERVAL ** );
void CDECL ber_free( BerElement *, int );
BerElement * CDECL ber_init( BERVAL * );
int WINAPIV ber_printf( BerElement *, char *, ... );
ULONG WINAPIV ber_scanf( BerElement *, char *, ... );

#endif /* __WINE_WINBER_H */
