/*
 * Unit test of the SHFileOperation function.
 *
 * Copyright 2002 Andriy Palamarchuk
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

#include <stdarg.h>
#include <stdio.h>
 
#define WINE_NOWINSOCK
#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "shellapi.h"

#include "wine/test.h"

CHAR CURR_DIR[MAX_PATH];

/* creates a file with the specified name for tests */
void createTestFile(CHAR *name)
{
    HANDLE file;
    DWORD written;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failure to open file %s\n", name);
    WriteFile(file, name, strlen(name), &written, NULL);
    WriteFile(file, "\n", strlen("\n"), &written, NULL);
    CloseHandle(file);
}

BOOL file_exists(CHAR *name)
{
    return GetFileAttributesA(name) != INVALID_FILE_ATTRIBUTES;
}

/* initializes the tests */
void init_shfo_tests(void)
{
    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);
    createTestFile(".\\test1.txt");
    createTestFile(".\\test2.txt");
    createTestFile(".\\test3.txt");
    CreateDirectoryA(".\\test4.txt", NULL);
    CreateDirectoryA(".\\testdir2", NULL);
}

/* cleans after tests */
void clean_after_shfo_tests(void)
{
    DeleteFileA(".\\test1.txt");
    DeleteFileA(".\\test2.txt");
    DeleteFileA(".\\test3.txt");
    DeleteFileA(".\\test4.txt\\test1.txt");
    DeleteFileA(".\\test4.txt\\test2.txt");
    DeleteFileA(".\\test4.txt\\test3.txt");
    RemoveDirectoryA(".\\test4.txt");
    DeleteFileA(".\\testdir2\\test1.txt");
    DeleteFileA(".\\testdir2\\test2.txt");
    DeleteFileA(".\\testdir2\\test3.txt");
    DeleteFileA(".\\testdir2\\test4.txt\\test1.txt");
    RemoveDirectoryA(".\\testdir2\\test4.txt");
    RemoveDirectoryA(".\\testdir2");
}

/*
 puts into the specified buffer file names with current directory.
 files - string with file names, separated by null characters. Ends on a double
 null characters
*/
void set_curr_dir_path(CHAR *buf, CHAR* files)
{
    buf[0] = 0;
    while (files[0])
    {
        strcpy(buf, CURR_DIR);
        buf += strlen(buf);
        buf[0] = '\\';
        buf++;
        strcpy(buf, files);
        buf += strlen(buf) + 1;
        files += strlen(files) + 1;
    }
    buf[0] = 0;
}


/* tests the FO_DELETE action */
void test_delete(void)
{
    SHFILEOPSTRUCTA shfo;
    CHAR buf[MAX_PATH];

    sprintf(buf, "%s\\%s", CURR_DIR, "test?.txt");
    buf[strlen(buf) + 1] = '\0';

    shfo.hwnd = NULL;
    shfo.wFunc = FO_DELETE;
    shfo.pFrom = buf;
    shfo.pTo = "\0";
    shfo.fFlags = FOF_FILESONLY | FOF_NOCONFIRMATION | FOF_SILENT;
    shfo.hNameMappings = NULL;
    shfo.lpszProgressTitle = NULL;

    ok(!SHFileOperationA(&shfo), "Deletion was successful\n");
    ok(file_exists(".\\test4.txt"), "Directory should not be removed\n");
    ok(!file_exists(".\\test1.txt"), "File should be removed\n");

    ok(!SHFileOperationA(&shfo), "Directory exists, but is not removed\n");
    ok(file_exists(".\\test4.txt"), "Directory should not be removed\n");

    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;

    ok(!SHFileOperationA(&shfo), "Directory removed\n");
    ok(!file_exists(".\\test4.txt"), "Directory should be removed\n");

    ok(!SHFileOperationA(&shfo), "The requested file does not exist\n");

    init_shfo_tests();
    sprintf(buf, "%s\\%s", CURR_DIR, "test4.txt");
    buf[strlen(buf) + 1] = '\0';
    ok(MoveFileA(".\\test1.txt", ".\\test4.txt\\test1.txt"), "Fill the subdirectory\n");
    ok(!SHFileOperationA(&shfo), "Directory removed\n");
    ok(!file_exists(".\\test4.txt"), "Directory is removed\n");

    init_shfo_tests();
    shfo.pFrom = ".\\test1.txt\0.\\test4.txt\0";
    ok(!SHFileOperationA(&shfo), "Directory and a file removed\n");
    ok(!file_exists(".\\test1.txt"), "The file should be removed\n");
    ok(!file_exists(".\\test4.txt"), "Directory should be removed\n");
    ok(file_exists(".\\test2.txt"), "This file should not be removed\n");
}

