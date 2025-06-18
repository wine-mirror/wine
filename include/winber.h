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

void CDECL ber_free( BerElement *, int );
void CDECL ber_bvfree( BERVAL * );

BerElement * CDECL ber_alloc_t( int ) __WINE_DEALLOC(ber_free);
BERVAL * CDECL ber_bvdup( BERVAL * ) __WINE_DEALLOC(ber_bvfree);
void CDECL ber_bvecfree( BERVAL ** );
ULONG CDECL ber_first_element( BerElement *, ULONG *, char ** );
int CDECL ber_flatten( BerElement *, BERVAL ** );
BerElement * CDECL ber_init( BERVAL * ) __WINE_DEALLOC(ber_free);
ULONG CDECL ber_next_element( BerElement *, ULONG *, char * );
ULONG CDECL ber_peek_tag( BerElement *, ULONG * );
int WINAPIV ber_printf( BerElement *, char *, ... );
ULONG WINAPIV ber_scanf( BerElement *, char *, ... );
ULONG CDECL ber_skip_tag( BerElement *, ULONG * );

#endif /* __WINE_WINBER_H */
