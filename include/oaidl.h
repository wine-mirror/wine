#ifndef __WINE_OAIDL_H
#define __WINE_OAIDL_H


#include "wine/obj_base.h"

/* the following depend only on obj_base.h */
#include "wine/obj_oleaut.h"


/*****************************************************************
 *  SafeArray defines and structs 
 */

#define FADF_AUTO       ( 0x1 )
#define FADF_STATIC     ( 0x2 )
#define FADF_EMBEDDED   ( 0x4 )
#define FADF_FIXEDSIZE  ( 0x10 )
#define FADF_BSTR       ( 0x100 )
#define FADF_UNKNOWN    ( 0x200 )
#define FADF_DISPATCH   ( 0x400 )
#define FADF_VARIANT    ( 0x800 )
#define FADF_RESERVED   ( 0xf0e8 )

typedef struct  tagSAFEARRAYBOUND 
{
  ULONG cElements;                  /* Number of elements in dimension */
  LONG  lLbound;                    /* Lower bound of dimension */
} SAFEARRAYBOUND;

typedef struct  tagSAFEARRAY
{ 
  USHORT          cDims;            /* Count of array dimension */
  USHORT          fFeatures;        /* Flags describing the array */
  ULONG           cbElements;       /* Size of each element */
  ULONG           cLocks;           /* Number of lock on array */
  PVOID           pvData;           /* Pointer to data valid when cLocks > 0 */
  SAFEARRAYBOUND  rgsabound[ 1 ];   /* One bound for each dimension */
} SAFEARRAY, *LPSAFEARRAY;


typedef enum tagCALLCONV {
    CC_CDECL    = 1,
    CC_MSCPASCAL  = CC_CDECL + 1,
    CC_PASCAL   = CC_MSCPASCAL,
    CC_MACPASCAL  = CC_PASCAL + 1,
    CC_STDCALL    = CC_MACPASCAL + 1,
    CC_RESERVED   = CC_STDCALL + 1,
    CC_SYSCALL    = CC_RESERVED + 1,
    CC_MPWCDECL   = CC_SYSCALL + 1,
    CC_MPWPASCAL  = CC_MPWCDECL + 1,
    CC_MAX    = CC_MPWPASCAL + 1
} CALLCONV;

#endif /* _WINE_OAIDL_H */
