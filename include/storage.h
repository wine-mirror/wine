#ifndef __WINE_STORAGE_H
#define __WINE_STORAGE_H

#include "objbase.h"

/* Does this look like a cellar to you? */

struct storage_header {
	BYTE	magic[8];	/* 00: magic */
	BYTE	unknown1[36];	/* 08: unknown */
	DWORD	num_of_bbd_blocks;/* 2C: length of big datablocks */
	DWORD	root_startblock;/* 30: root storage first big block */
	DWORD	unknown2[2];	/* 34: unknown */
	DWORD	sbd_startblock;	/* 3C: small block depot first big block */
	DWORD	unknown3[3];	/* 40: unknown */
	DWORD	bbd_list[109];	/* 4C: big data block list (up to end of sector)*/
};
struct storage_pps_entry {
	WCHAR	pps_rawname[32];/* 00: \0 terminated widechar name */
	WORD	pps_sizeofname;	/* 40: namelength in bytes */
	BYTE	pps_type;	/* 42: flags, 1 storage/dir, 2 stream, 5 root */
	BYTE	pps_unknown0;	/* 43: unknown */
	DWORD	pps_prev;	/* 44: previous pps */
	DWORD	pps_next;	/* 48: next pps */
	DWORD	pps_dir;	/* 4C: directory pps */
	GUID	pps_guid;	/* 50: class ID */
	DWORD	pps_unknown1;	/* 60: unknown */
	FILETIME pps_ft1;	/* 64: filetime1 */
	FILETIME pps_ft2;	/* 70: filetime2 */
	DWORD	pps_sb;		/* 74: data startblock */
	DWORD	pps_size;	/* 78: datalength. (<0x1000)?small:big blocks*/
	DWORD	pps_unknown2;	/* 7C: unknown */
};

#define STORAGE_CHAINENTRY_FAT		0xfffffffd
#define STORAGE_CHAINENTRY_ENDOFCHAIN	0xfffffffe
#define STORAGE_CHAINENTRY_FREE		0xffffffff

typedef LPOLESTR16 *SNB16;
typedef LPOLESTR32 *SNB32;
DECL_WINELIB_TYPE(SNB)

typedef struct IStorage16 IStorage16,*LPSTORAGE16;
typedef struct IStorage32 IStorage32,*LPSTORAGE32;
typedef struct IStream16 IStream16,*LPSTREAM16;
typedef struct IStream32 IStream32,*LPSTREAM32;

typedef struct IEnumSTATSTG IEnumSTATSTG,*LPENUMSTATSTG;

typedef struct {
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
} STATSTG;

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
#define STGM_NOSCRATCH		0x00100000
#define STGM_CREATE		0x00001000
#define STGM_CONVERT		0x00020000
#define STGM_FAILIFTHERE	0x00000000


/*****************************************************************************
 * IStorage16 interface
 */
#define ICOM_INTERFACE IStorage16
ICOM_BEGIN(IStorage16,IUnknown)
    ICOM_METHOD5(HRESULT,CreateStream,   LPCOLESTR16,pwcsName, DWORD,grfMode, DWORD,reserved1, DWORD,reserved2, IStream16**,ppstm);
    ICOM_METHOD5(HRESULT,OpenStream,     LPCOLESTR16,pwcsName, void*,reserved1, DWORD,grfMode, DWORD,reserved2, IStream16**,ppstm);
    ICOM_METHOD5(HRESULT,CreateStorage,  LPCOLESTR16,pwcsName, DWORD,grfMode, DWORD,dwStgFmt, DWORD,reserved2, IStorage16**,ppstg);
    ICOM_METHOD6(HRESULT,OpenStorage,    LPCOLESTR16,pwcsName, IStorage16*,pstgPriority, DWORD,grfMode, SNB16,snb16Exclude, DWORD,reserved, IStorage16**,ppstg);
    ICOM_METHOD4(HRESULT,CopyTo,         DWORD,ciidExclude, const IID*,rgiidExclude, SNB16,snb16Exclude, IStorage16*,pstgDest);
    ICOM_METHOD4(HRESULT,MoveElementTo,  LPCOLESTR16,pwcsName, IStorage16*,pstgDest, LPCOLESTR16,pwcsNewName, DWORD,grfFlags);
    ICOM_METHOD1(HRESULT,Commit,         DWORD,grfCommitFlags);
    ICOM_METHOD (HRESULT,Revert);
    ICOM_METHOD4(HRESULT,EnumElements,   DWORD,reserved1, void*,reserved2, DWORD,reserved3, IEnumSTATSTG**,ppenum);
    ICOM_METHOD1(HRESULT,DestroyElement, LPCOLESTR16,pwcsName);
    ICOM_METHOD2(HRESULT,RenameElement,  LPCOLESTR16,pwcsOldName, LPCOLESTR16,pwcsNewName);
    ICOM_METHOD4(HRESULT,SetElementTimes,LPCOLESTR16,pwcsName, const FILETIME*,pctime, const FILETIME*,patime, const FILETIME*,pmtime);
    ICOM_METHOD1(HRESULT,SetClass,       REFCLSID,clsid);
    ICOM_METHOD2(HRESULT,SetStateBits,   DWORD,grfStateBits, DWORD,grfMask);
    ICOM_METHOD2(HRESULT,Stat,           STATSTG*,pstatstg, DWORD,grfStatFlag);
