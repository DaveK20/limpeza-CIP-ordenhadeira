// ╔══════════════════════════════════════════════════════════════╗
// ║   STORAGE.CPP — NVS via Preferences (ESP32)                 ║
// ║  Substitui EEPROM.update/read do main.cpp original          ║
// ╚══════════════════════════════════════════════════════════════╝
#include "storage.h"
#include <Preferences.h>

static Preferences prefs;

void storage_init() {
  Serial.println("[NVS] Inicializando armazenamento...");
}

// Carrega todas as configurações; usa defaults se for primeira vez
void storage_loadAll(CipConfig& cfg) {
  prefs.begin(NVS_NAMESPACE, true);  // somente-leitura

  cfg.volAlc    = prefs.getUInt(NVS_KEY_VOL_ALC,    DEFAULT_VOL_ALC);
  cfg.volAcid   = prefs.getUInt(NVS_KEY_VOL_ACID,   DEFAULT_VOL_ACID);
  cfg.volSanit  = prefs.getUInt(NVS_KEY_VOL_SANIT,  DEFAULT_VOL_SANIT);
  cfg.tempPre   = prefs.getUChar(NVS_KEY_TEMP_PRE,  DEFAULT_TEMP_PRE_ENXAGUE);
  cfg.tempAlc   = prefs.getUChar(NVS_KEY_TEMP_ALC,  DEFAULT_TEMP_ALC);
  cfg.tempAcid  = prefs.getUChar(NVS_KEY_TEMP_ACID, DEFAULT_TEMP_ACID);
  cfg.tempoCirculacaoMs = prefs.getULong(NVS_KEY_TEMPO_CIRC, DEFAULT_TEMPO_CIRCULACAO);

  prefs.end();
  Serial.printf("[NVS] Config carregada: alc=%umL acid=%umL sanit=%umL "
                "tPre=%u tAlc=%u tAcid=%u circ=%lums\n",
                cfg.volAlc, cfg.volAcid, cfg.volSanit,
                cfg.tempPre, cfg.tempAlc, cfg.tempAcid,
                cfg.tempoCirculacaoMs);
}

// Salva configurações (só grava se o valor mudou — preserva flash)
void storage_saveConfig(const CipConfig& cfg) {
  prefs.begin(NVS_NAMESPACE, false);
  prefs.putUInt(NVS_KEY_VOL_ALC,    cfg.volAlc);
  prefs.putUInt(NVS_KEY_VOL_ACID,   cfg.volAcid);
  prefs.putUInt(NVS_KEY_VOL_SANIT,  cfg.volSanit);
  prefs.putUChar(NVS_KEY_TEMP_PRE,  cfg.tempPre);
  prefs.putUChar(NVS_KEY_TEMP_ALC,  cfg.tempAlc);
  prefs.putUChar(NVS_KEY_TEMP_ACID, cfg.tempAcid);
  prefs.putULong(NVS_KEY_TEMPO_CIRC, cfg.tempoCirculacaoMs);
  prefs.end();
  Serial.println("[NVS] Configurações salvas");
}

// Salva fila do ciclo personalizado (array de uint8_t, tamanho variável)
void storage_saveCicloCustom(const uint8_t* fila, uint8_t tamanho) {
  prefs.begin(NVS_NAMESPACE, false);
  // Salva como blob: 1 byte de tamanho + N bytes de etapas
  uint8_t buf[MAX_ETAPAS_CUSTOM + 1];
  buf[0] = tamanho;
  for (uint8_t i = 0; i < tamanho; i++) buf[i + 1] = fila[i];
  prefs.putBytes(NVS_KEY_CICLO_CUSTOM, buf, tamanho + 1);
  prefs.end();
  Serial.printf("[NVS] Ciclo custom salvo (%u etapas)\n", tamanho);
}

// Carrega fila do ciclo personalizado
void storage_loadCicloCustom(uint8_t* fila, uint8_t& tamanho) {
  prefs.begin(NVS_NAMESPACE, true);
  uint8_t buf[MAX_ETAPAS_CUSTOM + 1] = {};
  size_t len = prefs.getBytes(NVS_KEY_CICLO_CUSTOM, buf, sizeof(buf));
  prefs.end();

  if (len < 1) { tamanho = 0; return; }
  tamanho = min((uint8_t)buf[0], (uint8_t)MAX_ETAPAS_CUSTOM);
  for (uint8_t i = 0; i < tamanho; i++) fila[i] = buf[i + 1];
  Serial.printf("[NVS] Ciclo custom carregado (%u etapas)\n", tamanho);
}