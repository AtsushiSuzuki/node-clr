#include "node-clr.h"


#pragma managed(push, off)

extern "C" BOOL WINAPI DllMain(
	HINSTANCE hintDLL,
	DWORD fdwReason,
	LPVOID lpReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
			break;
		case DLL_PROCESS_DETACH:
			CoUninitialize();
			break;
	}
	return TRUE;
}

#pragma managed(pop)
