/*
 * DDEML library
 *
 * Copyright 1997 Alexandre Julliard
 * Copyright 1997 Len White
 */

/* Only empty stubs for now */

#include <stdlib.h>
#include <strings.h>
#include "ddeml.h"
#include "debug.h"
#include "windows.h"
#include "heap.h"

/* FIXME: What are these values? */
#define DMLERR_NO_ERROR		0

/* Has defined in atom.c file.
 */
#define MAX_ATOM_LEN              255

/* Maximum buffer size ( including the '\0' ).
 */
#define MAX_BUFFER_LEN            (MAX_ATOM_LEN + 1)

static LONG     DDE_current_handle;

/* This is a simple list to keep track of the strings created
 * by DdeCreateStringHandle.  The list is used to free
 * the strings whenever DdeUninitialize is called.
 * This mechanism is not complete and does not handle multiple instances.
 * Most of the DDE API use a DWORD parameter indicating witch instance
 * of a given program is calling them.  The API are supposed to
 * associate the data to the instance that created it.
 */
typedef struct tagHSZNode HSZNode;
struct tagHSZNode
{
    HSZNode* next;
    HSZ hsz;
};

/* Start off the list pointer with a NULL.
 */
static HSZNode* pHSZNodes = NULL;


/******************************************************************************
 *            RemoveHSZNodes    (INTERNAL)
 *
 * Remove a node from the list of HSZ nodes.
 */
static void RemoveHSZNode( DWORD idInst, HSZ hsz )
{
    HSZNode* pPrev = NULL;
    HSZNode* pCurrent = NULL;

    /* Set the current node at the start of the list.
     */
    pCurrent = pHSZNodes;
    /* While we have more nodes.
     */
    while( pCurrent != NULL )
    {
        /* If we found the node we were looking for.
         */
        if( pCurrent->hsz == hsz )
        {
            /* Remove the node.
             */
            /* If the first node in the list is to to be removed.
             * Set the global list pointer to the next node.
             */
            if( pCurrent == pHSZNodes )
            {
                pHSZNodes = pCurrent->next;
            }
            /* Just fix the pointers has to skip the current
             * node so we can delete it.
             */
            else
            {
                pPrev->next = pCurrent->next;
            }
            /* Destroy this node.
             */
            free( pCurrent );
            break;
        }
        /* Save the previous node pointer.
         */
        pPrev = pCurrent;
        /* Move on to the next node.
         */
        pCurrent = pCurrent->next;
    }
}

/******************************************************************************
 *            FreeAndRemoveHSZNodes    (INTERNAL)
 *
 * Frees up all the strings still allocated in the list and
 * remove all the nodes from the list of HSZ nodes.
 */
static void FreeAndRemoveHSZNodes( DWORD idInst )
{
    /* Free any strings created in this instance.
     */
    while( pHSZNodes != NULL )
    {
        DdeFreeStringHandle32( idInst, pHSZNodes->hsz );
    }
}

/******************************************************************************
 *            InsertHSZNode    (INTERNAL)
 *
 * Insert a node to the head of the list.
 */
static void InsertHSZNode( DWORD idInst, HSZ hsz )
{
    if( hsz != 0 )
    {
        HSZNode* pNew = NULL;
        /* Create a new node for this HSZ.
         */
        pNew = (HSZNode*) malloc( sizeof( HSZNode ) );
        if( pNew != NULL )
        {
            /* Set the handle value.
             */
            pNew->hsz = hsz;
            /* Attach the node to the head of the list.
             */
            pNew->next = pHSZNodes;
            /* The new node is now at the head of the list
             * so set the global list pointer to it.
             */
            pHSZNodes = pNew;
        }
    }
}


/******************************************************************************
 *            DdeInitialize16   (DDEML.2)
 */
UINT16 WINAPI DdeInitialize16( LPDWORD pidInst, PFNCALLBACK16 pfnCallback,
                               DWORD afCmd, DWORD ulRes)
{
    return (UINT16)DdeInitialize32A(pidInst,(PFNCALLBACK32)pfnCallback,
                                    afCmd, ulRes);
}


/******************************************************************************
 *            DdeInitialize32A   (USER32.106)
 */
