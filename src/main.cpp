/**
 * @file main.cpp
 * @author DaveK2 (davefr@outlook.com.br)
 * @brief CIP ordenhadeira Campus Bom Jesus do Itabapoana
 * @version 0.8.3.1
 * @date 2023-02-15
 *
 * @copyright Copyright (c) 2023
 *
 */

/*
OBJETIVOS RESTANTES
  [] - ter uma nocao do tempo necessario para esvaziar o tanque e mostrar no display
  [x] - ciclo personalizado
  [] - precisao das bombas peristalticas
  [] - controle da contatora
  [] - controle da resistencia
  [] - painel de controle para o usuario
  [] - primeiro teste do prototipo
  [] - display de informacoes uteis

  IMPORTANTE!!!
  Olhar rotinas CIP e corrigir inconsistencias
  remover variaveis desnecessarias

  correcao do interrupt, fazer com que o while funcione somente enqunato o interrupt seja falso e colocar
  um tempo para que o interrupt seja desativado novamente com millis()
*/

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <String.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

void estadoBombas(uint8_t, uint8_t, uint8_t);
void aquecerResistencia(uint8_t);
void misturar(uint8_t);
void esvaziarTanque(float);
void adicionarSolucao(float, uint8_t);
void converterProgramaParaEEPROM(float, uint8_t);
void confirmarSelecao(void (*funcao)(), uint8_t botao);
void printOpcoesLCD(String linha0, String linha1);
void printInformacoes();
void encherTanque();
void rotinaPreEnxague();
void rotinaEnxague();
void rotinaBase();
void rotinaAcida();
void rotinaSanitizante();
void interromperOperacao();
void cicloCIP();
void cicloPersonalizado();
void criarCicloPersonalizado();
void selecionarOpcao();
void alterarVolumeSolucao();
void pegarVolSolucaoEEPROM();
void salvarCicloPersonalizado();
void usarCicloPersonalizado();
void salvarSolucaoNaEEPROM();
void interrupcao();
void alterarTemperatura();
void pegarTempSolucaoEEPROM();
void salvarTempNaEEPROM();
void printOpcoesLCD();
void printarCicloPersonalizado();

uint8_t requiredOffset(String palavra);
float converterEEPROMParaPrograma(uint8_t posicao);
float calcSolucao(float);
float tempAgua();

// CONTROLE RELAYS
#define pinBoia 3             // futuramente substituir pelo ultrassonico se necessario
#define relayAlc 4            // bomba peristaltica alcalina
#define relayAcid 5           // bomba peristaltica acida
#define relaySanit 6          // bomba peristaltica sanitizante
#define relayEncherTanque 7   // solenoide responsavel por encher tanque
#define relayEsvaziarTanque 8 // solenoide responsavel por esvaziar
// #define relayMisturador 9       // manter solucao diluida
// #define botaoTemperatura 9         // botao alterar temperaturas das solucoes
#define relayResistencia 10 // aquecer agua
// #define botaoCicloCip 12           // botao inicializar ciclo cip
// #define botaoCicloPersonalizado 13 // botao selecao, criacao de ciclo personalizado
#define botaoRemover 14      // botao alterar volume das solucoes
#define botaoSetaDireita 9   // botao de interacao com o sistema
#define botaoSetaEsquerda 13 // botao de interacao com o sistema
#define botaoOK 12           // botao para confirmacao das selecoes

#define interruptPushButton 2 // botao de interrupcao de ciclo

// POSICOES NA EEPROM, ONDE AS SOLUCOES SERAO SALVAS
#define EEPROM_ALC 10
#define EEPROM_ACID 11
#define EEPROM_SANIT 12

// POSICOES NA EEPROM, ONDE AS TEMPERATURAS SERAO SALVAS
#define EEPROM_TEMP_PRE_EXAGUE 13
#define EEPROM_TEMP_ALC 14
#define EEPROM_TEMP_ACID 15
#define EEPROM_TEMP_SANIT 16

#define col 16    // Serve para definir o numero de colunas do display utilizado
#define lin 2     // Serve para definir o numero de linhas do display utilizado
#define ende 0x27 // Serve para definir o endereço do display.

LiquidCrystal_I2C lcd(ende, col, lin); // Chamada da funcação LiquidCrystal para ser usada com o I2C

// PINOS LEITURA
#define tempSensor 11 // DS18B20
OneWire oneWire(tempSensor);
DallasTemperature sensors(&oneWire); // encaminha referências OneWire para o sensor

