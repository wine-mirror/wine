/*
 *	TYPELIB
 *
 *	Copyright 1997	Marcus Meissner
 *		      1999  Rein Klazes
 * there is much left to do here before it can be usefull for real world
 * programs
 * know problems:
 * -. Only one format of typelibs is supported
 * -. All testing until sofar is done using special written windows programs
 * -. Data structures are straightforward, but slow for look-ups.
 * -. (related) nothing is hashed
 * -. a typelib is always read in its entirely into memory and never released.
 * -. there are a number of stubs in ITypeLib and ITypeInfo interfaces. Most
 *      of them I don't know yet how to implement them.
 * -. Most error return values are just guessed not checked with windows
 *      behaviour.
 * -. all locale stuf ignored
 * -. move stuf to wine/dlls
 * -. didn't bother with a c++ interface
 * -. lousy fatal error handling
 * -. some methods just return pointers to internal data structures, this is
 *      partly laziness, partly I want to check how windows does it.
 * 
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "winerror.h"
#include "winreg.h"         /* for HKEY_LOCAL_MACHINE */
#include "winnls.h"         /* for PRIMARYLANGID */
#include "wine/winbase16.h" /* for RegQueryValue16(HKEY,LPSTR,LPSTR,LPDWORD) */
#include "heap.h"
#include "wine/obj_base.h"
#include "debugtools.h"
#include "winversion.h"
/* FIXME: get rid of these */
typedef struct ITypeInfoVtbl ITypeLib_VTable, *LPTYPEINFO_VTABLE ; 
typedef struct ITypeLibVtbl *LPTYPELIB_VTABLE  ; 
#include "typelib.h"

DEFAULT_DEBUG_CHANNEL(ole)
DECLARE_DEBUG_CHANNEL(typelib)

/****************************************************************************
 *		QueryPathOfRegTypeLib16	[TYPELIB.14]
 *
 * the path is "Classes\Typelib\<guid>\<major>.<minor>\<lcid>\win16\"
 * RETURNS
 *	path of typelib
 */
HRESULT WINAPI
QueryPathOfRegTypeLib16(	
	REFGUID guid,	/* [in] referenced guid */
	WORD wMaj,	/* [in] major version */
	WORD wMin,	/* [in] minor version */
	LCID lcid,	/* [in] locale id */
	LPBSTR16 path	/* [out] path of typelib */
) {
	char	xguid[80];
	char	typelibkey[100],pathname[260];
	DWORD	plen;

	if (HIWORD(guid)) {
		WINE_StringFromCLSID(guid,xguid);
		sprintf(typelibkey,"SOFTWARE\\Classes\\Typelib\\%s\\%d.%d\\%lx\\win16",
			xguid,wMaj,wMin,lcid
		);
	} else {
		sprintf(xguid,"<guid 0x%08lx>",(DWORD)guid);
		FIXME_(ole)("(%s,%d,%d,0x%04lx,%p),can't handle non-string guids.\n",xguid,wMaj,wMin,(DWORD)lcid,path);
		return E_FAIL;
	}
	plen = sizeof(pathname);
	if (RegQueryValue16(HKEY_LOCAL_MACHINE,typelibkey,pathname,&plen)) {
		/* try again without lang specific id */
		if (SUBLANGID(lcid))
			return QueryPathOfRegTypeLib16(guid,wMaj,wMin,PRIMARYLANGID(lcid),path);
		FIXME_(ole)("key %s not found\n",typelibkey);
		return E_FAIL;
	}
	*path = SysAllocString16(pathname);
	return S_OK;
}
 
/****************************************************************************
 *		QueryPathOfRegTypeLib	[OLEAUT32.164]
 * RETURNS
 *	path of typelib
 */
HRESULT WINAPI
QueryPathOfRegTypeLib(	
	REFGUID guid,	/* [in] referenced guid */
	WORD wMaj,	/* [in] major version */
	WORD wMin,	/* [in] minor version */
	LCID lcid,	/* [in] locale id */
	LPBSTR path	/* [out] path of typelib */
) {
	char	xguid[80];
	char	typelibkey[100],pathname[260];
	DWORD	plen;


	if (HIWORD(guid)) {
		WINE_StringFromCLSID(guid,xguid);
		sprintf(typelibkey,"SOFTWARE\\Classes\\Typelib\\%s\\%d.%d\\%lx\\win32",
			xguid,wMaj,wMin,lcid
		);
	} else {
		sprintf(xguid,"<guid 0x%08lx>",(DWORD)guid);
		FIXME_(ole)("(%s,%d,%d,0x%04lx,%p),stub!\n",xguid,wMaj,wMin,(DWORD)lcid,path);
		return E_FAIL;
	}
	plen = sizeof(pathname);
	if (RegQueryValue16(HKEY_LOCAL_MACHINE,typelibkey,pathname,&plen)) {
		/* try again without lang specific id */
		if (SUBLANGID(lcid))
			return QueryPathOfRegTypeLib(guid,wMaj,wMin,PRIMARYLANGID(lcid),path);
		FIXME_(ole)("key %s not found\n",typelibkey);
		return E_FAIL;
	}
	*path = HEAP_strdupAtoW(GetProcessHeap(),0,pathname);
	return S_OK;
}

/******************************************************************************
 * LoadTypeLib [TYPELIB.3]  Loads and registers a type library
 * NOTES
 *    Docs: OLECHAR FAR* szFile
 *    Docs: iTypeLib FAR* FAR* pptLib
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: Status
 */
HRESULT WINAPI LoadTypeLib16(
    LPOLESTR szFile, /* [in] Name of file to load from */
    void * *pptLib) /* [out] Pointer to pointer to loaded type library */
{
    FIXME_(ole)("('%s',%p): stub\n",debugstr_w((LPWSTR)szFile),pptLib);

    if (pptLib!=0)
      *pptLib=0;

    return E_FAIL;
}

/******************************************************************************
 *		LoadTypeLib	[OLEAUT32.161]
 * Loads and registers a type library
 * NOTES
 *    Docs: OLECHAR FAR* szFile
 *    Docs: iTypeLib FAR* FAR* pptLib
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: Status
 */
int TLB_ReadTypeLib(PCHAR file, ITypeLib **ppTypelib);
HRESULT WINAPI LoadTypeLib(
    OLECHAR *szFile,   /* [in] Name of file to load from */
    ITypeLib * *pptLib) /* [out] Pointer to pointer to loaded type library */
{
    return LoadTypeLibEx(szFile, REGKIND_DEFAULT, pptLib);
}

/******************************************************************************
 *		LoadTypeLibEx	[OLEAUT32.183]
 * Loads and optionally registers a type library
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: Status
 */
HRESULT WINAPI LoadTypeLibEx(
    LPOLESTR szFile,   /* [in] Name of file to load from */
    REGKIND  regkind,  /* specify kind of registration */
    ITypeLib **pptLib) /* [out] Pointer to pointer to loaded type library */
{
    LPSTR p;
    HRESULT res;
    TRACE_(typelib)("('%s',%d,%p)\n",debugstr_w(szFile), regkind, pptLib);
    
    p=HEAP_strdupWtoA(GetProcessHeap(),0,szFile);
    
    if(regkind != REGKIND_NONE)
        FIXME_(typelib) ("registration of typelibs not supported yet!\n");

    res= TLB_ReadTypeLib(p, pptLib);
    /* XXX need to free p ?? */

    TRACE_(typelib)(" returns %ld\n",res);

    return res;
}

/******************************************************************************
 *		LoadRegTypeLib	[OLEAUT32.162]
 */
HRESULT WINAPI LoadRegTypeLib(	
	REFGUID rguid,	/* [in] referenced guid */
	WORD wVerMajor,	/* [in] major version */
	WORD wVerMinor,	/* [in] minor version */
	LCID lcid,	/* [in] locale id */
	ITypeLib **ppTLib	/* [out] path of typelib */
) {
    BSTR bstr=NULL;
    HRESULT res=QueryPathOfRegTypeLib( rguid, wVerMajor, wVerMinor,
		    lcid, &bstr);
    if(SUCCEEDED(res)){
        res= LoadTypeLib(bstr, ppTLib);
        SysFreeString(bstr);
    }
    if(TRACE_ON(typelib)){
        char xriid[50];
        WINE_StringFromCLSID((LPCLSID)rguid,xriid);
        TRACE_(typelib)("(IID: %s) load %s (%p)\n",xriid,
                SUCCEEDED(res)? "SUCCESS":"FAILED", *ppTLib);
    }
    return res;
}	


/******************************************************************************
 *		RegisterTypeLib	[OLEAUT32.163]
 * Adds information about a type library to the System Registry           
 * NOTES
 *    Docs: ITypeLib FAR * ptlib
 *    Docs: OLECHAR FAR* szFullPath
 *    Docs: OLECHAR FAR* szHelpDir
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: Status
 */
HRESULT WINAPI RegisterTypeLib(
     ITypeLib * ptlib,      /*[in] Pointer to the library*/
     OLECHAR * szFullPath, /*[in] full Path of the library*/
     OLECHAR * szHelpDir)  /*[in] dir to the helpfile for the library,
							 may be NULL*/
{   FIXME_(ole)("(%p,%s,%s): stub\n",ptlib, debugstr_w(szFullPath),debugstr_w(szHelpDir));
    return S_OK;	/* FIXME: pretend everything is OK */
}


/******************************************************************************
 *	UnRegisterTypeLib	[OLEAUT32.186]
 * Removes information about a type library from the System Registry           
 * NOTES
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: Status
 */
HRESULT WINAPI UnRegisterTypeLib(
    REFGUID libid,	/* [in] Guid of the library */
	WORD wVerMajor,	/* [in] major version */
	WORD wVerMinor,	/* [in] minor version */
	LCID lcid,	/* [in] locale id */
	SYSKIND syskind)
{   
    char xriid[50];
    WINE_StringFromCLSID((LPCLSID)libid,xriid);
    TRACE_(typelib)("(IID: %s): stub\n",xriid);
    return S_OK;	/* FIXME: pretend everything is OK */
}

/****************************************************************************
 *	OABuildVersion				(TYPELIB.15)
 * RETURNS
 *	path of typelib
 */
DWORD WINAPI OABuildVersion16(void)
{
WINDOWS_VERSION ver = VERSION_GetVersion();

    switch (ver) {
      case WIN95:
	return MAKELONG(0xbd0, 0xa); /* Win95A */
      case WIN31:
	return MAKELONG(0xbd3, 0x3); /* WfW 3.11 */
      default:
	FIXME_(ole)("Version value not known yet. Please investigate it !");
	return MAKELONG(0xbd0, 0xa); /* return Win95A for now */
    }
}

/* for better debugging info leave the static out for the time being */
#define static

/*=======================Itypelib methods ===============================*/
/* ITypeLib methods */
static HRESULT WINAPI ITypeLib_fnQueryInterface( LPTYPELIB This, REFIID riid,
    VOID **ppvObject); 
static ULONG WINAPI ITypeLib_fnAddRef( LPTYPELIB This); 
static ULONG WINAPI ITypeLib_fnRelease( LPTYPELIB This); 
static UINT WINAPI ITypeLib_fnGetTypeInfoCount( LPTYPELIB This); 
static HRESULT WINAPI ITypeLib_fnGetTypeInfo( LPTYPELIB This, UINT index, 
    ITypeInfo **ppTInfo); 

static HRESULT WINAPI ITypeLib_fnGetTypeInfoType( LPTYPELIB This, UINT index,
    TYPEKIND *pTKind); 

static HRESULT WINAPI ITypeLib_fnGetTypeInfoOfGuid( LPTYPELIB This, REFGUID guid,
    ITypeInfo **ppTinfo);

static HRESULT WINAPI ITypeLib_fnGetLibAttr( LPTYPELIB This, 
    LPTLIBATTR *ppTLibAttr); 

static HRESULT WINAPI ITypeLib_fnGetTypeComp( LPTYPELIB This,
    ITypeComp **ppTComp); 

static HRESULT WINAPI ITypeLib_fnGetDocumentation( LPTYPELIB This, INT index,
    BSTR *pBstrName, BSTR *pBstrDocString, DWORD *pdwHelpContext, 
    BSTR *pBstrHelpFile);

static HRESULT WINAPI ITypeLib_fnIsName( LPTYPELIB This, LPOLESTR szNameBuf,
    ULONG lHashVal, BOOL *pfName);

static HRESULT WINAPI ITypeLib_fnFindName( LPTYPELIB This, LPOLESTR szNameBuf,
    ULONG lHashVal, ITypeInfo **ppTInfo, MEMBERID *rgMemId, UINT16 *pcFound);

static VOID WINAPI ITypeLib_fnReleaseTLibAttr( LPTYPELIB This,
		TLIBATTR *pTLibAttr);

static HRESULT WINAPI ITypeLib2_fnGetCustData( ITypeLib * This, REFGUID guid, 
        VARIANT *pVarVal); 

static HRESULT WINAPI ITypeLib2_fnGetLibStatistics( ITypeLib * This, 
        UINT *pcUniqueNames, UINT *pcchUniqueNames); 

static HRESULT WINAPI ITypeLib2_fnGetDocumentation2( ITypeLib * This, 
        INT index, LCID lcid, BSTR *pbstrHelpString,
        INT *pdwHelpStringContext, BSTR *pbstrHelpStringDll); 

static HRESULT WINAPI ITypeLib2_fnGetAllCustData( ITypeLib * This,
        CUSTDATA *pCustData);
static ICOM_VTABLE(ITypeLib) tlbvt = {
    ITypeLib_fnQueryInterface,
    ITypeLib_fnAddRef,
    ITypeLib_fnRelease,
    ITypeLib_fnGetTypeInfoCount,
    ITypeLib_fnGetTypeInfo,
    ITypeLib_fnGetTypeInfoType,
    ITypeLib_fnGetTypeInfoOfGuid,
    ITypeLib_fnGetLibAttr,
    ITypeLib_fnGetTypeComp,
    ITypeLib_fnGetDocumentation,
    ITypeLib_fnIsName,
    ITypeLib_fnFindName,
    ITypeLib_fnReleaseTLibAttr,
    ITypeLib2_fnGetCustData,
    ITypeLib2_fnGetLibStatistics,
    ITypeLib2_fnGetDocumentation2,
    ITypeLib2_fnGetAllCustData
 };
