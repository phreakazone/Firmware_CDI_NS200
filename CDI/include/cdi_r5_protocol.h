#ifndef CDI_R5_PROTOCOL_H
#define CDI_R5_PROTOCOL_H

#include "cdi_r5.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef bool (*cdi_r5_persist_fn)(const cdi_r5_store_image_t *image,
                                  void *context);

typedef struct {
    cdi_r5_store_image_t *store;
    cdi_r5_map_t working;
    uint32_t rpm;
    uint16_t tps_permille, tps_raw, hv_center, hv_side;
    uint16_t setup_trigger_cdeg, pickup_quality, strobe_samples, first_start_seconds;
    bool hv_enabled, physical_arm, pro_jumper, strobe_active;
    cdi_r5_persist_fn persist;
    void *persist_context;
} cdi_r5_protocol_t;

uint16_t cdi_r5_crc16(const void *data, size_t length);
void cdi_r5_protocol_init(cdi_r5_protocol_t *protocol,
                          cdi_r5_store_image_t *store);
void cdi_r5_protocol_set_persist(cdi_r5_protocol_t *protocol,
                                 cdi_r5_persist_fn persist,
                                 void *context);
/* Frame: @sequence,COMMAND,args*CRC16. Returns response length, zero on overflow. */
size_t cdi_r5_protocol_handle(cdi_r5_protocol_t *protocol,
                              const char *frame,
                              char *response,
                              size_t response_size);

#endif
