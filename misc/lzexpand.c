/*
 * LZ Decompression functions 
 *
 * Copyright 1996 Marcus Meissner
 */
/* 
 * FIXME: return values might be wrong
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "windows.h"
#include "file.h"
#include "heap.h"
#include "ldt.h"
#include "lzexpand.h"
#include "debug.h"


/* The readahead length of the decompressor. Reading single bytes
 * using _lread() would be SLOW.
 */
#define	GETLEN	2048

/* Format of first 14 byte of LZ compressed file */
struct lzfileheader {
	BYTE	magic[8];
	BYTE	compressiontype;
	CHAR	lastchar;
	DWORD	reallength;		
};
static BYTE LZMagic[8]={'S','Z','D','D',0x88,0xf0,0x27,0x33};

static struct lzstate {
	HFILE32	lzfd;		/* the handle used by the program */
	HFILE32	realfd;		/* the real filedescriptor */
	CHAR	lastchar;	/* the last char of the filename */

	DWORD	reallength;	/* the decompressed length of the file */
	DWORD	realcurrent;	/* the position the decompressor currently is */
	DWORD	realwanted;	/* the position the user wants to read from */

	BYTE	table[0x1000];	/* the rotating LZ table */
	UINT32	curtabent;	/* CURrent TABle ENTry */

	BYTE	stringlen;	/* length and position of current string */ 
	DWORD	stringpos;	/* from stringtable */


	WORD	bytetype;	/* bitmask within blocks */

	BYTE	*get;		/* GETLEN bytes */
	DWORD	getcur;		/* current read */
	DWORD	getlen;		/* length last got */
} *lzstates=NULL;
static int nroflzstates=0;

/* reads one compressed byte, including buffering */
#define GET(lzs,b)	_lzget(lzs,&b)
#define GET_FLUSH(lzs)	lzs->getcur=lzs->getlen;

static int
_lzget(struct lzstate *lzs,BYTE *b) {
	if (lzs->getcur<lzs->getlen) {
		*b		= lzs->get[lzs->getcur++];
		return		1;
	} else {
		int ret = _lread32(lzs->realfd,lzs->get,GETLEN);
		if (ret==HFILE_ERROR32)
			return HFILE_ERROR32;
		if (ret==0)
			return 0;
		lzs->getlen	= ret;
		lzs->getcur	= 1;
		*b		= *(lzs->get);
		return 1;
	}
}
/* internal function, reads lzheader
 * returns BADINHANDLE for non filedescriptors
 * return 0 for file not compressed using LZ 
 * return UNKNOWNALG for unknown algorithm
 * returns lzfileheader in *head
 */
static INT32 read_header(HFILE32 fd,struct lzfileheader *head)
{
	BYTE	buf[14];

	if (_llseek32(fd,0,SEEK_SET)==-1)
		return LZERROR_BADINHANDLE;

	/* We can't directly read the lzfileheader struct due to 
	 * structure element alignment
	 */
	if (_lread32(fd,buf,14)<14)
		return 0;
	memcpy(head->magic,buf,8);
	memcpy(&(head->compressiontype),buf+8,1);
	memcpy(&(head->lastchar),buf+9,1);

	/* FIXME: consider endianess on non-intel architectures */
	memcpy(&(head->reallength),buf+10,4);

	if (memcmp(head->magic,LZMagic,8))
		return 0;
	if (head->compressiontype!='A')
		return LZERROR_UNKNOWNALG;
	return 1;
}

/***********************************************************************
 *           LZStart16   (LZEXPAND.7)
 */
INT16 WINAPI LZStart16(void)
{
    TRACE(file,"(void)\n");
    return 1;
}


/***********************************************************************
 *           LZStart32   (LZ32.6)
 */
INT32 WINAPI LZStart32(void)
{
    TRACE(file,"(void)\n");
    return 1;
}


/***********************************************************************
 *           LZInit16   (LZEXPAND.3)
 */
HFILE16 WINAPI LZInit16( HFILE16 hfSrc )
{
    return LZInit32( HFILE16_TO_HFILE32(hfSrc) );
}


