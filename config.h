#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    // Paths
    char path_panel[4096];
    char path_popup[4096];

    // Settings
    int update_ms;
    int sync_sec;
    char output_format[16];
    char start_date[32];

    // Power Calculation
    double pc_rest_base;
    double mon_base;
    double mon_delta;
    double periph_watt;

    // GENERIC SPEAKERS
    double speakers_eco;
    double speakers_standby;
    double speakers_active;
    int speakers_timeout_sec; // NEW: Configurable Timeout

    double euro_per_kwh;
    double mobo_overhead;
    double psu_efficiency;

    // Thresholds
    double limit_mhz_warn;
    double limit_mhz_crit;
    double limit_temp_warn;
    double limit_temp_crit;
    double limit_ssd_warn;
    double limit_ssd_crit;
    double limit_ram_warn;
    double limit_ram_crit;
    double limit_net_warn;
    double limit_net_crit;
    double limit_soc_warn;
    double limit_soc_crit;
    double limit_wall_warn;
    double limit_wall_crit;

    // Hardware IDs
    char main_ssd[64];
    char ssd_label[32];

    // Visuals
    char font_family[64];
    int font_size;

    // Colors
    char color_safe[16];
    char color_warn[16];
    char color_crit[16];
    char color_sep[16];

} AppConfig;

AppConfig load_config(const char *filename);

#endif