/* TypeInfo Methods */

static HRESULT WINAPI ITypeInfo_fnQueryInterface( LPTYPEINFO This, REFIID riid,
    VOID **ppvObject); 
static ULONG WINAPI ITypeInfo_fnAddRef( LPTYPEINFO This); 
static ULONG WINAPI ITypeInfo_fnRelease( LPTYPEINFO This); 
static HRESULT WINAPI ITypeInfo_fnGetTypeAttr( LPTYPEINFO This,
        LPTYPEATTR  *ppTypeAttr); 

static HRESULT WINAPI ITypeInfo_fnGetTypeComp( LPTYPEINFO This,
        ITypeComp  * *ppTComp); 

static HRESULT WINAPI ITypeInfo_fnGetFuncDesc( LPTYPEINFO This, UINT index,
        LPFUNCDESC  *ppFuncDesc); 

static HRESULT WINAPI ITypeInfo_fnGetVarDesc( LPTYPEINFO This, UINT index,
        LPVARDESC  *ppVarDesc); 

static HRESULT WINAPI ITypeInfo_fnGetNames( LPTYPEINFO This, MEMBERID memid,
        BSTR  *rgBstrNames, UINT cMaxNames, UINT  *pcNames);


static HRESULT WINAPI ITypeInfo_fnGetRefTypeOfImplType( LPTYPEINFO This,
        UINT index, HREFTYPE  *pRefType); 

static HRESULT WINAPI ITypeInfo_fnGetImplTypeFlags( LPTYPEINFO This,
        UINT index, INT  *pImplTypeFlags); 

static HRESULT WINAPI ITypeInfo_fnGetIDsOfNames( LPTYPEINFO This,
        LPOLESTR  *rgszNames, UINT cNames, MEMBERID  *pMemId); 

static HRESULT WINAPI ITypeInfo_fnInvoke( LPTYPEINFO This, VOID  *pIUnk,
        MEMBERID memid, UINT16 dwFlags, DISPPARAMS  *pDispParams, 
        VARIANT  *pVarResult, EXCEPINFO  *pExcepInfo, UINT  *pArgErr);

static HRESULT WINAPI ITypeInfo_fnGetDocumentation( LPTYPEINFO This,
        MEMBERID memid, BSTR  *pBstrName, BSTR  *pBstrDocString, 
        DWORD  *pdwHelpContext, BSTR  *pBstrHelpFile); 

static HRESULT WINAPI ITypeInfo_fnGetDllEntry( LPTYPEINFO This,
        MEMBERID memid, INVOKEKIND invKind, BSTR  *pBstrDllName,
        BSTR  *pBstrName, WORD  *pwOrdinal); 

static HRESULT WINAPI ITypeInfo_fnGetRefTypeInfo( LPTYPEINFO This,
        HREFTYPE hRefType, ITypeInfo  * *ppTInfo); 

static HRESULT WINAPI ITypeInfo_fnAddressOfMember( LPTYPEINFO This,
        MEMBERID memid, INVOKEKIND invKind, PVOID *ppv); 

static HRESULT WINAPI ITypeInfo_fnCreateInstance( LPTYPEINFO This, 
        IUnknown *pUnk, REFIID riid, VOID  * *ppvObj); 

static HRESULT WINAPI ITypeInfo_fnGetMops( LPTYPEINFO This, MEMBERID memid, 
        BSTR  *pBstrMops);


static HRESULT WINAPI ITypeInfo_fnGetContainingTypeLib( LPTYPEINFO This,
        ITypeLib  * *ppTLib, UINT  *pIndex); 

static HRESULT WINAPI ITypeInfo_fnReleaseTypeAttr( LPTYPEINFO This,
        TYPEATTR *pTypeAttr); 

static HRESULT WINAPI ITypeInfo_fnReleaseFuncDesc( LPTYPEINFO This,
        FUNCDESC *pFuncDesc); 

static HRESULT WINAPI ITypeInfo_fnReleaseVarDesc( LPTYPEINFO This,
        VARDESC *pVarDesc); 
/* itypeinfo2 methods */
static HRESULT WINAPI ITypeInfo2_fnGetTypeKind( ITypeInfo * This,
    TYPEKIND *pTypeKind);
static HRESULT WINAPI ITypeInfo2_fnGetTypeFlags( ITypeInfo * This,
    UINT *pTypeFlags);
static HRESULT WINAPI ITypeInfo2_fnGetFuncIndexOfMemId( ITypeInfo * This,
    MEMBERID memid, INVOKEKIND invKind, UINT *pFuncIndex);
static HRESULT WINAPI ITypeInfo2_fnGetVarIndexOfMemId( ITypeInfo * This,
    MEMBERID memid, UINT *pVarIndex);
static HRESULT WINAPI ITypeInfo2_fnGetCustData( ITypeInfo * This,
    REFGUID guid, VARIANT *pVarVal);
static HRESULT WINAPI ITypeInfo2_fnGetFuncCustData( ITypeInfo * This,
    UINT index, REFGUID guid, VARIANT *pVarVal);
static HRESULT WINAPI ITypeInfo2_fnGetParamCustData( ITypeInfo * This,
    UINT indexFunc, UINT indexParam, REFGUID guid, VARIANT *pVarVal);
static HRESULT WINAPI ITypeInfo2_fnGetVarCustData( ITypeInfo * This,
    UINT index, REFGUID guid, VARIANT *pVarVal);
static HRESULT WINAPI ITypeInfo2_fnGetImplTypeCustData( ITypeInfo * This,
    UINT index, REFGUID guid, VARIANT *pVarVal);
static HRESULT WINAPI ITypeInfo2_fnGetDocumentation2( ITypeInfo * This,
    MEMBERID memid, LCID lcid, BSTR *pbstrHelpString,
    INT *pdwHelpStringContext, BSTR *pbstrHelpStringDll);
static HRESULT WINAPI ITypeInfo2_fnGetAllCustData( ITypeInfo * This,
    CUSTDATA *pCustData);
static HRESULT WINAPI ITypeInfo2_fnGetAllFuncCustData( ITypeInfo * This,
    UINT index, CUSTDATA *pCustData);
static HRESULT WINAPI ITypeInfo2_fnGetAllParamCustData( ITypeInfo * This,
    UINT indexFunc, UINT indexParam, CUSTDATA *pCustData);
static HRESULT WINAPI ITypeInfo2_fnGetAllVarCustData( ITypeInfo * This,
    UINT index, CUSTDATA *pCustData);
static HRESULT WINAPI ITypeInfo2_fnGetAllImplTypeCustData( ITypeInfo * This,
    UINT index, CUSTDATA *pCustData);

static ICOM_VTABLE(ITypeInfo) tinfvt = {
    ITypeInfo_fnQueryInterface,
    ITypeInfo_fnAddRef,
    ITypeInfo_fnRelease,
    ITypeInfo_fnGetTypeAttr,
    ITypeInfo_fnGetTypeComp,
    ITypeInfo_fnGetFuncDesc,
    ITypeInfo_fnGetVarDesc,
    ITypeInfo_fnGetNames,
    ITypeInfo_fnGetRefTypeOfImplType,
    ITypeInfo_fnGetImplTypeFlags,
    ITypeInfo_fnGetIDsOfNames,
    ITypeInfo_fnInvoke,
    ITypeInfo_fnGetDocumentation,
    ITypeInfo_fnGetDllEntry,
    ITypeInfo_fnGetRefTypeInfo,
    ITypeInfo_fnAddressOfMember,
    ITypeInfo_fnCreateInstance,
    ITypeInfo_fnGetMops,
    ITypeInfo_fnGetContainingTypeLib,
    ITypeInfo_fnReleaseTypeAttr,
    ITypeInfo_fnReleaseFuncDesc,
    ITypeInfo_fnReleaseVarDesc,

    ITypeInfo2_fnGetTypeKind,
    ITypeInfo2_fnGetTypeFlags,
    ITypeInfo2_fnGetFuncIndexOfMemId,
    ITypeInfo2_fnGetVarIndexOfMemId,
    ITypeInfo2_fnGetCustData,
    ITypeInfo2_fnGetFuncCustData,
    ITypeInfo2_fnGetParamCustData,
    ITypeInfo2_fnGetVarCustData,
    ITypeInfo2_fnGetImplTypeCustData,
    ITypeInfo2_fnGetDocumentation2,
    ITypeInfo2_fnGetAllCustData,
    ITypeInfo2_fnGetAllFuncCustData,
    ITypeInfo2_fnGetAllParamCustData,
    ITypeInfo2_fnGetAllVarCustData,
    ITypeInfo2_fnGetAllImplTypeCustData,

};

static TYPEDESC stndTypeDesc[VT_LPWSTR+1]={/* VT_LPWSTR is largest type that */
										/* may appear in type description*/
	{{0}, 0},{{0}, 1},{{0}, 2},{{0}, 3},{{0}, 4},
        {{0}, 5},{{0}, 6},{{0}, 7},{{0}, 8},{{0}, 9},
    {{0},10},{{0},11},{{0},12},{{0},13},{{0},14},
        {{0},15},{{0},16},{{0},17},{{0},18},{{0},19},
    {{0},20},{{0},21},{{0},22},{{0},23},{{0},24},
        {{0},25},{{0},26},{{0},27},{{0},28},{{0},29},
    {{0},30},{{0},31}};

static void TLB_abort()
{
		*((int *)0)=0;
}
static void * TLB_Alloc(unsigned size)
{
    void * ret;
    if((ret=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,size))==NULL){
        /* FIXME */
        ERR_(ole)("cannot allocate memory\n");
    }
    return ret;
}

/* candidate for a more global appearance... */
static BSTR  TLB_DupAtoBstr(PCHAR Astr)
{
    int len;
    BSTR bstr;
    DWORD *pdw  ;
    if(!Astr)
        return NULL;
    len=strlen(Astr);
    pdw  =TLB_Alloc((len+3)*sizeof(OLECHAR));
    pdw[0]=(len)*sizeof(OLECHAR);
    bstr=(BSTR)&( pdw[1]);
    lstrcpyAtoW( bstr, Astr);
    TRACE_(typelib)("copying %s to (%p)\n", Astr, bstr);
    return bstr;
}

static void TLB_Free(void * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}
/* read function */
DWORD TLB_Read(void *buffer,  DWORD count, TLBContext *pcx, long where )
{
	DWORD bytesread=0;
	
	if ((   where != DO_NOT_SEEK &&
		(0xffffffff == SetFilePointer( pcx->hFile, where + pcx->oStart,
					       0,FILE_BEGIN))
	    ) ||
	    !ReadFile(pcx->hFile, buffer, count, &bytesread, NULL)
	) {
        /* FIXME */
		ERR_(typelib)("read error is 0x%lx reading %ld bytes at 0x%lx\n",
			    GetLastError(), count, where);
		TLB_abort();
		exit(1);
	}
	return bytesread;
}

static void TLB_ReadGuid( GUID *pGuid, int offset,   TLBContext *pcx)
{
    if(offset<0 || pcx->pTblDir->pGuidTab.offset <0){
        memset(pGuid,0, sizeof(GUID));
        return;
    }
    TLB_Read(pGuid, sizeof(GUID), pcx, pcx->pTblDir->pGuidTab.offset+offset );
}

PCHAR TLB_ReadName( TLBContext *pcx, int offset)
{
    char * name;
    TLBNameIntro niName;
    TLB_Read(&niName, sizeof(niName), pcx,
				pcx->pTblDir->pNametab.offset+offset);
    niName.namelen &= 0xFF; /* FIXME: correct ? */
    name=TLB_Alloc((niName.namelen & 0xff) +1);
    TLB_Read(name, (niName.namelen & 0xff), pcx, DO_NOT_SEEK);
    name[niName.namelen & 0xff]='\0';
    return name;
}
PCHAR TLB_ReadString( TLBContext *pcx, int offset)
{
    char * string;
    INT16 length;
    if(offset<0) return NULL;
    TLB_Read(&length, sizeof(INT16), pcx, pcx->pTblDir->pStringtab.offset+offset);
    if(length <= 0) return 0;
    string=TLB_Alloc(length +1);
    TLB_Read(string, length, pcx, DO_NOT_SEEK);
    string[length]='\0';
    return string;
}
/*
 * read a value and fill a VARIANT structure 
 */
