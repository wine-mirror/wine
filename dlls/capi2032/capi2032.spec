 1 stdcall CAPI_REGISTER          (long long long long ptr) wrapCAPI_REGISTER
 2 stdcall CAPI_RELEASE           (long)                    wrapCAPI_RELEASE
 3 stdcall CAPI_PUT_MESSAGE       (long ptr)                wrapCAPI_PUT_MESSAGE
 4 stdcall CAPI_GET_MESSAGE       (long ptr)                wrapCAPI_GET_MESSAGE
 5 stdcall CAPI_WAIT_FOR_SIGNAL   (long)                    wrapCAPI_WAIT_FOR_SIGNAL
 6 stdcall CAPI_GET_MANUFACTURER  (str)                     wrapCAPI_GET_MANUFACTURER
 7 stdcall CAPI_GET_VERSION       (ptr ptr ptr ptr)         wrapCAPI_GET_VERSION
 8 stdcall CAPI_GET_SERIAL_NUMBER (str)                     wrapCAPI_GET_SERIAL_NUMBER
 9 stdcall CAPI_GET_PROFILE       (ptr long)                wrapCAPI_GET_PROFILE
10 stdcall CAPI_INSTALLED         ()                        wrapCAPI_INSTALLED
99 stdcall CAPI_MANUFACTURER      (long long long ptr long) wrapCAPI_MANUFACTURER
