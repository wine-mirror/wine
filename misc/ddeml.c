/*
 * DDEML library
 *
 * Copyright 1997 Alexandre Julliard
 * Copyright 1997 Len White
 * Copyright 1999 Keith Matthews
 */

/* Only empty stubs for now */

#include <stdlib.h>
#include <string.h>
#include "winbase.h"
#include "winuser.h"
#include "ddeml.h"
#include "winerror.h"
#include "heap.h"
#include "shm_semaph.h"
#include "debug.h"
#include "winnt.h"

/* Has defined in atom.c file.
 */
#define MAX_ATOM_LEN              255

/* Maximum buffer size ( including the '\0' ).
 */
#define MAX_BUFFER_LEN            (MAX_ATOM_LEN + 1)


static DDE_HANDLE_ENTRY *DDE_Handle_Table_Base = NULL;
static DWORD 		DDE_Max_Assigned_Instance = 0;  // OK for present, may have to worry about wrap-around later
static const char	inst_string[]= "DDEMaxInstance";
static LPCWSTR 		DDEInstanceAccess = (LPCWSTR)&inst_string;
static const char	handle_string[] = "DDEHandleAccess";
static LPCWSTR      	DDEHandleAccess = (LPCWSTR)&handle_string;
static HANDLE	     	inst_count_mutex = 0;
static HANDLE	     	handle_mutex = 0;
       DDE_HANDLE_ENTRY *this_instance;
       SECURITY_ATTRIBUTES *s_att= NULL;
       DWORD 	     	err_no = 0;

/*  typedef struct {
	DWORD		nLength;
	LPVOID		lpSecurityDescriptor;
	BOOL		bInheritHandle;
}	SECURITY_ATTRIBUTES; */

#define TRUE	1
#define FALSE	0



/* This is a simple list to keep track of the strings created
 * by DdeCreateStringHandle.  The list is used to free
 * the strings whenever DdeUninitialize is called.
 * This mechanism is not complete and does not handle multiple instances.
 * Most of the DDE API use a DWORD parameter indicating which instance
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
 *
 ******************************************************************************
 *
 *	Change History
 *
 *  Vn       Date    	Author         		Comment
 *
 *  1.0      Dec 1998  Corel/Macadamian    Initial version
 *
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
 *
 ******************************************************************************
 *
 *	Change History
 *
 *  Vn       Date    	Author         		Comment
 *
 *  1.0      Dec 1998  Corel/Macadamian    Initial version
 *
 */
static void FreeAndRemoveHSZNodes( DWORD idInst )
{
    /* Free any strings created in this instance.
     */
    while( pHSZNodes != NULL )
    {
        DdeFreeStringHandle( idInst, pHSZNodes->hsz );
    }
}

/******************************************************************************
 *            InsertHSZNode    (INTERNAL)
 *
 * Insert a node to the head of the list.
 *
 ******************************************************************************
 *
 *	Change History
 *
 *  Vn       Date    	Author         		Comment
 *
 *  1.0      Dec 1998  Corel/Macadamian    Initial version
 *
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
 *	Release_reserved_mutex
 *
 *	generic routine to release a reserved mutex
 *
 *
 ******************************************************************************
 *
 *	Change History
 *
 *  Vn       Date    	Author         		Comment
 *
 *  1.0      Jan 1999  Keith Matthews        Initial version
 *  1.1	     Mar 1999  Keith Matthews	     Corrected Heap handling.
 *
 */
 DWORD Release_reserved_mutex (HANDLE mutex, LPTSTR mutex_name, BOOL release_handle_m, BOOL release_this_i )
{
	ReleaseMutex(mutex);
        if ( (err_no=GetLastError()) != 0 )
        {
                ERR(ddeml,"ReleaseMutex failed - %s mutex %li\n",mutex_name,err_no);
                HeapFree(SystemHeap, 0, this_instance);
		if ( release_handle_m )
		{
			ReleaseMutex(handle_mutex);
		}
                return DMLERR_SYS_ERROR;
         }
	if ( release_this_i )
	{
                HeapFree(SystemHeap, 0, this_instance);
	}
	return DMLERR_NO_ERROR;
}