static void TLB_ReadValue( VARIANT * pVar, int offset, TLBContext *pcx )
{
    int size;
    if(offset <0) { /* data is packed in here */
        pVar->vt = (offset & 0x7c000000 )>> 26;
        V_UNION(pVar, iVal) = offset & 0xffff;
        return;
    }
    TLB_Read(&(pVar->vt), sizeof(VARTYPE), pcx, 
        pcx->pTblDir->pCustData.offset + offset );
    switch(pVar->vt){
        case VT_EMPTY:  /* FIXME: is this right? */
        case VT_NULL:   /* FIXME: is this right? */
        case VT_I2  :   /* this should not happen */
        case VT_I4  :
        case VT_R4  :
        case VT_ERROR   : 
        case VT_BOOL    : 
        case VT_I1  : 
        case VT_UI1 : 
        case VT_UI2 : 
        case VT_UI4 : 
        case VT_INT : 
        case VT_UINT    : 
        case VT_VOID    : /* FIXME: is this right? */
        case VT_HRESULT : 
            size=4; break;
        case VT_R8  :
        case VT_CY  :
        case VT_DATE    : 
        case VT_I8  : 
        case VT_UI8 : 
        case VT_DECIMAL :  /* FIXME: is this right? */
        case VT_FILETIME :
            size=8;break;
            /* pointer types with known behaviour */
        case VT_BSTR    :{
            char * ptr;
            TLB_Read(&size, sizeof(INT), pcx, DO_NOT_SEEK );
            ptr=TLB_Alloc(size);/* allocate temp buffer */
            TLB_Read(ptr, size, pcx, DO_NOT_SEEK ); /* read string (ANSI) */
            V_UNION(pVar, bstrVal)=SysAllocStringLen(NULL,size);
            /* FIXME: do we need a AtoW conversion here? */
            V_UNION(pVar, bstrVal[size])=L'\0';
            while(size--) V_UNION(pVar, bstrVal[size])=ptr[size];
            TLB_Free(ptr);
        }
            size=-4; break;
    /* FIXME: this will not work AT ALL when the variant contains a pointer */
        case VT_DISPATCH :
        case VT_VARIANT : 
        case VT_UNKNOWN : 
        case VT_PTR : 
        case VT_SAFEARRAY :
        case VT_CARRAY  : 
        case VT_USERDEFINED : 
        case VT_LPSTR   : 
        case VT_LPWSTR  : 
        case VT_BLOB    : 
        case VT_STREAM  : 
        case VT_STORAGE : 
        case VT_STREAMED_OBJECT : 
        case VT_STORED_OBJECT   : 
        case VT_BLOB_OBJECT : 
        case VT_CF  : 
        case VT_CLSID   : 
        default: 
            size=0; 
            FIXME_(ole)("VARTYPE %d is not supported, setting pointer to NULL\n",
                pVar->vt);
    }

    if(size>0) /* (big|small) endian correct? */
        TLB_Read(&(V_UNION(pVar, iVal)), size, pcx, DO_NOT_SEEK );
        return ;
}
/*
 * create a linked list with custom data
 */
static int TLB_CustData( TLBContext *pcx, int offset, TLBCustData** ppCustData )
{
    TLBCDGuid entry;
    TLBCustData* pNew;
    int count=0;
    while(offset >=0){
        count++;
        pNew=TLB_Alloc(sizeof(TLBCustData));
        TLB_Read(&entry, sizeof(entry), pcx, 
            pcx->pTblDir->pCDGuids.offset+offset);
        TLB_ReadGuid(&(pNew->guid), entry.GuidOffset , pcx);
        TLB_ReadValue(&(pNew->data), entry.DataOffset, pcx);
        /* add new custom data at head of the list */
        pNew->next=*ppCustData;
        *ppCustData=pNew;
        offset = entry.next;
    }
    return count;
}

static void TLB_GetTdesc(TLBContext *pcx, INT type,TYPEDESC * pTd )
{
    if(type <0)
        pTd->vt=type & VT_TYPEMASK;
    else
        *pTd=pcx->pLibInfo->pTypeDesc[type/(2*sizeof(INT))];
}
static void TLB_DoFuncs(TLBContext *pcx, int cFuncs, int cVars,
                          int offset, TLBFuncDesc ** pptfd)
{
    /* 
     * member information is stored in a data structure at offset
     * indicated by the memoffset field of the typeinfo structure
     * There are several distinctive parts.
     * the first part starts with a field that holds the total length 
     * of this (first) part excluding this field. Then follow the records,
     * for each member there is one record.
     *
     * First entry is always the length of the record (excluding this
     * length word). 
     * Rest of the record depends on the type of the member. If there is 
     * a field indicating the member type (function variable intereface etc)
     * I have not found it yet. At this time we depend on the information
     * in the type info and the usual order how things are stored.
     *
     * Second follows an array sized nrMEM*sizeof(INT) with a memeber id
     * for each member;
     * 
     * Third is a equal sized array with file offsets to the name entry 
     * of each member.
     * 
     * Forth and last (?) part is an array with offsets to the records in the
     * first part of this file segment.
     */

    int infolen, nameoffset, reclength, nrattributes;
    char recbuf[512];
    TLBFuncRecord * pFuncRec=(TLBFuncRecord *) recbuf;
    int i, j;
    int recoffset=offset+sizeof(INT);
    TLB_Read(&infolen,sizeof(INT), pcx, offset);
    for(i=0;i<cFuncs;i++){
        *pptfd=TLB_Alloc(sizeof(TLBFuncDesc));
    /* name, eventually add to a hash table */
        TLB_Read(&nameoffset, sizeof(INT), pcx, 
            offset + infolen + (cFuncs + cVars + i + 1) * sizeof(INT));
        (*pptfd)->Name=TLB_ReadName(pcx, nameoffset);
    /* read the function information record */
        TLB_Read(&reclength, sizeof(INT), pcx, recoffset);
        reclength &=0x1ff;
        TLB_Read(pFuncRec, reclength - sizeof(INT), pcx, DO_NOT_SEEK) ; 
    /* do the attributes */
        nrattributes=(reclength-pFuncRec->nrargs*3*sizeof(int)-0x18)
            /sizeof(int);
        if(nrattributes>0){
            (*pptfd)->helpcontext = pFuncRec->OptAttr[0] ;
            if(nrattributes>1){
                (*pptfd)->HelpString = TLB_ReadString(pcx,
                                                      pFuncRec->OptAttr[1]) ;
                if(nrattributes>2){
                    if(pFuncRec->FKCCIC & 0x2000)
                        (*pptfd)->Entry = (char *) pFuncRec->OptAttr[2] ;
                    else
                        (*pptfd)->Entry = TLB_ReadString(pcx,
                                                         pFuncRec->OptAttr[2]);
                    if(nrattributes>5 )
                        (*pptfd)->HelpStringContext = pFuncRec->OptAttr[5] ;
                    if(nrattributes>6 && pFuncRec->FKCCIC & 0x80){
                        TLB_CustData(pcx, pFuncRec->OptAttr[6],
                                &(*pptfd)->pCustData);
                    }
                }
            }
        }
    /* fill the FuncDesc Structure */
        TLB_Read(&(*pptfd)->funcdesc.memid, sizeof(INT), pcx, 
            offset + infolen + ( i + 1) * sizeof(INT));
        (*pptfd)->funcdesc.funckind = (pFuncRec->FKCCIC) & 0x7;
        (*pptfd)->funcdesc.invkind = ((pFuncRec->FKCCIC) >>3) & 0xF;
        (*pptfd)->funcdesc.callconv = (pFuncRec->FKCCIC) >>8 & 0xF;
        (*pptfd)->funcdesc.cParams = pFuncRec->nrargs ;
        (*pptfd)->funcdesc.cParamsOpt = pFuncRec->nroargs ;
        (*pptfd)->funcdesc.oVft = pFuncRec->VtableOffset ;
        (*pptfd)->funcdesc.wFuncFlags = LOWORD(pFuncRec->Flags)  ;
        TLB_GetTdesc(pcx, pFuncRec->DataType,   
            &(*pptfd)->funcdesc.elemdescFunc.tdesc) ;
                                    
        /* do the parameters/arguments */
        if(pFuncRec->nrargs){
            TLBParameterInfo paraminfo;
            (*pptfd)->funcdesc.lprgelemdescParam=
                TLB_Alloc(pFuncRec->nrargs * sizeof(ELEMDESC));
            (*pptfd)->pParamDesc=TLB_Alloc(pFuncRec->nrargs *
                sizeof(TLBParDesc));

            TLB_Read(&paraminfo,sizeof(paraminfo), pcx, recoffset+reclength -
                pFuncRec->nrargs * sizeof(TLBParameterInfo));
            for(j=0;j<pFuncRec->nrargs;j++){
                TLB_GetTdesc(pcx, paraminfo.DataType,   
                    &(*pptfd)->funcdesc.lprgelemdescParam[j].tdesc) ;
                V_UNION(&((*pptfd)->funcdesc.lprgelemdescParam[j]),
                    paramdesc.wParamFlags) = paraminfo.Flags;
                (*pptfd)->pParamDesc[j].Name=(void *)paraminfo.oName;
                TLB_Read(&paraminfo,sizeof(TLBParameterInfo), pcx,
                        DO_NOT_SEEK);
            }
            /* second time around */
            for(j=0;j<pFuncRec->nrargs;j++){
                /* name */
                (*pptfd)->pParamDesc[j].Name=
                    TLB_ReadName(pcx, (int)(*pptfd)->pParamDesc[j].Name);
                /* default value */
                if((PARAMFLAG_FHASDEFAULT & V_UNION(&((*pptfd)->funcdesc.
                    lprgelemdescParam[j]),paramdesc.wParamFlags)) &&
                    ((pFuncRec->FKCCIC) & 0x1000)){
                    INT *pInt=(INT *)((char *)pFuncRec + reclength -
                        (pFuncRec->nrargs * 4 + 1) * sizeof(INT) );
                    PARAMDESC * pParamDesc= &V_UNION(&((*pptfd)->funcdesc.
                        lprgelemdescParam[j]),paramdesc);
                    pParamDesc->pparamdescex = TLB_Alloc(sizeof(PARAMDESCEX));
                    pParamDesc->pparamdescex->cBytes= sizeof(PARAMDESCEX);
                    TLB_ReadValue(&(pParamDesc->pparamdescex->varDefaultValue), 
                        pInt[j], pcx);
                }
                /* custom info */
                if(nrattributes>7+j && pFuncRec->FKCCIC & 0x80)
                    TLB_CustData(pcx, pFuncRec->OptAttr[7+j],
									&(*pptfd)->pParamDesc[j].pCustData);
            }
        }
    /* scode is not used: archaic win16 stuff FIXME: right? */
        (*pptfd)->funcdesc.cScodes = 0 ;
        (*pptfd)->funcdesc.lprgscode = NULL ;
        pptfd=&((*pptfd)->next);
        recoffset += reclength;
    }
}
static void TLB_DoVars(TLBContext *pcx, int cFuncs, int cVars,
                          int offset, TLBVarDesc ** pptvd)
{
    int infolen, nameoffset, reclength;
    char recbuf[256];
    TLBVarRecord * pVarRec=(TLBVarRecord *) recbuf;
    int i;
    int recoffset;
    TLB_Read(&infolen,sizeof(INT), pcx, offset);
    TLB_Read(&recoffset,sizeof(INT), pcx, offset + infolen + 
        ((cFuncs+cVars)*2+cFuncs + 1)*sizeof(INT));
    recoffset += offset+sizeof(INT);
    for(i=0;i<cVars;i++){
        *pptvd=TLB_Alloc(sizeof(TLBVarDesc));
    /* name, eventually add to a hash table */
        TLB_Read(&nameoffset, sizeof(INT), pcx, 
            offset + infolen + (cFuncs + cVars + i + 1) * sizeof(INT));
        (*pptvd)->Name=TLB_ReadName(pcx, nameoffset);
    /* read the variable information record */
        TLB_Read(&reclength, sizeof(INT), pcx, recoffset);
        reclength &=0xff;
        TLB_Read(pVarRec, reclength - sizeof(INT), pcx, DO_NOT_SEEK) ; 
    /* Optional data */
        if(reclength >(6*sizeof(INT)) )
            (*pptvd)->HelpContext=pVarRec->HelpContext;
        if(reclength >(7*sizeof(INT)) )
            (*pptvd)->HelpString = TLB_ReadString(pcx, pVarRec->oHelpString) ;
        if(reclength >(8*sizeof(INT)) )
        if(reclength >(9*sizeof(INT)) )
            (*pptvd)->HelpStringContext=pVarRec->HelpStringContext;
    /* fill the VarDesc Structure */
        TLB_Read(&(*pptvd)->vardesc.memid, sizeof(INT), pcx, 
            offset + infolen + ( i + 1) * sizeof(INT));
        (*pptvd)->vardesc.varkind = pVarRec->VarKind;
        (*pptvd)->vardesc.wVarFlags = pVarRec->Flags;
        TLB_GetTdesc(pcx, pVarRec->DataType,    
            &(*pptvd)->vardesc.elemdescVar.tdesc) ;
/*   (*pptvd)->vardesc.lpstrSchema; is reserved (SDK) fixme?? */
        if(pVarRec->VarKind == VAR_CONST ){
            V_UNION(&((*pptvd)->vardesc),lpvarValue)=TLB_Alloc(sizeof(VARIANT));
            TLB_ReadValue(V_UNION(&((*pptvd)->vardesc),lpvarValue), 
                pVarRec->OffsValue, pcx);
        }else
            V_UNION(&((*pptvd)->vardesc),oInst)=pVarRec->OffsValue;
        pptvd=&((*pptvd)->next);
        recoffset += reclength;
    }
}
/* fill in data for a hreftype (offset). When the refernced type is contained
 * in the typelib, its just an (file) offset in the type info base dir.
 * If comes fom import, its an offset+1 in the ImpInfo table
 * */
static void TLB_DoRefType(TLBContext *pcx, 
                          int offset, TLBRefType ** pprtd)
{
    int j;
    if(!HREFTYPE_INTHISFILE( offset)) {
        /* external typelib */
        TLBImpInfo impinfo;
        TLBImpLib *pImpLib=(pcx->pLibInfo->pImpLibs);
        TLB_Read(&impinfo, sizeof(impinfo), pcx, 
            pcx->pTblDir->pImpInfo.offset + (offset & 0xfffffffc));
        for(j=0;pImpLib;j++){   /* search the known offsets of all import libraries */
            if(pImpLib->offset==impinfo.oImpFile) break;
            pImpLib=pImpLib->next;
        }
        if(pImpLib){
            (*pprtd)->reference=offset;
            (*pprtd)->pImpTLInfo=pImpLib;
            TLB_ReadGuid(&(*pprtd)->guid, impinfo.oGuid, pcx);
        }else{
            ERR_(typelib)("Cannot find a reference\n");
            (*pprtd)->reference=-1;
            (*pprtd)->pImpTLInfo=(void *)-1;
        }
    }else{
        /* in this typelib */
        (*pprtd)->reference=offset;
        (*pprtd)->pImpTLInfo=(void *)-2;
    }
}

