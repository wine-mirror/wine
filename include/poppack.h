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
#  ifndef __WINE_INTERNAL_POPPACK
#    undef __WINE_PSHPACK_H3
#  endif
/* Depth == 3 */

#  if __WINE_PSHPACK_H2 == 1
#    pragma pack(1)
#  elif __WINE_PSHPACK_H2 == 2
#    pragma pack(2)
#  elif __WINE_PSHPACK_H2 == 8
#    pragma pack(8)
#  else
#    pragma pack(4)
#  endif

#elif defined(__WINE_PSHPACK_H2)
#  ifndef __WINE_INTERNAL_POPPACK
#    undef __WINE_PSHPACK_H2
#  endif
/* Depth == 2 */

#  if __WINE_PSHPACK_H == 1
#    pragma pack(1)
#  elif __WINE_PSHPACK_H == 2
#    pragma pack(2)
#  elif __WINE_PSHPACK_H == 8
#    pragma pack(8)
#  else
#    pragma pack(4)
#  endif

#elif defined(__WINE_PSHPACK_H)
#  ifndef __WINE_INTERNAL_POPPACK
#    undef __WINE_PSHPACK_H
#  endif
/* Depth == 1 */

#  if defined(__SUNPRO_CC)
#    warning "Assuming a default alignment of 4"
#    pragma pack(4)
#  else
#    pragma pack()
#  endif

#else
/* Depth == 0 ! */

#error "Popping alignment isn't possible since no alignment has been pushed"

#endif

#undef __WINE_INTERNAL_POPPACK
