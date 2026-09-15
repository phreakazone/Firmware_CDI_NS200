#include "cdi_port.h"
#include "main.h"
#include <string.h>

/*
 * Board integration points:
 * - TIM2: free-running 1 MHz reference clock
 * - TIM1 compare: precision ignition scheduling
 * - ADC: TPS, temperature and isolated HV feedback
 * - fan output drives a transistor/MOSFET or relay driver, never the fan directly
 * - BLE RX/TX are wired to the project's custom GATT command/notify characteristics
 */
#ifndef CDI_STM32_OTA_ENABLE
#define CDI_STM32_OTA_ENABLE 0
#endif

static cdi_context_t *s_ctx;
static volatile bool s_pulse_pending;
static volatile uint32_t s_pulse_time;
static char s_line[196];
static uint16_t s_line_len;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern void CDI_BLE_Notify(const uint8_t *data, uint16_t size);
extern void CDI_BLE_NotifyTelemetry(const uint8_t *data, uint16_t size);
extern void CDI_BLE_NotifyOtaStatus(const uint8_t *data, uint16_t size);
extern bool CDI_NVM_Load(void *data, size_t size);
extern bool CDI_NVM_Save(const void *data, size_t size);
extern void CDI_SetIgnitionEnable(bool enabled);
extern void CDI_SetChargerEnable(bool enabled);
extern void CDI_SetFanEnable(bool enabled);
extern void CDI_SetStrobeEnable(bool enabled) __attribute__((weak));
extern void CDI_PulseIgnitionGate(uint16_t gate_us);
extern uint8_t CDI_ADC_LoadPercent(void);
extern uint16_t CDI_ADC_RawTps(void) __attribute__((weak));
extern uint16_t CDI_ADC_TemperatureRaw(void);
extern uint16_t CDI_ADC_HvMaximumX10(void);
#if CDI_STM32_OTA_ENABLE
extern bool CDI_Bootloader_Begin(uint32_t size, uint32_t crc32);
extern bool CDI_Bootloader_Write(uint32_t offset, const uint8_t *data, size_t size);
extern bool CDI_Bootloader_Finish(void);
extern void CDI_Bootloader_Abort(void);
#endif

static uint32_t port_micros(void) { return __HAL_TIM_GET_COUNTER(&htim2); }
static void set_ignition(bool enabled) { CDI_SetIgnitionEnable(enabled); }
static void set_charger(bool enabled) { CDI_SetChargerEnable(enabled); }
static void set_fan(bool enabled) { CDI_SetFanEnable(enabled); }
static void set_strobe(bool enabled) { if (CDI_SetStrobeEnable) CDI_SetStrobeEnable(enabled); }
static uint16_t read_raw_tps(void) {
    if (CDI_ADC_RawTps) return CDI_ADC_RawTps();
    return (uint16_t)((uint32_t)CDI_ADC_LoadPercent() * 4095u / 100u);
}
static bool load_config(void *data,size_t size){ return CDI_NVM_Load(data,size); }
static bool save_config(const void *data,size_t size){ return CDI_NVM_Save(data,size); }

#if CDI_STM32_OTA_ENABLE
static bool ota_begin(uint32_t size,uint32_t crc){ return CDI_Bootloader_Begin(size,crc); }
static bool ota_write(uint32_t off,const uint8_t *data,size_t size){ return CDI_Bootloader_Write(off,data,size); }
static bool ota_finish(void){ return CDI_Bootloader_Finish(); }
static void ota_abort(void){ CDI_Bootloader_Abort(); }
#else
static bool ota_begin(uint32_t size,uint32_t crc){ (void)size;(void)crc;return false; }
static bool ota_write(uint32_t off,const uint8_t *data,size_t size){ (void)off;(void)data;(void)size;return false; }
static bool ota_finish(void){ return false; }
static void ota_abort(void){}
#endif

