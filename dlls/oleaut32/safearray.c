/*************************************************************************
 * OLE Automation
 * SafeArray Implementation
 *
 * This file contains the implementation of the SafeArray interface.
 *
 * Copyright 1999 Sylvain St-Germain
 */

#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winerror.h"
#include "winbase.h"
#include "oleauto.h"
#include "wine/obj_base.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ole);

#define SYSDUPSTRING(str) SysAllocStringLen((str), SysStringLen(str))

/* Localy used methods */
static INT  
endOfDim(LONG *coor, SAFEARRAYBOUND *mat, LONG dim, LONG realDim);

static ULONG   
calcDisplacement(LONG *coor, SAFEARRAYBOUND *mat, LONG dim);

static BOOL  
isPointer(USHORT feature);

static INT   
getFeatures(VARTYPE vt);

static BOOL  
validCoordinate(LONG *coor, SAFEARRAY *psa);

static BOOL  
resizeSafeArray(SAFEARRAY *psa, LONG lDelta);

static BOOL  
validArg(SAFEARRAY *psa);

static ULONG   
getArraySize(SAFEARRAY *psa);

static HRESULT 
duplicateData(SAFEARRAY *psa, SAFEARRAY **ppsaOut);

/* Association between VARTYPE and their size.
   A size of zero is defined for the unsupported types.  */

#define VARTYPE_NOT_SUPPORTED 0
static const ULONG VARTYPE_SIZE[] =
{
  /* this is taken from wtypes.h.  Only [S]es are supported by the SafeArray */
VARTYPE_NOT_SUPPORTED,  /* VT_EMPTY    [V]   [P]    nothing			*/
VARTYPE_NOT_SUPPORTED,  /* VT_NULL     [V]   [P]    SQL style Nul	*/
2,                      /* VT_I2       [V][T][P][S] 2 byte signed int */
4,                      /* VT_I4       [V][T][P][S] 4 byte signed int */
4,                      /* VT_R4       [V][T][P][S] 4 byte real	*/
8,                      /* VT_R8       [V][T][P][S] 8 byte real	*/
8,                      /* VT_CY       [V][T][P][S] currency */
8,                      /* VT_DATE     [V][T][P][S] date */
sizeof(BSTR),           /* VT_BSTR     [V][T][P][S] OLE Automation string*/
sizeof(LPDISPATCH),     /* VT_DISPATCH [V][T][P][S] IDispatch *	*/
4,                      /* VT_ERROR    [V][T]   [S] SCODE	*/
4,                      /* VT_BOOL     [V][T][P][S] True=-1, False=0*/
sizeof(VARIANT),        /* VT_VARIANT  [V][T][P][S] VARIANT *	*/
sizeof(LPUNKNOWN),      /* VT_UNKNOWN  [V][T]   [S] IUnknown * */
sizeof(DECIMAL),        /* VT_DECIMAL  [V][T]   [S] 16 byte fixed point	*/
VARTYPE_NOT_SUPPORTED,                         /* no VARTYPE here..... */
VARTYPE_NOT_SUPPORTED,	/* VT_I1          [T]       signed char	*/
1,                      /* VT_UI1      [V][T][P][S] unsigned char			*/
VARTYPE_NOT_SUPPORTED,	/* VT_UI2         [T][P]    unsigned short	*/
VARTYPE_NOT_SUPPORTED,	/* VT_UI4         [T][P]    unsigned short	*/
VARTYPE_NOT_SUPPORTED,	/* VT_I8          [T][P]    signed 64-bit int			*/
VARTYPE_NOT_SUPPORTED,	/* VT_UI8         [T][P]    unsigned 64-bit int		*/
VARTYPE_NOT_SUPPORTED,	/* VT_INT         [T]       signed machine int		*/
VARTYPE_NOT_SUPPORTED,	/* VT_UINT        [T]       unsigned machine int	*/
VARTYPE_NOT_SUPPORTED,	/* VT_VOID        [T]       C style void			*/
VARTYPE_NOT_SUPPORTED,	/* VT_HRESULT     [T]       Standard return type	*/
VARTYPE_NOT_SUPPORTED,	/* VT_PTR         [T]       pointer type			*/
VARTYPE_NOT_SUPPORTED,	/* VT_SAFEARRAY   [T]       (use VT_ARRAY in VARIANT)*/
VARTYPE_NOT_SUPPORTED,	/* VT_CARRAY      [T]       C style array			*/
VARTYPE_NOT_SUPPORTED,	/* VT_USERDEFINED [T]       user defined type			*/
VARTYPE_NOT_SUPPORTED,	/* VT_LPSTR       [T][P]    null terminated string	*/
VARTYPE_NOT_SUPPORTED,	/* VT_LPWSTR      [T][P]    wide null term string		*/
VARTYPE_NOT_SUPPORTED,	/* VT_FILETIME       [P]    FILETIME			*/
VARTYPE_NOT_SUPPORTED,	/* VT_BLOB           [P]    Length prefixed bytes */
VARTYPE_NOT_SUPPORTED,	/* VT_STREAM         [P]    Name of stream follows		*/
VARTYPE_NOT_SUPPORTED,	/* VT_STORAGE        [P]    Name of storage follows	*/
VARTYPE_NOT_SUPPORTED,	/* VT_STREAMED_OBJECT[P]    Stream contains an object*/
VARTYPE_NOT_SUPPORTED,	/* VT_STORED_OBJECT  [P]    Storage contains object*/
VARTYPE_NOT_SUPPORTED,	/* VT_BLOB_OBJECT    [P]    Blob contains an object*/
VARTYPE_NOT_SUPPORTED,	/* VT_CF             [P]    Clipboard format			*/
VARTYPE_NOT_SUPPORTED,	/* VT_CLSID          [P]    A Class ID			*/
VARTYPE_NOT_SUPPORTED,	/* VT_VECTOR         [P]    simple counted array		*/
VARTYPE_NOT_SUPPORTED,	/* VT_ARRAY    [V]          SAFEARRAY*			*/
VARTYPE_NOT_SUPPORTED 	/* VT_BYREF    [V]          void* for local use	*/
};

