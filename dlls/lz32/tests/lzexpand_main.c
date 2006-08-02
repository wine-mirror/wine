/*
 * Unit test suite for lz32 functions
 *
 * Copyright 2004 Evan Parry, Daniel Kegel
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <stdlib.h>
#include <winerror.h>
#include <lzexpand.h>

#include "wine/test.h"

static char filename_[] = "testfile.xx_";
static char filename[] = "testfile.xxx";
static char filename2[] = "testfile.yyy";

static WCHAR filename_W[] = {'t','e','s','t','f','i','l','e','.','x','x','_',0};
static WCHAR filenameW[]  = {'t','e','s','t','f','i','l','e','.','x','x','x',0};

/* This is the hex string representation of the file created by compressing
   a simple text file with the contents "This is a test file."
 
   The file was created using COMPRESS.EXE from the Windows Server 2003
   Resource Kit from Microsoft.  The resource kit was retrieved from the
   following URL:  

   http://www.microsoft.com/downloads/details.aspx?FamilyID=9d467a69-57ff-4ae7-96ee-b18c4790cffd&displaylang=en
 */
static const unsigned char compressed_file[] = 
  {0x53,0x5A,0x44,0x44,0x88,0xF0,0x27,0x33,0x41,
   0x74,0x75,0x14,0x00,0x00,0xDF,0x54,0x68,0x69,
   0x73,0x20,0xF2,0xF0,0x61,0x20,0xFF,0x74,0x65,
   0x73,0x74,0x20,0x66,0x69,0x6C,0x03,0x65,0x2E};
static const DWORD compressed_file_size = sizeof(compressed_file);

static const char uncompressed_data[] = "This is a test file.";
static const DWORD uncompressed_data_size = sizeof(uncompressed_data) - 1;

static char *buf;

static void test_LZOpenFileA(void)
{
  OFSTRUCT test;
  DWORD retval;
  INT file;
  char expected[MAX_PATH];
  char filled_0xA5[OFS_MAXPATHNAME];
  static char badfilename_[] = "badfilename_";

  SetLastError(0xfaceabee);
  /* Check for nonexistent file. */
  file = LZOpenFileA(badfilename_, &test, OF_READ);
  ok(file == LZERROR_BADINHANDLE, 
     "LZOpenFileA succeeded on nonexistent file\n");
  ok(GetLastError() == ERROR_FILE_NOT_FOUND, 
     "GetLastError() returns %ld\n", GetLastError());
  LZClose(file);

  /* Create an empty file. */
  file = LZOpenFileA(filename_, &test, OF_CREATE);
  ok(file >= 0, "LZOpenFileA failed on creation\n");
  LZClose(file);
  retval = GetFileAttributesA(filename_);
  ok(retval != INVALID_FILE_ATTRIBUTES, "GetFileAttributesA: error %ld\n", 
     GetLastError());

  /* Check various opening options. */
  file = LZOpenFileA(filename_, &test, OF_READ);
  ok(file >= 0, "LZOpenFileA failed on read\n");
  LZClose(file);
  file = LZOpenFileA(filename_, &test, OF_WRITE);
  ok(file >= 0, "LZOpenFileA failed on write\n");
  LZClose(file);
  file = LZOpenFileA(filename_, &test, OF_READWRITE);
  ok(file >= 0, "LZOpenFileA failed on read/write\n");
  LZClose(file);
  file = LZOpenFileA(filename_, &test, OF_EXIST);
  ok(file >= 0, "LZOpenFileA failed on read/write\n");
  LZClose(file);

  /* If the file "foo.xxx" does not exist, LZOpenFileA should then
     check for the file "foo.xx_" and open that -- at least on some
     operating systems.  Doesn't seem to on my copy of Win98.   
   */
  retval = GetCurrentDirectoryA(MAX_PATH, expected);
  ok(retval > 0, "GetCurrentDirectoryA returned %ld, GLE=0x%lx\n", 
     retval, GetLastError());
  lstrcatA(expected, "\\");
  lstrcatA(expected, filename);
  /* Compressed file name ends with underscore. */
  retval = lstrlenA(expected);
  expected[retval-1] = '_';
  memset(&filled_0xA5, 0xA5, OFS_MAXPATHNAME);
  memset(&test, 0xA5, sizeof(test));

  /* Try to open compressed file. */
  file = LZOpenFileA(filename, &test, OF_EXIST);
  if(file != LZERROR_BADINHANDLE) {
    ok(test.cBytes == sizeof(OFSTRUCT), 
       "LZOpenFileA set test.cBytes to %d\n", test.cBytes);
    ok(test.nErrCode == 0, "LZOpenFileA set test.nErrCode to %d\n", 
       test.nErrCode);
    ok(lstrcmpA(test.szPathName, expected) == 0, 
       "LZOpenFileA returned '%s', but was expected to return '%s'\n", 
       test.szPathName, expected);
    LZClose(file);
  } else {
    ok(test.cBytes == 0xA5, 
       "LZOpenFileA set test.cBytes to %d\n", test.cBytes);
    ok(test.nErrCode == ERROR_FILE_NOT_FOUND, 
       "LZOpenFileA set test.nErrCode to %d\n", test.nErrCode);
    ok(strncmp(test.szPathName, filled_0xA5, OFS_MAXPATHNAME) == 0, 
       "LZOpenFileA returned '%s', but was expected to return '%s'\n", 
       test.szPathName, filled_0xA5);
  }

  /* Delete the file then make sure it doesn't exist anymore. */
  file = LZOpenFileA(filename_, &test, OF_DELETE);
  ok(file >= 0, "LZOpenFileA failed on delete\n");
  LZClose(file);

  retval = GetFileAttributesA(filename_);
  ok(retval == INVALID_FILE_ATTRIBUTES, 
     "GetFileAttributesA succeeded on deleted file\n");
}

