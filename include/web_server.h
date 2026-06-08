#pragma once
// ╔══════════════════════════════════════════════════════════════╗
// ║        WEB_SERVER.H — AP + Captive Portal + Rotas           ║
// ╚══════════════════════════════════════════════════════════════╝
#include <Arduino.h>

void webserver_init();
void webserver_loop(); // Necessário para o DNS do captive portal