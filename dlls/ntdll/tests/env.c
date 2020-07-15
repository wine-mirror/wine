/*
 * Unit test suite for ntdll env functions
 *
 * Copyright 2003 Eric Pouech
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

#include <stdio.h>

#include "ntdll_test.h"

static NTSTATUS (WINAPI *pRtlMultiByteToUnicodeN)( LPWSTR dst, DWORD dstlen, LPDWORD reslen,
                                                   LPCSTR src, DWORD srclen );
static NTSTATUS (WINAPI *pRtlCreateEnvironment)(BOOLEAN, PWSTR*);
static NTSTATUS (WINAPI *pRtlDestroyEnvironment)(PWSTR);
static NTSTATUS (WINAPI *pRtlQueryEnvironmentVariable_U)(PWSTR, PUNICODE_STRING, PUNICODE_STRING);
static NTSTATUS (WINAPI* pRtlQueryEnvironmentVariable)(WCHAR*, WCHAR*, SIZE_T, WCHAR*, SIZE_T, SIZE_T*);
static void     (WINAPI *pRtlSetCurrentEnvironment)(PWSTR, PWSTR*);
static NTSTATUS (WINAPI *pRtlSetEnvironmentVariable)(PWSTR*, PUNICODE_STRING, PUNICODE_STRING);
static NTSTATUS (WINAPI *pRtlExpandEnvironmentStrings_U)(LPWSTR, PUNICODE_STRING, PUNICODE_STRING, PULONG);
static NTSTATUS (WINAPI *pRtlCreateProcessParameters)(RTL_USER_PROCESS_PARAMETERS**,
                                                      const UNICODE_STRING*, const UNICODE_STRING*,
                                                      const UNICODE_STRING*, const UNICODE_STRING*,
                                                      PWSTR, const UNICODE_STRING*, const UNICODE_STRING*,
                                                      const UNICODE_STRING*, const UNICODE_STRING*);
static void (WINAPI *pRtlDestroyProcessParameters)(RTL_USER_PROCESS_PARAMETERS *);

static void *initial_env;

static WCHAR  small_env[] = {'f','o','o','=','t','o','t','o',0,
                             'f','o','=','t','i','t','i',0,
                             'f','o','o','o','=','t','u','t','u',0,
                             's','r','=','a','n','=','o','u','o',0,
                             'g','=','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
                                     'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
                                     'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
                                     'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
                                     'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
                                     'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
                                     'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
                                     'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
                                     'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
                                     'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',0,
			     '=','o','O','H','=','I','I','I',0,
                             'n','u','l','=',0,
                             0};

static void testQuery(void)
{
    struct test
    {
        const char *var;
        int len;
        NTSTATUS status;
        const char *val;
        NTSTATUS alt;
    };

    static const struct test tests[] =
    {
        {"foo", 256, STATUS_SUCCESS, "toto"},
        {"FoO", 256, STATUS_SUCCESS, "toto"},
        {"foo=", 256, STATUS_VARIABLE_NOT_FOUND, NULL},
        {"foo ", 256, STATUS_VARIABLE_NOT_FOUND, NULL},
        {"foo", 1, STATUS_BUFFER_TOO_SMALL, "toto"},
        {"foo", 3, STATUS_BUFFER_TOO_SMALL, "toto"},
        {"foo", 4, STATUS_SUCCESS, "toto", STATUS_BUFFER_TOO_SMALL},
        {"foo", 5, STATUS_SUCCESS, "toto"},
        {"fooo", 256, STATUS_SUCCESS, "tutu"},
        {"f", 256, STATUS_VARIABLE_NOT_FOUND, NULL},
        {"g", 256, STATUS_SUCCESS, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},
        {"sr=an", 256, STATUS_SUCCESS, "ouo", STATUS_VARIABLE_NOT_FOUND},
        {"sr", 256, STATUS_SUCCESS, "an=ouo"},
	{"=oOH", 256, STATUS_SUCCESS, "III"},
        {"", 256, STATUS_VARIABLE_NOT_FOUND, NULL},
        {"nul", 256, STATUS_SUCCESS, ""},
        {NULL, 0, 0, NULL}
    };

    WCHAR               bn[257];
    WCHAR               bv[257];
    UNICODE_STRING      name;
    UNICODE_STRING      value;
    NTSTATUS            nts;
    SIZE_T              name_length;
    SIZE_T              value_length;
    SIZE_T              return_length;
    unsigned int i;

    for (i = 0; tests[i].var; i++)
    {
        const struct test *test = &tests[i];
        name.Length = strlen(test->var) * 2;
        name.MaximumLength = name.Length + 2;
        name.Buffer = bn;
        value.Length = 0;
        value.MaximumLength = test->len * 2;
        value.Buffer = bv;
        bv[test->len] = '@';

        pRtlMultiByteToUnicodeN( bn, sizeof(bn), NULL, test->var, strlen(test->var)+1 );
        nts = pRtlQueryEnvironmentVariable_U(small_env, &name, &value);
        ok( nts == test->status || (test->alt && nts == test->alt),
            "[%d]: Wrong status for '%s', expecting %x got %x\n",
            i, test->var, test->status, nts );
        if (nts == test->status) switch (nts)
        {
        case STATUS_SUCCESS:
            pRtlMultiByteToUnicodeN( bn, sizeof(bn), NULL, test->val, strlen(test->val)+1 );
            ok( value.Length == strlen(test->val) * sizeof(WCHAR), "Wrong length %d for %s\n",
                value.Length, test->var );
            ok((value.Length == strlen(test->val) * sizeof(WCHAR) && memcmp(bv, bn, value.Length) == 0) ||
	       lstrcmpW(bv, bn) == 0, 
	       "Wrong result for %s/%d\n", test->var, test->len);
            ok(bv[test->len] == '@', "Writing too far away in the buffer for %s/%d\n", test->var, test->len);
            break;
        case STATUS_BUFFER_TOO_SMALL:
            ok( value.Length == strlen(test->val) * sizeof(WCHAR), 
                "Wrong returned length %d (too small buffer) for %s\n", value.Length, test->var );
            break;
        }
    }

    if (pRtlQueryEnvironmentVariable)
    {
        for (i = 0; tests[i].var; i++)
        {
            const struct test* test = &tests[i];
            name_length = strlen(test->var);
            value_length = test->len;
            value.Buffer = bv;
            bv[test->len] = '@';

            pRtlMultiByteToUnicodeN(bn, sizeof(bn), NULL, test->var, strlen(test->var) + 1);
            nts = pRtlQueryEnvironmentVariable(small_env, bn, name_length, bv, value_length, &return_length);
            ok(nts == test->status || (test->alt && nts == test->alt),
                "[%d]: Wrong status for '%s', expecting %x got %x\n",
                i, test->var, test->status, nts);
            if (nts == test->status) switch (nts)
            {
            case STATUS_SUCCESS:
                pRtlMultiByteToUnicodeN(bn, sizeof(bn), NULL, test->val, strlen(test->val) + 1);
                ok(return_length == strlen(test->val), "Wrong length %ld for %s\n",
                    return_length, test->var);
                ok(!memcmp(bv, bn, return_length), "Wrong result for %s/%d\n", test->var, test->len);
                ok(bv[test->len] == '@', "Writing too far away in the buffer for %s/%d\n", test->var, test->len);
                break;
            case STATUS_BUFFER_TOO_SMALL:
                ok(return_length == (strlen(test->val) + 1),
                    "Wrong returned length %ld (too small buffer) for %s\n", return_length, test->var);
                break;
            }
        }
    }
    else win_skip("RtlQueryEnvironmentVariable not available, skipping tests\n");
}

static void testSetHelper(LPWSTR* env, const char* var, const char* val, NTSTATUS ret, NTSTATUS alt)
{
    WCHAR               bvar[256], bval1[256], bval2[256];
    UNICODE_STRING      uvar;
    UNICODE_STRING      uval;
    NTSTATUS            nts;

    uvar.Length = strlen(var) * sizeof(WCHAR);
    uvar.MaximumLength = uvar.Length + sizeof(WCHAR);
    uvar.Buffer = bvar;
    pRtlMultiByteToUnicodeN( bvar, sizeof(bvar), NULL, var, strlen(var)+1 );
    if (val)
    {
        uval.Length = strlen(val) * sizeof(WCHAR);
        uval.MaximumLength = uval.Length + sizeof(WCHAR);
        uval.Buffer = bval1;
        pRtlMultiByteToUnicodeN( bval1, sizeof(bval1), NULL, val, strlen(val)+1 );
    }
    nts = pRtlSetEnvironmentVariable(env, &uvar, val ? &uval : NULL);
    ok(nts == ret || (alt && nts == alt), "Setting var %s=%s (%x/%x)\n", var, val, nts, ret);
    if (nts == STATUS_SUCCESS)
    {
        uval.Length = 0;
        uval.MaximumLength = sizeof(bval2);
        uval.Buffer = bval2;
        nts = pRtlQueryEnvironmentVariable_U(*env, &uvar, &uval);
        switch (nts)
        {
        case STATUS_SUCCESS:
            ok(lstrcmpW(bval1, bval2) == 0, "Cannot get value written to environment\n");
            break;
        case STATUS_VARIABLE_NOT_FOUND:
            ok(val == NULL ||
               broken(strchr(var,'=') != NULL), /* variable containing '=' may be set but not found again on NT4 */
               "Couldn't find variable, but didn't delete it. val = %s\n", val);
            break;
        default:
            ok(0, "Wrong ret %u for %s\n", nts, var);
            break;
        }
    }
}

