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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <winsock2.h>
#include <wininet.h>
#include <stdio.h>
#include <errno.h>

#include "winetest.h"

#define USER_AGENT  "Winetest Shell"
#define SERVER_NAME "test.winehq.org"
#define URL_PATH "/submit"
#define SEP "--8<--cut-here--8<--"
#define CONTENT_HEADERS "Content-Type: multipart/form-data; boundary=\"" SEP "\"\r\n" \
                        "Content-Length: %u\r\n\r\n"
static const char body1[] = "--" SEP "\r\n"
    "Content-Disposition: form-data; name=\"reportfile\"; filename=\"%s\"\r\n"
    "Content-Type: application/octet-stream\r\n\r\n";
static const char body2[] = "\r\n--" SEP "\r\n"
    "Content-Disposition: form-data; name=\"submit\"\r\n\r\n"
    "Upload File\r\n"
    "--" SEP "--\r\n";

static SOCKET
open_http (const char *server)
{
    WSADATA wsad;
    struct sockaddr_in sa;
    SOCKET s;

    report (R_STATUS, "Opening HTTP connection to %s", server);
    if (WSAStartup (MAKEWORD (2,2), &wsad)) return INVALID_SOCKET;

    sa.sin_family = AF_INET;
    sa.sin_port = htons (80);
    sa.sin_addr.s_addr = inet_addr (server);
    memset (sa.sin_zero, 0, sizeof(sa.sin_zero));
    if (sa.sin_addr.s_addr == INADDR_NONE) {
        struct hostent *host = gethostbyname (server);
        if (!host) {
            report (R_ERROR, "Hostname lookup failed for %s", server);
            goto failure;
        }
        sa.sin_addr.s_addr = ((struct in_addr *)host->h_addr)->s_addr;
    }
    s = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        report (R_ERROR, "Can't open network socket: %d",
                WSAGetLastError ());
        goto failure;
    }
    if (!connect (s, (struct sockaddr*)&sa, sizeof (struct sockaddr_in)))
        return s;

    report (R_ERROR, "Can't connect: %d", WSAGetLastError ());
    closesocket (s);
 failure:
    WSACleanup ();
    return INVALID_SOCKET;
}

static int
close_http (SOCKET s)
{
    int ret;

    ret = closesocket (s);
    return (WSACleanup () || ret);
}

static int
send_buf (SOCKET s, const char *buf, size_t length)
{
    int sent;

    while (length > 0) {
        sent = send (s, buf, length, 0);
        if (sent == SOCKET_ERROR) {
            if (errno == EINTR) continue;
            return 1;
        }
        buf += sent;
        length -= sent;
    }
    return 0;
}

static int WINAPIV send_str (SOCKET s, ...)
{
    va_list ap;
    char *p;
    int ret;
    size_t len;

    va_start (ap, s);
    p = vstrmake (&len, ap);
    va_end (ap);
    ret = send_buf (s, p, len);
    free(p);
    return ret;
}