/* process Implemented Interfaces of a com class */
static void TLB_DoImplTypes(TLBContext *pcx, int count,
                          int offset, TLBRefType ** pprtd)
{
    int i;
    TLBRefRecord refrec;
    for(i=0;i<count;i++){
        if(offset<0) break; /* paranoia */
        *pprtd=TLB_Alloc(sizeof(TLBRefType));
        TLB_Read(&refrec,sizeof(refrec),pcx,offset+pcx->pTblDir->pRefTab.offset);
        TLB_DoRefType(pcx, refrec.reftype, pprtd);
        (*pprtd)->flags=refrec.flags;
        (*pprtd)->ctCustData=
            TLB_CustData(pcx, refrec.oCustData, &(*pprtd)->pCustData);
        offset=refrec.onext;
        pprtd=&((*pprtd)->next);
    }
}
/*
 * process a typeinfo record
 */
TLBTypeInfo * TLB_DoTypeInfo(TLBContext *pcx, int count, TLBLibInfo* pLibInfo)
{
    TLBTypeInfoBase tiBase;
    TLBTypeInfo *ptiRet;
    ptiRet=TLB_Alloc(sizeof(TLBTypeInfo));
    ptiRet->lpvtbl = &tinfvt;
    ptiRet->ref=1;
    TLB_Read(&tiBase, sizeof(tiBase) ,pcx ,
        pcx->pTblDir->pTypeInfoTab.offset+count*sizeof(tiBase));
/* this where we are coming from */
    ptiRet->pTypeLib=pLibInfo;
    ptiRet->index=count;
/* fill in the typeattr fields */
    TLB_ReadGuid(&ptiRet->TypeAttr.guid, tiBase.posguid, pcx);
    ptiRet->TypeAttr.lcid=pLibInfo->LibAttr.lcid;   /* FIXME: correct? */
    ptiRet->TypeAttr.memidConstructor=MEMBERID_NIL ;/* FIXME */
    ptiRet->TypeAttr.memidDestructor=MEMBERID_NIL ; /* FIXME */
    ptiRet->TypeAttr.lpstrSchema=NULL;              /* reserved */
    ptiRet->TypeAttr.cbSizeInstance=tiBase.size;
    ptiRet->TypeAttr.typekind=tiBase.typekind & 0xF;
    ptiRet->TypeAttr.cFuncs=LOWORD(tiBase.cElement);
    ptiRet->TypeAttr.cVars=HIWORD(tiBase.cElement);
    ptiRet->TypeAttr.cbAlignment=(tiBase.typekind >> 11 )& 0x1F; /* there are more flags there */
    ptiRet->TypeAttr.wTypeFlags=tiBase.flags;
    ptiRet->TypeAttr.wMajorVerNum=LOWORD(tiBase.version);
    ptiRet->TypeAttr.wMinorVerNum=HIWORD(tiBase.version);
    ptiRet->TypeAttr.cImplTypes=tiBase.cImplTypes;
    ptiRet->TypeAttr.cbSizeVft=tiBase.cbSizeVft; /* FIXME: this is only the non inherited part */
    if(ptiRet->TypeAttr.typekind == TKIND_ALIAS)
        TLB_GetTdesc(pcx, tiBase.datatype1, 
            &ptiRet->TypeAttr.tdescAlias) ;
/*  FIXME: */
/*    IDLDESC  idldescType; *//* never saw this one != zero  */

/* name, eventually add to a hash table */
    ptiRet->Name=TLB_ReadName(pcx, tiBase.NameOffset);
    TRACE_(typelib)("reading %s\n", ptiRet->Name);
    /* help info */
    ptiRet->DocString=TLB_ReadString(pcx, tiBase.docstringoffs);
    ptiRet->dwHelpStringContext=tiBase.helpstringcontext;
    ptiRet->dwHelpContext=tiBase.helpcontext;
/* note: InfoType's Help file and HelpStringDll come from the containing
 * library. Further HelpString and Docstring appear to be the same thing :(
 */
    /* functions */
    if(ptiRet->TypeAttr.cFuncs >0 )
        TLB_DoFuncs(pcx, ptiRet->TypeAttr.cFuncs ,ptiRet->TypeAttr.cVars, 
        tiBase.memoffset, & ptiRet->funclist);
    /* variables */
    if(ptiRet->TypeAttr.cVars >0 )
        TLB_DoVars(pcx, ptiRet->TypeAttr.cFuncs ,ptiRet->TypeAttr.cVars, 
        tiBase.memoffset, & ptiRet->varlist);
    if(ptiRet->TypeAttr.cImplTypes >0 ){
        if(ptiRet->TypeAttr.typekind == TKIND_COCLASS)
            TLB_DoImplTypes(pcx, ptiRet->TypeAttr.cImplTypes , 
                tiBase.datatype1, & ptiRet->impltypelist);
        else if(ptiRet->TypeAttr.typekind != TKIND_DISPATCH){
            ptiRet->impltypelist=TLB_Alloc(sizeof(TLBRefType));
            TLB_DoRefType(pcx, tiBase.datatype1, & ptiRet->impltypelist);
        }
	}
    ptiRet->ctCustData=
        TLB_CustData(pcx, tiBase.oCustData, &ptiRet->pCustData);
    return ptiRet;
}


long TLB_FindTlb(TLBContext *pcx)
{/* FIXME: should parse the file properly
  * hack to find our tlb data
  */
#define TLBBUFSZ 1024
    char buff[TLBBUFSZ+1]; /* room for a trailing '\0' */
    long ret=0,found=0;
    int count;
    char *pChr;

#define LOOK_FOR_MAGIC(magic)				\
    count=TLB_Read(buff, TLBBUFSZ, pcx, 0);		\
    do {						\
        buff[count]='\0';				\
	pChr = buff;					\
	while (pChr) {					\
	    pChr = memchr(pChr,magic[0],count-(pChr-buff));\
	    if (pChr) {					\
		if (!memcmp(pChr,magic,4)) {		\
		    ret+= pChr-buff;			\
		    found = 1;				\
		    break;				\
		}					\
		pChr++;					\
	    }						\
	}						\
	if (found) break;				\
        ret+=count;					\
	count=TLB_Read(buff, TLBBUFSZ, pcx, DO_NOT_SEEK);\
    } while(count>0);


    LOOK_FOR_MAGIC(TLBMAGIC2);
    if(count)
        return ret;

    LOOK_FOR_MAGIC(TLBMAGIC1);
    if(count)
        ERR_(ole)("type library format not (yet) implemented\n");
    else
        ERR_(ole)("not type library found in this file\n");
    return -1;
}
#undef LOOK_FOR_MAGIC

int TLB_ReadTypeLib(PCHAR file, ITypeLib **ppTypeLib)
{
    TLBContext cx;
    OFSTRUCT ofStruct;
    long oStart,lPSegDir;
    TLBLibInfo* pLibInfo=NULL;
    TLB2Header tlbHeader;
    TLBSegDir tlbSegDir;
    if((cx.hFile=OpenFile(file, &ofStruct, OF_READWRITE))==HFILE_ERROR) {
	ERR_(typelib)("cannot open %s error 0x%lx\n",file, GetLastError());
	return E_FAIL;
    }
    /* get pointer to beginning of typelib data */
    cx.oStart=0;
    if((oStart=TLB_FindTlb(&cx))<0){
        if(oStart==-1)
            ERR_(typelib)("cannot locate typelib in  %s\n",file);
        else
            ERR_(typelib)("unsupported typelib format in  %s\n",file);
        return E_FAIL;
    }
    cx.oStart=oStart;
    pLibInfo=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(TLBLibInfo));
    if (!pLibInfo){
	CloseHandle(cx.hFile);
        return E_OUTOFMEMORY;
    }
    pLibInfo->lpvtbl = &tlbvt;
    pLibInfo->ref=1;
    cx.pLibInfo=pLibInfo;
    /* read header */
    TLB_Read((void*)&tlbHeader, sizeof(tlbHeader), &cx, 0);
    /* there is a small number of information here until the next important
     * part:
     * the segment directory . Try to calculate the amount of data */
    lPSegDir=sizeof(tlbHeader)+
            (tlbHeader.nrtypeinfos)*4+
            (tlbHeader.varflags & HELPDLLFLAG? 4 :0);
    /* now read the segment directory */
    TLB_Read((void*)&tlbSegDir, sizeof(tlbSegDir), &cx, lPSegDir);  
    cx.pTblDir=&tlbSegDir;
    /* just check two entries */
    if (	tlbSegDir.pTypeInfoTab.res0c != 0x0F ||
		tlbSegDir.pImpInfo.res0c != 0x0F
    ) {
        ERR_(typelib)("cannot find the table directory, ptr=0x%lx\n",lPSegDir);
	CloseHandle(cx.hFile);
        return E_FAIL;
    }
    /* now fill our internal data */
    /* TLIBATTR fields */
    TLB_ReadGuid(&pLibInfo->LibAttr.guid, tlbHeader.posguid, &cx);
    pLibInfo->LibAttr.lcid=tlbHeader.lcid;
    pLibInfo->LibAttr.syskind=tlbHeader.varflags & 0x0f; /* check the mask */
    pLibInfo->LibAttr.wMajorVerNum=LOWORD(tlbHeader.version);
    pLibInfo->LibAttr.wMinorVerNum=HIWORD(tlbHeader.version);
    pLibInfo->LibAttr.wLibFlags=(WORD) tlbHeader.flags & 0xffff;/* check mask */
    /* name, eventually add to a hash table */
    pLibInfo->Name=TLB_ReadName(&cx, tlbHeader.NameOffset);
    /* help info */
    pLibInfo->DocString=TLB_ReadString(&cx, tlbHeader.helpstring);
    pLibInfo->HelpFile=TLB_ReadString(&cx, tlbHeader.helpfile);
    if( tlbHeader.varflags & HELPDLLFLAG){
            int offset;
            TLB_Read(&offset, sizeof(offset), &cx, sizeof(tlbHeader));
            pLibInfo->HelpStringDll=TLB_ReadString(&cx, offset);
    }

    pLibInfo->dwHelpContext=tlbHeader.helpstringcontext;
    /* custom data */
    if(tlbHeader.CustomDataOffset >= 0) {
        pLibInfo->ctCustData=
            TLB_CustData(&cx, tlbHeader.CustomDataOffset, &pLibInfo->pCustData);
    }
    /* fill in typedescriptions */
    if(tlbSegDir.pTypdescTab.length >0){
        int i, j, cTD=tlbSegDir.pTypdescTab.length / (2*sizeof(INT));
        INT16 td[4];
        pLibInfo->pTypeDesc=
            TLB_Alloc( cTD * sizeof(TYPEDESC));
        TLB_Read(td, sizeof(td), &cx, tlbSegDir.pTypdescTab.offset);
        for(i=0;i<cTD;){
            /* FIXME: add several sanity checks here */
            pLibInfo->pTypeDesc[i].vt=td[0] & VT_TYPEMASK;
            if(td[0]==VT_PTR ||td[0]==VT_SAFEARRAY){/* FIXME: check safearray */
                if(td[3]<0)
                    V_UNION(&(pLibInfo->pTypeDesc[i]),lptdesc)=
                        & stndTypeDesc[td[2]];
                else
                    V_UNION(&(pLibInfo->pTypeDesc[i]),lptdesc)=
                        & pLibInfo->pTypeDesc[td[3]/8];
            }else if(td[0]==VT_CARRAY)
                V_UNION(&(pLibInfo->pTypeDesc[i]),lpadesc)=
                    (void *)((int) td[2]);  /* temp store offset in*/
                                            /* array descr table here */
            else if(td[0]==VT_USERDEFINED)
                V_UNION(&(pLibInfo->pTypeDesc[i]),hreftype)=MAKELONG(td[2],td[3]);
            if(++i<cTD) TLB_Read(td, sizeof(td), &cx, DO_NOT_SEEK);
        }
        /* second time around to fill the array subscript info */
        for(i=0;i<cTD;i++){
            if(pLibInfo->pTypeDesc[i].vt != VT_CARRAY) continue;
            if(tlbSegDir.pArrayDescriptions.offset>0){
                TLB_Read(td, sizeof(td), &cx, tlbSegDir.pArrayDescriptions.offset +
                    (int) V_UNION(&(pLibInfo->pTypeDesc[i]),lpadesc));
                V_UNION(&(pLibInfo->pTypeDesc[i]),lpadesc)=
                    TLB_Alloc(sizeof(ARRAYDESC)+sizeof(SAFEARRAYBOUND)*(td[3]-1));
                if(td[1]<0)
                    V_UNION(&(pLibInfo->pTypeDesc[i]),lpadesc)->tdescElem.vt=td[0] & VT_TYPEMASK;
                else
                    V_UNION(&(pLibInfo->pTypeDesc[i]),lpadesc)->tdescElem=stndTypeDesc[td[0]/8];
                V_UNION(&(pLibInfo->pTypeDesc[i]),lpadesc)->cDims=td[2];
                for(j=0;j<td[2];j++){
                    TLB_Read(& V_UNION(&(pLibInfo->pTypeDesc[i]),lpadesc)->rgbounds[j].cElements, 
                        sizeof(INT), &cx, DO_NOT_SEEK);
                    TLB_Read(& V_UNION(&(pLibInfo->pTypeDesc[i]),lpadesc)
                        ->rgbounds[j].lLbound, 
                        sizeof(INT), &cx, DO_NOT_SEEK);
                }
            }else{
                V_UNION(&(pLibInfo->pTypeDesc[i]),lpadesc)=NULL;
                ERR_(ole)("didn't find array description data\n");
            }
        }
    }
    /* imported type libs */
    if(tlbSegDir.pImpFiles.offset>0){
        TLBImpLib **ppImpLib=&(pLibInfo->pImpLibs);
        int offset=tlbSegDir.pImpFiles.offset;
        int oGuid;
        UINT16 size;
        while(offset < tlbSegDir.pImpFiles.offset +tlbSegDir.pImpFiles.length){
            *ppImpLib=TLB_Alloc(sizeof(TLBImpLib));
            (*ppImpLib)->offset=offset - tlbSegDir.pImpFiles.offset;
            TLB_Read(&oGuid, sizeof(INT), &cx, offset);
            TLB_ReadGuid(&(*ppImpLib)->guid, oGuid, &cx);
            /* we are skipping some unknown info here */
            TLB_Read(& size,sizeof(UINT16), &cx, offset+3*(sizeof(INT)));
            size >>=2;
            (*ppImpLib)->name=TLB_Alloc(size+1);
            TLB_Read((*ppImpLib)->name,size, &cx, DO_NOT_SEEK);
            offset=(offset+3*(sizeof(INT))+sizeof(UINT16)+size+3) & 0xfffffffc;

            ppImpLib=&(*ppImpLib)->next;
        }
    }
    /* type info's */
    if(tlbHeader.nrtypeinfos >=0 ){
        /*pLibInfo->TypeInfoCount=tlbHeader.nrtypeinfos; */
        TLBTypeInfo **ppTI=&(pLibInfo->pTypeInfo);
        int i;
        for(i=0;i<(int)tlbHeader.nrtypeinfos;i++){
            *ppTI=TLB_DoTypeInfo(&cx, i, pLibInfo);
            ppTI=&((*ppTI)->next);
            (pLibInfo->TypeInfoCount)++;
        }
    }

    CloseHandle(cx.hFile);
    *ppTypeLib=(LPTYPELIB)pLibInfo;
    return S_OK;
}

