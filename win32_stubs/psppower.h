#ifndef PSPPOWER_H
#define PSPPOWER_H

#ifdef __cplusplus
extern "C"
{
#endif

#define PSP_POWER_CB_POWER_SWITCH		1
#define PSP_POWER_CB_SUSPENDING			2
#define PSP_POWER_CB_RESUMING			1
#define PSP_POWER_CB_RESUME_COMPLETE	2

	static int scePowerGetBatteryLifePercent(void)
	{
		return 50;
	}

	static int scePowerGetBatteryChargingStatus(void)
	{
		return 1;
	}

	static int scePowerIsBatteryExist(void)
	{
		return 1;
	}

	static int scePowerGetBatteryLifeTime(void)
	{
		return 60;
	}

	static int scePowerGetCpuClockFrequencyInt()
	{
		return 333;
	}

	static int scePowerGetBusClockFrequencyInt()
	{
		return scePowerGetCpuClockFrequencyInt() / 2;
	}

	static void scePowerRegisterCallback(int, int)
	{
	}

	static void scePowerSetClockFrequency(int, int, int)
	{
	}

#ifdef __cplusplus
}
#endif

#endif
