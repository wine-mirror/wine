/*
 * Unit test suite for CreateProcess function.
 *
 * Copyright 2002 Eric Pouech
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "wine/test.h"
#include "winbase.h"
#include "winuser.h"

static char     base[MAX_PATH];
static char     selfname[MAX_PATH];
static char     resfile[MAX_PATH];

static int      myARGC;
static char**   myARGV;

/* ---------------- portable memory allocation thingie */

static char     memory[16384];
static char*    memory_index = memory;

static char*    grab_memory(size_t len)
{
    char*       ret = memory_index;
    /* align on dword */
    len = (len + 3) & ~3;
    memory_index += len;
    assert(memory_index <= memory + sizeof(memory));
    return ret;
}

static void     release_memory(void)
{
    memory_index = memory;
}

/* ---------------- simplistic tool to encode/decode strings (to hide \ " ' and such) */

static char*    encodeA(const char* str)
{
    char*       ptr;
    size_t      len,i;

    if (!str) return "";
    len = strlen(str) + 1;
    ptr = grab_memory(len * 2 + 1);
    for (i = 0; i < len; i++)
        sprintf(&ptr[i * 2], "%02x", (unsigned char)str[i]);
    ptr[2 * len] = '\0';
    return ptr;
}

static char*    encodeW(const WCHAR* str)
{
    char*       ptr;
    size_t      len,i;

    if (!str) return "";
    len = lstrlenW(str) + 1;
    ptr = grab_memory(len * 4 + 1);
    assert(ptr);
    for (i = 0; i < len; i++)
        sprintf(&ptr[i * 4], "%04x", (unsigned int)(unsigned short)str[i]);
    ptr[4 * len] = '\0';
    return ptr;
}

static unsigned decode_char(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    assert(c >= 'A' && c <= 'F');
    return c - 'A' + 10;
}

static char*    decodeA(const char* str)
{
    char*       ptr;
    size_t      len,i;

    len = strlen(str) / 2;
    if (!len--) return NULL;
    ptr = grab_memory(len + 1);
    for (i = 0; i < len; i++)
        ptr[i] = (decode_char(str[2 * i]) << 4) | decode_char(str[2 * i + 1]);
    ptr[len] = '\0';
    return ptr;
}

static WCHAR*   decodeW(const char* str)
{
    size_t      len;
    WCHAR*      ptr;
    int         i;

    len = strlen(str) / 4;
    if (!len--) return NULL;
    ptr = (WCHAR*)grab_memory(len * 2 + 1);
    for (i = 0; i < len; i++)
        ptr[i] = (decode_char(str[4 * i]) << 12) |
            (decode_char(str[4 * i + 1]) << 8) |
            (decode_char(str[4 * i + 2]) << 4) |
            (decode_char(str[4 * i + 3]) << 0);
    ptr[len] = '\0';
    return ptr;
}

/******************************************************************
 *		init
 *
 * generates basic information like:
 *      base:           absolute path to curr dir
 *      selfname:       the way to reinvoke ourselves
 */
static int     init(void)
{
    myARGC = winetest_get_mainargs( &myARGV );
    if (!GetCurrentDirectoryA(sizeof(base), base)) return 0;
    strcpy(selfname, myARGV[0]);
    return 1;
}

/******************************************************************
 *		get_file_name
 *
 * generates an absolute file_name for temporary file
 *
 */
static void     get_file_name(char* buf)
{
    char        path[MAX_PATH];

    buf[0] = '\0';
    GetTempPathA(sizeof(path), path);
    GetTempFileNameA(path, "wt", 0, buf);
}

/******************************************************************
 *		static void     childPrintf
 *
 */
static void     childPrintf(HANDLE h, const char* fmt, ...)
{
    va_list     valist;
    char        buffer[2048];
    DWORD       w;

    va_start(valist, fmt);
    vsprintf(buffer, fmt, valist);
    va_end(valist);
    WriteFile(h, buffer, strlen(buffer), &w, NULL);
}


/******************************************************************
 *		doChild
 *
 * output most of the information in the child process
 */
