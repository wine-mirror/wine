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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#include "wine/debug.h"
#include "winldap_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

/***********************************************************************
 *      ber_alloc_t     (WLDAP32.@)
 */
WLDAP32_BerElement * CDECL WLDAP32_ber_alloc_t( int options )
{
    WLDAP32_BerElement *ret;

    TRACE( "%d\n", options );

    if (options != WLDAP32_LBER_USE_DER || !(ret = malloc( sizeof(*ret) ))) return NULL;
    if ((ret->opaque = (char *)ber_alloc_t( options ))) return ret;
    free( ret );
    return NULL;
}

/***********************************************************************
 *      ber_bvdup     (WLDAP32.@)
 */
BERVAL * CDECL WLDAP32_ber_bvdup( BERVAL *berval )
{
    TRACE( "%p\n", berval );
    return bervalWtoW( berval );
}

/***********************************************************************
 *      ber_bvecfree     (WLDAP32.@)
 */
void CDECL WLDAP32_ber_bvecfree( BERVAL **berval )
{
    TRACE( "%p\n", berval );
    bvarrayfreeW( berval );
}

/***********************************************************************
 *      ber_bvfree     (WLDAP32.@)
 */
void CDECL WLDAP32_ber_bvfree( BERVAL *berval )
{
    TRACE( "berval\n" );
    free( berval );
}

/***********************************************************************
 *      ber_first_element     (WLDAP32.@)
 */
ULONG CDECL WLDAP32_ber_first_element( WLDAP32_BerElement *ber, ULONG *len, char **opaque )
{
    TRACE( "%p, %p, %p\n", ber, len, opaque );
    return ber_first_element( BER(ber), len, opaque );
}

/***********************************************************************
 *      ber_flatten     (WLDAP32.@)
 */
int CDECL WLDAP32_ber_flatten( WLDAP32_BerElement *ber, BERVAL **berval )
{
    struct berval *bervalU;
    struct WLDAP32_berval *bervalW;

    TRACE( "%p, %p\n", ber, berval );

    if (ber_flatten( BER(ber), &bervalU )) return WLDAP32_LBER_ERROR;

    bervalW = bervalUtoW( bervalU );
    ber_bvfree( bervalU );
    if (!bervalW)
        return WLDAP32_LBER_ERROR;

    *berval = bervalW;
    return 0;
}

/***********************************************************************
 *      ber_free     (WLDAP32.@)
 */
void CDECL WLDAP32_ber_free( WLDAP32_BerElement *ber, int freebuf )
{
    TRACE( "%p, %d\n", ber, freebuf );
    ber_free( BER(ber), freebuf );
    free( ber );
}

/***********************************************************************
 *      ber_init     (WLDAP32.@)
 */
WLDAP32_BerElement * CDECL WLDAP32_ber_init( BERVAL *berval )
{
    struct berval *bervalU;
    WLDAP32_BerElement *ret;
    BerElement *ber;

    TRACE( "%p\n", berval );

    if (!(ret = malloc( sizeof(*ret) ))) return NULL;
    if (!(bervalU = bervalWtoU( berval )))
    {
        free( ret );
        return NULL;
    }

    ber = ber_init( bervalU );
    free( bervalU );
    if ((ret->opaque = (char *)ber)) return ret;
    free( ret );
    return NULL;
}

/***********************************************************************
 *      ber_next_element     (WLDAP32.@)
 */
ULONG CDECL WLDAP32_ber_next_element( WLDAP32_BerElement *ber, ULONG *len, char *opaque )
{
    TRACE( "%p, %p, %p\n", ber, len, opaque );
    return ber_next_element( BER(ber), len, opaque );
}

/***********************************************************************
 *      ber_peek_tag     (WLDAP32.@)
 */
ULONG CDECL WLDAP32_ber_peek_tag( WLDAP32_BerElement *ber, ULONG *len )
{
    TRACE( "%p, %p\n", ber, len );
    return ber_peek_tag( BER(ber), len );
}

/***********************************************************************
 *      ber_skip_tag     (WLDAP32.@)
 */
ULONG CDECL WLDAP32_ber_skip_tag( WLDAP32_BerElement *ber, ULONG *len )
{
    TRACE( "%p, %p\n", ber, len );
    return ber_skip_tag( BER(ber), len );
}

/***********************************************************************
 *      ber_printf     (WLDAP32.@)
 */
