/* Integrasi R7 ke STM32Cube/VS Code.
 * File ini sengaja dijaga oleh macro agar host test tidak memerlukan HAL.
 * Konfigurasi peripheral wajib mengikuti README_R7_CUBEIDE.md.
 */
#ifdef CDI_STM32_HAL_EXAMPLE
#include "main.h"
#include "cdi_board.h"
#include "cdi_r5.h"
#include "cdi_r5_charger.h"
#include "cdi_r5_protocol.h"
#include "cdi_r5_ble.h"
#include "cdi_r8_oem_learn.h"
#include <string.h>

extern TIM_HandleTypeDef htim1; /* CH2 PA9 + CH2N PB8, charger */
extern TIM_HandleTypeDef htim2; /* CH1 pickup, CH2/CH3 compare spark */
extern ADC_HandleTypeDef hadc1;

static cdi_r5_store_image_t store;
static cdi_r5_protocol_t protocol;
static cdi_r5_charger_t charger;
static cdi_r8_oem_learner_t oem_learner;
static cdi_r8_ota_t ota;
static cdi_r5_engine_config_t engine = {
    .timer_hz=4000000u, .pulses_per_revolution=1u, .gate_pulse_us=80u
};
static uint32_t last_pickup;
static uint32_t last_pickup_ms;
static uint8_t soft_phase;
static bool battery_ok_state;
static bool center_high, side_high;
static uint32_t center_width, side_width;
static bool strobe_high;
static uint32_t strobe_width;
static uint16_t adc_dma[6]; /* PA3 TPS, PA4 TEMP, PA6 HVC, PA7 HVS, PB0 VBAT, PA5 REF */
static int16_t last_advance_cdeg;
static uint8_t last_limiter_state;
static uint16_t telemetry_sequence, applied_edge=0xffffu;
static uint32_t first_start_good_ms;
static bool first_start_proof_written;
static uint16_t ota_reboot_ms;

/* Ganti dengan dua halaman flash yang aman. Jangan menulis flash saat mesin hidup. */
__attribute__((weak)) bool R5_FlashLoad(cdi_r5_store_image_t *image) { (void)image; return false; }
__attribute__((weak)) bool R5_FlashSave(const cdi_r5_store_image_t *image) { (void)image; return false; }
__attribute__((weak)) bool R8_FirstStartProofLoad(void) { return false; }
__attribute__((weak)) bool R8_FirstStartProofWrite(void) { return false; }
__attribute__((weak)) bool R8_FirstStartProofClear(void) { return false; }
__attribute__((weak)) bool R5_BleIsConnected(void) { return false; }
__attribute__((weak)) void R5_BleNotifyTelemetry(const uint8_t *data, uint16_t length)
{ (void)data; (void)length; }
__attribute__((weak)) void R5_BleNotifyResponse(const uint8_t *data, uint16_t length)
{ (void)data; (void)length; }
__attribute__((weak)) void R8_BleNotifyOtaStatus(const uint8_t *data,uint16_t length)
{(void)data;(void)length;}

static bool persist_maps(const cdi_r5_store_image_t *image, void *context)
{
    (void)context;
    return R5_FlashSave(image);
}

static void force_safe(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_RESET);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_2);
    center_high = side_high = false;
}

static void sync_setup(void)
{
    const cdi_r7_setup_t *s=&store.setup;
    engine.trigger_angle_cdeg=s->trigger_angle_cdeg; engine.side_offset_cdeg=s->side_offset_cdeg;
    engine.pulses_per_revolution=s->pulses_per_revolution; engine.gate_pulse_us=s->gate_pulse_us;
    engine.calibrated=s->stage>=CDI_R7_STAGE_TDC_SAVED;
    engine.center_enabled=s->center_enabled!=0; engine.side_enabled=s->side_enabled!=0;
    engine.rpm_limit_override=0; engine.advance_cap_cdeg=0; engine.hv_target_override=0;
    if(s->stage==CDI_R7_STAGE_FIRST_START){engine.rpm_limit_override=s->first_start_rpm_limit;engine.advance_cap_cdeg=s->first_start_advance_cap_cdeg;engine.hv_target_override=s->first_start_hv_volts;}
    if(applied_edge!=s->pickup_edge && protocol.rpm==0u){HAL_TIM_IC_Stop_IT(&htim2,TIM_CHANNEL_1);__HAL_TIM_SET_CAPTUREPOLARITY(&htim2,TIM_CHANNEL_1,s->pickup_edge==CDI_R7_EDGE_RISING?TIM_INPUTCHANNELPOLARITY_RISING:TIM_INPUTCHANNELPOLARITY_FALLING);HAL_TIM_IC_Start_IT(&htim2,TIM_CHANNEL_1);applied_edge=s->pickup_edge;}
}