/******************************************************************************
 *		IncrementInstanceId
 *
 *	generic routine to increment the max instance Id and allocate a new application instance
 *
 ******************************************************************************
 *
 *	Change History
 *
 *  Vn       Date    	Author         		Comment
 *
 *  1.0      Jan 1999  Keith Matthews        Initial version
 *
 */
DWORD IncrementInstanceId()
{
    	SECURITY_ATTRIBUTES s_attrib;
	/*  Need to set up Mutex in case it is not already present */
	// increment handle count & get value
	if ( !inst_count_mutex )
	{
		s_attrib.bInheritHandle = TRUE;
		s_attrib.lpSecurityDescriptor = NULL;
		s_attrib.nLength = sizeof(s_attrib);
		inst_count_mutex = CreateMutexW(&s_attrib,1,DDEInstanceAccess); // 1st time through
	} else {
		WaitForSingleObject(inst_count_mutex,1000); // subsequent calls
		/*  FIXME  - needs refinement with popup for timeout, also is timeout interval OK */
	}
	if ( (err_no=GetLastError()) != 0 )
	{
		ERR(ddeml,"CreateMutex failed - inst_count %li\n",err_no);
		err_no=Release_reserved_mutex (handle_mutex,"handle_mutex",0,1);
		return DMLERR_SYS_ERROR;
	}
	DDE_Max_Assigned_Instance++;
	this_instance->Instance_id = DDE_Max_Assigned_Instance;
	if (Release_reserved_mutex(inst_count_mutex,"instance_count",1,0)) return DMLERR_SYS_ERROR;
	return DMLERR_NO_ERROR;
}

/******************************************************************************
 *            DdeInitialize16   (DDEML.2)
 */
UINT16 WINAPI DdeInitialize16( LPDWORD pidInst, PFNCALLBACK16 pfnCallback,
                               DWORD afCmd, DWORD ulRes)
{
    TRACE(ddeml,"DdeInitialize16 called - calling DdeInitializeA\n");
    return (UINT16)DdeInitializeA(pidInst,(PFNCALLBACK)pfnCallback,
                                    afCmd, ulRes);
}


/******************************************************************************
 *            DdeInitializeA   (USER32.106)
 */
UINT WINAPI DdeInitializeA( LPDWORD pidInst, PFNCALLBACK pfnCallback,
                                DWORD afCmd, DWORD ulRes )
{
    TRACE(ddeml,"DdeInitializeA called - calling DdeInitializeW\n");
    return DdeInitializeW(pidInst,pfnCallback,afCmd,ulRes);
}


/******************************************************************************
 * DdeInitializeW [USER32.107]
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
 *
 ******************************************************************************
 *
 *	Change History
 *
 *  Vn       Date    	Author         		Comment
 *
 *  1.0      Pre 1998  Alexandre/Len	     Initial Stub
 *  1.1      Jan 1999  Keith Matthews        Initial (near-)complete version
 *  1.2	     Mar 1999  Keith Matthews	     Corrected Heap handling, CreateMutex failure handling
 *
 */
