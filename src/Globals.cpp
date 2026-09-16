#include "Globals.h"

float volAlc = 1.5;
float volAcid = 2.6;
float volSanit = 0;

float volAlcPersonalizado = 0;
float volAcidPersonalizado = 0;
float volSanitPersonalizado = 0;

float fluxo_bomba = 2;
uint8_t vol_tanque = 50;

uint8_t tempPreEnxague = 39;
uint8_t tempAlc = 39;
uint8_t tempAcid = 39;

uint8_t tempAlcPersonalizado = 0;
uint8_t tempAcidPersonalizado = 0;
uint8_t tempPreEnxaguePersonalizado = 0;

uint8_t timer = 100;
uint8_t vetorRotinas[8];
uint16_t delaySetas = 300;
uint8_t percorrerOpcoes = 0;

volatile bool interromper = false;
bool statusSensorTemperatura = true;

unsigned long tempoEsvaziarTanque = 150000;
unsigned long tempoCirculacaoSolucao = 300000UL;

unsigned long ultima_coleta;
unsigned long ultima_interrupcao;
unsigned long ultimo_print_display;

uint8_t tamVetorRotinas = sizeof(vetorRotinas) / sizeof(vetorRotinas[0]);
