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
#include "cabinet.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cabinet);

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
     neither can we.... */
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
 */
BOOL FDI_read_entries(
	HFDI            hfdi,
	INT_PTR         hf,
	PFDICABINETINFO pfdici)
{
  int num_folders, num_files, header_resv, folder_resv = 0;
  LONG base_offset, cabsize;
  USHORT setid, cabidx, flags;
  cab_UBYTE buf[64], block_resv;
  char *prevname, *previnfo, *nextname, *nextinfo;

  TRACE("(hfdi == ^%p, hf == %d, pfdici == ^%p)\n", hfdi, hf, pfdici);

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
  if (!PFDI_READ(hfdi, hf, buf, cfhead_SIZEOF)) {
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
    if (!PFDI_READ(hfdi, hf, buf, cfheadext_SIZEOF)) {
      WARN("bunk reserve-sizes?\n");
      PFDI_INT(hfdi)->perf->erfOper = FDIERROR_CORRUPT_CABINET;
      PFDI_INT(hfdi)->perf->erfType = 0; /* ? */
      PFDI_INT(hfdi)->perf->fError = TRUE;
      return FALSE;
    }

    header_resv = EndGetI16(buf+cfheadext_HeaderReserved);
    folder_resv = buf[cfheadext_FolderReserved];
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
    }
    previnfo = FDI_read_string(hfdi, hf, cabsize);
  }

  if (flags & cfheadNEXT_CABINET) {
    nextname = FDI_read_string(hfdi, hf, cabsize);
    if (!nextname) {
      PFDI_INT(hfdi)->perf->erfOper = FDIERROR_CORRUPT_CABINET;
      PFDI_INT(hfdi)->perf->erfType = 0; /* ? */
      PFDI_INT(hfdi)->perf->fError = TRUE;
      return FALSE;
    }
    nextinfo = FDI_read_string(hfdi, hf, cabsize);
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
  rv = FDI_read_entries(hfdi, hf, pfdici); 

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
  FIXME("(hfdi == ^%p, pszCabinet == ^%p, pszCabPath == ^%p, flags == %0d, \
        pfnfdin == ^%p, pfnfdid == ^%p, pvUser == ^%p): stub\n",
        hfdi, pszCabinet, pszCabPath, flags, pfnfdin, pfnfdid, pvUser);

  if (!REALLY_IS_FDI(hfdi)) {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
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