/* tests the FO_RENAME action */
void test_rename()
{
    SHFILEOPSTRUCTA shfo, shfo2;
    CHAR from[MAX_PATH];
    CHAR to[MAX_PATH];
    DWORD retval;

    shfo.hwnd = NULL;
    shfo.wFunc = FO_RENAME;
    shfo.pFrom = from;
    shfo.pTo = to;
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    shfo.hNameMappings = NULL;
    shfo.lpszProgressTitle = NULL;

    set_curr_dir_path(from, "test1.txt\0");
    set_curr_dir_path(to, "test4.txt\0");
    ok(SHFileOperationA(&shfo), "File is not renamed moving to other directory "
       "when specifying directory name only\n");
    ok(file_exists(".\\test1.txt"), "The file is not removed\n");

    set_curr_dir_path(from, "test3.txt\0");
    set_curr_dir_path(to, "test4.txt\\test1.txt\0");
    ok(!SHFileOperationA(&shfo), "File is renamed moving to other directory\n");
    ok(file_exists(".\\test4.txt\\test1.txt"), "The file is renamed\n");

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    retval = SHFileOperationA(&shfo); /* W98 returns 0, W2K and newer returns ERROR_GEN_FAILURE, both do nothing */
    ok(!retval || retval == ERROR_GEN_FAILURE, "Can't rename many files, retval = %lx\n", retval);
    ok(file_exists(".\\test1.txt"), "The file is not renamed - many files are specified\n");

    memcpy(&shfo2, &shfo, sizeof(SHFILEOPSTRUCTA));
    shfo2.fFlags |= FOF_MULTIDESTFILES;

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    retval = SHFileOperationA(&shfo2); /* W98 returns 0, W2K and newer returns ERROR_GEN_FAILURE, both do nothing */
    ok(!retval || retval == ERROR_GEN_FAILURE, "Can't rename many files, retval = %lx\n", retval);
    ok(file_exists(".\\test1.txt"), "The file is not renamed - many files are specified\n");

    set_curr_dir_path(from, "test1.txt\0");
    set_curr_dir_path(to, "test6.txt\0");
    ok(!SHFileOperationA(&shfo), "Rename file\n");
    ok(!file_exists(".\\test1.txt"), "The file is renamed\n");
    ok(file_exists(".\\test6.txt"), "The file is renamed\n");
    set_curr_dir_path(from, "test6.txt\0");
    set_curr_dir_path(to, "test1.txt\0");
    ok(!SHFileOperationA(&shfo), "Rename file back\n");

    set_curr_dir_path(from, "test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0");
    ok(!SHFileOperationA(&shfo), "Rename dir\n");
    ok(!file_exists(".\\test4.txt"), "The dir is renamed\n");
    ok(file_exists(".\\test6.txt"), "The dir is renamed\n");
    set_curr_dir_path(from, "test6.txt\0");
    set_curr_dir_path(to, "test4.txt\0");
    ok(!SHFileOperationA(&shfo), "Rename dir back\n");
}