static void     doChild(const char* file)
{
    STARTUPINFOA        siA;
    STARTUPINFOW        siW;
    int                 i;
    char*               ptrA;
    WCHAR*              ptrW;
    char                bufA[MAX_PATH];
    WCHAR               bufW[MAX_PATH];
    HANDLE              hFile = CreateFileA(file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);

    if (hFile == INVALID_HANDLE_VALUE) return;

    /* output of startup info (Ansi) */
    GetStartupInfoA(&siA);
    childPrintf(hFile,
                "[StartupInfoA]\ncb=%08ld\nlpDesktop=%s\nlpTitle=%s\n"
                "dwX=%lu\ndwY=%lu\ndwXSize=%lu\ndwYSize=%lu\n"
                "dwXCountChars=%lu\ndwYCountChars=%lu\ndwFillAttribute=%lu\n"
                "dwFlags=%lu\nwShowWindow=%u\n"
                "hStdInput=%lu\nhStdOutput=%lu\nhStdError=%lu\n\n",
                siA.cb, encodeA(siA.lpDesktop), encodeA(siA.lpTitle),
                siA.dwX, siA.dwY, siA.dwXSize, siA.dwYSize,
                siA.dwXCountChars, siA.dwYCountChars, siA.dwFillAttribute,
                siA.dwFlags, siA.wShowWindow,
                (DWORD)siA.hStdInput, (DWORD)siA.hStdOutput, (DWORD)siA.hStdError);

    /* since GetStartupInfoW is only implemented in win2k,
     * zero out before calling so we can notice the difference
     */
    memset(&siW, 0, sizeof(siW));
    GetStartupInfoW(&siW);
    childPrintf(hFile,
                "[StartupInfoW]\ncb=%08ld\nlpDesktop=%s\nlpTitle=%s\n"
                "dwX=%lu\ndwY=%lu\ndwXSize=%lu\ndwYSize=%lu\n"
                "dwXCountChars=%lu\ndwYCountChars=%lu\ndwFillAttribute=%lu\n"
                "dwFlags=%lu\nwShowWindow=%u\n"
                "hStdInput=%lu\nhStdOutput=%lu\nhStdError=%lu\n\n",
                siW.cb, encodeW(siW.lpDesktop), encodeW(siW.lpTitle),
                siW.dwX, siW.dwY, siW.dwXSize, siW.dwYSize,
                siW.dwXCountChars, siW.dwYCountChars, siW.dwFillAttribute,
                siW.dwFlags, siW.wShowWindow,
                (DWORD)siW.hStdInput, (DWORD)siW.hStdOutput, (DWORD)siW.hStdError);

    /* Arguments */
    childPrintf(hFile, "[Arguments]\nargcA=%d\n", myARGC);
    for (i = 0; i < myARGC; i++)
    {
        childPrintf(hFile, "argvA%d=%s\n", i, encodeA(myARGV[i]));
    }
    childPrintf(hFile, "CommandLineA=%s\n", encodeA(GetCommandLineA()));

#if 0
    int                 argcW;
    WCHAR**             argvW;

    /* this is part of shell32... and should be tested there */
    argvW = CommandLineToArgvW(GetCommandLineW(), &argcW);
    for (i = 0; i < argcW; i++)
    {
        childPrintf(hFile, "argvW%d=%s\n", i, encodeW(argvW[i]));
    }
#endif
    childPrintf(hFile, "CommandLineW=%s\n\n", encodeW(GetCommandLineW()));

    /* output of environment (Ansi) */
    ptrA = GetEnvironmentStringsA();
    if (ptrA)
    {
        childPrintf(hFile, "[EnvironmentA]\n");
        i = 0;
        while (*ptrA)
        {
            if (strlen(ptrA) < 128)
            {
                childPrintf(hFile, "env%d=%s\n", i, encodeA(ptrA));
                i++;
            }
            ptrA += strlen(ptrA) + 1;
        }
        childPrintf(hFile, "\n");
    }

    /* output of environment (Unicode) */
    ptrW = GetEnvironmentStringsW();
    if (ptrW)
    {
        childPrintf(hFile, "[EnvironmentW]\n");
        i = 0;
        while (*ptrW)
        {
            if (lstrlenW(ptrW) < 128)
            {
                childPrintf(hFile, "env%d=%s\n", i, encodeW(ptrW));
                i++;
            }
            ptrW += lstrlenW(ptrW) + 1;
        }
        childPrintf(hFile, "\n");
    }

    childPrintf(hFile, "[Misc]\n");
    if (GetCurrentDirectoryA(sizeof(bufA), bufA))
        childPrintf(hFile, "CurrDirA=%s\n", encodeA(bufA));
    if (GetCurrentDirectoryW(sizeof(bufW) / sizeof(bufW[0]), bufW))
        childPrintf(hFile, "CurrDirW=%s\n", encodeW(bufW));
    childPrintf(hFile, "\n");

    CloseHandle(hFile);
}

static char* getChildString(const char* sect, const char* key)
{
    char        buf[1024];
    char*       ret;

    GetPrivateProfileStringA(sect, key, "-", buf, sizeof(buf), resfile);
    if (buf[0] == '\0' || (buf[0] == '-' && buf[1] == '\0')) return NULL;
    assert(!(strlen(buf) & 1));
    ret = decodeA(buf);
    return ret;
}

