// ╔══════════════════════════════════════════════════════════════╗
// ║      WEB_SERVER.CPP — AP, Captive Portal e Rotas REST       ║
// ╚══════════════════════════════════════════════════════════════╝
#include "web_server.h"
#include "cip_cycle.h"
#include "espnow_manager.h"
#include "config.h"

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

static AsyncWebServer server(80);
static AsyncEventSource events("/events");
static DNSServer dnsServer;

// ──────────────────────────────────────────────────────────────
//  SSE — Envio periódico de estado (temperature, ciclo, módulos)
// ──────────────────────────────────────────────────────────────
static void sendStatusEvent()
{
    // Atualiza temperatura a cada ciclo
    cip_lerTemperatura();

    String payload = "{";
    payload += "\"cip\":" + cip_getStatusJson() + ",";
    payload += "\"espnow\":" + espnow_getStatusJson();
    payload += "}";

    events.send(payload.c_str(), "status", millis());
}

// ──────────────────────────────────────────────────────────────
//  Captive Portal — redireciona qualquer DNS para o ESP32
// ──────────────────────────────────────────────────────────────
class CaptiveRequestHandler : public AsyncWebHandler
{
public:
    bool canHandle(AsyncWebServerRequest *req) override
    {
        return !req->host().equals(AP_IP);
    }
    void handleRequest(AsyncWebServerRequest *req) override
    {
        req->redirect("http://" AP_IP "/");
    }
};

// ──────────────────────────────────────────────────────────────
//  Rotas
// ──────────────────────────────────────────────────────────────
static void setupRoutes()
{

    // ── Interface principal ──────────────────────────────────────
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *req)
              { req->send(LittleFS, "/index.html", "text/html"); });

    // ── SSE ─────────────────────────────────────────────────────
    events.onConnect([](AsyncEventSourceClient *client)
                     { client->send("connected", "init", millis()); });
    server.addHandler(&events);

    // ── Iniciar ciclo CIP ────────────────────────────────────────
    server.on("/api/cip/iniciar", HTTP_POST, [](AsyncWebServerRequest *req)
              {
    if (!espnow_allModulesReady()) {
      req->send(403, "application/json",
                "{\"ok\":false,\"msg\":\"Módulos offline\"}");
      return;
    }
    bool ok = cip_iniciar();
    req->send(200, "application/json",
              ok ? "{\"ok\":true}" : "{\"ok\":false,\"msg\":\"Já em execução\"}"); });

    // ── Parar ciclo ──────────────────────────────────────────────
    server.on("/api/cip/parar", HTTP_POST, [](AsyncWebServerRequest *req)
              {
    cip_parar();
    req->send(200, "application/json", "{\"ok\":true}"); });

    // ── Status instantâneo ───────────────────────────────────────
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *req)
              {
    cip_lerTemperatura();
    String payload = "{\"cip\":" + cip_getStatusJson()
                   + ",\"espnow\":" + espnow_getStatusJson() + "}";
    req->send(200, "application/json", payload); });

    // ── Leitura das configurações ────────────────────────────────
    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *req)
              {
    StaticJsonDocument<256> doc;
    doc["volAlcalina"]    = cipConfig.volAlcalina;
    doc["volAcida"]       = cipConfig.volAcida;
    doc["volSanitizante"] = cipConfig.volSanitizante;
    doc["tempAlcalina"]   = cipConfig.tempAlcalina;
    doc["tempAcida"]      = cipConfig.tempAcida;
    doc["tempSanitizante"]= cipConfig.tempSanitizante;
    doc["tempoCirculacao"]= cipConfig.tempoCirculacao;
    doc["tempoEnxague"]   = cipConfig.tempoEnxague;
    String out; serializeJson(doc, out);
    req->send(200, "application/json", out); });

    // ── Salvar configurações ─────────────────────────────────────
    AsyncCallbackJsonWebHandler *cfgHandler =
        new AsyncCallbackJsonWebHandler("/api/config",
                                        [](AsyncWebServerRequest *req, JsonVariant &json)
                                        {
                                            JsonObject obj = json.as<JsonObject>();
                                            CipConfig cfg = cipConfig; // copia atual
                                            if (obj.containsKey("volAlcalina"))
                                                cfg.volAlcalina = obj["volAlcalina"];
                                            if (obj.containsKey("volAcida"))
                                                cfg.volAcida = obj["volAcida"];
                                            if (obj.containsKey("volSanitizante"))
                                                cfg.volSanitizante = obj["volSanitizante"];
                                            if (obj.containsKey("tempAlcalina"))
                                                cfg.tempAlcalina = obj["tempAlcalina"];
                                            if (obj.containsKey("tempAcida"))
                                                cfg.tempAcida = obj["tempAcida"];
                                            if (obj.containsKey("tempSanitizante"))
                                                cfg.tempSanitizante = obj["tempSanitizante"];
                                            if (obj.containsKey("tempoCirculacao"))
                                                cfg.tempoCirculacao = obj["tempoCirculacao"];
                                            if (obj.containsKey("tempoEnxague"))
                                                cfg.tempoEnxague = obj["tempoEnxague"];
                                            cip_setConfig(cfg);
                                            req->send(200, "application/json", "{\"ok\":true}");
                                        });
    server.addHandler(cfgHandler);

    // ── Captive portal: Android / iOS / Windows ──────────────────
    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *r)
              { r->redirect("/"); });
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *r)
              { r->redirect("/"); });
    server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *r)
              { r->redirect("/"); });
    server.on("/fwlink", HTTP_GET, [](AsyncWebServerRequest *r)
              { r->redirect("/"); });

    // ── Arquivos estáticos (LittleFS) ────────────────────────────
    server.serveStatic("/", LittleFS, "/");

    // ── 404 → redireciona para home ──────────────────────────────
    server.onNotFound([](AsyncWebServerRequest *req)
                      { req->redirect("/"); });

    // ── Handler genérico de captive portal ───────────────────────
    server.addHandler(new CaptiveRequestHandler());
}

// ──────────────────────────────────────────────────────────────
//  Inicialização
// ──────────────────────────────────────────────────────────────
void webserver_init()
{
    // Sobe rede Wi-Fi em modo AP
    WiFi.mode(WIFI_AP_STA); // AP + STA para ESP-NOW funcionar
    WiFi.softAP(WIFI_SSID, strlen(WIFI_PASSWORD) ? WIFI_PASSWORD : nullptr);
    delay(200);

    IPAddress ip;
    ip.fromString(AP_IP);
    IPAddress gw(ip);
    IPAddress mask(255, 255, 255, 0);
    WiFi.softAPConfig(ip, gw, mask);

    Serial.printf("[WIFI] AP: %s  IP: %s\n", WIFI_SSID, AP_IP);

    // DNS: qualquer domínio → IP do ESP32 (captive portal)
    dnsServer.start(53, "*", ip);

    // LittleFS
    if (!LittleFS.begin())
    {
        Serial.println("[FS] ❌ LittleFS não montado. Execute: Tools > ESP32 Sketch Data Upload");
    }

    setupRoutes();
    server.begin();
    Serial.println("[HTTP] ✅ Servidor iniciado");
}

// ──────────────────────────────────────────────────────────────
//  Loop — DNS + SSE periódico
// ──────────────────────────────────────────────────────────────
void webserver_loop()
{
    dnsServer.processNextRequest();

    static uint32_t lastEvent = 0;
    if (millis() - lastEvent >= STATUS_UPDATE_INTERVAL_MS)
    {
        lastEvent = millis();
        if (events.count() > 0)
            sendStatusEvent();
    }
}