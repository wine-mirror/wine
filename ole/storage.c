/* Compound Storage
 *
 * Implemented using the documentation of the LAOLA project at
 * <URL:http://wwwwbs.cs.tu-berlin.de/~schwartz/pmh/index.html>
 * (Thanks to Martin Schwartz <schwartz@cs.tu-berlin.de>)
 *
 * Copyright 1998 Marcus Meissner
 */

#include <assert.h>
#include <time.h>
#include <string.h>
#include "windef.h"
#include "winerror.h"
#include "wine/winestring.h"
#include "wine/winbase16.h"
#include "file.h"
#include "ole.h"
#include "wine/obj_base.h"
#include "wine/obj_storage.h"
#include "heap.h"
#include "module.h"
#include "ldt.h"
#include "debug.h"

DECLARE_DEBUG_CHANNEL(ole)
DECLARE_DEBUG_CHANNEL(relay)

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


static const BYTE STORAGE_magic[8]   ={0xd0,0xcf,0x11,0xe0,0xa1,0xb1,0x1a,0xe1};
static const BYTE STORAGE_notmagic[8]={0x0e,0x11,0xfc,0x0d,0xd0,0xcf,0x11,0xe0};
static const BYTE STORAGE_oldmagic[8]={0xd0,0xcf,0x11,0xe0,0x0e,0x11,0xfc,0x0d};

#define BIGSIZE		512
#define SMALLSIZE		64

#define SMALLBLOCKS_PER_BIGBLOCK	(BIGSIZE/SMALLSIZE)

#define READ_HEADER	assert(STORAGE_get_big_block(hf,-1,(LPBYTE)&sth));assert(!memcmp(STORAGE_magic,sth.magic,sizeof(STORAGE_magic)));
static ICOM_VTABLE(IStorage16) stvt16;
static ICOM_VTABLE(IStorage16) *segstvt16 = NULL;
static ICOM_VTABLE(IStream16) strvt16;
static ICOM_VTABLE(IStream16) *segstrvt16 = NULL;

/*ULONG WINAPI IStorage16_AddRef(LPSTORAGE16 this);*/
static void _create_istorage16(LPSTORAGE16 *stg);
static void _create_istream16(LPSTREAM16 *str);

#define IMPLEMENTED 1


/******************************************************************************
 *		STORAGE_get_big_block	[Internal]
 *
 * Reading OLE compound storage
 */
static BOOL
STORAGE_get_big_block(HFILE hf,int n,BYTE *block) {
	assert(n>=-1);
	if (-1==_llseek(hf,(n+1)*BIGSIZE,SEEK_SET)) {
		WARN(ole," seek failed (%ld)\n",GetLastError());
		return FALSE;
	}
	assert((n+1)*BIGSIZE==_llseek(hf,0,SEEK_CUR));
	if (BIGSIZE!=_lread(hf,block,BIGSIZE)) {
		WARN(ole,"(block size %d): read didn't read (%ld)\n",n,GetLastError());
		assert(0);
		return FALSE;
	}
	return TRUE;
}

/******************************************************************************
 * STORAGE_put_big_block [INTERNAL]
 */
static BOOL
STORAGE_put_big_block(HFILE hf,int n,BYTE *block) {
	assert(n>=-1);
	if (-1==_llseek(hf,(n+1)*BIGSIZE,SEEK_SET)) {
		WARN(ole," seek failed (%ld)\n",GetLastError());
		return FALSE;
	}
	assert((n+1)*BIGSIZE==_llseek(hf,0,SEEK_CUR));
	if (BIGSIZE!=_lwrite(hf,block,BIGSIZE)) {
		WARN(ole," write failed (%ld)\n",GetLastError());
		return FALSE;
	}
	return TRUE;
}

/******************************************************************************
 * STORAGE_get_next_big_blocknr [INTERNAL]
 */
static int
STORAGE_get_next_big_blocknr(HFILE hf,int blocknr) {
	INT	bbs[BIGSIZE/sizeof(INT)];
	struct	storage_header	sth;

	READ_HEADER;
	
	assert(blocknr>>7<sth.num_of_bbd_blocks);
	if (sth.bbd_list[blocknr>>7]==0xffffffff)
		return -5;
	if (!STORAGE_get_big_block(hf,sth.bbd_list[blocknr>>7],(LPBYTE)bbs))
		return -5;
	assert(bbs[blocknr&0x7f]!=STORAGE_CHAINENTRY_FREE);
	return bbs[blocknr&0x7f];
}

/******************************************************************************
 * STORAGE_get_nth_next_big_blocknr [INTERNAL]
 */
static int
STORAGE_get_nth_next_big_blocknr(HFILE hf,int blocknr,int nr) {
	INT	bbs[BIGSIZE/sizeof(INT)];
	int	lastblock = -1;
	struct storage_header sth;

	READ_HEADER;
	
	assert(blocknr>=0);
	while (nr--) {
		assert((blocknr>>7)<sth.num_of_bbd_blocks);
		assert(sth.bbd_list[blocknr>>7]!=0xffffffff);

		/* simple caching... */
		if (lastblock!=sth.bbd_list[blocknr>>7]) {
			assert(STORAGE_get_big_block(hf,sth.bbd_list[blocknr>>7],(LPBYTE)bbs));
			lastblock = sth.bbd_list[blocknr>>7];
		}
		blocknr = bbs[blocknr&0x7f];
	}
	return blocknr;
}

/******************************************************************************
 *		STORAGE_get_root_pps_entry	[Internal]
 */
static BOOL
STORAGE_get_root_pps_entry(HFILE hf,struct storage_pps_entry *pstde) {
	int	blocknr,i;
	BYTE	block[BIGSIZE];
	struct storage_pps_entry	*stde=(struct storage_pps_entry*)block;
	struct storage_header sth;

	READ_HEADER;
	blocknr = sth.root_startblock;
	while (blocknr>=0) {
		assert(STORAGE_get_big_block(hf,blocknr,block));
		for (i=0;i<4;i++) {
			if (!stde[i].pps_sizeofname)
				continue;
			if (stde[i].pps_type==5) {
				*pstde=stde[i];
				return TRUE;
			}
		}
		blocknr=STORAGE_get_next_big_blocknr(hf,blocknr);
	}
	return FALSE;
}

/******************************************************************************
 * STORAGE_get_small_block [INTERNAL]
 */
static BOOL
STORAGE_get_small_block(HFILE hf,int blocknr,BYTE *sblock) {
	BYTE				block[BIGSIZE];
	int				bigblocknr;
	struct storage_pps_entry	root;

	assert(blocknr>=0);
	assert(STORAGE_get_root_pps_entry(hf,&root));
	bigblocknr = STORAGE_get_nth_next_big_blocknr(hf,root.pps_sb,blocknr/SMALLBLOCKS_PER_BIGBLOCK);
	assert(bigblocknr>=0);
	assert(STORAGE_get_big_block(hf,bigblocknr,block));

	memcpy(sblock,((LPBYTE)block)+SMALLSIZE*(blocknr&(SMALLBLOCKS_PER_BIGBLOCK-1)),SMALLSIZE);
	return TRUE;
}

/******************************************************************************
 * STORAGE_put_small_block [INTERNAL]
 */
static BOOL
STORAGE_put_small_block(HFILE hf,int blocknr,BYTE *sblock) {
	BYTE				block[BIGSIZE];
	int				bigblocknr;
	struct storage_pps_entry	root;

	assert(blocknr>=0);

	assert(STORAGE_get_root_pps_entry(hf,&root));
	bigblocknr = STORAGE_get_nth_next_big_blocknr(hf,root.pps_sb,blocknr/SMALLBLOCKS_PER_BIGBLOCK);
	assert(bigblocknr>=0);
	assert(STORAGE_get_big_block(hf,bigblocknr,block));

	memcpy(((LPBYTE)block)+SMALLSIZE*(blocknr&(SMALLBLOCKS_PER_BIGBLOCK-1)),sblock,SMALLSIZE);
	assert(STORAGE_put_big_block(hf,bigblocknr,block));
	return TRUE;
}

/******************************************************************************
 * STORAGE_get_next_small_blocknr [INTERNAL]
 */
