/**
 * @file CustomCycle.h
 * @brief Criacao, execucao e visualizacao de um ciclo de limpeza personalizado
 * pelo usuario (sequencia livre de rotinas salva em vetorRotinas).
 */
#pragma once

#include <Arduino.h>

/// Painel interativo para montar um novo ciclo personalizado.
void criarCicloPersonalizado();

/// Executa o ciclo personalizado ja salvo na memoria.
void usarCicloPersonalizado();

/// Monta uma string com a sequencia de etapas do ciclo personalizado atual.
String printarCicloPersonalizado();
