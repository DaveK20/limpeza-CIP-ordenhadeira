// ╔══════════════════════════════════════════════════════════════╗
// ║      CIP_CYCLE.CPP — Máquina de estados não-bloqueante       ║
// ║                                                              ║
// ║  Princípio: NUNCA chama delay(). Toda espera usa millis().   ║
// ║  Cada chamada a cip_loop() avança a máquina de estados e     ║
// ║  retorna imediatamente, mantendo a web responsiva.           ║
// ╚══════════════════════════════════════════════════════════════╝
#include "cip_cycle.h"
#include "actuators.h"
#include "espnow_manager.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// ──────────────────────────────────────────────────────────────
//  Sensor DS18B20
// ──────────────────────────────────────────────────────────────
static OneWire           oneWire(PIN_DS18B20);
static DallasTemperature sensors(&oneWire);

// ──────────────────────────────────────────────────────────────
//  Globais
// ──────────────────────────────────────────────────────────────
CipState  cipState  = {};
CipConfig cipConfig = {};

// ──────────────────────────────────────────────────────────────
//  Variáveis internas da máquina de estados
// ──────────────────────────────────────────────────────────────
static unsigned long _timerInicio = 0;   // início do timer atual
static unsigned long _timerDuracao = 0;  // duração do timer atual
static bool          _timerAtivo = false;

// Fila de etapas do CIP padrão (imutável)
static const uint8_t CIP_SEQUENCIA[] = {
  ETAPA_PRE_ENXAGUE,
  ETAPA_ALCALINA,
  ETAPA_ENXAGUE_INTERM,
  ETAPA_ACIDA,
  ETAPA_ENXAGUE_INTERM,
  ETAPA_SANITIZANTE,
  ETAPA_FINALIZADO
};
static const uint8_t CIP_SEQ_TAM = sizeof(CIP_SEQUENCIA);

// Índice na sequência CIP
static uint8_t _cipSeqIdx = 0;

// Sub-estado dentro de cada etapa
static uint8_t _subEstado = 0;

// ──────────────────────────────────────────────────────────────
//  Helpers internos
// ──────────────────────────────────────────────────────────────

static void iniciarTimer(unsigned long ms) {
  _timerInicio  = millis();
  _timerDuracao = ms;
  _timerAtivo   = true;
}

static bool timerExpirou() {
  if (!_timerAtivo) return true;
  if (millis() - _timerInicio >= _timerDuracao) {
    _timerAtivo = false;
    return true;
  }
  return false;
}

static void setProc(ProcedimentoAtual p) {
  cipState.procAtual = p;
  Serial.printf("[CIP] ↳ %s\n", procedimentoToString(p));
}

// Avança para a próxima etapa da sequência (CIP ou custom)
static void proximaEtapa() {
  actuators_safeState();  // garante estado seguro entre etapas
  _subEstado = 0;

  if (cipState.tipoCiclo == CICLO_CIP) {
    _cipSeqIdx++;
    if (_cipSeqIdx >= CIP_SEQ_TAM) {
      cipState.etapaAtual = ETAPA_FINALIZADO;
    } else {
      cipState.etapaAtual = (EtapaCIP)CIP_SEQUENCIA[_cipSeqIdx];
    }
  } else {
    cipState.filaCustomIdx++;
    if (cipState.filaCustomIdx >= cipState.filaCustomTam) {
      cipState.etapaAtual = ETAPA_FINALIZADO;
    } else {
      cipState.etapaAtual = (EtapaCIP)cipState.filaCustom[cipState.filaCustomIdx];
    }
  }
  Serial.printf("[CIP] ══ Etapa: %s\n", etapaToString(cipState.etapaAtual));
}

// Calcula tempo de dosagem da bomba (ms)
static unsigned long calcTempoSolucao(uint16_t volMl) {
  return (unsigned long)((float)volMl / FLUXO_BOMBA * 1000.0f);
}

// ──────────────────────────────────────────────────────────────
//  Temperatura
// ──────────────────────────────────────────────────────────────
float cip_lerTemperatura() {
  sensors.requestTemperatures();
  float t = sensors.getTempCByIndex(0);
  if (t != DEVICE_DISCONNECTED_C) cipState.temperaturaAtual = t;
  return cipState.temperaturaAtual;
}

