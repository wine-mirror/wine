#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wininet.h"

#include "wine/test.h"

int goon = 0;

VOID WINAPI callback(
     HINTERNET hInternet,
     DWORD dwContext,
     DWORD dwInternetStatus,
     LPVOID lpvStatusInformation,
     DWORD dwStatusInformationLength
)
{
    char name[124];

    switch (dwInternetStatus)
    {
        case INTERNET_STATUS_RESOLVING_NAME:
            strcpy(name,"INTERNET_STATUS_RESOLVING_NAME");
            break;
        case INTERNET_STATUS_NAME_RESOLVED:
            strcpy(name,"INTERNET_STATUS_NAME_RESOLVED");
            break;
        case INTERNET_STATUS_CONNECTING_TO_SERVER:
            strcpy(name,"INTERNET_STATUS_CONNECTING_TO_SERVER");
            break;
        case INTERNET_STATUS_CONNECTED_TO_SERVER:
            strcpy(name,"INTERNET_STATUS_CONNECTED_TO_SERVER");
            break;
        case INTERNET_STATUS_SENDING_REQUEST:
            strcpy(name,"INTERNET_STATUS_SENDING_REQUEST");
            break;
        case INTERNET_STATUS_REQUEST_SENT:
            strcpy(name,"INTERNET_STATUS_REQUEST_SENT");
            break;
        case INTERNET_STATUS_RECEIVING_RESPONSE:
            strcpy(name,"INTERNET_STATUS_RECEIVING_RESPONSE");
            break;
        case INTERNET_STATUS_RESPONSE_RECEIVED:
            strcpy(name,"INTERNET_STATUS_RESPONSE_RECEIVED");
            break;
        case INTERNET_STATUS_CTL_RESPONSE_RECEIVED:
            strcpy(name,"INTERNET_STATUS_CTL_RESPONSE_RECEIVED");
            break;
        case INTERNET_STATUS_PREFETCH:
            strcpy(name,"INTERNET_STATUS_PREFETCH");
            break;
        case INTERNET_STATUS_CLOSING_CONNECTION:
            strcpy(name,"INTERNET_STATUS_CLOSING_CONNECTION");
            break;
        case INTERNET_STATUS_CONNECTION_CLOSED:
            strcpy(name,"INTERNET_STATUS_CONNECTION_CLOSED");
            break;
        case INTERNET_STATUS_HANDLE_CREATED:
            strcpy(name,"INTERNET_STATUS_HANDLE_CREATED");
            break;
        case INTERNET_STATUS_HANDLE_CLOSING:
            strcpy(name,"INTERNET_STATUS_HANDLE_CLOSING");
            break;
        case INTERNET_STATUS_REQUEST_COMPLETE:
            strcpy(name,"INTERNET_STATUS_REQUEST_COMPLETE");
            goon = 1;
            break;
        case INTERNET_STATUS_REDIRECT:
            strcpy(name,"INTERNET_STATUS_REDIRECT");
            break;
        case INTERNET_STATUS_INTERMEDIATE_RESPONSE:
            strcpy(name,"INTERNET_STATUS_INTERMEDIATE_RESPONSE");
            break;
    }

    trace("Callback %p 0x%lx %s(%li) %p %ld\n",hInternet,dwContext,name,dwInternetStatus,lpvStatusInformation,dwStatusInformationLength);
}