static const int LAST_VARTYPE = sizeof(VARTYPE_SIZE)/sizeof(ULONG);


/*************************************************************************
 *		SafeArrayAllocDescriptor (OLEAUT32.36)
 * Allocate the appropriate amount of memory for the SafeArray descriptor
 */
HRESULT WINAPI SafeArrayAllocDescriptor( 
  UINT    cDims, 
  SAFEARRAY **ppsaOut) 
{
  SAFEARRAYBOUND *sab;
  LONG allocSize = 0;

  /* SAFEARRAY + SAFEARRAYBOUND * (cDims -1) ( -1 because there is already one
                                             ( in SAFEARRAY struct */
  allocSize = sizeof(**ppsaOut) + (sizeof(*sab) * (cDims-1));

  /* Allocate memory for SAFEARRAY struc */
  if(( (*ppsaOut)=HeapAlloc( 
        GetProcessHeap(), HEAP_ZERO_MEMORY, allocSize)) == NULL){
    return(E_UNEXPECTED);
  }
  TRACE("SafeArray: %lu bytes allocated for descriptor.\n", allocSize);

  return(S_OK);
}

/*************************************************************************
 *		SafeArrayAllocData (OLEAUT32.37)
 * Allocate the appropriate amount of data for the SafeArray data
 */
HRESULT WINAPI SafeArrayAllocData(
  SAFEARRAY *psa) 
{
  ULONG  ulWholeArraySize;   /* to store the size of the whole thing */

  if(! validArg(psa)) 
    return E_INVALIDARG;

  ulWholeArraySize = getArraySize(psa);

  /* Allocate memory for the data itself */
  if((psa->pvData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 
        psa->cbElements*ulWholeArraySize)) == NULL)
    return(E_UNEXPECTED);

  TRACE("SafeArray: %lu bytes allocated for data at %p (%lu objects).\n", 
    psa->cbElements*ulWholeArraySize, psa->pvData, ulWholeArraySize);

  return(S_OK);
}

/*************************************************************************
 *		SafeArrayCreate (OLEAUT32.15)
 * Create a SafeArray object by encapsulating AllocDescriptor and AllocData 
 */
SAFEARRAY* WINAPI SafeArrayCreate(
  VARTYPE        vt, 
  UINT         cDims, 
  SAFEARRAYBOUND *rgsabound)
{
  SAFEARRAY *psa;
  HRESULT   hRes;
  USHORT    cDim;

  /* Validate supported VARTYPE */
  if ( (vt >= LAST_VARTYPE) ||
       ( VARTYPE_SIZE[vt] == VARTYPE_NOT_SUPPORTED ) )
    return NULL;

  /* Allocate memory for the array descriptor */
  if( FAILED( hRes = SafeArrayAllocDescriptor(cDims, &psa)))
    return NULL;

  /* setup data members... */ 
  psa->cDims     = cDims;
  psa->fFeatures = getFeatures(vt);
  psa->cLocks    = 0;
  psa->pvData    = NULL;
  psa->cbElements= VARTYPE_SIZE[vt];

  /* Invert the bounds ... */
  for(cDim=0; cDim < psa->cDims; cDim++) {
    psa->rgsabound[cDim].cElements = rgsabound[psa->cDims-cDim-1].cElements;
    psa->rgsabound[cDim].lLbound   = rgsabound[psa->cDims-cDim-1].lLbound;
  }

  /* allocate memory for the data... */ 
  if( FAILED( hRes = SafeArrayAllocData(psa))) {
    SafeArrayDestroyDescriptor(psa); 
    ERR("() : Failed to allocate the Safe Array data\n");
    return NULL;
  }

  return(psa); 
}

/*************************************************************************
 *		SafeArrayDestroyDescriptor (OLEAUT32.38)
 * Frees the memory associated with the descriptor.
 */
HRESULT WINAPI SafeArrayDestroyDescriptor(
  SAFEARRAY *psa)
{
  /* Check for lockness before to free... */
  if(psa->cLocks > 0) 
    return DISP_E_ARRAYISLOCKED;

  /* The array is unlocked, then, deallocate memory */
  if(HeapFree( GetProcessHeap(), 0, psa) == FALSE) 
    return E_UNEXPECTED;
  
  return(S_OK);
}


