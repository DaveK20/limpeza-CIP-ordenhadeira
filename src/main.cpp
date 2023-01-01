/**
 * @file main.cpp
 * @author DaveK2 (davefr@outlook.com.br)
 * @brief CIP ordenhadeira Campus Bom Jesus do Itabapoana
 * @version 0.3
 * @date 2023-01-01
 *
 * @copyright Copyright (c) 2023
 *
 */

/*
OBJETIVOS CURTO PRAZO
  [x] - Calibrar bombas peristalticas 09/11/2022 - 23/11/2022
  [x] - Implementar sensor de temperatura 09/11/2022 - 11/11/2022
  [x] - Controlar valvula solenoide com sensor de nivel 23/11/2022 - 15/12/2022
  [x] - Prender projeto em uma placa de madeira 09/11/2022 - 09/11/2022
  [] - Ter uma nocao do tempo necessario para esvaziar o tanque e mostrar no display
  [] - Aperfeicoar ciclo CIP
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
void controleTemperatura();
void ControleBomba();
void valvula();
void cicloCIP();
void misturar();
void printInformacoes();
void encherTanque();
void esvaziarTanque(float);
void rotinaPreEnxague();
void statusTanque();
float tempAgua();
float calcSolucao(float);

// CONTROLE RELAYS
#define relayAcid 2
#define relayAlc 3
#define relaySanit 4
#define relayBoia 6 // futuramente substituir pelo ultrassonico se necessario
#define relayEncherTanque 7
#define relayEsvaziarTanque 8

// PINOS LEITURA
#define tempSensor 5 // DS18B20
OneWire oneWire(tempSensor);
DallasTemperature sensors(&oneWire); // encaminha referências OneWire para o sensor

#define tempo_esvaziar_tanque 180000

// VOLUME DE SOLUCAO A SER INSERIDO
float bombaAcid = 2.5;
float bombaAlc = 0.75;
float bombaSanit = 0; // ainda nao descoberto

// TEMPERATURAS IDEAIS DA AGUA
float tempBase = 75;         // solucao base
float tempAcido = 43;        // solucao acida
float tempSanitizante = 0;   // solucao sanitizante
float tempIntermitente = 55; // temperatura da agua em lavagens intermitente

/*
  23/11/2022 - Calibração das bombas peristálticas com copo graduado impreciso
  100ml ~ 65 segundos
  aproximadamente 1,538 ml/s
  */
float um_ml = 1.66; // volume de despejado por 1s

int ml_inserido = 200; // volume inserido como teste

bool teste = false;                               // finalizar o funcionamento da bomba
float bomba_delay = (ml_inserido / um_ml) * 1000; // calculo de despejo da bomba

#define COLETA_DELAY 3000
unsigned long tempo_ultima_coleta;

void setup()
{
  Serial.begin(9600);
  pinMode(relayAcid, OUTPUT);
  pinMode(relayAlc, OUTPUT);
  pinMode(relaySanit, OUTPUT);
  pinMode(relayBoia, INPUT_PULLUP);

  estadoBombas(HIGH, HIGH, HIGH);

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
}

/**
 * @brief Enche o tanque de agua
 *
 */
void encherTanque()
{
  Serial.println("Enchendo tanque...");
  // futuramente tambem substituir isso por millis()
  while (digitalRead(relayBoia) == HIGH)
  {
    digitalWrite(relayEncherTanque, LOW); // despejando agua no tanque
  }
  Serial.println("Tanque Cheio");
  delay(1000);
}

/**
 * @brief Despejando agua com solucao no sistema de limpeza
 *
 * @param tempSolucao a ser verificada
 */
void esvaziarTanque(float tempSolucao)
{
  Serial.println("Despejando agua");
  /*
  if (tempAgua() == tempSolucao)
  {
    digitalWrite(relayEncherTanque, LOW);
    delay(1000); // tempo necessario para se liberar totalmente 50L de agua no sistema
  }*/
  Serial.println("Agua despejada");
  delay(10000);
  digitalWrite(relayEsvaziarTanque, HIGH);
}

/**
 * @brief tempo restante para encher/esvaziar o tanque
 *
 */
void statusTanque()
{
}

/**
 * @brief Ativa misturador da agua com solucao
 *
 */
void misturar()
{
  // realizar essa tarefa em paralelo com outras, aquecer a agua por exemplo
  Serial.println("Misturando Solucao");
  /*
  if(digitalRead(relayBoia) == LOW){
    digitalWrite(relayMisturador, LOW);
  }
  delay(tempo especifico de mistura);
  */
}

/**
 * @brief Controle da injecao de solucao na agua
 *
 * @param solucao a ser adicionada
 * @param relay a ser ativado => 1 - Acido || 2 - Base || 3 - Sanitizante
 */
void adicionarSolucao(float solucao, int relay)
{
  Serial.print("Adicionando solucao ");
  if (relay == 1)
  {
    Serial.println("acida");
    estadoBombas(LOW, HIGH, HIGH);
  }

  else if (relay == 2)
  {
    Serial.println("base");
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

/**
 * @brief Ligar resistencia, e aquecer agua na temperatura inserida
 *
 * @param tempSolucao a ser despejada no tanque
 */
void aquecerResistencia(float tempSolucao)
{
  Serial.println("Ligando resistencia");
  /*
  while (tempAgua() != tempSolucao)
  {
    // relay de ativacao indefinido
  }*/
  delay(10000);
  Serial.println("Agua aquecida");
}

void rotinaPreEnxague()
{
  Serial.println("ETAPA PRE-ENXAGUE");
  encherTanque();                       // adicionar agua
  aquecerResistencia(tempIntermitente); // aquecer
  misturar();
  esvaziarTanque(tempBase); // liberar apos atingir temperatura
}

/**
 * @brief Controle do ciclo automatico de limpeza da ordenhadeira
 *
 */
void cicloCIP()
{
  ////////////////// etapa pre-enxague
  rotinaPreEnxague();

  ////////////////// etapa base

  Serial.println("CLICLO BASE");
  encherTanque();                // adicionar agua
  adicionarSolucao(bombaAlc, 1); // adicionar solucao
  aquecerResistencia(tempBase);  // aquecer
  misturar();
  esvaziarTanque(tempBase); // liberar apos atingir temperatura

  ////////////////// etapa pre-enxague
  rotinaPreEnxague();

  ////////////////// etapa acido

  Serial.println("CLICLO ACIDO");
  encherTanque();                 // adicionar agua
  adicionarSolucao(bombaAcid, 2); // adicionar solucao
  aquecerResistencia(tempAcido);  // aquecer
  misturar();
  esvaziarTanque(tempAcido); // liberar apos atingir temperatura

  ////////////////// etapa pre-enxague
  rotinaPreEnxague();

  ////////////////// etapa sanitizante

  Serial.println("CLICLO SANITIZANTE");
  encherTanque();                  // adicionar agua
  adicionarSolucao(bombaSanit, 3); // adicionar solucao
  misturar();
  esvaziarTanque(tempSanitizante); // liberar apos atingir temperatura
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