/*
 *	icon extracting
 *
 * taken and slightly changed from shell
 * this should replace the icon extraction code in shell32 and shell16 once
 * it needs a serious test for compliance with the native API
 *
 * Copyright 2000 Juergen Schmied
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
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>	/* abs() */
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "winbase.h"
#include "windef.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winbase16.h"
#include "cursoricon.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(icon);

#include "pshpack1.h"

typedef struct
{
    BYTE        bWidth;          /* Width, in pixels, of the image	*/
    BYTE        bHeight;         /* Height, in pixels, of the image	*/
    BYTE        bColorCount;     /* Number of colors in image (0 if >=8bpp) */
    BYTE        bReserved;       /* Reserved ( must be 0)		*/
    WORD        wPlanes;         /* Color Planes			*/
    WORD        wBitCount;       /* Bits per pixel			*/
    DWORD       dwBytesInRes;    /* How many bytes in this resource?	*/
    DWORD       dwImageOffset;   /* Where in the file is this image?	*/
} icoICONDIRENTRY, *LPicoICONDIRENTRY;

typedef struct
{
    WORD            idReserved;   /* Reserved (must be 0) */
    WORD            idType;       /* Resource Type (RES_ICON or RES_CURSOR) */
    WORD            idCount;      /* How many images */
    icoICONDIRENTRY idEntries[1]; /* An entry for each image (idCount of 'em) */
} icoICONDIR, *LPicoICONDIR;

#include "poppack.h"

#if 0
static void dumpIcoDirEnty ( LPicoICONDIRENTRY entry )
{
	TRACE("width = 0x%08x height = 0x%08x\n", entry->bWidth, entry->bHeight);
	TRACE("colors = 0x%08x planes = 0x%08x\n", entry->bColorCount, entry->wPlanes);
	TRACE("bitcount = 0x%08x bytesinres = 0x%08lx offset = 0x%08lx\n",
	entry->wBitCount, entry->dwBytesInRes, entry->dwImageOffset);
}
static void dumpIcoDir ( LPicoICONDIR entry )
{
	TRACE("type = 0x%08x count = 0x%08x\n", entry->idType, entry->idCount);
}
#endif

/**********************************************************************
 *  find_entry_by_id
 *
 * Find an entry by id in a resource directory
 * Copied from loader/pe_resource.c (FIXME: should use exported resource functions)
 */
static const IMAGE_RESOURCE_DIRECTORY *find_entry_by_id( const IMAGE_RESOURCE_DIRECTORY *dir,
                                                         WORD id, const void *root )
{
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;
    int min, max, pos;

    entry = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    min = dir->NumberOfNamedEntries;
    max = min + dir->NumberOfIdEntries - 1;
    while (min <= max)
    {
        pos = (min + max) / 2;
        if (entry[pos].u1.s2.Id == id)
            return (IMAGE_RESOURCE_DIRECTORY *)((char *)root + entry[pos].u2.s3.OffsetToDirectory);
        if (entry[pos].u1.s2.Id > id) max = pos - 1;
        else min = pos + 1;
    }
    return NULL;
}

/**********************************************************************
 *  find_entry_default
 *
 * Find a default entry in a resource directory
 * Copied from loader/pe_resource.c (FIXME: should use exported resource functions)
 */
static const IMAGE_RESOURCE_DIRECTORY *find_entry_default( const IMAGE_RESOURCE_DIRECTORY *dir,
                                                           const void *root )
{
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;
    entry = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    return (IMAGE_RESOURCE_DIRECTORY *)((char *)root + entry->u2.s3.OffsetToDirectory);
}

/*************************************************************************
 *				USER32_GetResourceTable
 */
