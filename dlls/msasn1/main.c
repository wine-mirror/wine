/*
 * Copyright 2014 Austin English
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

#include "windef.h"
#include "winbase.h"
#include "msasn1.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msasn1);

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(instance);
            break;
    }

    return TRUE;
}

ASN1module_t WINAPI ASN1_CreateModule(ASN1uint32_t ver, ASN1encodingrule_e rule, ASN1uint32_t flags,
                                      ASN1uint32_t pdu, const ASN1GenericFun_t encoder[],
                                      const ASN1GenericFun_t decoder[], const ASN1FreeFun_t freemem[],
                                      const ASN1uint32_t size[], ASN1magic_t magic)
{
    ASN1module_t module = NULL;

    FIXME("(%08x %08x %08x %u %p %p %p %p %u): Stub!\n", ver, rule, flags, pdu, encoder, decoder, freemem, size, magic);

    if (!encoder || !decoder || !freemem || !size)
        return module;

    return module;
}
