/*
 * Copyright 2019 Fabian Maurer
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
#include <stdio.h>

#include "wine/test.h"

static void read_all_from_handle(HANDLE handle, BYTE **str, int *len)
{
    char buffer[4096];
    DWORD bytes_read;
    DWORD length = 0;
    BYTE *ret = calloc(1, 1);

    for (;;)
    {
        BOOL success = ReadFile(handle, buffer, sizeof(buffer), &bytes_read, NULL);
        if (!success || !bytes_read)
            break;
        ret = realloc(ret, length + bytes_read);
        memcpy((char *)ret + length, buffer, bytes_read);
        length += bytes_read;
    }

    *str = ret;
    *len = length;
}

static void write_to_handle(HANDLE handle, const BYTE *str, int len)
{
	DWORD dummy;
	WriteFile(handle, str, len, &dummy, NULL);
}

static void check_find_output(const BYTE *child_output, int child_output_len, const BYTE *out_expected, int out_expected_len, const char *file, int line)
{
    BOOL strings_are_equal;
    char *child_output_copy;
    char *out_expected_copy;
    int i, pos;

    if (child_output_len != out_expected_len)
        strings_are_equal = FALSE;
    else
    {
        strings_are_equal = memcmp(child_output, out_expected, out_expected_len) == 0;
    }

    /* Format strings for debug printing */
    child_output_copy = calloc(1, child_output_len * 4 + 1);
    out_expected_copy = calloc(1, out_expected_len * 4 + 1);

    for (i = 0, pos = 0; i < child_output_len; i++)
    {
        if (child_output[i] && child_output[i] != '\r' && child_output[i] < 128)
            child_output_copy[pos++] = child_output[i];
        else
        {
            sprintf(&child_output_copy[pos], "\\x%02x", child_output[i]);
            pos += 4;
        }
    }

    for (i = 0, pos = 0; i < out_expected_len; i++)
    {
        if (out_expected[i] && out_expected[i] != '\r' && out_expected[i] < 128)
            out_expected_copy[pos++] = out_expected[i];
        else
        {
            sprintf(&out_expected_copy[pos], "\\x%02x", out_expected[i]);
            pos += 4;
        }

    }

    ok_(file, line)(strings_are_equal, "\n#################### Expected:\n"
                                       "%s\n"
                                       "#################### But got:\n"
                                       "%s\n"
                                       "####################\n",
                                       out_expected_copy, child_output_copy);

    free(child_output_copy);
    free(out_expected_copy);
}

static void mangle_text(const BYTE *input, int input_len, BYTE *output, int output_max, int *output_len) {
    WCHAR buffer[200];
    int count_wchar;

    /* Check for UTF-16 LE BOM */
    if (input_len > 1 && input[0] == 0xFF && input[1] == 0xFE)
    {
        int buffer_count = 0;
        int i;

        /* Copy utf16le into a WCHAR array, stripping the BOM */
        for (i = 2; i < input_len; i += 2)
        {
            buffer[buffer_count++] = input[i] + (input[i + 1] << 8);
        }

        *output_len = WideCharToMultiByte(GetConsoleCP(), 0, buffer, buffer_count, (char *)output, output_max, NULL, NULL);
    }
    else
    {
        count_wchar = MultiByteToWideChar(GetConsoleCP(), 0, (char *)input, input_len, buffer, ARRAY_SIZE(buffer));
        *output_len = WideCharToMultiByte(GetConsoleCP(), 0, buffer, count_wchar, (char *)output, output_max, NULL, NULL);
    }
}

