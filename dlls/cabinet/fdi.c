/*
 * File Decompression Interface
 *
 * Copyright 2000-2002 Stuart Caie
 * Copyright 2002 Patrik Stridvall
 * Copyright 2003 Greg Turner
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * This is (or will be) a largely redundant reimplementation of the stuff in
 * cabextract.c... it would theoretically be preferable to have only one, shared
 * implementation, however there are semantic differences which may discourage efforts
 * to unify the two.  It should be possible, if awkward, to go back and reimplement
 * cabextract.c using FDI (once the FDI implementation is complete, of course).
 *   -gmt
 */

#include "config.h"

#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "fdi.h"
#include "msvcrt/fcntl.h" /* _O_.* */
#include "cabinet.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cabinet);

struct fdi_cab {
  struct fdi_cab *next;                /* for making a list of cabinets  */
  LPCSTR filename;                     /* input name of cabinet          */
  int *fh;                             /* open file handle or NULL       */
  cab_off_t filelen;                   /* length of cabinet file         */
  struct fdi_cab *prevcab, *nextcab;   /* multipart cabinet chains       */
  char *prevname, *nextname;           /* and their filenames            */
  char *previnfo, *nextinfo;           /* and their visible names        */
  struct fdi_folder *folders;          /* first folder in this cabinet   */
  struct fdi_file *files;              /* first file in this cabinet     */
  cab_UBYTE block_resv;                /* reserved space in datablocks   */
  cab_UBYTE flags;                     /* header flags                   */
};

struct fdi_file {
  struct fdi_file *next;               /* next file in sequence          */
  struct fdi_folder *folder;           /* folder that contains this file */
  LPCSTR filename;                     /* output name of file            */
  int    fh;                           /* open file handle or NULL       */
  cab_ULONG length;                    /* uncompressed length of file    */
  cab_ULONG offset;                    /* uncompressed offset in folder  */
  cab_UWORD index;                     /* magic index number of folder   */
  cab_UWORD time, date, attribs;       /* MS-DOS time/date/attributes    */
};

struct fdi_folder {
  struct fdi_folder *next;
  struct fdi_cab *cab[CAB_SPLITMAX];   /* cabinet(s) this folder spans   */
  cab_off_t offset[CAB_SPLITMAX];      /* offset to data blocks (32 bit) */
  cab_UWORD comp_type;                 /* compression format/window size */
  cab_ULONG comp_size;                 /* compressed size of folder      */
  cab_UBYTE num_splits;                /* number of split blocks + 1     */
  cab_UWORD num_blocks;                /* total number of blocks         */
  struct fdi_file *contfile;           /* the first split file           */
};

/*
 * ugh, well, this ended up being pretty damn silly...
 * now that I've conceeded to build equivalent structures to struct cab.*,
 * I should have just used those, or, better yet, unified the two... sue me.
 * (Note to Microsoft: That's a joke.  Please /don't/ actually sue me! -gmt).
 * Nevertheless, I've come this far, it works, so I'm not gonna change it
 * for now.
 */

/*
 * this structure fills the gaps between what is available in a PFDICABINETINFO
 * vs what is needed by FDICopy.  Memory allocated for these becomes the responsibility
 * of the caller to free.  Yes, I am aware that this is totally, utterly inelegant.
 */
typedef struct {
  char *prevname, *previnfo;
  char *nextname, *nextinfo;
  int folder_resv, header_resv;
} MORE_ISCAB_INFO, *PMORE_ISCAB_INFO;

/***********************************************************************
 *		FDICreate (CABINET.20)
 */
