/************************************************************************
 * FILE.C     Copyright (C) 1993 John Burton
 *
 * File I/O routines for the Linux Wine Project.
 *
 * WARNING : Many options of OpenFile are not yet implemeted.
 *
 * NOV 93 Erik Bos (erik@xs4all.nl)
 *		- removed ParseDosFileName, and DosDrive structures.
 *		- structures dynamically configured at runtime.
 *		- _lopen modified to use DOS_GetUnixFileName.
 *		- Existing functions modified to use dosfs functions.
 *		- Added _llseek, _lcreat, GetDriveType, GetTempDrive, 
 *		  GetWindowsDirectory, GetSystemDirectory, GetTempFileName.
 *
 ************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>
#include "dos_fs.h"
#include "windows.h"
#include "msdos.h"
#include "options.h"
#include "stddebug.h"
#include "debug.h"

#define MAX_PATH 255

char WindowsDirectory[256], SystemDirectory[256], TempDirectory[256];

/***************************************************************************
 _lopen 

 Emulate the _lopen windows call
 ***************************************************************************/
INT _lopen (LPSTR lpPathName, INT iReadWrite)
{
  int  handle;
  char *UnixFileName;
  int mode = 0;

  dprintf_file(stddeb, "_lopen: open('%s', %X);\n", lpPathName, iReadWrite);
  if ((UnixFileName = DOS_GetUnixFileName(lpPathName)) == NULL)
  	return HFILE_ERROR;
  switch(iReadWrite & 3)
  {
  case OF_READ:      mode = O_RDONLY; break;
  case OF_WRITE:     mode = O_WRONLY; break;
  case OF_READWRITE: mode = O_RDWR; break;
  }
  handle = open( UnixFileName, mode );
  if (( handle == -1 ) && Options.allowReadOnly)
    handle = open( UnixFileName, O_RDONLY );

  dprintf_file(stddeb, "_lopen: open: %s (handle %d)\n", UnixFileName, handle);

  if (handle == -1)
  	return HFILE_ERROR;
  else
  	return handle;
}

/***************************************************************************
 _lread
 ***************************************************************************/
INT _lread (INT hFile, LPSTR lpBuffer, WORD wBytes)
{
  int result;

  dprintf_file(stddeb, "_lread: handle %d, buffer = %ld, length = %d\n",
	  		hFile, (long) lpBuffer, wBytes);
  
  result = read (hFile, lpBuffer, wBytes);

  if (result == -1)
  	return HFILE_ERROR;
  else
  	return result;
}

/****************************************************************************
 _lwrite
****************************************************************************/
INT _lwrite (INT hFile, LPCSTR lpBuffer, WORD wBytes)
{
    int result;

    dprintf_file(stddeb, "_lwrite: handle %d, buffer = %ld, length = %d\n",
		 hFile, (long) lpBuffer, wBytes);

    result = write (hFile, lpBuffer, wBytes);

    if (result == -1)
        return HFILE_ERROR;
    else
        return result;
}

/***************************************************************************
 _lclose
 ***************************************************************************/
INT _lclose (INT hFile)
{
    dprintf_file(stddeb, "_lclose: handle %d\n", hFile);
    
    if (hFile == 1 || hFile == 2) {
	fprintf( stderr, "Program attempted to close stdout or stderr!\n" );
	return 0;
    }
    if (close (hFile))
        return HFILE_ERROR;
    else
        return 0;
}

/**************************************************************************
 OpenFile
 **************************************************************************/
