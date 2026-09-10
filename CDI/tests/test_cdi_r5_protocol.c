#include "cdi_r5_protocol.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void frame(const char *payload, char *out, size_t n)
{
    snprintf(out, n, "@%s*%04X", payload, cdi_r5_crc16(payload, strlen(payload)));
}

static bool persisted;
static bool persist_ok(const cdi_r5_store_image_t *image, void *context)
{
    (void)context;
    persisted = cdi_r5_store_validate(image) == CDI_R5_OK;
    return persisted;
}

static bool persist_fail(const cdi_r5_store_image_t *image, void *context)
{ (void)image; (void)context; return false; }

int main(void)
{
    cdi_r5_store_image_t store;
    cdi_r5_protocol_t p;
    char in[128], out[256];
    size_t n;
    cdi_r5_load_defaults(&store);
    cdi_r5_protocol_init(&p, &store);
    cdi_r5_protocol_set_persist(&p, persist_ok, NULL);
    p.rpm = 1500u; p.tps_permille = 400u;
    p.hv_center = 284u; p.hv_side = 283u;
    frame("0,PING", in, sizeof(in));
    n = cdi_r5_protocol_handle(&p, in, out, sizeof(out));
    assert(n > 0u && strstr(out, "ACK,PONG_R7_2") != NULL);
    frame("1,GET,STATUS", in, sizeof(in));
    n = cdi_r5_protocol_handle(&p, in, out, sizeof(out));
    assert(n > 0u && strstr(out, "STATUS,1500,400,284,283") != NULL);
    frame("2,LIVE,1,2,500", in, sizeof(in));
    n = cdi_r5_protocol_handle(&p, in, out, sizeof(out));
    assert(n > 0u && strstr(out, "ACK,LIVE") != NULL);
    frame("3,LIVE,1,2,900", in, sizeof(in));
    n = cdi_r5_protocol_handle(&p, in, out, sizeof(out));
    assert(n > 0u && strstr(out, "STEP_MAX_2DEG") != NULL);
    frame("4,SAVE,2", in, sizeof(in));
    n = cdi_r5_protocol_handle(&p, in, out, sizeof(out));
    assert(n > 0u && strstr(out, "ENGINE_OR_HV") != NULL);
    p.rpm = 0u;
    frame("5,SAVE,2", in, sizeof(in));
    n = cdi_r5_protocol_handle(&p, in, out, sizeof(out));
    assert(n > 0u && strstr(out, "ACK,SAVE") != NULL && persisted);
    frame("6,LOAD,3", in, sizeof(in));
    n = cdi_r5_protocol_handle(&p, in, out, sizeof(out));
    assert(n > 0u && strstr(out, "PRO_LOCKED") != NULL);
    p.pro_jumper = true;
    n = cdi_r5_protocol_handle(&p, in, out, sizeof(out));
    assert(n > 0u && strstr(out, "ACK,LOAD") != NULL);
    in[strlen(in) - 1u] = in[strlen(in) - 1u] == '0' ? '1' : '0';
    n = cdi_r5_protocol_handle(&p, in, out, sizeof(out));
    assert(n > 0u && strstr(out, "ERR,CRC") != NULL);
    p.pro_jumper=false;p.hv_enabled=false;p.hv_center=0;p.hv_side=0;p.rpm=0;p.pickup_quality=20;
    cdi_r5_protocol_set_persist(&p,persist_fail,NULL);
    frame("7,SETUP,EDGE,RISING",in,sizeof(in));assert(cdi_r5_protocol_handle(&p,in,out,sizeof(out))>0&&strstr(out,"ERR,FLASH")&&store.setup.pickup_edge==CDI_R7_EDGE_FALLING&&cdi_r5_store_validate(&store)==CDI_R5_OK);
    cdi_r5_protocol_set_persist(&p,persist_ok,NULL);
    p.pickup_quality=20;
    frame("7,SETUP,PICKUP,CONFIRM",in,sizeof(in));assert(cdi_r5_protocol_handle(&p,in,out,sizeof(out))>0&&strstr(out,"ACK,PICKUP_OK"));
    frame("8,SETUP,PPR,1",in,sizeof(in));assert(cdi_r5_protocol_handle(&p,in,out,sizeof(out))>0&&store.setup.pulses_per_revolution==1);
    frame("9,SETUP,GATE_US,80",in,sizeof(in));assert(cdi_r5_protocol_handle(&p,in,out,sizeof(out))>0&&store.setup.gate_pulse_us==80);
    frame("10,SETUP,STROBE,ON",in,sizeof(in));assert(cdi_r5_protocol_handle(&p,in,out,sizeof(out))>0&&p.strobe_active);
    frame("11,SETUP,OFFSET,7200",in,sizeof(in));assert(cdi_r5_protocol_handle(&p,in,out,sizeof(out))>0);p.strobe_samples=12;
    frame("12,SETUP,SAVE_TDC",in,sizeof(in));assert(cdi_r5_protocol_handle(&p,in,out,sizeof(out))>0&&store.setup.trigger_angle_cdeg==7200);
    frame("13,SETUP,FIRST_START",in,sizeof(in));assert(cdi_r5_protocol_handle(&p,in,out,sizeof(out))>0&&store.setup.center_enabled&&!store.setup.side_enabled);p.first_start_seconds=3;
    frame("14,SETUP,READY,CENTER",in,sizeof(in));assert(cdi_r5_protocol_handle(&p,in,out,sizeof(out))>0&&store.setup.stage==CDI_R7_STAGE_READY);
    p.first_start_seconds=0;frame("15,SETUP,READY,THREE,500",in,sizeof(in));assert(cdi_r5_protocol_handle(&p,in,out,sizeof(out))>0&&store.setup.side_enabled&&store.setup.side_offset_cdeg==500);
    puts("cdi_r5_protocol: all tests passed");
    return 0;
}
