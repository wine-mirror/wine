/*
 * Defines the COM interfaces and APIs related to structured data storage.
 *
 * Depends on 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_STORAGE_H
#define __WINE_WINE_OBJ_STORAGE_H


#include "winbase.h"


/*****************************************************************************
 * Predeclare the structures
 */
typedef LPOLESTR16 *SNB16;
typedef LPOLESTR32 *SNB32;
DECL_WINELIB_TYPE(SNB)

typedef struct STATSTG STATSTG;


/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IEnumSTATSTG,	0x0000000dL, 0, 0);
typedef struct IEnumSTATSTG IEnumSTATSTG,*LPENUMSTATSTG;

DEFINE_GUID   (IID_IFillLockBytes,	0x99caf010L, 0x415e, 0x11cf, 0x88, 0x14, 0x00, 0xaa, 0x00, 0xb5, 0x69, 0xf5);
typedef struct IFillLockBytes IFillLockBytes,*LPFILLLOCKBYTES;

DEFINE_GUID   (IID_ILayoutStorage,	0x0e6d4d90L, 0x6738, 0x11cf, 0x96, 0x08, 0x00, 0xaa, 0x00, 0x68, 0x0d, 0xb4);
typedef struct ILayoutStorage ILayoutStorage,*LPLAYOUTSTORAGE;

DEFINE_OLEGUID(IID_ILockBytes,		0x0000000aL, 0, 0);
typedef struct ILockBytes ILockBytes,*LPLOCKBYTES;

DEFINE_OLEGUID(IID_IPersist,		0x0000010cL, 0, 0);
typedef struct IPersist IPersist,*LPPERSIST;

DEFINE_OLEGUID(IID_IPersistFile,	0x0000010bL, 0, 0);
typedef struct IPersistFile IPersistFile,*LPPERSISTFILE;

DEFINE_OLEGUID(IID_IPersistStorage,	0x0000010aL, 0, 0);
typedef struct IPersistStorage IPersistStorage,*LPPERSISTSTORAGE;

DEFINE_OLEGUID(IID_IPersistStream,	0x00000109L, 0, 0);
typedef struct IPersistStream IPersistStream,*LPPERSISTSTREAM;

DEFINE_GUID   (IID_IProgressNotify,	0xa9d758a0L, 0x4617, 0x11cf, 0x95, 0xfc, 0x00, 0xaa, 0x00, 0x68, 0x0d, 0xb4);
typedef struct IProgressNotify IProgressNotify,*LPPROGRESSNOTIFY;

DEFINE_OLEGUID(IID_IRootStorage,	0x00000012L, 0, 0);
typedef struct IRootStorage IRootStorage,*LPROOTSTORAGE;

