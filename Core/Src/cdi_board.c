#include "cdi_board.h"

ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
IWDG_HandleTypeDef hiwdg;
static DMA_HandleTypeDef hdma_adc1;

static void gpio_init(void)
{
  GPIO_InitTypeDef g = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_GPIOB_CLK_ENABLE(); __HAL_RCC_GPIOE_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5 | GPIO_PIN_9, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);
  g.Pin=GPIO_PIN_1|GPIO_PIN_2; g.Mode=GPIO_MODE_OUTPUT_PP; g.Pull=GPIO_NOPULL; g.Speed=GPIO_SPEED_FREQ_LOW; HAL_GPIO_Init(GPIOA,&g);
  g.Pin=GPIO_PIN_5 | GPIO_PIN_9; HAL_GPIO_Init(GPIOB,&g);
  g.Pin=GPIO_PIN_4; HAL_GPIO_Init(GPIOE,&g);
  /* PB2 no longer gates HV. PB3/PB4 are conditioned, passive OEM-fire taps. */
  g.Pin=GPIO_PIN_2; g.Mode=GPIO_MODE_INPUT; g.Pull=GPIO_PULLDOWN; HAL_GPIO_Init(GPIOB,&g);
  g.Pin=GPIO_PIN_3|GPIO_PIN_4; g.Mode=GPIO_MODE_IT_RISING; g.Pull=GPIO_PULLDOWN; HAL_GPIO_Init(GPIOB,&g);
  HAL_NVIC_SetPriority(EXTI3_IRQn,1,1); HAL_NVIC_EnableIRQ(EXTI3_IRQn);
  HAL_NVIC_SetPriority(EXTI4_IRQn,1,1); HAL_NVIC_EnableIRQ(EXTI4_IRQn);
  g.Pin=GPIO_PIN_10; g.Pull=GPIO_PULLUP; HAL_GPIO_Init(GPIOA,&g);
  g.Pin=GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7; g.Mode=GPIO_MODE_ANALOG; g.Pull=GPIO_NOPULL; HAL_GPIO_Init(GPIOA,&g);
  g.Pin=GPIO_PIN_0; HAL_GPIO_Init(GPIOB,&g);
  g.Pin=GPIO_PIN_0; g.Mode=GPIO_MODE_AF_PP; g.Pull=GPIO_NOPULL; g.Speed=GPIO_SPEED_FREQ_HIGH; g.Alternate=GPIO_AF1_TIM2; HAL_GPIO_Init(GPIOA,&g);
  g.Pin=GPIO_PIN_9; g.Mode=GPIO_MODE_AF_PP; g.Alternate=GPIO_AF1_TIM1; HAL_GPIO_Init(GPIOA,&g);
  g.Pin=GPIO_PIN_8; g.Alternate=GPIO_AF1_TIM1; HAL_GPIO_Init(GPIOB,&g);
}

static void adc_init(void)
{
  ADC_ChannelConfTypeDef c={0};
  const uint32_t channels[6]={ADC_CHANNEL_8,ADC_CHANNEL_9,ADC_CHANNEL_11,ADC_CHANNEL_12,ADC_CHANNEL_15,ADC_CHANNEL_10};
  __HAL_RCC_ADC_CLK_ENABLE(); __HAL_RCC_DMA1_CLK_ENABLE();
  hdma_adc1.Instance=DMA1_Channel1; hdma_adc1.Init.Request=DMA_REQUEST_ADC1;
  hdma_adc1.Init.Direction=DMA_PERIPH_TO_MEMORY; hdma_adc1.Init.PeriphInc=DMA_PINC_DISABLE;
  hdma_adc1.Init.MemInc=DMA_MINC_ENABLE; hdma_adc1.Init.PeriphDataAlignment=DMA_PDATAALIGN_HALFWORD;
  hdma_adc1.Init.MemDataAlignment=DMA_MDATAALIGN_HALFWORD; hdma_adc1.Init.Mode=DMA_CIRCULAR;
  hdma_adc1.Init.Priority=DMA_PRIORITY_HIGH; if(HAL_DMA_Init(&hdma_adc1)!=HAL_OK) Error_Handler();
  hadc1.Instance=ADC1; hadc1.Init.ClockPrescaler=ADC_CLOCK_ASYNC_DIV4; hadc1.Init.Resolution=ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign=ADC_DATAALIGN_RIGHT; hadc1.Init.ScanConvMode=ADC_SCAN_ENABLE; hadc1.Init.EOCSelection=ADC_EOC_SEQ_CONV;
  hadc1.Init.LowPowerAutoWait=DISABLE; hadc1.Init.ContinuousConvMode=ENABLE; hadc1.Init.NbrOfConversion=6;
  hadc1.Init.DiscontinuousConvMode=DISABLE; hadc1.Init.ExternalTrigConv=ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge=ADC_EXTERNALTRIGCONVEDGE_NONE; hadc1.Init.DMAContinuousRequests=ENABLE;
  hadc1.Init.Overrun=ADC_OVR_DATA_OVERWRITTEN; hadc1.Init.OversamplingMode=DISABLE;
  __HAL_LINKDMA(&hadc1,DMA_Handle,hdma_adc1); if(HAL_ADC_Init(&hadc1)!=HAL_OK) Error_Handler();
  c.SamplingTime=ADC_SAMPLETIME_247CYCLES_5; c.SingleDiff=ADC_SINGLE_ENDED; c.OffsetNumber=ADC_OFFSET_NONE; c.Offset=0;
  for(uint32_t i=0;i<6;i++){ c.Channel=channels[i]; c.Rank=ADC_REGULAR_RANK_1+i; if(HAL_ADC_ConfigChannel(&hadc1,&c)!=HAL_OK) Error_Handler(); }
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn,3,0); HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}