/*************************************************************************
 *		SafeArrayLock (OLEAUT32.21)
 * Increment the lock counter
 *
 * Doc says (MSDN Library ) that psa->pvData should be made available (!= NULL)
 * only when psa->cLocks is > 0... I don't get it since pvData is allocated 
 * before the array is locked, therefore  
 */
HRESULT WINAPI SafeArrayLock(
  SAFEARRAY *psa)
{
  if(! validArg(psa))     
    return E_INVALIDARG;

  psa->cLocks++;

  return(S_OK);
}

/*************************************************************************
 *		SafeArrayUnlock (OLEAUT32.22)
 * Decrement the lock counter
 */
HRESULT WINAPI SafeArrayUnlock(
  SAFEARRAY *psa)
{
  if(! validArg(psa)) 
    return E_INVALIDARG;

  if (psa->cLocks > 0) 
    psa->cLocks--;

  return(S_OK);
}


/*************************************************************************
 *		SafeArrayPutElement (OLEAUT32.26)
 * Set the data at the given coordinate
 */
HRESULT WINAPI SafeArrayPutElement(
  SAFEARRAY *psa, 
  LONG      *rgIndices, 
  void      *pv)
{
  ULONG stepCountInSAData     = 0;    /* Number of array item to skip to get to 
                                         the desired one... */
  PVOID elementStorageAddress = NULL; /* Adress to store the data */

  /* Validate the index given */
  if(! validCoordinate(rgIndices, psa)) 
    return DISP_E_BADINDEX;
  if(! validArg(psa))
    return E_INVALIDARG;

  if( SafeArrayLock(psa) == S_OK) {

    /* Figure out the number of items to skip */
    stepCountInSAData = calcDisplacement(rgIndices, psa->rgsabound, psa->cDims);
  
    /* Figure out the number of byte to skip ... */
    elementStorageAddress = (char *) psa->pvData+(stepCountInSAData*psa->cbElements);
  
    if(isPointer(psa->fFeatures)) { /* increment ref count for this pointer */

      *((PVOID*)elementStorageAddress) = *(PVOID*)pv; 
      IUnknown_AddRef( *(IUnknown**)pv);

    } else { 

      if(psa->fFeatures == FADF_BSTR) { /* Create a new object */
        BSTR pbstrReAllocStr = NULL;
        if(pv &&
           ((pbstrReAllocStr = SYSDUPSTRING( (OLECHAR*)pv )) == NULL)) {
          SafeArrayUnlock(psa);  
          return E_OUTOFMEMORY;
        } else 
          *((BSTR*)elementStorageAddress) = pbstrReAllocStr;
      }
      else if(psa->fFeatures == FADF_VARIANT) {
        HRESULT hr = VariantCopy(elementStorageAddress, pv);
        if (FAILED(hr)) {
          SafeArrayUnlock(psa);
          return hr;
        }
      }
      else /* duplicate the memory */
        memcpy(elementStorageAddress, pv, SafeArrayGetElemsize(psa) );
    }

  } else {
    ERR("SafeArray: Cannot lock array....\n");
    return E_UNEXPECTED; /* UNDOC error condition */
  }

  TRACE("SafeArray: item put at adress %p.\n",elementStorageAddress);
  return SafeArrayUnlock(psa);  
}


/*************************************************************************
 *		SafeArrayGetElement (OLEAUT32.25)
 * Return the data element corresponding the the given coordinate
 */
HRESULT WINAPI SafeArrayGetElement(
  SAFEARRAY *psa, 
  LONG      *rgIndices, 
  void      *pv)
{
  ULONG stepCountInSAData     = 0;    /* Number of array item to skip to get to 
                                         the desired one... */
  PVOID elementStorageAddress = NULL; /* Adress to store the data */

  if(! validArg(psa)) 
    return E_INVALIDARG;
  
  if(! validCoordinate(rgIndices, psa)) /* Validate the index given */
    return(DISP_E_BADINDEX);

  if( SafeArrayLock(psa) == S_OK) {

    /* Figure out the number of items to skip */
    stepCountInSAData = calcDisplacement(rgIndices, psa->rgsabound, psa->cDims);
  
    /* Figure out the number of byte to skip ... */
    elementStorageAddress = (char *) psa->pvData+(stepCountInSAData*psa->cbElements);
  
    if( psa->fFeatures == FADF_BSTR) {           /* reallocate the obj */
      BSTR pbstrStoredStr = *(OLECHAR**)elementStorageAddress;
      BSTR pbstrReturnedStr = NULL;
      if( pbstrStoredStr &&
          ((pbstrReturnedStr = SYSDUPSTRING( pbstrStoredStr )) == NULL) ) {
        SafeArrayUnlock(psa);
        return E_OUTOFMEMORY;
      } else 
        *((BSTR*)pv) = pbstrReturnedStr; 
    }
    else if( psa->fFeatures == FADF_VARIANT) {
      HRESULT hr = VariantCopy(pv, elementStorageAddress);
      if (FAILED(hr)) {
        SafeArrayUnlock(psa);
        return hr;
      }
    }
    else if( isPointer(psa->fFeatures) )         /* simply copy the pointer */
      *(PVOID*)pv = *((PVOID*)elementStorageAddress); 
    else                                         /* copy the bytes */
      memcpy(pv, elementStorageAddress, psa->cbElements );

  } else {
    ERR("SafeArray: Cannot lock array....\n");
    return E_UNEXPECTED; /* UNDOC error condition */
  }

  return( SafeArrayUnlock(psa) );  
}

