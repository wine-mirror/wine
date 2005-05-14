/*
 *	Monikers
 *
 *	Copyright 1998	Marcus Meissner
 *      Copyright 1999  Noomen Hamza
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
 * - IRunningObjectTable should work interprocess, but currently doesn't.
 *   Native (on Win2k at least) uses an undocumented RPC interface, IROT, to
 *   communicate with RPCSS which contains the table of marshalled data.
 * - IRunningObjectTable should use marshalling instead of simple ref
 *   counting as there is the possibility of using the running object table
 *   to access objects in other apartments.
 */

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wtypes.h"
#include "wine/debug.h"
#include "ole2.h"

#include "compobj_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define  BLOCK_TAB_SIZE 20 /* represent the first size table and it's increment block size */

/* define the structure of the running object table elements */
typedef struct RunObject{

    IUnknown*  pObj; /* points on a running object*/
    IMoniker*  pmkObj; /* points on a moniker who identifies this object */
    FILETIME   lastModifObj;
    DWORD      identRegObj; /* registration key relative to this object */
    DWORD      regTypeObj; /* registration type : strong or weak */
}RunObject;

/* define the RunningObjectTableImpl structure */
typedef struct RunningObjectTableImpl{

    IRunningObjectTableVtbl *lpVtbl;
    ULONG      ref;

    RunObject* runObjTab;            /* pointer to the first object in the table       */
    DWORD      runObjTabSize;       /* current table size                            */
    DWORD      runObjTabLastIndx;  /* first free index element in the table.        */
    DWORD      runObjTabRegister; /* registration key of the next registered object */

} RunningObjectTableImpl;

static RunningObjectTableImpl* runningObjectTableInstance = NULL;

static HRESULT WINAPI RunningObjectTableImpl_GetObjectIndex(RunningObjectTableImpl*,DWORD,IMoniker*,DWORD *);

/* define the EnumMonikerImpl structure */
typedef struct EnumMonikerImpl{

    IEnumMonikerVtbl *lpVtbl;
    ULONG      ref;

    RunObject* TabMoniker;    /* pointer to the first object in the table       */
    DWORD      TabSize;       /* current table size                             */
    DWORD      TabLastIndx;   /* last used index element in the table.          */
    DWORD      TabCurrentPos;    /* enum position in the list			*/

} EnumMonikerImpl;


/* IEnumMoniker Local functions*/
static HRESULT WINAPI EnumMonikerImpl_CreateEnumROTMoniker(RunObject* runObjTab,
                                                 ULONG TabSize,
						 ULONG TabLastIndx,
                                                 ULONG TabCurrentPos,
                                                 IEnumMoniker ** ppenumMoniker);
/***********************************************************************
 *        RunningObjectTable_QueryInterface
 */
static HRESULT WINAPI
RunningObjectTableImpl_QueryInterface(IRunningObjectTable* iface,
                                      REFIID riid,void** ppvObject)
{
    RunningObjectTableImpl *This = (RunningObjectTableImpl *)iface;

    TRACE("(%p,%p,%p)\n",This,riid,ppvObject);

    /* validate arguments */
    if (This==0)
        return CO_E_NOTINITIALIZED;

    if (ppvObject==0)
        return E_INVALIDARG;

    *ppvObject = 0;

    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IRunningObjectTable, riid))
        *ppvObject = (IRunningObjectTable*)This;

    if ((*ppvObject)==0)
        return E_NOINTERFACE;

    IRunningObjectTable_AddRef(iface);

    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_AddRef
 */
static ULONG WINAPI
RunningObjectTableImpl_AddRef(IRunningObjectTable* iface)
{
    RunningObjectTableImpl *This = (RunningObjectTableImpl *)iface;

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);
}

/***********************************************************************
 *        RunningObjectTable_Initialize
 */