static bool temperaturaAtingida(float alvo) {
  float t = cipState.temperaturaAtual;
  return (t >= alvo - HISTERESE_TEMP);
}

static void controlarResistencia(float alvo) {
  float t = cipState.temperaturaAtual;
  if (t < alvo - HISTERESE_TEMP)       resistencia(true);
  else if (t > alvo + HISTERESE_TEMP)  resistencia(false);
  // Segurança absoluta
  if (t > TEMP_MAX_SEGURANCA)           resistencia(false);
}

// ──────────────────────────────────────────────────────────────
//  Nomes legíveis
// ──────────────────────────────────────────────────────────────
const char* etapaToString(EtapaCIP e) {
  switch (e) {
    case ETAPA_IDLE:           return "Aguardando";
    case ETAPA_PRE_ENXAGUE:    return "Pré-enxágue";
    case ETAPA_ALCALINA:       return "Limpeza Alcalina";
    case ETAPA_ENXAGUE_INTERM: return "Enxágue Intermitente";
    case ETAPA_ACIDA:          return "Limpeza Ácida";
    case ETAPA_SANITIZANTE:    return "Sanitização";
    case ETAPA_FINALIZADO:     return "Ciclo Finalizado";
    case ETAPA_ERRO:           return "ERRO";
    default:                   return "Desconhecido";
  }
}

const char* procedimentoToString(ProcedimentoAtual p) {
  switch (p) {
    case PROC_IDLE:                  return "Sistema em repouso";
    case PROC_POSICIONANDO_VALVULAS: return "Posicionando válvulas";
    case PROC_ENCHENDO_TANQUE:       return "Enchendo o tanque";
    case PROC_AQUECENDO_AGUA:        return "Aquecendo a água";
    case PROC_DOSANDO_SOLUCAO:       return "Dosando solução";
    case PROC_CIRCULANDO:            return "Circulando pelo sistema";
    case PROC_DESCARTANDO:           return "Descartando líquido";
    case PROC_ENXAGUE_CICLO:         return "Realizando enxágue";
    case PROC_AGUARDANDO:            return "Aguardando condição";
    case PROC_CONCLUIDO:             return "Etapa concluída";
    default:                         return "Desconhecido";
  }
}

// ──────────────────────────────────────────────────────────────
//  Inicialização
// ──────────────────────────────────────────────────────────────
void cip_init() {
  sensors.begin();
  cipState = {};
  cipState.etapaAtual = ETAPA_IDLE;
  cipState.procAtual  = PROC_IDLE;
  Serial.println("[CIP] Inicializado");
}

// ──────────────────────────────────────────────────────────────
//  Cancelamento (parada segura imediata)
// ──────────────────────────────────────────────────────────────
void cip_cancelar() {
  actuators_safeState();
  cipState.rodando    = false;
  cipState.etapaAtual = ETAPA_IDLE;
  cipState.procAtual  = PROC_IDLE;
  _subEstado          = 0;
  _timerAtivo         = false;
  Serial.println("[CIP] 🛑 Ciclo cancelado");
}

// ──────────────────────────────────────────────────────────────
//  Iniciar ciclos
// ──────────────────────────────────────────────────────────────
bool cip_iniciarCIP() {
  if (!espnow_allModulesReady()) {
    Serial.println("[CIP] ❌ Módulos offline — bloqueado");
    return false;
  }
  if (cipState.rodando) return false;
  cipState.rodando    = true;
  cipState.tipoCiclo  = CICLO_CIP;
  _cipSeqIdx          = 0;
  _subEstado          = 0;
  cipState.etapaAtual = (EtapaCIP)CIP_SEQUENCIA[0];
  cipState.mensagemErro[0] = '\0';
  Serial.println("[CIP] ▶ Ciclo CIP iniciado");
  return true;
}

bool cip_iniciarCustom(const uint8_t* fila, uint8_t tam) {
  if (!espnow_allModulesReady()) {
    Serial.println("[CIP] ❌ Módulos offline — bloqueado");
    return false;
  }
  if (cipState.rodando || tam == 0) return false;
  cipState.rodando       = true;
  cipState.tipoCiclo     = CICLO_CUSTOM;
  cipState.filaCustomTam = tam;
  cipState.filaCustomIdx = 0;
  memcpy(cipState.filaCustom, fila, tam);
  _subEstado             = 0;
  cipState.etapaAtual    = (EtapaCIP)fila[0];
  cipState.mensagemErro[0] = '\0';
  Serial.printf("[CIP] ▶ Ciclo personalizado iniciado (%u etapas)\n", tam);
  return true;
}

