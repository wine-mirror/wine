/*
 *	icon extracting
 *
 * taken and slightly changed from shell
 * this should replace the icon extraction code in shell32 and shell16 once
 * it needs a serious test for compliance with the native API 
 */
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "winbase.h"
#include "windef.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"
#include "neexe.h"
#include "cursoricon.h"
#include "module.h"
#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(icon)

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
/*************************************************************************
 *				USER32_GetResourceTable
 */
static DWORD USER32_GetResourceTable(LPBYTE peimage, LPBYTE *retptr)
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

	  /* copy the entrys */
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
 *  failure:0; success: icon handle or nr of icons (nIconIndex-1)
 */
static HRESULT WINAPI ICO_ExtractIconExW(
	LPCWSTR lpszExeFileName,
	HICON * RetPtr,
	INT nIconIndex,
	UINT nIcons,
	UINT cxDesired,
	UINT cyDesired )
{
	HGLOBAL		hRet = E_FAIL;
	LPBYTE		pData;
	DWORD		sig;
	HFILE		hFile;
	UINT16		iconDirCount = 0,iconCount = 0;
	LPBYTE		peimage;
	HANDLE		fmapping;
	ULONG		uSize;
	
	TRACE("(file %s,start %d,extract %d\n", debugstr_w(lpszExeFileName), nIconIndex, nIcons);

	hFile = CreateFileW( lpszExeFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, -1 );
	if (hFile == INVALID_HANDLE_VALUE) return hRet;

	/* Map the file */
	fmapping = CreateFileMappingA( hFile, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL );
	if (!fmapping)
	{
	  WARN("CreateFileMapping error %ld\n", GetLastError() );
	  goto end_1;
	}

	if ( !(peimage = MapViewOfFile(fmapping,FILE_MAP_READ,0,0,0))) 
	{
	  WARN("MapViewOfFile error %ld\n", GetLastError() );
	  goto end_2;
	}


	sig = USER32_GetResourceTable(peimage,&pData);

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
	      hRet = iconDirCount;
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
	        RetPtr[i-nIconIndex] = LookupIconIdFromDirectoryEx( pCIDir, TRUE, cxDesired, cyDesired, 0);
	      }

	      for( icon = nIconIndex; icon < nIconIndex + nIcons; icon++ )
	      {
	        pCIDir = NULL;
	        if( lpiID )
	          pCIDir = ICO_LoadIcon( peimage, lpiID->idEntries + RetPtr[icon-nIconIndex], &uSize);
	        else
		  for( i = 0; i < iconCount; i++ )
		    if( pIconStorage[i].id == (RetPtr[icon-nIconIndex] | 0x8000) )
		      pCIDir = USER32_LoadResource( peimage, pIconStorage + i,*(WORD*)pData, &uSize );

	        if( pCIDir )
		  RetPtr[icon-nIconIndex] = (HICON) CreateIconFromResourceEx(pCIDir,uSize,TRUE,0x00030000, cxDesired, cyDesired, LR_DEFAULTCOLOR);
	        else
		  RetPtr[icon-nIconIndex] = 0;
	      }
	      hRet = S_OK;
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
	  PIMAGE_RESOURCE_DIRECTORY	rootresdir,iconresdir,icongroupresdir;
	  PIMAGE_RESOURCE_DATA_ENTRY	idataent,igdataent;
	  PIMAGE_RESOURCE_DIRECTORY_ENTRY	xresent;
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
	    /* FIXME: doesn't work when the resources are not in a seperate section */
	    if (pe_sections[i].VirtualAddress == pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress) 
	    {
	      rootresdir = (PIMAGE_RESOURCE_DIRECTORY)(peimage+pe_sections[i].PointerToRawData);
	      break;
	    }
	  }

	  if (!rootresdir) 
	  {
	    WARN("haven't found section for resource directory.\n");
	    goto end_3;		/* failure */
	  }

	  /* search for the group icon directory */
	  if (!(icongroupresdir = GetResDirEntryW(rootresdir, RT_GROUP_ICONW, (DWORD)rootresdir, FALSE))) 
	  {
	    WARN("No Icongroupresourcedirectory!\n");
	    goto end_3;		/* failure */
	  }
	  iconDirCount = icongroupresdir->NumberOfNamedEntries + icongroupresdir->NumberOfIdEntries;

	  /* only number of icons requested */
	  if( nIcons == 0 )
	  {
	    hRet = iconDirCount;
	    goto end_3;		/* success */
	  }

	  if( nIconIndex < 0 )
	  {
	    /* search resource id */
	    int n = 0;
	    int iId = abs(nIconIndex);
	    PIMAGE_RESOURCE_DIRECTORY_ENTRY xprdeTmp = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(icongroupresdir+1);

	    while(n<iconDirCount && xprdeTmp)
	    {              
              if(xprdeTmp->u1.Id ==  iId)
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
	      goto end_3;		/* failure */
	    }
	  }
	  else
	  {
	    /* check nIconIndex to be in range */
	    if (nIconIndex >= iconDirCount) 
	    {
	      WARN("nIconIndex %d is larger than iconDirCount %d\n",nIconIndex,iconDirCount);
	      goto end_3;		/* failure */
	    }
	  }

	  /* assure we don't get too much */
	  if( nIcons > iconDirCount - nIconIndex )
	    nIcons = iconDirCount - nIconIndex;

	  /* starting from specified index */
	  xresent = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(icongroupresdir+1) + nIconIndex;

	  for (i=0; i < nIcons; i++,xresent++) 
	  {
	    PIMAGE_RESOURCE_DIRECTORY	resdir;

	    /* go down this resource entry, name */
	    resdir = (PIMAGE_RESOURCE_DIRECTORY)((DWORD)rootresdir+(xresent->u2.s.OffsetToDirectory));

	    /* default language (0) */
	    resdir = GetResDirEntryW(resdir,(LPWSTR)0,(DWORD)rootresdir,TRUE);
	    igdataent = (PIMAGE_RESOURCE_DATA_ENTRY)resdir;

	    /* lookup address in mapped image for virtual address */
	    igdata = NULL;

	    for (j=0;j<pe_header->FileHeader.NumberOfSections;j++) 
	    {
	      if (igdataent->OffsetToData < pe_sections[j].VirtualAddress)
	        continue;
	      if (igdataent->OffsetToData+igdataent->Size > pe_sections[j].VirtualAddress+pe_sections[j].SizeOfRawData)
		continue;
	      igdata = peimage+(igdataent->OffsetToData-pe_sections[j].VirtualAddress+pe_sections[j].PointerToRawData);
	    }

	    if (!igdata) 
	    {
	      WARN("no matching real address for icongroup!\n");
	      goto end_3;	/* failure */
	    }
	    RetPtr[i] = (HICON)LookupIconIdFromDirectoryEx(igdata, TRUE, cxDesired, cyDesired, LR_DEFAULTCOLOR);
	  }

	  if (!(iconresdir=GetResDirEntryW(rootresdir,RT_ICONW,(DWORD)rootresdir,FALSE))) 
	  {
	    WARN("No Iconresourcedirectory!\n");
	    goto end_3;		/* failure */
	  }

	  for (i=0; i<nIcons; i++) 
	  {
	    PIMAGE_RESOURCE_DIRECTORY	xresdir;
	    xresdir = GetResDirEntryW(iconresdir,(LPWSTR)(DWORD)RetPtr[i],(DWORD)rootresdir,FALSE);
	    xresdir = GetResDirEntryW(xresdir,(LPWSTR)0,(DWORD)rootresdir,TRUE);
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
	    RetPtr[i] = (HICON) CreateIconFromResourceEx(idata,idataent->Size,TRUE,0x00030000, cxDesired, cyDesired, LR_DEFAULTCOLOR);
	  }
	  hRet = S_OK;	/* return first icon */
	  goto end_3;		/* sucess */
	}			/* if(sig == IMAGE_NT_SIGNATURE) */

