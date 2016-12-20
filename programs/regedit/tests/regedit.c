/*
 * Copyright 2010 Andrew Eikum for CodeWeavers
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

#include <windows.h>

#include "wine/test.h"

static BOOL supports_wchar;

#define lok ok_(__FILE__,line)

#define exec_import_str(c) r_exec_import_str(__LINE__, c)
static BOOL r_exec_import_str(unsigned line, const char *file_contents)
{
    STARTUPINFOA si = {sizeof(STARTUPINFOA)};
    PROCESS_INFORMATION pi;
    HANDLE regfile;
    DWORD written, dr;
    BOOL br;
    char cmd[] = "regedit /s test.reg";

    regfile = CreateFileA("test.reg", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    lok(regfile != INVALID_HANDLE_VALUE, "Failed to create test.reg file\n");
    if(regfile == INVALID_HANDLE_VALUE)
        return FALSE;

    br = WriteFile(regfile, file_contents, strlen(file_contents), &written,
            NULL);
    lok(br == TRUE, "WriteFile failed: %d\n", GetLastError());

    CloseHandle(regfile);

    if(!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        return FALSE;

    dr = WaitForSingleObject(pi.hProcess, 10000);
    if(dr == WAIT_TIMEOUT)
        TerminateProcess(pi.hProcess, 1);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    br = DeleteFileA("test.reg");
    lok(br == TRUE, "DeleteFileA failed: %d\n", GetLastError());

    return (dr != WAIT_TIMEOUT);
}

#define exec_import_wstr(c) r_exec_import_wstr(__LINE__, c)
static BOOL r_exec_import_wstr(unsigned line, const WCHAR *file_contents)
{
    STARTUPINFOA si = {sizeof(STARTUPINFOA)};
    PROCESS_INFORMATION pi;
    HANDLE regfile;
    DWORD written, dr;
    BOOL br;
    char cmd[] = "regedit /s test.reg";

    regfile = CreateFileA("test.reg", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    lok(regfile != INVALID_HANDLE_VALUE, "Failed to create test.reg file\n");
    if(regfile == INVALID_HANDLE_VALUE)
        return FALSE;

    br = WriteFile(regfile, file_contents,
            lstrlenW(file_contents) * sizeof(WCHAR), &written, NULL);
    lok(br == TRUE, "WriteFile failed: %d\n", GetLastError());

    CloseHandle(regfile);

    if(!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        return FALSE;

    dr = WaitForSingleObject(pi.hProcess, 10000);
    if(dr == WAIT_TIMEOUT)
        TerminateProcess(pi.hProcess, 1);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    br = DeleteFileA("test.reg");
    lok(br == TRUE, "DeleteFileA failed: %d\n", GetLastError());

    return (dr != WAIT_TIMEOUT);
}

#define TODO_REG_TYPE    (0x0001u)
#define TODO_REG_SIZE    (0x0002u)
#define TODO_REG_DATA    (0x0004u)

/* verify_reg() adapted from programs/reg/tests/reg.c */
#define verify_reg(k,v,t,d,s,todo) verify_reg_(__LINE__,k,v,t,d,s,todo)
static void verify_reg_(unsigned line, HKEY hkey, const char *value,
                        DWORD exp_type, const void *exp_data, DWORD exp_size, DWORD todo)
{
    DWORD type, size;
    BYTE data[256];
    LONG err;

    size = sizeof(data);
    memset(data, 0xdd, size);
    err = RegQueryValueExA(hkey, value, NULL, &type, data, &size);
    lok(err == ERROR_SUCCESS, "RegQueryValueEx failed: got %d\n", err);
    if (err != ERROR_SUCCESS)
        return;

    todo_wine_if (todo & TODO_REG_TYPE)
        lok(type == exp_type, "got wrong type %d, expected %d\n", type, exp_type);
    todo_wine_if (todo & TODO_REG_SIZE)
        lok(size == exp_size, "got wrong size %d, expected %d\n", size, exp_size);
    todo_wine_if (todo & TODO_REG_DATA)
        lok(memcmp(data, exp_data, size) == 0, "got wrong data\n");
}

#define verify_reg_wsz(k,n,e) r_verify_reg_wsz(__LINE__,k,n,e)
static void r_verify_reg_wsz(unsigned line, HKEY key, const char *value_name, const WCHAR *exp_value)
{
    LONG lr;
    DWORD fnd_type, fnd_len;
    WCHAR fnd_value[1024], value_nameW[1024];

    MultiByteToWideChar(CP_ACP, 0, value_name, -1, value_nameW,
            sizeof(value_nameW)/sizeof(value_nameW[0]));

    fnd_len = sizeof(fnd_value);
    lr = RegQueryValueExW(key, value_nameW, NULL, &fnd_type, (BYTE*)fnd_value, &fnd_len);
    lok(lr == ERROR_SUCCESS, "RegQueryValueExW failed: %d\n", lr);
    if(lr != ERROR_SUCCESS)
        return;

    lok(fnd_type == REG_SZ, "Got wrong type: %d\n", fnd_type);
    if(fnd_type != REG_SZ)
        return;
    lok(!lstrcmpW(exp_value, fnd_value),
            "Strings differ: expected %s, got %s\n",
            wine_dbgstr_w(exp_value), wine_dbgstr_w(fnd_value));
}