static int
send_file_direct (const char * url, const char *name)
{
    SOCKET s;
    HANDLE file;
#define BUFLEN 8192
    char buffer[BUFLEN+1];
    DWORD bytes_read, filesize;
    size_t total, count;
    char *str;
    int ret;

    /* RFC 2616 */
    static const char head[] = "POST " URL_PATH " HTTP/1.0\r\n"
        "Host: " SERVER_NAME "\r\n"
        "User-Agent: " USER_AGENT "\r\n"
        CONTENT_HEADERS
        "\r\n";

    if (url) {
        report (R_WARNING, "Can't submit to an alternative URL");
        return 0;
    }

    s = open_http (SERVER_NAME);
    if (s == INVALID_SOCKET) return 1;

    file = CreateFileA( name, GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, 0, NULL );

    if ((file == INVALID_HANDLE_VALUE) &&
        (GetLastError() == ERROR_INVALID_PARAMETER)) {
        /* FILE_SHARE_DELETE not supported on win9x */
        file = CreateFileA( name, GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, 0, NULL );
    }
    if (file == INVALID_HANDLE_VALUE)
    {
        report (R_WARNING, "Can't open file '%s': %u", name, GetLastError());
        goto abort1;
    }

    report (R_STATUS, "Sending header");
    filesize = GetFileSize( file, NULL );
    str = strmake (&total, body1, name);
    ret = send_str (s, head, filesize + total + sizeof body2 - 1) ||
        send_buf (s, str, total);
    free(str);
    if (ret) {
        report (R_WARNING, "Error sending header: %d, %d",
                errno, WSAGetLastError ());
        goto abort2;
    }

    report (R_STATUS, "Sending %u bytes of data", filesize);
    report (R_PROGRESS, 2, filesize);
    total = 0;
    while (total < filesize && ReadFile( file, buffer, BUFLEN/2, &bytes_read, NULL )) {
        if (aborting) goto abort2;
        if (!bytes_read) break;
        total += bytes_read;
        if (total > filesize) bytes_read -= total - filesize;
        if (send_buf (s, buffer, bytes_read)) {
            report (R_WARNING, "Error sending body: %d, %d",
                    errno, WSAGetLastError ());
            goto abort2;
        }
        report (R_DELTA, bytes_read, "Network transfer: In progress");
    }
    CloseHandle( file );

    if (send_buf (s, body2, sizeof body2 - 1)) {
        report (R_WARNING, "Error sending trailer: %d, %d",
                errno, WSAGetLastError ());
        goto abort1;
    }
    report (R_DELTA, 0, "Network transfer: Done");

    total = 0;
    while ((bytes_read = recv (s, buffer+total, BUFLEN-total, 0))) {
        if ((signed)bytes_read == SOCKET_ERROR) {
            if (errno == EINTR) continue;
            report (R_WARNING, "Error receiving reply: %d, %d",
                    errno, WSAGetLastError ());
            goto abort1;
        }
        total += bytes_read;
        if (total == BUFLEN) {
            report (R_WARNING, "Buffer overflow");
            goto abort1;
        }
    }
    if (close_http (s)) {
        report (R_WARNING, "Error closing connection: %d, %d",
                errno, WSAGetLastError ());
        return 1;
    }

    str = strmake (&count, "Received %s (%d bytes).\n",
                   name, filesize);
    ret = total < count || memcmp (str, buffer + total - count, count) != 0;
    free(str);
    if (ret) {
        buffer[total] = 0;
        str = strstr (buffer, "\r\n\r\n");
        if (!str) str = buffer;
        else str = str + 4;
        report (R_ERROR, "Can't submit logfile '%s'. "
                "Server response: %s", name, str);
    }
    return ret;

 abort2:
    CloseHandle( file );
 abort1:
    close_http (s);
    return 1;
}