/*************************************************************************
 *		SafeArrayGetUBound (OLEAUT32.19)
 * return the UP bound for a given array dimension
 */
HRESULT WINAPI SafeArrayGetUBound(
  SAFEARRAY *psa, 
  UINT    nDim,
  LONG      *plUbound)
{
  if(! validArg(psa))   
    return E_INVALIDARG;

  if(nDim > psa->cDims) 
    return DISP_E_BADINDEX;

  if(0 == nDim)
    return DISP_E_BADINDEX;

  *plUbound = psa->rgsabound[nDim-1].lLbound + 
              psa->rgsabound[nDim-1].cElements - 1;

  return S_OK;
}

/*************************************************************************
 *		SafeArrayGetLBound (OLEAUT32.20)
 * Return the LO bound for a given array dimension 
 */
HRESULT WINAPI SafeArrayGetLBound(
  SAFEARRAY *psa,
  UINT    nDim, 
  LONG      *plLbound)
{
  if(! validArg(psa))   
    return E_INVALIDARG;

  if(nDim > psa->cDims) 
    return DISP_E_BADINDEX;

  if(0 == nDim)
    return DISP_E_BADINDEX;
  
  *plLbound = psa->rgsabound[nDim-1].lLbound;
  return S_OK;
}

/*************************************************************************
 *		SafeArrayGetDim (OLEAUT32.17)
 * returns the number of dimension in the array
 */
UINT WINAPI SafeArrayGetDim(
  SAFEARRAY * psa)
{ 
  /*
   * A quick test in Windows shows that the behavior here for an invalid
   * pointer is to return 0.
   */
  if(! validArg(psa)) 
    return 0;

  return psa->cDims;
}

/*************************************************************************
 *		SafeArrayGetElemsize (OLEAUT32.18)
 * Return the size of the element in the array
 */
UINT WINAPI SafeArrayGetElemsize(
  SAFEARRAY * psa)
{ 
  /*
   * A quick test in Windows shows that the behavior here for an invalid
   * pointer is to return 0.
   */
  if(! validArg(psa)) 
    return 0;

  return psa->cbElements;
}

/*************************************************************************
 *		SafeArrayAccessData (OLEAUT32.23)
 * increment the access count and return the data 
 */
HRESULT WINAPI SafeArrayAccessData(
  SAFEARRAY *psa, 
  void      **ppvData)
{ 
  HRESULT hRes;

  if(! validArg(psa)) 
    return E_INVALIDARG;

  hRes = SafeArrayLock(psa);

  switch (hRes) {
    case S_OK: 
      (*ppvData) = psa->pvData;
      break;
    case E_INVALIDARG:
      (*ppvData) = NULL;
      return E_INVALIDARG;
  }
  
  return S_OK;
}


/*************************************************************************
 *		SafeArrayUnaccessData (OLEAUT32.24)
 * Decrement the access count
 */
HRESULT WINAPI SafeArrayUnaccessData(
  SAFEARRAY * psa)
{ 
  if(! validArg(psa)) 
    return E_INVALIDARG;

  return(SafeArrayUnlock(psa));
}

/************************************************************************ 
 *		SafeArrayPtrOfIndex (OLEAUT32.148)
 * Return a pointer to the element at rgIndices
 */
HRESULT WINAPI SafeArrayPtrOfIndex(
  SAFEARRAY *psa, 
  LONG      *rgIndices, 
  void      **ppvData)
{ 
  ULONG stepCountInSAData     = 0;    /* Number of array item to skip to get to 
                                         the desired one... */

  if(! validArg(psa))                   
    return E_INVALIDARG;

  if(! validCoordinate(rgIndices, psa)) 
    return DISP_E_BADINDEX;

  /* Figure out the number of items to skip */
  stepCountInSAData = calcDisplacement(rgIndices, psa->rgsabound, psa->cDims);
  
  *ppvData = (char *) psa->pvData+(stepCountInSAData*psa->cbElements);

  return S_OK;
}

/************************************************************************ 
 *		SafeArrayDestroyData (OLEAUT32.39)
 * Frees the memory data bloc
 */
