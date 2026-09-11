#include "cdi_board.h"
void TIM2_IRQHandler(void){ HAL_TIM_IRQHandler(&htim2); }
/* DMA ADC berjalan circular; flag dibersihkan HAL melalui handle yang diekspor. */
void DMA1_Channel1_IRQHandler(void){ HAL_DMA_IRQHandler(hadc1.DMA_Handle); }
void EXTI3_IRQHandler(void){ HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3); }
