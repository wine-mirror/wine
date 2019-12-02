#include <windows.h>

static HINSTANCE instance;

BOOL WINAPI DllMain(HINSTANCE instance_new, DWORD reason, LPVOID reserved)
{
     switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            instance = instance_new;
            break;
    }

    return TRUE;
}

void WINAPI get_path(char *buffer, int buffer_size)
{
    GetModuleFileNameA(instance, buffer, buffer_size);
}