static void timers_init(void)
{
  TIM_IC_InitTypeDef ic={0}; TIM_OC_InitTypeDef oc={0}; TIM_MasterConfigTypeDef master={0}; TIM_BreakDeadTimeConfigTypeDef bd={0};
  __HAL_RCC_TIM1_CLK_ENABLE(); __HAL_RCC_TIM2_CLK_ENABLE();
  htim2.Instance=TIM2; htim2.Init.Prescaler=7; htim2.Init.CounterMode=TIM_COUNTERMODE_UP;
  htim2.Init.Period=0xffffffffu; htim2.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1; htim2.Init.AutoReloadPreload=TIM_AUTORELOAD_PRELOAD_DISABLE;
  if(HAL_TIM_IC_Init(&htim2)!=HAL_OK || HAL_TIM_OC_Init(&htim2)!=HAL_OK) Error_Handler();
  ic.ICPolarity=TIM_INPUTCHANNELPOLARITY_FALLING; ic.ICSelection=TIM_ICSELECTION_DIRECTTI; ic.ICPrescaler=TIM_ICPSC_DIV1; ic.ICFilter=4;
  if(HAL_TIM_IC_ConfigChannel(&htim2,&ic,TIM_CHANNEL_1)!=HAL_OK) Error_Handler();
  oc.OCMode=TIM_OCMODE_TIMING; oc.Pulse=0; oc.OCPolarity=TIM_OCPOLARITY_HIGH; oc.OCFastMode=TIM_OCFAST_DISABLE;
  if(HAL_TIM_OC_ConfigChannel(&htim2,&oc,TIM_CHANNEL_2)!=HAL_OK || HAL_TIM_OC_ConfigChannel(&htim2,&oc,TIM_CHANNEL_3)!=HAL_OK || HAL_TIM_OC_ConfigChannel(&htim2,&oc,TIM_CHANNEL_4)!=HAL_OK) Error_Handler();
  HAL_NVIC_SetPriority(TIM2_IRQn,1,0); HAL_NVIC_EnableIRQ(TIM2_IRQn);

  htim1.Instance=TIM1; htim1.Init.Prescaler=0; htim1.Init.CounterMode=TIM_COUNTERMODE_UP;
  htim1.Init.Period=319; htim1.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1; htim1.Init.RepetitionCounter=0;
  htim1.Init.AutoReloadPreload=TIM_AUTORELOAD_PRELOAD_DISABLE; if(HAL_TIM_PWM_Init(&htim1)!=HAL_OK) Error_Handler();
  master.MasterOutputTrigger=TIM_TRGO_RESET; master.MasterOutputTrigger2=TIM_TRGO2_RESET; master.MasterSlaveMode=TIM_MASTERSLAVEMODE_DISABLE;
  HAL_TIMEx_MasterConfigSynchronization(&htim1,&master);
  oc.OCMode=TIM_OCMODE_PWM1; oc.Pulse=0; oc.OCPolarity=TIM_OCPOLARITY_HIGH; oc.OCNPolarity=TIM_OCNPOLARITY_HIGH;
  oc.OCFastMode=TIM_OCFAST_DISABLE; oc.OCIdleState=TIM_OCIDLESTATE_RESET; oc.OCNIdleState=TIM_OCNIDLESTATE_RESET;
  if(HAL_TIM_PWM_ConfigChannel(&htim1,&oc,TIM_CHANNEL_2)!=HAL_OK) Error_Handler();
  bd.OffStateRunMode=TIM_OSSR_DISABLE; bd.OffStateIDLEMode=TIM_OSSI_DISABLE; bd.LockLevel=TIM_LOCKLEVEL_OFF;
  bd.DeadTime=26; bd.BreakState=TIM_BREAK_DISABLE; bd.BreakPolarity=TIM_BREAKPOLARITY_HIGH; bd.BreakFilter=0;
  bd.Break2State=TIM_BREAK2_DISABLE; bd.Break2Polarity=TIM_BREAK2POLARITY_HIGH; bd.Break2Filter=0; bd.AutomaticOutput=TIM_AUTOMATICOUTPUT_DISABLE;
  if(HAL_TIMEx_ConfigBreakDeadTime(&htim1,&bd)!=HAL_OK) Error_Handler();
}

void CDI_Board_Init(void)
{
  gpio_init(); adc_init(); timers_init();
  hiwdg.Instance=IWDG; hiwdg.Init.Prescaler=IWDG_PRESCALER_64; hiwdg.Init.Reload=1000; hiwdg.Init.Window=IWDG_WINDOW_DISABLE;
  if(HAL_IWDG_Init(&hiwdg)!=HAL_OK) Error_Handler();
}