static void testSet(void)
{
    LPWSTR              env;
    char                tmp[16];
    int                 i;

    ok(pRtlCreateEnvironment(FALSE, &env) == STATUS_SUCCESS, "Creating environment\n");

    testSetHelper(&env, "cat", "dog", STATUS_SUCCESS, 0);
    testSetHelper(&env, "cat", "horse", STATUS_SUCCESS, 0);
    testSetHelper(&env, "cat", "zz", STATUS_SUCCESS, 0);
    testSetHelper(&env, "cat", NULL, STATUS_SUCCESS, 0);
    testSetHelper(&env, "cat", NULL, STATUS_SUCCESS, STATUS_VARIABLE_NOT_FOUND);
    testSetHelper(&env, "foo", "meouw", STATUS_SUCCESS, 0);
    testSetHelper(&env, "me=too", "also", STATUS_SUCCESS, STATUS_INVALID_PARAMETER);
    testSetHelper(&env, "me", "too=also", STATUS_SUCCESS, 0);
    testSetHelper(&env, "=too", "also", STATUS_SUCCESS, 0);
    testSetHelper(&env, "=", "also", STATUS_SUCCESS, 0);

    for (i = 0; i < 128; i++)
    {
        sprintf(tmp, "zork%03d", i);
        testSetHelper(&env, tmp, "is alive", STATUS_SUCCESS, 0);
    }

    for (i = 0; i < 128; i++)
    {
        sprintf(tmp, "zork%03d", i);
        testSetHelper(&env, tmp, NULL, STATUS_SUCCESS, 0);
    }
    testSetHelper(&env, "fOo", NULL, STATUS_SUCCESS, 0);

    ok(pRtlDestroyEnvironment(env) == STATUS_SUCCESS, "Destroying environment\n");
}

