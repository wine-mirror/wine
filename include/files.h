#ifndef _FILES_H
#define _FILES_H

#define OPEN_MAX 256

/***************************************************************************
 This structure stores the infomation needed for a single DOS drive
 ***************************************************************************/
struct DosDriveStruct
{
  char RootDirectory [256];    /* Unix base for this drive letter */
  char CurrentDirectory [256]; /* Current directory for this drive */
  char VolumeLabel [11];
  unsigned long	serialnumber;
};

#endif /*_FILES_H*/
