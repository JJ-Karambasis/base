#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <dxc/dxcapi.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <third_party/stb/stb_image_write.h>
#include <base.h>
#include <gdi/gdi.h>
#include "utest.h"

#include "job_tests.cpp"
#include "akon_tests.cpp"
#include "gdi_tests.cpp"

UTEST_STATE();
int main(int ArgCount, const char** Args) {
	Base_Init();
	SDL_Init(SDL_INIT_VIDEO);

	return utest_main(ArgCount, Args);
}