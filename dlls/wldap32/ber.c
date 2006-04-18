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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winldap.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

#ifndef LBER_ERROR
# define LBER_ERROR (~0U)
#endif

/***********************************************************************
 *      ber_alloc_t     (WLDAP32.@)
 */
BerElement *WLDAP32_ber_alloc_t( INT options )
{
#ifdef HAVE_LDAP
    return ber_alloc_t( options );
#else
    return NULL;
#endif
}


/***********************************************************************
 *      ber_bvdup     (WLDAP32.@)
 */
BERVAL *WLDAP32_ber_bvdup( BERVAL *berval )
{
#ifdef HAVE_LDAP
    return ber_bvdup( berval );
#else
    return NULL;
#endif
}


/***********************************************************************
 *      ber_bvecfree     (WLDAP32.@)
 */
void WLDAP32_ber_bvecfree( PBERVAL *berval )
{
#ifdef HAVE_LDAP
    ber_bvecfree( berval );
#endif
}


/***********************************************************************
 *      ber_bvfree     (WLDAP32.@)
 */
void WLDAP32_ber_bvfree( BERVAL *berval )
{
#ifdef HAVE_LDAP
    ber_bvfree( berval );
#endif
}


/***********************************************************************
 *      ber_first_element     (WLDAP32.@)
 */
ULONG WLDAP32_ber_first_element( BerElement *berelement, ULONG *len, CHAR **opaque )
{
#ifdef HAVE_LDAP
    return ber_first_element( berelement, len, opaque );
#else
    return LBER_ERROR;
#endif
}


/***********************************************************************
 *      ber_flatten     (WLDAP32.@)
 */
INT WLDAP32_ber_flatten( BerElement *berelement, PBERVAL *berval )
{
#ifdef HAVE_LDAP
    return ber_flatten( berelement, berval );
#else
    return LBER_ERROR;
#endif
}


/***********************************************************************
 *      ber_free     (WLDAP32.@)
 */
void WLDAP32_ber_free( BerElement *berelement, INT buf )
{
#ifdef HAVE_LDAP
    ber_free( berelement, buf );
#endif
}


/***********************************************************************
 *      ber_init     (WLDAP32.@)
 */
BerElement *WLDAP32_ber_init( BERVAL *berval )
{
#ifdef HAVE_LDAP
    return ber_init( berval );
#else
    return NULL;
#endif
}


/***********************************************************************
 *      ber_next_element     (WLDAP32.@)
 */
ULONG WLDAP32_ber_next_element( BerElement *berelement, ULONG *len, CHAR *opaque )
{
#ifdef HAVE_LDAP
    return ber_next_element( berelement, len, opaque );
#else
    return LBER_ERROR;
#endif
}


/***********************************************************************
 *      ber_peek_tag     (WLDAP32.@)
 */
ULONG WLDAP32_ber_peek_tag( BerElement *berelement, ULONG *len )
{
#ifdef HAVE_LDAP
    return ber_peek_tag( berelement, len );
#else
    return LBER_ERROR;
#endif
}


/***********************************************************************
 *      ber_skip_tag     (WLDAP32.@)
 */
ULONG WLDAP32_ber_skip_tag( BerElement *berelement, ULONG *len )
{
#ifdef HAVE_LDAP
    return ber_skip_tag( berelement, len );
#else
    return LBER_ERROR;
#endif
}


/***********************************************************************
 *      ber_printf     (WLDAP32.@)
 */
INT WLDAP32_ber_printf( BerElement *berelement, PCHAR fmt, ... )
{
#ifdef HAVE_LDAP
    va_list list;
    int ret = 0;
    char new_fmt[2];

    new_fmt[1] = 0;
    va_start( list, fmt );
    while (*fmt)
    {
        new_fmt[0] = *fmt++;
        switch(new_fmt[0])
        {
        case 'b':
        case 'e':
        case 'i':
            {
                int i = va_arg( list, int );
                ret = ber_printf( berelement, new_fmt, i );
                break;
            }
        case 'o':
        case 's':
            {
                char *str = va_arg( list, char * );
                ret = ber_printf( berelement, new_fmt, str );
                break;
            }
        case 't':
            {
                unsigned int tag = va_arg( list, unsigned int );
                ret = ber_printf( berelement, new_fmt, tag );
                break;
            }
        case 'v':
            {
                char **array = va_arg( list, char ** );
                ret = ber_printf( berelement, new_fmt, array );
                break;
            }
        case 'V':
            {
                struct berval **array = va_arg( list, struct berval ** );
                ret = ber_printf( berelement, new_fmt, array );
                break;
            }
        case 'X':
            {
                char *str = va_arg( list, char * );
                int len = va_arg( list, int );
                ret = ber_printf( berelement, "B" /* 'X' is deprecated */, str, len );
                break;
            }
        case 'n':
        case '{':
        case '}':
        case '[':
        case ']':
            ret = ber_printf( berelement, new_fmt );
            break;
        default:
            FIXME( "Unknown format '%c'\n", new_fmt[0] );
            ret = -1;
            break;
        }
        if (ret == -1) break;
    }
    va_end( list );
    return ret;
#else
    return LBER_ERROR;
#endif
}


/***********************************************************************
 *      ber_scanf     (WLDAP32.@)
 */
INT WLDAP32_ber_scanf( BerElement *berelement, PCHAR fmt, ... )
{
#ifdef HAVE_LDAP
    va_list list;
    int ret = 0;
    char new_fmt[2];

    new_fmt[1] = 0;
    va_start( list, fmt );
    while (*fmt)
    {
        new_fmt[0] = *fmt++;
        switch(new_fmt[0])
        {
        case 'a':
            {
                char **ptr = va_arg( list, char ** );
                ret = ber_scanf( berelement, new_fmt, ptr );
                break;
            }
        case 'b':
        case 'e':
        case 'i':
            {
                int *i = va_arg( list, int * );
                ret = ber_scanf( berelement, new_fmt, i );
                break;
            }
        case 't':
            {
                unsigned int *tag = va_arg( list, unsigned int * );
                ret = ber_scanf( berelement, new_fmt, tag );
                break;
            }
        case 'v':
            {
                char ***array = va_arg( list, char *** );
                ret = ber_scanf( berelement, new_fmt, array );
                break;
            }
        case 'B':
            {
                char **str = va_arg( list, char ** );
                int *len = va_arg( list, int * );
                ret = ber_scanf( berelement, new_fmt, str, len );
                break;
            }
        case 'O':
            {
                struct berval **ptr = va_arg( list, struct berval ** );
                ret = ber_scanf( berelement, new_fmt, ptr );
                break;
            }
        case 'V':
            {
                struct berval ***array = va_arg( list, struct berval *** );
                ret = ber_scanf( berelement, new_fmt, array );
                break;
            }
        case 'n':
        case 'x':
        case '{':
        case '}':
        case '[':
        case ']':
            ret = ber_scanf( berelement, new_fmt );
            break;
        default:
            FIXME( "Unknown format '%c'\n", new_fmt[0] );
            ret = -1;
            break;
        }
        if (ret == -1) break;
    }
    va_end( list );
    return ret;
#else
    return LBER_ERROR;
#endif
}