/* FIXME: this may be moved to the wtmain.c file, because it may be needed by
 * others... (windows uses stricmp while Un*x uses strcasecmp...)
 */
static int wtstrcasecmp(const char* p1, const char* p2)
{
    char c1, c2;

    c1 = c2 = '@';
    while (c1 == c2 && c1)
    {
        c1 = *p1++; c2 = *p2++;
        if (c1 != c2)
        {
            c1 = toupper(c1); c2 = toupper(c2);
        }
    }
    return c1 - c2;
}

static int strCmp(const char* s1, const char* s2, BOOL sensitive)
{
    if (!s1 && !s2) return 0;
    if (!s2) return -1;
    if (!s1) return 1;
    return (sensitive) ? strcmp(s1, s2) : wtstrcasecmp(s1, s2);
}

#define okChildString(sect, key, expect) \
    do { \
        char* result = getChildString((sect), (key)); \
        ok(strCmp(result, expect, 1) == 0, "%s:%s expected %s, got %s", (sect), (key), (expect)?(expect):"(null)", result); \
    } while (0)

#define okChildIString(sect, key, expect) \
    do { \
        char* result = getChildString(sect, key); \
        ok(strCmp(result, expect, 0) == 0, "%s:%s expected %s, got %s", sect, key, expect, result); \
    } while (0)

/* using !expect insures that the test will fail if the sect/key isn't present
 * in result file
 */
#define okChildInt(sect, key, expect) \
    do { \
        UINT result = GetPrivateProfileIntA((sect), (key), !(expect), resfile); \
        ok(result == expect, "%s:%s expected %d, but got %d\n", (sect), (key), (int)(expect), result); \
   } while (0)

