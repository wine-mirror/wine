/*
 * Copyright 2010 Piotr Caban for CodeWeavers
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

#include "msvcp90.h"
#include "stdio.h"

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msvcp90);

/* char_traits<char> */
/* ?assign@?$char_traits@D@std@@SAXAADABD@Z */
void CDECL MSVCP_char_traits_char_assign(char *ch, const char *assign)
{
    *ch = *assign;
}

/* ?eq@?$char_traits@D@std@@SA_NABD0@Z */
MSVCP_BOOL CDECL MSVCP_char_traits_char_eq(const char *ch1, const char *ch2)
{
    return *ch1 == *ch2;
}

/* ?lt@?$char_traits@D@std@@SA_NABD0@Z */
MSVCP_BOOL CDECL MSVCP_char_traits_lt(const char *ch1, const char *ch2)
{
    return *ch1 < *ch2;
}

/*?compare@?$char_traits@D@std@@SAHPBD0I@Z */
int CDECL MSVCP_char_traits_char_compare(
        const char *s1, const char *s2, unsigned int count)
{
    int ret = memcmp(s1, s2, count);
    return (ret>0 ? 1 : (ret<0 ? -1 : 0));
}

/* ?length@?$char_traits@D@std@@SAIPBD@Z */
int CDECL MSVCP_char_traits_char_length(const char *str)
{
    return strlen(str);
}

/* ?_Copy_s@?$char_traits@D@std@@SAPADPADIPBDI@Z */
char* CDECL MSVCP_char_traits_char__Copy_s(char *dest,
        unsigned int size, const char *src, unsigned int count)
{
    if(!dest || !src || size<count) {
        if(dest && size)
            dest[0] = '\0';
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        return dest;
    }

    return memcpy(dest, src, count);
}

/* ?copy@?$char_traits@D@std@@SAPADPADPBDI@Z */
char* CDECL MSVCP_char_traits_char_copy(
        char *dest, const char *src, unsigned int count)
{
    return MSVCP_char_traits_char__Copy_s(dest, count, src, count);
}

/* ?find@?$char_traits@D@std@@SAPBDPBDIABD@Z */
const char * CDECL MSVCP_char_traits_char_find(
        const char *str, unsigned int range, const char *c)
{
    return memchr(str, *c, range);
}

/* ?_Move_s@?$char_traits@D@std@@SAPADPADIPBDI@Z */
char* CDECL MSVCP_char_traits_char__Move_s(char *dest,
        unsigned int size, const char *src, unsigned int count)
{
    if(!dest || !src || size<count) {
        if(dest && size)
            dest[0] = '\0';
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        return dest;
    }

    return memmove(dest, src, count);
}

/* ?move@?$char_traits@D@std@@SAPADPADPBDI@Z */
char* CDECL MSVCP_char_traits_char_move(
        char *dest, const char *src, unsigned int count)
{
    return MSVCP_char_traits_char__Move_s(dest, count, src, count);
}

/* ?assign@?$char_traits@D@std@@SAPADPADID@Z */
char* CDECL MSVCP_char_traits_char_assignn(char *str, unsigned int num, char c)
{
    return memset(str, c, num);
}

/* ?to_char_type@?$char_traits@D@std@@SADABH@Z */
char CDECL MSVCP_char_traits_char_to_char_type(const int *i)
{
    return (char)*i;
}

/* ?to_int_type@?$char_traits@D@std@@SAHABD@Z */
int CDECL MSVCP_char_traits_char_to_int_type(const char *ch)
{
    return (int)*ch;
}

/* ?eq_int_type@?$char_traits@D@std@@SA_NABH0@Z */
MSVCP_BOOL CDECL MSVCP_char_traits_char_eq_int_type(const int *i1, const int *i2)
{
    return *i1 == *i2;
}

/* ?eof@?$char_traits@D@std@@SAHXZ */
int CDECL MSVCP_char_traits_char_eof(void)
{
    return EOF;
}

/* ?not_eof@?$char_traits@D@std@@SAHABH@Z */
int CDECL MSVCP_char_traits_char_not_eof(int *in)
{
    return (*in==EOF ? !EOF : *in);
}
