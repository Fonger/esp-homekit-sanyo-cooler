#pragma once

#include <ir/ir.h>
#include <ir/raw.h>
#include <ir/rx.h>
#include <ir/tx.h>
#include <event_groups.h>

void ir_init();

typedef enum {
  ac_fan_auto = 0x00,
  ac_fan_low = 0x01,
  ac_fan_med = 0x02,
  ac_fan_high = 0x03
} ac_fan;
typedef enum { ac_mode_cooler = 0, ac_mode_dehumidifier = 1 } ac_mode;
typedef enum {
  ac_cmd_off = 0b010,  // 2
  ac_cmd_mode = 0b001, // 1
  ac_cmd_temp = 0b101  // 5
} ac_cmd;

typedef struct {
  bool active;
  ac_mode mode;
  ac_fan fan;
  uint8_t target_temperature;
  uint8_t timer;
} ac_state_t;

#define AC_TIMER_STAY 0x40
#define AC_TIMER_SET 0xC0

void send_ir_signal(ac_state_t *state);
int parse_ir_signal(ac_state_t *state_out, uint8_t *signal);
void ir_decode_task(void *_args);
void ir_dump_task(void *arg);

extern EventGroupHandle_t sync_flags;
#define SYNC_FLAGS_UPDATE (1 << 0)