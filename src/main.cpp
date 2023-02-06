/**
 * @file main.cpp
 * @author DaveK2 (davefr@outlook.com.br)
 * @brief CIP ordenhadeira Campus Bom Jesus do Itabapoana
 * @version 0.8.1
 * @date 2023-02-06
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
*/

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>

void estadoBombas(uint8_t, uint8_t, uint8_t);
void aquecerResistencia(uint8_t);
void misturar(uint8_t);
void esvaziarTanque(float);
void adicionarSolucao(float, int);
void converterProgramaParaEEPROM(float);
void confirmarSelecao(void (*funcao)(), int botao);
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

float converterEEPROMParaPrograma(int posicao);
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
#define botaoTemperatura 9         // botao alterar temperaturas das solucoes
#define relayResistencia 10        // aquecer agua
#define botaoCicloCip 12           // botao inicializar ciclo cip
#define botaoCicloPersonalizado 13 // botao selecao, criacao de ciclo personalizado
#define botaoSolucao 14            // botao alterar volume das solucoes
#define botaoSetaDireita 15        // botao de interacao com o sistema
#define botaoSetaEsquerda 16       // botao de interacao com o sistema
#define botaoOK 17                 // botao para confirmacao das selecoes

#define interruptPushButton 2 // botao de interrupcao de ciclo

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

// POSICOES NA EEPROM, ONDE AS SOLUCOES SERAO SALVAS
#define EEPROM_ALC 10
#define EEPROM_ACID 11
#define EEPROM_SANIT 12

// POSICOES NA EEPROM, ONDE AS TEMPERATURAS SERAO SALVAS
#define EEPROM_TEMP_PRE_EXAGUE 13
#define EEPROM_TEMP_ALC 14
#define EEPROM_TEMP_ACID 15
#define EEPROM_TEMP_SANIT 16

// TEMPERATURAS IDEAIS DA AGUA
// De acordo com o artigo "Limpeza e Desinfecção de Equipamentos de Ordenha e Tanques" de MARCOS VEIGA SANTOS
int tempPreEnxague = 55; // temperatura na primeira lavagem
int tempAlc = 75;        // solucao base
int tempAcid = 43;       // solucao acida
int tempSanit = 0;       // solucao sanitizante

int tempAlcPersonalizado = 0;
int tempAcidPersonalizado = 0;
int tempSanitPersonalizado = 0;
int tempPreEnxaguePersonalizado = 0;

/*
  23/11/2022 - Calibração das bombas peristálticas com copo graduado impreciso
  100ml ~ 65 segundos
  aproximadamente 1,538 ml/s
  */
float um_ml = 1.66;                                                   // volume de despejado por 1s
float ml_inserido = 200;                                              // volume inserido como teste
float bomba_delay = (ml_inserido / um_ml) * 1000;                     // calculo de despejo da bomba
int timer = 100;                                                      // timer de espera para o usuario apertar o botao
int vetorRotinas[10] = {255, 255, 255, 255, 255, 255, 255, 255, 255}; // vetor para salvar um ciclo de limpeza personalizado
int delaySetas = 300;                                                 // delay das setas de selecao

#define DELAY_ESVAZIAR_TANQUE 18000 // tempo estimado para que o tanque fique vazio
#define COLETA_DELAY 3000           // coleta e amostragem dos dados de temperatura e status das bombas
bool interromper = false;           // interromper ciclo

unsigned long tempo_ultima_coleta;

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
  pinMode(botaoCicloCip, INPUT);
  pinMode(botaoCicloPersonalizado, INPUT);
  pinMode(botaoSetaDireita, INPUT);
  pinMode(botaoSetaEsquerda, INPUT);
  pinMode(botaoSolucao, INPUT);
  pinMode(botaoTemperatura, INPUT);
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


   if (digitalRead(botaoSetaEsquerda) == HIGH)
   {
     Serial.println("alto");
   }
   else
   {
     Serial.println("baixo");
   }*/

  printInformacoes();

  selecionarOpcao();
}

/**
 * @brief botoes de selecao de ciclos e do volume das solucoes
 *
 */
