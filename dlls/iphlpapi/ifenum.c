/* Copyright (C) 2003,2006 Juan Lang
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * Implementation notes
 * FIXME:
 * - I don't support IPv6 addresses here, since SIOCGIFCONF can't return them
 *
 * There are three implemented methods for determining the MAC address of an
 * interface:
 * - a specific IOCTL (Linux)
 * - looking in the ARP cache (at least Solaris)
 * - using the sysctl interface (FreeBSD and Mac OS X)
 * Solaris and some others have SIOCGENADDR, but I haven't gotten that to work
 * on the Solaris boxes at SourceForge's compile farm, whereas SIOCGARP does.
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif

#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif

#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif

#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif

#ifdef HAVE_NET_IF_TYPES_H
#include <net/if_types.h>
#endif

#include "ifenum.h"

#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
#define ifreq_len(ifr) \
 max(sizeof(struct ifreq), sizeof((ifr)->ifr_name)+(ifr)->ifr_addr.sa_len)
#else
#define ifreq_len(ifr) sizeof(struct ifreq)
#endif

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

#ifndef IF_NAMESIZE
#define IF_NAMESIZE 16
#endif

#ifndef INADDR_NONE
#define INADDR_NONE (~0U)
#endif

#define INITIAL_INTERFACES_ASSUMED 4

#define INDEX_IS_LOOPBACK 0x00800000

/* Functions */

static int isLoopbackInterface(int fd, const char *name)
{
  int ret = 0;

  if (name) {
    struct ifreq ifr;

    lstrcpynA(ifr.ifr_name, name, IFNAMSIZ);
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0)
      ret = ifr.ifr_flags & IFF_LOOPBACK;
  }
  return ret;
}

/* The comments say MAX_ADAPTER_NAME is required, but really only IF_NAMESIZE
 * bytes are necessary.
 */
char *getInterfaceNameByIndex(DWORD index, char *name)
{
  return if_indextoname(index, name);
}

DWORD getInterfaceIndexByName(const char *name, PDWORD index)
{
  DWORD ret;
  unsigned int idx;

  if (!name)
    return ERROR_INVALID_PARAMETER;
  if (!index)
    return ERROR_INVALID_PARAMETER;
  idx = if_nametoindex(name);
  if (idx) {
    *index = idx;
    ret = NO_ERROR;
  }
  else
    ret = ERROR_INVALID_DATA;
  return ret;
}

DWORD getNumNonLoopbackInterfaces(void)
{
  DWORD numInterfaces;
  int fd = socket(PF_INET, SOCK_DGRAM, 0);

  if (fd != -1) {
    struct if_nameindex *indexes = if_nameindex();

    if (indexes) {
      struct if_nameindex *p;

      for (p = indexes, numInterfaces = 0; p && p->if_name; p++)
        if (!isLoopbackInterface(fd, p->if_name))
          numInterfaces++;
      if_freenameindex(indexes);
    }
    else
      numInterfaces = 0;
    close(fd);
  }
  else
    numInterfaces = 0;
  return numInterfaces;
}

DWORD getNumInterfaces(void)
{
  DWORD numInterfaces;
  struct if_nameindex *indexes = if_nameindex();

  if (indexes) {
    struct if_nameindex *p;

    for (p = indexes, numInterfaces = 0; p && p->if_name; p++)
      numInterfaces++;
    if_freenameindex(indexes);
  }
  else
    numInterfaces = 0;
  return numInterfaces;
}

InterfaceIndexTable *getInterfaceIndexTable(void)
{
  DWORD numInterfaces;
  InterfaceIndexTable *ret;
  struct if_nameindex *indexes = if_nameindex();

  if (indexes) {
    struct if_nameindex *p;
    DWORD size = sizeof(InterfaceIndexTable);

    for (p = indexes, numInterfaces = 0; p && p->if_name; p++)
      numInterfaces++;
    if (numInterfaces > 1)
      size += (numInterfaces - 1) * sizeof(DWORD);
    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    if (ret) {
      for (p = indexes; p && p->if_name; p++)
        ret->indexes[ret->numIndexes++] = p->if_index;
    }
    if_freenameindex(indexes);
  }
  else
    ret = NULL;
  return ret;
}

