#include "Menu.h"
#include <avr/wdt.h>
#include "Config.h"
#include "Globals.h"
#include "Display.h"
#include "Utils.h"
#include "Storage.h"
#include "CleaningCycle.h"
#include "CustomCycle.h"

static const char *opcoes[] = {"CIP", "Personalizado", "Temperatura", "Solucao", "Circulacao"};                // painel de selecao selecionarOpcao()
static const char *percorrerCicloPersonalinado[] = {"Usar Ciclo", "Novo Ciclo", "Verificar ciclo"};             // painel de selecao cicloPersonalizado()
static const char *solucao[] = {"Alcalino", "Acido", "Sanitizante"};                                            // vetor do nome das solucoes a serem alteradas
static const char *stringTemp[] = {"Pre-enxague", "Alcalino", "Acido"};                                         // vetor da temperatura das solucoes e ciclos

void selecionarOpcao()
{
  if (percorrerOpcoes <= 1)
  {
    printOpcoesLCD("Ciclo", opcoes[percorrerOpcoes]);
  }
  else
  {
    printOpcoesLCD("Alterar", opcoes[percorrerOpcoes]);
  }

  if (!digitalRead(botaoSetaDireita) && percorrerOpcoes < 4)
  {
    percorrerOpcoes++;
    delay(delaySetas);
    lcd.clear();
  }

  if (!digitalRead(botaoSetaEsquerda) && percorrerOpcoes > 0)
  {
    percorrerOpcoes--;
    delay(delaySetas);
    lcd.clear();
  }

  if (!digitalRead(botaoOK))
  {
    switch (percorrerOpcoes)
    {
    case 0:
      if (statusSensorTemperatura == true)
      {
        confirmarSelecao(cicloCIP, botaoOK);
      }
      break;
    case 1:
      if (statusSensorTemperatura == true)
      {
        confirmarSelecao(cicloPersonalizado, botaoOK);
      }
      break;
    case 2:
      confirmarSelecao(alterarTemperatura, botaoOK);
      break;
    case 3:
      confirmarSelecao(alterarVolumeSolucao, botaoOK);
      break;
    case 4:
      confirmarSelecao(alterarTempoCirculacao, botaoOK);
      break;
    }
    safeDelay(2000);
  }
}

void confirmarSelecao(void (*funcao)(), uint8_t botao)
{
  safeDelay(500);
  lcd.clear();

  for (float i = timer / 2; i >= 0; i--) // timer de espera do clique de confirmacao
  {
    printOpcoesLCD("Pressione", "Novamente " + String(i / 10));
    if (!digitalRead(botao))
    {
      funcao();
      break;
    }
    if (i == 0)
    {
      lcd.clear();
      printOpcoesLCD("Operacao", "cancelada");
      safeDelay(1500);
      break;
    }
    delay(100);
  }
  lcd.clear();
}

void alterarVolumeSolucao()
{
  // salvando volume das solucoes em um vetor
  float volSolucao[] = {volAlcPersonalizado, volAcidPersonalizado, volSanitPersonalizado};
  lcd.clear();
  printOpcoesLCD("Alterar Volumes", "");

  for (uint8_t i = 0; i < sizeof(volSolucao) / sizeof(volSolucao[0]); i++) // passando pelo array de solucoes
  {
    // timer para evitar o bounce do pushbutton
    safeDelay(1000);
    lcd.clear();
    while (digitalRead(botaoOK))
    {
      // imprimindo a solucao atual que esta sendo alterada
      Serial.print(solucao[i]);
      Serial.print(" : ");
      Serial.println(volSolucao[i]);
      printOpcoesLCD(solucao[i], String(volSolucao[i]) + " mL");

      // incrementa o valo ao botao ser pressionado
      if (!digitalRead(botaoSetaDireita))
      {
        volSolucao[i] += 0.1;
        delay(delaySetas);
        lcd.clear();
      }
      // decrementa o valo ao botao ser pressionado
      if (!digitalRead(botaoSetaEsquerda))
      {
        // garante que o valor nao desca mais que zero e estoure o vetor
        if (volSolucao[i] > 0)
        {
          volSolucao[i] -= 0.1;
          delay(delaySetas);
          lcd.clear();
        }
      }
    }
  }

  // atribuindo os valores do array para as devidas variaveis
  volAlcPersonalizado = volSolucao[0];
  volAcidPersonalizado = volSolucao[1];
  volSanitPersonalizado = volSolucao[2];

  // confirma se deseja salvar dados na memoria
  Serial.println("Salvar na memoria ?");
  lcd.clear();
  printOpcoesLCD("Salvar", "na memoria?");
  safeDelay(2000);
  confirmarSelecao(salvarSolucaoNaEEPROM, botaoOK);
}