void selecionarOpcao()
{
  if (digitalRead(botaoCicloCip) == HIGH)
  {
    Serial.println("Ciclo CIP");
    confirmarSelecao(cicloCIP, botaoCicloCip);
  }

  if (digitalRead(botaoCicloPersonalizado) == HIGH)
  {
    Serial.println("Ciclo Personalizado");
    confirmarSelecao(cicloPersonalizado, botaoCicloPersonalizado);
  }

  if (digitalRead(botaoSolucao) == HIGH)
  {
    Serial.println("Alterar volume Solucao");
    confirmarSelecao(alterarVolumeSolucao, botaoSolucao);
  }

  if (digitalRead(botaoTemperatura) == HIGH)
  {
    Serial.println("Alterar temperatura das etapas");
    confirmarSelecao(alterarTemperatura, botaoTemperatura);
  }
}

/**
 * @brief confirma ciclo selecionado
 *
 * @param funcao a ser selecionada
 * @param botao a ser pressionado para confirmacao
 */
void confirmarSelecao(void (*funcao)(), int botao)
{
  delay(1000);
  for (float i = timer / 2; i >= 0; i--)
  {
    Serial.print("Pressione novamente para continuar ");
    Serial.println(i / 10);
    if (digitalRead(botao) == HIGH)
    {
      funcao();
      break;
    }
    delay(100);
  }
}

/**
 * @brief converte e salva o valor da solucao na EEPROM
 *
 * @param solucao a ser convertida
 * @param posicao a ser salva na memoria
 */
