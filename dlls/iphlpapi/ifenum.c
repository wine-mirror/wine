/* Copyright (C) 2003 Juan Lang
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
 * Implementation notes
 * Interface index fun:
 * - Windows may rely on an index being cleared in the topmost 8 bits in some
 *   APIs; see GetFriendlyIfIndex and the mention of "backward compatible"
 *   indexes.  It isn't clear which APIs might fail with non-backward-compatible
 *   indexes, but I'll keep them bits clear just in case.
 * - Even though if_nametoindex and if_indextoname seem to be pretty portable,
 *   Linux, at any rate, uses the same interface index for all virtual
 *   interfaces of a real interface as well as for the real interface itself.
 *   If I used the Linux index as my index, this would break my statement that
 *   an index is a key, and that an interface has 0 or 1 IP addresses.
 *   If that behavior were consistent across UNIXen (I don't know), it could
 *   help me implement multiple IP addresses more in the Windows way.
 * I used to assert I could not use UNIX interface indexes as my iphlpapi
 * indexes due to restrictions in netapi32 and wsock32, but I have removed
 * those restrictions, so using if_nametoindex and if_indextoname rather
 * than my current mess would probably be better.
 * FIXME:
 * - I don't support IPv6 addresses here, since SIOCGIFCONF can't return them
 * - the memory interface uses malloc/free; it should be using HeapAlloc instead
 *
 * There are three implemened methods for determining the MAC address of an
 * interface:
 * - a specific IOCTL (Linux)
 * - looking in the ARP cache (at least Solaris)
 * - using the sysctl interface (FreeBSD and MacOSX)
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

#ifndef INADDR_NONE
#define INADDR_NONE (~0U)
#endif

#define INITIAL_INTERFACES_ASSUMED 4

#define INDEX_IS_LOOPBACK 0x00800000

/* Type declarations */

typedef struct _InterfaceNameMapEntry {
  char  name[IFNAMSIZ];
  BOOL  inUse;
  BOOL  usedLastPass;
} InterfaceNameMapEntry;

typedef struct _InterfaceNameMap {
  DWORD numInterfaces;
  DWORD nextAvailable;
  DWORD numAllocated;
  InterfaceNameMapEntry table[1];
} InterfaceNameMap;

/* Global variables */

static CRITICAL_SECTION mapCS;
static InterfaceNameMap *gNonLoopbackInterfaceMap = NULL;
static InterfaceNameMap *gLoopbackInterfaceMap = NULL;

/* Functions */

void interfaceMapInit(void)
{
    InitializeCriticalSection(&mapCS);
}

void interfaceMapFree(void)
{
    DeleteCriticalSection(&mapCS);
    if (gNonLoopbackInterfaceMap)
        free(gNonLoopbackInterfaceMap);
    if (gLoopbackInterfaceMap)
        free(gLoopbackInterfaceMap);
}

/* Sizes the passed-in map to have enough space for numInterfaces interfaces.
 * If map is NULL, allocates a new map.  If it is not, may reallocate the
 * existing map and return a map of increased size.  Returns the allocated map,
 * or NULL if it could not allocate a map of the requested size.
 */
static InterfaceNameMap *sizeMap(InterfaceNameMap *map, DWORD numInterfaces)
{
  if (!map) {
    numInterfaces = max(numInterfaces, INITIAL_INTERFACES_ASSUMED);
    map = (InterfaceNameMap *)calloc(1, sizeof(InterfaceNameMap) +
     (numInterfaces - 1) * sizeof(InterfaceNameMapEntry));
    if (map)
      map->numAllocated = numInterfaces;
  }
  else {
    if (map->numAllocated < numInterfaces) {
      map = (InterfaceNameMap *)realloc(map, sizeof(InterfaceNameMap) +
       (numInterfaces - 1) * sizeof(InterfaceNameMapEntry));
      if (map)
        memset(&map->table[map->numAllocated], 0,
         (numInterfaces - map->numAllocated) * sizeof(InterfaceNameMapEntry));
    }
  }
  return map;
}