#define verify_reg_nonexist(k,n) r_verify_reg_nonexist(__LINE__,k,n)
static void r_verify_reg_nonexist(unsigned line, HKEY key, const char *value_name)
{
    LONG lr;
    DWORD fnd_type, fnd_len;
    char fnd_value[32];

    fnd_len = sizeof(fnd_value);
    lr = RegQueryValueExA(key, value_name, NULL, &fnd_type, (BYTE*)fnd_value, &fnd_len);
    lok(lr == ERROR_FILE_NOT_FOUND, "Reg value shouldn't exist: %s\n",
            value_name);
}

#define KEY_BASE "Software\\Wine\\regedit_test"

static void test_basic_import(void)
{
    HKEY hkey;
    DWORD dword = 0x17;
    char exp_binary[] = {0xAA,0xBB,0xCC,0x11};
    WCHAR wide_test[] = {0xFEFF,'W','i','n','d','o','w','s',' ','R','e','g',
        'i','s','t','r','y',' ','E','d','i','t','o','r',' ','V','e','r','s',
        'i','o','n',' ','5','.','0','0','\n','\n',
        '[','H','K','E','Y','_','C','U','R','R','E','N','T','_','U','S','E',
        'R','\\','S','o','f','t','w','a','r','e','\\','W','i','n','e','\\',
        'r','e','g','e','d','i','t','_','t','e','s','t',']','\n',
        '"','T','e','s','t','V','a','l','u','e','3','"','=','"',0x3041,'V','a',
        'l','u','e','"','\n',0};
    WCHAR wide_test_r[] = {0xFEFF,'W','i','n','d','o','w','s',' ','R','e','g',
        'i','s','t','r','y',' ','E','d','i','t','o','r',' ','V','e','r','s',
        'i','o','n',' ','5','.','0','0','\r','\r',
        '[','H','K','E','Y','_','C','U','R','R','E','N','T','_','U','S','E',
        'R','\\','S','o','f','t','w','a','r','e','\\','W','i','n','e','\\',
        'r','e','g','e','d','i','t','_','t','e','s','t',']','\r',
        '"','T','e','s','t','V','a','l','u','e','5','"','=','"',0x3041,'V','a',
        'l','u','e','"','\r',0};
    WCHAR wide_exp[] = {0x3041,'V','a','l','u','e',0};
    LONG lr;

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS || lr == ERROR_FILE_NOT_FOUND,
            "RegDeleteKeyA failed: %d\n", lr);

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "\"TestValue\"=\"AValue\"\n");
    lr = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    ok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: %d\n", lr);
    verify_reg(hkey, "TestValue", REG_SZ, "AValue", 7, 0);

    exec_import_str("REGEDIT4\r\n\r\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\r\n"
                "\"TestValue2\"=\"BValue\"\r\n");
    verify_reg(hkey, "TestValue2", REG_SZ, "BValue", 7, 0);

    if (supports_wchar)
    {
        exec_import_wstr(wide_test);
        verify_reg_wsz(hkey, "TestValue3", wide_exp);

        exec_import_wstr(wide_test_r);
        verify_reg_wsz(hkey, "TestValue5", wide_exp);
    }
    else
        win_skip("Some WCHAR tests skipped\n");

    exec_import_str("REGEDIT4\r\r"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\r"
                "\"TestValue4\"=\"DValue\"\r");
    verify_reg(hkey, "TestValue4", REG_SZ, "DValue", 7, 0);

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "\"TestDword\"=dword:00000017\n");
    verify_reg(hkey, "TestDword", REG_DWORD, &dword, sizeof(dword), 0);

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "\"TestBinary\"=hex:aa,bb,cc,11\n");
    verify_reg(hkey, "TestBinary", REG_BINARY, exp_binary, sizeof(exp_binary), 0);

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "\"With=Equals\"=\"asdf\"\n");
    verify_reg(hkey, "With=Equals", REG_SZ, "asdf", 5, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Line1\"=\"Value1\"\n\n"
                    "\"Line2\"=\"Value2\"\n\n\n"
                    "\"Line3\"=\"Value3\"\n\n\n\n"
                    "\"Line4\"=\"Value4\"\n\n");
    verify_reg(hkey, "Line1", REG_SZ, "Value1", 7, 0);
    verify_reg(hkey, "Line2", REG_SZ, "Value2", 7, 0);
    verify_reg(hkey, "Line3", REG_SZ, "Value3", 7, 0);
    verify_reg(hkey, "Line4", REG_SZ, "Value4", 7, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine1\"=dword:00000782\n\n"
                    "\"Wine2\"=\"Test Value\"\n"
                    "\"Wine3\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n"
                    "#comment\n"
                    "@=\"Test\"\n"
                    ";comment\n\n"
                    "\"Wine4\"=dword:12345678\n\n");
    dword = 0x782;
    verify_reg(hkey, "Wine1", REG_DWORD, &dword, sizeof(dword), 0);
    verify_reg(hkey, "Wine2", REG_SZ, "Test Value", 11, 0);
    verify_reg(hkey, "Wine3", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "", REG_SZ, "Test", 5, 0);
    dword = 0x12345678;
    verify_reg(hkey, "Wine4", REG_DWORD, &dword, sizeof(dword), 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine5\"=\"No newline\"");
    todo_wine verify_reg(hkey, "Wine5", REG_SZ, "No newline", 11, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine6\"=dword:00000050\n\n"
                    "\"Wine7\"=\"No newline\"");
    dword = 0x50;
    verify_reg(hkey, "Wine6", REG_DWORD, &dword, sizeof(dword), 0);
    todo_wine verify_reg(hkey, "Wine7", REG_SZ, "No newline", 11, 0);

    RegCloseKey(hkey);

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: %d\n", lr);
}