void cip_setConfig(const CipConfig& cfg) { cipConfig = cfg; }

// ──────────────────────────────────────────────────────────────
//  MÁQUINA DE ESTADOS — chamada a cada loop()
// ──────────────────────────────────────────────────────────────

// Lê temperatura periodicamente sem bloquear
static void atualizarTemp() {
  static unsigned long _ultimaLeitura = 0;
  if (millis() - _ultimaLeitura >= 2000) {
    _ultimaLeitura = millis();
    cip_lerTemperatura();
  }
}

// Sub-rotina: encher tanque (não bloqueante)
// Retorna true quando concluído
static bool fsm_encherTanque(bool comResistencia, bool tanqueAquecimento) {
  switch (_subEstado) {
    case 0:
      setProc(PROC_ENCHENDO_TANQUE);
      valvula_ts(tanqueAquecimento);
      valvula_tm(!tanqueAquecimento);
      resistencia(false);
      _subEstado = 1;
      return false;
    case 1:
      // Aguarda bóia sinalizar tanque cheio
      if (tanqueAquecimento ? boia_solucao() : boia_mistura()) {
        valvula_ts(false);
        valvula_tm(false);
        _subEstado = 2;
        Serial.println("[CIP] Tanque cheio");
        iniciarTimer(TEMPO_POSICIONAMENTO_VALVULA);
      }
      return false;
    case 2:
      if (!timerExpirou()) return false;
      if (comResistencia) {
        setProc(PROC_AQUECENDO_AGUA);
        resistencia(true);
      }
      _subEstado = 0;
      return true;  // concluído
  }
  return true;
}

// Sub-rotina: esvaziar tanque (não bloqueante)
// Retorna true quando concluído
static bool fsm_esvaziarTanque(float tempAlvo) {
  switch (_subEstado) {
    case 0:
      setProc(PROC_AQUECENDO_AGUA);
      _subEstado = 1;
      return false;
    case 1:
      controlarResistencia(tempAlvo);
      if (temperaturaAtingida(tempAlvo)) {
        resistencia(false);
        setProc(PROC_DESCARTANDO);
        ordenha(true);
        iniciarTimer(2000);
        _subEstado = 2;
      }
      return false;
    case 2:
      if (!timerExpirou()) return false;
      iniciarTimer(TEMPO_ESVAZIAR_TANQUE);
      _subEstado = 3;
      return false;
    case 3:
      if (!timerExpirou()) return false;
      ordenha(false);
      _subEstado = 0;
      return true;
  }
  return true;
}

// Sub-rotina: dosar solução (não bloqueante)
// Retorna true quando concluído
static bool fsm_dosarSolucao(uint8_t tipo, uint16_t volMl) {
  switch (_subEstado) {
    case 0:
      setProc(PROC_DOSANDO_SOLUCAO);
      switch (tipo) {
        case 1: bomba_alc(true);  break;
        case 2: bomba_acid(true); break;
        case 3: bomba_sanit(true); break;
      }
      iniciarTimer(calcTempoSolucao(volMl));
      _subEstado = 1;
      return false;
    case 1:
      if (!timerExpirou()) return false;
      bombas_setEstado(HIGH, HIGH, HIGH);
      _subEstado = 0;
      return true;
  }
  return true;
}

// ── PRÉ-ENXÁGUE ──────────────────────────────────────────────
static void fsm_preEnxague() {
  static uint8_t fase = 0;
  static bool subDone = false;

  switch (fase) {
    case 0:
      setProc(PROC_POSICIONANDO_VALVULAS);
      valvula_vasao(true);     // direcionar para saída
      iniciarTimer(TEMPO_POSICIONAMENTO_VALVULA);
      fase = 1; _subEstado = 0; break;
    case 1:
      if (!timerExpirou()) return;
      subDone = false; fase = 2; _subEstado = 0; break;
    case 2:  // encher tanque de aquecimento COM resistência
      if (fsm_encherTanque(true, true)) { fase = 3; _subEstado = 0; }
      break;
    case 3:  // esvaziar com temperatura do pré-enxágue
      if (fsm_esvaziarTanque(cipConfig.tempPre)) {
        fase = 0; proximaEtapa();
      }
      break;
  }
}

