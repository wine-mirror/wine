/*
 * Unit test suite for string functions.
 *
 * Copyright 2004 Uwe Bonnes
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

#include "wine/test.h"
#include "winbase.h"
#include <string.h>
#include <mbstring.h>
#include <stdlib.h>
#include <mbctype.h>

#define expect_eq(expr, value, type, format) { type ret = (expr); ok((value) == ret, #expr " expected " format " got " format "\n", value, ret); }

static void* (*pmemcpy)(void *, const void *, size_t n);
static int* (*pmemcmp)(void *, const void *, size_t n);

#define SETNOFAIL(x,y) x = (void*)GetProcAddress(hMsvcrt,y)
#define SET(x,y) SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y)

static void test_swab( void ) {
    char original[]  = "BADCFEHGJILKNMPORQTSVUXWZY@#";
    char expected1[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ@#";
    char expected2[] = "ABCDEFGHIJKLMNOPQRSTUVWX$";
    char expected3[] = "$";
    
    char from[30];
    char to[30];
    
    int testsize;
    
    /* Test 1 - normal even case */                               
    memset(to,'$', sizeof(to));
    memset(from,'@', sizeof(from));
    testsize = 26;
    memcpy(from, original, testsize);
    _swab( from, to, testsize );
    ok(memcmp(to,expected1,testsize) == 0, "Testing even size %d returned '%*.*s'\n", testsize, testsize, testsize, to);

    /* Test 2 - uneven case  */                               
    memset(to,'$', sizeof(to));
    memset(from,'@', sizeof(from));
    testsize = 25;
    memcpy(from, original, testsize);
    _swab( from, to, testsize );
    ok(memcmp(to,expected2,testsize) == 0, "Testing odd size %d returned '%*.*s'\n", testsize, testsize, testsize, to);

    /* Test 3 - from = to */                               
    memset(to,'$', sizeof(to));
    memset(from,'@', sizeof(from));
    testsize = 26;
    memcpy(to, original, testsize);
    _swab( to, to, testsize );
    ok(memcmp(to,expected1,testsize) == 0, "Testing overlapped size %d returned '%*.*s'\n", testsize, testsize, testsize, to);

    /* Test 4 - 1 bytes */                               
    memset(to,'$', sizeof(to));
    memset(from,'@', sizeof(from));
    testsize = 1;
    memcpy(from, original, testsize);
    _swab( from, to, testsize );
    ok(memcmp(to,expected3,testsize) == 0, "Testing small size %d returned '%*.*s'\n", testsize, testsize, testsize, to);
}

#if 0      /* use this to generate more tests */

static void test_codepage(int cp)
{
    int i;
    int prev;
    int count = 1;

    ok(_setmbcp(cp) == 0, "Couldn't set mbcp\n");

    prev = _mbctype[0];
    printf("static int result_cp_%d_mbctype[] = { ", cp);
    for (i = 1; i < 257; i++)
    {
        if (_mbctype[i] != prev)
        {
            printf("0x%x,%d, ", prev, count);
            prev = _mbctype[i];
            count = 1;
        }
        else
            count++;
    }
    printf("0x%x,%d };\n", prev, count);
}

#define test_codepage_todo(cp, todo) test_codepage(cp)

#else

/* RLE-encoded mbctype tables for given codepages */
static int result_cp_1252_mbctype[] = { 0x0,66, 0x10,26, 0x0,6, 0x20,26, 0x0,8, 0x20,1,
  0x0,6, 0x10,1, 0x0,1, 0x10,1, 0x0,1, 0x10,1, 0x0,11, 0x20,1, 0x0,1, 0x20,1, 0x0,1,
  0x20,1, 0x10,1, 0x0,10, 0x20,1, 0x0,10, 0x20,1, 0x0,4, 0x20,1, 0x0,5, 0x10,23, 0x0,1,
  0x10,7, 0x20,24, 0x0,1, 32,8 };
