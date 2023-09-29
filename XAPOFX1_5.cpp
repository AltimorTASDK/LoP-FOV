#include <Windows.h>

extern "C" {
FARPROC orig_CreateFX;
}

static class functions {
public:
	functions()
	{
		const auto mod = LoadLibraryEx("XAPOFX1_5.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		orig_CreateFX = GetProcAddress(mod, "CreateFX");
	}
} functions;
