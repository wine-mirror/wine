/*
 * HTTP handling functions.
 *
 * Copyright 2003 Ferenc Wagner
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
 *
 */
#include <winsock.h>
#include <stdio.h>
#include <errno.h>

#include "winetest.h"

SOCKET
open_http (const char *ipnum)
{
    WSADATA wsad;
    struct sockaddr_in sa;
    SOCKET s;

    report (R_STATUS, "Opening HTTP connection to %s", ipnum);
    if (WSAStartup (MAKEWORD (2,2), &wsad)) return INVALID_SOCKET;

    s = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s != INVALID_SOCKET) {
        sa.sin_family = AF_INET;
        sa.sin_port = htons (80);
        sa.sin_addr.s_addr = inet_addr (ipnum);
        if (!connect (s, (struct sockaddr*)&sa,
                      sizeof (struct sockaddr_in)))
            return s;
    }
    WSACleanup ();
    return INVALID_SOCKET;
}

int
close_http (SOCKET s)
{
    int ret;

    ret = closesocket (s);
    return (WSACleanup () || ret);
}

int
send_buf (SOCKET s, const char *buf, size_t length)
{
    int sent;

    while (length > 0) {
        sent = send (s, buf, length, 0);
        if (sent == SOCKET_ERROR) return 1;
        buf += sent;
        length -= sent;
    }
    return 0;
}

int
send_str (SOCKET s, ...)
{
    va_list ap;
    char *p;
    int ret;
    size_t len;

    va_start (ap, s);
    p = vstrmake (&len, ap);
    va_end (ap);
    if (!p) return 1;
    ret = send_buf (s, p, len);
    free (p);
    return ret;
}

int
send_file (const char *name)
{
    SOCKET s;
    FILE *f;
    unsigned char buffer[8192];
    size_t bytes_read, total, filesize;
    char *str;
    int ret;

    /* RFC 2068 */
#define SEP "-"
    const char head[] = "POST /~wferi/cgi-bin/winetests.cgi HTTP/1.0\r\n"
        "Host: afavant\r\n"
        "User-Agent: Winetests Shell\r\n"
        "Content-Type: multipart/form-data; boundary=" SEP "\r\n"
        "Content-Length: %u\r\n\r\n";
    const char body1[] = "--" SEP "\r\n"
        "Content-Disposition: form-data; name=reportfile; filename=\"%s\"\r\n"
        "Content-Type: application/octet-stream\r\n\r\n";
    const char body2[] = "\r\n--" SEP "\r\n"
        "Content-Dispoition: form-data; name=submit\r\n\r\n"
        "Upload File\r\n"
        "--" SEP "--\r\n";

    s = open_http ("157.181.170.47");
    if (s == INVALID_SOCKET) {
        report (R_WARNING, "Can't open network connection: %d",
                WSAGetLastError ());
        return 1;
    }

    f = fopen (name, "rb");
    if (!f) {
        report (R_WARNING, "Can't open file '%s': %d", name, errno);
        goto abort1;
    }
    fseek (f, 0, SEEK_END);
    filesize = ftell (f);
    if (filesize > 1024*1024) {
        report (R_WARNING,
                "File too big (%d > 1 MB), copy and submit manually",
                filesize);
        goto abort2;
    }
    fseek (f, 0, SEEK_SET);

    report (R_STATUS, "Sending header");
    str = strmake (&total, body1, name);
    ret = send_str (s, head, filesize + total + sizeof body2 - 1) ||
        send_buf (s, str, total);
    free (str);
    if (ret) {
        report (R_WARNING, "Error sending header: %d, %d",
                errno, WSAGetLastError ());
        goto abort2;
    }

    report (R_STATUS, "Sending %u bytes of data", filesize);
    report (R_PROGRESS, filesize);
    while ((bytes_read = fread (buffer, 1, sizeof buffer / 8, f))) {
        if (send_buf (s, buffer, bytes_read)) {
            report (R_WARNING, "Error sending body: %d, %d",
                    errno, WSAGetLastError ());
            goto abort2;
        }
        report (R_DELTA, bytes_read, "Network transfer: In progress");
    }
    fclose (f);

    if (send_buf (s, body2, sizeof body2 - 1)) {
        report (R_WARNING, "Error sending trailer: %d, %d",
                errno, WSAGetLastError ());
        goto abort2;
    }
    report (R_DELTA, 0, "Network transfer: Done");

    total = 0;
    while ((bytes_read = recv (s, buffer + total,
                               sizeof buffer - total, 0))) {
        if ((signed)bytes_read == SOCKET_ERROR) {
            report (R_WARNING, "Error receiving reply: %d, %d",
                    errno, WSAGetLastError ());
            goto abort1;
        }
        total += bytes_read;
        if (total == sizeof buffer) {
            report (R_WARNING, "Buffer overflow");
            goto abort1;
        }
    }
    if (close_http (s)) {
        report (R_WARNING, "Error closing connection: %d, %d",
                errno, WSAGetLastError ());
        return 1;
    }

    str = strmake (&bytes_read, "Received %s (%d bytes).\n",
                   name, filesize);
    ret = memcmp (str, buffer + total - bytes_read, bytes_read);
    free (str);
    return ret!=0;

 abort2:
    fclose (f);
 abort1:
    close_http (s);
    return 1;
}