ICOM_END(IStorage16)
#undef ICOM_INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IStorage16_QueryInterface(p,a,b) ICOM_ICALL2(QueryInterface,p,a,b)
#define IStorage16_AddRef(p)             ICOM_ICALL (AddRef,p)
#define IStorage16_Release(p)            ICOM_ICALL (Release,p)
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


/*****************************************************************************
 * IStorage32 interface
 */
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
#define IStorage32_QueryInterface(p,a,b) ICOM_ICALL2(QueryInterface,p,a,b)
#define IStorage32_AddRef(p)             ICOM_ICALL (AddRef,p)
#define IStorage32_Release(p)            ICOM_ICALL (Release,p)
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
#endif


/*****************************************************************************
 * IStream16 interface
 */
#define ICOM_INTERFACE IStream16
ICOM_BEGIN(IStream16,IUnknown)
    ICOM_METHOD3(HRESULT,Read,        void*,pv, ULONG,cb, ULONG*,pcbRead);
    ICOM_METHOD3(HRESULT,Write,       const void*,pv, ULONG,cb, ULONG*,pcbWritten);
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
#define IStream16_QueryInterface(p,a,b) ICOM_ICALL2(QueryInterface,p,a,b)
#define IStream16_AddRef(p)             ICOM_ICALL (AddRef,p)
#define IStream16_Release(p)            ICOM_ICALL (Release,p)
/*** IStream16 methods ***/
#define IStream16_Read(p,a,b,c)         ICOM_CALL3(Read,p,a,b,c)
#define IStream16_Write(p,a,b,c)        ICOM_CALL3(Write,p,a,b,c)
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


/*****************************************************************************
 * IStream32 interface
 */
#define ICOM_INTERFACE IStream32
ICOM_BEGIN(IStream32,IUnknown)
    ICOM_METHOD3(HRESULT,Read,        void*,pv, ULONG,cb, ULONG*,pcbRead);
    ICOM_METHOD3(HRESULT,Write,       const void*,pv, ULONG,cb, ULONG*,pcbWritten);
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
#define IStream32_QueryInterface(p,a,b) ICOM_ICALL2(QueryInterface,p,a,b)
#define IStream32_AddRef(p)             ICOM_ICALL (AddRef,p)
#define IStream32_Release(p)            ICOM_ICALL (Release,p)
/*** IStream32 methods ***/
#define IStream32_Read(p,a,b,c)         ICOM_CALL3(Read,p,a,b,c)
#define IStream32_Write(p,a,b,c)        ICOM_CALL3(Write,p,a,b,c)
#define IStream32_Seek(p)               ICOM_CALL3(Seek,p)
#define IStream32_SetSize(p,a,b)        ICOM_CALL1(SetSize,p,a,b)
#define IStream32_CopyTo(pa,b,c,d)      ICOM_CALL4(CopyTo,pa,b,c,d)
#define IStream32_Commit(p,a)           ICOM_CALL1(Commit,p,a)
#define IStream32_Revert(p)             ICOM_CALL (Revert,p)
#define IStream32_LockRegion(pa,b,c)    ICOM_CALL3(LockRegion,pa,b,c)
#define IStream32_UnlockRegion(p,a,b,c) ICOM_CALL3(UnlockRegion,p,a,b,c)
#define IStream32_Stat(p,a,b)           ICOM_CALL2(Stat,p,a,b)
#define IStream32_Clone(p,a)            ICOM_CALL1(Clone,p,a)
#endif

#endif
