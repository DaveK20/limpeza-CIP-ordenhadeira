/**
 * @file main.cpp
 * @author DaveK2 (davefr@outlook.com.br)
 * @brief CIP ordenhadeira Campus Bom Jesus do Itabapoana
 * @version 0.6.2
 * @date 2023-01-23
 *
 * @copyright Copyright (c) 2023
 *
 */

/*
OBJETIVOS RESTANTES
  [] - ter uma nocao do tempo necessario para esvaziar o tanque e mostrar no display
  [] - ciclo personalizado
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
void selecionarCiclo();
void alterarVolumeSolucao();
void pegarVolSolucaoEEPROM();
void salvarCicloPersonalizado();
void usarCicloPersonalizado();
void salvarSolucaoNaEEPROM();

float converterEEPROMParaPrograma(int posicao);
float calcSolucao(float);
float tempAgua();

// CONTROLE RELAYS
#define relayBoia 3                // futuramente substituir pelo ultrassonico se necessario
#define relayAlc 4                 // bomba peristaltica alcalina
#define relayAcid 5                // bomba peristaltica acida
#define relaySanit 6               // bomba peristaltica sanitizante
#define relayEncherTanque 7        // solenoide responsavel por encher tanque
#define relayEsvaziarTanque 8      // solenoide responsavel por esvaziar
#define relayMisturador 9          // manter solucao diluida
#define relayResistencia 10        // aquecer agua
#define botaoCicloCip 12           // botao de interacao com o sistema
#define botaoCicloPersonalizado 13 // botao de interacao com o sistema
#define botaoSetaDireita 14        // botao de interacao com o sistema
#define botaoSetaEsquerda 15       // botao de interacao com o sistema
#define botaoSolucao 16            // botao de interacao com o sistema
#define interruptPushButton 2      // botao de interrupcao de ciclo

// PINOS LEITURA
#define tempSensor 11 // DS18B20
OneWire oneWire(tempSensor);
DallasTemperature sensors(&oneWire); // encaminha referências OneWire para o sensor

// VOLUME DE SOLUCAO A SER INSERIDO NO SISTEMA
float bombaAcid = 2.5;
float bombaAlc = 0.75;
float bombaSanit = 0; // ainda nao descoberto

// VOLUME DE SOLUCAO INSERIDO PELO USUARIO E A SER SALVO NA EEPROM
float volAcidPersonalizado = 0;
float volAlcPersonalizado = 0;
float volSanitPersonalizado = 0;

// POSICOES NA EEPROM, ONDE AS SOLUCOES SERAO SALVAS
#define EEPROM_ALC 11
#define EEPROM_ACID 12
#define EEPROM_SANIT 13

// TEMPERATURAS IDEAIS DA AGUA
// De acordo com o artigo "Limpeza e Desinfecção de Equipamentos de Ordenha e Tanques" de MARCOS VEIGA SANTOS
float tempBase = 75;       // solucao base
float tempAcido = 43;      // solucao acida
float tempSanitizante = 0; // solucao sanitizante
float tempPreEnxague = 55; // temperatura da agua em lavagens intermitente

/*
  23/11/2022 - Calibração das bombas peristálticas com copo graduado impreciso
  100ml ~ 65 segundos
  aproximadamente 1,538 ml/s
  */
float um_ml = 1.66;                               // volume de despejado por 1s
float ml_inserido = 200;                          // volume inserido como teste
float bomba_delay = (ml_inserido / um_ml) * 1000; // calculo de despejo da bomba
int timer = 100;

#define DELAY_ESVAZIAR_TANQUE 18000 // tempo estimado para que o tanque fique vazio
#define COLETA_DELAY 3000           // coleta e amostragem dos dados de temperatura e status das bombas
bool interromper = false;           // interromper ciclo

unsigned long tempo_ultima_coleta;

int vetor[10];

