#include "sensors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <limits.h>
#include <ctype.h>

#define DISK_FILE "/home/rob/.config/manjaro_system_metrics/data/stats.dat"

// --- CACHED PATHS (Static) ---
static char p_gpu_power[256] = {0};
static char p_cpu_temp[256]  = {0};
static char p_ssd_temp[256]  = {0};
static char p_ram_temp[256]  = {0};
static char p_net_temp[256]  = {0};
static char p_cpu_freq[16][256];

// Monitor Helpers
static char p_monitor_status[256] = "/sys/class/graphics/fb0/blank";
static char p_brightness[256] = "/sys/class/backlight/amdgpu_bl0/brightness";
static double g_max_brightness = 255.0;

// --- DETECTION LOGIC (Run Once) ---
void detect_hwmon(const char *target, char *out_dir) {
    DIR *dr = opendir("/sys/class/hwmon");
    struct dirent *en;
    if (!dr) return;

    while ((en = readdir(dr))) {
        char npath[256], nval[64];
        snprintf(npath, sizeof(npath), "/sys/class/hwmon/%s/name", en->d_name);
        FILE *f = fopen(npath, "r");
        if (f) {
            if (fscanf(f, "%63s", nval) == 1) {
                if (strstr(nval, target) != NULL) {
                    snprintf(out_dir, 256, "/sys/class/hwmon/%s", en->d_name);
                    fclose(f); break;
                }
            }
            fclose(f);
        }
    }
    closedir(dr);
}

void detect_ssd_sensor(const char *id_str, char *out_dir) {
    out_dir[0] = '\0';
    char controller_target[32] = {0};

    if (id_str && id_str[0] != '\0') {
        char label_path[PATH_MAX];
        snprintf(label_path, sizeof(label_path), "/dev/disk/by-label/%s", id_str);
        char real_dev_path[PATH_MAX];
        if (realpath(label_path, real_dev_path)) {
            char *base_name = strrchr(real_dev_path, '/');
            if (base_name) base_name++; else base_name = real_dev_path;
            if (strncmp(base_name, "nvme", 4) == 0) {
                int i = 4;
                while(isdigit(base_name[i])) i++;
                strncpy(controller_target, base_name, i);
                controller_target[i] = '\0';
            }
        }
    }

    DIR *dr = opendir("/sys/class/hwmon");
    struct dirent *en;
    if (!dr) return;
    char first_nvme_found[256] = "";

    while ((en = readdir(dr))) {
        if (en->d_name[0] == '.') continue;
        char sys_hwmon_path[PATH_MAX];
        snprintf(sys_hwmon_path, sizeof(sys_hwmon_path), "/sys/class/hwmon/%s", en->d_name);
        char real_hwmon_path[PATH_MAX];
        if (realpath(sys_hwmon_path, real_hwmon_path)) {
            int is_nvme = 0;
            if (strstr(real_hwmon_path, "/nvme/") != NULL) is_nvme = 1;
            if (!is_nvme) {
                char name_p[256], name_v[64];
                snprintf(name_p, sizeof(name_p), "%s/name", sys_hwmon_path);
                FILE *f = fopen(name_p, "r");
                if (f) {
                    if (fscanf(f, "%63s", name_v) == 1 && strstr(name_v, "nvme")) is_nvme = 1;
                    fclose(f);
                }
            }
            if (is_nvme) {
                if (first_nvme_found[0] == '\0') strcpy(first_nvme_found, sys_hwmon_path);
                if (controller_target[0] != '\0') {
                    char search_pattern[64];
                    snprintf(search_pattern, sizeof(search_pattern), "/%s/", controller_target);
                    if (strstr(real_hwmon_path, search_pattern) != NULL) {
                        strcpy(out_dir, sys_hwmon_path);
                        closedir(dr); return;
                    }
                }
            }
        }
    }
    closedir(dr);
    if (out_dir[0] == '\0' && first_nvme_found[0] != '\0') strcpy(out_dir, first_nvme_found);
}

