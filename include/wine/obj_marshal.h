/*
 * Defines the COM interfaces and APIs that allow an interface to 
 * specify a custom marshaling for its objects.
 *
 * Depends on 'obj_storage.h' and 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_MARSHAL_H
#define __WINE_WINE_OBJ_MARSHAL_H


/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IMarshal,		0x00000003L, 0, 0);
typedef struct IMarshal IMarshal,*LPMARSHAL;

DEFINE_OLEGUID(IID_IStdMarshalInfo,	0x00000018L, 0, 0);
typedef struct IStdMarshalInfo IStdMarshalInfo,*LPSTDMARSHALINFO;


/*****************************************************************************
 * IMarshal interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IStdMarshalInfo interface
 */
/* FIXME: not implemented */


#endif /* __WINE_WINE_OBJ_MARSHAL_H */