void cdi_stm32_port_init(cdi_context_t *ctx) {
    cdi_hal_t hal = {
        .micros = port_micros,
        .set_ignition = set_ignition,
        .set_charger = set_charger,
        .set_fan = set_fan,
        .set_strobe = set_strobe,
        .read_raw_tps = read_raw_tps,
        .load_config = load_config,
        .save_config = save_config,
        .ota_begin = ota_begin,
        .ota_write = ota_write,
        .ota_finish = ota_finish,
        .ota_abort = ota_abort
    };
    s_ctx=ctx;
    HAL_TIM_Base_Start(&htim2);
    HAL_TIM_OC_Start_IT(&htim1,TIM_CHANNEL_1);
    cdi_init(ctx,&hal);
}

void cdi_stm32_reference_isr(uint32_t timestamp_us) {
    s_pulse_time=timestamp_us;
    s_pulse_pending=true;
}

static void schedule_fire(uint32_t delay_us) {
    uint32_t now=__HAL_TIM_GET_COUNTER(&htim1);
    __HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1,now+delay_us);
}

void cdi_stm32_process(void) {
    static uint32_t last_telemetry_us;
    static uint16_t telemetry_sequence;
    if(!s_ctx) return;
    if(s_pulse_pending) {
        __disable_irq();
        uint32_t timestamp=s_pulse_time;
        s_pulse_pending=false;
        __enable_irq();
        cdi_trigger_result_t result=cdi_on_reference_pulse(s_ctx,timestamp);
        if(result.fire) schedule_fire(result.delay_us);
    }
    cdi_set_inputs(s_ctx,CDI_ADC_LoadPercent(),CDI_ADC_TemperatureRaw(),CDI_ADC_HvMaximumX10());
    cdi_tick(s_ctx);
    uint32_t now=port_micros();
    if(now-last_telemetry_us>=50000u){
        uint8_t packet[20];last_telemetry_us=now;
        cdi_build_telemetry_packet(s_ctx,(uint8_t)(telemetry_sequence&1u),telemetry_sequence++,packet);
        CDI_BLE_NotifyTelemetry(packet,sizeof(packet));
    }
}

void cdi_stm32_ota_data_rx(const uint8_t *packet,uint16_t size){
    if(!s_ctx||!packet||size<8u)return;
    uint32_t offset=(uint32_t)packet[0]|((uint32_t)packet[1]<<8)|((uint32_t)packet[2]<<16)|((uint32_t)packet[3]<<24);
    uint8_t count=packet[4];
    if((uint16_t)(count+7u)!=size)return;
    uint16_t supplied=(uint16_t)packet[size-2u]|((uint16_t)packet[size-1u]<<8);
    if(cdi_crc16(packet,size-2u)!=supplied)return;
    cdi_ota_data(s_ctx,offset,packet+5u,count);
    uint8_t status[16];cdi_build_ota_status(s_ctx,status);
    CDI_BLE_NotifyOtaStatus(status,sizeof(status));
}

void cdi_stm32_ble_rx(const uint8_t *data,uint16_t size) {
    if(!s_ctx||!data) return;
    for(uint16_t i=0;i<size;++i) {
        char ch=(char)data[i];
        if(ch=='\n'||ch=='\r') {
            if(s_line_len) {
                char reply[220];
                s_line[s_line_len]='\0';
                size_t n=cdi_protocol_exchange(s_ctx,s_line,reply,sizeof(reply));
                if(n) CDI_BLE_Notify((const uint8_t*)reply,(uint16_t)n);
                s_line_len=0;
            }
        } else if(s_line_len+1u<sizeof(s_line)) s_line[s_line_len++]=ch;
        else s_line_len=0;
    }
}

/* Call from HAL_TIM_OC_DelayElapsedCallback for TIM1 channel 1. */
void cdi_stm32_fire_compare_callback(void) {
    if(!s_ctx||s_ctx->telemetry.ota_active) return;
    uint16_t gate = s_ctx->config.gate_us ? s_ctx->config.gate_us : 80u;
    CDI_PulseIgnitionGate(gate);
}