end_3:	UnmapViewOfFile(peimage);	/* success */
end_2:	CloseHandle(fmapping);
end_1:	_lclose( hFile);
	return hRet;
}

/***********************************************************************
 *           PrivateExtractIconsW			[USER32.@]
 *
 * NOTES
 *  nIndex = 1: a small and a large icon are extracted. 
 *  the higher word of sizeXY contains the size of the small icon, the lower
 *  word the size of the big icon. phicon points to HICON[2].
 *
 * RETURNS
 *  nIcons > 0: HRESULT
 *  nIcons = 0: the number of icons
 */

HRESULT WINAPI PrivateExtractIconsW ( 
	LPCWSTR lpwstrFile,
	int nIndex,
	DWORD sizeX,
	DWORD sizeY,
	HICON * phicon,	/* HICON* */
	DWORD w,	/* 0 */
	UINT nIcons,
	DWORD y )	/* 0x80 maybe LR_* constant */
{
	DWORD ret;
	TRACE("%s 0x%08x 0x%08lx 0x%08lx %p 0x%08lx 0x%08x 0x%08lx stub\n",
	debugstr_w(lpwstrFile),nIndex, sizeX ,sizeY ,phicon,w,nIcons,y );


	if ((nIcons == 2) && HIWORD(sizeX) && HIWORD(sizeY))
	{
	  ret = ICO_ExtractIconExW(lpwstrFile, phicon, nIndex, 1, sizeX & 0xffff, sizeY & 0xffff );
	  if (!SUCCEEDED(ret)) return ret;
	  ret = ICO_ExtractIconExW(lpwstrFile, phicon+1, nIndex, 1, (sizeX>>16) & 0xffff, (sizeY>>16) & 0xffff );
	}
	else
	{
	  ret = ICO_ExtractIconExW(lpwstrFile, phicon, nIndex, nIcons, sizeX & 0xffff, sizeY & 0xffff );
	}
	
	FIXME_(icon)("hicon=%08x ret=0x%08lx\n", *phicon, ret);
	
	return ret;
}

