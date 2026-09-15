cmake_minimum_required(VERSION 3.22)
project(NS200_CDI_R9 C ASM)
set(CMAKE_C_STANDARD 11)

set(CPU_FLAGS -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
set(APP_TARGET NS200_CDI_R9_APP)
set(BOOT_TARGET NS200_CDI_R9_BOOT)
add_executable(${APP_TARGET})
set_target_properties(${APP_TARGET} PROPERTIES SUFFIX ".elf")
target_compile_options(${APP_TARGET} PRIVATE ${CPU_FLAGS} -ffunction-sections -fdata-sections -Wall -Wextra $<$<CONFIG:Debug>:-Og> $<$<CONFIG:Debug>:-g3>)
target_link_options(${APP_TARGET} PRIVATE ${CPU_FLAGS} 
    -T${CMAKE_CURRENT_SOURCE_DIR}/STM32WB55CGUX_FLASH.ld 
    -Wl,--gc-sections,-Map=${APP_TARGET}.map 
    --specs=nano.specs 
    --specs=nosys.specs
)
target_compile_definitions(${APP_TARGET} PRIVATE USE_HAL_DRIVER STM32WB55xx CDI_STM32_HAL_EXAMPLE)

set(HAL ThirdParty/Drivers/STM32WBxx_HAL_Driver)
set(WPAN ThirdParty/Middlewares/ST/STM32_WPAN)

# Header aplikasi kita sendiri
target_include_directories(${APP_TARGET} PRIVATE
 Core/Inc STM32_WPAN/App CDI/include)

# Header pustaka vendor ST ditandai SYSTEM agar compiler mengabaikan warning bawaan ST
target_include_directories(${APP_TARGET} SYSTEM PRIVATE
 ${HAL}/Inc ${HAL}/Inc/Legacy ThirdParty/Drivers/CMSIS/Include
 ThirdParty/Drivers/CMSIS/Device/ST/STM32WBxx/Include
 ThirdParty/Utilities/lpm/tiny_lpm ThirdParty/Utilities/sequencer
 ${WPAN} ${WPAN}/ble ${WPAN}/ble/core ${WPAN}/ble/core/auto ${WPAN}/ble/core/template
 ${WPAN}/ble/svc/Inc ${WPAN}/ble/svc/Src ${WPAN}/utilities
 ${WPAN}/interface/patterns/ble_thread ${WPAN}/interface/patterns/ble_thread/tl ${WPAN}/interface/patterns/ble_thread/shci)

target_sources(${APP_TARGET} PRIVATE
 Startup/startup_stm32wb55xx.s
 Core/Src/main.c Core/Src/system_stm32wbxx.c Core/Src/stm32wbxx_hal_msp.c Core/Src/stm32wbxx_it.c
 Core/Src/app_entry.c Core/Src/app_debug.c Core/Src/hw_timerserver.c Core/Src/hw_uart.c Core/Src/stm32_lpm_if.c
 Core/Src/syscalls.c Core/Src/sysmem.c Core/Src/cdi_board.c Core/Src/cdi_irq.c Core/Src/cdi_flash_store.c Core/Src/cdi_ota_flash.c
 STM32_WPAN/App/app_ble.c STM32_WPAN/App/p2p_stub.c STM32_WPAN/App/cdi_stm32wb.c STM32_WPAN/Target/hw_ipcc.c
 CDI/src/cdi_r5.c CDI/src/cdi_r8_oem_learn.c CDI/src/cdi_r8_ota.c CDI/src/cdi_r5_charger.c CDI/src/cdi_r5_protocol.c CDI/src/cdi_r5_ble.c CDI/examples/stm32wb55_r5_hal_integration.c
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

# Supresi warning unused-parameter hanya khusus pada file vendor ST
set_source_files_properties(
 ${HAL}/Src/stm32wbxx_hal_ipcc.c
 ${HAL}/Src/stm32wbxx_hal_pwr_ex.c
 ${WPAN}/utilities/stm_queue.c
 PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter"
)

add_custom_command(TARGET ${APP_TARGET} POST_BUILD
 COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${APP_TARGET}>
 COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${APP_TARGET}> ${APP_TARGET}.hex
 COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${APP_TARGET}> ${APP_TARGET}.bin)

add_executable(${BOOT_TARGET} Startup/startup_stm32wb55xx.s
 Bootloader/system_boot.c Bootloader/bootloader.c)
set_target_properties(${BOOT_TARGET} PROPERTIES SUFFIX ".elf")
target_include_directories(${BOOT_TARGET} PRIVATE CDI/include)
target_include_directories(${BOOT_TARGET} SYSTEM PRIVATE
 ThirdParty/Drivers/CMSIS/Include ThirdParty/Drivers/CMSIS/Device/ST/STM32WBxx/Include)
target_compile_definitions(${BOOT_TARGET} PRIVATE STM32WB55xx)
target_compile_options(${BOOT_TARGET} PRIVATE ${CPU_FLAGS} -ffunction-sections -fdata-sections -Wall -Wextra -Os)
target_link_options(${BOOT_TARGET} PRIVATE ${CPU_FLAGS}
    -T${CMAKE_CURRENT_SOURCE_DIR}/Bootloader/STM32WB55_BOOT.ld
    -Wl,--gc-sections,-Map=${BOOT_TARGET}.map 
    --specs=nano.specs 
    --specs=nosys.specs
)

add_custom_command(TARGET ${BOOT_TARGET} POST_BUILD
 COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${BOOT_TARGET}>
 COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${BOOT_TARGET}> ${BOOT_TARGET}.hex
 COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${BOOT_TARGET}> ${BOOT_TARGET}.bin)

find_program(PYTHON3_EXECUTABLE python REQUIRED)
add_custom_target(factory_image ALL
 COMMAND ${PYTHON3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/make_factory_image.py
   ${CMAKE_CURRENT_BINARY_DIR}/${BOOT_TARGET}.bin
   ${CMAKE_CURRENT_BINARY_DIR}/${APP_TARGET}.bin
   ${CMAKE_CURRENT_BINARY_DIR}/NS200_CDI_R9_FACTORY.bin
 COMMAND ${CMAKE_OBJCOPY} -I binary -O ihex --change-addresses 0x08000000
   ${CMAKE_CURRENT_BINARY_DIR}/NS200_CDI_R9_FACTORY.bin
   ${CMAKE_CURRENT_BINARY_DIR}/NS200_CDI_R9_FACTORY.hex
 DEPENDS ${BOOT_TARGET} ${APP_TARGET})