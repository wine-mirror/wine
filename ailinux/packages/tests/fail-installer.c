#include <windows.h>
#include <stdio.h>
int main(void) {
    HANDLE handle=CreateFileW(L"C:\\ailinux-intentional-failure.txt",GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(handle==INVALID_HANDLE_VALUE) return 41;
    const char payload[]="FAIL_INJECTED\n";
    DWORD written=0;
    WriteFile(handle,payload,sizeof(payload)-1,&written,NULL);
    CloseHandle(handle);
    return 23;
}
