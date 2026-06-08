#pragma once
// ╔══════════════════════════════════════════════════════════════╗
// ║          ESPNOW_MANAGER.H — Gerenciador ESP-NOW              ║
// ║  Compatível com espnow_main.ino / espnow_config.h            ║
// ╚══════════════════════════════════════════════════════════════╝
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "config.h"

// ──────────────────────────────────────────────────────────────
//  Estruturas de dados
// ──────────────────────────────────────────────────────────────

typedef enum : uint8_t
{
    MSG_REGISTER = 0x01,
    MSG_HEARTBEAT = 0x02,
    MSG_ACK = 0x03,
    MSG_COMMAND = 0x04,
} MessageType;

typedef struct
{
    MessageType type;
    char moduleName[32];
    uint8_t mac[6];
    uint32_t timestamp;
} EspNowPacket;

typedef struct
{
    char name[32];
    uint8_t mac[6];
    uint32_t lastSeen;
    bool active;
} ModuleInfo;

// ──────────────────────────────────────────────────────────────
//  Estado global dos módulos (acessado pelo web_server)
// ──────────────────────────────────────────────────────────────
extern ModuleInfo espnow_modules[10];
extern uint8_t espnow_moduleCount;
extern bool espnow_allReady;

// ──────────────────────────────────────────────────────────────
//  API pública
// ──────────────────────────────────────────────────────────────
void espnow_init();
void espnow_loop(); // Checar timeouts
bool espnow_isModuleActive(const char *name);
bool espnow_allModulesReady();
void espnow_sendCommand(const char *moduleName, const char *cmd);
String espnow_getStatusJson(); // Retorna JSON para o SSE