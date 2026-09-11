//
// [MOD] Added 2026-09-09.
//
// Own versioning for this fork, mirroring the convention already established on the ESPHome
// side (see openlcc.yaml's esphome: project: version comment): 0.0.1 was the untouched upstream
// state (commit dcddc1d); 1.0.0 is reached once the original LCC's functions are fully
// reproduced and confirmed on the machine. Bump this with every meaningful firmware change and
// tag the corresponding commit as v.MAJOR.MINOR.PATCH to match.
//
// Per the project's own README, the major version is reserved for RP2040<->ESP32 protocol
// (ESP_RP2040_PROTOCOL_VERSION in esp-protocol.h) breaking changes.

#ifndef RP2040_BIANCA_VERSION_H
#define RP2040_BIANCA_VERSION_H

#define RP2040_FIRMWARE_VERSION_MAJOR 0
#define RP2040_FIRMWARE_VERSION_MINOR 3
#define RP2040_FIRMWARE_VERSION_PATCH 0
#define RP2040_FIRMWARE_VERSION_STRING "0.3.0"

#endif //RP2040_BIANCA_VERSION_H