static int result_cp_1250_mbctype[] = { 0x0,66, 0x10,26, 0x0,6, 0x20,26, 0x0,15, 0x10,1,
  0x0,1, 0x10,4, 0x0,10, 0x20,1, 0x0,1, 0x20,4, 0x0,3, 0x10,1, 0x0,1, 0x10,1, 0x0,4,
  0x10,1, 0x0,4, 0x10,1, 0x0,3, 0x20,1, 0x0,1, 0x20,1, 0x0,3, 0x20,2, 0x0,1, 0x10,1,
  0x0,1, 0x20,2, 0x10,23, 0x0,1, 0x10,7, 0x20,24, 0x0,1, 0x20,7, 0,1 };
static int result_cp_932_mbctype[] = { 0x0,65, 0x8,1, 0x18,26, 0x8,6, 0x28,26, 0x8,4,
  0x0,1, 0x8,1, 0xc,31, 0x8,1, 0xa,5, 0x9,58, 0xc,29, 0,3 };
static int result_cp_936_mbctype[] = { 0x0,65, 0x8,1, 0x18,26, 0x8,6, 0x28,26, 0x8,6,
  0xc,126, 0,1 };
static int result_cp_949_mbctype[] = { 0x0,66, 0x18,26, 0x8,6, 0x28,26, 0x8,6, 0xc,126,
  0,1 };
static int result_cp_950_mbctype[] = { 0x0,65, 0x8,1, 0x18,26, 0x8,6, 0x28,26, 0x8,4,
  0x0,2, 0x4,32, 0xc,94, 0,1 };
static int result_cp_20932_mbctype[] = { 0x0,2, 0x8,64, 0x18,26, 0x8,6, 0x28,26, 0x8,19,
  0xc,1, 0x8,18, 0xc,94, 0,1 };

static int todo_none[] = { -2 };
static int todo_cp_932[] = { 254, -2 };
static int todo_cp_20932[] = { 143, -2 };

void test_cp_table(int cp, int *result, int *todo)
{
    int i;
    int count = 0;
    int curr = 0;
    _setmbcp(cp);
    for (i = 0; i < 256; i++)
    {
        if (count == 0)
        {
            curr = result[0];
            count = result[1];
            result += 2;
        }
	if (i == *todo + 1)
	{
            todo_wine ok(_mbctype[i] == curr, "CP%d: Mismatch in ctype for character %d - %d instead of %d\n", cp, i-1, _mbctype[i], curr);
            todo++;
	}
	else
            ok(_mbctype[i] == curr, "CP%d: Mismatch in ctype for character %d - %d instead of %d\n", cp, i-1, _mbctype[i], curr);
        count--;
    }
}

#define test_codepage(num) test_cp_table(num, result_cp_##num##_mbctype, todo_none);
#define test_codepage_todo(num, todo) test_cp_table(num, result_cp_##num##_mbctype, todo);

#endif

static void test_mbcp(void)
{
    int mb_orig_max = __mb_cur_max;
    int curr_mbcp = _getmbcp();
    unsigned char *mbstring = (unsigned char *)"\xb0\xb1\xb2 \xb3\xb4 \xb5"; /* incorrect string */
    unsigned char *mbstring2 = (unsigned char *)"\xb0\xb1\xb2\xb3Q\xb4\xb5"; /* correct string */
    unsigned char *mbsonlylead = (unsigned char *)"\xb0";

    /* some two single-byte code pages*/
    test_codepage(1252);
    test_codepage(1250);
    /* double byte code pages */
    test_codepage_todo(932, todo_cp_932);
    test_codepage(936);
    test_codepage(949);
    test_codepage(950);
    test_codepage_todo(20932, todo_cp_20932);

    _setmbcp(936);
    ok(__mb_cur_max == mb_orig_max, "__mb_cur_max shouldn't be updated (is %d != %d)\n", __mb_cur_max, mb_orig_max);
    ok(_ismbblead('\354'), "\354 should be a lead byte\n");
    ok(_ismbblead(' ') == FALSE, "' ' should not be a lead byte\n");
    ok(_ismbblead(0x1234b0), "0x1234b0 should not be a lead byte\n");
    ok(_ismbblead(0x123420) == FALSE, "0x123420 should not be a lead byte\n");
    ok(_ismbbtrail('\xb0'), "\xa0 should be a trail byte\n");
    ok(_ismbbtrail(' ') == FALSE, "' ' should not be a trail byte\n");

    /* _mbsnextc */
    expect_eq(_mbsnextc(mbstring), 0xb0b1, int, "%x");
    expect_eq(_mbsnextc(&mbstring[2]), 0xb220, int, "%x");  /* lead + invalid tail */
    expect_eq(_mbsnextc(&mbstring[3]), 0x20, int, "%x");    /* single char */

    /* _mbclen/_mbslen */
    expect_eq(_mbclen(mbstring), 2, int, "%d");
    expect_eq(_mbclen(&mbstring[2]), 2, int, "%d");
    expect_eq(_mbclen(&mbstring[3]), 1, int, "%d");
    expect_eq(_mbslen(mbstring2), 4, int, "%d");
    expect_eq(_mbslen(mbsonlylead), 0, int, "%d");          /* lead + NUL not counted as character */
    expect_eq(_mbslen(mbstring), 4, int, "%d");             /* lead + invalid trail counted */


    /* functions that depend on locale codepage, not mbcp.
     * we hope the current locale to be SBCS because setlocale(LC_ALL, ".1252") seems not to work yet
     * (as of Wine 0.9.43)
     */
    if (__mb_cur_max == 1)
    {
        expect_eq(mblen((char *)mbstring, 3), 1, int, "%x");
        expect_eq(_mbstrlen((char *)mbstring2), 7, int, "%d");
    }
    else
        skip("Current locale has double-byte charset - could leave to false positives\n");

    _setmbcp(curr_mbcp);
}