/***********************************************************************
 *           LZInit32   (LZ32.2)
 *
 * initializes internal decompression buffers, returns lzfiledescriptor.
 * (return value the same as hfSrc, if hfSrc is not compressed)
 * on failure, returns error code <0
 * lzfiledescriptors range from 0x400 to 0x410 (only 16 open files per process)
 * we use as much as we need, we just OR 0x400 to the passed HFILE.
 *
 * since _llseek uses the same types as libc.lseek, we just use the macros of 
 *  libc
 */
HFILE32 WINAPI LZInit32( HFILE32 hfSrc )
{

	struct	lzfileheader	head;
	struct	lzstate		*lzs;
	DWORD	ret;

	TRACE(file,"(%d)\n",hfSrc);
	ret=read_header(hfSrc,&head);
	if (ret<=0) {
		_llseek32(hfSrc,0,SEEK_SET);
		return ret?ret:hfSrc;
	}
	lzstates = HeapReAlloc( SystemHeap, 0, lzstates,
                                (++nroflzstates)*sizeof(struct lzstate) );
	lzs		= lzstates+(nroflzstates-1);

	memset(lzs,'\0',sizeof(*lzs));
	lzs->realfd	= hfSrc;
	lzs->lzfd	= hfSrc | 0x400;
	lzs->lastchar	= head.lastchar;
	lzs->reallength = head.reallength;

	lzs->get	= HEAP_xalloc( GetProcessHeap(), 0, GETLEN );
	lzs->getlen	= 0;
	lzs->getcur	= 0;

	/* Yes, preinitialize with spaces */
	memset(lzs->table,' ',0x1000);
	/* Yes, start 16 byte from the END of the table */
	lzs->curtabent	= 0xff0; 
	return lzs->lzfd;
}


/***********************************************************************
 *           LZDone   (LZEXPAND.9) (LZ32.8)
 */
void WINAPI LZDone(void)
{
    TRACE(file,"(void)\n");
}


/***********************************************************************
 *           GetExpandedName16   (LZEXPAND.10)
 */
INT16 WINAPI GetExpandedName16( LPCSTR in, LPSTR out )
{
    return (INT16)GetExpandedName32A( in, out );
}


/***********************************************************************
 *           GetExpandedName32A   (LZ32.9)
 *
 * gets the full filename of the compressed file 'in' by opening it
 * and reading the header
 *
 * "file." is being translated to "file"
 * "file.bl_" (with lastchar 'a') is being translated to "file.bla"
 * "FILE.BL_" (with lastchar 'a') is being translated to "FILE.BLA"
 */

INT32 WINAPI GetExpandedName32A( LPCSTR in, LPSTR out )
{
	struct lzfileheader	head;
	HFILE32		fd;
	OFSTRUCT	ofs;
	INT32		fnislowercased,ret,len;
	LPSTR		s,t;

	TRACE(file,"(%s)\n",in);
	fd=OpenFile32(in,&ofs,OF_READ);
	if (fd==HFILE_ERROR32)
		return (INT32)(INT16)LZERROR_BADINHANDLE;
	strcpy(out,in);
	ret=read_header(fd,&head);
	if (ret<=0) {
		/* not a LZ compressed file, so the expanded name is the same
		 * as the input name */
		_lclose32(fd);
		return 1;
	}


	/* look for directory prefix and skip it. */
	s=out;
	while (NULL!=(t=strpbrk(s,"/\\:")))
		s=t+1;

	/* now mangle the basename */
	if (!*s) {
		/* FIXME: hmm. shouldn't happen? */
		WARN(file,"Specified a directory or what? (%s)\n",in);
		_lclose32(fd);
		return 1;
	}
	/* see if we should use lowercase or uppercase on the last char */
	fnislowercased=1;
	t=s+strlen(s)-1;
	while (t>=out) {
		if (!isalpha(*t)) {
			t--;
			continue;
		}
		fnislowercased=islower(*t);
		break;
	}
	if (isalpha(head.lastchar)) {
		if (fnislowercased)
			head.lastchar=tolower(head.lastchar);
		else
			head.lastchar=toupper(head.lastchar);
	}	

	/* now look where to replace the last character */
	if (NULL!=(t=strchr(s,'.'))) {
		if (t[1]=='\0') {
			t[0]='\0';
		} else {
			len=strlen(t)-1;
			if (t[len]=='_')
				t[len]=head.lastchar;
		}
	} /* else no modification necessary */
	_lclose32(fd);
	return 1;
}


