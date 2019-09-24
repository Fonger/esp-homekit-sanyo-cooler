#include <FreeRTOS.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <event_groups.h>
#include <stdio.h>
#include <stdlib.h>
#include <task.h>

#include "homekit_callback.h"
#include "homekit_config.h"
#include "ir.h"
#include "ir/generic.h"

EventGroupHandle_t sync_flags;

static ir_generic_config_t ir_protocol_config = {
  .header_mark = 7500,
  .header_space = -4000,
  .bit1_mark = 500,
  .bit1_space = -1500,
  .bit0_mark = 500,
  .bit0_space = -500,
  .footer_mark = 500,
  .footer_space = -500,
  .tolerance = 25,
};


void ir_init() {
  ir_tx_init();
  ir_rx_init(IR_RX_GPIO, 1024);
  sync_flags = xEventGroupCreate();
  xEventGroupSetBits(sync_flags, SYNC_FLAGS_UPDATE);
}

void send_ir_signal(ac_state_t *state) {
  if ((xEventGroupGetBits(sync_flags) & SYNC_FLAGS_UPDATE) == 0)
    return;

  // 22c on
  uint8_t signal[6] = {0}; //{0x0e, 0x31, 0x40, 0x00, 0x08, 0x10};
  const size_t signal_size = 6;
  ac_cmd cmd = state->active ? ac_cmd_temp : ac_cmd_off;

  signal[0] = 0x0E; // fixed header
  signal[1] = (state->target_temperature - 5) | ((uint8_t)cmd << 5);

  if (state->timer > 0) {
    signal[2] = AC_TIMER_SET;
    signal[3] = 0x30 | ((uint8_t)state->timer); // timer hour 1~12
  } else if (state->timer == 0) {
    signal[2] = AC_TIMER_STAY;
    signal[3] = 0;
  } else { // negative: disable
    signal[2] = AC_TIMER_SET;
    signal[3] = 0;
  }

  signal[4] = (uint8_t)state->mode | ((uint8_t)state->fan << 2);
  signal[5] = 0;
  for (int i = 1; i <= 4; i++) {
    signal[5] += (signal[i] >> 4) + (signal[i] & 0x0F);
  }

  printf("Send ");
  for (int i = 0; i < 6; i++) {
    printf("0x%02x ", signal[i]);
  }
  printf("\n...");
  ir_generic_send(&ir_protocol_config, signal, signal_size);
}

int parse_ir_signal(ac_state_t *state_out, uint8_t *signal) {
  if (signal[0] != 0x0E)
    return -1;

  uint8_t checksum = 0;
  for (int i = 1; i <= 4; i++) {
    checksum += (signal[i] >> 4) + (signal[i] & 0x0F);
  }
  if (checksum != signal[5])
    return -2;

  if (signal[2] == AC_TIMER_SET) {
    uint8_t timer = signal[3] & 0x0F;
    if (timer != 0 && (signal[3] & 0xF0) != 0x30)
      return -3;
    state_out->timer = timer;
  } else if (signal[2] == AC_TIMER_STAY) {
    // non op?
  } else {
    return -4;
  }
  state_out->target_temperature = (signal[1] & 0x1F) + 5;
  state_out->active = signal[1] >> 5 != ac_cmd_off;
  state_out->mode = signal[4] & 0b11;
  state_out->fan = signal[4] >> 2;

  return 0;
}

void ir_decode_task(void *_args) {
  ir_decoder_t *generic_decoder = ir_generic_make_decoder(&ir_protocol_config);
  uint8_t buffer[32];
  while (1) {
    uint16_t size = ir_recv(generic_decoder, 0, buffer, sizeof(buffer));
    if (size != 6) {
      printf("decode fail");
      continue;
    }

    xEventGroupClearBits(sync_flags, SYNC_FLAGS_UPDATE);

    printf("Decoded packet (size = %d): ", size);
    for (int i = 0; i < size; i++) {
      printf("0x%02x ", buffer[i]);
      if (i % 16 == 15)
        printf("\n");
    }
    if (size % 16)
      printf("\n");

    printf("decode result = %d\n", parse_ir_signal(&AC, buffer));

    homekit_characteristic_notify(&ac_rotation_speed, ac_rotation_speed.value);

    homekit_value_t new_active = ac_active_get();
    if (!homekit_value_equal(&new_active, &ac_active.value)) {
      ac_active.value = new_active;
      homekit_characteristic_notify(&ac_active, ac_active.value);
    }

    homekit_value_t new_target_temperature =
      HOMEKIT_FLOAT(AC.target_temperature);
    if (!homekit_value_equal(&new_target_temperature,
                             &target_temperature.value)) {
      target_temperature.value = new_target_temperature;
      homekit_characteristic_notify(&target_temperature,
                                    target_temperature.value);
    }

    homekit_value_t new_fan_rotation_speed = ac_speed_get();
    if (!homekit_value_equal(&new_fan_rotation_speed,
                             &ac_rotation_speed.value)) {
      ac_rotation_speed.value = new_target_temperature;
      homekit_characteristic_notify(&ac_rotation_speed,
                                    ac_rotation_speed.value);
    }

    xEventGroupSetBits(sync_flags, SYNC_FLAGS_UPDATE);
  }
  generic_decoder->free(generic_decoder);
  vTaskDelete(NULL);
}

void ir_dump_task(void *arg) {
  ir_decoder_t *raw_decoder = ir_raw_make_decoder();

  uint16_t buffer_size = sizeof(int16_t) * 1024;
  int16_t *buffer = malloc(buffer_size);
  while (1) {
    int size = ir_recv(raw_decoder, 0, buffer, buffer_size);
    if (size <= 0)
      continue;

    printf("Decoded packet (size = %d):\n", size);
    for (int i = 0; i < size; i++) {
      printf("%5d ", buffer[i]);
      if (i % 16 == 15)
        printf("\n");
    }

    if (size % 16)
      printf("\n");
  }

  raw_decoder->free(raw_decoder);
  vTaskDelete(NULL);
}