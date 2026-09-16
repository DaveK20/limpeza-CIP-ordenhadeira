/**
 * @file Utils.h
 * @brief Utilitarios de baixo nivel usados por todos os modulos.
 */
#pragma once

#include <Arduino.h>

/**
 * @brief delay que alimenta o watchdog em pequenos passos, evitando reset indesejado.
 *
 * @param ms tempo total a aguardar, em milissegundos
 */
void safeDelay(unsigned long ms);