static void run_find_stdin_(const WCHAR *commandline, const BYTE *input, int input_len, const BYTE *out_expected, int out_expected_len, int exitcode_expected, const char *file, int line)
{
    HANDLE child_stdin_read;
    HANDLE child_stdout_write;
    HANDLE parent_stdin_write;
    HANDLE parent_stdout_read;
    STARTUPINFOW startup_info = {0};
    SECURITY_ATTRIBUTES security_attributes;
    PROCESS_INFORMATION process_info = {0};
    BYTE *child_output = NULL;
    int child_output_len;
    WCHAR cmd[4096];
    DWORD exitcode;

    security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    security_attributes.bInheritHandle = TRUE;
    security_attributes.lpSecurityDescriptor = NULL;

    CreatePipe(&parent_stdout_read, &child_stdout_write, &security_attributes, 0);
    CreatePipe(&child_stdin_read,   &parent_stdin_write, &security_attributes, 0);

    SetHandleInformation(parent_stdout_read, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(parent_stdin_write, HANDLE_FLAG_INHERIT, 0);

    startup_info.cb = sizeof(STARTUPINFOW);
    startup_info.hStdInput = child_stdin_read;
    startup_info.hStdOutput = child_stdout_write;
    startup_info.hStdError = NULL;
    startup_info.dwFlags |= STARTF_USESTDHANDLES;

    wsprintfW(cmd, L"find.exe %s", commandline);

    CreateProcessW(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &startup_info, &process_info);
    CloseHandle(child_stdin_read);
    CloseHandle(child_stdout_write);

    write_to_handle(parent_stdin_write, input, input_len);
    CloseHandle(parent_stdin_write);

    read_all_from_handle(parent_stdout_read, &child_output, &child_output_len);
    CloseHandle(parent_stdout_read);

    GetExitCodeProcess(process_info.hProcess, &exitcode);
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);

    check_find_output(child_output, child_output_len, out_expected, out_expected_len, file, line);

    ok_(file, line)(exitcode == exitcode_expected, "Expected exitcode %d, got %ld\n", exitcode_expected, exitcode);

    free(child_output);
}