static void testExpand(void)
{
    static const struct test
    {
        const char *src;
        const char *dst;
    } tests[] =
    {
        {"hello%foo%world",             "hellototoworld"},
        {"hello%=oOH%world",            "helloIIIworld"},
        {"hello%foo",                   "hello%foo"},
        {"hello%bar%world",             "hello%bar%world"},
        /*
         * {"hello%foo%world%=oOH%eeck",   "hellototoworldIIIeeck"},
         * Interestingly enough, with a 8 WCHAR buffers, we get on 2k:
         *      helloIII
         * so it seems like strings overflowing the buffer are written 
         * (truncated) but the write cursor is not advanced :-/
         */
        {NULL, NULL}
    };

    const struct test*  test;
    NTSTATUS            nts;
    UNICODE_STRING      us_src, us_dst;
    WCHAR               src[256], dst[256], rst[256];
    ULONG               ul;

    for (test = tests; test->src; test++)
    {
        pRtlMultiByteToUnicodeN(src, sizeof(src), NULL, test->src, strlen(test->src)+1);
        pRtlMultiByteToUnicodeN(rst, sizeof(rst), NULL, test->dst, strlen(test->dst)+1);

        us_src.Length = strlen(test->src) * sizeof(WCHAR);
        us_src.MaximumLength = us_src.Length + 2;
        us_src.Buffer = src;

        us_dst.Length = 0;
        us_dst.MaximumLength = 0;
        us_dst.Buffer = NULL;

        nts = pRtlExpandEnvironmentStrings_U(small_env, &us_src, &us_dst, &ul);
        ok(nts == STATUS_BUFFER_TOO_SMALL, "Call failed (%u)\n", nts);
        ok(ul == strlen(test->dst) * sizeof(WCHAR) + sizeof(WCHAR), 
           "Wrong  returned length for %s: %u\n", test->src, ul );

        us_dst.Length = 0;
        us_dst.MaximumLength = sizeof(dst);
        us_dst.Buffer = dst;

        nts = pRtlExpandEnvironmentStrings_U(small_env, &us_src, &us_dst, &ul);
        ok(nts == STATUS_SUCCESS, "Call failed (%u)\n", nts);
        ok(ul == us_dst.Length + sizeof(WCHAR), 
           "Wrong returned length for %s: %u\n", test->src, ul);
        ok(ul == strlen(test->dst) * sizeof(WCHAR) + sizeof(WCHAR), 
           "Wrong  returned length for %s: %u\n", test->src, ul);
        ok(lstrcmpW(dst, rst) == 0, "Wrong result for %s: expecting %s\n",
           test->src, test->dst);

        us_dst.Length = 0;
        us_dst.MaximumLength = 8 * sizeof(WCHAR);
        us_dst.Buffer = dst;
        dst[8] = '-';
        nts = pRtlExpandEnvironmentStrings_U(small_env, &us_src, &us_dst, &ul);
        ok(nts == STATUS_BUFFER_TOO_SMALL, "Call failed (%u)\n", nts);
        ok(ul == strlen(test->dst) * sizeof(WCHAR) + sizeof(WCHAR), 
           "Wrong  returned length for %s (with buffer too small): %u\n", test->src, ul);
        ok(dst[8] == '-', "Writing too far in buffer (got %c/%d)\n", dst[8], dst[8]);
    }

}