InterfaceIndexTable *getNonLoopbackInterfaceIndexTable(void)
{
  DWORD numInterfaces;
  InterfaceIndexTable *ret;
  int fd = socket(PF_INET, SOCK_DGRAM, 0);

  if (fd != -1) {
    struct if_nameindex *indexes = if_nameindex();

    if (indexes) {
      struct if_nameindex *p;
      DWORD size = sizeof(InterfaceIndexTable);

      for (p = indexes, numInterfaces = 0; p && p->if_name; p++)
        if (!isLoopbackInterface(fd, p->if_name))
          numInterfaces++;
      if (numInterfaces > 1)
        size += (numInterfaces - 1) * sizeof(DWORD);
      ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
      if (ret) {
        for (p = indexes; p && p->if_name; p++)
          if (!isLoopbackInterface(fd, p->if_name))
            ret->indexes[ret->numIndexes++] = p->if_index;
      }
      if_freenameindex(indexes);
    }
    else
      ret = NULL;
    close(fd);
  }
  else
    ret = NULL;
  return ret;
}

static DWORD getInterfaceBCastAddrByName(const char *name)
{
  DWORD ret = INADDR_ANY;

  if (name) {
    int fd = socket(PF_INET, SOCK_DGRAM, 0);

    if (fd != -1) {
      struct ifreq ifr;

      lstrcpynA(ifr.ifr_name, name, IFNAMSIZ);
      if (ioctl(fd, SIOCGIFBRDADDR, &ifr) == 0)
        memcpy(&ret, ifr.ifr_addr.sa_data + 2, sizeof(DWORD));
      close(fd);
    }
  }
  return ret;
}

static DWORD getInterfaceMaskByName(const char *name)
{
  DWORD ret = INADDR_NONE;

  if (name) {
    int fd = socket(PF_INET, SOCK_DGRAM, 0);

    if (fd != -1) {
      struct ifreq ifr;

      lstrcpynA(ifr.ifr_name, name, IFNAMSIZ);
      if (ioctl(fd, SIOCGIFNETMASK, &ifr) == 0)
        memcpy(&ret, ifr.ifr_addr.sa_data + 2, sizeof(DWORD));
      close(fd);
    }
  }
  return ret;
}

