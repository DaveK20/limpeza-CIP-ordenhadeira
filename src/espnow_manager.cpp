// ╔══════════════════════════════════════════════════════════════╗
// ║         ESPNOW_MANAGER.CPP — Implementação ESP-NOW           ║
// ╚══════════════════════════════════════════════════════════════╝
#include "espnow_manager.h"

// ──────────────────────────────────────────────────────────────
//  Variáveis globais (declaração)
// ──────────────────────────────────────────────────────────────
ModuleInfo espnow_modules[10];
uint8_t    espnow_moduleCount = 0;
bool       espnow_allReady    = false;

const char* REQUIRED_MODULES[] = { "ORDENHA", "MOTOR" };

static uint8_t broadcastMac[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

// ──────────────────────────────────────────────────────────────
//  Helpers internos
// ──────────────────────────────────────────────────────────────
static bool macEqual(const uint8_t* a, const uint8_t* b) {
  return memcmp(a, b, 6) == 0;
}

static void addPeerIfNew(const uint8_t* mac) {
  if (!esp_now_is_peer_exist(mac)) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = ESPNOW_CHANNEL;
    peer.encrypt = false;
    esp_now_add_peer(&peer);
  }
}

static int findByMac(const uint8_t* mac) {
  for (int i = 0; i < espnow_moduleCount; i++)
    if (macEqual(espnow_modules[i].mac, mac)) return i;
  return -1;
}

static int findByName(const char* name) {
  for (int i = 0; i < espnow_moduleCount; i++)
    if (strcmp(espnow_modules[i].name, name) == 0) return i;
  return -1;
}

static void checkAllReady() {
  bool ok = true;
  for (int i = 0; i < REQUIRED_MODULE_COUNT; i++) {
    int idx = findByName(REQUIRED_MODULES[i]);
    if (idx == -1 || !espnow_modules[idx].active) { ok = false; break; }
  }
  if (ok != espnow_allReady) {
    espnow_allReady = ok;
    Serial.printf("[ESPNOW] %s\n", ok
      ? "✅ Todos os módulos conectados!"
      : "⚠ Sistema incompleto — módulo(s) offline");
  }
}

static void registerModule(const uint8_t* mac, const char* name) {
  int idx = findByMac(mac);
  if (idx == -1) {
    if (espnow_moduleCount >= 10) return;
    idx = espnow_moduleCount++;
    memcpy(espnow_modules[idx].mac, mac, 6);
    Serial.printf("[ESPNOW] 📡 Novo módulo: %s\n", name);
  }
  strncpy(espnow_modules[idx].name, name, 31);
  espnow_modules[idx].lastSeen = millis();
  espnow_modules[idx].active   = true;
  addPeerIfNew(mac);
  checkAllReady();
}

static void sendAck(const uint8_t* dest) {
  EspNowPacket ack = {};
  ack.type = MSG_ACK;
  strncpy(ack.moduleName, "CENTRAL", 31);
  WiFi.macAddress(ack.mac);
  ack.timestamp = millis();
  esp_now_send(dest, (uint8_t*)&ack, sizeof(ack));
}

// ──────────────────────────────────────────────────────────────
//  Callbacks ESP-NOW
// ──────────────────────────────────────────────────────────────
static void onReceive(const uint8_t* mac, const uint8_t* data, int len) {
  if (len < (int)sizeof(EspNowPacket)) return;
  EspNowPacket pkt;
  memcpy(&pkt, data, sizeof(pkt));
  if (pkt.type == MSG_REGISTER || pkt.type == MSG_HEARTBEAT) {
    registerModule(mac, pkt.moduleName);
    sendAck(mac);
  }
}

static void onSent(const uint8_t* mac, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS)
    Serial.println("[ESPNOW] ✗ Falha no envio");
}

// ──────────────────────────────────────────────────────────────
//  API pública
// ──────────────────────────────────────────────────────────────
void espnow_init() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESPNOW] ❌ Falha ao inicializar!");
    return;
  }
  esp_now_register_recv_cb(onReceive);
  esp_now_register_send_cb(onSent);
  Serial.println("[ESPNOW] ✅ Central inicializada");
}

void espnow_loop() {
  static uint32_t lastCheck = 0;
  if (millis() - lastCheck < 1000) return;
  lastCheck = millis();

  bool changed = false;
  for (int i = 0; i < espnow_moduleCount; i++) {
    bool alive = (millis() - espnow_modules[i].lastSeen) < MODULE_TIMEOUT_MS;
    if (espnow_modules[i].active != alive) {
      espnow_modules[i].active = alive;
      changed = true;
      Serial.printf("[ESPNOW] %s %s\n",
        alive ? "✅ Online:" : "❌ Offline:", espnow_modules[i].name);
    }
  }
  if (changed) checkAllReady();
}

bool espnow_isModuleActive(const char* name) {
  int idx = findByName(name);
  return (idx != -1 && espnow_modules[idx].active);
}

bool espnow_allModulesReady() { return espnow_allReady; }

void espnow_sendCommand(const char* moduleName, const char* cmd) {
  int idx = findByName(moduleName);
  if (idx == -1 || !espnow_modules[idx].active) return;
  EspNowPacket pkt = {};
  pkt.type = MSG_COMMAND;
  strncpy(pkt.moduleName, cmd, 31);
  WiFi.macAddress(pkt.mac);
  pkt.timestamp = millis();
  esp_now_send(espnow_modules[idx].mac, (uint8_t*)&pkt, sizeof(pkt));
}

String espnow_getStatusJson() {
  String json = "{\"modules\":[";
  for (int i = 0; i < REQUIRED_MODULE_COUNT; i++) {
    if (i > 0) json += ",";
    bool active = espnow_isModuleActive(REQUIRED_MODULES[i]);
    json += "{\"name\":\"" + String(REQUIRED_MODULES[i]) + "\","
          + "\"active\":" + (active ? "true" : "false") + "}";
  }
  json += "],\"allReady\":" + String(espnow_allReady ? "true" : "false") + "}";
  return json;
}