HRESULT WINAPI SafeArrayDestroyData(
  SAFEARRAY *psa)
{ 
  HRESULT  hRes;
  ULONG    ulWholeArraySize; /* count spot in array  */
  ULONG    ulDataIter;       /* to iterate the data space */

  if(! validArg(psa)) 
    return E_INVALIDARG;

  if(psa->cLocks > 0) 
    return DISP_E_ARRAYISLOCKED;

  ulWholeArraySize = getArraySize(psa);

  if(isPointer(psa->fFeatures)) {           /* release the pointers */
    IUnknown *punk;

    for(ulDataIter=0; ulDataIter < ulWholeArraySize; ulDataIter++) {
      punk = *(IUnknown**)((char *) psa->pvData+(ulDataIter*(psa->cbElements)));	

      if( punk != NULL) 
        IUnknown_Release(punk);
    }

  }
  else if(psa->fFeatures & FADF_BSTR) {  /* deallocate the obj */
    BSTR bstr;

    for(ulDataIter=0; ulDataIter < ulWholeArraySize; ulDataIter++) {
      bstr = *(BSTR*)((char *) psa->pvData+(ulDataIter*(psa->cbElements)));

      if( bstr != NULL) 
        SysFreeString( bstr );
    }
  }
  else if(psa->fFeatures & FADF_VARIANT) {  /* deallocate the obj */

    for(ulDataIter=0; ulDataIter < ulWholeArraySize; ulDataIter++) {
      VariantClear((VARIANT*)((char *) psa->pvData+(ulDataIter*(psa->cbElements))));
    }
  }
      
  /* check if this array is a Vector, in which case do not free the data 
     block since it has been allocated by AllocDescriptor and therefore
     deserve to be freed by DestroyDescriptor */
  if(!(psa->fFeatures & FADF_CREATEVECTOR)) { /* Set when we do CreateVector */

    /* free the whole chunk */
    if((hRes = HeapFree( GetProcessHeap(), 0, psa->pvData)) == 0) /*falied*/
      return E_UNEXPECTED; /* UNDOC error condition */

    psa->pvData = NULL;
  }
  
  return S_OK;
}

/************************************************************************ 
 *		SafeArrayCopyData (OLEAUT32.412)
 * Copy the psaSource's data block into psaTarget if dimension and size
 * permits it.
 */
HRESULT WINAPI SafeArrayCopyData(
  SAFEARRAY *psaSource,
  SAFEARRAY **psaTarget)
{ 
  USHORT   cDimCount;        /* looper */
  LONG     lDelta;           /* looper */
  IUnknown *punk;   
  ULONG    ulWholeArraySize; /* Number of item in SA */
  BSTR   bstr;

  if(! (validArg(psaSource) && validArg(*psaTarget)) ) 
    return E_INVALIDARG;

  if(SafeArrayGetDim(psaSource) != SafeArrayGetDim(*psaTarget))
    return E_INVALIDARG;

  ulWholeArraySize = getArraySize(psaSource); 

  /* The two arrays boundaries must be of same lenght */
  for(cDimCount=0;cDimCount < psaSource->cDims; cDimCount++)
    if( psaSource->rgsabound[cDimCount].cElements != 
      (*psaTarget)->rgsabound[cDimCount].cElements)
      return E_INVALIDARG;

  if( isPointer((*psaTarget)->fFeatures) ) {         /* the target contains ptr 
                                                        that must be released */
    for(lDelta=0;lDelta < ulWholeArraySize; lDelta++) {
      punk = *(IUnknown**)
        ((char *) (*psaTarget)->pvData + (lDelta * (*psaTarget)->cbElements));

      if( punk != NULL) 
        IUnknown_Release(punk);
    }

  }
  else if( (*psaTarget)->fFeatures & FADF_BSTR) {    /* the target contain BSTR
                                                        that must be freed */ 
    for(lDelta=0;lDelta < ulWholeArraySize; lDelta++) {
      bstr = 
        *(BSTR*)((char *) (*psaTarget)->pvData + (lDelta * (*psaTarget)->cbElements));

      if( bstr != NULL) 
        SysFreeString( bstr );
    }
  }
  else if( (*psaTarget)->fFeatures & FADF_VARIANT) {

    for(lDelta=0;lDelta < ulWholeArraySize; lDelta++) {
      VariantClear((VARIANT*)((char *) (*psaTarget)->pvData + (lDelta * (*psaTarget)->cbElements)));
    }
  }

  return duplicateData(psaSource, psaTarget);
}

/************************************************************************ 
 *		SafeArrayDestroy (OLEAUT32.16)
 * Deallocates all memory reserved for the SafeArray
 */
HRESULT WINAPI SafeArrayDestroy(
  SAFEARRAY * psa)
{ 
  HRESULT hRes;

  if(! validArg(psa)) 
    return E_INVALIDARG;

  if(psa->cLocks > 0) 
    return DISP_E_ARRAYISLOCKED;

  if((hRes = SafeArrayDestroyData( psa )) == S_OK)
    if((hRes = SafeArrayDestroyDescriptor( psa )) == S_OK)
      return S_OK;

  return E_UNEXPECTED; /* UNDOC error condition */
}

/************************************************************************ 
 *		SafeArrayCopy (OLEAUT32.27)
 * Make a dupplicate of a SafeArray
 */
