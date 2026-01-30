#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

void parse_double(const char *line, const char *key, double *out) {
    char *found = strstr(line, key);
    if (found == line) {
        char *eq = strchr(found, '=');
        if (eq) *out = atof(eq + 1);
    }
}

void parse_int(const char *line, const char *key, int *out) {
    char *found = strstr(line, key);
    if (found == line) {
        char *eq = strchr(found, '=');
        if (eq) *out = atoi(eq + 1);
    }
}

void parse_str(const char *line, const char *key, char *out) {
    char *found = strstr(line, key);
    if (found == line) {
        char *eq = strchr(found, '=');
        if (eq) {
            char *start = eq + 1;
            while (*start == ' ' || *start == '\t') start++;
            sscanf(start, "%[^\n]", out);
            char *newline = strchr(out, '\n'); if (newline) *newline = '\0';
            char *cr = strchr(out, '\r'); if (cr) *cr = '\0';
        }
    }
}

AppConfig load_config(const char *filename) {
    AppConfig cfg;
    memset(&cfg, 0, sizeof(cfg));

    // --- DEFAULTS ---
    strcpy(cfg.path_panel, "/dev/shm/dashboard_panel.txt");
    strcpy(cfg.path_popup, "/dev/shm/dashboard_tooltip.txt");
    cfg.update_ms = 1000;
    cfg.sync_sec = 60;
    strcpy(cfg.output_format, "json");
    strcpy(cfg.start_date, "01-01-2024");
    strcpy(cfg.ssd_label, "SSD");

    cfg.font_size = 11;
    strcpy(cfg.font_family, "Monospace");

    cfg.psu_efficiency = 0.85;
    cfg.mobo_overhead = 0.05;

    // Speaker Defaults
    cfg.speakers_eco = 4.0;
    cfg.speakers_standby = 12.0;
    cfg.speakers_active = 22.0;
    cfg.speakers_timeout_sec = 900; // Default 15 mins

    // Threshold Defaults
    cfg.limit_ssd_warn = 50.0; cfg.limit_ssd_crit = 70.0;
    cfg.limit_ram_warn = 60.0; cfg.limit_ram_crit = 80.0;
    cfg.limit_net_warn = 60.0; cfg.limit_net_crit = 80.0;
    cfg.limit_soc_warn = 20.0; cfg.limit_soc_crit = 45.0;

    FILE *file = fopen(filename, "r");
    if (!file) {
        return cfg;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n') continue;

        parse_int(line, "update_ms", &cfg.update_ms);
        parse_int(line, "sync_sec", &cfg.sync_sec);
        parse_str(line, "output_format", cfg.output_format);
        parse_str(line, "start_date", cfg.start_date);

        parse_double(line, "pc_rest_base", &cfg.pc_rest_base);
        parse_double(line, "mon_base", &cfg.mon_base);
        parse_double(line, "mon_delta", &cfg.mon_delta);
        parse_double(line, "periph_watt", &cfg.periph_watt);

        // Generic Speaker Parsing
        parse_double(line, "speakers_eco", &cfg.speakers_eco);
        parse_double(line, "speakers_standby", &cfg.speakers_standby);
        parse_double(line, "speakers_active", &cfg.speakers_active);

        // Timeout Parsing (Convert Mins to Secs)
        int min_val = 0;
        parse_int(line, "speakers_timeout_min", &min_val);
        if (min_val > 0) cfg.speakers_timeout_sec = min_val * 60;

        parse_double(line, "euro_per_kwh", &cfg.euro_per_kwh);
        parse_double(line, "mobo_overhead", &cfg.mobo_overhead);
        parse_double(line, "psu_efficiency", &cfg.psu_efficiency);

        parse_double(line, "limit_mhz_warn", &cfg.limit_mhz_warn);
        parse_double(line, "limit_mhz_crit", &cfg.limit_mhz_crit);
        parse_double(line, "limit_temp_warn", &cfg.limit_temp_warn);
        parse_double(line, "limit_temp_crit", &cfg.limit_temp_crit);
        parse_double(line, "limit_ssd_warn", &cfg.limit_ssd_warn);
        parse_double(line, "limit_ssd_crit", &cfg.limit_ssd_crit);
        parse_double(line, "limit_ram_warn", &cfg.limit_ram_warn);
        parse_double(line, "limit_ram_crit", &cfg.limit_ram_crit);
        parse_double(line, "limit_net_warn", &cfg.limit_net_warn);
        parse_double(line, "limit_net_crit", &cfg.limit_net_crit);
        parse_double(line, "limit_soc_warn", &cfg.limit_soc_warn);
        parse_double(line, "limit_soc_crit", &cfg.limit_soc_crit);
        parse_double(line, "limit_wall_warn", &cfg.limit_wall_warn);
        parse_double(line, "limit_wall_crit", &cfg.limit_wall_crit);

        parse_str(line, "ssd_label", cfg.ssd_label);
        parse_int(line, "font_size", &cfg.font_size);
        parse_str(line, "font_family", cfg.font_family);
        parse_str(line, "main_ssd", cfg.main_ssd);

        parse_str(line, "color_safe", cfg.color_safe);
        parse_str(line, "color_warn", cfg.color_warn);
        parse_str(line, "color_crit", cfg.color_crit);
        parse_str(line, "color_sep", cfg.color_sep);
    }

    fclose(file);
    return cfg;
}