void converterProgramaParaEEPROM(float solucao, int posicao)
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
float converterEEPROMParaPrograma(int posicao)
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

  // passando pelos arrays
  for (int i = 0; i < 3; i++)
  {
    // timer para evitar o bounce do pushbutton
    delay(1500);
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
      }
      // decrementa o valo ao votao ser pressionado
      if (digitalRead(botaoSetaEsquerda) == HIGH)
      {
        // garante que o valor não desça mais que zero
        if (volSolucao[i] > 0)
        {
          volSolucao[i] -= 0.1;
          delay(delaySetas);
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
  confirmarSelecao(salvarSolucaoNaEEPROM, botaoOK);
}

/**
 * @brief alterar temperatura da agua aquecida
 *
 */
void alterarTemperatura()
{
  // inicializando arrays para salvar os nomes e volumes das solucoes a serem alteradas
  String stringTemp[] = {"temp Pre-enxague", "Temp Alcalino", "Temp Acido", "Temp Sanitizante"};
  int vetorTemp[] = {tempPreEnxaguePersonalizado, tempAlcPersonalizado, tempAcidPersonalizado, tempSanitPersonalizado};

  // passando pelos arrays
  for (int i = 0; i < 4; i++)
  {
    // timer para evitar o bounce do pushbutton
    delay(500);
    while (digitalRead(botaoOK) == LOW)
    {
      // imprimindo a solucao atual que esta sendo alterada
      Serial.print(stringTemp[i]);
      Serial.print(" : ");
      Serial.println(vetorTemp[i]);
      // incrementa o valo ao votao ser pressionado
      if (digitalRead(botaoSetaDireita) == HIGH)
      {
        vetorTemp[i] += 1;
        delay(delaySetas);
      }
      // decrementa o valo ao votao ser pressionado
      if (digitalRead(botaoSetaEsquerda) == HIGH)
      {
        // garante que o valor não desça mais que zero
        if (vetorTemp[i] > 0)
        {
          vetorTemp[i] -= 1;
          delay(delaySetas);
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
  EEPROM.update(EEPROM_TEMP_PRE_EXAGUE, tempPreEnxaguePersonalizado);
  EEPROM.update(EEPROM_TEMP_ALC, tempAlcPersonalizado);
  EEPROM.update(EEPROM_TEMP_ACID, tempAcidPersonalizado);
  EEPROM.update(EEPROM_TEMP_SANIT, tempSanitPersonalizado);

  Serial.print("Temp Pré-Enxague: ");
  Serial.println(EEPROM.read(EEPROM_TEMP_PRE_EXAGUE));
  Serial.print("Temp Alc:");
  Serial.println(EEPROM.read(EEPROM_TEMP_ALC));
  Serial.print("Temp Acid:");
  Serial.println(EEPROM.read(EEPROM_TEMP_ACID));
  Serial.print("Temp Sanit:");
  Serial.println(EEPROM.read(EEPROM_TEMP_SANIT));
  pegarTempSolucaoEEPROM();
}

/**
 * @brief define os valores das solucoes com valores salvos na EEPROM 11 (alcalina),12 (acida) e 13 (sanitizante)
 *
 */
void pegarTempSolucaoEEPROM()
{
  tempPreEnxague = EEPROM.read(EEPROM_TEMP_PRE_EXAGUE);
  tempAlc = EEPROM.read(EEPROM_ALC);
  tempAcid = EEPROM.read(EEPROM_ACID);
  tempSanit = EEPROM.read(EEPROM_SANIT);

  tempPreEnxaguePersonalizado = tempPreEnxague;
  tempAlcPersonalizado = tempAlc;
  tempAcidPersonalizado = tempAcid;
  tempSanitPersonalizado = tempSanit;
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
  // etapa pre-enxague
  rotinaPreEnxague();

  // etapa base
  rotinaBase();
  rotinaEnxague();

  // etapa acido
  rotinaAcida();
  rotinaEnxague();

  // etapa sanitizante
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
  String solucao[] = {"usar ciclo personalizado", "criar novo ciclo personalizado"};

  int countSolucao = 0;

  while (digitalRead(botaoOK) == LOW)
  {
    Serial.println();
    Serial.print(solucao[countSolucao]);

    // navegando para a direita sobre as opcoes
    if (digitalRead(botaoSetaDireita) == HIGH && countSolucao < 1)
    {
      countSolucao++;
      delay(delaySetas);
    }

    // navegando para a esquerda sobre as opcoes
    if (digitalRead(botaoSetaEsquerda) == HIGH && countSolucao > 0)
    {
      countSolucao--;
      delay(delaySetas);
    }
  }

  if (countSolucao == 0)
  {
    //  usar ciclo criado
    confirmarSelecao(usarCicloPersonalizado, botaoOK);
  }
  else
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
  String possiveisRotinas[5] = {"pre-enxague", "lavagem-intermitente", "alcalina", "acido", "sanitizante"};
  int percorrerPossiveisRotinas = 0; // percorrendo vetor de string representando as possiveis rotinas
  int percorrerRotinas = 0;          // percorrendo o vetor de rotinas salvas

  delay(500); // delay para esperar o valor do botao ser zerado

  while (digitalRead(botaoCicloPersonalizado) == LOW)
  {
    // indicando quantas posicoes foram preenchidas do vetor
    for (int c = 0; c < percorrerRotinas; c++)
    {
      Serial.print("*");
    }
    Serial.println();
    // indicando quais valorem foram preenchidos
    for (int c = 0; c < percorrerRotinas; c++)
    {
      if (vetorRotinas[percorrerPossiveisRotinas] != 255)
      {
        Serial.print(vetorRotinas[percorrerPossiveisRotinas]);
      }
    }
    Serial.println();

    // conta quantas posicoes do vetor foram preenchidas
    Serial.print("count: ");
    Serial.println(percorrerRotinas);

    // mostra no display as etapas possiveis
    Serial.print("Etapa: ");
    Serial.println(percorrerPossiveisRotinas);

    // navegando para a direita sobre as opcoes
    if (digitalRead(botaoSetaDireita) == HIGH && percorrerPossiveisRotinas < 4)
    {
      percorrerPossiveisRotinas++;
      delay(delaySetas);
    }

    // navegando para a esquerda sobre as opcoes
    if (digitalRead(botaoSetaEsquerda) == HIGH && percorrerPossiveisRotinas > 0)
    {
      percorrerPossiveisRotinas--;
      delay(delaySetas);
    }

    // adicionando ao vetor a ordem da rotina
    if (digitalRead(botaoOK) == HIGH)
    {
      if (possiveisRotinas[percorrerPossiveisRotinas] == "pre-enxague")
      {
        vetorRotinas[percorrerRotinas] = 1;
      }
      if (possiveisRotinas[percorrerPossiveisRotinas] == "lavagem-intermitente")
      {
        vetorRotinas[percorrerRotinas] = 2;
      }
      if (possiveisRotinas[percorrerPossiveisRotinas] == "alcalina")
      {
        vetorRotinas[percorrerRotinas] = 3;
      }
      if (possiveisRotinas[percorrerPossiveisRotinas] == "acido")
      {
        vetorRotinas[percorrerRotinas] = 4;
      }
      if (possiveisRotinas[percorrerPossiveisRotinas] == "sanitizante")
      {
        vetorRotinas[percorrerRotinas] = 5;
      }

      if (percorrerRotinas < sizeof(vetorRotinas) / sizeof(int)) // enquanto o vetor de rotinas nao se encontra cheio
      {
        Serial.println("adicionado ao vetor!");
        percorrerRotinas++;
      }
      else
      {
        Serial.println("vetor cheio");
      }
      delay(1000);
    }
    Serial.println();

    if (digitalRead(botaoCicloCip) == HIGH)
    {
      if (percorrerRotinas > 0 && percorrerRotinas < sizeof(vetorRotinas) / sizeof(int))
      {
        vetorRotinas[percorrerRotinas] = 255;
        percorrerRotinas--;
        Serial.println("removendo item");
        vetorRotinas[percorrerRotinas] = 255;
      }
      else
      {
        Serial.println("nao ha nada para remover");
      }
      delay(1000);
    }
  }

  // preenche o restante do vetor
  while (percorrerRotinas < sizeof(vetorRotinas) / sizeof(int))
  {
    vetorRotinas[percorrerRotinas] = 255;
    percorrerRotinas++;
  }

  // Imprimindo vetor resultante
  for (int count = 0; count < sizeof(vetorRotinas) / sizeof(int); count++)
  {
    Serial.print(" ");
    Serial.print(vetorRotinas[count]);
    Serial.print(" - ");
  }
  Serial.println();

  Serial.println("Salvar ciclo na memoria ?");
  confirmarSelecao(salvarCicloPersonalizado, botaoCicloPersonalizado);
}

/**
 * @brief usa ciclo de limpeza ja salvo na memoria
 *
 */
void usarCicloPersonalizado()
{
  Serial.println("usando ciclo salvo na memoria");
  for (int i = 0; i < sizeof(vetorRotinas) / sizeof(int); i++)
  {
    Serial.print(" ");
    Serial.print(EEPROM.read(i));
    Serial.print(" - ");

    if (EEPROM.read(i) == 1) // pre-enxague
    {
      rotinaPreEnxague();
    }
    if (EEPROM.read(i) == 2) // lavagem intermitente
    {
      rotinaEnxague();
    }
    if (EEPROM.read(i) == 3) // ciclo base
    {
      rotinaBase();
    }
    if (EEPROM.read(i) == 4) // ciclo acido
    {
      rotinaAcida();
    }
    if (EEPROM.read(i) == 5) // ciclo sanitizante
    {
      rotinaSanitizante();
    }
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
  delay(500);
  for (int i = 0; i < 10; i++)
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
}

/**
 * @brief enche o tanque de agua
 *
 * @param resistencia se igual a 1 aquece resistencia
 */
void encherTanque(int resistencia)
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
  if (tempAgua() == tempSolucao && digitalRead(pinBoia) == LOW)
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
void adicionarSolucao(float solucao, int relay)
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
  Serial.println("ETAPA PRE-ENXAGUE");
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
  Serial.println("ETAPA ENXAGUE");
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
  Serial.println("ETAPA BASE");
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
  Serial.println("ETAPA ACIDO");
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
  Serial.println("ETAPA SANITIZANTE");
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
    Serial.print("Etapa pre-enxague: ");
    Serial.println(tempPreEnxague);
    Serial.print("Etapa alcalina: ");
    Serial.println(tempAlc);
    Serial.print("Etapa acida: ");
    Serial.println(tempAcid);
    Serial.print("Etapa sanitizante: ");
    Serial.println(tempSanit);
    Serial.println();

    Serial.println("CICLO PERSONALIZADO SALVO");
    for (int i = 0; i < sizeof(vetorRotinas) / sizeof(int); i++)
    {
      Serial.print(" ");
      Serial.print(EEPROM.read(i));
      Serial.print(" - ");
    }

    Serial.println();
    tempo_ultima_coleta = millis();
  }
}