// ── ALCALINA ─────────────────────────────────────────────────
static void fsm_alcalina() {
  static uint8_t fase = 0;

  switch (fase) {
    case 0:
      setProc(PROC_POSICIONANDO_VALVULAS);
      valvula_vasao(false);    // direcionar para tanque de mistura
      iniciarTimer(2000);
      fase = 1; break;
    case 1:
      if (!timerExpirou()) return;
      _subEstado = 0; fase = 2; break;
    case 2:  // dosar solução alcalina
      if (fsm_dosarSolucao(1, cipConfig.volAlc)) { fase = 3; _subEstado = 0; }
      break;
    case 3:  // encher tanque de aquecimento COM resistência
      if (fsm_encherTanque(true, true)) { fase = 4; _subEstado = 0; }
      break;
    case 4:  // aguardar temperatura e sucionar
      if (fsm_esvaziarTanque(cipConfig.tempAlc)) { fase = 5; _subEstado = 0; }
      break;
    case 5:
      setProc(PROC_POSICIONANDO_VALVULAS);
      valvula_ciclo(true);
      iniciarTimer(TEMPO_POSICIONAMENTO_VALVULA);
      fase = 6; break;
    case 6:
      if (!timerExpirou()) return;
      setProc(PROC_CIRCULANDO);
      ordenha(true);
      iniciarTimer(cipConfig.tempoCirculacaoMs);
      fase = 7; break;
    case 7:
      if (!timerExpirou()) return;
      ordenha(false);
      valvula_vasao(true);     // apontar para saída
      iniciarTimer(TEMPO_POSICIONAMENTO_VALVULA);
      fase = 8; break;
    case 8:
      if (!timerExpirou()) return;
      setProc(PROC_DESCARTANDO);
      ordenha(true);
      iniciarTimer(TEMPO_ESVAZIAR_TANQUE);
      fase = 9; break;
    case 9:
      if (!timerExpirou()) return;
      ordenha(false);
      fase = 0; proximaEtapa();
      break;
  }
}

// ── ENXÁGUE INTERMITENTE ─────────────────────────────────────
static void fsm_enxague() {
  static uint8_t fase = 0;

  switch (fase) {
    case 0:
      setProc(PROC_POSICIONANDO_VALVULAS);
      valvula_ciclo(true);
      valvula_vasao(true);
      iniciarTimer(TEMPO_POSICIONAMENTO_VALVULA);
      fase = 1; break;
    case 1:
      if (!timerExpirou()) return;
      _subEstado = 0; fase = 2; break;
    case 2:  // encher tanque de mistura SEM resistência
      if (fsm_encherTanque(false, false)) { fase = 3; _subEstado = 0; }
      break;
    case 3:  // esvaziar à temperatura atual
      if (fsm_esvaziarTanque(cipState.temperaturaAtual)) {
        fase = 0; proximaEtapa();
      }
      break;
  }
}

// ── ÁCIDA ─────────────────────────────────────────────────────
static void fsm_acida() {
  static uint8_t fase = 0;

  switch (fase) {
    case 0:
      setProc(PROC_POSICIONANDO_VALVULAS);
      valvula_vasao(false);
      iniciarTimer(2000);
      fase = 1; break;
    case 1:
      if (!timerExpirou()) return;
      _subEstado = 0; fase = 2; break;
    case 2:
      if (fsm_dosarSolucao(2, cipConfig.volAcid)) { fase = 3; _subEstado = 0; }
      break;
    case 3:
      if (fsm_encherTanque(true, true)) { fase = 4; _subEstado = 0; }
      break;
    case 4:
      if (fsm_esvaziarTanque(cipConfig.tempAcid)) { fase = 5; _subEstado = 0; }
      break;
    case 5:
      setProc(PROC_POSICIONANDO_VALVULAS);
      valvula_ciclo(true);
      iniciarTimer(TEMPO_POSICIONAMENTO_VALVULA);
      fase = 6; break;
    case 6:
      if (!timerExpirou()) return;
      setProc(PROC_CIRCULANDO);
      ordenha(true);
      iniciarTimer(cipConfig.tempoCirculacaoMs);
      fase = 7; break;
    case 7:
      if (!timerExpirou()) return;
      ordenha(false);
      valvula_vasao(true);
      iniciarTimer(TEMPO_POSICIONAMENTO_VALVULA);
      fase = 8; break;
    case 8:
      if (!timerExpirou()) return;
      setProc(PROC_DESCARTANDO);
      ordenha(true);
      iniciarTimer(TEMPO_ESVAZIAR_TANQUE);
      fase = 9; break;
    case 9:
      if (!timerExpirou()) return;
      ordenha(false);
      fase = 0; proximaEtapa();
      break;
  }
}

