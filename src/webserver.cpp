// ╔══════════════════════════════════════════════════════════════╗
// ║    WEB_SERVER.CPP — AP, Captive Portal, Rotas REST + SSE    ║
// ║                                                              ║
// ║  MUDANÇAS v2:                                               ║
// ║   • LittleFS removido — index.html embedado em PROGMEM      ║
// ║   • Captive portal reforçado (iOS/Android/Windows)           ║
// ║   • POST handlers com body buffer manual (sem               ║
// ║     AsyncCallbackJsonWebHandler que falha em body vazio)     ║
// ╚══════════════════════════════════════════════════════════════╝
#include "web_server.h"
#include "cip_cycle.h"
#include "espnow_manager.h"
#include "storage.h"
#include "config.h"

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>

static AsyncWebServer server(80);
static AsyncEventSource events("/events");
static DNSServer dnsServer;

// ──────────────────────────────────────────────────────────────
//  HTML embedado (index.html em PROGMEM)
//  Gerado a partir do arquivo index.html do projeto
// ──────────────────────────────────────────────────────────────
static const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1"/>
<title>CIP — Ordenhadeira</title>
<style>
:root{
  --bg:#0d1117;--surface:#161b22;--border:#30363d;
  --green:#3fb950;--green-dim:#1a3a22;
  --orange:#e3a84a;--orange-dim:#3a2a10;
  --red:#f85149;--red-dim:#3a1010;
  --blue:#58a6ff;--blue-dim:#0d2040;
  --purple:#bc8cff;--purple-dim:#2a1545;
  --text:#e6edf3;--muted:#8b949e;
  --mono:'Share Tech Mono',monospace;
  --sans:'Barlow',sans-serif;
}
*{box-sizing:border-box;margin:0;padding:0}
html{background:var(--bg);color:var(--text);font-family:var(--sans)}
body{background:var(--bg)}
::-webkit-scrollbar{width:5px}
::-webkit-scrollbar-thumb{background:var(--border);border-radius:3px}
.app{display:flex;flex-direction:column;min-height:100dvh}
header{display:flex;align-items:center;gap:10px;padding:12px 18px;
  background:var(--surface);border-bottom:1px solid var(--border);
  position:sticky;top:0;z-index:100}
.logo{font-family:var(--mono);font-size:.95rem;letter-spacing:2px;color:var(--green)}
.subtitle{font-size:.7rem;color:var(--muted)}
.dot-live{margin-left:auto;width:8px;height:8px;border-radius:50%;
  background:var(--green);box-shadow:0 0 6px var(--green);
  animation:pulse 2s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.3}}
nav{display:flex;background:var(--surface);border-bottom:1px solid var(--border);overflow-x:auto}
nav button{flex:1;min-width:80px;padding:11px 6px;background:none;border:none;
  border-bottom:2px solid transparent;color:var(--muted);font-family:var(--sans);
  font-size:.72rem;font-weight:600;letter-spacing:.8px;text-transform:uppercase;
  cursor:pointer;transition:all .2s;white-space:nowrap}
nav button.active{color:var(--text);border-bottom-color:var(--green)}
.screen{display:none;padding:18px;gap:14px;flex-direction:column;
  overflow-y:auto;-webkit-overflow-scrolling:touch}
.screen.active{display:flex}
.card{background:var(--surface);border:1px solid var(--border);border-radius:8px;padding:14px}
.card-title{font-family:var(--mono);font-size:.68rem;letter-spacing:2px;
  color:var(--muted);text-transform:uppercase;margin-bottom:12px;
  padding-bottom:7px;border-bottom:1px solid var(--border)}
.modules-row{display:flex;gap:10px}
.mod-badge{flex:1;display:flex;flex-direction:column;align-items:center;
  gap:7px;padding:12px 8px;border-radius:6px;border:1px solid var(--border);transition:all .3s}
.mod-badge.online{background:var(--green-dim);border-color:var(--green)}
.mod-badge.offline{background:var(--red-dim);border-color:var(--red)}
.mod-badge .dot{width:12px;height:12px;border-radius:50%}
.mod-badge.online .dot{background:var(--green);box-shadow:0 0 7px var(--green)}
.mod-badge.offline .dot{background:var(--red);box-shadow:0 0 7px var(--red);animation:blink .8s infinite}
@keyframes blink{0%,100%{opacity:1}50%{opacity:.2}}
.mod-name{font-family:var(--mono);font-size:.8rem}
.mod-st{font-size:.62rem;font-weight:700;letter-spacing:1px;text-transform:uppercase}
.mod-badge.online .mod-st{color:var(--green)}
.mod-badge.offline .mod-st{color:var(--red)}
.temp-big{display:flex;align-items:baseline;gap:5px}
.temp-val{font-family:var(--mono);font-size:2.6rem;color:var(--orange);line-height:1}
.temp-unit{font-size:1.1rem;color:var(--muted)}
.lbl{font-size:.66rem;letter-spacing:1.5px;color:var(--muted);text-transform:uppercase;margin-bottom:4px}
.val-blue{font-family:var(--mono);font-size:.95rem;color:var(--blue)}
.val-orange{font-size:.88rem;color:var(--orange)}
.prog-wrap{height:3px;background:var(--border);border-radius:2px;overflow:hidden;margin-top:5px}
.prog-bar{height:100%;background:var(--blue);border-radius:2px;transition:width .5s}
.btn-cip-wrap{display:flex;flex-direction:column;align-items:center;gap:12px}
.btn-cip{position:relative;width:130px;height:130px;border-radius:50%;
  border:3px solid var(--border);background:var(--surface);
  display:flex;flex-direction:column;align-items:center;justify-content:center;
  gap:5px;cursor:pointer;-webkit-tap-highlight-color:transparent;user-select:none;
  transition:border-color .2s}
