/*
 * WSOCK32 specific functions
 *
 * Copyright (C) 1993,1994,1996,1997 John Brezak, Erik Bos, Alex Korobka.
 */

#include "config.h"

#include <sys/types.h>
#include "windef.h"
#include "debugtools.h"
#include "winsock2.h"
#include "winnt.h"
#include "wscontrol.h"
#include <ctype.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef HAVE_SYS_SOCKIO_H
# include <sys/sockio.h>
#endif
#ifdef HAVE_NET_IF_H
# include <net/if.h>
#endif

DEFAULT_DEBUG_CHANNEL(winsock);


/***********************************************************************
 *      WsControl()
 *
 * WsControl seems to be an undocumented Win95 function. A lot of 
 * discussion about WsControl can be found on the net, e.g.
 * Subject:      Re: WSOCK32.DLL WsControl Exported Function
 * From:         "Peter Rindfuss" <rindfuss-s@medea.wz-berlin.de>
 * Date:         1997/08/17
 *
 * WSCNTL_TCPIP_QUERY_INFO option is partially implemeted based
 * on observing the behaviour of WsControl with an app in 
 * Windows 98.  It is not fully implemented, and there could
 * be (are?) errors due to incorrect assumptions made.
 *
 *
 * WsControl returns WSCTL_SUCCESS on success.
 * STATUS_BUFFER_TOO_SMALL is returned if the output buffer length
 * (*pcbResponseInfoLen) is too small, otherwise errors return -1.
 *
 * It doesn't seem to generate errors that can be retrieved by 
 * WSAGetLastError().
 *
 */

