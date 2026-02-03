#include <alsa/asoundlib.h>
#include "sensors.h"
#include "config.h"      // Actual definition of PeripheralState
#include "discovery.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/sysinfo.h>

// --- Helper: Open with NO BUFFERING ---
static FILE* fopen_nobuf(const char *path) {
    FILE *f = fopen(path, "r");
    if (f) setvbuf(f, NULL, _IONBF, 0);
    return f;
}

static FILE* find_and_open_hwmon(const char *target_name, const char *file_suffix) {
    char found_dir[256] = {0};
    DIR *dr = opendir("/sys/class/hwmon");
    if (!dr) return NULL;
    struct dirent *en;
    while ((en = readdir(dr))) {
        char name_path[256], name_val[64];
        snprintf(name_path, sizeof(name_path), "/sys/class/hwmon/%s/name", en->d_name);
        FILE *f_name = fopen(name_path, "r");
        if (f_name) {
            if (fscanf(f_name, "%63s", name_val) == 1) {
                if (strstr(name_val, target_name) != NULL) {
                    snprintf(found_dir, sizeof(found_dir), "/sys/class/hwmon/%s", en->d_name);
                    fclose(f_name);
                    break;
                }
            }
            fclose(f_name);
        }
    }
    closedir(dr);
    if (found_dir[0] != '\0') {
        char full_path[256];
        snprintf(full_path, sizeof(full_path), "%s/%s", found_dir, file_suffix);
        return fopen_nobuf(full_path);
    }
    return NULL;
}

void init_sensors(SensorContext *ctx, const AppConfig *cfg) {
    memset(ctx, 0, sizeof(SensorContext));
    if (cfg->path_data[0]) strncpy(ctx->path_data_file, cfg->path_data, sizeof(ctx->path_data_file) - 1);

    ctx->fp_gpu_power = find_and_open_hwmon(cfg->hw_gpu, "power1_average");
    if (!ctx->fp_gpu_power) ctx->fp_gpu_power = find_and_open_hwmon(cfg->hw_gpu, "power1_input");

    ctx->fp_cpu_temp = find_and_open_hwmon(cfg->hw_cpu, "temp1_input");
    ctx->fp_ssd_temp = find_and_open_hwmon(cfg->hw_disk, "temp1_input");
    ctx->fp_ram_temp = find_and_open_hwmon(cfg->hw_ram, "temp1_input");
    if (!ctx->fp_ram_temp) ctx->fp_ram_temp = find_and_open_hwmon("jc42", "temp1_input");
    ctx->fp_net_temp = find_and_open_hwmon(cfg->hw_net, "temp1_input");

    for (int i = 0; i < 8; i++) {
        char path[256];
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", i * 2);
        ctx->fp_cpu_freq[i] = fopen_nobuf(path);
    }

    char discovered_path[256] = {0};
    scan_for_monitor(discovered_path, sizeof(discovered_path));
    if (discovered_path[0] != '\0') ctx->fp_drm_status = fopen_nobuf(discovered_path);
    else if (cfg->path_monitor[0]) ctx->fp_drm_status = fopen_nobuf(cfg->path_monitor);

    if (cfg->path_audio[0]) ctx->fp_audio_status = fopen_nobuf(cfg->path_audio);
}

SystemVitals read_fast_vitals(SensorContext *ctx, const PeripheralState *p) {
    SystemVitals v = {0};
    double val_buf;

    // Gated reads: Skip if monitor is off
    if (p->is_monitor_connected && ctx->fp_gpu_power) {
        rewind(ctx->fp_gpu_power);
        if (fscanf(ctx->fp_gpu_power, "%lf", &val_buf) == 1) v.soc_w = val_buf / 1000000.0;
    }

    // Always poll temps (nobuf fix)
    if (ctx->fp_cpu_temp) { rewind(ctx->fp_cpu_temp); if (fscanf(ctx->fp_cpu_temp, "%lf", &val_buf) == 1) v.max_temp = val_buf / 1000.0; }
    if (ctx->fp_ssd_temp) { rewind(ctx->fp_ssd_temp); if (fscanf(ctx->fp_ssd_temp, "%lf", &val_buf) == 1) v.ssd_temp = val_buf / 1000.0; }
    if (ctx->fp_ram_temp) { rewind(ctx->fp_ram_temp); if (fscanf(ctx->fp_ram_temp, "%lf", &val_buf) == 1) v.ram_temp = val_buf / 1000.0; }
    if (ctx->fp_net_temp) { rewind(ctx->fp_net_temp); if (fscanf(ctx->fp_net_temp, "%lf", &val_buf) == 1) v.net_temp = val_buf / 1000.0; }

    if (p->is_monitor_connected) {
        long long mhz_sum = 0; int core_count = 0;
        for (int i = 0; i < 8; i++) {
            if (ctx->fp_cpu_freq[i]) {
                rewind(ctx->fp_cpu_freq[i]);
                long long khz;
                if (fscanf(ctx->fp_cpu_freq[i], "%lld", &khz) == 1) { mhz_sum += khz / 1000; core_count++; }
            }
        }
        v.cpu_mhz = (core_count > 0) ? (int)(mhz_sum / core_count) : 0;
    }
    return v;
}

