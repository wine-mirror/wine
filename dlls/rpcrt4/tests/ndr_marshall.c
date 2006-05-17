/*
 * Unit test suite for ndr marshalling functions
 *
 * Copyright 2006 Huw Davies
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

#include "wine/test.h"
#include <windef.h>
#include <winbase.h>
#include <winnt.h>
#include <winerror.h>

#include "wine/unicode.h"
#include "rpc.h"
#include "rpcdce.h"
#include "rpcproxy.h"


static int my_alloc_called;
static int my_free_called;
void * CALLBACK my_alloc(size_t size)
{
    my_alloc_called++;
    return NdrOleAllocate(size);
}

void CALLBACK my_free(void *ptr)
{
    my_free_called++;
    return NdrOleFree(ptr);
}

static const MIDL_STUB_DESC Object_StubDesc = 
    {
    NULL,
    my_alloc,
    my_free,
    { 0 },
    0,
    0,
    0,
    0,
    NULL, /* format string, filled in by tests */
    1, /* -error bounds_check flag */
    0x20000, /* Ndr library version */
    0,
    0x50100a4, /* MIDL Version 5.1.164 */
    0,
    NULL,
    0,  /* notify & notify_flag routine table */
    1,  /* Flags */
    0,  /* Reserved3 */
    0,  /* Reserved4 */
    0   /* Reserved5 */
    };


static void test_pointer_marshal(const unsigned char *formattypes,
                                 void *memsrc,
                                 long srcsize,
                                 const void *wiredata,
                                 long wiredatalen,
                                 const char *msgpfx)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    DWORD size;
    void *ptr;
    unsigned char *mem, *mem_orig;

    my_alloc_called = my_free_called = 0;

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = formattypes;

    NdrClientInitializeNew(
                           &RpcMessage,
                           &StubMsg,
                           &StubDesc,
                           0);

    StubMsg.BufferLength = 0;
    NdrPointerBufferSize( &StubMsg,
                          memsrc,
                          formattypes );
    ok(StubMsg.BufferLength >= wiredatalen, "%s: length %ld\n", msgpfx, StubMsg.BufferLength);

    /*NdrGetBuffer(&_StubMsg, _StubMsg.BufferLength, NULL);*/
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    StubMsg.BufferEnd = StubMsg.BufferStart + StubMsg.BufferLength;

    memset(StubMsg.BufferStart, 0x0, StubMsg.BufferLength); /* This is a hack to clear the padding between the ptr and longlong/double */

    ptr = NdrPointerMarshall( &StubMsg,  memsrc, formattypes );
    ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %ld\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
    ok(!memcmp(StubMsg.BufferStart, wiredata, wiredatalen), "%s: incorrectly marshaled\n", msgpfx);

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 0;

#if 0 /* NdrPointerMemorySize crashes under Wine, remove #if 0 when this is fixed */
    size = NdrPointerMemorySize( &StubMsg, formattypes );
    ok(size == StubMsg.MemorySize, "%s: mem size %ld size %ld\n", msgpfx, StubMsg.MemorySize, size); 
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %ld\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen); 
    if(formattypes[1] & 0x10 /* FC_POINTER_DEREF */)
        ok(size == srcsize + 4, "%s: mem size %ld\n", msgpfx, size);
    else
        ok(size == srcsize, "%s: mem size %ld\n", msgpfx, size);

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 16;
    size = NdrPointerMemorySize( &StubMsg, formattypes );
    ok(size == StubMsg.MemorySize, "%s: mem size %ld size %ld\n", msgpfx, StubMsg.MemorySize, size); 
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %ld\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen); 
    if(formattypes[1] & 0x10 /* FC_POINTER_DEREF */)
        ok(size == srcsize + 4 + 16, "%s: mem size %ld\n", msgpfx, size);
    else
        ok(size == srcsize + 16, "%s: mem size %ld\n", msgpfx, size);
 
    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 1;
    size = NdrPointerMemorySize( &StubMsg, formattypes );
    ok(size == StubMsg.MemorySize, "%s: mem size %ld size %ld\n", msgpfx, StubMsg.MemorySize, size); 
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %ld\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen); 
    if(formattypes[1] & 0x10 /* FC_POINTER_DEREF */)
        ok(size == srcsize + 4 + (srcsize == 8 ? 8 : 4), "%s: mem size %ld\n", msgpfx, size);
    else
        ok(size == srcsize + (srcsize == 8 ? 8 : 4), "%s: mem size %ld\n", msgpfx, size);