DWORD WINAPI WsControl(DWORD protocoll,
                       DWORD action,
                       LPVOID pRequestInfo,
                       LPDWORD pcbRequestInfoLen,
                       LPVOID pResponseInfo,
                       LPDWORD pcbResponseInfoLen) 
{
   /* Get the command structure into a pointer we can use,
      rather than void */
   TDIObjectID *pcommand = (TDIObjectID *)pRequestInfo;
  
   TRACE ("   WsControl TOI_ID=>0x%lx<, {TEI_ENTITY=0x%lx, TEI_INSTANCE=0x%lx}, TOI_CLASS=0x%lx, TOI_TYPE=0x%lx\n",
          pcommand->toi_id, pcommand->toi_entity.tei_entity, pcommand->toi_entity.tei_instance,
          pcommand->toi_class, pcommand->toi_type );
  


   switch (action) 
   {
      case WSCNTL_TCPIP_QUERY_INFO: 
      {
         switch (pcommand->toi_id)
         {
            /* 
               ENTITY_LIST_ID seems to get number of adapters in the system.
               (almost like an index to be used when calling other WsControl options)
            */
            case ENTITY_LIST_ID: 
            {
               TDIEntityID *baseptr = pResponseInfo;
               int numInt = 0, i;

               if (pcommand->toi_class != INFO_CLASS_GENERIC &&
                   pcommand->toi_type != INFO_TYPE_PROVIDER) 
               { 
                  FIXME ("Unexpected Option for ENTITY_LIST_ID request -> toi_class=0x%lx, toi_type=0x%lx\n",
                       pcommand->toi_class, pcommand->toi_type);
                  return (WSAEOPNOTSUPP); 
               }
           
               numInt = WSCNTL_GetInterfaceCount(); 
               if (numInt < 0)
               {
                  ERR ("Unable to open /proc filesystem to determine number of network interfaces!\n");
                  return (-1); 
               }

               if (*pcbResponseInfoLen < sizeof(TDIEntityID)*(numInt*2) ) 
               {
                  return (STATUS_BUFFER_TOO_SMALL);
               }
           
               /* 0 it out first */
               memset(baseptr, 0, sizeof(TDIEntityID)*(numInt*2)); 
               
               for (i=0; i<numInt; i++)
               {
                  /* tei_instance is an network interface identifier.
                     I'm not quite sure what the difference is between tei_entity values of 
                     CL_NL_ENTITY and IF_ENTITY */
                  baseptr->tei_entity = CL_NL_ENTITY;  baseptr->tei_instance = i; baseptr++;
                  baseptr->tei_entity = IF_ENTITY;     baseptr->tei_instance = i; baseptr++; 
               }

               /* Calculate size of out buffer */
               *pcbResponseInfoLen = sizeof(TDIEntityID)*(numInt*2);
            
               break;
            }
          
          
            /* ENTITY_TYPE_ID is used to obtain simple information about a 
               network card, such as MAC Address, description, interface type,
               number of network addresses, etc. */
            case ENTITY_TYPE_ID:  /* ALSO: IP_MIB_STATS_ID */
            {
               if (pcommand->toi_class == INFO_CLASS_GENERIC && pcommand->toi_type == INFO_TYPE_PROVIDER) 
               {
                  if (pcommand->toi_entity.tei_entity == IF_ENTITY)
                  {
                     * ((ULONG *)pResponseInfo) = IF_MIB; 

                     /* Calculate size of out buffer */
                     *pcbResponseInfoLen = sizeof (ULONG);

                  }
                  else if (pcommand->toi_entity.tei_entity == CL_NL_ENTITY) 
                  {
                     * ((ULONG *)pResponseInfo) = CL_NL_IP;  

                     /* Calculate size of out buffer */
                     *pcbResponseInfoLen = sizeof (ULONG); 
                  }
               }
               else if (pcommand->toi_class == INFO_CLASS_PROTOCOL &&
                        pcommand->toi_type == INFO_TYPE_PROVIDER)
               {
                  if (pcommand->toi_entity.tei_entity == IF_ENTITY)
                  {
                     /* In this case, we are requesting specific information about a 
                        a particular network adapter. (MAC Address, speed, data transmitted/received,
                        etc.)
                     */ 
                     IFEntry *IntInfo = (IFEntry *) pResponseInfo;
                     char ifName[512];
                     struct ifreq ifInfo;
                     int sock;

                     
                     if (!WSCNTL_GetInterfaceName(pcommand->toi_entity.tei_instance, ifName))
                     {
                        ERR ("Unable to parse /proc filesystem!\n");
                        return (-1);
                     }
               
                     /* Get a socket so that we can use ioctl */
                     if ( (sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
                     {
                        ERR ("Error creating socket!\n");
                        return (-1);
                     }
                   
                     /* 0 out return structure first */
                     memset (IntInfo, 0, sizeof(IFEntry));
                     
                     /* Interface ID */
                     IntInfo->if_index = pcommand->toi_entity.tei_instance;
                     
                     /* MAC Address - Let's try to do this in a cross-platform way... */
                     #if defined(SIOCGIFHWADDR) /* Linux */
                        strcpy(ifInfo.ifr_name, ifName);
                        if (ioctl(sock, SIOCGIFHWADDR, &ifInfo) < 0)
                        {
                           ERR ("Error obtaining MAC Address!\n");
                           close(sock);
                           return (-1);
                        }
                        else
                        {
                           /* FIXME: Is it correct to assume size of 6? */
                           memcpy(IntInfo->if_physaddr, ifInfo.ifr_hwaddr.sa_data, 6);
                           IntInfo->if_physaddrlen=6;
                        }
                     #elif defined(SIOCGENADDR) /* Solaris */
                        if (ioctl(sock, SIOCGENADDR, &ifInfo) < 0)
                        {
                           ERR ("Error obtaining MAC Address!\n");
                           close(sock);
                           return (-1);
                        }
                        else
                        {
                           /* FIXME: Is it correct to assume size of 6? */
		           memcpy(IntInfo->if_physaddr, ifInfo.ifr_enaddr, 6);
                           IntInfo->if_physaddrlen=6;
                        }
                     #else
                        memset (IntInfo->if_physaddr, 0, 6);
                        ERR ("Unable to determine MAC Address on your platform!\n");
                     #endif

                     
                     /* Interface name and length */
                     strcpy (IntInfo->if_descr, ifName);
                     IntInfo->if_descrlen= strlen (IntInfo->if_descr);
                     
                     /* Obtain bytes transmitted/received for interface */
                     if ( (WSCNTL_GetTransRecvStat(pcommand->toi_entity.tei_instance, 
                           &IntInfo->if_inoctets, &IntInfo->if_outoctets)) < 0)
                     {
                        ERR ("Error obtaining transmit/receive stats for the network interface!\n");
                        close(sock);
                        return (-1);
                     }
                     
                     
                     /* FIXME: How should the below be properly calculated? ******************/
                     IntInfo->if_type =  0x6; /* Ethernet (?) */
	             IntInfo->if_speed = 1000000; /* Speed of interface (bits per second?) */
                     /************************************************************************/

                     close(sock);
                     *pcbResponseInfoLen = sizeof (IFEntry) + IntInfo->if_descrlen; 
                  }
                  else if (pcommand->toi_entity.tei_entity == CL_NL_ENTITY) 
                  {
                     IPSNMPInfo *infoStruc = (IPSNMPInfo *) pResponseInfo;
                     int numInt;
                     
                     /* This case is used to obtain general statistics about the 
                        network */
                     
                     if (*pcbResponseInfoLen < sizeof(IPSNMPInfo) )
                     {
                        return (STATUS_BUFFER_TOO_SMALL);
                     }
                     else
                     {
                        /* 0 it out first */
                        memset(infoStruc, 0, sizeof(IPSNMPInfo));
            
                        /* Get the number of interfaces */
                        numInt = WSCNTL_GetInterfaceCount(); 
                        if (numInt < 0)
                        {
                           ERR ("Unable to open /proc filesystem to determine number of network interfaces!\n");
                           return (-1); 
                        }

                        infoStruc->ipsi_numif           = numInt; /* # of interfaces */
                        infoStruc->ipsi_numaddr         = numInt; /* # of addresses */
                        infoStruc->ipsi_numroutes       = numInt; /* # of routes ~ FIXME - Is this right? */

                        /* FIXME: How should the below be properly calculated? ******************/
                        infoStruc->ipsi_forwarding      = 0x0;
                        infoStruc->ipsi_defaultttl      = 0x0;
                        infoStruc->ipsi_inreceives      = 0x0;
                        infoStruc->ipsi_inhdrerrors     = 0x0;
                        infoStruc->ipsi_inaddrerrors    = 0x0;
                        infoStruc->ipsi_forwdatagrams   = 0x0;
                        infoStruc->ipsi_inunknownprotos = 0x0;
                        infoStruc->ipsi_indiscards      = 0x0;
                        infoStruc->ipsi_indelivers      = 0x0;
                        infoStruc->ipsi_outrequests     = 0x0;
                        infoStruc->ipsi_routingdiscards = 0x0;
                        infoStruc->ipsi_outdiscards     = 0x0;
                        infoStruc->ipsi_outnoroutes     = 0x0;
                        infoStruc->ipsi_reasmtimeout    = 0x0;
                        infoStruc->ipsi_reasmreqds      = 0x0;
                        infoStruc->ipsi_reasmoks        = 0x0;
                        infoStruc->ipsi_reasmfails      = 0x0;
                        infoStruc->ipsi_fragoks         = 0x0;
                        infoStruc->ipsi_fragfails       = 0x0;
                        infoStruc->ipsi_fragcreates     = 0x0;
                        /************************************************************************/
                      
                        /* Calculate size of out buffer */
                        *pcbResponseInfoLen = sizeof(IPSNMPInfo);
                     }
                  }
               }
               else
               {
                  FIXME ("Unexpected Option for ENTITY_TYPE_ID request -> toi_class=0x%lx, toi_type=0x%lx\n",
                       pcommand->toi_class, pcommand->toi_type);
                  
                  return (WSAEOPNOTSUPP); 
               }

               break;
            }


            /* IP_MIB_ADDRTABLE_ENTRY_ID is used to obtain more detailed information about a 
               particular network adapter */
            case IP_MIB_ADDRTABLE_ENTRY_ID: 
            {
               IPAddrEntry *baseIPInfo = (IPAddrEntry *) pResponseInfo;
               char ifName[512];
               struct ifreq ifInfo;
               int sock;

               if (*pcbResponseInfoLen < sizeof(IPAddrEntry))
               {
                  return (STATUS_BUFFER_TOO_SMALL); 
               }
               
               if (!WSCNTL_GetInterfaceName(pcommand->toi_entity.tei_instance, ifName))
               {
                  ERR ("Unable to parse /proc filesystem!\n");
                  return (-1);
               }
               
               
               /* Get a socket so we can use ioctl */
               if ( (sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
               {
                  ERR ("Error creating socket!\n");
                  return (-1);
               }
	       
               /* 0 it out first */
               memset(baseIPInfo, 0, sizeof(IPAddrEntry) );
               
               /* Interface Id */
               baseIPInfo->iae_index     = pcommand->toi_entity.tei_instance;
               
               /* IP Address */
               strcpy (ifInfo.ifr_name, ifName);
	       ifInfo.ifr_addr.sa_family = AF_INET;
	       if (ioctl(sock, SIOCGIFADDR, &ifInfo) < 0) 
               {
                  baseIPInfo->iae_addr = 0x0;
               }
               else
               {
                  struct ws_sockaddr_in *ipTemp = (struct ws_sockaddr_in *)&ifInfo.ifr_addr;
                  baseIPInfo->iae_addr = ipTemp->sin_addr.S_un.S_addr;
               }
               
               /* Broadcast Address */
               strcpy (ifInfo.ifr_name, ifName);
	       if (ioctl(sock, SIOCGIFBRDADDR, &ifInfo) < 0)
               {
                  baseIPInfo->iae_bcastaddr = 0x0;
               }
	       else
               {
                  struct ws_sockaddr_in *ipTemp = (struct ws_sockaddr_in *)&ifInfo.ifr_broadaddr;
                  baseIPInfo->iae_bcastaddr = ipTemp->sin_addr.S_un.S_addr; 
               }

               /* Subnet Mask */
	       strcpy(ifInfo.ifr_name, ifName);
	       if (ioctl(sock, SIOCGIFNETMASK, &ifInfo) < 0)
               {
                  baseIPInfo->iae_mask = 0x0;
	       }
               else
	       {
                  /* Trying to avoid some compile problems across platforms.
                     (Linux, FreeBSD, Solaris...) */
                  #ifndef ifr_netmask
                     #ifndef ifr_addr
                        baseIPInfo->iae_mask = 0;
                        ERR ("Unable to determine Netmask on your platform!\n");
                     #else
                        struct ws_sockaddr_in *ipTemp = (struct ws_sockaddr_in *)&ifInfo.ifr_addr;
                        baseIPInfo->iae_mask = ipTemp->sin_addr.S_un.S_addr; 
                     #endif
                  #else  
                     struct ws_sockaddr_in *ipTemp = (struct ws_sockaddr_in *)&ifInfo.ifr_netmask;
                     baseIPInfo->iae_mask = ipTemp->sin_addr.S_un.S_addr; 
                  #endif
               }

               /* FIXME: How should the below be properly calculated? ******************/
               baseIPInfo->iae_reasmsize = 0x0;
               baseIPInfo->iae_context   = 0x0;
               baseIPInfo->iae_pad       = 0x0;
               /************************************************************************/
             
               /* Calculate size of out buffer */
               *pcbResponseInfoLen = sizeof(IPAddrEntry);
               close(sock);
               break;
            }


            default: 
            {
               FIXME ("Command ID Not Supported -> toi_id=0x%lx, toi_entity={tei_entity=0x%lx, tei_instance=0x%lx}, toi_class=0x%lx, toi_type=0x%lx\n",
                       pcommand->toi_id, pcommand->toi_entity.tei_entity, pcommand->toi_entity.tei_instance, 
                       pcommand->toi_class, pcommand->toi_type);
              
               return (WSAEOPNOTSUPP); 
            }
         }
      
         break;
      }
    
      case WSCNTL_TCPIP_ICMP_ECHO:
      {
         unsigned int addr = *(unsigned int*)pRequestInfo;
         #if 0
            int timeout= *(unsigned int*)(inbuf+4);
            short x1 = *(unsigned short*)(inbuf+8);
            short sendbufsize = *(unsigned short*)(inbuf+10);
            char x2 = *(unsigned char*)(inbuf+12);
            char ttl = *(unsigned char*)(inbuf+13);
            char service = *(unsigned char*)(inbuf+14);
            char type= *(unsigned char*)(inbuf+15); /* 0x2: don't fragment*/
         #endif      
      
         FIXME("(ICMP_ECHO) to 0x%08x stub \n", addr);
         break;
      }

      default:
      {
         FIXME("Protocoll Not Supported -> protocoll=0x%lx, action=0x%lx, Request=%p, RequestLen=%p, Response=%p, ResponseLen=%p\n",
	       protocoll, action, pRequestInfo, pcbRequestInfoLen, pResponseInfo, pcbResponseInfoLen);
      
         return (WSAEOPNOTSUPP); 
      }
   }
   
   
   return (WSCTL_SUCCESS); 
}



/* 
  Helper function for WsControl - Get count of the number of interfaces
  by parsing /proc filesystem.
*/
int WSCNTL_GetInterfaceCount(void)
{
   FILE *procfs;
   char buf[512];  /* Size doesn't matter, something big */
   int  intcnt=0;
 
 
   /* Open /proc filesystem file for network devices */ 
   procfs = fopen(PROCFS_NETDEV_FILE, "r");
   if (!procfs) 
   {
      /* If we can't open the file, return an error */
      return (-1);
   }
   
   /* Omit first two lines, they are only headers */
   fgets(buf, sizeof buf, procfs);	
   fgets(buf, sizeof buf, procfs);

   while (fgets(buf, sizeof buf, procfs)) 
   {
      /* Each line in the file represents a network interface */
      intcnt++;
   }

   fclose(procfs);
   return(intcnt);
}


/*
   Helper function for WsControl - Get name of device from interface number
   by parsing /proc filesystem.
*/
int WSCNTL_GetInterfaceName(int intNumber, char *intName)
{
   FILE *procfs;
   char buf[512]; /* Size doesn't matter, something big */
   int  i;

   /* Open /proc filesystem file for network devices */ 
   procfs = fopen(PROCFS_NETDEV_FILE, "r");
   if (!procfs) 
   {
      /* If we can't open the file, return an error */
      return (-1);
   }
   
   /* Omit first two lines, they are only headers */
   fgets(buf, sizeof(buf), procfs);	
   fgets(buf, sizeof(buf), procfs);

   for (i=0; i<intNumber; i++)
   {
      /* Skip the lines that don't interest us. */
      fgets(buf, sizeof(buf), procfs);
   }
   fgets(buf, sizeof(buf), procfs); /* This is the line we want */

   
   /* Parse out the line, grabbing only the name of the device
      to the intName variable 
      
      The Line comes in like this: (we only care about the device name)
      lo:   21970 377 0 0 0 0 0 0 21970 377 0 0 0 0 0 0
   */
   i=0; 
   while (isspace(buf[i])) /* Skip initial space(s) */
   {
      i++;
   }

   while (buf[i]) 
   {
      if (isspace(buf[i]))
      {
         break;
      }
      
      if (buf[i] == ':')  /* FIXME: Not sure if this block (alias detection) works properly */
      {
         /* This interface could be an alias... */
         int hold = i;
         char *dotname = intName;
         *intName++ = buf[i++];
         
         while (isdigit(buf[i]))
         {
            *intName++ = buf[i++];
         }
         
         if (buf[i] != ':') 
         {
            /* ... It wasn't, so back up */
            i = hold;
            intName = dotname;
         }
 
         if (buf[i] == '\0')
         {
            fclose(procfs);
            return(FALSE);
         }
         
         i++;
         break;
      }
      
      *intName++ = buf[i++];
   }
   *intName++ = '\0';

   fclose(procfs);
   return(TRUE);
}


/*
   Helper function for WsControl - This function returns the bytes (octets) transmitted
   and received for the supplied interface number from the /proc fs. 
*/
int WSCNTL_GetTransRecvStat(int intNumber, unsigned long *transBytes, unsigned long *recvBytes)
{
   FILE *procfs;
   char buf[512], result[512]; /* Size doesn't matter, something big */
   int  i, bufPos, resultPos;

   /* Open /proc filesystem file for network devices */ 
   procfs = fopen(PROCFS_NETDEV_FILE, "r");
   if (!procfs) 
   {
      /* If we can't open the file, return an error */
      return (-1);
   }
   
   /* Omit first two lines, they are only headers */
   fgets(buf, sizeof(buf), procfs);	
   fgets(buf, sizeof(buf), procfs);

   for (i=0; i<intNumber; i++)
   {
      /* Skip the lines that don't interest us. */
      fgets(buf, sizeof(buf), procfs);
   }
   fgets(buf, sizeof(buf), procfs); /* This is the line we want */



   /* Parse out the line, grabbing the number of bytes transmitted
      and received on the interface.
      
      The Line comes in like this: (we care about columns 2 and 10)
      lo:   21970 377 0 0 0 0 0 0 21970 377 0 0 0 0 0 0
   */

   /* Start at character 0 in the buffer */
   bufPos=0;
   
   /* Skip initial space(s) */ 
   while (isspace(buf[bufPos])) 
      bufPos++;


   /* Skip the name and its trailing spaces (if any) */
   while (buf[bufPos]) 
   {
      if (isspace(buf[bufPos]))
         break;
 
      if (buf[bufPos] == ':') /* Could be an alias */
      {
         int hold = bufPos;

         while(isdigit (buf[bufPos]))
            bufPos++;
         if (buf[bufPos] != ':')
            bufPos = hold;
         if (buf[bufPos] == '\0')
         {
            fclose(procfs);
            return(FALSE);
         }
         
         bufPos++;
         break;
      }

      bufPos++;
   }
   while (isspace(buf[bufPos]))
      bufPos++;


   /* This column (#2) is the number of bytes received. */
   resultPos = 0;
   while (!isspace(buf[bufPos]))
   {
      result[resultPos] = buf[bufPos];
      result[resultPos+1]='\0';
      resultPos++; bufPos++;
   }
   *recvBytes = strtoul (result, NULL, 10); /* convert string to unsigned long, using base 10 */

   
   /* Skip columns #3 to #9 (Don't need them) */
   for  (i=0; i<7; i++)
   {
      while (isspace(buf[bufPos]))
         bufPos++;
      while (!isspace(buf[bufPos])) 
         bufPos++;
   }


   /* This column (#10) is the number of bytes transmitted */
   while (isspace(buf[bufPos]))
       bufPos++;

   resultPos = 0;
   while (!isspace(buf[bufPos]))
   {
      result[resultPos] = buf[bufPos];
      result[resultPos+1]='\0';
      resultPos++; bufPos++;
   }
   *transBytes = strtoul (result, NULL, 10); /* convert string to unsigned long, using base 10 */


   fclose(procfs);
   return(TRUE);
}




/***********************************************************************
 *       WS_s_perror         (WSOCK32.1108)
 */
void WINAPI WS_s_perror(LPCSTR message)
{
    FIXME("(%s): stub\n",message);
    return;
}