// VOLUME DE SOLUCAO A SER INSERIDO NO SISTEMA
float volAlc = 0.75;
float volAcid = 2.5;
float volSanit = 0; // ainda nao descoberto

// VOLUME DE SOLUCAO INSERIDO PELO USUARIO E A SER SALVO NA EEPROM
float volAlcPersonalizado = 0;
float volAcidPersonalizado = 0;
float volSanitPersonalizado = 0;

// TEMPERATURAS IDEAIS DA AGUA
// De acordo com o artigo "Limpeza e Desinfecção de Equipamentos de Ordenha e Tanques" de MARCOS VEIGA SANTOS
uint8_t tempPreEnxague = 55; // temperatura na primeira lavagem
uint8_t tempAlc = 75;        // solucao base
uint8_t tempAcid = 43;       // solucao acida
uint8_t tempSanit = 0;       // solucao sanitizante

uint8_t tempAlcPersonalizado = 0;
uint8_t tempAcidPersonalizado = 0;
uint8_t tempSanitPersonalizado = 0;
uint8_t tempPreEnxaguePersonalizado = 0;

/*
  23/11/2022 - Calibração das bombas peristálticas com copo graduado impreciso
  100ml ~ 65 segundos
  aproximadamente 1,538 ml/s
  */
float um_ml = 1.66;                                                            // volume de despejado por 1s
float ml_inserido = 200;                                                       // volume inserido como teste
float bomba_delay = (ml_inserido / um_ml) * 1000;                              // calculo de despejo da bomba
uint8_t timer = 100;                                                           // timer de espera para o usuario apertar o botao
uint8_t vetorRotinas[10] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255}; // vetor para salvar um ciclo de limpeza personalizado
int delaySetas = 300;                                                          // delay das setas de selecao
uint8_t percorrerOpcoes = 0;                                                   // opcoes do vetor de selecao

#define DELAY_ESVAZIAR_TANQUE 18000 // tempo estimado para que o tanque fique vazio
#define COLETA_DELAY 3000           // coleta e amostragem dos dados de temperatura e status das bombas
bool interromper = false;           // interromper ciclo

unsigned long tempo_ultima_coleta;

// painel de selecao selecionarOpcao()
const char *opcoes[] = {"CIP", "Personalizado", "Temperatura", "Solucao", "Monitoramento"};
// painel de selecao cicloPersonalizado()
const char *percorrerCicloPersonalinado[] = {"Usar Ciclo", "Novo Ciclo", "Verificar ciclo"};
// painel de selecao criarCicloPersonalizado()
const char *possiveisRotinas[5] = {"Pre-enxague", "Enxague", "Alcalina", "Acida", "Sanitizante"};

void setup()
{
  Serial.begin(9600);
  pinMode(relayAcid, OUTPUT);
  pinMode(relayAlc, OUTPUT);
  pinMode(relaySanit, OUTPUT);
  pinMode(relayEncherTanque, OUTPUT);
  pinMode(relayEsvaziarTanque, OUTPUT);
  // pinMode(relayMisturador, OUTPUT);
  pinMode(relayResistencia, OUTPUT);
  pinMode(pinBoia, INPUT_PULLUP);
  // pinMode(botaoCicloCip, INPUT);
  // pinMode(botaoCicloPersonalizado, INPUT);
  pinMode(botaoSetaDireita, INPUT);
  pinMode(botaoSetaEsquerda, INPUT);
  pinMode(botaoRemover, INPUT);
  // pinMode(botaoTemperatura, INPUT);
  pinMode(botaoOK, INPUT);
  pinMode(interruptPushButton, INPUT);

  attachInterrupt(digitalPinToInterrupt(interruptPushButton), interromperOperacao, CHANGE);

  estadoBombas(HIGH, HIGH, HIGH);
  digitalWrite(relayEncherTanque, HIGH);
  digitalWrite(relayEsvaziarTanque, HIGH);
  // digitalWrite(relayMisturador, HIGH);
  digitalWrite(relayResistencia, HIGH);

  Serial.println("Inicializando sistema...");
  sensors.begin();

  EEPROM.begin();

  pegarVolSolucaoEEPROM();
  pegarTempSolucaoEEPROM();

  lcd.init();      // Serve para iniciar a comunicação com o display já conectado
  lcd.backlight(); // Serve para ligar a luz do display
  lcd.clear();     // Serve para limpar a tela do display
}