void R5_Init(void)
{
    force_safe();
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_9,GPIO_PIN_RESET);
    if (!R5_FlashLoad(&store) || cdi_r5_store_validate(&store) != CDI_R5_OK){cdi_r5_load_defaults(&store);(void)R5_FlashSave(&store);}
    if(store.setup.stage==CDI_R7_STAGE_FIRST_START && R8_FirstStartProofLoad()){
        store.setup.first_start_proven=1u; store.setup.stage=CDI_R7_STAGE_READY;
        store.setup.center_enabled=1u;
        store.setup.side_enabled=store.oem_profile.valid&&store.oem_profile.side_samples>=10u;
        cdi_r5_store_seal(&store); (void)R5_FlashSave(&store); (void)R8_FirstStartProofClear();
    } else if(store.setup.stage!=CDI_R7_STAGE_FIRST_START) (void)R8_FirstStartProofClear();
    cdi_r5_protocol_init(&protocol, &store);
    cdi_r5_protocol_set_persist(&protocol, persist_maps, NULL);
    cdi_r8_oem_learn_init(&oem_learner,engine.timer_hz);
    cdi_r8_protocol_attach_oem_learner(&protocol,&oem_learner);
    cdi_r8_ota_init(&ota,R8_OtaFlashErase,R8_OtaFlashProgram,
                    R8_OtaFlashFinalize,NULL);
    cdi_r8_protocol_attach_ota(&protocol,&ota);
    cdi_r5_charger_init(&charger);
    sync_setup();
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma, 6u);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    cdi_r5_decision_t d;
    cdi_r5_status_t decision_status;
    uint32_t now, period;
    if (htim->Instance != TIM2 || htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1) return;
    now = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
    period = now - last_pickup;
    if(last_pickup!=0u&&period>10000u&&period<3000000u){static uint32_t prev;if(prev&&period>prev*65u/100u&&period<prev*135u/100u){if(protocol.pickup_quality<100u)++protocol.pickup_quality;}else if(protocol.pickup_quality)--protocol.pickup_quality;prev=period;}
    last_pickup = now;
    last_pickup_ms = HAL_GetTick();
    sync_setup(); engine.trigger_angle_cdeg=protocol.setup_trigger_cdeg;
    engine.output_permission=!protocol.strobe_active&&engine.center_enabled&&
        store.setup.operating_mode==CDI_R8_OP_DIY&&
        store.setup.diy_oem_unplug_confirmed&&battery_ok_state&&
        !protocol.firmware_update_active;
    engine.pro_enabled=store.setup.pro_enabled!=0u;
    protocol.output_permission=engine.output_permission;
    protocol.pro_enabled=engine.pro_enabled;
    if(store.setup.operating_mode==CDI_R8_OP_OEM_LEARN){
        if(last_pickup!=0u&&period>10000u&&period<3000000u){
            protocol.rpm=(uint32_t)(((uint64_t)engine.timer_hz*60u)/
                ((uint64_t)period*engine.pulses_per_revolution));
            cdi_r8_oem_learn_pickup(&oem_learner,now,period,
                protocol.tps_permille,&protocol.working);
        }
        force_safe(); return;
    }
    if(protocol.strobe_active&&last_pickup!=0u&&period>10000u&&period<3000000u){uint32_t delay=(uint32_t)(((uint64_t)period*engine.pulses_per_revolution*protocol.setup_trigger_cdeg)/36000u);strobe_high=false;strobe_width=engine.timer_hz/2500u;__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_4,now+delay);HAL_TIM_OC_Start_IT(&htim2,TIM_CHANNEL_4);if(protocol.strobe_samples<65535u)++protocol.strobe_samples;force_safe();return;}
    decision_status = cdi_r5_make_decision(&engine, &protocol.working, period,
                                           protocol.tps_permille, &soft_phase, &d);
    if (decision_status == CDI_R5_OK) {
        protocol.rpm = d.rpm;
        last_advance_cdeg = d.advance_cdeg;
        last_limiter_state = (uint8_t)d.action;
    }
    if (decision_status != CDI_R5_OK || d.action != CDI_R5_SPARK_FIRE ||
        charger.fault_latched) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_RESET);
        return;
    }
    center_high = side_high = false;
    center_width = side_width = d.gate_width_ticks;
    if(engine.center_enabled){__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_2,now+d.center_delay_ticks);HAL_TIM_OC_Start_IT(&htim2,TIM_CHANNEL_2);}
    if(engine.side_enabled){__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_3,now+d.side_delay_ticks);HAL_TIM_OC_Start_IT(&htim2,TIM_CHANNEL_3);}
}