static int isLoopbackInterface(int fd, const char *name)
{
  int ret = 0;

  if (name) {
    struct ifreq ifr;

    strncpy(ifr.ifr_name, name, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ-1] = '\0';
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0)
      ret = ifr.ifr_flags & IFF_LOOPBACK;
  }
  return ret;
}

static void countInterfaces(int fd, caddr_t buf, size_t len)
{
  caddr_t ifPtr = buf;
  DWORD numNonLoopbackInterfaces = 0, numLoopbackInterfaces = 0;

  while (ifPtr && ifPtr < buf + len) {
    struct ifreq *ifr = (struct ifreq *)ifPtr;

    if (isLoopbackInterface(fd, ifr->ifr_name))
      numLoopbackInterfaces++;
    else
      numNonLoopbackInterfaces++;
    ifPtr += ifreq_len(ifr);
  }
  gNonLoopbackInterfaceMap = sizeMap(gNonLoopbackInterfaceMap,
   numNonLoopbackInterfaces);
  gLoopbackInterfaceMap = sizeMap(gLoopbackInterfaceMap,
   numLoopbackInterfaces);
}

/* Stores the name in the given map, and increments the map's numInterfaces
 * member if stored successfully.  Will store in the same slot as previously if
 * usedLastPass is set, otherwise will store in a new slot.
 * Assumes map and name are not NULL, and the usedLastPass flag is set
 * correctly for each entry in the map, and that map->numInterfaces <
 * map->numAllocated.
 * FIXME: this is kind of expensive, doing a linear scan of the map with a
 * string comparison of each entry to find the old slot.
 */
static void storeInterfaceInMap(InterfaceNameMap *map, const char *name)
{
  if (map && name) {
    DWORD ndx;
    BOOL stored = FALSE;

    /* look for previous slot, mark in use if so */
    for (ndx = 0; !stored && ndx < map->nextAvailable; ndx++) {
      if (map->table[ndx].usedLastPass && !strncmp(map->table[ndx].name, name,
       sizeof(map->table[ndx].name))) {
        map->table[ndx].inUse = TRUE;
        stored = TRUE;
      }
    }
    /* look for new slot */
    for (ndx = 0; !stored && ndx < map->numAllocated; ndx++) {
      if (!map->table[ndx].inUse) {
        strncpy(map->table[ndx].name, name, IFNAMSIZ);
        map->table[ndx].name[IFNAMSIZ-1] = '\0';
        map->table[ndx].inUse = TRUE;
        stored = TRUE;
        if (ndx >= map->nextAvailable)
          map->nextAvailable = ndx + 1;
      }
    }
    if (stored)
      map->numInterfaces++;
  }
}

/* Sets all used entries' usedLastPass flag to their inUse flag, clears
 * their inUse flag, and clears their numInterfaces member.
 */
static void markOldInterfaces(InterfaceNameMap *map)
{
  if (map) {
    DWORD ndx;

    map->numInterfaces = 0;
    for (ndx = 0; ndx < map->nextAvailable; ndx++) {
      map->table[ndx].usedLastPass = map->table[ndx].inUse;
      map->table[ndx].inUse = FALSE;
    }
  }
}

static void classifyInterfaces(int fd, caddr_t buf, size_t len)
{
  caddr_t ifPtr = buf;

  markOldInterfaces(gNonLoopbackInterfaceMap);
  markOldInterfaces(gLoopbackInterfaceMap);
  while (ifPtr && ifPtr < buf + len) {
    struct ifreq *ifr = (struct ifreq *)ifPtr;

    if (ifr->ifr_addr.sa_family == AF_INET) {
      if (isLoopbackInterface(fd, ifr->ifr_name))
        storeInterfaceInMap(gLoopbackInterfaceMap, ifr->ifr_name);
      else
        storeInterfaceInMap(gNonLoopbackInterfaceMap, ifr->ifr_name);
    }
    ifPtr += ifreq_len(ifr);
  }
}

