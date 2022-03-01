/*
 * Copyright 2020 Vijay Kiran Kamuju
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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "msasn1.h"
#include "wine/test.h"

static void test_CreateModule(void)
{
    const ASN1GenericFun_t encfn[] = { NULL };
    const ASN1GenericFun_t decfn[] = { NULL };
    const ASN1FreeFun_t freefn[] = { NULL };
    const ASN1uint32_t size[] = { 0 };
    ASN1magic_t name = 0x61736e31;
    ASN1module_t mod;

    mod = ASN1_CreateModule(0, 0, 0, 0, NULL, NULL, NULL, NULL, 0);
    ok(!mod, "Expected Failure.\n");

    mod = ASN1_CreateModule(0, 0, 0, 0, encfn, NULL, NULL, NULL, 0);
    ok(!mod, "Expected Failure.\n");

    mod = ASN1_CreateModule(0, 0, 0, 0, encfn, decfn, NULL, NULL, 0);
    ok(!mod, "Expected Failure.\n");

    mod = ASN1_CreateModule(0, 0, 0, 0, encfn, decfn, freefn, NULL, 0);
    ok(!mod, "Expected Failure.\n");

    mod = ASN1_CreateModule(0, 0, 0, 0, encfn, decfn, freefn, size, 0);
    ok(!!mod, "Failed to create module.\n");
    ok(mod->nModuleName==0, "Got Module name = %ld.\n",mod->nModuleName);
    ok(mod->eRule==0, "Got eRule = %08x.\n",mod->eRule);
    ok(mod->dwFlags==0, "Got Flags = %08lx.\n",mod->dwFlags);
    ok(mod->cPDUs==0, "Got PDUs = %08lx.\n",mod->cPDUs);
    ok(mod->apfnFreeMemory==freefn, "Free function = %p.\n",mod->apfnFreeMemory);
    ok(mod->acbStructSize==size, "Struct size = %p.\n",mod->acbStructSize);
    ok(!mod->PER.apfnEncoder, "Encoder function should not be set.\n");
    ok(!mod->PER.apfnDecoder, "Decoder function should not be set.\n");
    ASN1_CloseModule(mod);

    mod = ASN1_CreateModule(ASN1_THIS_VERSION, ASN1_BER_RULE_DER, ASN1FLAGS_NOASSERT, 1, encfn, decfn, freefn, size, name);
    ok(!!mod, "Failed to create module.\n");
    ok(mod->nModuleName==name, "Got Module name = %ld.\n",mod->nModuleName);
    ok(mod->eRule==ASN1_BER_RULE_DER, "Got eRule = %08x.\n",mod->eRule);
    ok(mod->cPDUs==1, "Got PDUs = %08lx.\n",mod->cPDUs);
    ok(mod->dwFlags==ASN1FLAGS_NOASSERT, "Got Flags = %08lx.\n",mod->dwFlags);
    ok(mod->apfnFreeMemory==freefn, "Free function = %p.\n",mod->apfnFreeMemory);
    ok(mod->acbStructSize==size, "Struct size = %p.\n",mod->acbStructSize);
    ok(mod->BER.apfnEncoder==(ASN1BerEncFun_t *)encfn, "Encoder function = %p.\n",mod->BER.apfnEncoder);
    ok(mod->BER.apfnDecoder==(ASN1BerDecFun_t *)decfn, "Decoder function = %p.\n",mod->BER.apfnDecoder);
    ASN1_CloseModule(mod);

    mod = ASN1_CreateModule(ASN1_THIS_VERSION, ASN1_PER_RULE_ALIGNED, ASN1FLAGS_NOASSERT, 1, encfn, decfn, freefn, size, name);
    ok(!!mod, "Failed to create module.\n");
    ok(mod->nModuleName==name, "Got Module name = %ld.\n",mod->nModuleName);
    ok(mod->eRule==ASN1_PER_RULE_ALIGNED, "Got eRule = %08x.\n",mod->eRule);
    ok(mod->cPDUs==1, "Got PDUs = %08lx.\n",mod->cPDUs);
    ok(mod->dwFlags==ASN1FLAGS_NOASSERT, "Got Flags = %08lx.\n",mod->dwFlags);
    ok(mod->apfnFreeMemory==freefn, "Free function = %p.\n",mod->apfnFreeMemory);
    ok(mod->acbStructSize==size, "Struct size = %p.\n",mod->acbStructSize);
    ok(mod->PER.apfnEncoder==(ASN1PerEncFun_t *)encfn /* WINXP & WIN2008 */ ||
       broken(!mod->PER.apfnEncoder), "Encoder function = %p.\n",mod->PER.apfnEncoder);
    ok(mod->PER.apfnDecoder==(ASN1PerDecFun_t *)decfn /* WINXP & WIN2008 */ ||
       broken(!mod->PER.apfnDecoder), "Decoder function = %p.\n",mod->PER.apfnDecoder);
    ASN1_CloseModule(mod);
}

