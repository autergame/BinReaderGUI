#include "Myassert.h"

void my_assert(const char* exp, const char* func, const char* file, unsigned int line)
{
#ifdef _DEBUG
    __debugbreak();
#endif
    fprintf(stderr, "ERROR: %s %s\n%s %d\n", func, exp, file, line);
    scanf_s("Press enter to exit.");
    _Exit(EXIT_FAILURE);
}