#pragma once
// ╔══════════════════════════════════════════════════════════════╗
// ║       ACTUATORS.H — Controle de Relés e Válvulas            ║
// ║  Extrai e organiza funções de baixo nível do main.cpp        ║
// ╚══════════════════════════════════════════════════════════════╝
#include <Arduino.h>
#include "config.h"

// ── Inicialização ─────────────────────────────────────────────
void actuators_init();

// ── Estado seguro (tudo desligado) ────────────────────────────
void actuators_safeState();

// ── Bombas peristálticas (LOW = liga, HIGH = desliga) ─────────
void bombas_setEstado(uint8_t alc, uint8_t acid, uint8_t sanit);
void bomba_alc(bool ligar);
void bomba_acid(bool ligar);
void bomba_sanit(bool ligar);

// ── Válvulas solenóides ───────────────────────────────────────
void valvula_ciclo(bool abrir);
void valvula_vasao(bool abrir);   // true = saída, false = tanque mistura
void valvula_ts(bool abrir);      // tanque de aquecimento
void valvula_tm(bool abrir);      // tanque de mistura

// ── Atuadores principais ──────────────────────────────────────
void resistencia(bool ligar);
void ordenha(bool ligar);

// ── Sensores ──────────────────────────────────────────────────
bool boia_solucao();   // true = tanque cheio
bool boia_mistura();   // true = tanque cheio