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

  dprintf_file(stddeb, "_lopen: open('%s', %X);\n", lpPathName, iReadWrite);
  if ((UnixFileName = DOS_GetUnixFileName(lpPathName)) == NULL)
  	return HFILE_ERROR;
  iReadWrite &= 0x000F;
  handle =  open (UnixFileName, iReadWrite);
  if( ( handle == -1 ) && Options.allowReadOnly )
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
INT _lwrite (INT hFile, LPSTR lpBuffer, WORD wBytes)
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
#ifdef WINELIB
    dprintf_file(stdnimp, "OpenFile: not implemented\n");
#else
    char                      filename[MAX_PATH+1];
    int                       action;
    struct stat               s;
    struct tm                 *now;
    int                       res;
    int                       verify_time;
  
    dprintf_file(stddeb,"Openfile(%s,<struct>,%d) ",lpFileName,wStyle);
  
    action = wStyle & 0xff00;
  
  
    /* OF_CREATE is completly different from all other options, so
       handle it first */

    if (action & OF_CREATE)
      {
      int handle;
      char *unixfilename;

      if (!(action & OF_REOPEN))
        strcpy(ofs->szPathName, lpFileName);
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
	if( !( index(lpFileName,'\\') || index(lpFileName,'/') || 
	      index(lpFileName,':')))
	while(1)
	{
	  char temp[MAX_PATH+1];
	  strcpy (filename, lpFileName);
	  if ( (!stat(DOS_GetUnixFileName(filename), &s)) && (S_ISREG(s.st_mode)) )
	    break;
	  GetWindowsDirectory (filename,MAX_PATH);
	  if ((!filename[0])||(filename[strlen(filename)-1]!='\\'))
            strcat(filename, "\\");
	  strcat (filename, lpFileName);
	  if ( (!stat(DOS_GetUnixFileName(filename), &s)) && (S_ISREG(s.st_mode)) )
	    break;
	  GetSystemDirectory (filename,MAX_PATH);
	  if ((!filename[0])||(filename[strlen(filename)-1]!='\\'))
 	    strcat(filename, "\\");
	  strcat (filename, lpFileName);
	  if ( (!stat(DOS_GetUnixFileName(filename), &s)) && (S_ISREG(s.st_mode)) )
	    break;
	  if (!DOS_FindFile(temp,MAX_PATH,lpFileName,NULL,WindowsPath))
	    {
	      strcpy(filename, DOS_GetDosFileName(temp));
	      break;
	    }
	  strcpy (filename, lpFileName);
	  break;
	}
	else
	  strcpy (filename,lpFileName);

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
    
   /* Now we are actually going to open the file. According to Microsoft's
       Knowledge Basis, this is done by calling int 21h, ax=3dh. */    

#if 0
    AX = 0x3d00;
    AL = (AL & 0x0f) | (wStyle & 0x70); /* Handle OF_SHARE_xxx etc. */
    AL = (AL & 0xf0) | (wStyle & 0x03); /* Handle OF_READ etc. */
    DS = SELECTOROF(ofs->szPathName);
    DX = OFFSETOF(ofs->szPathName);
  
    OpenExistingFile (context);

    if (EFL & 0x00000001)     /* Cflag */
    {
      ofs->nErrCode = AL;
      return -1;
      }

    return AX;
#endif
      /* FIXME: Quick hack to get it to work without calling DOS  --AJ */
    {
        int mode, handle;
        switch(wStyle & 3)
        {
            case 0: mode = O_RDONLY; break;
            case 1: mode = O_WRONLY; break;
            default: mode = O_RDWR; break;
        }
        if ((handle = open(DOS_GetUnixFileName(ofs->szPathName), mode)) == -1)
        {
            ofs->nErrCode = 2;  /* not found */
            return -1;
        }
          /* don't bother with locking for now */

        return handle;
    }
#endif /*WINELIB*/
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

	return ( lseek(hFile, lOffset, origin) );
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
long _hread(int hf, void FAR *hpvBuffer, long cbBuffer)
{
	return read(hf, hpvBuffer, cbBuffer);
}
/***************************************************************************
 _hwrite
 ***************************************************************************/
long _hwrite(int hf, const void FAR *hpvBuffer, long cbBuffer)
{
	return write(hf, hpvBuffer, cbBuffer);
}
