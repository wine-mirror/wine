/* Unit test suite for FormatMessageA
 *
 * Copyright 2002 Mike McCormack for Codeweavers
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
#include "winbase.h"

/* #define ok(cond,failstr) if(!(cond)) {printf("line %d : %s\n",__LINE__,failstr);exit(1);} */

DWORD doit(DWORD flags, LPCVOID src, DWORD msg_id, DWORD lang_id,
           LPSTR out, DWORD outsize, ... )
{
    va_list list;
    DWORD r;

    va_start(list, outsize);
    r = FormatMessageA(flags, src, msg_id,
        lang_id, out, outsize, &list);
    va_end(list);
    return r;
}

void test_message_from_string(void)
{
    CHAR out[0x100] = {0};
    DWORD r;
    WCHAR szwTest[] = { 't','e','s','t',0};

    /* the basics */
    r = FormatMessageA(FORMAT_MESSAGE_FROM_STRING, "test", 0,
        0, out, sizeof out/sizeof (CHAR),NULL);
    ok(!strcmp("test", out),"failed");
    ok(r==4,"failed");

    /* using the format feature */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!s!", 0,
        0, out, sizeof out/sizeof (CHAR), "test");
    ok(!strcmp("test", out),"failed");
    ok(r==4,"failed");

    /* no format */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1", 0,
        0, out, sizeof out/sizeof (CHAR), "test");
    ok(!strcmp("test", out),"failed");
    ok(r==4,"failed");

    /* two pieces */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1%2", 0,
        0, out, sizeof out/sizeof (CHAR), "te","st");
    ok(!strcmp("test", out),"failed");
    ok(r==4,"failed");

    /* three pieces */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1%3%2%1", 0,
        0, out, sizeof out/sizeof (CHAR), "t","s","e");
    ok(!strcmp("test", out),"failed");
    ok(r==4,"failed");

    /* s doesn't seem to work in format strings */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%!s!", 0,
        0, out, sizeof out/sizeof (CHAR), "test");
    ok(!strcmp("!s!", out),"failed");
    ok(r==3,"failed");

    /* S is unicode */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!S!", 0,
        0, out, sizeof out/sizeof (CHAR), szwTest);
    ok(!strcmp("test", out),"failed");
    ok(r==4,"failed");

    /* as characters */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!c!%2!c!%3!c!%1!c!", 0,
        0, out, sizeof out/sizeof (CHAR), 't','e','s');
    ok(!strcmp("test", out),"failed");
    ok(r==4,"failed");

    /* some numbers */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!d!%2!d!%3!d!", 0,
        0, out, sizeof out/sizeof (CHAR), 1,2,3);
    ok(!strcmp("123", out),"failed");
    ok(r==3,"failed");

    /* a single digit with some spacing */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!4d!", 0,
        0, out, sizeof out/sizeof (CHAR), 1);
    ok(!strcmp("   1", out),"failed");
    ok(r==4,"failed");

    /* a single digit, left justified */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!-4d!", 0,
        0, out, sizeof out/sizeof (CHAR), 1);
    ok(!strcmp("1   ", out),"failed");
    ok(r==4,"failed");

    /* two digit decimal number */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!4d!", 0,
        0, out, sizeof out/sizeof (CHAR), 11);
    ok(!strcmp("  11", out),"failed");
    ok(r==4,"failed");

    /* a hex number */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!4x!", 0,
        0, out, sizeof out/sizeof (CHAR), 11);
    ok(!strcmp("   b", out),"failed");
    ok(r==4,"failed");

    /* a hex number, upper case */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!4X!", 0,
        0, out, sizeof out/sizeof (CHAR), 11);
    ok(!strcmp("   B", out),"failed");
    ok(r==4,"failed");

    /* a hex number, upper case, left justified */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!-4X!", 0,
        0, out, sizeof out/sizeof (CHAR), 11);
    ok(!strcmp("B   ", out),"failed");
    ok(r==4,"failed");

    /* a long hex number, upper case */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!4X!", 0,
        0, out, sizeof out/sizeof (CHAR), 0x1ab);
    ok(!strcmp(" 1AB", out),"failed");
    ok(r==4,"failed");

    /* two percent... */
    r = doit(FORMAT_MESSAGE_FROM_STRING, " %%%% ", 0,
        0, out, sizeof out/sizeof (CHAR));
    ok(!strcmp(" %% ", out),"failed");
    ok(r==4,"failed");

    /* periods are special cases */
    r = doit(FORMAT_MESSAGE_FROM_STRING, " %.%. %1!d!", 0,
        0, out, sizeof out/sizeof (CHAR), 0x1ab);
    ok(!strcmp(" .. 427", out),"failed");
    ok(r==7,"failed");

    /* %0 ends the line */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "test%0test", 0,
        0, out, sizeof out/sizeof (CHAR));
    ok(!strcmp("test", out),"failed");
    ok(r==4,"failed");

    /* %! prints an exclaimation */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "yah%!%0   ", 0,
        0, out, sizeof out/sizeof (CHAR));
    ok(!strcmp("yah!", out),"failed");
    ok(r==4,"failed");

    /* %space */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "% %   ", 0,
        0, out, sizeof out/sizeof (CHAR));
    ok(!strcmp("    ", out),"failed");
    ok(r==4,"failed");

    /* line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "hi\n", 0,
        0, out, sizeof out/sizeof (CHAR));
    ok(!strcmp("hi\r\n", out),"failed");
    ok(r==4,"failed");

    /* carriage return line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "hi\r\n", 0,
        0, out, sizeof out/sizeof (CHAR));
    ok(!strcmp("hi\r\n", out),"failed");
    ok(r==4,"failed");

    /* carriage return line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "\r", 0,
        0, out, sizeof out/sizeof (CHAR));
    ok(!strcmp("\r\n", out),"failed");
    ok(r==2,"failed");

    /* carriage return line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "\r\r\n", 0,
        0, out, sizeof out/sizeof (CHAR));
    ok(!strcmp("\r\n\r\n", out),"failed");
    ok(r==4,"failed");

    /* change of pace... test the low byte of dwflags */
    /* line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, "hi\n", 0,
        0, out, sizeof out/sizeof (CHAR));
    ok(!strcmp("hi ", out),"failed");
    ok(r==3,"failed");

    /* carriage return line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, "hi\r\n", 0,
        0, out, sizeof out/sizeof (CHAR));
    ok(!strcmp("hi ", out),"failed");
    ok(r==3,"failed");

    /* carriage return line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, "\r", 0,
        0, out, sizeof out/sizeof (CHAR));
    ok(!strcmp(" ", out),"failed");
    ok(r==1,"failed");

    /* carriage return line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, "\r\r\n", 0,
        0, out, sizeof out/sizeof (CHAR));
    ok(!strcmp("  ", out),"failed");
    ok(r==2,"failed");
}

START_TEST(format_msg)
{
    test_message_from_string();
}
