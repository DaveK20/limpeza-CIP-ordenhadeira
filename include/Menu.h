/**
 * @file Menu.h
 * @brief Navegacao por botoes no painel LCD: selecao de ciclos e ajuste de
 * parametros (volume, temperatura, tempo de circulacao).
 */
#pragma once

#include <Arduino.h>

/// Painel principal de selecao de ciclos e ajustes.
void selecionarOpcao();

/**
 * @brief confirma a opcao selecionada antes de executa-la
 *
 * @param funcao a ser executada apos a confirmacao
 * @param botao a ser pressionado para confirmacao
 */
void confirmarSelecao(void (*funcao)(), uint8_t botao);

/// Painel de ajuste do volume das solucoes (alcalina, acida, sanitizante).
void alterarVolumeSolucao();

/// Painel de ajuste da temperatura de cada etapa do ciclo.
void alterarTemperatura();

/// Painel de ajuste do tempo de circulacao da solucao.
void alterarTempoCirculacao();

/// Painel de selecao do ciclo de limpeza personalizado (usar/criar/verificar).
void cicloPersonalizado();
