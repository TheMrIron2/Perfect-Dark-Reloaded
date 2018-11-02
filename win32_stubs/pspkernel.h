#ifndef PSPKERNEL_H
#define PSPKERNEL_H

#include <windows.h>

#ifdef __cplusplus
extern "C"
{
#endif

	typedef int SceSize;
	typedef int SceUID;

	typedef int K_CALLBACK(int, int, void*);
	typedef int K_THREAD_CALLBACK(SceSize, void*);

	typedef int PspDebugRegBlock;

	static void sceKernelDelayThread(int)
	{
	}

	static SceUID sceKernelCreateCallback(const char*, K_CALLBACK, void*)
	{
		return 0;
	}

	static void sceKernelRegisterExitCallback(SceUID)
	{
	}

	static void sceKernelSleepThreadCB()
	{
	}
	
	static int sceKernelCreateThread(const char*, K_THREAD_CALLBACK*, int, int, int, int)
	{
		return 1;
	}

	static void sceKernelStartThread(int, int, int)
	{
	}

	static void pspDebugDumpException(PspDebugRegBlock*)
	{
	}

	static void pspDebugInstallErrorHandler(void (*) (PspDebugRegBlock*))
	{
	}

	static void sceKernelExitGame(void)
	{
		DebugBreak();
	}

#ifdef __cplusplus
}
#endif

#endif