bool R5_GpioExtiCallback(uint16_t pin)
{
    if(store.setup.operating_mode!=CDI_R8_OP_OEM_LEARN ||
       oem_learner.state!=CDI_R8_LEARN_ACTIVE) return false;
    if(pin==GPIO_PIN_3){
        cdi_r8_oem_learn_center_fire(&oem_learner,
            __HAL_TIM_GET_COUNTER(&htim2),&store.setup); return true;
    }
    if(pin==GPIO_PIN_4){
        cdi_r8_oem_learn_side_fire(&oem_learner,
            __HAL_TIM_GET_COUNTER(&htim2),&store.setup); return true;
    }
    return false;
}

void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM2) return;
    if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {
        if (!center_high) {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
            center_high = true;
            __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_2,
                __HAL_TIM_GET_COMPARE(htim, TIM_CHANNEL_2) + center_width);
        } else {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
            center_high = false;
            HAL_TIM_OC_Stop_IT(htim, TIM_CHANNEL_2);
        }
    } else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
        if (!side_high) {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
            side_high = true;
            __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_3,
                __HAL_TIM_GET_COMPARE(htim, TIM_CHANNEL_3) + side_width);
        } else {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
            side_high = false;
            HAL_TIM_OC_Stop_IT(htim, TIM_CHANNEL_3);
        }
    } else if(htim->Channel==HAL_TIM_ACTIVE_CHANNEL_4){if(!strobe_high){HAL_GPIO_WritePin(GPIOB,GPIO_PIN_9,GPIO_PIN_SET);strobe_high=true;__HAL_TIM_SET_COMPARE(htim,TIM_CHANNEL_4,__HAL_TIM_GET_COMPARE(htim,TIM_CHANNEL_4)+strobe_width);}else{HAL_GPIO_WritePin(GPIOB,GPIO_PIN_9,GPIO_PIN_RESET);strobe_high=false;HAL_TIM_OC_Stop_IT(htim,TIM_CHANNEL_4);}}
}

