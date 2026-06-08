#pragma once

// ╔══════════════════════════════════════════════════════════════╗
// ║              CONFIG.H — Parâmetros do Sistema CIP            ║
// ║   Edite este arquivo para ajustar o comportamento do sistema ║
// ╚══════════════════════════════════════════════════════════════╝

// ──────────────────────────────────────────────────────────────
//  REDE Wi-Fi (Modo AP)
// ──────────────────────────────────────────────────────────────
#define WIFI_SSID "CIP-Ordenha"
#define WIFI_PASSWORD "" // Vazio = rede aberta
#define AP_IP "192.168.4.1"

// ──────────────────────────────────────────────────────────────
//  PINOS GERAIS
// ──────────────────────────────────────────────────────────────
#define PIN_DS18B20 15    // Sensor de temperatura DS18B20
#define PIN_LED_STATUS 2  // LED interno do ESP32
#define PIN_ROLE_SELECT 4 // LOW = Central, HIGH = Módulo (ESP-NOW)

// ──────────────────────────────────────────────────────────────
//  PINOS DE ATUADORES (ajuste conforme seu protótipo)
// ──────────────────────────────────────────────────────────────
#define PIN_VALVULA_1 16    // Válvula solenoide 1
#define PIN_VALVULA_2 17    // Válvula solenoide 2
#define PIN_VALVULA_3 18    // Válvula solenoide 3 (descarte)
#define PIN_BOMBA_PERIST 19 // Bomba peristáltica (dosadora)
#define PIN_RESISTENCIA 21  // Contator da resistência 18kW
#define PIN_BOIA_ALTA 34    // Bóia: tanque cheio (entrada digital)
#define PIN_BOIA_BAIXA 35   // Bóia: tanque vazio  (entrada digital)

// ──────────────────────────────────────────────────────────────
//  ESP-NOW — Módulos esperados
// ──────────────────────────────────────────────────────────────
#define ESPNOW_CHANNEL 1
#define MODULE_TIMEOUT_MS 6000 // Tempo sem heartbeat → offline
#define HEARTBEAT_INTERVAL_MS 2000

// Nomes dos módulos obrigatórios
const char *REQUIRED_MODULES[] = {"ORDENHA", "MOTOR"};
const uint8_t REQUIRED_MODULE_COUNT = 2;

// ──────────────────────────────────────────────────────────────
//  PARÂMETROS PADRÃO DO CICLO CIP
//  (podem ser alterados pela interface web em tempo real)
// ──────────────────────────────────────────────────────────────

// Volumes de solução (mL)
#define DEFAULT_VOL_ALCALINA 90
#define DEFAULT_VOL_ACIDA 70
#define DEFAULT_VOL_SANITIZANTE 150

// Temperaturas alvo (°C)
#define DEFAULT_TEMP_ALCALINA 75
#define DEFAULT_TEMP_ACIDA 45
#define DEFAULT_TEMP_SANITIZANTE 0 // Sem aquecimento

// Tempo de circulação (segundos)
#define DEFAULT_TEMPO_CIRCULACAO 300 // 5 minutos

// Tempo de enxágue intermitente (segundos)
#define DEFAULT_TEMPO_ENXAGUE 120 // 2 minutos

// ──────────────────────────────────────────────────────────────
//  SEGURANÇA
// ──────────────────────────────────────────────────────────────
#define HOLD_INICIAR_MS 2000         // Tempo de hold para iniciar ciclo (ms)
#define TEMP_MAX_SEGURANCA 90        // °C — desliga resistência acima disso
#define TIMEOUT_SEM_MODULOS_MS 10000 // Para ciclo se módulos ficarem offline

// ──────────────────────────────────────────────────────────────
//  ATUALIZAÇÃO DA INTERFACE
// ──────────────────────────────────────────────────────────────
#define STATUS_UPDATE_INTERVAL_MS 1000 // Frequência do SSE (Server-Sent Events)