/***********************************************************************
 *           GetExpandedName32W   (LZ32.11)
 */
INT32 WINAPI GetExpandedName32W( LPCWSTR in, LPWSTR out )
{
	char	*xin,*xout;
	INT32	ret;

	xout	= HeapAlloc( GetProcessHeap(), 0, lstrlen32W(in)+3 );
	xin	= HEAP_strdupWtoA( GetProcessHeap(), 0, in );
	ret	= GetExpandedName16(xin,xout);
	if (ret>0) lstrcpyAtoW(out,xout);
	HeapFree( GetProcessHeap(), 0, xin );
	HeapFree( GetProcessHeap(), 0, xout );
	return	ret;
}


/***********************************************************************
 *           LZRead16   (LZEXPAND.5)
 */
INT16 WINAPI LZRead16( HFILE16 fd, LPVOID buf, UINT16 toread )
{
    return LZRead32(fd,buf,toread);
}


/***********************************************************************
 *           LZRead32   (LZ32.4)
 */
INT32 WINAPI LZRead32( HFILE32 fd, LPVOID vbuf, UINT32 toread )
{
	int	i,howmuch;
	BYTE	b,*buf;
	struct	lzstate	*lzs;

	buf=(LPBYTE)vbuf;
	TRACE(file,"(%d,%p,%d)\n",fd,buf,toread);
	howmuch=toread;
	for (i=0;i<nroflzstates;i++)
		if (lzstates[i].lzfd==fd)
			break;
	if (i==nroflzstates)
		return _lread32(fd,buf,toread);
	lzs=lzstates+i;

/* The decompressor itself is in a define, cause we need it twice
 * in this function. (the decompressed byte will be in b)
 */
#define DECOMPRESS_ONE_BYTE 						\
		if (lzs->stringlen) {					\
			b		= lzs->table[lzs->stringpos];	\
			lzs->stringpos	= (lzs->stringpos+1)&0xFFF;	\
			lzs->stringlen--;				\
		} else {						\
			if (!(lzs->bytetype&0x100)) {			\
				if (1!=GET(lzs,b)) 			\
					return toread-howmuch;		\
				lzs->bytetype = b|0xFF00;		\
			}						\
			if (lzs->bytetype & 1) {			\
				if (1!=GET(lzs,b))			\
					return toread-howmuch;		\
			} else {					\
				BYTE	b1,b2;				\
									\
				if (1!=GET(lzs,b1))			\
					return toread-howmuch;		\
				if (1!=GET(lzs,b2))			\
					return toread-howmuch;		\
				/* Format:				\
				 * b1 b2				\
				 * AB CD 				\
				 * where CAB is the stringoffset in the table\
				 * and D+3 is the len of the string	\
				 */					\
				lzs->stringpos	= b1|((b2&0xf0)<<4);	\
				lzs->stringlen	= (b2&0xf)+2; 		\
				/* 3, but we use a  byte already below ... */\
				b		= lzs->table[lzs->stringpos];\
				lzs->stringpos	= (lzs->stringpos+1)&0xFFF;\
			}						\
			lzs->bytetype>>=1;				\
		}							\
		/* store b in table */					\
		lzs->table[lzs->curtabent++]= b;			\
		lzs->curtabent	&= 0xFFF;				\
		lzs->realcurrent++;

	/* if someone has seeked, we have to bring the decompressor 
	 * to that position
	 */
	if (lzs->realcurrent!=lzs->realwanted) {
		/* if the wanted position is before the current position 
		 * I see no easy way to unroll ... We have to restart at
		 * the beginning. *sigh*
		 */
		if (lzs->realcurrent>lzs->realwanted) {
			/* flush decompressor state */
			_llseek32(lzs->realfd,14,SEEK_SET);
			GET_FLUSH(lzs);
			lzs->realcurrent= 0;
			lzs->bytetype	= 0;
			lzs->stringlen	= 0;
			memset(lzs->table,' ',0x1000);
			lzs->curtabent	= 0xFF0;
		}
		while (lzs->realcurrent<lzs->realwanted) {
			DECOMPRESS_ONE_BYTE;
		}
	}

	while (howmuch) {
		DECOMPRESS_ONE_BYTE;
		lzs->realwanted++;
		*buf++		= b;
		howmuch--;
	}
	return 	toread;
#undef DECOMPRESS_ONE_BYTE
}