UINT32 WINAPI DdeInitialize32A( LPDWORD pidInst, PFNCALLBACK32 pfnCallback,
                                DWORD afCmd, DWORD ulRes )
{
    return DdeInitialize32W(pidInst,pfnCallback,afCmd,ulRes);
}


/******************************************************************************
 * DdeInitialize32W [USER32.107]
 * Registers an application with the DDEML
 *
 * PARAMS
 *    pidInst     [I] Pointer to instance identifier
 *    pfnCallback [I] Pointer to callback function
 *    afCmd       [I] Set of command and filter flags
 *    ulRes       [I] Reserved
 *
 * RETURNS
 *    Success: DMLERR_NO_ERROR
 *    Failure: DMLERR_DLL_USAGE, DMLERR_INVALIDPARAMETER, DMLERR_SYS_ERROR
 */
UINT32 WINAPI DdeInitialize32W( LPDWORD pidInst, PFNCALLBACK32 pfnCallback,
                                DWORD afCmd, DWORD ulRes )
{
    FIXME(ddeml, "(%p,%p,0x%lx,%ld): stub\n",pidInst,pfnCallback,afCmd,ulRes);

    if(pidInst)
      *pidInst = 0;

    if( ulRes )
        ERR(dde, "Reserved value not zero?  What does this mean?\n");

    return DMLERR_NO_ERROR;
}


/*****************************************************************
 *            DdeUninitialize16   (DDEML.3)
 */
BOOL16 WINAPI DdeUninitialize16( DWORD idInst )
{
    return (BOOL16)DdeUninitialize32( idInst );
}


/*****************************************************************
 * DdeUninitialize32 [USER32.119]  Frees DDEML resources
 *
 * PARAMS
 *    idInst [I] Instance identifier
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL32 WINAPI DdeUninitialize32( DWORD idInst )
{

    FIXME(ddeml, "(%ld): stub\n", idInst);

    /* Free the nodes that were not freed by this instance
     * and remove the nodes from the list of HSZ nodes.
     */
    FreeAndRemoveHSZNodes( idInst );
    
    return TRUE;
}


/*****************************************************************
 * DdeConnectList16 [DDEML.4]
 */
HCONVLIST WINAPI DdeConnectList16( DWORD idInst, HSZ hszService, HSZ hszTopic,
                 HCONVLIST hConvList, LPCONVCONTEXT16 pCC )
{
    return DdeConnectList32(idInst, hszService, hszTopic, hConvList, 
                            (LPCONVCONTEXT32)pCC);
}


/******************************************************************************
 * DdeConnectList32 [USER32.93]  Establishes conversation with DDE servers
 *
 * PARAMS
 *    idInst     [I] Instance identifier
 *    hszService [I] Handle to service name string
 *    hszTopic   [I] Handle to topic name string
 *    hConvList  [I] Handle to conversation list
 *    pCC        [I] Pointer to structure with context data
 *
 * RETURNS
 *    Success: Handle to new conversation list
 *    Failure: 0
 */
HCONVLIST WINAPI DdeConnectList32( DWORD idInst, HSZ hszService, HSZ hszTopic,
                 HCONVLIST hConvList, LPCONVCONTEXT32 pCC )
{
    FIXME(ddeml, "(%ld,%ld,%ld,%ld,%p): stub\n", idInst, hszService, hszTopic,
          hConvList,pCC);
    return 1;
}


/*****************************************************************
 * DdeQueryNextServer16 [DDEML.5]
 */
HCONV WINAPI DdeQueryNextServer16( HCONVLIST hConvList, HCONV hConvPrev )
{
    return DdeQueryNextServer32(hConvList, hConvPrev);
}


/*****************************************************************
 * DdeQueryNextServer32 [USER32.112]
 */
HCONV WINAPI DdeQueryNextServer32( HCONVLIST hConvList, HCONV hConvPrev )
{
    FIXME(ddeml, "(%ld,%ld): stub\n",hConvList,hConvPrev);
    return 0;
}

/*****************************************************************
 * DdeQueryString32A [USER32.113]
 */