void loop()
{
  /*
  if (teste == false)
  {
    estadoBombas(LOW, LOW, LOW);
    Serial.println(calcSolucao(volAcid));
    delay(bomba_delay);
  }
  estadoBombas(HIGH, HIGH, HIGH);
  teste = true;



  if (digitalRead(botaoOK) == HIGH)
  {
    Serial.println("alto");
  }
  else
  {
    Serial.println("baixo");
  }*/

  selecionarOpcao();
}

/**
 * @brief printa informacoes no centro da tela
 *
 * @param linha0 coluna 0, linha 0 do LCD
 * @param linha1 coluna 0, linha 1 do LCD
 */
void printOpcoesLCD(String linha0, String linha1)
{
  // lcd.setCursor(col, line);
  lcd.setCursor(requiredOffset(linha0), 0);
  lcd.print(linha0);

  lcd.setCursor(requiredOffset(linha1), 1);
  lcd.print(linha1);
}

/**
 * @brief printa no centro do LCD I2C
 *
 * @param palavra a ser verificada
 * @return posicao central a ser escrita
 */
uint8_t requiredOffset(String palavra)
{
  uint8_t offset = palavra.length();
  offset = offset / 2;
  return 8 - offset;
}

/**
 * @brief botoes de selecao de ciclos e do volume das solucoes
 *
 */
void selecionarOpcao()
{
  Serial.println();
  Serial.print(opcoes[percorrerOpcoes]);

  printOpcoesLCD("Ciclo", opcoes[percorrerOpcoes]);

  if (digitalRead(botaoSetaDireita))

    if (digitalRead(botaoSetaDireita) == HIGH && percorrerOpcoes < 4)
    {
      percorrerOpcoes++;
      delay(delaySetas);
      lcd.clear();
    }

  if (digitalRead(botaoSetaEsquerda) == HIGH && percorrerOpcoes > 0)
  {
    percorrerOpcoes--;
    delay(delaySetas);
    lcd.clear();
  }

  if (digitalRead(botaoOK) == HIGH)
  {
    switch (percorrerOpcoes)
    {
    case 0:
      confirmarSelecao(cicloCIP, botaoOK);
      break;
    case 1:
      confirmarSelecao(cicloPersonalizado, botaoOK);
      break;
    case 2:
      confirmarSelecao(alterarTemperatura, botaoOK);
      break;
    case 3:
      confirmarSelecao(alterarVolumeSolucao, botaoOK);
      break;
    case 4:
      while (digitalRead(botaoRemover) == LOW)
      {
        printInformacoes();
      }
      break;
    }
    delay(1500);
  }
}

/**
 * @brief confirma ciclo selecionado
 *
 * @param funcao a ser selecionada
 * @param botao a ser pressionado para confirmacao
 */
void confirmarSelecao(void (*funcao)(), uint8_t botao)
{
  delay(1000);
  lcd.clear();
  for (float i = timer / 2; i >= 0; i--)
  {
    Serial.print("Pressione novamente para continuar ");
    printOpcoesLCD("Pressione", String(i / 10));
    if (digitalRead(botao) == HIGH)
    {
      funcao();
      break;
    }
    delay(100);
  }
  lcd.clear();
}

/**
 * @brief converte e salva o valor da solucao na EEPROM
 *
 * @param solucao a ser convertida
 * @param posicao a ser salva na memoria
 */
void converterProgramaParaEEPROM(float solucao, uint8_t posicao)
{
  int aux = solucao * 100;
  aux /= 5;
  Serial.print(aux);

  EEPROM.update(posicao, aux);
}

/**
 * @brief converte os valores que estao sendo pegos na EEPROM
 *
 * @param posicao da memoria na EEPROM definidas como 11 (alcalina),12 (acida) e 13 (sanitizante)
 * @return float
 */
float converterEEPROMParaPrograma(uint8_t posicao)
{
  float aux = EEPROM.read(posicao);
  aux /= 100;
  return aux * 5;
}

/**
 * @brief define os valores das solucoes com valores salvos na EEPROM 11 (alcalina),12 (acida) e 13 (sanitizante)
 *
 */
void pegarVolSolucaoEEPROM()
{
  volAlc = converterEEPROMParaPrograma(EEPROM_ALC);
  volAcid = converterEEPROMParaPrograma(EEPROM_ACID);
  volSanit = converterEEPROMParaPrograma(EEPROM_SANIT);

  volAlcPersonalizado = volAlc;
  volAcidPersonalizado = volAcid;
  volSanitPersonalizado = volSanit;
}

