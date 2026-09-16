#include "CleaningCycle.h"
#include <avr/wdt.h>
#include "Config.h"
#include "Globals.h"
#include "Display.h"
#include "Utils.h"
#include "Actuators.h"
#include "TemperatureSensor.h"

void interromperOperacao()
{
  interromper = true;
}

void interrupcao()
{
  if (interromper)
  {
    Serial.println("Rotina interrompida com sucesso!");
    interromper = false;
    /*
    TODO: Certificar de esvaziar ambos os tanques
    */
    aquecerResistencia(HIGH);
    esvaziarTanque(tempAgua());

    lcd.clear();
    printOpcoesLCD("Ciclo", "cancelado");
    safeDelay(2000);
  }
}

void rotinaPreEnxague()
{
  wdt_reset();
  if (interromper)
    return;

  Serial.println();
  Serial.println("ROTINA PRE-ENXAGUE");
  lcd.clear();
  printOpcoesLCD("Rotina", "PRE-ENXAGUE");
  safeDelay(2000);
  lcd.clear();
  printOpcoesLCD("Posicionando", "vs_vasao");
  digitalWrite(vs_vasao, LOW); // abrindo a valvula de vasao para a saida
  safeDelay(tempoPosicionamentoValvula);
  encherTanque(1, vs_ts, boiaSolucao); // enchendo tanque de aquecimento
  esvaziarTanque(tempPreEnxague);      // succionando agua do tanque de aquecimento
}

void rotinaEnxague()
{
  wdt_reset();
  if (interromper == false)
  {
    Serial.println();
    Serial.println("ROTINA ENXAGUE");
    lcd.clear();
    printOpcoesLCD("Rotina", "ENXAGUE");
    safeDelay(2000);

    digitalWrite(vs_ciclo, HIGH);

    digitalWrite(vs_vasao, LOW); // apontando para saida
    lcd.clear();
    printOpcoesLCD("Posicionando", "vs_vazao/ciclo");

    safeDelay(tempoPosicionamentoValvula);

    encherTanque(0, vs_tm, boiaMistura); // adicionar agua
    esvaziarTanque(-1);                  // liberar apos atingir temperatura
  }
}

void rotinaSolucao(uint8_t solucao, float volSolucao, uint8_t tempSolucao)
{
  wdt_reset();

  if (interromper == false)
  {
    lcd.clear();

    if (solucao == 1)
      printOpcoesLCD("Rotina", "BASE");
    else
      printOpcoesLCD("Rotina", "ACIDA");

    safeDelay(3000);
    digitalWrite(vs_ciclo, LOW);  // puxando do tanque de AQUECIMENTO
    digitalWrite(vs_vasao, HIGH); // apontando para o tanque de solucao
    lcd.clear();
    printOpcoesLCD("Posicionando", "vs_vazao/ciclo");
    safeDelay(tempoPosicionamentoValvula);
    encherTanque(1, vs_ts, boiaSolucao); // adicionar e aquecer somente agua limpa (resistencia nunca toca produto quimico)
    esvaziarTanque(tempSolucao);         // liberar apos atingir temperatura
    lcd.clear();
    printOpcoesLCD("Posicionando", "vs_ciclo");
    digitalWrite(vs_ciclo, HIGH); // puxando do tanque de mistura

    safeDelay(tempoPosicionamentoValvula);

    digitalWrite(ControleOrdenha, HIGH);
    Serial.println("ativando succao");

    adicionarSolucao(volSolucao, solucao); // despeja a solucao na linha, diluindo conforme a agua e succionada

    Serial.println("circulando solucao");
    lcd.clear();

    switch (solucao)
    {
    case 1:
      printOpcoesLCD("Circulando", "solucao base");
      break;
    case 2:
      printOpcoesLCD("Circulando", "solucao acida");
      break;
    default:
      printOpcoesLCD("Solucao", "inexistente");
      break;
    }
    safeDelay(tempoCirculacaoSolucao);  // tempo de circulacao da solucao na ordenha
    digitalWrite(ControleOrdenha, LOW); // desativar succao
    digitalWrite(vs_vasao, LOW);        // apontando para fora
    lcd.clear();
    printOpcoesLCD("Despejando", "para fora");
    safeDelay(tempoPosicionamentoValvula); // tempo para as valvulas se posicionarem
    digitalWrite(ControleOrdenha, HIGH);
    safeDelay(tempoEsvaziarTanque);
    digitalWrite(ControleOrdenha, LOW);
    safeDelay(20000);
  }
}

void rotinaSanitizante()
{
  if (interromper == false)
  {
    Serial.println();
    Serial.println("ROTINA SANITIZANTE");
    lcd.clear();
    printOpcoesLCD("Rotina", "SANITIZANTE");
    safeDelay(2000);
    encherTanque(0, vs_ts, boiaSolucao); // adicionar somente agua limpa
    lcd.clear();
    printOpcoesLCD("Posicionando", "vs_ciclo");
    safeDelay(2000);
    digitalWrite(vs_ciclo, LOW);           // puxando do tanque de aquecimento
    safeDelay(tempoPosicionamentoValvula); // tempo para as valvulas se posicionarem

    Serial.println("ativando succao da ordenha");
    lcd.clear();
    printOpcoesLCD("Ativando succao", "ordenhadeira");
    digitalWrite(ControleOrdenha, HIGH);

    adicionarSolucao(volSanit, 3); // despeja o sanitizante na linha, diluindo conforme a agua e succionada

    lcd.clear();
    printOpcoesLCD("Despejando", "para fora");
    safeDelay(tempoEsvaziarTanque);

    digitalWrite(ControleOrdenha, LOW);
    lcd.clear();
    printOpcoesLCD("Agua", "despejada");
    safeDelay(2000);
  }
}

void cicloCIP()
{
  Serial.println("Inicializando ciclo CIP...");
  lcd.clear();
  printOpcoesLCD("Inicializando", "ciclo CIP...");
  safeDelay(2000);
  // rotina pre-enxague
  rotinaPreEnxague();

  // rotina base
  rotinaSolucao(1, volAlc, tempAlc);
  rotinaEnxague();

  // rotina acido
  rotinaSolucao(2, volAcid, tempAcid);
  rotinaEnxague();

  // rotina sanitizante
  rotinaSanitizante();

  // caso as acoes sejam canceladas
  interrupcao();
}
