/************************************************************************
 * FILE.C     Copyright (C) 1993 John Burton
 *
 * File I/O routines for the Linux Wine Project.
 *
 * WARNING : Many options of OpenFile are not yet implemeted.
 *
 * NOV 93 Erik Bos (erik@(trashcan.)hacktic.nl
 *		- removed ParseDosFileName, and DosDrive structures.
 *		- structures dynamically configured at runtime.
 *		- _lopen modified to use GetUnixFileName.
 *
 * DEC 93 Erik Bos (erik@(trashcan.)hacktic.nl)
 *		- Existing functions modified to use dosfs functions.
 *		- Added _llseek, _lcreate, GetDriveType, GetTempDrive, 
 *		  GetWindowsDirectory, GetSystemDirectory, GetTempFileName.
 *
 ************************************************************************/

#define DEBUG_FILE

#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include "prototypes.h"

char WindowsDirectory[256], SystemDirectory[256], TempDirectory[256];


/***************************************************************************
 _lopen 

 Emulate the _lopen windows call
 ***************************************************************************/
int _lopen (LPSTR lpPathName, int iReadWrite)
{
  int  handle;
  char *UnixFileName;

#ifdef DEBUG_FILE
  fprintf (stderr, "_lopen: open %s\n", lpPathName);
#endif

  if ((UnixFileName = GetUnixFileName(lpPathName)) == NULL)
  	return HFILE_ERROR;

  handle =  open (UnixFileName, iReadWrite);

#ifdef DEBUG_FILE
  fprintf (stderr, "_lopen: open: %s (handle %d)\n", UnixFileName, handle);
#endif

  if (handle == -1)
  	return HFILE_ERROR;
  else
  	return handle;
}

/***************************************************************************
 _lread
 ***************************************************************************/
WORD _lread (int hFile, LPSTR lpBuffer, int wBytes)
{
  int result;

#ifdef DEBUG_FILE
  fprintf(stderr, "_lread: handle %d, buffer = %ld, length = %d\n",
	  		hFile, lpBuffer, wBytes);
#endif
  
  result = read (hFile, lpBuffer, wBytes);

  if (result == -1)
  	return HFILE_ERROR;
  else
  	return result;
}

/****************************************************************************
 _lwrite
****************************************************************************/
WORD _lwrite (int hFile, LPSTR lpBuffer, int wBytes)
{
	int result;

#ifdef DEBUG_FILE
  fprintf(stderr, "_lwrite: handle %d, buffer = %ld, length = %d\n",
	  		hFile, lpBuffer, wBytes);
#endif
	result = write (hFile, lpBuffer, wBytes);

	if (result == -1)
  		return HFILE_ERROR;
  	else
  		return result;
}

/***************************************************************************
 _lclose
 ***************************************************************************/
int _lclose (int hFile)
{
#ifdef DEBUG_FILE
  fprintf(stderr, "_lclose: handle %d\n", hFile);
#endif
  
  close (hFile);
}

/**************************************************************************
 OpenFile

 Warning:  This is nearly totally untested.  It compiles, that's it...
                                            -SL 9/13/93
 **************************************************************************/
int OpenFile (LPSTR lpFileName, LPOFSTRUCT ofs, WORD wStyle)
{
  int base,flags;

#ifdef DEBUG_FILE
  fprintf(stderr,"Openfile(%s,<struct>,%d) ",lpFileName,wStyle);
#endif

  base=wStyle&0xF;
  flags=wStyle&0xFFF0;
  
  flags&=0xFF0F;  /* strip SHARE bits for now */
  flags&=0xD7FF;  /* strip PROMPT & CANCEL bits for now */
  flags&=0x7FFF;  /* strip REOPEN bit for now */
  flags&=0xFBFF;  /* strib VERIFY bit for now */
  
  if(flags&OF_CREATE) { base |=O_CREAT; flags &=0xEFFF; }

  fprintf(stderr,"now %d,%d\n",base,flags);

  if(flags&(OF_DELETE|OF_EXIST))
    {
      fprintf(stderr,"Unsupported OpenFile option\n");
      return -1;
    }
  else
    {
      return _lopen (lpFileName, wStyle);
   }
}

/**************************************************************************
 SetHandleCount

 Changes the number of file handles available to the application.  Since
 Linux isn't limited to 20 files, this one's easy. - SL
 **************************************************************************/

