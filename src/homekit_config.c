#include <FreeRTOS.h>
#include <esp8266.h>
#include <stdio.h>

#include "homekit_callback.h"
#include "homekit_config.h"
#include "ir.h"

ac_state_t AC = {.target_temperature = 22,
                 .mode = ac_mode_cooler,
                 .fan = ac_fan_med,
                 .active = false};

bool homekit_initialized = false;

homekit_characteristic_t current_humidity =
  HOMEKIT_CHARACTERISTIC_(CURRENT_RELATIVE_HUMIDITY, 0);
homekit_characteristic_t current_temperature =
  HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0);
homekit_characteristic_t target_temperature = HOMEKIT_CHARACTERISTIC_(
  COOLING_THRESHOLD_TEMPERATURE, DEFAULT_COOLER_TEMPERATURE,
  .min_value = (float[]){MIN_COOLER_TEMPERATURE},
  .max_value = (float[]){MAX_COOLER_TEMPERATURE}, .min_step = (float[]){1},
  .getter = ac_target_temperature_get, .setter = ac_target_temperature_set);
homekit_characteristic_t units =
  HOMEKIT_CHARACTERISTIC_(TEMPERATURE_DISPLAY_UNITS, 0);
homekit_characteristic_t current_heater_cooler_state =
  HOMEKIT_CHARACTERISTIC_(CURRENT_HEATER_COOLER_STATE, 0);
/* INACTIVE = 0; IDLE = 1; HEATING = 2; COOLING = 3; */

homekit_characteristic_t target_heater_cooler_state =
  HOMEKIT_CHARACTERISTIC_(TARGET_HEATER_COOLER_STATE, 2,
                          .valid_values = /* AUTO = 0; HEAT = 1; COOL = 2; */
                          {
                            .count = 1,
                            .values = (uint8_t[]){2},
                          });

homekit_characteristic_t ac_active = HOMEKIT_CHARACTERISTIC_(
  ACTIVE, 0, .getter = ac_active_get, .setter = ac_active_set);
homekit_characteristic_t ac_rotation_speed = HOMEKIT_CHARACTERISTIC_(
  ROTATION_SPEED, 0, .getter = ac_speed_get, .setter = ac_speed_set);

homekit_accessory_t *homekit_accessories[] = {
  HOMEKIT_ACCESSORY(
      .id = 1, .category = homekit_accessory_category_air_conditioner,
      .services =
        (homekit_service_t *[]){
          HOMEKIT_SERVICE(
            ACCESSORY_INFORMATION,
            .characteristics =
              (homekit_characteristic_t *[]){
                HOMEKIT_CHARACTERISTIC(NAME, "AC"),
                HOMEKIT_CHARACTERISTIC(MANUFACTURER, "台灣三洋 SANYO"),
                HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "20190920"),
                HOMEKIT_CHARACTERISTIC(MODEL, "RL-900"),
                HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.0.1"),
                HOMEKIT_CHARACTERISTIC(IDENTIFY, ac_identify), NULL}),
          HOMEKIT_SERVICE(HEATER_COOLER, .primary = true,
                          .characteristics =
                            (homekit_characteristic_t *[]){
                              HOMEKIT_CHARACTERISTIC(NAME, "冷氣"), &ac_active,
                              &current_temperature, &target_temperature,
                              &current_heater_cooler_state,
                              &target_heater_cooler_state, &units,
                              &ac_rotation_speed, NULL}),
          /*HOMEKIT_SERVICE(
            HUMIDITY_SENSOR,
            .characteristics =
              (homekit_characteristic_t *[]){
                HOMEKIT_CHARACTERISTIC(NAME, "濕度"), &current_humidity,
                HOMEKIT_CHARACTERISTIC(ACTIVE, 1), NULL}),*/
          NULL}),
  NULL};
;
homekit_server_config_t homekit_config = {.accessories = homekit_accessories,
                                          .password = HOMEKIT_PASSWORD,
                                          .on_event = on_homekit_event};

EventGroupHandle_t sync_flags;