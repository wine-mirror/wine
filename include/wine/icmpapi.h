/*
 * Interface to the ICMP functions.
 *
 * This header is not part of the standard headers, it is usually 
 * delivered separately and this is why it is not directly in 'include'.
 *
 * Depends on ipexport.h (there is no include directive in the original)
 */

#ifndef __WINE_ICMPAPI_H
#define __WINE_ICMPAPI_H

HANDLE WINAPI IcmpCreateFile(
    VOID
    );

BOOL WINAPI IcmpCloseHandle(
    HANDLE  IcmpHandle
    );

DWORD WINAPI IcmpSendEcho(
    HANDLE                 IcmpHandle,
    IPAddr                 DestinationAddress,
    LPVOID                 RequestData,
    WORD                   RequestSize,
    PIP_OPTION_INFORMATION RequestOptions,
    LPVOID                 ReplyBuffer,
    DWORD                  ReplySize,
    DWORD                  Timeout
    );


#endif /* __WINE_ICMPAPI_H */
