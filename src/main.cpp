// ╔══════════════════════════════════════════════════════════════╗
// ║           CIP_SYSTEM.INO — Sketch Principal                  ║
// ║                                                              ║
// ║  Bibliotecas (instalar via Library Manager):                 ║
// ║   • ESPAsyncWebServer + AsyncTCP  (me-no-dev)               ║
// ║   • ArduinoJson v6+               (Benoit Blanchon)          ║
// ║   • DallasTemperature + OneWire                              ║
// ║                                                              ║
// ║  Nota: LittleFS removido — HTML embedado em PROGMEM.        ║
// ║  Não é mais necessário o "ESP32 Sketch Data Upload".         ║
// ╚══════════════════════════════════════════════════════════════╝

#include "config.h"
#include "actuators.h"
#include "espnow_manager.h"
#include "cip_cycle.h"
#include "storage.h"
#include "web_server.h"

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n╔══════════════════════════════════╗");
  Serial.println("║   Sistema CIP — Ordenhadeira     ║");
  Serial.println("║   IFF Bom Jesus do Itabapoana    ║");
  Serial.println("╚══════════════════════════════════╝\n");

  // 1. Atuadores (pinos de saída + estado seguro)
  actuators_init();

  // 2. Armazenamento NVS
  storage_init();

  // 3. Ciclo CIP + carrega configurações persistidas
  cip_init();
  storage_loadAll(cipConfig);

  // 4. Web server (AP + captive portal, HTML em PROGMEM)
  webserver_init();

  // 5. ESP-NOW (comunicação com módulos ORDENHA e MOTOR)
  espnow_init();

  Serial.println("[BOOT] ✅ Sistema pronto!\n");
}

void loop() {
  webserver_loop();    // DNS captive portal + SSE
  espnow_loop();       // Heartbeat / timeout dos módulos
  cip_loop();          // Máquina de estados CIP (não bloqueia)

  // LED: piscando = aguardando módulos | fixo = todos prontos
  static unsigned long lastBlink = 0;
  if (!espnow_allReady) {
    if (millis() - lastBlink >= 500) {
      lastBlink = millis();
      digitalWrite(PIN_LED_STATUS, !digitalRead(PIN_LED_STATUS));
    }
  } else {
    digitalWrite(PIN_LED_STATUS, HIGH);
  }
}