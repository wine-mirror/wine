/*
 * Unit tests for file functions in Wine
 *
 * Copyright (c) 2002 Jakob Eriksson
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
 *
 */

#include "winbase.h"
#include "winerror.h"
#include "wine/test.h"


#include <stdlib.h>
#include <time.h>


LPCSTR filename = "testfile.xxx";
LPCSTR sillytext =
"en larvig liten text dx \033 gx hej 84 hej 4484 ! \001\033 bla bl\na.. bla bla."
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"sdlkfjasdlkfj a dslkj adsklf  \n  \nasdklf askldfa sdlkf \nsadklf asdklf asdf ";


static void test__hread( void )
{
    HFILE filehandle;
    char buffer[10000];
    long bytes_read;
    long bytes_wanted;
    UINT i;

    filehandle = _lcreat( filename, 0 );
    ok( HFILE_ERROR != filehandle, "couldn't create file. Wrong permissions on directory?" );

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains." );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains." );

    filehandle = _lopen( filename, OF_READ );

    ok( HFILE_ERROR != filehandle, "couldn't open file again?");
    
    bytes_read = _hread( filehandle, buffer, 2 * strlen( sillytext ) );
    
    ok( strlen( sillytext ) == bytes_read, "file read size error." );

    for (bytes_wanted = 0; bytes_wanted < strlen( sillytext ); bytes_wanted++)
    {
        ok( 0 == _llseek( filehandle, 0, FILE_BEGIN ), "_llseek complains." );
        ok( _hread( filehandle, buffer, bytes_wanted ) == bytes_wanted, "erratic _hread return value." );
        for (i = 0; i < bytes_wanted; i++)
        {
            ok( buffer[i] == sillytext[i], "that's not what's written." );
        }
    }

    ok( HFILE_ERROR != _lclose( filehandle ), "_lclose complains." );

    ok( DeleteFileA( filename ) != 0, "DeleteFile complains." );
}


static void test__hwrite( void )
{
    HFILE filehandle;
    char buffer[10000];
    long bytes_read;
    UINT bytes_written;
    UINT blocks;
    UINT i;
    char *contents;
    HLOCAL memory_object;
    char checksum[1];

    filehandle = _lcreat( filename, 0 );
    ok( HFILE_ERROR != filehandle, "couldn't create file. Wrong permissions on directory?" );

    ok( HFILE_ERROR != _hwrite( filehandle, "", 0 ), "_hwrite complains." );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains." );

    filehandle = _lopen( filename, OF_READ );
    
    bytes_read = _hread( filehandle, buffer, 1);

    ok( 0 == bytes_read, "file read size error." );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains." );

    filehandle = _lopen( filename, OF_READWRITE );

    bytes_written = 0;
    checksum[0] = '\0';
    srand( (unsigned)time( NULL ) );
    for (blocks = 0; blocks < 100; blocks++)
    {
        for (i = 0; i < sizeof( buffer ); i++)
        {
            buffer[i] = rand(  );
            checksum[0] = checksum[0] + buffer[i];
        }
        ok( HFILE_ERROR != _hwrite( filehandle, buffer, sizeof( buffer ) ), "_hwrite complains." );
        bytes_written = bytes_written + sizeof( buffer );
    }

    ok( HFILE_ERROR != _hwrite( filehandle, checksum, 1 ), "_hwrite complains." );
    bytes_written++;

    ok( HFILE_ERROR != _lclose( filehandle ), "_lclose complains." );

    memory_object = LocalAlloc( LPTR, bytes_written );

    ok( 0 != memory_object, "LocalAlloc fails. (Could be out of memory.)" );

    contents = LocalLock( memory_object );

    filehandle = _lopen( filename, OF_READ );

    contents = LocalLock( memory_object );

    ok( NULL != contents, "LocalLock whines." );

    ok( bytes_written == _hread( filehandle, contents, bytes_written), "read length differ from write length." );

    checksum[0] = '\0';
    i = 0;
    do
    {
        checksum[0] = checksum[0] + contents[i];
        i++;
    }
    while (i < bytes_written - 1);

    ok( checksum[0] == contents[i], "stored checksum differ from computed checksum." );

    ok( HFILE_ERROR != _lclose( filehandle ), "_lclose complains." );

    ok( DeleteFileA( filename ) != 0, "DeleteFile complains." );
}


static void test__lclose( void )
{
    HFILE filehandle;

    filehandle = _lcreat( filename, 0 );
    ok( HFILE_ERROR != filehandle, "couldn't create file. Wrong permissions on directory?" );

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains." );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains." );

    ok( HFILE_ERROR == _lclose(filehandle), "_lclose should whine about this." );

    ok( HFILE_ERROR == _lclose(filehandle), "_lclose should whine about this." );

    ok( DeleteFileA( filename ) != 0, "DeleteFileA complains." );
}


