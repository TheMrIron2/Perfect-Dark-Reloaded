#ifndef PSPAUDIOLIB_H
#define PSPAUDIOLIB_H

#ifdef __cplusplus
extern "C"
{
#endif

	typedef void (CHANNEL_CALLBACK) (void*, unsigned int, void*);

	static void pspAudioInit(void)
	{
	}

	static void pspAudioEndPre(void)
	{
	}

	static void pspAudioEnd(void)
	{
	}

	static void pspAudioSetChannelCallback(int, CHANNEL_CALLBACK*, int)
	{
	}

#ifdef __cplusplus
}
#endif

#endif
