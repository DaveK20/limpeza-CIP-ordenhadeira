/**
 * @file Config.h
 * @brief Pinagem, enderecos de EEPROM e temporizacoes do sistema CIP.
 */
#pragma once

// ---------- DISPLAY LCD ----------
#define col 16    // numero de colunas do display
#define lin 2     // numero de linhas do display
#define ende 0x27 // endereco I2C do display

// ---------- RELES: BOMBAS PERISTALTICAS ----------
#define relayAlc 30   // bomba peristaltica alcalina
#define relayAcid 26  // bomba peristaltica acida
#define relaySanit 28 // bomba peristaltica sanitizante

// ---------- VALVULAS E ATUADORES ----------
#define vs_ciclo 32         // valvula de inox conectada com o tanque de aquecimento e mistura
#define vs_vasao 34         // valvula de inox direcionada para o tanque de aquecimento e a saida
#define vs_ts 36            // valvula de latao do tanque de aquecimento
#define vs_tm 38            // valvula de latao do tanque de mistura
#define ControleOrdenha 40  // acionamento da succao da ordenha
#define relayResistencia 46 // aquecer agua

// ---------- BOTOES ----------
#define botaoSetaEsquerda 6        // botao de interacao com o sistema AZUL
#define botaoOK 5                  // botao para confirmacao das selecoes
#define botaoSetaDireita 4         // botao de interacao com o sistema
#define botaoRemover 3             // botao alterar volume das solucoes BRANCO
#define botaoInterromperOperacao 2 // botao de interrupcao de ciclo

// ---------- BOIAS (SENSORES DE NIVEL) ----------
#define boiaSolucao 42 // boia do tanque de aquecimento
#define boiaMistura 44 // boia do tanque de mistura

// ---------- SENSOR DE TEMPERATURA ----------
#define tempSensor 14 // DS18B20
#define LED_STATUS_SENSOR 17

// ---------- ENDERECOS NA EEPROM: VOLUME DAS SOLUCOES ----------
#define EEPROM_ALC 10
#define EEPROM_ACID 11
#define EEPROM_SANIT 12

// ---------- ENDERECOS NA EEPROM: TEMPERATURAS ----------
#define EEPROM_TEMP_PRE_EXAGUE 13
#define EEPROM_TEMP_ALC 14
#define EEPROM_TEMP_ACID 15

// ---------- ENDERECO NA EEPROM: TEMPO DE CIRCULACAO (em minutos, 1 byte) ----------
#define EEPROM_TEMPO_CIRCULACAO 16

// ---------- TEMPORIZACOES ----------
#define tempoInterrupcao 3000            // delay minimo ate ser possivel acionar a proxima interrupcao
#define tempoColetaDados 3000            // coleta e amostragem dos dados de temperatura e status das bombas
#define tempoPosicionamentoValvula 12000 // tempo de posicionamento das valvulas solenoide
#define tempoPosicionamentoLatao 6000
#define tempoDisplay 100 // tempo para atualizar informacoes no display