HRESULT WINAPI SafeArrayCopy(
  SAFEARRAY *psa, 
  SAFEARRAY **ppsaOut)
{ 
  HRESULT hRes;
  DWORD   dAllocSize;
  ULONG   ulWholeArraySize; /* size of the thing */

  if(! validArg(psa)) 
    return E_INVALIDARG;

  if((hRes=SafeArrayAllocDescriptor(psa->cDims, ppsaOut)) == S_OK){

    /* Duplicate the SAFEARRAY struc */
    memcpy(*ppsaOut, psa, 
            sizeof(*psa)+(sizeof(*(psa->rgsabound))*(psa->cDims-1)));

    (*ppsaOut)->pvData = NULL; /* do not point to the same data area */

    /* make sure the new safe array doesn't have the FADF_CREATEVECTOR flag,
       because the data has not been allocated with the descriptor. */
    (*ppsaOut)->fFeatures &= ~FADF_CREATEVECTOR;  
 
    /* Get the allocated memory size for source and allocate it for target */ 
    ulWholeArraySize = getArraySize(psa); /* Number of item in SA */
    dAllocSize = ulWholeArraySize*psa->cbElements;

    (*ppsaOut)->pvData = 
      HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dAllocSize);
    if( (*ppsaOut)->pvData != NULL) {   /* HeapAlloc succeed */

      if( (hRes=duplicateData(psa, ppsaOut)) != S_OK) { /* E_OUTOFMEMORY */
        HeapFree(GetProcessHeap(), 0, (*ppsaOut)->pvData);
        (*ppsaOut)->pvData = NULL;
        SafeArrayDestroyDescriptor(*ppsaOut);
        return hRes;
      }
        
    } else { /* failed to allocate or dupplicate... */
      SafeArrayDestroyDescriptor(*ppsaOut);
      return E_UNEXPECTED; /* UNDOC error condition */
    }
  } else { /* failed to allocate mem for descriptor */
    return E_OUTOFMEMORY; /* UNDOC error condiftion */
  }

  return S_OK;
}

/************************************************************************ 
 *		SafeArrayCreateVector (OLEAUT32.411)
 * Creates a one dimension safearray where the data is next to the 
 * SAFEARRAY structure.
 */
SAFEARRAY* WINAPI SafeArrayCreateVector(
  VARTYPE vt, 
  LONG    lLbound, 
  ULONG   cElements) 
{ 
  SAFEARRAY *psa;

  /* Validate supported VARTYPE */
  if ( (vt >= LAST_VARTYPE) ||
       ( VARTYPE_SIZE[vt] == VARTYPE_NOT_SUPPORTED ) )
    return NULL;

  /* Allocate memory for the array descriptor and data contiguously  */
  if( FAILED( psa = HeapAlloc( GetProcessHeap(), 
                      HEAP_ZERO_MEMORY, 
                      (sizeof(*psa) + (VARTYPE_SIZE[vt] * cElements))))) {
    return NULL;
  }
										
  /* setup data members... */ 
  psa->cDims      = 1; /* always and forever */
  psa->fFeatures  = getFeatures(vt) | FADF_CREATEVECTOR;  /* undocumented flag used by Microsoft */
  psa->cLocks     = 0;
  psa->pvData     = (BYTE*)psa + sizeof(*psa);
  psa->cbElements = VARTYPE_SIZE[vt];

  psa->rgsabound[0].cElements = cElements;
  psa->rgsabound[0].lLbound   = lLbound;

  return(psa); 				  
} 

/************************************************************************ 
 *		SafeArrayRedim (OLEAUT32.40)
 * Changes the caracteristics of the last dimension of the SafeArray
 */
HRESULT WINAPI SafeArrayRedim(
  SAFEARRAY      *psa, 
  SAFEARRAYBOUND *psaboundNew)
{ 
  LONG   lDelta;  /* hold difference in size */
  USHORT cDims=1; /* dims counter */

  if( !validArg(psa) )                    
    return E_INVALIDARG;

  if( psa->cLocks > 0 )                    
    return DISP_E_ARRAYISLOCKED;

  if( psa->fFeatures & FADF_FIXEDSIZE )    
    return E_INVALIDARG;

  if( SafeArrayLock(psa)==E_UNEXPECTED ) 
    return E_UNEXPECTED;/* UNDOC error condition */

  /* find the delta in number of array spot to apply to the new array */
  lDelta = psaboundNew->cElements - psa->rgsabound[0].cElements;
  for(; cDims < psa->cDims; cDims++)
    /* delta in number of spot implied by modifying the last dimension */
    lDelta *= psa->rgsabound[cDims].cElements;

  if (lDelta == 0) { ;/* same size, maybe a change of lLbound, just set it */

  } else /* need to enlarge (lDelta +) reduce (lDelta -) */
    if(! resizeSafeArray(psa, lDelta)) 
      return E_UNEXPECTED; /* UNDOC error condition */

  /* the only modifyable dimension sits in [0] as the dimensions were reversed  
     at array creation time... */
  psa->rgsabound[0].cElements = psaboundNew->cElements;
  psa->rgsabound[0].lLbound   = psaboundNew->lLbound;

  return SafeArrayUnlock(psa);
}

/************************************************************************
 * NOT WINDOWS API - SafeArray* Utility functions
 ************************************************************************/

/************************************************************************ 
 * Used to validate the SAFEARRAY type of arg
 */
static BOOL validArg(
  SAFEARRAY *psa) 
{
  SAFEARRAYBOUND *sab;
  LONG psaSize  = 0;
  LONG descSize = 0;
  LONG fullSize = 0;

  /*
   * Let's check for the null pointer just in case.
   */
  if (psa == NULL)
    return FALSE;

  /* Check whether the size of the chunk makes sense... That's the only thing
     I can think of now... */

  psaSize = HeapSize(GetProcessHeap(), 0, psa);
  if (psaSize == -1)
    /* uh, foreign heap. Better don't mess with it ! */
    return TRUE;

  /* size of the descriptor when the SA is not created with CreateVector */
  descSize = sizeof(*psa) + (sizeof(*sab) * (psa->cDims-1));

  /* size of the descriptor + data when created with CreateVector */
  fullSize = sizeof(*psa) + (psa->cbElements * psa->rgsabound[0].cElements);

  return((psaSize >= descSize) || (psaSize >= fullSize));
}

