/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * DDEML library
 *
 * Copyright 1997 Alexandre Julliard
 * Copyright 1997 Len White
 * Copyright 1999 Keith Matthews
 * Copyright 2000 Corel
 * Copyright 2001 Eric Pouech
 */

#ifndef __WINE_DDEML_PRIVATE_H
#define __WINE_DDEML_PRIVATE_H

/* defined in atom.c file.
 */

#define MAX_ATOM_LEN              255

/* Maximum buffer size ( including the '\0' ).
 */
#define MAX_BUFFER_LEN            (MAX_ATOM_LEN + 1)

/* This is a simple list to keep track of the strings created
 * by DdeCreateStringHandle.  The list is used to free
 * the strings whenever DdeUninitialize is called.
 * This mechanism is not complete and does not handle multiple instances.
 * Most of the DDE API use a DWORD parameter indicating which instance
 * of a given program is calling them.  The API are supposed to
 * associate the data to the instance that created it.
 */

/* The internal structures (prefixed by WDML) are used as follows:
 * + a WDML_INSTANCE is created for each instance creation (DdeInitialize)
 *      - a popup windows (InstanceClass) is created for each instance. It will be
 *	  used to receive all the DDEML events (server registration, conversation 
 *	  confirmation...)
 * + when registring a server (DdeNameService) a WDML_SERVER is created
 *	- a popup window (ServerNameClass) is created
 * + a conversation is represented by two WDML_CONV structures:
 *	- one on the client side, the other one on the server side
 *	- this is needed because the address spaces may be different
 *	- therefore, two lists of links are kept for each instance
 *	- two windows are created for a conversation:
 *		o a popup window on client side (ClientConvClass)
 *		o a child window (of the ServerName) on the server side
 *		  (ServerConvClass)
 *	- all the exchanges then take place between those two windows
 * + a (warm or link) is represented by two WDML_LINK structures:
 *	- one on client side, the other one on server side
 *	- therefore, two lists of links are kept for each instance
 *	
 * To help getting back to data, WDML windows store information:
 *	- offset 0: the DDE instance
 *	- offset 4: the current conversation (for ClientConv and ServerConv only)
 *
 */

typedef struct tagHSZNode
{
    struct tagHSZNode*	next;
    HSZ 			hsz;
    HSZ 			hsz2;
} HSZNode;

typedef struct tagWDML_SERVER
{
    struct tagWDML_SERVER*	next;
    HSZ				hszService;
    HSZ				hszTopic;
    BOOL			filterOn;
    HWND			hwndServer;
} WDML_SERVER;

typedef struct tagWDML_XACT {
    struct tagWDML_XACT*	next;
    DWORD			xActID;
    UINT			ddeMsg;
    HDDEDATA			hDdeData;
    DWORD			dwTimeout;
    DWORD			hUser;
    union {
	struct {
	    UINT	wType;
	    UINT	wFmt;
	    HSZ		hszItem;
	    HGLOBAL	hDdeAdvise;
	} advise;
	struct {
	    UINT	wFmt;
	    HSZ		hszItem;
	} unadvise;
	struct {
	    HGLOBAL	hMem;
	} execute;
	struct {
	    HGLOBAL	hMem;
	    HSZ		hszItem;
	} poke;
	struct {
	    HSZ		hszItem;
	} request;
    } u;
} WDML_XACT;

typedef struct tagWDML_CONV 
{
    struct tagWDML_CONV*	next;		/* to link all the conversations */
    struct tagWDML_INSTANCE*	thisInstance;
    HSZ				hszService;	/* pmt used for connection */
    HSZ				hszTopic;	/* pmt used for connection */
    UINT			afCmd;		/* service name flag */
    CONVCONTEXT			convContext;
    HWND			hwndClient;	/* source of conversation (ClientConvClass) */
    HWND			hwndServer;	/* destination of conversation (ServerConvClass) */
    WDML_XACT*			transactions;	/* pending transactions (client only) */
    DWORD			hUser;		/* user defined value */
} WDML_CONV;

/* DDE_LINK struct defines hot, warm, and cold links */
typedef struct tagWDML_LINK {
    struct tagWDML_LINK*	next;		/* to link all the active links */
    HCONV			hConv;		/* to get back to the converstaion */
    UINT			transactionType;/* 0 for no link */
    HSZ				hszItem;	/* item targetted for (hot/warm) link */
    UINT			uFmt;		/* format for data */
    HDDEDATA			hDdeData;	/* data them selves */
} WDML_LINK;

