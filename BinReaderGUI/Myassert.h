#ifndef _MYASSERT_H_
#define _MYASSERT_H_

#include <stdio.h>
#include <stdlib.h>

void my_assert(const char* exp, const char* func, const char* file, unsigned int line);
#define myassert(expression) if (expression) { my_assert(#expression, __FUNCTION__, __FILE__, __LINE__); }

#endif //_MYASSERT_H_