/***********************************************************************
 *           PrivateExtractIconsA			[USER32.@]
 */

HRESULT WINAPI PrivateExtractIconsA ( 
	LPCSTR lpstrFile,
	DWORD nIndex,
	DWORD sizeX,
	DWORD sizeY,
	HICON * phicon,
	DWORD w,	/* 0 */
	UINT  nIcons,
	DWORD y )	/* 0x80 */
{
	DWORD ret;
	LPWSTR lpwstrFile = HEAP_strdupAtoW(GetProcessHeap(), 0, lpstrFile);
	
	FIXME_(icon)("%s 0x%08lx 0x%08lx 0x%08lx %p 0x%08lx 0x%08x 0x%08lx stub\n",
	lpstrFile, nIndex, sizeX, sizeY, phicon, w, nIcons, y );

	ret = PrivateExtractIconsW(lpwstrFile, nIndex, sizeX, sizeY, phicon, w, nIcons, y);

	FIXME_(icon)("hicon=%08x ret=0x%08lx\n", *phicon, ret);
	
	HeapFree(GetProcessHeap(), 0, lpwstrFile);
	return ret;
}

/***********************************************************************
 *           PrivateExtractIconExW			[USER32.443]
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
 *           PrivateExtractIconExA			[USER32.442]
 */
HRESULT WINAPI PrivateExtractIconExA (
	LPCSTR lpstrFile,
	DWORD nIndex,
	HICON * phIconLarge,
	HICON * phIconSmall,
	UINT nIcons )
{
	DWORD ret;
	LPWSTR lpwstrFile = HEAP_strdupAtoW(GetProcessHeap(), 0, lpstrFile);

	TRACE("%s 0x%08lx %p %p 0x%08x\n",
	lpstrFile, nIndex, phIconLarge, phIconSmall, nIcons);

	ret = PrivateExtractIconExW(lpwstrFile,nIndex,phIconLarge, phIconSmall, nIcons);

	HeapFree(GetProcessHeap(), 0, lpwstrFile);
	return ret;
}

