/*
 * Defines the COM interfaces and APIs related to IDataObject.
 *
 * Depends on 'obj_moniker.h', 'obj_storage.h' and 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_DATAOBJECT_H
#define __WINE_WINE_OBJ_DATAOBJECT_H


/*****************************************************************************
 * Predeclare the structures
 */
typedef struct DVTARGETDEVICE16 DVTARGETDEVICE16, *LPDVTARGETDEVICE16;
typedef struct DVTARGETDEVICE32 DVTARGETDEVICE32, *LPDVTARGETDEVICE32;
DECL_WINELIB_TYPE(DVTARGETDEVICE)
DECL_WINELIB_TYPE(LPDVTARGETDEVICE)

typedef struct FORMATETC16 FORMATETC16, *LPFORMATETC16;
typedef struct FORMATETC32 FORMATETC32, *LPFORMATETC32;
DECL_WINELIB_TYPE(FORMATETC)
DECL_WINELIB_TYPE(LPFORMATETC)

typedef struct STGMEDIUM32 STGMEDIUM32, *LPSTGMEDIUM32;
DECL_WINELIB_TYPE(STGMEDIUM)
DECL_WINELIB_TYPE(LPSTGMEDIUM)


/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IAdviseSink,		0x0000010fL, 0, 0);
typedef struct IAdviseSink IAdviseSink,*LPADVISESINK;

DEFINE_OLEGUID(IID_IAdviseSink2,	0x00000125L, 0, 0);
typedef struct IAdviseSink2 IAdviseSink2,*LPADVISESINK2;

DEFINE_OLEGUID(IID_IDataAdviseHolder,	0x00000110L, 0, 0);
typedef struct IDataAdviseHolder IDataAdviseHolder,*LPDATAADVISEHOLDER;

DEFINE_OLEGUID(IID_IDataObject,		0x0000010EL, 0, 0);
typedef struct IDataObject IDataObject,*LPDATAOBJECT;

DEFINE_OLEGUID(IID_IEnumFORMATETC,	0x00000103L, 0, 0);
typedef struct IEnumFORMATETC IEnumFORMATETC,*LPENUMFORMATETC;

DEFINE_OLEGUID(IID_IEnumSTATDATA,	0x00000105L, 0, 0);
typedef struct IEnumSTATDATA IEnumSTATDATA,*LPENUMSTATDATA;


/*****************************************************************************
 * DVTARGETDEVICE structure
 */
struct DVTARGETDEVICE32
{
    DWORD tdSize;
    WORD tdDriverNameOffset;
    WORD tdDeviceNameOffset;
    WORD tdPortNameOffset;
    WORD tdExtDevmodeOffset;
    BYTE tdData[1];
};


/*****************************************************************************
 * FORMATETC structure
 */
/* wished data format */
struct FORMATETC32
{
    CLIPFORMAT32 cfFormat;
    DVTARGETDEVICE32* ptd;
    DWORD dwAspect;
    LONG lindex;
    DWORD tymed;
};


/*****************************************************************************
 * STGMEDIUM structure
 */
typedef enum tagTYMED
{	TYMED_HGLOBAL   = 1,
	TYMED_FILE      = 2,
	TYMED_ISTREAM   = 4,
	TYMED_ISTORAGE  = 8,
	TYMED_GDI       = 16,
	TYMED_MFPICT    = 32,
	TYMED_ENHMF     = 64,
	TYMED_NULL      = 0
} TYMED;
  
/* dataobject as answer to a request */
struct STGMEDIUM32
{
    DWORD tymed;
    union {
        HBITMAP32 hBitmap;
        HMETAFILEPICT32 hMetaFilePict;
        HENHMETAFILE32 hEnhMetaFile;
        HGLOBAL32 hGlobal;
        LPOLESTR32 lpszFileName;
        IStream32 *pstm;
        IStorage32 *pstg;
    } u;
    IUnknown *pUnkForRelease;
};   


/*****************************************************************************
 * IAdviseSink interface
 */
/* FIXME: not implemented */
#define ICOM_INTERFACE IAdviseSink
ICOM_BEGIN(IAdviseSink,IUnknown)
ICOM_END(IAdviseSink)
#undef ICOM_INTERFACE


/*****************************************************************************
 * IAdviseSink2 interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IDataAdviseHolder interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IDataObject interface
 */
