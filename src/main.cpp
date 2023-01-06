/**
 * @file main.cpp
 * @author DaveK2 (davefr@outlook.com.br)
 * @brief CIP ordenhadeira Campus Bom Jesus do Itabapoana
 * @version 0.4
 * @date 2023-01-04
 *
 * @copyright Copyright (c) 2023
 *
 */

/*
OBJETIVOS CURTO PRAZO
  [] - Ter uma nocao do tempo necessario para esvaziar o tanque e mostrar no display 
  [] - Aperfeicoar ciclo CIP
  [] - iniciar trabalhos no ciclo personalizado
  
OBJETIVOS LONGO PRAZO
  [] - Testar resistencia
  [] - Testar contatora
MELHORIAS DO PRODUTO
  [] - Ciclo de enxague customizado
  [] - Display simplificado de acesso
*/

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

void estadoBombas(uint8_t, uint8_t, uint8_t);
void adicionarSolucao(float solucao, int relay);
void cicloCIP();
void misturar(uint8_t);
void aquecerResistencia(uint8_t);
void printInformacoes();
void encherTanque();
void esvaziarTanque(float);
void rotinaPreEnxague();
void statusBoia_ISR();
float tempAgua();
float calcSolucao(float);
void funcaoBotao1();
void interromperOperacao();

// CONTROLE RELAYS
#define relayBoia 3 // futuramente substituir pelo ultrassonico se necessario
#define relayAlc 4
#define relayAcid 5
#define relaySanit 6
#define relayEncherTanque 7
#define relayEsvaziarTanque 8
#define relayMisturador 9
#define relayResistencia 10
#define pushButton1 12
#define interruptPushButton 2

// PINOS LEITURA
#define tempSensor 11 // DS18B20
OneWire oneWire(tempSensor);
DallasTemperature sensors(&oneWire); // encaminha referências OneWire para o sensor

// VOLUME DE SOLUCAO A SER INSERIDO
float bombaAcid = 2.5;
float bombaAlc = 0.75;
float bombaSanit = 0; // ainda nao descoberto

// TEMPERATURAS IDEAIS DA AGUA
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

#define DELAY_ESVAZIAR_TANQUE 18000
#define COLETA_DELAY 3000
bool interromper = false;

unsigned long tempo_ultima_coleta;

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
  pinMode(pushButton1, INPUT);
  pinMode(interruptPushButton, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPushButton), interromperOperacao, CHANGE);

  estadoBombas(HIGH, HIGH, HIGH);
  digitalWrite(relayEncherTanque, HIGH);
  digitalWrite(relayEsvaziarTanque, HIGH);
  digitalWrite(relayMisturador, HIGH);
  digitalWrite(relayResistencia, HIGH);

  Serial.println("Inicializando sistema...");
  sensors.begin();
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

  if ((millis() - tempo_ultima_coleta) > COLETA_DELAY)
  {
    printInformacoes();
    tempo_ultima_coleta = millis();
  }

  if (digitalRead(pushButton1) == HIGH)
  {
    cicloCIP();
  }
}

void interromperOperacao()
{
  Serial.println("cheguei na interrupcao");
  interromper = true;
}

/**
 * @brief Enche o tanque de agua
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
 * @brief Despejando agua com solucao no sistema de limpeza
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
    aquecerResistencia(HIGH); // aquecer
    misturar(HIGH);
    digitalWrite(relayEsvaziarTanque, LOW);
    delay(1000); // tempo necessario para se liberar totalmente 50L de agua no sistema
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
 * @brief Ativa/desativa misturador da agua com solucao
 *
 * @param status HIGH == desligado || LOW == ligado
 */
void misturar(uint8_t status)
{
  if (interromper == false)
  {
    // realizar essa tarefa em paralelo com outras, aquecer a agua por exemplo
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
 * @brief Controle da injecao de solucao na agua
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
    // reduzir o numero de delays existentes para ganhar tempo no aquecimento
    // da agua, maior desafio a ser superado ate o momento, utilizar millis() 28/12/2022
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
 * @brief Pre lavagem do tanque apos ordenha
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
  encherTanque(0);          // adicionar agua
  esvaziarTanque(tempBase); // liberar apos atingir temperatura
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
 * @brief Controle do ciclo automatico de limpeza da ordenhadeira
 *
 */
void cicloCIP()
{
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
    if(digitalRead(relayBoia) == LOW){
      esvaziarTanque(tempAgua());
    }
  }
}

/**
 * @brief Quantidade de solucao a ser despejada em 50L de agua
 *
 * @param solucao
 * @return tempo(ms)
 */
float calcSolucao(float solucao)
{
  return ((solucao * 50) / um_ml) * 1000;
}

/**
 * @brief Coletando temperatura do sensor DS18B20
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
 * @brief Controle das tres bombas peristalticas HIGH - Desativado || LOW - Ativado
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
 * @brief Printando informacoes na tela
 *
 */
void printInformacoes()
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
}