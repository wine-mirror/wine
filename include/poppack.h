#if defined(__WINE_PSHPACK_H3)
#  ifndef __WINE_INTERNAL_POPPACK
#    undef __WINE_PSHPACK_H3
#  endif
/* Depth == 3 */

#  if defined(__GNUC__) || defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#    if __WINE_PSHPACK_H2 == 1
#      pragma pack(1)
#    elif __WINE_PSHPACK_H2 == 2
#      pragma pack(2)
#    elif __WINE_PSHPACK_H2 == 8
#      pragma pack(8)
#    else
#      pragma pack(4)
#    endif
#  elif !defined(RC_INVOKED)
#    error "Adjusting the alignment is not supported with this compiler"
#  endif

#elif defined(__WINE_PSHPACK_H2)
#  ifndef __WINE_INTERNAL_POPPACK
#    undef __WINE_PSHPACK_H2
#  endif
/* Depth == 2 */

#  if defined(__GNUC__) || defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#    if __WINE_PSHPACK_H == 1
#      pragma pack(1)
#    elif __WINE_PSHPACK_H == 2
#      pragma pack(2)
#    elif __WINE_PSHPACK_H == 8
#      pragma pack(8)
#    else
#      pragma pack(4)
#    endif
#  elif !defined(RC_INVOKED)
#    error "Adjusting the alignment is not supported with this compiler"
#  endif

#elif defined(__WINE_PSHPACK_H)
#  ifndef __WINE_INTERNAL_POPPACK
#    undef __WINE_PSHPACK_H
#  endif
/* Depth == 1 */

#  if defined(__GNUC__) || defined(__SUNPRO_C)
#    pragma pack()
#  elif defined(__SUNPRO_CC)
#    warning "Assuming a default alignment of 4"
#    pragma pack(4)
#  elif !defined(RC_INVOKED)
#    error "Adjusting the alignment is not supported with this compiler"
#  endif

#else
/* Depth == 0 ! */

#error "Popping alignment isn't possible since no alignment has been pushed"

#endif

#undef __WINE_INTERNAL_POPPACK