UINT WINAPI DdeInitializeW( LPDWORD pidInst, PFNCALLBACK pfnCallback,
                                DWORD afCmd, DWORD ulRes )
{
    DDE_HANDLE_ENTRY *reference_inst;
    SECURITY_ATTRIBUTES s_attrib;
    s_att = &s_attrib;

//  probably not really capable of handling mutliple processes, but should handle
//	multiple instances within one process

    if( ulRes )
    {
        ERR(dde, "Reserved value not zero?  What does this mean?\n");
        FIXME(ddeml, "(%p,%p,0x%lx,%ld): stub\n",pidInst,pfnCallback,afCmd,ulRes);
        /* trap this and no more until we know more */
        return DMLERR_NO_ERROR;
    }
    if (!pfnCallback ) 
    {
	/*  this one may be wrong - MS dll seems to accept the condition, leave this until we find out more !! */


        /* can't set up the instance with nothing to act as a callback */
        TRACE(ddeml,"No callback provided\n");
        return DMLERR_INVALIDPARAMETER; /* might be DMLERR_DLL_USAGE */
     }

     /* grab enough heap for one control struct - not really necessary for re-initialise
	but allows us to use same validation routines */
     this_instance= (DDE_HANDLE_ENTRY*)HeapAlloc( SystemHeap, 0, sizeof(DDE_HANDLE_ENTRY) );
     if ( this_instance == NULL )
     {
	// catastrophe !! warn user & abort
	ERR (ddeml,"Instance create failed - out of memory\n");
	return DMLERR_SYS_ERROR;
     }
     this_instance->Next_Entry = NULL;
     this_instance->Monitor=(afCmd|APPCLASS_MONITOR);

     // messy bit, spec implies that 'Client Only' can be set in 2 different ways, catch 1 here

     this_instance->Client_only=afCmd&APPCMD_CLIENTONLY;
     this_instance->Instance_id = *pidInst; // May need to add calling proc Id
     this_instance->CallBack=*pfnCallback;
     this_instance->Txn_count=0;
     this_instance->Unicode = TRUE;
     this_instance->Win16 = FALSE;
     this_instance->Monitor_flags = afCmd & MF_MASK;

     // isolate CBF flags in one go, expect this will go the way of all attempts to be clever !!

     this_instance->CBF_Flags=afCmd^((afCmd&MF_MASK)|((afCmd&APPCMD_MASK)|(afCmd&APPCLASS_MASK)));

     if ( ! this_instance->Client_only )
     {

	// Check for other way of setting Client-only !!

	this_instance->Client_only=(this_instance->CBF_Flags&CBF_FAIL_ALLSVRXACTIONS)
			==CBF_FAIL_ALLSVRXACTIONS;
     }

     TRACE(ddeml,"instance created - checking validity \n");

    if( *pidInst == 0 ) {
        /*  Initialisation of new Instance Identifier */
        TRACE(ddeml,"new instance, callback %p flags %lX\n",pfnCallback,afCmd);
	/*  Need to set up Mutex in case it is not already present */
	s_att->bInheritHandle = TRUE;
	s_att->lpSecurityDescriptor = NULL;
	s_att->nLength = sizeof(s_att);
	handle_mutex = CreateMutexW(s_att,1,DDEHandleAccess);
	if ( (err_no=GetLastError()) != 0 )
	{
		ERR(ddeml,"CreateMutex failed - handle list  %li\n",err_no);
                HeapFree(SystemHeap, 0, this_instance);
		return DMLERR_SYS_ERROR;
	}
	TRACE(ddeml,"Handle Mutex created/reserved\n");
        if (DDE_Handle_Table_Base == NULL ) 
	{
                /* can't be another instance in this case, assign to the base pointer */
                DDE_Handle_Table_Base= this_instance;

		// since first must force filter of XTYP_CONNECT and XTYP_WILDCONNECT for
		//    present
		//   -------------------------------      NOTE NOTE NOTE    --------------------------
		//
		//	   the manual is not clear if this condition
		//	applies to the first call to DdeInitialize from an application, or the 
		//	first call for a given callback !!!
		//

		this_instance->CBF_Flags=this_instance->CBF_Flags|APPCMD_FILTERINITS;
 		TRACE(ddeml,"First application instance detected OK\n");
		// allocate new instance ID
		if ((err_no = IncrementInstanceId()) ) return err_no;
   	} else {
                /* really need to chain the new one in to the latest here, but after checking conditions
                such as trying to start a conversation from an application trying to monitor */
                reference_inst =  DDE_Handle_Table_Base;
		TRACE(ddeml,"Subsequent application instance - starting checks\n");
                while ( reference_inst->Next_Entry != NULL ) 
		{
			//
			//	This set of tests will work if application uses same instance Id
			//	at application level once allocated - which is what manual implies
			//	should happen. If someone tries to be 
			//	clever (lazy ?) it will fail to pick up that later calls are for
			//	the same application - should we trust them ?
			//
                        if ( this_instance->Instance_id == reference_inst->Instance_id) 
			{
				// Check 1 - must be same Client-only state

                                if ( this_instance->Client_only != reference_inst->Client_only)
				{
					if ( Release_reserved_mutex(handle_mutex,"handle_mutex",0,1))
						return DMLERR_SYS_ERROR;
                                        return DMLERR_DLL_USAGE;
                                }

				// Check 2 - cannot use 'Monitor' with any non-monitor modes

                                if ( this_instance->Monitor != reference_inst->Monitor) 
				{
					if ( Release_reserved_mutex(handle_mutex,"handle_mutex",0,1))
						return DMLERR_SYS_ERROR;
                                        return DMLERR_INVALIDPARAMETER;
                                }

				// Check 3 - must supply different callback address

				if ( this_instance->CallBack == reference_inst->CallBack)
				{
					if ( Release_reserved_mutex(handle_mutex,"handle_mutex",0,1))
						return DMLERR_SYS_ERROR;
                                        return DMLERR_DLL_USAGE;
				}
                        }
                        reference_inst = reference_inst->Next_Entry;
                }
		//  All cleared, add to chain

		TRACE(ddeml,"Application Instance checks finished\n");
                if ((err_no = IncrementInstanceId())) return err_no;
		if ( Release_reserved_mutex(handle_mutex,"handle_mutex",0,0)) return DMLERR_SYS_ERROR;
		reference_inst->Next_Entry = this_instance;
        }
	pidInst = (LPDWORD)this_instance->Instance_id;
	TRACE(ddeml,"New application instance processing finished OK\n");
     } else {
        /* Reinitialisation situation   --- FIX  */
        TRACE(ddeml,"reinitialisation of (%p,%p,0x%lx,%ld): stub\n",pidInst,pfnCallback,afCmd,ulRes);
	WaitForSingleObject(handle_mutex,1000);
        if ( (err_no=GetLastError()) != 0 )
            {

	     /*  FIXME  - needs refinement with popup for timeout, also is timeout interval OK */

                    ERR(ddeml,"WaitForSingleObject failed - handle list %li\n",err_no);
                    HeapFree(SystemHeap, 0, this_instance);
                    return DMLERR_SYS_ERROR;
        }
        if (DDE_Handle_Table_Base == NULL ) 
	{
		if ( Release_reserved_mutex(handle_mutex,"handle_mutex",0,1)) return DMLERR_SYS_ERROR;
        	return DMLERR_DLL_USAGE;
 	}
        HeapFree(SystemHeap, 0, this_instance); // finished - release heap space used as work store
        // can't reinitialise if we have initialised nothing !!
        reference_inst =  DDE_Handle_Table_Base;
        /* must first check if we have been given a valid instance to re-initialise !!  how do we do that ? */
	while ( reference_inst->Next_Entry != NULL )
	{
		if ( *pidInst == reference_inst->Instance_id && pfnCallback == reference_inst->CallBack )
		{
			// Check 1 - cannot change client-only mode if set via APPCMD_CLIENTONLY

			if (  reference_inst->Client_only )
			{
			   if  ((reference_inst->CBF_Flags & CBF_FAIL_ALLSVRXACTIONS) != CBF_FAIL_ALLSVRXACTIONS) 
			   {
				// i.e. Was set to Client-only and through APPCMD_CLIENTONLY

				if ( ! ( afCmd & APPCMD_CLIENTONLY))
				{
					if ( Release_reserved_mutex(handle_mutex,"handle_mutex",0,1))
						return DMLERR_SYS_ERROR;
                                	return DMLERR_DLL_USAGE;
				}
			   }
			}
			// Check 2 - cannot change monitor modes

                        if ( this_instance->Monitor != reference_inst->Monitor) 
			{
				if ( Release_reserved_mutex(handle_mutex,"handle_mutex",0,1))
					return DMLERR_SYS_ERROR;
                                return DMLERR_DLL_USAGE;
                        }

			// Check 3 - trying to set Client-only via APPCMD when not set so previously

			if (( afCmd&APPCMD_CLIENTONLY) && ! reference_inst->Client_only )
			{
				if ( Release_reserved_mutex(handle_mutex,"handle_mutex",0,1))
					return DMLERR_SYS_ERROR;
                                return DMLERR_DLL_USAGE;
			}
		}
	}
	//		All checked - change relevant flags

	reference_inst->CBF_Flags = this_instance->CBF_Flags;
	reference_inst->Client_only = this_instance->Client_only;
	reference_inst->Monitor_flags = this_instance->Monitor_flags;
	if ( Release_reserved_mutex(handle_mutex,"handle_mutex",0,1))
		return DMLERR_SYS_ERROR;
     }

    return DMLERR_NO_ERROR;
}