static int
STORAGE_get_next_small_blocknr(HFILE hf,int blocknr) {
	BYTE				block[BIGSIZE];
	LPINT				sbd = (LPINT)block;
	int				bigblocknr;
	struct storage_header		sth;

	READ_HEADER;
	assert(blocknr>=0);
	bigblocknr = STORAGE_get_nth_next_big_blocknr(hf,sth.sbd_startblock,blocknr/128);
	assert(bigblocknr>=0);
	assert(STORAGE_get_big_block(hf,bigblocknr,block));
	assert(sbd[blocknr & 127]!=STORAGE_CHAINENTRY_FREE);
	return sbd[blocknr & (128-1)];
}

/******************************************************************************
 * STORAGE_get_nth_next_small_blocknr [INTERNAL]
 */
static int
STORAGE_get_nth_next_small_blocknr(HFILE hf,int blocknr,int nr) {
	int	lastblocknr;
	BYTE	block[BIGSIZE];
	LPINT	sbd = (LPINT)block;
	struct storage_header sth;

	READ_HEADER;
	lastblocknr=-1;
	assert(blocknr>=0);
	while ((nr--) && (blocknr>=0)) {
		if (lastblocknr/128!=blocknr/128) {
			int	bigblocknr;
			bigblocknr = STORAGE_get_nth_next_big_blocknr(hf,sth.sbd_startblock,blocknr/128);
			assert(bigblocknr>=0);
			assert(STORAGE_get_big_block(hf,bigblocknr,block));
			lastblocknr = blocknr;
		}
		assert(lastblocknr>=0);
		lastblocknr=blocknr;
		blocknr=sbd[blocknr & (128-1)];
		assert(blocknr!=STORAGE_CHAINENTRY_FREE);
	}
	return blocknr;
}

/******************************************************************************
 * STORAGE_get_pps_entry [INTERNAL]
 */
static int
STORAGE_get_pps_entry(HFILE hf,int n,struct storage_pps_entry *pstde) {
	int	blocknr;
	BYTE	block[BIGSIZE];
	struct storage_pps_entry *stde = (struct storage_pps_entry*)(((LPBYTE)block)+128*(n&3));
	struct storage_header sth;

	READ_HEADER;
	/* we have 4 pps entries per big block */
	blocknr = STORAGE_get_nth_next_big_blocknr(hf,sth.root_startblock,n/4);
	assert(blocknr>=0);
	assert(STORAGE_get_big_block(hf,blocknr,block));

	*pstde=*stde;
	return 1;
}

/******************************************************************************
 *		STORAGE_put_pps_entry	[Internal]
 */
static int
STORAGE_put_pps_entry(HFILE hf,int n,struct storage_pps_entry *pstde) {
	int	blocknr;
	BYTE	block[BIGSIZE];
	struct storage_pps_entry *stde = (struct storage_pps_entry*)(((LPBYTE)block)+128*(n&3));
	struct storage_header sth;

	READ_HEADER;

	/* we have 4 pps entries per big block */
	blocknr = STORAGE_get_nth_next_big_blocknr(hf,sth.root_startblock,n/4);
	assert(blocknr>=0);
	assert(STORAGE_get_big_block(hf,blocknr,block));
	*stde=*pstde;
	assert(STORAGE_put_big_block(hf,blocknr,block));
	return 1;
}

/******************************************************************************
 *		STORAGE_look_for_named_pps	[Internal]
 */
static int
STORAGE_look_for_named_pps(HFILE hf,int n,LPOLESTR name) {
	struct storage_pps_entry	stde;
	int				ret;

	if (n==-1)
		return -1;
	if (1!=STORAGE_get_pps_entry(hf,n,&stde))
		return -1;

	if (!lstrcmpW(name,stde.pps_rawname))
		return n;
	if (stde.pps_prev != -1) {
		ret=STORAGE_look_for_named_pps(hf,stde.pps_prev,name);
		if (ret!=-1)
			return ret;
	}
	if (stde.pps_next != -1) {
		ret=STORAGE_look_for_named_pps(hf,stde.pps_next,name);
		if (ret!=-1)
			return ret;
	}
	return -1;
}

/******************************************************************************
 *		STORAGE_dump_pps_entry	[Internal]
 *
 * FIXME
 *    Function is unused
 */
void
STORAGE_dump_pps_entry(struct storage_pps_entry *stde) {
	char	name[33],xguid[50];

	WINE_StringFromCLSID(&(stde->pps_guid),xguid);

	lstrcpyWtoA(name,stde->pps_rawname);
	if (!stde->pps_sizeofname)
		return;
	DUMP("name: %s\n",name);
	DUMP("type: %d\n",stde->pps_type);
	DUMP("prev pps: %ld\n",stde->pps_prev);
	DUMP("next pps: %ld\n",stde->pps_next);
	DUMP("dir pps: %ld\n",stde->pps_dir);
	DUMP("guid: %s\n",xguid);
	if (stde->pps_type !=2) {
		time_t	t;

		t = DOSFS_FileTimeToUnixTime(&(stde->pps_ft1),NULL);
		DUMP("ts1: %s\n",ctime(&t));
		t = DOSFS_FileTimeToUnixTime(&(stde->pps_ft2),NULL);
		DUMP("ts2: %s\n",ctime(&t));
	}
	DUMP("startblock: %ld\n",stde->pps_sb);
	DUMP("size: %ld\n",stde->pps_size);
}

/******************************************************************************
 * STORAGE_init_storage [INTERNAL]
 */
static BOOL 
STORAGE_init_storage(HFILE hf) {
	BYTE	block[BIGSIZE];
	LPDWORD	bbs;
	struct storage_header *sth;
	struct storage_pps_entry *stde;

	assert(-1!=_llseek(hf,0,SEEK_SET));
	/* block -1 is the storage header */
	sth = (struct storage_header*)block;
	memcpy(sth->magic,STORAGE_magic,8);
	memset(sth->unknown1,0,sizeof(sth->unknown1));
	memset(sth->unknown2,0,sizeof(sth->unknown2));
	memset(sth->unknown3,0,sizeof(sth->unknown3));
	sth->num_of_bbd_blocks	= 1;
	sth->root_startblock	= 1;
	sth->sbd_startblock	= 0xffffffff;
	memset(sth->bbd_list,0xff,sizeof(sth->bbd_list));
	sth->bbd_list[0]	= 0;
	assert(BIGSIZE==_lwrite(hf,block,BIGSIZE));
	/* block 0 is the big block directory */
	bbs=(LPDWORD)block;
	memset(block,0xff,sizeof(block)); /* mark all blocks as free */
	bbs[0]=STORAGE_CHAINENTRY_ENDOFCHAIN; /* for this block */
	bbs[1]=STORAGE_CHAINENTRY_ENDOFCHAIN; /* for directory entry */
	assert(BIGSIZE==_lwrite(hf,block,BIGSIZE));
	/* block 1 is the root directory entry */
	memset(block,0x00,sizeof(block));
	stde = (struct storage_pps_entry*)block;
	lstrcpyAtoW(stde->pps_rawname,"RootEntry");
	stde->pps_sizeofname	= lstrlenW(stde->pps_rawname)*2+2;
	stde->pps_type		= 5;
	stde->pps_dir		= -1;
	stde->pps_next		= -1;
	stde->pps_prev		= -1;
	stde->pps_sb		= 0xffffffff;
	stde->pps_size		= 0;
	assert(BIGSIZE==_lwrite(hf,block,BIGSIZE));
	return TRUE;
}

/******************************************************************************
 *		STORAGE_set_big_chain	[Internal]
 */
static BOOL
STORAGE_set_big_chain(HFILE hf,int blocknr,INT type) {
	BYTE	block[BIGSIZE];
	LPINT	bbd = (LPINT)block;
	int	nextblocknr,bigblocknr;
	struct storage_header sth;

	READ_HEADER;
	assert(blocknr!=type);
	while (blocknr>=0) {
		bigblocknr = sth.bbd_list[blocknr/128];
		assert(bigblocknr>=0);
		assert(STORAGE_get_big_block(hf,bigblocknr,block));

		nextblocknr = bbd[blocknr&(128-1)];
		bbd[blocknr&(128-1)] = type;
		if (type>=0)
			return TRUE;
		assert(STORAGE_put_big_block(hf,bigblocknr,block));
		type = STORAGE_CHAINENTRY_FREE;
		blocknr = nextblocknr;
	}
	return TRUE;
}

