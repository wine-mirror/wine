/*
 * Compound Storage (32 bit version)
 *
 * Implemented using the documentation of the LAOLA project at
 * <URL:http://wwwwbs.cs.tu-berlin.de/~schwartz/pmh/index.html>
 * (Thanks to Martin Schwartz <schwartz@cs.tu-berlin.de>)
 *
 * This include file contains definitions of types and function
 * prototypes that are used in the many files implementing the 
 * storage functionality
 *
 * Copyright 1998,1999 Francis Beaudet
 * Copyright 1998,1999 Thuy Nguyen
 */
#ifndef __STORAGE32_H__
#define __STORAGE32_H__

/*
 * Definitions for the file format offsets.
 */
static const ULONG OFFSET_BIGBLOCKSIZEBITS   = 0x0000001e;
static const ULONG OFFSET_SMALLBLOCKSIZEBITS = 0x00000020;
static const ULONG OFFSET_BBDEPOTCOUNT	     = 0x0000002C;
static const ULONG OFFSET_ROOTSTARTBLOCK     = 0x00000030;
static const ULONG OFFSET_SBDEPOTSTART	     = 0x0000003C;
static const ULONG OFFSET_EXTBBDEPOTSTART    = 0x00000044;
static const ULONG OFFSET_EXTBBDEPOTCOUNT    = 0x00000048;
static const ULONG OFFSET_BBDEPOTSTART	     = 0x0000004C;
static const ULONG OFFSET_PS_NAME            = 0x00000000;
static const ULONG OFFSET_PS_NAMELENGTH	     = 0x00000040;
static const ULONG OFFSET_PS_PROPERTYTYPE    = 0x00000042;
static const ULONG OFFSET_PS_PREVIOUSPROP    = 0x00000044;
static const ULONG OFFSET_PS_NEXTPROP        = 0x00000048;   
static const ULONG OFFSET_PS_DIRPROP	     = 0x0000004C;
static const ULONG OFFSET_PS_GUID            = 0x00000050;
static const ULONG OFFSET_PS_TSS1	     = 0x00000064;
static const ULONG OFFSET_PS_TSD1            = 0x00000068;
static const ULONG OFFSET_PS_TSS2            = 0x0000006C;
static const ULONG OFFSET_PS_TSD2            = 0x00000070;
static const ULONG OFFSET_PS_STARTBLOCK	     = 0x00000074; 
static const ULONG OFFSET_PS_SIZE	     = 0x00000078; 
static const WORD  DEF_BIG_BLOCK_SIZE_BITS   = 0x0009;
static const WORD  DEF_SMALL_BLOCK_SIZE_BITS = 0x0006;
static const WORD  DEF_BIG_BLOCK_SIZE        = 0x0200;
static const WORD  DEF_SMALL_BLOCK_SIZE      = 0x0040;
static const ULONG BLOCK_EXTBBDEPOT          = 0xFFFFFFFC;
static const ULONG BLOCK_SPECIAL             = 0xFFFFFFFD;
static const ULONG BLOCK_END_OF_CHAIN        = 0xFFFFFFFE;
static const ULONG BLOCK_UNUSED              = 0xFFFFFFFF;
static const ULONG PROPERTY_NULL             = 0xFFFFFFFF;

#define PROPERTY_NAME_MAX_LEN    0x20
#define PROPERTY_NAME_BUFFER_LEN 0x40             

#define PROPSET_BLOCK_SIZE 0x00000080

/*
 * Property type of relation
 */
#define PROPERTY_RELATION_PREVIOUS 0
#define PROPERTY_RELATION_NEXT     1
#define PROPERTY_RELATION_DIR      2

/*
 * Property type constants
 */
#define PROPTYPE_STORAGE 0x01
#define PROPTYPE_STREAM  0x02
#define PROPTYPE_ROOT    0x05

/*
 * These defines assume a hardcoded blocksize. The code will assert
 * if the blocksize is different. Some changes will have to be done if it
 * becomes the case.
 */
#define BIG_BLOCK_SIZE           0x200
#define COUNT_BBDEPOTINHEADER    109
#define LIMIT_TO_USE_SMALL_BLOCK 0x1000
#define NUM_BLOCKS_PER_DEPOT_BLOCK 128

/*
 * These are signatures to detect the type of Document file.
 */