.btn-cip svg{width:32px;height:32px}
.btn-cip .btn-lbl{font-family:var(--mono);font-size:.65rem;letter-spacing:2px;color:var(--muted)}
.ring{position:absolute;inset:-4px;border-radius:50%;border:4px solid transparent;
  border-top-color:var(--green);transform:rotate(-90deg);opacity:0;transition:none}
.btn-cip.holding .ring{opacity:1}
.btn-cip.idle{border-color:var(--green)}
.btn-cip.idle svg path{stroke:var(--green)}
.btn-cip.idle .btn-lbl{color:var(--green)}
.btn-cip.running{border-color:var(--orange)}
.btn-cip.running svg path{stroke:var(--orange)}
.btn-cip.running .btn-lbl{color:var(--orange)}
.btn-cip.blocked{opacity:.45;cursor:not-allowed}
.hold-hint{font-size:.68rem;color:var(--muted);font-family:var(--mono);letter-spacing:1px;text-align:center}
.btn-cancel-wrap{display:none}
.btn-cancel-wrap.visible{display:flex;flex-direction:column;align-items:center;gap:8px;width:100%}
.btn-cancel{width:100%;padding:13px;background:var(--red-dim);border:1px solid var(--red);
  color:var(--red);border-radius:6px;font-family:var(--mono);font-size:.78rem;letter-spacing:2px;
  cursor:pointer;position:relative;overflow:hidden;user-select:none;-webkit-tap-highlight-color:transparent}
.cancel-fill{position:absolute;left:0;top:0;height:100%;width:0%;
  background:rgba(248,81,73,.18);transition:none;pointer-events:none}
.cancel-hint{font-size:.65rem;color:var(--red);font-family:var(--mono);opacity:.7}
.err-banner{display:none;padding:9px 12px;background:var(--red-dim);
  border:1px solid var(--red);border-radius:6px;font-size:.78rem;
  color:var(--red);font-family:var(--mono)}
.err-banner.visible{display:block}
.rotinas-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.rotina-btn{display:flex;align-items:center;gap:10px;padding:11px 12px;
  border-radius:6px;border:1.5px solid var(--border);background:var(--surface);
  cursor:pointer;transition:all .15s;-webkit-tap-highlight-color:transparent}
.rotina-btn:active{transform:scale(.97)}
.rotina-btn .rnum{width:28px;height:28px;border-radius:6px;display:flex;
  align-items:center;justify-content:center;font-family:var(--mono);
  font-size:.85rem;font-weight:700;flex-shrink:0}
