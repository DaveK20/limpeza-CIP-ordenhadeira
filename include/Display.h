/**
 * @file Display.h
 * @brief Controle do display LCD I2C.
 */
#pragma once

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

extern LiquidCrystal_I2C lcd;

/// Inicializa a comunicacao com o display e liga o backlight.
void displayInit();

/**
 * @brief Imprime duas linhas centralizadas no LCD, com indicadores de
 * interrupcao (*) e falha do sensor de temperatura (!) na coluna 15.
 *
 * @param linha0 coluna 0, linha 0 do LCD
 * @param linha1 coluna 0, linha 1 do LCD
 */
void printOpcoesLCD(String linha0, String linha1);