/* tests the FO_COPY action */
void test_copy(void)
{
    SHFILEOPSTRUCTA shfo, shfo2;
    CHAR from[MAX_PATH];
    CHAR to[MAX_PATH];
    FILEOP_FLAGS tmp_flags;
    DWORD retval;

    shfo.hwnd = NULL;
    shfo.wFunc = FO_COPY;
    shfo.pFrom = from;
    shfo.pTo = to;
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    shfo.hNameMappings = NULL;
    shfo.lpszProgressTitle = NULL;

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    ok(SHFileOperationA(&shfo), "Can't copy many files\n");
    ok(!file_exists(".\\test6.txt"), "The file is not copied - many files are "
       "specified as a target\n");

    memcpy(&shfo2, &shfo, sizeof(SHFILEOPSTRUCTA));
    shfo2.fFlags |= FOF_MULTIDESTFILES;

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    ok(!SHFileOperationA(&shfo2), "Can't copy many files\n");
    ok(file_exists(".\\test6.txt"), "The file is copied - many files are "
       "specified as a target\n");
    DeleteFileA(".\\test6.txt");
    DeleteFileA(".\\test7.txt");
    RemoveDirectoryA(".\\test8.txt");

    /* number of sources do not correspond to number of targets */
    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0");
    ok(SHFileOperationA(&shfo2), "Can't copy many files\n");
    ok(!file_exists(".\\test6.txt"), "The file is not copied - many files are "
       "specified as a target\n");

    set_curr_dir_path(from, "test1.txt\0");
    set_curr_dir_path(to, "test4.txt\0");
    ok(!SHFileOperationA(&shfo), "Prepare test to check how directories are copied recursively\n");
    ok(file_exists(".\\test4.txt\\test1.txt"), "The file is copied\n");

    set_curr_dir_path(from, "test?.txt\0");
    set_curr_dir_path(to, "testdir2\0");
    ok(!file_exists(".\\testdir2\\test1.txt"), "The file is not copied yet\n");
    ok(!file_exists(".\\testdir2\\test4.txt"), "The directory is not copied yet\n");
    ok(!SHFileOperationA(&shfo), "Files and directories are copied to directory\n");
    ok(file_exists(".\\testdir2\\test1.txt"), "The file is copied\n");
    ok(file_exists(".\\testdir2\\test4.txt"), "The directory is copied\n");
    ok(file_exists(".\\testdir2\\test4.txt\\test1.txt"), "The file in subdirectory is copied\n");
    clean_after_shfo_tests();

    init_shfo_tests();
    shfo.fFlags |= FOF_FILESONLY;
    ok(!file_exists(".\\testdir2\\test1.txt"), "The file is not copied yet\n");
    ok(!file_exists(".\\testdir2\\test4.txt"), "The directory is not copied yet\n");
    ok(!SHFileOperationA(&shfo), "Files are copied to other directory\n");
    ok(file_exists(".\\testdir2\\test1.txt"), "The file is copied\n");
    ok(!file_exists(".\\testdir2\\test4.txt"), "The directory is copied\n");
    clean_after_shfo_tests();

    init_shfo_tests();
    set_curr_dir_path(from, "test1.txt\0test2.txt\0");
    ok(!file_exists(".\\testdir2\\test1.txt"), "The file is not copied yet\n");
    ok(!file_exists(".\\testdir2\\test2.txt"), "The file is not copied yet\n");
    ok(!SHFileOperationA(&shfo), "Files are copied to other directory \n");
    ok(file_exists(".\\testdir2\\test1.txt"), "The file is copied\n");
    ok(file_exists(".\\testdir2\\test2.txt"), "The file is copied\n");
    clean_after_shfo_tests();

    /* Copying multiple files with one not existing as source, fails the
       entire operation in Win98/ME/2K/XP, but not in 95/NT */
    init_shfo_tests();
    tmp_flags = shfo.fFlags;
    set_curr_dir_path(from, "test1.txt\0test10.txt\0test2.txt\0");
    ok(!file_exists(".\\testdir2\\test1.txt"), "The file is not copied yet\n");
    ok(!file_exists(".\\testdir2\\test2.txt"), "The file is not copied yet\n");
    retval = SHFileOperationA(&shfo);
    if (!retval)
      /* Win 95/NT returns success but copies only the files up to the non-existent source */
      ok(file_exists(".\\testdir2\\test1.txt"), "The file is not copied\n");
    else
    {
      /* Win 98/ME/2K/XP fail the entire operation with return code 1026 if one source file does not exist */
      ok(retval == 1026, "Files are copied to other directory\n");
      ok(!file_exists(".\\testdir2\\test1.txt"), "The file is copied\n");
    }
    ok(!file_exists(".\\testdir2\\test2.txt"), "The file is copied\n");
    shfo.fFlags = tmp_flags;
}

