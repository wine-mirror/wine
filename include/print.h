#include "windows.h"

HANDLE16 WINAPI OpenJob(LPSTR lpOutput, LPSTR lpTitle, HDC16 hDC);
int WINAPI CloseJob(HANDLE16 hJob);
int WINAPI WriteSpool(HANDLE16 hJob, LPSTR lpData, WORD cch);
int WINAPI DeleteJob(HANDLE16 hJob, WORD wNotUsed);
int WINAPI StartSpoolPage(HANDLE16 hJob);
int WINAPI EndSpoolPage(HANDLE16 hJob);
DWORD WINAPI GetSpoolJob(int nOption, LONG param);
int WINAPI WriteDialog(HANDLE16 hJob, LPSTR lpMsg, WORD cchMsg);