#if defined (SIOCGIFHWADDR)
DWORD getInterfacePhysicalByName(const char *name, PDWORD len, PBYTE addr,
 PDWORD type)
{
  DWORD ret;
  int fd;

  if (!name || !len || !addr || !type)
    return ERROR_INVALID_PARAMETER;

  fd = socket(PF_INET, SOCK_DGRAM, 0);
  if (fd != -1) {
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(struct ifreq));
    lstrcpynA(ifr.ifr_name, name, IFNAMSIZ);
    if ((ioctl(fd, SIOCGIFHWADDR, &ifr)))
      ret = ERROR_INVALID_DATA;
    else {
      unsigned int addrLen;

      switch (ifr.ifr_hwaddr.sa_family)
      {
#ifdef ARPHRD_LOOPBACK
        case ARPHRD_LOOPBACK:
          addrLen = 0;
          *type = MIB_IF_TYPE_LOOPBACK;
          break;
#endif
#ifdef ARPHRD_ETHER
        case ARPHRD_ETHER:
          addrLen = ETH_ALEN;
          *type = MIB_IF_TYPE_ETHERNET;
          break;
#endif
#ifdef ARPHRD_FDDI
        case ARPHRD_FDDI:
          addrLen = ETH_ALEN;
          *type = MIB_IF_TYPE_FDDI;
          break;
#endif
#ifdef ARPHRD_IEEE802
        case ARPHRD_IEEE802: /* 802.2 Ethernet && Token Ring, guess TR? */
          addrLen = ETH_ALEN;
          *type = MIB_IF_TYPE_TOKENRING;
          break;
#endif
#ifdef ARPHRD_IEEE802_TR
        case ARPHRD_IEEE802_TR: /* also Token Ring? */
          addrLen = ETH_ALEN;
          *type = MIB_IF_TYPE_TOKENRING;
          break;
#endif
#ifdef ARPHRD_SLIP
        case ARPHRD_SLIP:
          addrLen = 0;
          *type = MIB_IF_TYPE_SLIP;
          break;
#endif
#ifdef ARPHRD_PPP
        case ARPHRD_PPP:
          addrLen = 0;
          *type = MIB_IF_TYPE_PPP;
          break;
#endif
        default:
          addrLen = min(MAX_INTERFACE_PHYSADDR, sizeof(ifr.ifr_hwaddr.sa_data));
          *type = MIB_IF_TYPE_OTHER;
      }
      if (addrLen > *len) {
        ret = ERROR_INSUFFICIENT_BUFFER;
        *len = addrLen;
      }
      else {
        if (addrLen > 0)
          memcpy(addr, ifr.ifr_hwaddr.sa_data, addrLen);
        /* zero out remaining bytes for broken implementations */
        memset(addr + addrLen, 0, *len - addrLen);
        *len = addrLen;
        ret = NO_ERROR;
      }
    }
    close(fd);
  }
  else
    ret = ERROR_NO_MORE_FILES;
  return ret;
}
#elif defined (SIOCGARP)
DWORD getInterfacePhysicalByName(const char *name, PDWORD len, PBYTE addr,
 PDWORD type)
{
  DWORD ret;
  int fd;

  if (!name || !len || !addr || !type)
    return ERROR_INVALID_PARAMETER;

  fd = socket(PF_INET, SOCK_DGRAM, 0);
  if (fd != -1) {
    if (isLoopbackInterface(fd, name)) {
      *type = MIB_IF_TYPE_LOOPBACK;
      memset(addr, 0, *len);
      *len = 0;
      ret=NOERROR;
    }
    else {
      struct arpreq arp;
      struct sockaddr_in *saddr;
      struct ifreq ifr;

      /* get IP addr */
      lstrcpynA(ifr.ifr_name, name, IFNAMSIZ);
      ioctl(fd, SIOCGIFADDR, &ifr);
      memset(&arp, 0, sizeof(struct arpreq));
      arp.arp_pa.sa_family = AF_INET;
      saddr = (struct sockaddr_in *)&arp; /* proto addr is first member */
      saddr->sin_family = AF_INET;
      memcpy(&saddr->sin_addr.s_addr, ifr.ifr_addr.sa_data + 2, sizeof(DWORD));
      if ((ioctl(fd, SIOCGARP, &arp)))
        ret = ERROR_INVALID_DATA;
      else {
        /* FIXME:  heh:  who said it was ethernet? */
        int addrLen = ETH_ALEN;

        if (addrLen > *len) {
          ret = ERROR_INSUFFICIENT_BUFFER;
          *len = addrLen;
        }
        else {
          if (addrLen > 0)
            memcpy(addr, &arp.arp_ha.sa_data[0], addrLen);
          /* zero out remaining bytes for broken implementations */
          memset(addr + addrLen, 0, *len - addrLen);
          *len = addrLen;
          *type = MIB_IF_TYPE_ETHERNET;
          ret = NO_ERROR;
        }
      }
    }
    close(fd);
  }
    else
      ret = ERROR_NO_MORE_FILES;

  return ret;
}
#elif defined (HAVE_SYS_SYSCTL_H) && defined (HAVE_NET_IF_DL_H)
DWORD getInterfacePhysicalByName(const char *name, PDWORD len, PBYTE addr,
 PDWORD type)
{
  DWORD ret;
  struct if_msghdr *ifm;
  struct sockaddr_dl *sdl;
  u_char *p, *buf;
  size_t mibLen;
  int mib[] = { CTL_NET, AF_ROUTE, 0, AF_LINK, NET_RT_IFLIST, 0 };
  int addrLen;
  BOOL found = FALSE;

  if (!name || !len || !addr || !type)
    return ERROR_INVALID_PARAMETER;

  if (sysctl(mib, 6, NULL, &mibLen, NULL, 0) < 0)
    return ERROR_NO_MORE_FILES;

  buf = HeapAlloc(GetProcessHeap(), 0, mibLen);
  if (!buf)
    return ERROR_NOT_ENOUGH_MEMORY;

  if (sysctl(mib, 6, buf, &mibLen, NULL, 0) < 0) {
    HeapFree(GetProcessHeap(), 0, buf);
    return ERROR_NO_MORE_FILES;
  }

  ret = ERROR_INVALID_DATA;
  for (p = buf; !found && p < buf + mibLen; p += ifm->ifm_msglen) {
    ifm = (struct if_msghdr *)p;
    sdl = (struct sockaddr_dl *)(ifm + 1);

    if (ifm->ifm_type != RTM_IFINFO || (ifm->ifm_addrs & RTA_IFP) == 0)
      continue;

    if (sdl->sdl_family != AF_LINK || sdl->sdl_nlen == 0 ||
     memcmp(sdl->sdl_data, name, max(sdl->sdl_nlen, strlen(name))) != 0)
      continue;

    found = TRUE;
    addrLen = min(MAX_INTERFACE_PHYSADDR, sdl->sdl_alen);
    if (addrLen > *len) {
      ret = ERROR_INSUFFICIENT_BUFFER;
      *len = addrLen;
    }
    else {
      if (addrLen > 0)
        memcpy(addr, LLADDR(sdl), addrLen);
      /* zero out remaining bytes for broken implementations */
      memset(addr + addrLen, 0, *len - addrLen);
      *len = addrLen;
#if defined(HAVE_NET_IF_TYPES_H)
      switch (sdl->sdl_type)
      {
        case IFT_ETHER:
          *type = MIB_IF_TYPE_ETHERNET;
          break;
        case IFT_FDDI:
          *type = MIB_IF_TYPE_FDDI;
          break;
        case IFT_ISO88024: /* Token Bus */
          *type = MIB_IF_TYPE_TOKENRING;
          break;
        case IFT_ISO88025: /* Token Ring */
          *type = MIB_IF_TYPE_TOKENRING;
          break;
        case IFT_PPP:
          *type = MIB_IF_TYPE_PPP;
          break;
        case IFT_SLIP:
          *type = MIB_IF_TYPE_SLIP;
          break;
        case IFT_LOOP:
          *type = MIB_IF_TYPE_LOOPBACK;
          break;
        default:
          *type = MIB_IF_TYPE_OTHER;
      }
#else
      /* default if we don't know */
      *type = MIB_IF_TYPE_ETHERNET;
#endif
      ret = NO_ERROR;
    }
  }
  HeapFree(GetProcessHeap(), 0, buf);
  return ret;
}
#endif