DWORD WINAPI DdeQueryString32A(DWORD idInst, HSZ hsz, LPSTR psz, DWORD cchMax, INT32 iCodePage)
{
    DWORD ret = 0;
    CHAR pString[MAX_BUFFER_LEN];

    FIXME(ddeml,
         "(%ld, 0x%lx, %p, %ld, %d): stub\n",
         idInst,
         hsz,
         psz, 
         cchMax,
         iCodePage);

    if( iCodePage == CP_WINANSI )
    {
        /* If psz is null, we have to return only the length
         * of the string.
         */
        if( psz == NULL )
        {
            psz = pString;
            cchMax = MAX_BUFFER_LEN;
}

        ret = GlobalGetAtomName32A( hsz, (LPSTR)psz, cchMax );
    }
    
    return ret;
}

/*****************************************************************
 * DdeQueryString32W [USER32.114]
 */
DWORD WINAPI DdeQueryString32W(DWORD idInst, HSZ hsz, LPWSTR psz, DWORD cchMax, INT32 iCodePage)
{
    DWORD ret = 0;
    WCHAR pString[MAX_BUFFER_LEN];
    int factor = 1;

    FIXME(ddeml,
         "(%ld, 0x%lx, %p, %ld, %d): stub\n",
         idInst,
         hsz,
         psz, 
         cchMax,
         iCodePage);

    if( iCodePage == CP_WINUNICODE )
    {
        /* If psz is null, we have to return only the length
         * of the string.
         */
        if( psz == NULL )
        {
            psz = pString;
            cchMax = MAX_BUFFER_LEN;
            /* Note: According to documentation if the psz parameter
             * was NULL this API must return the length of the string in bytes.
             */
            factor = (int) sizeof(WCHAR)/sizeof(BYTE);
        }
        ret = GlobalGetAtomName32W( hsz, (LPWSTR)psz, cchMax ) * factor;
    }
    return ret;
}


/*****************************************************************
 *            DdeDisconnectList (DDEML.6)
 */
BOOL16 WINAPI DdeDisconnectList16( HCONVLIST hConvList )
{
    return (BOOL16)DdeDisconnectList32(hConvList);
}


/******************************************************************************
 * DdeDisconnectList32 [USER32.98]  Destroys list and terminates conversations
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL32 WINAPI DdeDisconnectList32(
    HCONVLIST hConvList) /* [in] Handle to conversation list */
{
    FIXME(ddeml, "(%ld): stub\n", hConvList);
    return TRUE;
}


/*****************************************************************
 *            DdeConnect16   (DDEML.7)
 */
HCONV WINAPI DdeConnect16( DWORD idInst, HSZ hszService, HSZ hszTopic,
                           LPCONVCONTEXT16 pCC )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}


/*****************************************************************
 *            DdeConnect32   (USER32.92)
 */
HCONV WINAPI DdeConnect32( DWORD idInst, HSZ hszService, HSZ hszTopic,
                           LPCONVCONTEXT32 pCC )
{
    FIXME(ddeml, "(0x%lx,%ld,%ld,%p): stub\n",idInst,hszService,hszTopic,
          pCC);
    return 0;
}


/*****************************************************************
 *            DdeDisconnect16   (DDEML.8)
 */
BOOL16 WINAPI DdeDisconnect16( HCONV hConv )
{
    return (BOOL16)DdeDisconnect32( hConv );
}

/*****************************************************************
 *            DdeSetUserHandle (DDEML.10)
 */
BOOL16 WINAPI DdeSetUserHandle( HCONV hConv, DWORD id, DWORD hUser )
{
    FIXME( ddeml, "(%ld,%ld,%ld): stub\n",hConv,id, hUser );
    return 0;
}

/*****************************************************************
 *            DdeCreateDataHandle16 (DDEML.14)
 */
HDDEDATA WINAPI DdeCreateDataHandle16( DWORD idInst, LPBYTE pSrc, DWORD cb, 
                                     DWORD cbOff, HSZ hszItem, UINT16 wFmt, 
                                     UINT16 afCmd )
{
    return DdeCreateDataHandle32(idInst,
                                pSrc,
                                cb,
                                cbOff,
                                hszItem,
                                wFmt,
                                afCmd);
}

/*****************************************************************
 *            DdeCreateDataHandle32 (USER32.94)
 */