static void run_find_file_(const WCHAR *commandline, const BYTE *input, int input_len, const BYTE *out_expected, int out_expected_len, int exitcode_expected, const char *file, int line)
{
    char path_temp_file[MAX_PATH];
    char path_temp_dir[MAX_PATH];
    HANDLE handle_file;
    WCHAR commandline_new[MAX_PATH];
    BYTE *out_expected_new;
    char header[MAX_PATH];
    int header_len;

    GetTempPathA(ARRAY_SIZE(path_temp_dir), path_temp_dir);
    GetTempFileNameA(path_temp_dir, "", 0, path_temp_file);
    handle_file = CreateFileA(path_temp_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    write_to_handle(handle_file, input, input_len);
    CloseHandle(handle_file);

    wsprintfW(commandline_new, L"%s %hs", commandline, path_temp_file);

    CharUpperA(path_temp_file);
    wsprintfA(header, "\r\n---------- %s\r\n", path_temp_file);
    header_len = lstrlenA(header);
    out_expected_new = malloc(header_len + out_expected_len);
    memcpy(out_expected_new, header, header_len);
    memcpy(out_expected_new + header_len, out_expected, out_expected_len);

    run_find_stdin_(commandline_new, (BYTE*)"", 0, out_expected_new, header_len + out_expected_len, exitcode_expected, file, line);
    free(out_expected_new);

    DeleteFileA(path_temp_file);
}

#define run_find_stdin_str(commandline, input, out_expected, exitcode_expected) \
        run_find_str_(commandline, input, out_expected, exitcode_expected, 0, __FILE__, __LINE__)

#define run_find_file_str(commandline, input, out_expected, exitcode_expected) \
        run_find_str_(commandline, input, out_expected, exitcode_expected, 1, __FILE__, __LINE__)

static void run_find_str_(const char *commandline, const char *input, const char *out_expected, int exitcode_expected, BOOL is_file, const char *file, int line)
{
    WCHAR *commandlineW;
    int len_commandlineW;

    /* Turn commandline into WCHAR string */
    len_commandlineW = MultiByteToWideChar(CP_UTF8, 0, commandline, -1, 0, 0);
    commandlineW = malloc(len_commandlineW * sizeof(WCHAR));
    MultiByteToWideChar(CP_UTF8, 0, commandline, -1, commandlineW, len_commandlineW);

    if (is_file)
        run_find_file_(commandlineW, (BYTE *)input, lstrlenA(input), (BYTE *)out_expected, lstrlenA(out_expected), exitcode_expected, file, line);
    else
        run_find_stdin_(commandlineW, (BYTE *)input, lstrlenA(input), (BYTE *)out_expected, lstrlenA(out_expected), exitcode_expected, file, line);
    free(commandlineW);
}

#define run_find_stdin_unicode(input, out_expected, exitcode_expected) \
        run_find_unicode_(input, sizeof(input), out_expected, sizeof(out_expected), exitcode_expected, 0,  __FILE__, __LINE__)

#define run_find_file_unicode(input, out_expected, exitcode_expected) \
        run_find_unicode_(input, sizeof(input), out_expected, sizeof(out_expected), exitcode_expected, 1, __FILE__, __LINE__)

static void run_find_unicode_(const BYTE *input, int input_len, const BYTE *out_expected, int out_expected_len, int exitcode_expected, BOOL is_file, const char *file, int line)
{
    /* Need "test" as char and quoted wchar */
    static const WCHAR wstr_quoted_test[] = L"\"test\"";
    static const char str_test[] = "test";

    BYTE out_expected_mangled[200] = {0};
    int out_expected_mangled_len;

    mangle_text(out_expected, out_expected_len, out_expected_mangled, ARRAY_SIZE(out_expected_mangled), &out_expected_mangled_len);

    /* Mangling can destroy the test string, so check manually if it matches */
    if (!strstr((char*)out_expected_mangled, str_test))
    {
        out_expected_mangled_len = 0;
        exitcode_expected = 1;
    }

    if (is_file)
        run_find_file_(wstr_quoted_test, input, input_len, out_expected_mangled, out_expected_mangled_len, exitcode_expected, file, line);
    else
        run_find_stdin_(wstr_quoted_test, input, input_len, out_expected_mangled, out_expected_mangled_len, exitcode_expected, file, line);
}

static void run_find_file_multi(void)
{
    char path_temp_file1[MAX_PATH];
    char path_temp_file2[MAX_PATH];
    char path_temp_file3[MAX_PATH];
    char path_temp_dir[MAX_PATH];
    HANDLE handle_file;
    WCHAR commandline_new[MAX_PATH];
    char out_expected[500];
    const char* input = "ab\nbd";

    GetTempPathA(ARRAY_SIZE(path_temp_dir), path_temp_dir);
    GetTempFileNameA(path_temp_dir, "", 0, path_temp_file1);
    GetTempFileNameA(path_temp_dir, "", 0, path_temp_file2);
    GetTempFileNameA(path_temp_dir, "", 0, path_temp_file3);
    handle_file = CreateFileA(path_temp_file1, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    write_to_handle(handle_file, (BYTE*)input, strlen(input));
    CloseHandle(handle_file);
    handle_file = CreateFileA(path_temp_file2, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    write_to_handle(handle_file, (BYTE*)input, strlen(input));
    CloseHandle(handle_file);

    wsprintfW(commandline_new, L"\"b\" C:\\doesnotexist1 %hs C:\\doesnotexist1 %hs C:\\doesnotexist1 %hs",  path_temp_file1, path_temp_file2, path_temp_file3);

    /* Keep file open during the test */
    handle_file = CreateFileA(path_temp_file3, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);

    CharUpperA(path_temp_file1);
    CharUpperA(path_temp_file2);
    CharUpperA(path_temp_file3);
    wsprintfA(out_expected,
        "File not found - C:\\DOESNOTEXIST1\r\n"
        "\r\n---------- %s\r\n"
        "ab\r\nbd\r\n"
        "File not found - C:\\DOESNOTEXIST1\r\n"
        "\r\n---------- %s\r\n"
        "ab\r\nbd\r\n"
        "File not found - C:\\DOESNOTEXIST1\r\n"
        "File not found - %s\r\n",
        path_temp_file1, path_temp_file2, path_temp_file3);

    run_find_stdin_(commandline_new, (BYTE*)"", 0, (BYTE*)out_expected, strlen(out_expected), 0, __FILE__, __LINE__);

    CloseHandle(handle_file);
    DeleteFileA(path_temp_file1);
    DeleteFileA(path_temp_file2);
    DeleteFileA(path_temp_file3);
}

static void test_errors(void)
{
    run_find_stdin_str("",       "", "FIND: Parameter format not correct\r\n", 2);
    todo_wine /* Quotes are not properly passed into wine yet */
    run_find_stdin_str("test",   "", "FIND: Parameter format not correct\r\n", 2);
    todo_wine /* Quotes are not properly passed into wine yet */
    run_find_stdin_str("\"test", "", "FIND: Parameter format not correct\r\n", 2);
    run_find_stdin_str("\"test\" /XYZ", "", "FIND: Invalid switch\r\n", 2);
    run_find_stdin_str("\"test\" C:\\doesnotexist.dat", "", "File not found - C:\\DOESNOTEXIST.DAT\r\n", 1);
}

static void test_singleline_without_switches(void)
{
    run_find_stdin_str("\"\"",      "test",  "",          1);
    run_find_stdin_str("\"test\"",  "",      "",          1);
    run_find_stdin_str("\"test\"",  "test",  "test\r\n",  0);
    run_find_stdin_str("\"test\"",  "test2", "test2\r\n", 0);
    run_find_stdin_str("\"test\"",  "test\r2", "test\r2\r\n", 0);
    run_find_stdin_str("\"test2\"", "test",  "",          1);
}

static void test_multiline(void)
{
    /* Newline in input shouldn't work */
    run_find_stdin_str("\"t1\r\nt1\"", "t1\r\nt1", "", 1);
    run_find_stdin_str("\"t1\nt1\"",   "t1\nt1",   "", 1);

    /* Newline should always be displayed as \r\n */
    run_find_stdin_str("\"test1\"", "test1\ntest2", "test1\r\n", 0);
    run_find_stdin_str("\"test1\"", "test1\r\ntest2", "test1\r\n", 0);

    /* Test with empty line */
    run_find_stdin_str("\"test1\"", "test1\n\ntest2", "test1\r\n", 0);

    /* Two strings to be found */
    run_find_stdin_str("\"test\"", "junk1\ntest1\ntest2\r\njunk", "test1\r\ntest2\r\n", 0);
}

static const BYTE str_empty[] = {};
static const BYTE str_jap_shiftjis[]        = { 0x8E,0x84,0x82,0xCD,'t','e','s','t','!','\r','\n' };
static const BYTE str_jap_utf8_bom[]        = { 0xEF,0xBB,0xBF,0xE7,0xA7,0x81,0xE3,0x81,0xAF,'j','a','p','t','e','s','t','!','\r','\n' };
static const BYTE str_jap_utf8_nobom[]      = {                0xE7,0xA7,0x81,0xE3,0x81,0xAF,'j','a','p','t','e','s','t','!','\r','\n' };
static const BYTE str_jap_utf16le_bom[]     = { 0xFF,0xFE,0xC1,0x79,0x6F,0x30,'t',0,'e',0,'s',0,'t',0,'!',0,'\r',0,'\n',0 };
static const BYTE str_jap_utf16le_nobom[]   = {           0xC1,0x79,0x6F,0x30,'t',0,'e',0,'s',0,'t',0,'!',0 };
static const BYTE str_jap_utf16be_bom[]     = { 0xFE,0xFF,0x79,0xC1,0x30,0x6F,0,'t',0,'e',0,'s',0,'t',0,'!' };
static const BYTE str_jap_utf16be_nobom[]   = {           0x79,0xC1,0x30,0x6F,0,'t',0,'e',0,'s',0,'t',0,'!' };
static const BYTE str_rus_utf8_bom[]        = { 0xEF,0xBB,0xBF,0xD0,0xBF,0xD1,0x80,0xD0,0xB8,0xD0,0xB2,0xD0,0xB5,0xD1,0x82,0x20,'t','e','s','t','!','\r','\n' };
static const BYTE str_rus_utf8_nobom[]      = {                0xD0,0xBF,0xD1,0x80,0xD0,0xB8,0xD0,0xB2,0xD0,0xB5,0xD1,0x82,0x20,'t','e','s','t','!','\r','\n' };
static const BYTE str_en_utf8_bom[]         = { 0xEF,0xBB,0xBF,'e','n','t','e','s','t','\r','\n' };
static const BYTE str_en_utf8_nobom[]       = {                'e','n','t','e','s','t','\r','\n' };

static void test_unicode_support_stdin(void)
{
    /* Test unicode support on STDIN
     * Those depend on the active codepage - e.g. 932 (japanese) behaves different from 1252 (latin)
     * All unicode tests must check for the string "test".
     */

    /* Test UTF-8 BOM */
    run_find_stdin_unicode(str_en_utf8_nobom, str_en_utf8_nobom, 0);
    run_find_stdin_unicode(str_en_utf8_bom, str_en_utf8_bom,  0);

    /* Test russian characters */
    run_find_stdin_unicode(str_rus_utf8_bom, str_rus_utf8_bom, 0);
    run_find_stdin_unicode(str_rus_utf8_nobom, str_rus_utf8_nobom, 0);

    /* Test japanese characters */
    run_find_stdin_unicode(str_jap_utf8_nobom, str_jap_utf8_nobom, 0);
    run_find_stdin_unicode(str_jap_utf8_bom, str_jap_utf8_bom, 0);
    run_find_stdin_unicode(str_jap_shiftjis, str_jap_shiftjis, 0);

    /* Test unsupported encodings */
    run_find_stdin_unicode(str_jap_utf16le_nobom, str_empty, 1);
    run_find_stdin_unicode(str_jap_utf16be_bom,   str_empty, 1);
    run_find_stdin_unicode(str_jap_utf16be_nobom, str_empty, 1);

    /* Test utf16le */
    todo_wine
    run_find_stdin_unicode(str_jap_utf16le_bom, str_jap_utf16le_bom, 0);
}

static void test_file_search(void)
{
    run_find_file_str("\"\"", "test", "", 1);
    run_find_file_str("\"test\"", "", "", 1);
    run_find_file_str("\"test\"", "test",  "test\r\n", 0);
    run_find_file_str("\"test\"", "test2", "test2\r\n", 0);
    run_find_file_str("\"test\"", "test\r2", "test\r2\r\n", 0);
    run_find_file_str("\"test2\"", "test",  "", 1);
    run_find_file_str("\"test\"", "test\nother\ntest2\ntest3", "test\r\ntest2\r\ntest3\r\n", 0);
}

static void test_unicode_support_file(void)
{
    /* Test unicode support on files */

    /* Test UTF-8 BOM */
    run_find_file_unicode(str_en_utf8_nobom, str_en_utf8_nobom, 0);
    run_find_file_unicode(str_en_utf8_bom, str_en_utf8_bom,  0);

    /* Test russian characters */
    run_find_file_unicode(str_rus_utf8_bom, str_rus_utf8_bom, 0);
    run_find_file_unicode(str_rus_utf8_nobom, str_rus_utf8_nobom, 0);

    /* Test japanese characters */
    run_find_file_unicode(str_jap_utf8_nobom, str_jap_utf8_nobom, 0);
    run_find_file_unicode(str_jap_utf8_bom, str_jap_utf8_bom, 0);
    run_find_file_unicode(str_jap_shiftjis, str_jap_shiftjis, 0);

    /* Test unsupported encodings */
    run_find_file_unicode(str_jap_utf16le_nobom, str_empty, 1);
    run_find_file_unicode(str_jap_utf16be_bom,   str_empty, 1);
    run_find_file_unicode(str_jap_utf16be_nobom, str_empty, 1);

    /* Test utf16le */
    todo_wine
    run_find_file_unicode(str_jap_utf16le_bom, str_jap_utf16le_bom, 0);
}


START_TEST(find)
{
    if (PRIMARYLANGID(GetUserDefaultUILanguage()) != LANG_ENGLISH)
    {
        skip("Error tests only work with english locale.\n");
    }
    else
    {
        test_errors();
        run_find_file_multi();
    }
    test_singleline_without_switches();
    test_multiline();
    test_unicode_support_stdin();
    test_file_search();
    test_unicode_support_file();
}