void alterarTemperatura()
{
  // vetor de valores da temperatura das solucoes e ciclos
  uint8_t vetorTemp[] = {tempPreEnxaguePersonalizado, tempAlcPersonalizado, tempAcidPersonalizado};

  // passando pelos arrays
  for (uint8_t i = 0; i < sizeof(vetorTemp) / sizeof(uint8_t); i++)
  {
    // timer para evitar o bounce do pushbutton
    safeDelay(500);
    lcd.clear();

    while (digitalRead(botaoOK))
    {
      wdt_reset();
      // imprimindo a solucao atual que esta sendo alterada
      Serial.print(stringTemp[i]);
      Serial.print(" : ");
      Serial.println(vetorTemp[i]);

      printOpcoesLCD(stringTemp[i], String(vetorTemp[i]) + " C");
      // incrementa o valo ao votao ser pressionado
      if (!digitalRead(botaoSetaDireita))
      {
        vetorTemp[i] += 1;
        delay(delaySetas);
        lcd.clear();
      }
      // decrementa o valo ao votao ser pressionado
      if (!digitalRead(botaoSetaEsquerda))
      {
        // garante que o valor nao desca mais que zero
        if (vetorTemp[i] > 0)
        {
          vetorTemp[i] -= 1;
          delay(delaySetas);
          lcd.clear();
        }
      }
    }
  }
  // atribuindo temperatura do vetor as variaveis
  tempPreEnxaguePersonalizado = vetorTemp[0];
  tempAlcPersonalizado = vetorTemp[1];
  tempAcidPersonalizado = vetorTemp[2];

  // confirma se deseja salvar dados na memoria
  Serial.println("Salvar na memoria ?");
  lcd.clear();
  printOpcoesLCD("Salvar", "na memoria?");
  safeDelay(2000);
  confirmarSelecao(salvarTempNaEEPROM, botaoOK);
}

void alterarTempoCirculacao()
{
  uint8_t minutos = (uint8_t)(tempoCirculacaoSolucao / 60000UL);
  if (minutos == 0)
    minutos = 1;

  safeDelay(500);
  lcd.clear();

  while (digitalRead(botaoOK))
  {
    wdt_reset();
    printOpcoesLCD("Circulacao", String(minutos) + " min");
    Serial.print("Circulacao: ");
    Serial.print(minutos);
    Serial.println(" min");

    if (!digitalRead(botaoSetaDireita) && minutos < 60)
    {
      minutos++;
      delay(delaySetas);
      lcd.clear();
    }
    if (!digitalRead(botaoSetaEsquerda) && minutos > 1)
    {
      minutos--;
      delay(delaySetas);
      lcd.clear();
    }
  }

  tempoCirculacaoSolucao = (unsigned long)minutos * 60000UL;

  Serial.println("Salvar na memoria ?");
  lcd.clear();
  printOpcoesLCD("Salvar", "na memoria?");
  safeDelay(2000);
  confirmarSelecao(salvarTempoCirculacaoNaEEPROM, botaoOK);
}

void cicloPersonalizado()
{
  safeDelay(2000);
  Serial.println();

  uint8_t countPercorrerCicloPersonalizado = 0;

  lcd.clear();
  while (digitalRead(botaoOK))
  {
    wdt_reset();
    Serial.println();
    Serial.print(percorrerCicloPersonalinado[countPercorrerCicloPersonalizado]);
    printOpcoesLCD("Personalizado", percorrerCicloPersonalinado[countPercorrerCicloPersonalizado]);
    Serial.println(countPercorrerCicloPersonalizado);
    // navegando para a direita sobre as opcoes
    if (!digitalRead(botaoSetaDireita) && countPercorrerCicloPersonalizado < 2)
    {
      countPercorrerCicloPersonalizado++;
      delay(delaySetas);
      lcd.clear();
    }

    // navegando para a esquerda sobre as opcoes
    if (!digitalRead(botaoSetaEsquerda) && countPercorrerCicloPersonalizado > 0)
    {
      countPercorrerCicloPersonalizado--;
      delay(delaySetas);
      lcd.clear();
    }
  }
  lcd.clear();
  if (countPercorrerCicloPersonalizado == 0)
  {
    confirmarSelecao(usarCicloPersonalizado, botaoOK);
  }
  if (countPercorrerCicloPersonalizado == 1)
  {
    confirmarSelecao(criarCicloPersonalizado, botaoOK);
  }
  if (countPercorrerCicloPersonalizado == 2)
  {
    while (digitalRead(botaoRemover))
    {
      printarCicloPersonalizado();
      printOpcoesLCD("", "Ciclo salvo");
    }
  }
}
