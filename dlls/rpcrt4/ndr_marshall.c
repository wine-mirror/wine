/*
 * NDR data marshalling
 *
 * Copyright 2002 Greg Turner
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
 *
 * TODO:
 *  - figure out whether we *really* got this right
 *  - check for errors and throw exceptions
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"

#include "ndr_misc.h"

#include "wine/rpcfc.h"
#include "wine/obj_base.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define BUFFER_PARANOIA 40

/***********************************************************************
 *            NdrConformantStringMarshall [RPCRT4.@]
 */
unsigned char *WINAPI NdrConformantStringMarshall(MIDL_STUB_MESSAGE *pStubMsg, unsigned char *pszMessage,
  PFORMAT_STRING pFormat)
{ 
  UINT32 len, i;
  unsigned char *c;

  TRACE("(pStubMsg == ^%p, pszMessage == ^%p, pFormat == ^%p)\n", pStubMsg, pszMessage, pFormat);
  
  if (*pFormat == RPC_FC_C_CSTRING) {
    len = strlen(pszMessage);
    assert( (pStubMsg->BufferLength > (len + 13)) && (pStubMsg->Buffer != NULL) );
    /* in DCE terminology this is a Conformant Varying String */
    c = pStubMsg->Buffer;
    ZeroMemory(c, 12);
    *((UINT32 *)c) = len + 1;   /* max length: strlen + 1 (for '\0') */
    c += 8;                     /* offset: 0 */
    *((UINT32 *)c) = len + 1;   /* actual length: (same) */
    c += 4;
    for (i = 0; i <= len; i++)
      *(c++) = *(pszMessage++); /* copy the string itself into the remaining space */
  } else {
    ERR("Unhandled string type: %#x\n", *pFormat); 
    /* FIXME what to do here? */
    return NULL;
  }
  
  /* success */
  pStubMsg->fBufferValid = 1;
  return NULL; /* is this always right? */
}

/***********************************************************************
 *           NdrConformantStringBufferSize [RPCRT4.@]
 */
void WINAPI NdrConformantStringBufferSize(PMIDL_STUB_MESSAGE pStubMsg, unsigned char* pMemory, PFORMAT_STRING pFormat)
{
  TRACE("(pStubMsg == ^%p, pMemory == ^%p, pFormat == ^%p)\n", pStubMsg, pMemory, pFormat);

  if (*pFormat == RPC_FC_C_CSTRING) {
    pStubMsg->BufferLength = strlen(pMemory) + BUFFER_PARANOIA;
  } else {
    ERR("Unhandled string type: %#x\n", *pFormat); 
    /* FIXME what to do here? */
  }
}

/************************************************************************
 *            NdrConformantStringMemorySize [RPCRT4.@]
 */
unsigned long WINAPI NdrConformantStringMemorySize( PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat )
{
  FIXME("(pStubMsg == ^%p, pFormat == ^%p): stub.\n", pStubMsg, pFormat);
  return 0;
}

/************************************************************************
 *           NdrConformantStringUnmarshall [RPCRT4.@]
 */
unsigned char *WINAPI NdrConformantStringUnmarshall( PMIDL_STUB_MESSAGE pStubMsg, unsigned char** ppMemory,
  PFORMAT_STRING pFormat, unsigned char fMustAlloc )
{
  FIXME("stub\n");
  return NULL;
}

#undef BUFFER_PARANOIA
