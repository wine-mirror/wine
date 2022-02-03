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
 *
 * Allocate a berelement structure.
 *
 * PARAMS
 *  options [I] Must be LBER_USE_DER.
 *
 * RETURNS
 *  Success: Pointer to an allocated berelement structure.
 *  Failure: NULL
 *
 * NOTES
 *  Free the berelement structure with ber_free.
 */
BerElement * CDECL ber_alloc_t( int options )
{
    BerElement *ret;
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
 *
 * Copy a berval structure.
 *
 * PARAMS
 *  berval [I] Pointer to the berval structure to be copied.
 *
 * RETURNS
 *  Success: Pointer to a copy of the berval structure.
 *  Failure: NULL
 *
 * NOTES
 *  Free the copy with ber_bvfree.
 */
BERVAL * CDECL ber_bvdup( BERVAL *berval )
{
    return bervalWtoW( berval );
}


/***********************************************************************
 *      ber_bvecfree     (WLDAP32.@)
 *
 * Free an array of berval structures.
 *
 * PARAMS
 *  berval [I] Pointer to an array of berval structures.
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  Use this function only to free an array of berval structures
 *  returned by a call to ber_scanf with a 'V' in the format string.
 */
void CDECL ber_bvecfree( BERVAL **berval )
{
    bvarrayfreeW( berval );
}


/***********************************************************************
 *      ber_bvfree     (WLDAP32.@)
 *
 * Free a berval structure.
 *
 * PARAMS
 *  berval [I] Pointer to a berval structure.
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  Use this function only to free berval structures allocated by
 *  an LDAP API.
 */
void CDECL ber_bvfree( BERVAL *berval )
{
    free( berval );
}


/***********************************************************************
 *      ber_first_element     (WLDAP32.@)
 *
 * Return the tag of the first element in a set or sequence.
 *
 * PARAMS
 *  berelement [I] Pointer to a berelement structure.
 *  len        [O] Receives the length of the first element.
 *  opaque     [O] Receives a pointer to a cookie.
 *
 * RETURNS
 *  Success: Tag of the first element.
 *  Failure: LBER_DEFAULT (no more data).
 *
 * NOTES
 *  len and cookie should be passed to ber_next_element.
 */
ULONG CDECL ber_first_element( BerElement *ber, ULONG *len, char **opaque )
{
    struct ber_first_element_params params = { BER(ber), (unsigned int *)len, opaque };
    return LDAP_CALL( ber_first_element, &params );
}


/***********************************************************************
 *      ber_flatten     (WLDAP32.@)
 *
 * Flatten a berelement structure into a berval structure.
 *
 * PARAMS
 *  berelement [I] Pointer to a berelement structure.
 *  berval    [O] Pointer to a berval structure.
 *
 * RETURNS
 *  Success: 0
 *  Failure: LBER_ERROR
 *
 * NOTES
 *  Free the berval structure with ber_bvfree.
 */
int CDECL ber_flatten( BerElement *ber, BERVAL **berval )
{
    struct bervalU *bervalU;
    struct berval *bervalW;
    struct ber_flatten_params params = { BER(ber), &bervalU };

    if (LDAP_CALL( ber_flatten, &params )) return LBER_ERROR;

    if (!(bervalW = bervalUtoW( bervalU ))) return LBER_ERROR;
    LDAP_CALL( ber_bvfree, bervalU );
    if (!bervalW) return LBER_ERROR;
    *berval = bervalW;
    return 0;
}


/***********************************************************************
 *      ber_free     (WLDAP32.@)
 *
 * Free a berelement structure.
 *
 * PARAMS
 *  berelement [I] Pointer to the berelement structure to be freed.
 *  buf       [I] Flag.
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  Set buf to 0 if the berelement was allocated with ldap_first_attribute
 *  or ldap_next_attribute, otherwise set it to 1.
 */
void CDECL ber_free( BerElement *ber, int freebuf )
{
    struct ber_free_params params = { BER(ber), freebuf };
    LDAP_CALL( ber_free, &params );
    free( ber );
}


/***********************************************************************
 *      ber_init     (WLDAP32.@)
 *
 * Initialise a berelement structure from a berval structure.
 *
 * PARAMS
 *  berval [I] Pointer to a berval structure.
 *
 * RETURNS
 *  Success: Pointer to a berelement structure.
 *  Failure: NULL
 *
 * NOTES
 *  Call ber_free to free the returned berelement structure.
 */
BerElement * CDECL ber_init( BERVAL *berval )
{
    struct bervalU *bervalU;
    BerElement *ret;
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
 *
 * Return the tag of the next element in a set or sequence.
 *
 * PARAMS
 *  berelement [I]   Pointer to a berelement structure.
 *  len        [I/O] Receives the length of the next element.
 *  opaque     [I/O] Pointer to a cookie.
 *
 * RETURNS
 *  Success: Tag of the next element.
 *  Failure: LBER_DEFAULT (no more data).
 *
 * NOTES
 *  len and cookie are initialized by ber_first_element and should
 *  be passed on in subsequent calls to ber_next_element.
 */
ULONG CDECL ber_next_element( BerElement *ber, ULONG *len, char *opaque )
{
    struct ber_next_element_params params = { BER(ber), (unsigned int *)len, opaque };
    return LDAP_CALL( ber_next_element, &params );
}


/***********************************************************************
 *      ber_peek_tag     (WLDAP32.@)
 *
 * Return the tag of the next element.
 *
 * PARAMS
 *  berelement [I] Pointer to a berelement structure.
 *  len        [O] Receives the length of the next element.
 *
 * RETURNS
 *  Success: Tag of the next element.
 *  Failure: LBER_DEFAULT (no more data).
 */
ULONG CDECL ber_peek_tag( BerElement *ber, ULONG *len )
{
    struct ber_peek_tag_params params = { BER(ber), (unsigned int *)len };
    return LDAP_CALL( ber_peek_tag, &params );
}


/***********************************************************************
 *      ber_skip_tag     (WLDAP32.@)
 *
 * Skip the current tag and return the tag of the next element.
 *
 * PARAMS
 *  berelement [I] Pointer to a berelement structure.
 *  len        [O] Receives the length of the skipped element.
 *
 * RETURNS
 *  Success: Tag of the next element.
 *  Failure: LBER_DEFAULT (no more data).
 */
ULONG CDECL ber_skip_tag( BerElement *ber, ULONG *len )
{
    struct ber_skip_tag_params params = { BER(ber), (unsigned int *)len };
    return LDAP_CALL( ber_skip_tag, &params );
}


/***********************************************************************
 *      ber_printf     (WLDAP32.@)
 *
 * Encode a berelement structure.
 *
 * PARAMS
 *  berelement [I/O] Pointer to a berelement structure.
 *  fmt        [I]   Format string.
 *  ...        [I]   Values to encode.
 *
 * RETURNS
 *  Success: Non-negative number.
 *  Failure: LBER_ERROR
 *
 * NOTES
 *  berelement must have been allocated with ber_alloc_t. This function
 *  can be called multiple times to append data.
 */
int WINAPIV ber_printf( BerElement *ber, char *fmt, ... )
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
 *
 * Decode a berelement structure.
 *
 * PARAMS
 *  berelement [I/O] Pointer to a berelement structure.
 *  fmt        [I]   Format string.
 *  ...        [I]   Pointers to values to be decoded.
 *
 * RETURNS
 *  Success: Non-negative number.
 *  Failure: LBER_ERROR
 *
 * NOTES
 *  berelement must have been allocated with ber_init. This function
 *  can be called multiple times to decode data.
 */
ULONG WINAPIV ber_scanf( BerElement *ber, char *fmt, ... )
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