HDDEDATA WINAPI DdeCreateDataHandle32( DWORD idInst, LPBYTE pSrc, DWORD cb, 
                                       DWORD cbOff, HSZ hszItem, UINT32 wFmt, 
                                       UINT32 afCmd )
{
    FIXME( ddeml,
          "(%ld,%p,%ld,%ld,0x%lx,%d,%d): stub\n",
          idInst,
          pSrc,
          cb,
          cbOff,
           hszItem,
          wFmt, 
          afCmd );

    return 0;
}

/*****************************************************************
 *            DdeDisconnect32   (USER32.97)
 */
BOOL32 WINAPI DdeDisconnect32( HCONV hConv )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}


/*****************************************************************
 *            DdeReconnect   (DDEML.37) (USER32.115)
 */
HCONV WINAPI DdeReconnect( HCONV hConv )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}


/*****************************************************************
 *            DdeCreateStringHandle16   (DDEML.21)
 */
HSZ WINAPI DdeCreateStringHandle16( DWORD idInst, LPCSTR str, INT16 codepage )
{
    return DdeCreateStringHandle32A( idInst, str, codepage );
}


/*****************************************************************
 * DdeCreateStringHandle32A [USER32.95]
 *
 * RETURNS
 *    Success: String handle
 *    Failure: 0
 */
HSZ WINAPI DdeCreateStringHandle32A( DWORD idInst, LPCSTR psz, INT32 codepage )
{
  HSZ hsz = 0;
  TRACE(ddeml, "(%ld,%s,%d): stub\n",idInst,debugstr_a(psz),codepage);
  
  if (codepage==CP_WINANSI)
  {
      hsz = GlobalAddAtom32A (psz);
      /* Save the handle so we know to clean it when
       * uninitialize is called.
       */
      InsertHSZNode( idInst, hsz );
      return hsz;
  }
    return 0;  
}


/******************************************************************************
 * DdeCreateStringHandle32W [USER32.96]  Creates handle to identify string
 *
 * RETURNS
 *    Success: String handle
 *    Failure: 0
 */
HSZ WINAPI DdeCreateStringHandle32W(
    DWORD idInst,   /* [in] Instance identifier */
    LPCWSTR psz,    /* [in] Pointer to string */
    INT32 codepage) /* [in] Code page identifier */
{
  HSZ hsz = 0;

    FIXME(ddeml, "(%ld,%s,%d): stub\n",idInst,debugstr_w(psz),codepage);

  if (codepage==CP_WINUNICODE)
  {
      hsz = GlobalAddAtom32W (psz);
      /* Save the handle so we know to clean it when
       * uninitialize is called.
       */
      InsertHSZNode( idInst, hsz );
      return hsz;
}
  return 0;
}


/*****************************************************************
 *            DdeFreeStringHandle16   (DDEML.22)
 */
BOOL16 WINAPI DdeFreeStringHandle16( DWORD idInst, HSZ hsz )
{
    return (BOOL32)DdeFreeStringHandle32( idInst, hsz );
}


/*****************************************************************
 *            DdeFreeStringHandle32   (USER32.101)
 * RETURNS: success: nonzero
 *          fail:    zero
 */
BOOL32 WINAPI DdeFreeStringHandle32( DWORD idInst, HSZ hsz )
{
    TRACE( ddeml, "(%ld,%ld): stub\n",idInst, hsz );
    /* Remove the node associated with this HSZ.
     */
    RemoveHSZNode( idInst, hsz );
    /* Free the string associated with this HSZ.
     */
    return GlobalDeleteAtom (hsz) ? 0 : hsz;
}


/*****************************************************************
 *            DdeFreeDataHandle16   (DDEML.19)
 */
BOOL16 WINAPI DdeFreeDataHandle16( HDDEDATA hData )
{
    return (BOOL32)DdeFreeDataHandle32( hData );
}


/*****************************************************************
 *            DdeFreeDataHandle32   (USER32.100)
 */
BOOL32 WINAPI DdeFreeDataHandle32( HDDEDATA hData )
{
    FIXME( ddeml, "empty stub\n" );
    return TRUE;
}




/*****************************************************************
 *            DdeKeepStringHandle16   (DDEML.24)
 */
BOOL16 WINAPI DdeKeepStringHandle16( DWORD idInst, HSZ hsz )
{
    return (BOOL32)DdeKeepStringHandle32( idInst, hsz );
}


