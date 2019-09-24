#pragma once

#include <homekit/characteristics.h>
#include <homekit/homekit.h>

#include "../config.h"
#include "ir.h"

void led_write(bool on);

extern ac_state_t AC;

void on_homekit_event(homekit_event_t event);
extern bool homekit_initialized;

extern homekit_characteristic_t current_humidity;
extern homekit_characteristic_t current_temperature;
extern homekit_characteristic_t target_temperature;
extern homekit_characteristic_t units;
extern homekit_characteristic_t current_heater_cooler_state;
extern homekit_characteristic_t target_heater_cooler_state;

extern homekit_characteristic_t ac_active;
extern homekit_characteristic_t ac_rotation_speed;

extern homekit_server_config_t homekit_config;