static HRESULT WINAPI
RunningObjectTableImpl_Destroy(void)
{
    TRACE("()\n");

    if (runningObjectTableInstance==NULL)
        return E_INVALIDARG;

    /* free the ROT table memory */
    HeapFree(GetProcessHeap(),0,runningObjectTableInstance->runObjTab);

    /* free the ROT structure memory */
    HeapFree(GetProcessHeap(),0,runningObjectTableInstance);
    runningObjectTableInstance = NULL;

    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_Release
 */
static ULONG WINAPI
RunningObjectTableImpl_Release(IRunningObjectTable* iface)
{
    DWORD i;
    RunningObjectTableImpl *This = (RunningObjectTableImpl *)iface;
    ULONG ref;

    TRACE("(%p)\n",This);

    ref = InterlockedDecrement(&This->ref);

    /* unitialize ROT structure if there's no more reference to it*/
    if (ref == 0) {

        /* release all registered objects */
        for(i=0;i<This->runObjTabLastIndx;i++)
        {
            if (( This->runObjTab[i].regTypeObj &  ROTFLAGS_REGISTRATIONKEEPSALIVE) != 0)
                IUnknown_Release(This->runObjTab[i].pObj);

            IMoniker_Release(This->runObjTab[i].pmkObj);
        }
       /*  RunningObjectTable data structure will be not destroyed here ! the destruction will be done only
        *  when RunningObjectTableImpl_UnInitialize function is called
        */

        /* there's no more elements in the table */
        This->runObjTabRegister=0;
        This->runObjTabLastIndx=0;
    }

    return ref;
}

/***********************************************************************
 *        RunningObjectTable_Register
 *
 * PARAMS
 * grfFlags       [in] Registration options 
 * punkObject     [in] the object being registered
 * pmkObjectName  [in] the moniker of the object being registered
 * pdwRegister    [in] the value identifying the registration
 */
static HRESULT WINAPI
RunningObjectTableImpl_Register(IRunningObjectTable* iface, DWORD grfFlags,
               IUnknown *punkObject, IMoniker *pmkObjectName, DWORD *pdwRegister)
{
    HRESULT res=S_OK;
    RunningObjectTableImpl *This = (RunningObjectTableImpl *)iface;

    TRACE("(%p,%ld,%p,%p,%p)\n",This,grfFlags,punkObject,pmkObjectName,pdwRegister);

    /*
     * there's only two types of register : strong and or weak registration
     * (only one must be passed on parameter)
     */
    if ( ( (grfFlags & ROTFLAGS_REGISTRATIONKEEPSALIVE) || !(grfFlags & ROTFLAGS_ALLOWANYCLIENT)) &&
         (!(grfFlags & ROTFLAGS_REGISTRATIONKEEPSALIVE) ||  (grfFlags & ROTFLAGS_ALLOWANYCLIENT)) &&
         (grfFlags) )
        return E_INVALIDARG;

    if (punkObject==NULL || pmkObjectName==NULL || pdwRegister==NULL)
        return E_INVALIDARG;

    /* verify if the object to be registered was registered before */
    if (RunningObjectTableImpl_GetObjectIndex(This,-1,pmkObjectName,NULL)==S_OK)
        res = MK_S_MONIKERALREADYREGISTERED;

    /* put the new registered object in the first free element in the table */
    This->runObjTab[This->runObjTabLastIndx].pObj = punkObject;
    This->runObjTab[This->runObjTabLastIndx].pmkObj = pmkObjectName;
    This->runObjTab[This->runObjTabLastIndx].regTypeObj = grfFlags;
    This->runObjTab[This->runObjTabLastIndx].identRegObj = This->runObjTabRegister;
    CoFileTimeNow(&(This->runObjTab[This->runObjTabLastIndx].lastModifObj));

    /* gives a registration identifier to the registered object*/
    (*pdwRegister)= This->runObjTabRegister;

    if (This->runObjTabRegister == 0xFFFFFFFF){

        FIXME("runObjTabRegister: %ld is out of data limite \n",This->runObjTabRegister);
	return E_FAIL;
    }
    This->runObjTabRegister++;
    This->runObjTabLastIndx++;

    if (This->runObjTabLastIndx == This->runObjTabSize){ /* table is full ! so it must be resized */

        This->runObjTabSize+=BLOCK_TAB_SIZE; /* newsize table */
        This->runObjTab=HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,This->runObjTab,
                        This->runObjTabSize * sizeof(RunObject));
        if (!This->runObjTab)
            return E_OUTOFMEMORY;
    }
    /* add a reference to the object in the strong registration case */
    if ((grfFlags & ROTFLAGS_REGISTRATIONKEEPSALIVE) !=0 ) {
        TRACE("strong registration, reffing %p\n", punkObject);
        /* this is wrong; we should always add a reference to the object */
        IUnknown_AddRef(punkObject);
    }
    
    IMoniker_AddRef(pmkObjectName);

    return res;
}

/***********************************************************************
 *        RunningObjectTable_Revoke
 *
 * PARAMS
 *  dwRegister [in] Value identifying registration to be revoked
 */
