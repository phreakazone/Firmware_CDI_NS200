#include "stm32wb55xx.h"
#include "cdi_r8_ota.h"

uint32_t SystemCoreClock = 4000000u;
const uint32_t AHBPrescTable[16]={0,0,0,0,0,0,0,0,1,2,3,4,6,7,8,9};
const uint32_t APBPrescTable[8]={0,0,0,0,1,2,3,4};

void SystemInit(void)
{
  SCB->VTOR=CDI_R8_BOOT_ADDR;
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
  SCB->CPACR|=(3UL<<(10UL*2U))|(3UL<<(11UL*2U));
#endif
}

void SystemCoreClockUpdate(void){SystemCoreClock=4000000u;}
