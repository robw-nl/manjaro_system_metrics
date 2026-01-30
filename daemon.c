#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "config.h"
#include "sensors.h"

#define MAX_PATH 4096

// --- PROTOTYPES ---
const char* get_color(double val, double warn, double crit, const AppConfig *cfg);
void write_panel_json(FILE *fp, const AppConfig *cfg, SystemVitals v, double soc_w, double system_w, double ext_w, double wall_w, double cost);
void write_atomic_file(const char *final_path, const AppConfig *cfg,
                       void (*write_func)(FILE*, const AppConfig*, SystemVitals, double, double, double, double, double),
                       SystemVitals v, double soc_w, double sys_w, double ext_w, double wall_w, double cost);

const char* get_color(double val, double warn, double crit, const AppConfig *cfg) {
    if (val < 1.0) return cfg->color_safe;
    if (val >= crit) return cfg->color_crit;
    if (val >= warn) return cfg->color_warn;
    return cfg->color_safe;
}

void write_panel_json(FILE *fp, const AppConfig *cfg, SystemVitals v, double soc_w, double system_w, double ext_w, double wall_w, double cost) {
    const char* c_mhz  = get_color(v.cpu_mhz, cfg->limit_mhz_warn, cfg->limit_mhz_crit, cfg);
    const char* c_soc  = get_color(v.max_temp, cfg->limit_temp_warn, cfg->limit_temp_crit, cfg);
    const char* c_ssd  = get_color(v.ssd_temp, cfg->limit_ssd_warn, cfg->limit_ssd_crit, cfg);
    const char* c_ram  = get_color(v.ram_temp, cfg->limit_ram_warn, cfg->limit_ram_crit, cfg);
    const char* c_net  = get_color(v.net_temp, cfg->limit_net_warn, cfg->limit_net_crit, cfg);
    const char* c_wall = get_color(wall_w, cfg->limit_wall_warn, cfg->limit_wall_crit, cfg);

    fprintf(fp, "{"
    "\"font_size\":%d,"
    "\"font_family\":\"%s\","
    "\"cpu\":{\"val\":%d,\"unit\":\"MHz\",\"color\":\"%s\"},"
    "\"temp\":{\"val\":%.1f,\"unit\":\"°C\",\"color\":\"%s\"},"
    "\"ssd\":{\"val\":%.0f,\"unit\":\"°C\",\"color\":\"%s\",\"label\":\"%s\"},"
    "\"ram\":{\"val\":%.0f,\"unit\":\"°C\",\"color\":\"%s\"},"
    "\"net\":{\"val\":%.0f,\"unit\":\"°C\",\"color\":\"%s\"},"
    "\"soc\":{\"val\":%.1f,\"unit\":\"W\",\"color\":\"%s\"},"
    "\"sys\":{\"val\":%.1f,\"unit\":\"W\",\"color\":\"%s\"},"
    "\"ext\":{\"val\":%.1f,\"unit\":\"W\",\"color\":\"%s\"},"
    "\"wall\":{\"val\":%.1f,\"unit\":\"W\",\"color\":\"%s\"},"
    "\"cost\":{\"val\":%.2f,\"unit\":\"€\",\"color\":\"%s\"},"
    "\"sep_color\":\"%s\""
    "}",
    cfg->font_size, cfg->font_family,
    v.cpu_mhz, c_mhz,
    v.max_temp, c_soc,
    v.ssd_temp, c_ssd, cfg->ssd_label,
    v.ram_temp, c_ram,
    v.net_temp, c_net,
    soc_w, cfg->color_safe,
    system_w, cfg->color_safe,
    ext_w, cfg->color_safe,
    wall_w, c_wall,
    cost, cfg->color_safe,
    cfg->color_sep
    );
}

void write_atomic_file(const char *final_path, const AppConfig *cfg,
                       void (*write_func)(FILE*, const AppConfig*, SystemVitals, double, double, double, double, double),
                       SystemVitals v, double soc_w, double sys_w, double ext_w, double wall_w, double cost) {
    char tmp_path[MAX_PATH];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", final_path);
    FILE *fp = fopen(tmp_path, "w");
    if (!fp) return;
    write_func(fp, cfg, v, soc_w, sys_w, ext_w, wall_w, cost);
    fflush(fp);
    fclose(fp);
    rename(tmp_path, final_path);
}

