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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"
#include "winerror.h"
#include "nb30.h"
#include "lmcons.h"
#include "iphlpapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(netbios);

HMODULE NETAPI32_hModule = 0;

static UCHAR NETBIOS_Enum(PNCB ncb)
{
    int             i;
    LANA_ENUM *lanas = (PLANA_ENUM) ncb->ncb_buffer;
    DWORD apiReturn, size = 0;
    PMIB_IFTABLE table;
    UCHAR ret;
 
    TRACE("NCBENUM\n");
 
    apiReturn = GetIfTable(NULL, &size, FALSE);
    if (apiReturn != NO_ERROR)
      {
        table = (PMIB_IFTABLE)malloc(size);
        if (table)
          {
            apiReturn = GetIfTable(table, &size, FALSE);
            if (apiReturn == NO_ERROR)
              {
                lanas->length = 0;
                for (i = 0; i < table->dwNumEntries && lanas->length < MAX_LANA;
                 i++)
                  {
                    if (table->table[i].dwType != MIB_IF_TYPE_LOOPBACK)
                      {
                        lanas->lana[lanas->length] = table->table[i].dwIndex;
                        lanas->length++;
                      }
                  }
                ret = NRC_GOODRET;
              }
            else
                ret = NRC_SYSTEM;
            free(table);
          }
        else
            ret = NRC_NORESOURCES;
      }
    else
        ret = NRC_SYSTEM;
    return ret;
}


static UCHAR NETBIOS_Astat(PNCB ncb)
{
    PADAPTER_STATUS astat = (PADAPTER_STATUS) ncb->ncb_buffer;
    MIB_IFROW row;
  
    TRACE("NCBASTAT (Adapter %d)\n", ncb->ncb_lana_num);
  
    memset(astat, 0, sizeof astat);
  
    row.dwIndex = ncb->ncb_lana_num;
    if (GetIfEntry(&row) != NO_ERROR)
        return NRC_INVADDRESS;
    /* doubt anyone cares, but why not.. */
    if (row.dwType == MIB_IF_TYPE_TOKENRING)
        astat->adapter_type = 0xff;
    else
        astat->adapter_type = 0xfe; /* for Ethernet */
    return NRC_GOODRET;
}

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%lx,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
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
        if(pncb->ncb_lana_num < MAX_LANA )
          {
            MIB_IFROW row;

            row.dwIndex = pncb->ncb_lana_num;
            if (GetIfEntry(&row) != NO_ERROR)
                ret = NRC_GOODRET;
            else
                ret = NRC_ILLCMD; /* NetBIOS emulator not found */
          }
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

NET_API_STATUS  WINAPI NetServerEnum(
  LPCWSTR servername,
  DWORD level,
  LPBYTE* bufptr,
  DWORD prefmaxlen,
  LPDWORD entriesread,
  LPDWORD totalentries,
  DWORD servertype,
  LPCWSTR domain,
  LPDWORD resume_handle
)
{
    FIXME("Stub (%p, %ld %p %ld %p %p %ld %s %p)\n",servername, level, bufptr,
          prefmaxlen, entriesread, totalentries, servertype, debugstr_w(domain), resume_handle);

    return ERROR_NO_BROWSER_SERVERS_FOUND;
}