void winapi_test(int flags)
{
    DWORD rc;
    CHAR buffer[4000];
    DWORD length;
    DWORD out;
    const char *types[2] = { "*", NULL };
    HINTERNET hi,hic,hor;

    trace("Starting with flags 0x%x\n",flags);

    trace("InternetOpenA <--\n");
    hi = InternetOpenA("",0x0,0x0,0x0,flags);
    ok((hi != 0x0),"InternetOpen Failed");
    trace("InternetOpenA -->\n");

    if (hi == 0x0) goto abort;

    InternetSetStatusCallback(hi,&callback);

    trace("InternetConnectA <--\n");
    hic=InternetConnectA(hi,"www.winehq.com",0x0,0x0,0x0,0x3,0x0,0xdeadbeef);
    ok((hic != 0x0),"InternetConnect Failed");
    trace("InternetConnectA -->\n");

    if (hic == 0x0) goto abort;

    trace("HttpOpenRequestA <--\n");
    hor = HttpOpenRequestA(hic, "GET",
                          "/about/",
                          0x0,0x0,types,0x00400800,0xdeadbead);
    if (hor == 0x0 && GetLastError() == 12007 /* ERROR_INTERNET_NAME_NOT_RESOLVED */) {
        /*
         * If the internet name can't be resolved we are probably behind
         * a firewall or in some other way not directly connected to the
         * Internet. Not enough reason to fail the test. Just ignore and
         * abort.
         */
    } else  {
        ok((hor != 0x0),"HttpOpenRequest Failed");
    }
    trace("HttpOpenRequestA -->\n");

    if (hor == 0x0) goto abort;

    trace("HttpSendRequestA -->\n");
    rc = HttpSendRequestA(hor, "", 0xffffffff,0x0,0x0);
    if (flags)
        ok(((rc == 0)&&(GetLastError()==997)),
            "Asyncronous HttpSendRequest NOT returning 0 with error 997");
    else
        ok((rc != 0), "Syncronous HttpSendRequest returning 0");
    trace("HttpSendRequestA <--\n");

    while ((flags)&&(!goon))
        Sleep(100);

    length = 4;
    rc = InternetQueryOptionA(hor,0x17,&out,&length);
    trace("Option 0x17 -> %li  %li\n",rc,out);

    length = 100;
    rc = InternetQueryOptionA(hor,0x22,buffer,&length);
    trace("Option 0x22 -> %li  %s\n",rc,buffer);

    length = 4000;
    rc = HttpQueryInfoA(hor,0x16,buffer,&length,0x0);
    buffer[length]=0;
    trace("Option 0x16 -> %li  %s\n",rc,buffer);

    length = 4000;
    rc = InternetQueryOptionA(hor,0x22,buffer,&length);
    buffer[length]=0;
    trace("Option 0x22 -> %li  %s\n",rc,buffer);

    length = 16;
    rc = HttpQueryInfoA(hor,0x5,&buffer,&length,0x0);
    trace("Option 0x5 -> %li  %s  (%li)\n",rc,buffer,GetLastError());

    length = 100;
    rc = HttpQueryInfoA(hor,0x1,buffer,&length,0x0);
    buffer[length]=0;
    trace("Option 0x1 -> %li  %s\n",rc,buffer);

    length = 100;
    trace("Entery Query loop\n");

    while (length)
    {

        rc = InternetQueryDataAvailable(hor,&length,0x0,0x0);
        ok((rc != 0),"InternetQueryDataAvailable failed");

        if (length)
        {
            char *buffer;
            buffer = (char*)HeapAlloc(GetProcessHeap(),0,length+1);

            rc = InternetReadFile(hor,buffer,length,&length);

            buffer[length]=0;

            trace("ReadFile -> %li %li\n",rc,length);

            HeapFree(GetProcessHeap(),0,buffer);
        }
    }
abort:
    if (hor != 0x0) {
        rc = InternetCloseHandle(hor);
        ok ((rc != 0), "InternetCloseHandle of handle opened by HttpOpenRequestA failed");
    }
    if (hor != 0x0) {
        rc = InternetCloseHandle(hic);
        ok ((rc != 0), "InternetCloseHandle of handle opened by InternetConnectA failed");
    }
    if (hi != 0x0) {
      rc = InternetCloseHandle(hi);
      ok ((rc != 0), "InternetCloseHandle of handle opened by InternetOpenA failed");
      if (flags)
          Sleep(100);
    }
}


START_TEST(http)
{
    winapi_test(0x10000000);
    winapi_test(0x00000000);
}
