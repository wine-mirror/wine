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

#define BUFFER_PARANOIA 20

#if defined(__i386__)
  #define LITTLE_ENDIAN_UINT32_WRITE(pchar, word32) \
    (*((UINT32 *)(pchar)) = (word32))

  #define LITTLE_ENDIAN_UINT32_READ(pchar) \
    (*((UINT32 *)(pchar)))
#else
  /* these would work for i386 too, but less efficient */
  #define LITTLE_ENDIAN_UINT32_WRITE(pchar, word32) \
    (*(pchar)     = LOBYTE(LOWORD(word32)), \
     *((pchar)+1) = HIBYTE(LOWORD(word32)), \
     *((pchar)+2) = LOBYTE(HIWORD(word32)), \
     *((pchar)+3) = HIBYTE(HIWORD(word32)), \
     (word32)) /* allow as r-value */

  #define LITTLE_ENDIAN_UINT32_READ(pchar) \
    (MAKELONG( \
      MAKEWORD(*(pchar), *((pchar)+1)), \
      MAKEWORD(*((pchar)+2), *((pchar)+3))))
#endif

/*
 * NdrConformantString:
 * 
 * What MS calls a ConformantString is, in DCE terminology,
 * a Varying Conformant String.
 * [
 *   maxlen: DWORD (max # of CHARTYPE characters, inclusive of '\0')
 *   offset: DWORD (actual elements begin at (offset) CHARTYPE's into (data))
 *   length: DWORD (# of CHARTYPE characters, inclusive of '\0')
 *   [ 
 *     data: CHARTYPE[maxlen]
 *   ] 
 * ], where CHARTYPE is the appropriate character type (specified externally)
 *
 */

/***********************************************************************
 *            NdrConformantStringMarshall [RPCRT4.@]
 */
unsigned char *WINAPI NdrConformantStringMarshall(MIDL_STUB_MESSAGE *pStubMsg, unsigned char *pszMessage,
  PFORMAT_STRING pFormat)
{ 
  UINT32 len, i;
  unsigned char *c;

  TRACE("(pStubMsg == ^%p, pszMessage == ^%p, pFormat == ^%p)\n", pStubMsg, pszMessage, pFormat);
  
  assert(pFormat);
  if (*pFormat == RPC_FC_C_CSTRING) {
    len = strlen(pszMessage);
    assert( (pStubMsg->BufferLength > (len + 13)) && (pStubMsg->Buffer != NULL) );
    c = pStubMsg->Buffer;
    memset(c, 0, 12);
    LITTLE_ENDIAN_UINT32_WRITE(c, len + 1); /* max length: strlen + 1 (for '\0') */
    c += 8;                             /* offset: 0 */
    LITTLE_ENDIAN_UINT32_WRITE(c, len + 1); /* actual length: (same) */
    c += 4;
    for (i = 0; i <= len; i++)
      *(c++) = *(pszMessage++);         /* the string itself */
  } else {
    ERR("Unhandled string type: %#x\n", *pFormat); 
    /* FIXME: raise an exception. */
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

  assert(pFormat);
  if (*pFormat == RPC_FC_C_CSTRING) {
    /* we need 12 octets for the [maxlen, offset, len] DWORDS, + 1 octet for '\0' */
    pStubMsg->BufferLength = strlen(pMemory) + 13 + BUFFER_PARANOIA;
  } else {
    ERR("Unhandled string type: %#x\n", *pFormat); 
    /* FIXME: raise an exception */
  }
}

/************************************************************************
 *            NdrConformantStringMemorySize [RPCRT4.@]
 */
unsigned long WINAPI NdrConformantStringMemorySize( PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat )
{
  unsigned long rslt = 0;

  TRACE("(pStubMsg == ^%p, pFormat == ^%p)\n", pStubMsg, pFormat);
   
  assert(pStubMsg && pFormat);

  if (*pFormat == RPC_FC_C_CSTRING) {
    rslt = LITTLE_ENDIAN_UINT32_READ(pStubMsg->Buffer); /* maxlen */
  } else {
    ERR("Unhandled string type: %#x\n", *pFormat);
    /* FIXME: raise an exception */
  }

  TRACE("  --> %lu\n", rslt);
  return rslt;
}

/************************************************************************
 *           NdrConformantStringUnmarshall [RPCRT4.@]
 */
unsigned char *WINAPI NdrConformantStringUnmarshall( PMIDL_STUB_MESSAGE pStubMsg, unsigned char** ppMemory,
  PFORMAT_STRING pFormat, unsigned char fMustAlloc )
{
  unsigned long len, ofs;

  TRACE("(pStubMsg == ^%p, *pMemory == ^%p, pFormat == ^%p, fMustAlloc == %u)\n",
    pStubMsg, *ppMemory, pFormat, fMustAlloc);

  assert(pFormat && ppMemory && pStubMsg);

  len = NdrConformantStringMemorySize(pStubMsg, pFormat);

  /* now the actual length (in bytes) that we need to store
     the unmarshalled string is in len, including terminating '\0' */

  if ( fMustAlloc || (!(*ppMemory)) || (pStubMsg->Memory != *ppMemory) || 
       (pStubMsg->MemorySize < (len+BUFFER_PARANOIA)) ) {
    /* crap, looks like we need to do something about the Memory.  I don't
       understand, it doesn't look like Microsoft is doing this the same
       way... but then how do they do it?  AFAICS the Memory is never deallocated by
       the stub code so where does it go?... anyhow, I guess we'll just do it
       our own way for now... */
    pStubMsg->MemorySize = len + BUFFER_PARANOIA;
    pStubMsg->Memory = *ppMemory;
    /* FIXME: pfnAllocate? or does that not apply to these "Memory" parts? */
    *ppMemory = pStubMsg->Memory = HeapReAlloc(GetProcessHeap(), 0, pStubMsg->Memory, pStubMsg->MemorySize);   
  } 

  if (!(pStubMsg->Memory)) {
    ERR("Memory Allocation Failure\n");
    /* FIXME: raise an exception */
    return NULL;
  }

  /* OK, we've got our ram.  now do the real unmarshalling */
  if (*pFormat == RPC_FC_C_CSTRING) {
    char *c = *ppMemory;

    pStubMsg->Buffer += 4;
    ofs = LITTLE_ENDIAN_UINT32_READ(pStubMsg->Buffer);
    pStubMsg->Buffer += 4;
    len = LITTLE_ENDIAN_UINT32_READ(pStubMsg->Buffer);
    pStubMsg->Buffer += 4;

    pStubMsg->Buffer += ofs;

    while ((*c++ = *(pStubMsg->Buffer++)) != '\0') 
      ;
  } else {
    ERR("Unhandled string type: %#x\n", *pFormat);
    /* FIXME: raise an exception */
  }

  return NULL; /* FIXME: is this always right? */
}

/***********************************************************************
 *           NdrConvert [RPCRT4.@]
 */
void WINAPI NdrConvert( PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat )
{
  FIXME("(pStubMsg == ^%p, pFormat == ^%p): stub.\n", pStubMsg, pFormat);
  /* FIXME: since this stub doesn't do any converting, the proper behavior
     is to raise an exception */
}

/***********************************************************************
 *           NdrConvert2 [RPCRT4.@]
 */
void WINAPI NdrConvert2( PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat, long NumberParams )
{
  FIXME("(pStubMsg == ^%p, pFormat == ^%p, NumberParams == %ld): stub.\n", pStubMsg, pFormat, NumberParams);
  /* FIXME: since this stub doesn't do any converting, the proper behavior
     is to raise an exception */
}
