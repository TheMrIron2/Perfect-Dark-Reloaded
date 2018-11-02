#ifndef UNISTD_H
#define UNISTD_H

#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

	static void getcwd(char* dir, size_t size)
	{
		strncpy(dir, ".", size);
	}

#ifdef __cplusplus
}
#endif

#endif
