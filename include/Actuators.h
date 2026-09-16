/**
 * @file Actuators.h
 * @brief Controle dos reles (bombas, resistencia), valvulas e tanques.
 */
#pragma once

#include <Arduino.h>

/// Define o estado inicial e seguro de todos os reles/valvulas.
void definicaoInicialReles();

/// Aciona cada rele/bomba sequencialmente, para verificar a fiacao do sistema.
void testeSistema();

/**
 * @brief controle das tres bombas peristalticas HIGH - Desativado || LOW - Ativado
 *
 * @param estadoAlc bomba alcalina
 * @param estadoAcid bomba acida
 * @param estadoSanit bomba sanitizante
 */
void estadoBombas(uint8_t estadoAlc, uint8_t estadoAcid, uint8_t estadoSanit);

/**
 * @brief ligar/desligar resistencia
 *
 * @param status LOW - DESLIGA | HIGH - LIGA
 */
void aquecerResistencia(uint8_t status);

/**
 * @brief injeta solucao quimica (base, acido ou sanitizante) via bomba peristaltica
 *
 * @param solucao_ml volume em mL a ser adicionado
 * @param relay bomba a ser ativada => 1 - Base || 2 - Acido || 3 - Sanitizante
 */
void adicionarSolucao(float solucao_ml, uint8_t relay);

/**
 * @brief enche o tanque de agua
 *
 * @param resistencia 1 - liga resistencia apos encher | 0 - mantem desligada
 * @param tanque valvula solenoide responsavel por encher o tanque
 * @param boia pino da boia que indica tanque cheio
 */
void encherTanque(uint8_t resistencia, uint8_t tanque, uint8_t boia);

/**
 * @brief aquece a agua ate a temperatura alvo e aciona a succao para esvaziar o tanque
 *
 * @param tempSolucao temperatura alvo em graus Celsius
 */
void esvaziarTanque(float tempSolucao);