#if !defined (OPEN_MAX)
/* This one is for the Sun */
#define OPEN_MAX _POSIX_OPEN_MAX
#endif
WORD SetHandleCount (WORD wNumber)
{
  printf("SetHandleCount(%d)\n",wNumber);
  return((wNumber<OPEN_MAX) ? wNumber : OPEN_MAX);
}

/***************************************************************************
 _llseek
 ***************************************************************************/
LONG _llseek (int hFile, LONG lOffset, int nOrigin)
{
	int origin;
	
#ifdef DEBUG_FILE
  fprintf(stderr, "_llseek: handle %d, offset %ld, origin %d\n", hFile, lOffset, nOrigin);
#endif

	switch (nOrigin) {
		case 1: origin = SEEK_CUR;
			break;
		case 2: origin = SEEK_END;
			break;
		default: origin = SEEK_SET;
			break;
		}

	return ( lseek(hFile, lOffset, origin) );
}

/***************************************************************************
 _lcreate
 ***************************************************************************/
LONG _lcreate (LPSTR lpszFilename, int fnAttribute)
{
	int handle;
	char *UnixFileName;

#ifdef DEBUG_FILE
	fprintf(stderr, "_lcreate: filename %s, attributes %d\n",lpszFilename, 
  			fnAttribute);
#endif

	if ((UnixFileName = GetUnixFileName(lpszFilename)) == NULL)
  		return HFILE_ERROR;

	handle =  open (UnixFileName, O_CREAT | O_TRUNC | O_WRONLY );

	if (handle == -1)
		return HFILE_ERROR;
	else
		return handle;
}

/***************************************************************************
 GetDriveType
 ***************************************************************************/
UINT GetDriveType(int drive)
{

#ifdef DEBUG_FILE
	fprintf(stderr,"GetDriveType %c:\n",'A'+drive);
#endif

	if (!DOS_ValidDrive(drive))
		return 0;

	if (drive == 0 || drive == 1)
		return DRIVE_REMOVABLE;
		 
	return DRIVE_REMOTE;
}

/***************************************************************************
 GetTempDrive
 ***************************************************************************/
BYTE GetTempDrive(BYTE chDriveLetter)
{
#ifdef DEBUG_FILE
	fprintf(stderr,"GetTempDrive (%d)\n",chDriveLetter);
#endif
	return('C');
}

/***************************************************************************
 GetWindowsDirectory
 ***************************************************************************/
UINT GetWindowsDirectory(LPSTR lpszSysPath, UINT cbSysPath)
{
	if (cbSysPath < strlen(WindowsDirectory)) 
		*lpszSysPath = 0;
	else
		strcpy(lpszSysPath, WindowsDirectory);
	
#ifdef DEBUG_FILE
	fprintf(stderr,"GetWindowsDirectory (%s)\n",lpszSysPath);
#endif

	return(strlen(lpszSysPath));
}
/***************************************************************************
 GetSystemDirectory
 ***************************************************************************/
UINT GetSystemDirectory(LPSTR lpszSysPath, UINT cbSysPath)
{
	if (cbSysPath < strlen(SystemDirectory))
		*lpszSysPath = 0;
	else
		strcpy(lpszSysPath, SystemDirectory);

#ifdef DEBUG_FILE
	fprintf(stderr,"GetSystemDirectory (%s)\n",lpszSysPath);
#endif

	return(strlen(lpszSysPath));
}
/***************************************************************************
 GetTempFileName
 ***************************************************************************/
int GetTempFileName(BYTE bDriveLetter, LPCSTR lpszPrefixString, UINT uUnique, LPSTR lpszTempFileName)
{
	int unique;
	char tempname[256];
	
	if (uUnique == 0)
		unique = time(NULL)%99999L;
	else
		unique = uUnique%99999L;

	strcpy(tempname,lpszPrefixString);
	tempname[3]='\0';

	sprintf(lpszTempFileName,"%s\%s%d.tmp",WindowsDirectory, tempname, 
		unique);

	ToDos(lpszTempFileName);

#ifdef DEBUG_FILE
	fprintf(stderr,"GetTempFilename: %c %s %d => %s\n",bDriveLetter,
		lpszPrefixString,uUnique,lpszTempFileName);
#endif

	return unique;
}

/***************************************************************************
 SetErrorMode
 ***************************************************************************/
WORD SetErrorMode(WORD x)
{
	fprintf(stderr,"wine: SetErrorMode %4x (ignored)\n",x);
}