/*****************************************************************
 *            DdeKeepStringHandle32  (USER32.108)
 */
BOOL32 WINAPI DdeKeepStringHandle32( DWORD idInst, HSZ hsz )
{
    FIXME( ddeml, "empty stub\n" );
    return TRUE;
}


/*****************************************************************
 *            DdeClientTransaction16  (DDEML.11)
 */
HDDEDATA WINAPI DdeClientTransaction16( LPVOID pData, DWORD cbData,
                                        HCONV hConv, HSZ hszItem, UINT16 wFmt,
                                        UINT16 wType, DWORD dwTimeout,
                                        LPDWORD pdwResult )
{
    return DdeClientTransaction32( (LPBYTE)pData, cbData, hConv, hszItem,
                                   wFmt, wType, dwTimeout, pdwResult );
}


/*****************************************************************
 *            DdeClientTransaction32  (USER32.90)
 */
HDDEDATA WINAPI DdeClientTransaction32( LPBYTE pData, DWORD cbData,
                                        HCONV hConv, HSZ hszItem, UINT32 wFmt,
                                        UINT32 wType, DWORD dwTimeout,
                                        LPDWORD pdwResult )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}

/*****************************************************************
 *            DdeAbandonTransaction (DDEML.12)
 */
BOOL16 WINAPI DdeAbandonTransaction( DWORD idInst, HCONV hConv, 
                                     DWORD idTransaction )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}


/*****************************************************************
 * DdePostAdvise16 [DDEML.13]
 */
BOOL16 WINAPI DdePostAdvise16( DWORD idInst, HSZ hszTopic, HSZ hszItem )
{
    return (BOOL16)DdePostAdvise32(idInst, hszTopic, hszItem);
}


/******************************************************************************
 * DdePostAdvise32 [USER32.110]  Send transaction to DDE callback function.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL32 WINAPI DdePostAdvise32(
    DWORD idInst, /* [in] Instance identifier */
    HSZ hszTopic, /* [in] Handle to topic name string */
    HSZ hszItem)  /* [in] Handle to item name string */
{
    FIXME(ddeml, "(%ld,%ld,%ld): stub\n",idInst,hszTopic,hszItem);
    return TRUE;
}


/*****************************************************************
 *            DdeAddData (DDEML.15)
 */
HDDEDATA WINAPI DdeAddData( HDDEDATA hData, LPBYTE pSrc, DWORD cb,
                            DWORD cbOff )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}


/******************************************************************************
 * DdeGetData32 [USER32.102]  Copies data from DDE object ot local buffer
 *
 * RETURNS
 *    Size of memory object associated with handle
 */
DWORD WINAPI DdeGetData32(
    HDDEDATA hData, /* [in] Handle to DDE object */
    LPBYTE pDst,    /* [in] Pointer to destination buffer */
    DWORD cbMax,    /* [in] Amount of data to copy */
    DWORD cbOff)    /* [in] Offset to beginning of data */
{
    FIXME(ddeml, "(%ld,%p,%ld,%ld): stub\n",hData,pDst,cbMax,cbOff);
    return cbMax;
}


/*****************************************************************
 * DdeGetData16 [DDEML.16]
 */
DWORD WINAPI DdeGetData16(
    HDDEDATA hData,
    LPBYTE pDst,
    DWORD cbMax, 
    DWORD cbOff)
{
    return DdeGetData32(hData, pDst, cbMax, cbOff);
}


/*****************************************************************
 *            DdeAccessData16 (DDEML.17)
 */
LPBYTE WINAPI DdeAccessData16( HDDEDATA hData, LPDWORD pcbDataSize )
{
     return DdeAccessData32(hData, pcbDataSize);
}

/*****************************************************************
 *            DdeAccessData32 (USER32.88)
 */
LPBYTE WINAPI DdeAccessData32( HDDEDATA hData, LPDWORD pcbDataSize )
{
     FIXME( ddeml, "(%ld,%p): stub\n", hData, pcbDataSize);
     return 0;
}

/*****************************************************************
 *            DdeUnaccessData16 (DDEML.18)
 */
BOOL16 WINAPI DdeUnaccessData16( HDDEDATA hData )
{
     return DdeUnaccessData32(hData);
}

