/*
 * internal pidl functions
 * 1998 <juergen.schmied@metronet.de>
 *
 * DO NOT use this definitions outside the shell32.dll !
 *
 * The contents of a pidl should never used from a application
 * directly. 
 *
 * This stuff is used from SHGetFileAttributes, ShellFolder 
 * EnumIDList and ShellView.
 */
 
#ifndef __WINE_PIDL_H
#define __WINE_PIDL_H

#include "shlobj.h"

/* 
* the pidl does cache fileattributes to speed up SHGetAttributes when
* displaying a big number of files.
*
* a pidl of NULL means the desktop
*
* The structure of the pidl seens to be a union. The first byte of the
* PIDLDATA desribes the type of pidl.
*
* first byte - 	my Computer	0x1F
*		control/printer	0x2E
*		drive		0x23 
*		folder	 	0x31 
* drive: the second byte is the start of a string
*	  C  :  \ 
*	 43 3A 5C
* file: see the PIDLDATA structure	 
*/

#define PT_DESKTOP	0x00 /*fixme*/
#define PT_MYCOMP	0x1F
#define PT_SPECIAL	0x2E
#define PT_DRIVE	0x23
#define PT_FOLDER	0x31
#define PT_VALUE	0x33 /*fixme*/

#pragma pack(1)
typedef BYTE PIDLTYPE;

typedef struct tagPIDLDATA
{	PIDLTYPE type;
	union
	{ struct
	  { CHAR szDriveName[4];
	    /* end of MS compatible*/
	    DWORD dwSFGAO;
	  } drive;
	  struct 
	  { BYTE dummy;
	    DWORD dwFileSize;
	    WORD uFileDate;
	    WORD uFileTime;
	    WORD uFileAttribs;
	    /* end of MS compatible*/
	    DWORD dwSFGAO;
	    CHAR  szAlternateName[14];	/* the 8.3 Name*/
	    CHAR szText[1];		/* last entry, variable size */
	  } file, folder, generic; 
	}u;
} PIDLDATA, *LPPIDLDATA;
#pragma pack(4)

LPITEMIDLIST WINAPI _ILCreateDesktop(void);
LPITEMIDLIST WINAPI _ILCreateMyComputer(void);
LPITEMIDLIST WINAPI _ILCreateDrive(LPCSTR);
LPITEMIDLIST WINAPI _ILCreateFolder(LPCSTR);
LPITEMIDLIST WINAPI _ILCreateValue(LPCSTR);
LPITEMIDLIST WINAPI _ILCreate(PIDLTYPE,LPVOID,UINT16);

BOOL32 WINAPI _ILGetDrive(LPCITEMIDLIST,LPSTR,UINT16);
DWORD WINAPI _ILGetItemText(LPCITEMIDLIST,LPSTR,UINT16);
DWORD WINAPI _ILGetFolderText(LPCITEMIDLIST,LPSTR,DWORD);
DWORD WINAPI _ILGetValueText(LPCITEMIDLIST,LPSTR,DWORD);
DWORD WINAPI _ILGetDataText(LPCITEMIDLIST,LPCITEMIDLIST,LPSTR,DWORD);
DWORD WINAPI _ILGetPidlPath(LPCITEMIDLIST,LPSTR,DWORD);
DWORD WINAPI _ILGetData(PIDLTYPE,LPCITEMIDLIST,LPVOID,UINT32);

BOOL32 WINAPI _ILIsDesktop(LPCITEMIDLIST);
BOOL32 WINAPI _ILIsMyComputer(LPCITEMIDLIST);
BOOL32 WINAPI _ILIsDrive(LPCITEMIDLIST);
BOOL32 WINAPI _ILIsFolder(LPCITEMIDLIST);
BOOL32 WINAPI _ILIsValue(LPCITEMIDLIST);

BOOL32 WINAPI _ILHasFolders(LPSTR,LPCITEMIDLIST);

LPPIDLDATA WINAPI _ILGetDataPointer(LPCITEMIDLIST);
LPSTR WINAPI _ILGetTextPointer(PIDLTYPE type, LPPIDLDATA pidldata);
BOOL32 WINAPI _ILGetFileDate (LPCITEMIDLIST pidl, LPSTR pOut, UINT32 uOutSize);
BOOL32 WINAPI _ILGetFileSize (LPCITEMIDLIST pidl, LPSTR pOut, UINT32 uOutSize);
BOOL32 WINAPI _ILGetExtension (LPCITEMIDLIST pidl, LPSTR pOut, UINT32 uOutSize);

void pdump (LPCITEMIDLIST pidl);
#endif
