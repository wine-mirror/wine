/*
 * Copyright (C) 1999 Patrik Stridvall
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

#if defined(__WINE_PSHPACK_H3)

   /* Depth > 3 */
#  error "Alignment nesting > 3 is not supported"

#else

#  if !defined(__WINE_PSHPACK_H)
#    define __WINE_PSHPACK_H  1
     /* Depth == 1 */
#  elif !defined(__WINE_PSHPACK_H2)
#    define __WINE_PSHPACK_H2 1
     /* Depth == 2 */
#    define __WINE_INTERNAL_POPPACK
#    include <poppack.h>
#  elif !defined(__WINE_PSHPACK_H3)
#    define __WINE_PSHPACK_H3 1
     /* Depth == 3 */
#    define __WINE_INTERNAL_POPPACK
#    include <poppack.h>
#  endif

#  if _MSC_VER >= 800
#   pragma warning(disable:4103)
#  endif

#  if defined(__GNUC__) || defined(__SUNPRO_C) || defined(__SUNPRO_CC) || defined(_MSC_VER)
#    pragma pack(1)
#  elif !defined(RC_INVOKED)
#    error "Adjusting the alignment is not supported with this compiler"
#  endif

#endif