DWORD getInterfacePhysicalByIndex(DWORD index, PDWORD len, PBYTE addr,
 PDWORD type)
{
  char nameBuf[IF_NAMESIZE];
  char *name = getInterfaceNameByIndex(index, nameBuf);

  if (name)
    return getInterfacePhysicalByName(name, len, addr, type);
  else
    return ERROR_INVALID_DATA;
}

static DWORD getInterfaceMtuByName(const char *name, PDWORD mtu)
{
  DWORD ret;
  int fd;

  if (!name)
    return ERROR_INVALID_PARAMETER;
  if (!mtu)
    return ERROR_INVALID_PARAMETER;

  fd = socket(PF_INET, SOCK_DGRAM, 0);
  if (fd != -1) {
    struct ifreq ifr;

    lstrcpynA(ifr.ifr_name, name, IFNAMSIZ);
    if ((ioctl(fd, SIOCGIFMTU, &ifr)))
      ret = ERROR_INVALID_DATA;
    else {
#ifndef __sun
      *mtu = ifr.ifr_mtu;
#else
      *mtu = ifr.ifr_metric;
#endif
      ret = NO_ERROR;
    }
    close(fd);
  }
  else
    ret = ERROR_NO_MORE_FILES;
  return ret;
}

static DWORD getInterfaceStatusByName(const char *name, PDWORD status)
{
  DWORD ret;
  int fd;

  if (!name)
    return ERROR_INVALID_PARAMETER;
  if (!status)
    return ERROR_INVALID_PARAMETER;

  fd = socket(PF_INET, SOCK_DGRAM, 0);
  if (fd != -1) {
    struct ifreq ifr;

    lstrcpynA(ifr.ifr_name, name, IFNAMSIZ);
    if ((ioctl(fd, SIOCGIFFLAGS, &ifr)))
      ret = ERROR_INVALID_DATA;
    else {
      if (ifr.ifr_flags & IFF_UP)
        *status = MIB_IF_OPER_STATUS_OPERATIONAL;
      else
        *status = MIB_IF_OPER_STATUS_NON_OPERATIONAL;
      ret = NO_ERROR;
    }
    close(fd);
  }
  else
    ret = ERROR_NO_MORE_FILES;
  return ret;
}