static const BYTE STORAGE_magic[8]    ={0xd0,0xcf,0x11,0xe0,0xa1,0xb1,0x1a,0xe1};
static const BYTE STORAGE_oldmagic[8] ={0xd0,0xcf,0x11,0xe0,0x0e,0x11,0xfc,0x0d};

/*
 * Forward declarations of all the structures used by the storage
 * module.
 */
typedef struct Storage32BaseImpl     Storage32BaseImpl;
typedef struct Storage32Impl         Storage32Impl;
typedef struct Storage32InternalImpl Storage32InternalImpl;
typedef struct BlockChainStream      BlockChainStream;
typedef struct SmallBlockChainStream SmallBlockChainStream;
typedef struct IEnumSTATSTGImpl      IEnumSTATSTGImpl;
typedef struct StgProperty           StgProperty;
typedef struct StgStreamImpl         StgStreamImpl;

/*
 * This utility structure is used to read/write the information in a storage
 * property.
 */
struct StgProperty
{
  WCHAR	         name[PROPERTY_NAME_MAX_LEN];
  WORD	         sizeOfNameString;
  BYTE	         propertyType;
  ULONG	         previousProperty;
  ULONG	         nextProperty;
  ULONG          dirProperty;
  GUID           propertyUniqueID;
  ULONG          timeStampS1;
  ULONG          timeStampD1;
  ULONG          timeStampS2;
  ULONG          timeStampD2;
  ULONG          startingBlock;
  ULARGE_INTEGER size;
};

/*************************************************************************
 * Big Block File support
 *
 * The big block file is an abstraction of a flat file separated in
 * same sized blocks. The implementation for the methods described in 
 * this section appear in stg_bigblockfile.c
 */

/*
 * Declaration of the data structures
 */
typedef struct BigBlockFile BigBlockFile,*LPBIGBLOCKFILE;
typedef struct MappedPage   MappedPage,*LPMAPPEDPAGE;
typedef struct BigBlock     BigBlock,*LPBIGBLOCK;

struct BigBlockFile
{
  ULARGE_INTEGER filesize;
  ULONG blocksize;
  HANDLE32 hfile;
  HANDLE32 hfilemap;
  DWORD flProtect;
  MappedPage *headmap_ro;
  MappedPage *headmap_w;
  BigBlock *headblock;
};

/*
 * Declaration of the functions used to manipulate the BigBlockFile
 * data structure.
 */
BigBlockFile*  BIGBLOCKFILE_Construct(HANDLE32 hFile,
                                      DWORD openFlags,
                                      ULONG blocksize);
void           BIGBLOCKFILE_Destructor(LPBIGBLOCKFILE This);
void*          BIGBLOCKFILE_GetBigBlock(LPBIGBLOCKFILE This, ULONG index);
void*          BIGBLOCKFILE_GetROBigBlock(LPBIGBLOCKFILE This, ULONG index);
void           BIGBLOCKFILE_ReleaseBigBlock(LPBIGBLOCKFILE This, void *pBlock);
void           BIGBLOCKFILE_SetSize(LPBIGBLOCKFILE This, ULARGE_INTEGER newSize);
ULARGE_INTEGER BIGBLOCKFILE_GetSize(LPBIGBLOCKFILE This);

/****************************************************************************
 * Storage32BaseImpl definitions.
 *
 * This stucture defines the base information contained in all implementations
 * of IStorage32 contained in this filee storage implementation.
 *
 * In OOP terms, this is the base class for all the IStorage32 implementations
 * contained in this file.
 */
struct Storage32BaseImpl
{
  ICOM_VTABLE(IStorage32)*  lpvtbl;   /* Needs to be the first item in the stuct
				       * since we want to cast this in a Storage32 pointer */

  /*
   * Reference count of this object
   */
  ULONG ref;
  
  /* 
   * Ancestor storage (top level) 
   */
  Storage32Impl* ancestorStorage;		 
  
  /*
   * Index of the property for the root of
   * this storage
   */
  ULONG rootPropertySetIndex;
  
  /* 
   * virtual Destructor method.
   */
  void (*v_destructor)(Storage32BaseImpl*);
};


/*
 * Prototypes for the methods of the Storage32BaseImpl class.
 */
HRESULT WINAPI Storage32BaseImpl_QueryInterface(
            IStorage32*        iface,
            REFIID             riid,
            void**             ppvObject);
        
ULONG WINAPI Storage32BaseImpl_AddRef( 
            IStorage32*        iface);
        
ULONG WINAPI Storage32BaseImpl_Release( 
            IStorage32*        iface);
        
