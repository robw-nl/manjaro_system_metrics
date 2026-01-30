#ifndef SENSORS_H
#define SENSORS_H

// FAST: Changes every second
typedef struct {
    double soc_w;
    double max_temp;
    double ssd_temp;
    double ram_temp;
    double net_temp;
    int cpu_mhz;
} SystemVitals;

// SLOW: Changes rarely (Cache this!)
typedef struct {
    int is_audio_active;
    int is_monitor_on;
    double brightness;
} PeripheralState;

typedef struct {
    double total_ws;
    double total_sec;
} Accumulator;

// --- LIFECYCLE ---
void init_sensors(const char *ssd_label);

// --- READERS ---
SystemVitals read_fast_vitals();     // Reads /sys files (Power, Temp)
PeripheralState read_slow_vitals();  // Checks Audio/Monitor status
long get_uptime();

// --- PERSISTENCE ---
void save_to_ssd(Accumulator acc);
Accumulator load_from_ssd();

#endif
