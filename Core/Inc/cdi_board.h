#ifndef CDI_BOARD_H
#define CDI_BOARD_H

#include "main.h"

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern IWDG_HandleTypeDef hiwdg;

void CDI_Board_Init(void);
void R5_Init(void);
void R5_OneMillisecond(void);

#endif