static void test_Startup(void)
{
    char                buffer[MAX_PATH];
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup;

    /* let's start simplistic */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildInt("StartupInfoA", "cb", startup.cb);
    okChildString("StartupInfoA", "lpDesktop", startup.lpDesktop);
    okChildString("StartupInfoA", "lpTitle", startup.lpTitle);
    okChildInt("StartupInfoA", "dwX", startup.dwX);
    okChildInt("StartupInfoA", "dwY", startup.dwY);
    okChildInt("StartupInfoA", "dwXSize", startup.dwXSize);
    okChildInt("StartupInfoA", "dwYSize", startup.dwYSize);
    okChildInt("StartupInfoA", "dwXCountChars", startup.dwXCountChars);
    okChildInt("StartupInfoA", "dwYCountChars", startup.dwYCountChars);
    okChildInt("StartupInfoA", "dwFillAttribute", startup.dwFillAttribute);
    okChildInt("StartupInfoA", "dwFlags", startup.dwFlags);
    okChildInt("StartupInfoA", "wShowWindow", startup.wShowWindow);
    release_memory();
    assert(DeleteFileA(resfile) != 0);

    /* not so simplistic now */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.lpTitle = "I'm the title string";
    startup.lpDesktop = "I'm the desktop string";
    startup.dwXCountChars = 0x12121212;
    startup.dwYCountChars = 0x23232323;
    startup.dwX = 0x34343434;
    startup.dwY = 0x45454545;
    startup.dwXSize = 0x56565656;
    startup.dwYSize = 0x67676767;
    startup.dwFillAttribute = 0xA55A;

    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildInt("StartupInfoA", "cb", startup.cb);
    okChildString("StartupInfoA", "lpDesktop", startup.lpDesktop);
    okChildString("StartupInfoA", "lpTitle", startup.lpTitle);
    okChildInt("StartupInfoA", "dwX", startup.dwX);
    okChildInt("StartupInfoA", "dwY", startup.dwY);
    okChildInt("StartupInfoA", "dwXSize", startup.dwXSize);
    okChildInt("StartupInfoA", "dwYSize", startup.dwYSize);
    okChildInt("StartupInfoA", "dwXCountChars", startup.dwXCountChars);
    okChildInt("StartupInfoA", "dwYCountChars", startup.dwYCountChars);
    okChildInt("StartupInfoA", "dwFillAttribute", startup.dwFillAttribute);
    okChildInt("StartupInfoA", "dwFlags", startup.dwFlags);
    okChildInt("StartupInfoA", "wShowWindow", startup.wShowWindow);
    release_memory();
    assert(DeleteFileA(resfile) != 0);

    /* not so simplistic now */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.lpTitle = "I'm the title string";
    startup.lpDesktop = NULL;
    startup.dwXCountChars = 0x12121212;
    startup.dwYCountChars = 0x23232323;
    startup.dwX = 0x34343434;
    startup.dwY = 0x45454545;
    startup.dwXSize = 0x56565656;
    startup.dwYSize = 0x67676767;
    startup.dwFillAttribute = 0xA55A;

    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildInt("StartupInfoA", "cb", startup.cb);
    okChildString("StartupInfoA", "lpDesktop", startup.lpDesktop);
    okChildString("StartupInfoA", "lpTitle", startup.lpTitle);
    okChildInt("StartupInfoA", "dwX", startup.dwX);
    okChildInt("StartupInfoA", "dwY", startup.dwY);
    okChildInt("StartupInfoA", "dwXSize", startup.dwXSize);
    okChildInt("StartupInfoA", "dwYSize", startup.dwYSize);
    okChildInt("StartupInfoA", "dwXCountChars", startup.dwXCountChars);
    okChildInt("StartupInfoA", "dwYCountChars", startup.dwYCountChars);
    okChildInt("StartupInfoA", "dwFillAttribute", startup.dwFillAttribute);
    okChildInt("StartupInfoA", "dwFlags", startup.dwFlags);
    okChildInt("StartupInfoA", "wShowWindow", startup.wShowWindow);
    release_memory();
    assert(DeleteFileA(resfile) != 0);

    /* not so simplistic now */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.lpTitle = "I'm the title string";
    startup.lpDesktop = "";
    startup.dwXCountChars = 0x12121212;
    startup.dwYCountChars = 0x23232323;
    startup.dwX = 0x34343434;
    startup.dwY = 0x45454545;
    startup.dwXSize = 0x56565656;
    startup.dwYSize = 0x67676767;
    startup.dwFillAttribute = 0xA55A;

    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildInt("StartupInfoA", "cb", startup.cb);
    todo_wine okChildString("StartupInfoA", "lpDesktop", startup.lpDesktop);
    okChildString("StartupInfoA", "lpTitle", startup.lpTitle);
    okChildInt("StartupInfoA", "dwX", startup.dwX);
    okChildInt("StartupInfoA", "dwY", startup.dwY);
    okChildInt("StartupInfoA", "dwXSize", startup.dwXSize);
    okChildInt("StartupInfoA", "dwYSize", startup.dwYSize);
    okChildInt("StartupInfoA", "dwXCountChars", startup.dwXCountChars);
    okChildInt("StartupInfoA", "dwYCountChars", startup.dwYCountChars);
    okChildInt("StartupInfoA", "dwFillAttribute", startup.dwFillAttribute);
    okChildInt("StartupInfoA", "dwFlags", startup.dwFlags);
    okChildInt("StartupInfoA", "wShowWindow", startup.wShowWindow);
    release_memory();
    assert(DeleteFileA(resfile) != 0);

    /* not so simplistic now */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.lpTitle = NULL;
    startup.lpDesktop = "I'm the desktop string";
    startup.dwXCountChars = 0x12121212;
    startup.dwYCountChars = 0x23232323;
    startup.dwX = 0x34343434;
    startup.dwY = 0x45454545;
    startup.dwXSize = 0x56565656;
    startup.dwYSize = 0x67676767;
    startup.dwFillAttribute = 0xA55A;

    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildInt("StartupInfoA", "cb", startup.cb);
    okChildString("StartupInfoA", "lpDesktop", startup.lpDesktop);
    okChildString("StartupInfoA", "lpTitle", startup.lpTitle);
    okChildInt("StartupInfoA", "dwX", startup.dwX);
    okChildInt("StartupInfoA", "dwY", startup.dwY);
    okChildInt("StartupInfoA", "dwXSize", startup.dwXSize);
    okChildInt("StartupInfoA", "dwYSize", startup.dwYSize);
    okChildInt("StartupInfoA", "dwXCountChars", startup.dwXCountChars);
    okChildInt("StartupInfoA", "dwYCountChars", startup.dwYCountChars);
    okChildInt("StartupInfoA", "dwFillAttribute", startup.dwFillAttribute);
    okChildInt("StartupInfoA", "dwFlags", startup.dwFlags);
    okChildInt("StartupInfoA", "wShowWindow", startup.wShowWindow);
    release_memory();
    assert(DeleteFileA(resfile) != 0);

    /* not so simplistic now */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.lpTitle = "";
    startup.lpDesktop = "I'm the desktop string";
    startup.dwXCountChars = 0x12121212;
    startup.dwYCountChars = 0x23232323;
    startup.dwX = 0x34343434;
    startup.dwY = 0x45454545;
    startup.dwXSize = 0x56565656;
    startup.dwYSize = 0x67676767;
    startup.dwFillAttribute = 0xA55A;

    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildInt("StartupInfoA", "cb", startup.cb);
    okChildString("StartupInfoA", "lpDesktop", startup.lpDesktop);
    todo_wine okChildString("StartupInfoA", "lpTitle", startup.lpTitle);
    okChildInt("StartupInfoA", "dwX", startup.dwX);
    okChildInt("StartupInfoA", "dwY", startup.dwY);
    okChildInt("StartupInfoA", "dwXSize", startup.dwXSize);
    okChildInt("StartupInfoA", "dwYSize", startup.dwYSize);
    okChildInt("StartupInfoA", "dwXCountChars", startup.dwXCountChars);
    okChildInt("StartupInfoA", "dwYCountChars", startup.dwYCountChars);
    okChildInt("StartupInfoA", "dwFillAttribute", startup.dwFillAttribute);
    okChildInt("StartupInfoA", "dwFlags", startup.dwFlags);
    okChildInt("StartupInfoA", "wShowWindow", startup.wShowWindow);
    release_memory();
    assert(DeleteFileA(resfile) != 0);

    /* not so simplistic now */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.lpTitle = "";
    startup.lpDesktop = "";
    startup.dwXCountChars = 0x12121212;
    startup.dwYCountChars = 0x23232323;
    startup.dwX = 0x34343434;
    startup.dwY = 0x45454545;
    startup.dwXSize = 0x56565656;
    startup.dwYSize = 0x67676767;
    startup.dwFillAttribute = 0xA55A;

    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildInt("StartupInfoA", "cb", startup.cb);
    todo_wine okChildString("StartupInfoA", "lpDesktop", startup.lpDesktop);
    todo_wine okChildString("StartupInfoA", "lpTitle", startup.lpTitle);
    okChildInt("StartupInfoA", "dwX", startup.dwX);
    okChildInt("StartupInfoA", "dwY", startup.dwY);
    okChildInt("StartupInfoA", "dwXSize", startup.dwXSize);
    okChildInt("StartupInfoA", "dwYSize", startup.dwYSize);
    okChildInt("StartupInfoA", "dwXCountChars", startup.dwXCountChars);
    okChildInt("StartupInfoA", "dwYCountChars", startup.dwYCountChars);
    okChildInt("StartupInfoA", "dwFillAttribute", startup.dwFillAttribute);
    okChildInt("StartupInfoA", "dwFlags", startup.dwFlags);
    okChildInt("StartupInfoA", "wShowWindow", startup.wShowWindow);
    release_memory();
    assert(DeleteFileA(resfile) != 0);

    /* TODO: test for A/W and W/A and W/W */
}

