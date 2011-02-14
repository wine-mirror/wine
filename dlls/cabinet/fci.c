/*
 * File Compression Interface
 *
 * Copyright 2002 Patrik Stridvall
 * Copyright 2005 Gerold Jens Wucherpfennig
 * Copyright 2011 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*

There is still some work to be done:

- no real compression yet
- unknown behaviour if files>=2GB or cabinet >=4GB
- check if the maximum size for a cabinet is too small to store any data
- call pfnfcignc on exactly the same position as MS FCIAddFile in every case
- probably check err

*/



#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"
#include "fci.h"
#include "cabinet.h"
#include "wine/list.h"
#include "wine/debug.h"


#ifdef WORDS_BIGENDIAN
#define fci_endian_ulong(x) RtlUlongByteSwap(x)
#define fci_endian_uword(x) RtlUshortByteSwap(x)
#else
#define fci_endian_ulong(x) (x)
#define fci_endian_uword(x) (x)
#endif


typedef struct {
  cab_UBYTE signature[4]; /* !CAB for unfinished cabinets else MSCF */
  cab_ULONG reserved1;
  cab_ULONG cbCabinet;    /*  size of the cabinet file in bytes*/
  cab_ULONG reserved2;
  cab_ULONG coffFiles;    /* offset to first CFFILE section */
  cab_ULONG reserved3;
  cab_UBYTE versionMinor; /* 3 */
  cab_UBYTE versionMajor; /* 1 */
  cab_UWORD cFolders;     /* number of CFFOLDER entries in the cabinet*/
  cab_UWORD cFiles;       /* number of CFFILE entries in the cabinet*/
  cab_UWORD flags;        /* 1=prev cab, 2=next cabinet, 4=reserved sections*/
  cab_UWORD setID;        /* identification number of all cabinets in a set*/
  cab_UWORD iCabinet;     /* number of the cabinet in a set */
  /* additional area if "flags" were set*/
} CFHEADER; /* minimum 36 bytes */

typedef struct {
  cab_ULONG coffCabStart; /* offset to the folder's first CFDATA section */
  cab_UWORD cCFData;      /* number of this folder's CFDATA sections */
  cab_UWORD typeCompress; /* compression type of data in CFDATA section*/
  /* additional area if reserve flag was set */
} CFFOLDER; /* minimum 8 bytes */

typedef struct {
  cab_ULONG cbFile;          /* size of the uncompressed file in bytes */
  cab_ULONG uoffFolderStart; /* offset of the uncompressed file in the folder */
  cab_UWORD iFolder;         /* number of folder in the cabinet 0=first  */
                             /* for special values see below this structure*/
  cab_UWORD date;            /* last modification date*/
  cab_UWORD time;            /* last modification time*/
  cab_UWORD attribs;         /* DOS fat attributes and UTF indicator */
  /* ... and a C string with the name of the file */
} CFFILE; /* 16 bytes + name of file */


typedef struct {
  cab_ULONG csum;          /* checksum of this entry*/
  cab_UWORD cbData;        /* number of compressed bytes  */
  cab_UWORD cbUncomp;      /* number of bytes when data is uncompressed */
  /* optional reserved area */
  /* compressed data */
} CFDATA;

struct folder
{
    struct list entry;
    struct list files_list;
    cab_ULONG   data_start;
    cab_UWORD   data_count;
    cab_UWORD   compression;
};

struct file
{
    struct list entry;
    cab_ULONG   size;    /* uncompressed size */
    cab_ULONG   offset;  /* offset in folder */
    cab_UWORD   folder;  /* index of folder */
    cab_UWORD   date;
    cab_UWORD   time;
    cab_UWORD   attribs;
    char        name[1];
};

typedef struct
{
  unsigned int       magic;
  PERF perf;
  PFNFCIFILEPLACED   fileplaced;
  PFNFCIALLOC        alloc;
  PFNFCIFREE         free;
  PFNFCIOPEN         open;
  PFNFCIREAD         read;
  PFNFCIWRITE        write;
  PFNFCICLOSE        close;
  PFNFCISEEK         seek;
  PFNFCIDELETE       delete;
  PFNFCIGETTEMPFILE  gettemp;
  PCCAB              pccab;
  BOOL               fPrevCab;
  BOOL               fNextCab;
  BOOL               fSplitFolder;
  cab_ULONG          statusFolderCopied;
  cab_ULONG          statusFolderTotal;
  BOOL               fGetNextCabInVain;
  void               *pv;
  char szPrevCab[CB_MAX_CABINET_NAME];    /* previous cabinet name */
  char szPrevDisk[CB_MAX_DISK_NAME];      /* disk name of previous cabinet */
  CCAB               oldCCAB;
  char*              data_in;  /* uncompressed data blocks */
  cab_UWORD          cdata_in;
  char*              data_out; /* compressed data blocks */
  ULONG              cCompressedBytesInFolder;
  cab_UWORD          cFolders;
  cab_UWORD          cFiles;
  cab_ULONG          cDataBlocks;
  cab_ULONG          cbFileRemainer; /* uncompressed, yet to be written data */
               /* of spanned file of a spanning folder of a spanning cabinet */
  char               szFileNameCFDATA1[CB_MAX_FILENAME];
  int                handleCFDATA1;
  char               szFileNameCFDATA2[CB_MAX_FILENAME];
  int                handleCFDATA2;
  cab_ULONG          sizeFileCFDATA1;
  cab_ULONG          sizeFileCFDATA2;
  BOOL               fNewPrevious;
  cab_ULONG          estimatedCabinetSize;
  struct list        folders_list;
  struct list        files_list;
  cab_ULONG          folders_size;
  cab_ULONG          files_size;
  cab_ULONG          placed_files_size;
} FCI_Int;

#define FCI_INT_MAGIC 0xfcfcfc05

static void set_error( FCI_Int *fci, int oper, int err )
{
    fci->perf->erfOper = oper;
    fci->perf->erfType = err;
    fci->perf->fError = TRUE;
    if (err) SetLastError( err );
}

static FCI_Int *get_fci_ptr( HFCI hfci )
{
    FCI_Int *fci= (FCI_Int *)hfci;

    if (!fci || !fci->magic == FCI_INT_MAGIC)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return NULL;
    }
    return fci;
}

static cab_ULONG get_folder_size( FCI_Int *fci )
{
    if (fci->fNextCab || fci->fGetNextCabInVain)
        return sizeof(CFFOLDER) + fci->oldCCAB.cbReserveCFFolder;
    else
        return sizeof(CFFOLDER) + fci->pccab->cbReserveCFFolder;
}

static struct file *add_file( FCI_Int *fci, const char *filename )
{
    unsigned int size = FIELD_OFFSET( struct file, name[strlen(filename) + 1] );
    struct file *file = fci->alloc( size );

    if (!file)
    {
        set_error( fci, FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }
    file->size    = 0;
    file->offset  = fci->cDataBlocks * CAB_BLOCKMAX + fci->cdata_in;
    file->folder  = fci->cFolders;
    file->date    = 0;
    file->time    = 0;
    file->attribs = 0;
    strcpy( file->name, filename );
    list_add_tail( &fci->files_list, &file->entry );
    return file;
}

static struct file *copy_file( FCI_Int *fci, const struct file *orig )
{
    unsigned int size = FIELD_OFFSET( struct file, name[strlen(orig->name) + 1] );
    struct file *file = fci->alloc( size );

    if (!file)
    {
        set_error( fci, FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }
    memcpy( file, orig, size );
    return file;
}

static void free_file( FCI_Int *fci, struct file *file )
{
    list_remove( &file->entry );
    fci->free( file );
}

static struct folder *add_folder( FCI_Int *fci )
{
    struct folder *folder = fci->alloc( sizeof(*folder) );