static int
send_file_wininet (const char *url, const char *name)
{
    int ret = 0;
    HMODULE wininet_mod = NULL;
    HINTERNET (WINAPI *pInternetOpen)(LPCSTR agent, DWORD access_type, LPCSTR proxy_name, LPCSTR proxy_bypass, DWORD flags);
    HINTERNET (WINAPI *pInternetConnect)(HINTERNET session, LPCSTR server_name, INTERNET_PORT server_port, LPCSTR username, LPCSTR password, DWORD service, DWORD flags, DWORD_PTR *context);
    HINTERNET (WINAPI *pHttpOpenRequest)(HINTERNET connection, LPCSTR verb, LPCSTR object_name, LPCSTR version, LPCSTR referer, LPCSTR *accept_types, DWORD flags, DWORD_PTR context);
    BOOL (WINAPI *pHttpSendRequestEx)(HINTERNET request, LPINTERNET_BUFFERSA buffers_in, LPINTERNET_BUFFERSA buffers_out, DWORD flags, DWORD_PTR context);
    BOOL (WINAPI *pInternetWriteFile)(HINTERNET file, LPCVOID buffer, DWORD number_of_bytes_to_write, LPDWORD number_of_bytes_written);
    BOOL (WINAPI *pHttpEndRequest)(HINTERNET request, LPINTERNET_BUFFERSA buffers_out, DWORD flags, DWORD_PTR context);
    BOOL (WINAPI *pInternetReadFile)(HINTERNET file, LPCVOID buffer, DWORD number_of_bytes_to_read, LPDWORD number_of_bytes_read);
    BOOL (WINAPI *pInternetCloseHandle)(HINTERNET Handle) = NULL;
    HANDLE file = INVALID_HANDLE_VALUE;
    DWORD filesize, bytes_read, bytes_written;
    size_t total, count;
    char *str = NULL;
    HINTERNET session = NULL;
    HINTERNET connection = NULL;
    HINTERNET request = NULL;
    INTERNET_BUFFERSA buffers_in = { 0 };
    char buffer[BUFLEN+1];
    URL_COMPONENTSA uc = { 0 };

    static const char extra_headers[] =
        CONTENT_HEADERS;

    wininet_mod = LoadLibraryA("wininet.dll");
    if (wininet_mod == NULL)
        goto done;
    pInternetOpen = (void *)GetProcAddress(wininet_mod, "InternetOpenA");
    pInternetConnect = (void *)GetProcAddress(wininet_mod, "InternetConnectA");
    pHttpOpenRequest = (void *)GetProcAddress(wininet_mod, "HttpOpenRequestA");
    pHttpSendRequestEx = (void *)GetProcAddress(wininet_mod, "HttpSendRequestExA");
    pInternetWriteFile = (void *)GetProcAddress(wininet_mod, "InternetWriteFile");
    pHttpEndRequest = (void *)GetProcAddress(wininet_mod, "HttpEndRequestA");
    pInternetReadFile = (void *)GetProcAddress(wininet_mod, "InternetReadFile");
    pInternetCloseHandle = (void *)GetProcAddress(wininet_mod, "InternetCloseHandle");
    if (pInternetOpen == NULL || pInternetConnect == NULL || pHttpOpenRequest == NULL || pHttpSendRequestEx == NULL || pHttpEndRequest == NULL ||
        pInternetWriteFile == NULL || pInternetReadFile == NULL || pInternetCloseHandle == NULL) {
        goto done;
    }
    if (url) {
        BOOL (WINAPI *pInternetCrackUrlA)(const char *url, DWORD url_length, DWORD flags, URL_COMPONENTSA *ret_comp);
        pInternetCrackUrlA = (void *)GetProcAddress(wininet_mod, "InternetCrackUrlA");
        if (pInternetCrackUrlA == NULL)
            goto done;
        uc.dwStructSize = sizeof(uc);
        uc.dwSchemeLength = uc.dwHostNameLength = uc.dwUserNameLength =
            uc.dwPasswordLength = uc.dwUrlPathLength = uc.dwExtraInfoLength =
            strlen(url) + 1;
        uc.lpszScheme = xalloc(uc.dwSchemeLength);
        uc.lpszHostName = xalloc(uc.dwHostNameLength);
        uc.lpszUserName = xalloc(uc.dwUserNameLength);
        uc.lpszPassword = xalloc(uc.dwPasswordLength);
        uc.lpszUrlPath = xalloc(uc.dwUrlPathLength);
        uc.lpszExtraInfo = xalloc(uc.dwExtraInfoLength);
        if (!pInternetCrackUrlA(url, 0, 0, &uc)) {
            report (R_WARNING, "Can't parse submit URL '%s'", url);
            goto done;
        }
        if ((uc.nScheme != INTERNET_SCHEME_HTTP && uc.nScheme != INTERNET_SCHEME_HTTPS) || *uc.lpszExtraInfo) {
            report (R_WARNING, "Can't submit report to scheme %s or extra info '%s'", uc.lpszScheme, uc.lpszExtraInfo);
            goto done;
        }

    } else {
        uc.nScheme = INTERNET_SCHEME_HTTP;
        uc.lpszHostName = xstrdup(SERVER_NAME);
        uc.nPort = INTERNET_DEFAULT_HTTP_PORT;
        uc.lpszUserName = xstrdup("");
        uc.lpszPassword = xstrdup("");
        uc.lpszUrlPath = xstrdup(URL_PATH);
    }

    ret = 1;

    file = CreateFileA( name, GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, 0, NULL );

    if ((file == INVALID_HANDLE_VALUE) &&
        (GetLastError() == ERROR_INVALID_PARAMETER)) {
        /* FILE_SHARE_DELETE not supported on win9x */
        file = CreateFileA( name, GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, 0, NULL );
    }
    if (file == INVALID_HANDLE_VALUE) {
        report (R_WARNING, "Can't open file '%s': %u", name, GetLastError());
        goto done;
    }

    report (R_STATUS, "Opening %s connection to %s:%d",
            (uc.nScheme == INTERNET_SCHEME_HTTP ? "http" : "https"),
            uc.lpszHostName, uc.nPort);
    session = pInternetOpen (USER_AGENT, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (session == NULL) {
        report (R_WARNING, "Unable to open connection, error %u", GetLastError());
        goto done;
    }
    connection = pInternetConnect (session, uc.lpszHostName, uc.nPort, uc.lpszUserName, uc.lpszPassword, INTERNET_SERVICE_HTTP, 0, 0);
    if (connection == NULL) {
        report (R_WARNING, "Unable to connect, error %u", GetLastError());
        goto done;
    }
    request = pHttpOpenRequest (connection, "POST", uc.lpszUrlPath, NULL, NULL, NULL,
                                INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_NO_UI |
                                INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_RELOAD | (uc.nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_FLAG_SECURE : 0), 0);
    if (request == NULL) {
        report (R_WARNING, "Unable to open request, error %u", GetLastError());
        goto done;
    }

    report (R_STATUS, "Sending request");
    filesize = GetFileSize( file, NULL );
    str = strmake (&total, body1, name);
    memset(&buffers_in, 0, sizeof(INTERNET_BUFFERSA));
    buffers_in.dwStructSize = sizeof(INTERNET_BUFFERSA);
    buffers_in.dwBufferTotal = filesize + total + sizeof body2 - 1;
    buffers_in.lpcszHeader = strmake (&count, extra_headers, buffers_in.dwBufferTotal);
    buffers_in.dwHeadersLength = count;
    if (! pHttpSendRequestEx(request, &buffers_in, NULL, 0, 0)) {
        report (R_WARNING, "Unable to send request, error %u", GetLastError());
        goto done;
    }

    if (! pInternetWriteFile(request, str, total, &bytes_written) || bytes_written != total) {
        report (R_WARNING, "Unable to write body data, error %u", GetLastError());
        goto done;
    }

    report (R_STATUS, "Sending %u bytes of data", filesize);
    report (R_PROGRESS, 2, filesize);
    total = 0;
    while (total < filesize && ReadFile( file, buffer, BUFLEN/2, &bytes_read, NULL )) {
        if (aborting) goto done;
        if (!bytes_read) break;
        total += bytes_read;
        if (total > filesize) bytes_read -= total - filesize;
        if (! pInternetWriteFile (request, buffer, bytes_read, &bytes_written) || bytes_written != bytes_read) {
            report (R_WARNING, "Error sending body: %u", GetLastError ());
            goto done;
        }
        report (R_DELTA, bytes_read, "Network transfer: In progress");
    }

    if (! pInternetWriteFile(request, body2, sizeof body2 - 1, &bytes_written) || bytes_written != sizeof body2 - 1) {
        report (R_WARNING, "Unable to write final body data, error %u", GetLastError());
        goto done;
    }
    if (! pHttpEndRequest(request, NULL, 0, 0)) {
        report (R_WARNING, "Unable to end request, error %u", GetLastError());
        goto done;
    }
    report (R_DELTA, 0, "Network transfer: Done");

    total = 0;
    do
    {
        if (! pInternetReadFile(request, buffer+total, BUFLEN-total, &bytes_read)) {
            report (R_WARNING, "Error receiving reply: %u", GetLastError ());
            goto done;
        }
        total += bytes_read;
        if (total == BUFLEN) {
            report (R_WARNING, "Buffer overflow");
            goto done;
        }
    }
    while (bytes_read != 0);

    free(str);
    str = strmake (&count, "Received %s (%d bytes).\n",
                   name, filesize);
    if (total < count || memcmp (str, buffer + total - count, count) != 0) {
        buffer[total] = 0;
        report (R_ERROR, "Can't submit logfile '%s'. "
                "Server response: %s", name, buffer);
    }

 done:
    free(uc.lpszScheme);
    free(uc.lpszHostName);
    free(uc.lpszUserName);
    free(uc.lpszPassword);
    free(uc.lpszUrlPath);
    free(uc.lpszExtraInfo);
    free((void *)buffers_in.lpcszHeader);
    free(str);
    if (pInternetCloseHandle != NULL && request != NULL)
        pInternetCloseHandle (request);
    if (pInternetCloseHandle != NULL && connection != NULL)
        pInternetCloseHandle (connection);
    if (pInternetCloseHandle != NULL && session != NULL)
        pInternetCloseHandle (session);
    if (file != INVALID_HANDLE_VALUE)
        CloseHandle (file);
    if (wininet_mod != NULL)
        FreeLibrary (wininet_mod);

    return ret;
}

int
send_file (const char *url, const char *name)
{
    return send_file_wininet( url, name ) || send_file_direct( url, name );
}