HRESULT WINAPI Storage32BaseImpl_OpenStream( 
            IStorage32*        iface,
            const OLECHAR32*   pwcsName,  /* [string][in] */
            void*              reserved1, /* [unique][in] */
            DWORD              grfMode,   /* [in] */        
            DWORD              reserved2, /* [in] */        
            IStream32**        ppstm);    /* [out] */   
    
HRESULT WINAPI Storage32BaseImpl_OpenStorage( 
            IStorage32*        iface,
            const OLECHAR32*   pwcsName,      /* [string][unique][in] */ 
            IStorage32*        pstgPriority,  /* [unique][in] */         
            DWORD              grfMode,       /* [in] */                 
            SNB32              snbExclude,    /* [unique][in] */         
            DWORD              reserved,      /* [in] */                 
            IStorage32**       ppstg);        /* [out] */                
          
HRESULT WINAPI Storage32BaseImpl_EnumElements( 
            IStorage32*        iface,
            DWORD              reserved1, /* [in] */                  
            void*              reserved2, /* [size_is][unique][in] */ 
            DWORD              reserved3, /* [in] */                  
            IEnumSTATSTG**     ppenum);   /* [out] */   

HRESULT WINAPI Storage32BaseImpl_Stat( 
            IStorage32*        iface,
            STATSTG*           pstatstg,     /* [out] */ 
            DWORD              grfStatFlag); /* [in] */  

HRESULT WINAPI Storage32BaseImpl_RenameElement(
            IStorage32*        iface,
            const OLECHAR32*   pwcsOldName,  /* [string][in] */
            const OLECHAR32*   pwcsNewName); /* [string][in] */

HRESULT WINAPI Storage32BaseImpl_CreateStream(
            IStorage32*        iface,
            const OLECHAR32*   pwcsName,  /* [string][in] */
            DWORD              grfMode,   /* [in] */
            DWORD              reserved1, /* [in] */
            DWORD              reserved2, /* [in] */
            IStream32**        ppstm);    /* [out] */

HRESULT WINAPI Storage32BaseImpl_SetClass(
            IStorage32*        iface,
            REFCLSID           clsid);  /* [in] */

/****************************************************************************
 * Storage32Impl definitions.
 *
 * This implementation of the IStorage32 interface represents a root
 * storage. Basically, a document file.
 */
struct Storage32Impl
{
  ICOM_VTABLE(IStorage32) *lpvtbl;   /* Needs to be the first item in the stuct
				      * since we want to cast this in a Storage32 pointer */

  /*
   * Declare the member of the Storage32BaseImpl class to allow
   * casting as a Storage32BaseImpl
   */
  ULONG		        ref;
  struct Storage32Impl* ancestorStorage;		 
  ULONG                 rootPropertySetIndex;
  void (*v_destructor)(struct Storage32Impl*);
  
  /*
   * The following data members are specific to the Storage32Impl
   * class
   */
  HANDLE32           hFile;      /* Physical support for the Docfile */
  
  /*
   * File header
   */
  WORD  bigBlockSizeBits;
  WORD  smallBlockSizeBits;
  ULONG bigBlockSize;
  ULONG smallBlockSize;
  ULONG bigBlockDepotCount;
  ULONG rootStartBlock;
  ULONG smallBlockDepotStart;
  ULONG extBigBlockDepotStart;
  ULONG extBigBlockDepotCount;
  ULONG bigBlockDepotStart[COUNT_BBDEPOTINHEADER];

  ULONG blockDepotCached[NUM_BLOCKS_PER_DEPOT_BLOCK];
  ULONG indexBlockDepotCached;

  /*
   * Abstraction of the big block chains for the chains of the header.
   */
  BlockChainStream* rootBlockChain;
  BlockChainStream* smallBlockDepotChain;
  BlockChainStream* smallBlockRootChain;  

  /*
   * Pointer to the big block file abstraction
   */
  BigBlockFile* bigBlockFile; 
};

/*
 * Method declaration for the Storage32Impl class
 */        

HRESULT WINAPI Storage32Impl_CreateStorage( 
            IStorage32*      iface,
            const OLECHAR32* pwcsName,  /* [string][in] */ 
            DWORD            grfMode,   /* [in] */ 
            DWORD            dwStgFmt,  /* [in] */ 
            DWORD            reserved2, /* [in] */ 
            IStorage32**     ppstg);    /* [out] */ 
        