/*================== ITypeLib(2) Methods ===================================*/

/* ITypeLib::QueryInterface
 */
static HRESULT WINAPI ITypeLib_fnQueryInterface( LPTYPELIB This, REFIID riid,
    VOID **ppvObject)
{
    if(TRACE_ON(typelib)){
        char xriid[50];
        WINE_StringFromCLSID((LPCLSID)riid,xriid);
        TRACE_(typelib)("(%p)->(IID: %s)\n",This,xriid);
    }
    *ppvObject=NULL;
    if(IsEqualIID(riid, &IID_IUnknown) || 
            IsEqualIID(riid,&IID_ITypeLib)||
            IsEqualIID(riid,&IID_ITypeLib2))
        *ppvObject = This;
    if(*ppvObject){
        ITypeLib_AddRef(This);
        TRACE_(typelib)("-- Interface: (%p)->(%p)\n",ppvObject,*ppvObject);
        return S_OK;
    }
    TRACE_(typelib)("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

/* ITypeLib::AddRef
 */
static ULONG WINAPI ITypeLib_fnAddRef( LPTYPELIB iface)
{
	ICOM_THIS( TLBLibInfo, iface);
    TRACE_(typelib)("(%p)->ref is %u\n",This, This->ref);
    return ++(This->ref);
}

/* ITypeLib::Release
 */
static ULONG WINAPI ITypeLib_fnRelease( LPTYPELIB iface)
{
	ICOM_THIS( TLBLibInfo, iface);
    FIXME_(typelib)("(%p)->ref is %u:   stub\n",This, This->ref);
    (This->ref)--;
    return S_OK;
}

/* ITypeLib::GetTypeInfoCount
 * 
 * Returns the number of type descriptions in the type library
 */
static UINT WINAPI ITypeLib_fnGetTypeInfoCount( LPTYPELIB iface)
{
	ICOM_THIS( TLBLibInfo, iface);
    TRACE_(typelib)("(%p)->count is %d\n",This, This->TypeInfoCount);
    return This->TypeInfoCount;
}

/* ITypeLib::GetTypeInfo
 *
 *etrieves the specified type description in the library.
 */
static HRESULT WINAPI ITypeLib_fnGetTypeInfo( LPTYPELIB iface, UINT index, 
    ITypeInfo **ppTInfo)
{
    int i;
	ICOM_THIS( TLBLibInfo, iface);
	TLBTypeInfo **ppTLBTInfo=(TLBTypeInfo **)ppTInfo;
    TRACE_(typelib)("(%p) index %d \n",This, index);
    for(i=0,*ppTLBTInfo=This->pTypeInfo;*ppTLBTInfo && i != index;i++)
        *ppTLBTInfo=(*ppTLBTInfo)->next;
    if(*ppTLBTInfo){
        (*ppTLBTInfo)->lpvtbl->fnAddRef(*ppTInfo);
        TRACE_(typelib)("-- found (%p)->(%p)\n",ppTLBTInfo,*ppTLBTInfo);
        return S_OK;
    }
    TRACE_(typelib)("-- element not found\n");
    return TYPE_E_ELEMENTNOTFOUND;
}

/* ITypeLibs::GetTypeInfoType
 *
 * Retrieves the type of a type description.
 */
static HRESULT WINAPI ITypeLib_fnGetTypeInfoType( LPTYPELIB iface, UINT index,
    TYPEKIND *pTKind)
{
    int i;
    TLBTypeInfo *pTInfo;
	ICOM_THIS( TLBLibInfo, iface);
    TRACE_(typelib)("(%p) index %d \n",This, index);
    for(i=0,pTInfo=This->pTypeInfo;pTInfo && i != index;i++)
        pTInfo=(pTInfo)->next;
    if(pTInfo){
        *pTKind=pTInfo->TypeAttr.typekind;
        TRACE_(typelib)("-- found Type (%p)->%d\n",pTKind,*pTKind);
        return S_OK;
    }
    TRACE_(typelib)("-- element not found\n");
    return TYPE_E_ELEMENTNOTFOUND;
}

/* ITypeLib::GetTypeInfoOfGuid
 *
 * Retrieves the type description that corresponds to the specified GUID.
 *
 */
static HRESULT WINAPI ITypeLib_fnGetTypeInfoOfGuid( LPTYPELIB iface,
				REFGUID guid, ITypeInfo **ppTInfo)
{
    int i;
	ICOM_THIS( TLBLibInfo, iface);
	TLBTypeInfo **ppTLBTInfo=(TLBTypeInfo **)ppTInfo;
    if(TRACE_ON(typelib)){
        char xriid[50];
        WINE_StringFromCLSID((LPCLSID)guid,xriid);
        TRACE_(typelib)("(%p) guid %sx)\n",This,xriid);
    }
    for(i=0,*ppTLBTInfo=This->pTypeInfo;*ppTLBTInfo && 
            !IsEqualIID(guid,&(*ppTLBTInfo)->TypeAttr.guid);i++)
        *ppTLBTInfo=(*ppTLBTInfo)->next;
    if(*ppTLBTInfo){
        (*ppTLBTInfo)->lpvtbl->fnAddRef(*ppTInfo);
        TRACE_(typelib)("-- found (%p)->(%p)\n",ppTLBTInfo,*ppTLBTInfo);
        return S_OK;
    }
    TRACE_(typelib)("-- element not found\n");
    return TYPE_E_ELEMENTNOTFOUND;
}

/* ITypeLib::GetLibAttr
 *
 * Retrieves the structure that contains the library's attributes.
 *
 */
static HRESULT WINAPI ITypeLib_fnGetLibAttr( LPTYPELIB iface, 
    LPTLIBATTR *ppTLibAttr)
{
	ICOM_THIS( TLBLibInfo, iface);
    TRACE_(typelib)("(%p)\n",This);
    /* FIXME: must do a copy here */
    *ppTLibAttr=&This->LibAttr;
    return S_OK;
}

/* ITypeLib::GetTypeComp
 *
 * Enables a client compiler to bind to a library's types, variables,
 * constants, and global functions.
 *
 */
static HRESULT WINAPI ITypeLib_fnGetTypeComp( LPTYPELIB iface,
    ITypeComp **ppTComp)
{
	ICOM_THIS( TLBLibInfo, iface);
    FIXME_(typelib)("(%p): stub!\n",This);
    return E_NOTIMPL;
}

/* ITypeLib::GetDocumentation
 *
 * Retrieves the library's documentation string, the complete Help file name
 * and path, and the context identifier for the library Help topic in the Help
 * file.
 *
 */
static HRESULT WINAPI ITypeLib_fnGetDocumentation( LPTYPELIB iface, INT index,
    BSTR *pBstrName, BSTR *pBstrDocString, DWORD *pdwHelpContext, 
    BSTR *pBstrHelpFile)
{
	ICOM_THIS( TLBLibInfo, iface);
    HRESULT result;
    ITypeInfo *pTInfo;
    TRACE_(typelib)("(%p) index %d Name(%p) DocString(%p)"
           " HelpContext(%p) HelpFile(%p)\n",
        This, index, pBstrName, pBstrDocString, pdwHelpContext, pBstrHelpFile);
    if(index<0){ /* documentation for the typelib */
        if(pBstrName)
            *pBstrName=TLB_DupAtoBstr(This->Name);
        if(pBstrDocString)
            *pBstrName=TLB_DupAtoBstr(This->DocString);
        if(pdwHelpContext)
            *pdwHelpContext=This->dwHelpContext;
        if(pBstrHelpFile)
            *pBstrName=TLB_DupAtoBstr(This->HelpFile);
    }else {/* for a typeinfo */
        result=ITypeLib_fnGetTypeInfo(iface, index, &pTInfo);
        if(SUCCEEDED(result)){
            result=ITypeInfo_GetDocumentation(pTInfo, MEMBERID_NIL,  pBstrName,
                pBstrDocString, pdwHelpContext, pBstrHelpFile);
            ITypeInfo_Release(pTInfo);
        }
        if(!SUCCEEDED(result))
            return result;
    }
    return S_OK;
}

/* ITypeLib::IsName
 *
 * Indicates whether a passed-in string contains the name of a type or member
 * described in the library.
 *
 */
static HRESULT WINAPI ITypeLib_fnIsName( LPTYPELIB iface, LPOLESTR szNameBuf,
    ULONG lHashVal, BOOL *pfName)
{
	ICOM_THIS( TLBLibInfo, iface);
    TLBTypeInfo *pTInfo;
    TLBFuncDesc *pFInfo;
    TLBVarDesc *pVInfo;
    int i;
    PCHAR astr= HEAP_strdupWtoA( GetProcessHeap(), 0, szNameBuf );
    *pfName=TRUE;
    if(!strcmp(astr,This->Name)) goto ITypeLib_fnIsName_exit;
    for(pTInfo=This->pTypeInfo;pTInfo;pTInfo=pTInfo->next){
        if(!strcmp(astr,pTInfo->Name)) goto ITypeLib_fnIsName_exit;
        for(pFInfo=pTInfo->funclist;pFInfo;pFInfo=pFInfo->next) {
            if(!strcmp(astr,pFInfo->Name)) goto ITypeLib_fnIsName_exit;
            for(i=0;i<pFInfo->funcdesc.cParams;i++)
                if(!strcmp(astr,pFInfo->pParamDesc[i].Name))
                    goto ITypeLib_fnIsName_exit;
        }
        for(pVInfo=pTInfo->varlist;pVInfo;pVInfo=pVInfo->next) ;
            if(!strcmp(astr,pVInfo->Name)) goto ITypeLib_fnIsName_exit;
       
    }
    *pfName=FALSE;

ITypeLib_fnIsName_exit:
    TRACE_(typelib)("(%p)slow! search for %s: %s found!\n", This,
            debugstr_a(astr), *pfName?"NOT":"");
    
    HeapFree( GetProcessHeap(), 0, astr );
    return S_OK;
}

/* ITypeLib::FindName
 *
 * Finds occurrences of a type description in a type library. This may be used
 * to quickly verify that a name exists in a type library.
 *
 */
static HRESULT WINAPI ITypeLib_fnFindName( LPTYPELIB iface, LPOLESTR szNameBuf,
    ULONG lHashVal, ITypeInfo **ppTInfo, MEMBERID *rgMemId, UINT16 *pcFound)
{
	ICOM_THIS( TLBLibInfo, iface);
    TLBTypeInfo *pTInfo;
    TLBFuncDesc *pFInfo;
    TLBVarDesc *pVInfo;
    int i,j = 0;
    PCHAR astr= HEAP_strdupWtoA( GetProcessHeap(), 0, szNameBuf );
    for(pTInfo=This->pTypeInfo;pTInfo && j<*pcFound; pTInfo=pTInfo->next){
        if(!strcmp(astr,pTInfo->Name)) goto ITypeLib_fnFindName_exit;
        for(pFInfo=pTInfo->funclist;pFInfo;pFInfo=pFInfo->next) {
            if(!strcmp(astr,pFInfo->Name)) goto ITypeLib_fnFindName_exit;
            for(i=0;i<pFInfo->funcdesc.cParams;i++)
                if(!strcmp(astr,pFInfo->pParamDesc[i].Name))
                    goto ITypeLib_fnFindName_exit;
        }
        for(pVInfo=pTInfo->varlist;pVInfo;pVInfo=pVInfo->next) ;
            if(!strcmp(astr,pVInfo->Name)) goto ITypeLib_fnFindName_exit;
        continue;
ITypeLib_fnFindName_exit:
        pTInfo->lpvtbl->fnAddRef((LPTYPEINFO)pTInfo);
        ppTInfo[j]=(LPTYPEINFO)pTInfo;
        j++;
    }
    TRACE_(typelib)("(%p)slow! search for %d with %s: found %d TypeInfo's!\n",
            This, *pcFound, debugstr_a(astr), j);

    *pcFound=j;
    
    HeapFree( GetProcessHeap(), 0, astr );
    return S_OK;
}

/* ITypeLib::ReleaseTLibAttr
 *
 * Releases the TLIBATTR originally obtained from ITypeLib::GetLibAttr.
 *
 */
static VOID WINAPI ITypeLib_fnReleaseTLibAttr( LPTYPELIB iface, TLIBATTR *pTLibAttr)
{
	ICOM_THIS( TLBLibInfo, iface);
    TRACE_(typelib)("freeing (%p)\n",This);
    /* nothing to do */
}

/* ITypeLib2::GetCustData
 *
 * gets the custom data
 */
static HRESULT WINAPI ITypeLib2_fnGetCustData( ITypeLib * iface, REFGUID guid, 
        VARIANT *pVarVal)
{
	ICOM_THIS( TLBLibInfo, iface);
    TLBCustData *pCData;
    for(pCData=This->pCustData; pCData; pCData = pCData->next)
        if( IsEqualIID(guid, &pCData->guid)) break;
    if(TRACE_ON(typelib)){
        char xriid[50];
        WINE_StringFromCLSID((LPCLSID)guid,xriid);
        TRACE_(typelib)("(%p) guid %s %s found!x)\n",This,xriid,
                pCData? "" : "NOT");
    }
    if(pCData){
        VariantInit( pVarVal);
        VariantCopy( pVarVal, &pCData->data);
        return S_OK;
    }
    return E_INVALIDARG;  /* FIXME: correct? */
}

/* ITypeLib2::GetLibStatistics
 *
 * Returns statistics about a type library that are required for efficient
 * sizing of hash tables.
 *
 */
static HRESULT WINAPI ITypeLib2_fnGetLibStatistics( ITypeLib * iface, 
        UINT *pcUniqueNames, UINT *pcchUniqueNames)
{
	ICOM_THIS( TLBLibInfo, iface);
    FIXME_(typelib)("(%p): stub!\n", This);
    if(pcUniqueNames) *pcUniqueNames=1;
    if(pcchUniqueNames) *pcchUniqueNames=1;
    return S_OK;
}

/* ITypeLib2::GetDocumentation2
 *
 * Retrieves the library's documentation string, the complete Help file name
 * and path, the localization context to use, and the context ID for the
 * library Help topic in the Help file.
 *
 */
static HRESULT WINAPI ITypeLib2_fnGetDocumentation2( ITypeLib * iface, 
        INT index, LCID lcid, BSTR *pbstrHelpString,
        INT *pdwHelpStringContext, BSTR *pbstrHelpStringDll)
{
	ICOM_THIS( TLBLibInfo, iface);
    HRESULT result;
    ITypeInfo *pTInfo;
    FIXME_(typelib)("(%p) index %d lcid %ld half implemented stub!\n", This,
            index, lcid);
    /* the help string should be obtained from the helpstringdll,
     * using the _DLLGetDocumentation function, based on the supplied
     * lcid. Nice to do sometime...
     */
    if(index<0){ /* documentation for the typelib */
        if(pbstrHelpString)
            *pbstrHelpString=TLB_DupAtoBstr(This->DocString);
        if(pdwHelpStringContext)
            *pdwHelpStringContext=This->dwHelpContext;
        if(pbstrHelpStringDll)
            *pbstrHelpStringDll=TLB_DupAtoBstr(This->HelpStringDll);
    }else {/* for a typeinfo */
        result=ITypeLib_fnGetTypeInfo(iface, index, &pTInfo);
        if(SUCCEEDED(result)){
            result=ITypeInfo2_fnGetDocumentation2(pTInfo, MEMBERID_NIL, lcid,
                pbstrHelpString, pdwHelpStringContext, pbstrHelpStringDll);
            ITypeInfo_Release(pTInfo);
        }
        if(!SUCCEEDED(result))
            return result;
    }
    return S_OK;
}

/* ITypeLib2::GetAllCustData
 *
 * Gets all custom data items for the library. 
 *
 */
static HRESULT WINAPI ITypeLib2_fnGetAllCustData( ITypeLib * iface,
        CUSTDATA *pCustData)
{
	ICOM_THIS( TLBLibInfo, iface);
    TLBCustData *pCData;
    int i;
    TRACE_(typelib)("(%p) returning %d items\n", This, This->ctCustData); 
    pCustData->prgCustData = TLB_Alloc(This->ctCustData * sizeof(CUSTDATAITEM));
    if(pCustData->prgCustData ){
        pCustData->cCustData=This->ctCustData;
        for(i=0, pCData=This->pCustData; pCData; i++, pCData = pCData->next){
            pCustData->prgCustData[i].guid=pCData->guid;
            VariantCopy(& pCustData->prgCustData[i].varValue, & pCData->data);
        }
    }else{
        ERR_(typelib)(" OUT OF MEMORY! \n");
        return E_OUTOFMEMORY;
    }
    return S_OK;
}


/*================== ITypeInfo(2) Methods ===================================*/

/* ITypeInfo::QueryInterface
 */
static HRESULT WINAPI ITypeInfo_fnQueryInterface( LPTYPEINFO iface, REFIID riid,
    VOID **ppvObject)
{
	ICOM_THIS( TLBTypeInfo, iface);
    if(TRACE_ON(typelib)){
        char xriid[50];
        WINE_StringFromCLSID((LPCLSID)riid,xriid);
        TRACE_(typelib)("(%p)->(IID: %s)\n",This,xriid);
    }
    *ppvObject=NULL;
    if(IsEqualIID(riid, &IID_IUnknown) || 
            IsEqualIID(riid,&IID_ITypeInfo)||
            IsEqualIID(riid,&IID_ITypeInfo2))
        *ppvObject = This;
    if(*ppvObject){
        ITypeInfo_AddRef(iface);
        TRACE_(typelib)("-- Interface: (%p)->(%p)\n",ppvObject,*ppvObject);
        return S_OK;
    }
    TRACE_(typelib)("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

/* ITypeInfo::AddRef
 */
static ULONG WINAPI ITypeInfo_fnAddRef( LPTYPEINFO iface)
{
	ICOM_THIS( TLBTypeInfo, iface);
    TRACE_(typelib)("(%p)->ref is %u\n",This, This->ref);
    (This->pTypeLib->ref)++;
    return ++(This->ref);
}

/* ITypeInfo::Release
 */
static ULONG WINAPI ITypeInfo_fnRelease( LPTYPEINFO iface)
{
	ICOM_THIS( TLBTypeInfo, iface);
    FIXME_(typelib)("(%p)->ref is %u:   stub\n",This, This->ref);
    (This->ref)--;
    (This->pTypeLib->ref)--;
    return S_OK;
}

/* ITypeInfo::GetTypeAttr
 *
 * Retrieves a TYPEATTR structure that contains the attributes of the type
 * description.
 *
 */
static HRESULT WINAPI ITypeInfo_fnGetTypeAttr( LPTYPEINFO iface,
        LPTYPEATTR  *ppTypeAttr)
{
	ICOM_THIS( TLBTypeInfo, iface);
     TRACE_(typelib)("(%p)\n",This);
    /* FIXME: must do a copy here */
    *ppTypeAttr=&This->TypeAttr;
    return S_OK;
}

/* ITypeInfo::GetTypeComp
 *
 * Retrieves the ITypeComp interface for the type description, which enables a
 * client compiler to bind to the type description's members.
 *
 */
static HRESULT WINAPI ITypeInfo_fnGetTypeComp( LPTYPEINFO iface,
        ITypeComp  * *ppTComp)
{
	ICOM_THIS( TLBTypeInfo, iface);
    FIXME_(typelib)("(%p) stub!\n", This);
    return S_OK;
}

/* ITypeInfo::GetFuncDesc
 *
 * Retrieves the FUNCDESC structure that contains information about a
 * specified function.
 *
 */
static HRESULT WINAPI ITypeInfo_fnGetFuncDesc( LPTYPEINFO iface, UINT index,
        LPFUNCDESC  *ppFuncDesc)
{
	ICOM_THIS( TLBTypeInfo, iface);
    int i;
    TLBFuncDesc * pFDesc; 
    TRACE_(typelib)("(%p) index %d\n", This, index);
    for(i=0, pFDesc=This->funclist; i!=index && pFDesc; i++, pFDesc=pFDesc->next)
        ;
    if(pFDesc){
        /* FIXME: must do a copy here */
        *ppFuncDesc=&pFDesc->funcdesc;
        return S_OK;
    }
    return E_INVALIDARG;
}

/* ITypeInfo::GetVarDesc
 *
 * Retrieves a VARDESC structure that describes the specified variable. 
 *
 */
static HRESULT WINAPI ITypeInfo_fnGetVarDesc( LPTYPEINFO iface, UINT index,
        LPVARDESC  *ppVarDesc)
{
	ICOM_THIS( TLBTypeInfo, iface);
    int i;
    TLBVarDesc * pVDesc; 
    TRACE_(typelib)("(%p) index %d\n", This, index);
    for(i=0, pVDesc=This->varlist; i!=index && pVDesc; i++, pVDesc=pVDesc->next)
        ;
    if(pVDesc){
        /* FIXME: must do a copy here */
        *ppVarDesc=&pVDesc->vardesc;
        return S_OK;
    }
    return E_INVALIDARG;
}

/* ITypeInfo_GetNames
 *
 * Retrieves the variable with the specified member ID (or the name of the
 * property or method and its parameters) that correspond to the specified
 * function ID.
 */
static HRESULT WINAPI ITypeInfo_fnGetNames( LPTYPEINFO iface, MEMBERID memid,
        BSTR  *rgBstrNames, UINT cMaxNames, UINT  *pcNames)
{
	ICOM_THIS( TLBTypeInfo, iface);
    TLBFuncDesc * pFDesc; 
    TLBVarDesc * pVDesc; 
    int i;
    TRACE_(typelib)("(%p) memid=0x%08lx Maxname=%d\n", This, memid,
            cMaxNames);
    for(pFDesc=This->funclist; pFDesc->funcdesc.memid != memid && pFDesc;
            pFDesc=pFDesc->next)
        ;
    if(pFDesc){
        /* function found, now return function and parameter names */
        for(i=0; i<cMaxNames && i <= pFDesc->funcdesc.cParams; i++){
            if(!i) *rgBstrNames=TLB_DupAtoBstr(pFDesc->Name);
            else
                rgBstrNames[i]=TLB_DupAtoBstr(pFDesc->pParamDesc[i-1].Name);
                
        }
        *pcNames=i;
    }else{
        for(pVDesc=This->varlist; pVDesc->vardesc.memid != memid && pVDesc;
                pVDesc=pVDesc->next)
            ;
        if(pVDesc){
            *rgBstrNames=TLB_DupAtoBstr(pFDesc->Name);
            *pcNames=1;
        }else{
            if(This->TypeAttr.typekind==TKIND_INTERFACE && 
                    This->TypeAttr.cImplTypes ){
                /* recursive search */
                ITypeInfo *pTInfo;
                HRESULT result;
                result=This->lpvtbl->fnGetRefTypeInfo(iface, 
                        This->impltypelist->reference, &pTInfo);
                if(SUCCEEDED(result)){
                    result=ITypeInfo_GetNames(pTInfo, memid, rgBstrNames,
                            cMaxNames, pcNames);
                    ITypeInfo_Release(pTInfo);
                    return result;
                }
                WARN_(typelib)("Could not search inherited interface!\n");
            } else
                WARN_(typelib)("no names found\n");
            *pcNames=0;
            return TYPE_E_ELEMENTNOTFOUND;
        }
    }
    return S_OK;
}


/* ITypeInfo::GetRefTypeOfImplType
 *
 * If a type description describes a COM class, it retrieves the type
 * description of the implemented interface types. For an interface,
 * GetRefTypeOfImplType returns the type information for inherited interfaces,
 * if any exist.
 *
 */
static HRESULT WINAPI ITypeInfo_fnGetRefTypeOfImplType( LPTYPEINFO iface,
        UINT index, HREFTYPE  *pRefType)
{
	ICOM_THIS( TLBTypeInfo, iface);
    int(i);
    TLBRefType *pIref;
    TRACE_(typelib)("(%p) index %d\n", This, index);
    for(i=0, pIref=This->impltypelist; i<index && pIref;
            i++, pIref=pIref->next)
        ;
    if(i==index){
        *pRefType=pIref->reference;
        return S_OK;
    }
    return TYPE_E_ELEMENTNOTFOUND;
}

/* ITypeInfo::GetImplTypeFlags
 * 
 * Retrieves the IMPLTYPEFLAGS enumeration for one implemented interface 
 * or base interface in a type description.
 */
static HRESULT WINAPI ITypeInfo_fnGetImplTypeFlags( LPTYPEINFO iface,
        UINT index, INT  *pImplTypeFlags)
{
	ICOM_THIS( TLBTypeInfo, iface);
    int(i);
    TLBRefType *pIref;
    TRACE_(typelib)("(%p) index %d\n", This, index);
    for(i=0, pIref=This->impltypelist; i<index && pIref; i++, pIref=pIref->next)
        ;
    if(i==index && pIref){
        *pImplTypeFlags=pIref->flags;
        return S_OK;
    }
    *pImplTypeFlags=0;
    return TYPE_E_ELEMENTNOTFOUND;
}

/* GetIDsOfNames
 * Maps between member names and member IDs, and parameter names and
 * parameter IDs.
 */
static HRESULT WINAPI ITypeInfo_fnGetIDsOfNames( LPTYPEINFO iface,
        LPOLESTR  *rgszNames, UINT cNames, MEMBERID  *pMemId)
{
	ICOM_THIS( TLBTypeInfo, iface);
    TLBFuncDesc * pFDesc; 
    TLBVarDesc * pVDesc; 
    HRESULT ret=S_OK;
    PCHAR aszName= HEAP_strdupWtoA( GetProcessHeap(), 0, *rgszNames);
    TRACE_(typelib)("(%p) Name %s cNames %d\n", This, debugstr_a(aszName),
            cNames);
    for(pFDesc=This->funclist; pFDesc; pFDesc=pFDesc->next) {
        int i, j;
        if( !strcmp(aszName, pFDesc->Name)) {
            if(cNames) *pMemId=pFDesc->funcdesc.memid;
            for(i=1; i < cNames; i++){
                PCHAR aszPar= HEAP_strdupWtoA( GetProcessHeap(), 0,
                        rgszNames[i]);
                for(j=0; j<pFDesc->funcdesc.cParams; j++)
                    if(strcmp(aszPar,pFDesc->pParamDesc[j].Name))
                            break;
                if( j<pFDesc->funcdesc.cParams)
                    pMemId[i]=j;
                else
                   ret=DISP_E_UNKNOWNNAME;
                HeapFree( GetProcessHeap(), 0, aszPar);         
            };
            HeapFree (GetProcessHeap(), 0, aszName);
            return ret;
        }
    }   
    for(pVDesc=This->varlist; pVDesc; pVDesc=pVDesc->next) {
        if( !strcmp(aszName, pVDesc->Name)) {
            if(cNames) *pMemId=pVDesc->vardesc.memid;
            HeapFree (GetProcessHeap(), 0, aszName);
            return ret;
        }
    }
    /* not found, see if this is and interface with an inheritance */       
    if(This->TypeAttr.typekind==TKIND_INTERFACE && 
            This->TypeAttr.cImplTypes ){
        /* recursive search */
        ITypeInfo *pTInfo;
        ret=ITypeInfo_GetRefTypeInfo(iface, 
                This->impltypelist->reference, &pTInfo);
        if(SUCCEEDED(ret)){
            ret=ITypeInfo_GetIDsOfNames(pTInfo, rgszNames, cNames, pMemId );
            ITypeInfo_Release(pTInfo);
            return ret;
        }
        WARN_(typelib)("Could not search inherited interface!\n");
    } else
        WARN_(typelib)("no names found\n");
    return DISP_E_UNKNOWNNAME;
}

/* ITypeInfo::Invoke
 * 
 * Invokes a method, or accesses a property of an object, that implements the
 * interface described by the type description.
 */
static HRESULT WINAPI ITypeInfo_fnInvoke( LPTYPEINFO iface, VOID  *pIUnk,
        MEMBERID memid, UINT16 dwFlags, DISPPARAMS  *pDispParams,
        VARIANT  *pVarResult, EXCEPINFO  *pExcepInfo, UINT  *pArgErr)
{
	ICOM_THIS( TLBTypeInfo, iface);
    FIXME_(typelib)("(%p) stub!", This);
    return S_OK;
}

/* ITypeInfo::GetDocumentation
 * 
 * Retrieves the documentation string, the complete Help file name and path,
 * and the context ID for the Help topic for a specified type description.
 */
static HRESULT WINAPI ITypeInfo_fnGetDocumentation( LPTYPEINFO iface,
        MEMBERID memid, BSTR  *pBstrName, BSTR  *pBstrDocString,
        DWORD  *pdwHelpContext, BSTR  *pBstrHelpFile)
{
	ICOM_THIS( TLBTypeInfo, iface);
    TLBFuncDesc * pFDesc; 
    TLBVarDesc * pVDesc; 
    TRACE_(typelib)("(%p) memid %ld Name(%p) DocString(%p)"
           " HelpContext(%p) HelpFile(%p)\n",
        This, memid, pBstrName, pBstrDocString, pdwHelpContext, pBstrHelpFile);
    if(memid==MEMBERID_NIL){ /* documentation for the typeinfo */
        if(pBstrName)
            *pBstrName=TLB_DupAtoBstr(This->Name);
        if(pBstrDocString)
            *pBstrDocString=TLB_DupAtoBstr(This->DocString);
        if(pdwHelpContext)
            *pdwHelpContext=This->dwHelpContext;
        if(pBstrHelpFile)
            *pBstrHelpFile=TLB_DupAtoBstr(This->DocString);/* FIXME */
        return S_OK;
    }else {/* for a member */
    for(pFDesc=This->funclist; pFDesc; pFDesc=pFDesc->next)
        if(pFDesc->funcdesc.memid==memid){
            return S_OK;
        }
    for(pVDesc=This->varlist; pVDesc; pVDesc=pVDesc->next)
        if(pVDesc->vardesc.memid==memid){
            return S_OK;
        }
    }
    return TYPE_E_ELEMENTNOTFOUND;
}

/*  ITypeInfo::GetDllEntry
 * 
 * Retrieves a description or specification of an entry point for a function
 * in a DLL.
 */
static HRESULT WINAPI ITypeInfo_fnGetDllEntry( LPTYPEINFO iface, MEMBERID memid,
        INVOKEKIND invKind, BSTR  *pBstrDllName, BSTR  *pBstrName,
        WORD  *pwOrdinal)
{
	ICOM_THIS( TLBTypeInfo, iface);
    FIXME_(typelib)("(%p) stub!\n", This);
    return E_FAIL;
}

/* ITypeInfo::GetRefTypeInfo
 * 
 * If a type description references other type descriptions, it retrieves
 * the referenced type descriptions.
 */
static HRESULT WINAPI ITypeInfo_fnGetRefTypeInfo( LPTYPEINFO iface,
        HREFTYPE hRefType, ITypeInfo  * *ppTInfo)
{
	ICOM_THIS( TLBTypeInfo, iface);
    HRESULT result;
    if(HREFTYPE_INTHISFILE(hRefType)){
        ITypeLib *pTLib;
        int Index;
        result=This->lpvtbl->fnGetContainingTypeLib(iface, &pTLib, 
                &Index);
        if(SUCCEEDED(result)){
            result=ITypeLib_GetTypeInfo(pTLib,
                    HREFTYPE_INDEX(hRefType),
                    ppTInfo);
            ITypeLib_Release(pTLib );
        }
    } else{
        /* imported type lib */
        TLBRefType * pRefType;
        TLBLibInfo *pTypeLib;
        for( pRefType=This->impltypelist; pRefType &&
                pRefType->reference != hRefType; pRefType=pRefType->next)
            ;
        if(!pRefType)
            return TYPE_E_ELEMENTNOTFOUND; /* FIXME : correct? */
        pTypeLib=pRefType->pImpTLInfo->pImpTypeLib;
        if(pTypeLib) /* typelib already loaded */
            result=ITypeLib_GetTypeInfoOfGuid(
                    (LPTYPELIB)pTypeLib, &pRefType->guid, ppTInfo);
        else{
            result=LoadRegTypeLib( &pRefType->pImpTLInfo->guid,
                    0,0,0, /* FIXME */
                    (LPTYPELIB *)&pTypeLib);
            if(!SUCCEEDED(result)){
                BSTR libnam=TLB_DupAtoBstr(pRefType->pImpTLInfo->name);
                result=LoadTypeLib(libnam, (LPTYPELIB *)&pTypeLib);
                SysFreeString(libnam);
            }
            if(SUCCEEDED(result)){
                result=ITypeLib_GetTypeInfoOfGuid(
                        (LPTYPELIB)pTypeLib, &pRefType->guid, ppTInfo);
                pRefType->pImpTLInfo->pImpTypeLib=pTypeLib;
           }
        }
    }
    TRACE_(typelib)("(%p) hreftype 0x%04lx loaded %s (%p)\n", This, hRefType,
            SUCCEEDED(result)? "SUCCESS":"FAILURE", *ppTInfo);
    return result;
}

/* ITypeInfo::AddressOfMember
 * 
 * Retrieves the addresses of static functions or variables, such as those
 * defined in a DLL.
 */
static HRESULT WINAPI ITypeInfo_fnAddressOfMember( LPTYPEINFO iface,
        MEMBERID memid, INVOKEKIND invKind, PVOID *ppv)
{
	ICOM_THIS( TLBTypeInfo, iface);
    FIXME_(typelib)("(%p) stub!\n", This);
    return S_OK;
}

/* ITypeInfo::CreateInstance
 * 
 * Creates a new instance of a type that describes a component object class 
 * (coclass).
 */
static HRESULT WINAPI ITypeInfo_fnCreateInstance( LPTYPEINFO iface, 
        IUnknown *pUnk, REFIID riid, VOID  **ppvObj) 
{
	ICOM_THIS( TLBTypeInfo, iface);
    FIXME_(typelib)("(%p) stub!\n", This);
    return S_OK;
}

/* ITypeInfo::GetMops
 *
 * Retrieves marshaling information.
 */
static HRESULT WINAPI ITypeInfo_fnGetMops( LPTYPEINFO iface, MEMBERID memid,
				BSTR  *pBstrMops)
{
	ICOM_THIS( TLBTypeInfo, iface);
    FIXME_(typelib)("(%p) stub!\n", This);
    return S_OK;
}

/* ITypeInfo::GetContainingTypeLib
 * 
 * Retrieves the containing type library and the index of the type description
 * within that type library.
 */
static HRESULT WINAPI ITypeInfo_fnGetContainingTypeLib( LPTYPEINFO iface,
        ITypeLib  * *ppTLib, UINT  *pIndex)
{
	ICOM_THIS( TLBTypeInfo, iface);
    *ppTLib=(LPTYPELIB )(This->pTypeLib);
    *pIndex=This->index;
    ITypeLib_AddRef(*ppTLib);
    TRACE_(typelib)("(%p) returns (%p) index %d!\n", This, *ppTLib, *pIndex);
    return S_OK;
}

/* ITypeInfo::ReleaseTypeAttr
 *
 * Releases a TYPEATTR previously returned by GetTypeAttr.
 *
 */
static HRESULT WINAPI ITypeInfo_fnReleaseTypeAttr( LPTYPEINFO iface,
        TYPEATTR* pTypeAttr)
{
	ICOM_THIS( TLBTypeInfo, iface);
    TRACE_(typelib)("(%p)->(%p)\n", This, pTypeAttr);
    return S_OK;
}

/* ITypeInfo::ReleaseFuncDesc
 *
 * Releases a FUNCDESC previously returned by GetFuncDesc. *
 */
static HRESULT WINAPI ITypeInfo_fnReleaseFuncDesc( LPTYPEINFO iface,
        FUNCDESC *pFuncDesc)
{
	ICOM_THIS( TLBTypeInfo, iface);
    TRACE_(typelib)("(%p)->(%p)\n", This, pFuncDesc);
    return S_OK;
}

/* ITypeInfo::ReleaseVarDesc
 *
 * Releases a VARDESC previously returned by GetVarDesc.
 */
static HRESULT WINAPI ITypeInfo_fnReleaseVarDesc( LPTYPEINFO iface,
        VARDESC *pVarDesc)
{
	ICOM_THIS( TLBTypeInfo, iface);
    TRACE_(typelib)("(%p)->(%p)\n", This, pVarDesc);
    return S_OK;
}

/* ITypeInfo2::GetTypeKind
 *
 * Returns the TYPEKIND enumeration quickly, without doing any allocations.
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetTypeKind( ITypeInfo * iface,
    TYPEKIND *pTypeKind)
{
	ICOM_THIS( TLBTypeInfo, iface);
    *pTypeKind=This->TypeAttr.typekind;
    TRACE_(typelib)("(%p) type 0x%0x\n", This,*pTypeKind);
    return S_OK;
}

/* ITypeInfo2::GetTypeFlags
 *
 * Returns the type flags without any allocations. This returns a DWORD type
 * flag, which expands the type flags without growing the TYPEATTR (type
 * attribute). 
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetTypeFlags( ITypeInfo * iface,
    UINT *pTypeFlags)
{
	ICOM_THIS( TLBTypeInfo, iface);
    *pTypeFlags=This->TypeAttr.wTypeFlags;
    TRACE_(typelib)("(%p) flags 0x%04x\n", This,*pTypeFlags);
     return S_OK;
}

/* ITypeInfo2::GetFuncIndexOfMemId
 * Binds to a specific member based on a known DISPID, where the member name
 * is not known (for example, when binding to a default member).
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetFuncIndexOfMemId( ITypeInfo * iface,
    MEMBERID memid, INVOKEKIND invKind, UINT *pFuncIndex)
{
	ICOM_THIS( TLBTypeInfo, iface);
    TLBFuncDesc *pFuncInfo;
    int i;
    HRESULT result;
    /* FIXME: should check for invKind??? */
    for(i=0, pFuncInfo=This->funclist;pFuncInfo && 
            memid != pFuncInfo->funcdesc.memid; i++, pFuncInfo=pFuncInfo->next);
    if(pFuncInfo){
        *pFuncIndex=i;
        result= S_OK;
    }else{
        *pFuncIndex=0;
        result=E_INVALIDARG;
    }
    TRACE_(typelib)("(%p) memid 0x%08lx invKind 0x%04x -> %s\n", This,
            memid, invKind, SUCCEEDED(result)? "SUCCES":"FAILED");
    return result;
}

/* TypeInfo2::GetVarIndexOfMemId
 *
 * Binds to a specific member based on a known DISPID, where the member name
 * is not known (for example, when binding to a default member). 
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetVarIndexOfMemId( ITypeInfo * iface,
    MEMBERID memid, UINT *pVarIndex)
{
	ICOM_THIS( TLBTypeInfo, iface);
    TLBVarDesc *pVarInfo;
    int i;
    HRESULT result;
    for(i=0, pVarInfo=This->varlist; pVarInfo && 
            memid != pVarInfo->vardesc.memid; i++, pVarInfo=pVarInfo->next)
        ;
    if(pVarInfo){
        *pVarIndex=i;
        result= S_OK;
    }else{
        *pVarIndex=0;
        result=E_INVALIDARG;
    }
    TRACE_(typelib)("(%p) memid 0x%08lx -> %s\n", This,
            memid, SUCCEEDED(result)? "SUCCES":"FAILED");
    return result;
}

/* ITypeInfo2::GetCustData
 *
 * Gets the custom data
 */
static HRESULT WINAPI ITypeInfo2_fnGetCustData( ITypeInfo * iface,
    REFGUID guid, VARIANT *pVarVal)
{
	ICOM_THIS( TLBTypeInfo, iface);
    TLBCustData *pCData;
    for(pCData=This->pCustData; pCData; pCData = pCData->next)
        if( IsEqualIID(guid, &pCData->guid)) break;
    if(TRACE_ON(typelib)){
        char xriid[50];
        WINE_StringFromCLSID((LPCLSID)guid,xriid);
        TRACE_(typelib)("(%p) guid %s %s found!x)\n", This, xriid,
                pCData? "" : "NOT");
    }
    if(pCData){
        VariantInit( pVarVal);
        VariantCopy( pVarVal, &pCData->data);
        return S_OK;
    }
    return E_INVALIDARG;  /* FIXME: correct? */
}

/* ITypeInfo2::GetFuncCustData
 *
 * Gets the custom data
 */
static HRESULT WINAPI ITypeInfo2_fnGetFuncCustData( ITypeInfo * iface,
    UINT index, REFGUID guid, VARIANT *pVarVal)
{
	ICOM_THIS( TLBTypeInfo, iface);
    TLBCustData *pCData=NULL;
    TLBFuncDesc * pFDesc; 
    int i;
    for(i=0, pFDesc=This->funclist; i!=index && pFDesc; i++,
            pFDesc=pFDesc->next)
        ;
    if(pFDesc)
        for(pCData=pFDesc->pCustData; pCData; pCData = pCData->next)
            if( IsEqualIID(guid, &pCData->guid)) break;
    if(TRACE_ON(typelib)){
        char xriid[50];
        WINE_StringFromCLSID((LPCLSID)guid,xriid);
        TRACE_(typelib)("(%p) guid %s %s found!x)\n",This,xriid,
                pCData? "" : "NOT");
    }
    if(pCData){
        VariantInit( pVarVal);
        VariantCopy( pVarVal, &pCData->data);
        return S_OK;
    }
    return E_INVALIDARG;  /* FIXME: correct? */
}

/* ITypeInfo2::GetParamCustData
 *
 * Gets the custom data
 */
static HRESULT WINAPI ITypeInfo2_fnGetParamCustData( ITypeInfo * iface,
    UINT indexFunc, UINT indexParam, REFGUID guid, VARIANT *pVarVal)
{   
	ICOM_THIS( TLBTypeInfo, iface);
    TLBCustData *pCData=NULL;
    TLBFuncDesc * pFDesc; 
    int i;
    for(i=0, pFDesc=This->funclist; i!=indexFunc && pFDesc; i++,
            pFDesc=pFDesc->next)
        ;
    if(pFDesc && indexParam >=0 && indexParam<pFDesc->funcdesc.cParams)
        for(pCData=pFDesc->pParamDesc[indexParam].pCustData; pCData; 
                pCData = pCData->next)
            if( IsEqualIID(guid, &pCData->guid)) break;
    if(TRACE_ON(typelib)){
        char xriid[50];
        WINE_StringFromCLSID((LPCLSID)guid,xriid);
        TRACE_(typelib)("(%p) guid %s %s found!x)\n",This,xriid,
                pCData? "" : "NOT");
    }
    if(pCData){
        VariantInit( pVarVal);
        VariantCopy( pVarVal, &pCData->data);
        return S_OK;
    }
    return E_INVALIDARG;  /* FIXME: correct? */
}

/* ITypeInfo2::GetVarcCustData
 *
 * Gets the custom data
 */
static HRESULT WINAPI ITypeInfo2_fnGetVarCustData( ITypeInfo * iface,
    UINT index, REFGUID guid, VARIANT *pVarVal)
{   
	ICOM_THIS( TLBTypeInfo, iface);
    TLBCustData *pCData=NULL;
    TLBVarDesc * pVDesc; 
    int i;
    for(i=0, pVDesc=This->varlist; i!=index && pVDesc; i++,
            pVDesc=pVDesc->next)
        ;
    if(pVDesc)
        for(pCData=pVDesc->pCustData; pCData; pCData = pCData->next)
            if( IsEqualIID(guid, &pCData->guid)) break;
    if(TRACE_ON(typelib)){
        char xriid[50];
        WINE_StringFromCLSID((LPCLSID)guid,xriid);
        TRACE_(typelib)("(%p) guid %s %s found!x)\n",This,xriid,
                pCData? "" : "NOT");
    }
    if(pCData){
        VariantInit( pVarVal);
        VariantCopy( pVarVal, &pCData->data);
        return S_OK;
    }
    return E_INVALIDARG;  /* FIXME: correct? */
}

/* ITypeInfo2::GetImplcCustData
 *
 * Gets the custom data
 */
static HRESULT WINAPI ITypeInfo2_fnGetImplTypeCustData( ITypeInfo * iface,
    UINT index, REFGUID guid, VARIANT *pVarVal)
{   
	ICOM_THIS( TLBTypeInfo, iface);
    TLBCustData *pCData=NULL;
    TLBRefType * pRDesc; 
    int i;
    for(i=0, pRDesc=This->impltypelist; i!=index && pRDesc; i++,
            pRDesc=pRDesc->next)
        ;
    if(pRDesc)
        for(pCData=pRDesc->pCustData; pCData; pCData = pCData->next)
            if( IsEqualIID(guid, &pCData->guid)) break;
    if(TRACE_ON(typelib)){
        char xriid[50];
        WINE_StringFromCLSID((LPCLSID)guid,xriid);
        TRACE_(typelib)("(%p) guid %s %s found!x)\n",This,xriid,
                pCData? "" : "NOT");
    }
    if(pCData){
        VariantInit( pVarVal);
        VariantCopy( pVarVal, &pCData->data);
        return S_OK;
    }
    return E_INVALIDARG;  /* FIXME: correct? */
}

/* ITypeInfo2::GetDocumentation2
 * 
 * Retrieves the documentation string, the complete Help file name and path,
 * the localization context to use, and the context ID for the library Help
 * topic in the Help file.
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetDocumentation2( ITypeInfo * iface,
    MEMBERID memid, LCID lcid, BSTR *pbstrHelpString,
    INT *pdwHelpStringContext, BSTR *pbstrHelpStringDll)
{
	ICOM_THIS( TLBTypeInfo, iface);
    TLBFuncDesc * pFDesc; 
    TLBVarDesc * pVDesc; 
    TRACE_(typelib)("(%p) memid %ld lcid(0x%lx)  HelpString(%p) "
           "HelpStringContext(%p) HelpStringDll(%p)\n",
        This, memid, lcid, pbstrHelpString, pdwHelpStringContext,
        pbstrHelpStringDll );
    /* the help string should be obtained from the helpstringdll,
     * using the _DLLGetDocumentation function, based on the supplied
     * lcid. Nice to do sometime...
     */
    if(memid==MEMBERID_NIL){ /* documentation for the typeinfo */
        if(pbstrHelpString)
            *pbstrHelpString=TLB_DupAtoBstr(This->Name);
        if(pdwHelpStringContext)
            *pdwHelpStringContext=This->dwHelpStringContext;
        if(pbstrHelpStringDll)
            *pbstrHelpStringDll=
                TLB_DupAtoBstr(This->pTypeLib->HelpStringDll);/* FIXME */
        return S_OK;
    }else {/* for a member */
    for(pFDesc=This->funclist; pFDesc; pFDesc=pFDesc->next)
        if(pFDesc->funcdesc.memid==memid){
             if(pbstrHelpString)
                *pbstrHelpString=TLB_DupAtoBstr(pFDesc->HelpString);
            if(pdwHelpStringContext)
                *pdwHelpStringContext=pFDesc->HelpStringContext;
            if(pbstrHelpStringDll)
                *pbstrHelpStringDll=
                    TLB_DupAtoBstr(This->pTypeLib->HelpStringDll);/* FIXME */
        return S_OK;
    }
    for(pVDesc=This->varlist; pVDesc; pVDesc=pVDesc->next)
        if(pVDesc->vardesc.memid==memid){
             if(pbstrHelpString)
                *pbstrHelpString=TLB_DupAtoBstr(pVDesc->HelpString);
            if(pdwHelpStringContext)
                *pdwHelpStringContext=pVDesc->HelpStringContext;
            if(pbstrHelpStringDll)
                *pbstrHelpStringDll=
                    TLB_DupAtoBstr(This->pTypeLib->HelpStringDll);/* FIXME */
            return S_OK;
        }
    }
    return TYPE_E_ELEMENTNOTFOUND;
}

/* ITypeInfo2::GetAllCustData
 *
 * Gets all custom data items for the Type info. 
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetAllCustData( ITypeInfo * iface,
    CUSTDATA *pCustData)
{
	ICOM_THIS( TLBTypeInfo, iface);
    TLBCustData *pCData;
    int i;
    TRACE_(typelib)("(%p) returning %d items\n", This, This->ctCustData); 
    pCustData->prgCustData = TLB_Alloc(This->ctCustData * sizeof(CUSTDATAITEM));
    if(pCustData->prgCustData ){
        pCustData->cCustData=This->ctCustData;
        for(i=0, pCData=This->pCustData; pCData; i++, pCData = pCData->next){
            pCustData->prgCustData[i].guid=pCData->guid;
            VariantCopy(& pCustData->prgCustData[i].varValue, & pCData->data);
        }
    }else{
        ERR_(typelib)(" OUT OF MEMORY! \n");
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

/* ITypeInfo2::GetAllFuncCustData
 *
 * Gets all custom data items for the specified Function
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetAllFuncCustData( ITypeInfo * iface,
    UINT index, CUSTDATA *pCustData)
{
	ICOM_THIS( TLBTypeInfo, iface);
    TLBCustData *pCData;
    TLBFuncDesc * pFDesc; 
    int i;
    TRACE_(typelib)("(%p) index %d\n", This, index); 
    for(i=0, pFDesc=This->funclist; i!=index && pFDesc; i++,
            pFDesc=pFDesc->next)
        ;
    if(pFDesc){
        pCustData->prgCustData =
            TLB_Alloc(pFDesc->ctCustData * sizeof(CUSTDATAITEM));
        if(pCustData->prgCustData ){
            pCustData->cCustData=pFDesc->ctCustData;
            for(i=0, pCData=pFDesc->pCustData; pCData; i++,
                    pCData = pCData->next){
                pCustData->prgCustData[i].guid=pCData->guid;
                VariantCopy(& pCustData->prgCustData[i].varValue,
                        & pCData->data);
            }
        }else{
            ERR_(typelib)(" OUT OF MEMORY! \n");
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }
    return TYPE_E_ELEMENTNOTFOUND;
}

/* ITypeInfo2::GetAllParamCustData
 *
 * Gets all custom data items for the Functions
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetAllParamCustData( ITypeInfo * iface,
    UINT indexFunc, UINT indexParam, CUSTDATA *pCustData)
{
	ICOM_THIS( TLBTypeInfo, iface);
    TLBCustData *pCData=NULL;
    TLBFuncDesc * pFDesc; 
    int i;
    TRACE_(typelib)("(%p) index %d\n", This, indexFunc); 
    for(i=0, pFDesc=This->funclist; i!=indexFunc && pFDesc; i++,
            pFDesc=pFDesc->next)
        ;
    if(pFDesc && indexParam >=0 && indexParam<pFDesc->funcdesc.cParams){
        pCustData->prgCustData = 
            TLB_Alloc(pFDesc->pParamDesc[indexParam].ctCustData *
                    sizeof(CUSTDATAITEM));
        if(pCustData->prgCustData ){
            pCustData->cCustData=pFDesc->pParamDesc[indexParam].ctCustData;
            for(i=0, pCData=pFDesc->pParamDesc[indexParam].pCustData;
                    pCData; i++, pCData = pCData->next){
                pCustData->prgCustData[i].guid=pCData->guid;
                VariantCopy(& pCustData->prgCustData[i].varValue,
                        & pCData->data);
            }
        }else{
            ERR_(typelib)(" OUT OF MEMORY! \n");
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }
    return TYPE_E_ELEMENTNOTFOUND;
}

/* ITypeInfo2::GetAllVarCustData
 *
 * Gets all custom data items for the specified Variable
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetAllVarCustData( ITypeInfo * iface,
    UINT index, CUSTDATA *pCustData)
{
	ICOM_THIS( TLBTypeInfo, iface);
    TLBCustData *pCData;
    TLBVarDesc * pVDesc; 
    int i;
    TRACE_(typelib)("(%p) index %d\n", This, index); 
    for(i=0, pVDesc=This->varlist; i!=index && pVDesc; i++,
            pVDesc=pVDesc->next)
        ;
    if(pVDesc){
        pCustData->prgCustData =
            TLB_Alloc(pVDesc->ctCustData * sizeof(CUSTDATAITEM));
        if(pCustData->prgCustData ){
            pCustData->cCustData=pVDesc->ctCustData;
            for(i=0, pCData=pVDesc->pCustData; pCData; i++,
                    pCData = pCData->next){
                pCustData->prgCustData[i].guid=pCData->guid;
                VariantCopy(& pCustData->prgCustData[i].varValue,
                        & pCData->data);
            }
        }else{
            ERR_(typelib)(" OUT OF MEMORY! \n");
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }
    return TYPE_E_ELEMENTNOTFOUND;
}

/* ITypeInfo2::GetAllImplCustData
 *
 * Gets all custom data items for the specified implementation type
 *
 */
static HRESULT WINAPI ITypeInfo2_fnGetAllImplTypeCustData( ITypeInfo * iface,
    UINT index, CUSTDATA *pCustData)
{
	ICOM_THIS( TLBTypeInfo, iface);
    TLBCustData *pCData;
    TLBRefType * pRDesc; 
    int i;
    TRACE_(typelib)("(%p) index %d\n", This, index); 
    for(i=0, pRDesc=This->impltypelist; i!=index && pRDesc; i++,
            pRDesc=pRDesc->next)
        ;
    if(pRDesc){
        pCustData->prgCustData =
            TLB_Alloc(pRDesc->ctCustData * sizeof(CUSTDATAITEM));
        if(pCustData->prgCustData ){
            pCustData->cCustData=pRDesc->ctCustData;
            for(i=0, pCData=pRDesc->pCustData; pCData; i++,
                    pCData = pCData->next){
                pCustData->prgCustData[i].guid=pCData->guid;
                VariantCopy(& pCustData->prgCustData[i].varValue,
                        & pCData->data);
            }
        }else{
            ERR_(typelib)(" OUT OF MEMORY! \n");
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }
    return TYPE_E_ELEMENTNOTFOUND;
}

