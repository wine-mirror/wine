/* wscontrol.h
 *
 * This header file includes #defines, structure and type definitions,
 * and function declarations that support the implementation of the
 * (undocumented) Winsock 1 call WsControl.
 *
 * The functionality of WsControl was created by observing its behaviour
 * in Windows 98, so there are likely to be bugs with the assumptions
 * that were made.
 */

#ifndef WSCONTROL_H_INCLUDED
#define WSCONTROL_H_INCLUDED

typedef unsigned char uchar; /* This doesn't seem to be in any standard headers */

#define WSCTL_SUCCESS        0         
#define PROCFS_NETDEV_FILE   "/proc/net/dev" /* Points to the file in the /proc fs 
                                                that lists the network devices.
                                                Do we need an #ifdef LINUX for this? */

/* WsControl Helper Functions */
int WSCNTL_GetInterfaceCount(void); /* Obtains the number of network interfaces */
int WSCNTL_GetInterfaceName(int, char *); /* Obtains the name of an interface */
int WSCNTL_GetTransRecvStat(int intNumber, ulong *transBytes, ulong *recvBytes); /* Obtains bytes
                                                                         recv'd/trans by interface */

/*
 *      TCP/IP action codes.
 */
#define WSCNTL_TCPIP_QUERY_INFO             0x00000000
#define WSCNTL_TCPIP_SET_INFO               0x00000001
#define WSCNTL_TCPIP_ICMP_ECHO              0x00000002
#define WSCNTL_TCPIP_TEST                   0x00000003


/* Structure of an entity ID */
typedef struct TDIEntityID 
{
   ulong tei_entity;
   ulong tei_instance;
} TDIEntityID;

/* Structure of an object ID */
typedef struct TDIObjectID 
{
   TDIEntityID  toi_entity;
   ulong        toi_class;
   ulong        toi_type;
   ulong        toi_id;
} TDIObjectID;

typedef struct IPSNMPInfo 
{
   ulong  ipsi_forwarding;
   ulong  ipsi_defaultttl;
   ulong  ipsi_inreceives;
   ulong  ipsi_inhdrerrors;
   ulong  ipsi_inaddrerrors;
   ulong  ipsi_forwdatagrams;
   ulong  ipsi_inunknownprotos;
   ulong  ipsi_indiscards;
   ulong  ipsi_indelivers;
   ulong  ipsi_outrequests;
   ulong  ipsi_routingdiscards;
   ulong  ipsi_outdiscards;
   ulong  ipsi_outnoroutes;
   ulong  ipsi_reasmtimeout;
   ulong  ipsi_reasmreqds;
   ulong  ipsi_reasmoks;
   ulong  ipsi_reasmfails;
   ulong  ipsi_fragoks;
   ulong  ipsi_fragfails;
   ulong  ipsi_fragcreates;
   ulong  ipsi_numif;
   ulong  ipsi_numaddr;
   ulong  ipsi_numroutes;
} IPSNMPInfo;

typedef struct IPAddrEntry 
{
   ulong  iae_addr;
   ulong  iae_index;
   ulong  iae_mask;
   ulong  iae_bcastaddr;
   ulong  iae_reasmsize;
   ushort iae_context;
   ushort iae_pad;
} IPAddrEntry;


#define	MAX_PHYSADDR_SIZE    8
#define	MAX_IFDESCR_LEN      256
typedef struct IFEntry 
{
   ulong if_index;
   ulong if_type;
   ulong if_mtu;
   ulong if_speed;
   ulong if_physaddrlen;
   uchar if_physaddr[MAX_PHYSADDR_SIZE];
   ulong if_adminstatus;
   ulong if_operstatus;
   ulong if_lastchange;
   ulong if_inoctets;
   ulong if_inucastpkts;
   ulong if_innucastpkts;
   ulong if_indiscards;
   ulong if_inerrors;
   ulong if_inunknownprotos;
   ulong if_outoctets;
   ulong if_outucastpkts;
   ulong if_outnucastpkts;
   ulong if_outdiscards;
   ulong if_outerrors;
   ulong if_outqlen;
   ulong if_descrlen;
   uchar if_descr[1];
} IFEntry;


/* Not sure what EXACTLY most of this stuff does.
   WsControl was implemented mainly by observing
   its behaviour in Win98 ************************/
#define	INFO_CLASS_GENERIC         0x100
#define	INFO_CLASS_PROTOCOL        0x200
#define	INFO_TYPE_PROVIDER         0x100
#define	ENTITY_LIST_ID             0
#define	CL_NL_ENTITY               0x301
#define	IF_ENTITY                  0x200
#define	ENTITY_TYPE_ID             1
#define	IP_MIB_ADDRTABLE_ENTRY_ID  0x102
/************************************************/

/* Valid values to get back from entity type ID query */
#define	CO_TL_NBF   0x400    /* Entity implements NBF prot. */
#define	CO_TL_SPX   0x402    /* Entity implements SPX prot. */
#define	CO_TL_TCP   0x404    /* Entity implements TCP prot. */
#define	CO_TL_SPP   0x406    /* Entity implements SPP prot. */
#define	CL_TL_NBF   0x401    /* CL NBF protocol */
#define	CL_TL_UDP   0x403    /* Entity implements UDP */
#define	ER_ICMP     0x380    /* The ICMP protocol */
#define	CL_NL_IPX   0x301    /* Entity implements IPX */
#define	CL_NL_IP    0x303    /* Entity implements IP */
#define	AT_ARP      0x280    /* Entity implements ARP */
#define	AT_NULL     0x282    /* Entity does no address */
#define	IF_GENERIC  0x200    /* Generic interface */
#define	IF_MIB      0x202    /* Supports MIB-2 interface */


#endif /* WSCONTROL_H_INCLUDED */
