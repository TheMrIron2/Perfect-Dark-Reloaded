	.set noreorder

#include "pspstub.s"

	STUB_START "cwbhook_extender_driver",0x00090000,0x00080005
	STUB_FUNC  0xC1352C0B,cwbCtrlSetSamplingCycle
	STUB_FUNC  0x358CE507,cwbCtrlGetSamplingCycle
	STUB_FUNC  0x4EC00388,cwbCtrlSetSamplingMode
	STUB_FUNC  0xF295AA17,cwbCtrlGetSamplingMode
	STUB_FUNC  0x002E688E,cwbCtrlPeekBufferPositive
	STUB_FUNC  0x2C86FB88,cwbCtrlSetButtonMasks
	STUB_FUNC  0xC1966010,cwbCtrlGetButtonMask
	STUB_FUNC  0xE90B71E2,cwbCtrlRegisterButtonCallback
	STUB_END
