/*** prog3.c ***/
#include "syscall.h"

int
main()
{
	Fork_POS(1);
	Fork_POS(2);
	Fork_POS(3);
	Exit(0);
}
