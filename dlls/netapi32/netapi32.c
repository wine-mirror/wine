/*
 * Copyright 2001 Mike McCormack
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
 */

#include "config.h"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"
#include "winerror.h"
#include "nb30.h"

#ifdef HAVE_SYS_FILE_H
# include <sys/file.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
# include <sys/sockio.h>
#endif
#ifdef HAVE_NET_IF_H
# include <net/if.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef HAVE_SOCKADDR_SA_LEN
#  ifndef max
#   define max(a,b) ((a) > (b) ? (a) : (b))
#  endif
#  define ifreq_size(i) max(sizeof(struct ifreq),\
sizeof((i).ifr_name)+(i).ifr_addr.sa_len)
# else
#  define ifreq_size(i) sizeof(struct ifreq)
# endif /* defined(HAVE_SOCKADDR_SA_LEN) */

WINE_DEFAULT_DEBUG_CHANNEL(netbios);

HMODULE NETAPI32_hModule = 0;

struct NetBiosAdapter
{
    int valid;
    unsigned char address[6];
};

static struct NetBiosAdapter NETBIOS_Adapter[MAX_LANA];

# ifdef SIOCGIFHWADDR
int get_hw_address(int sd, struct ifreq *ifr, unsigned char *address)
{
    if (ioctl(sd, SIOCGIFHWADDR, ifr) < 0)
        return -1;
    memcpy(address, (unsigned char *)&ifr->ifr_hwaddr.sa_data, 6);
    return 0;
}
# else
#  ifdef SIOCGENADDR
int get_hw_address(int sd, struct ifreq *ifr, unsigned char *address)
{
    if (ioctl(sd, SIOCGENADDR, ifr) < 0)
        return -1;
    memcpy(address, (unsigned char *) ifr->ifr_enaddr, 6);
    return 0;
}
#   else
int get_hw_address(int sd, struct ifreq *ifr, unsigned char *address)
{
    return -1;
}
#  endif /* SIOCGENADDR */
# endif /* SIOCGIFHWADDR */

static UCHAR NETBIOS_Enum(PNCB ncb)
{
#ifdef HAVE_NET_IF_H
    int             sd;
    struct ifreq    ifr, *ifrp;
    struct ifconf   ifc;
    unsigned char   buf[1024];
    int             i, ofs;
#endif
    LANA_ENUM *lanas = (PLANA_ENUM) ncb->ncb_buffer;

    TRACE("NCBENUM\n");

    lanas->length = 0;

#ifdef HAVE_NET_IF_H
    /* BSD 4.4 defines the size of an ifreq to be
     * max(sizeof(ifreq), sizeof(ifreq.ifr_name)+ifreq.ifr_addr.sa_len
     * However, under earlier systems, sa_len isn't present, so
     *  the size is just sizeof(struct ifreq)
     */

    sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sd < 0)
        return NRC_OPENERROR;

    memset(buf, 0, sizeof(buf));
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    /* get the ifconf interface */
    if (ioctl (sd, SIOCGIFCONF, (char *)&ifc) < 0)
    {
        close(sd);
        return NRC_OPENERROR;
    }

    /* loop through the interfaces, looking for a valid one */
    /* n = ifc.ifc_len; */
    ofs = 0;
    for (i = 0; i < ifc.ifc_len; i++)
    {
        unsigned char *a = NETBIOS_Adapter[i].address;

        ifrp = (struct ifreq *)((char *)ifc.ifc_buf+ofs);
        strncpy(ifr.ifr_name, ifrp->ifr_name, IFNAMSIZ);

        /* try to get the address for this interface */
        if(get_hw_address(sd, &ifr, a)==0)
        {
            /* make sure it's not blank */
            /* if (!a[0] && !a[1] && !a[2] && !a[3] && !a[4] && !a[5])
	        continue; */

            TRACE("Found valid adapter %d at %02x:%02x:%02x:%02x:%02x:%02x\n", i,
                        a[0],a[1],a[2],a[3],a[4],a[5]);

            NETBIOS_Adapter[i].valid = TRUE;
            lanas->lana[lanas->length] = i;
            lanas->length++;
        }
        ofs += ifreq_size(ifr);
    }
    close(sd);
#endif /* HAVE_NET_IF_H */
    return NRC_GOODRET;
}


static UCHAR NETBIOS_Astat(PNCB ncb)
{
    struct NetBiosAdapter *nad = &NETBIOS_Adapter[ncb->ncb_lana_num];
    PADAPTER_STATUS astat = (PADAPTER_STATUS) ncb->ncb_buffer;

    TRACE("NCBASTAT (Adapter %d)\n", ncb->ncb_lana_num);

    if(!nad->valid)
        return NRC_INVADDRESS;

    memset(astat, 0, sizeof astat);
    memcpy(astat->adapter_address, nad->address, sizeof astat->adapter_address);

    return NRC_GOODRET;
}

BOOL WINAPI
NETAPI32_LibMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%lx,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
            NETAPI32_hModule = hinstDLL;
	    break;
	case DLL_PROCESS_DETACH:
	    break;
    }

    return TRUE;
}

BOOL WINAPI Netbios(PNCB pncb)
{
    UCHAR ret = NRC_ILLCMD;

    TRACE("ncb = %p\n",pncb);

    if(!pncb)
        return NRC_INVADDRESS;

    switch(pncb->ncb_command&0x7f)
    {
    case NCBRESET:
        FIXME("NCBRESET adapter %d\n",pncb->ncb_lana_num);
        if( (pncb->ncb_lana_num < MAX_LANA ) &&
             NETBIOS_Adapter[pncb->ncb_lana_num].valid)
            ret = NRC_GOODRET;
        else
            ret = NRC_ILLCMD; /* NetBIOS emulator not found */
        break;

    case NCBADDNAME:
        FIXME("NCBADDNAME\n");
        break;

    case NCBADDGRNAME:
        FIXME("NCBADDGRNAME\n");
        break;

    case NCBDELNAME:
        FIXME("NCBDELNAME\n");
        break;

    case NCBSEND:
        FIXME("NCBSEND\n");
        break;

    case NCBRECV:
        FIXME("NCBRECV\n");
        break;

    case NCBHANGUP:
        FIXME("NCBHANGUP\n");
        break;

    case NCBCANCEL:
        FIXME("NCBCANCEL\n");
        break;

    case NCBLISTEN:
        FIXME("NCBLISTEN\n");
        break;

    case NCBASTAT:
        ret = NETBIOS_Astat(pncb);
        break;

    case NCBENUM:
        ret = NETBIOS_Enum(pncb);
        break;

    default:
        FIXME("(%p): command code %02x\n", pncb, pncb->ncb_command);

        ret = NRC_ILLCMD; /* NetBIOS emulator not found */
    }
    pncb->ncb_retcode = ret;
    return ret;
}

