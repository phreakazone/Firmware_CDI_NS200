cmake_minimum_required(VERSION 3.22)
project(NS200_CDI_R7 C ASM)
set(CMAKE_C_STANDARD 11)

set(CPU_FLAGS -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
add_executable(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")
target_compile_options(${PROJECT_NAME} PRIVATE ${CPU_FLAGS} -ffunction-sections -fdata-sections -Wall -Wextra $<$<CONFIG:Debug>:-Og> $<$<CONFIG:Debug>:-g3>)
target_link_options(${PROJECT_NAME} PRIVATE ${CPU_FLAGS} -T${CMAKE_CURRENT_SOURCE_DIR}/STM32WB55CGUX_FLASH.ld -Wl,--gc-sections,-Map=${PROJECT_NAME}.map --specs=nano.specs --specs=nosys.specs)
target_compile_definitions(${PROJECT_NAME} PRIVATE USE_HAL_DRIVER STM32WB55xx CDI_STM32_HAL_EXAMPLE)

set(HAL ThirdParty/Drivers/STM32WBxx_HAL_Driver)
set(WPAN ThirdParty/Middlewares/ST/STM32_WPAN)
target_include_directories(${PROJECT_NAME} PRIVATE
 Core/Inc STM32_WPAN/App CDI/include
 ${HAL}/Inc ${HAL}/Inc/Legacy ThirdParty/Drivers/CMSIS/Include
 ThirdParty/Drivers/CMSIS/Device/ST/STM32WBxx/Include
 ThirdParty/Utilities/lpm/tiny_lpm ThirdParty/Utilities/sequencer
 ${WPAN} ${WPAN}/ble ${WPAN}/ble/core ${WPAN}/ble/core/auto ${WPAN}/ble/core/template
 ${WPAN}/ble/svc/Inc ${WPAN}/ble/svc/Src ${WPAN}/utilities
 ${WPAN}/interface/patterns/ble_thread ${WPAN}/interface/patterns/ble_thread/tl ${WPAN}/interface/patterns/ble_thread/shci)

target_sources(${PROJECT_NAME} PRIVATE
 Startup/startup_stm32wb55xx.s
 Core/Src/main.c Core/Src/system_stm32wbxx.c Core/Src/stm32wbxx_hal_msp.c Core/Src/stm32wbxx_it.c
 Core/Src/app_entry.c Core/Src/app_debug.c Core/Src/hw_timerserver.c Core/Src/hw_uart.c Core/Src/stm32_lpm_if.c
 Core/Src/syscalls.c Core/Src/sysmem.c Core/Src/cdi_board.c Core/Src/cdi_irq.c Core/Src/cdi_flash_store.c
 STM32_WPAN/App/app_ble.c STM32_WPAN/App/p2p_stub.c STM32_WPAN/App/cdi_stm32wb.c STM32_WPAN/Target/hw_ipcc.c
 CDI/src/cdi_r5.c CDI/src/cdi_r5_charger.c CDI/src/cdi_r5_protocol.c CDI/src/cdi_r5_ble.c CDI/examples/stm32wb55_r5_hal_integration.c
 ${HAL}/Src/stm32wbxx_hal.c ${HAL}/Src/stm32wbxx_hal_cortex.c ${HAL}/Src/stm32wbxx_hal_dma.c ${HAL}/Src/stm32wbxx_hal_dma_ex.c
 ${HAL}/Src/stm32wbxx_hal_exti.c ${HAL}/Src/stm32wbxx_hal_flash.c ${HAL}/Src/stm32wbxx_hal_flash_ex.c ${HAL}/Src/stm32wbxx_hal_gpio.c
 ${HAL}/Src/stm32wbxx_hal_hsem.c ${HAL}/Src/stm32wbxx_hal_ipcc.c ${HAL}/Src/stm32wbxx_hal_iwdg.c ${HAL}/Src/stm32wbxx_hal_pwr.c
 ${HAL}/Src/stm32wbxx_hal_pwr_ex.c ${HAL}/Src/stm32wbxx_hal_rcc.c ${HAL}/Src/stm32wbxx_hal_rcc_ex.c ${HAL}/Src/stm32wbxx_hal_rng.c
 ${HAL}/Src/stm32wbxx_hal_rtc.c ${HAL}/Src/stm32wbxx_hal_rtc_ex.c ${HAL}/Src/stm32wbxx_hal_tim.c ${HAL}/Src/stm32wbxx_hal_tim_ex.c
 ${HAL}/Src/stm32wbxx_hal_uart.c ${HAL}/Src/stm32wbxx_hal_uart_ex.c ${HAL}/Src/stm32wbxx_hal_adc.c ${HAL}/Src/stm32wbxx_hal_adc_ex.c
 ${HAL}/Src/stm32wbxx_ll_rcc.c
 ThirdParty/Utilities/lpm/tiny_lpm/stm32_lpm.c ThirdParty/Utilities/sequencer/stm32_seq.c
 ${WPAN}/interface/patterns/ble_thread/tl/tl_mbox.c ${WPAN}/interface/patterns/ble_thread/shci/shci.c
 ${WPAN}/interface/patterns/ble_thread/tl/hci_tl.c ${WPAN}/interface/patterns/ble_thread/tl/hci_tl_if.c
 ${WPAN}/interface/patterns/ble_thread/tl/shci_tl.c ${WPAN}/interface/patterns/ble_thread/tl/shci_tl_if.c
 ${WPAN}/utilities/dbg_trace.c ${WPAN}/utilities/otp.c ${WPAN}/utilities/stm_list.c ${WPAN}/utilities/stm_queue.c
 ${WPAN}/ble/core/template/osal.c ${WPAN}/ble/core/auto/ble_gap_aci.c ${WPAN}/ble/core/auto/ble_gatt_aci.c
 ${WPAN}/ble/core/auto/ble_hal_aci.c ${WPAN}/ble/core/auto/ble_hci_le.c ${WPAN}/ble/core/auto/ble_l2cap_aci.c
 ${WPAN}/ble/svc/Src/svc_ctl.c)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
 COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
 COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
 COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin)
