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
{
    TYMED_HGLOBAL   = 1,
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
#define ICOM_INTERFACE IAdviseSink
#define IAdviseSink_METHODS \
    ICOM_VMETHOD2(OnDataChange, FORMATETC32*,pFormatetc, STGMEDIUM32*,pStgmed) \
    ICOM_VMETHOD2(OnViewChange, DWORD,dwAspect, LONG,lindex) \
    ICOM_VMETHOD1(OnRename,     IMoniker*,pmk) \
    ICOM_VMETHOD (OnSave) \
    ICOM_VMETHOD (OnClose)
#define IAdviseSink_IMETHODS \
    IUnknown_IMETHODS \
    IAdviseSink_METHODS
ICOM_DEFINE(IAdviseSink,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
/*** IUnknown methods ***/
#define IAdviseSink_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IAdviseSink_AddRef(p)             ICOM_CALL (AddRef,p)
#define IAdviseSink_Release(p)            ICOM_CALL (Release,p)
/*** IAdviseSink methods ***/
#define IAdviseSink_OnDataChange(p,a,b) ICOM_CALL2(OnDataChange,p,a,b)
#define IAdviseSink_OnViewChange(p,a,b) ICOM_CALL2(OnViewChange,p,a,b)
#define IAdviseSink_OnRename(p,a)       ICOM_CALL1(OnRename,p,a)
#define IAdviseSink_OnSave(p)           ICOM_CALL (OnSave,p)
#define IAdviseSink_OnClose(p)          ICOM_CALL (OnClose,p)
#endif


/*****************************************************************************
 * IAdviseSink2 interface
 */
#define ICOM_INTERFACE IAdviseSink2
#define IAdviseSink2_METHODS \
    ICOM_VMETHOD1(OnLinkSrcChange, IMoniker*,pmk)
#define IAdviseSink2_IMETHODS \
    IAdviseSink_IMETHODS \
    IAdviseSink2_METHODS
ICOM_DEFINE(IAdviseSink2,IAdviseSink)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
/*** IUnknown methods ***/
#define IAdviseSink2_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IAdviseSink2_AddRef(p)             ICOM_CALL (AddRef,p)
#define IAdviseSink2_Release(p)            ICOM_CALL (Release,p)
/*** IAdviseSink methods ***/
#define IAdviseSink2_OnDataChange(p,a,b) ICOM_CALL2(IAdviseSink,OnDataChange,p,a,b)
#define IAdviseSink2_OnViewChange(p,a,b) ICOM_CALL2(IAdviseSink,OnViewChange,p,a,b)
#define IAdviseSink2_OnRename(p,a)       ICOM_CALL1(IAdviseSink,OnRename,p,a)
#define IAdviseSink2_OnSave(p)           ICOM_CALL (IAdviseSink,OnSave,p)
#define IAdviseSink2_OnClose(p)          ICOM_CALL (IAdviseSink,OnClose,p)
/*** IAdviseSink2 methods ***/
#define IAdviseSink2_OnLinkSrcChange(p,a) ICOM_CALL(OnLinkSrcChange,p,a)
#endif


/*****************************************************************************
 * IDataAdviseHolder interface
 */
#define ICOM_INTERFACE IDataAdviseHolder
#define IDataAdviseHolder_METHODS \
    ICOM_METHOD5(HRESULT,Advise,           IDataObject*,pDataObject, FORMATETC32*,pFetc, DWORD,advf, IAdviseSink*,pAdvise, DWORD*,pdwConnection) \
    ICOM_METHOD1(HRESULT,Unadvise,         DWORD,dwConnection) \
    ICOM_METHOD1(HRESULT,EnumAdvise,       IEnumSTATDATA**,ppenumAdvise) \
    ICOM_METHOD3(HRESULT,SendOnDataChange, IDataObject*,pDataObject, DWORD,dwReserved, DWORD,advf)
#define IDataAdviseHolder_IMETHODS \
    IUnknown_IMETHODS \
    IDataAdviseHolder_METHODS
ICOM_DEFINE(IDataAdviseHolder,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
/*** IUnknown methods ***/
#define IDataAdviseHolder_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDataAdviseHolder_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDataAdviseHolder_Release(p)            ICOM_CALL (Release,p)
/*** IDataAdviseHolder methods ***/
#define IDataAdviseHolder_Advise(p,a,b,c,d,e)       ICOM_CALL5(Advise,p,a,b,c,d,e)
#define IDataAdviseHolder_Unadvise(p,a)             ICOM_CALL1(Unadvise,p,a)
#define IDataAdviseHolder_EnumAdvise(p,a)           ICOM_CALL1(EnumAdvise,p,a)
#define IDataAdviseHolder_SendOnDataChange(p,a,b,c) ICOM_CALL3(SendOnDataChange,p,a,b,c)
#endif

/* FIXME: not implemented */
HRESULT WINAPI CreateDataAdviseHolder(LPDATAADVISEHOLDER* ppDAHolder);


/*****************************************************************************
 * IDataObject interface
 */
#define ICOM_INTERFACE IDataObject
#define IDataObject_METHODS \
    ICOM_METHOD2(HRESULT,GetData,               LPFORMATETC32,pformatetcIn, STGMEDIUM32*,pmedium) \
    ICOM_METHOD2(HRESULT,GetDataHere,           LPFORMATETC32,pformatetc, STGMEDIUM32*,pmedium) \
    ICOM_METHOD1(HRESULT,QueryGetData,          LPFORMATETC32,pformatetc) \
    ICOM_METHOD2(HRESULT,GetCanonicalFormatEtc, LPFORMATETC32,pformatectIn, LPFORMATETC32,pformatetcOut) \
    ICOM_METHOD3(HRESULT,SetData,               LPFORMATETC32,pformatetc, STGMEDIUM32*,pmedium, BOOL32,fRelease) \
    ICOM_METHOD2(HRESULT,EnumFormatEtc,         DWORD,dwDirection, IEnumFORMATETC**,ppenumFormatEtc) \
    ICOM_METHOD4(HRESULT,DAdvise,               LPFORMATETC32*,pformatetc, DWORD,advf, IAdviseSink*,pAdvSink, DWORD*,pdwConnection) \
    ICOM_METHOD1(HRESULT,DUnadvise,             DWORD,dwConnection) \
    ICOM_METHOD1(HRESULT,EnumDAdvise,           IEnumSTATDATA**,ppenumAdvise)
#define IDataObject_IMETHODS \
    IUnknown_IMETHODS \
    IDataObject_METHODS
ICOM_DEFINE(IDataObject,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
/*** IUnknown methods ***/
#define IDataObject_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDataObject_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDataObject_Release(p)            ICOM_CALL (Release,p)
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
#define IEnumFORMATETC_METHODS \
    ICOM_METHOD3(HRESULT,Next,  ULONG,celt, FORMATETC32*,rgelt, ULONG*,pceltFethed) \
    ICOM_METHOD1(HRESULT,Skip,  ULONG,celt) \
    ICOM_METHOD (HRESULT,Reset) \
    ICOM_METHOD1(HRESULT,Clone, IEnumFORMATETC**,ppenum)
#define IEnumFORMATETC_IMETHODS \
    IUnknown_IMETHODS \
    IEnumFORMATETC_METHODS
ICOM_DEFINE(IEnumFORMATETC,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
/*** IUnknown methods ***/
#define IEnumFORMATETC_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IEnumFORMATETC_AddRef(p)             ICOM_CALL (AddRef,p)
#define IEnumFORMATETC_Release(p)            ICOM_CALL (Release,p)
/*** IEnumFORMATETC methods ***/
#define IEnumFORMATETC_Next(p,a,b,c) ICOM_CALL3(Next,p,a,b,c)
#define IEnumFORMATETC_Skip(p,a)     ICOM_CALL1(Skip,p,a)
#define IEnumFORMATETC_Reset(p)      ICOM_CALL (Reset,p)
#define IEnumFORMATETC_Clone(p,a)    ICOM_CALL1(Clone,p,a)
#endif


/*****************************************************************************
 * IEnumSTATDATA interface
 */
typedef struct tagSTATDATA
{
    FORMATETC32 formatetc;
    DWORD advf;
    IAdviseSink* pAdvSink;
    DWORD dwConnection;
} STATDATA32;

#define ICOM_INTERFACE IEnumSTATDATA
#define IEnumSTATDATA_METHODS \
    ICOM_METHOD3(HRESULT,Next,  ULONG,celt, STATDATA32*,rgelt, ULONG*,pceltFethed) \
    ICOM_METHOD1(HRESULT,Skip,  ULONG,celt) \
    ICOM_METHOD (HRESULT,Reset) \
    ICOM_METHOD1(HRESULT,Clone, IEnumSTATDATA**,ppenum)
#define IEnumSTATDATA_IMETHODS \
    IUnknown_IMETHODS \
    IEnumSTATDATA_METHODS
ICOM_DEFINE(IEnumSTATDATA,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
/*** IUnknown methods ***/
#define IEnumSTATDATA_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IEnumSTATDATA_AddRef(p)             ICOM_CALL (AddRef,p)
#define IEnumSTATDATA_Release(p)            ICOM_CALL (Release,p)
/*** IEnumSTATDATA methods ***/
#define IEnumSTATDATA_Next(p,a,b,c) ICOM_CALL3(Next,p,a,b,c)
#define IEnumSTATDATA_Skip(p,a)     ICOM_CALL1(Skip,p,a)
#define IEnumSTATDATA_Reset(p)      ICOM_CALL (Reset,p)
#define IEnumSTATDATA_Clone(p,a)    ICOM_CALL1(Clone,p,a)
#endif


/*****************************************************************************
 * Additional API
 */

/* FIXME: not implemented */
HRESULT WINAPI CreateDataCache(LPUNKNOWN pUnkOuter, REFCLSID rclsid, REFIID iid, LPVOID* ppv);


#endif /* __WINE_WINE_OBJ_DATAOBJECT_H */
