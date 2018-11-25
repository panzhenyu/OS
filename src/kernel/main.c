#include "print.h"

int main()
{
	put_str("I am kernel\n");
	put_str("I am kernel1\n");
	put_str("I am kernel2\n");
	put_str("I am kernel3\n");
	put_str("I am kernel4\n");
	put_str("I am kernel5\n");
	put_str("I am kernel6\n");
	put_str("I am kernel7\n");
	put_str("I am kernel8\n");
	put_str("I am kernel9\n");
	put_int32(-32767);
	put_char('\n');
	put_int32(0);
	put_char('\n');
	put_int32(32767);
	put_char('\n');
	while(1);
	return 0;
}