static void test_LZRead(void)
{
  HANDLE file;
  DWORD ret;
  int cfile;
  OFSTRUCT test;
  BOOL retok;

  /* Create the compressed file. */
  file = CreateFileA(filename_, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, 0);
  ok(file != INVALID_HANDLE_VALUE, "Could not create test file\n");
  retok = WriteFile(file, compressed_file, compressed_file_size, &ret, 0);
  ok( retok, "WriteFile: error %ld\n", GetLastError());
  ok(ret == compressed_file_size, "Wrote wrong number of bytes with WriteFile?\n");
  CloseHandle(file);

  cfile = LZOpenFileA(filename_, &test, OF_READ);
  ok(cfile > 0, "LZOpenFileA failed\n");

  ret = LZRead(cfile, buf, uncompressed_data_size);
  ok(ret == uncompressed_data_size, "Read wrong number of bytes\n");

  /* Compare what we read with what we think we should read. */
  ok(memcmp(buf, uncompressed_data, uncompressed_data_size) == 0,
     "buffer contents mismatch\n");

  todo_wine {
     /* Wine returns the number of bytes actually read instead of an error */
     ret = LZRead(cfile, buf, uncompressed_data_size);
     ok(ret == LZERROR_READ, "Expected read-past-EOF to return LZERROR_READ\n");
  }

  LZClose(cfile);

  ret = DeleteFileA(filename_);
  ok(ret, "DeleteFileA: error %ld\n", GetLastError());
}

static void test_LZCopy(void)
{
  HANDLE file;
  DWORD ret;
  int source, dest;
  OFSTRUCT stest, dtest;
  BOOL retok;

  /* Create the compressed file. */
  file = CreateFileA(filename_, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, 0);
  ok(file != INVALID_HANDLE_VALUE, 
     "CreateFileA: error %ld\n", GetLastError());
  retok = WriteFile(file, compressed_file, compressed_file_size, &ret, 0);
  ok( retok, "WriteFile error %ld\n", GetLastError());
  ok(ret == compressed_file_size, "Wrote wrong number of bytes\n");
  CloseHandle(file);

  source = LZOpenFileA(filename_, &stest, OF_READ);
  ok(source >= 0, "LZOpenFileA failed on compressed file\n");
  dest = LZOpenFileA(filename2, &dtest, OF_CREATE);
  ok(dest >= 0, "LZOpenFileA failed on creating new file %d\n", dest);

  ret = LZCopy(source, dest);
  ok(ret > 0, "LZCopy error\n");

  LZClose(source);
  LZClose(dest);

  file = CreateFileA(filename2, GENERIC_READ, 0, NULL, OPEN_EXISTING,
		    0, 0);
  ok(file != INVALID_HANDLE_VALUE,
     "CreateFileA: error %ld\n", GetLastError());

  retok = ReadFile(file, buf, uncompressed_data_size*2, &ret, 0);
  ok( retok && ret == uncompressed_data_size, "ReadFile: error %ld\n", GetLastError());
  /* Compare what we read with what we think we should read. */
  ok(!memcmp(buf, uncompressed_data, uncompressed_data_size),
     "buffer contents mismatch\n");
  CloseHandle(file);

  ret = DeleteFileA(filename_);
  ok(ret, "DeleteFileA: error %ld\n", GetLastError());
  ret = DeleteFileA(filename2);
  ok(ret, "DeleteFileA: error %ld\n", GetLastError());
}