/* Panggil setiap 1 ms. Skala TPS disimpan oleh Quick Setup pada firmware yang sama. */
void R5_OneMillisecond(void)
{
    static uint8_t divider;
    static uint8_t telemetry_divider;
    uint32_t arr;
    uint16_t battery_mv = cdi_r5_vbat_adc_to_mv(adc_dma[4]);
    bool battery_ok = battery_mv >= 9500u && battery_mv <= 16000u;
    battery_ok_state = battery_ok;
    bool fault_low = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_10) == GPIO_PIN_RESET;
    if(ota_reboot_ms!=0u&&--ota_reboot_ms==0u)NVIC_SystemReset();
    protocol.tps_raw=adc_dma[0];
    if (store.setup.tps_open_adc > store.setup.tps_closed_adc + 50u) {
        uint32_t raw = adc_dma[0];
        if (raw <= store.setup.tps_closed_adc) protocol.tps_permille = 0u;
        else if (raw >= store.setup.tps_open_adc) protocol.tps_permille = 1000u;
        else protocol.tps_permille = (uint16_t)(
            ((raw - store.setup.tps_closed_adc) * 1000u) /
            (uint32_t)(store.setup.tps_open_adc - store.setup.tps_closed_adc));
    } else {
        protocol.tps_permille = 0u;
    }
    sync_setup(); engine.output_permission=!protocol.strobe_active&&engine.center_enabled&&
        store.setup.operating_mode==CDI_R8_OP_DIY&&
        store.setup.diy_oem_unplug_confirmed&&battery_ok&&
        !protocol.firmware_update_active;
    engine.pro_enabled=store.setup.pro_enabled!=0u;
    protocol.output_permission=engine.output_permission;
    protocol.pro_enabled=engine.pro_enabled;
    if ((uint32_t)(HAL_GetTick() - last_pickup_ms) > 500u)
        protocol.rpm = 0u;
    protocol.hv_center = cdi_r5_hv_adc_to_volts(adc_dma[2]);
    protocol.hv_side = cdi_r5_hv_adc_to_volts(adc_dma[3]);
    protocol.hv_enabled = charger.duty_permille!=0u || protocol.hv_center>=30u || protocol.hv_side>=30u;
    if (!engine.output_permission || fault_low || protocol.strobe_active)
        force_safe();
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,store.setup.fan_mode==CDI_R7_FAN_OFF?GPIO_PIN_RESET:GPIO_PIN_SET);
    if(store.setup.stage!=CDI_R7_STAGE_FIRST_START){first_start_good_ms=0u;protocol.first_start_seconds=0u;}
    else if(protocol.rpm>=500u&&protocol.rpm<=3200u&&!charger.fault_latched){
        if(first_start_good_ms<60000u)++first_start_good_ms;
        protocol.first_start_seconds=(uint16_t)(first_start_good_ms/1000u);
        if(first_start_good_ms>=3000u&&!first_start_proof_written){
            force_safe(); charger.duty_permille=0u;
            first_start_proof_written=R8_FirstStartProofWrite();
            if(first_start_proof_written){store.setup.first_start_proven=1u;cdi_r5_store_seal(&store);}
        }
    }
    if(store.setup.stage==CDI_R7_STAGE_FIRST_START&&store.setup.first_start_proven&&
       protocol.rpm==0u&&protocol.hv_center<30u&&protocol.hv_side<30u){
        store.setup.stage=CDI_R7_STAGE_READY;store.setup.center_enabled=1u;
        store.setup.side_enabled=store.oem_profile.valid&&store.oem_profile.side_samples>=10u;
        cdi_r5_store_seal(&store);
        if(R5_FlashSave(&store)) (void)R8_FirstStartProofClear();
    }
    if (++divider < 10u) return;
    divider = 0u;
    cdi_r5_charger_update(&charger, engine.hv_target_override?engine.hv_target_override:protocol.working.hv_target_volts,
                          adc_dma[2], adc_dma[3], engine.output_permission,
                          engine.output_permission && !protocol.strobe_active, fault_low);
    if (charger.duty_permille == 0u) {
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
        HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_2);
    } else {
        arr = __HAL_TIM_GET_AUTORELOAD(&htim1) + 1u;
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2,
            (arr * charger.duty_permille) / 1000u);
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
        HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
    }
    if (++telemetry_divider >= 5u) { /* dipanggil sesudah divider 10 ms: 50 ms/20 Hz */
        cdi_r5_ble_telemetry_t t = {0};
        uint8_t packet[CDI_R5_BLE_TELEMETRY_SIZE];
        size_t n;
        telemetry_divider = 0u;
        t.sequence = ++telemetry_sequence;
        t.rpm = protocol.rpm > 65535u ? 65535u : (uint16_t)protocol.rpm;
        t.tps_permille = protocol.tps_permille;
        t.advance_cdeg = last_advance_cdeg;
        t.battery_centivolts = (uint16_t)(battery_mv / 10u);
        t.hv_center_volts = protocol.hv_center;
        t.hv_side_volts = protocol.hv_side;
        t.temperature_cdeg = INT16_MIN; /* isi setelah kurva NTC varian motor dikalibrasi */
        t.active_slot = store.active_slot;
        t.limiter_state = last_limiter_state;
        t.flags = (engine.output_permission ? CDI_R5_TF_ARM : 0u) |
                  (engine.pro_enabled ? CDI_R5_TF_PRO_JUMPER : 0u) |
                  (protocol.hv_enabled ? CDI_R5_TF_HV_ENABLED : 0u) |
                  (engine.calibrated ? CDI_R5_TF_CALIBRATED : 0u) |
                  (R5_BleIsConnected() ? CDI_R5_TF_BLE_LINK : 0u);
        if(store.setup.stage==CDI_R7_STAGE_READY)t.flags|=CDI_R5_TF_READY;
        if(store.setup.stage==CDI_R7_STAGE_FIRST_START)t.flags|=CDI_R5_TF_FIRST_START;
        t.fault_bits = fault_low ? CDI_R5_FAULT_HW_CLAMP : 0u;
        if (!battery_ok) t.fault_bits |= CDI_R5_FAULT_BATTERY;
        if (!engine.calibrated) t.fault_bits |= CDI_R5_FAULT_CALIBRATION;
        if (charger.fault_latched) t.fault_bits |= CDI_R5_FAULT_HV_OVERVOLT;
        t.setup_stage=store.setup.stage;t.output_flags=(store.setup.center_enabled?CDI_R7_OF_CENTER:0u)|(store.setup.side_enabled?CDI_R7_OF_SIDE:0u)|(protocol.strobe_active?CDI_R7_OF_STROBE:0u)|(store.setup.fan_mode!=CDI_R7_FAN_OFF?CDI_R7_OF_FAN:0u);t.trigger_angle_cdeg=protocol.setup_trigger_cdeg;t.pickup_quality=protocol.pickup_quality>100u?100u:(uint8_t)protocol.pickup_quality;t.first_start_seconds=protocol.first_start_seconds>255u?255u:(uint8_t)protocol.first_start_seconds;
        n = cdi_r5_ble_encode_telemetry(&t, packet, sizeof(packet));
        if (n != 0u && R5_BleIsConnected())
            R5_BleNotifyTelemetry(packet, (uint16_t)n);
    }
}

