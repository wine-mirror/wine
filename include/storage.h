#ifndef _WINE_STORAGE_H
#define _WINE_STORAGE_H

/* Does this look like a cellar to you? */

#define STDMETHOD(xfn) HRESULT (CALLBACK *fn##xfn)
#define STDMETHOD_(ret,xfn) ret (CALLBACK *fn##xfn)
#define PURE
#define FAR
#define THIS_ THIS,

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

#define THIS LPSTORAGE16 this
typedef struct IStorage16_VTable {
	/* IUnknown methods */
	STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID FAR* ppvObj);
	STDMETHOD_(ULONG,AddRef) (THIS);
	STDMETHOD_(ULONG,Release) (THIS);
	/* IStorage methods */
	STDMETHOD(CreateStream)(THIS_ LPCOLESTR16 pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream16 **ppstm);
        STDMETHOD(OpenStream)(THIS_ LPCOLESTR16 pwcsName, void *reserved1, DWORD grfMode, DWORD reserved2, IStream16 **ppstm);
        STDMETHOD(CreateStorage)(THIS_ LPCOLESTR16 pwcsName, DWORD grfMode, DWORD dwStgFmt, DWORD reserved2, IStorage16 **ppstg);
        STDMETHOD(OpenStorage)(THIS_ LPCOLESTR16 pwcsName,IStorage16 *pstgPriority, DWORD grfMode, SNB16 SNB16Exclude, DWORD reserved, IStorage16 **ppstg);
	STDMETHOD(CopyTo)(THIS_ DWORD ciidExclude, const IID *rgiidExclude, SNB16 SNB16Exclude, IStorage16 *pstgDest);
	STDMETHOD(MoveElementTo)(THIS_ LPCOLESTR16 pwcsName, IStorage16 *pstgDest, LPCOLESTR16 pwcsNewName, DWORD grfFlags);
	STDMETHOD(Commit)(THIS_ DWORD grfCommitFlags);
	STDMETHOD(Revert)(THIS);
	STDMETHOD(EnumElements)(THIS_ DWORD reserved1,void *reserved2, DWORD reserved3, IEnumSTATSTG **ppenum);
        STDMETHOD(DestroyElement)(THIS_ LPCOLESTR16 pwcsName);
	STDMETHOD(RenameElement)(THIS_ LPCOLESTR16 pwcsOldName, LPCOLESTR16 pwcsNewName);
	STDMETHOD(SetElementTimes)(THIS_ LPCOLESTR16 pwcsName, const FILETIME *pctime,  const FILETIME *patime, const FILETIME *pmtime);
	STDMETHOD(SetClass)(THIS_ REFCLSID clsid);
	STDMETHOD(SetStateBits)(THIS_ DWORD grfStateBits, DWORD grfMask);
	STDMETHOD(Stat)(THIS_ STATSTG *pstatstg, DWORD grfStatFlag);
} IStorage16_VTable,*LPSTORAGE16_VTABLE;

struct IStorage16 {
	LPSTORAGE16_VTABLE		lpvtbl;
	DWORD				ref;
	SEGPTR				thisptr; /* pointer to this struct as segmented */
	struct storage_pps_entry	stde;
	int				ppsent;
	HFILE32				hf;
};
#undef THIS
#define THIS LPSTORAGE32 this
typedef struct IStorage32_VTable {
	/* IUnknown methods */
	STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID FAR* ppvObj);
	STDMETHOD_(ULONG,AddRef) (THIS);
	STDMETHOD_(ULONG,Release) (THIS);
	/* IStorage methods */
	STDMETHOD(CreateStream)(THIS_ LPCOLESTR32 pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream32 **ppstm);
        STDMETHOD(OpenStream)(THIS_ LPCOLESTR32 pwcsName, void *reserved1, DWORD grfMode, DWORD reserved2, IStream32 **ppstm);
        STDMETHOD(CreateStorage)(THIS_ LPCOLESTR32 pwcsName, DWORD grfMode, DWORD dwStgFmt, DWORD reserved2, IStorage32 **ppstg);
        STDMETHOD(OpenStorage)(THIS_ LPCOLESTR32 pwcsName,IStorage32 *pstgPriority, DWORD grfMode, SNB32 SNB16Exclude, DWORD reserved, IStorage32 **ppstg);
	STDMETHOD(CopyTo)(THIS_ DWORD ciidExclude, const IID *rgiidExclude, SNB32 SNB16Exclude, IStorage32 *pstgDest);
	STDMETHOD(MoveElementTo)(THIS_ LPCOLESTR32 pwcsName, IStorage32 *pstgDest, LPCOLESTR32 pwcsNewName, DWORD grfFlags);
	STDMETHOD(Commit)(THIS_ DWORD grfCommitFlags);
	STDMETHOD(Revert)(THIS);
	STDMETHOD(EnumElements)(THIS_ DWORD reserved1,void *reserved2, DWORD reserved3, IEnumSTATSTG **ppenum);
        STDMETHOD(DestroyElement)(THIS_ LPCOLESTR32 pwcsName);
	STDMETHOD(RenameElement)(THIS_ LPCOLESTR32 pwcsOldName, LPCOLESTR32 pwcsNewName);
	STDMETHOD(SetElementTimes)(THIS_ LPCOLESTR32 pwcsName, const FILETIME *pctime,  const FILETIME *patime, const FILETIME *pmtime);
	STDMETHOD(SetClass)(THIS_ REFCLSID clsid);
	STDMETHOD(SetStateBits)(THIS_ DWORD grfStateBits, DWORD grfMask);
	STDMETHOD(Stat)(THIS_ STATSTG *pstatstg, DWORD grfStatFlag);
} IStorage32_VTable,*LPSTORAGE32_VTABLE;