/*****************************************************************
 *            DdeUninitialize16   (DDEML.3)
 */
BOOL16 WINAPI DdeUninitialize16( DWORD idInst )
{
    return (BOOL16)DdeUninitialize( idInst );
}


/*****************************************************************
 * DdeUninitialize [USER32.119]  Frees DDEML resources
 *
 * PARAMS
 *    idInst [I] Instance identifier
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI DdeUninitialize( DWORD idInst )
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
    return DdeConnectList(idInst, hszService, hszTopic, hConvList, 
                            (LPCONVCONTEXT)pCC);
}


/******************************************************************************
 * DdeConnectList [USER32.93]  Establishes conversation with DDE servers
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
HCONVLIST WINAPI DdeConnectList( DWORD idInst, HSZ hszService, HSZ hszTopic,
                 HCONVLIST hConvList, LPCONVCONTEXT pCC )
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
    return DdeQueryNextServer(hConvList, hConvPrev);
}


/*****************************************************************
 * DdeQueryNextServer [USER32.112]
 */
HCONV WINAPI DdeQueryNextServer( HCONVLIST hConvList, HCONV hConvPrev )
{
    FIXME(ddeml, "(%ld,%ld): stub\n",hConvList,hConvPrev);
    return 0;
}

/*****************************************************************
 * DdeQueryStringA [USER32.113]
 *
 *****************************************************************
 *
 *	Change History
 *
 *  Vn       Date    	Author         		Comment
 *
 *  1.0      Dec 1998  Corel/Macadamian    Initial version
 *
 */
