/*
 * Defines the COM interfaces and APIs related to saving properties to file.
 *
 * Depends on 'obj_storage.h' and 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_PROPERTYSTORAGE_H
#define __WINE_WINE_OBJ_PROPERTYSTORAGE_H


/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IEnumSTATPROPSETSTG,	0x0000013bL, 0, 0);
typedef struct IEnumSTATPROPSETSTG IEnumSTATPROPSETSTG,*LPENUMSTATPROPSETSTG;

DEFINE_OLEGUID(IID_IEnumSTATPROPSTG,	0x00000139L, 0, 0);
typedef struct IEnumSTATPROPSTG IEnumSTATPROPSTG,*LPENUMSTATPROPSTG;

DEFINE_OLEGUID(IID_IPropertySetStorage,	0x0000013aL, 0, 0);
typedef struct IPropertySetStorage IPropertySetStorage,*LPPROPERTYSETSTORAGE;

DEFINE_OLEGUID(IID_IPropertyStorage,	0x00000138L, 0, 0);
typedef struct IPropertyStorage IPropertyStorage,*LPPROPERTYSTORAGE;


/*****************************************************************************
 * IEnumSTATPROPSETSTG interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IEnumSTATPROPSTG interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IPropertySetStorage interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IPropertyStorage interface
 */
/* FIXME: not implemented */



#endif /* __WINE_WINE_OBJ_PROPERTYSTORAGE_H */
