#include "cdi_r8_oem_learn.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    cdi_r5_store_image_t store;
    cdi_r8_oem_learner_t learn;
    uint32_t tick = 1000000u;
    uint32_t period = 160000u; /* 1500 RPM at 4 MHz, PPR=1 */
    unsigned i;
    cdi_r5_load_defaults(&store);
    cdi_r8_oem_learn_init(&learn, 4000000u);
    cdi_r8_oem_learn_start(&learn);
    for (i = 0u; i < 24u; ++i) {
        tick += period;
        cdi_r8_oem_learn_pickup(&learn, tick, period, 330u,
                                &store.slots[1]);
        /* trigger 60 BTDC, fire 10 BTDC => 50 degrees after pickup */
        cdi_r8_oem_learn_center_fire(&learn,
            tick + (period * 5000u) / 36000u, &store.setup);
        cdi_r8_oem_learn_side_fire(&learn,
            tick + (period * 5100u) / 36000u, &store.setup);
    }
    assert(learn.profile.accepted_pulses == 24u);
    assert(learn.profile.side_samples == 24u);
    assert(learn.profile.side_offset_cdeg >= 99 &&
           learn.profile.side_offset_cdeg <= 101);
    assert(cdi_r8_oem_learn_finish(&learn, &store.slots[1], true));
    assert(learn.profile.valid == 1u);
    assert(store.slots[1].advance_cdeg[1][2] >= 999 &&
           store.slots[1].advance_cdeg[1][2] <= 1001);
    puts("cdi_r8_oem_learn: all tests passed");
    return 0;
}
