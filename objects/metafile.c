/*
 * Metafile functions
 *
 * Copyright  David W. Metcalfe, 1994
 *            Niels de Carpentier, Albrecht Kleine, Huw Davies 1996
 *
 */

/*
 * These functions are primarily involved with metafile playback or anything
 * that touches a HMETAFILE.
 * For recording of metafiles look in graphics/metafiledrv/
 *
 * Note that (32 bit) HMETAFILEs are GDI objects, while HMETAFILE16s are 
 * global memory handles so these cannot be interchanged.
 *
 * Memory-based metafiles are just stored as a continuous block of memory with
 * a METAHEADER at the head with METARECORDs appended to it.  mtType is
 * METAFILE_MEMORY (1).  Note this is indentical to the disk image of a
 * disk-based metafile - even mtType is METAFILE_MEMORY.
 * 16bit HMETAFILE16s are global handles to this block
 * 32bit HMETAFILEs are GDI handles METAFILEOBJs, which contains a ptr to
 * the memory.
 * Disk-based metafiles are rather different. HMETAFILE16s point to a
 * METAHEADER which has mtType equal to METAFILE_DISK (2).  Following the 9
 * WORDs of the METAHEADER there are a further 3 WORDs of 0, 1 of 0x117, 1
 * more 0, then 2 which may be a time stamp of the file and then the path of
 * the file (METAHEADERDISK). I've copied this for 16bit compatibility.
 *
 * HDMD - 14/4/1999
 */  


#include <string.h>
#include <fcntl.h>
#include "wine/winbase16.h"
#include "metafile.h"
#include "bitmap.h"
#include "heap.h"
#include "toolhelp.h"
#include "debug.h"
#include "global.h"

/******************************************************************
 *         MF_AddHandle
 *
 *    Add a handle to an external handle table and return the index
 */
static int MF_AddHandle(HANDLETABLE16 *ht, WORD htlen, HGDIOBJ16 hobj)
{
    int i;

    for (i = 0; i < htlen; i++)
    {
	if (*(ht->objectHandle + i) == 0)
	{
	    *(ht->objectHandle + i) = hobj;
	    return i;
	}
    }
    return -1;
}


/******************************************************************
 *         MF_Create_HMETATFILE
 *
 * Creates a (32 bit) HMETAFILE object from a METAHEADER
 *
 * HMETAFILEs are GDI objects.
 */
HMETAFILE MF_Create_HMETAFILE(METAHEADER *mh)
{
    HMETAFILE hmf = GDI_AllocObject( sizeof(METAFILEOBJ), METAFILE_MAGIC );
    METAFILEOBJ *metaObj = (METAFILEOBJ *)GDI_HEAP_LOCK( hmf );
    metaObj->mh = mh;
    GDI_HEAP_UNLOCK( hmf );
    return hmf;
}

/******************************************************************
 *         MF_Create_HMETATFILE16
 *
 * Creates a HMETAFILE16 object from a METAHEADER
 *
 * HMETAFILE16s are Global memory handles.
 */
HMETAFILE16 MF_Create_HMETAFILE16(METAHEADER *mh)
{
    HMETAFILE16 hmf;
    DWORD size;

    if(mh->mtType == METAFILE_MEMORY)
        size = mh->mtSize * sizeof(WORD);
    else
        size = sizeof(METAHEADER) + sizeof(METAHEADERDISK);

    hmf = GLOBAL_CreateBlock( GMEM_MOVEABLE, mh, mh->mtSize * sizeof(WORD),
                              GetCurrentPDB16(), FALSE, FALSE, FALSE, NULL );
    return hmf;
}

/******************************************************************
 *         MF_GetMetaHeader
 *
 * Returns ptr to METAHEADER associated with HMETAFILE
 * Should be followed by call to MF_ReleaseMetaHeader
 */
static METAHEADER *MF_GetMetaHeader( HMETAFILE hmf )
{
    METAFILEOBJ *metaObj = (METAFILEOBJ *)GDI_GetObjPtr( hmf, METAFILE_MAGIC );
    return metaObj->mh;
}

/******************************************************************
 *         MF_GetMetaHeader16
 *
 * Returns ptr to METAHEADER associated with HMETAFILE16
 * Should be followed by call to MF_ReleaseMetaHeader16
 */
static METAHEADER *MF_GetMetaHeader16( HMETAFILE16 hmf )
{
    return GlobalLock16(hmf);
}

/******************************************************************
 *         MF_ReleaseMetaHeader
 *
 * Releases METAHEADER associated with HMETAFILE
 */
static BOOL MF_ReleaseMetaHeader( HMETAFILE hmf )
{
    return GDI_HEAP_UNLOCK( hmf );
}

/******************************************************************
 *         MF_ReleaseMetaHeader16
 *
 * Releases METAHEADER associated with HMETAFILE16
 */
static BOOL16 MF_ReleaseMetaHeader16( HMETAFILE16 hmf )
{
    return GlobalUnlock16( hmf );
}


/******************************************************************
 *	     DeleteMetaFile16   (GDI.127)
 */
BOOL16 WINAPI DeleteMetaFile16(  HMETAFILE16 hmf )
{
    return !GlobalFree16( hmf );
}

/******************************************************************
 *          DeleteMetaFile  (GDI32.69)
 *
 *  Delete a memory-based metafile.
 */

BOOL WINAPI DeleteMetaFile( HMETAFILE hmf )
{
    METAHEADER *mh = MF_GetMetaHeader( hmf );

    if(!mh) return FALSE;
    HeapFree( SystemHeap, 0, mh );
    GDI_FreeObject( hmf );
    return TRUE;
}

/******************************************************************
 *         MF_ReadMetaFile
 *
 * Returns a pointer to a memory based METAHEADER read in from file HFILE
 *
 */
static METAHEADER *MF_ReadMetaFile(HFILE hfile)
{
    METAHEADER *mh;
    DWORD BytesRead, size;

    size = sizeof(METAHEADER);
    mh = HeapAlloc( SystemHeap, 0, size );
    if(!mh) return NULL;
    if(ReadFile( hfile, mh, size, &BytesRead, NULL) == 0 ||
       BytesRead != size) {
        HeapFree( SystemHeap, 0, mh );
	return NULL;
    }
    size = mh->mtSize * 2;
    mh = HeapReAlloc( SystemHeap, 0, mh, size );
    if(!mh) return NULL;
    size -= sizeof(METAHEADER);
    if(ReadFile( hfile, (char *)mh + sizeof(METAHEADER), size, &BytesRead,
		 NULL) == 0 ||
       BytesRead != size) {
        HeapFree( SystemHeap, 0, mh );
	return NULL;
    }

    if (mh->mtType != METAFILE_MEMORY) {
        WARN(metafile, "Disk metafile had mtType = %04x\n", mh->mtType);
	mh->mtType = METAFILE_MEMORY;
    }
    return mh;
}

