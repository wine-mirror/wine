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
* The structure of the pidl seems to be a union. The first byte of the
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

#define PT_DESKTOP	0x00 /* internal */
#define PT_MYCOMP	0x1F
#define PT_SPECIAL	0x2E
#define PT_DRIVE	0x23
#define PT_FOLDER	0x31
#define PT_VALUE	0x32

#pragma pack(1)
typedef BYTE PIDLTYPE;

typedef struct tagPIDLDATA
{	PIDLTYPE type;			/*00*/
	union
	{ struct
	  { CHAR szDriveName[4];	/*01*/
	    /* end of MS compatible*/
	    DWORD dwSFGAO;		/*05*/
	    /* the drive seems to be 19 bytes every time */
	  } drive;
	  struct 
	  { BYTE dummy;			/*01 is 0x00 for files or dirs */
	    DWORD dwFileSize;		/*02*/
	    WORD uFileDate;		/*06*/
	    WORD uFileTime;		/*08*/
	    WORD uFileAttribs;		/*10*/
	    CHAR szNames[1];		/*12*/
	    /* Here are comming two strings. The first is the long name. 
	    The second the dos name when needed or just 0x00 */
	  } file, folder, generic; 
	}u;
} PIDLDATA, *LPPIDLDATA;
#pragma pack(4)

/*
 * getting string values from pidls
 *
 * return value is strlen()
 */
DWORD WINAPI _ILGetDrive(LPCITEMIDLIST,LPSTR,UINT16);
DWORD WINAPI _ILGetItemText(LPCITEMIDLIST,LPSTR,UINT16);
DWORD WINAPI _ILGetFolderText(LPCITEMIDLIST,LPSTR,DWORD);
DWORD WINAPI _ILGetValueText(LPCITEMIDLIST,LPSTR,DWORD);
DWORD WINAPI _ILGetPidlPath(LPCITEMIDLIST,LPSTR,DWORD);

/*
 * getting special values from simple pidls
 */
BOOL32 WINAPI _ILGetFileDate (LPCITEMIDLIST pidl, LPSTR pOut, UINT32 uOutSize);
BOOL32 WINAPI _ILGetFileSize (LPCITEMIDLIST pidl, LPSTR pOut, UINT32 uOutSize);
BOOL32 WINAPI _ILGetExtension (LPCITEMIDLIST pidl, LPSTR pOut, UINT32 uOutSize);


/*
 * testing simple pidls
 */
BOOL32 WINAPI _ILIsDesktop(LPCITEMIDLIST);
BOOL32 WINAPI _ILIsMyComputer(LPCITEMIDLIST);
BOOL32 WINAPI _ILIsDrive(LPCITEMIDLIST);
BOOL32 WINAPI _ILIsFolder(LPCITEMIDLIST);
BOOL32 WINAPI _ILIsValue(LPCITEMIDLIST);

/*
 * simple pidls from strings
 */
LPITEMIDLIST WINAPI _ILCreateDesktop(void);
LPITEMIDLIST WINAPI _ILCreateMyComputer(void);
LPITEMIDLIST WINAPI _ILCreateDrive(LPCSTR);
LPITEMIDLIST WINAPI _ILCreateFolder(LPCSTR, LPCSTR);
LPITEMIDLIST WINAPI _ILCreateValue(LPCSTR, LPCSTR);

/*
 * raw pidl handling (binary) 
 *
 * data is binary / sizes are bytes
 */
DWORD WINAPI _ILGetData(PIDLTYPE,LPCITEMIDLIST,LPVOID,UINT32);
LPITEMIDLIST WINAPI _ILCreate(PIDLTYPE,LPVOID,UINT16);

/*
 * helper functions (getting struct-pointer)
 */
LPPIDLDATA WINAPI _ILGetDataPointer(LPCITEMIDLIST);
LPSTR WINAPI _ILGetTextPointer(PIDLTYPE type, LPPIDLDATA pidldata);
LPSTR WINAPI _ILGetSTextPointer(PIDLTYPE type, LPPIDLDATA pidldata);

void pdump (LPCITEMIDLIST pidl);
#endif