/* Dipanggil callback WRITE characteristic COMMAND dari CubeWB. */
void R5_BleCommandReceived(const uint8_t *data, uint16_t length)
{
    uint8_t reply[256];
    size_t n = cdi_r5_ble_handle_command(&protocol, data, length,
                                         reply, sizeof(reply));
    if (n != 0u) R5_BleNotifyResponse(reply, (uint16_t)n);
    if(ota.state==CDI_R8_OTA_READY&&ota_reboot_ms==0u)ota_reboot_ms=1000u;
}

static void ota_status_notify(void)
{
    uint8_t p[CDI_R8_BLE_OTA_STATUS_SIZE]={0};uint16_t crc;
    p[0]=0x18u;p[1]=0xCDu;p[2]=1u;p[3]=(uint8_t)ota.state;
    p[4]=(uint8_t)ota.received;p[5]=(uint8_t)(ota.received>>8u);
    p[6]=(uint8_t)(ota.received>>16u);p[7]=(uint8_t)(ota.received>>24u);
    p[8]=(uint8_t)ota.expected_length;p[9]=(uint8_t)(ota.expected_length>>8u);
    p[10]=(uint8_t)(ota.expected_length>>16u);p[11]=(uint8_t)(ota.expected_length>>24u);
    p[12]=(uint8_t)ota.error_code;p[13]=(uint8_t)(ota.error_code>>8u);
    crc=cdi_r5_crc16(p,14u);p[14]=(uint8_t)crc;p[15]=(uint8_t)(crc>>8u);
    R8_BleNotifyOtaStatus(p,sizeof(p));
}

void R8_BleOtaDataReceived(const uint8_t *data,uint16_t length)
{
    uint32_t offset;uint8_t n;uint16_t supplied;
    if(data==NULL||length<7u)return;
    offset=(uint32_t)data[0]|((uint32_t)data[1]<<8u)|
           ((uint32_t)data[2]<<16u)|((uint32_t)data[3]<<24u);n=data[4];
    if((uint16_t)(n+7u)!=length){ota.state=CDI_R8_OTA_ERROR;ota.error_code=8u;ota_status_notify();return;}
    supplied=(uint16_t)data[length-2u]|((uint16_t)data[length-1u]<<8u);
    if(supplied!=cdi_r5_crc16(data,length-2u)){
        ota.state=CDI_R8_OTA_ERROR;ota.error_code=9u;ota_status_notify();return;
    }
    (void)cdi_r8_ota_write(&ota,offset,data+5u,n);ota_status_notify();
}

#endif