/***********************************************************************
 *           LZSeek16   (LZEXPAND.4)
 */
LONG WINAPI LZSeek16( HFILE16 fd, LONG off, INT16 type )
{
    return LZSeek32( fd, off, type );
}


/***********************************************************************
 *           LZSeek32   (LZ32.3)
 */
LONG WINAPI LZSeek32( HFILE32 fd, LONG off, INT32 type )
{
	int	i;
	struct	lzstate	*lzs;
	LONG	newwanted;

	TRACE(file,"(%d,%ld,%d)\n",fd,off,type);
	for (i=0;i<nroflzstates;i++)
		if (lzstates[i].lzfd==fd)
			break;
	/* not compressed? just use normal _llseek() */
	if (i==nroflzstates)
		return _llseek32(fd,off,type);
	lzs		= lzstates+i;
	newwanted	= lzs->realwanted;
	switch (type) {
	case 1:	/* SEEK_CUR */
		newwanted      += off;
		break;
	case 2:	/* SEEK_END */
		newwanted	= lzs->reallength-off;
		break;
	default:/* SEEK_SET */
		newwanted	= off;
		break;
	}
	if (newwanted>lzs->reallength)
		return LZERROR_BADVALUE;
	if (newwanted<0)
		return LZERROR_BADVALUE;
	lzs->realwanted	= newwanted;
	return newwanted;
}


/***********************************************************************
 *           LZCopy16   (LZEXPAND.1)
 */
LONG WINAPI LZCopy16( HFILE16 src, HFILE16 dest )
{
    return LZCopy32( src, dest );
}


/***********************************************************************
 *           LZCopy32   (LZ32.0)
 *
 * Copies everything from src to dest
 * if src is a LZ compressed file, it will be uncompressed.
 * will return the number of bytes written to dest or errors.
 */
LONG WINAPI LZCopy32( HFILE32 src, HFILE32 dest )
{
	int	usedlzinit=0,i,ret,wret;
	LONG	len;
	HFILE32	oldsrc = src;
#define BUFLEN	1000
	BYTE	buf[BUFLEN];
	INT32	WINAPI (*xread)(HFILE32,LPVOID,UINT32);

	TRACE(file,"(%d,%d)\n",src,dest);
	if (src<0x400) {
		src = LZInit32(src);
		if (src!=oldsrc)
			usedlzinit=1;
		if (src>0xfff0)
			return 0;
	}

	for (i=0;i<nroflzstates;i++)
		if (src==lzstates[i].lzfd)
			break;
	/* not compressed? just copy */
	if (i==nroflzstates)
		xread=(INT32(*)(HFILE32,LPVOID,UINT32))_lread32;
	else
		xread=LZRead32;
	len=0;
	while (1) {
		ret=xread(src,buf,BUFLEN);
		if (ret<=0) {
			if (ret==0)
				break;
			if (ret==-1)
				return LZERROR_READ;
			return ret;
		}
		len    += ret;
		wret	= _lwrite32(dest,buf,ret);
		if (wret!=ret)
			return LZERROR_WRITE;
	}
	if (usedlzinit)
		LZClose32(src);
	return len;
#undef BUFLEN
}

/* reverses GetExpandedPathname */
static LPSTR LZEXPAND_MangleName( LPCSTR fn )
{
    char *p;
    char *mfn = (char *)HEAP_xalloc( GetProcessHeap(), 0,
                                     strlen(fn) + 3 ); /* "._" and \0 */
    strcpy( mfn, fn );
    if (!(p = strrchr( mfn, '\\' ))) p = mfn;
    if ((p = strchr( p, '.' )))
    {
        p++;
        if (strlen(p) < 3) strcat( p, "_" );  /* append '_' */
        else p[strlen(p)-1] = '_';  /* replace last character */
    }
    else strcat( mfn, "._" );	/* append "._" */
    return mfn;
}


