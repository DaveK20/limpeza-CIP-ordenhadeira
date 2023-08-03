/**
 * @file main.cpp
 * @author DaveK2 (davefr@outlook.com.br)
 * @brief CIP ordenhadeira Campus Bom Jesus do Itabapoana
 * @version 0.9
 * @date 2023-08-02
 *
 * @copyright Copyright (c) 2023
 *
 */

/*
OBJETIVOS RESTANTES
  [] - ter uma nocao do tempo necessario para esvaziar o tanque e mostrar no display
  [x] - ciclo personalizado
  [x] - precisao das bombas peristalticas
  [x] - controle da contatora
  [x] - controle da resistencia
  [x] - painel de controle para o usuario
  [] - primeiro teste do prototipo
  [x] - display de informacoes uteis

  IMPORTANTE!!!
  Olhar rotinas CIP e corrigir inconsistencias
  remover variaveis desnecessarias
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
void esvaziarTanque(float);
void confirmarSelecao(void (*funcao)(), uint8_t botao);
void printOpcoesLCD(String linha0, String linha1);
void printInformacoes();
void encherTanque(uint8_t, uint8_t);
void rotinaSolucao(uint8_t solucao, float volSolucao, uint8_t tempSolucao);
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
void pegarCicloDaEEPROM();

uint8_t requiredOffset(String palavra);
float converterEEPROMParaPrograma(uint8_t posicao);
float calcSolucao(float);
float tempAgua();

#define col 16    // Serve para definir o numero de colunas do display utilizado
#define lin 2     // Serve para definir o numero de linhas do display utilizado
#define ende 0x27 // Serve para definir o endereço do display.

// CONTROLE RELAYS
#define relayResistencia 10 // aquecer agua
#define relayAlc 45         // bomba peristaltica alcalina
#define relayAcid 43        // bomba peristaltica acida
#define relaySanit 41       // bomba peristaltica sanitizante
#define vs_ciclo 35         // valvula de inox
#define vs_vasao 33         // valvula de inox
#define vs_ts 39            // valvula de latao
#define vs_tm 37            // valvula de latao
#define ControleOrdenha 31  // acionamneto da succao da ordenha

#define botaoSetaEsquerda 40        // botao de interacao com o sistema AZUL
#define botaoOK 42                  // botao para confirmacao das selecoes
#define botaoSetaDireita 44         // botao de interacao com o sistema
#define botaoInterromperOperacao 46 // botao de interrupcao de ciclo
#define botaoRemover 48             // botao alterar volume das solucoes BRANCO
#define boiaSolucao 49              // boia do tanque de solucao
#define boiaMistura 666             // boia do tanque de mistura

#define tempSensor 10 // DS18B20

// POSICOES NA EEPROM, ONDE AS SOLUCOES SERAO SALVAS
#define EEPROM_ALC 10
#define EEPROM_ACID 11
#define EEPROM_SANIT 12

// POSICOES NA EEPROM, ONDE AS TEMPERATURAS SERAO SALVAS
#define EEPROM_TEMP_PRE_EXAGUE 13
#define EEPROM_TEMP_ALC 14
#define EEPROM_TEMP_ACID 15

#define INTERRUPT_DELAY 20000       // delay minimo até ser possivel acionar a proxima interrupcao
#define circularSolucao 300000      // tempo que a solucao alcalina/acida vai circular no sistema
#define DELAY_ESVAZIAR_TANQUE 20000 // tempo estimado para que o tanque fique vazio
#define COLETA_DELAY 3000           // coleta e amostragem dos dados de temperatura e status das bombas

LiquidCrystal_I2C lcd(ende, col, lin); // Chamada da funcação LiquidCrystal para ser usada com o I2C

// PINOS LEITURA

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

uint8_t tempAlcPersonalizado = 0;
uint8_t tempAcidPersonalizado = 0;
uint8_t tempPreEnxaguePersonalizado = 0;

// BOMBA PERISTALTICA
float fluxo_bomba = 2; // Fluxo em ml/s da bomba peristáltica previamente calibrada.
uint8_t vol_tanque = 50;

uint8_t timer = 100;         // timer de espera para o usuario apertar o botao
uint8_t vetorRotinas[10];    // vetor para salvar um ciclo de limpeza personalizado
uint16_t delaySetas = 300;   // delay das setas de selecao
uint8_t percorrerOpcoes = 0; // opcoes do vetor de selecao

bool interromper = false; // interromper ciclo

unsigned long tempo_ultima_coleta;
unsigned long last_interrupt_time;
float temperatura;

// Tamanho do vetor de rotinas
uint8_t tamVetorRotinas = sizeof(vetorRotinas) / sizeof(vetorRotinas[0]);

const char *opcoes[] = {"CIP", "Personalizado", "Temperatura", "Solucao"};                        // painel de selecao selecionarOpcao()
const char *percorrerCicloPersonalinado[] = {"Usar Ciclo", "Novo Ciclo", "Verificar ciclo"};      // painel de selecao cicloPersonalizado()
const char *possiveisRotinas[5] = {"Pre-enxague", "Enxague", "Alcalina", "Acida", "Sanitizante"}; // painel de selecao criarCicloPersonalizado()
const char *solucao[] = {"Alcalino", "Acido", "Sanitizante"};                                     // vetor do nome das solucoes a serem alteradas
const char *stringTemp[] = {"Pre-enxague", "Alcalino", "Acido"};                                  // vetor da temperatura das solucoes e ciclos

void setup()
{
  Serial.begin(9600);
  pinMode(relayAcid, OUTPUT);
  pinMode(relayAlc, OUTPUT);
  pinMode(relaySanit, OUTPUT);
  pinMode(relayResistencia, OUTPUT);
  pinMode(vs_ciclo, OUTPUT);
  pinMode(vs_vasao, OUTPUT);
  pinMode(vs_ts, OUTPUT);
  pinMode(vs_tm, OUTPUT);
  pinMode(ControleOrdenha, OUTPUT);

  pinMode(boiaSolucao, INPUT_PULLUP);
  pinMode(boiaMistura, INPUT_PULLUP);

  pinMode(botaoSetaEsquerda, INPUT_PULLUP);
  pinMode(botaoOK, INPUT_PULLUP);
  pinMode(botaoSetaDireita, INPUT_PULLUP);
  pinMode(botaoInterromperOperacao, INPUT_PULLUP);
  pinMode(botaoRemover, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(botaoInterromperOperacao), interromperOperacao, CHANGE);

  estadoBombas(HIGH, HIGH, HIGH);
  digitalWrite(relayResistencia, HIGH);

  Serial.println("Inicializando sistema...");
  sensors.begin();

  EEPROM.begin();

  pegarVolSolucaoEEPROM();
  pegarTempSolucaoEEPROM();
  pegarCicloDaEEPROM();

  lcd.init();      // Serve para iniciar a comunicação com o display já conectado
  lcd.backlight(); // Serve para ligar a luz do display
  lcd.clear();     // Serve para limpar a tela do display
}

void loop()
{
  // cicloCIP();

  digitalWrite(vs_tm, LOW);
  digitalWrite(vs_ts, LOW);
  digitalWrite(vs_ciclo, LOW);
  digitalWrite(vs_vasao, LOW);

  cicloCIP();

  /*if (!digitalRead(botaoSetaEsquerda))
  {
    Serial.println("ESQUERDA");
  }
  if (!digitalRead(botaoSetaDireita))
  {
    Serial.println("DIREITA");
  }
  if (!digitalRead(botaoOK))
  {
    Serial.println("OK");
  }
  if (!digitalRead(botaoRemover))
  {
    Serial.println("REMOVER");
  }
  if (!digitalRead(botaoInterromperOperacao))
  {
    Serial.println("INTERROMPER");
  }*/

  /*selecionarOpcao();

  if (interromper == true)
  {
    if ((millis() - last_interrupt_time) > INTERRUPT_DELAY)
    {
      lcd.clear();
      interromper = false;
      last_interrupt_time = millis();
    }
  }*/
}

