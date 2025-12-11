#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <base.h>
#include <dxc/dxcapi.h>
#include <third_party/stb/stb_image_write.h>
#include <gdi/gdi.h>
#include "utest.h"

#include "job_tests.cpp"
#include "akon_tests.cpp"
#include "gdi_tests.cpp"

UTEST_STATE();
int main(int ArgCount, const char** Args) {
	Base_Init();
	int Result = utest_main(ArgCount, Args);
	return Result;
}

#ifdef OS_WIN32
#pragma comment(lib, "base.lib")
#pragma comment(lib, "dxcompiler.lib")
#endif