// --- INITIALIZATION ---
void init_sensors(const char *ssd_label) {
    char gpu_dir[256] = "";
    char cpu_dir[256] = "";
    char ssd_dir[256] = "";
    char ram_dir[256] = "";
    char net_dir[256] = "";

    detect_hwmon("amdgpu", gpu_dir);
    detect_hwmon("k10temp", cpu_dir);
    detect_hwmon("r8169", net_dir);
    detect_ssd_sensor(ssd_label, ssd_dir);
    detect_hwmon("spd5118", ram_dir);
    if (ram_dir[0] == '\0') detect_hwmon("jc42", ram_dir);

    if (gpu_dir[0]) snprintf(p_gpu_power, 256, "%s/power1_average", gpu_dir);
    if (cpu_dir[0]) snprintf(p_cpu_temp, 256, "%s/temp1_input", cpu_dir);
    if (ssd_dir[0]) snprintf(p_ssd_temp, 256, "%s/temp1_input", ssd_dir);
    if (ram_dir[0]) snprintf(p_ram_temp, 256, "%s/temp1_input", ram_dir);
    if (net_dir[0]) snprintf(p_net_temp, 256, "%s/temp1_input", net_dir);

    for (int i = 0; i < 16; i++) {
        snprintf(p_cpu_freq[i], 256, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", i);
    }

    FILE *f = fopen("/sys/class/backlight/amdgpu_bl0/max_brightness", "r");
    if (f) {
        fscanf(f, "%lf", &g_max_brightness);
        fclose(f);
    }
}

// --- FAST LOOP READERS ---
SystemVitals read_fast_vitals() {
    SystemVitals v = {0};
    FILE *f;
    if (p_gpu_power[0] && (f = fopen(p_gpu_power, "r"))) { double uw; if(fscanf(f, "%lf", &uw)==1) v.soc_w = uw/1000000.0; fclose(f); }
    if (p_cpu_temp[0] && (f = fopen(p_cpu_temp, "r"))) { double m; if(fscanf(f, "%lf", &m)==1) v.max_temp = m/1000.0; fclose(f); }
    if (p_ssd_temp[0] && (f = fopen(p_ssd_temp, "r"))) { double m; if(fscanf(f, "%lf", &m)==1) v.ssd_temp = m/1000.0; fclose(f); }
    if (p_ram_temp[0] && (f = fopen(p_ram_temp, "r"))) { double m; if(fscanf(f, "%lf", &m)==1) v.ram_temp = m/1000.0; fclose(f); }
    if (p_net_temp[0] && (f = fopen(p_net_temp, "r"))) { double m; if(fscanf(f, "%lf", &m)==1) v.net_temp = m/1000.0; fclose(f); }

    long long mhz_sum = 0; int core_count = 0;
    for (int i = 0; i < 16; i++) {
        if ((f = fopen(p_cpu_freq[i], "r"))) {
            long long khz;
            if (fscanf(f, "%lld", &khz) == 1) { mhz_sum += khz/1000; core_count++; }
            fclose(f);
        }
    }
    v.cpu_mhz = (core_count > 0) ? (int)(mhz_sum / core_count) : 0;
    return v;
}

// --- SLOW LOOP READERS ---
// This runs every 5 seconds. It uses the ROBUST logic from your upload.
PeripheralState read_slow_vitals() {
    PeripheralState p = {0, 0, 1.0}; // Default: Audio OFF, Monitor OFF, Brightness Max

    // 1. Audio: Robust Scan of all cards/pcms (Safe here because it's infrequent)
    DIR *dir_cards = opendir("/proc/asound");
    if (dir_cards) {
        struct dirent *card_entry;
        while ((card_entry = readdir(dir_cards)) != NULL) {
            if (strncmp(card_entry->d_name, "card", 4) == 0) {
                // Check pcm0p through pcm5p (Covers all common outputs)
                for (int i = 0; i <= 5; i++) {
                    char path[256], status[64];
                    snprintf(path, sizeof(path), "/proc/asound/%s/pcm%dp/sub0/status", card_entry->d_name, i);
                    FILE *f = fopen(path, "r");
                    if (f) {
                        if (fgets(status, sizeof(status), f) && strstr(status, "RUNNING")) {
                            p.is_audio_active = 1;
                        }
                        fclose(f);
                    }
                    if (p.is_audio_active) break;
                }
            }
            if (p.is_audio_active) break;
        }
        closedir(dir_cards);
    }

    // 2. Monitor Status
    FILE *f = fopen(p_monitor_status, "r");
    if (f) {
        int val = 0;
        if (fscanf(f, "%d", &val) == 1) {
            p.is_monitor_on = (val == 0); // 0 = On, 1 = Blank
        } else {
            p.is_monitor_on = 1;
        }
        fclose(f);
    } else {
        p.is_monitor_on = 1;
    }

    // 3. Brightness
    f = fopen(p_brightness, "r");
    if (f) {
        double cur;
        if (fscanf(f, "%lf", &cur) == 1) p.brightness = cur / g_max_brightness;
        fclose(f);
    }

    return p;
}

long get_uptime() {
    struct sysinfo s_info;
    if (sysinfo(&s_info) == 0) return s_info.uptime;
    return 0;
}

void save_to_ssd(Accumulator acc) {
    FILE *f = fopen(DISK_FILE, "w");
    if (f) { fprintf(f, "%.6lf %.6lf", acc.total_ws, acc.total_sec); fclose(f); }
}

Accumulator load_from_ssd() {
    Accumulator acc = {0.0, 0.0, 0};
    FILE *f = fopen(DISK_FILE, "r");
    if (f) { fscanf(f, "%lf %lf", &acc.total_ws, &acc.total_sec); fclose(f); }
    return acc;
}
