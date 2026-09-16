/**
 * @file main.cpp
 * @author DaveK2 (davefr@outlook.com.br)
 * @brief CIP ordenhadeira Campus Bom Jesus do Itabapoana
 * @version 0.9.3
 * @date 2023-09-12
 *
 * @copyright Copyright (c) 2023
 *
 */
/*
OBJETIVOS RESTANTES
  [] - ter uma nocao do tempo necessario para esvaziar o tanque e mostrar no display
  [x] - ciclo personalizado
  [x] - precisao das bombas peristalticas
  [x] - controle da contatora
  [x] - controle da resistencia
  [x] - painel de controle para o usuario
  [] - primeiro teste do prototipo
  [x] - display de informacoes uteis

  IMPORTANTE!!!
  Olhar rotinas CIP e corrigir inconsistencias
  remover variaveis desnecessarias
*/

#include <Arduino.h>
#include <EEPROM.h>
#include <DallasTemperature.h>
#include <avr/wdt.h>

#include "Config.h"
#include "Globals.h"
#include "Display.h"
#include "TemperatureSensor.h"
#include "Actuators.h"
#include "Storage.h"
#include "CleaningCycle.h"
#include "Menu.h"

void setup()
{
  Serial.begin(9600);
  pinMode(relayAcid, OUTPUT);
  pinMode(relayAlc, OUTPUT);
  pinMode(relaySanit, OUTPUT);
  pinMode(relayResistencia, OUTPUT);
  pinMode(vs_ciclo, OUTPUT);
  pinMode(vs_vasao, OUTPUT);
  pinMode(vs_ts, OUTPUT);
  pinMode(vs_tm, OUTPUT);
  pinMode(ControleOrdenha, OUTPUT);

  pinMode(LED_STATUS_SENSOR, OUTPUT);
  pinMode(boiaSolucao, INPUT_PULLUP);
  pinMode(boiaMistura, INPUT_PULLUP);

  pinMode(botaoSetaEsquerda, INPUT_PULLUP);
  pinMode(botaoOK, INPUT_PULLUP);
  pinMode(botaoSetaDireita, INPUT_PULLUP);
  pinMode(botaoInterromperOperacao, INPUT_PULLUP);
  pinMode(botaoRemover, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(botaoInterromperOperacao), interromperOperacao, FALLING);

  definicaoInicialReles();
  tempSensorInit();

  Serial.println("Buscando definicoes da memoria..");
  EEPROM.begin();
  pegarVolSolucaoEEPROM();
  pegarTempSolucaoEEPROM();
  pegarCicloDaEEPROM();
  pegarTempoCirculacaoEEPROM();

  Serial.println("Iniciando display...");
  displayInit();

  printOpcoesLCD("Inicializando", "Sistema...");

  lcd.clear(); // Serve para limpar a tela do display
  Serial.println("Sistema iniciado.");

  if (tempAgua() == DEVICE_DISCONNECTED_C)
    statusSensorTemperatura = false;

  wdt_enable(WDTO_8S);
}

void loop()
{
  Serial.println("bs: " + String(digitalRead(boiaSolucao)) + " bm: " + String(digitalRead(boiaMistura)));
  wdt_reset();

  if (!statusSensorTemperatura)
    digitalWrite(LED_STATUS_SENSOR, HIGH);
  else
    digitalWrite(LED_STATUS_SENSOR, LOW);

  selecionarOpcao();

  if (interromper)
  {
    if ((millis() - ultima_interrupcao) > tempoInterrupcao)
    {
      lcd.clear();
      interromper = false;
      ultima_interrupcao = millis();
    }
  }
}