static void test__lcreat( void )
{
    HFILE filehandle;
    char buffer[10000];
    WIN32_FIND_DATAA search_results;

    filehandle = _lcreat( filename, 0 );
    ok( HFILE_ERROR != filehandle, "couldn't create file. Wrong permissions on directory?" );

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains." );

    ok( 0 == _llseek( filehandle, 0, FILE_BEGIN ), "_llseek complains." );

    ok( _hread( filehandle, buffer, strlen( sillytext ) ) ==  strlen( sillytext ), "erratic _hread return value." );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains." );

    ok( INVALID_HANDLE_VALUE != FindFirstFileA( filename, &search_results ), "should be able to find file" );

    ok( DeleteFileA( filename ) != 0, "DeleteFileA complains." );

    filehandle = _lcreat( filename, 1 );
    ok( HFILE_ERROR != filehandle, "couldn't create file!?" );

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite shouldn't be able to write never the less." );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains." );

    ok( INVALID_HANDLE_VALUE != FindFirstFileA( filename, &search_results ), "should be able to find file" );

    ok( DeleteFileA( filename ) != 0, "DeleteFileA complains." );


    filehandle = _lcreat( filename, 2 );
    ok( HFILE_ERROR != filehandle, "couldn't create file. Wrong permissions on directory?" );

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains." );

    ok( 0 == _llseek( filehandle, 0, FILE_BEGIN ), "_llseek complains." );

    ok( _hread( filehandle, buffer, strlen( sillytext ) ) ==  strlen( sillytext ), "erratic _hread return value." );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains." );

    todo_wine
    {
        ok( INVALID_HANDLE_VALUE == FindFirstFileA( filename, &search_results ), "should NOT be able to find file" );
    }

    ok( DeleteFileA( filename ) != 0, "DeleteFileA complains." );
    
    filehandle = _lcreat( filename, 4 );
    ok( HFILE_ERROR != filehandle, "couldn't create file. Wrong permissions on directory?" );

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains." );

    ok( 0 == _llseek( filehandle, 0, FILE_BEGIN ), "_llseek complains." );

    ok( _hread( filehandle, buffer, strlen( sillytext ) ) ==  strlen( sillytext ), "erratic _hread return value." );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains." );

    todo_wine
    {
        ok( INVALID_HANDLE_VALUE == FindFirstFileA( filename, &search_results ), "should NOT be able to find file" );
    }

    ok( DeleteFileA( filename ) != 0, "DeleteFileA complains." );
}


void test__llseek( void )
{
    INT i;
    HFILE filehandle;
    char buffer[1];
    long bytes_read;

    filehandle = _lcreat( filename, 0 );

    ok( HFILE_ERROR != filehandle, "couldn't create file. Wrong permissions on directory?" );
    for (i = 0; i < 400; i++)
    {
        ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains." );
    }
    ok( HFILE_ERROR != _llseek( filehandle, 400 * strlen( sillytext ), FILE_CURRENT ), "should be able to seek" );
    ok( HFILE_ERROR != _llseek( filehandle, 27 + 35 * strlen( sillytext ), FILE_BEGIN ), "should be able to seek" );

    bytes_read = _hread( filehandle, buffer, 1);
    ok( 1 == bytes_read, "file read size error." );
    ok( buffer[0] == sillytext[27], "_llseek error. It got lost seeking..." );
    ok( HFILE_ERROR != _llseek( filehandle, -400 * strlen( sillytext ), FILE_END ), "should be able to seek" );

    bytes_read = _hread( filehandle, buffer, 1);
    ok( 1 == bytes_read, "file read size error." );
    ok( buffer[0] == sillytext[0], "_llseek error. It got lost seeking..." );
    ok( HFILE_ERROR != _llseek( filehandle, 1000000, FILE_END ), "should be able to seek past file. Poor, poor Windows programmers." );
    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains." );

    ok( DeleteFileA( filename ) != 0, "DeleteFileA complains." );
}


