/**
 * @file Globals.h
 * @brief Estado compartilhado do sistema (volumes, temperaturas, flags de operacao).
 */
#pragma once

#include <Arduino.h>

// ---------- VOLUME das solucoes A SER INSERIDO NO SISTEMA ----------
extern float volAlc;
extern float volAcid;
extern float volSanit; // ainda nao descoberto

// ---------- VOLUME INSERIDO PELO USUARIO E A SER SALVO NA EEPROM ----------
extern float volAlcPersonalizado;
extern float volAcidPersonalizado;
extern float volSanitPersonalizado;

// ---------- BOMBA PERISTALTICA ----------
extern float fluxo_bomba; // Fluxo em ml/s da bomba peristaltica previamente calibrada.
extern uint8_t vol_tanque;

// ---------- TEMPERATURAS IDEAIS DA AGUA ----------
// De acordo com o artigo "Limpeza e Desinfeccao de Equipamentos de Ordenha e Tanques" de MARCOS VEIGA SANTOS
extern uint8_t tempPreEnxague; // temperatura na primeira lavagem
extern uint8_t tempAlc;        // solucao base
extern uint8_t tempAcid;       // solucao acida

extern uint8_t tempAlcPersonalizado;
extern uint8_t tempAcidPersonalizado;
extern uint8_t tempPreEnxaguePersonalizado;

// ---------- NAVEGACAO / INTERFACE ----------
extern uint8_t timer;             // timer de espera para o usuario apertar o botao
extern uint8_t vetorRotinas[8];   // vetor para salvar um ciclo de limpeza personalizado
extern uint8_t tamVetorRotinas;   // tamanho do vetor de rotinas
extern uint16_t delaySetas;       // delay das setas de selecao
extern uint8_t percorrerOpcoes;   // opcoes do vetor de selecao

// ---------- ESTADO DO SISTEMA ----------
extern volatile bool interromper; // interromper ciclo
extern bool statusSensorTemperatura;

// ---------- TEMPOS DE OPERACAO ----------
extern unsigned long tempoEsvaziarTanque;   // tempo estimado para que o tanque fique vazio
extern unsigned long tempoCirculacaoSolucao; // tempo de circulacao da solucao em ms (padrao: 5 minutos)

extern unsigned long ultima_coleta;
extern unsigned long ultima_interrupcao;
extern unsigned long ultimo_print_display;