HFDI __cdecl FDICreate(
	PFNALLOC pfnalloc,
	PFNFREE  pfnfree,
	PFNOPEN  pfnopen,
	PFNREAD  pfnread,
	PFNWRITE pfnwrite,
	PFNCLOSE pfnclose,
	PFNSEEK  pfnseek,
	int      cpuType,
	PERF     perf)
{
  HFDI rv;

  TRACE("(pfnalloc == ^%p, pfnfree == ^%p, pfnopen == ^%p, pfnread == ^%p, pfnwrite == ^%p, \
        pfnclose == ^%p, pfnseek == ^%p, cpuType == %d, perf == ^%p)\n", 
        pfnalloc, pfnfree, pfnopen, pfnread, pfnwrite, pfnclose, pfnseek,
        cpuType, perf);

  /* PONDERME: Certainly, we cannot tolerate a missing pfnalloc, as we call it just below.
     pfnfree is tested as well, for symmetry.  As for the rest, should we test these
     too?  In a vacuum, I would say yes... but does Windows care?  If not, then, I guess,
     neither do we.... */
  if ((!pfnalloc) || (!pfnfree)) {
    perf->erfOper = FDIERROR_NONE;
    perf->erfType = ERROR_BAD_ARGUMENTS;
    perf->fError = TRUE;

    SetLastError(ERROR_BAD_ARGUMENTS);
    return NULL;
  }

  if (!((rv = ((HFDI) (*pfnalloc)(sizeof(FDI_Int)))))) {
    perf->erfOper = FDIERROR_ALLOC_FAIL;
    perf->erfType = ERROR_NOT_ENOUGH_MEMORY;
    perf->fError = TRUE;

    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return NULL;
  }
  
  PFDI_INT(rv)->FDI_Intmagic = FDI_INT_MAGIC;
  PFDI_INT(rv)->pfnalloc = pfnalloc;
  PFDI_INT(rv)->pfnfree = pfnfree;
  PFDI_INT(rv)->pfnopen = pfnopen;
  PFDI_INT(rv)->pfnread = pfnread;
  PFDI_INT(rv)->pfnwrite = pfnwrite;
  PFDI_INT(rv)->pfnclose = pfnclose;
  PFDI_INT(rv)->pfnseek = pfnseek;
  /* no-brainer: we ignore the cpu type; this is only used
     for the 16-bit versions in Windows anyhow... */
  PFDI_INT(rv)->perf = perf;

  return rv;
}

/*******************************************************************
 * FDI_getoffset (internal)
 *
 * returns the file pointer position of a cab
 */
long FDI_getoffset(HFDI hfdi, INT_PTR hf)
{
  return PFDI_SEEK(hfdi, hf, 0L, SEEK_CUR);
}

/**********************************************************************
 * FDI_realloc (internal)
 *
 * we can't use _msize; the user might not be using malloc, so we require
 * an explicit specification of the previous size. utterly inefficient.
 */
void *FDI_realloc(HFDI hfdi, void *mem, size_t prevsize, size_t newsize)
{
  void *rslt = NULL;
  char *irslt, *imem;
  size_t copysize = (prevsize < newsize) ? prevsize : newsize;
  if (prevsize == newsize) return mem;
  rslt = PFDI_ALLOC(hfdi, newsize); 
  if (rslt)
    for (irslt = (char *)rslt, imem = (char *)mem; (copysize); copysize--)
      *irslt++ = *imem++;
  PFDI_FREE(hfdi, mem);
  return rslt;
}

/**********************************************************************
 * FDI_read_string (internal)
 *
 * allocate and read an aribitrarily long string from the cabinet
 */
char *FDI_read_string(HFDI hfdi, INT_PTR hf, long cabsize)
{
  size_t len=256,
         oldlen = 0,
         base = FDI_getoffset(hfdi, hf),
         maxlen = cabsize - base;
  BOOL ok = FALSE;
  int i;
  cab_UBYTE *buf = NULL;

  TRACE("(hfdi == ^%p, hf == %d)\n", hfdi, hf);

  do {
    if (len > maxlen) len = maxlen;
    if (!(buf = FDI_realloc(hfdi, buf, oldlen, len))) break;
    oldlen = len;
    if (!PFDI_READ(hfdi, hf, buf, len)) break;

    /* search for a null terminator in what we've just read */
    for (i=0; i < len; i++) {
      if (!buf[i]) {ok=TRUE; break;}
    }

    if (!ok) {
      if (len == maxlen) {
        ERR("WARNING: cabinet is truncated\n");
        break;
      }
      len += 256;
      PFDI_SEEK(hfdi, hf, base, SEEK_SET);
    }
  } while (!ok);

  if (!ok) {
    if (buf)
      PFDI_FREE(hfdi, buf);
    else
      ERR("out of memory!\n");
    return NULL;
  }

  /* otherwise, set the stream to just after the string and return */
  PFDI_SEEK(hfdi, hf, base + ((cab_off_t) strlen((char *) buf)) + 1, SEEK_SET);

  return (char *) buf;
}