static void test_invalid_import(void)
{
    LONG lr;
    HKEY hkey;

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS || lr == ERROR_FILE_NOT_FOUND, "RegDeleteKeyA failed: %d\n", lr);

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "\"TestNoEndQuote\"=\"Asdffdsa\n");
    lr = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    ok(lr == ERROR_SUCCESS, "RegOpenKeyExA failed: %d\n", lr);
    verify_reg_nonexist(hkey, "TestNoEndQuote");

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "\"TestNoBeginQuote\"=Asdffdsa\"\n");
    verify_reg_nonexist(hkey, "TestNoBeginQuote");

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "\"TestNoQuotes\"=Asdffdsa\n");
    verify_reg_nonexist(hkey, "TestNoQuotes");

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "\"NameNoEndQuote=\"Asdffdsa\"\n");
    verify_reg_nonexist(hkey, "NameNoEndQuote");

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "NameNoBeginQuote\"=\"Asdffdsa\"\n");
    verify_reg_nonexist(hkey, "NameNoBeginQuote");

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "NameNoQuotes=\"Asdffdsa\"\n");
    verify_reg_nonexist(hkey, "NameNoQuotes");

    exec_import_str("REGEDIT4\n\n"
                "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                "\"MixedQuotes=Asdffdsa\"\n");
    verify_reg_nonexist(hkey, "MixedQuotes");
    verify_reg_nonexist(hkey, "MixedQuotes=Asdffdsa");

    RegCloseKey(hkey);

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: %d\n", lr);
}

