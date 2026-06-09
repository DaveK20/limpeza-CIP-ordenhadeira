// ╔══════════════════════════════════════════════════════════════╗
// ║     ACTUATORS.CPP — Implementação dos atuadores              ║
// ╚══════════════════════════════════════════════════════════════╝
#include "actuators.h"

// Array de todos os pinos de saída para facilitar init/reset
static const uint8_t OUTPUT_PINS[] = {
  PIN_RELAY_ALC, PIN_RELAY_ACID, PIN_RELAY_SANIT,
  PIN_VS_CICLO,  PIN_VS_VASAO,   PIN_VS_TS,  PIN_VS_TM,
  PIN_CONTROLE_ORDENHA, PIN_RELAY_RESISTENCIA
};

void actuators_init() {
  for (uint8_t i = 0; i < sizeof(OUTPUT_PINS); i++) {
    pinMode(OUTPUT_PINS[i], OUTPUT);
  }
  pinMode(PIN_BOIA_SOLUCAO, INPUT_PULLUP);
  pinMode(PIN_BOIA_MISTURA, INPUT_PULLUP);
  pinMode(PIN_LED_STATUS,   OUTPUT);
  actuators_safeState();
  Serial.println("[ACT] Atuadores inicializados");
}

// Estado seguro: bombas desligadas (HIGH), resistência OFF,
// ordenha OFF, válvulas de tanque fechadas (HIGH)
void actuators_safeState() {
  bombas_setEstado(HIGH, HIGH, HIGH);
  resistencia(false);
  ordenha(false);
  digitalWrite(PIN_VS_TM,    HIGH);  // fecha valvula tanque mistura
  digitalWrite(PIN_VS_TS,    HIGH);  // fecha valvula tanque aquecimento
  digitalWrite(PIN_VS_CICLO, HIGH);
  digitalWrite(PIN_VS_VASAO, HIGH);
}

// ── Bombas ────────────────────────────────────────────────────
// Relés ativo-baixo: LOW = bomba ligada, HIGH = desligada
void bombas_setEstado(uint8_t alc, uint8_t acid, uint8_t sanit) {
  digitalWrite(PIN_RELAY_ALC,   alc);
  digitalWrite(PIN_RELAY_ACID,  acid);
  digitalWrite(PIN_RELAY_SANIT, sanit);
}

void bomba_alc(bool ligar)   { digitalWrite(PIN_RELAY_ALC,   ligar ? LOW : HIGH); }
void bomba_acid(bool ligar)  { digitalWrite(PIN_RELAY_ACID,  ligar ? LOW : HIGH); }
void bomba_sanit(bool ligar) { digitalWrite(PIN_RELAY_SANIT, ligar ? LOW : HIGH); }

// ── Válvulas ──────────────────────────────────────────────────
void valvula_ciclo(bool abrir) { digitalWrite(PIN_VS_CICLO, abrir ? LOW : HIGH); }
void valvula_vasao(bool saida) { digitalWrite(PIN_VS_VASAO, saida ? HIGH : LOW); }
void valvula_ts(bool abrir)    { digitalWrite(PIN_VS_TS,    abrir ? LOW : HIGH); }
void valvula_tm(bool abrir)    { digitalWrite(PIN_VS_TM,    abrir ? LOW : HIGH); }

// ── Principais ────────────────────────────────────────────────
void resistencia(bool ligar) { digitalWrite(PIN_RELAY_RESISTENCIA, ligar ? HIGH : LOW); }
void ordenha(bool ligar)     { digitalWrite(PIN_CONTROLE_ORDENHA,  ligar ? HIGH : LOW); }

// ── Sensores digitais ─────────────────────────────────────────
bool boia_solucao() { return digitalRead(PIN_BOIA_SOLUCAO); }
bool boia_mistura() { return digitalRead(PIN_BOIA_MISTURA); }