#define ICOM_INTERFACE IDataObject
ICOM_BEGIN(IDataObject,IUnknown)
    ICOM_METHOD2(HRESULT,GetData,               LPFORMATETC32,pformatetcIn, STGMEDIUM32*,pmedium);
    ICOM_METHOD2(HRESULT,GetDataHere,           LPFORMATETC32,pformatetc, STGMEDIUM32*,pmedium);
    ICOM_METHOD1(HRESULT,QueryGetData,          LPFORMATETC32,pformatetc);
    ICOM_METHOD2(HRESULT,GetCanonicalFormatEtc, LPFORMATETC32,pformatectIn, LPFORMATETC32,pformatetcOut);
    ICOM_METHOD3(HRESULT,SetData,               LPFORMATETC32,pformatetc, STGMEDIUM32*,pmedium, BOOL32,fRelease);
    ICOM_METHOD2(HRESULT,EnumFormatEtc,         DWORD,dwDirection, IEnumFORMATETC**,ppenumFormatEtc);
    ICOM_METHOD4(HRESULT,DAdvise,               LPFORMATETC32*,pformatetc, DWORD,advf, IAdviseSink*,pAdvSink, DWORD*,pdwConnection);
    ICOM_METHOD1(HRESULT,DUnadvise,             DWORD,dwConnection);
    ICOM_METHOD1(HRESULT,EnumDAdvise,           IEnumSTATDATA**,ppenumAdvise);
ICOM_END(IDataObject)
#undef ICOM_INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDataObject_QueryInterface(p,a,b) ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define IDataObject_AddRef(p)             ICOM_ICALL (IUnknown,AddRef,p)
#define IDataObject_Release(p)            ICOM_ICALL (IUnknown,Release,p)
/*** IDataObject methods ***/
#define IDataObject_GetData(p,a,b)               ICOM_CALL2(GetData,p,a,b)
#define IDataObject_GetDataHere(p,a,b)           ICOM_CALL2(GetDataHere,p,a,b)
#define IDataObject_QueryGetData(p,a)            ICOM_CALL1(QueryGetData,p,a)
#define IDataObject_GetCanonicalFormatEtc(p,a,b) ICOM_CALL2(GetCanonicalFormatEtc,p,a,b)
#define IDataObject_SetData(p,a,b,c)             ICOM_CALL3(SetData,p,a,b,c)
#define IDataObject_EnumFormatEtc(p,a,b)         ICOM_CALL2(EnumFormatEtc,p,a,b)
#define IDataObject_DAdvise(p,a,b,c,d)           ICOM_CALL4(DAdvise,p,a,b,c,d)
#define IDataObject_DUnadvise(p,a)               ICOM_CALL1(DUnadvise,p,a)
#define IDataObject_EnumDAdvise(p,a)             ICOM_CALL1(EnumDAdvise,p,a)
#endif


/*****************************************************************************
 * IEnumFORMATETC interface
 */
#define ICOM_INTERFACE IEnumFORMATETC
ICOM_BEGIN(IEnumFORMATETC,IUnknown)
    ICOM_METHOD3(HRESULT,Next,  ULONG,celt, FORMATETC32*,rgelt, ULONG*,pceltFethed);
    ICOM_METHOD1(HRESULT,Skip,  ULONG,celt);
    ICOM_METHOD (HRESULT,Reset);
    ICOM_METHOD1(HRESULT,Clone, IEnumFORMATETC**,ppenum);
ICOM_END(IEnumFORMATETC)
#undef ICOM_INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IEnumFORMATETC_QueryInterface(p,a,b) ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define IEnumFORMATETC_AddRef(p)             ICOM_ICALL (IUnknown,AddRef,p)
#define IEnumFORMATETC_Release(p)            ICOM_ICALL (IUnknown,Release,p)
/*** IEnumFORMATETC methods ***/
#define IEnumFORMATETC_Next(p,a,b,c) ICOM_CALL3(Next,p,a,b,c)
#define IEnumFORMATETC_Skip(p,a)     ICOM_CALL1(Skip,p,a)
#define IEnumFORMATETC_Reset(p)      ICOM_CALL (Reset,p)
#define IEnumFORMATETC_Clone(p,a)    ICOM_CALL1(Clone,p,a)
#endif


/*****************************************************************************
 * IEnumSTATDATA interface
 */
/* FIXME: not implemented */


#endif /* __WINE_WINE_OBJ_DATAOBJECT_H */
