/*
 *	shell icon cache (SIC)
 *
 */
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winuser16.h"
#include "wine/winbase16.h"
#include "neexe.h"
#include "cursoricon.h"
#include "module.h"
#include "heap.h"
#include "debugtools.h"
#include "winversion.h"

#include "shellapi.h"
#include "pidl.h"
#include "shell32_main.h"

DEFAULT_DEBUG_CHANNEL(shell)

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
 *				SHELL_GetResourceTable
 */
static DWORD SHELL_GetResourceTable(HFILE hFile, LPBYTE *retptr)
{	IMAGE_DOS_HEADER	mz_header;
	char			magic[4];
	int			size;

	TRACE("0x%08x %p\n", hFile, retptr);  

	*retptr = NULL;
	_llseek( hFile, 0, SEEK_SET );
	if ((_lread(hFile,&mz_header,sizeof(mz_header)) != sizeof(mz_header)) || (mz_header.e_magic != IMAGE_DOS_SIGNATURE))
	{ if (mz_header.e_cblp == 1)	/* .ICO file ? */
	  { *retptr = (LPBYTE)-1;	/* ICONHEADER.idType, must be 1 */
	    return 1;
	  }
	  else
	    return 0; /* failed */
	}
	_llseek( hFile, mz_header.e_lfanew, SEEK_SET );

	if (_lread( hFile, magic, sizeof(magic) ) != sizeof(magic))
	  return 0;

	_llseek( hFile, mz_header.e_lfanew, SEEK_SET);

	if (*(DWORD*)magic  == IMAGE_NT_SIGNATURE)
	  return IMAGE_NT_SIGNATURE;

	if (*(WORD*)magic == IMAGE_OS2_SIGNATURE)
	{ IMAGE_OS2_HEADER	ne_header;
	  LPBYTE		pTypeInfo = (LPBYTE)-1;

	  if (_lread(hFile,&ne_header,sizeof(ne_header))!=sizeof(ne_header))
	    return 0;

	  if (ne_header.ne_magic != IMAGE_OS2_SIGNATURE)
	    return 0;

	  size = ne_header.rname_tab_offset - ne_header.resource_tab_offset;

	  if( size > sizeof(NE_TYPEINFO) )
	  { pTypeInfo = (BYTE*)HeapAlloc( GetProcessHeap(), 0, size);
	    if( pTypeInfo ) 
	    { _llseek(hFile, mz_header.e_lfanew+ne_header.resource_tab_offset, SEEK_SET);
	      if( _lread( hFile, (char*)pTypeInfo, size) != size )
	      { HeapFree( GetProcessHeap(), 0, pTypeInfo); 
		pTypeInfo = NULL;
	      }
	    }
	  }
	  *retptr = pTypeInfo;
	  return IMAGE_OS2_SIGNATURE;
	}
	return 0; /* failed */
}
/*************************************************************************
 *			SHELL_LoadResource
 */
static BYTE * SHELL_LoadResource( HFILE hFile, NE_NAMEINFO* pNInfo, WORD sizeShift, ULONG *uSize)
{	BYTE*  ptr;

	TRACE("0x%08x %p 0x%08x\n", hFile, pNInfo, sizeShift);

	*uSize = (DWORD)pNInfo->length << sizeShift;
	if( (ptr = (BYTE*)HeapAlloc(GetProcessHeap(),0, *uSize) ))
	{ _llseek( hFile, (DWORD)pNInfo->offset << sizeShift, SEEK_SET);
	  _lread( hFile, (char*)ptr, pNInfo->length << sizeShift);
	  return ptr;
	}
	return 0;
}

/*************************************************************************
 *                      ICO_LoadIcon
 */
static BYTE * ICO_LoadIcon( HFILE hFile, LPicoICONDIRENTRY lpiIDE, ULONG *uSize)
{	BYTE*  ptr;

	TRACE("0x%08x %p\n", hFile, lpiIDE);

	*uSize = lpiIDE->dwBytesInRes;
	if( (ptr = (BYTE*)HeapAlloc(GetProcessHeap(),0, *uSize)) )
	{ _llseek( hFile, lpiIDE->dwImageOffset, SEEK_SET);
	  _lread( hFile, (char*)ptr, lpiIDE->dwBytesInRes);
	  return ptr;
	}

	return 0;
}