static WCHAR *get_params_string( RTL_USER_PROCESS_PARAMETERS *params, UNICODE_STRING *str )
{
    if (params->Flags & PROCESS_PARAMS_FLAG_NORMALIZED) return str->Buffer;
    return (WCHAR *)((char *)params + (UINT_PTR)str->Buffer);
}

static UINT_PTR check_string_( int line, RTL_USER_PROCESS_PARAMETERS *params, UNICODE_STRING *str,
                               const UNICODE_STRING *expect, UINT_PTR pos )
{
    if (expect)
    {
        ok_(__FILE__,line)( str->Length == expect->Length, "wrong length %u/%u\n",
                            str->Length, expect->Length );
        ok_(__FILE__,line)( str->MaximumLength == expect->MaximumLength ||
                            broken( str->MaximumLength == 1 && expect->MaximumLength == 2 ), /* winxp */
                            "wrong maxlength %u/%u\n", str->MaximumLength, expect->MaximumLength );
    }
    if (!str->MaximumLength)
    {
        ok_(__FILE__,line)( str->Buffer == NULL, "buffer not null %p\n", str->Buffer );
        return pos;
    }
    ok_(__FILE__,line)( (UINT_PTR)str->Buffer == ((pos + sizeof(void*) - 1) & ~(sizeof(void *) - 1)) ||
                        (!expect && (UINT_PTR)str->Buffer == pos) ||  /* initial params are not aligned */
                        broken( (UINT_PTR)str->Buffer == ((pos + 3) & ~3) ), "wrong buffer %lx/%lx\n",
                        (UINT_PTR)str->Buffer, pos );
    if (str->Length < str->MaximumLength)
    {
        WCHAR *ptr = get_params_string( params, str );
        ok_(__FILE__,line)( !ptr[str->Length / sizeof(WCHAR)], "string not null-terminated %s\n",
                            wine_dbgstr_wn( ptr, str->MaximumLength / sizeof(WCHAR) ));
    }
    return (UINT_PTR)str->Buffer + str->MaximumLength;
}
#define check_string(params,str,expect,pos) check_string_(__LINE__,params,str,expect,pos)

