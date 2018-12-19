#ifndef PSPGU_H
#define PSPGU_H

#ifdef __cplusplus
extern "C"
{
#endif

#define PSP_GU	2

	typedef unsigned char	u8;
	typedef unsigned int	ScePspRGBA8888;

	static void sceGu(void)
	{
	}

	static void* sceGeEdramGetAddr(void)
	{
		return 0;
	}

#ifdef __cplusplus
}
#endif

#endif
