/**
 * @file Storage.h
 * @brief Persistencia de volumes, temperaturas, tempo de circulacao e ciclo
 * personalizado na EEPROM.
 */
#pragma once

#include <Arduino.h>

/// Carrega o volume das solucoes salvo na EEPROM (posicoes EEPROM_ALC/ACID/SANIT).
void pegarVolSolucaoEEPROM();

/// Salva o volume personalizado das solucoes na EEPROM.
void salvarSolucaoNaEEPROM();

/// Carrega as temperaturas dos ciclos salvas na EEPROM.
void pegarTempSolucaoEEPROM();

/// Salva as temperaturas personalizadas dos ciclos na EEPROM.
void salvarTempNaEEPROM();

/// Carrega o tempo de circulacao (em minutos) salvo na EEPROM.
void pegarTempoCirculacaoEEPROM();

/// Salva o tempo de circulacao atual na EEPROM.
void salvarTempoCirculacaoNaEEPROM();

/// Carrega o ciclo personalizado salvo na EEPROM para vetorRotinas.
void pegarCicloDaEEPROM();

/// Salva o ciclo personalizado (vetorRotinas) na EEPROM.
void salvarCicloPersonalizadoNaEEPROM();
