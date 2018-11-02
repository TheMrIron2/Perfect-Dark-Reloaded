#ifndef PSPCTRL_H
#define PSPCTRL_H

#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define PSP_CTRL_MODE_ANALOG 0

#define PSP_CTRL_CROSS		0
#define PSP_CTRL_CIRCLE		1
#define PSP_CTRL_SQUARE		2
#define PSP_CTRL_TRIANGLE	3
#define PSP_CTRL_SELECT		0
#define PSP_CTRL_START		1
#define PSP_CTRL_UP			0
#define PSP_CTRL_DOWN		1
#define PSP_CTRL_LEFT		0
#define PSP_CTRL_RIGHT		1
#define PSP_CTRL_LTRIGGER	0
#define PSP_CTRL_RTRIGGER	1

	typedef struct SceCtrlDataS
	{
		int Buttons;
		int Lx;
		int Ly;
		int TimeStamp;
	} SceCtrlData;

	static void sceCtrlSetSamplingCycle(int)
	{
	}

	static void sceCtrlSetSamplingMode(int)
	{
	}

	static void sceCtrlPeekBufferPositive(SceCtrlData* pad, int)
	{
		memset(pad, 0, sizeof(*pad));
	}
	
	static void sceCtrlReadBufferPositive(SceCtrlData* pad, int i)
	{
		sceCtrlPeekBufferPositive(pad, i);
	}

#ifdef __cplusplus
}
#endif

#endif