int main() {
    char config_path[MAX_PATH];
    ssize_t len = readlink("/proc/self/exe", config_path, sizeof(config_path) - 1);
    if (len != -1) {
        config_path[len] = '\0';
        char *last_slash = strrchr(config_path, '/');
        if (last_slash) strcpy(last_slash + 1, "metrics.conf");
    } else {
        strcpy(config_path, "metrics.conf");
    }

    AppConfig cfg = load_config(config_path);
    if (cfg.psu_efficiency <= 0.0) cfg.psu_efficiency = 0.85;

    init_sensors(cfg.main_ssd);
    Accumulator acc = load_from_ssd();

    time_t last_sync = time(NULL);
    double session_peak_w = 0.0;
    double session_max_temp = 0.0;
    char peak_time_str[16] = "--:--";
    double last_uptime = 0.0;

    // --- STATE VARIABLES ---
    PeripheralState periph_cache = {0, 1, 1.0};
    int tick_count = 0;

    // Use Configured Timeout (Active/Standby at boot)
    int audio_cooldown = cfg.speakers_timeout_sec;

    while (1) {
        SystemVitals v = read_fast_vitals();

        if (tick_count % 5 == 0) {
            periph_cache = read_slow_vitals();
            tick_count = 0;
        }
        tick_count++;

        time_t now_time = time(NULL);
        double uptime = (double)get_uptime();

        if (uptime < last_uptime || last_uptime == 0) {
            session_peak_w = 0.0;
            session_max_temp = 0.0;
            strcpy(peak_time_str, "--:--");
            audio_cooldown = cfg.speakers_timeout_sec; // Reset on reboot
        }
        last_uptime = uptime;

        // --- 3-STATE GENERIC AUDIO LOGIC ---
        double audio_w = 0.0;

        if (periph_cache.is_audio_active) {
            // 1. ACTIVE
            audio_w = cfg.speakers_active;
            audio_cooldown = cfg.speakers_timeout_sec; // Reset Timer
        } else if (audio_cooldown > 0) {
            // 2. STANDBY (Cooldown)
            audio_w = cfg.speakers_standby;
            audio_cooldown--;
        } else {
            // 3. ECO
            audio_w = cfg.speakers_eco;
        }

        // --- Calculations ---
        double soc_w = v.soc_w;
        double system_w = cfg.pc_rest_base + (soc_w * cfg.mobo_overhead);

        double mon_w = cfg.mon_base + (periph_cache.brightness * cfg.mon_delta);
        if (!periph_cache.is_monitor_on) mon_w = cfg.mon_base * 0.1;

        double ext_w = mon_w + cfg.periph_watt + audio_w;
        double wall_w = ((soc_w + system_w) / cfg.psu_efficiency) + ext_w;

        acc.total_ws += wall_w;
        acc.total_sec += 1.0;
        double kwh = acc.total_ws / 3600000.0;
        double cost = kwh * cfg.euro_per_kwh;
        double avg_w = (acc.total_sec > 0) ? (acc.total_ws / acc.total_sec) : 0.0;

        if (wall_w > session_peak_w) {
            session_peak_w = wall_w;
            struct tm *tm_now = localtime(&now_time);
            strftime(peak_time_str, sizeof(peak_time_str), "%H:%M", tm_now);
        }
        if (v.max_temp > session_max_temp) session_max_temp = v.max_temp;

        write_atomic_file(cfg.path_panel, &cfg, write_panel_json, v, soc_w, system_w, ext_w, wall_w, cost);

        char tmp_popup[MAX_PATH];
        snprintf(tmp_popup, sizeof(tmp_popup), "%s.tmp", cfg.path_popup);
        FILE *ft = fopen(tmp_popup, "w");
        if (ft) {
            fprintf(ft, "Measuring Since: %s\n"
            "Wall Avg: %.1fW / Peak: %.0fW\n"
            "Today's Peak: %.0fW / %.0f°C\n"
            "Peak Time: %s\n"
            "------------------------------------------\n"
            "Consumption: %.3f kWh\n"
            "Total Cost: €%.2f\n"
            "Total Time: %.0f:%02.0fh",
            cfg.start_date, avg_w, session_peak_w, session_peak_w, session_max_temp, peak_time_str,
            kwh, cost, acc.total_sec / 3600.0, (acc.total_sec / 60) - ((int)(acc.total_sec / 3600.0) * 60));
            fflush(ft); fclose(ft); rename(tmp_popup, cfg.path_popup);
        }

        if (difftime(now_time, last_sync) >= cfg.sync_sec) {
            save_to_ssd(acc); last_sync = now_time;
        }

        usleep(cfg.update_ms * 1000);
    }
    return 0;
}
