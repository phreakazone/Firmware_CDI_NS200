#include "cdi_r5_ble.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    cdi_r5_ble_telemetry_t t = {
        9u, 7250u, 650u, 2345, 1280u, 285u, 283u, 8450,
        1u,0u,CDI_R5_TF_ARM|CDI_R5_TF_CALIBRATED,0u,CDI_R7_STAGE_READY,CDI_R7_OF_CENTER,6000u,88u,4u
    };
    uint8_t packet[CDI_R5_BLE_TELEMETRY_SIZE];
    assert(cdi_r5_ble_encode_telemetry(&t, packet, sizeof(packet)) ==
           CDI_R5_BLE_TELEMETRY_SIZE);
    assert(packet[3] == CDI_R5_BLE_KIND_DIAGNOSTIC);
    assert(cdi_r5_ble_validate_telemetry(packet, sizeof(packet)));
    packet[8] ^= 1u;
    assert(!cdi_r5_ble_validate_telemetry(packet, sizeof(packet)));
    t.sequence++;
    assert(cdi_r5_ble_encode_telemetry(&t, packet, sizeof(packet)) ==
           CDI_R5_BLE_TELEMETRY_SIZE);
    assert(packet[3] == CDI_R5_BLE_KIND_CORE);
    assert(cdi_r5_ble_validate_telemetry(packet, sizeof(packet)));
    puts("cdi_r5_ble: all tests passed");
    return 0;
}