DWORD getInterfaceEntryByName(const char *name, PMIB_IFROW entry)
{
  BYTE addr[MAX_INTERFACE_PHYSADDR];
  DWORD ret, len = sizeof(addr), type;

  if (!name)
    return ERROR_INVALID_PARAMETER;
  if (!entry)
    return ERROR_INVALID_PARAMETER;

  if (getInterfacePhysicalByName(name, &len, addr, &type) == NO_ERROR) {
    WCHAR *assigner;
    const char *walker;

    memset(entry, 0, sizeof(MIB_IFROW));
    for (assigner = entry->wszName, walker = name; *walker; 
     walker++, assigner++)
      *assigner = *walker;
    *assigner = 0;
    getInterfaceIndexByName(name, &entry->dwIndex);
    entry->dwPhysAddrLen = len;
    memcpy(entry->bPhysAddr, addr, len);
    memset(entry->bPhysAddr + len, 0, sizeof(entry->bPhysAddr) - len);
    entry->dwType = type;
    /* FIXME: how to calculate real speed? */
    getInterfaceMtuByName(name, &entry->dwMtu);
    /* lie, there's no "administratively down" here */
    entry->dwAdminStatus = MIB_IF_ADMIN_STATUS_UP;
    getInterfaceStatusByName(name, &entry->dwOperStatus);
    /* punt on dwLastChange? */
    entry->dwDescrLen = min(strlen(name), MAX_INTERFACE_DESCRIPTION - 1);
    memcpy(entry->bDescr, name, entry->dwDescrLen);
    entry->bDescr[entry->dwDescrLen] = '\0';
    entry->dwDescrLen++;
    ret = NO_ERROR;
  }
  else
    ret = ERROR_INVALID_DATA;
  return ret;
}

DWORD getInterfaceEntryByIndex(DWORD index, PMIB_IFROW entry)
{
  char nameBuf[IF_NAMESIZE];
  char *name = getInterfaceNameByIndex(index, nameBuf);

  if (name)
    return getInterfaceEntryByName(name, entry);
  else
    return ERROR_INVALID_DATA;
}

/* Enumerates the IP addresses in the system using SIOCGIFCONF, returning
 * the count to you in *pcAddresses.  It also returns to you the struct ifconf
 * used by the call to ioctl, so that you may process the addresses further.
 * Free ifc->ifc_buf using HeapFree.
 * Returns NO_ERROR on success, something else on failure.
 */
static DWORD enumIPAddresses(PDWORD pcAddresses, struct ifconf *ifc)
{
  DWORD ret;
  int fd;

  fd = socket(PF_INET, SOCK_DGRAM, 0);
  if (fd != -1) {
    int ioctlRet = 0;
    DWORD guessedNumAddresses = 0, numAddresses = 0;
    caddr_t ifPtr;
    int lastlen;

    ret = NO_ERROR;
    ifc->ifc_len = 0;
    ifc->ifc_buf = NULL;
    /* there is no way to know the interface count beforehand,
       so we need to loop again and again upping our max each time
       until returned is constant across 2 calls */
    do {
      lastlen = ifc->ifc_len;
      HeapFree(GetProcessHeap(), 0, ifc->ifc_buf);
      if (guessedNumAddresses == 0)
        guessedNumAddresses = INITIAL_INTERFACES_ASSUMED;
      else
        guessedNumAddresses *= 2;
      ifc->ifc_len = sizeof(struct ifreq) * guessedNumAddresses;
      ifc->ifc_buf = HeapAlloc(GetProcessHeap(), 0, ifc->ifc_len);
      ioctlRet = ioctl(fd, SIOCGIFCONF, ifc);
    } while ((ioctlRet == 0) && (ifc->ifc_len != lastlen));

    if (ioctlRet == 0) {
      ifPtr = ifc->ifc_buf;
      while (ifPtr && ifPtr < ifc->ifc_buf + ifc->ifc_len) {
        struct ifreq *ifr = (struct ifreq *)ifPtr;

        if (ifr->ifr_addr.sa_family == AF_INET)
          numAddresses++;

        ifPtr += ifreq_len((struct ifreq *)ifPtr);
      }
    }
    else
      ret = ERROR_INVALID_PARAMETER; /* FIXME: map from errno to Win32 */
    if (!ret)
      *pcAddresses = numAddresses;
    else
    {
      HeapFree(GetProcessHeap(), 0, ifc->ifc_buf);
      ifc->ifc_buf = NULL;
    }
    close(fd);
  }
  else
    ret = ERROR_NO_SYSTEM_RESOURCES;
  return ret;
}