DWORD WINAPI DdeQueryStringA(DWORD idInst, HSZ hsz, LPSTR psz, DWORD cchMax, INT iCodePage)
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

        ret = GlobalGetAtomNameA( hsz, (LPSTR)psz, cchMax );
    }
    
    return ret;
}

/*****************************************************************
 * DdeQueryStringW [USER32.114]
 *
 *****************************************************************
 *
 *	Change History
 *
 *  Vn       Date    	Author         		Comment
 *
 *  1.0      Dec 1998  Corel/Macadamian    Initial version
 *
 */
DWORD WINAPI DdeQueryStringW(DWORD idInst, HSZ hsz, LPWSTR psz, DWORD cchMax, INT iCodePage)
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
        ret = GlobalGetAtomNameW( hsz, (LPWSTR)psz, cchMax ) * factor;
    }
    return ret;
}

/*****************************************************************
*
*		DdeQueryString16 (DDEML.23)
*
******************************************************************
 *
 *	Change History
 *
 *  Vn       Date    	Author         		Comment
 *
 *  1.0      March 1999 K Matthews		stub only
 */

DWORD WINAPI DdeQueryString16(DWORD idInst, HSZ hsz, LPSTR lpsz, DWORD cchMax, int codepage)
{
	FIXME(ddeml,"(%ld, 0x%lx, %p, %ld, %d): stub \n", 
         idInst,
         hsz,
         lpsz, 
         cchMax,
         codepage);
	return 0;
}


/*****************************************************************
 *            DdeDisconnectList (DDEML.6)
 */
BOOL16 WINAPI DdeDisconnectList16( HCONVLIST hConvList )
{
    return (BOOL16)DdeDisconnectList(hConvList);
}


/******************************************************************************
 * DdeDisconnectList [USER32.98]  Destroys list and terminates conversations
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI DdeDisconnectList(
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
 *            DdeConnect   (USER32.92)
 */
HCONV WINAPI DdeConnect( DWORD idInst, HSZ hszService, HSZ hszTopic,
                           LPCONVCONTEXT pCC )
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
    return (BOOL16)DdeDisconnect( hConv );
}

/*****************************************************************
 *            DdeSetUserHandle16 (DDEML.10)
 */
BOOL16 WINAPI DdeSetUserHandle16( HCONV hConv, DWORD id, DWORD hUser )
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
    return DdeCreateDataHandle(idInst,
                                pSrc,
                                cb,
                                cbOff,
                                hszItem,
                                wFmt,
                                afCmd);
}

/*****************************************************************
 *            DdeCreateDataHandle (USER32.94)
 */
