/**
 * @file CleaningCycle.h
 * @brief Rotinas do ciclo de limpeza CIP (pre-enxague, enxague, solucoes,
 * sanitizante) e tratamento de interrupcao.
 */
#pragma once

#include <Arduino.h>

/// attachInterrupt responsavel por sinalizar a interrupcao dos ciclos de limpeza.
void interromperOperacao();

/// Esvazia o tanque com seguranca caso o ciclo tenha sido interrompido.
void interrupcao();

/// Pre lavagem do tanque apos ordenha.
void rotinaPreEnxague();

/// Lavagem intermitente entre os ciclos acido e base.
void rotinaEnxague();

/**
 * @brief adicao de solucoes ao tanque para realizacao da rotina completa de limpeza
 *
 * @param solucao 1 - Alcalina | 2 - Acida
 * @param volSolucao volume em ml a ser adicionado
 * @param tempSolucao temperatura da rotina
 */
void rotinaSolucao(uint8_t solucao, float volSolucao, uint8_t tempSolucao);

/// Rotina da solucao sanitizante adicionada na agua.
void rotinaSanitizante();

/// Controle do ciclo automatico completo de limpeza da ordenhadeira.
void cicloCIP();
