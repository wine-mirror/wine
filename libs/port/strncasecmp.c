/*
 * strncasecmp function
 *
 * Copyright 1996 Alexandre Julliard
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

#include "config.h"
#include "wine/port.h"

#include <ctype.h>

#ifndef HAVE_STRNCASECMP
#undef strncasecmp
int strncasecmp( const char *str1, const char *str2, size_t n )
{
    const unsigned char *ustr1 = (const unsigned char *)str1;
    const unsigned char *ustr2 = (const unsigned char *)str2;
    int res;

    if (!n) return 0;
    while ((--n > 0) && *ustr1)
    {
        if ((res = toupper(*ustr1) - toupper(*ustr2))) return res;
        ustr1++;
        ustr2++;
    }
    return toupper(*ustr1) - toupper(*ustr2);
}
#endif /* HAVE_STRNCASECMP */