/******************************************************************
 * FDI_read_entries (internal)
 *
 * process the cabinet header in the style of FDIIsCabinet, but
 * without the sanity checks (and bug)
 *
 * if pmii is non-null, some info not expressed in FDICABINETINFO struct
 * will be stored there... responsibility to free the enclosed stuff is
 * delegated to the caller in this case.
 */
BOOL FDI_read_entries(
	HFDI             hfdi,
	INT_PTR          hf,
	PFDICABINETINFO  pfdici,
	PMORE_ISCAB_INFO pmii)
{
  int num_folders, num_files, header_resv, folder_resv = 0;
  LONG base_offset, cabsize;
  USHORT setid, cabidx, flags;
  cab_UBYTE buf[64], block_resv;
  char *prevname = NULL, *previnfo = NULL, *nextname = NULL, *nextinfo = NULL;

  TRACE("(hfdi == ^%p, hf == %d, pfdici == ^%p)\n", hfdi, hf, pfdici);

  if (pmii) ZeroMemory(pmii, sizeof(MORE_ISCAB_INFO));

  /* get basic offset & size info */
  base_offset = FDI_getoffset(hfdi, hf);

  if (PFDI_SEEK(hfdi, hf, 0, SEEK_END) == -1) {
    PFDI_INT(hfdi)->perf->erfOper = FDIERROR_NOT_A_CABINET;
    PFDI_INT(hfdi)->perf->erfType = 0;
    PFDI_INT(hfdi)->perf->fError = TRUE;
    return FALSE;
  }

  cabsize = FDI_getoffset(hfdi, hf);

  if ((cabsize == -1) || (base_offset == -1) || 
      ( PFDI_SEEK(hfdi, hf, base_offset, SEEK_SET) == -1 )) {
    PFDI_INT(hfdi)->perf->erfOper = FDIERROR_NOT_A_CABINET;
    PFDI_INT(hfdi)->perf->erfType = 0;
    PFDI_INT(hfdi)->perf->fError = TRUE;
    return FALSE;
  }

  /* read in the CFHEADER */
  if (PFDI_READ(hfdi, hf, buf, cfhead_SIZEOF) != cfhead_SIZEOF) {
    PFDI_INT(hfdi)->perf->erfOper = FDIERROR_NOT_A_CABINET;
    PFDI_INT(hfdi)->perf->erfType = 0;
    PFDI_INT(hfdi)->perf->fError = TRUE;
    return FALSE;
  }
  
  /* check basic MSCF signature */
  if (EndGetI32(buf+cfhead_Signature) != 0x4643534d) {
    PFDI_INT(hfdi)->perf->erfOper = FDIERROR_NOT_A_CABINET;
    PFDI_INT(hfdi)->perf->erfType = 0;
    PFDI_INT(hfdi)->perf->fError = TRUE;
    return FALSE;
  }

  /* get the number of folders */
  num_folders = EndGetI16(buf+cfhead_NumFolders);
  if (num_folders == 0) {
    /* PONDERME: is this really invalid? */
    WARN("wierd cabinet detect failure: no folders in cabinet\n");
    PFDI_INT(hfdi)->perf->erfOper = FDIERROR_NOT_A_CABINET;
    PFDI_INT(hfdi)->perf->erfType = 0;
    PFDI_INT(hfdi)->perf->fError = TRUE;
    return FALSE;
  }

  /* get the number of files */
  num_files = EndGetI16(buf+cfhead_NumFiles);
  if (num_files == 0) {
    /* PONDERME: is this really invalid? */
    WARN("wierd cabinet detect failure: no files in cabinet\n");
    PFDI_INT(hfdi)->perf->erfOper = FDIERROR_NOT_A_CABINET;
    PFDI_INT(hfdi)->perf->erfType = 0;
    PFDI_INT(hfdi)->perf->fError = TRUE;
    return FALSE;
  }

  /* setid */
  setid = EndGetI16(buf+cfhead_SetID);

  /* cabinet (set) index */
  cabidx = EndGetI16(buf+cfhead_CabinetIndex);

  /* check the header revision */
  if ((buf[cfhead_MajorVersion] > 1) ||
      (buf[cfhead_MajorVersion] == 1 && buf[cfhead_MinorVersion] > 3))
  {
    WARN("cabinet format version > 1.3\n");
    PFDI_INT(hfdi)->perf->erfOper = FDIERROR_UNKNOWN_CABINET_VERSION;
    PFDI_INT(hfdi)->perf->erfType = 0; /* ? */
    PFDI_INT(hfdi)->perf->fError = TRUE;
    return FALSE;
  }

  /* pull the flags out */
  flags = EndGetI16(buf+cfhead_Flags);

  /* read the reserved-sizes part of header, if present */
  if (flags & cfheadRESERVE_PRESENT) {
    if (PFDI_READ(hfdi, hf, buf, cfheadext_SIZEOF) != cfheadext_SIZEOF) {
      ERR("bunk reserve-sizes?\n");
      PFDI_INT(hfdi)->perf->erfOper = FDIERROR_CORRUPT_CABINET;
      PFDI_INT(hfdi)->perf->erfType = 0; /* ? */
      PFDI_INT(hfdi)->perf->fError = TRUE;
      return FALSE;
    }

    header_resv = EndGetI16(buf+cfheadext_HeaderReserved);
    if (pmii) pmii->header_resv = header_resv;
    folder_resv = buf[cfheadext_FolderReserved];
    if (pmii) pmii->folder_resv = folder_resv;
    block_resv  = buf[cfheadext_DataReserved];

    if (header_resv > 60000) {
      WARN("WARNING; header reserved space > 60000\n");
    }

    /* skip the reserved header */
    if ((header_resv) && (PFDI_SEEK(hfdi, hf, header_resv, SEEK_CUR) == -1L)) {
      ERR("seek failure: header_resv\n");
      PFDI_INT(hfdi)->perf->erfOper = FDIERROR_CORRUPT_CABINET;
      PFDI_INT(hfdi)->perf->erfType = 0; /* ? */
      PFDI_INT(hfdi)->perf->fError = TRUE;
      return FALSE;
    }
  }

  if (flags & cfheadPREV_CABINET) {
    prevname = FDI_read_string(hfdi, hf, cabsize);
    if (!prevname) {
      PFDI_INT(hfdi)->perf->erfOper = FDIERROR_CORRUPT_CABINET;
      PFDI_INT(hfdi)->perf->erfType = 0; /* ? */
      PFDI_INT(hfdi)->perf->fError = TRUE;
      return FALSE;
    } else
      if (pmii)
        pmii->prevname = prevname;
      else
        PFDI_FREE(hfdi, prevname);
    previnfo = FDI_read_string(hfdi, hf, cabsize);
    if (previnfo) {
      if (pmii) 
        pmii->previnfo = previnfo;
      else
        PFDI_FREE(hfdi, previnfo);
    }
  }

  if (flags & cfheadNEXT_CABINET) {
    nextname = FDI_read_string(hfdi, hf, cabsize);
    if (!nextname) {
      if ((flags & cfheadPREV_CABINET) && pmii) {
        if (pmii->prevname) PFDI_FREE(hfdi, prevname);
        if (pmii->previnfo) PFDI_FREE(hfdi, previnfo);
      }
      PFDI_INT(hfdi)->perf->erfOper = FDIERROR_CORRUPT_CABINET;
      PFDI_INT(hfdi)->perf->erfType = 0; /* ? */
      PFDI_INT(hfdi)->perf->fError = TRUE;
      return FALSE;
    } else
      if (pmii)
        pmii->nextname = nextname;
      else
        PFDI_FREE(hfdi, nextname);
    nextinfo = FDI_read_string(hfdi, hf, cabsize);
    if (nextinfo) {
      if (pmii)
        pmii->nextinfo = nextinfo;
      else
        PFDI_FREE(hfdi, nextinfo);
    }
  }

  /* we could process the whole cabinet searching for problems;
     instead lets stop here.  Now let's fill out the paperwork */
  pfdici->cbCabinet = cabsize;
  pfdici->cFolders  = num_folders;
  pfdici->cFiles    = num_files;
  pfdici->setID     = setid;
  pfdici->iCabinet  = cabidx;
  pfdici->fReserve  = (flags & cfheadRESERVE_PRESENT) ? TRUE : FALSE;
  pfdici->hasprev   = (flags & cfheadPREV_CABINET) ? TRUE : FALSE;
  pfdici->hasnext   = (flags & cfheadNEXT_CABINET) ? TRUE : FALSE;
  return TRUE;
}

