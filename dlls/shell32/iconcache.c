/*
 *	shell icon cache (SIC)
 *
 *
 */
#include <string.h>
#include "windows.h"
#include "wine/winuser16.h"
#include "neexe.h"
#include "cursoricon.h"
#include "module.h"
#include "heap.h"
#include "debug.h"
#include "sysmetrics.h"
#include "winversion.h"
#include "shell.h"
#include "shlobj.h"
#include "pidl.h"
#include "shell32_main.h"

#pragma pack(1)

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
    WORD            idReserved;   /* Reserved (must be 0)		*/
    WORD            idType;       /* Resource Type (1 for icons)	*/
    WORD            idCount;      /* How many images?			*/
    icoICONDIRENTRY idEntries[1]; /* An entry for each image (idCount of 'em) */
} icoICONDIR, *LPicoICONDIR;

#pragma pack(4)

/*************************************************************************
 *				SHELL_GetResourceTable
 */
static DWORD SHELL_GetResourceTable(HFILE32 hFile,LPBYTE *retptr)
{	IMAGE_DOS_HEADER	mz_header;
	char			magic[4];
	int			size;

	TRACE(shell,"\n");  

	*retptr = NULL;
	_llseek32( hFile, 0, SEEK_SET );
	if ((_lread32(hFile,&mz_header,sizeof(mz_header)) != sizeof(mz_header)) || (mz_header.e_magic != IMAGE_DOS_SIGNATURE))
	{ if (mz_header.e_cblp == 1)	/* .ICO file ? */
	  { *retptr = (LPBYTE)-1;	/* ICONHEADER.idType, must be 1 */
  	    return 1;
	  }
	  else
	    return 0; /* failed */
	}
	_llseek32( hFile, mz_header.e_lfanew, SEEK_SET );

	if (_lread32( hFile, magic, sizeof(magic) ) != sizeof(magic))
	  return 0;

	_llseek32( hFile, mz_header.e_lfanew, SEEK_SET);

	if (*(DWORD*)magic  == IMAGE_NT_SIGNATURE)
	  return IMAGE_NT_SIGNATURE;

	if (*(WORD*)magic == IMAGE_OS2_SIGNATURE)
	{ IMAGE_OS2_HEADER	ne_header;
	  LPBYTE		pTypeInfo = (LPBYTE)-1;

	  if (_lread32(hFile,&ne_header,sizeof(ne_header))!=sizeof(ne_header))
	    return 0;

	  if (ne_header.ne_magic != IMAGE_OS2_SIGNATURE)
	    return 0;

	  size = ne_header.rname_tab_offset - ne_header.resource_tab_offset;

	  if( size > sizeof(NE_TYPEINFO) )
	  { pTypeInfo = (BYTE*)HeapAlloc( GetProcessHeap(), 0, size);
	    if( pTypeInfo ) 
	    { _llseek32(hFile, mz_header.e_lfanew+ne_header.resource_tab_offset, SEEK_SET);
	      if( _lread32( hFile, (char*)pTypeInfo, size) != size )
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
static HGLOBAL16 SHELL_LoadResource(HINSTANCE32 hInst, HFILE32 hFile, NE_NAMEINFO* pNInfo, WORD sizeShift)
{	BYTE*  ptr;
	HGLOBAL16 handle = DirectResAlloc( hInst, 0x10, (DWORD)pNInfo->length << sizeShift);

	TRACE(shell,"\n");

	if( (ptr = (BYTE*)GlobalLock16( handle )) )
	{ _llseek32( hFile, (DWORD)pNInfo->offset << sizeShift, SEEK_SET);
	  _lread32( hFile, (char*)ptr, pNInfo->length << sizeShift);
	  return handle;
	}
	return 0;
}

/*************************************************************************
 *                      ICO_LoadIcon
 */
static HGLOBAL16 ICO_LoadIcon(HINSTANCE32 hInst, HFILE32 hFile, LPicoICONDIRENTRY lpiIDE)
{	BYTE*  ptr;
	HGLOBAL16 handle = DirectResAlloc( hInst, 0x10, lpiIDE->dwBytesInRes);
	TRACE(shell,"\n");
	if( (ptr = (BYTE*)GlobalLock16( handle )) )
	{ _llseek32( hFile, lpiIDE->dwImageOffset, SEEK_SET);
	  _lread32( hFile, (char*)ptr, lpiIDE->dwBytesInRes);
	  return handle;
	}
	return 0;
}

/*************************************************************************
 *                      ICO_GetIconDirectory
 *
 *  Read .ico file and build phony ICONDIR struct for GetIconID
 */
static HGLOBAL16 ICO_GetIconDirectory(HINSTANCE32 hInst, HFILE32 hFile, LPicoICONDIR* lplpiID ) 
{	WORD    id[3];  /* idReserved, idType, idCount */
	LPicoICONDIR	lpiID;
	int		i;
 
	TRACE(shell,"\n"); 
	_llseek32( hFile, 0, SEEK_SET );
	if( _lread32(hFile,(char*)id,sizeof(id)) != sizeof(id) ) 
	  return 0;

	/* check .ICO header 
	*
	* - see http://www.microsoft.com/win32dev/ui/icons.htm
	*/

	if( id[0] || id[1] != 1 || !id[2] ) 
	  return 0;

	i = id[2]*sizeof(icoICONDIRENTRY) + sizeof(id);

	lpiID = (LPicoICONDIR)HeapAlloc( GetProcessHeap(), 0, i);

	if( _lread32(hFile,(char*)lpiID->idEntries,i) == i )
	{ HGLOBAL16 handle = DirectResAlloc( hInst, 0x10,id[2]*sizeof(ICONDIRENTRY) + sizeof(id) );
	  if( handle ) 
	  { CURSORICONDIR*     lpID = (CURSORICONDIR*)GlobalLock16( handle );
	    lpID->idReserved = lpiID->idReserved = id[0];
	    lpID->idType = lpiID->idType = id[1];
	    lpID->idCount = lpiID->idCount = id[2];
	    for( i=0; i < lpiID->idCount; i++ )
	    { memcpy((void*)(lpID->idEntries + i),(void*)(lpiID->idEntries + i), sizeof(ICONDIRENTRY) - 2);
	      lpID->idEntries[i].icon.wResId = i;
            }
	    *lplpiID = lpiID;
	    return handle;
	  }
	}
	/* fail */

	HeapFree( GetProcessHeap(), 0, lpiID);
	return 0;
}

/*************************************************************************
 *			InternalExtractIcon		[SHELL.39]
 *
 * This abortion is called directly by Progman
 *  fixme: the icon section is broken (don't have a handle for
 *    ICO_GetIconDirectory....)
 *
 */
#define ICO_INVALID_FILE	1
#define ICO_NO_ICONS		0

HGLOBAL32 WINAPI ICO_ExtractIconEx(LPCSTR lpszExeFileName, HICON32* RetPtr, UINT32 nIconIndex, UINT32 n, UINT32 cxDesired, UINT32 cyDesired )
{	HGLOBAL32	hRet = ICO_NO_ICONS;
	LPBYTE		pData;
	OFSTRUCT	ofs;
	DWORD		sig;
	HFILE32		hFile = OpenFile32( lpszExeFileName, &ofs, OF_READ );
	UINT16		iconDirCount = 0,iconCount = 0;
	LPBYTE		peimage;
	HANDLE32	fmapping;
	
	TRACE(shell,"(file %s,start %d,extract %d\n", lpszExeFileName, nIconIndex, n);

	if( hFile == HFILE_ERROR32 || !n )
	  return ICO_INVALID_FILE;

	sig = SHELL_GetResourceTable(hFile,&pData);

/* ico file */
	if( sig==IMAGE_OS2_SIGNATURE || sig==1 ) /* .ICO file */
	{ HICON16	hIcon = 0;
	  NE_TYPEINFO*	pTInfo = (NE_TYPEINFO*)(pData + 2);
	  NE_NAMEINFO*	pIconStorage = NULL;
	  NE_NAMEINFO*	pIconDir = NULL;
	  LPicoICONDIR	lpiID = NULL;
 
	  if( pData == (BYTE*)-1 )
	  { hIcon = ICO_GetIconDirectory(0, hFile, &lpiID);	/* check for .ICO file */
	    if( hIcon ) 
	    { iconDirCount = 1; iconCount = lpiID->idCount; 
	    }
	  }
	  else while( pTInfo->type_id && !(pIconStorage && pIconDir) )
	  { if( pTInfo->type_id == NE_RSCTYPE_GROUP_ICON )	/* find icon directory and icon repository */
	    { iconDirCount = pTInfo->count;
	      pIconDir = ((NE_NAMEINFO*)(pTInfo + 1));
	      TRACE(shell,"\tfound directory - %i icon families\n", iconDirCount);
	    }
	    if( pTInfo->type_id == NE_RSCTYPE_ICON ) 
	    { iconCount = pTInfo->count;
	      pIconStorage = ((NE_NAMEINFO*)(pTInfo + 1));
	      TRACE(shell,"\ttotal icons - %i\n", iconCount);
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

	        if( lpiID == NULL )
	          hIcon = SHELL_LoadResource( 0, hFile, pIconDir + i, *(WORD*)pData );
	        RetPtr[i-nIconIndex] = GetIconID( hIcon, 3 );
	        GlobalFree16(hIcon); 
              }

	      for( icon = nIconIndex; icon < nIconIndex + n; icon++ )
	      { hIcon = 0;
	        if( lpiID )
	        { hIcon = ICO_LoadIcon( 0, hFile, lpiID->idEntries + RetPtr[icon-nIconIndex]);
	        }
	        else
	        { for( i = 0; i < iconCount; i++ )
	          { if( pIconStorage[i].id == (RetPtr[icon-nIconIndex] | 0x8000) )
	            { hIcon = SHELL_LoadResource( 0, hFile, pIconStorage + i,*(WORD*)pData );
	            }
	          }
	        }
	        if( hIcon )
	        { RetPtr[icon-nIconIndex] = LoadIconHandler( hIcon, TRUE ); 
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
		  
	  if ( !(fmapping = CreateFileMapping32A(hFile,NULL,PAGE_READONLY|SEC_COMMIT,0,0,NULL))) 
	  { WARN(shell,"failed to create filemap.\n");	/* FIXME, INVALID_HANDLE_VALUE? */
	    hRet = ICO_INVALID_FILE;
	    goto end_2;		/* failure */
	  }

	  if ( !(peimage = MapViewOfFile(fmapping,FILE_MAP_READ,0,0,0))) 
	  { WARN(shell,"failed to mmap filemap.\n");
	    hRet = ICO_INVALID_FILE;
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
	  { WARN(shell,"haven't found section for resource directory.\n");
	    goto end_4;		/* failure */
	  }
  /* search the group icon dir*/
	  if (!(icongroupresdir = GetResDirEntryW(rootresdir,RT_GROUP_ICON32W, (DWORD)rootresdir,FALSE))) 
	  { WARN(shell,"No Icongroupresourcedirectory!\n");
	    goto end_4;		/* failure */
	  }
	  iconDirCount = icongroupresdir->NumberOfNamedEntries+icongroupresdir->NumberOfIdEntries;

  /* number of icons requested */
	  if( nIconIndex == -1 )
	  { hRet = iconDirCount;
	    goto end_3;		/* success */
	  }
  
	  if (nIconIndex >= iconDirCount) 
	  { WARN(shell,"nIconIndex %d is larger than iconDirCount %d\n",nIconIndex,iconDirCount);
	    goto end_4;		/* failure */
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
	    { WARN(shell,"no matching real address for icongroup!\n");
	      goto end_4;	/* failure */
	    }
	    RetPtr[i] = LookupIconIdFromDirectoryEx32(igdata, TRUE, cxDesired, cyDesired, LR_DEFAULTCOLOR);
	  }

	  if (!(iconresdir=GetResDirEntryW(rootresdir,RT_ICON32W,(DWORD)rootresdir,FALSE))) 
	  { WARN(shell,"No Iconresourcedirectory!\n");
	    goto end_4;		/* failure */
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
	    { WARN(shell,"no matching real address found for icondata!\n");
	      RetPtr[i]=0;
	      continue;
	    }
	    RetPtr[i] = CreateIconFromResourceEx32(idata,idataent->Size,TRUE,0x00030000, cxDesired, cyDesired, LR_DEFAULTCOLOR);
	  }
	  hRet = RetPtr[0];	/* return first icon */
	  goto end_3;		/* sucess */
	}
	hRet = ICO_INVALID_FILE;
	goto end_1;		/* unknown filetype */

/* cleaning up (try & catch would be nicer:-) ) */
end_4:	hRet = 0;	/* failure */
end_3:	UnmapViewOfFile(peimage);	/* success */
end_2:	CloseHandle(fmapping);
end_1:	_lclose32( hFile);
	return hRet;
}

/********************** THE ICON CACHE ********************************/
HIMAGELIST ShellSmallIconList = 0;
HIMAGELIST ShellBigIconList = 0;
HDPA	hdpa=0;

#define INVALID_INDEX -1

typedef struct
{	LPCSTR sSourceFile;	/* file icon is from */
	DWORD dwSourceIndex;	/* index within the file */
	DWORD dwListIndex;	/* index within the iconlist */
} SIC_ENTRY, * LPSIC_ENTRY;

/*****************************************************************************
*	SIC_CompareEntrys [called by comctl32.dll]
* NOTES
*  Callback for DPA_Search
*/
INT32 CALLBACK SIC_CompareEntrys( LPVOID p1, LPVOID p2, LPARAM lparam)
{	TRACE(shell,"%p %p\n", p1, p2);

	if (((LPSIC_ENTRY)p1)->dwSourceIndex != ((LPSIC_ENTRY)p2)->dwSourceIndex) /* first the faster one*/
	  return 1;

	if (strcasecmp(((LPSIC_ENTRY)p1)->sSourceFile,((LPSIC_ENTRY)p2)->sSourceFile))
	  return 1;

	return 0;  
}
/*****************************************************************************
*	SIC_IconAppend (internal)
* NOTES
*  appends a icon pair to the end of the cache
*/
static INT32 SIC_IconAppend (LPCSTR sSourceFile, INT32 dwSourceIndex, HICON32 hSmallIcon, HICON32 hBigIcon)
{	LPSIC_ENTRY lpsice;
	INT32 index, index1;
	
	TRACE(shell,"%s %i %x %x\n", sSourceFile, dwSourceIndex, hSmallIcon ,hBigIcon);

	lpsice = (LPSIC_ENTRY) SHAlloc (sizeof (SIC_ENTRY));

	lpsice->sSourceFile = HEAP_strdupA (GetProcessHeap(),0,sSourceFile);
	lpsice->dwSourceIndex = dwSourceIndex;
	
	index = pDPA_InsertPtr(hdpa, 0x7fff, lpsice);
	if ( INVALID_INDEX == index )
	{ SHFree(lpsice);
	  return INVALID_INDEX;
        } 

	index = pImageList_AddIcon (ShellSmallIconList, hSmallIcon);
	index1= pImageList_AddIcon (ShellBigIconList, hBigIcon);

	if (index!=index1)
	{ FIXME(shell,"iconlists out of sync 0x%x 0x%x\n", index, index1);
	}
	lpsice->dwListIndex = index;
	
	return lpsice->dwListIndex;
	
}
/****************************************************************************
*	SIC_LoadIcon	[internal]
* NOTES
*  gets small/big icon by number from a file
*/
static INT32 SIC_LoadIcon (LPCSTR sSourceFile, INT32 dwSourceIndex)
{	HICON32	hiconLarge=0;
	HICON32	hiconSmall=0;

	ICO_ExtractIconEx(sSourceFile, &hiconLarge, dwSourceIndex, 1, 32, 32  );
	ICO_ExtractIconEx(sSourceFile, &hiconSmall, dwSourceIndex, 1, 16, 16  );


	if ( !hiconLarge ||  !hiconSmall)
	{ FIXME(shell, "*** failure loading icon %i from %s (%x %x)\n", dwSourceIndex, sSourceFile, hiconLarge, hiconSmall);
	  return -1;
	}
	return SIC_IconAppend (sSourceFile, dwSourceIndex, hiconSmall, hiconLarge);		
}
/*****************************************************************************
*	SIC_GetIconIndex [internal]
* NOTES
*  look in the cache for a proper icon. if not available the icon is taken
*  from the file and cached
*/
static INT32 SIC_GetIconIndex (LPCSTR sSourceFile, INT32 dwSourceIndex )
{	SIC_ENTRY sice;
	INT32 index = INVALID_INDEX;
		
	TRACE(shell,"%s %i\n", sSourceFile, dwSourceIndex);

	sice.sSourceFile = sSourceFile;
	sice.dwSourceIndex = dwSourceIndex;
	
	if (NULL != pDPA_GetPtr (hdpa, 0))
	{ index = pDPA_Search (hdpa, &sice, -1L, SIC_CompareEntrys, 0, 0);
	}

	if ( INVALID_INDEX == index )
	{  return SIC_LoadIcon (sSourceFile, dwSourceIndex);
	}
	
	TRACE(shell, "-- found\n");
	return ((LPSIC_ENTRY)pDPA_GetPtr(hdpa, index))->dwListIndex;
}
/****************************************************************************
*	SIC_LoadIcon	[internal]
* NOTES
*  retrives the specified icon from the iconcache. if not found try's to load the icon
*/
static HICON32 SIC_GetIcon (LPCSTR sSourceFile, INT32 dwSourceIndex, BOOL32 bSmallIcon )
{	INT32 index;

	TRACE(shell,"%s %i\n", sSourceFile, dwSourceIndex);

	index = SIC_GetIconIndex(sSourceFile, dwSourceIndex);
	if (INVALID_INDEX == index)
	{ return INVALID_INDEX;
	}

	if (bSmallIcon)
	  return pImageList_GetIcon(ShellSmallIconList, index, ILD_NORMAL);
	return pImageList_GetIcon(ShellBigIconList, index, ILD_NORMAL);
	
}
/*****************************************************************************
*	SIC_Initialize [internal]
* NOTES
*  hack to load the resources from the shell32.dll under a different dll name 
*  will be removed when the resource-compiler is ready
*/
BOOL32 SIC_Initialize(void)
{	CHAR   		szShellPath[MAX_PATH];
	HGLOBAL32	hSmRet, hLgRet;
	HICON32		*pSmRet, *pLgRet;
	UINT32		index;
 
	TRACE(shell,"\n");

	if (hdpa)	/* already initialized?*/
	  return TRUE;
	  
  	hdpa = pDPA_Create(16);

	if (!hdpa)
	{ return(FALSE);
	}

	GetSystemDirectory32A(szShellPath,MAX_PATH);
	PathAddBackslash32A(szShellPath);
	strcat(szShellPath,"shell32.dll");

	hSmRet = GlobalAlloc32( GMEM_FIXED | GMEM_ZEROINIT, sizeof(HICON32)*40);
	hLgRet = GlobalAlloc32( GMEM_FIXED | GMEM_ZEROINIT, sizeof(HICON32)*40);

	pSmRet = (HICON32*)GlobalLock32(hSmRet);
	pLgRet = (HICON32*)GlobalLock32(hLgRet);

	ExtractIconEx32A ( szShellPath, 0, pLgRet, pSmRet, 40 );

	ShellSmallIconList = pImageList_Create(16,16,ILC_COLORDDB | ILC_MASK,0,0x20);
	ShellBigIconList = pImageList_Create(32,32,ILC_COLORDDB | ILC_MASK,0,0x20);

	for (index=0; index<40; index++)
	{ if (! pSmRet[index] )
	  { MSG("*** failure loading resources from %s\n", szShellPath );
	    MSG("*** this is a hack for loading the internal and external dll at the same time\n");
	    MSG("*** you can ignore it but you will miss some icons in win95 dialogs\n\n");
	    break;
	  }
	  SIC_IconAppend (szShellPath, index, pSmRet[index], pLgRet[index]);
	}
		
	GlobalUnlock32(hLgRet);
	GlobalFree32(hLgRet);

	GlobalUnlock32(hSmRet);
	GlobalFree32(hSmRet);

	TRACE(shell,"hIconSmall=%p hIconBig=%p\n",ShellSmallIconList, ShellBigIconList);

	return TRUE;
}

/*************************************************************************
 * Shell_GetImageList [SHELL32.71]
 *
 * PARAMETERS
 *  imglist[1|2] [OUT] pointer which recive imagelist handles
 *
 */
DWORD WINAPI Shell_GetImageList(HIMAGELIST * imglist1,HIMAGELIST * imglist2)
{	TRACE(shell,"(%p,%p)\n",imglist1,imglist2);
	if (imglist1)
	{ *imglist1=ShellBigIconList;
	}
	if (imglist2)
	{ *imglist2=ShellSmallIconList;
	}

	return TRUE;
}

/*************************************************************************
 * SHMapPIDLToSystemImageListIndex [SHELL32.77]
 *
 * PARAMETERS
 * x  pointer to an instance of IShellFolder 
 * 
 * NOTES
 *     first hack
 *
 */
DWORD WINAPI SHMapPIDLToSystemImageListIndex(LPSHELLFOLDER sh,LPITEMIDLIST pidl,DWORD z)
{	char	sTemp[MAX_PATH];
	DWORD	dwNr, ret = INVALID_INDEX;
	LPITEMIDLIST pidltemp = ILFindLastID(pidl);

 	WARN(shell,"(SF=%p,pidl=%p,0x%08lx)\n",sh,pidl,z);
	pdump(pidl);

	if (_ILIsDesktop(pidltemp))
	{ return 34;
	}
	else if (_ILIsMyComputer(pidltemp))
	{ if (HCR_GetDefaultIcon("CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}", sTemp, MAX_PATH, &dwNr))
	  { ret = SIC_GetIconIndex(sTemp, dwNr);
	    return (( INVALID_INDEX == ret) ? 15 : ret);
	  }
	}
	else if (_ILIsDrive (pidltemp))
	{ if (HCR_GetDefaultIcon("Drive", sTemp, MAX_PATH, &dwNr))
	  { ret = SIC_GetIconIndex(sTemp, dwNr);
	    return (( INVALID_INDEX == ret) ? 8 : ret);
	  }
	}
	else if (_ILIsFolder (pidltemp))
	{ if (HCR_GetDefaultIcon("Folder", sTemp, MAX_PATH, &dwNr))
	  { ret = SIC_GetIconIndex(sTemp, dwNr);
	    return (( INVALID_INDEX == ret) ? 3 : ret);
	  }
	}

	if (_ILGetExtension (pidltemp, sTemp, MAX_PATH))		/* object is file */
	{ if ( HCR_MapTypeToValue(sTemp, sTemp, MAX_PATH))
	  { if (HCR_GetDefaultIcon(sTemp, sTemp, MAX_PATH, &dwNr))
	    { if (!strcmp("%1",sTemp))					/* icon is in the file */
	      { _ILGetPidlPath(pidl, sTemp, MAX_PATH);
	        dwNr = 0;
	      }
	      ret = SIC_GetIconIndex(sTemp, dwNr);
	    }
	  }
	}
	return (( INVALID_INDEX == ret) ? 1 : ret);
}

/*************************************************************************
 * Shell_GetCachedImageIndex [SHELL32.72]
 *
 */
INT32 WINAPI Shell_GetCachedImageIndex32A(LPCSTR szPath, INT32 nIndex, DWORD z) 
{	WARN(shell,"(%s,%08x,%08lx) semi-stub.\n",debugstr_a(szPath),nIndex,z);
	return SIC_GetIconIndex(szPath, nIndex);
}

INT32 WINAPI Shell_GetCachedImageIndex32W(LPCWSTR szPath, INT32 nIndex, DWORD z) 
{	INT32 ret;
	LPSTR sTemp = HEAP_strdupWtoA (GetProcessHeap(),0,szPath);   
	
	WARN(shell,"(%s,%08x,%08lx) semi-stub.\n",debugstr_w(szPath),nIndex,z);

	ret = SIC_GetIconIndex(sTemp, nIndex);
	HeapFree(GetProcessHeap(),0,sTemp);
 	return ret;
}

INT32 WINAPI Shell_GetCachedImageIndex32AW(LPCVOID szPath, INT32 nIndex, DWORD z)
{	if( VERSION_OsIsUnicode())
	  return Shell_GetCachedImageIndex32W(szPath, nIndex, z);
	return Shell_GetCachedImageIndex32A(szPath, nIndex, z);
}

/*************************************************************************
*	ExtracticonEx32		[shell32.189]
*/
HICON32 WINAPI ExtractIconEx32AW ( LPCVOID lpszFile, INT32 nIconIndex, HICON32 * phiconLarge, HICON32 * phiconSmall, UINT32 nIcons )
{	if (VERSION_OsIsUnicode())
	  return ExtractIconEx32W ( lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
	return ExtractIconEx32A ( lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
}
/*************************************************************************
*	ExtracticonEx32A	[shell32.190]
* RETURNS
*  0 no icon found 
*  1 file is not valid
*  HICON32 handle of a icon (phiconLarge/Small == NULL)
*/
HICON32 WINAPI ExtractIconEx32A ( LPCSTR lpszFile, INT32 nIconIndex, HICON32 * phiconLarge, HICON32 * phiconSmall, UINT32 nIcons )
{	HICON32 ret=0;
	
	TRACE(shell,"file=%s idx=%i %p %p num=%i\n", lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons );

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
*	ExtracticonEx32W	[shell32.191]
*/
HICON32 WINAPI ExtractIconEx32W ( LPCWSTR lpszFile, INT32 nIconIndex, HICON32 * phiconLarge, HICON32 * phiconSmall, UINT32 nIcons )
{	LPSTR sFile;
	DWORD ret;
	
	TRACE(shell,"file=%s idx=%i %p %p num=%i\n", debugstr_w(lpszFile), nIconIndex, phiconLarge, phiconSmall, nIcons );

	sFile = HEAP_strdupWtoA (GetProcessHeap(),0,lpszFile);
	ret = ExtractIconEx32A ( sFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
	HeapFree(GetProcessHeap(),0,sFile);
	return ret;
}
