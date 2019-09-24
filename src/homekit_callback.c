
#include <FreeRTOS.h>
#include <esp8266.h>
#include <esplibs/libmain.h>
#include <espressif/esp_system.h>
#include <event_groups.h>
#include <stdio.h>
#include <stdlib.h>
#include <task.h>

#include <dht/dht.h>

#include "homekit_callback.h"
#include "homekit_config.h"
#include "ir.h"

void led_write(bool on) { gpio_write(LED_GPIO, on ? 0 : 1); }

void ac_identify_task(void *_args) {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      led_write(true);
      vTaskDelay(100 / portTICK_PERIOD_MS);
      led_write(false);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    vTaskDelay(250 / portTICK_PERIOD_MS);
  }

  led_write(false);
  vTaskDelete(NULL);
}

void ac_identify(homekit_value_t _value) {
  printf("AC identify\n");
  xTaskCreate(ac_identify_task, "Thermostat identify", 128, NULL, 2, NULL);
};

homekit_value_t ac_active_get() { return HOMEKIT_UINT8(AC.active); };
void ac_active_set(homekit_value_t value) {
  if ((xEventGroupGetBits(sync_flags) & SYNC_FLAGS_UPDATE) == 0)
    return;
  ac_active.value = value;
  AC.active = value.bool_value;

  send_ir_signal(&AC);
};

homekit_value_t ac_target_temperature_get() {
  return HOMEKIT_FLOAT((float)AC.target_temperature);
};
void ac_target_temperature_set(homekit_value_t value) {
  if ((xEventGroupGetBits(sync_flags) & SYNC_FLAGS_UPDATE) == 0)
    return;
  printf("set target temp = %g\n", value.float_value);

  AC.target_temperature = (uint8_t)value.float_value;
  target_temperature.value = value;
  if (AC.active) {
    send_ir_signal(&AC);
  } else {
    printf("AC is not active, change memory only\n");
  }
};

homekit_value_t ac_speed_get() {
  switch (AC.fan) {
  case ac_fan_auto:
    return HOMEKIT_FLOAT(50);
  case ac_fan_low:
    return HOMEKIT_FLOAT(33);
  case ac_fan_med:
    return HOMEKIT_FLOAT(67);
  case ac_fan_high:
    return HOMEKIT_FLOAT(100);
  default:
    return HOMEKIT_FLOAT(0);
  }
}

void ac_speed_set(homekit_value_t value) {
  if ((xEventGroupGetBits(sync_flags) & SYNC_FLAGS_UPDATE) == 0)
    return;
  float input = value.float_value;
  if (input >= 67) {
    ac_rotation_speed.value = HOMEKIT_FLOAT(100);
    AC.fan = ac_fan_high;
  } else if (input >= 33) {
    ac_rotation_speed.value = HOMEKIT_FLOAT(67);
    AC.fan = ac_fan_med;
  } else if (input > 0) {
    ac_rotation_speed.value = HOMEKIT_FLOAT(33);
    AC.fan = ac_fan_low;
  } else {
    ac_rotation_speed.value = HOMEKIT_FLOAT(0);
    ac_active.value = HOMEKIT_BOOL(false);
    AC.active = false;
  }

  homekit_characteristic_notify(&ac_rotation_speed, ac_rotation_speed.value);
  send_ir_signal(&AC);
}

void temperature_sensor_task(void *_args) {
  gpio_set_pullup(TEMPERATURE_SENSOR_GPIO, false, false);

  float humidity_value, temperature_value;
  while (1) {
    /*
    bool success = dht_read_float_data(DHT_TYPE_DHT22, TEMPERATURE_SENSOR_GPIO,
                                       &humidity_value, &temperature_value);
    if (success) {
      printf("[DHT22] temperature %g℃, humidity %g％\n", temperature_value,
             humidity_value);*/

    humidity_value = 50;
    temperature_value = (float)AC.target_temperature;
    current_temperature.value = HOMEKIT_FLOAT(temperature_value);
    current_humidity.value = HOMEKIT_FLOAT(humidity_value);

    homekit_characteristic_notify(&current_temperature,
                                  current_temperature.value);
    homekit_characteristic_notify(&current_humidity, current_humidity.value);

    /* INACTIVE = 0; IDLE = 1; HEATING = 2; COOLING = 3; */
    homekit_value_t new_state_value = HOMEKIT_UINT8(0);

    if (AC.active) {
      if (AC.target_temperature > current_temperature.value.float_value) {
        new_state_value = HOMEKIT_UINT8(1);
      } else {
        new_state_value = HOMEKIT_UINT8(3);
      }
    }
    if (!homekit_value_equal(&new_state_value,
                             &current_heater_cooler_state.value)) {
      current_heater_cooler_state.value = new_state_value;

      homekit_characteristic_notify(&current_heater_cooler_state,
                                    current_heater_cooler_state.value);
    }
    /*
    } else {
      printf("Couldn't read data from sensor\n");
    } */

    vTaskDelay(TEMPERATURE_POLL_PERIOD / portTICK_PERIOD_MS);
  }
}