static void test_mbsspn( void)
{
    unsigned char str1[]="cabernet";
    unsigned char str2[]="shiraz";
    unsigned char set[]="abc";
    unsigned char empty[]="";
    int ret;
    ret=_mbsspn( str1, set);
    ok( ret==3, "_mbsspn returns %d should be 3\n", ret);
    ret=_mbsspn( str2, set);
    ok( ret==0, "_mbsspn returns %d should be 0\n", ret);
    ret=_mbsspn( str1, empty);
    ok( ret==0, "_mbsspn returns %d should be 0\n", ret);
}

static void test_mbsspnp( void)
{
    unsigned char str1[]="cabernet";
    unsigned char str2[]="shiraz";
    unsigned char set[]="abc";
    unsigned char empty[]="";
    unsigned char full[]="abcenrt";
    unsigned char* ret;
    ret=_mbsspnp( str1, set);
    ok( ret[0]=='e', "_mbsspnp returns %c should be e\n", ret[0]);
    ret=_mbsspnp( str2, set);
    ok( ret[0]=='s', "_mbsspnp returns %c should be s\n", ret[0]);
    ret=_mbsspnp( str1, empty);
    ok( ret[0]=='c', "_mbsspnp returns %c should be c\n", ret[0]);
    ret=_mbsspnp( str1, full);
    ok( ret==NULL, "_mbsspnp returns %p should be NULL\n", ret);
}

static void test_strdup(void)
{
   char *str;
   str = _strdup( 0 );
   ok( str == 0, "strdup returns %s should be 0\n", str);
   free( str );
}

START_TEST(string)
{
    void *mem;
    static const char xilstring[]="c:/xilinx";
    int nLen;
    HMODULE hMsvcrt;

    hMsvcrt = GetModuleHandleA("msvcrt.dll");
    if (!hMsvcrt)
        hMsvcrt = GetModuleHandleA("msvcrtd.dll");
    ok(hMsvcrt != 0, "GetModuleHandleA failed\n");
    SET(pmemcpy,"memcpy");
    SET(pmemcmp,"memcmp");

    /* MSVCRT memcpy behaves like memmove for overlapping moves,
       MFC42 CString::Insert seems to rely on that behaviour */
    mem = malloc(100);
    ok(mem != NULL, "memory not allocated for size 0\n");
    strcpy((char*)mem,xilstring);
    nLen=strlen(xilstring);
    pmemcpy((char*)mem+5, mem,nLen+1);
    ok(pmemcmp((char*)mem+5,xilstring, nLen) == 0, 
       "Got result %s\n",(char*)mem+5);

    /* Test _swab function */
    test_swab();

    /* Test ismbblead*/
    test_mbcp();
   /* test _mbsspn */
    test_mbsspn();
    test_mbsspnp();
   /* test _strdup */
    test_strdup();
}