static HRESULT WINAPI
RunningObjectTableImpl_Revoke( IRunningObjectTable* iface, DWORD dwRegister) 
{

    DWORD index,j;
    RunningObjectTableImpl *This = (RunningObjectTableImpl *)iface;

    TRACE("(%p,%ld)\n",This,dwRegister);

    /* verify if the object to be revoked was registered before or not */
    if (RunningObjectTableImpl_GetObjectIndex(This,dwRegister,NULL,&index)==S_FALSE)

        return E_INVALIDARG;

    /* release the object if it was registered with a strong registrantion option */
    if ((This->runObjTab[index].regTypeObj & ROTFLAGS_REGISTRATIONKEEPSALIVE)!=0) {
        TRACE("releasing %p\n", This->runObjTab[index].pObj);
        /* this is also wrong; we should always release the object (see above) */
        IUnknown_Release(This->runObjTab[index].pObj);
    }
    
    IMoniker_Release(This->runObjTab[index].pmkObj);

    /* remove the object from the table */
    for(j=index; j<This->runObjTabLastIndx-1; j++)
        This->runObjTab[j]= This->runObjTab[j+1];

    This->runObjTabLastIndx--;

    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_IsRunning
 *
 * PARAMS
 *  pmkObjectName [in]  moniker of the object whose status is desired 
 */
static HRESULT WINAPI
RunningObjectTableImpl_IsRunning( IRunningObjectTable* iface, IMoniker *pmkObjectName)
{
    RunningObjectTableImpl *This = (RunningObjectTableImpl *)iface;

    TRACE("(%p,%p)\n",This,pmkObjectName);

    return RunningObjectTableImpl_GetObjectIndex(This,-1,pmkObjectName,NULL);
}

/***********************************************************************
 *        RunningObjectTable_GetObject
 *
 * PARAMS
 * pmkObjectName [in] Pointer to the moniker on the object 
 * ppunkObject   [out] variable that receives the IUnknown interface pointer
 */
static HRESULT WINAPI
RunningObjectTableImpl_GetObject( IRunningObjectTable* iface,
                     IMoniker *pmkObjectName, IUnknown **ppunkObject)
{
    DWORD index;
    RunningObjectTableImpl *This = (RunningObjectTableImpl *)iface;

    TRACE("(%p,%p,%p)\n",This,pmkObjectName,ppunkObject);

    if (ppunkObject==NULL)
        return E_POINTER;

    *ppunkObject=0;

    /* verify if the object was registered before or not */
    if (RunningObjectTableImpl_GetObjectIndex(This,-1,pmkObjectName,&index)==S_FALSE) {
        WARN("Moniker unavailable - needs to work interprocess?\n");
        return MK_E_UNAVAILABLE;
    }

    /* add a reference to the object then set output object argument */
    IUnknown_AddRef(This->runObjTab[index].pObj);
    *ppunkObject=This->runObjTab[index].pObj;

    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_NoteChangeTime
 *
 * PARAMS
 *  dwRegister [in] Value identifying registration being updated
 *  pfiletime  [in] Pointer to structure containing object's last change time
 */
static HRESULT WINAPI
RunningObjectTableImpl_NoteChangeTime(IRunningObjectTable* iface,
                                      DWORD dwRegister, FILETIME *pfiletime)
{
    DWORD index=-1;
    RunningObjectTableImpl *This = (RunningObjectTableImpl *)iface;

    TRACE("(%p,%ld,%p)\n",This,dwRegister,pfiletime);

    /* verify if the object to be changed was registered before or not */
    if (RunningObjectTableImpl_GetObjectIndex(This,dwRegister,NULL,&index)==S_FALSE)
        return E_INVALIDARG;

    /* set the new value of the last time change */
    This->runObjTab[index].lastModifObj= (*pfiletime);

    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_GetTimeOfLastChange
 *
 * PARAMS
 *  pmkObjectName  [in]  moniker of the object whose status is desired 
 *  pfiletime      [out] structure that receives object's last change time
 */
static HRESULT WINAPI
RunningObjectTableImpl_GetTimeOfLastChange(IRunningObjectTable* iface,
                            IMoniker *pmkObjectName, FILETIME *pfiletime)
{
    DWORD index=-1;
    RunningObjectTableImpl *This = (RunningObjectTableImpl *)iface;

    TRACE("(%p,%p,%p)\n",This,pmkObjectName,pfiletime);

    if (pmkObjectName==NULL || pfiletime==NULL)
        return E_INVALIDARG;

    /* verify if the object was registered before or not */
    if (RunningObjectTableImpl_GetObjectIndex(This,-1,pmkObjectName,&index)==S_FALSE)
        return MK_E_UNAVAILABLE;

    (*pfiletime)= This->runObjTab[index].lastModifObj;

    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_EnumRunning
 *
 * PARAMS
 *  ppenumMoniker  [out]  receives the IEnumMoniker interface pointer 
 */
static HRESULT WINAPI
RunningObjectTableImpl_EnumRunning(IRunningObjectTable* iface,
                                   IEnumMoniker **ppenumMoniker)
{
    /* create the unique instance of the EnumMonkikerImpl structure 
     * and copy the Monikers referenced in the ROT so that they can be 
     * enumerated by the Enum interface
     */
    HRESULT rc = 0;
    RunningObjectTableImpl *This = (RunningObjectTableImpl *)iface;

    rc=EnumMonikerImpl_CreateEnumROTMoniker(This->runObjTab, 
					 This->runObjTabSize,
					 This->runObjTabLastIndx, 0, 
					 ppenumMoniker);
    return rc;
}

/***********************************************************************
 *        GetObjectIndex
 */
static HRESULT WINAPI
RunningObjectTableImpl_GetObjectIndex(RunningObjectTableImpl* This,
               DWORD identReg, IMoniker* pmk, DWORD *indx)
{

    DWORD i;

    TRACE("(%p,%ld,%p,%p)\n",This,identReg,pmk,indx);

    if (pmk!=NULL)
        /* search object identified by a moniker */
        for(i=0 ; (i < This->runObjTabLastIndx) &&(!IMoniker_IsEqual(This->runObjTab[i].pmkObj,pmk)==S_OK);i++);
    else
        /* search object identified by a register identifier */
        for(i=0;((i<This->runObjTabLastIndx)&&(This->runObjTab[i].identRegObj!=identReg));i++);

    if (i==This->runObjTabLastIndx)  return S_FALSE;

    if (indx != NULL)  *indx=i;

    return S_OK;
}

/******************************************************************************
 *		GetRunningObjectTable (OLE2.30)
 */
HRESULT WINAPI
GetRunningObjectTable16(DWORD reserved, LPRUNNINGOBJECTTABLE *pprot)
{
    FIXME("(%ld,%p),stub!\n",reserved,pprot);
    return E_NOTIMPL;
}

/***********************************************************************
 *           GetRunningObjectTable (OLE32.@)
 */
HRESULT WINAPI
GetRunningObjectTable(DWORD reserved, LPRUNNINGOBJECTTABLE *pprot)
{
    IID riid=IID_IRunningObjectTable;
    HRESULT res;

    TRACE("()\n");

    if (reserved!=0)
        return E_UNEXPECTED;

    if(runningObjectTableInstance==NULL)
        return CO_E_NOTINITIALIZED;

    res = IRunningObjectTable_QueryInterface((IRunningObjectTable*)runningObjectTableInstance,&riid,(void**)pprot);

    return res;
}

/******************************************************************************
 *              OleRun        [OLE32.@]
 */
HRESULT WINAPI OleRun(LPUNKNOWN pUnknown)
{
    IRunnableObject *runable;
    IRunnableObject *This = (IRunnableObject *)pUnknown;
    LRESULT ret;

    ret = IRunnableObject_QueryInterface(This,&IID_IRunnableObject,(LPVOID*)&runable);
    if (ret)
        return 0; /* Appears to return no error. */
    ret = IRunnableObject_Run(runable,NULL);
    IRunnableObject_Release(runable);
    return ret;
}

/******************************************************************************
 *              MkParseDisplayName        [OLE32.@]
 */
HRESULT WINAPI MkParseDisplayName(LPBC pbc, LPCOLESTR szUserName,
				LPDWORD pchEaten, LPMONIKER *ppmk)
{
    FIXME("(%p, %s, %p, %p): stub.\n", pbc, debugstr_w(szUserName), pchEaten, *ppmk);

    if (!(IsValidInterface((LPUNKNOWN) pbc)))
        return E_INVALIDARG;

    return MK_E_SYNTAX;
}

/******************************************************************************
 *              CreateClassMoniker        [OLE32.@]
 */
HRESULT WINAPI CreateClassMoniker(REFCLSID rclsid, IMoniker ** ppmk)
{
    FIXME("%s\n", debugstr_guid( rclsid ));
    if( ppmk )
        *ppmk = NULL;
    return E_NOTIMPL;
}

/* Virtual function table for the IRunningObjectTable class. */
static IRunningObjectTableVtbl VT_RunningObjectTableImpl =
{
    RunningObjectTableImpl_QueryInterface,
    RunningObjectTableImpl_AddRef,
    RunningObjectTableImpl_Release,
    RunningObjectTableImpl_Register,
    RunningObjectTableImpl_Revoke,
    RunningObjectTableImpl_IsRunning,
    RunningObjectTableImpl_GetObject,
    RunningObjectTableImpl_NoteChangeTime,
    RunningObjectTableImpl_GetTimeOfLastChange,
    RunningObjectTableImpl_EnumRunning
};

/***********************************************************************
 *        RunningObjectTable_Initialize
 */
HRESULT WINAPI RunningObjectTableImpl_Initialize(void)
{
    TRACE("\n");

    /* create the unique instance of the RunningObjectTableImpl structure */
    runningObjectTableInstance = HeapAlloc(GetProcessHeap(), 0, sizeof(RunningObjectTableImpl));

    if (runningObjectTableInstance == 0)
        return E_OUTOFMEMORY;

    /* initialize the virtual table function */
    runningObjectTableInstance->lpVtbl = &VT_RunningObjectTableImpl;

    /* the initial reference is set to "1" ! because if set to "0" it will be not practis when */
    /* the ROT referred many times not in the same time (all the objects in the ROT will  */
    /* be removed every time the ROT is removed ) */
    runningObjectTableInstance->ref = 1;

    /* allocate space memory for the table which contains all the running objects */
    runningObjectTableInstance->runObjTab = HeapAlloc(GetProcessHeap(), 0, sizeof(RunObject[BLOCK_TAB_SIZE]));

    if (runningObjectTableInstance->runObjTab == NULL)
        return E_OUTOFMEMORY;

    runningObjectTableInstance->runObjTabSize=BLOCK_TAB_SIZE;
    runningObjectTableInstance->runObjTabRegister=1;
    runningObjectTableInstance->runObjTabLastIndx=0;

    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_UnInitialize
 */
HRESULT WINAPI RunningObjectTableImpl_UnInitialize()
{
    TRACE("\n");

    if (runningObjectTableInstance==NULL)
        return E_POINTER;

    RunningObjectTableImpl_Release((IRunningObjectTable*)runningObjectTableInstance);

    RunningObjectTableImpl_Destroy();

    return S_OK;
}

/***********************************************************************
 *        EnumMoniker_QueryInterface
 */
static HRESULT WINAPI EnumMonikerImpl_QueryInterface(IEnumMoniker* iface,REFIID riid,void** ppvObject)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;

    TRACE("(%p,%p,%p)\n",This,riid,ppvObject);

    /* validate arguments */
    if (ppvObject == NULL)
        return E_INVALIDARG;

    *ppvObject = NULL;

    if (IsEqualIID(&IID_IUnknown, riid))
        *ppvObject = (IEnumMoniker*)This;
    else
        if (IsEqualIID(&IID_IEnumMoniker, riid))
            *ppvObject = (IEnumMoniker*)This;

    if ((*ppvObject)==NULL)
        return E_NOINTERFACE;

    IEnumMoniker_AddRef(iface);

    return S_OK;
}

/***********************************************************************
 *        EnumMoniker_AddRef
 */
static ULONG   WINAPI EnumMonikerImpl_AddRef(IEnumMoniker* iface)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);
}

/***********************************************************************
 *        EnumMoniker_release
 */
static ULONG   WINAPI EnumMonikerImpl_Release(IEnumMoniker* iface)
{
    DWORD i;
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;
    ULONG ref;

    TRACE("(%p)\n",This);

    ref = InterlockedDecrement(&This->ref);

    /* unitialize rot structure if there's no more reference to it*/
    if (ref == 0) {

        /* release all registered objects in Moniker list */
        for(i=0; i < This->TabLastIndx ;i++)
        {
            IMoniker_Release(This->TabMoniker[i].pmkObj);
        }

        /* there're no more elements in the table */

	TRACE("(%p) Deleting\n",This);
	HeapFree (GetProcessHeap(), 0, This->TabMoniker); /* free Moniker list */
	HeapFree (GetProcessHeap(), 0, This);		  /* free Enum Instance */
	
    }

    return ref;
}
/***********************************************************************
 *        EnmumMoniker_Next
 */
static HRESULT   WINAPI EnumMonikerImpl_Next(IEnumMoniker* iface, ULONG celt, IMoniker** rgelt, ULONG * pceltFetched)
{
    ULONG i;
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;
    TRACE("(%p) TabCurrentPos %ld Tablastindx %ld\n",This, This->TabCurrentPos, This->TabLastIndx);

    /* retrieve the requested number of moniker from the current position */
    for(i=0; (This->TabCurrentPos < This->TabLastIndx) && (i < celt); i++)
	rgelt[i]=(IMoniker*)This->TabMoniker[This->TabCurrentPos++].pmkObj;

    if (pceltFetched!=NULL)
        *pceltFetched= i;

    if (i==celt)
        return S_OK;
    else
        return S_FALSE;

}

/***********************************************************************
 *        EnmumMoniker_Skip
 */
static HRESULT   WINAPI EnumMonikerImpl_Skip(IEnumMoniker* iface, ULONG celt)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;

    TRACE("(%p)\n",This);

    if  (This->TabCurrentPos+celt >= This->TabLastIndx)
	return S_FALSE;

    This->TabCurrentPos+=celt;

    return S_OK;
}

/***********************************************************************
 *        EnmumMoniker_Reset
 */
static HRESULT   WINAPI EnumMonikerImpl_Reset(IEnumMoniker* iface)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;

    This->TabCurrentPos = 0;	/* set back to start of list */

    TRACE("(%p)\n",This);

    return S_OK;
}

