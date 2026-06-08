// ╔══════════════════════════════════════════════════════════════╗
// ║       CIP_CYCLE.CPP — Implementação do Ciclo CIP             ║
// ║   ► Preencha as funções marcadas com TODO com sua lógica     ║
// ╚══════════════════════════════════════════════════════════════╝
#include "cip_cycle.h"
#include "espnow_manager.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// ──────────────────────────────────────────────────────────────
//  Sensor DS18B20
// ──────────────────────────────────────────────────────────────
static OneWire oneWire(PIN_DS18B20);
static DallasTemperature sensors(&oneWire);

// ──────────────────────────────────────────────────────────────
//  Estado e configuração globais
// ──────────────────────────────────────────────────────────────
CipState cipState = {};
CipConfig cipConfig = {};

// ──────────────────────────────────────────────────────────────
//  Nomes legíveis
// ──────────────────────────────────────────────────────────────
const char *etapaToString(EtapaCIP e)
{
    switch (e)
    {
    case ETAPA_IDLE:
        return "Aguardando";
    case ETAPA_PRE_ENXAGUE:
        return "Pré-enxágue";
    case ETAPA_ALCALINA:
        return "Limpeza Alcalina";
    case ETAPA_ENXAGUE_INTERM:
        return "Enxágue Intermitente";
    case ETAPA_ACIDA:
        return "Limpeza Ácida";
    case ETAPA_SANITIZANTE:
        return "Sanitização";
    case ETAPA_FINALIZADO:
        return "Ciclo Finalizado";
    case ETAPA_ERRO:
        return "ERRO";
    default:
        return "Desconhecido";
    }
}

const char *procedimentoToString(ProcedimentoAtual p)
{
    switch (p)
    {
    case PROC_IDLE:
        return "Sistema em repouso";
    case PROC_POSICIONANDO_VALVULAS:
        return "Posicionando válvulas";
    case PROC_ENCHENDO_TANQUE:
        return "Enchendo o tanque";
    case PROC_AQUECENDO_AGUA:
        return "Aquecendo a água";
    case PROC_DOSANDO_SOLUCAO:
        return "Dosando solução";
    case PROC_CIRCULANDO:
        return "Circulando pelo sistema";
    case PROC_DESCARTANDO:
        return "Descartando líquido";
    case PROC_ENXAGUE_CICLO:
        return "Realizando enxágue";
    case PROC_AGUARDANDO:
        return "Aguardando condição";
    case PROC_CONCLUIDO:
        return "Etapa concluída";
    default:
        return "Desconhecido";
    }
}

// ──────────────────────────────────────────────────────────────
//  Helpers de atuadores
//  TODO: ajuste os níveis lógico (HIGH/LOW) conforme seu relé
// ──────────────────────────────────────────────────────────────
static void valvula(uint8_t pin, bool abrir) { digitalWrite(pin, abrir ? HIGH : LOW); }
static void bomba(bool ligar) { digitalWrite(PIN_BOMBA_PERIST, ligar ? HIGH : LOW); }
static void resistencia(bool ligar) { digitalWrite(PIN_RESISTENCIA, ligar ? HIGH : LOW); }

static void setProcedimento(ProcedimentoAtual p)
{
    cipState.procAtual = p;
    Serial.printf("[CIP] ↳ %s\n", procedimentoToString(p));
}

static void setEtapa(EtapaCIP e)
{
    cipState.etapaAtual = e;
    cipState.tempoEtapaInicio = millis();
    Serial.printf("[CIP] ══ Etapa: %s\n", etapaToString(e));
}

// ──────────────────────────────────────────────────────────────
//  Leitura de temperatura
// ──────────────────────────────────────────────────────────────
float cip_lerTemperatura()
{
    sensors.requestTemperatures();
    float t = sensors.getTempCByIndex(0);
    if (t == DEVICE_DISCONNECTED_C)
        return -1.0f;
    cipState.temperaturaAtual = t;
    return t;
}

// ──────────────────────────────────────────────────────────────
//  Inicialização
// ──────────────────────────────────────────────────────────────
void cip_init()
{
    sensors.begin();

    // Configura pinos de saída
    pinMode(PIN_VALVULA_1, OUTPUT);
    pinMode(PIN_VALVULA_2, OUTPUT);
    pinMode(PIN_VALVULA_3, OUTPUT);
    pinMode(PIN_BOMBA_PERIST, OUTPUT);
    pinMode(PIN_RESISTENCIA, OUTPUT);

    // Garante tudo desligado no boot
    valvula(PIN_VALVULA_1, false);
    valvula(PIN_VALVULA_2, false);
    valvula(PIN_VALVULA_3, false);
    bomba(false);
    resistencia(false);

    cipState.rodando = false;
    cipState.etapaAtual = ETAPA_IDLE;
    cipState.procAtual = PROC_IDLE;

    Serial.println("[CIP] ✅ Sistema CIP inicializado");
}

// ──────────────────────────────────────────────────────────────
//  Parada de emergência
// ──────────────────────────────────────────────────────────────
void cip_parar()
{
    valvula(PIN_VALVULA_1, false);
    valvula(PIN_VALVULA_2, false);
    valvula(PIN_VALVULA_3, false);
    bomba(false);
    resistencia(false);
    cipState.rodando = false;
    cipState.etapaAtual = ETAPA_IDLE;
    cipState.procAtual = PROC_IDLE;
    Serial.println("[CIP] 🛑 Ciclo interrompido");
}

