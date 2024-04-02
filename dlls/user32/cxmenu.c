/*
 * cxmenu support
 *
 * Copyright 2019 Alexandre Julliard
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
 */

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <poll.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "winternl.h"
#include "user_private.h"
#include "wine/unixlib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cursor);

/***********************************************************************
 *           MENU_get_menu_socket
 *
 *  CrossOver HACKs for bug 6727.
 *
 *  On OSX, crossover.app can put a socket handle in
 *   the CX_MENU_SOCKET environemtn variable.  If that
 *   value is set then we will make an XML structure out
 *   of the menu bar and pass it upstream so that the mac
 *   application can display a proper mac-style menubar.
 *
 */
static int MENU_get_menu_socket(void)
{
    char *socketname;

    if ((socketname = getenv("CX_MENU_SOCKET")))
    {
        struct sockaddr_un sa;
        int sock = socket(AF_UNIX,SOCK_STREAM,0);
        TRACE("Found socket %s.\n",socketname);

        sa.sun_family=AF_UNIX;
        if (strlen(socketname) > (sizeof(sa.sun_path)-1))
        {
            TRACE("Socket name %s is too long for us to use!\n", socketname);
            return -1;
        }
        strcpy(sa.sun_path,socketname);

        if (!connect(sock, (struct sockaddr *) &sa, sizeof(sa)))
        {
            /* Make the socket nonblocking.  That prevents us from locking up
               if the Mac App stops listening. */
            int flags = fcntl(sock, F_GETFL);
            if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1)
            {
                TRACE("Failed to set socket to O_NONBLOCK.\n");
            }

            return sock;
        }
        else
        {
            WINE_WARN("Failed to connect to menu socket %s.  errno: %d\n",socketname,errno);
            return -1;
        }
    }

    return -1;
}

static BOOL MENU_write_data_to_pipe(int sock, const char *data, int len)
{
    int written = 0;
    short revents = 0;

    while (written < len && !(revents & (POLLERR | POLLHUP)))
    {
        struct pollfd pollstruct;
        int ready;

        pollstruct.fd = sock;
        pollstruct.events = POLLOUT;
        pollstruct.revents = revents;

        ready = poll(&pollstruct, 1, 1000);
        if (ready && (ready != -1))
        {
            int thisChunkSize;
            thisChunkSize = write(sock,data+written,len-written);
            if (thisChunkSize == -1)
            {
                TRACE("Failed to write menu info.  errno: %d\n", errno);
                break;
            }
            written += thisChunkSize;
        }
        else
        {
            /*  Timed out.  If the Mac app isn't listening, we
                can just error out... there will be plenty more
                menus where this one came from.  */
            return FALSE;
        }
    }

    if (written < len)
    {
        WARN("Failed to write to menu socket.  errno: %d\n",errno);
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *           send_cx_menu_data
 *
 *  CrossOver Hack for bug 6727.
 */
static NTSTATUS send_cx_menu_data( void *args )
{
    struct xml_buffer *buffer = args;
    int sock = MENU_get_menu_socket();

    if (sock == -1) return STATUS_UNSUCCESSFUL;
    MENU_write_data_to_pipe( sock, buffer->data, buffer->len );
    close( sock );
    return STATUS_SUCCESS;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    send_cx_menu_data,
};