/*************************************************************************
 *                      ICO_GetIconDirectory
 *
 * Reads .ico file and build phony ICONDIR struct
 * see http://www.microsoft.com/win32dev/ui/icons.htm
 */
#define HEADER_SIZE		(sizeof(CURSORICONDIR) - sizeof (CURSORICONDIRENTRY))
#define HEADER_SIZE_FILE	(sizeof(icoICONDIR) - sizeof (icoICONDIRENTRY))

static BYTE * ICO_GetIconDirectory( HFILE hFile, LPicoICONDIR* lplpiID, ULONG *uSize ) 
{	CURSORICONDIR	lpcid;	/* icon resource in resource-dir format */
	LPicoICONDIR	lpiID;	/* icon resource in file format */
	int		i;

	TRACE("0x%08x %p\n", hFile, lplpiID); 
	
	_llseek( hFile, 0, SEEK_SET );
	if( _lread(hFile,(char*)&lpcid, HEADER_SIZE_FILE) != HEADER_SIZE_FILE )
	  return 0;

	if( lpcid.idReserved || (lpcid.idType != 1) || (!lpcid.idCount) )
	  return 0;

	i = lpcid.idCount * sizeof(icoICONDIRENTRY);
	lpiID = (LPicoICONDIR)HeapAlloc( GetProcessHeap(), 0, HEADER_SIZE_FILE + i);

	if( _lread(hFile,(char*)lpiID->idEntries,i) == i )
	{ CURSORICONDIR * lpID;			/* icon resource in resource format */
	  *uSize = lpcid.idCount * sizeof(CURSORICONDIRENTRY) + HEADER_SIZE;
	  if( (lpID = (CURSORICONDIR*)HeapAlloc(GetProcessHeap(),0, *uSize) ))
	  {
	    /* copy the header */
	    lpID->idReserved 	= lpiID->idReserved = 0;
	    lpID->idType 	= lpiID->idType = 1;
	    lpID->idCount 	= lpiID->idCount = lpcid.idCount;

	    /* copy the entrys */
	    for( i=0; i < lpiID->idCount; i++ )
	    { memcpy((void*)&(lpID->idEntries[i]),(void*)&(lpiID->idEntries[i]), sizeof(CURSORICONDIRENTRY) - 2);
	      lpID->idEntries[i].wResId = i;
	    }

	    *lplpiID = lpiID;
	    return (BYTE *)lpID;
	  }
	}
	/* fail */

	HeapFree( GetProcessHeap(), 0, lpiID);
	return 0;
}

/*************************************************************************
 *
 * returns
 *  failure:0; success: icon handle or nr of icons (nIconIndex-1)
 */