static void test_process_params(void)
{
    static WCHAR empty[] = {0};
    static const UNICODE_STRING empty_str = { 0, sizeof(empty), empty };
    static const UNICODE_STRING null_str = { 0, 0, NULL };
    static WCHAR exeW[] = {'c',':','\\','f','o','o','.','e','x','e',0};
    static WCHAR dummyW[] = {'d','u','m','m','y','1',0};
    static WCHAR dummy_dirW[MAX_PATH] = {'d','u','m','m','y','2',0};
    static WCHAR dummy_env[] = {'a','=','b',0,'c','=','d',0,0};
    UNICODE_STRING image = { sizeof(exeW) - sizeof(WCHAR), sizeof(exeW), exeW };
    UNICODE_STRING dummy = { sizeof(dummyW) - sizeof(WCHAR), sizeof(dummyW), dummyW };
    UNICODE_STRING dummy_dir = { 6*sizeof(WCHAR), sizeof(dummy_dirW), dummy_dirW };
    RTL_USER_PROCESS_PARAMETERS *params = NULL;
    RTL_USER_PROCESS_PARAMETERS *cur_params = NtCurrentTeb()->Peb->ProcessParameters;
    SIZE_T size;
    WCHAR *str;
    UINT_PTR pos;
    MEMORY_BASIC_INFORMATION info;
    NTSTATUS status = pRtlCreateProcessParameters( &params, &image, NULL, NULL, NULL, NULL,
                                                   NULL, NULL, NULL, NULL );
    ok( !status, "failed %x\n", status );
    if (VirtualQuery( params, &info, sizeof(info) ) && info.AllocationBase == params)
    {
        size = info.RegionSize;
        ok( broken(TRUE), "not a heap block %p\n", params );  /* winxp */
        ok( params->AllocationSize == info.RegionSize,
            "wrong AllocationSize %x/%lx\n", params->AllocationSize, info.RegionSize );
    }
    else
    {
        size = HeapSize( GetProcessHeap(), 0, params );
        ok( size != ~(SIZE_T)0, "not a heap block %p\n", params );
        ok( params->AllocationSize == params->Size,
            "wrong AllocationSize %x/%x\n", params->AllocationSize, params->Size );
    }
    ok( params->Size < size || broken(params->Size == size), /* <= win2k3 */
        "wrong Size %x/%lx\n", params->Size, size );
    ok( params->Flags == 0, "wrong Flags %u\n", params->Flags );
    ok( params->DebugFlags == 0, "wrong Flags %u\n", params->DebugFlags );
    ok( params->ConsoleHandle == 0, "wrong ConsoleHandle %p\n", params->ConsoleHandle );
    ok( params->ConsoleFlags == 0, "wrong ConsoleFlags %u\n", params->ConsoleFlags );
    ok( params->hStdInput == 0, "wrong hStdInput %p\n", params->hStdInput );
    ok( params->hStdOutput == 0, "wrong hStdOutput %p\n", params->hStdOutput );
    ok( params->hStdError == 0, "wrong hStdError %p\n", params->hStdError );
    ok( params->dwX == 0, "wrong dwX %u\n", params->dwX );
    ok( params->dwY == 0, "wrong dwY %u\n", params->dwY );
    ok( params->dwXSize == 0, "wrong dwXSize %u\n", params->dwXSize );
    ok( params->dwYSize == 0, "wrong dwYSize %u\n", params->dwYSize );
    ok( params->dwXCountChars == 0, "wrong dwXCountChars %u\n", params->dwXCountChars );
    ok( params->dwYCountChars == 0, "wrong dwYCountChars %u\n", params->dwYCountChars );
    ok( params->dwFillAttribute == 0, "wrong dwFillAttribute %u\n", params->dwFillAttribute );
    ok( params->dwFlags == 0, "wrong dwFlags %u\n", params->dwFlags );
    ok( params->wShowWindow == 0, "wrong wShowWindow %u\n", params->wShowWindow );
    pos = (UINT_PTR)params->CurrentDirectory.DosPath.Buffer;

    ok( params->CurrentDirectory.DosPath.MaximumLength == MAX_PATH * sizeof(WCHAR),
        "wrong length %x\n", params->CurrentDirectory.DosPath.MaximumLength );
    pos = check_string( params, &params->CurrentDirectory.DosPath,
                        &cur_params->CurrentDirectory.DosPath, pos );
    if (params->DllPath.MaximumLength)
        pos = check_string( params, &params->DllPath, &cur_params->DllPath, pos );
    else
        pos = check_string( params, &params->DllPath, &null_str, pos );
    pos = check_string( params, &params->ImagePathName, &image, pos );
    pos = check_string( params, &params->CommandLine, &image, pos );
    pos = check_string( params, &params->WindowTitle, &empty_str, pos );
    pos = check_string( params, &params->Desktop, &empty_str, pos );
    pos = check_string( params, &params->ShellInfo, &empty_str, pos );
    pos = check_string( params, &params->RuntimeInfo, &null_str, pos );
    pos = (pos + 3) & ~3;
    ok( pos == params->Size || pos + 4 == params->Size,
        "wrong pos %lx/%x\n", pos, params->Size );
    pos = params->Size;
    if ((char *)params->Environment > (char *)params &&
        (char *)params->Environment < (char *)params + size)
    {
        ok( (char *)params->Environment - (char *)params == (UINT_PTR)pos,
            "wrong env %lx/%lx\n", (UINT_PTR)((char *)params->Environment - (char *)params), pos);
        str = params->Environment;
        while (*str) str += lstrlenW(str) + 1;
        str++;
        pos += (str - params->Environment) * sizeof(WCHAR);
        ok( ((pos + sizeof(void *) - 1) & ~(sizeof(void *) - 1)) == size ||
            broken( ((pos + 3) & ~3) == size ), "wrong size %lx/%lx\n", pos, size );
    }
    else ok( broken(TRUE), "environment not inside block\n" ); /* <= win2k3 */
    pRtlDestroyProcessParameters( params );

    status = pRtlCreateProcessParameters( &params, &image, &dummy, &dummy, &dummy, dummy_env,
                                          &dummy, &dummy, &dummy, &dummy );
    ok( !status, "failed %x\n", status );
    if (VirtualQuery( params, &info, sizeof(info) ) && info.AllocationBase == params)
    {
        size = info.RegionSize;
        ok( broken(TRUE), "not a heap block %p\n", params );  /* winxp */
        ok( params->AllocationSize == info.RegionSize,
            "wrong AllocationSize %x/%lx\n", params->AllocationSize, info.RegionSize );
    }
    else
    {
        size = HeapSize( GetProcessHeap(), 0, params );
        ok( size != ~(SIZE_T)0, "not a heap block %p\n", params );
        ok( params->AllocationSize == params->Size,
            "wrong AllocationSize %x/%x\n", params->AllocationSize, params->Size );
    }
    ok( params->Size < size || broken(params->Size == size), /* <= win2k3 */
        "wrong Size %x/%lx\n", params->Size, size );
    pos = (UINT_PTR)params->CurrentDirectory.DosPath.Buffer;

    if (params->CurrentDirectory.DosPath.Length == dummy_dir.Length + sizeof(WCHAR))
    {
        /* win10 appends a backslash */
        dummy_dirW[dummy_dir.Length / sizeof(WCHAR)] = '\\';
        dummy_dir.Length += sizeof(WCHAR);
    }
    pos = check_string( params, &params->CurrentDirectory.DosPath, &dummy_dir, pos );
    pos = check_string( params, &params->DllPath, &dummy, pos );
    pos = check_string( params, &params->ImagePathName, &image, pos );
    pos = check_string( params, &params->CommandLine, &dummy, pos );
    pos = check_string( params, &params->WindowTitle, &dummy, pos );
    pos = check_string( params, &params->Desktop, &dummy, pos );
    pos = check_string( params, &params->ShellInfo, &dummy, pos );
    pos = check_string( params, &params->RuntimeInfo, &dummy, pos );
    pos = (pos + 3) & ~3;
    ok( pos == params->Size || pos + 4 == params->Size,
        "wrong pos %lx/%x\n", pos, params->Size );
    pos = params->Size;
    if ((char *)params->Environment > (char *)params &&
        (char *)params->Environment < (char *)params + size)
    {
        ok( (char *)params->Environment - (char *)params == pos,
            "wrong env %lx/%lx\n", (UINT_PTR)((char *)params->Environment - (char *)params), pos);
        str = params->Environment;
        while (*str) str += lstrlenW(str) + 1;
        str++;
        pos += (str - params->Environment) * sizeof(WCHAR);
        ok( ((pos + sizeof(void *) - 1) & ~(sizeof(void *) - 1)) == size ||
            broken( ((pos + 3) & ~3) == size ), "wrong size %lx/%lx\n", pos, size );
    }
    else ok( broken(TRUE), "environment not inside block\n" );  /* <= win2k3 */
    pRtlDestroyProcessParameters( params );

    /* also test the actual parameters of the current process */

    ok( cur_params->Flags & PROCESS_PARAMS_FLAG_NORMALIZED, "current params not normalized\n" );
    if (VirtualQuery( cur_params, &info, sizeof(info) ) && info.AllocationBase == cur_params)
    {
        ok( broken(TRUE), "not a heap block %p\n", cur_params );  /* winxp */
        ok( cur_params->AllocationSize == info.RegionSize,
            "wrong AllocationSize %x/%lx\n", cur_params->AllocationSize, info.RegionSize );
    }
    else
    {
        size = HeapSize( GetProcessHeap(), 0, cur_params );
        ok( size != ~(SIZE_T)0, "not a heap block %p\n", cur_params );
        ok( cur_params->AllocationSize == cur_params->Size,
            "wrong AllocationSize %x/%x\n", cur_params->AllocationSize, cur_params->Size );
        ok( cur_params->Size == size, "wrong Size %x/%lx\n", cur_params->Size, size );
    }

    /* CurrentDirectory points outside the params, and DllPath may be null */
    pos = (UINT_PTR)cur_params->DllPath.Buffer;
    if (!pos) pos = (UINT_PTR)cur_params->ImagePathName.Buffer;
    pos = check_string( cur_params, &cur_params->DllPath, NULL, pos );
    pos = check_string( cur_params, &cur_params->ImagePathName, NULL, pos );
    pos = check_string( cur_params, &cur_params->CommandLine, NULL, pos );
    pos = check_string( cur_params, &cur_params->WindowTitle, NULL, pos );
    pos = check_string( cur_params, &cur_params->Desktop, NULL, pos );
    pos = check_string( cur_params, &cur_params->ShellInfo, NULL, pos );
    pos = check_string( cur_params, &cur_params->RuntimeInfo, NULL, pos );
    /* environment may follow */
    str = (WCHAR *)pos;
    if (pos - (UINT_PTR)cur_params < cur_params->Size)
    {
        while (*str) str += lstrlenW(str) + 1;
        str++;
    }
    ok( (char *)str == (char *)cur_params + cur_params->Size,
        "wrong end ptr %p/%p\n", str, (char *)cur_params + cur_params->Size );

    /* initial environment is a separate block */

    ok( (char *)initial_env < (char *)cur_params || (char *)initial_env >= (char *)cur_params + size,
        "initial environment inside block %p / %p\n", cur_params, initial_env );

    if (VirtualQuery( initial_env, &info, sizeof(info) ) && info.AllocationBase == initial_env)
    {
        todo_wine
        ok( broken(TRUE), "env not a heap block %p / %p\n", cur_params, initial_env );  /* winxp */
    }
    else
    {
        size = HeapSize( GetProcessHeap(), 0, initial_env );
        ok( size != ~(SIZE_T)0, "env is not a heap block %p / %p\n", cur_params, initial_env );
    }
}