// ── SANITIZANTE ──────────────────────────────────────────────
static void fsm_sanitizante() {
  static uint8_t fase = 0;

  switch (fase) {
    case 0:
      _subEstado = 0; fase = 1; break;
    case 1:  // encher tanque de mistura
      if (fsm_encherTanque(false, false)) { fase = 2; _subEstado = 0; }
      break;
    case 2:
      setProc(PROC_POSICIONANDO_VALVULAS);
      valvula_ciclo(false);
      iniciarTimer(2000);
      fase = 3; break;
    case 3:
      if (!timerExpirou()) return;
      _subEstado = 0; fase = 4; break;
    case 4:
      if (fsm_dosarSolucao(3, cipConfig.volSanit)) { fase = 5; _subEstado = 0; }
      break;
    case 5:
      if (fsm_esvaziarTanque(cipState.temperaturaAtual)) {
        fase = 0; proximaEtapa();
      }
      break;
  }
}

// ── Loop principal ────────────────────────────────────────────
void cip_loop() {
  atualizarTemp();
  if (!cipState.rodando) return;

  // Segurança: módulo offline → cancelar
  if (!espnow_allModulesReady()) {
    snprintf(cipState.mensagemErro, sizeof(cipState.mensagemErro),
             "Módulo desconectado durante o ciclo");
    setProc(PROC_IDLE);
    cipState.etapaAtual = ETAPA_ERRO;
    cip_cancelar();
    return;
  }

  // Segurança: temperatura crítica → desliga resistência
  if (cipState.temperaturaAtual > TEMP_MAX_SEGURANCA) resistencia(false);

  switch (cipState.etapaAtual) {
    case ETAPA_PRE_ENXAGUE:    fsm_preEnxague();   break;
    case ETAPA_ALCALINA:       fsm_alcalina();     break;
    case ETAPA_ENXAGUE_INTERM: fsm_enxague();      break;
    case ETAPA_ACIDA:          fsm_acida();        break;
    case ETAPA_SANITIZANTE:    fsm_sanitizante();  break;
    case ETAPA_FINALIZADO:
      cipState.rodando   = false;
      cipState.procAtual = PROC_CONCLUIDO;
      actuators_safeState();
      Serial.println("[CIP] ✅ Ciclo concluído!");
      break;
    case ETAPA_ERRO:
      cip_cancelar();
      break;
    default: break;
  }
}

// ──────────────────────────────────────────────────────────────
//  JSON para SSE
// ──────────────────────────────────────────────────────────────
String cip_getStatusJson() {
  String j = "{";
  j += "\"rodando\":"    + String(cipState.rodando ? "true" : "false")  + ",";
  j += "\"tipoCiclo\":"  + String((int)cipState.tipoCiclo)              + ",";
  j += "\"etapa\":"      + String((int)cipState.etapaAtual)             + ",";
  j += "\"etapaNome\":\"" + String(etapaToString(cipState.etapaAtual))  + "\",";
  j += "\"proc\":"       + String((int)cipState.procAtual)              + ",";
  j += "\"procNome\":\"" + String(procedimentoToString(cipState.procAtual)) + "\",";
  j += "\"temp\":"       + String(cipState.temperaturaAtual, 1)         + ",";
  j += "\"erro\":\""     + String(cipState.mensagemErro)                + "\",";
  // Fila custom
  j += "\"filaCustom\":[";
  for (uint8_t i = 0; i < cipState.filaCustomTam; i++) {
    if (i) j += ",";
    j += String(cipState.filaCustom[i]);
  }
  j += "],\"filaCustomIdx\":" + String(cipState.filaCustomIdx);
  j += "}";
  return j;
}