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
*	object        ! first byte /  ! format       ! living space
*	              ! size
*	----------------------------------------------------------------
*	my computer	0x1F/20		mycomp (2)	(usual)
*	drive		0x23/25		drive		(usual)
*	drive		0x25/25		drive		(lnk/persistant)
*	drive		0x29/25		drive
*	control/printer	0x2E
*	drive		0x2F		drive		(lnk/persistant)
*	folder/file	0x30		folder/file (1)	(lnk/persistant)
*	folder		0x31		folder		(usual)
*	value		0x32		file		(usual)
*	workgroup	0x41		network (3)
*	computer	0x42		network (4)
*	whole network	0x47		network (5)
*	history/favorites 0xb1		file
*	share		0xc3		metwork (6)
*
* guess: the persistant elements are non tracking
*
* (1) dummy byte is used, attributes are empty
* (2) IID_MyComputer = 20D04FE0L-3AEA-1069-A2D8-08002B30309D
* (3) two strings	"workgroup" "microsoft network"
* (4) one string	"\\sirius"
* (5) one string	"whole network" 
* (6) one string	"\\sirius\c"
*/

#define PT_DESKTOP	0x00 /* internal */
#define PT_MYCOMP	0x1F
#define PT_DRIVE	0x23
#define PT_DRIVE2	0x25
#define PT_DRIVE3	0x29
#define PT_SPECIAL	0x2E
#define PT_DRIVE1	0x2F
#define PT_FOLDER1	0x30
#define PT_FOLDER	0x31
#define PT_VALUE	0x32
#define PT_WORKGRP	0x41
#define PT_COMP		0x42
#define PT_NETWORK	0x47
#define PT_IESPECIAL	0xb1
#define PT_SHARE	0xc3

#include "pshpack1.h"
typedef BYTE PIDLTYPE;

typedef struct tagPIDLDATA
{	PIDLTYPE type;			/*00*/
	union
	{ struct
	  { BYTE dummy;
	    GUID guid;
	  } mycomp;
	  struct
	  { CHAR szDriveName[20];	/*01*/
	    DWORD dwUnknown;		/*21*/
	    /* the drive seems to be 25 bytes every time */
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
	  struct
	  { WORD dummy;		/*01*/
	    CHAR szNames[1];	/*03*/
	  } network;
	}u;
} PIDLDATA, *LPPIDLDATA;
#include "poppack.h"

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
BOOL WINAPI _ILGetFileDate (LPCITEMIDLIST pidl, LPSTR pOut, UINT uOutSize);
BOOL WINAPI _ILGetFileSize (LPCITEMIDLIST pidl, LPSTR pOut, UINT uOutSize);
BOOL WINAPI _ILGetExtension (LPCITEMIDLIST pidl, LPSTR pOut, UINT uOutSize);


/*
 * testing simple pidls
 */
BOOL WINAPI _ILIsDesktop(LPCITEMIDLIST);
BOOL WINAPI _ILIsMyComputer(LPCITEMIDLIST);
BOOL WINAPI _ILIsDrive(LPCITEMIDLIST);
BOOL WINAPI _ILIsFolder(LPCITEMIDLIST);
BOOL WINAPI _ILIsValue(LPCITEMIDLIST);

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
DWORD WINAPI _ILGetData(PIDLTYPE,LPCITEMIDLIST,LPVOID,UINT);
LPITEMIDLIST WINAPI _ILCreate(PIDLTYPE,LPCVOID,UINT16);

/*
 * helper functions (getting struct-pointer)
 */
LPPIDLDATA WINAPI _ILGetDataPointer(LPCITEMIDLIST);
LPSTR WINAPI _ILGetTextPointer(PIDLTYPE type, LPPIDLDATA pidldata);
LPSTR WINAPI _ILGetSTextPointer(PIDLTYPE type, LPPIDLDATA pidldata);

LPITEMIDLIST WINAPI ILFindChild(LPCITEMIDLIST pidl1,LPCITEMIDLIST pidl2);

void pdump (LPCITEMIDLIST pidl);
BOOL pcheck (LPCITEMIDLIST pidl);
#endif