INT OpenFile (LPSTR lpFileName, LPOFSTRUCT ofs, WORD wStyle)
{
    char         filename[MAX_PATH+1];
    int          action;
    struct stat  s;
    struct tm   *now;
    int          res, handle;
    int          verify_time = 0;
  
    dprintf_file(stddeb,"Openfile(%s,<struct>,%d)\n",lpFileName,wStyle);
  
    action = wStyle & 0xff00;
  
  
    /* OF_CREATE is completly different from all other options, so
       handle it first */

    if (action & OF_CREATE)
    {
        char *unixfilename;

        if (!(action & OF_REOPEN)) strcpy(ofs->szPathName, lpFileName);
        ofs->cBytes = sizeof(OFSTRUCT);
        ofs->fFixedDisk = FALSE;
        ofs->nErrCode = 0;
        *((int*)ofs->reserved) = 0;

        if ((unixfilename = DOS_GetUnixFileName (ofs->szPathName)) == NULL)
        {
            errno_to_doserr();
            ofs->nErrCode = ExtendedError;
            return -1;
        }
        handle = open (unixfilename, (wStyle & 0x0003) | O_CREAT, 0x666);
        if (handle == -1)
        {
            errno_to_doserr();
            ofs->nErrCode = ExtendedError;
        }   
        return handle;
    }


    /* If path isn't given, try to find the file. */

    if (!(action & OF_REOPEN))
    {
	char temp[MAX_PATH + 1];
	
	if(index(lpFileName,'\\') || index(lpFileName,'/') || 
	   index(lpFileName,':')) 
	{
	    strcpy (filename,lpFileName);
	    goto found;
	}
	/* Try current directory */
	if (DOS_FindFile(temp, MAX_PATH, lpFileName, NULL, ".")) {
	    strcpy(filename, DOS_GetDosFileName(temp));
	    goto found;
	}

	/* Try Windows directory */
	GetWindowsDirectory(filename, MAX_PATH);
	if (DOS_FindFile(temp, MAX_PATH, lpFileName, NULL, filename)) {
	    strcpy(filename, DOS_GetDosFileName(temp));
	    goto found;
	}
	
	/* Try Windows system directory */
	GetSystemDirectory(filename, MAX_PATH);
	if (DOS_FindFile(temp, MAX_PATH, lpFileName, NULL, filename)) {
	    strcpy(filename, DOS_GetDosFileName(temp));
	    goto found;
	}

	/* Try the path of the current executable */
	if (GetCurrentTask())
	{
	    char *p;
	    GetModuleFileName( GetCurrentTask(), filename, MAX_PATH );
	    if ((p = strrchr( filename, '\\' )))
	    {
		p[1] = '\0';
		if (DOS_FindFile(temp, MAX_PATH, lpFileName, NULL, filename)) {
		    strcpy(filename, DOS_GetDosFileName(temp));
		    goto found;
		}
	    }
	}

	/* Try all directories in path */

	if (DOS_FindFile(temp,MAX_PATH,lpFileName,NULL,WindowsPath))
	{
	    strcpy(filename, DOS_GetDosFileName(temp));
	    goto found;
	}
	/* ??? shouldn't we give an error here? */
	strcpy (filename, lpFileName);
	
	found:
	
	ofs->cBytes = sizeof(OFSTRUCT);
	ofs->fFixedDisk = FALSE;
	strcpy(ofs->szPathName, filename);
	ofs->nErrCode = 0;
        if (!(action & OF_VERIFY))
          *((int*)ofs->reserved) = 0;
    }
    

    if (action & OF_PARSE)
      return 0;

    if (action & OF_DELETE)
      return unlink(ofs->szPathName);


    /* Now on to getting some information about that file */

    if ((res = stat(DOS_GetUnixFileName(ofs->szPathName), &s)))
      {
      errno_to_doserr();
      ofs->nErrCode = ExtendedError;
      return -1;
    }
    
    now = localtime (&s.st_mtime);

    if (action & OF_VERIFY)
      verify_time = *((int*)ofs->reserved);
    
    *((WORD*)(&ofs->reserved[2]))=
         ((now->tm_hour * 0x2000) + (now->tm_min * 0x20) + (now->tm_sec / 2));
    *((WORD*)(&ofs->reserved[0]))=
         ((now->tm_year * 0x200) + (now->tm_mon * 0x20) + now->tm_mday);


    if (action & OF_VERIFY)
      return (verify_time != *((int*)ofs->reserved));

    if (action & OF_EXIST)
      return 0;
    
    if ((handle = _lopen( ofs->szPathName, wStyle )) == -1)
    {
        ofs->nErrCode = 2;  /* not found */
        return -1;
    }
    return handle;
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
  dprintf_file(stddeb,"SetHandleCount(%d)\n",wNumber);
  return((wNumber<OPEN_MAX) ? wNumber : OPEN_MAX);
}

