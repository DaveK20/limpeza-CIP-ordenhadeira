/**
 * @file TemperatureSensor.h
 * @brief Leitura do sensor de temperatura DS18B20 e controle da temperatura da agua.
 */
#pragma once

#include <Arduino.h>

/// Inicializa a comunicacao com o sensor DS18B20.
void tempSensorInit();

/**
 * @brief coletando temperatura do sensor DS18B20
 *
 * @return temperatura
 */
float tempAgua();

/**
 * @brief liga/desliga a resistencia de aquecimento conforme a temperatura alvo
 *
 * @param tempSolucao temperatura alvo em graus Celsius
 * @return true quando a temperatura alvo foi atingida
 */
bool controlarTemperatura(float tempSolucao);