static void test__llopen( void )
{
    HFILE filehandle;
    UINT bytes_read;
    char buffer[10000];

    filehandle = _lcreat( filename, 0 );
    ok( HFILE_ERROR != filehandle, "couldn't create file. Wrong permissions on directory?" );
    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains." );
    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains." );

    filehandle = _lopen( filename, OF_READ );
    ok( HFILE_ERROR == _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite shouldn't be able to write!" );
    bytes_read = _hread( filehandle, buffer, strlen( sillytext ) );
    ok( strlen( sillytext )  == bytes_read, "file read size error." );
    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains." );
    
    filehandle = _lopen( filename, OF_READWRITE );
    bytes_read = _hread( filehandle, buffer, 2 * strlen( sillytext ) );
    ok( strlen( sillytext )  == bytes_read, "file read size error." );
    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite should write just fine." );
    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains." );

    filehandle = _lopen( filename, OF_WRITE );
    ok( HFILE_ERROR == _hread( filehandle, buffer, 1 ), "you should only be able to write this file..." );
    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite should write just fine." );
    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains." );

    ok( DeleteFileA( filename ) != 0, "DeleteFileA complains." );
    /* TODO - add tests for the SHARE modes  -  use two processes to pull this one off */
}


static void test__lread( void )
{
    HFILE filehandle;
    char buffer[10000];
    long bytes_read;
    UINT bytes_wanted;
    UINT i;

    filehandle = _lcreat( filename, 0 );
    ok( HFILE_ERROR != filehandle, "couldn't create file. Wrong permissions on directory?" );

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains." );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains." );

    filehandle = _lopen( filename, OF_READ );

    ok( HFILE_ERROR != filehandle, "couldn't open file again?");
    
    bytes_read = _lread( filehandle, buffer, 2 * strlen( sillytext ) );
    
    ok( strlen( sillytext ) == bytes_read, "file read size error." );

    for (bytes_wanted = 0; bytes_wanted < strlen( sillytext ); bytes_wanted++)
    {
        ok( 0 == _llseek( filehandle, 0, FILE_BEGIN ), "_llseek complains." );
        ok( _lread( filehandle, buffer, bytes_wanted ) == bytes_wanted, "erratic _hread return value." );
        for (i = 0; i < bytes_wanted; i++)
        {
            ok( buffer[i] == sillytext[i], "that's not what's written." );
        }
    }

    ok( HFILE_ERROR != _lclose( filehandle ), "_lclose complains." );

    ok( DeleteFileA( filename ) != 0, "DeleteFile complains." );
}


static void test__lwrite( void )
{
    HFILE filehandle;
    char buffer[10000];
    long bytes_read;
    UINT bytes_written;
    UINT blocks;
    UINT i;
    char *contents;
    HLOCAL memory_object;
    char checksum[1];

    filehandle = _lcreat( filename, 0 );
    ok( HFILE_ERROR != filehandle, "couldn't create file. Wrong permissions on directory?" );

    ok( HFILE_ERROR != _lwrite( filehandle, "", 0 ), "_hwrite complains." );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains." );

    filehandle = _lopen( filename, OF_READ );
    
    bytes_read = _hread( filehandle, buffer, 1);

    ok( 0 == bytes_read, "file read size error." );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains." );

    filehandle = _lopen( filename, OF_READWRITE );

    bytes_written = 0;
    checksum[0] = '\0';
    srand( (unsigned)time( NULL ) );
    for (blocks = 0; blocks < 100; blocks++)
    {
        for (i = 0; i < sizeof( buffer ); i++)
        {
            buffer[i] = rand(  );
            checksum[0] = checksum[0] + buffer[i];
        }
        ok( HFILE_ERROR != _lwrite( filehandle, buffer, sizeof( buffer ) ), "_hwrite complains." );
        bytes_written = bytes_written + sizeof( buffer );
    }

    ok( HFILE_ERROR != _lwrite( filehandle, checksum, 1 ), "_hwrite complains." );
    bytes_written++;

    ok( HFILE_ERROR != _lclose( filehandle ), "_lclose complains." );

    memory_object = LocalAlloc( LPTR, bytes_written );

    ok( 0 != memory_object, "LocalAlloc fails. (Could be out of memory.)" );

    contents = LocalLock( memory_object );

    filehandle = _lopen( filename, OF_READ );

    contents = LocalLock( memory_object );

    ok( NULL != contents, "LocalLock whines." );

    ok( bytes_written == _hread( filehandle, contents, bytes_written), "read length differ from write length." );

    checksum[0] = '\0';
    i = 0;
    do
    {
        checksum[0] = checksum[0] + contents[i];
        i++;
    }
    while (i < bytes_written - 1);

    ok( checksum[0] == contents[i], "stored checksum differ from computed checksum." );
    return;
    
    ok( HFILE_ERROR != _lclose( filehandle ), "_lclose complains." );

    ok( DeleteFileA( filename ) != 0, "DeleteFile complains." );
}


START_TEST(file)
{
    test__hread(  );
    test__hwrite(  );
    test__lclose(  );
    test__lcreat(  );
    test__llseek(  );
    test__llopen(  );
    test__lread(  );
    test__lwrite(  );
}