/***************************************************************************
 _llseek
 ***************************************************************************/
LONG _llseek (INT hFile, LONG lOffset, INT nOrigin)
{
	int origin;
	
  	dprintf_file(stddeb, "_llseek: handle %d, offset %ld, origin %d\n", 
		hFile, lOffset, nOrigin);

	switch (nOrigin) {
		case 1: origin = SEEK_CUR;
			break;
		case 2: origin = SEEK_END;
			break;
		default: origin = SEEK_SET;
			break;
	}

	return lseek(hFile, lOffset, origin);
}

/***************************************************************************
 _lcreat
 ***************************************************************************/
INT _lcreat (LPSTR lpszFilename, INT fnAttribute)
{
	int handle;
	char *UnixFileName;

    	dprintf_file(stddeb, "_lcreat: filename %s, attributes %d\n",
		lpszFilename, fnAttribute);
	if ((UnixFileName = DOS_GetUnixFileName(lpszFilename)) == NULL)
  		return HFILE_ERROR;
	handle =  open (UnixFileName, O_CREAT | O_TRUNC | O_WRONLY, 426);

	if (handle == -1)
		return HFILE_ERROR;
	else
		return handle;
}

/***************************************************************************
 GetDriveType
 ***************************************************************************/
UINT GetDriveType(INT drive)
{

    	dprintf_file(stddeb,"GetDriveType %c:\n",'A'+drive);

	if (!DOS_ValidDrive(drive))
		return DRIVE_DOESNOTEXIST;

	if (drive == 0 || drive == 1)
		return DRIVE_REMOVABLE;
		 
	return DRIVE_FIXED;
}

/***************************************************************************
 GetTempDrive
 ***************************************************************************/
BYTE GetTempDrive(BYTE chDriveLetter)
{
    dprintf_file(stddeb,"GetTempDrive (%d)\n",chDriveLetter);
    if (TempDirectory[1] == ':') return TempDirectory[0];
    else return 'C';
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
	
    	dprintf_file(stddeb,"GetWindowsDirectory (%s)\n",lpszSysPath);

	ChopOffSlash(lpszSysPath);
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

    	dprintf_file(stddeb,"GetSystemDirectory (%s)\n",lpszSysPath);

	ChopOffSlash(lpszSysPath);
	return(strlen(lpszSysPath));
}
/***************************************************************************
 GetTempFileName
 ***************************************************************************/
INT GetTempFileName(BYTE bDriveLetter, LPCSTR lpszPrefixString, UINT uUnique, LPSTR lpszTempFileName)
{
	int unique;
	int handle;
	char tempname[256];
	
	if (uUnique == 0)
		unique = time(NULL)%99999L;
	else
		unique = uUnique%99999L;

	strcpy(tempname,lpszPrefixString);
	tempname[3]='\0';

	sprintf(lpszTempFileName,"%s\\%s%d.tmp", TempDirectory, tempname, 
		unique);

	ToDos(lpszTempFileName);

    	dprintf_file(stddeb,"GetTempFilename: %c %s %d => %s\n",bDriveLetter,
		lpszPrefixString,uUnique,lpszTempFileName);
	if ((handle = _lcreat (lpszTempFileName, 0x0000)) == -1) {
		fprintf(stderr,"GetTempFilename: can't create temp file '%s' !\n", lpszTempFileName);
	}
	else
		close(handle);

	return unique;
}

/***************************************************************************
 SetErrorMode
 ***************************************************************************/
WORD SetErrorMode(WORD x)
{
    dprintf_file(stdnimp,"wine: SetErrorMode %4x (ignored)\n",x);

    return 1;
}

/***************************************************************************
 _hread
 ***************************************************************************/
LONG _hread(INT hf, LPSTR hpvBuffer, LONG cbBuffer)
{
	return read(hf, hpvBuffer, cbBuffer);
}
/***************************************************************************
 _hwrite
 ***************************************************************************/
LONG _hwrite(INT hf, LPCSTR hpvBuffer, LONG cbBuffer)
{
	return write(hf, hpvBuffer, cbBuffer);
}
