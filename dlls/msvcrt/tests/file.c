/*
 * Unit test suite for file functions
 *
 * Copyright 2002 Bill Currie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "wine/test.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>
#include <windef.h>
#include <winbase.h>
#include <winnls.h>

static void test_fdopen( void )
{
    static char buffer[] = {0,1,2,3,4,5,6,7,8,9};
    int fd;
    FILE *file;

    fd = open ("fdopen.tst", O_WRONLY | O_CREAT | O_BINARY, _S_IREAD |_S_IWRITE);
    write (fd, buffer, sizeof (buffer));
    close (fd);

    fd = open ("fdopen.tst", O_RDONLY | O_BINARY);
    lseek (fd, 5, SEEK_SET);
    file = fdopen (fd, "rb");
    ok (fread (buffer, 1, sizeof (buffer), file) == 5, "read wrong byte count");
    ok (memcmp (buffer, buffer + 5, 5) == 0, "read wrong bytes");
    fclose (file);
    unlink ("fdopen.tst");
}

static WCHAR* AtoW( char* p )
{
    WCHAR* buffer;
    DWORD len = MultiByteToWideChar( CP_ACP, 0, p, -1, NULL, 0 );
    buffer = malloc( len * sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, p, -1, buffer, len );
    return buffer;
}

static void test_fgetwc( void )
{
#define LLEN 512

  char* tempf;
  FILE *tempfh;
  const char mytext[]= "This is test_fgetwc\n";
  WCHAR wtextW[LLEN+1];
  WCHAR *mytextW = NULL, *aptr, *wptr;
  BOOL diff_found = FALSE;
  unsigned int i;
  
  tempf=_tempnam(".","wne");
  tempfh = fopen(tempf,"wt"); /* open in TEXT mode */
  fputs(mytext,tempfh);
  fclose(tempfh);
  tempfh = fopen(tempf,"rt");
  fgetws(wtextW,LLEN,tempfh);
  mytextW = AtoW ((char*)mytext);
  aptr = mytextW;
  wptr = wtextW;

  for (i=0; i<strlen(mytext); i++, aptr++, wptr++)
    {
      diff_found |= (*aptr != *wptr);
    }
  ok(!(diff_found), "fgetwc difference found in TEXT mode");
  if(mytextW) free (mytextW);
  fclose(tempfh);
  unlink(tempf);
}
  
static void test_file_put_get( void )
{
  char* tempf;
  FILE *tempfh;
  const char mytext[]=  "This is a test_file_put_get\n";
  const char dostext[]= "This is a test_file_put_get\r\n";
  char btext[LLEN];
  WCHAR wtextW[LLEN+1];
  WCHAR *mytextW = NULL, *aptr, *wptr;
  BOOL diff_found = FALSE;
  unsigned int i;

  tempf=_tempnam(".","wne");
  tempfh = fopen(tempf,"wt"); /* open in TEXT mode */
  fputs(mytext,tempfh);
  fclose(tempfh);
  tempfh = fopen(tempf,"rb"); /* open in TEXT mode */
  fgets(btext,LLEN,tempfh);
  ok( strlen(mytext) + 1 == strlen(btext),"TEXT/BINARY mode not handled for write");
  ok( btext[strlen(mytext)-1] == '\r', "CR not written");
  fclose(tempfh);
  tempfh = fopen(tempf,"wb"); /* open in BINARY mode */
  fputs(dostext,tempfh);
  fclose(tempfh);
  tempfh = fopen(tempf,"rt"); /* open in TEXT mode */
  fgets(btext,LLEN,tempfh);
  ok(strcmp(btext, mytext) == 0,"_O_TEXT read doesn't strip CR");
  fclose(tempfh);
  tempfh = fopen(tempf,"rb"); /* open in TEXT mode */
  fgets(btext,LLEN,tempfh);
  ok(strcmp(btext, dostext) == 0,"_O_BINARY read doesn't preserve CR");

  fclose(tempfh);
  tempfh = fopen(tempf,"rt"); /* open in TEXT mode */
  fgetws(wtextW,LLEN,tempfh);
  mytextW = AtoW ((char*)mytext);
  aptr = mytextW;
  wptr = wtextW;

  for (i=0; i<strlen(mytext); i++, aptr++, wptr++)
    {
      diff_found |= (*aptr != *wptr);
    }
  ok(!(diff_found), "fgetwc doesn't strip CR in TEXT mode");
  if(mytextW) free (mytextW);
  fclose(tempfh);
  unlink(tempf);
}
static void test_file_write_read( void )
{
  char* tempf;
  int tempfd;
  const char mytext[]=  "This is test_file_write_read\nsecond line\n";
  const char dostext[]= "This is test_file_write_read\r\nsecond line\r\n";
  char btext[LLEN];

  tempf=_tempnam(".","wne");
  ok((tempfd = _open(tempf,_O_CREAT|_O_TRUNC|_O_TEXT|_O_RDWR,_S_IREAD | _S_IWRITE)) != -1,"Can't open"); /* open in TEXT mode */
  ok(_write(tempfd,mytext,strlen(mytext)) == lstrlenA(mytext), "_write _O_TEXT bad return value");
  _close(tempfd);
  tempfd = _open(tempf,_O_RDONLY|_O_BINARY,0); /* open in BINARY mode */
  ok(_read(tempfd,btext,LLEN) == lstrlenA(dostext), "_read _O_BINARY got bad length");
  ok( memcmp(dostext,btext,strlen(dostext)) == 0,"problems with _O_TEXT _write and _O_BINARY _write");
  ok( btext[strlen(dostext)-2] == '\r', "CR not written");
  _close(tempfd);
  tempfd = _open(tempf,_O_RDONLY|_O_TEXT); /* open in TEXT mode */
  ok(_read(tempfd,btext,LLEN) == lstrlenA(mytext), "_read _O_TEXT got bad length");
  ok( memcmp(mytext,btext,strlen(mytext)) == 0,"problems with _O_TEXT _write / _write");
  _close(tempfd);
  ok(unlink(tempf) !=-1 ,"Can't unlink");

  tempf=_tempnam(".","wne");
  ok((tempfd = _open(tempf,_O_CREAT|_O_TRUNC|_O_BINARY|_O_RDWR,0)) != -1,"Can't open %s",tempf); /* open in BINARY mode */
  ok(_write(tempfd,dostext,strlen(dostext)) == lstrlenA(dostext), "_write _O_TEXT bad return value");
  _close(tempfd);
  tempfd = _open(tempf,_O_RDONLY|_O_BINARY,0); /* open in BINARY mode */
  ok(_read(tempfd,btext,LLEN) == lstrlenA(dostext), "_read _O_BINARY got bad length");
  ok( memcmp(dostext,btext,strlen(dostext)) == 0,"problems with _O_TEXT _write and _O_BINARY _write");
  ok( btext[strlen(dostext)-2] == '\r', "CR not written");
  _close(tempfd);
  tempfd = _open(tempf,_O_RDONLY|_O_TEXT); /* open in TEXT mode */
  ok(_read(tempfd,btext,LLEN) == lstrlenA(mytext), "_read _O_TEXT got bad length");
  ok( memcmp(mytext,btext,strlen(mytext)) == 0,"problems with _O_TEXT _write / _write");
  _close(tempfd);

  unlink(tempf);
}


START_TEST(file)
{
    test_fdopen();
    test_fgetwc();
    test_file_put_get();
    test_file_write_read();
}