/************************************************************************ 
 * Used to reallocate memory
 */
static BOOL resizeSafeArray(
  SAFEARRAY *psa, 
  LONG lDelta)
{
  ULONG    ulWholeArraySize;  /* use as multiplicator */
  PVOID    pvNewBlock = NULL;  
  IUnknown *punk;
  BSTR   bstr;

  ulWholeArraySize = getArraySize(psa);

  if(lDelta < 0) {                    /* array needs to be shorthen  */
    if( isPointer(psa->fFeatures))    /* ptr that need to be released */
      for(;lDelta < 0; lDelta++) {
	      punk = *(IUnknown**)
          ((char *) psa->pvData+((ulWholeArraySize+lDelta)*psa->cbElements));
	
        if( punk != NULL )
          IUnknown_Release(punk);
	    }

    else if(psa->fFeatures & FADF_BSTR)  /* BSTR that need to be freed */
      for(;lDelta < 0; lDelta++) {
        bstr = *(BSTR*)
          ((char *) psa->pvData+((ulWholeArraySize+lDelta)*psa->cbElements));

        if( bstr != NULL )
          SysFreeString( bstr );
      }
    else if(psa->fFeatures & FADF_VARIANT)
      for(;lDelta < 0; lDelta++) {
        VariantClear((VARIANT*)((char *) psa->pvData+((ulWholeArraySize+lDelta)*psa->cbElements)));
      }
  }

  if (!(psa->fFeatures & FADF_CREATEVECTOR))
  {
    /* Ok now, if we are enlarging the array, we *MUST* move the whole block 
       pointed to by pvData.   If we are shorthening the array, this move is
       optional but we do it anyway becuase the benefit is that we are 
       releasing to the system the unused memory */

    if((pvNewBlock = HeapReAlloc(GetProcessHeap(), 0, psa->pvData, 
       (ulWholeArraySize + lDelta) * psa->cbElements)) == NULL) 
        return FALSE; /* TODO If we get here it means:
                         SHRINK situation :  we've deleted the undesired
                                             data and did not release the memory
                         GROWING situation:  we've been unable to grow the array
                      */
  }
  else
  {
    /* Allocate a new block, because the previous data has been allocated with 
       the descriptor in SafeArrayCreateVector function. */

    if((pvNewBlock = HeapAlloc(GetProcessHeap(), 0,
       ulWholeArraySize * psa->cbElements)) == NULL) 
        return FALSE;

    psa->fFeatures &= ~FADF_CREATEVECTOR;
  }
  /* reassign to the new block of data */
  psa->pvData = pvNewBlock;
  return TRUE;
}

/************************************************************************ 
 * Used to set the fFeatures data member of the SAFEARRAY structure. 
 */
static INT getFeatures(
  VARTYPE vt) 
{
  switch(vt) {
    case VT_BSTR:      return FADF_BSTR;
    case VT_UNKNOWN:   return FADF_UNKNOWN;
    case VT_DISPATCH:  return FADF_DISPATCH;
    case VT_VARIANT:   return FADF_VARIANT;
  }
  return 0;
}

/************************************************************************ 
 * Used to figure out if the fFeatures data member of the SAFEARRAY 
 * structure contain any information about the type of data stored... 
 */
static BOOL isPointer(
  USHORT feature) 
{
  switch(feature) {
    case FADF_UNKNOWN:  return TRUE; /* those are pointers */
    case FADF_DISPATCH: return TRUE;
  }
  return FALSE;
}

/************************************************************************ 
 * Used to calculate the displacement when accessing or modifying 
 * safearray data set.
 *
 *  Parameters: - LONG *coor is the desired location in the multidimension
 *              table.  Ex for a 3 dim table: coor[] = {1,2,3};
 *              - ULONG *mat is the format of the table.  Ex for a 3 dim
 *              table mat[] = {4,4,4};
 *              - USHORT dim is the number of dimension of the SafeArray
 */
static ULONG calcDisplacement(
  LONG           *coor, 
  SAFEARRAYBOUND *mat, 
  LONG           dim) 
{
  ULONG res = 0;
  LONG  iterDim;

  for(iterDim=0; iterDim<dim; iterDim++) 
    /* the -mat[dim] bring coor[dim] relative to 0 for calculation */
    res += ((coor[iterDim]-mat[iterDim].lLbound) * 
            endOfDim(coor, mat, iterDim+1, dim));

  TRACE("SafeArray: calculated displacement is %lu.\n", res);
  return(res);
}

/************************************************************************ 
 * Recursivity agent for calcDisplacement method.  Used within Put and 
 * Get methods.
 */
static INT endOfDim(
  LONG           *coor, 
  SAFEARRAYBOUND *mat, 
  LONG           dim, 
  LONG           realDim) 
{
  if(dim==realDim) 
    return 1;
  else 
    return (endOfDim(coor, mat, dim+1, realDim) * mat[dim].cElements);
}


