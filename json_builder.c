#include "json_builder.h"
#include <string.h>

// Private helper
static const char* get_color(double val, double warn, double crit, const AppConfig *cfg) {
    if (val < 1.0) return cfg->color_safe;
    if (val >= crit) return cfg->color_crit;
    if (val >= warn) return cfg->color_warn;
    return cfg->color_safe;
}

void json_build_panel(FILE *fp, const AppConfig *cfg, const SystemVitals *v, const DashboardPower *pwr) {
    const char* c_mhz  = get_color(v->cpu_mhz, cfg->limit_mhz_warn, cfg->limit_mhz_crit, cfg);
    const char* c_soc  = get_color(v->max_temp, cfg->limit_temp_warn, cfg->limit_temp_crit, cfg);
    const char* c_ssd  = get_color(v->ssd_temp, cfg->limit_ssd_warn, cfg->limit_ssd_crit, cfg);
    const char* c_ram  = get_color(v->ram_temp, cfg->limit_ram_warn, cfg->limit_ram_crit, cfg);
    const char* c_net  = get_color(v->net_temp, cfg->limit_net_warn, cfg->limit_net_crit, cfg);
    const char* c_wall = get_color(pwr->wall_w, cfg->limit_wall_warn, cfg->limit_wall_crit, cfg);

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
    (int)v->cpu_mhz, c_mhz,
            v->max_temp, c_soc,
            v->ssd_temp, c_ssd, cfg->ssd_label,
            v->ram_temp, c_ram,
            v->net_temp, c_net,
            pwr->soc_w, cfg->color_safe,
            pwr->system_w, cfg->color_safe,
            pwr->ext_w, cfg->color_safe,
            pwr->wall_w, c_wall,
            pwr->cost, cfg->color_safe,
            cfg->color_sep
    );
}

// NEW: Tooltip Logic
void json_build_tooltip(FILE *fp, const AppConfig *cfg, const Accumulator *acc, const DashboardPower *pwr) {
    double avg_w = (acc->total_sec > 0) ? (acc->total_ws / acc->total_sec) : 0.0;
    double kwh = acc->total_ws / 3600000.0;

    // Time Math
    int total_minutes = (int)(acc->total_sec / 60);
    int hours = total_minutes / 60;
    int minutes = total_minutes % 60;

    fprintf(fp, "Measuring Since: %s\n"
    "Wall Avg: %.1fW\n"
    "------------------------------------------\n"
    "Consumption: %.3f kWh\n"
    "Total Cost: €%.2f\n"
    "Total Time: %d:%02dh",
    cfg->start_date,
    avg_w,
    kwh,
    pwr->cost,
    hours, minutes);
}