/**
 * @brief printa informacoes no centro da tela
 *
 * @param linha0 coluna 0, linha 0 do LCD
 * @param linha1 coluna 0, linha 1 do LCD
 */
void printOpcoesLCD(String linha0, String linha1)
{
  lcd.setCursor(15, 0);
  if (interromper == true)
  {
    lcd.print("*");
  }
  else
  {
    lcd.print("");
  }

  lcd.setCursor(requiredOffset(linha0), 0);
  lcd.print(linha0);

  lcd.setCursor(requiredOffset(linha1), 1);
  lcd.print(linha1);
}

/**
 * @brief printa no centro do LCD I2C
 *
 * @param palavra a ser alinhada
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
  // percorrendo as posicoes do vetor
  Serial.print(opcoes[percorrerOpcoes]);
  printOpcoesLCD("Ciclo", opcoes[percorrerOpcoes]);

  if (digitalRead(botaoSetaDireita))
  {
    Serial.println("DIREITA");
    if (digitalRead(botaoSetaDireita) == HIGH && percorrerOpcoes < 3)
    {
      percorrerOpcoes++;
      delay(delaySetas);
      lcd.clear();
    }
  }

  if (digitalRead(botaoSetaEsquerda) == HIGH && percorrerOpcoes > 0)
  {
    Serial.println("ESQUERDA");
    percorrerOpcoes--;
    delay(delaySetas);
    lcd.clear();
  }

  if (digitalRead(botaoOK) == HIGH)
  {
    Serial.println("OK");
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
    }
    delay(1000);
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

  for (float i = timer / 2; i >= 0; i--) // timer de espera do clique de confirmacao
  {
    Serial.print("Pressione novamente para continuar ");
    printOpcoesLCD("Pressione", String(i / 10));
    if (digitalRead(botao) == HIGH)
    {
      funcao();
      break;
    }
    if (i == 0)
    {
      lcd.clear();
      printOpcoesLCD("Operacao", "cancelada");
      delay(1000);
      break;
    }
    delay(100);
  }
  lcd.clear();
}

/**
 * @brief converte e salva o valor da solucao na EEPROM com um erro de 0.05ml
 *
 * @param solucao a ser convertida
 * @param posicao a ser salva na memoria
 */