struct IStorage32 {
	LPSTORAGE32_VTABLE		lpvtbl;
	DWORD				ref;
	struct storage_pps_entry	stde;
	int				ppsent;
	HFILE32				hf;
};
#undef THIS

#define THIS LPSTREAM16 this
typedef struct IStream16_VTable {
        STDMETHOD(QueryInterface)(THIS_ REFIID riid, void  * *ppvObject);
        STDMETHOD_(ULONG,AddRef)(THIS);
        STDMETHOD_(ULONG,Release)(THIS);
	STDMETHOD(Read)(THIS_ void  *pv, ULONG cb, ULONG  *pcbRead);
	STDMETHOD(Write)(THIS_ const void  *pv,ULONG cb,ULONG  *pcbWritten);
	STDMETHOD(Seek)(THIS_ LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER  *plibNewPosition); 
	STDMETHOD(SetSize)(THIS_ ULARGE_INTEGER libNewSize);
	STDMETHOD(CopyTo)(THIS_ IStream16  *pstm,ULARGE_INTEGER cb,ULARGE_INTEGER  *pcbRead,ULARGE_INTEGER  *pcbWritten);
	STDMETHOD(Commit)(THIS_ DWORD grfCommitFlags);
	STDMETHOD(Revert)(THIS);
        STDMETHOD(LockRegion)(THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,DWORD dwLockType);
	STDMETHOD(UnlockRegion)(THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
	STDMETHOD(Stat)(THIS_ STATSTG  *pstatstg, DWORD grfStatFlag);
	STDMETHOD(Clone)(THIS_ IStream16 **ppstm);
} IStream16_VTable,*LPSTREAM16_VTABLE;

struct IStream16 {
	LPSTREAM16_VTABLE		lpvtbl;
	DWORD				ref;
	SEGPTR				thisptr; /* pointer to this struct as segmented */
	struct storage_pps_entry	stde;
	int				ppsent;
	HFILE32				hf;
	ULARGE_INTEGER			offset;
};
#undef THIS
#define THIS LPSTREAM32 this
typedef struct IStream32_VTable {
        STDMETHOD(QueryInterface)(THIS_ REFIID riid, void  * *ppvObject);
        STDMETHOD_(ULONG,AddRef)(THIS);
        STDMETHOD_(ULONG,Release)(THIS);
	STDMETHOD(Read)(THIS_ void  *pv, ULONG cb, ULONG  *pcbRead);
	STDMETHOD(Write)(THIS_ const void  *pv,ULONG cb,ULONG  *pcbWritten);
	STDMETHOD(Seek)(THIS_ LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER  *plibNewPosition); 
	STDMETHOD(SetSize)(THIS_ ULARGE_INTEGER libNewSize);
	STDMETHOD(CopyTo)(THIS_ IStream32  *pstm,ULARGE_INTEGER cb,ULARGE_INTEGER  *pcbRead,ULARGE_INTEGER  *pcbWritten);
	STDMETHOD(Commit)(THIS_ DWORD grfCommitFlags);
	STDMETHOD(Revert)(THIS);
        STDMETHOD(LockRegion)(THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,DWORD dwLockType);
	STDMETHOD(UnlockRegion)(THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
	STDMETHOD(Stat)(THIS_ STATSTG  *pstatstg, DWORD grfStatFlag);
	STDMETHOD(Clone)(THIS_ IStream32  **ppstm);
} IStream32_VTable,*LPSTREAM32_VTABLE;

struct IStream32 {
	LPSTREAM32_VTABLE		lpvtbl;
	DWORD				ref;
	struct storage_pps_entry	stde;
	int				ppsent;
	HFILE32				hf;
	ULARGE_INTEGER			offset;
};
#undef THIS

#undef STDMETHOD
#undef STDMETHOD_
#undef PURE
#undef FAR
#undef THIS_
#endif