#endif

    size = srcsize;
    if(formattypes[1] & 0x10) size += 4;

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 0;
    mem_orig = mem = HeapAlloc(GetProcessHeap(), 0, size); 

    if(formattypes[1] & 0x10 /* FC_POINTER_DEREF */)
        *(void**)mem = NULL;
    ptr = NdrPointerUnmarshall( &StubMsg, &mem, formattypes, 0 );
    ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
    ok(mem == mem_orig, "%s: mem has changed %p %p\n", msgpfx, mem, mem_orig);
    if(formattypes[1] & 0x10 /* FC_POINTER_DEREF */)
        ok(!memcmp(*(void**)mem, *(void**)memsrc, srcsize), "%s: incorrectly unmarshaled\n", msgpfx);
    else
        ok(!memcmp(mem, memsrc, srcsize), "%s: incorrectly unmarshaled\n", msgpfx);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %ld\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
    ok(StubMsg.MemorySize == 0, "%s: memorysize %ld\n", msgpfx, StubMsg.MemorySize);
    if(formattypes[1] & 0x10)
        my_alloc_called--; /* this should call my_alloc */
    ok(my_alloc_called == 0, "%s: didn't expect call to my_alloc\n", msgpfx);
 
    /* reset the buffer and call with must alloc */
    StubMsg.Buffer = StubMsg.BufferStart;
    if(formattypes[1] & 0x10 /* FC_POINTER_DEREF */)
        *(void**)mem = NULL;
    ptr = NdrPointerUnmarshall( &StubMsg, &mem, formattypes, 1 );
    ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
    /* doesn't allocate mem in this case */
todo_wine {
    ok(mem == mem_orig, "%s: mem has changed %p %p\n", msgpfx, mem, mem_orig);
 }
    if(formattypes[1] & 0x10 /* FC_POINTER_DEREF */)
        ok(!memcmp(*(void**)mem, *(void**)memsrc, srcsize), "%s: incorrectly unmarshaled\n", msgpfx);
    else
        ok(!memcmp(mem, memsrc, srcsize), "%s: incorrectly unmarshaled\n", msgpfx);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %ld\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
    ok(StubMsg.MemorySize == 0, "%s: memorysize %ld\n", msgpfx, StubMsg.MemorySize);

    if(formattypes[1] & 0x10)
        my_alloc_called--; /* this should call my_alloc */
todo_wine {
    ok(my_alloc_called == 0, "%s: didn't expect call to my_alloc\n", msgpfx);
    my_alloc_called = 0;
}
    if(formattypes[0] != 0x11 /* FC_RP */)
    {
        /* now pass the address of a NULL ptr */
        mem = NULL;
        StubMsg.Buffer = StubMsg.BufferStart;
        ptr = NdrPointerUnmarshall( &StubMsg, &mem, formattypes, 0 );
        ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
        ok(mem != StubMsg.BufferStart + wiredatalen - srcsize, "%s: mem points to buffer %p %p\n", msgpfx, mem, StubMsg.BufferStart);
        ok(!memcmp(mem, memsrc, size), "%s: incorrectly unmarshaled\n", msgpfx);
        ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %ld\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
        ok(StubMsg.MemorySize == 0, "%s: memorysize %ld\n", msgpfx, StubMsg.MemorySize);
        ok(my_alloc_called == 1, "%s: expected call to my_alloc\n", msgpfx);
        NdrPointerFree(&StubMsg, mem, formattypes);
 
        /* again pass address of NULL ptr, but pretend we're a server */
        mem = NULL;
        StubMsg.Buffer = StubMsg.BufferStart;
        StubMsg.IsClient = 0;
        ptr = NdrPointerUnmarshall( &StubMsg, &mem, formattypes, 0 );
        ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
todo_wine {
        ok(mem == StubMsg.BufferStart + wiredatalen - srcsize, "%s: mem doesn't point to buffer %p %p\n", msgpfx, mem, StubMsg.BufferStart);
 }
        ok(!memcmp(mem, memsrc, size), "%s: incorrecly unmarshaled\n", msgpfx);
        ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %ld\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
        ok(StubMsg.MemorySize == 0, "%s: memorysize %ld\n", msgpfx, StubMsg.MemorySize); 
todo_wine {
        ok(my_alloc_called == 1, "%s: didn't expect call to my_alloc\n", msgpfx);
 }
    }
    HeapFree(GetProcessHeap(), 0, mem_orig);
    HeapFree(GetProcessHeap(), 0, StubMsg.BufferStart);
}