/***********************************************************************
 *		FDIIsCabinet (CABINET.21)
 */
BOOL __cdecl FDIIsCabinet(
	HFDI            hfdi,
	INT_PTR         hf,
	PFDICABINETINFO pfdici)
{
  BOOL rv;

  TRACE("(hfdi == ^%p, hf == ^%d, pfdici == ^%p)\n", hfdi, hf, pfdici);

  if (!REALLY_IS_FDI(hfdi)) {
    ERR("REALLY_IS_FDI failed on ^%p\n", hfdi);
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  if (!hf) {
    ERR("(!hf)!\n");
    PFDI_INT(hfdi)->perf->erfOper = FDIERROR_CABINET_NOT_FOUND;
    PFDI_INT(hfdi)->perf->erfType = ERROR_INVALID_HANDLE;
    PFDI_INT(hfdi)->perf->fError = TRUE;
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  if (!pfdici) {
    ERR("(!pfdici)!\n");
    PFDI_INT(hfdi)->perf->erfOper = FDIERROR_NONE;
    PFDI_INT(hfdi)->perf->erfType = ERROR_BAD_ARGUMENTS;
    PFDI_INT(hfdi)->perf->fError = TRUE;
    SetLastError(ERROR_BAD_ARGUMENTS);
    return FALSE;
  }
  rv = FDI_read_entries(hfdi, hf, pfdici, NULL); 

  if (rv)
    pfdici->hasnext = FALSE; /* yuck. duplicate apparent cabinet.dll bug */

  return rv;
}

/***********************************************************************
 *		FDICopy (CABINET.22)
 */
BOOL __cdecl FDICopy(
	HFDI           hfdi,
	char          *pszCabinet,
	char          *pszCabPath,
	int            flags,
	PFNFDINOTIFY   pfnfdin,
	PFNFDIDECRYPT  pfnfdid,
	void          *pvUser)
{ 
  FDICABINETINFO    fdici;
  FDINOTIFICATION   fdin;
  MORE_ISCAB_INFO   mii;
  int               hf, i, idx;
  char              fullpath[MAX_PATH];
  size_t            pathlen, filenamelen;
  char              emptystring = '\0';
  cab_UBYTE         buf[64];
  BOOL              initialcab = TRUE;
  struct fdi_folder *fol = NULL, *linkfol = NULL, *firstfol = NULL; 
  struct fdi_file   *file = NULL, *linkfile = NULL, *firstfile = NULL;
  struct fdi_cab    _cab;
  struct fdi_cab    *cab = &_cab;

  TRACE("(hfdi == ^%p, pszCabinet == ^%p, pszCabPath == ^%p, flags == %0d, \
        pfnfdin == ^%p, pfnfdid == ^%p, pvUser == ^%p)\n",
        hfdi, pszCabinet, pszCabPath, flags, pfnfdin, pfnfdid, pvUser);

  if (!REALLY_IS_FDI(hfdi)) {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  while (TRUE) { /* this loop executes one per. cabinet */
    pathlen = (pszCabPath) ? strlen(pszCabPath) : 0;
    filenamelen = (pszCabinet) ? strlen(pszCabinet) : 0;
  
    /* slight overestimation here to save CPU cycles in the developer's brain */
    if ((pathlen + filenamelen + 3) > MAX_PATH) {
      ERR("MAX_PATH exceeded.\n");
      PFDI_INT(hfdi)->perf->erfOper = FDIERROR_CABINET_NOT_FOUND;
      PFDI_INT(hfdi)->perf->erfType = ERROR_FILE_NOT_FOUND;
      PFDI_INT(hfdi)->perf->fError = TRUE;
      SetLastError(ERROR_FILE_NOT_FOUND);
      return FALSE;
    }
  
    /* paste the path and filename together */
    idx = 0;
    if (pathlen) {
      for (i = 0; i < pathlen; i++) fullpath[idx++] = pszCabPath[i];
      if (fullpath[idx - 1] != '\\') fullpath[idx++] = '\\';
    }
    if (filenamelen) for (i = 0; i < filenamelen; i++) fullpath[idx++] = pszCabinet[i];
    fullpath[idx] = '\0';
  
    TRACE("full cab path/file name: %s\n", debugstr_a(fullpath));
  
    /* get a handle to the cabfile */
    hf = PFDI_OPEN(hfdi, fullpath, _O_BINARY | _O_RDONLY | _O_SEQUENTIAL, 0);
    if (hf == -1) {
      PFDI_INT(hfdi)->perf->erfOper = FDIERROR_CABINET_NOT_FOUND;
      PFDI_INT(hfdi)->perf->erfType = ERROR_FILE_NOT_FOUND;
      PFDI_INT(hfdi)->perf->fError = TRUE;
      SetLastError(ERROR_FILE_NOT_FOUND);
      return FALSE;
    }
  
    /* check if it's really a cabfile. Note that this doesn't implement the bug */
    if (!FDI_read_entries(hfdi, hf, &fdici, &mii)) {
      ERR("FDIIsCabinet failed.\n");
      PFDI_CLOSE(hfdi, hf);
      return FALSE;
    }
     
    /* cabinet notification */
    ZeroMemory(&fdin, sizeof(FDINOTIFICATION));
    fdin.setID = fdici.setID;
    fdin.iCabinet = fdici.iCabinet;
    fdin.pv = pvUser;
    fdin.psz1 = (mii.nextname) ? mii.nextname : &emptystring;
    fdin.psz2 = (mii.nextinfo) ? mii.nextinfo : &emptystring;
    fdin.psz3 = pszCabPath;

    if (((*pfnfdin)(fdintCABINET_INFO, &fdin))) {
      PFDI_INT(hfdi)->perf->erfOper = FDIERROR_USER_ABORT;
      PFDI_INT(hfdi)->perf->erfType = 0;
      PFDI_INT(hfdi)->perf->fError = TRUE;
      goto bail_and_fail;
    }

    /* read folders */
    for (i = 0; i < fdici.cFolders; i++) {
      if (PFDI_READ(hfdi, hf, buf, cffold_SIZEOF) != cffold_SIZEOF) {
        PFDI_INT(hfdi)->perf->erfOper = FDIERROR_CORRUPT_CABINET;
	PFDI_INT(hfdi)->perf->erfType = 0;
	PFDI_INT(hfdi)->perf->fError = TRUE;
	goto bail_and_fail;
      }

      if (mii.folder_resv > 0)
        PFDI_SEEK(hfdi, hf, mii.folder_resv, SEEK_CUR);

      fol = (struct fdi_folder *) PFDI_ALLOC(hfdi, sizeof(struct fdi_folder));
      if (!fol) {
        ERR("out of memory!\n");
	PFDI_INT(hfdi)->perf->erfOper = FDIERROR_ALLOC_FAIL;
        PFDI_INT(hfdi)->perf->erfType = ERROR_NOT_ENOUGH_MEMORY;
	PFDI_INT(hfdi)->perf->fError = TRUE;
	SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	goto bail_and_fail;
      }
      ZeroMemory(fol, sizeof(struct fdi_folder));
      if (!firstfol) firstfol = fol;

      fol->cab[0]     = cab;
      fol->offset[0]  = (cab_off_t) EndGetI32(buf+cffold_DataOffset);
      fol->num_blocks = EndGetI16(buf+cffold_NumBlocks);
      fol->comp_type  = EndGetI16(buf+cffold_CompType);

      if (!linkfol)
        cab->folders = fol; 
      else 
        linkfol->next = fol; 

      linkfol = fol;
    }

    /* read files */
    for (i = 0; i < fdici.cFiles; i++) {
      if (PFDI_READ(hfdi, hf, buf, cffile_SIZEOF) != cffile_SIZEOF) {
        PFDI_INT(hfdi)->perf->erfOper = FDIERROR_CORRUPT_CABINET;
	PFDI_INT(hfdi)->perf->erfType = 0;
	PFDI_INT(hfdi)->perf->fError = TRUE;
	goto bail_and_fail;
      }

      file = (struct fdi_file *) PFDI_ALLOC(hfdi, sizeof(struct fdi_file));
      if (!file) { 
        ERR("out of memory!\n"); 
	PFDI_INT(hfdi)->perf->erfOper = FDIERROR_ALLOC_FAIL;
        PFDI_INT(hfdi)->perf->erfType = ERROR_NOT_ENOUGH_MEMORY;
	PFDI_INT(hfdi)->perf->fError = TRUE;
	SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	goto bail_and_fail;
      }
      ZeroMemory(file, sizeof(struct fdi_file));
      if (!firstfile) firstfile = file;
        
      file->length   = EndGetI32(buf+cffile_UncompressedSize);
      file->offset   = EndGetI32(buf+cffile_FolderOffset);
      file->index    = EndGetI16(buf+cffile_FolderIndex);
      file->time     = EndGetI16(buf+cffile_Time);
      file->date     = EndGetI16(buf+cffile_Date);
      file->attribs  = EndGetI16(buf+cffile_Attribs);
      file->filename = FDI_read_string(hfdi, hf, fdici.cbCabinet);
  
      if (!file->filename) {
        PFDI_INT(hfdi)->perf->erfOper = FDIERROR_CORRUPT_CABINET;
	PFDI_INT(hfdi)->perf->erfType = 0;
	PFDI_INT(hfdi)->perf->fError = TRUE;
	goto bail_and_fail;
      }
  
      if (!linkfile)
        cab->files = file;
      else 
        linkfile->next = file;
  
      linkfile = file;
    }

    /* partial file notification (do it just once for the first cabinet) */
    if (initialcab && (firstfile->attribs & cffileCONTINUED_FROM_PREV) && (fdici.iCabinet != 0)) {
      /* OK, more MS bugs to simulate here, I think.  I don't have a huge spanning
       * cabinet to test this theory on ATM, but here's the deal.  The SDK says that we
       * are supposed to notify the user of the filename and "disk name" (info) of
       * the cabinet where the spanning file /started/.  That would certainly be convenient
       * for the consumer, who could decide to abort everything and try to start over with
       * that cabinet so as not to create a front-truncated output file.  Note that this
       * task would be a horrible bitch from the implementor's (wine's) perspective: the
       * information is associated nowhere with the file header and is not to be found in
       * the cabinet header.  So we would have to open the previous cabinet, and check
       * if it contains a single spanning file that's continued from yet another prior cabinet,
       * and so-on, until we find the beginning.  Note that cabextract.c has code to do exactly
       * this.  Luckily, MS clearly didn't implement this logic, so we don't have to either.
       * Watching the callbacks (and debugmsg +file) clearly shows that they don't open
       * the preceeding cabinet -- and therefore, I deduce, there is NO WAY they could
       * have implemented what's in the spec.  Instead, they are obviously just returning
       * the previous cabinet and it's info from the header of this cabinet.  So we shall
       * do the same.  Of course, I could be missing something...
       */
      ZeroMemory(&fdin, sizeof(FDINOTIFICATION));
      fdin.pv = pvUser;
      fdin.psz1 = (char *)firstfile->filename;
      fdin.psz2 = (mii.prevname) ? mii.prevname : &emptystring;
      fdin.psz3 = (mii.previnfo) ? mii.previnfo : &emptystring;

      if (((*pfnfdin)(fdintPARTIAL_FILE, &fdin))) {
        PFDI_INT(hfdi)->perf->erfOper = FDIERROR_USER_ABORT;
        PFDI_INT(hfdi)->perf->erfType = 0;
        PFDI_INT(hfdi)->perf->fError = TRUE;
        goto bail_and_fail;
      }

    }


    while (firstfol) {
      fol = firstfol;
      firstfol = firstfol->next;
      PFDI_FREE(hfdi, fol);
    }
    while (firstfile) {
      file = firstfile;
      firstfile = firstfile->next;
      PFDI_FREE(hfdi, file);
    }

    /* free the storage remembered by mii */
    if (mii.nextname) PFDI_FREE(hfdi, mii.nextname);
    if (mii.nextinfo) PFDI_FREE(hfdi, mii.nextinfo);
    if (mii.prevname) PFDI_FREE(hfdi, mii.prevname);
    if (mii.previnfo) PFDI_FREE(hfdi, mii.previnfo);
  
    PFDI_CLOSE(hfdi, hf);
    /* TODO: if (:?) */ return TRUE; /* else { ...; initialcab=FALSE; continue; } */

    bail_and_fail: /* here we free ram before error returns */

    while (firstfol) {
      fol = firstfol;
      firstfol = firstfol->next;
      PFDI_FREE(hfdi, fol);
    }
    while (firstfile) {
      file = firstfile;
      firstfile = firstfile->next;
      PFDI_FREE(hfdi, file);
    }

    /* free the storage remembered by mii */
    if (mii.nextname) PFDI_FREE(hfdi, mii.nextname);
    if (mii.nextinfo) PFDI_FREE(hfdi, mii.nextinfo);
    if (mii.prevname) PFDI_FREE(hfdi, mii.prevname);
    if (mii.previnfo) PFDI_FREE(hfdi, mii.previnfo);
  
    PFDI_CLOSE(hfdi, hf);
    return FALSE;
  }
}

/***********************************************************************
 *		FDIDestroy (CABINET.23)
 */
BOOL __cdecl FDIDestroy(HFDI hfdi)
{
  TRACE("(hfdi == ^%p)\n", hfdi);
  if (REALLY_IS_FDI(hfdi)) {
    PFDI_INT(hfdi)->FDI_Intmagic = 0; /* paranoia */
    PFDI_FREE(hfdi, hfdi); /* confusing, but correct */
    return TRUE;
  } else {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }
}

/***********************************************************************
 *		FDITruncateCabinet (CABINET.24)
 */
BOOL __cdecl FDITruncateCabinet(
	HFDI    hfdi,
	char   *pszCabinetName,
	USHORT  iFolderToDelete)
{
  FIXME("(hfdi == ^%p, pszCabinetName == %s, iFolderToDelete == %hu): stub\n",
    hfdi, debugstr_a(pszCabinetName), iFolderToDelete);

  if (!REALLY_IS_FDI(hfdi)) {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
