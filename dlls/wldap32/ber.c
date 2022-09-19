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
#include "winldap.h"
#include "winber.h"

#include "wine/debug.h"
#include "winldap_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

/***********************************************************************
 *      ber_alloc_t     (WLDAP32.@)
 */
WLDAP32_BerElement * CDECL WLDAP32_ber_alloc_t( int options )
{
    WLDAP32_BerElement *ret;
    struct ber_alloc_t_params params;

    if (!(ret = malloc( sizeof(*ret) ))) return NULL;
    params.options = options;
    params.ret = (void **)&BER(ret);
    if (LDAP_CALL( ber_alloc_t, &params ))
    {
        free( ret );
        return NULL;
    }
    return ret;
}

/***********************************************************************
 *      ber_bvdup     (WLDAP32.@)
 */
BERVAL * CDECL WLDAP32_ber_bvdup( BERVAL *berval )
{
    return bervalWtoW( berval );
}

/***********************************************************************
 *      ber_bvecfree     (WLDAP32.@)
 */
void CDECL WLDAP32_ber_bvecfree( BERVAL **berval )
{
    bvarrayfreeW( berval );
}

/***********************************************************************
 *      ber_bvfree     (WLDAP32.@)
 */
void CDECL WLDAP32_ber_bvfree( BERVAL *berval )
{
    free( berval );
}

/***********************************************************************
 *      ber_first_element     (WLDAP32.@)
 */
ULONG CDECL WLDAP32_ber_first_element( WLDAP32_BerElement *ber, ULONG *len, char **opaque )
{
    struct ber_first_element_params params = { BER(ber), (unsigned int *)len, opaque };
    return LDAP_CALL( ber_first_element, &params );
}

/***********************************************************************
 *      ber_flatten     (WLDAP32.@)
 */
int CDECL WLDAP32_ber_flatten( WLDAP32_BerElement *ber, BERVAL **berval )
{
    struct bervalU *bervalU;
    struct berval *bervalW;
    struct ber_flatten_params params = { BER(ber), &bervalU };

    if (LDAP_CALL( ber_flatten, &params )) return WLDAP32_LBER_ERROR;

    if (!(bervalW = bervalUtoW( bervalU ))) return WLDAP32_LBER_ERROR;
    LDAP_CALL( ber_bvfree, bervalU );
    if (!bervalW) return WLDAP32_LBER_ERROR;
    *berval = bervalW;
    return 0;
}

/***********************************************************************
 *      ber_free     (WLDAP32.@)
 */
void CDECL WLDAP32_ber_free( WLDAP32_BerElement *ber, int freebuf )
{
    struct ber_free_params params = { BER(ber), freebuf };
    LDAP_CALL( ber_free, &params );
    free( ber );
}

/***********************************************************************
 *      ber_init     (WLDAP32.@)
 */
WLDAP32_BerElement * CDECL WLDAP32_ber_init( BERVAL *berval )
{
    struct bervalU *bervalU;
    WLDAP32_BerElement *ret;
    struct ber_init_params params;

    if (!(ret = malloc( sizeof(*ret) ))) return NULL;
    if (!(bervalU = bervalWtoU( berval )))
    {
        free( ret );
        return NULL;
    }
    params.berval = bervalU;
    params.ret = (void **)&BER(ret);
    if (LDAP_CALL( ber_init, &params ))
    {
        free( ret );
        ret = NULL;
    }
    free( bervalU );
    return ret;
}

/***********************************************************************
 *      ber_next_element     (WLDAP32.@)
 */
ULONG CDECL WLDAP32_ber_next_element( WLDAP32_BerElement *ber, ULONG *len, char *opaque )
{
    struct ber_next_element_params params = { BER(ber), (unsigned int *)len, opaque };
    return LDAP_CALL( ber_next_element, &params );
}

/***********************************************************************
 *      ber_peek_tag     (WLDAP32.@)
 */
ULONG CDECL WLDAP32_ber_peek_tag( WLDAP32_BerElement *ber, ULONG *len )
{
    struct ber_peek_tag_params params = { BER(ber), (unsigned int *)len };
    return LDAP_CALL( ber_peek_tag, &params );
}

/***********************************************************************
 *      ber_skip_tag     (WLDAP32.@)
 */
ULONG CDECL WLDAP32_ber_skip_tag( WLDAP32_BerElement *ber, ULONG *len )
{
    struct ber_skip_tag_params params = { BER(ber), (unsigned int *)len };
    return LDAP_CALL( ber_skip_tag, &params );
}

/***********************************************************************
 *      ber_printf     (WLDAP32.@)
 */