HRESULT WINAPI Storage32Impl_CopyTo( 
            IStorage32*      iface,
            DWORD          ciidExclude,  /* [in] */ 
            const IID*     rgiidExclude, /* [size_is][unique][in] */ 
            SNB32            snbExclude, /* [unique][in] */ 
            IStorage32*    pstgDest);    /* [unique][in] */ 
        
HRESULT WINAPI Storage32Impl_MoveElementTo( 
            IStorage32*      iface,
            const OLECHAR32* pwcsName,    /* [string][in] */ 
            IStorage32*      pstgDest,    /* [unique][in] */ 
            const OLECHAR32* pwcsNewName, /* [string][in] */ 
            DWORD            grfFlags);   /* [in] */ 
        
HRESULT WINAPI Storage32Impl_Commit( 
            IStorage32*      iface,
            DWORD          grfCommitFlags); /* [in] */ 
        
HRESULT WINAPI Storage32Impl_Revert( 
            IStorage32*      iface);
        
HRESULT WINAPI Storage32Impl_DestroyElement( 
            IStorage32*      iface,
            const OLECHAR32* pwcsName); /* [string][in] */ 
        
HRESULT WINAPI Storage32Impl_SetElementTimes( 
            IStorage32*      iface,
            const OLECHAR32* pwcsName, /* [string][in] */ 
            const FILETIME*  pctime,   /* [in] */ 
            const FILETIME*  patime,   /* [in] */ 
            const FILETIME*  pmtime);  /* [in] */ 

HRESULT WINAPI Storage32Impl_SetStateBits( 
            IStorage32*      iface,
            DWORD          grfStateBits, /* [in] */ 
            DWORD          grfMask);     /* [in] */ 
        
void Storage32Impl_Destroy(
	    Storage32Impl* This);

HRESULT Storage32Impl_Construct(
	    Storage32Impl* This,
	    HANDLE32       hFile,
	    DWORD          openFlags);

BOOL32 Storage32Impl_ReadBigBlock(
            Storage32Impl* This,
	    ULONG          blockIndex,
	    void*          buffer);

BOOL32 Storage32Impl_WriteBigBlock(
            Storage32Impl* This,
	    ULONG          blockIndex,
	    void*          buffer);

void* Storage32Impl_GetROBigBlock(
            Storage32Impl* This,
	    ULONG          blockIndex);

void* Storage32Impl_GetBigBlock(
	    Storage32Impl* This,
	    ULONG          blockIndex);

void Storage32Impl_ReleaseBigBlock(
            Storage32Impl* This,
            void*          pBigBlock);

ULONG Storage32Impl_GetNextFreeBigBlock(
            Storage32Impl* This);

void Storage32Impl_FreeBigBlock(
            Storage32Impl* This,
	    ULONG blockIndex);

ULONG Storage32Impl_GetNextBlockInChain(
            Storage32Impl* This,
	    ULONG blockIndex);

void Storage32Impl_SetNextBlockInChain(
            Storage32Impl* This,
	    ULONG blockIndex,
      ULONG nextBlock);

HRESULT Storage32Impl_LoadFileHeader(
	    Storage32Impl* This);

void Storage32Impl_SaveFileHeader(
            Storage32Impl* This);

BOOL32 Storage32Impl_ReadProperty(
            Storage32Impl* This,
	    ULONG          index,
	    StgProperty*    buffer);

BOOL32 Storage32Impl_WriteProperty(
            Storage32Impl* This,
	    ULONG          index,
	    StgProperty*   buffer);

BlockChainStream* Storage32Impl_SmallBlocksToBigBlocks(
                      Storage32Impl* This,
                      SmallBlockChainStream** ppsbChain);

ULONG Storage32Impl_GetNextExtendedBlock(Storage32Impl* This,
                                         ULONG blockIndex);

void Storage32Impl_AddBlockDepot(Storage32Impl* This,
                                 ULONG blockIndex);

ULONG Storage32Impl_AddExtBlockDepot(Storage32Impl* This);

ULONG Storage32Impl_GetExtDepotBlock(Storage32Impl* This,
                                     ULONG depotIndex);

void Storage32Impl_SetExtDepotBlock(Storage32Impl* This,
                                    ULONG depotIndex,
                                    ULONG blockIndex);
/****************************************************************************
 * Storage32InternalImpl definitions.
 *
 * Definition of the implementation structure for the IStorage32 interface.
 * This one implements the IStorage32 interface for storage that are
 * inside another storage.
 */