/******************************************************************************
 * STORAGE_set_small_chain [Internal]
 */
static BOOL
STORAGE_set_small_chain(HFILE hf,int blocknr,INT type) {
	BYTE	block[BIGSIZE];
	LPINT	sbd = (LPINT)block;
	int	lastblocknr,nextsmallblocknr,bigblocknr;
	struct storage_header sth;

	READ_HEADER;

	assert(blocknr!=type);
	lastblocknr=-129;bigblocknr=-2;
	while (blocknr>=0) {
		/* cache block ... */
		if (lastblocknr/128!=blocknr/128) {
			bigblocknr = STORAGE_get_nth_next_big_blocknr(hf,sth.sbd_startblock,blocknr/128);
			assert(bigblocknr>=0);
			assert(STORAGE_get_big_block(hf,bigblocknr,block));
		}
		lastblocknr = blocknr;
		nextsmallblocknr = sbd[blocknr&(128-1)];
		sbd[blocknr&(128-1)] = type;
		assert(STORAGE_put_big_block(hf,bigblocknr,block));
		if (type>=0)
			return TRUE;
		type = STORAGE_CHAINENTRY_FREE;
		blocknr = nextsmallblocknr;
	}
	return TRUE;
}

/******************************************************************************
 *		STORAGE_get_free_big_blocknr	[Internal]
 */
static int 
STORAGE_get_free_big_blocknr(HFILE hf) {
	BYTE	block[BIGSIZE];
	LPINT	sbd = (LPINT)block;
	int	lastbigblocknr,i,curblock,bigblocknr;
	struct storage_header sth;

	READ_HEADER;
	curblock	= 0;
	lastbigblocknr	= -1;
	bigblocknr	= sth.bbd_list[curblock];
	while (curblock<sth.num_of_bbd_blocks) {
		assert(bigblocknr>=0);
		assert(STORAGE_get_big_block(hf,bigblocknr,block));
		for (i=0;i<128;i++)
			if (sbd[i]==STORAGE_CHAINENTRY_FREE) {
				sbd[i] = STORAGE_CHAINENTRY_ENDOFCHAIN;
				assert(STORAGE_put_big_block(hf,bigblocknr,block));
				memset(block,0x42,sizeof(block));
				assert(STORAGE_put_big_block(hf,i+curblock*128,block));
				return i+curblock*128;
			}
		lastbigblocknr = bigblocknr;
		bigblocknr = sth.bbd_list[++curblock];
	}
	bigblocknr = curblock*128;
	/* since we have marked all blocks from 0 up to curblock*128-1 
	 * the next free one is curblock*128, where we happily put our 
	 * next large block depot.
	 */
	memset(block,0xff,sizeof(block));
	/* mark the block allocated and returned by this function */
	sbd[1] = STORAGE_CHAINENTRY_ENDOFCHAIN;
	assert(STORAGE_put_big_block(hf,bigblocknr,block));

	/* if we had a bbd block already (mostlikely) we need
	 * to link the new one into the chain 
	 */
	if (lastbigblocknr!=-1)
		assert(STORAGE_set_big_chain(hf,lastbigblocknr,bigblocknr));
	sth.bbd_list[curblock]=bigblocknr;
	sth.num_of_bbd_blocks++;
	assert(sth.num_of_bbd_blocks==curblock+1);
	assert(STORAGE_put_big_block(hf,-1,(LPBYTE)&sth));

	/* Set the end of the chain for the bigblockdepots */
	assert(STORAGE_set_big_chain(hf,bigblocknr,STORAGE_CHAINENTRY_ENDOFCHAIN));
	/* add 1, for the first entry is used for the additional big block 
	 * depot. (means we already used bigblocknr) */
	memset(block,0x42,sizeof(block));
	/* allocate this block (filled with 0x42) */
	assert(STORAGE_put_big_block(hf,bigblocknr+1,block));
	return bigblocknr+1;
}


/******************************************************************************
 *		STORAGE_get_free_small_blocknr	[Internal]
 */