int WINAPIV WLDAP32_ber_printf( WLDAP32_BerElement *ber, char *fmt, ... )
{
    va_list list;
    int ret = 0;
    char new_fmt[2];

    TRACE( "%p(%p), %s\n", ber, ber->opaque, fmt );

    new_fmt[1] = 0;
    va_start( list, fmt );
    while (*fmt)
    {
        new_fmt[0] = *fmt++;
        switch (new_fmt[0])
        {
        case 'b':
        case 'e':
        case 'i':
        {
            int arg = va_arg( list, int );
            ret = ber_printf( BER(ber), new_fmt, arg );
            break;
        }
        case 'o':
        case 's':
        {
            char *arg = va_arg( list, char * );
            ret = ber_printf( BER(ber), new_fmt, arg );
            break;
        }
        case 't':
        {
            unsigned int arg = va_arg( list, unsigned int );
            ret = ber_printf( BER(ber), new_fmt, arg );
            break;
        }
        case 'v':
        {
            char **arg = va_arg( list, char ** );
            ret = ber_printf( BER(ber), new_fmt, arg );
            break;
        }
        case 'V':
        {
            struct WLDAP32_berval **array = va_arg( list, struct WLDAP32_berval ** );
            struct berval **arrayU;
            if (!(arrayU = bvarrayWtoU( array )))
            {
                ret = -1;
                break;
            }
            ret = ber_printf( BER(ber), new_fmt, arrayU );
            bvarrayfreeU( arrayU );
            break;
        }
        case 'X':
        {
            char *arg = va_arg( list, char * );
            int arg2 = va_arg( list, int );
            new_fmt[0] = 'B';  /* 'X' is deprecated */
            ret = ber_printf( BER(ber), new_fmt, arg, arg2 );
            break;
        }
        case 'n':
        case '{':
        case '}':
        case '[':
        case ']':
            ret = ber_printf( BER(ber), new_fmt );
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
}

/***********************************************************************
 *      ber_scanf     (WLDAP32.@)
 */
ULONG WINAPIV WLDAP32_ber_scanf( WLDAP32_BerElement *ber, char *fmt, ... )
{
    va_list list;
    int ret = 0;
    char new_fmt[2];

    TRACE( "%p, %s\n", ber, fmt );

    new_fmt[1] = 0;
    va_start( list, fmt );
    while (*fmt)
    {
        new_fmt[0] = *fmt++;
        switch (new_fmt[0])
        {
        case 'a':
        {
            char *str, **ptr = va_arg( list, char ** );
            if ((ret = ber_scanf( BER(ber), new_fmt, &str )) == -1) break;
            *ptr = strdup( str );
            ldap_memfree( str );
            break;
        }
        case 'b':
        case 'e':
        case 'i':
        {
            int *arg = va_arg( list, int * );
            ret = ber_scanf( BER(ber), new_fmt, arg );
            break;
        }
        case 't':
        {
            unsigned int *arg = va_arg( list, unsigned int * );
            ret = ber_scanf( BER(ber), new_fmt, arg );
            break;
        }
        case 'v':
        {
            char *str, **arrayU, **ptr, ***array = va_arg( list, char *** );
            if ((ret = ber_scanf( BER(ber), new_fmt, &arrayU )) == -1) break;
            *array = strarrayUtoU( arrayU );
            ptr = arrayU;
            while ((str = *ptr))
            {
                ldap_memfree( str );
                ptr++;
            }
            ldap_memfree( arrayU );
            break;
        }
        case 'B':
        {
            char *strU, **str = va_arg( list, char ** );
            int *len = va_arg( list, int * );
            if ((ret = ber_scanf( BER(ber), new_fmt, &strU, len )) == -1) break;
            *str = malloc( *len );
            memcpy( *str, strU, *len );
            ldap_memfree( strU );
            break;
        }
        case 'O':
        {
            struct WLDAP32_berval **berval = va_arg( list, struct WLDAP32_berval ** );
            struct berval *bervalU;
            if ((ret = ber_scanf( BER(ber), new_fmt, &bervalU )) == -1) break;
            *berval = bervalUtoW( bervalU );
            ber_bvfree( bervalU );
            break;
        }
        case 'V':
        {
            struct WLDAP32_berval ***array = va_arg( list, struct WLDAP32_berval *** );
            struct berval **arrayU;
            if ((ret = ber_scanf( BER(ber), new_fmt, &arrayU )) == -1) break;
            *array = bvarrayUtoW( arrayU );
            ber_bvecfree( arrayU );
            break;
        }
        case 'n':
        case 'x':
        case '{':
        case '}':
        case '[':
        case ']':
            ret = ber_scanf( BER(ber), new_fmt );
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
}