struct Storage32InternalImpl
{
  ICOM_VTABLE(IStorage32) *lpvtbl;	/* Needs to be the first item in the stuct
					 * since we want to cast this in a Storage32 pointer */

  /*
   * Declare the member of the Storage32BaseImpl class to allow
   * casting as a Storage32BaseImpl
   */
  ULONG		             ref;
  struct Storage32Impl* ancestorStorage;		 
  ULONG                    rootPropertySetIndex;
  void (*v_destructor)(struct Storage32InternalImpl*);

  /*
   * There is no specific data for this class.
   */
};

/*
 * Method definitions for the Storage32InternalImpl class.
 */
Storage32InternalImpl* Storage32InternalImpl_Construct(
	    Storage32Impl* ancestorStorage,	
	    ULONG          rootTropertyIndex);

void Storage32InternalImpl_Destroy(
       	    Storage32InternalImpl* This);

HRESULT WINAPI Storage32InternalImpl_Commit( 
	    IStorage32*            iface,
	    DWORD                  grfCommitFlags); /* [in] */ 

HRESULT WINAPI Storage32InternalImpl_Revert( 
     	    IStorage32*            iface);


/****************************************************************************
 * IEnumSTATSTGImpl definitions.
 *
 * Definition of the implementation structure for the IEnumSTATSTGImpl interface.
 * This class allows iterating through the content of a storage and to find
 * specific items inside it.
 */
struct IEnumSTATSTGImpl
{
  ICOM_VTABLE(IEnumSTATSTG) *lpvtbl;    /* Needs to be the first item in the stuct
					 * since we want to cast this in a IEnumSTATSTG pointer */
  
  ULONG		 ref;		        /* Reference count */
  Storage32Impl* parentStorage;         /* Reference to the parent storage */
  ULONG          firstPropertyNode;     /* Index of the root of the storage to enumerate */

  /*
   * The current implementation of the IEnumSTATSTGImpl class uses a stack
   * to walk the property sets to get the content of a storage. This stack
   * is implemented by the following 3 data members
   */
  ULONG          stackSize;
  ULONG          stackMaxSize;
  ULONG*         stackToVisit;

#define ENUMSTATSGT_SIZE_INCREMENT 10
};

/*
 * Method definitions for the IEnumSTATSTGImpl class.
 */
HRESULT WINAPI IEnumSTATSTGImpl_QueryInterface(
	    IEnumSTATSTG*     iface,
	    REFIID            riid,
	    void**            ppvObject);
        
ULONG WINAPI IEnumSTATSTGImpl_AddRef(
            IEnumSTATSTG*     iface); 
        
ULONG WINAPI IEnumSTATSTGImpl_Release(
            IEnumSTATSTG*     iface);
        
HRESULT WINAPI IEnumSTATSTGImpl_Next(
            IEnumSTATSTG*     iface,
	    ULONG             celt,
	    STATSTG*          rgelt,
	    ULONG*            pceltFetched);
        
HRESULT WINAPI IEnumSTATSTGImpl_Skip(
            IEnumSTATSTG*     iface,
	    ULONG             celt);
        
HRESULT WINAPI IEnumSTATSTGImpl_Reset(
            IEnumSTATSTG* iface);
        
HRESULT WINAPI IEnumSTATSTGImpl_Clone(
            IEnumSTATSTG*     iface,
	    IEnumSTATSTG**    ppenum);

IEnumSTATSTGImpl* IEnumSTATSTGImpl_Construct(
            Storage32Impl* This,
	    ULONG          firstPropertyNode);

void IEnumSTATSTGImpl_Destroy(
            IEnumSTATSTGImpl* This);

void IEnumSTATSTGImpl_PushSearchNode(
	    IEnumSTATSTGImpl* This,
	    ULONG             nodeToPush);

ULONG IEnumSTATSTGImpl_PopSearchNode(
            IEnumSTATSTGImpl* This,
	    BOOL32            remove);

ULONG IEnumSTATSTGImpl_FindProperty(
            IEnumSTATSTGImpl* This,
	    const OLECHAR32*  lpszPropName,
	    StgProperty*      buffer);

INT32 IEnumSTATSTGImpl_FindParentProperty(
  IEnumSTATSTGImpl *This,
  ULONG             childProperty,
  StgProperty      *currentProperty,
  ULONG            *propertyId);