int check_monitor_connected(SensorContext *ctx) {
    if (!ctx->fp_drm_status) return 0;
    char status[64]; rewind(ctx->fp_drm_status);
    if (fscanf(ctx->fp_drm_status, "%63s", status) == 1) return (strcmp(status, "connected") == 0);
    return 0;
}

int check_audio_active(SensorContext *ctx) {
    if (!ctx->fp_audio_status) return 0;
    char status[64]; rewind(ctx->fp_audio_status);
    if (fgets(status, sizeof(status), ctx->fp_audio_status)) if (strstr(status, "RUNNING")) return 1;
    return 0;
}

long get_uptime() { struct sysinfo s_info; return (sysinfo(&s_info) == 0) ? s_info.uptime : 0; }

void save_to_ssd(SensorContext *ctx, Accumulator acc) {
    if (!ctx->path_data_file[0]) return;
    FILE *f = fopen(ctx->path_data_file, "w");
    if (f) { fprintf(f, "%.6lf %.6lf", acc.total_ws, acc.total_sec); fclose(f); }
}

Accumulator load_from_ssd(SensorContext *ctx) {
    Accumulator acc = {0.0, 0.0};
    if (!ctx->path_data_file[0]) return acc;
    FILE *f = fopen(ctx->path_data_file, "r");
    if (f) { fscanf(f, "%lf %lf", &acc.total_ws, &acc.total_sec); fclose(f); }
    return acc;
}

double check_audio_volume() {
    long min, max, volume;
    snd_mixer_t *handle;
    snd_mixer_elem_t *elem;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "Master";

    // Open mixer and attach to the default card
    if (snd_mixer_open(&handle, 0) < 0) return 0.5;
    if (snd_mixer_attach(handle, card) < 0) { snd_mixer_close(handle); return 0.5; }
    if (snd_mixer_selem_register(handle, NULL, NULL) < 0) { snd_mixer_close(handle); return 0.5; }
    if (snd_mixer_load(handle) < 0) { snd_mixer_close(handle); return 0.5; }

    // Find the 'Master' playback element
    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    elem = snd_mixer_find_selem(handle, sid);

    if (!elem) {
        snd_mixer_close(handle);
        return 0.5;
    }

    // Get the range and current volume
    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    if (snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_MONO, &volume) < 0) {
        snd_mixer_close(handle);
        return 0.5;
    }

    snd_mixer_close(handle);

    // Ensure we don't divide by zero and return the ratio
    if (max - min == 0) return 0.0;
    return (double)(volume - min) / (double)(max - min);
}

void cleanup_sensors(SensorContext *ctx) {
    if (ctx->fp_gpu_power) fclose(ctx->fp_gpu_power);
    if (ctx->fp_cpu_temp) fclose(ctx->fp_cpu_temp);
    if (ctx->fp_ssd_temp) fclose(ctx->fp_ssd_temp);
    if (ctx->fp_ram_temp) fclose(ctx->fp_ram_temp);
    if (ctx->fp_net_temp) fclose(ctx->fp_net_temp);
    if (ctx->fp_drm_status) fclose(ctx->fp_drm_status);
    if (ctx->fp_audio_status) fclose(ctx->fp_audio_status);
    for (int i = 0; i < 8; i++) if (ctx->fp_cpu_freq[i]) fclose(ctx->fp_cpu_freq[i]);
}
