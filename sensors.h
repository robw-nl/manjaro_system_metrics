#ifndef SENSORS_H
#define SENSORS_H

#include <stdio.h>
#include "config.h" // Essential for PeripheralState and AppConfig definitions

typedef struct {
    double total_ws;
    double total_sec;
} Accumulator;

typedef struct {
    int cpu_mhz;
    double soc_w;
    double max_temp;
    double ssd_temp;
    double ram_temp;
    double net_temp;
} SystemVitals;

typedef struct {
    FILE *fp_gpu_power;
    FILE *fp_cpu_temp;
    FILE *fp_ssd_temp;
    FILE *fp_ram_temp;
    FILE *fp_net_temp;
    FILE *fp_cpu_freq[8];
    FILE *fp_drm_status;
    FILE *fp_audio_status;
    char path_data_file[4096];
} SensorContext;

// Implementation of the "Ghost Read" Improvement
SystemVitals read_fast_vitals(SensorContext *ctx, const PeripheralState *p);

void init_sensors(SensorContext *ctx, const AppConfig *cfg);
void cleanup_sensors(SensorContext *ctx);
int check_monitor_connected(SensorContext *ctx);
int check_audio_active(SensorContext *ctx);
double check_audio_volume();
long get_uptime();
void save_to_ssd(SensorContext *ctx, Accumulator acc);
Accumulator load_from_ssd(SensorContext *ctx);

#endif
