#pragma once

#include <Arduino.h>

// ╔══════════════════════════════════════════════════════════════╗
// ║              CONFIG.H — Parâmetros do Sistema CIP            ║
// ║   Baseado no main.cpp v0.9.3 (Arduino Mega → ESP32)         ║
// ║   Revise as pinagens antes de gravar!                        ║
// ╚══════════════════════════════════════════════════════════════╝

// ──────────────────────────────────────────────────────────────
//  REDE Wi-Fi (Modo AP + Captive Portal)
// ──────────────────────────────────────────────────────────────
#define WIFI_SSID         "CIP-Ordenha"
#define WIFI_PASSWORD     ""              // Vazio = rede aberta
#define AP_IP             "192.168.4.1"

// ──────────────────────────────────────────────────────────────
//  PINOS — SENSORES
//  TODO: ajuste os pinos conforme sua fiação no ESP32
// ──────────────────────────────────────────────────────────────
#define PIN_DS18B20         15    // Sensor de temperatura DS18B20
#define PIN_BOIA_SOLUCAO    34    // Bóia tanque de aquecimento (INPUT, ativo HIGH quando cheio)
#define PIN_BOIA_MISTURA    35    // Bóia tanque de mistura     (INPUT, ativo HIGH quando cheio)
#define PIN_LED_STATUS      2     // LED interno do ESP32

// ──────────────────────────────────────────────────────────────
//  PINOS — BOMBAS PERISTÁLTICAS (relés: LOW = liga, HIGH = desliga)
//  Origem: relayAlc=30, relayAcid=26, relaySanit=28 (Mega)
//  TODO: remapear para o ESP32
// ──────────────────────────────────────────────────────────────
#define PIN_RELAY_ALC       16    // Bomba peristáltica alcalina
#define PIN_RELAY_ACID      17    // Bomba peristáltica ácida
#define PIN_RELAY_SANIT     18    // Bomba peristáltica sanitizante

// ──────────────────────────────────────────────────────────────
//  PINOS — VÁLVULAS SOLENÓIDES
//  Origem: vs_ciclo=32, vs_vasao=34, vs_ts=36, vs_tm=38 (Mega)
//  TODO: remapear para o ESP32
// ──────────────────────────────────────────────────────────────
#define PIN_VS_CICLO        19    // Válvula inox — tanque de aquecimento/mistura
#define PIN_VS_VASAO        21    // Válvula inox — direcionamento saída/mistura
#define PIN_VS_TS           22    // Válvula latão — tanque de aquecimento
#define PIN_VS_TM           23    // Válvula latão — tanque de mistura

// ──────────────────────────────────────────────────────────────
//  PINOS — ATUADORES PRINCIPAIS
//  Origem: ControleOrdenha=40, relayResistencia=46 (Mega)
//  TODO: remapear para o ESP32
// ──────────────────────────────────────────────────────────────
#define PIN_CONTROLE_ORDENHA  25  // Contator: aciona sucção da ordenhadeira
#define PIN_RELAY_RESISTENCIA 26  // Contator: resistência 18kW (LOW = desliga)

// ──────────────────────────────────────────────────────────────
//  ESP-NOW — Canal e módulos obrigatórios
// ──────────────────────────────────────────────────────────────
#define ESPNOW_CHANNEL            1
#define MODULE_TIMEOUT_MS         6000
#define HEARTBEAT_INTERVAL_MS     2000

extern const char* REQUIRED_MODULES[]  /*= { "ORDENHA", "MOTOR" };*/;
const uint8_t REQUIRED_MODULE_COUNT = 2;

// ──────────────────────────────────────────────────────────────
//  PARÂMETROS DO CICLO CIP
//  Origem: variáveis e #defines do main.cpp
// ──────────────────────────────────────────────────────────────

// Volumes padrão de solução (ml/L * volume_tanque)
// Nota: no original eram ml/L × 50L; aqui guardamos direto em mL
#define DEFAULT_VOL_ALC           75    // 1.5 ml/L × 50L
#define DEFAULT_VOL_ACID          130   // 2.6 ml/L × 50L
#define DEFAULT_VOL_SANIT         0     // a ser calibrado

// Temperaturas padrão (°C)
#define DEFAULT_TEMP_PRE_ENXAGUE  39
#define DEFAULT_TEMP_ALC          39
#define DEFAULT_TEMP_ACID         39

// Tempo de circulação padrão (ms) — 5 minutos
#define DEFAULT_TEMPO_CIRCULACAO  300000UL

// Tempo estimado para esvaziar o tanque (ms) — 2,5 min
#define TEMPO_ESVAZIAR_TANQUE     150000UL

// Tempo de posicionamento das válvulas (ms)
#define TEMPO_POSICIONAMENTO_VALVULA 10000UL

// Fluxo da bomba peristáltica calibrada (mL/s)
#define FLUXO_BOMBA               2.0f

// Volume do tanque (L) — usado no cálculo de dosagem
#define VOLUME_TANQUE_L           50

// Histerese do controle de temperatura (°C)
#define HISTERESE_TEMP            2

// Temperatura máxima de segurança (°C)
#define TEMP_MAX_SEGURANCA        90

// ──────────────────────────────────────────────────────────────
//  ENDEREÇOS NA EEPROM (NVS no ESP32 via Preferences)
// ──────────────────────────────────────────────────────────────
#define NVS_NAMESPACE         "cip"
#define NVS_KEY_VOL_ALC       "volAlc"
#define NVS_KEY_VOL_ACID      "volAcid"
#define NVS_KEY_VOL_SANIT     "volSanit"
#define NVS_KEY_TEMP_PRE      "tempPre"
#define NVS_KEY_TEMP_ALC      "tempAlc"
#define NVS_KEY_TEMP_ACID     "tempAcid"
#define NVS_KEY_TEMPO_CIRC    "tempoCirc"
#define NVS_KEY_CICLO_CUSTOM  "cicloCustom"

// ──────────────────────────────────────────────────────────────
//  CICLO PERSONALIZADO
// ──────────────────────────────────────────────────────────────
#define MAX_ETAPAS_CUSTOM     10   // máximo de etapas na fila

// ──────────────────────────────────────────────────────────────
//  INTERFACE WEB
// ──────────────────────────────────────────────────────────────
#define HOLD_INICIAR_MS           2000   // Hold para iniciar ciclo
#define HOLD_CANCELAR_MS          2000   // Hold para cancelar ciclo
#define STATUS_UPDATE_INTERVAL_MS 1000   // Intervalo SSE