DWORD getNumIPAddresses(void)
{
  DWORD numAddresses = 0;
  struct ifconf ifc;

  if (!enumIPAddresses(&numAddresses, &ifc))
    HeapFree(GetProcessHeap(), 0, ifc.ifc_buf);
  return numAddresses;
}

DWORD getIPAddrTable(PMIB_IPADDRTABLE *ppIpAddrTable, HANDLE heap, DWORD flags)
{
  DWORD ret;

  if (!ppIpAddrTable)
    ret = ERROR_INVALID_PARAMETER;
  else
  {
    DWORD numAddresses = 0;
    struct ifconf ifc;

    ret = enumIPAddresses(&numAddresses, &ifc);
    if (!ret)
    {
      DWORD size = sizeof(MIB_IPADDRTABLE);

      if (numAddresses > 1)
        size += (numAddresses - 1) * sizeof(MIB_IPADDRROW);
      *ppIpAddrTable = HeapAlloc(heap, flags, size);
      if (*ppIpAddrTable) {
        DWORD i = 0, bcast;
        caddr_t ifPtr;

        ret = NO_ERROR;
        (*ppIpAddrTable)->dwNumEntries = numAddresses;
        ifPtr = ifc.ifc_buf;
        while (!ret && ifPtr && ifPtr < ifc.ifc_buf + ifc.ifc_len) {
          struct ifreq *ifr = (struct ifreq *)ifPtr;

          ifPtr += ifreq_len(ifr);

          if (ifr->ifr_addr.sa_family != AF_INET)
             continue;

          ret = getInterfaceIndexByName(ifr->ifr_name,
           &(*ppIpAddrTable)->table[i].dwIndex);
          memcpy(&(*ppIpAddrTable)->table[i].dwAddr, ifr->ifr_addr.sa_data + 2,
           sizeof(DWORD));
          (*ppIpAddrTable)->table[i].dwMask =
           getInterfaceMaskByName(ifr->ifr_name);
          /* the dwBCastAddr member isn't the broadcast address, it indicates
           * whether the interface uses the 1's broadcast address (1) or the
           * 0's broadcast address (0).
           */
          bcast = getInterfaceBCastAddrByName(ifr->ifr_name);
          (*ppIpAddrTable)->table[i].dwBCastAddr =
           (bcast & (*ppIpAddrTable)->table[i].dwMask) ? 1 : 0;
          /* FIXME: hardcoded reasm size, not sure where to get it */
          (*ppIpAddrTable)->table[i].dwReasmSize = 65535;

          (*ppIpAddrTable)->table[i].unused1 = 0;
          (*ppIpAddrTable)->table[i].wType = 0;
          i++;
        }
      }
      else
        ret = ERROR_OUTOFMEMORY;
      HeapFree(GetProcessHeap(), 0, ifc.ifc_buf);
    }
  }
  return ret;
}

char *toIPAddressString(unsigned int addr, char string[16])
{
  if (string) {
    struct in_addr iAddr;

    iAddr.s_addr = addr;
    /* extra-anal, just to make auditors happy */
    lstrcpynA(string, inet_ntoa(iAddr), 16);
  }
  return string;
}