.rotina-btn .rlbl{font-size:.82rem;color:var(--text);font-weight:500}
.r1{border-color:#4a6a8a;background:#0d1f2d}.r1 .rnum{background:#1a3a55;color:#7ab8e8}
.r2{border-color:#3a6a4a;background:#0d1f15}.r2 .rnum{background:#1a4030;color:#7ad888}
.r3{border-color:#5a4a8a;background:#1a0d2d}.r3 .rnum{background:#2a1a55;color:#bc8cff}
.r4{border-color:#8a5a20;background:#2a1a05}.r4 .rnum{background:#4a2a10;color:var(--orange)}
.r5{border-color:#4a8a8a;background:#0d2020}.r5 .rnum{background:#1a4040;color:#7ae8e8}
.fila-label{font-size:.68rem;letter-spacing:1.5px;color:var(--muted);
  text-transform:uppercase;margin-bottom:8px}
.fila-container{min-height:48px;padding:8px;background:var(--bg);
  border:1px solid var(--border);border-radius:6px;
  display:flex;flex-wrap:wrap;gap:6px;align-content:flex-start}
.fila-item{display:flex;align-items:center;gap:5px;padding:5px 9px;
  border-radius:4px;font-family:var(--mono);font-size:.75rem;font-weight:600;
  cursor:pointer;transition:opacity .15s}
.fila-item:hover{opacity:.7}
.fila-item .fi-x{font-size:.6rem;color:rgba(255,255,255,.5)}
.fila-vazia{font-size:.75rem;color:var(--muted);font-family:var(--mono);
  padding:8px 0;width:100%;text-align:center}
.fi1{background:#1a3a55;color:#7ab8e8;border:1px solid #4a6a8a}
.fi2{background:#1a4030;color:#7ad888;border:1px solid #3a6a4a}
.fi3{background:#2a1a55;color:#bc8cff;border:1px solid #5a4a8a}
.fi4{background:#4a2a10;color:var(--orange);border:1px solid #8a5a20}
.fi5{background:#1a4040;color:#7ae8e8;border:1px solid #4a8a8a}
.fila-count{font-size:.7rem;color:var(--muted);font-family:var(--mono);
  text-align:right;margin-top:4px}
.btn-row{display:flex;gap:8px;margin-top:4px}
.btn-sm{flex:1;padding:11px;border-radius:6px;font-family:var(--mono);
  font-size:.75rem;letter-spacing:1px;cursor:pointer;border:1px solid;
  transition:background .2s}
.btn-iniciar-custom{background:var(--green-dim);border-color:var(--green);color:var(--green)}
.btn-iniciar-custom:disabled{opacity:.4;cursor:not-allowed}
.btn-limpar{background:var(--red-dim);border-color:var(--red);color:var(--red)}
.btn-salvar-fila{background:var(--blue-dim);border-color:var(--blue);color:var(--blue)}
.cfg-section-title{font-family:var(--mono);font-size:.63rem;letter-spacing:2px;
  color:var(--muted);text-transform:uppercase;margin-bottom:10px}
.cfg-divider{border-top:1px solid var(--border);padding-top:14px;margin-top:8px}
/* Layout de cada linha: label em cima, input+unidade embaixo */
.cfg-row{display:flex;flex-direction:column;gap:5px;margin-bottom:14px}
.cfg-row label{font-size:.78rem;color:var(--muted)}
.cfg-input-wrap{display:flex;align-items:center;gap:8px}
.cfg-input-wrap input[type=number]{
  flex:1;padding:10px 12px;background:var(--bg);border:1px solid var(--border);
  border-radius:6px;color:var(--text);font-family:var(--mono);font-size:1rem;
  outline:none;-moz-appearance:textfield;appearance:textfield;
  transition:border-color .2s}
.cfg-input-wrap input[type=number]:focus{border-color:var(--blue)}
.cfg-input-wrap input[type=number]::-webkit-outer-spin-button,
.cfg-input-wrap input[type=number]::-webkit-inner-spin-button{-webkit-appearance:none;margin:0}
.cfg-unit{font-size:.8rem;color:var(--muted);min-width:28px}
.btn-save{width:100%;padding:12px;background:var(--green-dim);border:1px solid var(--green);
  color:var(--green);border-radius:6px;font-family:var(--mono);font-size:.78rem;
  letter-spacing:2px;cursor:pointer;transition:background .2s;margin-top:4px}
.save-ok{display:none;text-align:center;font-size:.72rem;color:var(--green);
  margin-top:6px;font-family:var(--mono)}
.save-ok.visible{display:block}
</style>
</head>
<body>
<div class="app">
<header>
  <div>
    <div class="logo">&#9672; CIP / ORDENHA</div>
    <div class="subtitle">IFF Bom Jesus do Itabapoana</div>
  </div>
  <div class="dot-live" id="dotLive"></div>
</header>
<nav>
  <button class="active" onclick="tab('home',this)">In&#237;cio</button>
  <button onclick="tab('ciclo',this)">Ciclo CIP</button>
  <button onclick="tab('custom',this)">Personalizado</button>
  <button onclick="tab('config',this)">Config</button>
</nav>
<!-- TELA: INICIO -->
<div class="screen active" id="screen-home">
  <div class="card">
    <div class="card-title">Conex&#227;o dos M&#243;dulos</div>
    <div class="modules-row">
      <div class="mod-badge offline" id="hbadge-ORDENHA">
        <div class="dot"></div><div class="mod-name">ORDENHA</div>
        <div class="mod-st">offline</div>
      </div>
      <div class="mod-badge offline" id="hbadge-MOTOR">
        <div class="dot"></div><div class="mod-name">MOTOR</div>
        <div class="mod-st">offline</div>
      </div>
    </div>
  </div>
  <div class="card">
    <div class="card-title">Temperatura do Tanque</div>
    <div class="temp-big">
      <div class="temp-val" id="h-temp">--.-</div>
      <div class="temp-unit">&#176;C</div>
    </div>
  </div>
  <div class="card">
    <div class="card-title">Estado do Sistema</div>
    <div class="lbl">Etapa</div>
    <div class="val-blue" id="h-etapa">Aguardando</div>
    <div style="margin-top:10px" class="lbl">Procedimento</div>
    <div class="val-orange" id="h-proc">Sistema em repouso</div>
  </div>
</div>
<!-- TELA: CICLO CIP -->
<div class="screen" id="screen-ciclo">
  <div class="card">
    <div class="card-title">Iniciar Ciclo CIP</div>
    <div class="btn-cip-wrap">
      <button class="btn-cip blocked" id="btnCip"
              onpointerdown="holdCipStart()" onpointerup="holdCipEnd()"
              onpointerleave="holdCipEnd()">
        <div class="ring" id="cipRing"></div>
        <svg viewBox="0 0 24 24" fill="none">
          <path id="cipIco" d="M5 3l14 9-14 9V3z"
                stroke="#8b949e" stroke-width="1.8" stroke-linejoin="round" fill="none"/>
        </svg>
        <div class="btn-lbl" id="cipLbl">AGUARDANDO</div>
      </button>
      <div class="hold-hint" id="cipHint">Aguardando conex&#227;o dos m&#243;dulos</div>
      <div class="btn-cancel-wrap" id="cipCancelWrap">
        <button class="btn-cancel" id="btnCancelCip"
                onpointerdown="holdCancelStart('cip')" onpointerup="holdCancelEnd('cip')"
                onpointerleave="holdCancelEnd('cip')">
          <div class="cancel-fill" id="cancelFillCip"></div>
          &#9632; SEGURAR PARA CANCELAR
        </button>
        <div class="cancel-hint">Segure 2s para parar o ciclo imediatamente</div>
      </div>
    </div>
  </div>
  <div class="card">
    <div class="card-title">M&#243;dulos</div>
    <div class="modules-row">
      <div class="mod-badge offline" id="cbadge-ORDENHA">
        <div class="dot"></div><div class="mod-name" style="font-size:.75rem">ORDENHA</div>
        <div class="mod-st">offline</div>
      </div>
      <div class="mod-badge offline" id="cbadge-MOTOR">
        <div class="dot"></div><div class="mod-name" style="font-size:.75rem">MOTOR</div>
        <div class="mod-st">offline</div>
      </div>
    </div>
  </div>
  <div class="card">
    <div class="card-title">Monitoramento em Tempo Real</div>
    <div class="lbl">Etapa do Ciclo</div>
    <div class="val-blue" id="c-etapa">&#8212;</div>
    <div class="prog-wrap"><div class="prog-bar" id="cProg" style="width:0%"></div></div>
    <div style="margin-top:12px" class="lbl">Procedimento</div>
    <div class="val-orange" id="c-proc">&#8212;</div>
    <div style="margin-top:12px" class="lbl">Temperatura</div>
    <div class="temp-big">
      <div class="temp-val" style="font-size:1.7rem" id="c-temp">--.-</div>
      <div class="temp-unit">&#176;C</div>
    </div>
  </div>
  <div class="err-banner" id="cipErr"></div>
</div>
<!-- TELA: CICLO PERSONALIZADO -->
<div class="screen" id="screen-custom">
  <div class="card">
    <div class="card-title">Selecionar Etapas</div>
    <div class="rotinas-grid">
      <button class="rotina-btn r1" onclick="addEtapa(1,'Pr&#233;-enx&#225;gue','fi1')">
        <div class="rnum">1</div><div class="rlbl">Pr&#233;-enx&#225;gue</div>
      </button>
      <button class="rotina-btn r2" onclick="addEtapa(2,'Enx&#225;gue','fi2')">
        <div class="rnum">2</div><div class="rlbl">Enx&#225;gue</div>
      </button>
      <button class="rotina-btn r3" onclick="addEtapa(3,'Alcalina','fi3')">
        <div class="rnum">3</div><div class="rlbl">Alcalina</div>
      </button>
      <button class="rotina-btn r4" onclick="addEtapa(4,'&#193;cida','fi4')">
        <div class="rnum">4</div><div class="rlbl">&#193;cida</div>
      </button>
      <button class="rotina-btn r5" onclick="addEtapa(5,'Sanitizante','fi5')" style="grid-column:span 2">
        <div class="rnum">5</div><div class="rlbl">Sanitizante</div>
      </button>
    </div>
  </div>
  <div class="card">
    <div class="card-title">Fila Criada</div>
    <div class="fila-label">Toque em uma etapa para remov&#234;-la</div>
    <div class="fila-container" id="filaContainer">
      <div class="fila-vazia" id="filaVazia">Nenhuma etapa adicionada</div>
    </div>
    <div class="fila-count" id="filaCount">0 / 10</div>
    <div class="btn-row" style="margin-top:10px">
      <button class="btn-sm btn-limpar" onclick="limparFila()">&#10005; LIMPAR</button>
      <button class="btn-sm btn-salvar-fila" onclick="salvarFila()">&#128190; SALVAR</button>
    </div>
    <div class="btn-row">
      <button class="btn-sm btn-iniciar-custom" id="btnIniciarCustom"
              disabled onpointerdown="holdCustomStart()" onpointerup="holdCustomEnd()"
              onpointerleave="holdCustomEnd()">
        <div style="position:relative;display:inline-block;width:100%">
          <div class="cancel-fill" id="customFill" style="border-radius:4px"></div>
          &#9654; SEGURAR 2S PARA INICIAR
        </div>
      </button>
    </div>
    <div class="btn-cancel-wrap" id="customCancelWrap" style="margin-top:8px">
      <button class="btn-cancel" id="btnCancelCustom"
              onpointerdown="holdCancelStart('custom')" onpointerup="holdCancelEnd('custom')"
              onpointerleave="holdCancelEnd('custom')">
        <div class="cancel-fill" id="cancelFillCustom"></div>
        &#9632; SEGURAR PARA CANCELAR
      </button>
      <div class="cancel-hint">Segure 2s para parar o ciclo imediatamente</div>
    </div>
  </div>
  <div class="card" id="customMonitor" style="display:none">
    <div class="card-title">Execu&#231;&#227;o em Andamento</div>
    <div class="lbl">Etapa</div>
    <div class="val-blue" id="cu-etapa">&#8212;</div>
    <div class="prog-wrap"><div class="prog-bar" id="cuProg" style="width:0%"></div></div>
    <div style="margin-top:10px" class="lbl">Procedimento</div>
    <div class="val-orange" id="cu-proc">&#8212;</div>
    <div style="margin-top:12px" class="lbl">Progresso da Fila</div>
    <div class="fila-container" id="filaExec" style="pointer-events:none"></div>
  </div>
  <div class="err-banner" id="customErr"></div>
</div>
<!-- TELA: CONFIGURACOES -->
<div class="screen" id="screen-config">
  <div class="card">
    <div class="card-title">Configura&#231;&#245;es do Ciclo CIP</div>

    <div class="cfg-section-title">Volumes de Solu&#231;&#227;o</div>
    <div class="cfg-row">
      <label>Volume Alcalina</label>
      <div class="cfg-input-wrap">
        <input type="number" id="r-volAlc" min="0" max="300" step="5"
               value="75" oninput="sv('volAlc',this.value)">
        <span class="cfg-unit">mL</span>
      </div>
    </div>
    <div class="cfg-row">
      <label>Volume &#193;cida</label>
      <div class="cfg-input-wrap">
        <input type="number" id="r-volAcid" min="0" max="300" step="5"
               value="130" oninput="sv('volAcid',this.value)">
        <span class="cfg-unit">mL</span>
      </div>
    </div>
    <div class="cfg-row">
      <label>Volume Sanitizante</label>
      <div class="cfg-input-wrap">
        <input type="number" id="r-volSanit" min="0" max="300" step="5"
               value="0" oninput="sv('volSanit',this.value)">
        <span class="cfg-unit">mL</span>
      </div>
    </div>

    <div class="cfg-divider">
      <div class="cfg-section-title">Temperaturas Alvo</div>
      <div class="cfg-row">
        <label>Temperatura Pr&#233;-enx&#225;gue</label>
        <div class="cfg-input-wrap">
          <input type="number" id="r-tempPre" min="20" max="80" step="1"
                 value="39" oninput="sv('tempPre',this.value)">
          <span class="cfg-unit">&#176;C</span>
        </div>
      </div>
      <div class="cfg-row">
        <label>Temperatura Alcalina</label>
        <div class="cfg-input-wrap">
          <input type="number" id="r-tempAlc" min="20" max="85" step="1"
                 value="39" oninput="sv('tempAlc',this.value)">
          <span class="cfg-unit">&#176;C</span>
        </div>
      </div>
      <div class="cfg-row">
        <label>Temperatura &#193;cida</label>
        <div class="cfg-input-wrap">
          <input type="number" id="r-tempAcid" min="20" max="80" step="1"
                 value="39" oninput="sv('tempAcid',this.value)">
          <span class="cfg-unit">&#176;C</span>
        </div>
      </div>
    </div>

    <div class="cfg-divider">
      <div class="cfg-section-title">Tempo de Circula&#231;&#227;o</div>
      <div class="cfg-row">
        <label>Dura&#231;&#227;o da circula&#231;&#227;o</label>
        <div class="cfg-input-wrap">
          <input type="number" id="r-tempoCirculacaoMin" min="1" max="60" step="1"
                 value="5" oninput="sv('tempoCirculacaoMin',this.value)">
          <span class="cfg-unit">min</span>
        </div>
      </div>
    </div>
  </div>
  <button class="btn-save" onclick="salvarConfig()">&#10004; SALVAR CONFIGURA&#199;&#213;ES</button>
  <div class="save-ok" id="saveOk">Configura&#231;&#245;es salvas!</div>
</div>
</div>
<script>
let S={running:false,allReady:false,etapa:0,tipoCiclo:0,filaCustom:[],filaCustomIdx:0};
const NOMES=['','Pr\u00e9-enx\u00e1gue','Enx\u00e1gue','Alcalina','\u00c1cida','Sanitizante'];
const FCLR=['','fi1','fi2','fi3','fi4','fi5'];
let fila=[];
function tab(id,btn){
  document.querySelectorAll('.screen').forEach(s=>s.classList.remove('active'));
  document.querySelectorAll('nav button').forEach(b=>b.classList.remove('active'));
  document.getElementById('screen-'+id).classList.add('active');
  btn.classList.add('active');
}
const sse=new EventSource('/events');
sse.addEventListener('status',e=>{
  try{const d=JSON.parse(e.data);updateUI(d.cip,d.espnow);}catch(_){}
});
sse.onerror=()=>{
  document.getElementById('dotLive').style.cssText='background:var(--red);box-shadow:0 0 6px var(--red)';
};
setInterval(()=>{
  if(sse.readyState===2)
    fetch('/api/status').then(r=>r.json()).then(d=>updateUI(d.cip,d.espnow));
},4000);
function updateUI(cip,espnow){
  if(!cip||!espnow)return;
  espnow.modules.forEach(m=>{
    ['hbadge-','cbadge-'].forEach(p=>{
      const el=document.getElementById(p+m.name);
      if(!el)return;
      el.className='mod-badge '+(m.active?'online':'offline');
      el.querySelector('.mod-st').textContent=m.active?'online':'offline';
    });
  });
  S.allReady=espnow.allReady;
  const ts=(cip.temp>=0)?cip.temp.toFixed(1):'--.-';
  ['h-temp','c-temp'].forEach(id=>document.getElementById(id).textContent=ts);
  S.running=cip.rodando;S.etapa=cip.etapa;S.tipoCiclo=cip.tipoCiclo;
  S.filaCustom=cip.filaCustom||[];S.filaCustomIdx=cip.filaCustomIdx||0;
  setText('h-etapa',cip.etapaNome||'Aguardando');
  setText('h-proc',cip.procNome||'Sistema em repouso');
  setText('c-etapa',cip.etapaNome||'\u2014');
  setText('c-proc',cip.procNome||'\u2014');
  setText('cu-etapa',cip.etapaNome||'\u2014');
  setText('cu-proc',cip.procNome||'\u2014');
  const pct=Math.round((cip.etapa/6)*100);
  setW('cProg',pct);setW('cuProg',pct);
  setErr('cipErr',cip.erro);setErr('customErr',cip.erro);
  updateBtnCip();
  const showCancel=S.running&&S.tipoCiclo===1;
  const showCustomCancel=S.running&&S.tipoCiclo===2;
  toggleClass('cipCancelWrap','visible',showCancel);
  toggleClass('customCancelWrap','visible',showCustomCancel);
  const mon=document.getElementById('customMonitor');
  if(mon)mon.style.display=(S.running&&S.tipoCiclo===2)?'block':'none';
  renderFilaExec();
  const btnCust=document.getElementById('btnIniciarCustom');
  if(btnCust)btnCust.disabled=(!S.allReady||S.running||fila.length===0);
}
function setText(id,v){const el=document.getElementById(id);if(el)el.textContent=v;}
function setW(id,pct){const el=document.getElementById(id);if(el)el.style.width=pct+'%';}
function setErr(id,msg){
  const el=document.getElementById(id);if(!el)return;
  if(msg&&msg.length>0){el.textContent='\u26a0 '+msg;el.classList.add('visible');}
  else el.classList.remove('visible');
}
function toggleClass(id,cls,on){const el=document.getElementById(id);if(el)el.classList.toggle(cls,on);}
function updateBtnCip(){
  const btn=document.getElementById('btnCip');
  const lbl=document.getElementById('cipLbl');
  const ico=document.getElementById('cipIco');
  const hint=document.getElementById('cipHint');
  if(!btn)return;
  if(S.running&&S.tipoCiclo===1){
    btn.className='btn-cip running';lbl.textContent='RODANDO';
    ico.setAttribute('d','M6 6h4v12H6zm8 0h4v12h-4z');
    hint.textContent='Ciclo CIP em execu\u00e7\u00e3o';
  }else if(!S.allReady){
    btn.className='btn-cip blocked';lbl.textContent='AGUARDANDO';lbl.style.color='';
    ico.setAttribute('d','M5 3l14 9-14 9V3z');
    hint.textContent='Aguardando conex\u00e3o dos m\u00f3dulos';
  }else if(S.running){
    btn.className='btn-cip blocked';lbl.textContent='EM USO';lbl.style.color='var(--muted)';
    ico.setAttribute('d','M5 3l14 9-14 9V3z');
    hint.textContent='Ciclo personalizado em execu\u00e7\u00e3o';
  }else{
    btn.className='btn-cip idle';lbl.textContent='INICIAR';
    ico.setAttribute('d','M5 3l14 9-14 9V3z');
    hint.textContent='Segure 2s para iniciar o ciclo CIP';
  }
}
let holdCipT=null,holdCipRaf=null,holdCipMs=0;
function holdCipStart(){
  if(!S.allReady||S.running)return;
  holdCipMs=performance.now();
  document.getElementById('btnCip').classList.add('holding');
  const ring=document.getElementById('cipRing');
  function af(){
    const d=Math.min(((performance.now()-holdCipMs)/2000)*360,360);
    ring.style.transform='rotate('+(d-90)+'deg)';
    if(performance.now()-holdCipMs<2000)holdCipRaf=requestAnimationFrame(af);
  }
  holdCipRaf=requestAnimationFrame(af);
  holdCipT=setTimeout(()=>{iniciarCIP();holdCipEnd();},2000);
}
function holdCipEnd(){
  clearTimeout(holdCipT);cancelAnimationFrame(holdCipRaf);
  const ring=document.getElementById('cipRing');
  if(ring)ring.style.transform='rotate(-90deg)';
  const btn=document.getElementById('btnCip');
  if(btn)btn.classList.remove('holding');
}
let holdCancelT={cip:null,custom:null};
let holdCancelRaf={cip:null,custom:null};
function holdCancelStart(tipo){
  const fillId=tipo==='cip'?'cancelFillCip':'cancelFillCustom';
  const fill=document.getElementById(fillId);
  const t0=performance.now();
  function af(){
    const pct=Math.min(((performance.now()-t0)/2000)*100,100);
    if(fill)fill.style.width=pct+'%';
    if(performance.now()-t0<2000)holdCancelRaf[tipo]=requestAnimationFrame(af);
  }
  holdCancelRaf[tipo]=requestAnimationFrame(af);
  holdCancelT[tipo]=setTimeout(()=>{cancelarCiclo();holdCancelEnd(tipo);},2000);
}
function holdCancelEnd(tipo){
  clearTimeout(holdCancelT[tipo]);cancelAnimationFrame(holdCancelRaf[tipo]);
  const fillId=tipo==='cip'?'cancelFillCip':'cancelFillCustom';
  const fill=document.getElementById(fillId);
  if(fill)fill.style.width='0%';
}
let holdCustomT=null,holdCustomRaf=null;
function holdCustomStart(){
  if(!S.allReady||S.running||fila.length===0)return;
  const fill=document.getElementById('customFill');
  const t0=performance.now();
  function af(){
    const pct=Math.min(((performance.now()-t0)/2000)*100,100);
    if(fill)fill.style.width=pct+'%';
    if(performance.now()-t0<2000)holdCustomRaf=requestAnimationFrame(af);
  }
  holdCustomRaf=requestAnimationFrame(af);
  holdCustomT=setTimeout(()=>{iniciarCustom();holdCustomEnd();},2000);
}
function holdCustomEnd(){
  clearTimeout(holdCustomT);cancelAnimationFrame(holdCustomRaf);
  const fill=document.getElementById('customFill');
  if(fill)fill.style.width='0%';
}
function iniciarCIP(){
  fetch('/api/cip/iniciar',{method:'POST'})
    .then(r=>r.json()).then(d=>{if(!d.ok)alert('Erro: '+(d.msg||'?'));});
}
function cancelarCiclo(){fetch('/api/cip/cancelar',{method:'POST'});}
function iniciarCustom(){
  fetch('/api/cip/custom',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify({fila:fila.map(f=>f.num),salvar:false})
  }).then(r=>r.json()).then(d=>{if(!d.ok)alert('Erro: '+(d.msg||'?'));});
}
function addEtapa(num,nome,cls){
  if(fila.length>=10)return;
  fila.push({num,nome,cls});renderFila();
}
function removeEtapa(idx){fila.splice(idx,1);renderFila();}
function limparFila(){fila=[];renderFila();}
function salvarFila(){
  if(fila.length===0)return;
  fetch('/api/cip/custom',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify({fila:fila.map(f=>f.num),salvar:true})
  }).then(r=>r.json()).then(d=>{if(d.ok)showSaveOk('Fila salva na mem\u00f3ria!');});
}
function renderFila(){
  const cont=document.getElementById('filaContainer');
  const vazia=document.getElementById('filaVazia');
  const count=document.getElementById('filaCount');
  const btn=document.getElementById('btnIniciarCustom');
  cont.querySelectorAll('.fila-item').forEach(el=>el.remove());
  if(fila.length===0){if(vazia)vazia.style.display='block';}
  else{
    if(vazia)vazia.style.display='none';
    fila.forEach((f,i)=>{
      const el=document.createElement('div');
      el.className='fila-item '+f.cls;
      el.innerHTML='<span>'+f.num+'\u00b7'+f.nome+'</span><span class="fi-x">\u2715</span>';
      el.onclick=()=>removeEtapa(i);
      cont.appendChild(el);
    });
  }
  if(count)count.textContent=fila.length+' / 10';
  if(btn)btn.disabled=(!S.allReady||S.running||fila.length===0);
}
function renderFilaExec(){
  const cont=document.getElementById('filaExec');
  if(!cont||!S.running||S.tipoCiclo!==2)return;
  cont.innerHTML='';
  S.filaCustom.forEach((num,i)=>{
    const cls=FCLR[num]||'';
    const nome=NOMES[num]||'?';
    const el=document.createElement('div');
    el.className='fila-item '+cls;
    if(i===S.filaCustomIdx)el.style.cssText='outline:2px solid white;outline-offset:1px';
    el.textContent=num+'\u00b7'+nome;
    cont.appendChild(el);
  });
}
const CFG_KEYS=['volAlc','volAcid','volSanit','tempPre','tempAlc','tempAcid','tempoCirculacaoMin'];
let cfgLocal={};
function sv(k,v){
  const n=parseInt(v);
  const inp=document.getElementById('r-'+k);
  if(inp){
    const mn=parseInt(inp.min||0),mx=parseInt(inp.max||9999);
    cfgLocal[k]=Math.max(mn,Math.min(mx,isNaN(n)?mn:n));
  } else {
    cfgLocal[k]=isNaN(n)?0:n;
  }
}
function carregarConfig(){
  fetch('/api/config').then(r=>r.json()).then(cfg=>{
    cfgLocal={...cfg};
    CFG_KEYS.forEach(k=>{
      const inp=document.getElementById('r-'+k);
      if(inp&&cfg[k]!==undefined)inp.value=cfg[k];
    });
  });
}
function salvarConfig(){
  fetch('/api/config',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify(cfgLocal)
  }).then(r=>r.json()).then(d=>{if(d.ok)showSaveOk('Configura\u00e7\u00f5es salvas!');});
}
function showSaveOk(msg){
  const el=document.getElementById('saveOk');
  if(!el)return;
  el.textContent=msg;el.classList.add('visible');
  setTimeout(()=>el.classList.remove('visible'),2500);
}
window.addEventListener('DOMContentLoaded',()=>{
  carregarConfig();
  fetch('/api/status').then(r=>r.json()).then(d=>updateUI(d.cip,d.espnow)).catch(()=>{});
  fetch('/api/ciclo-custom').then(r=>r.json()).then(arr=>{
    if(arr&&arr.length>0){
      fila=arr.map(n=>({num:n,nome:NOMES[n]||'?',cls:FCLR[n]||''}));
      renderFila();
    }
  }).catch(()=>{});
});
</script>
</body>
</html>
)=====";

// ──────────────────────────────────────────────────────────────
//  SSE — payload periódico
// ──────────────────────────────────────────────────────────────
static void sendStatusEvent() {
  String payload = "{\"cip\":"    + cip_getStatusJson()
                 + ",\"espnow\":" + espnow_getStatusJson()
                 + "}";
  events.send(payload.c_str(), "status", millis());
}

// ──────────────────────────────────────────────────────────────
//  Helpers: resposta JSON rápida
// ──────────────────────────────────────────────────────────────
static void jsonOk(AsyncWebServerRequest* req) {
  req->send(200, "application/json", "{\"ok\":true}");
}
static void jsonErr(AsyncWebServerRequest* req, int code, const char* msg) {
  String r = "{\"ok\":false,\"msg\":\"";
  r += msg;
  r += "\"}";
  req->send(code, "application/json", r);
}

// ──────────────────────────────────────────────────────────────
//  Captive portal — redireciona qualquer host para a raiz
// ──────────────────────────────────────────────────────────────
class CaptiveHandler : public AsyncWebHandler {
public:
  bool canHandle(AsyncWebServerRequest* req)  {
    // Deixa passar apenas requisições para o próprio IP do AP
    String host = req->host();
    host.trim();
    return !host.startsWith(AP_IP) && !host.startsWith("192.168.4.1");
  }
  void handleRequest(AsyncWebServerRequest* req) override {
    req->redirect("http://" AP_IP "/");
  }
};

// ──────────────────────────────────────────────────────────────
//  Rotas
// ──────────────────────────────────────────────────────────────
static void setupRoutes() {

  // ── Página principal ─────────────────────────────────────────
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    AsyncWebServerResponse* res =
      req->beginResponse_P(200, "text/html; charset=utf-8",
                           (const uint8_t*)INDEX_HTML, strlen_P(INDEX_HTML));
    res->addHeader("Cache-Control", "no-cache");
    req->send(res);
  });

  // ── SSE ──────────────────────────────────────────────────────
  events.onConnect([](AsyncEventSourceClient* c) {
    c->send("connected", "init", millis());
  });
  server.addHandler(&events);

  // ── Status JSON ───────────────────────────────────────────────
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* req) {
    String p = "{\"cip\":"    + cip_getStatusJson()
             + ",\"espnow\":" + espnow_getStatusJson() + "}";
    req->send(200, "application/json", p);
  });

  // ── Configurações GET ─────────────────────────────────────────
  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest* req) {
    StaticJsonDocument<256> doc;
    doc["volAlc"]             = cipConfig.volAlc;
    doc["volAcid"]            = cipConfig.volAcid;
    doc["volSanit"]           = cipConfig.volSanit;
    doc["tempPre"]            = cipConfig.tempPre;
    doc["tempAlc"]            = cipConfig.tempAlc;
    doc["tempAcid"]           = cipConfig.tempAcid;
    doc["tempoCirculacaoMin"] = cipConfig.tempoCirculacaoMs / 60000UL;
    String out; serializeJson(doc, out);
    req->send(200, "application/json", out);
  });

  // ── Ciclo custom GET (carregar salvo) ─────────────────────────
  server.on("/api/ciclo-custom", HTTP_GET, [](AsyncWebServerRequest* req) {
    uint8_t fila[MAX_ETAPAS_CUSTOM]; uint8_t tam = 0;
    storage_loadCicloCustom(fila, tam);
    String j = "[";
    for (uint8_t i = 0; i < tam; i++) {
      if (i) j += ",";
      j += String(fila[i]);
    }
    j += "]";
    req->send(200, "application/json", j);
  });

  // ── Iniciar CIP (sem body) ────────────────────────────────────
  server.on("/api/cip/iniciar", HTTP_POST, [](AsyncWebServerRequest* req) {
    if (!espnow_allModulesReady())
      return jsonErr(req, 403, "M\u00f3dulos offline");
    if (!cip_iniciarCIP())
      return jsonErr(req, 409, "Ciclo j\u00e1 em execu\u00e7\u00e3o");
    jsonOk(req);
  });

  // ── Cancelar (sem body) ───────────────────────────────────────
  server.on("/api/cip/cancelar", HTTP_POST, [](AsyncWebServerRequest* req) {
    cip_cancelar();
    jsonOk(req);
  });

  // ── Ciclo custom POST — body acumulado manualmente ────────────
  // onBody é chamado com pedaços do body; onRequest() dispara depois
  // que o body todo chegou (quando index+len == total).
  server.on("/api/cip/custom", HTTP_POST,
    // handler chamado quando body já foi completo (ou quando não há body)
    [](AsyncWebServerRequest* req) {
      // Só cai aqui se não houve body (raro). Devolve erro.
      if (!req->_tempObject) {
        return jsonErr(req, 400, "Body vazio");
      }
      // O body foi salvo em _tempObject por onBody abaixo
      String* body = (String*)req->_tempObject;

      StaticJsonDocument<512> doc;
      if (deserializeJson(doc, *body) != DeserializationError::Ok) {
        delete body; req->_tempObject = nullptr;
        return jsonErr(req, 400, "JSON inv\u00e1lido");
      }
      delete body; req->_tempObject = nullptr;

      if (!espnow_allModulesReady())
        return jsonErr(req, 403, "M\u00f3dulos offline");

      JsonArray arr = doc["fila"].as<JsonArray>();
      uint8_t fila[MAX_ETAPAS_CUSTOM];
      uint8_t tam = 0;
      for (JsonVariant v : arr) {
        if (tam >= MAX_ETAPAS_CUSTOM) break;
        fila[tam++] = v.as<uint8_t>();
      }

      if (doc["salvar"] | false) {
        storage_saveCicloCustom(fila, tam);
      }

      if (!cip_iniciarCustom(fila, tam))
        return jsonErr(req, 409, "Fila vazia ou ciclo em execu\u00e7\u00e3o");

      jsonOk(req);
    },
    // upload handler (não usado)
    nullptr,
    // onBody: acumula chunks em _tempObject
    [](AsyncWebServerRequest* req, uint8_t* data, size_t len,
       size_t index, size_t total) {
      if (index == 0) {
        req->_tempObject = new String();
        ((String*)req->_tempObject)->reserve(total > 0 ? total : 256);
      }
      if (req->_tempObject) {
        ((String*)req->_tempObject)->concat((const char*)data, len);
      }
    }
  );

  // ── Configurações POST — mesmo padrão de body buffer ─────────
  server.on("/api/config", HTTP_POST,
    [](AsyncWebServerRequest* req) {
      if (!req->_tempObject)
        return jsonErr(req, 400, "Body vazio");

      String* body = (String*)req->_tempObject;
      StaticJsonDocument<256> doc;
      if (deserializeJson(doc, *body) != DeserializationError::Ok) {
        delete body; req->_tempObject = nullptr;
        return jsonErr(req, 400, "JSON inv\u00e1lido");
      }
      delete body; req->_tempObject = nullptr;

      CipConfig cfg = cipConfig;
      if (doc.containsKey("volAlc"))            cfg.volAlc   = doc["volAlc"];
      if (doc.containsKey("volAcid"))           cfg.volAcid  = doc["volAcid"];
      if (doc.containsKey("volSanit"))          cfg.volSanit = doc["volSanit"];
      if (doc.containsKey("tempPre"))           cfg.tempPre  = doc["tempPre"];
      if (doc.containsKey("tempAlc"))           cfg.tempAlc  = doc["tempAlc"];
      if (doc.containsKey("tempAcid"))          cfg.tempAcid = doc["tempAcid"];
      if (doc.containsKey("tempoCirculacaoMin"))
        cfg.tempoCirculacaoMs = (unsigned long)doc["tempoCirculacaoMin"] * 60000UL;

      cip_setConfig(cfg);
      storage_saveConfig(cfg);
      jsonOk(req);
    },
    nullptr,
    [](AsyncWebServerRequest* req, uint8_t* data, size_t len,
       size_t index, size_t total) {
      if (index == 0) {
        req->_tempObject = new String();
        ((String*)req->_tempObject)->reserve(total > 0 ? total : 256);
      }
      if (req->_tempObject)
        ((String*)req->_tempObject)->concat((const char*)data, len);
    }
  );

  // ── Captive portal — rotas específicas de cada OS ─────────────
  //  Android: /generate_204
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* r) {
    r->redirect("http://" AP_IP "/");
  });
  //  iOS / macOS: /hotspot-detect.html  e  /library/test/success.html
  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* r) {
    r->send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
  });
  server.on("/library/test/success.html", HTTP_GET, [](AsyncWebServerRequest* r) {
    r->send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
  });
  //  Windows: /ncsi.txt  e  /connecttest.txt
  server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* r) {
    r->send(200, "text/plain", "Microsoft NCSI");
  });
  server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* r) {
    r->send(200, "text/plain", "Microsoft Connect Test");
  });
  //  Kindle / outros
  server.on("/kindle-wifi/wifistub.html", HTTP_GET, [](AsyncWebServerRequest* r) {
    r->redirect("http://" AP_IP "/");
  });

  // ── Not found → redireciona para home (captive portal) ────────
  server.onNotFound([](AsyncWebServerRequest* r) {
    r->redirect("http://" AP_IP "/");
  });

  // ── Handler genérico de captive portal (último na fila) ───────
  server.addHandler(new CaptiveHandler());
}

// ──────────────────────────────────────────────────────────────
//  Inicialização
// ──────────────────────────────────────────────────────────────
void webserver_init() {
  // Modo AP+STA permite ESP-NOW operar na mesma interface
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(WIFI_SSID, strlen(WIFI_PASSWORD) ? WIFI_PASSWORD : nullptr);
  delay(200);

  IPAddress ip, gw, subnet;
  ip.fromString(AP_IP);
  gw = ip;
  subnet.fromString("255.255.255.0");
  WiFi.softAPConfig(ip, gw, subnet);

  // DNS wildcard: todos os domínios → IP do AP (captive portal)
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", ip);

  setupRoutes();
  server.begin();

  Serial.printf("[WIFI] AP: %s  IP: %s\n", WIFI_SSID, AP_IP);
  Serial.println("[WIFI] Captive portal ativo");
}

// ──────────────────────────────────────────────────────────────
//  Loop — DNS + SSE periódico
// ──────────────────────────────────────────────────────────────
void webserver_loop() {
  dnsServer.processNextRequest();

  static unsigned long last = 0;
  if (millis() - last >= STATUS_UPDATE_INTERVAL_MS && events.count() > 0) {
    last = millis();
    sendStatusEvent();
  }
}