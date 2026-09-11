#include "cdi_r5.h"

#include <assert.h>
#include <stdio.h>

static cdi_r5_engine_config_t engine(bool arm, bool pro)
{
    cdi_r5_engine_config_t e={.timer_hz=4000000u,.pulses_per_revolution=1u,.trigger_angle_cdeg=6000u,.side_offset_cdeg=100,.gate_pulse_us=80u,.calibrated=true,.output_permission=arm,.pro_enabled=pro,.center_enabled=true,.side_enabled=true};
    return e;
}

static void test_defaults_crc_and_lock(void)
{
    cdi_r5_store_image_t image;
    cdi_r5_load_defaults(&image);
    assert(cdi_r5_store_validate(&image) == CDI_R5_OK);
    assert(image.active_slot == 1u);
    assert(image.slots[0].hv_target_volts == 285u);
    assert(image.slots[3].hv_target_volts == 345u);
    assert(image.setup.stage == CDI_R7_STAGE_NEW);
    assert(cdi_r5_map_validate(&image.slots[3], false) == CDI_R5_ERR_PRO_LOCKED);
    image.slots[0].rpm_limit++;
    assert(cdi_r5_store_validate(&image) == CDI_R5_ERR_CRC);
}

static void test_first_start_overrides(void){cdi_r5_store_image_t i;cdi_r5_decision_t d={0};cdi_r5_engine_config_t e=engine(true,false);uint8_t p=0;cdi_r5_load_defaults(&i);e.rpm_limit_override=3000;e.advance_cap_cdeg=1000;e.hv_target_override=220;assert(cdi_r5_make_decision(&e,&i.slots[1],40000,0,&p,&d)==CDI_R5_OK);assert(d.rpm==6000&&d.action==CDI_R5_SPARK_HARD_CUT&&d.advance_cdeg<=1000&&d.hv_target_volts==220);}

static void test_schedule_and_physical_arm(void)
{
    cdi_r5_store_image_t image;
    cdi_r5_decision_t d = {0};
    cdi_r5_engine_config_t e = engine(false, false);
    uint8_t phase = 0u;
    cdi_r5_load_defaults(&image);
    assert(cdi_r5_make_decision(&e, &image.slots[1], 160000u, 500u,
                                &phase, &d) == CDI_R5_ERR_DISARMED);
    e.output_permission = true;
    assert(cdi_r5_make_decision(&e, &image.slots[1], 160000u, 500u,
                                &phase, &d) == CDI_R5_OK);
    assert(d.rpm == 1500u && d.hv_target_volts == 285u);
    assert(d.action == CDI_R5_SPARK_FIRE && d.center_delay_ticks > 0u);
    assert(d.side_delay_ticks > d.center_delay_ticks);
}

static void test_limiters(void)
{
    cdi_r5_store_image_t image;
    cdi_r5_decision_t d = {0};
    cdi_r5_engine_config_t e = engine(true, false);
    uint8_t phase = 0u;
    unsigned cuts = 0u, i;
    cdi_r5_load_defaults(&image);
    image.slots[1].rpm_limit = 9000u;
    for (i = 0u; i < 8u; ++i) {
        assert(cdi_r5_make_decision(&e, &image.slots[1], 26666u, 500u,
                                    &phase, &d) == CDI_R5_OK);
        if (d.action != CDI_R5_SPARK_FIRE) ++cuts;
    }
    assert(cuts == 4u);
    image.slots[1].limiter_type = CDI_R5_LIMITER_HARD;
    assert(cdi_r5_make_decision(&e, &image.slots[1], 26666u, 500u,
                                &phase, &d) == CDI_R5_OK);
    assert(d.action == CDI_R5_SPARK_HARD_CUT);
}

static void test_live_guard_and_slots(void)
{
    cdi_r5_store_image_t image;
    cdi_r5_map_t edit;
    int16_t old;
    cdi_r5_load_defaults(&image);
    edit = image.slots[1];
    old = edit.advance_cdeg[1][2];
    assert(cdi_r5_live_set_cell(&edit, 1u, 2u, old + 300,
                                true, false) == CDI_R5_ERR_LIVE_STEP);
    assert(cdi_r5_live_set_cell(&edit, 1u, 2u, old + 100,
                                true, false) == CDI_R5_OK);
    assert(cdi_r5_save_slot(&image, 2u, &edit, 1500u,
                            false, false) == CDI_R5_ERR_ENGINE_RUNNING);
    assert(cdi_r5_save_slot(&image, 2u, &edit, 0u,
                            true, false) == CDI_R5_ERR_HV_ACTIVE);
    assert(cdi_r5_save_slot(&image, 2u, &edit, 0u,
                            false, false) == CDI_R5_OK);
    assert(image.active_slot == 2u && cdi_r5_store_validate(&image) == CDI_R5_OK);
    assert(image.slots[2].name[0] == 'R');
}

static void test_two_pickups_per_revolution(void)
{
    cdi_r5_store_image_t image;
    cdi_r5_decision_t d = {0};
    cdi_r5_engine_config_t e = engine(true, false);
    uint8_t phase = 0u;
    cdi_r5_load_defaults(&image);
    e.pulses_per_revolution = 2u;
    assert(cdi_r5_make_decision(&e, &image.slots[1], 80000u, 500u,
                                &phase, &d) == CDI_R5_OK);
    assert(d.rpm == 1500u && d.center_delay_ticks < 40000u);
}

int main(void)
{
    test_defaults_crc_and_lock();
    test_schedule_and_physical_arm();
    test_limiters();
    test_live_guard_and_slots();
    test_two_pickups_per_revolution();
    test_first_start_overrides();
    puts("cdi_r5: all tests passed");
    return 0;
}