HDDEDATA WINAPI DdeCreateDataHandle( DWORD idInst, LPBYTE pSrc, DWORD cb, 
                                       DWORD cbOff, HSZ hszItem, UINT wFmt, 
                                       UINT afCmd )
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
 *            DdeDisconnect   (USER32.97)
 */
BOOL WINAPI DdeDisconnect( HCONV hConv )
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
    return DdeCreateStringHandleA( idInst, str, codepage );
}


/*****************************************************************
 * DdeCreateStringHandleA [USER32.95]
 *
 * RETURNS
 *    Success: String handle
 *    Failure: 0
 */
HSZ WINAPI DdeCreateStringHandleA( DWORD idInst, LPCSTR psz, INT codepage )
{
  HSZ hsz = 0;
  TRACE(ddeml, "(%ld,%s,%d): stub\n",idInst,debugstr_a(psz),codepage);
  
  if (codepage==CP_WINANSI)
  {
      hsz = GlobalAddAtomA (psz);
      /* Save the handle so we know to clean it when
       * uninitialize is called.
       */
      InsertHSZNode( idInst, hsz );
      return hsz;
  }
    return 0;  
}


/******************************************************************************
 * DdeCreateStringHandleW [USER32.96]  Creates handle to identify string
 *
 * RETURNS
 *    Success: String handle
 *    Failure: 0
 */
HSZ WINAPI DdeCreateStringHandleW(
    DWORD idInst,   /* [in] Instance identifier */
    LPCWSTR psz,    /* [in] Pointer to string */
    INT codepage) /* [in] Code page identifier */
{
  HSZ hsz = 0;

    FIXME(ddeml, "(%ld,%s,%d): stub\n",idInst,debugstr_w(psz),codepage);

  if (codepage==CP_WINUNICODE)
  {
      hsz = GlobalAddAtomW (psz);
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
    return (BOOL)DdeFreeStringHandle( idInst, hsz );
}


/*****************************************************************
 *            DdeFreeStringHandle   (USER32.101)
 * RETURNS: success: nonzero
 *          fail:    zero
 */
BOOL WINAPI DdeFreeStringHandle( DWORD idInst, HSZ hsz )
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
    return (BOOL)DdeFreeDataHandle( hData );
}


/*****************************************************************
 *            DdeFreeDataHandle   (USER32.100)
 */
BOOL WINAPI DdeFreeDataHandle( HDDEDATA hData )
{
    FIXME( ddeml, "empty stub\n" );
    return TRUE;
}




/*****************************************************************
 *            DdeKeepStringHandle16   (DDEML.24)
 */
BOOL16 WINAPI DdeKeepStringHandle16( DWORD idInst, HSZ hsz )
{
    return (BOOL)DdeKeepStringHandle( idInst, hsz );
}


/*****************************************************************
 *            DdeKeepStringHandle  (USER32.108)
 */
BOOL WINAPI DdeKeepStringHandle( DWORD idInst, HSZ hsz )
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
    return DdeClientTransaction( (LPBYTE)pData, cbData, hConv, hszItem,
                                   wFmt, wType, dwTimeout, pdwResult );
}


/*****************************************************************
 *            DdeClientTransaction  (USER32.90)
 */
HDDEDATA WINAPI DdeClientTransaction( LPBYTE pData, DWORD cbData,
                                        HCONV hConv, HSZ hszItem, UINT wFmt,
                                        UINT wType, DWORD dwTimeout,
                                        LPDWORD pdwResult )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}

/*****************************************************************
 *
 *            DdeAbandonTransaction16 (DDEML.12)
 *
 */
BOOL16 WINAPI DdeAbandonTransaction16( DWORD idInst, HCONV hConv, 
                                     DWORD idTransaction )
{
    FIXME( ddeml, "empty stub\n" );
    return TRUE;
}


/*****************************************************************
 *
 *            DdeAbandonTransaction (USER32.87)
 *
******************************************************************
 *
 *	Change History
 *
 *  Vn       Date    	Author         		Comment
 *
 *  1.0      March 1999 K Matthews		stub only
 */
BOOL WINAPI DdeAbandonTransaction( DWORD idInst, HCONV hConv, 
                                     DWORD idTransaction )
{
    FIXME( ddeml, "empty stub\n" );
    return TRUE;
}