/******************************************************************
 *         GetMetaFile16   (GDI.124)
 */
HMETAFILE16 WINAPI GetMetaFile16( LPCSTR lpFilename )
{
    METAHEADER *mh;
    HFILE hFile;
 
    TRACE(metafile,"%s\n", lpFilename);

    if(!lpFilename)
        return 0;

    if((hFile = CreateFileA(lpFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
			    0, -1)) == HFILE_ERROR)
        return 0;

    mh = MF_ReadMetaFile(hFile);
    CloseHandle(hFile);
    if(!mh) return 0;
    return MF_Create_HMETAFILE16( mh );
}

/******************************************************************
 *         GetMetaFileA   (GDI32.197)
 *
 *  Read a metafile from a file. Returns handle to a memory-based metafile.
 */
HMETAFILE WINAPI GetMetaFileA( LPCSTR lpFilename )
{
    METAHEADER *mh;
    HFILE hFile;
 
    TRACE(metafile,"%s\n", lpFilename);

    if(!lpFilename)
        return 0;

    if((hFile = CreateFileA(lpFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
			    0, -1)) == HFILE_ERROR)
        return 0;

    mh = MF_ReadMetaFile(hFile);
    CloseHandle(hFile);
    if(!mh) return 0;
    return MF_Create_HMETAFILE( mh );
}



/******************************************************************
 *         GetMetaFileW   (GDI32.199)
 */
HMETAFILE WINAPI GetMetaFileW( LPCWSTR lpFilename )
{
    METAHEADER *mh;
    HFILE hFile;
 
    TRACE(metafile,"%s\n", debugstr_w(lpFilename));

    if(!lpFilename)
        return 0;

    if((hFile = CreateFileW(lpFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
			    0, -1)) == HFILE_ERROR)
        return 0;

    mh = MF_ReadMetaFile(hFile);
    CloseHandle(hFile);
    if(!mh) return 0;
    return MF_Create_HMETAFILE( mh );
}


/******************************************************************
 *         MF_LoadDiskBasedMetaFile
 *
 * Creates a new memory-based metafile from a disk-based one.
 */
static METAHEADER *MF_LoadDiskBasedMetaFile(METAHEADER *mh)
{
    METAHEADERDISK *mhd;
    HFILE hfile;
    METAHEADER *mh2;

    if(mh->mtType != METAFILE_DISK) {
        ERR(metafile, "Not a disk based metafile\n");
	return NULL;
    }
    mhd = (METAHEADERDISK *)((char *)mh + sizeof(METAHEADER));

    if((hfile = CreateFileA(mhd->filename, GENERIC_READ, 0, NULL,
			    OPEN_EXISTING, 0, -1)) == HFILE_ERROR) {
        WARN(metafile, "Can't open file of disk based metafile\n");
        return NULL;
    }
    mh2 = MF_ReadMetaFile(hfile);
    CloseHandle(hfile);
    return mh2;
}

/******************************************************************
 *         MF_CreateMetaHeaderDisk
 *
 * Take a memory based METAHEADER and change it to a disk based METAHEADER
 * assosiated with filename.  Note: Trashes contents of old one. 
 */
METAHEADER *MF_CreateMetaHeaderDisk(METAHEADER *mh, LPCSTR filename)
{
    METAHEADERDISK *mhd;
    DWORD size;

    mh = HeapReAlloc( SystemHeap, 0, mh,
		      sizeof(METAHEADER) + sizeof(METAHEADERDISK));
    mh->mtType = METAFILE_DISK;
    size = HeapSize( SystemHeap, 0, mh );
    mhd = (METAHEADERDISK *)((char *)mh + sizeof(METAHEADER));
    strcpy(mhd->filename, filename);
    return mh;
}

/******************************************************************
 *         CopyMetaFile16   (GDI.151)
 */
HMETAFILE16 WINAPI CopyMetaFile16( HMETAFILE16 hSrcMetaFile, LPCSTR lpFilename)
{
    METAHEADER *mh = MF_GetMetaHeader16( hSrcMetaFile );
    METAHEADER *mh2 = NULL;
    HFILE hFile;

    TRACE(metafile,"(%08x,%s)\n", hSrcMetaFile, lpFilename);
    
    if(!mh) return 0;
    
    if(mh->mtType == METAFILE_DISK)
        mh2 = MF_LoadDiskBasedMetaFile(mh);
    else {
        mh2 = HeapAlloc( SystemHeap, 0, mh->mtSize * 2 );
        memcpy( mh2, mh, mh->mtSize * 2 );
    }
    MF_ReleaseMetaHeader16( hSrcMetaFile );

    if(lpFilename) {         /* disk based metafile */
        if((hFile = CreateFileA(lpFilename, GENERIC_WRITE, 0, NULL,
				CREATE_ALWAYS, 0, -1)) == HFILE_ERROR) {
	    HeapFree( SystemHeap, 0, mh2 );
	    return 0;
	}
	WriteFile(hFile, mh2, mh2->mtSize * 2, NULL, NULL);
	CloseHandle(hFile);
	mh2 = MF_CreateMetaHeaderDisk(mh2, lpFilename);
    }

    return MF_Create_HMETAFILE16( mh2 );
}


/******************************************************************
 *         CopyMetaFileA   (GDI32.23)
 *
 *  Copies the metafile corresponding to hSrcMetaFile to either
 *  a disk file, if a filename is given, or to a new memory based
 *  metafile, if lpFileName is NULL.
 *
 * RETURNS
 *
 *  Handle to metafile copy on success, NULL on failure.
 *
 * BUGS
 *
 *  Copying to disk returns NULL even if successful.
 */
