#include "Drv_UserTimer.h"
#include "Drv_Timer.h"

void Drv_Timer_Init(void)
{
}

TimerHandle Drv_Timer_Create(TimerNo timerNo,
	DrvTimerPriority priority,
	DrvTimerCallback timerCallback)
{
	return DRV_TIMER_INVALID_HANDLE;
}

void Drv_Timer_Release(TimerHandle timer)
{

}

void Drv_Timer_Start(TimerHandle timerHandle, uint32_t timeoutInUs)
{

}

uint32_t Drv_Timer_ReadElapsedTimeInUs(TimerHandle timerHandle)
{
	return 0;
}