/*****************************************************************
 * DdePostAdvise16 [DDEML.13]
 */
BOOL16 WINAPI DdePostAdvise16( DWORD idInst, HSZ hszTopic, HSZ hszItem )
{
    return (BOOL16)DdePostAdvise(idInst, hszTopic, hszItem);
}


/******************************************************************************
 * DdePostAdvise [USER32.110]  Send transaction to DDE callback function.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI DdePostAdvise(
    DWORD idInst, /* [in] Instance identifier */
    HSZ hszTopic, /* [in] Handle to topic name string */
    HSZ hszItem)  /* [in] Handle to item name string */
{
    FIXME(ddeml, "(%ld,%ld,%ld): stub\n",idInst,hszTopic,hszItem);
    return TRUE;
}


/*****************************************************************
 *            DdeAddData16 (DDEML.15)
 */
HDDEDATA WINAPI DdeAddData16( HDDEDATA hData, LPBYTE pSrc, DWORD cb,
                            DWORD cbOff )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}

/*****************************************************************
 *
 *            DdeAddData (USER32.89)
 *
******************************************************************
 *
 *	Change History
 *
 *  Vn       Date    	Author         		Comment
 *
 *  1.0      March 1999 K Matthews		stub only
 */
HDDEDATA WINAPI DdeAddData( HDDEDATA hData, LPBYTE pSrc, DWORD cb,
                            DWORD cbOff )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}


/*****************************************************************
 *
 *            DdeImpersonateClient (USER32.105)
 *
******************************************************************
 *
 *	Change History
 *
 *  Vn       Date    	Author         		Comment
 *
 *  1.0      March 1999 K Matthews		stub only
 */

BOOL WINAPI DdeImpersonateClient( HCONV hConv)
{
    FIXME( ddeml, "empty stub\n" );
    return TRUE;
}


/*****************************************************************
 *
 *            DdeSetQualityOfService (USER32.116)
 *
******************************************************************
 *
 *	Change History
 *
 *  Vn       Date    	Author         		Comment
 *
 *  1.0      March 1999 K Matthews		stub only
 */

BOOL WINAPI DdeSetQualityOfService( HWND hwndClient, CONST SECURITY_QUALITY_OF_SERVICE *pqosNew,
					PSECURITY_QUALITY_OF_SERVICE pqosPrev)
{
    FIXME( ddeml, "empty stub\n" );
    return TRUE;
}

/*****************************************************************
 *
 *            DdeSetUserHandle (USER32.117)
 *
******************************************************************
 *
 *	Change History
 *
 *  Vn       Date    	Author         		Comment
 *
 *  1.0      March 1999 K Matthews		stub only
 */

BOOL WINAPI DdeSetUserHandle( HCONV hConv, DWORD id, DWORD hUser)
{
    FIXME( ddeml, "empty stub\n" );
    return TRUE;
}

/******************************************************************************
 * DdeGetData [USER32.102]  Copies data from DDE object ot local buffer
 *
 * RETURNS
 *    Size of memory object associated with handle
 */
DWORD WINAPI DdeGetData(
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
    return DdeGetData(hData, pDst, cbMax, cbOff);
}


/*****************************************************************
 *            DdeAccessData16 (DDEML.17)
 */
LPBYTE WINAPI DdeAccessData16( HDDEDATA hData, LPDWORD pcbDataSize )
{
     return DdeAccessData(hData, pcbDataSize);
}

/*****************************************************************
 *            DdeAccessData (USER32.88)
 */
LPBYTE WINAPI DdeAccessData( HDDEDATA hData, LPDWORD pcbDataSize )
{
     FIXME( ddeml, "(%ld,%p): stub\n", hData, pcbDataSize);
     return 0;
}

/*****************************************************************
 *            DdeUnaccessData16 (DDEML.18)
 */
BOOL16 WINAPI DdeUnaccessData16( HDDEDATA hData )
{
     return DdeUnaccessData(hData);
}

/*****************************************************************
 *            DdeUnaccessData (USER32.118)
 */
BOOL WINAPI DdeUnaccessData( HDDEDATA hData )
{
     FIXME( ddeml, "(0x%lx): stub\n", hData);

     return 0;
}