static void enumerateInterfaces(void)
{
  int fd;

  fd = socket(PF_INET, SOCK_DGRAM, 0);
  if (fd != -1) {
    int ret, guessedNumInterfaces;
    struct ifconf ifc;

    /* try to avoid silly heap action by starting with the right size buffer */
    guessedNumInterfaces = 0;
    if (gNonLoopbackInterfaceMap)
      guessedNumInterfaces += gNonLoopbackInterfaceMap->numInterfaces;
    if (gLoopbackInterfaceMap)
      guessedNumInterfaces += gLoopbackInterfaceMap->numInterfaces;

    ret = 0;
    memset(&ifc, 0, sizeof(ifc));
    /* there is no way to know the interface count beforehand,
       so we need to loop again and again upping our max each time
       until returned < max */
    do {
      if (guessedNumInterfaces == 0)
        guessedNumInterfaces = INITIAL_INTERFACES_ASSUMED;
      else
        guessedNumInterfaces *= 2;
      if (ifc.ifc_buf)
        free(ifc.ifc_buf);
      ifc.ifc_len = sizeof(struct ifreq) * guessedNumInterfaces;
      ifc.ifc_buf = (char *)malloc(ifc.ifc_len);
      ret = ioctl(fd, SIOCGIFCONF, &ifc);
    } while (ret == 0 &&
     ifc.ifc_len == (sizeof(struct ifreq) * guessedNumInterfaces));

    if (ret == 0) {
      EnterCriticalSection(&mapCS);
      countInterfaces(fd, ifc.ifc_buf, ifc.ifc_len);
      classifyInterfaces(fd, ifc.ifc_buf, ifc.ifc_len);
      LeaveCriticalSection(&mapCS);
    }

    if (ifc.ifc_buf)
      free(ifc.ifc_buf);
    close(fd);
  }
}

DWORD getNumNonLoopbackInterfaces(void)
{
  enumerateInterfaces();
  return gNonLoopbackInterfaceMap ? gNonLoopbackInterfaceMap->numInterfaces : 0;
}

DWORD getNumInterfaces(void)
{
  DWORD ret = getNumNonLoopbackInterfaces();

  ret += gLoopbackInterfaceMap ? gLoopbackInterfaceMap->numInterfaces : 0;
  return ret;
}

const char *getInterfaceNameByIndex(DWORD index)
{
  DWORD realIndex;
  InterfaceNameMap *map;
  const char *ret = NULL;

  EnterCriticalSection(&mapCS);
  if (index & INDEX_IS_LOOPBACK) {
    realIndex = index ^ INDEX_IS_LOOPBACK;
    map = gLoopbackInterfaceMap;
  }
  else {
    realIndex = index;
    map = gNonLoopbackInterfaceMap;
  }
  if (map && realIndex < map->nextAvailable)
    ret = map->table[realIndex].name;
  LeaveCriticalSection(&mapCS);
  return ret;
}

DWORD getInterfaceIndexByName(const char *name, PDWORD index)
{
  DWORD ndx, ret;
  BOOL found = FALSE;

  if (!name)
    return ERROR_INVALID_PARAMETER;
  if (!index)
    return ERROR_INVALID_PARAMETER;

  EnterCriticalSection(&mapCS);
  for (ndx = 0; !found && gNonLoopbackInterfaceMap &&
   ndx < gNonLoopbackInterfaceMap->nextAvailable; ndx++)
    if (!strncmp(gNonLoopbackInterfaceMap->table[ndx].name, name, IFNAMSIZ)) {
      found = TRUE;
      *index = ndx;
    }
  for (ndx = 0; !found && gLoopbackInterfaceMap &&
   ndx < gLoopbackInterfaceMap->nextAvailable; ndx++)
    if (!strncmp(gLoopbackInterfaceMap->table[ndx].name, name, IFNAMSIZ)) {
      found = TRUE;
      *index = ndx | INDEX_IS_LOOPBACK;
    }
  LeaveCriticalSection(&mapCS);
  if (found)
    ret = NO_ERROR;
  else
    ret = ERROR_INVALID_DATA;
  return ret;
}

