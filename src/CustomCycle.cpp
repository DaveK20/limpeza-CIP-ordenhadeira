#include "CustomCycle.h"
#include <avr/wdt.h>
#include <EEPROM.h>
#include "Config.h"
#include "Globals.h"
#include "Display.h"
#include "Utils.h"
#include "CleaningCycle.h"
#include "Storage.h"
#include "Menu.h"

// vetor com o nome das possiveis rotinas (usado no painel de montagem do ciclo)
static const char *possiveisRotinas[5] = {"Pre-enxague", "Enxague", "Alcalina", "Acida", "Sanitizante"};

void criarCicloPersonalizado()
{
  // 1 - pre-enxague
  // 2 - lavagem intermitente
  // 3 - ciclo base
  // 4 - ciclo acido
  // 5 - ciclo sanitizante

  // vetor com as possiveis selecoes das rotinas
  uint8_t percorrerPossiveisRotinas = 0; // percorrendo vetor de string representando as possiveis rotinas
  uint8_t percorrerRotinas = 0;          // percorrendo o vetor de rotinas salvas
  lcd.clear();
  printOpcoesLCD("Criar", "ciclo");
  safeDelay(2000);
  lcd.clear();

  // zerando todas as posicoes do vetor
  for (uint8_t i = 0; i < tamVetorRotinas; i++)
  {
    vetorRotinas[i] = 255;
  }

  while (digitalRead(botaoInterromperOperacao))
  {
    wdt_reset();
    Serial.println(possiveisRotinas[percorrerPossiveisRotinas]);
    Serial.println(percorrerRotinas);

    // incrementando o vetor para visualizar as possiveis rotinas
    if (!digitalRead(botaoSetaDireita) && percorrerPossiveisRotinas < 4)
    {
      percorrerPossiveisRotinas++;
      delay(delaySetas);
      lcd.clear();
    }

    // decrementando o vetor para visualizar as possiveis rotinas
    if (!digitalRead(botaoSetaEsquerda) && percorrerPossiveisRotinas > 0)
    {
      percorrerPossiveisRotinas--;
      delay(delaySetas);
      lcd.clear();
    }

    // adicionando a rotina especifica selecionada a posicao do vetor
    if (!digitalRead(botaoOK))
    {
      if (percorrerRotinas < tamVetorRotinas)
      {
        switch (percorrerPossiveisRotinas)
        {
        case 0:
          vetorRotinas[percorrerRotinas] = 1;
          break;
        case 1:
          vetorRotinas[percorrerRotinas] = 2;
          break;
        case 2:
          vetorRotinas[percorrerRotinas] = 3;
          break;
        case 3:
          vetorRotinas[percorrerRotinas] = 4;
          break;
        case 4:
          vetorRotinas[percorrerRotinas] = 5;
          break;
        }
        percorrerRotinas++;
      }
      else
      {
        printOpcoesLCD("VETOR CHEIO", "");
        Serial.println("vetor cheio");
        delay(delaySetas * 3);
        lcd.clear();
      }
      delay(delaySetas);
    }

    // decrementando o vetor caso botaoRemover seja acionado
    if (!digitalRead(botaoRemover))
    {
      Serial.println("REMOVER");
      // decrementando posicao do vetor e atribuindo 255 ao seu valor
      if (percorrerRotinas > 0 && percorrerRotinas <= tamVetorRotinas)
      {
        lcd.clear();
        percorrerRotinas--;
        vetorRotinas[percorrerRotinas] = 255;
        Serial.println("removendo item");
        printOpcoesLCD("Removendo etapa", "");
      }
      else // garante que o vetor nao esteja vazio
      {
        lcd.clear();
        Serial.println("vetor vazio");
        printOpcoesLCD("VETOR VAZIO", "");
        delay(delaySetas * 3);
      }
      delay(delaySetas);
      lcd.clear();
    }

    printOpcoesLCD(printarCicloPersonalizado(), possiveisRotinas[percorrerPossiveisRotinas]);
  }

  lcd.clear();
  Serial.println("Salvar ciclo na memoria?");
  printOpcoesLCD("Salvar ciclo", "na memoria?");
  safeDelay(1500);
  confirmarSelecao(salvarCicloPersonalizadoNaEEPROM, botaoOK);
}

String printarCicloPersonalizado()
{
  Serial.println("printarCicloPersonalizado()");
  String printarOrdemCiclo;

  for (uint8_t count = 0; count < tamVetorRotinas; count++)
  {
    // imprimindo posicoes do vetor ignorando as que possuem 255 como valor
    if (vetorRotinas[count] != 255)
    {
      printarOrdemCiclo += String(vetorRotinas[count]);
      printarOrdemCiclo += " ";
    }
  }

  return printarOrdemCiclo;
}

void usarCicloPersonalizado()
{
  wdt_reset();
  Serial.println("usando ciclo salvo na memoria");
  lcd.clear();
  printOpcoesLCD("Usando ciclo", "salvo");
  safeDelay(2000);
  for (uint8_t i = 0; i < sizeof(vetorRotinas) / sizeof(vetorRotinas[0]); i++)
  {
    Serial.print(" ");
    Serial.print(EEPROM.read(i));

    if (EEPROM.read(i) == 1) // pre-enxague
      rotinaPreEnxague();
    else if (EEPROM.read(i) == 2) // lavagem intermitente
      rotinaEnxague();
    else if (EEPROM.read(i) == 3) // rotina base
      rotinaSolucao(1, volAlc, tempAlc);
    else if (EEPROM.read(i) == 4) // rotina acida
      rotinaSolucao(2, volAcid, tempAcid);
    else if (EEPROM.read(i) == 5) // rotina sanitizante
      rotinaSanitizante();

    delay(delaySetas);
  }
  interrupcao();
}
