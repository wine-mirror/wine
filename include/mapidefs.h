#ifndef __WINE_MAPIDEFS_H
#define __WINE_MAPIDEFS_H

#include "windef.h"

/* Some types */

typedef unsigned char*          LPBYTE;
#ifndef __LHANDLE
#define __LHANDLE
typedef unsigned long           LHANDLE, *LPLHANDLE;
#endif

#ifndef _tagCY_DEFINED
#define _tagCY_DEFINED
typedef union tagCY
{
   struct {
#ifdef BIG_ENDIAN
        long Hi;
        long Lo;
#else
      unsigned long Lo;
      long Hi;
#endif
   } u;
   LONGLONG int64;
} CY;
#endif /* _tagCY_DEFINED */

/* MAPI Recipient types */
#ifndef MAPI_ORIG
#define MAPI_ORIG        0          /* Message originator   */
#define MAPI_TO          1          /* Primary recipient    */
#define MAPI_CC          2          /* Copy recipient       */
#define MAPI_BCC         3          /* Blind copy recipient */
#define MAPI_P1          0x10000000 /* P1 resend recipient  */
#define MAPI_SUBMITTED   0x80000000 /* Already processed    */
#endif

/* Bit definitions for abFlags[0] */
#define MAPI_SHORTTERM   0x80
#define MAPI_NOTRECIP    0x40
#define MAPI_THISSESSION 0x20
#define MAPI_NOW         0x10
#define MAPI_NOTRESERVED 0x08

/* Bit definitions for abFlags[1]  */
#define MAPI_COMPOUND    0x80

#endif /*__WINE_MAPIDEFS_H*/
