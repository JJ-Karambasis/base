#include <base.h>
#include <base.c>
#include <utest.h>

#include "job_tests.c"

UTEST_STATE();
int main(int ArgCount, const char** Args) {
	Base_Init();

	return utest_main(ArgCount, Args);
}