static DWORD USER32_GetResourceTable(LPBYTE peimage,DWORD pesize,LPBYTE *retptr)
{
	IMAGE_DOS_HEADER	* mz_header;

	TRACE("%p %p\n", peimage, retptr);

	*retptr = NULL;

	mz_header = (IMAGE_DOS_HEADER*) peimage;

	if (mz_header->e_magic != IMAGE_DOS_SIGNATURE)
	{
	  if (mz_header->e_cblp == 1)	/* .ICO file ? */
	  {
	    *retptr = (LPBYTE)-1;	/* ICONHEADER.idType, must be 1 */
	    return 1;
	  }
	  else
	    return 0; /* failed */
	}
	if (mz_header->e_lfanew >= pesize) {
	    return 0; /* failed, happens with PKZIP DOS Exes for instance. */
	}
	if (*((DWORD*)(peimage + mz_header->e_lfanew)) == IMAGE_NT_SIGNATURE )
	  return IMAGE_NT_SIGNATURE;

	if (*((WORD*)(peimage + mz_header->e_lfanew)) == IMAGE_OS2_SIGNATURE )
	{
	  IMAGE_OS2_HEADER	* ne_header;

	  ne_header = (IMAGE_OS2_HEADER*)(peimage + mz_header->e_lfanew);

	  if (ne_header->ne_magic != IMAGE_OS2_SIGNATURE)
	    return 0;

	  if( (ne_header->ne_restab - ne_header->ne_rsrctab) <= sizeof(NE_TYPEINFO) )
	    *retptr = (LPBYTE)-1;
	  else
	    *retptr = peimage + mz_header->e_lfanew + ne_header->ne_rsrctab;

	  return IMAGE_OS2_SIGNATURE;
	}
	return 0; /* failed */
}
/*************************************************************************
 *			USER32_LoadResource
 */
static BYTE * USER32_LoadResource( LPBYTE peimage, NE_NAMEINFO* pNInfo, WORD sizeShift, ULONG *uSize)
{
	TRACE("%p %p 0x%08x\n", peimage, pNInfo, sizeShift);

	*uSize = (DWORD)pNInfo->length << sizeShift;
	return peimage + ((DWORD)pNInfo->offset << sizeShift);
}

/*************************************************************************
 *                      ICO_LoadIcon
 */
static BYTE * ICO_LoadIcon( LPBYTE peimage, LPicoICONDIRENTRY lpiIDE, ULONG *uSize)
{
	TRACE("%p %p\n", peimage, lpiIDE);

	*uSize = lpiIDE->dwBytesInRes;
	return peimage + lpiIDE->dwImageOffset;
}

/*************************************************************************
 *                      ICO_GetIconDirectory
 *
 * Reads .ico file and build phony ICONDIR struct
 * see http://www.microsoft.com/win32dev/ui/icons.htm
 */
#define HEADER_SIZE		(sizeof(CURSORICONDIR) - sizeof (CURSORICONDIRENTRY))
#define HEADER_SIZE_FILE	(sizeof(icoICONDIR) - sizeof (icoICONDIRENTRY))

static BYTE * ICO_GetIconDirectory( LPBYTE peimage, LPicoICONDIR* lplpiID, ULONG *uSize )
{
	CURSORICONDIR	* lpcid;	/* icon resource in resource-dir format */
	CURSORICONDIR	* lpID;		/* icon resource in resource format */
	int		i;

	TRACE("%p %p\n", peimage, lplpiID);

	lpcid = (CURSORICONDIR*)peimage;

	if( lpcid->idReserved || (lpcid->idType != 1) || (!lpcid->idCount) )
	  return 0;

	/* allocate the phony ICONDIR structure */
	*uSize = lpcid->idCount * sizeof(CURSORICONDIRENTRY) + HEADER_SIZE;
	if( (lpID = (CURSORICONDIR*)HeapAlloc(GetProcessHeap(),0, *uSize) ))
	{
	  /* copy the header */
	  lpID->idReserved = lpcid->idReserved;
	  lpID->idType = lpcid->idType;
	  lpID->idCount = lpcid->idCount;

	  /* copy the entries */
	  for( i=0; i < lpcid->idCount; i++ )
	  {
	    memcpy((void*)&(lpID->idEntries[i]),(void*)&(lpcid->idEntries[i]), sizeof(CURSORICONDIRENTRY) - 2);
	    lpID->idEntries[i].wResId = i;
	  }

	  *lplpiID = (LPicoICONDIR)peimage;
	  return (BYTE *)lpID;
	}
	return 0;
}