int WINAPIV WLDAP32_ber_printf( WLDAP32_BerElement *ber, char *fmt, ... )
{
    va_list list;
    int ret = 0;
    char new_fmt[2];

    new_fmt[1] = 0;
    va_start( list, fmt );
    while (*fmt)
    {
        struct ber_printf_params params = { BER(ber), new_fmt };
        new_fmt[0] = *fmt++;
        switch (new_fmt[0])
        {
        case 'b':
        case 'e':
        case 'i':
            params.arg1 = va_arg( list, int );
            ret = LDAP_CALL( ber_printf, &params );
            break;
        case 'o':
        case 's':
            params.arg1 = (ULONG_PTR)va_arg( list, char * );
            ret = LDAP_CALL( ber_printf, &params );
            break;
        case 't':
            params.arg1 = va_arg( list, unsigned int );
            ret = LDAP_CALL( ber_printf, &params );
            break;
        case 'v':
            params.arg1 = (ULONG_PTR)va_arg( list, char ** );
            ret = LDAP_CALL( ber_printf, &params );
            break;
        case 'V':
        {
            struct berval **array = va_arg( list, struct berval ** );
            struct bervalU **arrayU;
            if (!(arrayU = bvarrayWtoU( array )))
            {
                ret = -1;
                break;
            }
            params.arg1 = (ULONG_PTR)arrayU;
            ret = LDAP_CALL( ber_printf, &params );
            bvarrayfreeU( arrayU );
            break;
        }
        case 'X':
        {
            params.arg1 = (ULONG_PTR)va_arg( list, char * );
            params.arg2 = va_arg( list, int );
            new_fmt[0] = 'B';  /* 'X' is deprecated */
            ret = LDAP_CALL( ber_printf, &params );
            break;
        }
        case 'n':
        case '{':
        case '}':
        case '[':
        case ']':
            ret = LDAP_CALL( ber_printf, &params );
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

    new_fmt[1] = 0;
    va_start( list, fmt );
    while (*fmt)
    {
        struct ber_scanf_params params = { BER(ber), new_fmt };
        new_fmt[0] = *fmt++;
        switch (new_fmt[0])
        {
        case 'a':
        {
            char *str, **ptr = va_arg( list, char ** );
            params.arg1 = &str;
            if ((ret = LDAP_CALL( ber_scanf, &params )) == -1) break;
            *ptr = strdupU( str );
            LDAP_CALL( ldap_memfree, str );
            break;
        }
        case 'b':
        case 'e':
        case 'i':
            params.arg1 = va_arg( list, int * );
            ret = LDAP_CALL( ber_scanf, &params );
            break;
        case 't':
            params.arg1 = va_arg( list, unsigned int * );
            ret = LDAP_CALL( ber_scanf, &params );
            break;
        case 'v':
        {
            char *str, **arrayU, **ptr, ***array = va_arg( list, char *** );
            params.arg1 = &arrayU;
            if ((ret = LDAP_CALL( ber_scanf, &params )) == -1) break;
            *array = strarrayUtoU( arrayU );
            ptr = arrayU;
            while ((str = *ptr))
            {
                LDAP_CALL( ldap_memfree, str );
                ptr++;
            }
            LDAP_CALL( ldap_memfree, arrayU );
            break;
        }
        case 'B':
        {
            char *strU, **str = va_arg( list, char ** );
            int *len = va_arg( list, int * );
            params.arg1 = &strU;
            params.arg2 = len;
            if ((ret = LDAP_CALL( ber_scanf, &params )) == -1) break;
            *str = malloc( *len );
            memcpy( *str, strU, *len );
            LDAP_CALL( ldap_memfree, strU );
            break;
        }
        case 'O':
        {
            struct berval **berval = va_arg( list, struct berval ** );
            struct bervalU *bervalU;
            params.arg1 = &bervalU;
            if ((ret = LDAP_CALL( ber_scanf, &params )) == -1) break;
            *berval = bervalUtoW( bervalU );
            LDAP_CALL( ber_bvfree, bervalU );
            break;
        }
        case 'V':
        {
            struct berval ***array = va_arg( list, struct berval *** );
            struct bervalU **arrayU;
            params.arg1 = &arrayU;
            if ((ret = LDAP_CALL( ber_scanf, &params )) == -1) break;
            *array = bvarrayUtoW( arrayU );
            LDAP_CALL( ber_bvecfree, arrayU );
            break;
        }
        case 'n':
        case 'x':
        case '{':
        case '}':
        case '[':
        case ']':
            ret = LDAP_CALL( ber_scanf, &params );
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