/*****************************************************************
 *            DdeEnableCallback16 (DDEML.26)
 */
BOOL16 WINAPI DdeEnableCallback16( DWORD idInst, HCONV hConv, UINT16 wCmd )
{
     return DdeEnableCallback(idInst, hConv, wCmd);
}

/*****************************************************************
 *            DdeEnableCallback (USER32.99)
 */
BOOL WINAPI DdeEnableCallback( DWORD idInst, HCONV hConv, UINT wCmd )
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
    return DdeNameService( idInst, hsz1, hsz2, afCmd );
}


/******************************************************************************
 * DdeNameService [USER32.109]  {Un}registers service name of DDE server
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
HDDEDATA WINAPI DdeNameService( DWORD idInst, HSZ hsz1, HSZ hsz2,
                UINT afCmd )
{
    FIXME(ddeml, "(%ld,%ld,%ld,%d): stub\n",idInst,hsz1,hsz2,afCmd);
    return 1;
}


/*****************************************************************
 *            DdeGetLastError16  (DDEML.20)
 */
UINT16 WINAPI DdeGetLastError16( DWORD idInst )
{
    return (UINT16)DdeGetLastError( idInst );
}


/******************************************************************************
 * DdeGetLastError [USER32.103]  Gets most recent error code
 *
 * PARAMS
 *    idInst [I] Instance identifier
 *
 * RETURNS
 *    Last error code
 */
UINT WINAPI DdeGetLastError( DWORD idInst )
{
    FIXME(ddeml, "(%ld): stub\n",idInst);
    return 0;
}


/*****************************************************************
 *            DdeCmpStringHandles16 (DDEML.36)
 */
int WINAPI DdeCmpStringHandles16( HSZ hsz1, HSZ hsz2 )
{
     return DdeCmpStringHandles(hsz1, hsz2);
}

/*****************************************************************
 *            DdeCmpStringHandles (USER32.91)
 *
 * Compares the value of two string handles.  This comparison is
 * not case sensitive.
 *
 * Returns:
 * -1 The value of hsz1 is zero or less than hsz2
 * 0  The values of hsz 1 and 2 are the same or both zero.
 * 1  The value of hsz2 is zero of less than hsz1
 */
int WINAPI DdeCmpStringHandles( HSZ hsz1, HSZ hsz2 )
{
    CHAR psz1[MAX_BUFFER_LEN];
    CHAR psz2[MAX_BUFFER_LEN];
    int ret = 0;
    int ret1, ret2;

    TRACE( ddeml, "handle 1, handle 2\n" );

    ret1 = GlobalGetAtomNameA( hsz1, psz1, MAX_BUFFER_LEN );
    ret2 = GlobalGetAtomNameA( hsz2, psz2, MAX_BUFFER_LEN );
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
 *            PackDDElParam (USER32.414)
 *
 * RETURNS
 *   success: nonzero
 *   failure: zero
 */
UINT WINAPI PackDDElParam(UINT msg, UINT uiLo, UINT uiHi)
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
UINT WINAPI UnpackDDElParam(UINT msg, UINT lParam,
                              UINT *uiLo, UINT *uiHi)
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
UINT WINAPI FreeDDElParam(UINT msg, UINT lParam)
{
    FIXME(ddeml, "stub.\n");
    return 0;
}

/*****************************************************************
 *            ReuseDDElParam (USER32.446)
 *
 */
UINT WINAPI ReuseDDElParam(UINT lParam, UINT msgIn, UINT msgOut,
                             UINT uiLi, UINT uiHi)
{
    FIXME(ddeml, "stub.\n");
    return 0;
} 

/******************************************************************
 *		DdeQueryConvInfo16 (DDEML.9)
 *
 */
UINT16 WINAPI DdeQueryConvInfo16( HCONV hconv, DWORD idTransaction , LPCONVINFO16 lpConvInfo)
{
	FIXME(ddeml,"stub.\n");
	return 0;
}


/******************************************************************
 *		DdeQueryConvInfo (USER32.111)
 *
 */
UINT WINAPI DdeQueryConvInfo( HCONV hconv, DWORD idTransaction , LPCONVINFO lpConvInfo)
{
	FIXME(ddeml,"stub.\n");
	return 0;
}