// ──────────────────────────────────────────────────────────────
//  Iniciar ciclo (chamado pela interface)
// ──────────────────────────────────────────────────────────────
bool cip_iniciar()
{
    if (!espnow_allModulesReady())
    {
        Serial.println("[CIP] ❌ Módulos offline — ciclo bloqueado");
        return false;
    }
    if (cipState.rodando)
        return false;

    cipState.rodando = true;
    setEtapa(ETAPA_PRE_ENXAGUE);
    setProcedimento(PROC_POSICIONANDO_VALVULAS);
    Serial.println("[CIP] ▶ Ciclo CIP iniciado");
    return true;
}

void cip_setConfig(CipConfig &cfg) { cipConfig = cfg; }

// ──────────────────────────────────────────────────────────────
//  MÁQUINA DE ESTADOS — Loop principal
//  TODO: implemente a lógica real de cada etapa aqui
// ──────────────────────────────────────────────────────────────
void cip_loop()
{
    if (!cipState.rodando)
        return;

    // Segurança: módulo offline → parar
    if (!espnow_allModulesReady())
    {
        Serial.println("[CIP] ⚠ Módulo offline durante ciclo — parando");
        snprintf(cipState.mensagemErro, sizeof(cipState.mensagemErro),
                 "Módulo desconectado durante o ciclo");
        setEtapa(ETAPA_ERRO);
        cip_parar();
        return;
    }

    // Segurança: temperatura máxima
    if (cipState.temperaturaAtual > TEMP_MAX_SEGURANCA)
    {
        Serial.println("[CIP] 🌡 Temperatura crítica — desligando resistência");
        resistencia(false);
    }

    uint32_t tempoEtapa = millis() - cipState.tempoEtapaInicio;

    switch (cipState.etapaAtual)
    {

    // ── PRÉ-ENXÁGUE ────────────────────────────────────────────
    case ETAPA_PRE_ENXAGUE:
        switch (cipState.procAtual)
        {
        case PROC_POSICIONANDO_VALVULAS:
            // TODO: posicionar válvulas para pré-enxágue
            setProcedimento(PROC_ENCHENDO_TANQUE);
            break;
        case PROC_ENCHENDO_TANQUE:
            // TODO: abrir válvula de água, verificar bóia
            // if (digitalRead(PIN_BOIA_ALTA) == HIGH) {
            setProcedimento(PROC_AQUECENDO_AGUA);
            // }
            break;
        case PROC_AQUECENDO_AGUA:
            // TODO: ligar resistência e aguardar temperatura alvo
            // if (cip_lerTemperatura() >= TARGET_TEMP) {
            setProcedimento(PROC_CIRCULANDO);
            // }
            break;
        case PROC_CIRCULANDO:
            // TODO: acionar ordenhadeira via ESP-NOW, aguardar tempo
            // if (tempoEtapa >= (uint32_t)cipConfig.tempoCirculacao * 1000) {
            setProcedimento(PROC_DESCARTANDO);
            // }
            break;
        case PROC_DESCARTANDO:
            // TODO: direcionar para descarte
            // if (tempoEtapa >= TEMPO_DESCARTE_MS) {
            setEtapa(ETAPA_ALCALINA);
            setProcedimento(PROC_POSICIONANDO_VALVULAS);
            // }
            break;
        default:
            break;
        }
        break;

    // ── LIMPEZA ALCALINA ───────────────────────────────────────
    case ETAPA_ALCALINA:
        // TODO: implementar etapa alcalina (mesma estrutura acima)
        // Após concluir → setEtapa(ETAPA_ENXAGUE_INTERM)
        break;

    // ── ENXÁGUE INTERMITENTE ───────────────────────────────────
    case ETAPA_ENXAGUE_INTERM:
        // TODO: implementar enxágue
        // Após concluir → setEtapa(ETAPA_ACIDA)
        break;

    // ── LIMPEZA ÁCIDA ──────────────────────────────────────────
    case ETAPA_ACIDA:
        // TODO: implementar etapa ácida
        // Após concluir → setEtapa(ETAPA_SANITIZANTE)
        break;

    // ── SANITIZAÇÃO ────────────────────────────────────────────
    case ETAPA_SANITIZANTE:
        // TODO: implementar sanitização
        // Após concluir → setEtapa(ETAPA_FINALIZADO)
        break;

    // ── FINALIZADO ─────────────────────────────────────────────
    case ETAPA_FINALIZADO:
        cipState.rodando = false;
        cipState.procAtual = PROC_CONCLUIDO;
        Serial.println("[CIP] ✅ Ciclo CIP concluído com sucesso!");
        break;

    default:
        break;
    }
}

// ──────────────────────────────────────────────────────────────
//  JSON para SSE (Server-Sent Events)
// ──────────────────────────────────────────────────────────────
String cip_getStatusJson()
{
    String json = "{";
    json += "\"rodando\":" + String(cipState.rodando ? "true" : "false") + ",";
    json += "\"etapa\":" + String((int)cipState.etapaAtual) + ",";
    json += "\"etapaNome\":\"" + String(etapaToString(cipState.etapaAtual)) + "\",";
    json += "\"proc\":" + String((int)cipState.procAtual) + ",";
    json += "\"procNome\":\"" + String(procedimentoToString(cipState.procAtual)) + "\",";
    json += "\"temp\":" + String(cipState.temperaturaAtual, 1) + ",";
    json += "\"erro\":\"" + String(cipState.mensagemErro) + "\"";
    json += "}";
    return json;
}