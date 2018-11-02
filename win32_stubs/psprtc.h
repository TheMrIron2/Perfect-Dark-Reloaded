#ifndef PSPRTC_H
#define PSPRTC_H

#include <windows.h>

#ifdef __cplusplus
extern "C"
{
#endif

	typedef UINT64 u64;

	static int sceRtcGetTickResolution(void)
	{
		return 1000;
	}

	static void sceRtcGetCurrentTick(u64* u)
	{
		*u = GetTickCount();
	}

#ifdef __cplusplus
}
#endif

#endif