HMETAFILE WINAPI CopyMetaFileA(
		   HMETAFILE hSrcMetaFile, /* handle of metafile to copy */
		   LPCSTR lpFilename /* filename if copying to a file */
) {
    METAHEADER *mh = MF_GetMetaHeader( hSrcMetaFile );
    METAHEADER *mh2 = NULL;
    HFILE hFile;

    TRACE(metafile,"(%08x,%s)\n", hSrcMetaFile, lpFilename);
    
    if(!mh) return 0;
    
    if(mh->mtType == METAFILE_DISK)
        mh2 = MF_LoadDiskBasedMetaFile(mh);
    else {
        mh2 = HeapAlloc( SystemHeap, 0, mh->mtSize * 2 );
        memcpy( mh2, mh, mh->mtSize * 2 );
    }
    MF_ReleaseMetaHeader( hSrcMetaFile );

    if(lpFilename) {         /* disk based metafile */
        if((hFile = CreateFileA(lpFilename, GENERIC_WRITE, 0, NULL,
				CREATE_ALWAYS, 0, -1)) == HFILE_ERROR) {
	    HeapFree( SystemHeap, 0, mh2 );
	    return 0;
	}
	WriteFile(hFile, mh2, mh2->mtSize * 2, NULL, NULL);
	CloseHandle(hFile);
	mh2 = MF_CreateMetaHeaderDisk(mh2, lpFilename);
    }

    return MF_Create_HMETAFILE( mh2 );
}


/******************************************************************
 *         CopyMetaFileW   (GDI32.24)
 */
HMETAFILE WINAPI CopyMetaFileW( HMETAFILE hSrcMetaFile,
                                    LPCWSTR lpFilename )
{
    LPSTR p = HEAP_strdupWtoA( GetProcessHeap(), 0, lpFilename );
    HMETAFILE ret = CopyMetaFileA( hSrcMetaFile, p );
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}


/******************************************************************
 *         IsValidMetaFile   (GDI.410)
 *
 *  Attempts to check if a given metafile is correctly formatted.
 *  Currently, the only things verified are several properties of the
 *  header.
 *
 * RETURNS
 *  TRUE if hmf passes some tests for being a valid metafile, FALSE otherwise.
 *
 * BUGS
 *  This is not exactly what windows does, see _Undocumented_Windows_
 *  for details.
 */
BOOL16 WINAPI IsValidMetaFile16(HMETAFILE16 hmf)
{
    BOOL16 res=FALSE;
    METAHEADER *mh = MF_GetMetaHeader16(hmf);
    if (mh) {
        if (mh->mtType == METAFILE_MEMORY || mh->mtType == METAFILE_DISK) 
	    if (mh->mtHeaderSize == MFHEADERSIZE/sizeof(INT16))
	        if (mh->mtVersion == MFVERSION)
		    res=TRUE;
	MF_ReleaseMetaHeader16(hmf);
    }
    TRACE(metafile,"IsValidMetaFile %x => %d\n",hmf,res);
    return res;         
}


/*******************************************************************
 *         MF_PlayMetaFile
 *
 * Helper for PlayMetaFile
 */
static BOOL MF_PlayMetaFile( HDC hdc, METAHEADER *mh)
{

    METARECORD *mr;
    HANDLETABLE16 *ht;
    int offset = 0;
    WORD i;
    HPEN hPen;
    HBRUSH hBrush;
    HFONT hFont;
    DC *dc;
    BOOL loaded = FALSE;

    if (!mh) return FALSE;
    if(mh->mtType == METAFILE_DISK) { /* Create a memoery-based copy */
        mh = MF_LoadDiskBasedMetaFile(mh);
	if(!mh) return FALSE;
	loaded = TRUE;
    }

    /* save the current pen, brush and font */
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    hPen = dc->w.hPen;
    hBrush = dc->w.hBrush;
    hFont = dc->w.hFont;
    GDI_HEAP_UNLOCK(hdc);
    /* create the handle table */
    ht = HeapAlloc( SystemHeap, HEAP_ZERO_MEMORY,
		    sizeof(HANDLETABLE16) * mh->mtNoObjects);
    if(!ht) return FALSE;
    
    /* loop through metafile playing records */
    offset = mh->mtHeaderSize * 2;
    while (offset < mh->mtSize * 2)
    {
        mr = (METARECORD *)((char *)mh + offset);
	TRACE(metafile,"offset=%04x,size=%08lx\n",
            offset, mr->rdSize);
	if (!mr->rdSize) {
            TRACE(metafile,
		  "Entry got size 0 at offset %d, total mf length is %ld\n",
		  offset,mh->mtSize*2);
		break; /* would loop endlessly otherwise */
	}
	offset += mr->rdSize * 2;
	PlayMetaFileRecord16( hdc, ht, mr, mh->mtNoObjects );
    }

    SelectObject(hdc, hBrush);
    SelectObject(hdc, hPen);
    SelectObject(hdc, hFont);

    /* free objects in handle table */
    for(i = 0; i < mh->mtNoObjects; i++)
      if(*(ht->objectHandle + i) != 0)
        DeleteObject(*(ht->objectHandle + i));
    
    /* free handle table */
    HeapFree( SystemHeap, 0, ht );
    if(loaded)
        HeapFree( SystemHeap, 0, mh );
    return TRUE;
}

/******************************************************************
 *         PlayMetaFile16   (GDI.123)
 *
 */
BOOL16 WINAPI PlayMetaFile16( HDC16 hdc, HMETAFILE16 hmf )
{
    BOOL16 ret;
    METAHEADER *mh = MF_GetMetaHeader16( hmf );
    ret = MF_PlayMetaFile( hdc, mh );
    MF_ReleaseMetaHeader16( hmf );
    return ret;
}

/******************************************************************
 *         PlayMetaFile   (GDI32.265)
 *
 *  Renders the metafile specified by hmf in the DC specified by
 *  hdc. Returns FALSE on failure, TRUE on success.
 */
BOOL WINAPI PlayMetaFile( 
			     HDC hdc, /* handle of DC to render in */
			     HMETAFILE hmf /* handle of metafile to render */
)
{
    BOOL ret;
    METAHEADER *mh = MF_GetMetaHeader( hmf );
    ret = MF_PlayMetaFile( hdc, mh );
    MF_ReleaseMetaHeader( hmf );
    return ret;
}


/******************************************************************
 *            EnumMetaFile16   (GDI.175)
 *
 */