void setup()
{
  Serial.begin(9600);
  pinMode(relayAcid, OUTPUT);
  pinMode(relayAlc, OUTPUT);
  pinMode(relaySanit, OUTPUT);
  pinMode(relayEncherTanque, OUTPUT);
  pinMode(relayEsvaziarTanque, OUTPUT);
  pinMode(relayMisturador, OUTPUT);
  pinMode(relayResistencia, OUTPUT);
  pinMode(relayBoia, INPUT_PULLUP);
  pinMode(botaoCicloCip, INPUT);
  pinMode(botaoCicloPersonalizado, INPUT);
  pinMode(botaoSetaDireita, INPUT);
  pinMode(botaoSetaEsquerda, INPUT);
  pinMode(botaoSolucao, INPUT);
  pinMode(interruptPushButton, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPushButton), interromperOperacao, CHANGE);

  estadoBombas(HIGH, HIGH, HIGH);
  digitalWrite(relayEncherTanque, HIGH);
  digitalWrite(relayEsvaziarTanque, HIGH);
  digitalWrite(relayMisturador, HIGH);
  digitalWrite(relayResistencia, HIGH);

  Serial.println("Inicializando sistema...");
  sensors.begin();

  EEPROM.begin();

  pegarVolSolucaoEEPROM();
}

void loop()
{
  /*
  if (teste == false)
  {
    estadoBombas(LOW, LOW, LOW);
    Serial.println(calcSolucao(bombaAcid));
    delay(bomba_delay);
  }
  estadoBombas(HIGH, HIGH, HIGH);
  teste = true;
  */

  printInformacoes();

  selecionarCiclo();
}

/**
 * @brief botoes de selecao de ciclo
 *
 */
void selecionarCiclo()
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
}

/**
 * @brief confirmacao do ciclo selecionado
 *
 * @param funcao a ser selecionada
 * @param botao botao a ser pressionado para confirmacao
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
  bombaAlc = converterEEPROMParaPrograma(EEPROM_ALC);
  bombaAcid = converterEEPROMParaPrograma(EEPROM_ACID);
  bombaSanit = converterEEPROMParaPrograma(EEPROM_SANIT);

  volAlcPersonalizado = converterEEPROMParaPrograma(EEPROM_ALC);
  volAcidPersonalizado = converterEEPROMParaPrograma(EEPROM_ACID);
  volSanitPersonalizado = converterEEPROMParaPrograma(EEPROM_SANIT);
}

/**
 * @brief salva dados na EEPROM
 *
 */
void salvarSolucaoNaEEPROM()
{
  Serial.println("-SALVANDO NA EEPROM-");
  Serial.print("Alc:");
  converterProgramaParaEEPROM(volAlcPersonalizado, 11);
  Serial.println();
  Serial.print("Acid:");
  converterProgramaParaEEPROM(volAcidPersonalizado, 12);
  Serial.println();
  Serial.print("Sanit:");
  converterProgramaParaEEPROM(volSanitPersonalizado, 13);
  Serial.println();

  Serial.println("-CONVERTIDOS DA EEPROM-");
  Serial.println(EEPROM.read(11));
  Serial.println(EEPROM.read(12));
  Serial.println(EEPROM.read(13));

  pegarVolSolucaoEEPROM();
}

/**
 * @brief altera volume das solucoes a serem despejadas na agua
 *
 */
void alterarVolumeSolucao()
{
  delay(1500);
  while (digitalRead(botaoSolucao) == LOW)
  {
    Serial.print("Alcalino:");
    Serial.println(volAlcPersonalizado);
    if (digitalRead(botaoSetaDireita) == HIGH)
    {
      volAlcPersonalizado += 0.1;
      delay(500);
    }
    if (digitalRead(botaoSetaEsquerda) == HIGH)
    {
      if (volAlcPersonalizado > 0)
      {
        volAlcPersonalizado -= 0.1;
        delay(500);
      }
    }
  }
  delay(1500);

  while (digitalRead(botaoSolucao) == LOW)
  {
    Serial.print("Acido:");
    Serial.println(volAcidPersonalizado);
    if (digitalRead(botaoSetaDireita) == HIGH)
    {
      volAcidPersonalizado += 0.1;
      delay(500);
    }
    if (digitalRead(botaoSetaEsquerda) == HIGH)
    {
      if (volAcidPersonalizado > 0)
      {
        volAcidPersonalizado -= 0.1;
        delay(500);
      }
    }
  }
  delay(1500);

  while (digitalRead(botaoSolucao) == LOW)
  {
    Serial.print("Sanitizante:");
    Serial.println(volSanitPersonalizado);
    if (digitalRead(botaoSetaDireita) == HIGH)
    {
      volSanitPersonalizado += 0.1;
      delay(500);
    }
    if (digitalRead(botaoSetaEsquerda) == HIGH)
    {
      if (volSanitPersonalizado > 0)
      {
        volSanitPersonalizado -= 0.1;
        delay(500);
      }
    }
  }
  Serial.println("Salvar na memoria ?");
  confirmarSelecao(salvarSolucaoNaEEPROM, botaoSolucao);
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

  if (interromper == true)
  {
    Serial.println("Rotina interrompida com sucesso!");
    interromper = false;
    if (digitalRead(relayBoia) == LOW)
    {
      esvaziarTanque(tempAgua());
    }
  }
}

