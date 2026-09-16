#include "Actuators.h"
#include <avr/wdt.h>
#include "Config.h"
#include "Globals.h"
#include "Display.h"
#include "Utils.h"
#include "TemperatureSensor.h"

/**
 * @brief calcula o tempo de acionamento da bomba para despejar o volume de solucao desejado
 * levando em consideracao o volume do tanque e o fluxo da bomba
 *
 * @param solucao concentracao em mL/L a ser adicionada
 * @return tempo em ms
 */
static float calcSolucao(float solucao)
{
  // A dosagem total de produto referente a solucao por litro e o volume do tanque.
  float dosagem = solucao * vol_tanque;
  // O tempo de dosagem corresponde a quantidade total de dosagem a ser utilizada multiplicada pelo fluxo da bomba.
  return (dosagem / fluxo_bomba) * 1000;
}

void definicaoInicialReles()
{
  Serial.println("Definindo estado inicial dos reles...");
  estadoBombas(HIGH, HIGH, HIGH);
  digitalWrite(relayResistencia, LOW);
  digitalWrite(ControleOrdenha, LOW);
  digitalWrite(vs_tm, HIGH);
  digitalWrite(vs_ts, HIGH);
}

void testeSistema()
{
  Serial.println("Teste do sistema iniciado...");
  Serial.println("Testando bombas peristalticas...");

  uint8_t vetorReles[9] = {relayAlc, relayAcid, relaySanit, vs_ciclo, vs_vasao, vs_ts, vs_tm, ControleOrdenha, relayResistencia};

  for (uint8_t i = 0; i < sizeof(vetorReles) / sizeof(vetorReles[0]); i++)
  {
    digitalWrite(vetorReles[i], HIGH);
  }

  for (uint8_t i = 0; i < 3; i++)
  {
    digitalWrite(vetorReles[i], LOW);
    delay(2000);
    digitalWrite(vetorReles[i], HIGH);
    delay(1000);
  }
}

void estadoBombas(uint8_t estadoAlc, uint8_t estadoAcid, uint8_t estadoSanit)
{
  digitalWrite(relayAlc, estadoAlc);
  digitalWrite(relayAcid, estadoAcid);
  digitalWrite(relaySanit, estadoSanit);
}

void aquecerResistencia(uint8_t status)
{
  wdt_reset();
  Serial.println("========== AQUECIMENTO DA AGUA ==========");
  if (!interromper)
  {
    lcd.clear();
    if (status)
    {
      printOpcoesLCD("Ligando", "resistencia");
      Serial.println("Ligando resistencia");
      digitalWrite(relayResistencia, status);
    }
    else
    {
      printOpcoesLCD("Desligando", "resistencia");
      Serial.println("Desligando resistencia");
      digitalWrite(relayResistencia, status);
    }
  }
  safeDelay(2000);
  Serial.println("===============================");
}

void adicionarSolucao(float solucao_ml, uint8_t relay)
{
  wdt_reset();
  if (interromper == false)
  {
    Serial.println("========== ADICIONAR SOLUCAO ==========");
    lcd.clear();

    Serial.print("Adicionando solucao ");

    switch (relay)
    {
    case 1:
      printOpcoesLCD("Adicionando", "base");
      Serial.println("base");
      estadoBombas(LOW, HIGH, HIGH);
      break;

    case 2:
      printOpcoesLCD("Adicionando", "acido");
      Serial.println("acida");
      estadoBombas(HIGH, LOW, HIGH);
      break;

    case 3:
      printOpcoesLCD("Adicionando", "sanitizante");
      Serial.println("sanitizante");
      estadoBombas(HIGH, HIGH, LOW);
      break;

    default:
      printOpcoesLCD("Solucao", "inexistente");
      break;
    }
    Serial.println(calcSolucao(solucao_ml));
    safeDelay(calcSolucao(solucao_ml)); // calculo do tempo de despejo da solucao
    estadoBombas(HIGH, HIGH, HIGH);
    Serial.println("Solucao Adicionada!");
    lcd.clear();
    printOpcoesLCD("Solucao", "despejada");
    safeDelay(2000);
  }
  Serial.println("===============================");
}

void encherTanque(uint8_t resistencia, uint8_t tanque, uint8_t boia)
{
  wdt_reset();
  Serial.println("========== ENCHER TANQUE ==========");
  if (interromper == false)
  {
    Serial.println("Enchendo tanque...");
    lcd.clear();

    /*
    TODO: Implementar verificacao de qual dos dois tanques estao cheios
    */
    while (!digitalRead(boia)) // contato boia aberto
    {
      wdt_reset();
      printOpcoesLCD("Enchendo tanque", "");
      if (tanque == vs_tm)
      {
        printOpcoesLCD("", "de mistura...");
      }
      else if (tanque == vs_ts)
      {
        printOpcoesLCD("", "de aquecimento...");
      }
      digitalWrite(relayResistencia, LOW); // resistencia desligada
      digitalWrite(tanque, LOW);           // despejando agua no tanque
    }
    Serial.println("Tanque Cheio");
    lcd.clear();
    printOpcoesLCD("Tanque cheio", "");
    safeDelay(2000);
    lcd.clear();

    if (tanque == vs_tm)
    {
      printOpcoesLCD("Fechando", "vs_tm");
    }
    else if (tanque == vs_ts)
    {
      printOpcoesLCD("Fechando", "vs_ts");
    }

    digitalWrite(tanque, HIGH); // fechando valvula
    safeDelay(tempoPosicionamentoLatao);
    lcd.clear();
    if (resistencia)
    {
      Serial.println("Resistencia ligada");
      printOpcoesLCD("Resistencia", "ligada");
      aquecerResistencia(HIGH); // ligar resistencia
    }
    else
    {
      Serial.println("esistencia desligada");
      printOpcoesLCD("Resistencia", "desligada");
      aquecerResistencia(LOW); // desligar resistencia
    }
  }
  Serial.println("===============================");
}

void esvaziarTanque(float tempSolucao) // implementar duas funcoes, do tanque de mistura e de solucao
{
  wdt_reset();
  Serial.println("========== ESVAZIAR TANQUE ==========");
  lcd.clear();
  printOpcoesLCD("Aquecendo", "resistencia");
  if (interromper == false)
  {
    while (!controlarTemperatura(tempSolucao) /*&& digitalRead(boiaSolucao)*/)
    {
      if (tempSolucao == -1)
        break;

      if (!digitalRead(botaoOK))
        break;

      if (interromper)
        return;

      lcd.clear();
      Serial.println("monitorando temperatura no display");
      printOpcoesLCD("Temperatura", String(tempAgua()) + " C");
      safeDelay(500);
    }
    lcd.clear();
    printOpcoesLCD("Atingiu temperatura", "boia liberada");
    aquecerResistencia(LOW); // desligando resistencia

    Serial.println("ativando succao da ordenha");
    lcd.clear();
    printOpcoesLCD("Ativando succao", "ordenhadeira");
    digitalWrite(ControleOrdenha, HIGH); // ATIVAR SUCCAO DO SISTEMA
    safeDelay(2000);

    lcd.clear();
    printOpcoesLCD("Despejando agua", "...");
    Serial.println("Despejando agua...");
    safeDelay(tempoEsvaziarTanque);

    lcd.clear();
    Serial.println("Agua despejada");
    printOpcoesLCD("Agua", "despejada");

    Serial.println("desativando succao da ordenha");
    digitalWrite(ControleOrdenha, LOW); // DESATIVAR SUCCAO DO SISTEMA
  }
  Serial.println("===============================");
}