/************************************************************************ 
 * Method used to validate the coordinate received in Put and Get 
 * methods.
 */
static BOOL validCoordinate(
  LONG      *coor, 
  SAFEARRAY *psa) 
{
  INT   iter=0;
  LONG    lUBound;
  LONG    lLBound;
  HRESULT hRes;

  for(; iter<psa->cDims; iter++) {
    if((hRes = SafeArrayGetLBound(psa, (iter+1), &lLBound)) != S_OK)
      return FALSE;
    if((hRes = SafeArrayGetUBound(psa, (iter+1), &lUBound)) != S_OK)
      return FALSE;
 
    if(lLBound == lUBound) 
      return FALSE; 
   
    if((coor[iter] >= lLBound) && (coor[iter] <= lUBound))
      return TRUE;
    else
      return FALSE;
  }
  return FALSE;
}  

/************************************************************************ 
 * Method used to calculate the number of cells of the SA
 */
static ULONG getArraySize(
  SAFEARRAY *psa) 
{
  USHORT cCount; 
  ULONG  ulWholeArraySize = 1;

  for(cCount=0; cCount < psa->cDims; cCount++) /* foreach dimensions... */
    ulWholeArraySize *= psa->rgsabound[cCount].cElements;

  return ulWholeArraySize;  
}


/************************************************************************ 
 * Method used to handle data space dupplication for Copy32 and CopyData32
 */
static HRESULT duplicateData(
  SAFEARRAY *psa, 
  SAFEARRAY **ppsaOut) 
{
  ULONG    ulWholeArraySize; /* size of the thing */
  LONG     lDelta;

  ulWholeArraySize = getArraySize(psa); /* Number of item in SA */
  
  SafeArrayLock(*ppsaOut);

  if( isPointer(psa->fFeatures) ) {  /* If datatype is object increment 
                                        object's reference count */
    IUnknown *punk;

    for(lDelta=0; lDelta < ulWholeArraySize; lDelta++) {
      punk = *(IUnknown**)((char *) psa->pvData+(lDelta * psa->cbElements));

      if( punk != NULL)
        IUnknown_AddRef(punk);
    }

    /* Copy the source array data into target array */
    memcpy((*ppsaOut)->pvData, psa->pvData, 
      ulWholeArraySize*psa->cbElements);

  }
  else if( psa->fFeatures & FADF_BSTR ) { /* if datatype is BSTR allocate 
                                             the BSTR in the new array */
    BSTR   pbstrReAllocStr = NULL;

    for(lDelta=0; lDelta < ulWholeArraySize; lDelta++) {
      if(( pbstrReAllocStr = SYSDUPSTRING(
            *(BSTR*)((char *) psa->pvData+(lDelta * psa->cbElements)))) == NULL) {

        SafeArrayUnlock(*ppsaOut);
        return E_OUTOFMEMORY;
      }

      *((BSTR*)((char *) (*ppsaOut)->pvData+(lDelta * psa->cbElements))) = 
        pbstrReAllocStr;
    }

  }
  else if( psa->fFeatures & FADF_VARIANT ) {

    for(lDelta=0; lDelta < ulWholeArraySize; lDelta++) {
      VariantCopy((VARIANT*)((char *) (*ppsaOut)->pvData+(lDelta * psa->cbElements)),
                  (VARIANT*)((char *) psa->pvData+(lDelta * psa->cbElements)));
    }

  }
  else { /* Simply copy the source array data into target array */

    memcpy((*ppsaOut)->pvData, psa->pvData, 
      ulWholeArraySize*psa->cbElements);
  }

  SafeArrayUnlock(*ppsaOut);

  return S_OK;
}


/************************************************************************ 
 *		SafeArrayGetVarType (OLEAUT32.77)
 * Returns the VARTYPE stored in the given safearray
 */
HRESULT WINAPI SafeArrayGetVarType(
  SAFEARRAY* psa,
  VARTYPE*   pvt)
{
  HRESULT hr = E_INVALIDARG;
  VARTYPE vt = VT_EMPTY;

  /* const short VARTYPE_OFFSET = -4; */

  if (psa->fFeatures & FADF_HAVEVARTYPE)
  {
    /* VT tag @ negative offset 4 in the array descriptor */
    FIXME("Returning VT_BSTR instead of VT_...\n");
    vt = VT_BSTR;
  }
  else if (psa->fFeatures & FADF_RECORD)
  {
    vt = VT_RECORD;
  }
  else if (psa->fFeatures & FADF_BSTR)
  {
    vt = VT_BSTR;
  }
  else if (psa->fFeatures & FADF_UNKNOWN)
  {
    vt = VT_UNKNOWN;
  }
  else if (psa->fFeatures & FADF_DISPATCH)
  {
    vt = VT_DISPATCH;
  }
  else if (psa->fFeatures & FADF_VARIANT)
  {
    vt = VT_VARIANT;
  }

  if (vt != VT_EMPTY)
  {
    *pvt = vt;
    hr = S_OK;
  }

  TRACE("HRESULT = %08lx\n", hr);
  return hr;
}