BOOL16 WINAPI EnumMetaFile16( HDC16 hdc, HMETAFILE16 hmf,
			      MFENUMPROC16 lpEnumFunc, LPARAM lpData )
{
    METAHEADER *mh = MF_GetMetaHeader16(hmf);
    METARECORD *mr;
    HANDLETABLE16 *ht;
    HGLOBAL16 hHT;
    SEGPTR spht;
    int offset = 0;
    WORD i, seg;
    HPEN hPen;
    HBRUSH hBrush;
    HFONT hFont;
    DC *dc;
    BOOL16 result = TRUE, loaded = FALSE;

    TRACE(metafile,"(%04x, %04x, %08lx, %08lx)\n",
		     hdc, hmf, (DWORD)lpEnumFunc, lpData);


    if(!mh) return FALSE;
    if(mh->mtType == METAFILE_DISK) { /* Create a memoery-based copy */
        mh = MF_LoadDiskBasedMetaFile(mh);
	if(!mh) return FALSE;
	loaded = TRUE;
    }

    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    hPen = dc->w.hPen;
    hBrush = dc->w.hBrush;
    hFont = dc->w.hFont;
    GDI_HEAP_UNLOCK(hdc);

    /* create the handle table */
    
    hHT = GlobalAlloc16(GMEM_MOVEABLE | GMEM_ZEROINIT,
		     sizeof(HANDLETABLE16) * mh->mtNoObjects);
    spht = WIN16_GlobalLock16(hHT);
   
    seg = GlobalHandleToSel16(hmf);
    offset = mh->mtHeaderSize * 2;
    
    /* loop through metafile records */
    
    while (offset < (mh->mtSize * 2))
    {
	mr = (METARECORD *)((char *)mh + offset);
        if (!lpEnumFunc( hdc, (HANDLETABLE16 *)spht,
			 (METARECORD *) PTR_SEG_OFF_TO_HUGEPTR(seg, offset),
                         mh->mtNoObjects, (LONG)lpData ))
	{
	    result = FALSE;
	    break;
	}
	

	offset += (mr->rdSize * 2);
    }

    SelectObject(hdc, hBrush);
    SelectObject(hdc, hPen);
    SelectObject(hdc, hFont);

    ht = (HANDLETABLE16 *)GlobalLock16(hHT);

    /* free objects in handle table */
    for(i = 0; i < mh->mtNoObjects; i++)
      if(*(ht->objectHandle + i) != 0)
        DeleteObject(*(ht->objectHandle + i));

    /* free handle table */
    GlobalFree16(hHT);
    if(loaded)
        HeapFree( SystemHeap, 0, mh );
    MF_ReleaseMetaHeader16(hmf);
    return result;
}

/******************************************************************
 *            EnumMetaFile   (GDI32.88)
 *
 *  Loop through the metafile records in hmf, calling the user-specified
 *  function for each one, stopping when the user's function returns FALSE
 *  (which is considered to be failure)
 *  or when no records are left (which is considered to be success). 
 *
 * RETURNS
 *  TRUE on success, FALSE on failure.
 * 
 * HISTORY
 *   Niels de carpentier, april 1996
 */
BOOL WINAPI EnumMetaFile( 
			     HDC hdc, 
			     HMETAFILE hmf,
			     MFENUMPROC lpEnumFunc, 
			     LPARAM lpData 
) {
    METAHEADER *mh = MF_GetMetaHeader(hmf);
    METARECORD *mr;
    HANDLETABLE *ht;
    BOOL result = TRUE, loaded = FALSE;
    int i, offset = 0;
    DC *dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    HPEN hPen;
    HBRUSH hBrush;
    HFONT hFont;

    TRACE(metafile,"(%08x,%08x,%p,%p)\n",
		     hdc, hmf, lpEnumFunc, (void*)lpData);
    if (!mh) return 0;
    if(mh->mtType == METAFILE_DISK) { /* Create a memoery-based copy */
        mh = MF_LoadDiskBasedMetaFile(mh);
	if(!mh) return 0;
	loaded = TRUE;
    }
    
    /* save the current pen, brush and font */
    if (!dc) return 0;
    hPen = dc->w.hPen;
    hBrush = dc->w.hBrush;
    hFont = dc->w.hFont;
    GDI_HEAP_UNLOCK(hdc);


    ht = HeapAlloc( SystemHeap, HEAP_ZERO_MEMORY, 
			    sizeof(HANDLETABLE) * mh->mtNoObjects);

    /* loop through metafile records */
    offset = mh->mtHeaderSize * 2;
    
    while (offset < (mh->mtSize * 2))
    {
	mr = (METARECORD *)((char *)mh + offset);
        if (!lpEnumFunc( hdc, ht, mr, mh->mtNoObjects, (LONG)lpData ))
	{
	    result = FALSE;
	    break;
	}
	
	offset += (mr->rdSize * 2);
    }

    /* restore pen, brush and font */
    SelectObject(hdc, hBrush);
    SelectObject(hdc, hPen);
    SelectObject(hdc, hFont);

    /* free objects in handle table */
    for(i = 0; i < mh->mtNoObjects; i++)
      if(*(ht->objectHandle + i) != 0)
        DeleteObject(*(ht->objectHandle + i));

    /* free handle table */
    HeapFree( SystemHeap, 0, ht);
    if(loaded)
        HeapFree( SystemHeap, 0, mh );
    MF_ReleaseMetaHeader(hmf);
    return result;
}

static BOOL MF_Play_MetaCreateRegion( METARECORD *mr, HRGN hrgn );

/******************************************************************
 *             PlayMetaFileRecord16   (GDI.176)
 *
 *   Render a single metafile record specified by *mr in the DC hdc, while
 *   using the handle table *ht, of length nHandles, 
 *   to store metafile objects.
 *
 * BUGS
 *  The following metafile records are unimplemented:
 *
 *  FRAMEREGION, DRAWTEXT, SETDIBTODEV, ANIMATEPALETTE, SETPALENTRIES,
 *  RESIZEPALETTE, EXTFLOODFILL, RESETDC, STARTDOC, STARTPAGE, ENDPAGE,
 *  ABORTDOC, ENDDOC, CREATEBRUSH, CREATEBITMAPINDIRECT, and CREATEBITMAP.
 *
 */
