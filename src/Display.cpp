#include "Display.h"
#include "Config.h"
#include "Globals.h"

LiquidCrystal_I2C lcd(ende, col, lin); // Chamada da funcao LiquidCrystal para ser usada com o I2C

/**
 * @brief calcula a posicao central a ser escrita no LCD I2C
 *
 * @param palavra a ser alinhada
 * @return posicao central a ser escrita
 */
static uint8_t requiredOffset(String palavra)
{
  uint8_t offset = palavra.length();
  offset = offset / 2;
  return 8 - offset;
}

void displayInit()
{
  lcd.init();      // Serve para iniciar a comunicacao com o display ja conectado
  lcd.backlight(); // Serve para ligar a luz do display
}

void printOpcoesLCD(String linha0, String linha1)
{
  lcd.setCursor(15, 0);
  if (interromper == true)
  {
    lcd.print("*");
  }
  else
  {
    lcd.print(" ");
  }

  lcd.setCursor(15, 1);
  if (statusSensorTemperatura == false)
  {
    lcd.print("!");
  }
  else
  {
    lcd.print(" ");
  }

  lcd.setCursor(requiredOffset(linha0), 0);
  lcd.print(linha0);

  lcd.setCursor(requiredOffset(linha1), 1);
  lcd.print(linha1);
}
