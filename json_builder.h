#ifndef JSON_BUILDER_H
#define JSON_BUILDER_H

#include <stdio.h>
#include "config.h"
#include "sensors.h"

// Grouping the calculated power values to clean up function arguments
typedef struct {
    double soc_w;
    double system_w;
    double ext_w;
    double wall_w;
    double cost;
} DashboardPower;

// The main formatting function
void json_build_panel(FILE *fp, const AppConfig *cfg, const SystemVitals *v, const DashboardPower *pwr);

// Tooltip formatting
void json_build_tooltip(FILE *fp, const AppConfig *cfg, const Accumulator *acc, const DashboardPower *pwr);

#endif
