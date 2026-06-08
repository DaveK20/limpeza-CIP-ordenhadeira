#pragma once
// ╔══════════════════════════════════════════════════════════════╗
// ║            CIP_CYCLE.H — Lógica do Ciclo CIP                 ║
// ║   Preencha cip_cycle.cpp com a lógica real do seu sistema    ║
// ╚══════════════════════════════════════════════════════════════╝
#include <Arduino.h>
#include "config.h"

// ──────────────────────────────────────────────────────────────
//  Etapas do ciclo CIP
// ──────────────────────────────────────────────────────────────
typedef enum : uint8_t
{
    ETAPA_IDLE = 0,
    ETAPA_PRE_ENXAGUE = 1,
    ETAPA_ALCALINA = 2,
    ETAPA_ENXAGUE_INTERM = 3,
    ETAPA_ACIDA = 4,
    ETAPA_SANITIZANTE = 5,
    ETAPA_FINALIZADO = 6,
    ETAPA_ERRO = 7,
} EtapaCIP;

// ──────────────────────────────────────────────────────────────
//  Procedimentos internos de cada etapa
//  (exibidos na interface — lista expansível)
// ──────────────────────────────────────────────────────────────
typedef enum : uint8_t
{
    PROC_IDLE = 0,
    PROC_POSICIONANDO_VALVULAS = 1,
    PROC_ENCHENDO_TANQUE = 2,
    PROC_AQUECENDO_AGUA = 3,
    PROC_DOSANDO_SOLUCAO = 4,
    PROC_CIRCULANDO = 5,
    PROC_DESCARTANDO = 6,
    PROC_ENXAGUE_CICLO = 7,
    PROC_AGUARDANDO = 8,
    PROC_CONCLUIDO = 9,
} ProcedimentoAtual;

// ──────────────────────────────────────────────────────────────
//  Configurações ajustáveis pela interface web
// ──────────────────────────────────────────────────────────────
typedef struct
{
    // Volumes (mL)
    uint16_t volAlcalina = DEFAULT_VOL_ALCALINA;
    uint16_t volAcida = DEFAULT_VOL_ACIDA;
    uint16_t volSanitizante = DEFAULT_VOL_SANITIZANTE;
    // Temperaturas (°C)
    uint8_t tempAlcalina = DEFAULT_TEMP_ALCALINA;
    uint8_t tempAcida = DEFAULT_TEMP_ACIDA;
    uint8_t tempSanitizante = DEFAULT_TEMP_SANITIZANTE;
    // Tempos (segundos)
    uint16_t tempoCirculacao = DEFAULT_TEMPO_CIRCULACAO;
    uint16_t tempoEnxague = DEFAULT_TEMPO_ENXAGUE;
} CipConfig;

// ──────────────────────────────────────────────────────────────
//  Estado em tempo real (lido pelo web_server para o SSE)
// ──────────────────────────────────────────────────────────────
typedef struct
{
    bool rodando;
    EtapaCIP etapaAtual;
    ProcedimentoAtual procAtual;
    float temperaturaAtual;     // °C do DS18B20
    uint32_t tempoEtapaInicio;  // millis() do início da etapa
    uint32_t tempoEtapaDuracao; // duração total da etapa (ms)
    char mensagemErro[64];
} CipState;

// ──────────────────────────────────────────────────────────────
//  Nomes legíveis (para exibir na interface)
// ──────────────────────────────────────────────────────────────
const char *etapaToString(EtapaCIP e);
const char *procedimentoToString(ProcedimentoAtual p);

// ──────────────────────────────────────────────────────────────
//  API pública
// ──────────────────────────────────────────────────────────────
extern CipState cipState;
extern CipConfig cipConfig;

void cip_init();
void cip_loop();    // Chamar no loop() principal
bool cip_iniciar(); // Retorna false se módulos offline
void cip_parar();
void cip_setConfig(CipConfig &cfg);
float cip_lerTemperatura();
String cip_getStatusJson();