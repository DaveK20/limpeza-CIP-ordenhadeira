// ╔══════════════════════════════════════════════════════════════╗
// ║         CIP_SYSTEM.INO — Sketch Principal                    ║
// ║                                                              ║
// ║  Bibliotecas necessárias (instalar via Library Manager):     ║
// ║   • ESPAsyncWebServer  (me-no-dev)                           ║
// ║   • AsyncTCP           (me-no-dev)                           ║
// ║   • ArduinoJson        (Benoit Blanchon)  v6+                ║
// ║   • DallasTemperature  (Miles Burton)                        ║
// ║   • OneWire            (Jim Studt)                           ║
// ║                                                              ║
// ║  Upload do sistema de arquivos:                              ║
// ║   Tools → ESP32 Sketch Data Upload (LittleFS)                ║
// ╚══════════════════════════════════════════════════════════════╝

#include "config.h"
#include "espnow_manager.h"
#include "cip_cycle.h"
#include "web_server.h"

// ──────────────────────────────────────────────────────────────
void setup()
{
  Serial.begin(115200);
  delay(500);

  Serial.println("\n╔══════════════════════════════════╗");
  Serial.println("║   Sistema CIP — Ordenhadeira     ║");
  Serial.println("║   IFF Bom Jesus do Itabapoana    ║");
  Serial.println("╚══════════════════════════════════╝\n");

  pinMode(PIN_LED_STATUS, OUTPUT);

  // 1. Servidor Web (sobe o AP + captive portal)
  webserver_init();

  // 2. ESP-NOW (central para módulos ORDENHA e MOTOR)
  espnow_init();

  // 3. Ciclo CIP (sensores e atuadores)
  cip_init();

  Serial.println("[BOOT] ✅ Sistema pronto!\n");
}

// ──────────────────────────────────────────────────────────────
void loop()
{
  // DNS do captive portal + SSE periódico
  webserver_loop();

  // Heartbeat / timeout dos módulos ESP-NOW
  espnow_loop();

  // Máquina de estados do ciclo CIP
  cip_loop();

  // LED de status: piscando = aguardando módulos | fixo = pronto
  static uint32_t lastBlink = 0;
  if (!espnow_allReady)
  {
    if (millis() - lastBlink >= 500)
    {
      lastBlink = millis();
      digitalWrite(PIN_LED_STATUS, !digitalRead(PIN_LED_STATUS));
    }
  }
  else
  {
    digitalWrite(PIN_LED_STATUS, HIGH);
  }
}
