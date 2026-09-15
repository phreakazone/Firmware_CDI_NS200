#ifndef CDI_STM32_PORT_H
#define CDI_STM32_PORT_H
#include "cdi_firmware.h"
void cdi_stm32_port_init(cdi_context_t *ctx);
void cdi_stm32_reference_isr(uint32_t timestamp_us);
void cdi_stm32_process(void);
void cdi_stm32_ble_rx(const uint8_t *data, uint16_t size);
#endif