static void addMapEntriesToIndexTable(InterfaceIndexTable *table,
 const InterfaceNameMap *map)
{
  if (table && map) {
    DWORD ndx;

    for (ndx = 0; ndx < map->nextAvailable &&
     table->numIndexes < table->numAllocated; ndx++)
      if (map->table[ndx].inUse) {
        DWORD externalNdx = ndx;

        if (map == gLoopbackInterfaceMap)
          externalNdx |= INDEX_IS_LOOPBACK;
        table->indexes[table->numIndexes++] = externalNdx;
      }
  }
}

InterfaceIndexTable *getInterfaceIndexTable(void)
{
  DWORD numInterfaces;
  InterfaceIndexTable *ret;
 
  EnterCriticalSection(&mapCS);
  numInterfaces = getNumInterfaces();
  ret = (InterfaceIndexTable *)calloc(1,
   sizeof(InterfaceIndexTable) + (numInterfaces - 1) * sizeof(DWORD));
  if (ret) {
    ret->numAllocated = numInterfaces;
    addMapEntriesToIndexTable(ret, gNonLoopbackInterfaceMap);
    addMapEntriesToIndexTable(ret, gLoopbackInterfaceMap);
  }
  LeaveCriticalSection(&mapCS);
  return ret;
}

InterfaceIndexTable *getNonLoopbackInterfaceIndexTable(void)
{
  DWORD numInterfaces;
  InterfaceIndexTable *ret;

  EnterCriticalSection(&mapCS);
  numInterfaces = getNumNonLoopbackInterfaces();
  ret = (InterfaceIndexTable *)calloc(1,
   sizeof(InterfaceIndexTable) + (numInterfaces - 1) * sizeof(DWORD));
  if (ret) {
    ret->numAllocated = numInterfaces;
    addMapEntriesToIndexTable(ret, gNonLoopbackInterfaceMap);
  }
  LeaveCriticalSection(&mapCS);
  return ret;
}

DWORD getInterfaceIPAddrByName(const char *name)
{
  DWORD ret = INADDR_ANY;

  if (name) {
    int fd = socket(PF_INET, SOCK_DGRAM, 0);

    if (fd != -1) {
      struct ifreq ifr;

      strncpy(ifr.ifr_name, name, IFNAMSIZ);
      ifr.ifr_name[IFNAMSIZ-1] = '\0';
      if (ioctl(fd, SIOCGIFADDR, &ifr) == 0)
        memcpy(&ret, ifr.ifr_addr.sa_data + 2, sizeof(DWORD));
      close(fd);
    }
  }
  return ret;
}

DWORD getInterfaceIPAddrByIndex(DWORD index)
{
  DWORD ret;
  const char *name = getInterfaceNameByIndex(index);

  if (name)
    ret = getInterfaceIPAddrByName(name);
  else
    ret = INADDR_ANY;
  return ret;
}

DWORD getInterfaceBCastAddrByName(const char *name)
{
  DWORD ret = INADDR_ANY;

  if (name) {
    int fd = socket(PF_INET, SOCK_DGRAM, 0);

    if (fd != -1) {
      struct ifreq ifr;

      strncpy(ifr.ifr_name, name, IFNAMSIZ);
      ifr.ifr_name[IFNAMSIZ-1] = '\0';
      if (ioctl(fd, SIOCGIFBRDADDR, &ifr) == 0)
        memcpy(&ret, ifr.ifr_addr.sa_data + 2, sizeof(DWORD));
      close(fd);
    }
  }
  return ret;
}

DWORD getInterfaceBCastAddrByIndex(DWORD index)
{
  DWORD ret;
  const char *name = getInterfaceNameByIndex(index);

  if (name)
    ret = getInterfaceBCastAddrByName(name);
  else
    ret = INADDR_ANY;
  return ret;
}