/****************************************************************************
 * StgStreamImpl definitions.
 *
 * This class imlements the IStream32 inteface and represents a stream
 * located inside a storage object.
 */
struct StgStreamImpl
{
  ICOM_VTABLE(IStream32) *lpvtbl;  /* Needs to be the first item in the stuct
				    * since we want to cast this in a IStream pointer */
  
  /*
   * Reference count
   */
  ULONG		     ref;

  /*
   * Storage that is the parent(owner) of the stream
   */
  Storage32BaseImpl* parentStorage;

  /*
   * Index of the property that owns (points to) this stream.
   */
  ULONG              ownerProperty;

  /*
   * Helper variable that contains the size of the stream
   */
  ULARGE_INTEGER     streamSize;

  /*
   * This is the current position of the cursor in the stream
   */
  ULARGE_INTEGER     currentPosition;
  
  /*
   * The information in the stream is represented by a chain of small blocks
   * or a chain of large blocks. Depending on the case, one of the two
   * following variabled points to that information.
   */
  BlockChainStream*      bigBlockChain;
  SmallBlockChainStream* smallBlockChain;
  
};

/*
 * Method definition for the StgStreamImpl class.
 */
StgStreamImpl* StgStreamImpl_Construct(
		Storage32BaseImpl* parentStorage,
		ULONG              ownerProperty);

void StgStreamImpl_Destroy(
                StgStreamImpl* This);

void StgStreamImpl_OpenBlockChain(
                StgStreamImpl* This);

HRESULT WINAPI StgStreamImpl_QueryInterface(
		IStream32*      iface,
		REFIID         riid,		/* [in] */          
		void**         ppvObject);  /* [iid_is][out] */ 
        
ULONG WINAPI StgStreamImpl_AddRef(
		IStream32*      iface);
        
ULONG WINAPI StgStreamImpl_Release(
		IStream32*      iface);
        
HRESULT WINAPI StgStreamImpl_Read( 
	        IStream32*      iface,
		void*          pv,        /* [length_is][size_is][out] */
		ULONG          cb,        /* [in] */                     
		ULONG*         pcbRead);  /* [out] */                    
        
HRESULT WINAPI StgStreamImpl_Write(
		IStream32*      iface,
		const void*    pv,          /* [size_is][in] */ 
		ULONG          cb,          /* [in] */          
		ULONG*         pcbWritten); /* [out] */         
        
HRESULT WINAPI StgStreamImpl_Seek( 
		IStream32*      iface,
		LARGE_INTEGER   dlibMove,         /* [in] */ 
		DWORD           dwOrigin,         /* [in] */ 
		ULARGE_INTEGER* plibNewPosition); /* [out] */
        
HRESULT WINAPI StgStreamImpl_SetSize( 
	        IStream32*      iface,
		ULARGE_INTEGER  libNewSize);  /* [in] */ 
        
HRESULT WINAPI StgStreamImpl_CopyTo( 
		IStream32*      iface,
		IStream32*      pstm,         /* [unique][in] */ 
		ULARGE_INTEGER  cb,           /* [in] */         
		ULARGE_INTEGER* pcbRead,      /* [out] */        
		ULARGE_INTEGER* pcbWritten);  /* [out] */        

HRESULT WINAPI StgStreamImpl_Commit( 
	    	IStream32*      iface,
		DWORD           grfCommitFlags); /* [in] */ 
        
HRESULT WINAPI StgStreamImpl_Revert( 
		IStream32*  iface);
        
HRESULT WINAPI StgStreamImpl_LockRegion( 
		IStream32*     iface,
		ULARGE_INTEGER libOffset,   /* [in] */ 
		ULARGE_INTEGER cb,          /* [in] */ 
		DWORD          dwLockType); /* [in] */ 
        
HRESULT WINAPI StgStreamImpl_UnlockRegion( 
		IStream32*     iface,
		ULARGE_INTEGER libOffset,   /* [in] */ 
	        ULARGE_INTEGER cb,          /* [in] */ 
		DWORD          dwLockType); /* [in] */ 
        
HRESULT WINAPI StgStreamImpl_Stat( 
		IStream32*     iface,
	        STATSTG*       pstatstg,     /* [out] */
	        DWORD          grfStatFlag); /* [in] */ 
        
HRESULT WINAPI StgStreamImpl_Clone( 
		IStream32*     iface,
		IStream32**    ppstm);       /* [out] */ 


