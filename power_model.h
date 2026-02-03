#ifndef POWER_MODEL_H
#define POWER_MODEL_H

#include "config.h"
#include "sensors.h"
#include "json_builder.h"

typedef struct {
    int audio_cooldown;
    double last_uptime;
} PowerModelState;

void init_power_model(PowerModelState *state, const AppConfig *cfg);
DashboardPower calculate_power(PowerModelState *state, const AppConfig *cfg, const SystemVitals *v, const PeripheralState *p, const Accumulator *acc);

#endif