static void test_CommandLine(void)
{
    char                buffer[MAX_PATH];
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup;

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    /* the basics */
    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s \"C:\\Program Files\\my nice app.exe\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildInt("Arguments", "argcA", 4);
    okChildString("Arguments", "argvA3", "C:\\Program Files\\my nice app.exe");
    okChildString("Arguments", "argvA4", NULL);
    okChildString("Arguments", "CommandLineA", buffer);
    release_memory();
    assert(DeleteFileA(resfile) != 0);

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    /* from Frangois */
    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s \"a\\\"b\\\\\" c\\\" d", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildInt("Arguments", "argcA", 6);
    okChildString("Arguments", "argvA3", "a\"b\\");
    okChildString("Arguments", "argvA4", "c\"");
    okChildString("Arguments", "argvA5", "d");
    okChildString("Arguments", "argvA6", NULL);
    okChildString("Arguments", "CommandLineA", buffer);
    release_memory();
    assert(DeleteFileA(resfile) != 0);
}

static void test_Directory(void)
{
    char                buffer[MAX_PATH];
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup;
    char windir[MAX_PATH];

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    /* the basics */
    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s", selfname, resfile);
    GetWindowsDirectoryA( windir, sizeof(windir) );
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, windir, &startup, &info), "CreateProcess");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildIString("Misc", "CurrDirA", windir);
    release_memory();
    assert(DeleteFileA(resfile) != 0);
}

START_TEST(process)
{
    int b = init();
    ok(b, "Basic init of CreateProcess test");
    if (!b) return;

    if (myARGC >= 3)
    {
        doChild(myARGV[2]);
        return;
    }
    test_Startup();
    test_CommandLine();
    test_Directory();

    /* things that can be tested:
     *  lookup:         check the way program to be executed is searched
     *  environment:    check environment string passing
     *  handles:        check the handle inheritance stuff (+sec options)
     *  console:        check if console creation parameters work
     */
}