DWORD getInterfaceMaskByName(const char *name)
{
  DWORD ret = INADDR_NONE;

  if (name) {
    int fd = socket(PF_INET, SOCK_DGRAM, 0);

    if (fd != -1) {
      struct ifreq ifr;

      strncpy(ifr.ifr_name, name, IFNAMSIZ);
      ifr.ifr_name[IFNAMSIZ-1] = '\0';
      if (ioctl(fd, SIOCGIFNETMASK, &ifr) == 0)
        memcpy(&ret, ifr.ifr_addr.sa_data + 2, sizeof(DWORD));
      close(fd);
    }
  }
  return ret;
}

DWORD getInterfaceMaskByIndex(DWORD index)
{
  DWORD ret;
  const char *name = getInterfaceNameByIndex(index);

  if (name)
    ret = getInterfaceMaskByName(name);
  else
    ret = INADDR_NONE;
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
    strncpy(ifr.ifr_name, name, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ-1] = '\0';
    if ((ioctl(fd, SIOCGIFHWADDR, &ifr)))
      ret = ERROR_INVALID_DATA;
    else {
      int addrLen;

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

      memset(&arp, 0, sizeof(struct arpreq));
      arp.arp_pa.sa_family = AF_INET;
      saddr = (struct sockaddr_in *)&arp; /* proto addr is first member */
      saddr->sin_family = AF_INET;
      saddr->sin_addr.s_addr = getInterfaceIPAddrByName(name);
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

  buf = (u_char *)malloc(mibLen);
  if (!buf)
    return ERROR_NOT_ENOUGH_MEMORY;

  if (sysctl(mib, 6, buf, &mibLen, NULL, 0) < 0) {
    free(buf);
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
  free(buf);
  return ret;
}
#endif

DWORD getInterfacePhysicalByIndex(DWORD index, PDWORD len, PBYTE addr,
 PDWORD type)
{
  const char *name = getInterfaceNameByIndex(index);

  if (name)
    return getInterfacePhysicalByName(name, len, addr, type);
  else
    return ERROR_INVALID_DATA;
}

DWORD getInterfaceMtuByName(const char *name, PDWORD mtu)
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

    strncpy(ifr.ifr_name, name, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ-1] = '\0';
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
  }
  else
    ret = ERROR_NO_MORE_FILES;
  return ret;
}

DWORD getInterfaceMtuByIndex(DWORD index, PDWORD mtu)
{
  const char *name = getInterfaceNameByIndex(index);

  if (name)
    return getInterfaceMtuByName(name, mtu);
  else
    return ERROR_INVALID_DATA;
}

DWORD getInterfaceStatusByName(const char *name, PDWORD status)
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

    strncpy(ifr.ifr_name, name, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ-1] = '\0';
    if ((ioctl(fd, SIOCGIFFLAGS, &ifr)))
      ret = ERROR_INVALID_DATA;
    else {
      if (ifr.ifr_flags & IFF_UP)
        *status = MIB_IF_OPER_STATUS_OPERATIONAL;
      else
        *status = MIB_IF_OPER_STATUS_NON_OPERATIONAL;
      ret = NO_ERROR;
    }
  }
  else
    ret = ERROR_NO_MORE_FILES;
  return ret;
}

DWORD getInterfaceStatusByIndex(DWORD index, PDWORD status)
{
  const char *name = getInterfaceNameByIndex(index);

  if (name)
    return getInterfaceStatusByName(name, status);
  else
    return ERROR_INVALID_DATA;
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
  const char *name = getInterfaceNameByIndex(index);

  if (name)
    return getInterfaceEntryByName(name, entry);
  else
    return ERROR_INVALID_DATA;
}

char *toIPAddressString(unsigned int addr, char string[16])
{
  if (string) {
    struct in_addr iAddr;

    iAddr.s_addr = addr;
    /* extra-anal, just to make auditors happy */
    strncpy(string, inet_ntoa(iAddr), 16);
    string[16] = '\0';
  }
  return string;
}