static void test_CreateEncoder(void)
{
    const ASN1GenericFun_t encfn[] = { NULL };
    const ASN1GenericFun_t decfn[] = { NULL };
    const ASN1FreeFun_t freefn[] = { NULL };
    const ASN1uint32_t size[] = { 0 };
    ASN1magic_t name = 0x61736e31;
    ASN1encoding_t encoder = NULL;
    ASN1octet_t buf[] = {0x54,0x65,0x73,0x74,0};
    ASN1module_t mod;
    ASN1error_e ret;

    ret = ASN1_CreateEncoder(NULL, NULL, NULL, 0, NULL);
    ok(ret == ASN1_ERR_BADARGS,"Got error code %d.\n",ret);

    mod = ASN1_CreateModule(ASN1_THIS_VERSION, ASN1_BER_RULE_DER, ASN1FLAGS_NOASSERT, 1, encfn, decfn, freefn, size, name);
    ret = ASN1_CreateEncoder(mod, NULL, NULL, 0, NULL);
    ok(ret == ASN1_ERR_BADARGS,"Got error code %d.\n",ret);

    ret = ASN1_CreateEncoder(mod, &encoder, NULL, 0, NULL);
    ok(ASN1_SUCCEEDED(ret),"Got error code %d.\n",ret);
    ok(!!encoder,"Encoder creation failed.\n");
    ok(encoder->magic==0x44434e45,"Got invalid magic = %08lx.\n",encoder->magic);
    ok(!encoder->version,"Got incorrect version = %08lx.\n",encoder->version);
    ok(encoder->module==mod,"Got incorrect module = %p.\n",encoder->module);
    ok(!encoder->buf,"Got incorrect buf = %p.\n",encoder->buf);
    ok(!encoder->size,"Got incorrect size = %lu.\n",encoder->size);
    ok(!encoder->len,"Got incorrect length = %lu.\n",encoder->len);
    ok(encoder->err==ASN1_SUCCESS,"Got incorrect err = %d.\n",encoder->err);
    ok(!encoder->bit,"Got incorrect bit = %lu.\n",encoder->bit);
    ok(!encoder->pos,"Got incorrect pos = %p.\n",encoder->pos);
    ok(!encoder->cbExtraHeader,"Got incorrect cbExtraHeader = %lu.\n",encoder->cbExtraHeader);
    ok(encoder->eRule == ASN1_BER_RULE_DER,"Got incorrect eRule = %08x.\n",encoder->eRule);
    ok(encoder->dwFlags == ASN1ENCODE_NOASSERT,"Got incorrect dwFlags = %08lx.\n",encoder->dwFlags);
    ASN1_CloseEncoder(encoder);

    ret = ASN1_CreateEncoder(mod, &encoder, buf, 0, NULL);
    ok(ASN1_SUCCEEDED(ret),"Got error code %d.\n",ret);
    ok(!!encoder,"Encoder creation failed.\n");
    ok(encoder->magic==0x44434e45,"Got invalid magic = %08lx.\n",encoder->magic);
    ok(!encoder->version,"Got incorrect version = %08lx.\n",encoder->version);
    ok(encoder->module==mod,"Got incorrect module = %p.\n",encoder->module);
    ok(!encoder->buf,"Got incorrect buf = %p.\n",encoder->buf);
    ok(!encoder->size,"Got incorrect size = %lu.\n",encoder->size);
    ok(!encoder->len,"Got incorrect length = %lu.\n",encoder->len);
    ok(encoder->err==ASN1_SUCCESS,"Got incorrect err = %d.\n",encoder->err);
    ok(!encoder->bit,"Got incorrect bit = %lu.\n",encoder->bit);
    ok(!encoder->pos,"Got incorrect pos = %p.\n",encoder->pos);
    ok(!encoder->cbExtraHeader,"Got incorrect cbExtraHeader = %lu.\n",encoder->cbExtraHeader);
    ok(encoder->eRule == ASN1_BER_RULE_DER,"Got incorrect eRule = %08x.\n",encoder->eRule);
    ok(encoder->dwFlags == ASN1ENCODE_NOASSERT,"Got incorrect dwFlags = %08lx.\n",encoder->dwFlags);
    ASN1_CloseEncoder(encoder);

    ret = ASN1_CreateEncoder(mod, &encoder, buf, 2, NULL);
    ok(ASN1_SUCCEEDED(ret),"Got error code %d.\n",ret);
    ok(!!encoder,"Encoder creation failed.\n");
    ok(encoder->magic==0x44434e45,"Got invalid magic = %08lx.\n",encoder->magic);
    ok(!encoder->version,"Got incorrect version = %08lx.\n",encoder->version);
    ok(encoder->module==mod,"Got incorrect module = %p.\n",encoder->module);
    ok(encoder->buf==buf,"Got incorrect buf = %p.\n",encoder->buf);
    ok(encoder->size==2,"Got incorrect size = %lu.\n",encoder->size);
    ok(!encoder->len,"Got incorrect length = %lu.\n",encoder->len);
    ok(encoder->err==ASN1_SUCCESS,"Got incorrect err = %d.\n",encoder->err);
    ok(!encoder->bit,"Got incorrect bit = %lu.\n",encoder->bit);
    ok(encoder->pos==buf,"Got incorrect pos = %p.\n",encoder->pos);
    ok(!encoder->cbExtraHeader,"Got incorrect cbExtraHeader = %lu.\n",encoder->cbExtraHeader);
    ok(encoder->eRule == ASN1_BER_RULE_DER,"Got incorrect eRule = %08x.\n",encoder->eRule);
    ok(encoder->dwFlags == (ASN1ENCODE_NOASSERT|ASN1ENCODE_SETBUFFER),"Got incorrect dwFlags = %08lx.\n",encoder->dwFlags);
    ASN1_CloseEncoder(encoder);

    ret = ASN1_CreateEncoder(mod, &encoder, buf, 4, NULL);
    ok(ASN1_SUCCEEDED(ret),"Got error code %d.\n",ret);
    ok(!!encoder,"Encoder creation failed.\n");
    ok(encoder->magic==0x44434e45,"Got invalid magic = %08lx.\n",encoder->magic);
    ok(!encoder->version,"Got incorrect version = %08lx.\n",encoder->version);
    ok(encoder->module==mod,"Got incorrect module = %p.\n",encoder->module);
    ok(encoder->buf==buf,"Got incorrect buf = %p.\n",encoder->buf);
    ok(encoder->size==4,"Got incorrect size = %lu.\n",encoder->size);
    ok(!encoder->len,"Got incorrect length = %lu.\n",encoder->len);
    ok(encoder->err==ASN1_SUCCESS,"Got incorrect err = %d.\n",encoder->err);
    ok(!encoder->bit,"Got incorrect bit = %lu.\n",encoder->bit);
    ok(encoder->pos==buf,"Got incorrect pos = %p.\n",encoder->pos);
    ok(!encoder->cbExtraHeader,"Got incorrect cbExtraHeader = %lu.\n",encoder->cbExtraHeader);
    ok(encoder->eRule == ASN1_BER_RULE_DER,"Got incorrect rule = %08x.\n",encoder->eRule);
    ok(encoder->dwFlags == (ASN1ENCODE_NOASSERT|ASN1ENCODE_SETBUFFER),"Got incorrect dwFlags = %08lx.\n",encoder->dwFlags);
    ASN1_CloseEncoder(encoder);
    ASN1_CloseModule(mod);

    mod = ASN1_CreateModule(ASN1_THIS_VERSION, ASN1_BER_RULE_DER, ASN1FLAGS_NONE, 1, encfn, decfn, freefn, size, name);
    ret = ASN1_CreateEncoder(mod, &encoder, buf, 0, NULL);
    ok(encoder->dwFlags == 0,"Got incorrect dwFlags = %08lx.\n",encoder->dwFlags);
    ASN1_CloseEncoder(encoder);

    ret = ASN1_CreateEncoder(mod, &encoder, buf, 4, NULL);
    ok(encoder->dwFlags == ASN1ENCODE_SETBUFFER,"Got incorrect dwFlags = %08lx.\n",encoder->dwFlags);
    ASN1_CloseEncoder(encoder);
    ASN1_CloseModule(mod);

    mod = ASN1_CreateModule(ASN1_THIS_VERSION, ASN1_PER_RULE_ALIGNED, ASN1FLAGS_NOASSERT, 1, encfn, decfn, freefn, size, name);
    ret = ASN1_CreateEncoder(mod, &encoder, buf, 0, NULL);
    ok(ASN1_SUCCEEDED(ret),"Got error code %d.\n",ret);
    ok(!!encoder,"Encoder creation failed.\n");
    ok(encoder->magic==0x44434e45,"Got invalid magic = %08lx.\n",encoder->magic);
    ok(!encoder->version,"Got incorrect version = %08lx.\n",encoder->version);
    ok(encoder->module==mod,"Got incorrect module = %p.\n",encoder->module);
    ok(!encoder->buf,"Got incorrect buf = %p.\n",encoder->buf);
    ok(!encoder->size,"Got incorrect size = %lu.\n",encoder->size);
    ok(!encoder->len,"Got incorrect length = %lu.\n",encoder->len);
    ok(encoder->err==ASN1_SUCCESS,"Got incorrect err = %d.\n",encoder->err);
    ok(!encoder->bit,"Got incorrect bit = %lu.\n",encoder->bit);
    ok(!encoder->pos,"Got incorrect pos = %p.\n",encoder->pos);
    ok(!encoder->cbExtraHeader,"Got incorrect cbExtraHeader = %lu.\n",encoder->cbExtraHeader);
    ok(encoder->eRule == ASN1_PER_RULE_ALIGNED,"Got incorrect eRule = %08x.\n",encoder->eRule);
    ok(encoder->dwFlags == ASN1ENCODE_NOASSERT,"Got incorrect dwFlags = %08lx.\n",encoder->dwFlags);
    ASN1_CloseEncoder(encoder);

    ret = ASN1_CreateEncoder(mod, &encoder, buf, 4, NULL);
    ok(!!encoder,"Encoder creation failed.\n");
    ok(encoder->magic==0x44434e45,"Got invalid magic = %08lx.\n",encoder->magic);
    ok(!encoder->version,"Got incorrect version = %08lx.\n",encoder->version);
    ok(encoder->module==mod,"Got incorrect module = %p.\n",encoder->module);
    ok(encoder->buf==buf,"Got incorrect buf = %p.\n",encoder->buf);
    ok(encoder->size==4,"Got incorrect size = %lu.\n",encoder->size);
    ok(!encoder->len,"Got incorrect length = %lu.\n",encoder->len);
    ok(encoder->err==ASN1_SUCCESS,"Got incorrect err = %d.\n",encoder->err);
    ok(!encoder->bit,"Got incorrect bit = %lu.\n",encoder->bit);
    ok(encoder->pos==buf,"Got incorrect pos = %p.\n",encoder->pos);
    ok(!encoder->cbExtraHeader,"Got incorrect cbExtraHeader = %lu.\n",encoder->cbExtraHeader);
    ok(encoder->eRule == ASN1_PER_RULE_ALIGNED,"Got incorrect rule = %08x.\n",encoder->eRule);
    ok(encoder->dwFlags == (ASN1FLAGS_NOASSERT|ASN1ENCODE_SETBUFFER),"Got incorrect dwFlags = %08lx.\n",encoder->dwFlags);
    ASN1_CloseEncoder(encoder);
    ASN1_CloseModule(mod);

    mod = ASN1_CreateModule(ASN1_THIS_VERSION, ASN1_PER_RULE_ALIGNED, ASN1FLAGS_NONE, 1, encfn, decfn, freefn, size, name);
    ret = ASN1_CreateEncoder(mod, &encoder, buf, 0, NULL);
    ok(encoder->dwFlags == 0,"Got incorrect dwFlags = %08lx.\n",encoder->dwFlags);
    ASN1_CloseEncoder(encoder);

    ret = ASN1_CreateEncoder(mod, &encoder, buf, 4, NULL);
    ok(encoder->dwFlags == ASN1ENCODE_SETBUFFER,"Got incorrect dwFlags = %08lx.\n",encoder->dwFlags);
    ASN1_CloseEncoder(encoder);
    ASN1_CloseModule(mod);
}