static void test_comments(void)
{
    LONG lr;
    HKEY hkey;
    DWORD dword;

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS || lr == ERROR_FILE_NOT_FOUND, "RegDeleteKeyA failed: %d\n", lr);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "#comment\\\n"
                    "\"Wine1\"=\"Line 1\"\n"
                    ";comment\\\n"
                    "\"Wine2\"=\"Line 2\"\n\n");
    lr = RegOpenKeyExA(HKEY_CURRENT_USER, KEY_BASE, 0, KEY_READ, &hkey);
    verify_reg(hkey, "Wine1", REG_SZ, "Line 1", 7, 0);
    verify_reg(hkey, "Wine2", REG_SZ, "Line 2", 7, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine3\"=\"Value 1\"#comment\n"
                    "\"Wine4\"=\"Value 2\";comment\n"
                    "\"Wine5\"=dword:01020304 #comment\n"
                    "\"Wine6\"=dword:02040608 ;comment\n\n");
    verify_reg_nonexist(hkey, "Wine3");
    todo_wine verify_reg(hkey, "Wine4", REG_SZ, "Value 2", 8, 0);
    verify_reg_nonexist(hkey, "Wine5");
    dword = 0x2040608;
    todo_wine verify_reg(hkey, "Wine6", REG_DWORD, &dword, sizeof(dword), 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine7\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  #comment\n"
                    "  63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n"
                    "\"Wine8\"=\"A valid line\"\n"
                    "\"Wine9\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  ;comment\n"
                    "  63,6f,6e,63,61,74,65,6e,61,74,69,6f,6e,00,00\n"
                    "\"Wine10\"=\"Another valid line\"\n\n");
    verify_reg_nonexist(hkey, "Wine7");
    verify_reg(hkey, "Wine8", REG_SZ, "A valid line", 13, 0);
    todo_wine verify_reg(hkey, "Wine9", REG_MULTI_SZ, "Line concatenation\0", 20, 0);
    verify_reg(hkey, "Wine10", REG_SZ, "Another valid line", 19, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "#\"Comment1\"=\"Value 1\"\n"
                    ";\"Comment2\"=\"Value 2\"\n"
                    "    #\"Comment3\"=\"Value 3\"\n"
                    "    ;\"Comment4\"=\"Value 4\"\n"
                    "\"Wine11\"=\"Value 6\"#\"Comment5\"=\"Value 5\"\n"
                    "\"Wine12\"=\"Value 7\";\"Comment6\"=\"Value 6\"\n\n");
    verify_reg_nonexist(hkey, "Comment1");
    verify_reg_nonexist(hkey, "Comment2");
    verify_reg_nonexist(hkey, "Comment3");
    verify_reg_nonexist(hkey, "Comment4");
    todo_wine verify_reg_nonexist(hkey, "Wine11");
    verify_reg_nonexist(hkey, "Comment5");
    verify_reg(hkey, "Wine12", REG_SZ, "Value 7", 8, TODO_REG_SIZE|TODO_REG_DATA);
    verify_reg_nonexist(hkey, "Comment6");

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Wine13\"=#\"Value 8\"\n"
                    "\"Wine14\"=;\"Value 9\"\n"
                    "\"Wine15\"=\"#comment1\"\n"
                    "\"Wine16\"=\";comment2\"\n"
                    "\"Wine17\"=\"Value#comment3\"\n"
                    "\"Wine18\"=\"Value;comment4\"\n"
                    "\"Wine19\"=\"Value #comment5\"\n"
                    "\"Wine20\"=\"Value ;comment6\"\n"
                    "\"Wine21\"=#dword:00000001\n"
                    "\"Wine22\"=;dword:00000002\n"
                    "\"Wine23\"=dword:00000003#comment\n"
                    "\"Wine24\"=dword:00000004;comment\n\n");
    verify_reg_nonexist(hkey, "Wine13");
    verify_reg_nonexist(hkey, "Wine14");
    verify_reg(hkey, "Wine15", REG_SZ, "#comment1", 10, 0);
    verify_reg(hkey, "Wine16", REG_SZ, ";comment2", 10, 0);
    verify_reg(hkey, "Wine17", REG_SZ, "Value#comment3", 15, 0);
    verify_reg(hkey, "Wine18", REG_SZ, "Value;comment4", 15, 0);
    verify_reg(hkey, "Wine19", REG_SZ, "Value #comment5", 16, 0);
    verify_reg(hkey, "Wine20", REG_SZ, "Value ;comment6", 16, 0);
    verify_reg_nonexist(hkey, "Wine21");
    verify_reg_nonexist(hkey, "Wine22");
    verify_reg_nonexist(hkey, "Wine23");
    dword = 0x00000004;
    todo_wine verify_reg(hkey, "Wine24", REG_DWORD, &dword, sizeof(dword), 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line1\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,\\;comment\n"
                    "  63,61,74,\\;comment\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n\n");
    todo_wine verify_reg(hkey, "Multi-Line1", REG_MULTI_SZ, "Line concatenation\0", 20, 0);

    exec_import_str("REGEDIT4\n\n"
                    "[HKEY_CURRENT_USER\\" KEY_BASE "]\n"
                    "\"Multi-Line2\"=hex(7):4c,69,6e,65,20,\\\n"
                    "  63,6f,6e,\\;comment\n"
                    "  63,61,74,;comment\n"
                    "  65,6e,61,74,69,6f,6e,00,00\n\n");
    todo_wine verify_reg(hkey, "Multi-Line2", REG_MULTI_SZ, "Line concat", 12, 0);

    RegCloseKey(hkey);

    lr = RegDeleteKeyA(HKEY_CURRENT_USER, KEY_BASE);
    ok(lr == ERROR_SUCCESS, "RegDeleteKeyA failed: %d\n", lr);
}

START_TEST(regedit)
{
    WCHAR wchar_test[] = {0xFEFF,'W','i','n','d','o','w','s',' ','R','e','g',
        'i','s','t','r','y',' ','E','d','i','t','o','r',' ','V','e','r','s',
        'i','o','n',' ','5','.','0','0','\n','\n',0};

    if(!exec_import_str("REGEDIT4\r\n\r\n")){
        win_skip("regedit not available, skipping regedit tests\n");
        return;
    }

    supports_wchar = exec_import_wstr(wchar_test);

    test_basic_import();
    test_invalid_import();
    test_comments();
}
