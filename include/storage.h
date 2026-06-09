#pragma once
// ╔══════════════════════════════════════════════════════════════╗
// ║    STORAGE.H — Persistência em NVS (substitui EEPROM)       ║
// ║  ESP32 usa Preferences (NVS flash) em vez de EEPROM.update  ║
// ╚══════════════════════════════════════════════════════════════╝
#include <Arduino.h>
#include "config.h"
#include "cip_cycle.h"

void storage_init();
void storage_loadAll(CipConfig& cfg);
void storage_saveConfig(const CipConfig& cfg);
void storage_saveCicloCustom(const uint8_t* fila, uint8_t tamanho);
void storage_loadCicloCustom(uint8_t* fila, uint8_t& tamanho);