typedef struct tagWDML_INSTANCE
{
    struct tagWDML_INSTANCE*	next;
    DWORD           		instanceID;	/* needed to track monitor usage */
    BOOL			monitor;        /* have these two as full Booleans cos they'll be tested frequently */
    BOOL			clientOnly;	/* bit wasteful of space but it will be faster */
    BOOL			unicode;        /* Flag to indicate Win32 API used to initialise */
    BOOL			win16;          /* flag to indicate Win16 API used to initialize */
    HSZNode*			nodeList;	/* for cleaning upon exit */
    PFNCALLBACK     		callback;
    DWORD           		CBFflags;
    DWORD           		monitorFlags;
    UINT			txnCount;      	/* count transactions open to simplify closure */
    DWORD			lastError; 
    HWND			hwndEvent;
    WDML_SERVER*		servers;	/* list of registered servers */
    WDML_CONV*			convs[2];	/* active conversations for this instance (client and server) */
    WDML_LINK*			links[2];	/* active links for this instance (client and server) */
} WDML_INSTANCE;

extern WDML_INSTANCE*	WDML_InstanceList;	/* list of created instances, a process can create many */
extern DWORD 		WDML_MaxInstanceID;	/* FIXME: OK for present, may have to worry about wrap-around later */
extern HANDLE		handle_mutex;

/* header for the DDE Data objects */
typedef struct tagDDE_DATAHANDLE_HEAD
{
    short	cfFormat;
} DDE_DATAHANDLE_HEAD;

typedef enum tagWDML_SIDE
{
    WDML_CLIENT_SIDE = 0, WDML_SERVER_SIDE = 1
} WDML_SIDE;

/* server calls this. */
extern	WDML_SERVER*	WDML_AddServer(WDML_INSTANCE* thisInstance, HSZ hszService, HSZ hszTopic);
extern	void		WDML_RemoveServer(WDML_INSTANCE* thisInstance, HSZ hszService, HSZ hszTopic);
extern	WDML_SERVER*	WDML_FindServer(WDML_INSTANCE* thisInstance, HSZ hszService, HSZ hszTopic);
/* called both in DdeClientTransaction and server side. */		
extern	WDML_CONV* 	WDML_AddConv(WDML_INSTANCE* thisInstance, WDML_SIDE side, 
				     HSZ hszService, HSZ hszTopic, HWND hwndClient, HWND hwndServer);
extern	void		WDML_RemoveConv(WDML_INSTANCE* thisInstance, WDML_SIDE side, HCONV hConv);
extern	WDML_CONV*	WDML_GetConv(HCONV hConv);
extern	WDML_CONV*	WDML_FindConv(WDML_INSTANCE* thisInstance, WDML_SIDE side, 
				      HSZ hszService, HSZ hszTopic);	
extern	void		WDML_AddLink(WDML_INSTANCE* thisInstance, HCONV hConv, WDML_SIDE side, 
				     UINT wType, HSZ hszItem, UINT wFmt);
extern	WDML_LINK*	WDML_FindLink(WDML_INSTANCE* thisInstance, HCONV hConv, WDML_SIDE side, 
				      HSZ hszItem, UINT uFmt);
extern	void 		WDML_RemoveLink(WDML_INSTANCE* thisInstance, HCONV hConv, WDML_SIDE side, 
					HSZ hszItem, UINT wFmt);
extern	void		WDML_RemoveAllLinks(WDML_INSTANCE* thisInstance, HCONV hConv, WDML_SIDE side);
/* client calls these */
extern	WDML_XACT*	WDML_AllocTransaction(WDML_INSTANCE* thisInstance, UINT ddeMsg);
extern	void		WDML_QueueTransaction(WDML_CONV* pConv, WDML_XACT* pXAct);
extern	BOOL		WDML_UnQueueTransaction(WDML_CONV* pConv, WDML_XACT*  pXAct);
extern	void		WDML_FreeTransaction(WDML_XACT* pXAct);
extern	WDML_XACT*	WDML_FindTransaction(WDML_CONV* pConv, DWORD tid);
extern	HGLOBAL		WDML_DataHandle2Global(HDDEDATA hDdeData, BOOL fResponse, BOOL fRelease, 
					       BOOL fDeferUpd, BOOL dAckReq);
extern	HDDEDATA	WDML_Global2DataHandle(HGLOBAL hMem);
extern	WDML_INSTANCE*	WDML_FindInstance(DWORD InstId);
extern	BOOL 		WDML_WaitForMutex(HANDLE mutex);
extern	DWORD		WDML_ReleaseMutex(HANDLE mutex, LPSTR mutex_name, BOOL release_handle_m);
extern	void 		WDML_FreeAllHSZ(WDML_INSTANCE* thisInstance);
extern	void 		WDML_ReleaseAtom(WDML_INSTANCE* thisInstance, HSZ hsz);
extern	void 		WDML_ReserveAtom(WDML_INSTANCE* thisInstance, HSZ hsz);
/* broadcasting to DDE windows */
extern	void		WDML_BroadcastDDEWindows(const char* clsName, UINT uMsg, 
						 WPARAM wParam, LPARAM lParam);

extern const char WDML_szEventClass[]; /* class of window for events (aka instance) */

#define WM_WDML_REGISTER	(WM_USER + 0x200)
#define WM_WDML_UNREGISTER	(WM_USER + 0x201)

#endif  /* __WINE_DDEML_PRIVATE_H */