static NTSTATUS set_env_var(WCHAR **env, const WCHAR *var, const WCHAR *value)
{
    UNICODE_STRING var_string, value_string;
    RtlInitUnicodeString(&var_string, var);
    RtlInitUnicodeString(&value_string, value);
    return RtlSetEnvironmentVariable(env, &var_string, &value_string);
}

static void check_env_var_(int line, const char *var, const char *value)
{
    char buffer[20];
    DWORD size = GetEnvironmentVariableA(var, buffer, sizeof(buffer));
    if (value)
    {
        ok_(__FILE__, line)(size == strlen(value), "wrong size %u\n", size);
        ok_(__FILE__, line)(!strcmp(buffer, value), "wrong value %s\n", debugstr_a(buffer));
    }
    else
    {
        ok_(__FILE__, line)(!size, "wrong size %u\n", size);
        ok_(__FILE__, line)(GetLastError() == ERROR_ENVVAR_NOT_FOUND, "got error %u\n", GetLastError());
    }
}
#define check_env_var(a, b) check_env_var_(__LINE__, a, b)

static void test_RtlSetCurrentEnvironment(void)
{
    NTSTATUS status;
    WCHAR *old_env, *env, *prev;
    BOOL ret;

    status = RtlCreateEnvironment(FALSE, &env);
    ok(!status, "got %#x\n", status);

    ret = SetEnvironmentVariableA("testenv1", "heis");
    ok(ret, "got error %u\n", GetLastError());
    ret = SetEnvironmentVariableA("testenv2", "dyo");
    ok(ret, "got error %u\n", GetLastError());

    status = set_env_var(&env, L"testenv1", L"unus");
    ok(!status, "got %#x\n", status);
    status = set_env_var(&env, L"testenv3", L"tres");
    ok(!status, "got %#x\n", status);

    old_env = NtCurrentTeb()->Peb->ProcessParameters->Environment;

    RtlSetCurrentEnvironment(env, &prev);
    ok(prev == old_env, "got wrong previous env %p\n", prev);
    ok(NtCurrentTeb()->Peb->ProcessParameters->Environment == env, "got wrong current env\n");

    check_env_var("testenv1", "unus");
    check_env_var("testenv2", NULL);
    check_env_var("testenv3", "tres");
    check_env_var("PATH", NULL);

    RtlSetCurrentEnvironment(old_env, NULL);
    ok(NtCurrentTeb()->Peb->ProcessParameters->Environment == old_env, "got wrong current env\n");

    check_env_var("testenv1", "heis");
    check_env_var("testenv2", "dyo");
    check_env_var("testenv3", NULL);

    SetEnvironmentVariableA("testenv1", NULL);
    SetEnvironmentVariableA("testenv2", NULL);
}