static int 
STORAGE_get_free_small_blocknr(HFILE hf) {
	BYTE	block[BIGSIZE];
	LPINT	sbd = (LPINT)block;
	int	lastbigblocknr,newblocknr,i,curblock,bigblocknr;
	struct storage_pps_entry	root;
	struct storage_header sth;

	READ_HEADER;
	bigblocknr	= sth.sbd_startblock;
	curblock	= 0;
	lastbigblocknr	= -1;
	newblocknr	= -1;
	while (bigblocknr>=0) {
		if (!STORAGE_get_big_block(hf,bigblocknr,block))
			return -1;
		for (i=0;i<128;i++)
			if (sbd[i]==STORAGE_CHAINENTRY_FREE) {
				sbd[i]=STORAGE_CHAINENTRY_ENDOFCHAIN;
				newblocknr = i+curblock*128;
				break;
			}
		if (i!=128)
			break;
		lastbigblocknr = bigblocknr;
		bigblocknr = STORAGE_get_next_big_blocknr(hf,bigblocknr);
		curblock++;
	}
	if (newblocknr==-1) {
		bigblocknr = STORAGE_get_free_big_blocknr(hf);
		if (bigblocknr<0)
			return -1;
		READ_HEADER;
		memset(block,0xff,sizeof(block));
		sbd[0]=STORAGE_CHAINENTRY_ENDOFCHAIN;
		if (!STORAGE_put_big_block(hf,bigblocknr,block))
			return -1;
		if (lastbigblocknr==-1) {
			sth.sbd_startblock = bigblocknr;
			if (!STORAGE_put_big_block(hf,-1,(LPBYTE)&sth)) /* need to write it */
				return -1;
		} else {
			if (!STORAGE_set_big_chain(hf,lastbigblocknr,bigblocknr))
				return -1;
		}
		if (!STORAGE_set_big_chain(hf,bigblocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
			return -1;
		newblocknr = curblock*128;
	}
	/* allocate enough big blocks for storing the allocated small block */
	if (!STORAGE_get_root_pps_entry(hf,&root))
		return -1;
	if (root.pps_sb==-1)
		lastbigblocknr	= -1;
	else
		lastbigblocknr	= STORAGE_get_nth_next_big_blocknr(hf,root.pps_sb,(root.pps_size-1)/BIGSIZE);
	while (root.pps_size < (newblocknr*SMALLSIZE+SMALLSIZE-1)) {
		/* we need to allocate more stuff */
		bigblocknr = STORAGE_get_free_big_blocknr(hf);
		if (bigblocknr<0)
			return -1;
		READ_HEADER;
		if (root.pps_sb==-1) {
			root.pps_sb	 = bigblocknr;
			root.pps_size	+= BIGSIZE;
		} else {
			if (!STORAGE_set_big_chain(hf,lastbigblocknr,bigblocknr))
				return -1;
			root.pps_size	+= BIGSIZE;
		}
		lastbigblocknr = bigblocknr;
	}
	if (!STORAGE_set_big_chain(hf,lastbigblocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
		return -1;
	if (!STORAGE_put_pps_entry(hf,0,&root))
		return -1;
	return newblocknr;
}

/******************************************************************************
 *		STORAGE_get_free_pps_entry	[Internal]
 */
static int
STORAGE_get_free_pps_entry(HFILE hf) {
	int	blocknr,i,curblock,lastblocknr;
	BYTE	block[BIGSIZE];
	struct storage_pps_entry *stde = (struct storage_pps_entry*)block;
	struct storage_header sth;

	READ_HEADER;
	blocknr = sth.root_startblock;
	assert(blocknr>=0);
	curblock=0;
	while (blocknr>=0) {
		if (!STORAGE_get_big_block(hf,blocknr,block))
			return -1;
		for (i=0;i<4;i++) 
			if (stde[i].pps_sizeofname==0) /* free */
				return curblock*4+i;
		lastblocknr = blocknr;
		blocknr = STORAGE_get_next_big_blocknr(hf,blocknr);
		curblock++;
	}
	assert(blocknr==STORAGE_CHAINENTRY_ENDOFCHAIN);
	blocknr = STORAGE_get_free_big_blocknr(hf);
	/* sth invalidated */
	if (blocknr<0)
		return -1;
	
	if (!STORAGE_set_big_chain(hf,lastblocknr,blocknr))
		return -1;
	if (!STORAGE_set_big_chain(hf,blocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
		return -1;
	memset(block,0,sizeof(block));
	STORAGE_put_big_block(hf,blocknr,block);
	return curblock*4;
}

/* --- IStream16 implementation */

typedef struct
{
        /* IUnknown fields */
        ICOM_VTABLE(IStream16)*         lpvtbl;
        DWORD                           ref;
        /* IStream16 fields */
        SEGPTR                          thisptr; /* pointer to this struct as segmented */
        struct storage_pps_entry        stde;
        int                             ppsent;
        HFILE                         hf;
        ULARGE_INTEGER                  offset;
} IStream16Impl;

/******************************************************************************
 *		IStream16_QueryInterface	[STORAGE.518]
 */
HRESULT WINAPI IStream16_fnQueryInterface(
	IStream16* iface,REFIID refiid,LPVOID *obj
) {
	ICOM_THIS(IStream16Impl,iface);
	char    xrefiid[50];
	WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
	TRACE(relay,"(%p)->(%s,%p)\n",This,xrefiid,obj);
	if (!memcmp(&IID_IUnknown,refiid,sizeof(IID_IUnknown))) {
		*obj = This;
		return 0;
	}
	return OLE_E_ENUM_NOMORE;
	
}

/******************************************************************************
 * IStream16_AddRef [STORAGE.519]
 */
ULONG WINAPI IStream16_fnAddRef(IStream16* iface) {
	ICOM_THIS(IStream16Impl,iface);
	return ++(This->ref);
}

/******************************************************************************
 * IStream16_Release [STORAGE.520]
 */
ULONG WINAPI IStream16_fnRelease(IStream16* iface) {
	ICOM_THIS(IStream16Impl,iface);
	FlushFileBuffers(This->hf);
	This->ref--;
	if (!This->ref) {
		CloseHandle(This->hf);
		SEGPTR_FREE(This);
		return 0;
	}
	return This->ref;
}

/******************************************************************************
 *		IStream16_Seek	[STORAGE.523]
 *
 * FIXME
 *    Does not handle 64 bits
 */
HRESULT WINAPI IStream16_fnSeek(
	IStream16* iface,LARGE_INTEGER offset,DWORD whence,ULARGE_INTEGER *newpos
) {
	ICOM_THIS(IStream16Impl,iface);
	TRACE(relay,"(%p)->([%ld.%ld],%ld,%p)\n",This,offset.HighPart,offset.LowPart,whence,newpos);

	switch (whence) {
	/* unix SEEK_xx should be the same as win95 ones */
	case SEEK_SET:
		/* offset must be ==0 (<0 is invalid, and >0 cannot be handled
		 * right now.
		 */
		assert(offset.HighPart==0);
		This->offset.HighPart = offset.HighPart;
		This->offset.LowPart = offset.LowPart;
		break;
	case SEEK_CUR:
		if (offset.HighPart < 0) {
			/* FIXME: is this negation correct ? */
			offset.HighPart = -offset.HighPart;
			offset.LowPart = (0xffffffff ^ offset.LowPart)+1;

			assert(offset.HighPart==0);
			assert(This->offset.LowPart >= offset.LowPart);
			This->offset.LowPart -= offset.LowPart;
		} else {
			assert(offset.HighPart==0);
			This->offset.LowPart+= offset.LowPart;
		}
		break;
	case SEEK_END:
		assert(offset.HighPart==0);
		This->offset.LowPart = This->stde.pps_size-offset.LowPart;
		break;
	}
	if (This->offset.LowPart>This->stde.pps_size)
		This->offset.LowPart=This->stde.pps_size;
	if (newpos) *newpos = This->offset;
	return S_OK;
}

/******************************************************************************
 *		IStream16_Read	[STORAGE.521]
 */
HRESULT WINAPI IStream16_fnRead(
        IStream16* iface,void  *pv,ULONG cb,ULONG  *pcbRead
) {
	ICOM_THIS(IStream16Impl,iface);
	BYTE	block[BIGSIZE];
	ULONG	*bytesread=pcbRead,xxread;
	int	blocknr;

	TRACE(relay,"(%p)->(%p,%ld,%p)\n",This,pv,cb,pcbRead);
	if (!pcbRead) bytesread=&xxread;
	*bytesread = 0;

	if (cb>This->stde.pps_size-This->offset.LowPart)
		cb=This->stde.pps_size-This->offset.LowPart;
	if (This->stde.pps_size < 0x1000) {
		/* use small block reader */
		blocknr = STORAGE_get_nth_next_small_blocknr(This->hf,This->stde.pps_sb,This->offset.LowPart/SMALLSIZE);
		while (cb) {
			int	cc;

			if (!STORAGE_get_small_block(This->hf,blocknr,block)) {
			   WARN(ole,"small block read failed!!!\n");
				return E_FAIL;
			}
			cc = cb; 
			if (cc>SMALLSIZE-(This->offset.LowPart&(SMALLSIZE-1)))
				cc=SMALLSIZE-(This->offset.LowPart&(SMALLSIZE-1));
			memcpy((LPBYTE)pv,block+(This->offset.LowPart&(SMALLSIZE-1)),cc);
			This->offset.LowPart+=cc;
			(LPBYTE)pv+=cc;
			*bytesread+=cc;
			cb-=cc;
			blocknr = STORAGE_get_next_small_blocknr(This->hf,blocknr);
		}
	} else {
		/* use big block reader */
		blocknr = STORAGE_get_nth_next_big_blocknr(This->hf,This->stde.pps_sb,This->offset.LowPart/BIGSIZE);
		while (cb) {
			int	cc;

			if (!STORAGE_get_big_block(This->hf,blocknr,block)) {
				WARN(ole,"big block read failed!!!\n");
				return E_FAIL;
			}
			cc = cb; 
			if (cc>BIGSIZE-(This->offset.LowPart&(BIGSIZE-1)))
				cc=BIGSIZE-(This->offset.LowPart&(BIGSIZE-1));
			memcpy((LPBYTE)pv,block+(This->offset.LowPart&(BIGSIZE-1)),cc);
			This->offset.LowPart+=cc;
			(LPBYTE)pv+=cc;
			*bytesread+=cc;
			cb-=cc;
			blocknr=STORAGE_get_next_big_blocknr(This->hf,blocknr);
		}
	}
	return S_OK;
}

/******************************************************************************
 *		IStream16_Write	[STORAGE.522]
 */
HRESULT WINAPI IStream16_fnWrite(
        IStream16* iface,const void *pv,ULONG cb,ULONG *pcbWrite
) {
	ICOM_THIS(IStream16Impl,iface);
	BYTE	block[BIGSIZE];
	ULONG	*byteswritten=pcbWrite,xxwritten;
	int	oldsize,newsize,i,curoffset=0,lastblocknr,blocknr,cc;
	HFILE	hf = This->hf;

	if (!pcbWrite) byteswritten=&xxwritten;
	*byteswritten = 0;

	TRACE(relay,"(%p)->(%p,%ld,%p)\n",This,pv,cb,pcbWrite);
	/* do we need to junk some blocks? */
	newsize	= This->offset.LowPart+cb;
	oldsize	= This->stde.pps_size;
	if (newsize < oldsize) {
		if (oldsize < 0x1000) {
			/* only small blocks */
			blocknr=STORAGE_get_nth_next_small_blocknr(hf,This->stde.pps_sb,newsize/SMALLSIZE);

			assert(blocknr>=0);

			/* will set the rest of the chain to 'free' */
			if (!STORAGE_set_small_chain(hf,blocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
				return E_FAIL;
		} else {
			if (newsize >= 0x1000) {
				blocknr=STORAGE_get_nth_next_big_blocknr(hf,This->stde.pps_sb,newsize/BIGSIZE);
				assert(blocknr>=0);

				/* will set the rest of the chain to 'free' */
				if (!STORAGE_set_big_chain(hf,blocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
					return E_FAIL;
			} else {
				/* Migrate large blocks to small blocks 
				 * (we just migrate newsize bytes)
				 */
				LPBYTE	curdata,data = HeapAlloc(GetProcessHeap(),0,newsize+BIGSIZE);
				cc	= newsize;
				blocknr = This->stde.pps_sb;
				curdata = data;
				while (cc>0) {
					if (!STORAGE_get_big_block(hf,blocknr,curdata)) {
						HeapFree(GetProcessHeap(),0,data);
						return E_FAIL;
					}
					curdata	+= BIGSIZE;
					cc	-= BIGSIZE;
					blocknr	 = STORAGE_get_next_big_blocknr(hf,blocknr);
				}
				/* frees complete chain for this stream */
				if (!STORAGE_set_big_chain(hf,This->stde.pps_sb,STORAGE_CHAINENTRY_FREE))
					return E_FAIL;
				curdata	= data;
				blocknr = This->stde.pps_sb = STORAGE_get_free_small_blocknr(hf);
				if (blocknr<0)
					return E_FAIL;
				cc	= newsize;
				while (cc>0) {
					if (!STORAGE_put_small_block(hf,blocknr,curdata))
						return E_FAIL;
					cc	-= SMALLSIZE;
					if (cc<=0) {
						if (!STORAGE_set_small_chain(hf,blocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
							return E_FAIL;
						break;
					} else {
						int newblocknr = STORAGE_get_free_small_blocknr(hf);
						if (newblocknr<0)
							return E_FAIL;
						if (!STORAGE_set_small_chain(hf,blocknr,newblocknr))
							return E_FAIL;
						blocknr = newblocknr;
					}
					curdata	+= SMALLSIZE;
				}
				HeapFree(GetProcessHeap(),0,data);
			}
		}
		This->stde.pps_size = newsize;
	}

	if (newsize > oldsize) {
		if (oldsize >= 0x1000) {
			/* should return the block right before the 'endofchain' */
			blocknr = STORAGE_get_nth_next_big_blocknr(hf,This->stde.pps_sb,This->stde.pps_size/BIGSIZE);
			assert(blocknr>=0);
			lastblocknr	= blocknr;
			for (i=oldsize/BIGSIZE;i<newsize/BIGSIZE;i++) {
				blocknr = STORAGE_get_free_big_blocknr(hf);
				if (blocknr<0)
					return E_FAIL;
				if (!STORAGE_set_big_chain(hf,lastblocknr,blocknr))
					return E_FAIL;
				lastblocknr = blocknr;
			}
			if (!STORAGE_set_big_chain(hf,blocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
				return E_FAIL;
		} else {
			if (newsize < 0x1000) {
				/* find startblock */
				if (!oldsize)
					This->stde.pps_sb = blocknr = STORAGE_get_free_small_blocknr(hf);
				else
					blocknr = STORAGE_get_nth_next_small_blocknr(hf,This->stde.pps_sb,This->stde.pps_size/SMALLSIZE);
				if (blocknr<0)
					return E_FAIL;

				/* allocate required new small blocks */
				lastblocknr = blocknr;
				for (i=oldsize/SMALLSIZE;i<newsize/SMALLSIZE;i++) {
					blocknr = STORAGE_get_free_small_blocknr(hf);
					if (blocknr<0)
						return E_FAIL;
					if (!STORAGE_set_small_chain(hf,lastblocknr,blocknr))
						return E_FAIL;
					lastblocknr = blocknr;
				}
				/* and terminate the chain */
				if (!STORAGE_set_small_chain(hf,lastblocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
					return E_FAIL;
			} else {
				if (!oldsize) {
					/* no single block allocated yet */
					blocknr=STORAGE_get_free_big_blocknr(hf);
					if (blocknr<0)
						return E_FAIL;
					This->stde.pps_sb = blocknr;
				} else {
					/* Migrate small blocks to big blocks */
					LPBYTE	curdata,data = HeapAlloc(GetProcessHeap(),0,oldsize+BIGSIZE);
					cc	= oldsize;
					blocknr = This->stde.pps_sb;
					curdata = data;
					/* slurp in */
					while (cc>0) {
						if (!STORAGE_get_small_block(hf,blocknr,curdata)) {
							HeapFree(GetProcessHeap(),0,data);
							return E_FAIL;
						}
						curdata	+= SMALLSIZE;
						cc	-= SMALLSIZE;
						blocknr	 = STORAGE_get_next_small_blocknr(hf,blocknr);
					}
					/* free small block chain */
					if (!STORAGE_set_small_chain(hf,This->stde.pps_sb,STORAGE_CHAINENTRY_FREE))
						return E_FAIL;
					curdata	= data;
					blocknr = This->stde.pps_sb = STORAGE_get_free_big_blocknr(hf);
					if (blocknr<0)
						return E_FAIL;
					/* put the data into the big blocks */
					cc	= This->stde.pps_size;
					while (cc>0) {
						if (!STORAGE_put_big_block(hf,blocknr,curdata))
							return E_FAIL;
						cc	-= BIGSIZE;
						if (cc<=0) {
							if (!STORAGE_set_big_chain(hf,blocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
								return E_FAIL;
							break;
						} else {
							int newblocknr = STORAGE_get_free_big_blocknr(hf);
							if (newblocknr<0)
								return E_FAIL;
							if (!STORAGE_set_big_chain(hf,blocknr,newblocknr))
								return E_FAIL;
							blocknr = newblocknr;
						}
						curdata	+= BIGSIZE;
					}
					HeapFree(GetProcessHeap(),0,data);
				}
				/* generate big blocks to fit the new data */
				lastblocknr	= blocknr;
				for (i=oldsize/BIGSIZE;i<newsize/BIGSIZE;i++) {
					blocknr = STORAGE_get_free_big_blocknr(hf);
					if (blocknr<0)
						return E_FAIL;
					if (!STORAGE_set_big_chain(hf,lastblocknr,blocknr))
						return E_FAIL;
					lastblocknr = blocknr;
				}
				/* terminate chain */
				if (!STORAGE_set_big_chain(hf,lastblocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
					return E_FAIL;
			}
		}
		This->stde.pps_size = newsize;
	}

	/* There are just some cases where we didn't modify it, we write it out
	 * everytime
	 */
	if (!STORAGE_put_pps_entry(hf,This->ppsent,&(This->stde)))
		return E_FAIL;

	/* finally the write pass */
	if (This->stde.pps_size < 0x1000) {
		blocknr = STORAGE_get_nth_next_small_blocknr(hf,This->stde.pps_sb,This->offset.LowPart/SMALLSIZE);
		assert(blocknr>=0);
		while (cb>0) {
			/* we ensured that it is allocated above */
			assert(blocknr>=0);
			/* Read old block everytime, since we can have
			 * overlapping data at START and END of the write
			 */
			if (!STORAGE_get_small_block(hf,blocknr,block))
				return E_FAIL;

			cc = SMALLSIZE-(This->offset.LowPart&(SMALLSIZE-1));
			if (cc>cb)
				cc=cb;
			memcpy(	((LPBYTE)block)+(This->offset.LowPart&(SMALLSIZE-1)),
				(LPBYTE)(pv+curoffset),
				cc
			);
			if (!STORAGE_put_small_block(hf,blocknr,block))
				return E_FAIL;
			cb			-= cc;
			curoffset		+= cc;
			(LPBYTE)pv		+= cc;
			This->offset.LowPart	+= cc;
			*byteswritten		+= cc;
			blocknr = STORAGE_get_next_small_blocknr(hf,blocknr);
		}
	} else {
		blocknr = STORAGE_get_nth_next_big_blocknr(hf,This->stde.pps_sb,This->offset.LowPart/BIGSIZE);
		assert(blocknr>=0);
		while (cb>0) {
			/* we ensured that it is allocated above, so it better is */
			assert(blocknr>=0);
			/* read old block everytime, since we can have
			 * overlapping data at START and END of the write
			 */
			if (!STORAGE_get_big_block(hf,blocknr,block))
				return E_FAIL;

			cc = BIGSIZE-(This->offset.LowPart&(BIGSIZE-1));
			if (cc>cb)
				cc=cb;
			memcpy(	((LPBYTE)block)+(This->offset.LowPart&(BIGSIZE-1)),
				(LPBYTE)(pv+curoffset),
				cc
			);
			if (!STORAGE_put_big_block(hf,blocknr,block))
				return E_FAIL;
			cb			-= cc;
			curoffset		+= cc;
			(LPBYTE)pv		+= cc;
			This->offset.LowPart	+= cc;
			*byteswritten		+= cc;
			blocknr = STORAGE_get_next_big_blocknr(hf,blocknr);
		}
	}
	return S_OK;
}

/******************************************************************************
 *		_create_istream16	[Internal]
 */
static void _create_istream16(LPSTREAM16 *str) {
	IStream16Impl*	lpst;

	if (!strvt16.fnQueryInterface) {
		HMODULE16	wp = GetModuleHandle16("STORAGE");
		if (wp>=32) {
		  /* FIXME: what is This WIN32_GetProcAddress16. Should the name be IStream16_QueryInterface of IStream16_fnQueryInterface */
#define VTENT(xfn)  strvt16.fn##xfn = (void*)WIN32_GetProcAddress16(wp,"IStream16_"#xfn);assert(strvt16.fn##xfn)
			VTENT(QueryInterface);
			VTENT(AddRef);
			VTENT(Release);
			VTENT(Read);
			VTENT(Write);
			VTENT(Seek);
			VTENT(SetSize);
			VTENT(CopyTo);
			VTENT(Commit);
			VTENT(Revert);
			VTENT(LockRegion);
			VTENT(UnlockRegion);
			VTENT(Stat);
			VTENT(Clone);
#undef VTENT
			segstrvt16 = SEGPTR_NEW(ICOM_VTABLE(IStream16));
			memcpy(segstrvt16,&strvt16,sizeof(strvt16));
			segstrvt16 = (ICOM_VTABLE(IStream16)*)SEGPTR_GET(segstrvt16);
		} else {
#define VTENT(xfn) strvt16.fn##xfn = IStream16_fn##xfn;
			VTENT(QueryInterface);
			VTENT(AddRef);
			VTENT(Release);
			VTENT(Read);
			VTENT(Write);
			VTENT(Seek);
	/*
			VTENT(CopyTo);
			VTENT(Commit);
			VTENT(SetSize);
			VTENT(Revert);
			VTENT(LockRegion);
			VTENT(UnlockRegion);
			VTENT(Stat);
			VTENT(Clone);
	*/
#undef VTENT
			segstrvt16 = &strvt16;
		}
	}
	lpst = SEGPTR_NEW(IStream16Impl);
	lpst->lpvtbl	= segstrvt16;
	lpst->ref	= 1;
	lpst->thisptr	= SEGPTR_GET(lpst);
	*str = (void*)lpst->thisptr;
}


/* --- IStream32 implementation */

typedef struct
{
        /* IUnknown fields */
        ICOM_VTABLE(IStream)*         lpvtbl;
        DWORD                           ref;
        /* IStream32 fields */
        struct storage_pps_entry        stde;
        int                             ppsent;
        HFILE                         hf;
        ULARGE_INTEGER                  offset;
} IStream32Impl;

/*****************************************************************************
 *		IStream32_QueryInterface	[VTABLE]
 */
HRESULT WINAPI IStream_fnQueryInterface(
	IStream* iface,REFIID refiid,LPVOID *obj
) {
	ICOM_THIS(IStream32Impl,iface);
	char    xrefiid[50];

	WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
	TRACE(relay,"(%p)->(%s,%p)\n",This,xrefiid,obj);
	if (!memcmp(&IID_IUnknown,refiid,sizeof(IID_IUnknown))) {
		*obj = This;
		return 0;
	}
	return OLE_E_ENUM_NOMORE;
	
}

/******************************************************************************
 * IStream32_AddRef [VTABLE]
 */
ULONG WINAPI IStream_fnAddRef(IStream* iface) {
	ICOM_THIS(IStream32Impl,iface);
	return ++(This->ref);
}

/******************************************************************************
 * IStream32_Release [VTABLE]
 */
ULONG WINAPI IStream_fnRelease(IStream* iface) {
	ICOM_THIS(IStream32Impl,iface);
	FlushFileBuffers(This->hf);
	This->ref--;
	if (!This->ref) {
		CloseHandle(This->hf);
		SEGPTR_FREE(This);
		return 0;
	}
	return This->ref;
}

/* --- IStorage16 implementation */

typedef struct
{
        /* IUnknown fields */
        ICOM_VTABLE(IStorage16)*        lpvtbl;
        DWORD                           ref;
        /* IStorage16 fields */
        SEGPTR                          thisptr; /* pointer to this struct as segmented */
        struct storage_pps_entry        stde;
        int                             ppsent;
        HFILE                         hf;
} IStorage16Impl;

/******************************************************************************
 *		IStorage16_QueryInterface	[STORAGE.500]
 */
HRESULT WINAPI IStorage16_fnQueryInterface(
	IStorage16* iface,REFIID refiid,LPVOID *obj
) {
	ICOM_THIS(IStorage16Impl,iface);
	char    xrefiid[50];

	WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
	TRACE(relay,"(%p)->(%s,%p)\n",This,xrefiid,obj);

	if (!memcmp(&IID_IUnknown,refiid,sizeof(IID_IUnknown))) {
		*obj = This;
		return 0;
	}
	return OLE_E_ENUM_NOMORE;
}

/******************************************************************************
 * IStorage16_AddRef [STORAGE.501]
 */
ULONG WINAPI IStorage16_fnAddRef(IStorage16* iface) {
	ICOM_THIS(IStorage16Impl,iface);
	return ++(This->ref);
}

/******************************************************************************
 * IStorage16_Release [STORAGE.502]
 */
ULONG WINAPI IStorage16_fnRelease(IStorage16* iface) {
	ICOM_THIS(IStorage16Impl,iface);
	This->ref--;
	if (This->ref)
		return This->ref;
	SEGPTR_FREE(This);
	return 0;
}

/******************************************************************************
 * IStorage16_Stat [STORAGE.517]
 */
HRESULT WINAPI IStorage16_fnStat(
        LPSTORAGE16 iface,STATSTG16 *pstatstg, DWORD grfStatFlag
) {
	ICOM_THIS(IStorage16Impl,iface);
	TRACE(ole,"(%p)->(%p,0x%08lx)\n",
		This,pstatstg,grfStatFlag
	);
	pstatstg->pwcsName=(LPOLESTR16)SEGPTR_GET(SEGPTR_STRDUP_WtoA(This->stde.pps_rawname));
	pstatstg->type = This->stde.pps_type;
	pstatstg->cbSize.LowPart = This->stde.pps_size;
	pstatstg->mtime = This->stde.pps_ft1; /* FIXME */ /* why? */
	pstatstg->atime = This->stde.pps_ft2; /* FIXME */
	pstatstg->ctime = This->stde.pps_ft2; /* FIXME */
	pstatstg->grfMode	= 0; /* FIXME */
	pstatstg->grfLocksSupported = 0; /* FIXME */
	pstatstg->clsid		= This->stde.pps_guid;
	pstatstg->grfStateBits	= 0; /* FIXME */
	pstatstg->reserved	= 0;
	return S_OK;
}

/******************************************************************************
 *		IStorage16_Commit	[STORAGE.509]
 */
HRESULT WINAPI IStorage16_fnCommit(
        LPSTORAGE16 iface,DWORD commitflags
) {
	ICOM_THIS(IStorage16Impl,iface);
	FIXME(ole,"(%p)->(0x%08lx),STUB!\n",
		This,commitflags
	);
	return S_OK;
}

/******************************************************************************
 * IStorage16_CopyTo [STORAGE.507]
 */
HRESULT WINAPI IStorage16_fnCopyTo(LPSTORAGE16 iface,DWORD ciidExclude,const IID *rgiidExclude,SNB16 SNB16Exclude,IStorage16 *pstgDest) {
	ICOM_THIS(IStorage16Impl,iface);
	char	xguid[50];

	if (rgiidExclude)
		WINE_StringFromCLSID(rgiidExclude,xguid);
	else
		strcpy(xguid,"<no guid>");
	FIXME(ole,"IStorage16(%p)->(0x%08lx,%s,%p,%p),stub!\n",
		This,ciidExclude,xguid,SNB16Exclude,pstgDest
	);
	return S_OK;
}


/******************************************************************************
 * IStorage16_CreateStorage [STORAGE.505]
 */
HRESULT WINAPI IStorage16_fnCreateStorage(
	LPSTORAGE16 iface,LPCOLESTR16 pwcsName,DWORD grfMode,DWORD dwStgFormat,DWORD reserved2, IStorage16 **ppstg
) {
	ICOM_THIS(IStorage16Impl,iface);
	IStorage16Impl*	lpstg;
	int		ppsent,x;
	struct storage_pps_entry	stde;
	struct storage_header sth;
	HFILE		hf=This->hf;

	READ_HEADER;

	TRACE(ole,"(%p)->(%s,0x%08lx,0x%08lx,0x%08lx,%p)\n",
		This,pwcsName,grfMode,dwStgFormat,reserved2,ppstg
	);
	if (grfMode & STGM_TRANSACTED)
		FIXME(ole,"We do not support transacted Compound Storage. Using direct mode.\n");
	_create_istorage16(ppstg);
	lpstg = (IStorage16Impl*)PTR_SEG_TO_LIN(*ppstg);
	lpstg->hf		= This->hf;

	ppsent=STORAGE_get_free_pps_entry(lpstg->hf);
	if (ppsent<0)
		return E_FAIL;
	stde=This->stde;
	if (stde.pps_dir==-1) {
		stde.pps_dir = ppsent;
		x = This->ppsent;
	} else {
		FIXME(ole," use prev chain too ?\n");
		x=stde.pps_dir;
		if (1!=STORAGE_get_pps_entry(lpstg->hf,x,&stde))
			return E_FAIL;
		while (stde.pps_next!=-1) {
			x=stde.pps_next;
			if (1!=STORAGE_get_pps_entry(lpstg->hf,x,&stde))
				return E_FAIL;
		}
		stde.pps_next = ppsent;
	}
	assert(STORAGE_put_pps_entry(lpstg->hf,x,&stde));
	assert(1==STORAGE_get_pps_entry(lpstg->hf,ppsent,&(lpstg->stde)));
	lstrcpyAtoW(lpstg->stde.pps_rawname,pwcsName);
	lpstg->stde.pps_sizeofname = lstrlenA(pwcsName)*2+2;
	lpstg->stde.pps_next	= -1;
	lpstg->stde.pps_prev	= -1;
	lpstg->stde.pps_dir	= -1;
	lpstg->stde.pps_sb	= -1;
	lpstg->stde.pps_size	=  0;
	lpstg->stde.pps_type	=  1;
	lpstg->ppsent		= ppsent;
	/* FIXME: timestamps? */
	if (!STORAGE_put_pps_entry(lpstg->hf,ppsent,&(lpstg->stde)))
		return E_FAIL;
	return S_OK;
}

/******************************************************************************
 *		IStorage16_CreateStream	[STORAGE.503]
 */
HRESULT WINAPI IStorage16_fnCreateStream(
	LPSTORAGE16 iface,LPCOLESTR16 pwcsName,DWORD grfMode,DWORD reserved1,DWORD reserved2, IStream16 **ppstm
) {
	ICOM_THIS(IStorage16Impl,iface);
	IStream16Impl*	lpstr;
	int		ppsent,x;
	struct storage_pps_entry	stde;

	TRACE(ole,"(%p)->(%s,0x%08lx,0x%08lx,0x%08lx,%p)\n",
		This,pwcsName,grfMode,reserved1,reserved2,ppstm
	);
	if (grfMode & STGM_TRANSACTED)
		FIXME(ole,"We do not support transacted Compound Storage. Using direct mode.\n");
	_create_istream16(ppstm);
	lpstr = (IStream16Impl*)PTR_SEG_TO_LIN(*ppstm);
        DuplicateHandle( GetCurrentProcess(), This->hf, GetCurrentProcess(),
                         &lpstr->hf, 0, TRUE, DUPLICATE_SAME_ACCESS );
	lpstr->offset.LowPart	= 0;
	lpstr->offset.HighPart	= 0;

	ppsent=STORAGE_get_free_pps_entry(lpstr->hf);
	if (ppsent<0)
		return E_FAIL;
	stde=This->stde;
	if (stde.pps_next==-1)
		x=This->ppsent;
	else
		while (stde.pps_next!=-1) {
			x=stde.pps_next;
			if (1!=STORAGE_get_pps_entry(lpstr->hf,x,&stde))
				return E_FAIL;
		}
	stde.pps_next = ppsent;
	assert(STORAGE_put_pps_entry(lpstr->hf,x,&stde));
	assert(1==STORAGE_get_pps_entry(lpstr->hf,ppsent,&(lpstr->stde)));
	lstrcpyAtoW(lpstr->stde.pps_rawname,pwcsName);
	lpstr->stde.pps_sizeofname = lstrlenA(pwcsName)*2+2;
	lpstr->stde.pps_next	= -1;
	lpstr->stde.pps_prev	= -1;
	lpstr->stde.pps_dir	= -1;
	lpstr->stde.pps_sb	= -1;
	lpstr->stde.pps_size	=  0;
	lpstr->stde.pps_type	=  2;
	lpstr->ppsent		= ppsent;
	/* FIXME: timestamps? */
	if (!STORAGE_put_pps_entry(lpstr->hf,ppsent,&(lpstr->stde)))
		return E_FAIL;
	return S_OK;
}

/******************************************************************************
 *		IStorage16_OpenStorage	[STORAGE.506]
 */
HRESULT WINAPI IStorage16_fnOpenStorage(
	LPSTORAGE16 iface,LPCOLESTR16 pwcsName, IStorage16 *pstgPrio, DWORD grfMode, SNB16 snbExclude, DWORD reserved, IStorage16 **ppstg
) {
	ICOM_THIS(IStorage16Impl,iface);
	IStream16Impl*	lpstg;
	WCHAR		name[33];
	int		newpps;

	TRACE(relay,"(%p)->(%s,%p,0x%08lx,%p,0x%08lx,%p)\n",
		This,pwcsName,pstgPrio,grfMode,snbExclude,reserved,ppstg
	);
	if (grfMode & STGM_TRANSACTED)
		FIXME(ole,"We do not support transacted Compound Storage. Using direct mode.\n");
	_create_istorage16(ppstg);
	lpstg = (IStream16Impl*)PTR_SEG_TO_LIN(*ppstg);
        DuplicateHandle( GetCurrentProcess(), This->hf, GetCurrentProcess(),
                         &lpstg->hf, 0, TRUE, DUPLICATE_SAME_ACCESS );
	lstrcpyAtoW(name,pwcsName);
	newpps = STORAGE_look_for_named_pps(lpstg->hf,This->stde.pps_dir,name);
	if (newpps==-1) {
		IStream16_fnRelease((IStream16*)lpstg);
		return E_FAIL;
	}

	if (1!=STORAGE_get_pps_entry(lpstg->hf,newpps,&(lpstg->stde))) {
		IStream16_fnRelease((IStream16*)lpstg);
		return E_FAIL;
	}
	lpstg->ppsent		= newpps;
	return S_OK;
}

/******************************************************************************
 * IStorage16_OpenStream [STORAGE.504]
 */
HRESULT WINAPI IStorage16_fnOpenStream(
	LPSTORAGE16 iface,LPCOLESTR16 pwcsName, void *reserved1, DWORD grfMode, DWORD reserved2, IStream16 **ppstm
) {
	ICOM_THIS(IStorage16Impl,iface);
	IStream16Impl*	lpstr;
	WCHAR		name[33];
	int		newpps;

	TRACE(relay,"(%p)->(%s,%p,0x%08lx,0x%08lx,%p)\n",
		This,pwcsName,reserved1,grfMode,reserved2,ppstm
	);
	if (grfMode & STGM_TRANSACTED)
		FIXME(ole,"We do not support transacted Compound Storage. Using direct mode.\n");
	_create_istream16(ppstm);
	lpstr = (IStream16Impl*)PTR_SEG_TO_LIN(*ppstm);
        DuplicateHandle( GetCurrentProcess(), This->hf, GetCurrentProcess(),
                         &lpstr->hf, 0, TRUE, DUPLICATE_SAME_ACCESS );
	lstrcpyAtoW(name,pwcsName);
	newpps = STORAGE_look_for_named_pps(lpstr->hf,This->stde.pps_dir,name);
	if (newpps==-1) {
		IStream16_fnRelease((IStream16*)lpstr);
		return E_FAIL;
	}

	if (1!=STORAGE_get_pps_entry(lpstr->hf,newpps,&(lpstr->stde))) {
		IStream16_fnRelease((IStream16*)lpstr);
		return E_FAIL;
	}
	lpstr->offset.LowPart	= 0;
	lpstr->offset.HighPart	= 0;
	lpstr->ppsent		= newpps;
	return S_OK;
}

/******************************************************************************
 * _create_istorage16 [INTERNAL]
 */
static void _create_istorage16(LPSTORAGE16 *stg) {
	IStorage16Impl*	lpst;

	if (!stvt16.fnQueryInterface) {
		HMODULE16	wp = GetModuleHandle16("STORAGE");
		if (wp>=32) {
#define VTENT(xfn)  stvt16.fn##xfn = (void*)WIN32_GetProcAddress16(wp,"IStorage16_"#xfn);
			VTENT(QueryInterface)
			VTENT(AddRef)
			VTENT(Release)
			VTENT(CreateStream)
			VTENT(OpenStream)
			VTENT(CreateStorage)
			VTENT(OpenStorage)
			VTENT(CopyTo)
			VTENT(MoveElementTo)
			VTENT(Commit)
			VTENT(Revert)
			VTENT(EnumElements)
			VTENT(DestroyElement)
			VTENT(RenameElement)
			VTENT(SetElementTimes)
			VTENT(SetClass)
			VTENT(SetStateBits)
			VTENT(Stat)
#undef VTENT
			segstvt16 = SEGPTR_NEW(ICOM_VTABLE(IStorage16));
			memcpy(segstvt16,&stvt16,sizeof(stvt16));
			segstvt16 = (ICOM_VTABLE(IStorage16)*)SEGPTR_GET(segstvt16);
		} else {
#define VTENT(xfn) stvt16.fn##xfn = IStorage16_fn##xfn;
			VTENT(QueryInterface)
			VTENT(AddRef)
			VTENT(Release)
			VTENT(CreateStream)
			VTENT(OpenStream)
			VTENT(CreateStorage)
			VTENT(OpenStorage)
			VTENT(CopyTo)
			VTENT(Commit)
	/*  not (yet) implemented ...
			VTENT(MoveElementTo)
			VTENT(Revert)
			VTENT(EnumElements)
			VTENT(DestroyElement)
			VTENT(RenameElement)
			VTENT(SetElementTimes)
			VTENT(SetClass)
			VTENT(SetStateBits)
			VTENT(Stat)
	*/
#undef VTENT
			segstvt16 = &stvt16;
		}
	}
	lpst = SEGPTR_NEW(IStorage16Impl);
	lpst->lpvtbl	= segstvt16;
	lpst->ref	= 1;
	lpst->thisptr	= SEGPTR_GET(lpst);
	*stg = (void*)lpst->thisptr;
}

/******************************************************************************
 *	Storage API functions
 */

/******************************************************************************
 *		StgOpenStorageOnILockBytes    [OLE32.149]
 */
HRESULT WINAPI StgOpenStorageOnILockBytes(ILockBytes *plkbyt, IStorage *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstgOpen)
{
	FIXME(ole, "(not shown) stub!\n");
	return S_OK;
}


/******************************************************************************
 *		StgCreateDocfileOnILockBytes    [OLE32.145]
 */
HRESULT WINAPI StgCreateDocfileOnILockBytes(ILockBytes *plkbyt,DWORD grfMode, DWORD reserved, IStorage** ppstgOpen)
{
	FIXME(ole, "(not shown) stub!\n");
	return S_OK;
}


/******************************************************************************
 *		StgCreateDocFile16	[STORAGE.1]
 */
HRESULT WINAPI StgCreateDocFile16(
	LPCOLESTR16 pwcsName,DWORD grfMode,DWORD reserved,IStorage16 **ppstgOpen
) {
	HFILE		hf;
	int		i,ret;
	IStorage16Impl*	lpstg;
	struct storage_pps_entry	stde;

	TRACE(ole,"(%s,0x%08lx,0x%08lx,%p)\n",
		pwcsName,grfMode,reserved,ppstgOpen
	);
	_create_istorage16(ppstgOpen);
	hf = CreateFileA(pwcsName,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_NEW,0,0);
	if (hf==INVALID_HANDLE_VALUE) {
		WARN(ole,"couldn't open file for storage:%ld\n",GetLastError());
		return E_FAIL;
	}
	lpstg = (IStorage16Impl*)PTR_SEG_TO_LIN(*ppstgOpen);
	lpstg->hf = hf;
	/* FIXME: check for existance before overwriting? */
	if (!STORAGE_init_storage(hf)) {
		CloseHandle(hf);
		return E_FAIL;
	}
	i=0;ret=0;
	while (!ret) { /* neither 1 nor <0 */
		ret=STORAGE_get_pps_entry(hf,i,&stde);
		if ((ret==1) && (stde.pps_type==5)) {
			lpstg->stde	= stde;
			lpstg->ppsent	= i;
			break;
		}
		i++;
	}
	if (ret!=1) {
		IStorage16_fnRelease((IStorage16*)lpstg); /* will remove it */
		return E_FAIL;
	}

	return S_OK;
}

/******************************************************************************
 * StgIsStorageFile16 [STORAGE.5]
 */
HRESULT WINAPI StgIsStorageFile16(LPCOLESTR16 fn) {
	HFILE		hf;
	OFSTRUCT	ofs;
	BYTE		magic[24];

	TRACE(ole,"(\'%s\')\n",fn);
	hf = OpenFile(fn,&ofs,OF_SHARE_DENY_NONE);
	if (hf==HFILE_ERROR)
		return STG_E_FILENOTFOUND;
	if (24!=_lread(hf,magic,24)) {
		WARN(ole," too short\n");
		_lclose(hf);
		return S_FALSE;
	}
	if (!memcmp(magic,STORAGE_magic,8)) {
		WARN(ole," -> YES\n");
		_lclose(hf);
		return S_OK;
	}
	if (!memcmp(magic,STORAGE_notmagic,8)) {
		WARN(ole," -> NO\n");
		_lclose(hf);
		return S_FALSE;
	}
	if (!memcmp(magic,STORAGE_oldmagic,8)) {
		WARN(ole," -> old format\n");
		_lclose(hf);
		return STG_E_OLDFORMAT;
	}
	WARN(ole," -> Invalid header.\n");
	_lclose(hf);
	return STG_E_INVALIDHEADER;
}

/******************************************************************************
 * StgIsStorageFile32 [OLE32.146]
 */
HRESULT WINAPI 
StgIsStorageFile(LPCOLESTR fn) 
{
	LPOLESTR16	xfn = HEAP_strdupWtoA(GetProcessHeap(),0,fn);
	OLESTATUS	ret = StgIsStorageFile16(xfn);

	HeapFree(GetProcessHeap(),0,xfn);
	return ret;
}


/******************************************************************************
 * StgOpenStorage16 [STORAGE.3]
 */
HRESULT WINAPI StgOpenStorage16(
	const OLECHAR16 * pwcsName,IStorage16 *pstgPriority,DWORD grfMode,
	SNB16 snbExclude,DWORD reserved, IStorage16 **ppstgOpen
) {
	HFILE		hf;
	int		ret,i;
	IStorage16Impl*	lpstg;
	struct storage_pps_entry	stde;

	TRACE(ole,"(%s,%p,0x%08lx,%p,%ld,%p)\n",
		pwcsName,pstgPriority,grfMode,snbExclude,reserved,ppstgOpen
	);
	_create_istorage16(ppstgOpen);
	hf = CreateFileA(pwcsName,GENERIC_READ,0,NULL,0,0,0);
	if (hf==INVALID_HANDLE_VALUE) {
		WARN(ole,"Couldn't open file for storage\n");
		return E_FAIL;
	}
	lpstg = (IStorage16Impl*)PTR_SEG_TO_LIN(*ppstgOpen);
	lpstg->hf = hf;

	i=0;ret=0;
	while (!ret) { /* neither 1 nor <0 */
		ret=STORAGE_get_pps_entry(hf,i,&stde);
		if ((ret==1) && (stde.pps_type==5)) {
			lpstg->stde=stde;
			break;
		}
		i++;
	}
	if (ret!=1) {
		IStorage16_fnRelease((IStorage16*)lpstg); /* will remove it */
		return E_FAIL;
	}
	return S_OK;
	
}


