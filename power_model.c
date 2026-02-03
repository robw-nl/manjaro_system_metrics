#include "power_model.h"
#include <math.h> // For any future math needs

void init_power_model(PowerModelState *state, const AppConfig *cfg) {
    state->audio_cooldown = cfg->speakers_timeout_sec;
    state->last_uptime = 0.0;
}

DashboardPower calculate_power(
    PowerModelState *state,
    const AppConfig *cfg,
    const SystemVitals *v,
    const PeripheralState *p,
    const Accumulator *acc
) {
    // 1. Uptime Check (Detect Reboots or Time Skips)
    double uptime = get_uptime(); // Helper from sensors.h
    if (uptime < state->last_uptime || state->last_uptime == 0) {
        // Reset cooldowns on reboot
        state->audio_cooldown = cfg->speakers_timeout_sec;
    }
    state->last_uptime = uptime;

    // 2. Dynamic Audio Logic
    double audio_w = 0.0;
    if (p->is_audio_active) {
        // Range: 25.0W (Active) - 10.0W (Standby) = 15.0W dynamic range
        double range = cfg->speakers_active - cfg->speakers_standby;

        // Scale 10.0W + (Volume% * 15.0W)
        audio_w = cfg->speakers_standby + (p->volume_ratio * range);

        state->audio_cooldown = cfg->speakers_timeout_sec;
    } else if (state->audio_cooldown > 0) {
        // Hold at 10.0W until cooldown expires
        audio_w = cfg->speakers_standby;
        state->audio_cooldown--;
    } else {
        // Drop to 2.0W Deep Sleep
        audio_w = cfg->speakers_eco;
    }

    // 3. Monitor Logic
    double mon_w = 0.0;
    if (p->is_monitor_connected == 0) {
        mon_w = 0.0;
    } else if (p->idle_sec > cfg->mon_off_timeout_sec) {
        mon_w = cfg->mon_standby;
    } else if (p->idle_sec > cfg->mon_dim_timeout_sec) {
        mon_w = cfg->mon_logic + (cfg->mon_dim_preset * cfg->mon_backlight_max);
    } else {
        mon_w = cfg->mon_logic + (cfg->mon_brightness_preset * cfg->mon_backlight_max);
    }

    // 4. Wattage Summation
    DashboardPower pwr;
    pwr.soc_w = v->soc_w;
    pwr.system_w = cfg->pc_rest_base + (pwr.soc_w * cfg->mobo_overhead);
    pwr.ext_w = mon_w + cfg->periph_watt + audio_w;
    pwr.wall_w = ((pwr.soc_w + pwr.system_w) / cfg->psu_efficiency) + pwr.ext_w;

    // 5. Cost Calculation (Snapshot)
    // Note: We calculate cost based on the accumulators passed in
    double kwh = acc->total_ws / 3600000.0;
    pwr.cost = kwh * cfg->euro_per_kwh;

    return pwr;
}