/* tests the FO_MOVE action */
void test_move(void)
{
    SHFILEOPSTRUCTA shfo, shfo2;
    CHAR from[MAX_PATH];
    CHAR to[MAX_PATH];

    shfo.hwnd = NULL;
    shfo.wFunc = FO_MOVE;
    shfo.pFrom = from;
    shfo.pTo = to;
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    shfo.hNameMappings = NULL;
    shfo.lpszProgressTitle = NULL;

    set_curr_dir_path(from, "test1.txt\0");
    set_curr_dir_path(to, "test4.txt\0");
    ok(!SHFileOperationA(&shfo), "Prepare test to check how directories are moved recursively\n");
    ok(file_exists(".\\test4.txt\\test1.txt"), "The file is moved\n");

    set_curr_dir_path(from, "test?.txt\0");
    set_curr_dir_path(to, "testdir2\0");
    ok(!file_exists(".\\testdir2\\test2.txt"), "The file is not moved yet\n");
    ok(!file_exists(".\\testdir2\\test4.txt"), "The directory is not moved yet\n");
    ok(!SHFileOperationA(&shfo), "Files and directories are moved to directory\n");
    ok(file_exists(".\\testdir2\\test2.txt"), "The file is moved\n");
    ok(file_exists(".\\testdir2\\test4.txt"), "The directory is moved\n");
    ok(file_exists(".\\testdir2\\test4.txt\\test1.txt"), "The file in subdirectory is moved\n");

    clean_after_shfo_tests();
    init_shfo_tests();

    memcpy(&shfo2, &shfo, sizeof(SHFILEOPSTRUCTA));
    shfo2.fFlags |= FOF_MULTIDESTFILES;

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    ok(!SHFileOperationA(&shfo2), "Move many files\n");
    ok(file_exists(".\\test6.txt"), "The file is moved - many files are "
       "specified as a target\n");
    DeleteFileA(".\\test6.txt");
    DeleteFileA(".\\test7.txt");
    RemoveDirectoryA(".\\test8.txt");

    init_shfo_tests();

    /* number of sources do not correspond to number of targets */
    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0");
    ok(SHFileOperationA(&shfo2), "Can't move many files\n");
    ok(!file_exists(".\\test6.txt"), "The file is not moved - many files are "
       "specified as a target\n");

    init_shfo_tests();

    set_curr_dir_path(from, "test3.txt\0");
    set_curr_dir_path(to, "test4.txt\\test1.txt\0");
    ok(!SHFileOperationA(&shfo), "File is moved moving to other directory\n");
    ok(file_exists(".\\test4.txt\\test1.txt"), "The file is moved\n");

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    ok(SHFileOperationA(&shfo), "Can not move many files\n");
    ok(file_exists(".\\test1.txt"), "The file is not moved. Many files are specified\n");
    ok(file_exists(".\\test4.txt"), "The directory not is moved. Many files are specified\n");

    set_curr_dir_path(from, "test1.txt\0");
    set_curr_dir_path(to, "test6.txt\0");
    ok(!SHFileOperationA(&shfo), "Move file\n");
    ok(!file_exists(".\\test1.txt"), "The file is moved\n");
    ok(file_exists(".\\test6.txt"), "The file is moved\n");
    set_curr_dir_path(from, "test6.txt\0");
    set_curr_dir_path(to, "test1.txt\0");
    ok(!SHFileOperationA(&shfo), "Move file back\n");

    set_curr_dir_path(from, "test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0");
    ok(!SHFileOperationA(&shfo), "Move dir\n");
    ok(!file_exists(".\\test4.txt"), "The dir is moved\n");
    ok(file_exists(".\\test6.txt"), "The dir is moved\n");
    set_curr_dir_path(from, "test6.txt\0");
    set_curr_dir_path(to, "test4.txt\0");
    ok(!SHFileOperationA(&shfo), "Move dir back\n");
}

START_TEST(shlfileop)
{
    clean_after_shfo_tests();

    init_shfo_tests();
    test_delete();
    clean_after_shfo_tests();

    init_shfo_tests();
    test_rename();
    clean_after_shfo_tests();

    init_shfo_tests();
    test_copy();
    clean_after_shfo_tests();

    init_shfo_tests();
    test_move();
    clean_after_shfo_tests();
}
