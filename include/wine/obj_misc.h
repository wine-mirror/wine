/*
 * Defines miscellaneous COM interfaces and APIs defined in objidl.h.
 * These did not really fit into the other categories, whould have 
 * required their own specific category or are too rarely used to be 
 * put in 'obj_base.h'.
 *
 * Depends on 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_MISC_H
#define __WINE_WINE_OBJ_MISC_H


/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IEnumString,		0x00000101L, 0, 0);
typedef struct IEnumString IEnumString,*LPENUMSTRING;

DEFINE_OLEGUID(IID_IEnumUnknown,	0x00000100L, 0, 0);
typedef struct IEnumUnknown IEnumUnknown,*LPENUMUNKNOWN;

DEFINE_OLEGUID(IID_IMallocSpy,		0x0000001dL, 0, 0);
typedef struct IMallocSpy IMallocSpy,*LPMALLOCSPY;

DEFINE_OLEGUID(IID_IMultiQI,		0x00000020L, 0, 0);
typedef struct IMultiQI IMultiQI,*LPMULTIQI;


/*****************************************************************************
 * IEnumString interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IEnumUnknown interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IMallocSpy interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IMultiQI interface
 */
/* FIXME: not implemented */


#endif /* __WINE_WINE_OBJ_MISC_H */