    if (!folder)
    {
        set_error( fci, FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }
    folder->data_start  = fci->sizeFileCFDATA2;
    folder->compression = tcompTYPE_NONE;  /* FIXME */
    list_init( &folder->files_list );
    list_add_tail( &fci->folders_list, &folder->entry );
    fci->folders_size += get_folder_size( fci );
    fci->cFolders++;
    return folder;
}

static void free_folder( FCI_Int *fci, struct folder *folder )
{
    struct file *file, *next;

    LIST_FOR_EACH_ENTRY_SAFE( file, next, &folder->files_list, struct file, entry ) free_file( fci, file );
    list_remove( &folder->entry );
    fci->free( folder );
}

/* write all folders to disk and remove them from the list */
static BOOL write_folders( FCI_Int *fci, INT_PTR handle, cab_ULONG header_size, PFNFCISTATUS status_callback )
{
    struct folder *folder, *next;
    struct file *file;
    int err;
    BOOL ret = TRUE;
    CFFILE *cffile;
    CFFOLDER *cffolder;
    cab_ULONG file_size, folder_size = get_folder_size( fci );

    if (!(cffolder = fci->alloc( folder_size )))
    {
        set_error( fci, FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    memset( cffolder, 0, folder_size );

    if (!(cffile = fci->alloc( sizeof(CFFILE) + CB_MAX_FILENAME )))
    {
        fci->free( cffolder );
        set_error( fci, FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    /* write the folders */
    LIST_FOR_EACH_ENTRY( folder, &fci->folders_list, struct folder, entry )
    {
        cffolder->coffCabStart = fci_endian_ulong( folder->data_start + header_size );
        cffolder->cCFData      = fci_endian_uword( folder->data_count );
        cffolder->typeCompress = fci_endian_uword( folder->compression );
        if (fci->write( handle, cffolder, folder_size, &err, fci->pv ) != folder_size)
        {
            set_error( fci, FCIERR_CAB_FILE, err );
            ret = FALSE;
            break;
        }
    }

    /* write all the folders files */
    LIST_FOR_EACH_ENTRY( folder, &fci->folders_list, struct folder, entry )
    {
        LIST_FOR_EACH_ENTRY( file, &folder->files_list, struct file, entry )
        {
            cffile->cbFile          = fci_endian_ulong( file->size );
            cffile->uoffFolderStart = fci_endian_ulong( file->offset );
            cffile->iFolder         = fci_endian_uword( file->folder );
            cffile->date            = fci_endian_uword( file->date );
            cffile->time            = fci_endian_uword( file->time );
            cffile->attribs         = fci_endian_uword( file->attribs );
            lstrcpynA( (char *)(cffile + 1), file->name, CB_MAX_FILENAME );
            file_size = sizeof(CFFILE) + strlen( (char *)(cffile + 1) ) + 1;
            if (fci->write( handle, cffile, file_size, &err, fci->pv ) != file_size)
            {
                set_error( fci, FCIERR_CAB_FILE, err );
                ret = FALSE;
                break;
            }
            if (!fci->fSplitFolder)
            {
                fci->statusFolderCopied = 0;
                /* TODO TEST THIS further */
                fci->statusFolderTotal = fci->sizeFileCFDATA2 + fci->placed_files_size;
            }
            fci->statusFolderCopied += file_size;
            /* report status about copied size of folder */
            if (status_callback( statusFolder, fci->statusFolderCopied,
                                 fci->statusFolderTotal, fci->pv ) == -1)
            {
                set_error( fci, FCIERR_USER_ABORT, 0 );
                ret = FALSE;
                break;
            }
        }
    }

    /* cleanup */
    LIST_FOR_EACH_ENTRY_SAFE( folder, next, &fci->folders_list, struct folder, entry )
    {
        free_folder( fci, folder );
    }
    fci->free( cffolder );
    fci->free( cffile );
    return ret;
}

/* add all pending files to folder */
static BOOL add_files_to_folder( FCI_Int *fci, struct folder *folder, cab_ULONG payload )
{
    cab_ULONG sizeOfFiles = 0, sizeOfFilesPrev;
    cab_ULONG cbFileRemainer = 0;
    struct file *file, *next;

    LIST_FOR_EACH_ENTRY_SAFE( file, next, &fci->files_list, struct file, entry )
    {
        cab_ULONG size = sizeof(CFFILE) + strlen(file->name) + 1;

        /* fnfilfnfildest: placed file on cabinet */
        if (fci->fNextCab || fci->fGetNextCabInVain)
            fci->fileplaced( &fci->oldCCAB, file->name, file->size,
                             (file->folder == cffileCONTINUED_FROM_PREV), fci->pv );
        else
            fci->fileplaced( fci->pccab, file->name, file->size,
                             (file->folder == cffileCONTINUED_FROM_PREV), fci->pv );

        sizeOfFilesPrev = sizeOfFiles;
        /* set complete size of all processed files */
        if (file->folder == cffileCONTINUED_FROM_PREV && fci->cbFileRemainer != 0)
        {
            sizeOfFiles += fci->cbFileRemainer;
            fci->cbFileRemainer = 0;
        }
        else sizeOfFiles += file->size;

        /* check if spanned file fits into this cabinet folder */
        if (sizeOfFiles > payload)
        {
            if (file->folder == cffileCONTINUED_FROM_PREV)
                file->folder = cffileCONTINUED_PREV_AND_NEXT;
            else
                file->folder = cffileCONTINUED_TO_NEXT;
        }

        list_remove( &file->entry );
        list_add_tail( &folder->files_list, &file->entry );
        fci->placed_files_size += size;
        fci->cFiles++;

        /* This is only true for files which will be written into the */
        /* next cabinet of the spanning folder */
        if (sizeOfFiles > payload)
        {
            /* add a copy back onto the list */
            if (!(file = copy_file( fci, file ))) return FALSE;
            list_add_before( &next->entry, &file->entry );

            /* Files which data will be partially written into the current cabinet */
            if (file->folder == cffileCONTINUED_PREV_AND_NEXT || file->folder == cffileCONTINUED_TO_NEXT)
            {
                if (sizeOfFilesPrev <= payload)
                {
                    /* The size of the uncompressed, data of a spanning file in a */
                    /* spanning data */
                    cbFileRemainer = sizeOfFiles - payload;
                }
                file->folder = cffileCONTINUED_FROM_PREV;
            }
            else file->folder = 0;
        }
        else
        {
            fci->files_size -= size;
        }
    }
    fci->cbFileRemainer = cbFileRemainer;
    return TRUE;
}

/***********************************************************************
 *		FCICreate (CABINET.10)
 *
 * FCICreate is provided with several callbacks and
 * returns a handle which can be used to create cabinet files.
 *
 * PARAMS
 *   perf       [IO]  A pointer to an ERF structure.  When FCICreate
 *                    returns an error condition, error information may
 *                    be found here as well as from GetLastError.
 *   pfnfiledest [I]  A pointer to a function which is called when a file
 *                    is placed. Only useful for subsequent cabinet files.
 *   pfnalloc    [I]  A pointer to a function which allocates ram.  Uses
 *                    the same interface as malloc.
 *   pfnfree     [I]  A pointer to a function which frees ram.  Uses the
 *                    same interface as free.
 *   pfnopen     [I]  A pointer to a function which opens a file.  Uses
 *                    the same interface as _open.
 *   pfnread     [I]  A pointer to a function which reads from a file into
 *                    a caller-provided buffer.  Uses the same interface
 *                    as _read.
 *   pfnwrite    [I]  A pointer to a function which writes to a file from
 *                    a caller-provided buffer.  Uses the same interface
 *                    as _write.
 *   pfnclose    [I]  A pointer to a function which closes a file handle.
 *                    Uses the same interface as _close.
 *   pfnseek     [I]  A pointer to a function which seeks in a file.
 *                    Uses the same interface as _lseek.
 *   pfndelete   [I]  A pointer to a function which deletes a file.
 *   pfnfcigtf   [I]  A pointer to a function which gets the name of a
 *                    temporary file.
 *   pccab       [I]  A pointer to an initialized CCAB structure.
 *   pv          [I]  A pointer to an application-defined notification
 *                    function which will be passed to other FCI functions
 *                    as a parameter.
 *
 * RETURNS
 *   On success, returns an FCI handle of type HFCI.
 *   On failure, the NULL file handle is returned. Error
 *   info can be retrieved from perf.
 *
 * INCLUDES
 *   fci.h
 *
 */
HFCI __cdecl FCICreate(
	PERF perf,
	PFNFCIFILEPLACED   pfnfiledest,
	PFNFCIALLOC        pfnalloc,
	PFNFCIFREE         pfnfree,
	PFNFCIOPEN         pfnopen,
	PFNFCIREAD         pfnread,
	PFNFCIWRITE        pfnwrite,
	PFNFCICLOSE        pfnclose,
	PFNFCISEEK         pfnseek,
	PFNFCIDELETE       pfndelete,
	PFNFCIGETTEMPFILE  pfnfcigtf,
	PCCAB              pccab,
	void *pv)
{
  int err;
  FCI_Int *p_fci_internal;

  if (!perf) {
    SetLastError(ERROR_BAD_ARGUMENTS);
    return NULL;
  }
  if ((!pfnalloc) || (!pfnfree) || (!pfnopen) || (!pfnread) ||
      (!pfnwrite) || (!pfnclose) || (!pfnseek) || (!pfndelete) ||
      (!pfnfcigtf) || (!pccab)) {
    perf->erfOper = FCIERR_NONE;
    perf->erfType = ERROR_BAD_ARGUMENTS;
    perf->fError = TRUE;

    SetLastError(ERROR_BAD_ARGUMENTS);
    return NULL;
  }

  if (!((p_fci_internal = pfnalloc(sizeof(FCI_Int))))) {
    perf->erfOper = FCIERR_ALLOC_FAIL;
    perf->erfType = ERROR_NOT_ENOUGH_MEMORY;
    perf->fError = TRUE;

    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return NULL;
  }

  p_fci_internal->magic = FCI_INT_MAGIC;
  p_fci_internal->perf = perf;
  p_fci_internal->fileplaced = pfnfiledest;
  p_fci_internal->alloc = pfnalloc;
  p_fci_internal->free = pfnfree;
  p_fci_internal->open = pfnopen;
  p_fci_internal->read = pfnread;
  p_fci_internal->write = pfnwrite;
  p_fci_internal->close = pfnclose;
  p_fci_internal->seek = pfnseek;
  p_fci_internal->delete = pfndelete;
  p_fci_internal->gettemp = pfnfcigtf;
  p_fci_internal->pccab = pccab;
  p_fci_internal->fPrevCab = FALSE;
  p_fci_internal->fNextCab = FALSE;
  p_fci_internal->fSplitFolder = FALSE;
  p_fci_internal->fGetNextCabInVain = FALSE;
  p_fci_internal->pv = pv;
  p_fci_internal->data_in  = NULL;
  p_fci_internal->cdata_in = 0;
  p_fci_internal->data_out = NULL;
  p_fci_internal->cCompressedBytesInFolder = 0;
  p_fci_internal->cFolders = 0;
  p_fci_internal->cFiles = 0;
  p_fci_internal->cDataBlocks = 0;
  p_fci_internal->sizeFileCFDATA1 = 0;
  p_fci_internal->sizeFileCFDATA2 = 0;
  p_fci_internal->fNewPrevious = FALSE;
  p_fci_internal->estimatedCabinetSize = 0;
  p_fci_internal->statusFolderTotal = 0;
  p_fci_internal->folders_size = 0;
  p_fci_internal->files_size = 0;
  p_fci_internal->placed_files_size = 0;

  list_init( &p_fci_internal->folders_list );
  list_init( &p_fci_internal->files_list );

  memset(&p_fci_internal->oldCCAB, 0, sizeof(CCAB));
  memcpy(p_fci_internal->szPrevCab, pccab->szCab, CB_MAX_CABINET_NAME);
  memcpy(p_fci_internal->szPrevDisk, pccab->szDisk, CB_MAX_DISK_NAME);

  /* CFDATA */
  if( !p_fci_internal->gettemp(p_fci_internal->szFileNameCFDATA1,
                               CB_MAX_FILENAME, p_fci_internal->pv)) {
    set_error( p_fci_internal, FCIERR_NONE, ERROR_FUNCTION_FAILED );
    return FALSE;
  }
  /* safety */
  if ( strlen(p_fci_internal->szFileNameCFDATA1) >= CB_MAX_FILENAME ) {
    set_error( p_fci_internal, FCIERR_NONE, ERROR_INVALID_DATA );
    return FALSE;
  }

  p_fci_internal->handleCFDATA1 = p_fci_internal->open( p_fci_internal->szFileNameCFDATA1,
                                                        _O_RDWR | _O_CREAT | _O_EXCL | _O_BINARY,
                                                        _S_IREAD | _S_IWRITE, &err, pv);
  if(p_fci_internal->handleCFDATA1==0){
    set_error( p_fci_internal, FCIERR_TEMP_FILE, ERROR_OPEN_FAILED );
    return FALSE;
  }
  /* TODO error checking of err */

  /* CFDATA with checksum and ready to be copied into cabinet */
  if( !p_fci_internal->gettemp(p_fci_internal->szFileNameCFDATA2,
                               CB_MAX_FILENAME, p_fci_internal->pv)) {
    set_error( p_fci_internal, FCIERR_NONE, ERROR_FUNCTION_FAILED );
    return FALSE;
  }
  /* safety */
  if ( strlen(p_fci_internal->szFileNameCFDATA2) >= CB_MAX_FILENAME ) {
    set_error( p_fci_internal, FCIERR_NONE, ERROR_INVALID_DATA );
    return FALSE;
  }
  p_fci_internal->handleCFDATA2 = p_fci_internal->open( p_fci_internal->szFileNameCFDATA2,
                                                        _O_RDWR | _O_CREAT | _O_EXCL | _O_BINARY,
                                                        _S_IREAD | _S_IWRITE, &err, pv);
  if(p_fci_internal->handleCFDATA2==0){
    set_error( p_fci_internal, FCIERR_TEMP_FILE, ERROR_OPEN_FAILED );
    return FALSE;
  }
  /* TODO error checking of err */

  /* TODO close and delete new files when return FALSE */

  return (HFCI)p_fci_internal;
}






static BOOL fci_flush_data_block (FCI_Int *p_fci_internal, int* err,
    PFNFCISTATUS pfnfcis) {

  /* attention no checks if there is data available!!! */
  CFDATA data;
  CFDATA* cfdata=&data;
  char* reserved;
  UINT cbReserveCFData=p_fci_internal->pccab->cbReserveCFData;
  UINT i;

  /* TODO compress the data of p_fci_internal->data_in */
  /* and write it to p_fci_internal->data_out */
  memcpy(p_fci_internal->data_out, p_fci_internal->data_in,
    p_fci_internal->cdata_in /* number of bytes to copy */);

  cfdata->csum=0; /* checksum has to be set later */
  /* TODO set realsize of compressed data */
  cfdata->cbData   = p_fci_internal->cdata_in;
  cfdata->cbUncomp = p_fci_internal->cdata_in;

  /* write cfdata to p_fci_internal->handleCFDATA1 */
  if( p_fci_internal->write( p_fci_internal->handleCFDATA1, /* file handle */
      cfdata, sizeof(*cfdata), err, p_fci_internal->pv)
      != sizeof(*cfdata) ) {
    set_error( p_fci_internal, FCIERR_TEMP_FILE, ERROR_WRITE_FAULT );
    return FALSE;
  }
  /* TODO error handling of err */

  p_fci_internal->sizeFileCFDATA1 += sizeof(*cfdata);

  /* add optional reserved area */

  /* This allocation and freeing at each CFData block is a bit */
  /* inefficient, but it's harder to forget about freeing the buffer :-). */
  /* Reserved areas are used seldom besides that... */
  if (cbReserveCFData!=0) {
    if(!(reserved = p_fci_internal->alloc( cbReserveCFData))) {
      set_error( p_fci_internal, FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
      return FALSE;
    }
    for(i=0;i<cbReserveCFData;) {
      reserved[i++]='\0';
    }
    if( p_fci_internal->write( p_fci_internal->handleCFDATA1, /* file handle */
        reserved, /* memory buffer */
        cbReserveCFData, /* number of bytes to copy */
        err, p_fci_internal->pv) != cbReserveCFData ) {
      p_fci_internal->free(reserved);
      set_error( p_fci_internal, FCIERR_TEMP_FILE, ERROR_WRITE_FAULT );
      return FALSE;
    }
    /* TODO error handling of err PFCI_FREE(hfci, reserved)*/

    p_fci_internal->sizeFileCFDATA1 += cbReserveCFData;
    p_fci_internal->free( reserved);
  }

  /* write p_fci_internal->data_out to p_fci_internal->handleCFDATA1 */
  if( p_fci_internal->write( p_fci_internal->handleCFDATA1, /* file handle */
      p_fci_internal->data_out, /* memory buffer */
      cfdata->cbData, /* number of bytes to copy */
      err, p_fci_internal->pv) != cfdata->cbData) {
    set_error( p_fci_internal, FCIERR_TEMP_FILE, ERROR_WRITE_FAULT );
    return FALSE;
  }
  /* TODO error handling of err */

  p_fci_internal->sizeFileCFDATA1 += cfdata->cbData;

  /* reset the offset */
  p_fci_internal->cdata_in = 0;
  p_fci_internal->cCompressedBytesInFolder += cfdata->cbData;

  /* report status with pfnfcis about uncompressed and compressed file data */
  if( (*pfnfcis)(statusFile, cfdata->cbData, cfdata->cbUncomp,
      p_fci_internal->pv) == -1) {
    set_error( p_fci_internal, FCIERR_USER_ABORT, 0 );
    return FALSE;
  }

  ++(p_fci_internal->cDataBlocks);

  return TRUE;
} /* end of fci_flush_data_block */





static cab_ULONG fci_get_checksum(const void *pv, UINT cb, CHECKSUM seed)
{
  cab_ULONG     csum;
  cab_ULONG     ul;
  int           cUlong;
  const BYTE    *pb;

  csum = seed;
  cUlong = cb / 4;
  pb = pv;

  while (cUlong-- > 0) {
    ul = *pb++;
    ul |= (((cab_ULONG)(*pb++)) <<  8);
    ul |= (((cab_ULONG)(*pb++)) << 16);
    ul |= (((cab_ULONG)(*pb++)) << 24);

    csum ^= ul;
  }

  ul = 0;
  switch (cb % 4) {
    case 3:
      ul |= (((ULONG)(*pb++)) << 16);
    case 2:
      ul |= (((ULONG)(*pb++)) <<  8);
    case 1:
      ul |= *pb;
    default:
      break;
  }
  csum ^= ul;

  return csum;
} /* end of fci_get_checksum */



static BOOL fci_flushfolder_copy_cfdata(FCI_Int *p_fci_internal, char* buffer, UINT cbReserveCFData,
  PFNFCISTATUS pfnfcis, int* err, int handleCFDATA1new,
  cab_ULONG* psizeFileCFDATA1new, cab_ULONG* payload)
{
  cab_ULONG read_result;
  CFDATA* pcfdata=(CFDATA*)buffer;
  BOOL split_block=FALSE;
  cab_UWORD savedUncomp=0;

  *payload=0;

  /* while not all CFDATAs have been copied do */
  while(!FALSE) {
    if( p_fci_internal->fNextCab ) {
      if( split_block ) {
        /* internal error should never happen */
        set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
        return FALSE;
      }
    }
    /* REUSE the variable read_result */
    if ( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
      read_result=4;
    } else {
      read_result=0;
    }
    if (p_fci_internal->fPrevCab) {
      read_result+=strlen(p_fci_internal->szPrevCab)+1 +
        strlen(p_fci_internal->szPrevDisk)+1;
    }
    /* No more CFDATA fits into the cabinet under construction */
    /* So don't try to store more data into it */
    if( p_fci_internal->fNextCab &&
        (p_fci_internal->oldCCAB.cb <= sizeof(CFDATA) + cbReserveCFData +
        p_fci_internal->files_size + p_fci_internal->sizeFileCFDATA2 +
        p_fci_internal->placed_files_size + p_fci_internal->folders_size +
        sizeof(CFHEADER) +
        read_result +
        p_fci_internal->oldCCAB.cbReserveCFHeader +
        sizeof(CFFOLDER) +
        p_fci_internal->oldCCAB.cbReserveCFFolder +
        strlen(p_fci_internal->pccab->szCab)+1 +
        strlen(p_fci_internal->pccab->szDisk)+1
    )) {
      /* This may never be run for the first time the while loop is entered.
      Pray that the code that calls fci_flushfolder_copy_cfdata handles this.*/
      split_block=TRUE;  /* In this case split_block is abused to store */
      /* the complete data block into the next cabinet and not into the */
      /* current one. Originally split_block is the indicator that a */
      /* data block has been split across different cabinets. */
    } else {

      /* read CFDATA from p_fci_internal->handleCFDATA1 to cfdata*/
      read_result= p_fci_internal->read( p_fci_internal->handleCFDATA1,/*file handle*/
          buffer, /* memory buffer */
          sizeof(CFDATA)+cbReserveCFData, /* number of bytes to copy */
          err, p_fci_internal->pv);
      if (read_result!=sizeof(CFDATA)+cbReserveCFData) {
        if (read_result==0) break; /* ALL DATA has been copied */
        /* read error */
        set_error( p_fci_internal, FCIERR_NONE, ERROR_READ_FAULT );
        return FALSE;
      }
      /* TODO error handling of err */

      /* REUSE buffer p_fci_internal->data_out !!! */
      /* read data from p_fci_internal->handleCFDATA1 to */
      /*      p_fci_internal->data_out */
      if( p_fci_internal->read( p_fci_internal->handleCFDATA1 /* file handle */,
          p_fci_internal->data_out /* memory buffer */,
          pcfdata->cbData /* number of bytes to copy */,
          err, p_fci_internal->pv) != pcfdata->cbData ) {
        /* read error */
        set_error( p_fci_internal, FCIERR_NONE, ERROR_READ_FAULT );
        return FALSE;
      }
      /* TODO error handling of err */

      /* if cabinet size is too large */

      /* REUSE the variable read_result */
      if ( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
          p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
          p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
        read_result=4;
      } else {
        read_result=0;
      }
      if (p_fci_internal->fPrevCab) {
        read_result+=strlen(p_fci_internal->szPrevCab)+1 +
          strlen(p_fci_internal->szPrevDisk)+1;
      }

      /* Is cabinet with new CFDATA too large? Then data block has to be split */
      if( p_fci_internal->fNextCab &&
          (p_fci_internal->oldCCAB.cb < sizeof(CFDATA) + cbReserveCFData +
          pcfdata->cbData +
          p_fci_internal->files_size + p_fci_internal->sizeFileCFDATA2 +
          p_fci_internal->placed_files_size + p_fci_internal->folders_size +
          sizeof(CFHEADER) +
          read_result +
          p_fci_internal->oldCCAB.cbReserveCFHeader +
          sizeof(CFFOLDER) + /* size of new CFFolder entry */
          p_fci_internal->oldCCAB.cbReserveCFFolder +
          strlen(p_fci_internal->pccab->szCab)+1 + /* name of next cabinet */
          strlen(p_fci_internal->pccab->szDisk)+1  /* name of next disk */
      )) {
        /* REUSE read_result to save the size of the compressed data */
        read_result=pcfdata->cbData;
        /* Modify the size of the compressed data to store only a part of the */
        /* data block into the current cabinet. This is done to prevent */
        /* that the maximum cabinet size will be exceeded. The remainder */
        /* will be stored into the next following cabinet. */

        /* The cabinet will be of size "p_fci_internal->oldCCAB.cb". */
        /* Substract everything except the size of the block of data */
        /* to get it's actual size */
        pcfdata->cbData = p_fci_internal->oldCCAB.cb - (
          sizeof(CFDATA) + cbReserveCFData +
          p_fci_internal->files_size + p_fci_internal->sizeFileCFDATA2 +
          p_fci_internal->placed_files_size + p_fci_internal->folders_size +
          sizeof(CFHEADER) +
          p_fci_internal->oldCCAB.cbReserveCFHeader +
          sizeof(CFFOLDER) + /* set size of new CFFolder entry */
          p_fci_internal->oldCCAB.cbReserveCFFolder );
        /* substract the size of special header fields */
        if ( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
            p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
            p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
          pcfdata->cbData-=4;
        }
        if (p_fci_internal->fPrevCab) {
          pcfdata->cbData-=strlen(p_fci_internal->szPrevCab)+1 +
            strlen(p_fci_internal->szPrevDisk)+1;
        }
        pcfdata->cbData-=strlen(p_fci_internal->pccab->szCab)+1 +
          strlen(p_fci_internal->pccab->szDisk)+1;

        savedUncomp = pcfdata->cbUncomp;
        pcfdata->cbUncomp = 0; /* on split blocks of data this is zero */

        /* if split_block==TRUE then the above while loop won't */
        /* be executed again */
        split_block=TRUE; /* split_block is the indicator that */
                          /* a data block has been split across */
                          /* different cabinets.*/
      }

      /* This should never happen !!! */
      if (pcfdata->cbData==0) {
        /* set error */
        set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
        return FALSE;
      }

      /* set little endian */
      pcfdata->cbData=fci_endian_uword(pcfdata->cbData);
      pcfdata->cbUncomp=fci_endian_uword(pcfdata->cbUncomp);

      /* get checksum and write to cfdata.csum */
      pcfdata->csum = fci_get_checksum( &(pcfdata->cbData),
        sizeof(CFDATA)+cbReserveCFData -
        sizeof(pcfdata->csum), fci_get_checksum( p_fci_internal->data_out, /*buffer*/
        pcfdata->cbData, 0 ) );

      /* set little endian */
      pcfdata->csum=fci_endian_ulong(pcfdata->csum);

      /* write cfdata with checksum to p_fci_internal->handleCFDATA2 */
      if( p_fci_internal->write( p_fci_internal->handleCFDATA2, /* file handle */
          buffer, /* memory buffer */
          sizeof(CFDATA)+cbReserveCFData, /* number of bytes to copy */
          err, p_fci_internal->pv) != sizeof(CFDATA)+cbReserveCFData ) {
         set_error( p_fci_internal, FCIERR_TEMP_FILE, ERROR_WRITE_FAULT );
         return FALSE;
      }
      /* TODO error handling of err */

      p_fci_internal->sizeFileCFDATA2 += sizeof(CFDATA)+cbReserveCFData;

      /* reset little endian */
      pcfdata->cbData=fci_endian_uword(pcfdata->cbData);
      pcfdata->cbUncomp=fci_endian_uword(pcfdata->cbUncomp);
      pcfdata->csum=fci_endian_ulong(pcfdata->csum);

      /* write compressed data into p_fci_internal->handleCFDATA2 */
      if( p_fci_internal->write( p_fci_internal->handleCFDATA2, /* file handle */
          p_fci_internal->data_out, /* memory buffer */
          pcfdata->cbData, /* number of bytes to copy */
          err, p_fci_internal->pv) != pcfdata->cbData) {
        set_error( p_fci_internal, FCIERR_TEMP_FILE, ERROR_WRITE_FAULT );
        return FALSE;
      }
      /* TODO error handling of err */

      p_fci_internal->sizeFileCFDATA2 += pcfdata->cbData;
      ++(p_fci_internal->cDataBlocks);
      p_fci_internal->statusFolderCopied += pcfdata->cbData;
      (*payload)+=pcfdata->cbUncomp;
      /* if cabinet size too large and data has been split */
      /* write the remainder of the data block to the new CFDATA1 file */
      if( split_block  ) { /* This does not include the */
                                  /* abused one (just search for "abused" )*/
      /* copy all CFDATA structures from handleCFDATA1 to handleCFDATA1new */
        if (p_fci_internal->fNextCab==FALSE ) {
          /* internal error */
          set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
          return FALSE;
        }

        /* set cbData to the size of the remainder of the data block */
        pcfdata->cbData = read_result - pcfdata->cbData;
        /*recover former value of cfdata.cbData; read_result will be the offset*/
        read_result -= pcfdata->cbData;
        pcfdata->cbUncomp = savedUncomp;

        /* reset checksum, it will be computed later */
        pcfdata->csum=0;

        /* write cfdata WITHOUT checksum to handleCFDATA1new */
        if( p_fci_internal->write( handleCFDATA1new, /* file handle */
            buffer, /* memory buffer */
            sizeof(CFDATA)+cbReserveCFData, /* number of bytes to copy */
            err, p_fci_internal->pv) != sizeof(CFDATA)+cbReserveCFData ) {
          set_error( p_fci_internal, FCIERR_TEMP_FILE, ERROR_WRITE_FAULT );
          return FALSE;
        }
        /* TODO error handling of err don't forget PFCI_FREE(hfci, reserved) */

        *psizeFileCFDATA1new += sizeof(CFDATA)+cbReserveCFData;

        /* write compressed data into handleCFDATA1new */
        if( p_fci_internal->write( handleCFDATA1new, /* file handle */
            p_fci_internal->data_out + read_result, /* memory buffer + offset */
                                                /* to last part of split data */
            pcfdata->cbData, /* number of bytes to copy */
            err, p_fci_internal->pv) != pcfdata->cbData) {
          set_error( p_fci_internal, FCIERR_TEMP_FILE, ERROR_WRITE_FAULT );
          return FALSE;
        }
        /* TODO error handling of err */

        p_fci_internal->statusFolderCopied += pcfdata->cbData;

        *psizeFileCFDATA1new += pcfdata->cbData;
        /* the two blocks of the split data block have been written */
        /* don't reset split_data yet, because it is still needed see below */
      }

      /* report status with pfnfcis about copied size of folder */
      if( (*pfnfcis)(statusFolder,
          p_fci_internal->statusFolderCopied, /*cfdata.cbData(+previous ones)*/
          p_fci_internal->statusFolderTotal, /* total folder size */
          p_fci_internal->pv) == -1) {
        set_error( p_fci_internal, FCIERR_USER_ABORT, 0 );
        return FALSE;
      }
    }

    /* if cabinet size too large */
    /* write the remaining data blocks to the new CFDATA1 file */
    if ( split_block ) { /* This does include the */
                               /* abused one (just search for "abused" )*/
      if (p_fci_internal->fNextCab==FALSE ) {
        /* internal error */
        set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
        return FALSE;
      }
      /* copy all CFDATA structures from handleCFDATA1 to handleCFDATA1new */
      while(!FALSE) {
        /* read CFDATA from p_fci_internal->handleCFDATA1 to cfdata*/
        read_result= p_fci_internal->read( p_fci_internal->handleCFDATA1,/* handle */
            buffer, /* memory buffer */
            sizeof(CFDATA)+cbReserveCFData, /* number of bytes to copy */
            err, p_fci_internal->pv);
        if (read_result!=sizeof(CFDATA)+cbReserveCFData) {
          if (read_result==0) break; /* ALL DATA has been copied */
          /* read error */
          set_error( p_fci_internal,FCIERR_NONE, ERROR_READ_FAULT );
          return FALSE;
        }
        /* TODO error handling of err */

        /* REUSE buffer p_fci_internal->data_out !!! */
        /* read data from p_fci_internal->handleCFDATA1 to */
        /*      p_fci_internal->data_out */
        if( p_fci_internal->read( p_fci_internal->handleCFDATA1 /* file handle */,
            p_fci_internal->data_out /* memory buffer */,
            pcfdata->cbData /* number of bytes to copy */,
            err, p_fci_internal->pv) != pcfdata->cbData ) {
          /* read error */
          set_error( p_fci_internal, FCIERR_NONE, ERROR_READ_FAULT );
          return FALSE;
        }
        /* TODO error handling of err don't forget PFCI_FREE(hfci, reserved) */

        /* write cfdata with checksum to handleCFDATA1new */
        if( p_fci_internal->write( handleCFDATA1new, /* file handle */
            buffer, /* memory buffer */
            sizeof(CFDATA)+cbReserveCFData, /* number of bytes to copy */
            err, p_fci_internal->pv) != sizeof(CFDATA)+cbReserveCFData ) {
          set_error( p_fci_internal, FCIERR_TEMP_FILE, ERROR_WRITE_FAULT );
          return FALSE;
        }
        /* TODO error handling of err don't forget PFCI_FREE(hfci, reserved) */

        *psizeFileCFDATA1new += sizeof(CFDATA)+cbReserveCFData;

        /* write compressed data into handleCFDATA1new */
        if( p_fci_internal->write( handleCFDATA1new, /* file handle */
            p_fci_internal->data_out, /* memory buffer */
            pcfdata->cbData, /* number of bytes to copy */
            err, p_fci_internal->pv) != pcfdata->cbData) {
          set_error( p_fci_internal, FCIERR_TEMP_FILE, ERROR_WRITE_FAULT );
          return FALSE;
        }
        /* TODO error handling of err */

        *psizeFileCFDATA1new += pcfdata->cbData;
        p_fci_internal->statusFolderCopied += pcfdata->cbData;

        /* report status with pfnfcis about copied size of folder */
        if( (*pfnfcis)(statusFolder,
            p_fci_internal->statusFolderCopied,/*cfdata.cbData(+previous ones)*/
            p_fci_internal->statusFolderTotal, /* total folder size */
            p_fci_internal->pv) == -1) {
          set_error( p_fci_internal, FCIERR_USER_ABORT, 0 );
          return FALSE;
        }

      } /* end of WHILE */
      break; /* jump out of the next while loop */
    } /* end of if( split_data  ) */
  } /* end of WHILE */
  return TRUE;
} /* end of fci_flushfolder_copy_cfdata */





static BOOL fci_flush_folder( FCI_Int *p_fci_internal,
	BOOL                  fGetNextCab,
	PFNFCIGETNEXTCABINET  pfnfcignc,
	PFNFCISTATUS          pfnfcis)
{
  int err;
  int handleCFDATA1new;                         /* handle for new  temp file */
  char szFileNameCFDATA1new[CB_MAX_FILENAME];  /* name buffer for temp file */
  int handleCFFILE1new;                         /* handle for new  temp file */
  char szFileNameCFFILE1new[CB_MAX_FILENAME];  /* name buffer for temp file */
  UINT cbReserveCFData, cbReserveCFFolder;
  char* reserved;
  cab_ULONG sizeFileCFDATA1new=0;
  cab_ULONG payload;
  cab_ULONG read_result;
  struct folder *folder;

  if ((!pfnfcignc) || (!pfnfcis)) {
    set_error( p_fci_internal, FCIERR_NONE, ERROR_BAD_ARGUMENTS );
    return FALSE;
  }

  if( p_fci_internal->fGetNextCabInVain &&
      p_fci_internal->fNextCab ){
    /* internal error */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
    return FALSE;
  }

  /* If there was no FCIAddFile or FCIFlushFolder has already been called */
  /* this function will return TRUE */
  if( p_fci_internal->files_size == 0 ) {
    if ( p_fci_internal->sizeFileCFDATA1 != 0 ) {
      /* error handling */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
      return FALSE;
    }
    return TRUE;
  }

  if (p_fci_internal->data_in==NULL || p_fci_internal->data_out==NULL ) {
    /* error handling */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
    return FALSE;
  }

  /* FCIFlushFolder has already been called... */
  if (p_fci_internal->fSplitFolder && p_fci_internal->placed_files_size!=0) {
    return TRUE;
  }

  /* This can be set already, because it makes only a difference */
  /* when the current function exits with return FALSE */
  p_fci_internal->fSplitFolder=FALSE;


  if( p_fci_internal->fGetNextCabInVain ||
      p_fci_internal->fNextCab ){
    cbReserveCFData   = p_fci_internal->oldCCAB.cbReserveCFData;
    cbReserveCFFolder = p_fci_internal->oldCCAB.cbReserveCFFolder;
  } else {
    cbReserveCFData   = p_fci_internal->pccab->cbReserveCFData;
    cbReserveCFFolder = p_fci_internal->pccab->cbReserveCFFolder;
  }

  /* START of COPY */
  /* if there is data in p_fci_internal->data_in */
  if (p_fci_internal->cdata_in!=0) {

    if( !fci_flush_data_block(p_fci_internal, &err, pfnfcis) ) return FALSE;

  }
  /* reset to get the number of data blocks of this folder which are */
  /* actually in this cabinet ( at least partially ) */
  p_fci_internal->cDataBlocks=0;

  if ( p_fci_internal->fNextCab ||
       p_fci_internal->fGetNextCabInVain ) {
    read_result= p_fci_internal->oldCCAB.cbReserveCFHeader+
                 p_fci_internal->oldCCAB.cbReserveCFFolder;
    if ( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
      read_result+=4;
    }
  } else {
    read_result= p_fci_internal->pccab->cbReserveCFHeader+
                 p_fci_internal->pccab->cbReserveCFFolder;
    if ( p_fci_internal->pccab->cbReserveCFHeader != 0 ||
        p_fci_internal->pccab->cbReserveCFFolder != 0 ||
        p_fci_internal->pccab->cbReserveCFData   != 0 ) {
      read_result+=4;
    }
  }
  if (p_fci_internal->fPrevCab) {
    read_result+=strlen(p_fci_internal->szPrevCab)+1 +
      strlen(p_fci_internal->szPrevDisk)+1;
  }
  if (p_fci_internal->fNextCab) {
    read_result+=strlen(p_fci_internal->pccab->szCab)+1 +
      strlen(p_fci_internal->pccab->szDisk)+1;
  }

  p_fci_internal->statusFolderTotal = sizeof(CFHEADER)+read_result+
      sizeof(CFFOLDER) + p_fci_internal->placed_files_size+
      p_fci_internal->sizeFileCFDATA2 + p_fci_internal->files_size+
      p_fci_internal->sizeFileCFDATA1 + p_fci_internal->folders_size;
  p_fci_internal->statusFolderCopied = 0;

  /* report status with pfnfcis about copied size of folder */
  if( (*pfnfcis)(statusFolder, p_fci_internal->statusFolderCopied,
      p_fci_internal->statusFolderTotal, /* TODO total folder size */
      p_fci_internal->pv) == -1) {
    set_error( p_fci_internal, FCIERR_USER_ABORT, 0 );
    return FALSE;
  }

  /* get a new temp file */
  if(!p_fci_internal->gettemp(szFileNameCFDATA1new,CB_MAX_FILENAME, p_fci_internal->pv)) {
    set_error( p_fci_internal, FCIERR_NONE, ERROR_FUNCTION_FAILED );
    return FALSE;
  }
  /* safety */
  if ( strlen(szFileNameCFDATA1new) >= CB_MAX_FILENAME ) {
    set_error( p_fci_internal, FCIERR_NONE, ERROR_INVALID_DATA );
    return FALSE;
  }
  handleCFDATA1new = p_fci_internal->open(szFileNameCFDATA1new, _O_RDWR | _O_CREAT | _O_EXCL | _O_BINARY,
                                          _S_IREAD | _S_IWRITE, &err, p_fci_internal->pv);
  if(handleCFDATA1new==0){
    set_error( p_fci_internal, FCIERR_TEMP_FILE, ERROR_OPEN_FAILED );
    return FALSE;
  }
  /* TODO error handling of err */



  /* get a new temp file */
  if(!p_fci_internal->gettemp(szFileNameCFFILE1new,CB_MAX_FILENAME, p_fci_internal->pv)) {
    set_error( p_fci_internal, FCIERR_NONE, ERROR_FUNCTION_FAILED );
    p_fci_internal->close(handleCFDATA1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    return FALSE;
  }
  /* safety */
  if ( strlen(szFileNameCFFILE1new) >= CB_MAX_FILENAME ) {
    set_error( p_fci_internal, FCIERR_NONE, ERROR_INVALID_DATA );
    p_fci_internal->close(handleCFDATA1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    return FALSE;
  }
  handleCFFILE1new = p_fci_internal->open(szFileNameCFFILE1new, _O_RDWR | _O_CREAT | _O_EXCL | _O_BINARY,
                                          _S_IREAD | _S_IWRITE, &err, p_fci_internal->pv);
  if(handleCFFILE1new==0){
    set_error( p_fci_internal, FCIERR_TEMP_FILE, ERROR_OPEN_FAILED );
    return FALSE;
  }
  /* TODO error handling of err */

  /* USE the variable read_result */
  if ( p_fci_internal->fNextCab ||
       p_fci_internal->fGetNextCabInVain ) {
    read_result= p_fci_internal->oldCCAB.cbReserveCFHeader;
    if ( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
      read_result+=4;
    }
  } else {
    read_result= p_fci_internal->pccab->cbReserveCFHeader;
    if ( p_fci_internal->pccab->cbReserveCFHeader != 0 ||
        p_fci_internal->pccab->cbReserveCFFolder != 0 ||
        p_fci_internal->pccab->cbReserveCFData   != 0 ) {
      read_result+=4;
    }
  }
  if (p_fci_internal->fPrevCab) {
    read_result+=strlen(p_fci_internal->szPrevCab)+1 +
      strlen(p_fci_internal->szPrevDisk)+1;
  }
  read_result+= sizeof(CFHEADER) + p_fci_internal->sizeFileCFDATA2 +
    p_fci_internal->placed_files_size + p_fci_internal->folders_size;

  if(p_fci_internal->files_size!=0) {
    read_result+= sizeof(CFFOLDER)+p_fci_internal->pccab->cbReserveCFFolder;
  }

  /* Check if multiple cabinets have to be created. */

  /* Might be too much data for the maximum allowed cabinet size.*/
  /* When any further data will be added later, it might not */
  /* be possible to flush the cabinet, because there might */
  /* not be enough space to store the name of the following */
  /* cabinet and name of the corresponding disk. */
  /* So take care of this and get the name of the next cabinet */
  if( p_fci_internal->fGetNextCabInVain==FALSE &&
      p_fci_internal->fNextCab==FALSE &&
      (
        (
          p_fci_internal->pccab->cb < read_result +
          p_fci_internal->sizeFileCFDATA1 +
          p_fci_internal->files_size +
          CB_MAX_CABINET_NAME +   /* next cabinet name */
          CB_MAX_DISK_NAME        /* next disk name */
        ) || fGetNextCab
      )
  ) {
    /* save CCAB */
    p_fci_internal->oldCCAB = *p_fci_internal->pccab;
    /* increment cabinet index */
    ++(p_fci_internal->pccab->iCab);
    /* get name of next cabinet */
    p_fci_internal->estimatedCabinetSize=p_fci_internal->statusFolderTotal;
    if (!(*pfnfcignc)(p_fci_internal->pccab,
        p_fci_internal->estimatedCabinetSize, /* estimated size of cab */
        p_fci_internal->pv)) {
      /* error handling */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_FUNCTION_FAILED );
      p_fci_internal->close(handleCFDATA1new,&err,p_fci_internal->pv);
      /* TODO error handling of err */
      p_fci_internal->close(handleCFFILE1new,&err,p_fci_internal->pv);
      /* TODO error handling of err */
      return FALSE;
    }

    /* Skip a few lines of code. This is caught by the next if. */
    p_fci_internal->fGetNextCabInVain=TRUE;
  }

  /* too much data for cabinet */
  if( (p_fci_internal->fGetNextCabInVain ||
        p_fci_internal->fNextCab ) &&
      (
        (
          p_fci_internal->oldCCAB.cb < read_result +
          p_fci_internal->sizeFileCFDATA1 +
          p_fci_internal->files_size +
          strlen(p_fci_internal->pccab->szCab)+1 +   /* next cabinet name */
          strlen(p_fci_internal->pccab->szDisk)+1    /* next disk name */
        ) || fGetNextCab
      )
  ) {
    p_fci_internal->fGetNextCabInVain=FALSE;
    p_fci_internal->fNextCab=TRUE;

    /* return FALSE if there is not enough space left*/
    /* this should never happen */
    if (p_fci_internal->oldCCAB.cb <=
        p_fci_internal->files_size +
        read_result +
        strlen(p_fci_internal->pccab->szCab)+1 + /* next cabinet name */
        strlen(p_fci_internal->pccab->szDisk)+1  /* next disk name */
    ) {

      p_fci_internal->close(handleCFDATA1new,&err,p_fci_internal->pv);
      /* TODO error handling of err */
      p_fci_internal->delete(szFileNameCFDATA1new,&err,p_fci_internal->pv);
      /* TODO error handling of err */

      /* close and delete p_fci_internal->handleCFFILE1 */
      p_fci_internal->close(handleCFFILE1new,&err,p_fci_internal->pv);
      /* TODO error handling of err */
      p_fci_internal->delete(szFileNameCFFILE1new,&err,p_fci_internal->pv);
      /* TODO error handling of err */

      return FALSE;
    }

    /* the folder will be split across cabinets */
    p_fci_internal->fSplitFolder=TRUE;

  } else {
    /* this should never happen */
    if (p_fci_internal->fNextCab) {
      /* internal error */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
      return FALSE;
    }
  }

  /* set seek of p_fci_internal->handleCFDATA1 to 0 */
  if( p_fci_internal->seek(p_fci_internal->handleCFDATA1,0,SEEK_SET,&err,
    p_fci_internal->pv) !=0 ) {
    /* wrong return value */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_SEEK );
    p_fci_internal->close(handleCFDATA1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    p_fci_internal->close(handleCFFILE1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    return FALSE;
  }
  /* TODO error handling of err */

  if (!(folder = add_folder( p_fci_internal ))) return FALSE;

  if(!(reserved = p_fci_internal->alloc( cbReserveCFData+sizeof(CFDATA)))) {
    set_error( p_fci_internal, FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
    p_fci_internal->close(handleCFDATA1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    p_fci_internal->close(handleCFFILE1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    return FALSE;
  }

  if(!fci_flushfolder_copy_cfdata(p_fci_internal, reserved, cbReserveCFData, pfnfcis, &err,
      handleCFDATA1new, &sizeFileCFDATA1new, &payload
  )) {
    p_fci_internal->close(handleCFDATA1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    p_fci_internal->delete(szFileNameCFDATA1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    p_fci_internal->close(handleCFFILE1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    p_fci_internal->free(reserved);
    return FALSE;
  }

  p_fci_internal->free(reserved);

  folder->data_count = p_fci_internal->cDataBlocks;

  if (!add_files_to_folder( p_fci_internal, folder, payload ))
  {
    p_fci_internal->close(handleCFDATA1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    p_fci_internal->delete(szFileNameCFDATA1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    p_fci_internal->close(handleCFFILE1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    p_fci_internal->delete(szFileNameCFFILE1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    return FALSE;
  }

  /* close and delete p_fci_internal->handleCFDATA1 */
  p_fci_internal->close(p_fci_internal->handleCFDATA1,&err,p_fci_internal->pv);
  /* TODO error handling of err */
  p_fci_internal->delete(p_fci_internal->szFileNameCFDATA1,&err,p_fci_internal->pv);
  /* TODO error handling of err */

  /* put new CFDATA1 into hfci */
  memcpy(p_fci_internal->szFileNameCFDATA1,szFileNameCFDATA1new,
    CB_MAX_FILENAME);

  /* put CFDATA1 file handle */
  p_fci_internal->handleCFDATA1 = handleCFDATA1new;
  /* set file size */
  p_fci_internal->sizeFileCFDATA1 = sizeFileCFDATA1new;

  /* reset CFFolder specific information */
  p_fci_internal->cDataBlocks=0;
  p_fci_internal->cCompressedBytesInFolder=0;

  return TRUE;
}  /* end of fci_flush_folder */




static BOOL fci_flush_cabinet( FCI_Int *p_fci_internal,
	BOOL                  fGetNextCab,
	PFNFCIGETNEXTCABINET  pfnfcignc,
	PFNFCISTATUS          pfnfcis)
{
  int err;
  CFHEADER cfheader;
  struct {
    cab_UWORD  cbCFHeader;
    cab_UBYTE  cbCFFolder;
    cab_UBYTE  cbCFData;
  } cfreserved;
  cab_ULONG header_size;
  cab_ULONG read_result=0;
  int handleCABINET;                            /* file handle for cabinet   */
  char szFileNameCABINET[CB_MAX_CAB_PATH+CB_MAX_CABINET_NAME];/* name buffer */
  UINT cbReserveCFHeader, cbReserveCFFolder, i;
  char* reserved;
  BOOL returntrue=FALSE;

  /* TODO test if fci_flush_cabinet really aborts if there was no FCIAddFile */

  /* when FCIFlushCabinet was or FCIAddFile hasn't been called */
  if( p_fci_internal->files_size==0 && fGetNextCab ) {
    returntrue=TRUE;
  }

  if (!fci_flush_folder(p_fci_internal,fGetNextCab,pfnfcignc,pfnfcis)){
    /* TODO set error */
    return FALSE;
  }

  if(returntrue) return TRUE;

  if ( (p_fci_internal->fSplitFolder && p_fci_internal->fNextCab==FALSE)||
       (p_fci_internal->folders_size==0 &&
         (p_fci_internal->files_size!=0 ||
          p_fci_internal->placed_files_size!=0 )
     ) )
  {
      /* error */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
      return FALSE;
  }

  if( p_fci_internal->fNextCab ||
      p_fci_internal->fGetNextCabInVain ) {
    cbReserveCFFolder=p_fci_internal->oldCCAB.cbReserveCFFolder;
    cbReserveCFHeader=p_fci_internal->oldCCAB.cbReserveCFHeader;
    /* safety */
    if (strlen(p_fci_internal->oldCCAB.szCabPath)>=CB_MAX_CAB_PATH ||
        strlen(p_fci_internal->oldCCAB.szCab)>=CB_MAX_CABINET_NAME) {
      /* set error */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_INVALID_DATA );
      return FALSE;
    }
    /* get the full name of the cabinet */
    memcpy(szFileNameCABINET,p_fci_internal->oldCCAB.szCabPath,
      CB_MAX_CAB_PATH);
    memcpy(szFileNameCABINET+strlen(szFileNameCABINET),
      p_fci_internal->oldCCAB.szCab, CB_MAX_CABINET_NAME);
  } else {
    cbReserveCFFolder=p_fci_internal->pccab->cbReserveCFFolder;
    cbReserveCFHeader=p_fci_internal->pccab->cbReserveCFHeader;
    /* safety */
    if (strlen(p_fci_internal->pccab->szCabPath)>=CB_MAX_CAB_PATH ||
        strlen(p_fci_internal->pccab->szCab)>=CB_MAX_CABINET_NAME) {
      /* set error */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_INVALID_DATA );
      return FALSE;
    }
    /* get the full name of the cabinet */
    memcpy(szFileNameCABINET,p_fci_internal->pccab->szCabPath,
      CB_MAX_CAB_PATH);
    memcpy(szFileNameCABINET+strlen(szFileNameCABINET),
      p_fci_internal->pccab->szCab, CB_MAX_CABINET_NAME);
  }

  memcpy(cfheader.signature,"!CAB",4);
  cfheader.reserved1=0;
  cfheader.cbCabinet=   /* size of the cabinet file in bytes */
    sizeof(CFHEADER) +
    p_fci_internal->folders_size +
    p_fci_internal->placed_files_size +
    p_fci_internal->sizeFileCFDATA2;

  if (p_fci_internal->fPrevCab) {
    cfheader.cbCabinet+=strlen(p_fci_internal->szPrevCab)+1 +
      strlen(p_fci_internal->szPrevDisk)+1;
  }
  if (p_fci_internal->fNextCab) {
    cfheader.cbCabinet+=strlen(p_fci_internal->pccab->szCab)+1 +
      strlen(p_fci_internal->pccab->szDisk)+1;
  }
  if( p_fci_internal->fNextCab ||
      p_fci_internal->fGetNextCabInVain ) {
    cfheader.cbCabinet+=p_fci_internal->oldCCAB.cbReserveCFHeader;
    if ( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
      cfheader.cbCabinet+=4;
    }
  } else {
    cfheader.cbCabinet+=p_fci_internal->pccab->cbReserveCFHeader;
    if ( p_fci_internal->pccab->cbReserveCFHeader != 0 ||
        p_fci_internal->pccab->cbReserveCFFolder != 0 ||
        p_fci_internal->pccab->cbReserveCFData   != 0 ) {
      cfheader.cbCabinet+=4;
    }
  }

  if( ( ( p_fci_internal->fNextCab ||
          p_fci_internal->fGetNextCabInVain ) &&
        cfheader.cbCabinet > p_fci_internal->oldCCAB.cb
      ) ||
      ( ( p_fci_internal->fNextCab==FALSE &&
          p_fci_internal->fGetNextCabInVain==FALSE ) &&
        cfheader.cbCabinet > p_fci_internal->pccab->cb
      )
    )
  {
    set_error( p_fci_internal, FCIERR_NONE, ERROR_MORE_DATA );
    return FALSE;
  }


  cfheader.reserved2=0;
  cfheader.coffFiles=    /* offset to first CFFILE section */
   cfheader.cbCabinet - p_fci_internal->placed_files_size -
   p_fci_internal->sizeFileCFDATA2;

  cfheader.reserved3=0;
  cfheader.versionMinor=3;
  cfheader.versionMajor=1;
  /* number of CFFOLDER entries in the cabinet */
  cfheader.cFolders=p_fci_internal->cFolders;
  /* number of CFFILE entries in the cabinet */
  cfheader.cFiles=p_fci_internal->cFiles;
  cfheader.flags=0;    /* 1=prev cab, 2=next cabinet, 4=reserved sections */

  if( p_fci_internal->fPrevCab ) {
    cfheader.flags = cfheadPREV_CABINET;
  }

  if( p_fci_internal->fNextCab ) {
    cfheader.flags |= cfheadNEXT_CABINET;
  }

  if( p_fci_internal->fNextCab ||
      p_fci_internal->fGetNextCabInVain ) {
    if( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
      cfheader.flags |= cfheadRESERVE_PRESENT;
    }
    cfheader.setID = p_fci_internal->oldCCAB.setID;
    cfheader.iCabinet = p_fci_internal->oldCCAB.iCab-1;
  } else {
    if( p_fci_internal->pccab->cbReserveCFHeader != 0 ||
        p_fci_internal->pccab->cbReserveCFFolder != 0 ||
        p_fci_internal->pccab->cbReserveCFData   != 0 ) {
      cfheader.flags |= cfheadRESERVE_PRESENT;
    }
    cfheader.setID = p_fci_internal->pccab->setID;
    cfheader.iCabinet = p_fci_internal->pccab->iCab-1;
  }

  /* create the cabinet */
  handleCABINET = p_fci_internal->open( szFileNameCABINET, _O_RDWR | _O_CREAT | _O_TRUNC | _O_BINARY,
                                        _S_IREAD | _S_IWRITE, &err, p_fci_internal->pv );
  if(handleCABINET==-1)
  {
    set_error( p_fci_internal, FCIERR_CAB_FILE, err );
    return FALSE;
  }
  /* TODO error checking of err */

  /* set little endian */
  cfheader.reserved1=fci_endian_ulong(cfheader.reserved1);
  cfheader.cbCabinet=fci_endian_ulong(cfheader.cbCabinet);
  cfheader.reserved2=fci_endian_ulong(cfheader.reserved2);
  cfheader.coffFiles=fci_endian_ulong(cfheader.coffFiles);
  cfheader.reserved3=fci_endian_ulong(cfheader.reserved3);
  cfheader.cFolders=fci_endian_uword(cfheader.cFolders);
  cfheader.cFiles=fci_endian_uword(cfheader.cFiles);
  cfheader.flags=fci_endian_uword(cfheader.flags);
  cfheader.setID=fci_endian_uword(cfheader.setID);
  cfheader.iCabinet=fci_endian_uword(cfheader.iCabinet);

  /* write CFHEADER into cabinet file */
  if( p_fci_internal->write( handleCABINET, /* file handle */
      &cfheader, /* memory buffer */
      sizeof(cfheader), /* number of bytes to copy */
      &err, p_fci_internal->pv) != sizeof(cfheader) ) {
    /* write error */
    set_error( p_fci_internal, FCIERR_CAB_FILE, ERROR_WRITE_FAULT );
    return FALSE;
  }
  /* TODO error handling of err */

  /* reset little endian */
  cfheader.reserved1=fci_endian_ulong(cfheader.reserved1);
  cfheader.cbCabinet=fci_endian_ulong(cfheader.cbCabinet);
  cfheader.reserved2=fci_endian_ulong(cfheader.reserved2);
  cfheader.coffFiles=fci_endian_ulong(cfheader.coffFiles);
  cfheader.reserved3=fci_endian_ulong(cfheader.reserved3);
  cfheader.cFolders=fci_endian_uword(cfheader.cFolders);
  cfheader.cFiles=fci_endian_uword(cfheader.cFiles);
  cfheader.flags=fci_endian_uword(cfheader.flags);
  cfheader.setID=fci_endian_uword(cfheader.setID);
  cfheader.iCabinet=fci_endian_uword(cfheader.iCabinet);

  if( cfheader.flags & cfheadRESERVE_PRESENT ) {
    /* NOTE: No checks for maximum value overflows as designed by MS!!! */
    cfreserved.cbCFHeader = cbReserveCFHeader;
    cfreserved.cbCFFolder = cbReserveCFFolder;
    if( p_fci_internal->fNextCab ||
        p_fci_internal->fGetNextCabInVain ) {
      cfreserved.cbCFData = p_fci_internal->oldCCAB.cbReserveCFData;
    } else {
      cfreserved.cbCFData = p_fci_internal->pccab->cbReserveCFData;
    }

    /* set little endian */
    cfreserved.cbCFHeader=fci_endian_uword(cfreserved.cbCFHeader);

    /* write reserved info into cabinet file */
    if( p_fci_internal->write( handleCABINET, /* file handle */
        &cfreserved, /* memory buffer */
        sizeof(cfreserved), /* number of bytes to copy */
        &err, p_fci_internal->pv) != sizeof(cfreserved) ) {
      /* write error */
      set_error( p_fci_internal, FCIERR_CAB_FILE, ERROR_WRITE_FAULT );
      return FALSE;
    }
    /* TODO error handling of err */

    /* reset little endian */
    cfreserved.cbCFHeader=fci_endian_uword(cfreserved.cbCFHeader);
  }

  /* add optional reserved area */
  if (cbReserveCFHeader!=0) {
    if(!(reserved = p_fci_internal->alloc( cbReserveCFHeader))) {
      set_error( p_fci_internal, FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
      return FALSE;
    }
    for(i=0;i<cbReserveCFHeader;) {
      reserved[i++]='\0';
    }
    if( p_fci_internal->write( handleCABINET, /* file handle */
        reserved, /* memory buffer */
        cbReserveCFHeader, /* number of bytes to copy */
        &err, p_fci_internal->pv) != cbReserveCFHeader ) {
      p_fci_internal->free(reserved);
      /* write error */
      set_error( p_fci_internal, FCIERR_CAB_FILE, ERROR_WRITE_FAULT );
      return FALSE;
    }
    /* TODO error handling of err */
    p_fci_internal->free(reserved);
  }

  if( cfheader.flags & cfheadPREV_CABINET ) {
    if( p_fci_internal->write( handleCABINET, /* file handle */
        p_fci_internal->szPrevCab, /* memory buffer */
        strlen(p_fci_internal->szPrevCab)+1, /* number of bytes to copy */
        &err, p_fci_internal->pv) != strlen(p_fci_internal->szPrevCab)+1 ) {
      /* write error */
      set_error( p_fci_internal, FCIERR_CAB_FILE, ERROR_WRITE_FAULT );
      return FALSE;
    }
    /* TODO error handling of err */

    if( p_fci_internal->write( handleCABINET, /* file handle */
        p_fci_internal->szPrevDisk, /* memory buffer */
        strlen(p_fci_internal->szPrevDisk)+1, /* number of bytes to copy */
        &err, p_fci_internal->pv) != strlen(p_fci_internal->szPrevDisk)+1 ) {
      /* write error */
      set_error( p_fci_internal, FCIERR_CAB_FILE, ERROR_WRITE_FAULT );
      return FALSE;
    }
    /* TODO error handling of err */
  }

  if( cfheader.flags & cfheadNEXT_CABINET ) {
    if( p_fci_internal->write( handleCABINET, /* file handle */
        p_fci_internal->pccab->szCab, /* memory buffer */
        strlen(p_fci_internal->pccab->szCab)+1, /* number of bytes to copy */
        &err, p_fci_internal->pv) != strlen(p_fci_internal->pccab->szCab)+1 ) {
      /* write error */
      set_error( p_fci_internal, FCIERR_CAB_FILE, ERROR_WRITE_FAULT );
      return FALSE;
    }
    /* TODO error handling of err */

    if( p_fci_internal->write( handleCABINET, /* file handle */
        p_fci_internal->pccab->szDisk, /* memory buffer */
        strlen(p_fci_internal->pccab->szDisk)+1, /* number of bytes to copy */
        &err, p_fci_internal->pv) != strlen(p_fci_internal->pccab->szDisk)+1 ) {
      /* write error */
      set_error( p_fci_internal, FCIERR_CAB_FILE, ERROR_WRITE_FAULT );
      return FALSE;
    }
    /* TODO error handling of err */
  }

  /* add size of header size of all CFFOLDERs and size of all CFFILEs */
  header_size = p_fci_internal->placed_files_size + p_fci_internal->folders_size + sizeof(CFHEADER);
  if( p_fci_internal->fNextCab || p_fci_internal->fGetNextCabInVain ) {
      header_size+=p_fci_internal->oldCCAB.cbReserveCFHeader;
  } else {
      header_size+=p_fci_internal->pccab->cbReserveCFHeader;
  }

  if (p_fci_internal->fPrevCab) {
      header_size += strlen(p_fci_internal->szPrevCab)+1 +
        strlen(p_fci_internal->szPrevDisk)+1;
  }

  if (p_fci_internal->fNextCab) {
      header_size += strlen(p_fci_internal->oldCCAB.szCab)+1 +
        strlen(p_fci_internal->oldCCAB.szDisk)+1;
  }

  if( p_fci_internal->fNextCab || p_fci_internal->fGetNextCabInVain ) {
      header_size += p_fci_internal->oldCCAB.cbReserveCFHeader;
      if( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
          p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
          p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
        header_size += 4;
      }
  } else {
      header_size += p_fci_internal->pccab->cbReserveCFHeader;
      if( p_fci_internal->pccab->cbReserveCFHeader != 0 ||
          p_fci_internal->pccab->cbReserveCFFolder != 0 ||
          p_fci_internal->pccab->cbReserveCFData   != 0 ) {
        header_size += 4;
      }
  }

  if (!write_folders( p_fci_internal, handleCABINET, header_size, pfnfcis )) return FALSE;

  /* set seek of p_fci_internal->handleCFDATA2 to 0 */
  if( p_fci_internal->seek(p_fci_internal->handleCFDATA2,
      0, SEEK_SET, &err, p_fci_internal->pv) != 0 ) {
    /* wrong return value */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_SEEK );
    return FALSE;
  }
  /* TODO error handling of err */

  /* reset the number of folders for the next cabinet */
  p_fci_internal->cFolders=0;
  /* reset the number of files for the next cabinet */
  p_fci_internal->cFiles=0;

  /* while not all CFDATA structures have been copied to the cabinet do */
  if (p_fci_internal->data_out) while(!FALSE) {
    /* REUSE the variable read_result AGAIN */
    /* REUSE the buffer p_fci_internal->data_out AGAIN */
    /* read a block from p_fci_internal->handleCFDATA2 */
    read_result = p_fci_internal->read( p_fci_internal->handleCFDATA2 /* handle */,
        p_fci_internal->data_out, /* memory buffer */
        CB_MAX_CHUNK, /* number of bytes to copy */
        &err, p_fci_internal->pv);
    if( read_result == 0 ) break; /* ALL CFDATA structures have been copied */
    /* TODO error handling of err */

    /* write the block to the cabinet file */
    if( p_fci_internal->write( handleCABINET, /* file handle */
        p_fci_internal->data_out, /* memory buffer */
        read_result, /* number of bytes to copy */
        &err, p_fci_internal->pv) != read_result ) {
      /* write error */
      set_error( p_fci_internal, FCIERR_CAB_FILE, ERROR_WRITE_FAULT );
      return FALSE;
    }
    /* TODO error handling of err */

    p_fci_internal->statusFolderCopied += read_result;
    /* report status with pfnfcis about copied size of folder */
    if( (*pfnfcis)(statusFolder,
        p_fci_internal->statusFolderCopied, /* length of copied blocks */
        p_fci_internal->statusFolderTotal, /* total size of folder */
        p_fci_internal->pv) == -1) {
      /* set error code and abort */
      set_error( p_fci_internal, FCIERR_USER_ABORT, 0 );
      return FALSE;
    }
  } /* END OF while */

  /* set seek of the cabinet file to 0 */
  if( p_fci_internal->seek( handleCABINET,
      0, SEEK_SET, &err, p_fci_internal->pv) != 0 ) {
    /* wrong return value */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_SEEK );
    return FALSE;
  }
  /* TODO error handling of err */

  /* write the signature "MSCF" into the cabinet file */
  memcpy( cfheader.signature, "MSCF", 4 );
  if( p_fci_internal->write( handleCABINET, /* file handle */
      &cfheader, /* memory buffer */
      4, /* number of bytes to copy */
      &err, p_fci_internal->pv) != 4 ) {
    /* wrtie error */
    set_error( p_fci_internal, FCIERR_CAB_FILE, ERROR_WRITE_FAULT );
    return FALSE;
  }
  /* TODO error handling of err */

  /* close the cabinet file */
  p_fci_internal->close(handleCABINET,&err,p_fci_internal->pv);
  /* TODO error handling of err */

  p_fci_internal->close( p_fci_internal->handleCFDATA2,&err,p_fci_internal->pv);
  /* TODO error handling of err */
  p_fci_internal->delete( p_fci_internal->szFileNameCFDATA2, &err,
    p_fci_internal->pv);
  /* TODO error handling of err */

  /* get 3 temporary files and open them */
  /* write names and handles to hfci */

  p_fci_internal->sizeFileCFDATA2  = 0;
  p_fci_internal->placed_files_size  = 0;
  p_fci_internal->folders_size = 0;

  /* CFDATA with checksum and ready to be copied into cabinet */
  if( !p_fci_internal->gettemp( p_fci_internal->szFileNameCFDATA2,
                                CB_MAX_FILENAME, p_fci_internal->pv)) {
    /* error handling */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_FUNCTION_FAILED );
    return FALSE;
  }
  /* safety */
  if ( strlen(p_fci_internal->szFileNameCFDATA2) >= CB_MAX_FILENAME ) {
    /* set error code and abort */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_INVALID_DATA );
    return FALSE;
  }
  p_fci_internal->handleCFDATA2 = p_fci_internal->open( p_fci_internal->szFileNameCFDATA2,
                                                        _O_RDWR | _O_CREAT | _O_EXCL | _O_BINARY,
                                                        _S_IREAD | _S_IWRITE, &err, p_fci_internal->pv);
  /* check handle */
  if(p_fci_internal->handleCFDATA2==0){
    set_error( p_fci_internal, FCIERR_TEMP_FILE, ERROR_OPEN_FAILED );
    return FALSE;
  }
  /* TODO error checking of err */

  /* TODO close and delete new files when return FALSE */


  /* report status with pfnfcis about copied size of folder */
  (*pfnfcis)(statusCabinet,
    p_fci_internal->estimatedCabinetSize, /* estimated cabinet file size */
    cfheader.cbCabinet /* real cabinet file size */, p_fci_internal->pv);

  p_fci_internal->fPrevCab=TRUE;
  /* The sections szPrevCab and szPrevDisk are not being updated, because */
  /* MS CABINET.DLL always puts the first cabinet name and disk into them */

  if (p_fci_internal->fNextCab) {
    p_fci_internal->fNextCab=FALSE;

    if (p_fci_internal->files_size==0 && p_fci_internal->sizeFileCFDATA1!=0) {
      /* THIS CAN NEVER HAPPEN */
      /* set error code */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
      return FALSE;
    }

/* COPIED FROM FCIAddFile and modified */

    /* REUSE the variable read_result */
    if (p_fci_internal->fGetNextCabInVain) {
      read_result=p_fci_internal->oldCCAB.cbReserveCFHeader;
      if(p_fci_internal->files_size!=0) {
        read_result+=p_fci_internal->oldCCAB.cbReserveCFFolder;
      }
      if ( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
          p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
          p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
        read_result+=4;
      }
    } else {
      read_result=p_fci_internal->pccab->cbReserveCFHeader;
      if(p_fci_internal->files_size!=0) {
        read_result+=p_fci_internal->pccab->cbReserveCFFolder;
      }
      if ( p_fci_internal->pccab->cbReserveCFHeader != 0 ||
          p_fci_internal->pccab->cbReserveCFFolder != 0 ||
          p_fci_internal->pccab->cbReserveCFData   != 0 ) {
        read_result+=4;
      }
    }
    if ( p_fci_internal->fPrevCab ) {
      read_result+= strlen(p_fci_internal->szPrevCab)+1+
        strlen(p_fci_internal->szPrevDisk)+1;
    }
    read_result+= p_fci_internal->sizeFileCFDATA1 +
      p_fci_internal->files_size + p_fci_internal->sizeFileCFDATA2 +
      p_fci_internal->placed_files_size + p_fci_internal->folders_size +
      sizeof(CFHEADER) +
      sizeof(CFFOLDER); /* set size of new CFFolder entry */

    if( p_fci_internal->fNewPrevious ) {
      memcpy(p_fci_internal->szPrevCab, p_fci_internal->oldCCAB.szCab,
        CB_MAX_CABINET_NAME);
      memcpy(p_fci_internal->szPrevDisk, p_fci_internal->oldCCAB.szDisk,
        CB_MAX_DISK_NAME);
      p_fci_internal->fNewPrevious=FALSE;
    }

    /* too much data for the maximum size of a cabinet */
    if( p_fci_internal->fGetNextCabInVain==FALSE &&
        p_fci_internal->pccab->cb < read_result ) {
      return fci_flush_cabinet( p_fci_internal, FALSE, pfnfcignc, pfnfcis);
    }

    /* Might be too much data for the maximum size of a cabinet.*/
    /* When any further data will be added later, it might not */
    /* be possible to flush the cabinet, because there might */
    /* not be enough space to store the name of the following */
    /* cabinet and name of the corresponding disk. */
    /* So take care of this and get the name of the next cabinet */
    if (p_fci_internal->fGetNextCabInVain==FALSE && (
      p_fci_internal->pccab->cb < read_result +
      CB_MAX_CABINET_NAME + CB_MAX_DISK_NAME
    )) {
      /* save CCAB */
      p_fci_internal->oldCCAB = *p_fci_internal->pccab;
      /* increment cabinet index */
      ++(p_fci_internal->pccab->iCab);
      /* get name of next cabinet */
      p_fci_internal->estimatedCabinetSize=p_fci_internal->statusFolderTotal;
      if (!(*pfnfcignc)(p_fci_internal->pccab,
          p_fci_internal->estimatedCabinetSize, /* estimated size of cab */
          p_fci_internal->pv)) {
        /* error handling */
        set_error( p_fci_internal, FCIERR_NONE, ERROR_FUNCTION_FAILED );
        return FALSE;
      }
      /* Skip a few lines of code. This is caught by the next if. */
      p_fci_internal->fGetNextCabInVain=TRUE;
    }

    /* too much data for cabinet */
    if (p_fci_internal->fGetNextCabInVain && (
        p_fci_internal->oldCCAB.cb < read_result +
        strlen(p_fci_internal->oldCCAB.szCab)+1+
        strlen(p_fci_internal->oldCCAB.szDisk)+1
    )) {
      p_fci_internal->fGetNextCabInVain=FALSE;
      p_fci_internal->fNextCab=TRUE;
      return fci_flush_cabinet( p_fci_internal, FALSE, pfnfcignc, pfnfcis);
    }

    /* if the FolderThreshold has been reached flush the folder automatically */
    if( p_fci_internal->fGetNextCabInVain ) {
      if( p_fci_internal->cCompressedBytesInFolder >=
          p_fci_internal->oldCCAB.cbFolderThresh) {
        return fci_flush_folder(p_fci_internal, FALSE, pfnfcignc, pfnfcis);
      }
    } else {
      if( p_fci_internal->cCompressedBytesInFolder >=
          p_fci_internal->pccab->cbFolderThresh) {
        return fci_flush_folder(p_fci_internal, FALSE, pfnfcignc, pfnfcis);
      }
    }

/* END OF COPIED FROM FCIAddFile and modified */

    if( p_fci_internal->files_size>0 ) {
      if( !fci_flush_folder(p_fci_internal, FALSE, pfnfcignc, pfnfcis) ) return FALSE;
      p_fci_internal->fNewPrevious=TRUE;
    }
  } else {
    p_fci_internal->fNewPrevious=FALSE;
    if( p_fci_internal->files_size>0 || p_fci_internal->sizeFileCFDATA1) {
      /* THIS MAY NEVER HAPPEN */
      /* set error structures */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
      return FALSE;
    }
  }

  return TRUE;
} /* end of fci_flush_cabinet */





/***********************************************************************
 *		FCIAddFile (CABINET.11)
 *
 * FCIAddFile adds a file to the to be created cabinet file
 *
 * PARAMS
 *   hfci          [I]  An HFCI from FCICreate
 *   pszSourceFile [I]  A pointer to a C string which contains the name and
 *                      location of the file which will be added to the cabinet
 *   pszFileName   [I]  A pointer to a C string which contains the name under
 *                      which the file will be stored in the cabinet
 *   fExecute      [I]  A boolean value which indicates if the file should be
 *                      executed after extraction of self extracting
 *                      executables
 *   pfnfcignc     [I]  A pointer to a function which gets information about
 *                      the next cabinet
 *   pfnfcis      [IO]  A pointer to a function which will report status
 *                      information about the compression process
 *   pfnfcioi      [I]  A pointer to a function which reports file attributes
 *                      and time and date information
 *   typeCompress  [I]  Compression type
 *
 * RETURNS
 *   On success, returns TRUE
 *   On failure, returns FALSE
 *
 * INCLUDES
 *   fci.h
 *
 */
BOOL __cdecl FCIAddFile(
	HFCI                  hfci,
	char                 *pszSourceFile,
	char                 *pszFileName,
	BOOL                  fExecute,
	PFNFCIGETNEXTCABINET  pfnfcignc,
	PFNFCISTATUS          pfnfcis,
	PFNFCIGETOPENINFO     pfnfcigoi,
	TCOMP                 typeCompress)
{
  int err;
  cab_ULONG read_result;
  int file_handle;
  struct file *file;
  FCI_Int *p_fci_internal = get_fci_ptr( hfci );

  if (!p_fci_internal) return FALSE;

  if ((!pszSourceFile) || (!pszFileName) || (!pfnfcignc) || (!pfnfcis) ||
      (!pfnfcigoi) || strlen(pszFileName)>=CB_MAX_FILENAME) {
    set_error( p_fci_internal, FCIERR_NONE, ERROR_BAD_ARGUMENTS );
    return FALSE;
  }

  /* TODO check if pszSourceFile??? */

  if(p_fci_internal->fGetNextCabInVain && p_fci_internal->fNextCab) {
    /* internal error */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
    return FALSE;
  }

  if(p_fci_internal->fNextCab) {
    /* internal error */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
    return FALSE;
  }

  if (!(file = add_file( p_fci_internal, pszFileName ))) return FALSE;

  /* allocation of memory */
  if (p_fci_internal->data_in==NULL) {
    if (p_fci_internal->cdata_in!=0) {
      /* error handling */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
      return FALSE;
    }
    if (p_fci_internal->data_out!=NULL) {
      /* error handling */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
      return FALSE;
    }
    if(!(p_fci_internal->data_in = p_fci_internal->alloc(CB_MAX_CHUNK))) {
      set_error( p_fci_internal, FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
      return FALSE;
    }
    if (p_fci_internal->data_out==NULL) {
      if(!(p_fci_internal->data_out = p_fci_internal->alloc( 2 * CB_MAX_CHUNK))){
        set_error( p_fci_internal, FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
      }
    }
  }

  if (p_fci_internal->data_out==NULL) {
    p_fci_internal->free(p_fci_internal->data_in);
    /* error handling */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
    return FALSE;
  }

  /* get information about the file */
  /* set defaults in case callback doesn't set one or more fields */
  file_handle = pfnfcigoi( pszSourceFile, &file->date, &file->time, &file->attribs,
                           &err, p_fci_internal->pv );
  /* check file_handle */
  if(file_handle==0){
    set_error( p_fci_internal, FCIERR_OPEN_SRC, ERROR_OPEN_FAILED );
  }
  /* TODO error handling of err */

  if (fExecute) { file->attribs |= _A_EXEC; }

  /* REUSE the variable read_result */
  if (p_fci_internal->fGetNextCabInVain) {
    read_result=p_fci_internal->oldCCAB.cbReserveCFHeader +
      p_fci_internal->oldCCAB.cbReserveCFFolder;
    if ( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
      read_result+=4;
    }
  } else {
    read_result=p_fci_internal->pccab->cbReserveCFHeader +
      p_fci_internal->pccab->cbReserveCFFolder;
    if ( p_fci_internal->pccab->cbReserveCFHeader != 0 ||
        p_fci_internal->pccab->cbReserveCFFolder != 0 ||
        p_fci_internal->pccab->cbReserveCFData   != 0 ) {
      read_result+=4;
    }
  }
  if ( p_fci_internal->fPrevCab ) {
    read_result+= strlen(p_fci_internal->szPrevCab)+1+
      strlen(p_fci_internal->szPrevDisk)+1;
  }
  if ( p_fci_internal->fNextCab ) { /* this is never the case */
    read_result+= strlen(p_fci_internal->pccab->szCab)+1+
      strlen(p_fci_internal->pccab->szDisk)+1;
  }

  read_result+= sizeof(CFFILE) + strlen(pszFileName)+1 +
    p_fci_internal->files_size + p_fci_internal->sizeFileCFDATA2 +
    p_fci_internal->placed_files_size + p_fci_internal->folders_size +
    sizeof(CFHEADER) +
    sizeof(CFFOLDER); /* size of new CFFolder entry */

  /* Might be too much data for the maximum size of a cabinet.*/
  /* When any further data will be added later, it might not */
  /* be possible to flush the cabinet, because there might */
  /* not be enough space to store the name of the following */
  /* cabinet and name of the corresponding disk. */
  /* So take care of this and get the name of the next cabinet */
  if( p_fci_internal->fGetNextCabInVain==FALSE &&
      p_fci_internal->fNextCab==FALSE &&
      ( p_fci_internal->pccab->cb < read_result +
        CB_MAX_CABINET_NAME + CB_MAX_DISK_NAME
      )
  ) {
    /* save CCAB */
    p_fci_internal->oldCCAB = *p_fci_internal->pccab;
    /* increment cabinet index */
    ++(p_fci_internal->pccab->iCab);
    /* get name of next cabinet */
    p_fci_internal->estimatedCabinetSize=p_fci_internal->statusFolderTotal;
    if (!(*pfnfcignc)(p_fci_internal->pccab,
        p_fci_internal->estimatedCabinetSize, /* estimated size of cab */
        p_fci_internal->pv)) {
      /* error handling */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_FUNCTION_FAILED );
      return FALSE;
    }
    /* Skip a few lines of code. This is caught by the next if. */
    p_fci_internal->fGetNextCabInVain=TRUE;
  }

  if( p_fci_internal->fGetNextCabInVain &&
      p_fci_internal->fNextCab
  ) {
    /* THIS CAN NEVER HAPPEN */
    /* set error code */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
    return FALSE;
  }

  /* too much data for cabinet */
  if( p_fci_internal->fGetNextCabInVain &&
     (
      p_fci_internal->oldCCAB.cb < read_result +
      strlen(p_fci_internal->pccab->szCab)+1+
      strlen(p_fci_internal->pccab->szDisk)+1
  )) {
    p_fci_internal->fGetNextCabInVain=FALSE;
    p_fci_internal->fNextCab=TRUE;
    if(!fci_flush_cabinet( p_fci_internal, FALSE, pfnfcignc, pfnfcis)) return FALSE;
  }

  if( p_fci_internal->fNextCab ) {
    /* THIS MAY NEVER HAPPEN */
    /* set error code */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
    return FALSE;
  }

  /* read the contents of the file blockwise */
  while (!FALSE) {
    if (p_fci_internal->cdata_in > CAB_BLOCKMAX) {
      /* internal error */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
      return FALSE;
    }

    read_result = p_fci_internal->read( file_handle /* file handle */,
      (p_fci_internal->data_in + p_fci_internal->cdata_in) /* memory buffer */,
      (CAB_BLOCKMAX - p_fci_internal->cdata_in) /* number of bytes to copy */,
      &err, p_fci_internal->pv);
    /* TODO error handling of err */

    if( read_result==0 ) break;

    /* increment the block size */
    p_fci_internal->cdata_in += read_result;

    /* increment the file size */
    file->size += read_result;

    if ( p_fci_internal->cdata_in > CAB_BLOCKMAX ) {
      /* report internal error */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
      return FALSE;
    }
    /* write a whole block */
    if ( p_fci_internal->cdata_in == CAB_BLOCKMAX ) {

      if( !fci_flush_data_block(p_fci_internal, &err, pfnfcis) ) return FALSE;
    }
  }

  /* close the file from FCIAddFile */
  p_fci_internal->close(file_handle,&err,p_fci_internal->pv);
  /* TODO error handling of err */

  p_fci_internal->files_size += sizeof(CFFILE) + strlen(pszFileName)+1;

  /* REUSE the variable read_result */
  if (p_fci_internal->fGetNextCabInVain ||
      p_fci_internal->fNextCab
     ) {
    read_result=p_fci_internal->oldCCAB.cbReserveCFHeader +
      p_fci_internal->oldCCAB.cbReserveCFFolder;
    if ( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
      read_result+=4;
    }
  } else {
    read_result=p_fci_internal->pccab->cbReserveCFHeader +
      p_fci_internal->pccab->cbReserveCFFolder;
    if ( p_fci_internal->pccab->cbReserveCFHeader != 0 ||
        p_fci_internal->pccab->cbReserveCFFolder != 0 ||
        p_fci_internal->pccab->cbReserveCFData   != 0 ) {
      read_result+=4;
    }
  }
  if ( p_fci_internal->fPrevCab ) {
    read_result+= strlen(p_fci_internal->szPrevCab)+1+
      strlen(p_fci_internal->szPrevDisk)+1;
  }
  if ( p_fci_internal->fNextCab ) { /* this is never the case */
    read_result+= strlen(p_fci_internal->pccab->szCab)+1+
      strlen(p_fci_internal->pccab->szDisk)+1;
  }
  read_result+= p_fci_internal->sizeFileCFDATA1 +
    p_fci_internal->files_size + p_fci_internal->sizeFileCFDATA2 +
    p_fci_internal->placed_files_size + p_fci_internal->folders_size +
    sizeof(CFHEADER) +
    sizeof(CFFOLDER); /* set size of new CFFolder entry */

  /* too much data for the maximum size of a cabinet */
  /* (ignoring the unflushed data block) */
  if( p_fci_internal->fGetNextCabInVain==FALSE &&
      p_fci_internal->fNextCab==FALSE && /* this is always the case */
      p_fci_internal->pccab->cb < read_result ) {
    return fci_flush_cabinet( p_fci_internal, FALSE, pfnfcignc, pfnfcis);
  }

  /* Might be too much data for the maximum size of a cabinet.*/
  /* When any further data will be added later, it might not */
  /* be possible to flush the cabinet, because there might */
  /* not be enough space to store the name of the following */
  /* cabinet and name of the corresponding disk. */
  /* So take care of this and get the name of the next cabinet */
  /* (ignoring the unflushed data block) */
  if( p_fci_internal->fGetNextCabInVain==FALSE &&
      p_fci_internal->fNextCab==FALSE &&
      ( p_fci_internal->pccab->cb < read_result +
        CB_MAX_CABINET_NAME + CB_MAX_DISK_NAME
      )
  ) {
    /* save CCAB */
    p_fci_internal->oldCCAB = *p_fci_internal->pccab;
    /* increment cabinet index */
    ++(p_fci_internal->pccab->iCab);
    /* get name of next cabinet */
    p_fci_internal->estimatedCabinetSize=p_fci_internal->statusFolderTotal;
    if (!(*pfnfcignc)(p_fci_internal->pccab,
        p_fci_internal->estimatedCabinetSize,/* estimated size of cab */
        p_fci_internal->pv)) {
      /* error handling */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_FUNCTION_FAILED );
      return FALSE;
    }
    /* Skip a few lines of code. This is caught by the next if. */
    p_fci_internal->fGetNextCabInVain=TRUE;
  }

  if( p_fci_internal->fGetNextCabInVain &&
      p_fci_internal->fNextCab
  ) {
    /* THIS CAN NEVER HAPPEN */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
    return FALSE;
  }

  /* too much data for cabinet */
  if( (p_fci_internal->fGetNextCabInVain ||
      p_fci_internal->fNextCab) && (
      p_fci_internal->oldCCAB.cb < read_result +
      strlen(p_fci_internal->pccab->szCab)+1+
      strlen(p_fci_internal->pccab->szDisk)+1
  )) {

    p_fci_internal->fGetNextCabInVain=FALSE;
    p_fci_internal->fNextCab=TRUE;
    return fci_flush_cabinet( p_fci_internal, FALSE, pfnfcignc, pfnfcis);
  }

  if( p_fci_internal->fNextCab ) {
    /* THIS MAY NEVER HAPPEN */
    /* set error code */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
    return FALSE;
  }

  /* if the FolderThreshold has been reached flush the folder automatically */
  if( p_fci_internal->fGetNextCabInVain ) {
    if( p_fci_internal->cCompressedBytesInFolder >=
        p_fci_internal->oldCCAB.cbFolderThresh) {
      return FCIFlushFolder(hfci, pfnfcignc, pfnfcis);
    }
  } else {
    if( p_fci_internal->cCompressedBytesInFolder >=
        p_fci_internal->pccab->cbFolderThresh) {
      return FCIFlushFolder(hfci, pfnfcignc, pfnfcis);
    }
  }

  return TRUE;
} /* end of FCIAddFile */





/***********************************************************************
 *		FCIFlushFolder (CABINET.12)
 *
 * FCIFlushFolder completes the CFFolder structure under construction.
 *
 * All further data which is added by FCIAddFile will be associated to
 * the next CFFolder structure.
 *
 * FCIFlushFolder will be called by FCIAddFile automatically if the
 * threshold (stored in the member cbFolderThresh of the CCAB structure
 * pccab passed to FCICreate) is exceeded.
 *
 * FCIFlushFolder will be called by FCIFlushFolder automatically before
 * any data will be written into the cabinet file.
 *
 * PARAMS
 *   hfci          [I]  An HFCI from FCICreate
 *   pfnfcignc     [I]  A pointer to a function which gets information about
 *                      the next cabinet
 *   pfnfcis      [IO]  A pointer to a function which will report status
 *                      information about the compression process
 *
 * RETURNS
 *   On success, returns TRUE
 *   On failure, returns FALSE
 *
 * INCLUDES
 *   fci.h
 *
 */
BOOL __cdecl FCIFlushFolder(
	HFCI                  hfci,
	PFNFCIGETNEXTCABINET  pfnfcignc,
	PFNFCISTATUS          pfnfcis)
{
    FCI_Int *p_fci_internal = get_fci_ptr( hfci );

    if (!p_fci_internal) return FALSE;
    return fci_flush_folder(p_fci_internal,FALSE,pfnfcignc,pfnfcis);
}



/***********************************************************************
 *		FCIFlushCabinet (CABINET.13)
 *
 * FCIFlushCabinet stores the data which has been added by FCIAddFile
 * into the cabinet file. If the maximum cabinet size (stored in the
 * member cb of the CCAB structure pccab passed to FCICreate) has been
 * exceeded FCIFlushCabinet will be called automatic by FCIAddFile.
 * The remaining data still has to be flushed manually by calling
 * FCIFlushCabinet.
 *
 * After FCIFlushCabinet has been called (manually) FCIAddFile must
 * NOT be called again. Then hfci has to be released by FCIDestroy.
 *
 * PARAMS
 *   hfci          [I]  An HFCI from FCICreate
 *   fGetNextCab   [I]  Whether you want to add additional files to a
 *                      cabinet set (TRUE) or whether you want to
 *                      finalize it (FALSE)
 *   pfnfcignc     [I]  A pointer to a function which gets information about
 *                      the next cabinet
 *   pfnfcis      [IO]  A pointer to a function which will report status
 *                      information about the compression process
 *
 * RETURNS
 *   On success, returns TRUE
 *   On failure, returns FALSE
 *
 * INCLUDES
 *   fci.h
 *
 */
BOOL __cdecl FCIFlushCabinet(
	HFCI                  hfci,
	BOOL                  fGetNextCab,
	PFNFCIGETNEXTCABINET  pfnfcignc,
	PFNFCISTATUS          pfnfcis)
{
  FCI_Int *p_fci_internal = get_fci_ptr( hfci );

  if (!p_fci_internal) return FALSE;

  if(!fci_flush_cabinet(p_fci_internal,fGetNextCab,pfnfcignc,pfnfcis)) return FALSE;

  while( p_fci_internal->files_size>0 ||
         p_fci_internal->placed_files_size>0 ) {
    if(!fci_flush_cabinet(p_fci_internal,fGetNextCab,pfnfcignc,pfnfcis)) return FALSE;
  }

  return TRUE;
}


/***********************************************************************
 *		FCIDestroy (CABINET.14)
 *
 * Frees a handle created by FCICreate.
 * Only reason for failure would be an invalid handle.
 *
 * PARAMS
 *   hfci [I] The HFCI to free
 *
 * RETURNS
 *   TRUE for success
 *   FALSE for failure
 */
BOOL __cdecl FCIDestroy(HFCI hfci)
{
    int err;
    struct folder *folder, *folder_next;
    struct file *file, *file_next;
    FCI_Int *p_fci_internal = get_fci_ptr( hfci );

    if (!p_fci_internal) return FALSE;

    /* before hfci can be removed all temporary files must be closed */
    /* and deleted */
    p_fci_internal->magic = 0;

    LIST_FOR_EACH_ENTRY_SAFE( folder, folder_next, &p_fci_internal->folders_list, struct folder, entry )
    {
        free_folder( p_fci_internal, folder );
    }
    LIST_FOR_EACH_ENTRY_SAFE( file, file_next, &p_fci_internal->files_list, struct file, entry )
    {
        free_file( p_fci_internal, file );
    }

    p_fci_internal->close( p_fci_internal->handleCFDATA1,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    p_fci_internal->delete( p_fci_internal->szFileNameCFDATA1, &err,
      p_fci_internal->pv);
    /* TODO error handling of err */
    p_fci_internal->close( p_fci_internal->handleCFDATA2,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    p_fci_internal->delete( p_fci_internal->szFileNameCFDATA2, &err,
      p_fci_internal->pv);
    /* TODO error handling of err */

    /* data in and out buffers have to be removed */
    if (p_fci_internal->data_in!=NULL)
      p_fci_internal->free(p_fci_internal->data_in);
    if (p_fci_internal->data_out!=NULL)
      p_fci_internal->free(p_fci_internal->data_out);

    /* hfci can now be removed */
    p_fci_internal->free(hfci);
    return TRUE;
}
