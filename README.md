# esp-homekit-sanyo-cooler

This is a HomeKit ESP8266 infrared controller for my sister's air conditioner.

## Home Appliance

SANYO (台灣三洋) Remote Controller RL-900
Only Cooler/Dehumidifier. Temperature: 15-30.

This repo use Homekit HeaterCooler Service (iOS 10+) instead of old Thermostat Service (iOS < 10), which is buggy in Siri.

## Usage

1. Setup build environment for [esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos)
2. Run `git submodule update --init --recursive` for submodules this project use
3. Copy `config-sample.h` to `config.h` and edit the settings.