/**
 * @brief painel de selecao de ciclo de limpeza criado pelo usuario
 *
 */
void cicloPersonalizado()
{
  delay(1000);
  Serial.println();
  String v[2] = {"usar ciclo personalizado", "criar novo ciclo personalizado"};

  int count = 0;

  while (digitalRead(botaoCicloPersonalizado) == LOW)
  {
    Serial.println();
    Serial.print(v[count]);

    // navegando para a direita sobre as opcoes
    if (digitalRead(botaoSetaDireita) == HIGH && count < 1)
    {
      count++;
      delay(500);
    }

    // navegando para a esquerda sobre as opcoes
    if (digitalRead(botaoSetaEsquerda) == HIGH && count > 0)
    {
      count--;
      delay(500);
    }
  }

  if (count == 0)
  {
    confirmarSelecao(usarCicloPersonalizado, botaoCicloPersonalizado);
  }
  else
  {
    confirmarSelecao(criarCicloPersonalizado, botaoCicloPersonalizado);
  }

  //  usar ciclo criado
  // criar ciclo personalizado
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

  // vetor salvando as possiveis selecoes das rotinas
  String v[5] = {"pre-enxague", "lavagem-intermitente", "alcalina", "acido", "sanitizante"};
  int count = 0; // percorrendo vetor de string representando as rotinas
  int i = 0;     // percorrendo o vetor de rotinas

  // atribuindo um valor inicial para as posicoes do vetor
  for (int i = 0; i < 10; i++)
  {
    vetor[i] = 255; // valor geralmente atribuido as posicoes da EEPROM
  }

  delay(500); // delay para esperar o valor do botao ser zerado

  while (digitalRead(botaoCicloPersonalizado) == LOW)
  {
    for (int i = 0; i < 10; i++)
    {
      if (vetor[i] != 255)
      {
        Serial.print("*");
      }
    }

    Serial.println();
    Serial.print("Etapa: ");
    Serial.println(v[count]);

    // navegando para a direita sobre as opcoes
    if (digitalRead(botaoSetaDireita) == HIGH && count < 4)
    {
      count++;
      delay(500);
    }

    // navegando para a esquerda sobre as opcoes
    if (digitalRead(botaoSetaEsquerda) == HIGH && count > 0)
    {
      count--;
      delay(500);
    }

    // adicionando ao vetor a ordem da rotina
    if (digitalRead(botaoSolucao) == HIGH)
    {
      if (v[count] == "pre-enxague")
      {
        vetor[i] = 1;
      }
      if (v[count] == "lavagem-intermitente")
      {
        vetor[i] = 2;
      }
      if (v[count] == "alcalina")
      {
        vetor[i] = 3;
      }
      if (v[count] == "acido")
      {
        vetor[i] = 4;
      }
      if (v[count] == "sanitizante")
      {
        vetor[i] = 5;
      }

      if (i < 10) // enquanto o vetor de rotinas nao se encontra cheio
      {
        Serial.println("adicionado ao vetor!");
        i++;
      }
      else
      {
        Serial.println("vetor cheio");
      }
      delay(1000);
    }
  }

  // preenche o restante do vetor
  while (i < 10)
  {
    vetor[i] = 255;
    i++;
  }

  // Imprimindo vetor resultante
  for (int i = 0; i < 10; i++)
  {
    Serial.print(" ");
    Serial.print(vetor[i]);
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
  for (int i = 0; i < 10; i++)
  {
    Serial.print(" ");
    Serial.print(EEPROM.read(i));
    Serial.print(" - ");

    if (EEPROM.read(i) == 1) // pre-enxague
    {
      vetor[i] = 1;
    }
    if (EEPROM.read(i) == 2) // lavagem intermitente
    {
      vetor[i] = 2;
    }
    if (EEPROM.read(i) == 3) // ciclo base
    {
      vetor[i] = 3;
    }
    if (EEPROM.read(i) == 4) // ciclo acido
    {
      vetor[i] = 4;
    }
    if (EEPROM.read(i) == 5) // ciclo sanit
    {
      vetor[i] = 5;
    }
  }
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
    if (vetor[i] == 1) // pre-enxague
    {
      EEPROM.update(vetor[i], 1);
    }
    if (vetor[i] == 2) // lavagem intermitente
    {
      EEPROM.update(vetor[i], 2);
    }
    if (vetor[i] == 3) // ciclo base
    {
      EEPROM.update(vetor[i], 3);
    }
    if (vetor[i] == 4) // ciclo acido
    {
      EEPROM.update(vetor[i], 4);
    }
    if (vetor[i] == 5) // ciclo sanit
    {
      EEPROM.update(vetor[i], 5);
    }
    else
    {
      EEPROM.update(vetor[i], 255);
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
    while (digitalRead(relayBoia) == HIGH)
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
  if (tempAgua() == tempSolucao && digitalRead(relayBoia) == LOW)
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
      digitalWrite(relayMisturador, LOW);
    }
    else
    {
      Serial.println("Desligando misturador");
      digitalWrite(relayMisturador, HIGH);
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
  encherTanque(1);               // adicionar agua
  adicionarSolucao(bombaAlc, 1); // adicionar solucao
  esvaziarTanque(tempBase);      // liberar apos atingir temperatura
}

/**
 * @brief rotina da solucao acida adicionada na agua
 *
 */
void rotinaAcida()
{
  Serial.println();
  Serial.println("ETAPA ACIDO");
  encherTanque(1);                // adicionar agua
  adicionarSolucao(bombaAcid, 2); // adicionar solucao
  esvaziarTanque(tempAcido);      // liberar apos atingir temperatura
}

/**
 * @brief rotina da solucao sanitizante adicionada na agua
 *
 */
void rotinaSanitizante()
{
  Serial.println();
  Serial.println("ETAPA SANITIZANTE");
  encherTanque(0);                 // adicionar agua
  adicionarSolucao(bombaSanit, 3); // adicionar solucao
  esvaziarTanque(tempAgua());      // liberar apos atingir temperatura
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
  while (sensors.isConversionComplete() == false)
  {
  }
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0); // temp do sensor indice 0
}

/**
 * @brief controle das tres bombas peristalticas HIGH - Desativado || LOW - Ativado
 *
 * @param bombaAcid
 * @param bombaAlc
 * @param bombaSanit
 */
void estadoBombas(uint8_t bombaAlc, uint8_t bombaAcid, uint8_t bombaSanit)
{
  digitalWrite(relayAcid, bombaAlc);
  digitalWrite(relayAlc, bombaAcid);
  digitalWrite(relaySanit, bombaSanit);
}

/**
 * @brief printando informacoes na tela
 *
 */
void printInformacoes()
{
  if ((millis() - tempo_ultima_coleta) > COLETA_DELAY)
  {
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

    if (digitalRead(relayBoia) == HIGH)
    {
      Serial.println("Tanque vazio/esvaziando");
    }
    else
    {
      Serial.println("Acionada tanque cheio");
    }

    Serial.print("TEMP AGUA: ");
    Serial.println(tempAgua());
    Serial.print("TEMP TANQUE: ");
    Serial.println("Sem sensor");
    Serial.println();
    tempo_ultima_coleta = millis();
  }
}
