#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <base.h>
#include <dxc/dxcapi.h>
#include <third_party/stb/stb_image_write.h>
#include <gdi/gdi.h>
#include "utest.h"

#include "job_tests.cpp"
#include "gdi_tests.cpp"

UTEST_STATE();
int main(int ArgCount, const char** Args) {
	base* Base = Base_Init();
	GDI_Get_Tests();
	int Result = utest_main(ArgCount, Args);
	GDI_Delete_Tests();
	os_memory_stats MemoryStats = Base_Shutdown();
	Assert(MemoryStats.ReservedAmount == 0);
	Assert(MemoryStats.CommittedAmount == 0);
	Assert(MemoryStats.AllocatedFileCount == 0);
	Assert(MemoryStats.AllocatedTLSCount == 0);
	Assert(MemoryStats.AllocatedThreadCount == 0);
	Assert(MemoryStats.AllocatedMutexCount == 0);
	Assert(MemoryStats.AllocatedRWMutexCount == 0);
	Assert(MemoryStats.AllocatedSemaphoresCount == 0);
	Assert(MemoryStats.AllocatedEventsCount == 0);
	Assert(MemoryStats.AllocatedHotReloadCount == 0);
	Assert(MemoryStats.AllocatedLibraryCount == 0); 
	printf("Finished\n");
	return Result;
}

#ifdef OS_WIN32
#pragma comment(lib, "base.lib")
#pragma comment(lib, "dxcompiler.lib")
#endif