/********************************************************************************
 * The StorageUtl_ functions are miscelaneous utility functions. Most of which are
 * abstractions used to read values from file buffers without having to worry 
 * about bit order
 */
void StorageUtl_ReadWord(void* buffer, ULONG offset, WORD* value);
void StorageUtl_WriteWord(void* buffer, ULONG offset, WORD value);
void StorageUtl_ReadDWord(void* buffer, ULONG offset, DWORD* value);
void StorageUtl_WriteDWord(void* buffer, ULONG offset, DWORD value);
void StorageUtl_ReadGUID(void* buffer, ULONG offset, GUID* value);
void StorageUtl_WriteGUID(void* buffer, ULONG offset, GUID* value);
void StorageUtl_CopyPropertyToSTATSTG(STATSTG*     destination,
					     StgProperty* source,
					     int          statFlags);

/****************************************************************************
 * BlockChainStream definitions.
 *
 * The BlockChainStream class is a utility class that is used to create an
 * abstraction of the big block chains in the storage file.
 */
struct BlockChainStream
{
  Storage32Impl* parentStorage;
  ULONG*         headOfStreamPlaceHolder;
  ULONG          ownerPropertyIndex;
};

/*
 * Methods for the BlockChainStream class.
 */
BlockChainStream* BlockChainStream_Construct(
		Storage32Impl* parentStorage,	
		ULONG*         headOfStreamPlaceHolder,
		ULONG          propertyIndex);

void BlockChainStream_Destroy(
		BlockChainStream* This);

ULONG BlockChainStream_GetHeadOfChain(
		BlockChainStream* This);

BOOL32 BlockChainStream_ReadAt(
		BlockChainStream* This,
		ULARGE_INTEGER offset,
		ULONG          size,
		void*          buffer,
		ULONG*         bytesRead);

BOOL32 BlockChainStream_WriteAt(
		BlockChainStream* This,
		ULARGE_INTEGER offset,
		ULONG          size,
		const void*    buffer,
		ULONG*         bytesWritten);

BOOL32 BlockChainStream_SetSize(
		BlockChainStream* This,
		ULARGE_INTEGER    newSize);

ULARGE_INTEGER BlockChainStream_GetSize(
    BlockChainStream* This);

ULONG BlockChainStream_GetCount(
    BlockChainStream* This);

/****************************************************************************
 * SmallBlockChainStream definitions.
 *
 * The SmallBlockChainStream class is a utility class that is used to create an
 * abstraction of the small block chains in the storage file.
 */
struct SmallBlockChainStream
{
  Storage32Impl* parentStorage;
  ULONG          ownerPropertyIndex;
};

/*
 * Methods of the SmallBlockChainStream class.
 */
SmallBlockChainStream* SmallBlockChainStream_Construct(
	       Storage32Impl* parentStorage,	
	       ULONG          propertyIndex);

void SmallBlockChainStream_Destroy(
	       SmallBlockChainStream* This);

ULONG SmallBlockChainStream_GetHeadOfChain(
	       SmallBlockChainStream* This);

ULONG SmallBlockChainStream_GetNextBlockInChain(
	       SmallBlockChainStream* This,
	       ULONG                  blockIndex);

void SmallBlockChainStream_SetNextBlockInChain(
         SmallBlockChainStream* This,
         ULONG                  blockIndex,
         ULONG                  nextBlock);

void SmallBlockChainStream_FreeBlock(
         SmallBlockChainStream* This,
         ULONG                  blockIndex);

ULONG SmallBlockChainStream_GetNextFreeBlock(
         SmallBlockChainStream* This);

BOOL32 SmallBlockChainStream_ReadAt(
	       SmallBlockChainStream* This,
	       ULARGE_INTEGER offset,
	       ULONG          size,
	       void*          buffer,
	       ULONG*         bytesRead);

BOOL32 SmallBlockChainStream_WriteAt(
	       SmallBlockChainStream* This,
	       ULARGE_INTEGER offset,
	       ULONG          size,
	       const void*    buffer,
	       ULONG*         bytesWritten);

BOOL32 SmallBlockChainStream_SetSize(
	       SmallBlockChainStream* This,
	       ULARGE_INTEGER          newSize);

ULARGE_INTEGER SmallBlockChainStream_GetSize(
         SmallBlockChainStream* This);

ULONG SmallBlockChainStream_GetCount(
         SmallBlockChainStream* This);


#endif __STORAGE32_H__



