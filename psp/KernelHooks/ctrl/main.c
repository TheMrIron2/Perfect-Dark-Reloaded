#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <pspctrl_kernel.h>
#include <pspsysevent.h>

#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("pspQuakeHook_Module", PSP_MODULE_KERNEL, 1, 0);

int cwbCtrlSetSamplingCycle(int cycle)
{
    int k1 = pspSdkSetK1(0);
    int ret = sceCtrlSetSamplingCycle(cycle);
    pspSdkSetK1(k1);
    return ret;
}
int cwbCtrlGetSamplingCycle(int *pcycle)
{
    int k1 = pspSdkSetK1(0);
    int ret = sceCtrlGetSamplingCycle(pcycle);
    pspSdkSetK1(k1);
    return ret;
}
int cwbCtrlSetSamplingMode(int mode)
{
    int k1 = pspSdkSetK1(0);
    int ret = sceCtrlSetSamplingMode(mode);
    pspSdkSetK1(k1);
    return ret;
}

int cwbCtrlGetSamplingMode(int *pmode)
{
    int k1 = pspSdkSetK1(0);
    int ret = sceCtrlGetSamplingMode(pmode);
    pspSdkSetK1(k1);
    return ret;
}

int cwbCtrlPeekBufferPositive(SceCtrlData *pad_data, int count)
{
	int k1 = pspSdkSetK1(0);
	sceCtrlPeekBufferPositive(pad_data, count);
    pspSdkSetK1(k1);
	return 0;
}

void cwbCtrlSetButtonMasks(unsigned int mask, unsigned type)
{
    int k1 = pspSdkSetK1(0);
	sceCtrlSetButtonMasks(mask, type);
	pspSdkSetK1(k1);
}

int cwbCtrlGetButtonMask(unsigned int mask)
{
    int k1 = pspSdkSetK1(0);
    int ret = sceCtrlGetButtonMask(mask);
    pspSdkSetK1(k1);
    return ret;
}

int cwbCtrlRegisterButtonCallback(int no, unsigned int mask, void (*cb)(int, int, void*), void *arg)
{
    int k1 = pspSdkSetK1(0);
    int ret = sceCtrlRegisterButtonCallback(no, mask, cb, arg);
    pspSdkSetK1(k1);
    return ret;
}


int module_start(SceSize args, void *argp)
{
	return 0;
}

int module_stop(SceSize args, void *argp)
{
	return 0;
}