/*************************************************************************
 *	ICO_ExtractIconExW		[internal]
 *
 * NOTES
 *  nIcons = 0: returns number of Icons in file
 *
 * returns
 *  failure:0; success: number of icons in file (nIcons = 0) or nr of icons retrieved
 */
static UINT ICO_ExtractIconExW(
	LPCWSTR lpszExeFileName,
	HICON * RetPtr,
	INT nIconIndex,
	UINT nIcons,
	UINT cxDesired,
	UINT cyDesired,
	UINT *pIconId,
	UINT flags)
{
	UINT		ret = 0;
	LPBYTE		pData;
	DWORD		sig;
	HANDLE		hFile;
	UINT16		iconDirCount = 0,iconCount = 0;
	LPBYTE		peimage;
	HANDLE		fmapping;
	ULONG		uSize;
	DWORD		fsizeh,fsizel;

	TRACE("%s, %d, %d %p 0x%08x\n", debugstr_w(lpszExeFileName), nIconIndex, nIcons, pIconId, flags);

	hFile = CreateFileW( lpszExeFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
	if (hFile == INVALID_HANDLE_VALUE) return ret;
	fsizel = GetFileSize(hFile,&fsizeh);

	/* Map the file */
	fmapping = CreateFileMappingW( hFile, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL );
	CloseHandle( hFile );
	if (!fmapping)
	{
	  WARN("CreateFileMapping error %ld\n", GetLastError() );
	  return ret;
	}

	if ( !(peimage = MapViewOfFile(fmapping,FILE_MAP_READ,0,0,0)))
	{
	  WARN("MapViewOfFile error %ld\n", GetLastError() );
          CloseHandle( fmapping );
	  return ret;
	}
	CloseHandle( fmapping );

	sig = USER32_GetResourceTable(peimage,fsizel,&pData);

/* ico file */
	if( sig==IMAGE_OS2_SIGNATURE || sig==1 ) /* .ICO file */
	{
	  BYTE		*pCIDir = 0;
	  NE_TYPEINFO	*pTInfo = (NE_TYPEINFO*)(pData + 2);
	  NE_NAMEINFO	*pIconStorage = NULL;
	  NE_NAMEINFO	*pIconDir = NULL;
	  LPicoICONDIR	lpiID = NULL;

	  TRACE("-- OS2/icon Signature (0x%08lx)\n", sig);

	  if( pData == (BYTE*)-1 )
	  {
	    /* FIXME: pCIDir is allocated on the heap - memory leak */
	    pCIDir = ICO_GetIconDirectory(peimage, &lpiID, &uSize);	/* check for .ICO file */
	    if( pCIDir )
	    {
	      iconDirCount = 1; iconCount = lpiID->idCount;
	      TRACE("-- icon found %p 0x%08lx 0x%08x 0x%08x\n", pCIDir, uSize, iconDirCount, iconCount);
	    }
	  }
	  else while( pTInfo->type_id && !(pIconStorage && pIconDir) )
	  {
	    if( pTInfo->type_id == NE_RSCTYPE_GROUP_ICON )	/* find icon directory and icon repository */
	    {
	      iconDirCount = pTInfo->count;
	      pIconDir = ((NE_NAMEINFO*)(pTInfo + 1));
	      TRACE("\tfound directory - %i icon families\n", iconDirCount);
	    }
	    if( pTInfo->type_id == NE_RSCTYPE_ICON )
	    {
	      iconCount = pTInfo->count;
	      pIconStorage = ((NE_NAMEINFO*)(pTInfo + 1));
	      TRACE("\ttotal icons - %i\n", iconCount);
	    }
	    pTInfo = (NE_TYPEINFO *)((char*)(pTInfo+1)+pTInfo->count*sizeof(NE_NAMEINFO));
	  }

	  if( (pIconStorage && pIconDir) || lpiID )	  /* load resources and create icons */
	  {
	    if( nIcons == 0 )
	    {
	      ret = iconDirCount;
	    }
	    else if( nIconIndex < iconDirCount )
	    {
	      UINT16   i, icon;
	      if( nIcons > iconDirCount - nIconIndex )
	        nIcons = iconDirCount - nIconIndex;

	      for( i = nIconIndex; i < nIconIndex + nIcons; i++ )
	      {
	        /* .ICO files have only one icon directory */
	        if( lpiID == NULL )	/* *.ico */
	          pCIDir = USER32_LoadResource( peimage, pIconDir + i, *(WORD*)pData, &uSize );
	        RetPtr[i-nIconIndex] = (HICON)LookupIconIdFromDirectoryEx( pCIDir, TRUE, cxDesired, cyDesired, flags );
	      }

	      for( icon = nIconIndex; icon < nIconIndex + nIcons; icon++ )
	      {
	        pCIDir = NULL;
	        if( lpiID )
	          pCIDir = ICO_LoadIcon( peimage, lpiID->idEntries + (int)RetPtr[icon-nIconIndex], &uSize);
	        else
	          for ( i = 0; i < iconCount; i++ )
	            if ( pIconStorage[i].id == ((int)RetPtr[icon-nIconIndex] | 0x8000) )
	              pCIDir = USER32_LoadResource( peimage, pIconStorage + i,*(WORD*)pData, &uSize );

	        if( pCIDir )
	          RetPtr[icon - nIconIndex] = (HICON)CreateIconFromResourceEx(pCIDir, uSize, TRUE, 0x00030000, cxDesired, cyDesired, flags);
	        else
	          RetPtr[icon - nIconIndex] = 0;
	      }
	      ret = icon - nIconIndex;	/* return number of retrieved icons */
	    }
	  }
	}
/* end ico file */

/* exe/dll */
	else if( sig == IMAGE_NT_SIGNATURE )
	{
	  LPBYTE		idata,igdata;
	  PIMAGE_DOS_HEADER	dheader;
	  PIMAGE_NT_HEADERS	pe_header;
	  PIMAGE_SECTION_HEADER	pe_sections;
	  const IMAGE_RESOURCE_DIRECTORY *rootresdir,*iconresdir,*icongroupresdir;
	  const IMAGE_RESOURCE_DATA_ENTRY *idataent,*igdataent;
	  const IMAGE_RESOURCE_DIRECTORY_ENTRY *xresent;
	  int			i,j;

	  dheader = (PIMAGE_DOS_HEADER)peimage;
	  pe_header = (PIMAGE_NT_HEADERS)(peimage+dheader->e_lfanew);	  /* it is a pe header, USER32_GetResourceTable checked that */
	  pe_sections = (PIMAGE_SECTION_HEADER)(((char*)pe_header)+sizeof(*pe_header));	/* probably makes problems with short PE headers...*/
	  rootresdir = NULL;

	  /* search for the root resource directory */
	  for (i=0;i<pe_header->FileHeader.NumberOfSections;i++)
	  {
	    if (pe_sections[i].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
	      continue;
	    if (fsizel < pe_sections[i].PointerToRawData+pe_sections[i].SizeOfRawData) {
	      FIXME("File %s too short (section is at %ld bytes, real size is %ld)\n",
		      debugstr_w(lpszExeFileName),
		      pe_sections[i].PointerToRawData+pe_sections[i].SizeOfRawData,
		      fsizel
	      );
	      goto end;
	    }
	    /* FIXME: doesn't work when the resources are not in a separate section */
	    if (pe_sections[i].VirtualAddress == pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress)
	    {
	      rootresdir = (PIMAGE_RESOURCE_DIRECTORY)(peimage+pe_sections[i].PointerToRawData);
	      break;
	    }
	  }

	  if (!rootresdir)
	  {
	    WARN("haven't found section for resource directory.\n");
	    goto end;		/* failure */
	  }

	  /* search for the group icon directory */
	  if (!(icongroupresdir = find_entry_by_id(rootresdir, LOWORD(RT_GROUP_ICONW), rootresdir)))
	  {
	    WARN("No Icongroupresourcedirectory!\n");
	    goto end;		/* failure */
	  }
	  iconDirCount = icongroupresdir->NumberOfNamedEntries + icongroupresdir->NumberOfIdEntries;

	  /* only number of icons requested */
	  if( nIcons == 0 )
	  {
	    ret = iconDirCount;
	    goto end;		/* success */
	  }

	  if( nIconIndex < 0 )
	  {
	    /* search resource id */
	    int n = 0;
	    int iId = abs(nIconIndex);
	    PIMAGE_RESOURCE_DIRECTORY_ENTRY xprdeTmp = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(icongroupresdir+1);

	    while(n<iconDirCount && xprdeTmp)
	    {
              if(xprdeTmp->u1.s2.Id ==  iId)
              {
                  nIconIndex = n;
                  break;
              }
              n++;
              xprdeTmp++;
	    }
	    if (nIconIndex < 0)
	    {
	      WARN("resource id %d not found\n", iId);
	      goto end;		/* failure */
	    }
	  }
	  else
	  {
	    /* check nIconIndex to be in range */
	    if (nIconIndex >= iconDirCount)
	    {
	      WARN("nIconIndex %d is larger than iconDirCount %d\n",nIconIndex,iconDirCount);
	      goto end;		/* failure */
	    }
	  }

	  /* assure we don't get too much */
	  if( nIcons > iconDirCount - nIconIndex )
	    nIcons = iconDirCount - nIconIndex;

	  /* starting from specified index */
	  xresent = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(icongroupresdir+1) + nIconIndex;

	  for (i=0; i < nIcons; i++,xresent++)
	  {
              const IMAGE_RESOURCE_DIRECTORY *resdir;

	    /* go down this resource entry, name */
	    resdir = (PIMAGE_RESOURCE_DIRECTORY)((DWORD)rootresdir+(xresent->u2.s3.OffsetToDirectory));

	    /* default language (0) */
	    resdir = find_entry_default(resdir,rootresdir);
	    igdataent = (PIMAGE_RESOURCE_DATA_ENTRY)resdir;

	    /* lookup address in mapped image for virtual address */
	    igdata = NULL;

	    for (j=0;j<pe_header->FileHeader.NumberOfSections;j++)
	    {
	      if (igdataent->OffsetToData < pe_sections[j].VirtualAddress)
	        continue;
	      if (igdataent->OffsetToData+igdataent->Size > pe_sections[j].VirtualAddress+pe_sections[j].SizeOfRawData)
	        continue;

	      if (igdataent->OffsetToData-pe_sections[j].VirtualAddress+pe_sections[j].PointerToRawData+igdataent->Size > fsizel) {
	        FIXME("overflow in PE lookup (%s has len %ld, have offset %ld), short file?\n", debugstr_w(lpszExeFileName), fsizel,
	        	   igdataent->OffsetToData - pe_sections[j].VirtualAddress + pe_sections[j].PointerToRawData + igdataent->Size);
	        goto end; /* failure */
	      }
	      igdata = peimage+(igdataent->OffsetToData-pe_sections[j].VirtualAddress+pe_sections[j].PointerToRawData);
	    }

	    if (!igdata)
	    {
	      FIXME("no matching real address for icongroup!\n");
	      goto end;	/* failure */
	    }
	    RetPtr[i] = (HICON)LookupIconIdFromDirectoryEx(igdata, TRUE, cxDesired, cyDesired, flags);
	  }

	  if (pIconId)
	  	*pIconId = LOWORD(*RetPtr);
	  	 
	  if (!(iconresdir=find_entry_by_id(rootresdir,LOWORD(RT_ICONW),rootresdir)))
	  {
	    WARN("No Iconresourcedirectory!\n");
	    goto end;		/* failure */
	  }

	  for (i=0; i<nIcons; i++)
	  {
        const IMAGE_RESOURCE_DIRECTORY *xresdir;
        xresdir = find_entry_by_id(iconresdir,LOWORD(RetPtr[i]),rootresdir);
        xresdir = find_entry_default(xresdir,rootresdir);
	    idataent = (PIMAGE_RESOURCE_DATA_ENTRY)xresdir;
	    idata = NULL;

	    /* map virtual to address in image */
	    for (j=0;j<pe_header->FileHeader.NumberOfSections;j++)
	    {
	      if (idataent->OffsetToData < pe_sections[j].VirtualAddress)
	        continue;
	      if (idataent->OffsetToData+idataent->Size > pe_sections[j].VirtualAddress+pe_sections[j].SizeOfRawData)
	        continue;
	      idata = peimage+(idataent->OffsetToData-pe_sections[j].VirtualAddress+pe_sections[j].PointerToRawData);
	    }
	    if (!idata)
	    {
	      WARN("no matching real address found for icondata!\n");
	      RetPtr[i]=0;
	      continue;
	    }
	    RetPtr[i] = (HICON) CreateIconFromResourceEx(idata,idataent->Size,TRUE,0x00030000, cxDesired, cyDesired, flags);
	  }
	  ret = i;	/* return number of retrieved icons */
	}			/* if(sig == IMAGE_NT_SIGNATURE) */

end:
	UnmapViewOfFile(peimage);	/* success */
	return ret;
}

/***********************************************************************
 *           PrivateExtractIconsW			[USER32.@]
 *
 * NOTES
 *  nIndex = 1: a small and a large icon are extracted.
 *  the higher word of sizeXY contains the size of the small icon, the lower
 *  word the size of the big icon. phicon points to HICON[2].
 *
 * FIXME:
 * 1) should also support 16 bit EXE + DLLs, cursor and animated cursor as well
 *    as bitmap files.
 * 2) should return according to MSDN
 *  phicons == NULL	: the number of icons in file or 0 on error
 *  phicons != NULL	: the number of icons extracted or 0xFFFFFFFF on error
 * 	  does return in Win2000
 *  when file valid the number of icons or 0 on any error
 *  when file invalid and phicon == NULL returns 0
 *  when file invalid and phicon != NULL returns 0xFFFFFFFF
 *
 * *pIconID is always set to 0 when file invalid
 * *pIconID is always set to 0xFFFFFFFF for valid icon and cursor files
 * *pIconID is set to actual identifier for valid dll/exes or -1 on error
 */

UINT WINAPI PrivateExtractIconsW (
	LPCWSTR lpwstrFile,
	int nIndex,
	int sizeX,
	int sizeY,
	HICON * phicon,	/* [out] pointer to array of HICON handles */
	UINT* pIconId,	/* [out] pointer to returned icon identifier which fits best */
	UINT nIcons,    /* [in] number of icons to retrieve */
	UINT flags )	/* [in] LR_* flags used by LoadImage */
{
    UINT ret;
    TRACE("%s %d %dx%d %p %p %d 0x%08x\n",
          debugstr_w(lpwstrFile), nIndex, sizeX, sizeY, phicon, pIconId, nIcons, flags);

	if (pIconId) /* Invalidate icon identifier on entry */
		*pIconId = 0xFFFFFFFF;
		
    if (!phicon)
      return ICO_ExtractIconExW(lpwstrFile, NULL, nIndex, 0, sizeX & 0xffff, sizeY & 0xffff, pIconId, flags);
		
    if ((nIcons == 2) && HIWORD(sizeX) && HIWORD(sizeY))
    {
      ret = ICO_ExtractIconExW(lpwstrFile, phicon, nIndex, 1, sizeX & 0xffff, sizeY & 0xffff, pIconId, flags);
      if (!SUCCEEDED(ret)) return ret;
      ret = ICO_ExtractIconExW(lpwstrFile, phicon+1, nIndex, 1, (sizeX>>16) & 0xffff, (sizeY>>16) & 0xffff, pIconId, flags);
    } else
      ret = ICO_ExtractIconExW(lpwstrFile, phicon, nIndex, nIcons, sizeX & 0xffff, sizeY & 0xffff, pIconId, flags);
    return ret;
}

/***********************************************************************
 *           PrivateExtractIconsA			[USER32.@]
 */

UINT WINAPI PrivateExtractIconsA (
	LPCSTR lpstrFile,
	int nIndex,
	int sizeX,
	int sizeY,
	HICON * phicon,
	UINT* piconid,  /* [out] pointer to returned icon identifier which fits best */
	UINT nIcons,    /* [in] number of icons to retrieve */
	UINT flags )    /* [in] LR_* flags used by LoadImage */
{
    UINT ret;
    INT len = MultiByteToWideChar(CP_ACP, 0, lpstrFile, -1, NULL, 0);
    LPWSTR lpwstrFile = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));

    MultiByteToWideChar(CP_ACP, 0, lpstrFile, -1, lpwstrFile, len);
    ret = PrivateExtractIconsW(lpwstrFile, nIndex, sizeX, sizeY, phicon, piconid, nIcons, flags);

    HeapFree(GetProcessHeap(), 0, lpwstrFile);
    return ret;
}