static void test_simple_types()
{
    unsigned char wiredata[16];
    unsigned char ch;
    unsigned char *ch_ptr;
    unsigned short s;
    unsigned int i;
    unsigned long l;
    ULONGLONG ll;
    float f;
    double d;

    static const unsigned char fmtstr_up_char[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x2,            /* FC_CHAR */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_byte[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x1,            /* FC_BYTE */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_small[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x3,            /* FC_SMALL */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_usmall[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x4,            /* FC_USMALL */
        0x5c,           /* FC_PAD */
    };  
    static const unsigned char fmtstr_rp_char[] =
    {
        0x11, 0x8,      /* FC_RP [simple_pointer] */
        0x2,            /* FC_CHAR */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_rpup_char[] =
    {
        0x11, 0x14,     /* FC_RP [alloced_on_stack] */
        NdrFcShort( 0x2 ),      /* Offset= 2 (4) */
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x2,            /* FC_CHAR */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_rpup_char2[] =
    {
        0x11, 0x04,     /* FC_RP [alloced_on_stack] */
        NdrFcShort( 0x2 ),      /* Offset= 2 (4) */
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x2,            /* FC_CHAR */
        0x5c,           /* FC_PAD */
    };

    static const unsigned char fmtstr_up_wchar[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x5,            /* FC_WCHAR */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_short[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x6,            /* FC_SHORT */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_ushort[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x7,            /* FC_USHORT */
        0x5c,           /* FC_PAD */
    };
#if 0
    static const unsigned char fmtstr_up_enum16[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0xd,            /* FC_ENUM16 */
        0x5c,           /* FC_PAD */
    };
#endif
    static const unsigned char fmtstr_up_long[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x8,            /* FC_LONG */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_ulong[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x9,            /* FC_ULONG */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_enum32[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0xe,            /* FC_ENUM32 */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_errorstatus[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x10,           /* FC_ERROR_STATUS_T */
        0x5c,           /* FC_PAD */
    };

    static const unsigned char fmtstr_up_longlong[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0xb,            /* FC_HYPER */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_float[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0xa,            /* FC_FLOAT */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_double[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0xc,            /* FC_DOUBLE */
        0x5c,           /* FC_PAD */
    };

    ch = 0xa5;
    ch_ptr = &ch;
    *(void**)wiredata = ch_ptr;
    wiredata[sizeof(void*)] = ch;
 
    test_pointer_marshal(fmtstr_up_char, ch_ptr, 1, wiredata, 5,  "up_char");
    test_pointer_marshal(fmtstr_up_byte, ch_ptr, 1, wiredata, 5,  "up_byte");
    test_pointer_marshal(fmtstr_up_small, ch_ptr, 1, wiredata, 5,  "up_small");
    test_pointer_marshal(fmtstr_up_usmall, ch_ptr, 1, wiredata, 5, "up_usmall");

    test_pointer_marshal(fmtstr_rp_char, ch_ptr, 1, &ch, 1, "rp_char");

    test_pointer_marshal(fmtstr_rpup_char, &ch_ptr, 1, wiredata, 5, "rpup_char");
    test_pointer_marshal(fmtstr_rpup_char2, ch_ptr, 1, wiredata, 5, "rpup_char2");

    s = 0xa597;
    *(void**)wiredata = &s;
    *(unsigned short*)(wiredata + sizeof(void*)) = s;

    test_pointer_marshal(fmtstr_up_wchar, &s, 2, wiredata, 6,  "up_wchar");
    test_pointer_marshal(fmtstr_up_short, &s, 2, wiredata, 6,  "up_short");
    test_pointer_marshal(fmtstr_up_ushort, &s, 2, wiredata, 6,  "up_ushort");

    i = s;
    *(void**)wiredata = &i;
#if 0 /* Not sure why this crashes under Windows */
    test_pointer_marshal(fmtstr_up_enum16, &i, 2, wiredata, 6,  "up_enum16");
#endif

    l = 0xcafebabe;
    *(void**)wiredata = &l;
    *(unsigned long*)(wiredata + sizeof(void*)) = l;

    test_pointer_marshal(fmtstr_up_long, &l, 4, wiredata, 8,  "up_long");
    test_pointer_marshal(fmtstr_up_ulong, &l, 4, wiredata, 8,  "up_ulong");
    test_pointer_marshal(fmtstr_up_enum32, &l, 4, wiredata, 8,  "up_emun32");
    test_pointer_marshal(fmtstr_up_errorstatus, &l, 4, wiredata, 8,  "up_errorstatus");

    ll = ((ULONGLONG)0xcafebabe) << 32 | 0xdeadbeef;
    *(void**)wiredata = &ll;
    *(void**)(wiredata + sizeof(void*)) = NULL;
    *(ULONGLONG*)(wiredata + 2 * sizeof(void*)) = ll;
    test_pointer_marshal(fmtstr_up_longlong, &ll, 8, wiredata, 16,  "up_longlong");

    f = 3.1415;
    *(void**)wiredata = &f;
    *(float*)(wiredata + sizeof(void*)) = f;
    test_pointer_marshal(fmtstr_up_float, &f, 4, wiredata, 8,  "up_float");

    d = 3.1415;
    *(void**)wiredata = &d;
    *(void**)(wiredata + sizeof(void*)) = NULL;
    *(double*)(wiredata + 2 * sizeof(void*)) = d;
    test_pointer_marshal(fmtstr_up_double, &d, 8, wiredata, 16,  "up_double");

}


START_TEST( ndr_marshall )
{
    test_simple_types();
}
