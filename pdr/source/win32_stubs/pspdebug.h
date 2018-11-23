#ifndef PSPDEBUG_H
#define PSPDEBUG_H

#include <windows.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

	static void pspDebugScreenInit(void)
	{
	}

	static void pspDebugScreenSetTextColor(int)
	{
		DebugBreak();
	}

	static void pspDebugScreenPrintData(const char* d, int)
	{
		OutputDebugStringA(d);
		printf("%s", d);
	}

	static void pspDebugScreenPrintf(const char* f, ...)
	{
		va_list		args;
		static char	buffer[1024];

		va_start(args, f);
		vsnprintf(buffer, sizeof(buffer) - 1, f, args);
		va_end(args);

		pspDebugScreenPrintData(buffer, strlen(buffer));
	}

#ifdef __cplusplus
}
#endif

#endif
