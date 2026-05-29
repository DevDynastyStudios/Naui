#include "test.h"
#include "test_func.h"

int main(void)
{
	filesystem_test();
	archive_test();
	math_test();
	string_test();
	iterator_test();
	//json_test();
	TEST_CONCLUSION();
}