void converterProgramaParaEEPROM(float solucao, uint8_t posicao)
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
float converterEEPROMParaPrograma(uint8_t posicao)
{
  // puxando valores e convertendo da EEPROM para o programa
  float aux = EEPROM.read(posicao);
  aux /= 100;
  return aux * 5;
}

/**
 * @brief define os valores das solucoes com valores salvos na EEPROM 10 (alcalina),11 (acida) e 12 (sanitizante)
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

/**
 * @brief altera volume das solucoes a serem despejadas na agua
 *
 */
void alterarVolumeSolucao()
{
  // salvando volume das solucoes em um vetor
  float volSolucao[] = {volAlcPersonalizado, volAcidPersonalizado, volSanitPersonalizado};
  lcd.clear();
  printOpcoesLCD("Alterar Volumes", "");

  for (uint8_t i = 0; i < sizeof(volSolucao) / sizeof(volSolucao[0]); i++) // passando pelo array de solucoes
  {
    // timer para evitar o bounce do pushbutton
    delay(1000);
    lcd.clear();
    while (digitalRead(botaoOK) == LOW)
    {
      // imprimindo a solucao atual que esta sendo alterada
      Serial.print(solucao[i]);
      Serial.print(" : ");
      Serial.println(volSolucao[i]);
      printOpcoesLCD(solucao[i], String(volSolucao[i]));

      // incrementa o valo ao botao ser pressionado
      if (digitalRead(botaoSetaDireita) == HIGH)
      {
        volSolucao[i] += 0.1;
        delay(delaySetas);
        lcd.clear();
      }
      // decrementa o valo ao botao ser pressionado
      if (digitalRead(botaoSetaEsquerda) == HIGH)
      {
        // garante que o valor não desça mais que zero e estoure o vetor
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
  printOpcoesLCD("Salvar ciclo", "na memoria?");
  delay(1000);
  confirmarSelecao(salvarSolucaoNaEEPROM, botaoOK);
}

/**
 * @brief alterar temperatura da agua aquecida
 *
 */
void alterarTemperatura()
{
  // vetor de valores da temperatura das solucoes e ciclos
  uint8_t vetorTemp[] = {tempPreEnxaguePersonalizado, tempAlcPersonalizado, tempAcidPersonalizado};

  // passando pelos arrays
  for (uint8_t i = 0; i < sizeof(vetorTemp) / sizeof(uint8_t); i++)
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

  // confirma se deseja salvar dados na memoria
  Serial.println("Salvar na memoria ?");
  lcd.clear();
  printOpcoesLCD("Salvar ciclo", "na memoria?");
  delay(1000);
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
  delay(1000);
  EEPROM.update(EEPROM_TEMP_PRE_EXAGUE, tempPreEnxaguePersonalizado);
  EEPROM.update(EEPROM_TEMP_ALC, tempAlcPersonalizado);
  EEPROM.update(EEPROM_TEMP_ACID, tempAcidPersonalizado);
  pegarTempSolucaoEEPROM();
}

/**
 * @brief define os valores das solucoes com valores salvos na EEPROM 10 (alcalina),11 (acida) e 12 (sanitizante)
 *
 */
void pegarTempSolucaoEEPROM()
{
  tempPreEnxaguePersonalizado = EEPROM.read(EEPROM_TEMP_PRE_EXAGUE);
  tempAlcPersonalizado = EEPROM.read(EEPROM_TEMP_ALC);
  tempAcidPersonalizado = EEPROM.read(EEPROM_TEMP_ACID);

  tempPreEnxague = tempPreEnxaguePersonalizado;
  tempAlc = tempAlcPersonalizado;
  tempAcid = tempAcidPersonalizado;
}

/**
 * @brief attachInterrupt responsavel por interromper os ciclos de limpeza
 *
 */
void interromperOperacao()
{
  Serial.println("cheguei na interrupcao");
  Serial.println("INTERRROMPER");
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
    if (digitalRead(boiaSolucao) == LOW)
    {
      esvaziarTanque(tempAgua());
    }
    lcd.clear();
    printOpcoesLCD("Ciclo", "cancelado");
    delay(1000);
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
  // rotinaPreEnxague();

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

/**
 * @brief painel de selecao de ciclo de limpeza criado pelo usuario
 *
 */
void cicloPersonalizado()
{
  delay(1000);
  Serial.println();

  uint8_t countPercorrerCicloPersonalizado = 0;

  while (digitalRead(botaoOK) == LOW)
  {
    Serial.println();
    Serial.print(percorrerCicloPersonalinado[countPercorrerCicloPersonalizado]);
    printOpcoesLCD("Personalizado", percorrerCicloPersonalinado[countPercorrerCicloPersonalizado]);
    Serial.println(countPercorrerCicloPersonalizado);
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
    while (digitalRead(botaoRemover) == LOW)
    {
      printarCicloPersonalizado();
      printOpcoesLCD("", "Ciclo salvo");
    }
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
  delay(1000);
  lcd.clear();

  while (digitalRead(botaoInterromperOperacao) == LOW)
  {
    Serial.println(possiveisRotinas[percorrerPossiveisRotinas]);
    printOpcoesLCD("", possiveisRotinas[percorrerPossiveisRotinas]);

    Serial.println(percorrerRotinas);

    // incrementando o vetor para visualizar as possiveis rotinas
    if (digitalRead(botaoSetaDireita) == HIGH && percorrerPossiveisRotinas < 4)
    {
      percorrerPossiveisRotinas++;
      delay(delaySetas);
      lcd.clear();
    }

    // decrementando o vetor para visualizar as possiveis rotinas
    if (digitalRead(botaoSetaEsquerda) == HIGH && percorrerPossiveisRotinas > 0)
    {
      percorrerPossiveisRotinas--;
      delay(delaySetas);
      lcd.clear();
    }

    // adicionando a rotina especifica selecionada a posicao do vetor
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

      // incrementando o vetor ao celecionar uma rotina
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

    // decrementando o vetor caso botaoRemover seja acionado
    if (digitalRead(botaoRemover) == HIGH)
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
      }
      delay(delaySetas);
      lcd.clear();
    }
    // imprime constantemente o vetor resultado das acoes do usuario
    printarCicloPersonalizado();
  }

  lcd.clear();
  Serial.println("Salvar ciclo na memoria?");
  printOpcoesLCD("Salvar ciclo", "na memoria?");
  delay(1500);
  confirmarSelecao(salvarCicloPersonalizado, botaoOK);
}

/**
 * @brief pegando ciclo personalizado da EEPROM
 *
 */
void pegarCicloDaEEPROM()
{
  for (uint8_t count = 0; count < tamVetorRotinas; count++)
  {
    vetorRotinas[count] = EEPROM.read(count);
  }
}

/**
 * @brief imprimindo ciclo personalizado na memoria
 *
 */
void printarCicloPersonalizado()
{
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

  printOpcoesLCD(printarOrdemCiclo, "");
  Serial.println(printarOrdemCiclo);
}

/**
 * @brief usa ciclo de limpeza ja salvo na memoria
 *
 */
void usarCicloPersonalizado()
{
  Serial.println("usando ciclo salvo na memoria");
  printOpcoesLCD("Usando ciclo", "salvo");
  for (uint8_t i = 0; i < sizeof(vetorRotinas) / sizeof(vetorRotinas[0]); i++)
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

/**
 * @brief enche o tanque de agua
 *
 * @param resistencia 1 - liga resistencia | 0 - desliga resistencia
 * @param tanque valvula a ser alterada de posicao
 */
void encherTanque(uint8_t resistencia, uint8_t tanque)
{
  if (!interromper)
  {
    Serial.println("Enchendo tanque...");
    lcd.clear();
    printOpcoesLCD("Enchendo tanque", "...");
    while (digitalRead(boiaSolucao) == HIGH) // contato boia aberto
    {
      digitalWrite(relayResistencia, HIGH); // resistencia desligada
      digitalWrite(tanque, HIGH);           // despejando agua no tanque
    }
    digitalWrite(tanque, LOW); // fechando valvula
    Serial.println("Tanque Cheio");

    lcd.clear();
    printOpcoesLCD("Tanque cheio", "");
    if (resistencia)
    {
      Serial.println("ligando resistencia");
      aquecerResistencia(LOW); // ligar resistencia
    }
    else
    {
      Serial.println("desligando resistencia");
      aquecerResistencia(HIGH); // desligar resistencia
    }
  }
}

/**
 * @brief despejando agua no sistema
 *
 * @param tempSolucao temperatura dos ciclos
 * @param tanque pino da solenoide responsavel por encher o tanque
 */
void esvaziarTanque(float tempSolucao)
{
  if (interromper == false)
  {
    uint16_t ultimos_segundos;
    uint16_t delay_print = 2000;
    while (tempAgua() < tempSolucao && digitalRead(boiaSolucao) == LOW)
    {
      if ((millis() - ultimos_segundos) > delay_print)
      {
        printOpcoesLCD("Temperatura", String(tempAgua()));
        lcd.clear();
        delay_print = millis();
      }
    }
    aquecerResistencia(HIGH); // desligando resistencia

    Serial.println("ativando succao da ordenha");
    digitalWrite(ControleOrdenha, LOW); // ATIVAR SUCCAO DO SISTEMA

    printOpcoesLCD("Despejando agua", "...");

    Serial.println("Despejando agua...");
    delay(DELAY_ESVAZIAR_TANQUE);
    Serial.println("Agua despejada");

    lcd.clear();
    printOpcoesLCD("Agua", "despejada");

    Serial.println("desativando succao da ordenha");
    digitalWrite(ControleOrdenha, HIGH); // DESATIVAR SUCCAO DO SISTEMA
  }
}

/**
 * @brief controle da injecao de solucao na agua
 *
 * @param solucao_ml a ser adicionada
 * @param relay a ser ativado => 1 - Base || 2 - Acido || 3 - Sanitizante
 */
void adicionarSolucao(float solucao_ml, uint8_t relay)
{
  lcd.clear();
  printOpcoesLCD("Adicionando", "");
  if (interromper == false)
  {
    Serial.print("Adicionando solucao ");
    if (relay == 1)
    {
      printOpcoesLCD("", "base");
      Serial.println("base");
      estadoBombas(LOW, HIGH, HIGH);
    }

    else if (relay == 2)
    {
      printOpcoesLCD("", "acido");
      Serial.println("acida");
      estadoBombas(HIGH, LOW, HIGH);
    }

    else if (relay == 3)
    {
      printOpcoesLCD("", "sanitizante");
      Serial.println("sanitizante");
      estadoBombas(HIGH, HIGH, LOW);
    }
    Serial.println(calcSolucao(solucao_ml));
    delay(calcSolucao(solucao_ml)); // calculo do tempo de despejo da solucao
    estadoBombas(HIGH, HIGH, HIGH);
    Serial.println("Solucao Adicionada!");
  }
}

/**
 * @brief ligar resistencia
 *
 * @param status LOW - LIGADO | HIGH - DESLIGADO
 */
void aquecerResistencia(uint8_t status)
{
  if (interromper == false)
  {
    lcd.clear();
    if (status == LOW && !boiaSolucao)
    {
      printOpcoesLCD("Ligando", "resistencia");
      Serial.println("Ligando resistencia");
      digitalWrite(relayResistencia, LOW);
    }
    else
    {
      printOpcoesLCD("Desligando", "resistencia");
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
  lcd.clear();
  printOpcoesLCD("ROTINA", "PRE-ENXAGUE");

  digitalWrite(vs_vasao, HIGH); // abrindo a valvula de vasao para a saida
  encherTanque(1, vs_ts);       // enchendo tanque de solucao

  esvaziarTanque(tempPreEnxague); // succionando agua do tanque de solucao

  while (digitalRead(botaoOK))
  {
    Serial.println("aguardando OK");
    if (digitalRead(boiaMistura))
    {
      digitalWrite(vs_tm, LOW);
    }
    else
    {
      digitalWrite(vs_tm, HIGH);
    }

    Serial.println("ativando succao da ordenha");
    digitalWrite(ControleOrdenha, LOW);
  }

  esvaziarTanque(tempAgua()); // succionando agua do tanque de mistura
}

/**
 * @brief lavagem intermitente entre os ciclos acido e base
 *
 */
void rotinaEnxague()
{
  Serial.println();
  Serial.println("ROTINA ENXAGUE");
  lcd.clear();
  printOpcoesLCD("ROTINA", "ENXAGUE");

  digitalWrite(vs_ciclo, LOW);
  digitalWrite(vs_vasao, HIGH);

  encherTanque(0, vs_tm);     // adicionar agua
  esvaziarTanque(tempAgua()); // liberar apos atingir temperatura
}

/**
 * @brief adicao de solucoes ao tanque para realizacao da rotina completa de limpeza
 *
 * @param solucao 1 - Alcalina | 2 - Acida | 3 - Sanitizante
 * @param volSolucao volume em ml a ser adicionado
 * @param tempSolucao temperatura da rotina
 */
void rotinaSolucao(uint8_t solucao, float volSolucao, uint8_t tempSolucao)
{

  lcd.clear();
  if (solucao == 1)
  {
    printOpcoesLCD("ROTINA", "BASE");
    Serial.println("ROTINA BASE");
  }
  else if (solucao == 2)
  {
    printOpcoesLCD("ROTINA", "ACIDA");
    Serial.println("ROTINA ACIDA");
  }
  else if (solucao == 3)
  {
    printOpcoesLCD("ROTINA", "SANITIZANTE");
    Serial.println("ROTINA SANITIZANTE");
  }
  delay(1000);
  encherTanque(1, vs_ts);                // adicionar agua
  digitalWrite(vs_vasao, LOW);           // apontando para o tanque de mistura
  adicionarSolucao(volSolucao, solucao); // adicionar solucao
  esvaziarTanque(tempSolucao);           // liberar apos atingir temperatura
  digitalWrite(vs_ciclo, LOW);           // puxando do tanque de mistura
  delay(6000);                           // tempo para as valvulas se posicionarem
  digitalWrite(ControleOrdenha, LOW);    // ativar succao
  Serial.println("ativando succao");
  Serial.println("circulando solucao");
  delay(circularSolucao);     // tempo de circulacao da solucao na ordenha
  esvaziarTanque(tempAgua()); // despejando a agua destante
}

/**
 * @brief rotina da solucao base adicionada na agua
 *
 */
void rotinaBase()
{
  Serial.println();
  Serial.println("ROTINA BASE");
  lcd.clear();
  printOpcoesLCD("ROTINA", "BASE");
  encherTanque(1, vs_ts);      // adicionar agua
  adicionarSolucao(volAlc, 1); // adicionar solucao
  digitalWrite(vs_vasao, LOW); // apontando para o tanque de mistura
  digitalWrite(vs_tm, HIGH);
  esvaziarTanque(tempAlc); // liberar apos atingir temperatura

  digitalWrite(vs_tm, LOW);
  delay(6000);
  // ativar succao
  delay(circularSolucao);
  // desativar succao
}

/*
 * @brief rotina da solucao acida adicionada na agua
 */
void rotinaAcida()
{
  Serial.println();
  Serial.println("ROTINA ACIDO");
  lcd.clear();
  printOpcoesLCD("ROTINA", "ACIDA");
  // encherTanque(1, vs_ts); // adicionar agua

  adicionarSolucao(volAcid, 2); // adicionar solucao
  // esvaziarTanque(tempAcid, vs_vasao); // liberar apos atingir temperatura
}

/**
 * @brief rotina da solucao sanitizante adicionada na agua
 *
 */
void rotinaSanitizante()
{
  Serial.println();
  Serial.println("ROTINA SANITIZANTE");
  lcd.clear();
  printOpcoesLCD("ROTINA", "SANITIZANTE");
  encherTanque(0, vs_tm); // adicionar agua
  digitalWrite(vs_ciclo, HIGH);
  adicionarSolucao(volSanit, 3); // adicionar solucao

  esvaziarTanque(tempAgua()); // liberar apos atingir temperatura
}

/**
 * @brief quantidade de solucao a ser despejada
 * levando em consideração o volume do tanque
 *
 * @param solucao
 * @return tempo(ms)
 */
float calcSolucao(float solucao)
{
  // A dosagem total de produto referente a solução por litro e o volume do tanque.
  float dosagem = solucao * vol_tanque;
  // O tempo de dosagem corresponde a quantidade total de dosagem a ser utilizada multiplicada pelo fluxo da bomba.
  return (dosagem / fluxo_bomba) * 100;
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
  printOpcoesLCD("", "Erro de sensor");
  Serial.println("Erro ao detectar sensor");
  return 666.0;
}

/**
 * @brief controle das tres bombas peristalticas HIGH - Desativado || LOW - Ativado
 *
 * @param volAcid bomba acida
 * @param volAlc bomba alcalina
 * @param volSanit bomba sanitizante
 */
void estadoBombas(uint8_t volAlc, uint8_t volAcid, uint8_t volSanit)
{
  digitalWrite(relayAcid, volAlc);
  digitalWrite(relayAlc, volAcid);
  digitalWrite(relaySanit, volSanit);
}
