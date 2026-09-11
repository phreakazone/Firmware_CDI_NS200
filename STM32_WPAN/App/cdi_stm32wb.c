#include "common_blesvc.h"
#include "cdi_stm32wb.h"
#include "cdi_r5_ble.h"
#include <string.h>

typedef struct { uint16_t service, telemetry, command, response, ota_data, ota_status, connection; bool connected; } cdi_ble_ctx_t;
static cdi_ble_ctx_t ctx={.connection=0xffff};
static bool stack_ready;
extern void R5_BleCommandReceived(const uint8_t *data,uint16_t length);
extern void R8_BleOtaDataReceived(const uint8_t *data,uint16_t length);

#define COPY_CDI_UUID(dst,n) do { const uint8_t bytes[16]={0x30,0x32,0x53,0x4e,0x4b,0x0b,0x5f,0xa4,0x40,0x4e,0x9d,0x6c,(n),0x10,0x8f,0x7a}; memcpy((dst),bytes,16); } while(0)

static SVCCTL_EvtAckStatus_t cdi_event(void *event)
{
  hci_event_pckt *p=(hci_event_pckt *)(((hci_uart_pckt*)event)->data);
  if(p->evt==HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE){
    evt_blecore_aci *v=(evt_blecore_aci*)p->data;
    if(v->ecode==ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE){
      aci_gatt_attribute_modified_event_rp0 *a=(aci_gatt_attribute_modified_event_rp0*)v->data;
      if(a->Attr_Handle==ctx.command+1){ R5_BleCommandReceived(a->Attr_Data,a->Attr_Data_Length); return SVCCTL_EvtAckFlowEnable; }
      if(a->Attr_Handle==ctx.ota_data+1){ R8_BleOtaDataReceived(a->Attr_Data,a->Attr_Data_Length); return SVCCTL_EvtAckFlowEnable; }
    }
  }
  return SVCCTL_EvtNotAck;
}

void SVCCTL_InitCustomSvc(void)
{
  Char_UUID_t u; SVCCTL_RegisterSvcHandler(cdi_event);
  COPY_CDI_UUID(u.Char_UUID_128,0x00); aci_gatt_add_service(UUID_TYPE_128,(Service_UUID_t*)&u,PRIMARY_SERVICE,14,&ctx.service);
  COPY_CDI_UUID(u.Char_UUID_128,0x01); aci_gatt_add_char(ctx.service,UUID_TYPE_128,&u,CDI_R5_BLE_TELEMETRY_SIZE,CHAR_PROP_READ|CHAR_PROP_NOTIFY,ATTR_PERMISSION_NONE,GATT_NOTIFY_ATTRIBUTE_WRITE,10,1,&ctx.telemetry);
  COPY_CDI_UUID(u.Char_UUID_128,0x02); aci_gatt_add_char(ctx.service,UUID_TYPE_128,&u,220,CHAR_PROP_WRITE|CHAR_PROP_WRITE_WITHOUT_RESP,ATTR_PERMISSION_NONE,GATT_NOTIFY_ATTRIBUTE_WRITE,10,1,&ctx.command);
  COPY_CDI_UUID(u.Char_UUID_128,0x03); aci_gatt_add_char(ctx.service,UUID_TYPE_128,&u,220,CHAR_PROP_READ|CHAR_PROP_NOTIFY,ATTR_PERMISSION_NONE,GATT_NOTIFY_ATTRIBUTE_WRITE,10,1,&ctx.response);
  COPY_CDI_UUID(u.Char_UUID_128,0x04); aci_gatt_add_char(ctx.service,UUID_TYPE_128,&u,215,CHAR_PROP_WRITE|CHAR_PROP_WRITE_WITHOUT_RESP,ATTR_PERMISSION_NONE,GATT_NOTIFY_ATTRIBUTE_WRITE,10,1,&ctx.ota_data);
  COPY_CDI_UUID(u.Char_UUID_128,0x05); aci_gatt_add_char(ctx.service,UUID_TYPE_128,&u,CDI_R8_BLE_OTA_STATUS_SIZE,CHAR_PROP_READ|CHAR_PROP_NOTIFY,ATTR_PERMISSION_NONE,GATT_NOTIFY_ATTRIBUTE_WRITE,10,1,&ctx.ota_status);
  stack_ready=true;
}

void CDI_STM32WB_SetConnected(uint16_t handle,bool connected){ ctx.connection=connected?handle:0xffff; ctx.connected=connected; }
bool R5_BleIsConnected(void){ return ctx.connected; }
bool R8_BleStackIsReady(void){return stack_ready;}
void R5_BleNotifyTelemetry(const uint8_t *d,uint16_t n){ if(ctx.connected) aci_gatt_update_char_value(ctx.service,ctx.telemetry,0,n,(uint8_t*)d); }
void R5_BleNotifyResponse(const uint8_t *d,uint16_t n){ if(ctx.connected) aci_gatt_update_char_value(ctx.service,ctx.response,0,n,(uint8_t*)d); }
void R8_BleNotifyOtaStatus(const uint8_t *d,uint16_t n){if(ctx.connected)aci_gatt_update_char_value(ctx.service,ctx.ota_status,0,n,(uint8_t*)d);}
