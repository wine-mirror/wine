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
#    include "poppack.h"
#  elif !defined(__WINE_PSHPACK_H3)
#    define __WINE_PSHPACK_H3 1
     /* Depth == 3 */
#    define __WINE_INTERNAL_POPPACK
#    include "poppack.h"
#  endif

#  if defined(__GNUC__) || defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#    pragma pack(1)
#  elif !defined(RC_INVOKED)
#    error "Adjusting the alignment is not supported with this compiler"
#  endif

#endif