void WINAPI PlayMetaFileRecord16( 
	    HDC16 hdc, /* DC to render metafile into */
	    HANDLETABLE16 *ht, /* pointer to handle table for metafile objects */
	    METARECORD *mr, /* pointer to metafile record to render */
	    UINT16 nHandles /* size of handle table */
) {
    short s1;
    HANDLE16 hndl;
    char *ptr;
    BITMAPINFOHEADER *infohdr;

    TRACE(metafile,"(%04x %08lx %08lx %04x) function %04x\n",
		 hdc,(LONG)ht, (LONG)mr, nHandles, mr->rdFunction);
    
    switch (mr->rdFunction)
    {
    case META_EOF:
      break;

    case META_DELETEOBJECT:
      DeleteObject(*(ht->objectHandle + *(mr->rdParm)));
      *(ht->objectHandle + *(mr->rdParm)) = 0;
      break;

    case META_SETBKCOLOR:
	SetBkColor16(hdc, MAKELONG(*(mr->rdParm), *(mr->rdParm + 1)));
	break;

    case META_SETBKMODE:
	SetBkMode16(hdc, *(mr->rdParm));
	break;

    case META_SETMAPMODE:
	SetMapMode16(hdc, *(mr->rdParm));
	break;

    case META_SETROP2:
	SetROP216(hdc, *(mr->rdParm));
	break;

    case META_SETRELABS:
	SetRelAbs16(hdc, *(mr->rdParm));
	break;

    case META_SETPOLYFILLMODE:
	SetPolyFillMode16(hdc, *(mr->rdParm));
	break;

    case META_SETSTRETCHBLTMODE:
	SetStretchBltMode16(hdc, *(mr->rdParm));
	break;

    case META_SETTEXTCOLOR:
	SetTextColor16(hdc, MAKELONG(*(mr->rdParm), *(mr->rdParm + 1)));
	break;

    case META_SETWINDOWORG:
	SetWindowOrg16(hdc, *(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_SETWINDOWEXT:
	SetWindowExt16(hdc, *(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_SETVIEWPORTORG:
	SetViewportOrg16(hdc, *(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_SETVIEWPORTEXT:
	SetViewportExt16(hdc, *(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_OFFSETWINDOWORG:
	OffsetWindowOrg16(hdc, *(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_SCALEWINDOWEXT:
	ScaleWindowExt16(hdc, *(mr->rdParm + 3), *(mr->rdParm + 2),
		       *(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_OFFSETVIEWPORTORG:
	OffsetViewportOrg16(hdc, *(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_SCALEVIEWPORTEXT:
	ScaleViewportExt16(hdc, *(mr->rdParm + 3), *(mr->rdParm + 2),
			 *(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_LINETO:
	LineTo(hdc, (INT16)*(mr->rdParm + 1), (INT16)*(mr->rdParm));
	break;

    case META_MOVETO:
	MoveTo16(hdc, *(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_EXCLUDECLIPRECT:
	ExcludeClipRect16( hdc, *(mr->rdParm + 3), *(mr->rdParm + 2),
                           *(mr->rdParm + 1), *(mr->rdParm) );
	break;

    case META_INTERSECTCLIPRECT:
	IntersectClipRect16( hdc, *(mr->rdParm + 3), *(mr->rdParm + 2),
                             *(mr->rdParm + 1), *(mr->rdParm) );
	break;

    case META_ARC:
	Arc(hdc, (INT16)*(mr->rdParm + 7), (INT16)*(mr->rdParm + 6),
	      (INT16)*(mr->rdParm + 5), (INT16)*(mr->rdParm + 4),
	      (INT16)*(mr->rdParm + 3), (INT16)*(mr->rdParm + 2),
             (INT16)*(mr->rdParm + 1), (INT16)*(mr->rdParm));
	break;

    case META_ELLIPSE:
	Ellipse(hdc, (INT16)*(mr->rdParm + 3), (INT16)*(mr->rdParm + 2),
                  (INT16)*(mr->rdParm + 1), (INT16)*(mr->rdParm));
	break;

    case META_FLOODFILL:
	FloodFill(hdc, (INT16)*(mr->rdParm + 3), (INT16)*(mr->rdParm + 2),
                    MAKELONG(*(mr->rdParm), *(mr->rdParm + 1)));
	break;

    case META_PIE:
	Pie(hdc, (INT16)*(mr->rdParm + 7), (INT16)*(mr->rdParm + 6),
	      (INT16)*(mr->rdParm + 5), (INT16)*(mr->rdParm + 4),
	      (INT16)*(mr->rdParm + 3), (INT16)*(mr->rdParm + 2),
             (INT16)*(mr->rdParm + 1), (INT16)*(mr->rdParm));
	break;

    case META_RECTANGLE:
	Rectangle(hdc, (INT16)*(mr->rdParm + 3), (INT16)*(mr->rdParm + 2),
                    (INT16)*(mr->rdParm + 1), (INT16)*(mr->rdParm));
	break;

    case META_ROUNDRECT:
	RoundRect(hdc, (INT16)*(mr->rdParm + 5), (INT16)*(mr->rdParm + 4),
                    (INT16)*(mr->rdParm + 3), (INT16)*(mr->rdParm + 2),
                    (INT16)*(mr->rdParm + 1), (INT16)*(mr->rdParm));
	break;

    case META_PATBLT:
	PatBlt16(hdc, *(mr->rdParm + 5), *(mr->rdParm + 4),
                 *(mr->rdParm + 3), *(mr->rdParm + 2),
                 MAKELONG(*(mr->rdParm), *(mr->rdParm + 1)));
	break;

    case META_SAVEDC:
	SaveDC(hdc);
	break;

    case META_SETPIXEL:
	SetPixel(hdc, (INT16)*(mr->rdParm + 3), (INT16)*(mr->rdParm + 2),
                   MAKELONG(*(mr->rdParm), *(mr->rdParm + 1)));
	break;

    case META_OFFSETCLIPRGN:
	OffsetClipRgn16( hdc, *(mr->rdParm + 1), *(mr->rdParm) );
	break;

    case META_TEXTOUT:
	s1 = *(mr->rdParm);
	TextOut16(hdc, *(mr->rdParm + ((s1 + 1) >> 1) + 2),
                  *(mr->rdParm + ((s1 + 1) >> 1) + 1), 
                  (char *)(mr->rdParm + 1), s1);
	break;

    case META_POLYGON:
	Polygon16(hdc, (LPPOINT16)(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_POLYPOLYGON:
      PolyPolygon16(hdc, (LPPOINT16)(mr->rdParm + *(mr->rdParm) + 1),
                    (LPINT16)(mr->rdParm + 1), *(mr->rdParm)); 
      break;

    case META_POLYLINE:
	Polyline16(hdc, (LPPOINT16)(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_RESTOREDC:
	RestoreDC(hdc, (INT16)*(mr->rdParm));
	break;

    case META_SELECTOBJECT:
	SelectObject(hdc, *(ht->objectHandle + *(mr->rdParm)));
	break;

    case META_CHORD:
	Chord(hdc, (INT16)*(mr->rdParm + 7), (INT16)*(mr->rdParm + 6),
		(INT16)*(mr->rdParm+5), (INT16)*(mr->rdParm + 4),
		(INT16)*(mr->rdParm + 3), (INT16)*(mr->rdParm + 2),
               (INT16)*(mr->rdParm + 1), (INT16)*(mr->rdParm));
	break;

    case META_CREATEPATTERNBRUSH:
	switch (*(mr->rdParm))
	{
	case BS_PATTERN:
	    infohdr = (BITMAPINFOHEADER *)(mr->rdParm + 2);
	    MF_AddHandle(ht, nHandles,
			 CreatePatternBrush(CreateBitmap(infohdr->biWidth, 
				      infohdr->biHeight, 
				      infohdr->biPlanes, 
				      infohdr->biBitCount,
				      (LPSTR)(mr->rdParm + 
				      (sizeof(BITMAPINFOHEADER) / 2) + 4))));
	    break;

	case BS_DIBPATTERN:
	    s1 = mr->rdSize * 2 - sizeof(METARECORD) - 2;
	    hndl = GlobalAlloc16(GMEM_MOVEABLE, s1);
	    ptr = GlobalLock16(hndl);
	    memcpy(ptr, mr->rdParm + 2, s1);
	    GlobalUnlock16(hndl);
	    MF_AddHandle(ht, nHandles,
			 CreateDIBPatternBrush(hndl, *(mr->rdParm + 1)));
	    GlobalFree16(hndl);
	}
	break;
	
    case META_CREATEPENINDIRECT:
	MF_AddHandle(ht, nHandles, 
		     CreatePenIndirect16((LOGPEN16 *)(&(mr->rdParm))));
	break;

    case META_CREATEFONTINDIRECT:
	MF_AddHandle(ht, nHandles, 
		     CreateFontIndirect16((LOGFONT16 *)(&(mr->rdParm))));
	break;

    case META_CREATEBRUSHINDIRECT:
	MF_AddHandle(ht, nHandles, 
		     CreateBrushIndirect16((LOGBRUSH16 *)(&(mr->rdParm))));
	break;

    case META_CREATEPALETTE:
	MF_AddHandle(ht, nHandles, 
		     CreatePalette16((LPLOGPALETTE)mr->rdParm));
	break;

    case META_SETTEXTALIGN:
       	SetTextAlign16(hdc, *(mr->rdParm));
	break;

    case META_SELECTPALETTE:
	SelectPalette16(hdc, *(ht->objectHandle + *(mr->rdParm+1)),
			*(mr->rdParm));
	break;

    case META_SETMAPPERFLAGS:
	SetMapperFlags16(hdc, *(mr->rdParm));
	break;

    case META_REALIZEPALETTE:
	RealizePalette16(hdc);
	break;

    case META_ESCAPE:
	FIXME(metafile, "META_ESCAPE unimplemented.\n");
        break;

    case META_EXTTEXTOUT:
      {
        LPINT16 dxx;
	LPSTR sot; 
        DWORD len;

        s1 = mr->rdParm[2];                              /* String length */
        len = sizeof(METARECORD) + (((s1 + 1) >> 1) * 2) + 2 * sizeof(short)
	 + sizeof(UINT16) +  (mr->rdParm[3] ? sizeof(RECT16) : 0);  /* rec len without dx array */

	sot= (LPSTR)&mr->rdParm[4];			/* start_of_text */
	if (mr->rdParm[3])
	   sot+=sizeof(RECT16);				/* there is a rectangle, so add offset */
	 
	if (mr->rdSize == len / 2)
          dxx = NULL;                                   /* determine if array present */
        else 
	  if (mr->rdSize == (len + s1 * sizeof(INT16)) / 2)
            dxx = (LPINT16)(sot+(((s1+1)>>1)*2));	   
          else 
	  {
	   TRACE(metafile,"%s  len: %ld\n",
             sot,mr->rdSize);
           WARN(metafile,
	     "Please report: PlayMetaFile/ExtTextOut len=%ld slen=%d rdSize=%ld opt=%04x\n",
		   len,s1,mr->rdSize,mr->rdParm[3]);
           dxx = NULL; /* should't happen -- but if, we continue with NULL [for workaround] */
          }
        ExtTextOut16( hdc, mr->rdParm[1],              /* X position */
	                   mr->rdParm[0],              /* Y position */
	                   mr->rdParm[3],              /* options */
		     	   mr->rdParm[3] ? (LPRECT16) &mr->rdParm[4]:NULL,  /* rectangle */
		           sot,				/* string */
                           s1, dxx);                    /* length, dx array */
        if (dxx)                      
          TRACE(metafile,"%s  len: %ld  dx0: %d\n",
            sot,mr->rdSize,dxx[0]);
       }
       break;
    
    case META_STRETCHDIB:
      {
       LPBITMAPINFO info = (LPBITMAPINFO) &(mr->rdParm[11]);
       LPSTR bits = (LPSTR)info + DIB_BitmapInfoSize( info, mr->rdParm[2] );
       StretchDIBits16(hdc,mr->rdParm[10],mr->rdParm[9],mr->rdParm[8],
                       mr->rdParm[7],mr->rdParm[6],mr->rdParm[5],
                       mr->rdParm[4],mr->rdParm[3],bits,info,
                       mr->rdParm[2],MAKELONG(mr->rdParm[0],mr->rdParm[1]));
      }
      break;

    case META_DIBSTRETCHBLT:
      {
       LPBITMAPINFO info = (LPBITMAPINFO) &(mr->rdParm[10]); 
       LPSTR bits = (LPSTR)info + DIB_BitmapInfoSize( info, mr->rdParm[2] );
       StretchDIBits16(hdc,mr->rdParm[9],mr->rdParm[8],mr->rdParm[7],
                       mr->rdParm[6],mr->rdParm[5],mr->rdParm[4],
                       mr->rdParm[3],mr->rdParm[2],bits,info,
                       DIB_RGB_COLORS,MAKELONG(mr->rdParm[0],mr->rdParm[1]));
      }
      break;		  

    case META_STRETCHBLT:
      {
       HDC16 hdcSrc=CreateCompatibleDC16(hdc);
       HBITMAP hbitmap=CreateBitmap(mr->rdParm[10], /*Width */
                                        mr->rdParm[11], /*Height*/
                                        mr->rdParm[13], /*Planes*/
                                        mr->rdParm[14], /*BitsPixel*/
                                        (LPSTR)&mr->rdParm[15]);  /*bits*/
       SelectObject(hdcSrc,hbitmap);
       StretchBlt16(hdc,mr->rdParm[9],mr->rdParm[8],
                    mr->rdParm[7],mr->rdParm[6],
		    hdcSrc,mr->rdParm[5],mr->rdParm[4],
		    mr->rdParm[3],mr->rdParm[2],
		    MAKELONG(mr->rdParm[0],mr->rdParm[1]));
       DeleteDC(hdcSrc);		    
      }
      break;

    case META_BITBLT:
      {
       HDC16 hdcSrc=CreateCompatibleDC16(hdc);
       HBITMAP hbitmap=CreateBitmap(mr->rdParm[7]/*Width */,
                                        mr->rdParm[8]/*Height*/,
                                        mr->rdParm[10]/*Planes*/,
                                        mr->rdParm[11]/*BitsPixel*/,
                                        (LPSTR)&mr->rdParm[12]/*bits*/);
       SelectObject(hdcSrc,hbitmap);
       BitBlt(hdc,(INT16)mr->rdParm[6],(INT16)mr->rdParm[5],
                (INT16)mr->rdParm[4],(INT16)mr->rdParm[3],
                hdcSrc, (INT16)mr->rdParm[2],(INT16)mr->rdParm[1],
                MAKELONG(0,mr->rdParm[0]));
       DeleteDC(hdcSrc);		    
      }
      break;

    case META_CREATEREGION:
      {
	HRGN hrgn = CreateRectRgn(0,0,0,0);
 
	MF_Play_MetaCreateRegion(mr, hrgn);
	MF_AddHandle(ht, nHandles, hrgn);
      }
      break;

    case META_FILLREGION:
        FillRgn16(hdc, *(ht->objectHandle + *(mr->rdParm)),
		       *(ht->objectHandle + *(mr->rdParm+1)));
        break;

    case META_INVERTREGION:
        InvertRgn16(hdc, *(ht->objectHandle + *(mr->rdParm)));
        break; 

    case META_PAINTREGION:
        PaintRgn16(hdc, *(ht->objectHandle + *(mr->rdParm)));
        break;

    case META_SELECTCLIPREGION:
       	SelectClipRgn(hdc, *(ht->objectHandle + *(mr->rdParm)));
	break;

    case META_DIBCREATEPATTERNBRUSH:
	/*  *(mr->rdParm) may be BS_PATTERN or BS_DIBPATTERN: but there's no difference */
        TRACE(metafile,"%d\n",*(mr->rdParm));
	s1 = mr->rdSize * 2 - sizeof(METARECORD) - 2;
	hndl = GlobalAlloc16(GMEM_MOVEABLE, s1);
	ptr = GlobalLock16(hndl);
	memcpy(ptr, mr->rdParm + 2, s1);
	GlobalUnlock16(hndl);
	MF_AddHandle(ht, nHandles,CreateDIBPatternBrush16(hndl, *(mr->rdParm + 1)));
	GlobalFree16(hndl);
	break;

     case META_DIBBITBLT:
        {
		/*In practice ive found that theres two layout for META_DIBBITBLT,
		one (the first here) is the usual one when a src dc is actually passed
		int, the second occurs when the src dc is passed in as NULL to
		the creating BitBlt.
		as the second case has no dib, a size check will suffice to distinguish.


		Caolan.McNamara@ul.ie
		*/
		if (mr->rdSize > 12)
                {
                    LPBITMAPINFO info = (LPBITMAPINFO) &(mr->rdParm[8]);
                    LPSTR bits = (LPSTR)info + DIB_BitmapInfoSize( info, mr->rdParm[0] );
                    StretchDIBits16(hdc,mr->rdParm[7],mr->rdParm[6],mr->rdParm[5],
                                    mr->rdParm[4],mr->rdParm[3],mr->rdParm[2],
                                    mr->rdParm[5],mr->rdParm[4],bits,info,
                                    DIB_RGB_COLORS,MAKELONG(mr->rdParm[0],mr->rdParm[1]));
                }
		else	/*equivalent to a PatBlt*/
                {
                    PatBlt16(hdc, mr->rdParm[8], mr->rdParm[7],
                             mr->rdParm[6], mr->rdParm[5],
                             MAKELONG(*(mr->rdParm), *(mr->rdParm + 1)));
                }
        }
        break;	
       
    case META_SETTEXTCHAREXTRA:
        SetTextCharacterExtra16(hdc, (INT16)*(mr->rdParm));
	break;

    case META_SETTEXTJUSTIFICATION:
        SetTextJustification(hdc, *(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_EXTFLOODFILL:
        ExtFloodFill(hdc, (INT16)*(mr->rdParm + 4), (INT16)*(mr->rdParm + 3),
                     MAKELONG(*(mr->rdParm+1), *(mr->rdParm + 2)),*(mr->rdParm));
        break;

#define META_UNIMP(x) case x: FIXME(metafile, "PlayMetaFileRecord:record type "#x" not implemented.\n");break;
    META_UNIMP(META_FRAMEREGION)
    META_UNIMP(META_DRAWTEXT)
    META_UNIMP(META_SETDIBTODEV)
    META_UNIMP(META_ANIMATEPALETTE)
    META_UNIMP(META_SETPALENTRIES)
    META_UNIMP(META_RESIZEPALETTE)
    META_UNIMP(META_RESETDC)
    META_UNIMP(META_STARTDOC)
    META_UNIMP(META_STARTPAGE)
    META_UNIMP(META_ENDPAGE)
    META_UNIMP(META_ABORTDOC)
    META_UNIMP(META_ENDDOC)
    META_UNIMP(META_CREATEBRUSH)
    META_UNIMP(META_CREATEBITMAPINDIRECT)
    META_UNIMP(META_CREATEBITMAP)
#undef META_UNIMP

    default:
	WARN(metafile, "PlayMetaFileRecord: Unknown record type %x\n",
	                                      mr->rdFunction);
    }
}

/******************************************************************
 *         PlayMetaFileRecord   (GDI32.266)
 */
BOOL WINAPI PlayMetaFileRecord( HDC hdc,  HANDLETABLE *handletable, 
				METARECORD *metarecord, UINT handles )
{
    HANDLETABLE16 * ht = (void *)GlobalAlloc(GPTR, 
					     handles*sizeof(HANDLETABLE16));
    int i = 0;
    TRACE(metafile, "(%08x,%p,%p,%d)\n", hdc, handletable, metarecord,
	  handles); 
    for (i=0; i<handles; i++)  
        ht->objectHandle[i] =  handletable->objectHandle[i];
    PlayMetaFileRecord16(hdc, ht, metarecord, handles);
    for (i=0; i<handles; i++) 
        handletable->objectHandle[i] = ht->objectHandle[i];
    GlobalFree((HGLOBAL)ht);
    return TRUE;
}

/******************************************************************
 *         GetMetaFileBits   (GDI.159)
 *
 * Trade in a metafile object handle for a handle to the metafile memory.
 *
 */

HGLOBAL16 WINAPI GetMetaFileBits16(
				 HMETAFILE16 hmf /* metafile handle */
				 )
{
    TRACE(metafile,"hMem out: %04x\n", hmf);
    return hmf;
}

/******************************************************************
 *         SetMetaFileBits   (GDI.160)
 *
 * Trade in a metafile memory handle for a handle to a metafile object.
 * The memory region should hold a proper metafile, otherwise
 * problems will occur when it is used. Validity of the memory is not
 * checked. The function is essentially just the identity function.
 */
HMETAFILE16 WINAPI SetMetaFileBits16( 
				   HGLOBAL16 hMem 
			/* handle to a memory region holding a metafile */
)
{
    TRACE(metafile,"hmf out: %04x\n", hMem);

    return hMem;
}

/******************************************************************
 *         SetMetaFileBitsBetter   (GDI.196)
 *
 * Trade in a metafile memory handle for a handle to a metafile object,
 * making a cursory check (using IsValidMetaFile()) that the memory
 * handle points to a valid metafile.
 *
 * RETURNS
 *  Handle to a metafile on success, NULL on failure..
 */
HMETAFILE16 WINAPI SetMetaFileBitsBetter16( HMETAFILE16 hMeta )
{
   if( IsValidMetaFile16( hMeta ) )
       return (HMETAFILE16)GlobalReAlloc16( hMeta, 0, 
			   GMEM_SHARE | GMEM_NODISCARD | GMEM_MODIFY);
   return (HMETAFILE16)0;
}

/******************************************************************
 *         SetMetaFileBitsEx    (GDI32.323)
 * 
 *  Create a metafile from raw data. No checking of the data is performed.
 *  Use _GetMetaFileBitsEx_ to get raw data from a metafile.
 */
HMETAFILE WINAPI SetMetaFileBitsEx( 
     UINT size, /* size of metafile, in bytes */
     const BYTE *lpData /* pointer to metafile data */  
    )
{
    METAHEADER *mh = HeapAlloc( SystemHeap, 0, size );
    if (!mh) return 0;
    memcpy(mh, lpData, size);
    return MF_Create_HMETAFILE(mh);
}

/*****************************************************************
 *  GetMetaFileBitsEx     (GDI32.198)  Get raw metafile data
 * 
 *  Copies the data from metafile _hmf_ into the buffer _buf_.
 *  If _buf_ is zero, returns size of buffer required. Otherwise,
 *  returns number of bytes copied.
 */
UINT WINAPI GetMetaFileBitsEx( 
     HMETAFILE hmf, /* metafile */
     UINT nSize, /* size of buf */ 
     LPVOID buf   /* buffer to receive raw metafile data */  
) {
    METAHEADER *mh = MF_GetMetaHeader(hmf);
    UINT mfSize;

    TRACE(metafile, "(%08x,%d,%p)\n", hmf, nSize, buf);
    if (!mh) return 0;  /* FIXME: error code */
    if(mh->mtType == METAFILE_DISK)
        FIXME(metafile, "Disk-based metafile?\n");
    mfSize = mh->mtSize * 2;
    if (!buf) {
        MF_ReleaseMetaHeader(hmf);
	TRACE(metafile,"returning size %d\n", mfSize);
	return mfSize;
    }
    if(mfSize > nSize) mfSize = nSize;
    memmove(buf, mh, mfSize);
    MF_ReleaseMetaHeader(hmf);
    return mfSize;
}

/******************************************************************
 *         GetWinMetaFileBits [GDI32.241]
 */
UINT WINAPI GetWinMetaFileBits(HENHMETAFILE hemf,
                                UINT cbBuffer, LPBYTE lpbBuffer,
                                INT fnMapMode, HDC hdcRef)
{
  FIXME(metafile, "(%d,%d,%p,%d,%d): stub\n",
        hemf, cbBuffer, lpbBuffer, fnMapMode, hdcRef);
  return 0;
}

/******************************************************************
 *         MF_Play_MetaCreateRegion
 *
 *  Handles META_CREATEREGION for PlayMetaFileRecord().
 */

/*
 *	The layout of the record looks something like this:
 *	 
 *	 rdParm	meaning
 *	 0		Always 0?
 *	 1		Always 6?
 *	 2		Looks like a handle? - not constant
 *	 3		0 or 1 ??
 *	 4		Total number of bytes
 *	 5		No. of seperate bands = n [see below]
 *	 6		Largest number of x co-ords in a band
 *	 7-10		Bounding box x1 y1 x2 y2
 *	 11-...		n bands
 *
 *	 Regions are divided into bands that are uniform in the
 *	 y-direction. Each band consists of pairs of on/off x-coords and is
 *	 written as
 *		m y0 y1 x1 x2 x3 ... xm m
 *	 into successive rdParm[]s.
 *
 *	 This is probably just a dump of the internal RGNOBJ?
 *
 *	 HDMD - 18/12/97
 *
 */

static BOOL MF_Play_MetaCreateRegion( METARECORD *mr, HRGN hrgn )
{
    WORD band, pair;
    WORD *start, *end;
    INT16 y0, y1;
    HRGN hrgn2 = CreateRectRgn( 0, 0, 0, 0 );

    for(band  = 0, start = &(mr->rdParm[11]); band < mr->rdParm[5];
 					        band++, start = end + 1) {
        if(*start / 2 != (*start + 1) / 2) {
 	    WARN(metafile, "Delimiter not even.\n");
	    DeleteObject( hrgn2 );
 	    return FALSE;
        }

	end = start + *start + 3;
	if(end > (WORD *)mr + mr->rdSize) {
	    WARN(metafile, "End points outside record.\n");
	    DeleteObject( hrgn2 );
	    return FALSE;
        }

	if(*start != *end) {
	    WARN(metafile, "Mismatched delimiters.\n");
	    DeleteObject( hrgn2 );
	    return FALSE;
	}

	y0 = *(INT16 *)(start + 1);
	y1 = *(INT16 *)(start + 2);
	for(pair = 0; pair < *start / 2; pair++) {
	    SetRectRgn( hrgn2, *(INT16 *)(start + 3 + 2*pair), y0,
				 *(INT16 *)(start + 4 + 2*pair), y1 );
	    CombineRgn(hrgn, hrgn, hrgn2, RGN_OR);
        }
    }
    DeleteObject( hrgn2 );
    return TRUE;
 }
 

/*  LocalWords:  capatibility
 */
