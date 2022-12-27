/**
 * @file main.cpp
 * @author DaveK2 (davefr@outlook.com.br)
 * @brief CIP ordenhadeira Campus Bom Jesus do Itabapoana
 * @version 0.2
 * @date 2022-11-24
 *
 * @copyright Copyright (c) 2022
 *
 */

/*
OBJETIVOS CURTO PRAZO
  [x] - Calibrar bombas peristalticas 09/11/2022 - 23/11/2022
  [x] - Implementar sensor de temperatura 09/11/2022 - 11/11/2022
  [x] - Controlar valvula solenoide com sensor de nivel 23/11/2022 - 15/12/2022
  [x] - Prender projeto em uma placa de madeira 09/11/2022 - 09/11/2022
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
void printInformacoes();
float sensorTemperatura();
float calcSolucao(float);

// CONTROLE RELAYS
#define relayAcid 2
#define relayAlc 3
#define relaySanit 4
#define relayBoia 6
#define relayDespejarAgua 7

// PINOS LEITURA
#define tempSensor 5 // DS18B20
OneWire oneWire(tempSensor);
DallasTemperature sensors(&oneWire); /*encaminha referências OneWire para o sensor*/

#define tempo_esvaziar_tanque 180000

// VOLUME DE SOLUCAO A SER INSERIDO
float bombaAcid = 2.5;
float bombaAlc = 0.75;
float bombaSanit = 0; // ainda nao descoberto

// TEMPERATURAS IDEAIS DAS SOLUCOES
float tempBase = 75;
float tempAcido = 43;
float tempSanitizante = 0;

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
 * @brief Controle da injecao de solucao na agua
 *
 * @param solucao a ser adicionada
 * @param relay a ser ativado 1 - Acido || 2 - Base
 */
void adicionarSolucao(float solucao, int relay)
{
  if (relay == 1)
  {
    estadoBombas(LOW, HIGH, HIGH);
  }

  if (relay == 2)
  {
    estadoBombas(HIGH, LOW, HIGH);
  }

  if (relay == 3)
  {
    estadoBombas(HIGH, HIGH, LOW);
  }
  delay(calcSolucao(solucao));
  estadoBombas(HIGH, HIGH, HIGH);
}

/**
 * @brief Enche o tanque de agua
 *
 */
void encherTanque()
{
  Serial.println("Enchendo tanque...");
  while (relayBoia == HIGH)
  {
  }
  Serial.println("Tanque Cheio");
}

/**
 * @brief Liga a resistencia, e aquece a agua na temperatura inserida
 *
 * @param tempAgua
 */
void aquecerResistencia(float tempAgua)
{
  Serial.println("Ligando resistencia");
  delay(10000);
  /*while(temperatura < tempAgua && estadoTanque == true){
    aquecer agua
  }*/
  Serial.println("Agua aquecida");
}

/**
 * @brief Ativa misturador da agua com solucao
 *
 */
void misturar()
{
  Serial.println("Misturando Solucao");
  /* misturando solucao
  digitalWrite(relayMistura);
  delay(tempo especifico de mistura)*/
}

/**
 * @brief Despeja mistura no sistema
 *
 */
void liberarAgua()
{
  /*if(temperatura == 75){
    digitalWrite(JogaNoSistema, LOW)
    delay(tempo necessario para despejar 50L)
  }
  digitalWrite(JogaNoSistema, LOW)*/
}

/**
 * @brief Controle do ciclo automatico de limpeza da ordenhadeira
 *
 */
void cicloCIP()
{
  ////////////////// ciclo base //////////////////

  encherTanque();                // adicionar agua
  adicionarSolucao(bombaAlc, 1); // adicionar solucao
  aquecerResistencia(tempBase);  // aquecer
  liberarAgua();                 // liberar apos atingir temperatura

  ////////////////// ciclo acido //////////////////

  encherTanque();                 // adicionar agua
  adicionarSolucao(bombaAcid, 2); // adicionar solucao
  aquecerResistencia(tempAcido);  // aquecer
  liberarAgua();                  // liberar apos atingir temperatura

  ////////////////// ciclo sanitizante //////////////////

  encherTanque();                      // adicionar agua
  adicionarSolucao(bombaSanit, 3);     // adicionar solucao
  aquecerResistencia(tempSanitizante); // aquecer
  liberarAgua();                       // liberar apos atingir temperatura
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
float sensorTemperatura()
{
  delay(100);
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
  Serial.println(sensorTemperatura());
  Serial.print("TEMP TANQUE: ");
  Serial.println("Sem sensor");
  Serial.println();
}