/***********************************************************************
 *           LZOpenFile16   (LZEXPAND.2)
 */
HFILE16 WINAPI LZOpenFile16( LPCSTR fn, LPOFSTRUCT ofs, UINT16 mode )
{
    return LZOpenFile32A( fn, ofs, mode );
}


/***********************************************************************
 *           LZOpenFile32A   (LZ32.1)
 *
 * Opens a file. If not compressed, open it as a normal file.
 */
HFILE32 WINAPI LZOpenFile32A( LPCSTR fn, LPOFSTRUCT ofs, UINT32 mode )
{
	HFILE32	fd,cfd;

	TRACE(file,"(%s,%p,%d)\n",fn,ofs,mode);
	/* 0x70 represents all OF_SHARE_* flags, ignore them for the check */
	fd=OpenFile32(fn,ofs,mode);
	if (fd==HFILE_ERROR32)
        {
            LPSTR mfn = LZEXPAND_MangleName(fn);
            fd = OpenFile32(mfn,ofs,mode);
            HeapFree( GetProcessHeap(), 0, mfn );
	}
	if ((mode&~0x70)!=OF_READ)
		return fd;
	if (fd==HFILE_ERROR32)
		return HFILE_ERROR32;
	cfd=LZInit32(fd);
	if (cfd<=0)
		return fd;
	return cfd;
}


/***********************************************************************
 *           LZOpenFile32W   (LZ32.10)
 */
HFILE32 WINAPI LZOpenFile32W( LPCWSTR fn, LPOFSTRUCT ofs, UINT32 mode )
{
	LPSTR	xfn;
	LPWSTR	yfn;
	HFILE32	ret;

	xfn	= HEAP_strdupWtoA( GetProcessHeap(), 0, fn);
	ret	= LZOpenFile16(xfn,ofs,mode);
	HeapFree( GetProcessHeap(), 0, xfn );
	if (ret!=HFILE_ERROR32) {
		/* ofs->szPathName is an array with the OFSTRUCT */
                yfn = HEAP_strdupAtoW( GetProcessHeap(), 0, ofs->szPathName );
		memcpy(ofs->szPathName,yfn,lstrlen32W(yfn)*2+2);
                HeapFree( GetProcessHeap(), 0, yfn );
	}
	return	ret;
}


/***********************************************************************
 *           LZClose16   (LZEXPAND.6)
 */
void WINAPI LZClose16( HFILE16 fd )
{
    return LZClose32( fd );
}


/***********************************************************************
 *           LZClose32   (LZ32.5)
 */
void WINAPI LZClose32( HFILE32 fd )
{
	int	i;

	TRACE(file,"(%d)\n",fd);
	for (i=0;i<nroflzstates;i++)
		if (lzstates[i].lzfd==fd)
			break;
	if (i==nroflzstates) {
		_lclose32(fd);
		return;
	}
	if (lzstates[i].get)
		HeapFree( GetProcessHeap(), 0, lzstates[i].get );
	_lclose32(lzstates[i].realfd);
	memmove(lzstates+i,lzstates+i+1,
                sizeof(struct lzstate)*(nroflzstates-i-1));
	nroflzstates--;
	lzstates = HeapReAlloc( SystemHeap, 0, lzstates,
                                sizeof(struct lzstate)*nroflzstates );
}

/***********************************************************************
 *           CopyLZFile16   (LZEXPAND.8)
 */
LONG WINAPI CopyLZFile16( HFILE16 src, HFILE16 dest )
{
    TRACE(file,"(%d,%d)\n",src,dest);
    return LZCopy32(src,dest);
}


/***********************************************************************
 *           CopyLZFile32  (LZ32.7)
 *
 * Copy src to dest (including uncompressing src).
 * NOTE: Yes. This is exactly the same function as LZCopy.
 */
LONG WINAPI CopyLZFile32( HFILE32 src, HFILE32 dest )
{
    TRACE(file,"(%d,%d)\n",src,dest);
    return LZCopy32(src,dest);
}
