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
#define ICOM_INTERFACE IEnumString
ICOM_BEGIN(IEnumString,IUnknown)
    ICOM_METHOD3 (HRESULT, Next, ULONG, celt, LPOLESTR32, rgelt, ULONG*, pceltFetched);
    ICOM_METHOD1 (HRESULT, Skip, ULONG, celt);
    ICOM_METHOD  (HRESULT, Reset);
    ICOM_METHOD1 (HRESULT, Clone, IEnumString**, ppenum);
ICOM_END(IEnumString)

#undef ICOM_INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IEnumString_QueryInterface(p,a,b) ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define IEnumString_AddRef(p)             ICOM_ICALL (IUnknown,AddRef,p)
#define IEnumString_Release(p)            ICOM_ICALL (IUnknown,Release,p)
/* IBindCtx methods*/
#define IEnumString_Next(p,a,b,c)         ICOM_CALL3(Next,p,a,b,c);
#define IEnumString_Skip(p,a)             ICOM_CALL1(Skip,p,a);
#define IEnumString_Reset(p,a)            ICOM_CALL(Reset,p);
#define IEnumString_Clone(p,a)            ICOM_CALL1(Clone,p,a);
#endif

#define CreateEnumString WINELIB_NAME(CreateEnumString)



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
