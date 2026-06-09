#pragma once
// ╔══════════════════════════════════════════════════════════════╗
// ║        CIP_CYCLE.H — Máquina de Estados do Ciclo CIP        ║
// ║  Reescrito sem delay() bloqueante usando millis()            ║
// ╚══════════════════════════════════════════════════════════════╝
#include <Arduino.h>
#include "config.h"

// ──────────────────────────────────────────────────────────────
//  Etapas do ciclo
// ──────────────────────────────────────────────────────────────
typedef enum : uint8_t {
  ETAPA_IDLE            = 0,
  ETAPA_PRE_ENXAGUE     = 1,
  ETAPA_ALCALINA        = 2,
  ETAPA_ENXAGUE_INTERM  = 3,
  ETAPA_ACIDA           = 4,
  ETAPA_SANITIZANTE     = 5,
  ETAPA_FINALIZADO      = 6,
  ETAPA_ERRO            = 7,
} EtapaCIP;

// ──────────────────────────────────────────────────────────────
//  Procedimentos internos (exibidos na interface web)
// ──────────────────────────────────────────────────────────────
typedef enum : uint8_t {
  PROC_IDLE                  = 0,
  PROC_POSICIONANDO_VALVULAS = 1,
  PROC_ENCHENDO_TANQUE       = 2,
  PROC_AQUECENDO_AGUA        = 3,
  PROC_DOSANDO_SOLUCAO       = 4,
  PROC_CIRCULANDO            = 5,
  PROC_DESCARTANDO           = 6,
  PROC_ENXAGUE_CICLO         = 7,
  PROC_AGUARDANDO            = 8,
  PROC_CONCLUIDO             = 9,
} ProcedimentoAtual;

// ──────────────────────────────────────────────────────────────
//  Tipo de ciclo em execução
// ──────────────────────────────────────────────────────────────
typedef enum : uint8_t {
  CICLO_NENHUM      = 0,
  CICLO_CIP         = 1,
  CICLO_CUSTOM      = 2,
} TipoCiclo;

// ──────────────────────────────────────────────────────────────
//  Configurações (ajustáveis pela web e persistidas)
// ──────────────────────────────────────────────────────────────
typedef struct {
  uint16_t      volAlc;              // mL
  uint16_t      volAcid;             // mL
  uint16_t      volSanit;            // mL
  uint8_t       tempPre;             // °C pré-enxágue
  uint8_t       tempAlc;             // °C alcalina
  uint8_t       tempAcid;            // °C ácida
  unsigned long tempoCirculacaoMs;   // ms
} CipConfig;

// ──────────────────────────────────────────────────────────────
//  Estado em tempo real (lido pelo web_server para o SSE)
// ──────────────────────────────────────────────────────────────
typedef struct {
  bool              rodando;
  TipoCiclo         tipoCiclo;
  EtapaCIP          etapaAtual;
  ProcedimentoAtual procAtual;
  float             temperaturaAtual;
  char              mensagemErro[64];
  // Fila do ciclo personalizado
  uint8_t           filaCustom[MAX_ETAPAS_CUSTOM];
  uint8_t           filaCustomTam;
  uint8_t           filaCustomIdx;   // índice em execução
} CipState;

// ──────────────────────────────────────────────────────────────
//  Nomes legíveis
// ──────────────────────────────────────────────────────────────
const char* etapaToString(EtapaCIP e);
const char* procedimentoToString(ProcedimentoAtual p);

// ──────────────────────────────────────────────────────────────
//  Globais acessados por web_server e storage
// ──────────────────────────────────────────────────────────────
extern CipState  cipState;
extern CipConfig cipConfig;

// ──────────────────────────────────────────────────────────────
//  API pública
// ──────────────────────────────────────────────────────────────
void  cip_init();
void  cip_loop();                  // Chamar no loop() principal — não bloqueia
bool  cip_iniciarCIP();            // Retorna false se módulos offline
bool  cip_iniciarCustom(const uint8_t* fila, uint8_t tam);
void  cip_cancelar();              // Parada imediata (segura)
void  cip_setConfig(const CipConfig& cfg);
float cip_lerTemperatura();
String cip_getStatusJson();