static void test_CreateDecoder(void)
{
    const ASN1GenericFun_t encfn[] = { NULL };
    const ASN1GenericFun_t decfn[] = { NULL };
    const ASN1FreeFun_t freefn[] = { NULL };
    const ASN1uint32_t size[] = { 0 };
    ASN1magic_t name = 0x61736e31;
    ASN1decoding_t decoder = NULL;
    ASN1octet_t buf[] = {0x54,0x65,0x73,0x74,0};
    ASN1module_t mod;
    ASN1error_e ret;

    ret = ASN1_CreateDecoder(NULL, NULL, NULL, 0, NULL);
    ok(ret == ASN1_ERR_BADARGS,"Got error code %d.\n",ret);

    mod = ASN1_CreateModule(ASN1_THIS_VERSION, ASN1_BER_RULE_DER, ASN1FLAGS_NOASSERT, 1, encfn, decfn, freefn, size, name);
    ret = ASN1_CreateDecoder(mod, NULL, NULL, 0, NULL);
    ok(ret == ASN1_ERR_BADARGS,"Got error code %d.\n",ret);

    ret = ASN1_CreateDecoder(mod, &decoder, NULL, 0, NULL);
    ok(ASN1_SUCCEEDED(ret),"Got error code %d.\n",ret);
    ok(!!decoder,"Decoder creation failed.\n");
    ok(decoder->magic==0x44434544,"Got invalid magic = %08lx.\n",decoder->magic);
    ok(!decoder->version,"Got incorrect version = %08lx.\n",decoder->version);
    ok(decoder->module==mod,"Got incorrect module = %p.\n",decoder->module);
    ok(!decoder->buf,"Got incorrect buf = %p.\n",decoder->buf);
    ok(!decoder->size,"Got incorrect size = %lu.\n",decoder->size);
    ok(!decoder->len,"Got incorrect length = %lu.\n",decoder->len);
    ok(decoder->err==ASN1_SUCCESS,"Got incorrect err = %d.\n",decoder->err);
    ok(!decoder->bit,"Got incorrect bit = %lu.\n",decoder->bit);
    ok(!decoder->pos,"Got incorrect pos = %p.\n",decoder->pos);
    ok(decoder->eRule == ASN1_BER_RULE_DER,"Got incorrect eRule = %08x.\n",decoder->eRule);
    ok(decoder->dwFlags == ASN1DECODE_NOASSERT,"Got incorrect dwFlags = %08lx.\n",decoder->dwFlags);
    ASN1_CloseDecoder(decoder);

    ret = ASN1_CreateDecoder(mod, &decoder, buf, 0, NULL);
    ok(ASN1_SUCCEEDED(ret),"Got error code %d.\n",ret);
    ok(!!decoder,"Decoder creation failed.\n");
    ok(decoder->magic==0x44434544,"Got invalid magic = %08lx.\n",decoder->magic);
    ok(!decoder->version,"Got incorrect version = %08lx.\n",decoder->version);
    ok(decoder->module==mod,"Got incorrect module = %p.\n",decoder->module);
    ok(decoder->buf==buf,"Got incorrect buf = %s.\n",decoder->buf);
    ok(!decoder->size,"Got incorrect size = %lu.\n",decoder->size);
    ok(!decoder->len,"Got incorrect length = %lu.\n",decoder->len);
    ok(decoder->err==ASN1_SUCCESS,"Got incorrect err = %d.\n",decoder->err);
    ok(!decoder->bit,"Got incorrect bit = %lu.\n",decoder->bit);
    ok(decoder->pos==buf,"Got incorrect pos = %s.\n",decoder->pos);
    ok(decoder->eRule == ASN1_BER_RULE_DER,"Got incorrect eRule = %08x.\n",decoder->eRule);
    ok(decoder->dwFlags == (ASN1DECODE_NOASSERT|ASN1DECODE_SETBUFFER), "Got incorrect dwFlags = %08lx.\n",decoder->dwFlags);
    ASN1_CloseDecoder(decoder);

    ret = ASN1_CreateDecoder(mod, &decoder, buf, 2, NULL);
    ok(ASN1_SUCCEEDED(ret),"Got error code %d.\n",ret);
    ok(!!decoder,"Decoder creation failed.\n");
    ok(decoder->magic==0x44434544,"Got invalid magic = %08lx.\n",decoder->magic);
    ok(!decoder->version,"Got incorrect version = %08lx.\n",decoder->version);
    ok(decoder->module==mod,"Got incorrect module = %p.\n",decoder->module);
    ok(decoder->buf==buf,"Got incorrect buf = %p.\n",decoder->buf);
    ok(decoder->size==2,"Got incorrect size = %lu.\n",decoder->size);
    ok(!decoder->len,"Got incorrect length = %lu.\n",decoder->len);
    ok(decoder->err==ASN1_SUCCESS,"Got incorrect err = %d.\n",decoder->err);
    ok(!decoder->bit,"Got incorrect bit = %lu.\n",decoder->bit);
    ok(decoder->pos==buf,"Got incorrect pos = %p.\n",decoder->pos);
    ok(decoder->eRule == ASN1_BER_RULE_DER,"Got incorrect eRule = %08x.\n",decoder->eRule);
    ok(decoder->dwFlags == (ASN1DECODE_NOASSERT|ASN1DECODE_SETBUFFER),"Got incorrect dwFlags = %08lx.\n",decoder->dwFlags);
    ASN1_CloseDecoder(decoder);

    ret = ASN1_CreateDecoder(mod, &decoder, buf, 4, NULL);
    ok(ASN1_SUCCEEDED(ret),"Got error code %d.\n",ret);
    ok(!!decoder,"Decoder creation failed.\n");
    ok(decoder->magic==0x44434544,"Got invalid magic = %08lx.\n",decoder->magic);
    ok(!decoder->version,"Got incorrect version = %08lx.\n",decoder->version);
    ok(decoder->module==mod,"Got incorrect module = %p.\n",decoder->module);
    ok(decoder->buf==buf,"Got incorrect buf = %p.\n",decoder->buf);
    ok(decoder->size==4,"Got incorrect size = %lu.\n",decoder->size);
    ok(!decoder->len,"Got incorrect length = %lu.\n",decoder->len);
    ok(decoder->err==ASN1_SUCCESS,"Got incorrect err = %d.\n",decoder->err);
    ok(!decoder->bit,"Got incorrect bit = %lu.\n",decoder->bit);
    ok(decoder->pos==buf,"Got incorrect pos = %p.\n",decoder->pos);
    ok(decoder->eRule == ASN1_BER_RULE_DER,"Got incorrect rule = %08x.\n",decoder->eRule);
    ok(decoder->dwFlags == (ASN1DECODE_NOASSERT|ASN1DECODE_SETBUFFER),"Got incorrect dwFlags = %08lx.\n",decoder->dwFlags);
    ASN1_CloseDecoder(decoder);
    ASN1_CloseModule(mod);

    mod = ASN1_CreateModule(ASN1_THIS_VERSION, ASN1_BER_RULE_DER, ASN1FLAGS_NONE, 1, encfn, decfn, freefn, size, name);
    ret = ASN1_CreateDecoder(mod, &decoder, buf, 0, NULL);
    ok(decoder->dwFlags == ASN1DECODE_SETBUFFER,"Got incorrect dwFlags = %08lx.\n",decoder->dwFlags);
    ASN1_CloseDecoder(decoder);

    ret = ASN1_CreateDecoder(mod, &decoder, buf, 4, NULL);
    ok(decoder->dwFlags == ASN1DECODE_SETBUFFER,"Got incorrect dwFlags = %08lx.\n",decoder->dwFlags);
    ASN1_CloseDecoder(decoder);
    ASN1_CloseModule(mod);

    mod = ASN1_CreateModule(ASN1_THIS_VERSION, ASN1_PER_RULE_ALIGNED, ASN1FLAGS_NOASSERT, 1, encfn, decfn, freefn, size, name);
    ret = ASN1_CreateDecoder(mod, &decoder, buf, 0, NULL);
    ok(ASN1_SUCCEEDED(ret),"Got error code %d.\n",ret);
    ok(!!decoder,"Decoder creation failed.\n");
    ok(decoder->magic==0x44434544,"Got invalid magic = %08lx.\n",decoder->magic);
    ok(!decoder->version,"Got incorrect version = %08lx.\n",decoder->version);
    ok(decoder->module==mod,"Got incorrect module = %p.\n",decoder->module);
    ok(decoder->buf==buf,"Got incorrect buf = %s.\n",decoder->buf);
    ok(!decoder->size,"Got incorrect size = %lu.\n",decoder->size);
    ok(!decoder->len,"Got incorrect length = %lu.\n",decoder->len);
    ok(decoder->err==ASN1_SUCCESS,"Got incorrect err = %d.\n",decoder->err);
    ok(!decoder->bit,"Got incorrect bit = %lu.\n",decoder->bit);
    ok(decoder->pos==buf,"Got incorrect pos = %s.\n",decoder->pos);
    ok(decoder->eRule == ASN1_PER_RULE_ALIGNED,"Got incorrect eRule = %08x.\n",decoder->eRule);
    ok(decoder->dwFlags == (ASN1DECODE_NOASSERT|ASN1DECODE_SETBUFFER),"Got incorrect dwFlags = %08lx.\n",decoder->dwFlags);
    ASN1_CloseDecoder(decoder);

    ret = ASN1_CreateDecoder(mod, &decoder, buf, 4, NULL);
    ok(!!decoder,"Decoder creation failed.\n");
    ok(decoder->magic==0x44434544,"Got invalid magic = %08lx.\n",decoder->magic);
    ok(!decoder->version,"Got incorrect version = %08lx.\n",decoder->version);
    ok(decoder->module==mod,"Got incorrect module = %p.\n",decoder->module);
    ok(decoder->buf==buf,"Got incorrect buf = %p.\n",decoder->buf);
    ok(decoder->size==4,"Got incorrect size = %lu.\n",decoder->size);
    ok(!decoder->len,"Got incorrect length = %lu.\n",decoder->len);
    ok(decoder->err==ASN1_SUCCESS,"Got incorrect err = %d.\n",decoder->err);
    ok(!decoder->bit,"Got incorrect bit = %lu.\n",decoder->bit);
    ok(decoder->pos==buf,"Got incorrect pos = %p.\n",decoder->pos);
    ok(decoder->eRule == ASN1_PER_RULE_ALIGNED,"Got incorrect rule = %08x.\n",decoder->eRule);
    ok(decoder->dwFlags == (ASN1FLAGS_NOASSERT|ASN1DECODE_SETBUFFER),"Got incorrect dwFlags = %08lx.\n",decoder->dwFlags);
    ASN1_CloseDecoder(decoder);
    ASN1_CloseModule(mod);

    mod = ASN1_CreateModule(ASN1_THIS_VERSION, ASN1_PER_RULE_ALIGNED, ASN1FLAGS_NONE, 1, encfn, decfn, freefn, size, name);
    ret = ASN1_CreateDecoder(mod, &decoder, buf, 0, NULL);
    ok(decoder->dwFlags == ASN1DECODE_SETBUFFER,"Got incorrect dwFlags = %08lx.\n",decoder->dwFlags);
    ASN1_CloseDecoder(decoder);

    ret = ASN1_CreateDecoder(mod, &decoder, buf, 4, NULL);
    ok(decoder->dwFlags == ASN1DECODE_SETBUFFER,"Got incorrect dwFlags = %08lx.\n",decoder->dwFlags);
    ASN1_CloseDecoder(decoder);
    ASN1_CloseModule(mod);
}

START_TEST(asn1)
{
    test_CreateModule();
    test_CreateEncoder();
    test_CreateDecoder();
}