DEFINE_GUID   (IID_ISequentialStream,	0x0c733a30L, 0x2a1c, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
typedef struct ISequentialStream ISequentialStream,*LPSEQUENTIALSTREAM;

DEFINE_OLEGUID(IID_IStorage,		0x0000000bL, 0, 0);
typedef struct IStorage16 IStorage16,*LPSTORAGE16;
typedef struct IStorage32 IStorage32,*LPSTORAGE32;
DECL_WINELIB_TYPE(IStorage)
DECL_WINELIB_TYPE(LPSTORAGE)

DEFINE_OLEGUID(IID_IStream,		0x0000000cL, 0, 0);
typedef struct IStream16 IStream16,*LPSTREAM16;
typedef struct IStream32 IStream32,*LPSTREAM32;
DECL_WINELIB_TYPE(IStream)
DECL_WINELIB_TYPE(LPSTREAM)


/*****************************************************************************
 * STGM enumeration
 *
 * See IStorage and IStream
 */
#define STGM_DIRECT		0x00000000
#define STGM_TRANSACTED		0x00010000
#define STGM_SIMPLE		0x08000000
#define STGM_READ		0x00000000
#define STGM_WRITE		0x00000001
#define STGM_READWRITE		0x00000002
#define STGM_SHARE_DENY_NONE	0x00000040
#define STGM_SHARE_DENY_READ	0x00000030
#define STGM_SHARE_DENY_WRITE	0x00000020
#define STGM_SHARE_EXCLUSIVE	0x00000010
#define STGM_PRIORITY		0x00040000
#define STGM_DELETEONRELEASE	0x04000000
#define STGM_CREATE		0x00001000
#define STGM_CONVERT		0x00020000
#define STGM_FAILIFTHERE	0x00000000
#define STGM_NOSCRATCH		0x00100000
#define STGM_NOSNAPSHOT		0x00200000

/*****************************************************************************
 * STGTY enumeration
 *
 * See IStorage
 */
#define STGTY_STORAGE 1
#define STGTY_STREAM  2

/*****************************************************************************
 * STATFLAG enumeration
 *
 * See IStorage and IStream
 */
#define STATFLAG_DEFAULT 0
#define STATFLAG_NONAME  1

/*****************************************************************************
 * STREAM_SEEK enumeration
 *
 * See IStream
 */
#define STREAM_SEEK_SET 0
#define STREAM_SEEK_CUR 1
#define STREAM_SEEK_END 2

/*****************************************************************************
 * STATSTG structure
 */
struct STATSTG {
    LPOLESTR16	pwcsName;
    DWORD	type;
    ULARGE_INTEGER cbSize;
    FILETIME	mtime;
    FILETIME	ctime;
    FILETIME	atime;
    DWORD	grfMode;
    DWORD	grfLocksSupported;
    CLSID	clsid;
    DWORD	grfStateBits;
    DWORD	reserved;
};


/*****************************************************************************
 * IEnumSTATSTG interface
 */
#define ICOM_INTERFACE IEnumSTATSTG
ICOM_BEGIN(IEnumSTATSTG,IUnknown)
     ICOM_METHOD3(HRESULT, Next, ULONG, celt, STATSTG*, rgelt, ULONG*, pceltFetched);   
     ICOM_METHOD1(HRESULT, Skip, ULONG, celt);
     ICOM_CMETHOD(HRESULT, Reset);
     ICOM_METHOD1(HRESULT, Clone, IEnumSTATSTG**, ppenum);
ICOM_END(IEnumSTATSTG)
#undef ICOM_INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IEnumSTATSTG_QueryInterface(p,a,b) ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define IEnumSTATSTG_AddRef(p)             ICOM_ICALL (IUnknown,AddRef,p)
#define IEnumSTATSTG_Release(p)            ICOM_ICALL (IUnknown,Release,p)
/*** IEnumSTATSTG methods ***/
#define IEnumSTATSTG_Next(p,a,b,c)         ICOM_CALL3(Next,p,a,b,c)
#define IEnumSTATSTG_Skip(p,a)             ICOM_CALL1(Skip,p,a)
#define IEnumSTATSTG_Reset(p)              ICOM_CALL(Reset,p)
#define IEnumSTATSTG_Clone(p,a)            ICOM_CALL1(Clone,p,a)
#endif

/*****************************************************************************
 * IFillLockBytes interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * ILayoutStorage interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * ILockBytes interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IPersist interface
 */
#define ICOM_INTERFACE IPersist
ICOM_BEGIN(IPersist,IUnknown)
    ICOM_CMETHOD1(HRESULT,GetClassID, CLSID*,pClassID);
ICOM_END(IPersist)
#undef ICOM_INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IPersist_QueryInterface(p,a,b) ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define IPersist_AddRef(p)             ICOM_ICALL (IUnknown,AddRef,p)
#define IPersist_Release(p)            ICOM_ICALL (IUnknown,Release,p)
/*** IPersist methods ***/
#define IPersist_GetClassID(p,a) ICOM_CALL1(GetClassID,p,a)
#endif


/*****************************************************************************
 * IPersistFile interface
 */
#define ICOM_INTERFACE IPersistFile
ICOM_BEGIN(IPersistFile,IPersist)
    ICOM_CMETHOD (HRESULT,IsDirty);
    ICOM_METHOD2 (HRESULT,Load,          LPCOLESTR32,pszFileName, DWORD,dwMode);
    ICOM_METHOD2 (HRESULT,Save,          LPCOLESTR32,pszFileName, BOOL32,fRemember);
    ICOM_METHOD1 (HRESULT,SaveCompleted, LPCOLESTR32,pszFileName);
    ICOM_CMETHOD1(HRESULT,GetCurFile,    LPOLESTR32*,ppszFileName);
ICOM_END(IPersistFile)
#undef ICOM_INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IPersistFile_QueryInterface(p,a,b) ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define IPersistFile_AddRef(p)             ICOM_ICALL (IUnknown,AddRef,p)
#define IPersistFile_Release(p)            ICOM_ICALL (IUnknown,Release,p)
/*** IPersist methods ***/
#define IPersistFile_GetClassID(p,a) ICOM_ICALL1(IPersist,GetClassID,p,a)
/*** IPersistFile methods ***/
#define IPersistFile_IsDirty(p)         ICOM_CALL(IsDirty,p)
#define IPersistFile_Load(p,a,b)        ICOM_CALL2(Load,p,a,b)
#define IPersistFile_Save(p,a,b)        ICOM_CALL2(Save,p,a,b)
#define IPersistFile_SaveCompleted(p,a) ICOM_CALL1(SaveCompleted,p,a)
#define IPersistFile_GetCurFile(p,a)    ICOM_CALL1(GetCurFile,p,a)
#endif


/*****************************************************************************
 * IPersistStorage interface
 */
#define ICOM_INTERFACE IPersistStorage
ICOM_BEGIN(IPersistStorage, IPersist)
		ICOM_METHOD (HRESULT,IsDirty)
		ICOM_METHOD1(HRESULT,InitNew,         IStorage32*,pStg)
		ICOM_METHOD1(HRESULT,Load,            IStorage32*,pStg)
		ICOM_METHOD2(HRESULT,Save,            IStorage32*,pStgSave,  BOOL32,fSameAsLoad)
		ICOM_METHOD1(HRESULT,SaveCompleted,   IStorage32*,pStgNew);
		ICOM_METHOD (HRESULT,HandsOffStorage)
ICOM_END(IPersistStorage)
#undef ICOM_INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IPersistStorage_QueryInterface(p,a,b) ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define IPersistStorage_AddRef(p)             ICOM_ICALL (IUnknown,AddRef,p)
#define IPersistStorage_Release(p)            ICOM_ICALL (IUnknown,Release,p)
/*** IPersist methods ***/
#define IPersistStorage_GetClassID(p,a)    ICOM_ICALL1(IPersist,GetClassID,p,a)
/*** IPersistFile methods ***/
#define IPersistStorage_IsDirty(p)         ICOM_CALL (IsDirty,p)
#define IPersistStorage_InitNew(p,a)			 ICOM_CALL1(InitNew,p,a)
#define IPersistStorage_Load(p,a)          ICOM_CALL1(Load,p,a)
#define IPersistStorage_Save(p,a,b)        ICOM_CALL2(Save,p,a,b)
#define IPersistStorage_SaveCompleted(p,a) ICOM_CALL1(SaveCompleted,p,a)
#define IPersistStorage_HandsOffStorage(p) ICOM_CALL (HandsOffStorage,p)
#endif


/*****************************************************************************
 * IPersistStream interface
 */
#define ICOM_INTERFACE IPersistStream
ICOM_BEGIN(IPersistStream,IPersist);
          ICOM_METHOD(HRESULT, IsDirty);
          ICOM_METHOD1(HRESULT, Load, IStream32*, pStm);
          ICOM_METHOD2(HRESULT, Save, IStream32*, pStm, BOOL32, fClearDirty);
          ICOM_METHOD1(HRESULT, GetSizeMax, ULARGE_INTEGER*, pcbSize);
ICOM_END(IPersistStream)
#undef ICOM_INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IPersistStream_QueryInterface(p,a,b) ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define IPersistStream_AddRef(p)             ICOM_ICALL(IUnknown,AddRef,p)
#define IPersistStream_Release(p)            ICOM_ICALL(IUnknown,Release,p)
/*** IPersist methods ***/
#define IPersistStream_GetClassID(p,a)       ICOM_ICALL1(IPersist,GetClassID,p,a)
/*** IPersistStream ***/
#define IPersist_Stream_IsDirty(p)       ICOM_CALL(IsDirty,p)
#define IPersist_Stream_Load(p,a)        ICOM_CALL1(Load,p,a)
#define IPersist_Stream_Save(p,a,b)      ICOM_CALL2(Save,p,a,b)
#define IPersist_Stream_GetSizeMax(p,a)  ICOM_CALL(GetSizeMax,p)
#endif


/*****************************************************************************
 * IProgressNotify interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IRootStorage interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * ISequentialStream interface
 */
#define ICOM_INTERFACE ISequentialStream
ICOM_BEGIN(ISequentialStream,IUnknown)
    ICOM_METHOD3(HRESULT,Read,        void*,pv, ULONG,cb, ULONG*,pcbRead);
    ICOM_METHOD3(HRESULT,Write,       const void*,pv, ULONG,cb, ULONG*,pcbWritten);
ICOM_END(ISequentialStream)
#undef ICOM_INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define ISequentialStream_QueryInterface(p,a,b) ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define ISequentialStream_AddRef(p)             ICOM_ICALL (IUnknown,AddRef,p)
#define ISequentialStream_Release(p)            ICOM_ICALL (IUnknown,Release,p)
/*** ISequentialStream methods ***/
#define ISequentialStream_Read(p,a,b,c)  ICOM_CALL3(Read,p,a,b,c)
#define ISequentialStream_Write(p,a,b,c) ICOM_CALL3(Write,p,a,b,c)
#endif


/*****************************************************************************
 * IStorage interface
 */
#define ICOM_INTERFACE IStorage16
ICOM_BEGIN(IStorage16,IUnknown)
    ICOM_METHOD5(HRESULT,CreateStream,   LPCOLESTR16,pwcsName, DWORD,grfMode, DWORD,reserved1, DWORD,reserved2, IStream16**,ppstm)
    ICOM_METHOD5(HRESULT,OpenStream,     LPCOLESTR16,pwcsName, void*,reserved1, DWORD,grfMode, DWORD,reserved2, IStream16**,ppstm)
    ICOM_METHOD5(HRESULT,CreateStorage,  LPCOLESTR16,pwcsName, DWORD,grfMode, DWORD,dwStgFmt, DWORD,reserved2, IStorage16**,ppstg)
    ICOM_METHOD6(HRESULT,OpenStorage,    LPCOLESTR16,pwcsName, IStorage16*,pstgPriority, DWORD,grfMode, SNB16,snb16Exclude, DWORD,reserved, IStorage16**,ppstg)
    ICOM_METHOD4(HRESULT,CopyTo,         DWORD,ciidExclude, const IID*,rgiidExclude, SNB16,snb16Exclude, IStorage16*,pstgDest)
    ICOM_METHOD4(HRESULT,MoveElementTo,  LPCOLESTR16,pwcsName, IStorage16*,pstgDest, LPCOLESTR16,pwcsNewName, DWORD,grfFlags)
    ICOM_METHOD1(HRESULT,Commit,         DWORD,grfCommitFlags)
    ICOM_METHOD (HRESULT,Revert)
    ICOM_METHOD4(HRESULT,EnumElements,   DWORD,reserved1, void*,reserved2, DWORD,reserved3, IEnumSTATSTG**,ppenum)
    ICOM_METHOD1(HRESULT,DestroyElement, LPCOLESTR16,pwcsName)
    ICOM_METHOD2(HRESULT,RenameElement,  LPCOLESTR16,pwcsOldName, LPCOLESTR16,pwcsNewName)
    ICOM_METHOD4(HRESULT,SetElementTimes,LPCOLESTR16,pwcsName, const FILETIME*,pctime, const FILETIME*,patime, const FILETIME*,pmtime)
    ICOM_METHOD1(HRESULT,SetClass,       REFCLSID,clsid)
    ICOM_METHOD2(HRESULT,SetStateBits,   DWORD,grfStateBits, DWORD,grfMask)
    ICOM_METHOD2(HRESULT,Stat,           STATSTG*,pstatstg, DWORD,grfStatFlag)
ICOM_END(IStorage16)
#undef ICOM_INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IStorage16_QueryInterface(p,a,b) ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define IStorage16_AddRef(p)             ICOM_ICALL (IUnknown,AddRef,p)
#define IStorage16_Release(p)            ICOM_ICALL (IUnknown,Release,p)
/*** IStorage16 methods ***/
#define IStorage16_CreateStream(p,a,b,c,d,e)  ICOM_CALL5(CreateStream,p,a,b,c,d,e)
#define IStorage16_OpenStream(p,a,b,c,d,e)    ICOM_CALL5(OpenStream,p,a,b,c,d,e)
#define IStorage16_CreateStorage(p,a,b,c,d,e) ICOM_CALL5(CreateStorage,p,a,b,c,d,e)
#define IStorage16_OpenStorage(p,a,b,c,d,e,f) ICOM_CALL6(OpenStorage,p,a,b,c,d,e,f)
#define IStorage16_CopyTo(p,a,b,c,d)          ICOM_CALL4(CopyTo,p,a,b,c,d)
#define IStorage16_MoveElementTo(p,a,b,c,d)   ICOM_CALL4(MoveElementTo,p,a,b,c,d)
#define IStorage16_Commit(p,a)                ICOM_CALL1(Commit,p,a)
#define IStorage16_Revert(p)                  ICOM_CALL (Revert,p)
#define IStorage16_EnumElements(p,a,b,c,d)    ICOM_CALL4(EnumElements,p,a,b,c,d)
#define IStorage16_DestroyElement(p,a)        ICOM_CALL1(DestroyElement,p,a)
#define IStorage16_RenameElement(p,a,b)       ICOM_CALL2(RenameElement,p,a,b)
#define IStorage16_SetElementTimes(p,a,b,c,d) ICOM_CALL4(SetElementTimes,p,a,b,c,d)
#define IStorage16_SetClass(p,a)              ICOM_CALL1(SetClass,p,a)
#define IStorage16_SetStateBits(p,a,b)        ICOM_CALL2(SetStateBits,p,a,b)
#define IStorage16_Stat(p,a,b)                ICOM_CALL2(Stat,p,a,b)
#endif


#define ICOM_INTERFACE IStorage32
ICOM_BEGIN(IStorage32,IUnknown)
    ICOM_METHOD5(HRESULT,CreateStream,   LPCOLESTR32,pwcsName, DWORD,grfMode, DWORD,reserved1, DWORD,reserved2, IStream32**,ppstm);
    ICOM_METHOD5(HRESULT,OpenStream,     LPCOLESTR32,pwcsName, void*,reserved1, DWORD,grfMode, DWORD,reserved2, IStream32**,ppstm);
    ICOM_METHOD5(HRESULT,CreateStorage,  LPCOLESTR32,pwcsName, DWORD,grfMode, DWORD,dwStgFmt, DWORD,reserved2, IStorage32**,ppstg);
    ICOM_METHOD6(HRESULT,OpenStorage,    LPCOLESTR32,pwcsName, IStorage32*,pstgPriority, DWORD,grfMode, SNB32,snb16Exclude, DWORD,reserved, IStorage32**,ppstg);
    ICOM_METHOD4(HRESULT,CopyTo,         DWORD,ciidExclude, const IID*,rgiidExclude, SNB32,snb16Exclude, IStorage32*,pstgDest);
    ICOM_METHOD4(HRESULT,MoveElementTo,  LPCOLESTR32,pwcsName, IStorage32*,pstgDest, LPCOLESTR32,pwcsNewName, DWORD,grfFlags);
    ICOM_METHOD1(HRESULT,Commit,         DWORD,grfCommitFlags);
    ICOM_METHOD (HRESULT,Revert);
    ICOM_METHOD4(HRESULT,EnumElements,   DWORD,reserved1, void*,reserved2, DWORD,reserved3, IEnumSTATSTG**,ppenum);
    ICOM_METHOD1(HRESULT,DestroyElement, LPCOLESTR32,pwcsName);
    ICOM_METHOD2(HRESULT,RenameElement,  LPCOLESTR32,pwcsOldName, LPCOLESTR32,pwcsNewName);
    ICOM_METHOD4(HRESULT,SetElementTimes,LPCOLESTR32,pwcsName, const FILETIME*,pctime, const FILETIME*,patime, const FILETIME*,pmtime);
    ICOM_METHOD1(HRESULT,SetClass,       REFCLSID,clsid);
    ICOM_METHOD2(HRESULT,SetStateBits,   DWORD,grfStateBits, DWORD,grfMask);
    ICOM_METHOD2(HRESULT,Stat,           STATSTG*,pstatstg, DWORD,grfStatFlag);
ICOM_END(IStorage32)
#undef ICOM_INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IStorage32_QueryInterface(p,a,b) ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define IStorage32_AddRef(p)             ICOM_ICALL (IUnknown,AddRef,p)
#define IStorage32_Release(p)            ICOM_ICALL (IUnknown,Release,p)
/*** IStorage32 methods ***/
#define IStorage32_CreateStream(p,a,b,c,d,e)  ICOM_CALL5(CreateStream,p,a,b,c,d,e)
#define IStorage32_OpenStream(p,a,b,c,d,e)    ICOM_CALL5(OpenStream,p,a,b,c,d,e)
#define IStorage32_CreateStorage(p,a,b,c,d,e) ICOM_CALL5(CreateStorage,p,a,b,c,d,e)
#define IStorage32_OpenStorage(p,a,b,c,d,e,f) ICOM_CALL6(OpenStorage,p,a,b,c,d,e,f)
#define IStorage32_CopyTo(p,a,b,c,d)          ICOM_CALL4(CopyTo,p,a,b,c,d)
#define IStorage32_MoveElementTo(p,a,b,c,d)   ICOM_CALL4(MoveElementTo,p,a,b,c,d)
#define IStorage32_Commit(p,a)                ICOM_CALL1(Commit,p,a)
#define IStorage32_Revert(p)                  ICOM_CALL (Revert,p)
#define IStorage32_EnumElements(p,a,b,c,d)    ICOM_CALL4(EnumElements,p,a,b,c,d)
#define IStorage32_DestroyElement(p,a)        ICOM_CALL1(DestroyElement,p,a)
#define IStorage32_RenameElement(p,a,b)       ICOM_CALL2(RenameElement,p,a,b)
#define IStorage32_SetElementTimes(p,a,b,c,d) ICOM_CALL4(SetElementTimes,p,a,b,c,d)
#define IStorage32_SetClass(p,a)              ICOM_CALL1(SetClass,p,a)
#define IStorage32_SetStateBits(p,a,b)        ICOM_CALL2(SetStateBits,p,a,b)
#define IStorage32_Stat(p,a,b)                ICOM_CALL2(Stat,p,a,b)

#ifndef __WINE__
/* Duplicated for WINELIB */
/*** IUnknown methods ***/
#define IStorage_QueryInterface(p,a,b) ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define IStorage_AddRef(p)             ICOM_ICALL (IUnknown,AddRef,p)
#define IStorage_Release(p)            ICOM_ICALL (IUnknown,Release,p)
/*** IStorage methods ***/
#define IStorage_CreateStream(p,a,b,c,d,e)  ICOM_CALL5(CreateStream,p,a,b,c,d,e)
#define IStorage_OpenStream(p,a,b,c,d,e)    ICOM_CALL5(OpenStream,p,a,b,c,d,e)
#define IStorage_CreateStorage(p,a,b,c,d,e) ICOM_CALL5(CreateStorage,p,a,b,c,d,e)
#define IStorage_OpenStorage(p,a,b,c,d,e,f) ICOM_CALL6(OpenStorage,p,a,b,c,d,e,f)
#define IStorage_CopyTo(p,a,b,c,d)          ICOM_CALL4(CopyTo,p,a,b,c,d)
#define IStorage_MoveElementTo(p,a,b,c,d)   ICOM_CALL4(MoveElementTo,p,a,b,c,d)
#define IStorage_Commit(p,a)                ICOM_CALL1(Commit,p,a)
#define IStorage_Revert(p)                  ICOM_CALL (Revert,p)
#define IStorage_EnumElements(p,a,b,c,d)    ICOM_CALL4(EnumElements,p,a,b,c,d)
#define IStorage_DestroyElement(p,a)        ICOM_CALL1(DestroyElement,p,a)
#define IStorage_RenameElement(p,a,b)       ICOM_CALL2(RenameElement,p,a,b)
#define IStorage_SetElementTimes(p,a,b,c,d) ICOM_CALL4(SetElementTimes,p,a,b,c,d)
#define IStorage_SetClass(p,a)              ICOM_CALL1(SetClass,p,a)
#define IStorage_SetStateBits(p,a,b)        ICOM_CALL2(SetStateBits,p,a,b)
#define IStorage_Stat(p,a,b)                ICOM_CALL2(Stat,p,a,b)
#endif
#endif


/*****************************************************************************
 * IStream interface
 */
#define ICOM_INTERFACE IStream16
ICOM_BEGIN(IStream16,ISequentialStream)
    ICOM_METHOD3(HRESULT,Seek,        LARGE_INTEGER,dlibMove, DWORD,dwOrigin, ULARGE_INTEGER*,plibNewPosition); 
    ICOM_METHOD1(HRESULT,SetSize,     ULARGE_INTEGER,libNewSize);
    ICOM_METHOD4(HRESULT,CopyTo,      IStream16*,pstm, ULARGE_INTEGER,cb, ULARGE_INTEGER*,pcbRead, ULARGE_INTEGER*,pcbWritten);
    ICOM_METHOD1(HRESULT,Commit,      DWORD,grfCommitFlags);
    ICOM_METHOD (HRESULT,Revert);
    ICOM_METHOD3(HRESULT,LockRegion,  ULARGE_INTEGER,libOffset, ULARGE_INTEGER,cb, DWORD,dwLockType);
    ICOM_METHOD3(HRESULT,UnlockRegion,ULARGE_INTEGER,libOffset, ULARGE_INTEGER,cb, DWORD,dwLockType);
    ICOM_METHOD2(HRESULT,Stat,        STATSTG*,pstatstg, DWORD,grfStatFlag);
    ICOM_METHOD1(HRESULT,Clone,       IStream16**,ppstm);
ICOM_END(IStream16)
#undef ICOM_INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IStream16_QueryInterface(p,a,b) ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define IStream16_AddRef(p)             ICOM_ICALL (IUnknown,AddRef,p)
#define IStream16_Release(p)            ICOM_ICALL (IUnknown,Release,p)
/*** ISequentialStream methods ***/
#define IStream16_Read(p,a,b,c)  ICOM_ICALL3(ISequentialStream,Read,p,a,b,c)
#define IStream16_Write(p,a,b,c) ICOM_ICALL3(ISequentialStream,Write,p,a,b,c)
/*** IStream16 methods ***/
#define IStream16_Seek(p)               ICOM_CALL3(Seek,p)
#define IStream16_SetSize(p,a,b)        ICOM_CALL1(SetSize,p,a,b)
#define IStream16_CopyTo(pa,b,c,d)      ICOM_CALL4(CopyTo,pa,b,c,d)
#define IStream16_Commit(p,a)           ICOM_CALL1(Commit,p,a)
#define IStream16_Revert(p)             ICOM_CALL (Revert,p)
#define IStream16_LockRegion(pa,b,c)    ICOM_CALL3(LockRegion,pa,b,c)
#define IStream16_UnlockRegion(p,a,b,c) ICOM_CALL3(UnlockRegion,p,a,b,c)
#define IStream16_Stat(p,a,b)           ICOM_CALL2(Stat,p,a,b)
#define IStream16_Clone(p,a)            ICOM_CALL1(Clone,p,a)
#endif


#define ICOM_INTERFACE IStream32
ICOM_BEGIN(IStream32,ISequentialStream)
    ICOM_METHOD3(HRESULT,Seek,        LARGE_INTEGER,dlibMove, DWORD,dwOrigin, ULARGE_INTEGER*,plibNewPosition); 
    ICOM_METHOD1(HRESULT,SetSize,     ULARGE_INTEGER,libNewSize);
    ICOM_METHOD4(HRESULT,CopyTo,      IStream32*,pstm, ULARGE_INTEGER,cb, ULARGE_INTEGER*,pcbRead, ULARGE_INTEGER*,pcbWritten);
    ICOM_METHOD1(HRESULT,Commit,      DWORD,grfCommitFlags);
    ICOM_METHOD (HRESULT,Revert);
    ICOM_METHOD3(HRESULT,LockRegion,  ULARGE_INTEGER,libOffset, ULARGE_INTEGER,cb, DWORD,dwLockType);
    ICOM_METHOD3(HRESULT,UnlockRegion,ULARGE_INTEGER,libOffset, ULARGE_INTEGER,cb, DWORD,dwLockType);
    ICOM_METHOD2(HRESULT,Stat,        STATSTG*,pstatstg, DWORD,grfStatFlag);
    ICOM_METHOD1(HRESULT,Clone,       IStream32**,ppstm);
ICOM_END(IStream32)
#undef ICOM_INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IStream32_QueryInterface(p,a,b) ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define IStream32_AddRef(p)             ICOM_ICALL (IUnknown,AddRef,p)
#define IStream32_Release(p)            ICOM_ICALL (IUnknown,Release,p)
/*** ISequentialStream methods ***/
#define IStream32_Read(p,a,b,c)  ICOM_ICALL3(ISequentialStream,Read,p,a,b,c)
#define IStream32_Write(p,a,b,c) ICOM_ICALL3(ISequentialStream,Write,p,a,b,c)
/*** IStream32 methods ***/
#define IStream32_Seek(p,a,b,c)         ICOM_CALL3(Seek,p,a,b,c)
#define IStream32_SetSize(p,a)          ICOM_CALL1(SetSize,p,a)
#define IStream32_CopyTo(pa,b,c,d)      ICOM_CALL4(CopyTo,pa,b,c,d)
#define IStream32_Commit(p,a)           ICOM_CALL1(Commit,p,a)
#define IStream32_Revert(p)             ICOM_CALL (Revert,p)
#define IStream32_LockRegion(pa,b,c)    ICOM_CALL3(LockRegion,pa,b,c)
#define IStream32_UnlockRegion(p,a,b,c) ICOM_CALL3(UnlockRegion,p,a,b,c)
#define IStream32_Stat(p,a,b)           ICOM_CALL2(Stat,p,a,b)
#define IStream32_Clone(p,a)            ICOM_CALL1(Clone,p,a)

#ifndef __WINE__
/* Duplicated for WINELIB */
/*** IUnknown methods ***/
#define IStream_QueryInterface(p,a,b) ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define IStream_AddRef(p)             ICOM_ICALL (IUnknown,AddRef,p)
#define IStream_Release(p)            ICOM_ICALL (IUnknown,Release,p)
/*** ISequentialStream methods ***/
#define IStream_Read(p,a,b,c)  ICOM_ICALL3(ISequentialStream,Read,p,a,b,c)
#define IStream_Write(p,a,b,c) ICOM_ICALL3(ISequentialStream,Write,p,a,b,c)
/*** IStream methods ***/
#define IStream_Seek(p,a,b,c)         ICOM_CALL3(Seek,p,a,b,c)
#define IStream_SetSize(p,a)          ICOM_CALL1(SetSize,p,a)
#define IStream_CopyTo(pa,b,c,d)      ICOM_CALL4(CopyTo,pa,b,c,d)
#define IStream_Commit(p,a)           ICOM_CALL1(Commit,p,a)
#define IStream_Revert(p)             ICOM_CALL (Revert,p)
#define IStream_LockRegion(pa,b,c)    ICOM_CALL3(LockRegion,pa,b,c)
#define IStream_UnlockRegion(p,a,b,c) ICOM_CALL3(UnlockRegion,p,a,b,c)
#define IStream_Stat(p,a,b)           ICOM_CALL2(Stat,p,a,b)
#define IStream_Clone(p,a)            ICOM_CALL1(Clone,p,a)
#endif
#endif


/*****************************************************************************
 * StgXXX API
 */
/* FIXME: many functions are missing */
HRESULT WINAPI StgCreateDocFile16(LPCOLESTR16 pwcsName,DWORD grfMode,DWORD reserved,IStorage16 **ppstgOpen);
HRESULT WINAPI StgCreateDocfile32(LPCOLESTR32 pwcsName,DWORD grfMode,DWORD reserved,IStorage32 **ppstgOpen);
#define StgCreateDocfile WINELIB_NAME(StgCreateDocfile)

HRESULT WINAPI StgIsStorageFile16(LPCOLESTR16 fn);
HRESULT WINAPI StgIsStorageFile32(LPCOLESTR32 fn);
#define StgIsStorageFile WINELIB_NAME(StgIsStorageFile)

HRESULT WINAPI StgOpenStorage16(const OLECHAR16* pwcsName,IStorage16* pstgPriority,DWORD grfMode,SNB16 snbExclude,DWORD reserved,IStorage16**ppstgOpen);
HRESULT WINAPI StgOpenStorage32(const OLECHAR32* pwcsName,IStorage32* pstgPriority,DWORD grfMode,SNB32 snbExclude,DWORD reserved,IStorage32**ppstgOpen);
#define StgOpenStorage WINELIB_NAME(StgOpenStorage)

HRESULT WINAPI WriteClassStg32(IStorage32* pStg, REFCLSID rclsid);
#define WriteClassStg WINELIB_NAME(WriteClassStg)

#endif /* __WINE_WINE_OBJ_STORAGE_H */
