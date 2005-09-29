/*
 * WLDAP32 - LDAP support for Wine
 *
 * Copyright 2005 Hans Leidekker
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

#include "config.h"

#ifndef HAVE_LDAP

#include "windef.h"
#include "winldap.h"

#define LBER_ERROR (~0U)

BerElement *ber_alloc_t( INT options )
{
    return NULL;
}

BERVAL *ber_bvdup( BERVAL *berval )
{
    return NULL;
}

void ber_bvecfree( PBERVAL *berval )
{
}

void ber_bvfree( BERVAL *berval )
{
}

ULONG ber_first_element( BerElement *berelement, ULONG *len, CHAR **opaque )
{
    return LBER_ERROR;
}

INT ber_flatten( BerElement *berelement, PBERVAL *berval )
{
    return LBER_ERROR;
}

void ber_free( BerElement *berelement, INT buf )
{
}

BerElement *ber_init( BERVAL *berval )
{
    return NULL;
}

ULONG ber_next_element( BerElement *berelement, ULONG *len, CHAR *opaque )
{
    return LBER_ERROR;
}

ULONG ber_peek_tag( BerElement *berelement, ULONG *len )
{
    return LBER_ERROR;
}

INT ber_printf( BerElement *berelement, PCHAR fmt, ... )
{
    return LBER_ERROR;
}

ULONG ber_skip_tag( BerElement *berelement, ULONG *len )
{
    return LBER_ERROR;
}

INT ber_scanf( BerElement *berelement, PCHAR fmt, ... )
{
    return LBER_ERROR;
}

#endif /* HAVE_LDAP */
