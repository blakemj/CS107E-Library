/* 
 * File: nameless.c
 * ----------------
 *  This module is not compiled -mpoke-function-name, thus
 * these functions will not have their name stored in the
 * text section. Any functions that does not have a name
 * should be represented as "???" in the backtrace.
 */

#include "backtrace.h"
#include "nameless.h"

static int magic(int x)
{
	print_backtrace();
	return x + 2;
}

int mystery(void)
{
	return magic(1);
}