HICON WINAPI ICO_ExtractIconEx(LPCSTR lpszExeFileName, HICON * RetPtr, UINT nIconIndex, UINT n, UINT cxDesired, UINT cyDesired )
{	HGLOBAL		hRet = 0;
	LPBYTE		pData;
	OFSTRUCT	ofs;
	DWORD		sig;
	HFILE		hFile = OpenFile( lpszExeFileName, &ofs, OF_READ );
	UINT16		iconDirCount = 0,iconCount = 0;
	LPBYTE		peimage;
	HANDLE		fmapping;
	ULONG		uSize;
	
	TRACE("(file %s,start %d,extract %d\n", lpszExeFileName, nIconIndex, n);

	if( hFile == HFILE_ERROR || (nIconIndex!=-1 && !n) )
	  return hRet;

	sig = SHELL_GetResourceTable(hFile,&pData);

/* ico file */
	if( sig==IMAGE_OS2_SIGNATURE || sig==1 ) /* .ICO file */
	{ BYTE		*pCIDir = 0;
	  NE_TYPEINFO	*pTInfo = (NE_TYPEINFO*)(pData + 2);
	  NE_NAMEINFO	*pIconStorage = NULL;
	  NE_NAMEINFO	*pIconDir = NULL;
	  LPicoICONDIR	lpiID = NULL;

	  TRACE("-- OS2/icon Signature (0x%08lx)\n", sig);

	  if( pData == (BYTE*)-1 )
	  { pCIDir = ICO_GetIconDirectory(hFile, &lpiID, &uSize);	/* check for .ICO file */
	    if( pCIDir ) 
	    { iconDirCount = 1; iconCount = lpiID->idCount;
	      TRACE("-- icon found %p 0x%08lx 0x%08x 0x%08x\n", pCIDir, uSize, iconDirCount, iconCount);
	    }
	  }
	  else while( pTInfo->type_id && !(pIconStorage && pIconDir) )
	  { if( pTInfo->type_id == NE_RSCTYPE_GROUP_ICON )	/* find icon directory and icon repository */
	    { iconDirCount = pTInfo->count;
	      pIconDir = ((NE_NAMEINFO*)(pTInfo + 1));
	      TRACE("\tfound directory - %i icon families\n", iconDirCount);
	    }
	    if( pTInfo->type_id == NE_RSCTYPE_ICON ) 
	    { iconCount = pTInfo->count;
	      pIconStorage = ((NE_NAMEINFO*)(pTInfo + 1));
	      TRACE("\ttotal icons - %i\n", iconCount);
	    }
	    pTInfo = (NE_TYPEINFO *)((char*)(pTInfo+1)+pTInfo->count*sizeof(NE_NAMEINFO));
	  }

	  if( (pIconStorage && pIconDir) || lpiID )	  /* load resources and create icons */
	  { if( nIconIndex == (UINT16)-1 )
	    { RetPtr[0] = iconDirCount;
	    }
	    else if( nIconIndex < iconDirCount )
	    { UINT16   i, icon;
	      if( n > iconDirCount - nIconIndex ) 
	        n = iconDirCount - nIconIndex;

	      for( i = nIconIndex; i < nIconIndex + n; i++ ) 
	      { /* .ICO files have only one icon directory */

	        if( lpiID == NULL )	/* *.ico */
	          pCIDir = SHELL_LoadResource( hFile, pIconDir + i, *(WORD*)pData, &uSize );
	        RetPtr[i-nIconIndex] = pLookupIconIdFromDirectoryEx( pCIDir, TRUE,  GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
	        HeapFree(GetProcessHeap(), 0, pCIDir); 
	      }

	      for( icon = nIconIndex; icon < nIconIndex + n; icon++ )
	      { pCIDir = NULL;
	        if( lpiID )
	        { pCIDir = ICO_LoadIcon( hFile, lpiID->idEntries + RetPtr[icon-nIconIndex], &uSize);
	        }
	        else
	        { for( i = 0; i < iconCount; i++ )
	          { if( pIconStorage[i].id == (RetPtr[icon-nIconIndex] | 0x8000) )
	            { pCIDir = SHELL_LoadResource( hFile, pIconStorage + i,*(WORD*)pData, &uSize );
	            }
	          }
	        }
	        if( pCIDir )
	        { RetPtr[icon-nIconIndex] = (HICON) pCreateIconFromResourceEx(pCIDir,uSize,TRUE,0x00030000, cxDesired, cyDesired, LR_DEFAULTCOLOR);
	        }
	        else
	        { RetPtr[icon-nIconIndex] = 0;
	        }
	      }
	    }
	  }
	  if( lpiID ) 
	    HeapFree( GetProcessHeap(), 0, lpiID);
	  else 
	    HeapFree( GetProcessHeap(), 0, pData);
	} 
/* end ico file */

/* exe/dll */
	if( sig == IMAGE_NT_SIGNATURE)
	{ LPBYTE		idata,igdata;
	  PIMAGE_DOS_HEADER	dheader;
	  PIMAGE_NT_HEADERS	pe_header;
	  PIMAGE_SECTION_HEADER	pe_sections;
	  PIMAGE_RESOURCE_DIRECTORY	rootresdir,iconresdir,icongroupresdir;
	  PIMAGE_RESOURCE_DATA_ENTRY	idataent,igdataent;
	  PIMAGE_RESOURCE_DIRECTORY_ENTRY	xresent;
	  int			i,j;
		  
	  if ( !(fmapping = CreateFileMappingA(hFile,NULL,PAGE_READONLY|SEC_COMMIT,0,0,NULL))) 
	  { WARN("failed to create filemap.\n");	/* FIXME, INVALID_HANDLE_VALUE? */
	    goto end_2;		/* failure */
	  }

	  if ( !(peimage = MapViewOfFile(fmapping,FILE_MAP_READ,0,0,0))) 
	  { WARN("failed to mmap filemap.\n");
	    goto end_2;		/* failure */
	  }

	  dheader = (PIMAGE_DOS_HEADER)peimage;
	  pe_header = (PIMAGE_NT_HEADERS)(peimage+dheader->e_lfanew);	  /* it is a pe header, SHELL_GetResourceTable checked that */
	  pe_sections = (PIMAGE_SECTION_HEADER)(((char*)pe_header)+sizeof(*pe_header));	/* probably makes problems with short PE headers...*/
	  rootresdir = NULL;

	  for (i=0;i<pe_header->FileHeader.NumberOfSections;i++) 
	  { if (pe_sections[i].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
	      continue;
	    /* FIXME: doesn't work when the resources are not in a seperate section */
	    if (pe_sections[i].VirtualAddress == pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress) 
	    { rootresdir = (PIMAGE_RESOURCE_DIRECTORY)((char*)peimage+pe_sections[i].PointerToRawData);
	      break;
	    }
	  }

	  if (!rootresdir) 
	  { WARN("haven't found section for resource directory.\n");
	    goto end_3;		/* failure */
	  }
  /* search the group icon dir*/
	  if (!(icongroupresdir = GetResDirEntryW(rootresdir,RT_GROUP_ICONW, (DWORD)rootresdir,FALSE))) 
	  { WARN("No Icongroupresourcedirectory!\n");
	    goto end_3;		/* failure */
	  }
	  iconDirCount = icongroupresdir->NumberOfNamedEntries+icongroupresdir->NumberOfIdEntries;

  /* number of icons requested */
	  if( nIconIndex == -1 )
	  { hRet = iconDirCount;
	    goto end_3;		/* success */
	  }

	  if (nIconIndex >= iconDirCount) 
	  { WARN("nIconIndex %d is larger than iconDirCount %d\n",nIconIndex,iconDirCount);
	    goto end_3;		/* failure */
	  }

	  xresent = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(icongroupresdir+1);	/* caller just wanted the number of entries */

	  if( n > iconDirCount - nIconIndex )	/* assure we don't get too much ... */
	  { n = iconDirCount - nIconIndex;
	  }

	  xresent = xresent+nIconIndex;		/* starting from specified index ... */

	  for (i=0;i<n;i++,xresent++) 
	  { PIMAGE_RESOURCE_DIRECTORY	resdir;

	    /* go down this resource entry, name */
	    resdir = (PIMAGE_RESOURCE_DIRECTORY)((DWORD)rootresdir+(xresent->u2.s.OffsetToDirectory));

	    /* default language (0) */
	    resdir = GetResDirEntryW(resdir,(LPWSTR)0,(DWORD)rootresdir,TRUE);
	    igdataent = (PIMAGE_RESOURCE_DATA_ENTRY)resdir;

	    /* lookup address in mapped image for virtual address */
	    igdata = NULL;

	    for (j=0;j<pe_header->FileHeader.NumberOfSections;j++) 
	    { if (igdataent->OffsetToData < pe_sections[j].VirtualAddress)
	        continue;
	      if (igdataent->OffsetToData+igdataent->Size > pe_sections[j].VirtualAddress+pe_sections[j].SizeOfRawData)
		continue;
	      igdata = peimage+(igdataent->OffsetToData-pe_sections[j].VirtualAddress+pe_sections[j].PointerToRawData);
	    }

	    if (!igdata) 
	    { WARN("no matching real address for icongroup!\n");
	      goto end_3;	/* failure */
	    }
	    RetPtr[i] = (HICON)pLookupIconIdFromDirectoryEx(igdata, TRUE, cxDesired, cyDesired, LR_DEFAULTCOLOR);
	  }

	  if (!(iconresdir=GetResDirEntryW(rootresdir,RT_ICONW,(DWORD)rootresdir,FALSE))) 
	  { WARN("No Iconresourcedirectory!\n");
	    goto end_3;		/* failure */
	  }

	  for (i=0;i<n;i++) 
	  { PIMAGE_RESOURCE_DIRECTORY	xresdir;
	    xresdir = GetResDirEntryW(iconresdir,(LPWSTR)(DWORD)RetPtr[i],(DWORD)rootresdir,FALSE);
	    xresdir = GetResDirEntryW(xresdir,(LPWSTR)0,(DWORD)rootresdir,TRUE);
	    idataent = (PIMAGE_RESOURCE_DATA_ENTRY)xresdir;
	    idata = NULL;

	    /* map virtual to address in image */
	    for (j=0;j<pe_header->FileHeader.NumberOfSections;j++) 
	    { if (idataent->OffsetToData < pe_sections[j].VirtualAddress)
	        continue;
	      if (idataent->OffsetToData+idataent->Size > pe_sections[j].VirtualAddress+pe_sections[j].SizeOfRawData)
	        continue;
	      idata = peimage+(idataent->OffsetToData-pe_sections[j].VirtualAddress+pe_sections[j].PointerToRawData);
	    }
	    if (!idata) 
	    { WARN("no matching real address found for icondata!\n");
	      RetPtr[i]=0;
	      continue;
	    }
	    RetPtr[i] = (HICON) pCreateIconFromResourceEx(idata,idataent->Size,TRUE,0x00030000, cxDesired, cyDesired, LR_DEFAULTCOLOR);
	  }
	  hRet = RetPtr[0];	/* return first icon */
	  goto end_3;		/* sucess */
	}
	goto end_1;		/* unknown filetype */

/* cleaning up (try & catch would be nicer:-) ) */
end_3:	UnmapViewOfFile(peimage);	/* success */
end_2:	CloseHandle(fmapping);
end_1:	_lclose( hFile);
	return hRet;
}

/********************** THE ICON CACHE ********************************/

#define INVALID_INDEX -1

typedef struct
{	LPCSTR sSourceFile;	/* file (not path!) containing the icon */
	DWORD dwSourceIndex;	/* index within the file, if it is a resoure ID it will be negated */
	DWORD dwListIndex;	/* index within the iconlist */
	DWORD dwFlags;		/* GIL_* flags */
	DWORD dwAccessTime;
} SIC_ENTRY, * LPSIC_ENTRY;

static HDPA		sic_hdpa = 0;
static CRITICAL_SECTION	SHELL32_SicCS;

/*****************************************************************************
 * SIC_CompareEntrys			[called by comctl32.dll]
 *
 * NOTES
 *  Callback for DPA_Search
 */
INT CALLBACK SIC_CompareEntrys( LPVOID p1, LPVOID p2, LPARAM lparam)
{	TRACE("%p %p\n", p1, p2);

	if (((LPSIC_ENTRY)p1)->dwSourceIndex != ((LPSIC_ENTRY)p2)->dwSourceIndex) /* first the faster one*/
	  return 1;

	if (strcasecmp(((LPSIC_ENTRY)p1)->sSourceFile,((LPSIC_ENTRY)p2)->sSourceFile))
	  return 1;

	return 0;  
}
/*****************************************************************************
 * SIC_IconAppend			[internal]
 *
 * NOTES
 *  appends a icon pair to the end of the cache
 */
static INT SIC_IconAppend (LPCSTR sSourceFile, INT dwSourceIndex, HICON hSmallIcon, HICON hBigIcon)
{	LPSIC_ENTRY lpsice;
	INT ret, index, index1;
	
	TRACE("%s %i %x %x\n", sSourceFile, dwSourceIndex, hSmallIcon ,hBigIcon);

	lpsice = (LPSIC_ENTRY) SHAlloc (sizeof (SIC_ENTRY));

	lpsice->sSourceFile = HEAP_strdupA (GetProcessHeap(), 0, PathFindFilenameA(sSourceFile));
	lpsice->dwSourceIndex = dwSourceIndex;
	
	EnterCriticalSection(&SHELL32_SicCS);

	index = pDPA_InsertPtr(sic_hdpa, 0x7fff, lpsice);
	if ( INVALID_INDEX == index )
	{
	  SHFree(lpsice);
	  ret = INVALID_INDEX;
	}
	else
	{
	  index = pImageList_AddIcon (ShellSmallIconList, hSmallIcon);
	  index1= pImageList_AddIcon (ShellBigIconList, hBigIcon);

	  if (index!=index1)
	  {
	    FIXME("iconlists out of sync 0x%x 0x%x\n", index, index1);
	  }
	  lpsice->dwListIndex = index;
	  ret = lpsice->dwListIndex;
	}

	LeaveCriticalSection(&SHELL32_SicCS);
	return ret;	
}
/****************************************************************************
 * SIC_LoadIcon				[internal]
 *
 * NOTES
 *  gets small/big icon by number from a file
 */
static INT SIC_LoadIcon (LPCSTR sSourceFile, INT dwSourceIndex)
{	HICON	hiconLarge=0;
	HICON	hiconSmall=0;

	ICO_ExtractIconEx(sSourceFile, &hiconLarge, dwSourceIndex, 1, 32, 32  );
	ICO_ExtractIconEx(sSourceFile, &hiconSmall, dwSourceIndex, 1, 16, 16  );


	if ( !hiconLarge ||  !hiconSmall)
	{
	  WARN("failure loading icon %i from %s (%x %x)\n", dwSourceIndex, sSourceFile, hiconLarge, hiconSmall);
	  return -1;
	}
	return SIC_IconAppend (sSourceFile, dwSourceIndex, hiconSmall, hiconLarge);		
}
/*****************************************************************************
 * SIC_GetIconIndex			[internal]
 *
 * Parameters
 *	sSourceFile	[IN]	filename of file containing the icon
 *	index		[IN]	index/resID (negated) in this file
 *
 * NOTES
 *  look in the cache for a proper icon. if not available the icon is taken
 *  from the file and cached
 */
INT SIC_GetIconIndex (LPCSTR sSourceFile, INT dwSourceIndex )
{	SIC_ENTRY sice;
	INT ret, index = INVALID_INDEX;
		
	TRACE("%s %i\n", sSourceFile, dwSourceIndex);

	sice.sSourceFile = PathFindFilenameA(sSourceFile);
	sice.dwSourceIndex = dwSourceIndex;
	
	EnterCriticalSection(&SHELL32_SicCS);

	if (NULL != pDPA_GetPtr (sic_hdpa, 0))
	{
	  index = pDPA_Search (sic_hdpa, &sice, -1L, SIC_CompareEntrys, 0, 0);
	}

	if ( INVALID_INDEX == index )
	{
	  ret = SIC_LoadIcon (sSourceFile, dwSourceIndex);
	}
	else
	{
	  TRACE("-- found\n");
	  ret = ((LPSIC_ENTRY)pDPA_GetPtr(sic_hdpa, index))->dwListIndex;
	}

	LeaveCriticalSection(&SHELL32_SicCS);
	return ret;
}
/****************************************************************************
 * SIC_LoadIcon				[internal]
 *
 * NOTES
 *  retrives the specified icon from the iconcache. if not found try's to load the icon
 */
static HICON WINE_UNUSED SIC_GetIcon (LPCSTR sSourceFile, INT dwSourceIndex, BOOL bSmallIcon )
{	INT index;

	TRACE("%s %i\n", sSourceFile, dwSourceIndex);

	index = SIC_GetIconIndex(sSourceFile, dwSourceIndex);

	if (INVALID_INDEX == index)
	{
	  return INVALID_INDEX;
	}

	if (bSmallIcon)
	  return pImageList_GetIcon(ShellSmallIconList, index, ILD_NORMAL);
	return pImageList_GetIcon(ShellBigIconList, index, ILD_NORMAL);
	
}
/*****************************************************************************
 * SIC_Initialize			[internal]
 *
 * NOTES
 *  hack to load the resources from the shell32.dll under a different dll name 
 *  will be removed when the resource-compiler is ready
 */
BOOL SIC_Initialize(void)
{
	HICON		hSm, hLg;
	UINT		index;

	TRACE("\n");

	if (sic_hdpa)	/* already initialized?*/
	  return TRUE;
	  
	InitializeCriticalSection(&SHELL32_SicCS);
	MakeCriticalSectionGlobal(&SHELL32_SicCS);
	
	sic_hdpa = pDPA_Create(16);
	
	if (!sic_hdpa)
	{
	  return(FALSE);
	}

	ShellSmallIconList = pImageList_Create(16,16,ILC_COLORDDB | ILC_MASK,0,0x20);
	ShellBigIconList = pImageList_Create(32,32,ILC_COLORDDB | ILC_MASK,0,0x20);

	pImageList_SetBkColor(ShellSmallIconList, GetSysColor(COLOR_WINDOW));
	pImageList_SetBkColor(ShellBigIconList, GetSysColor(COLOR_WINDOW));

	for (index=1; index<39; index++)
	{
	  hSm = LoadImageA(shell32_hInstance, MAKEINTRESOURCEA(index), IMAGE_ICON, 16, 16,LR_SHARED);
	  hLg = LoadImageA(shell32_hInstance, MAKEINTRESOURCEA(index), IMAGE_ICON, 32, 32,LR_SHARED);

	  if(!hSm)
	  {
	    hSm = LoadImageA(shell32_hInstance, MAKEINTRESOURCEA(0), IMAGE_ICON, 16, 16,LR_SHARED);
	    hLg = LoadImageA(shell32_hInstance, MAKEINTRESOURCEA(0), IMAGE_ICON, 32, 32,LR_SHARED);
	  }
	  SIC_IconAppend ("shell32.dll", index, hSm, hLg);
	}

	TRACE("hIconSmall=%p hIconBig=%p\n",ShellSmallIconList, ShellBigIconList);

	return TRUE;
}
/*************************************************************************
 * SIC_Destroy
 *
 * frees the cache
 */
void SIC_Destroy(void)
{
	LPSIC_ENTRY lpsice;
	int i;

	TRACE("\n");

	EnterCriticalSection(&SHELL32_SicCS);

	if (sic_hdpa && NULL != pDPA_GetPtr (sic_hdpa, 0))
	{
	  for (i=0; i < pDPA_GetPtrCount(sic_hdpa); ++i)
	  {
	    lpsice = pDPA_GetPtr(sic_hdpa, i); 
	    SHFree(lpsice);
	  }
	  pDPA_Destroy(sic_hdpa);
	}

	sic_hdpa = NULL;

	LeaveCriticalSection(&SHELL32_SicCS);
	DeleteCriticalSection(&SHELL32_SicCS);
}
/*************************************************************************
 * Shell_GetImageList			[SHELL32.71]
 *
 * PARAMETERS
 *  imglist[1|2] [OUT] pointer which recive imagelist handles
 *
 */
BOOL WINAPI Shell_GetImageList(HIMAGELIST * lpBigList, HIMAGELIST * lpSmallList)
{	TRACE("(%p,%p)\n",lpBigList,lpSmallList);
	if (lpBigList)
	{ *lpBigList = ShellBigIconList;
	}
	if (lpSmallList)
	{ *lpSmallList = ShellSmallIconList;
	}

	return TRUE;
}
/*************************************************************************
 * PidlToSicIndex			[INTERNAL]
 *
 * PARAMETERS
 *	sh	[IN]	IShellFolder
 *	pidl	[IN]
 *	bBigIcon [IN]
 *	uFlags	[IN]	GIL_*
 *	pIndex	[OUT]	index within the SIC
 *
 */
BOOL PidlToSicIndex (
	IShellFolder * sh,
	LPITEMIDLIST pidl,
	BOOL bBigIcon,
	UINT uFlags,
	UINT * pIndex)
{	
	IExtractIconA	*ei;
	char		szIconFile[MAX_PATH];	/* file containing the icon */
	INT		iSourceIndex;		/* index or resID(negated) in this file */
	BOOL		ret = FALSE;
	UINT		dwFlags = 0;
	
	TRACE("sf=%p pidl=%p %s\n", sh, pidl, bBigIcon?"Big":"Small");

	if (SUCCEEDED (IShellFolder_GetUIObjectOf(sh, 0, 1, &pidl, &IID_IExtractIconA, 0, (void **)&ei)))
	{
	  if (SUCCEEDED(IExtractIconA_GetIconLocation(ei, uFlags, szIconFile, MAX_PATH, &iSourceIndex, &dwFlags)))
	  {
	    *pIndex = SIC_GetIconIndex(szIconFile, iSourceIndex);
	    ret = TRUE;
	  }
	  IExtractIconA_Release(ei);
	}

	if (INVALID_INDEX == *pIndex)	/* default icon when failed */
	  *pIndex = 1;

	return ret;

}

/*************************************************************************
 * SHMapPIDLToSystemImageListIndex	[SHELL32.77]
 *
 * PARAMETERS
 *	sh	[IN]		pointer to an instance of IShellFolder 
 *	pidl	[IN]
 *	pIndex	[OUT][OPTIONAL]	SIC index for big icon
 *
 */
UINT WINAPI SHMapPIDLToSystemImageListIndex(LPSHELLFOLDER sh, LPITEMIDLIST pidl, UINT * pIndex)
{
	UINT	Index;

	TRACE("(SF=%p,pidl=%p,%p)\n",sh,pidl,pIndex);
	pdump(pidl);
	
	if (pIndex)
	  PidlToSicIndex ( sh, pidl, 1, 0, pIndex);
	PidlToSicIndex ( sh, pidl, 0, 0, &Index);
	return Index;
}

/*************************************************************************
 * Shell_GetCachedImageIndex		[SHELL32.72]
 *
 */
INT WINAPI Shell_GetCachedImageIndexA(LPCSTR szPath, INT nIndex, BOOL bSimulateDoc) 
{
	WARN("(%s,%08x,%08x) semi-stub.\n",debugstr_a(szPath), nIndex, bSimulateDoc);
	return SIC_GetIconIndex(szPath, nIndex);
}

INT WINAPI Shell_GetCachedImageIndexW(LPCWSTR szPath, INT nIndex, BOOL bSimulateDoc) 
{	INT ret;
	LPSTR sTemp = HEAP_strdupWtoA (GetProcessHeap(),0,szPath);
	
	WARN("(%s,%08x,%08x) semi-stub.\n",debugstr_w(szPath), nIndex, bSimulateDoc);

	ret = SIC_GetIconIndex(sTemp, nIndex);
	HeapFree(GetProcessHeap(),0,sTemp);
	return ret;
}

INT WINAPI Shell_GetCachedImageIndexAW(LPCVOID szPath, INT nIndex, BOOL bSimulateDoc)
{	if( VERSION_OsIsUnicode())
	  return Shell_GetCachedImageIndexW(szPath, nIndex, bSimulateDoc);
	return Shell_GetCachedImageIndexA(szPath, nIndex, bSimulateDoc);
}

/*************************************************************************
 * ExtractIconEx			[shell32.189]
 */
HICON WINAPI ExtractIconExAW ( LPCVOID lpszFile, INT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIcons )
{	if (VERSION_OsIsUnicode())
	  return ExtractIconExW ( lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
	return ExtractIconExA ( lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
}
/*************************************************************************
 * ExtractIconExA			[shell32.190]
 * RETURNS
 *  0 no icon found 
 *  1 file is not valid
 *  HICON handle of a icon (phiconLarge/Small == NULL)
 */
HICON WINAPI ExtractIconExA ( LPCSTR lpszFile, INT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIcons )
{	HICON ret=0;
	
	TRACE("file=%s idx=%i %p %p num=%i\n", lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons );

	if (nIconIndex==-1)	/* Number of icons requested */
	  return ICO_ExtractIconEx(lpszFile, NULL, -1, 0, 0, 0  );
	  
	
	if (phiconLarge)
	{ ret = ICO_ExtractIconEx(lpszFile, phiconLarge, nIconIndex, nIcons, 32, 32  );
	  if ( nIcons==1)
	  { ret = phiconLarge[0];	    
	  }
	}

	/* if no pointers given and one icon expected, return the handle directly*/
	if (!phiconLarge && ! phiconSmall && nIcons==1 )
	  phiconSmall = &ret;
	
	if (phiconSmall)
	{ ret = ICO_ExtractIconEx(lpszFile, phiconSmall, nIconIndex, nIcons, 16, 16  );
	  if ( nIcons==1 )
	  { ret = phiconSmall[0];
	  }
	}

	return ret;
}
/*************************************************************************
 * ExtractIconExW			[shell32.191]
 */
HICON WINAPI ExtractIconExW ( LPCWSTR lpszFile, INT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIcons )
{	LPSTR sFile;
	DWORD ret;
	
	TRACE("file=%s idx=%i %p %p num=%i\n", debugstr_w(lpszFile), nIconIndex, phiconLarge, phiconSmall, nIcons );

	sFile = HEAP_strdupWtoA (GetProcessHeap(),0,lpszFile);
	ret = ExtractIconExA ( sFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
	HeapFree(GetProcessHeap(),0,sFile);
	return ret;
}
