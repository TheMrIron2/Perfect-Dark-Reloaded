#ifndef STAT_H
#define STAT_H

#include <windows.h>

#ifdef __cplusplus
extern "C"
{
#endif

	struct stat;

	static void sceIoMkdir(const char* d, int)
	{
		CreateDirectoryA(d, 0);
	}

#ifdef __cplusplus
}
#endif

#endif