/**
 * @brief salva volume das solucoes na EEPROM
 *
 */
void salvarSolucaoNaEEPROM()
{
  Serial.println("-SALVANDO NA EEPROM-");
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

/**
 * @brief altera volume das solucoes a serem despejadas na agua
 *
 */
void alterarVolumeSolucao()
{
  // inicializando arrays para salvar os nomes e volumes das solucoes a serem alteradas
  String solucao[] = {"Alcalino", "Acido", "Sanitizante"};
  float volSolucao[] = {volAlcPersonalizado, volAcidPersonalizado, volSanitPersonalizado};

  printOpcoesLCD("Alterar Volumes", "");
  delay(1500);
  lcd.clear();
  // passando pelos arrays
  for (uint8_t i = 0; i < 3; i++)
  {
    // timer para evitar o bounce do pushbutton
    printOpcoesLCD(solucao[i], String(volSolucao[i]));
    delay(1500);
    lcd.clear();
    while (digitalRead(botaoOK) == LOW)
    {
      // imprimindo a solucao atual que esta sendo alterada
      Serial.print(solucao[i]);
      Serial.print(" : ");
      Serial.println(volSolucao[i]);

      // incrementa o valo ao votao ser pressionado
      if (digitalRead(botaoSetaDireita) == HIGH)
      {
        volSolucao[i] += 0.1;
        delay(delaySetas);
        lcd.clear();
      }
      // decrementa o valo ao votao ser pressionado
      if (digitalRead(botaoSetaEsquerda) == HIGH)
      {
        // garante que o valor não desça mais que zero
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
  printOpcoesLCD("Salvar na memoria ?", "");
  confirmarSelecao(salvarSolucaoNaEEPROM, botaoOK);
}

/**
 * @brief alterar temperatura da agua aquecida
 *
 */
void alterarTemperatura()
{
  // inicializando arrays para salvar os nomes e volumes das solucoes a serem alteradas
  String stringTemp[] = {"Pre-enxague", "Alcalino", "Acido", "Sanitizante"};
  uint8_t vetorTemp[] = {tempPreEnxaguePersonalizado, tempAlcPersonalizado, tempAcidPersonalizado, tempSanitPersonalizado};

  // passando pelos arrays
  for (uint8_t i = 0; i < 4; i++)
  {
    // timer para evitar o bounce do pushbutton
    delay(500);
    lcd.clear();
    while (digitalRead(botaoOK) == LOW)
    {
      // imprimindo a solucao atual que esta sendo alterada
      Serial.print(stringTemp[i]);
      Serial.print(" : ");
      Serial.println(vetorTemp[i]);

      printOpcoesLCD(stringTemp[i], String(vetorTemp[i]));
      // incrementa o valo ao votao ser pressionado
      if (digitalRead(botaoSetaDireita) == HIGH)
      {
        vetorTemp[i] += 1;
        delay(delaySetas);
        lcd.clear();
      }
      // decrementa o valo ao votao ser pressionado
      if (digitalRead(botaoSetaEsquerda) == HIGH)
      {
        // garante que o valor não desça mais que zero
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
  tempSanitPersonalizado = vetorTemp[3];

  // atribuindo os valores do array para as devidas variaveis
  Serial.println("Temps");
  Serial.print("Pré-Enxague: ");
  Serial.println(tempPreEnxaguePersonalizado);
  Serial.print("Alc:");
  Serial.println(tempAlcPersonalizado);
  Serial.print("Acid:");
  Serial.println(tempAcidPersonalizado);
  Serial.print("Sanit:");
  Serial.println(tempSanitPersonalizado);

  // confirma se deseja salvar dados na memoria
  Serial.println("Salvar na memoria ?");
  confirmarSelecao(salvarTempNaEEPROM, botaoOK);
}

/**
 * @brief salva temperatura dos ciclos na EEPROM
 *
 */
void salvarTempNaEEPROM()
{
  Serial.println("-SALVANDO NA EEPROM-");
  lcd.clear();
  printOpcoesLCD("Salvando", "temperatura");
  EEPROM.update(EEPROM_TEMP_PRE_EXAGUE, tempPreEnxaguePersonalizado);
  EEPROM.update(EEPROM_TEMP_ALC, tempAlcPersonalizado);
  EEPROM.update(EEPROM_TEMP_ACID, tempAcidPersonalizado);
  EEPROM.update(EEPROM_TEMP_SANIT, tempSanitPersonalizado);
  pegarTempSolucaoEEPROM();
}

/**
 * @brief define os valores das solucoes com valores salvos na EEPROM 11 (alcalina),12 (acida) e 13 (sanitizante)
 *
 */
void pegarTempSolucaoEEPROM()
{
  tempPreEnxaguePersonalizado = EEPROM.read(EEPROM_TEMP_PRE_EXAGUE);
  tempAlcPersonalizado = EEPROM.read(EEPROM_TEMP_ALC);
  tempAcidPersonalizado = EEPROM.read(EEPROM_TEMP_ACID);
  tempSanitPersonalizado = EEPROM.read(EEPROM_TEMP_SANIT);

  tempPreEnxague = tempPreEnxaguePersonalizado;
  tempAlc = tempAlcPersonalizado;
  tempAcid = tempAcidPersonalizado;
  tempSanit = tempSanitPersonalizado;

  /*
  Serial.print("Temp Pré-Enxague: ");
  Serial.println(EEPROM.read(EEPROM_TEMP_PRE_EXAGUE));
  Serial.print("Temp Alc:");
  Serial.println(EEPROM.read(EEPROM_TEMP_ALC));
  Serial.print("Temp Acid:");
  Serial.println(EEPROM.read(EEPROM_TEMP_ACID));
  Serial.print("Temp Sanit:");
  Serial.println(EEPROM.read(EEPROM_TEMP_SANIT));
  */
}

/**
 * @brief attachInterrupt responsavel por interromper os ciclos de limpeza
 *
 */
void interromperOperacao()
{
  Serial.println("cheguei na interrupcao");
  interromper = true;
}

/**
 * @brief esvazia o tanque caso a interrupcao seja true e ele esteja cheio
 *
 */
void interrupcao()
{
  if (interromper == true)
  {
    Serial.println("Rotina interrompida com sucesso!");
    interromper = false;
    if (digitalRead(pinBoia) == LOW)
    {
      esvaziarTanque(tempAgua());
    }
  }
}

/**
 * @brief controle do ciclo automatico de limpeza da ordenhadeira
 *
 */
void cicloCIP()
{
  Serial.println();
  Serial.println("Inicializando ciclo CIP...");
  printOpcoesLCD("Inicializando", "ciclo CIP...");
  // rotina pre-enxague
  rotinaPreEnxague();

  // rotina base
  rotinaBase();
  rotinaEnxague();

  // rotina acido
  rotinaAcida();
  rotinaEnxague();

  // rotina sanitizante
  rotinaSanitizante();

  // caso as acoes sejam canceladas
  interrupcao();
}

/**
 * @brief painel de selecao de ciclo de limpeza criado pelo usuario
 *
 */
void cicloPersonalizado()
{
  delay(1500);
  Serial.println();

  uint8_t countPercorrerCicloPersonalizado = 0;

  // adicionar uma forma de cancelar as selecoes de ciclo

  while (digitalRead(botaoOK) == LOW)
  {
    Serial.println();
    Serial.print(percorrerCicloPersonalinado[countPercorrerCicloPersonalizado]);
    printOpcoesLCD("Personalizado", percorrerCicloPersonalinado[countPercorrerCicloPersonalizado]);

    // navegando para a direita sobre as opcoes
    if (digitalRead(botaoSetaDireita) == HIGH && countPercorrerCicloPersonalizado < 2)
    {
      countPercorrerCicloPersonalizado++;
      delay(delaySetas);
      lcd.clear();
    }

    // navegando para a esquerda sobre as opcoes
    if (digitalRead(botaoSetaEsquerda) == HIGH && countPercorrerCicloPersonalizado > 0)
    {
      countPercorrerCicloPersonalizado--;
      delay(delaySetas);
      lcd.clear();
    }
  }

  switch (countPercorrerCicloPersonalizado)
  {
  case 0:
    confirmarSelecao(usarCicloPersonalizado, botaoOK);
    break;
  case 1:
    confirmarSelecao(criarCicloPersonalizado, botaoOK);
    break;
  case 2:
    lcd.clear();
    while (digitalRead(botaoRemover) == LOW)
    {
      Serial.println("Cheguei aqui");
      printarCicloPersonalizado();
    }
    break;
  }
  if (countPercorrerCicloPersonalizado == 1)
  {
    // criar ciclo personalizado
    confirmarSelecao(criarCicloPersonalizado, botaoOK);
  }
}

/**
 * @brief cria um ciclo CIP personalizado
 *
 */
void criarCicloPersonalizado()
{
  // 1 - pre-enxague
  // 2 - lavagem intermitente
  // 3 - ciclo base
  // 4 - ciclo acido
  // 5 - ciclo sanitizante

  // vetor com as possiveis selecoes das rotinas

  uint8_t percorrerPossiveisRotinas = 0; // percorrendo vetor de string  representando as possiveis rotinas
  uint8_t percorrerRotinas = 0;          // percorrendo o vetor de rotinas salvas
  lcd.clear();
  printOpcoesLCD("Criar", "ciclo");
  delay(1500);
  lcd.clear();

  while (digitalRead(interruptPushButton))
  {
    Serial.println(possiveisRotinas[percorrerPossiveisRotinas]);
    printOpcoesLCD("", possiveisRotinas[percorrerPossiveisRotinas]);

    uint8_t tamVetorRotinas = sizeof(vetorRotinas) / sizeof(vetorRotinas[0]);

    if (digitalRead(botaoSetaDireita) == HIGH && percorrerPossiveisRotinas < 4)
    {
      percorrerPossiveisRotinas++;
      delay(delaySetas);
      lcd.clear();
    }

    if (digitalRead(botaoSetaEsquerda) == HIGH && percorrerPossiveisRotinas > 0)
    {
      percorrerPossiveisRotinas--;
      delay(delaySetas);
      lcd.clear();
    }

    if (digitalRead(botaoOK) == HIGH)
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

      if (percorrerRotinas < tamVetorRotinas)
      {
        percorrerRotinas++;
      }
      else
      {
        lcd.clear();
        printOpcoesLCD("VETOR CHEIO", "");
        Serial.println("vetor cheio");
      }
      delay(delaySetas);
      lcd.clear();
    }

    if (digitalRead(botaoRemover) == HIGH)
    {
      if (percorrerRotinas > 0 && percorrerRotinas <= tamVetorRotinas)
      {
        lcd.clear();
        percorrerRotinas--;
        vetorRotinas[percorrerRotinas] = 255;
        Serial.println("removendo item");
        printOpcoesLCD("Removendo etapa", "");
      }
      else
      {
        Serial.println("vetor vazio");
        printOpcoesLCD("VETOR VAZIO", "");
      }
      delay(delaySetas);
      lcd.clear();
    }

    
    //essa porra ta dando problema corrigir ainda hoje
    printarCicloPersonalizado();
  }

  Serial.println("Salvar ciclo na memoria?");
  printOpcoesLCD("Salvar ciclo", "na memoria?");
  delay(1500);
  confirmarSelecao(salvarCicloPersonalizado, botaoOK);
}

/**
 * @brief imprimindo ciclo personalizado na memoria
 *
 */
void printarCicloPersonalizado()
{
  String printarOrdemCiclo;
  for (uint8_t count = 0; count < sizeof(vetorRotinas) / sizeof(uint8_t); count++)
  {
    if (vetorRotinas[count] != 255)
    {
      printarOrdemCiclo += String(vetorRotinas[count]);
      printarOrdemCiclo += " ";
      // printar o array de rotinas salvas
    }
  }
  printOpcoesLCD(printarOrdemCiclo, "");
}

/**
 * @brief usa ciclo de limpeza ja salvo na memoria
 *
 */
void usarCicloPersonalizado()
{
  Serial.println("usando ciclo salvo na memoria");
  printOpcoesLCD("Usando ciclo", "salvo");
  for (uint8_t i = 0; i < sizeof(vetorRotinas) / sizeof(int); i++)
  {
    Serial.print(" ");
    Serial.print(EEPROM.read(i));

    if (EEPROM.read(i) == 1) // pre-enxague
    {
      rotinaPreEnxague();
    }
    if (EEPROM.read(i) == 2) // lavagem intermitente
    {
      rotinaEnxague();
    }
    if (EEPROM.read(i) == 3) // rotina base
    {
      rotinaBase();
    }
    if (EEPROM.read(i) == 4) // rotina acida
    {
      rotinaAcida();
    }
    if (EEPROM.read(i) == 5) // rotina sanitizante
    {
      rotinaSanitizante();
    }
    delay(delaySetas);
  }
  interrupcao();
}

/**
 * @brief salva ciclo de limpeza personalizado
 *
 */
void salvarCicloPersonalizado()
{
  Serial.println("salvando ciclo na memoria...");
  lcd.clear();
  printOpcoesLCD("Salvando ciclo", "na memoria");
  delay(500);
  for (uint8_t i = 0; i < 10; i++)
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

/**
 * @brief enche o tanque de agua
 *
 * @param resistencia se igual a 1 aquece resistencia
 */
void encherTanque(uint8_t resistencia)
{
  if (interromper == false)
  {
    Serial.println("Enchendo tanque...");
    // futuramente tambem substituir isso por millis()
    while (digitalRead(pinBoia) == HIGH)
    {
      digitalWrite(relayResistencia, HIGH);
      digitalWrite(relayEncherTanque, LOW); // despejando agua no tanque
    }
    digitalWrite(relayEncherTanque, HIGH); // fechando valvula
    Serial.println("Tanque Cheio");
    misturar(LOW);
    if (resistencia == 1)
    {
      aquecerResistencia(LOW); // ligar resistencia
    }
    else
    {
      aquecerResistencia(HIGH); // desligar resistencia
    }
  }
}

/**
 * @brief despejando agua com solucao no sistema de limpeza
 *
 * @param tempSolucao a ser verificada
 */
void esvaziarTanque(float tempSolucao)
{
  if (interromper == false)
  {
    /*
  if (tempAgua() >= tempSolucao && digitalRead(pinBoia) == LOW)
  {
    Serial.println("Despejando agua...");
    aquecerResistencia(HIGH); // aquecer
    misturar(HIGH);
    digitalWrite(relayEsvaziarTanque, LOW);
    delay(1000); // tempo necessario para se agua do tanque
  }*/
    misturar(HIGH);
    aquecerResistencia(HIGH);
    Serial.println("Despejando agua...");
    digitalWrite(relayEsvaziarTanque, LOW);
    delay(DELAY_ESVAZIAR_TANQUE);
    Serial.println("Agua despejada");
    digitalWrite(relayEsvaziarTanque, HIGH);
  }
}

/**
 * @brief ativa/desativa misturador da agua com solucao
 *
 * @param status HIGH == desligado || LOW == ligado
 */
void misturar(uint8_t status)
{
  if (interromper == false)
  {
    if (status == LOW)
    {
      Serial.println("Ligando misturador");
      // digitalWrite(relayMisturador, LOW);
    }
    else
    {
      Serial.println("Desligando misturador");
      // digitalWrite(relayMisturador, HIGH);
    }
  }
}

/**
 * @brief controle da injecao de solucao na agua
 *
 * @param solucao a ser adicionada
 * @param relay a ser ativado => 1 - Acido || 2 - Base || 3 - Sanitizante
 */
void adicionarSolucao(float solucao, uint8_t relay)
{
  if (interromper == false)
  {
    Serial.print("Adicionando solucao ");
    if (relay == 1)
    {
      Serial.println("base");
      estadoBombas(LOW, HIGH, HIGH);
    }

    else if (relay == 2)
    {
      Serial.println("acida");
      estadoBombas(HIGH, LOW, HIGH);
    }

    else if (relay == 3)
    {
      Serial.println("sanitizante");
      estadoBombas(HIGH, HIGH, LOW);
    }
    delay(calcSolucao(solucao));
    estadoBombas(HIGH, HIGH, HIGH);
    Serial.println("Solucao Adicionada!");
  }
}

/**
 * @brief ligar resistencia
 *
 * @param status
 */
void aquecerResistencia(uint8_t status)
{
  if (interromper == false)
  {
    if (status == LOW)
    {
      Serial.println("Ligando resistencia");
      digitalWrite(relayResistencia, LOW);
    }
    else
    {
      Serial.println("Desligando resistencia");
      digitalWrite(relayResistencia, HIGH);
    }
  }
}

/**
 * @brief pre lavagem do tanque apos ordenha
 *
 */
void rotinaPreEnxague()
{
  Serial.println();
  Serial.println("ROTINA PRE-ENXAGUE");
  encherTanque(1);                // adicionar agua
  esvaziarTanque(tempPreEnxague); // liberar apos atingir temperatura
}

/**
 * @brief lavagem intermitente entre os ciclos acido e base
 *
 */
void rotinaEnxague()
{
  Serial.println();
  Serial.println("ROTINA ENXAGUE");
  encherTanque(0);            // adicionar agua
  esvaziarTanque(tempAgua()); // liberar apos atingir temperatura
}

/**
 * @brief rotina da solucao base adicionada na agua
 *
 */
void rotinaBase()
{
  Serial.println();
  Serial.println("ROTINA BASE");
  encherTanque(1);             // adicionar agua
  adicionarSolucao(volAlc, 1); // adicionar solucao
  esvaziarTanque(tempAlc);     // liberar apos atingir temperatura
}

/**
 * @brief rotina da solucao acida adicionada na agua
 *
 */
void rotinaAcida()
{
  Serial.println();
  Serial.println("ROTINA ACIDO");
  encherTanque(1);              // adicionar agua
  adicionarSolucao(volAcid, 2); // adicionar solucao
  esvaziarTanque(tempAcid);     // liberar apos atingir temperatura
}

/**
 * @brief rotina da solucao sanitizante adicionada na agua
 *
 */
void rotinaSanitizante()
{
  Serial.println();
  Serial.println("ROTINA SANITIZANTE");
  encherTanque(0);               // adicionar agua
  adicionarSolucao(volSanit, 3); // adicionar solucao
  esvaziarTanque(tempAgua());    // liberar apos atingir temperatura
}

/**
 * @brief quantidade de solucao a ser despejada em 50L de agua
 *
 * @param solucao
 * @return tempo(ms)
 */
float calcSolucao(float solucao)
{
  return ((solucao * 50) / um_ml) * 1000;
}

/**
 * @brief coletando temperatura do sensor DS18B20
 *
 * @return temperatura
 */
float tempAgua()
{
  sensors.requestTemperatures();
  if (sensors.isConversionComplete() == true)
  {
    return sensors.getTempCByIndex(0); // temp do sensor indice 0
  }
  if (sensors.isConversionComplete() == false)
  {
    Serial.println("Erro ao detectar sensor");
    return;
  }
}

/**
 * @brief controle das tres bombas peristalticas HIGH - Desativado || LOW - Ativado
 *
 * @param volAcid
 * @param volAlc
 * @param volSanit
 */
void estadoBombas(uint8_t volAlc, uint8_t volAcid, uint8_t volSanit)
{
  digitalWrite(relayAcid, volAlc);
  digitalWrite(relayAlc, volAcid);
  digitalWrite(relaySanit, volSanit);
}

/**
 * @brief printando informacoes na tela
 *
 */
void printInformacoes()
{

  /*
  if ((millis() - tempo_ultima_coleta) > COLETA_DELAY)
  {
    Serial.println();
    Serial.println("||----------------Monitoramento---------------||");
    Serial.print("BOMBA PERISTALTICA: ");
    if (digitalRead(relayAcid) == LOW)
    {
      Serial.println("adicionando acido...");
    }
    if (digitalRead(relayAlc) == LOW)
    {
      Serial.println("adicionando base...");
    }
    if (digitalRead(relaySanit) == LOW)
    {
      Serial.println("adicionando sanitizante...");
    }

    if (digitalRead(relayAcid) == HIGH || digitalRead(relayAlc) == HIGH || digitalRead(relaySanit) == HIGH)
    {
      Serial.println("desligada.");
    }

    Serial.print("BOIA: ");

    if (digitalRead(pinBoia) == HIGH)
    {
      Serial.println("Tanque vazio/esvaziando");
    }
    else
    {
      Serial.println("Acionada tanque cheio");
    }

    Serial.print("TEMP AGUA: ");
    Serial.println(tempAgua());

    Serial.println("VOLUME DAS SOLUCOES");
    Serial.print("Alcalino:");
    Serial.println(volAlc);
    Serial.print("Acido:");
    Serial.println(volAcid);
    Serial.print("Sanitizante:");
    Serial.println(volSanit);
    Serial.println();

    Serial.println("TEMPERATURA DA AGUA");
    Serial.print("Rotina pre-enxague: ");
    Serial.println(tempPreEnxague);
    Serial.print("Rotina alcalina: ");
    Serial.println(tempAlc);
    Serial.print("Rotina acida: ");
    Serial.println(tempAcid);
    Serial.print("Rotina sanitizante: ");
    Serial.println(tempSanit);
    Serial.println();

    Serial.println("CICLO PERSONALIZADO SALVO");
    for (uint8_t i = 0; i < sizeof(vetorRotinas) / sizeof(int); i++)
    {
      Serial.print(" ");
      Serial.print(EEPROM.read(i));
      Serial.print(" - ");
    }

    Serial.println();
    tempo_ultima_coleta = millis();
  }*/
}