/*****************************************************************
 *            DdeUnaccessData32 (USER32.118)
 */
BOOL32 WINAPI DdeUnaccessData32( HDDEDATA hData )
{
     FIXME( ddeml, "(0x%lx): stub\n", hData);

     return 0;
}

/*****************************************************************
 *            DdeEnableCallback16 (DDEML.26)
 */
BOOL16 WINAPI DdeEnableCallback16( DWORD idInst, HCONV hConv, UINT16 wCmd )
{
     return DdeEnableCallback32(idInst, hConv, wCmd);
}

/*****************************************************************
 *            DdeEnableCallback32 (USER32.99)
 */
BOOL32 WINAPI DdeEnableCallback32( DWORD idInst, HCONV hConv, UINT32 wCmd )
{
     FIXME( ddeml, "(%ld, 0x%lx, %d) stub\n", idInst, hConv, wCmd);

     return 0;
}

/*****************************************************************
 *            DdeNameService16  (DDEML.27)
 */
HDDEDATA WINAPI DdeNameService16( DWORD idInst, HSZ hsz1, HSZ hsz2,
                                  UINT16 afCmd )
{
    return DdeNameService32( idInst, hsz1, hsz2, afCmd );
}


/******************************************************************************
 * DdeNameService32 [USER32.109]  {Un}registers service name of DDE server
 *
 * PARAMS
 *    idInst [I] Instance identifier
 *    hsz1   [I] Handle to service name string
 *    hsz2   [I] Reserved
 *    afCmd  [I] Service name flags
 *
 * RETURNS
 *    Success: Non-zero
 *    Failure: 0
 */
HDDEDATA WINAPI DdeNameService32( DWORD idInst, HSZ hsz1, HSZ hsz2,
                UINT32 afCmd )
{
    FIXME(ddeml, "(%ld,%ld,%ld,%d): stub\n",idInst,hsz1,hsz2,afCmd);
    return 1;
}


/*****************************************************************
 *            DdeGetLastError16  (DDEML.20)
 */
UINT16 WINAPI DdeGetLastError16( DWORD idInst )
{
    return (UINT16)DdeGetLastError32( idInst );
}


/******************************************************************************
 * DdeGetLastError32 [USER32.103]  Gets most recent error code
 *
 * PARAMS
 *    idInst [I] Instance identifier
 *
 * RETURNS
 *    Last error code
 */
UINT32 WINAPI DdeGetLastError32( DWORD idInst )
{
    FIXME(ddeml, "(%ld): stub\n",idInst);
    return 0;
}


/*****************************************************************
 *            DdeCmpStringHandles16 (DDEML.36)
 */
int WINAPI DdeCmpStringHandles16( HSZ hsz1, HSZ hsz2 )
{
     return DdeCmpStringHandles32A(hsz1, hsz2);
}

/*****************************************************************
 *            DdeCmpStringHandles32A (USER32.91)
 *
 * Compares the value of two string handles.  This comparison is
 * not case sensitive.
 *
 * Returns:
 * -1 The value of hsz1 is zero or less than hsz2
 * 0  The values of hsz 1 and 2 are the same or both zero.
 * 1  The value of hsz2 is zero of less than hsz1
 */
int WINAPI DdeCmpStringHandles32A( HSZ hsz1, HSZ hsz2 )
{
    CHAR psz1[MAX_BUFFER_LEN];
    CHAR psz2[MAX_BUFFER_LEN];
    int ret = 0;
    int ret1, ret2;

    TRACE( ddeml, "handle 1, handle 2\n" );

    ret1 = GlobalGetAtomName32A( hsz1, psz1, MAX_BUFFER_LEN );
    ret2 = GlobalGetAtomName32A( hsz2, psz2, MAX_BUFFER_LEN );
    /* Make sure we found both strings.
     */
    if( ret1 == 0 && ret2 == 0 )
    {
        /* If both are not found, return both  "zero strings".
         */
        ret = 0;
    }
    else if( ret1 == 0 )
    {
        /* If hsz1 is a not found, return hsz1 is "zero string".
         */
        ret = -1;
    }
    else if( ret2 == 0 )
    {
        /* If hsz2 is a not found, return hsz2 is "zero string".
         */
        ret = 1;
    }
    else
    {
        /* Compare the two strings we got ( case insensitive ).
         */
        ret = strcasecmp( psz1, psz2 );
        /* Since strcmp returns any number smaller than
         * 0 when the first string is found to be less than
         * the second one we must make sure we are returning
         * the proper values.
         */
        if( ret < 0 )
        {
            ret = -1;
        }
        else if( ret > 0 )
        {
            ret = 1;
        }
    }

    return ret;
}

