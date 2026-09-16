#include "Storage.h"
#include <EEPROM.h>
#include "Config.h"
#include "Globals.h"
#include "Display.h"
#include "Utils.h"

/**
 * @brief converte e salva o valor da solucao na EEPROM com um erro de 0.05ml
 *
 * @param solucao a ser convertida
 * @param posicao a ser salva na memoria
 */
static void converterProgramaParaEEPROM(float solucao, uint8_t posicao)
{
  // calculo para que possa ser possivel salvar um float na EEPROM
  int aux = solucao * 100;
  aux /= 5;
  Serial.print(aux);

  EEPROM.update(posicao, aux);
}

/**
 * @brief converte os valores que estao sendo pegos na EEPROM
 *
 * @param posicao da memoria na EEPROM definidas como 10 (alcalina),11 (acida) e 12 (sanitizante)
 * @return float
 */
static float converterEEPROMParaPrograma(uint8_t posicao)
{
  // puxando valores e convertendo da EEPROM para o programa
  return (float)EEPROM.read(posicao) * 5.0f / 100.0f;
}

void pegarVolSolucaoEEPROM()
{
  volAlc = converterEEPROMParaPrograma(EEPROM_ALC);
  volAcid = converterEEPROMParaPrograma(EEPROM_ACID);
  volSanit = converterEEPROMParaPrograma(EEPROM_SANIT);

  volAlcPersonalizado = volAlc;
  volAcidPersonalizado = volAcid;
  volSanitPersonalizado = volSanit;
}

void salvarSolucaoNaEEPROM()
{
  Serial.println("-SALVANDO NA EEPROM-");
  lcd.clear();
  printOpcoesLCD("Salvando", "solucao");
  delay(1000);

  Serial.print("Alc:");
  converterProgramaParaEEPROM(volAlcPersonalizado, EEPROM_ALC);
  Serial.println();
  Serial.print("Acid:");
  converterProgramaParaEEPROM(volAcidPersonalizado, EEPROM_ACID);
  Serial.println();
  Serial.print("Sanit:");
  converterProgramaParaEEPROM(volSanitPersonalizado, EEPROM_SANIT);
  Serial.println();

  pegarVolSolucaoEEPROM();
}

void pegarTempSolucaoEEPROM()
{
  if (EEPROM.read(EEPROM_TEMP_PRE_EXAGUE) != 255)
    tempPreEnxaguePersonalizado = EEPROM.read(EEPROM_TEMP_PRE_EXAGUE);
  else
    tempPreEnxaguePersonalizado = 45;

  if (EEPROM.read(EEPROM_TEMP_ALC) != 255)
    tempAlcPersonalizado = EEPROM.read(EEPROM_TEMP_ALC);
  else
    tempAlcPersonalizado = 75;

  if (EEPROM.read(EEPROM_TEMP_ACID) != 255)
    tempAcidPersonalizado = EEPROM.read(EEPROM_TEMP_ACID);
  else
    tempAcidPersonalizado = 55;

  tempPreEnxague = tempPreEnxaguePersonalizado;
  tempAlc = tempAlcPersonalizado;
  tempAcid = tempAcidPersonalizado;
}

void salvarTempNaEEPROM()
{
  Serial.println("-SALVANDO NA EEPROM-");
  lcd.clear();
  printOpcoesLCD("Salvando", "temperatura");
  safeDelay(2000);
  EEPROM.update(EEPROM_TEMP_PRE_EXAGUE, tempPreEnxaguePersonalizado);
  EEPROM.update(EEPROM_TEMP_ALC, tempAlcPersonalizado);
  EEPROM.update(EEPROM_TEMP_ACID, tempAcidPersonalizado);
  pegarTempSolucaoEEPROM();
}

void pegarTempoCirculacaoEEPROM()
{
  uint8_t minutos = EEPROM.read(EEPROM_TEMPO_CIRCULACAO);
  if (minutos == 255 || minutos == 0)
  {
    minutos = 5; // padrao: 5 minutos
  }
  tempoCirculacaoSolucao = (unsigned long)minutos * 60000UL;

  Serial.print("Tempo circulacao: ");
  Serial.print(minutos);
  Serial.println(" min");
}

void salvarTempoCirculacaoNaEEPROM()
{
  uint8_t minutos = (uint8_t)(tempoCirculacaoSolucao / 60000UL);
  Serial.println("-SALVANDO TEMPO CIRCULACAO NA EEPROM-");
  lcd.clear();
  printOpcoesLCD("Salvando", "circulacao");
  EEPROM.update(EEPROM_TEMPO_CIRCULACAO, minutos);
  safeDelay(1500);
  lcd.clear();
}

void pegarCicloDaEEPROM()
{
  for (uint8_t count = 0; count < tamVetorRotinas; count++)
  {
    vetorRotinas[count] = EEPROM.read(count);
  }
}

void salvarCicloPersonalizadoNaEEPROM()
{
  Serial.println("salvando ciclo na memoria...");
  lcd.clear();
  printOpcoesLCD("Salvando", "ciclo");
  delay(500);
  for (uint8_t i = 0; i < tamVetorRotinas; i++)
  {
    switch (vetorRotinas[i])
    {
    case 1:
      EEPROM.update(i, 1); // pre-enxague
      break;
    case 2:
      EEPROM.update(i, 2); // lavagem intermitente
      break;
    case 3:
      EEPROM.update(i, 3); // ciclo base
      break;
    case 4:
      EEPROM.update(i, 4); // ciclo acido
      break;
    case 5:
      EEPROM.update(i, 5); // ciclo sanitizante
      break;
    default:
      EEPROM.update(i, 255);
    }
  }
  Serial.println("ciclo salvo!");
  delay(500);
  lcd.clear();
}