/***********************************************************************
 *        EnmumMoniker_Clone
 */
static HRESULT   WINAPI EnumMonikerImpl_Clone(IEnumMoniker* iface, IEnumMoniker ** ppenum)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;

    TRACE("(%p)\n",This);
    /* copy the enum structure */ 
    return EnumMonikerImpl_CreateEnumROTMoniker(This->TabMoniker, This->TabSize,
					 This->TabLastIndx, This->TabCurrentPos,
					 ppenum);
}

/* Virtual function table for the IEnumMoniker class. */
static IEnumMonikerVtbl VT_EnumMonikerImpl =
{
    EnumMonikerImpl_QueryInterface,
    EnumMonikerImpl_AddRef,
    EnumMonikerImpl_Release,
    EnumMonikerImpl_Next,
    EnumMonikerImpl_Skip,
    EnumMonikerImpl_Reset,
    EnumMonikerImpl_Clone
};

/***********************************************************************
 *        EnumMonikerImpl_CreateEnumROTMoniker
 *        Used by EnumRunning to create the structure and EnumClone
 *	  to copy the structure
 */
HRESULT WINAPI EnumMonikerImpl_CreateEnumROTMoniker(RunObject* TabMoniker,
                                                 ULONG TabSize,
						 ULONG TabLastIndx,
                                                 ULONG TabCurrentPos,
                                                 IEnumMoniker ** ppenumMoniker)
{
    int i;
    EnumMonikerImpl* This = NULL;

    if (TabCurrentPos > TabSize)
	return E_INVALIDARG;

    if (ppenumMoniker == NULL)
        return E_INVALIDARG;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(EnumMonikerImpl));

    if (!ppenumMoniker) return E_OUTOFMEMORY;

    TRACE("(%p)\n", This);

    /* initialize the virtual table function */
    This->lpVtbl = &VT_EnumMonikerImpl;

    /* the initial reference is set to "1" */
    This->ref = 1;			/* set the ref count to one         */
    This->TabCurrentPos=0;		/* Set the list start posn to start */
    This->TabSize=TabSize; 		/* Need the same size table as ROT */
    This->TabLastIndx=TabLastIndx; 	/* end element */
    This->TabMoniker=HeapAlloc(GetProcessHeap(),0,This->TabSize*sizeof(RunObject));

    if (This->TabMoniker==NULL) {
        HeapFree(GetProcessHeap(), 0, This);
        return E_OUTOFMEMORY;
    }
    for (i=0; i < This->TabLastIndx; i++){

        This->TabMoniker[i]=TabMoniker[i];
        IMoniker_AddRef(TabMoniker[i].pmkObj);
    }

    *ppenumMoniker =  (IEnumMoniker*)This;

    return S_OK;
}