static void test_LZOpenFileW(void)
{
  OFSTRUCT test;
  DWORD retval;
  INT file;
  char expected[MAX_PATH];
  static WCHAR badfilenameW[] = {'b','a','d','f','i','l','e','n','a','m','e','.','x','t','n',0};

  SetLastError(0xfaceabee);
  /* Check for nonexistent file. */
  file = LZOpenFileW(badfilenameW, &test, OF_READ);
  if(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
  {
    trace("LZOpenFileW call not implemented, skipping rest of the test\n");
    return;
  }
  ok(file == LZERROR_BADINHANDLE, 
     "LZOpenFileW succeeded on nonexistent file\n");
  LZClose(file);

  /* Create an empty file. */
  file = LZOpenFileW(filename_W, &test, OF_CREATE);
  ok(file >= 0, "LZOpenFile failed on creation\n");
  LZClose(file);
  retval = GetFileAttributesW(filename_W);
  ok(retval != INVALID_FILE_ATTRIBUTES, "GetFileAttributes: error %ld\n", 
    GetLastError());

  /* Check various opening options. */
  file = LZOpenFileW(filename_W, &test, OF_READ);
  ok(file >= 0, "LZOpenFileW failed on read\n");
  LZClose(file);
  file = LZOpenFileW(filename_W, &test, OF_WRITE);
  ok(file >= 0, "LZOpenFileW failed on write\n");
  LZClose(file);
  file = LZOpenFileW(filename_W, &test, OF_READWRITE);
  ok(file >= 0, "LZOpenFileW failed on read/write\n");
  LZClose(file);
  file = LZOpenFileW(filename_W, &test, OF_EXIST);
  ok(file >= 0, "LZOpenFileW failed on read/write\n");
  LZClose(file);

  /* If the file "foo.xxx" does not exist, LZOpenFileW should then
     check for the file "foo.xx_" and open that -- at least on some
     operating systems.  Doesn't seem to on my copy of Win98.   
   */
  retval = GetCurrentDirectoryA(MAX_PATH, expected);
  ok(retval > 0, "GetCurrentDirectoryW returned %ld, GLE=0x%lx\n", 
     retval, GetLastError());
  lstrcatA(expected, "\\");
  /* It's probably better to use WideCharToMultiByte() on filenameW: */
  lstrcatA(expected, filename);
  /* Compressed file name ends with underscore. */
  retval = lstrlenA(expected);
  expected[retval-1] = '_';
  memset(&test, 0xA5, sizeof(test));

  /* Try to open compressed file. */
  file = LZOpenFileW(filenameW, &test, OF_EXIST);
  ok(file >= 0, "LZOpenFileW failed on switching to a compressed file name\n");
  ok(test.cBytes == sizeof(OFSTRUCT), 
     "LZOpenFileW set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_SUCCESS, "LZOpenFileW set test.nErrCode to %d\n", 
     test.nErrCode);
  /* Note that W-function returns A-string by a OFSTRUCT.szPathName: */
  ok(lstrcmpA(test.szPathName, expected) == 0, 
     "LZOpenFileW returned '%s', but was expected to return '%s'\n", 
     test.szPathName, expected);
  LZClose(file);

  /* Delete the file then make sure it doesn't exist anymore. */
  file = LZOpenFileW(filename_W, &test, OF_DELETE);
  ok(file >= 0, "LZOpenFileW failed on delete\n");
  LZClose(file);

  retval = GetFileAttributesW(filename_W);
  ok(retval == INVALID_FILE_ATTRIBUTES, 
     "GetFileAttributesW succeeded on deleted file\n");
}


START_TEST(lzexpand_main)
{
  buf = malloc(uncompressed_data_size * 2);
  test_LZOpenFileA();
  test_LZOpenFileW();
  test_LZRead();
  test_LZCopy();
  free(buf);
}