START_TEST(env)
{
    HMODULE mod = GetModuleHandleA("ntdll.dll");

    initial_env = NtCurrentTeb()->Peb->ProcessParameters->Environment;

    pRtlMultiByteToUnicodeN = (void *)GetProcAddress(mod,"RtlMultiByteToUnicodeN");
    pRtlCreateEnvironment = (void*)GetProcAddress(mod, "RtlCreateEnvironment");
    pRtlDestroyEnvironment = (void*)GetProcAddress(mod, "RtlDestroyEnvironment");
    pRtlQueryEnvironmentVariable_U = (void*)GetProcAddress(mod, "RtlQueryEnvironmentVariable_U");
    pRtlQueryEnvironmentVariable = (void*)GetProcAddress(mod, "RtlQueryEnvironmentVariable");
    pRtlSetCurrentEnvironment = (void*)GetProcAddress(mod, "RtlSetCurrentEnvironment");
    pRtlSetEnvironmentVariable = (void*)GetProcAddress(mod, "RtlSetEnvironmentVariable");
    pRtlExpandEnvironmentStrings_U = (void*)GetProcAddress(mod, "RtlExpandEnvironmentStrings_U");
    pRtlCreateProcessParameters = (void*)GetProcAddress(mod, "RtlCreateProcessParameters");
    pRtlDestroyProcessParameters = (void*)GetProcAddress(mod, "RtlDestroyProcessParameters");

    testQuery();
    testSet();
    testExpand();
    test_process_params();
    test_RtlSetCurrentEnvironment();
}