/*****************************************************************
 *            DdeCmpStringHandles32W (USER32.623)
 *
 * Compares the value of two string handles.  This comparison is
 * not case sensitive.
 *
 * Returns:
 * -1 The value of hsz1 is zero or less than hsz2
 * 0  The values of hsz 1 and 2 are the same or both zero.
 * 1  The value of hsz2 is zero of less than hsz1
 */
int WINAPI DdeCmpStringHandles32W( HSZ hsz1, HSZ hsz2 )
{
    WCHAR pwsz1[MAX_BUFFER_LEN];
    WCHAR pwsz2[MAX_BUFFER_LEN];
    int ret = 0;
    int ret1, ret2;

    TRACE( ddeml, "handle 1, handle 2\n" );

    ret1 = GlobalGetAtomName32W( hsz1, pwsz1, MAX_BUFFER_LEN );
    ret2 = GlobalGetAtomName32W( hsz2, pwsz2, MAX_BUFFER_LEN );
    /* Make sure we found both strings.
     */
    if( ret1 == 0 && ret2 == 0 )
    {
        /* If both are not found, return both  "zero strings".
         */
        ret = 0;
    }
    else if( ret1 == 0 )
    {
        /* If hsz1 is a not found, return hsz1 is "zero string".
         */
        ret = -1;
    }
    else if( ret2 == 0 )
    {
        /* If hsz2 is a not found, return hsz2 is "zero string".
         */
        ret = 1;
    }
    else
    {
        LPSTR psz1, psz2;
        psz1 = HEAP_strdupWtoA( GetProcessHeap(), 0, pwsz1 );
        psz2 = HEAP_strdupWtoA( GetProcessHeap(), 0, pwsz2 );
        if( psz1 != NULL && psz2 != NULL )
        {
            /* Compare the two strings we got ( case insensitive ).
             */
            ret = strcasecmp( psz1, psz2 );
            /* Since strcmp returns any number smaller than
             * 0 when the first string is found to be less than
             * the second one we must make sure we are returning
             * the proper values.
             */
            if( ret < 0 )
            {
                ret = -1;
            }
            else if( ret > 0 )
            {
                ret = 1;
            }
        }
        HeapFree( GetProcessHeap(), 0, psz1 );
        HeapFree( GetProcessHeap(), 0, psz2 );
    }
    return ret;
}


/*****************************************************************
 *            PackDDElParam (USER32.414)
 *
 * RETURNS
 *   success: nonzero
 *   failure: zero
 */
UINT32 WINAPI PackDDElParam(UINT32 msg, UINT32 uiLo, UINT32 uiHi)
{
    FIXME(ddeml, "stub.\n");
    return 0;
}


/*****************************************************************
 *            UnpackDDElParam (USER32.562)
 *
 * RETURNS
 *   success: nonzero
 *   failure: zero
 */
UINT32 WINAPI UnpackDDElParam(UINT32 msg, UINT32 lParam,
                              UINT32 *uiLo, UINT32 *uiHi)
{
    FIXME(ddeml, "stub.\n");
    return 0;
}


/*****************************************************************
 *            FreeDDElParam (USER32.204)
 *
 * RETURNS
 *   success: nonzero
 *   failure: zero
 */
UINT32 WINAPI FreeDDElParam(UINT32 msg, UINT32 lParam)
{
    FIXME(ddeml, "stub.\n");
    return 0;
}

/*****************************************************************
 *            ReuseDDElParam (USER32.446)
 *
 */
UINT32 WINAPI ReuseDDElParam(UINT32 lParam, UINT32 msgIn, UINT32 msgOut,
                             UINT32 uiLi, UINT32 uiHi)
{
    FIXME(ddeml, "stub.\n");
    return 0;
} 
