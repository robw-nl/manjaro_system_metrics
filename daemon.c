#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "config.h"
#include "sensors.h"
#include "json_builder.h"
#include "power_model.h"
#include "discovery.h"

#define MAX_PATH 4096

void update_panel_file(const char *final_path, const AppConfig *cfg, const SystemVitals *v, const DashboardPower *pwr) {
    char tmp_path[MAX_PATH];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", final_path);
    FILE *fp = fopen(tmp_path, "w");
    if (!fp) return;
    json_build_panel(fp, cfg, v, pwr);
    fflush(fp); fclose(fp);
    rename(tmp_path, final_path);
}

void update_tooltip_file(const char *final_path, const AppConfig *cfg, const Accumulator *acc, const DashboardPower *pwr) {
    char tmp_path[MAX_PATH];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", final_path);
    FILE *fp = fopen(tmp_path, "w");
    if (!fp) return;
    json_build_tooltip(fp, cfg, acc, pwr);
    fflush(fp); fclose(fp);
    rename(tmp_path, final_path);
}

void sleep_until_next_tick(struct timespec *target, int interval_ms) {
    target->tv_nsec += interval_ms * 1000000L;
    if (target->tv_nsec >= 1000000000L) { target->tv_nsec -= 1000000000L; target->tv_sec++; }
    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, target, NULL) == EINTR) { }
}

int main() {
    char config_path[MAX_PATH];
    ssize_t len = readlink("/proc/self/exe", config_path, sizeof(config_path) - 1);
    if (len != -1) {
        config_path[len] = '\0';
        char *last = strrchr(config_path, '/');
        if (last) strcpy(last + 1, "metrics.conf");
    } else strcpy(config_path, "metrics.conf");

    AppConfig cfg = load_config(config_path);
    SensorContext sensors;
    init_sensors(&sensors, &cfg);
    PowerModelState logic_state;
    init_power_model(&logic_state, &cfg);
    Accumulator acc = load_from_ssd(&sensors);

    time_t last_sync = time(NULL);
    PeripheralState periph_cache = {0, 1, 0, 0.0};
    unsigned int tick = 0;
    struct timespec next_tick;
    clock_gettime(CLOCK_MONOTONIC, &next_tick);

    // --- State for Hysteresis (place these right before the loop) ---
    SystemVitals last_v = {0};
    DashboardPower last_pwr = {0};
    int force_update = 1; // Ensures we write the first iteration

    while (1) {
        DashboardPower pwr;

        // 1. Inputs: Check Hardware States
        periph_cache.is_monitor_connected = check_monitor_connected(&sensors);
        periph_cache.is_audio_active = check_audio_active(&sensors);

        // 2. Read Vitals: Gated by Monitor Status (Ghost Read Prevention)
        SystemVitals v = read_fast_vitals(&sensors, &periph_cache);

        // 3. Audio Volume: Only poll if audio is actually playing
        if (periph_cache.is_audio_active) {
            if (tick % 5 == 0) periph_cache.volume_ratio = check_audio_volume();
        } else {
            periph_cache.volume_ratio = 0.0;
        }

        // 4. Brain: Calculate all power metrics
        pwr = calculate_power(&logic_state, &cfg, &v, &periph_cache, &acc);
        acc.total_ws += pwr.wall_w;
        acc.total_sec += 1.0;

        // 5. Output with Hysteresis: Only write to /dev/shm if values changed significantly
        // Thresholds: Freq > 10MHz, Temp > 0.5C, Wall Power > 0.2W
        if (force_update ||
            abs(v.cpu_mhz - last_v.cpu_mhz) > 10 ||
            fabs(v.max_temp - last_v.max_temp) > 0.5 ||
            fabs(pwr.wall_w - last_pwr.wall_w) > 0.2 ||
            (tick % 30 == 0)) // Heartbeat: Force write every 30 ticks
        {
            update_panel_file(cfg.path_panel, &cfg, &v, &pwr);

            // Sync current state for next comparison
            last_v = v;
            last_pwr = pwr;
            force_update = 0;
        }

        // 6. Tooltip Update: Always on the 60s tick
        if (tick % 60 == 0) {
            update_tooltip_file(cfg.path_tooltip, &cfg, &acc, &pwr);
        }

        // 7. Persistence: Save to SSD based on sync_sec
        time_t now_time = time(NULL);
        if (difftime(now_time, last_sync) >= cfg.sync_sec) {
            save_to_ssd(&sensors, acc);
            last_sync = now_time;
        }

        // 8. The Metronome: Precise 1s timing
        sleep_until_next_tick(&next_tick, cfg.update_ms);
        tick++;
    }

    cleanup_sensors(&sensors);
    return 0;
}
