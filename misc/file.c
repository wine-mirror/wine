/************************************************************************
 * FILE.C     Copyright (C) 1993 John Burton
 *
 * File I/O routines for the Linux Wine Project.
 *
 * There are two main parts to this module - first there are the
 * actual emulation functions, and secondly a routine for translating
 * DOS filenames into UNIX style filenames.
 *
 * For each DOS drive letter, we need to store the location in the unix
 * file system to map that drive to, and the current directory for that
 * drive.
 *
 * Finally we need to store the current drive letter in this module.
 *
 * WARNING : Many options of OpenFile are not yet implemeted.
 *
 * 
 ************************************************************************/

#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>

#define OPEN_MAX 256

/***************************************************************************
 This structure stores the infomation needed for a single DOS drive
 ***************************************************************************/
struct DosDriveStruct
{
  char RootDirectory [256];    /* Unix base for this drive letter */
  char CurrentDirectory [256]; /* Current directory for this drive */
};

/***************************************************************************
 Table for DOS drives
 ***************************************************************************/
struct DosDriveStruct DosDrives[] = 
{
  {"/mnt/floppy1" , ""},
  {"/mnt/floppy2" , ""},
  {"/mnt/HardDisk", "/wine"},
  {""             , "/wine/Wine"}
};

#define NUM_DRIVES (sizeof (DosDrives) / sizeof (struct DosDriveStruct))


/**************************************************************************
 Global variable to store current drive letter (0 = A, 1 = B etc)
 **************************************************************************/
int CurrentDrive = 2;

/**************************************************************************
 ParseDOSFileName

 Return a fully specified DOS filename with the disk letter separated from
 the path information.
 **************************************************************************/
void ParseDOSFileName (char *UnixFileName, char *DosFileName, int *Drive)
{
  int character;

  for (character = 0; character < strlen(DosFileName); character++)
    {
      if (DosFileName[character] == '\\')
	DosFileName[character] = '/';
    }


  if (DosFileName[1] == ':')
    {
      *Drive = DosFileName[0] - 'A';
      if (*Drive > 26)
	*Drive = *Drive - 'a' + 'A';
      DosFileName = DosFileName + 2;
    }
  else
    *Drive = CurrentDrive;

  if (DosFileName[0] == '/')
    {
      strcpy (UnixFileName, DosDrives[*Drive].RootDirectory);
      strcat (UnixFileName, DosFileName);
      return;
    }

  strcpy (UnixFileName, DosDrives[*Drive].RootDirectory);
  strcat (UnixFileName, DosDrives[*Drive].CurrentDirectory);
  strcat (UnixFileName, "/");
  strcat (UnixFileName, DosFileName);
  return;
}


/***************************************************************************
 _lopen 

 Emulate the _lopen windows call
 ***************************************************************************/
WORD KERNEL__lopen (LPSTR lpPathName, WORD iReadWrite)
{
  char UnixFileName[256];
  int  Drive;
  int  handle;

  ParseDOSFileName (UnixFileName, lpPathName, &Drive);



  handle =  open (UnixFileName, iReadWrite);

#ifdef DEBUG_FILE
  fprintf (stderr, "_lopen: %s (Handle = %d)\n", UnixFileName, handle);
#endif
  return handle;
}

/***************************************************************************
 _lread
 ***************************************************************************/
WORD KERNEL__lread (WORD hFile, LPSTR lpBuffer, WORD wBytes)
{
  int result;
  
  result = read (hFile, lpBuffer, wBytes);
#ifdef DEBUG_FILE
  fprintf(stderr, "_lread: hFile = %d, lpBuffer = %s, wBytes = %d\n",
	  hFile, lpBuffer, wBytes);
#endif
  return result;
}


/****************************************************************************
 _lwrite
****************************************************************************/
WORD KERNEL__lwrite (WORD hFile, LPSTR lpBuffer, WORD wBytes)
{
#ifdef DEBUG_FILE
  fprintf(stderr, "_lwrite: hFile = %d, lpBuffer = %s, wBytes = %d\n",
	  hFile, lpBuffer, wBytes);
#endif
  return write (hFile, lpBuffer, wBytes);
}

/***************************************************************************
 _lclose
 ***************************************************************************/
WORD KERNEL__lclose (WORD hFile)
{
  close (hFile);
}

/**************************************************************************
 OpenFile

 Warning:  This is nearly totally untested.  It compiles, that's it...
                                            -SL 9/13/93
 **************************************************************************/
WORD KERNEL_OpenFile (LPSTR lpFileName, LPOFSTRUCT ofs, WORD wStyle)
{
  int base,flags;

  fprintf(stderr,"Openfile(%s,<struct>,%d) ",lpFileName,wStyle);

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
      return KERNEL__lopen (lpFileName, wStyle);
   }
}

/**************************************************************************
 SetHandleCount

 Changes the number of file handles available to the application.  Since
 Linux isn't limited to 20 files, this one's easy. - SL
 **************************************************************************/

WORD SetHandleCount (WORD wNumber)
{
  printf("SetHandleCount(%d)\n",wNumber);
  return((wNumber<OPEN_MAX) ? wNumber : OPEN_MAX);
}





