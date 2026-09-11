#include "cdi_r5_charger.h"

#include <assert.h>
#include <stdio.h>

static uint16_t adc_for_volts(uint16_t volts)
{
    uint64_t n = (uint64_t)volts * 4095u * 8200u;
    uint64_t d = 3300u * 1088200u / 1000u;
    return (uint16_t)(n / d);
}

int main(void)
{
    cdi_r5_charger_t c;
    unsigned i;
    cdi_r5_charger_init(&c);
    assert(cdi_r5_vbat_adc_to_mv(2640u) > 11500u);
    assert(cdi_r5_vbat_adc_to_mv(2640u) < 12100u);
    assert(cdi_r5_vbat_adc_to_mv(4095u) > 18000u);
    cdi_r5_charger_update(&c, 285u, 0u, 0u,
                          false, false, false);
    assert(c.state == CDI_R5_CHG_OFF && c.duty_permille == 0u);
    for (i = 0u; i < 20u; ++i)
        cdi_r5_charger_update(&c, 285u,
                              adc_for_volts(100u), adc_for_volts(100u),
                              true, true, false);
    assert(c.state == CDI_R5_CHG_RAMP && c.duty_permille == 40u);
    cdi_r5_charger_update(&c, 285u,
                          adc_for_volts(292u), adc_for_volts(292u),
                          true, true, false);
    assert(c.state == CDI_R5_CHG_REGULATING && c.duty_permille == 0u);
    cdi_r5_charger_update(&c, 345u,
                          adc_for_volts(375u), adc_for_volts(360u),
                          true, true, false);
    assert(c.state == CDI_R5_CHG_FAULT && c.fault_latched);
    assert(!cdi_r5_charger_clear_fault(&c, true, true));
    assert(cdi_r5_charger_clear_fault(&c, true, false));
    puts("cdi_r5_charger: all tests passed");
    return 0;
}
