#ifndef CDI_BOARD_H
#define CDI_BOARD_H

#include "main.h"
#include "cdi_r8_ota.h"
#include <stddef.h>

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern IWDG_HandleTypeDef hiwdg;

void CDI_Board_Init(void);
void R5_Init(void);
void R5_OneMillisecond(void);
bool R5_GpioExtiCallback(uint16_t pin);
bool R8_OtaFlashErase(void *context);
bool R8_OtaFlashProgram(uint32_t offset,const uint8_t *data,size_t length,void *context);
bool R8_OtaFlashFinalize(const cdi_r8_ota_manifest_t *manifest,void *context);

#endif