/***********************************************************************
 *           PrivateExtractIconExW			[USER32.@]
 * NOTES
 *  if nIcons = -1 it returns the number of icons in any case !!!
 */
HRESULT WINAPI PrivateExtractIconExW (
	LPCWSTR lpwstrFile,
	DWORD nIndex,
	HICON * phIconLarge,
	HICON * phIconSmall,
	UINT nIcons )
{
	DWORD cyicon, cysmicon, cxicon, cxsmicon;
	HRESULT ret = 0;

	TRACE("%s 0x%08lx %p %p 0x%08x\n",
	debugstr_w(lpwstrFile),nIndex,phIconLarge, phIconSmall, nIcons);

	if (nIndex == 1 && phIconSmall && phIconLarge)
	{
	  HICON hIcon[2];
	  cxicon = GetSystemMetrics(SM_CXICON);
	  cyicon = GetSystemMetrics(SM_CYICON);
	  cxsmicon = GetSystemMetrics(SM_CXSMICON);
	  cysmicon = GetSystemMetrics(SM_CYSMICON);

	  ret = PrivateExtractIconsW ( lpwstrFile, nIndex, cxicon | (cxsmicon<<16),  cyicon | (cysmicon<<16),
	  (HICON*) &hIcon, 0, 2, 0 );
	  *phIconLarge = hIcon[0];
	  *phIconSmall = hIcon[1];
 	  return ret;
	}

	if (nIndex != -1)
	{
	  if (phIconSmall)
	  {
	    /* extract n small icons */
	    cxsmicon = GetSystemMetrics(SM_CXSMICON);
	    cysmicon = GetSystemMetrics(SM_CYSMICON);
	    ret = PrivateExtractIconsW ( lpwstrFile, nIndex, cxsmicon, cysmicon, phIconSmall, 0, nIcons, 0 );
	  }
	  if (phIconLarge )
	  {
	    /* extract n large icons */
	    cxicon = GetSystemMetrics(SM_CXICON);
	    cyicon = GetSystemMetrics(SM_CYICON);
	    ret = PrivateExtractIconsW ( lpwstrFile, nIndex, cxicon, cyicon, phIconLarge, 0, nIcons, 0 );
	  }
	  return ret;
	}

	/* get the number of icons */
	return PrivateExtractIconsW ( lpwstrFile, 0, 0, 0, 0, 0, 0, 0 );
}

/***********************************************************************
 *           PrivateExtractIconExA			[USER32.@]
 */
HRESULT WINAPI PrivateExtractIconExA (
	LPCSTR lpstrFile,
	DWORD nIndex,
	HICON * phIconLarge,
	HICON * phIconSmall,
	UINT nIcons )
{
	DWORD ret;
        INT len = MultiByteToWideChar( CP_ACP, 0, lpstrFile, -1, NULL, 0 );
        LPWSTR lpwstrFile = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );

	TRACE("%s 0x%08lx %p %p 0x%08x\n", lpstrFile, nIndex, phIconLarge, phIconSmall, nIcons);

        MultiByteToWideChar( CP_ACP, 0, lpstrFile, -1, lpwstrFile, len );
	ret = PrivateExtractIconExW(lpwstrFile,nIndex,phIconLarge, phIconSmall, nIcons);
	HeapFree(GetProcessHeap(), 0, lpwstrFile);
	return ret;
}
