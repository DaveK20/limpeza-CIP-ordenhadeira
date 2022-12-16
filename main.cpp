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

#define tempSensor 5 // DS18B20

OneWire oneWire(tempSensor);
DallasTemperature sensors(&oneWire); /*encaminha referências OneWire para o sensor*/

void estadoBombas(uint8_t, uint8_t, uint8_t);
float sensorTemperatura();
void controleTemperatura();
void ControleBomba();
void valvula();
float calcSolucao(float);
void cicloCIP();

// CONTROLE BOMBAS PERISTALTICAS
int relayAcid = 2;
int relayAlc = 3;
int relaySanit = 4;

// VOLUME DE SOLUCAO A SER INSERIDO
float bombaAcid = 2.5;
float bombaAlc = 0.75;
float bombaSanit = 0; // ainda nao descoberto

// TEMPERATURAS IDEAIS DAS SOLUCOES
float tempBase = 75;
float tempAcido = 43;
float tempSanitizante;

/*
  23/11/2022 - Calibração das bombas peristálticas com copo graduado impreciso
  100ml ~ 65 segundos
  aproximadamente 1,538 ml/s
  */
float um_ml = 1.66; // volume de despejado por 1s

int ml_inserido = 200; // volume inserido como teste

bool teste = false;                               // finalizar o funcionamento da bomba
float bomba_delay = (ml_inserido / um_ml) * 1000; // calculo de despejo da bomba

void setup()
{
  Serial.begin(9600);
  pinMode(relayAcid, OUTPUT);
  pinMode(relayAlc, OUTPUT);
  pinMode(relaySanit, OUTPUT);
  pinMode(7, OUTPUT);

  Serial.println("Inicializando sistema...");
  sensors.begin();
}

void loop()
{
  if (teste == false)
  {
    estadoBombas(LOW, LOW, LOW);
    Serial.println(calcSolucao(bombaAcid));
    delay(bomba_delay);
  }
  estadoBombas(HIGH, HIGH, HIGH);
  teste = true;

  /*
  Serial.print("TempC: ");
  Serial.println(sensorTemperatura());
  delay(3000);
  */
}

void adicionarAcido()
{
  estadoBombas(LOW, HIGH, HIGH);
  delay(calcSolucao(bombaAcid));
  estadoBombas(HIGH, HIGH, HIGH);
}

void adicionarBase()
{
  estadoBombas(HIGH, LOW, HIGH);
  delay(calcSolucao(bombaAlc));
  estadoBombas(HIGH, HIGH, HIGH);
}

void adicionarSanitizante()
{
  estadoBombas(HIGH, HIGH, LOW);
  delay(calcSolucao(bombaSanit));
  estadoBombas(HIGH, HIGH, HIGH);
}

/**
 * @brief responsavel por encher o tanque
 *
 */
void encherTanque()
{
  Serial.println("Enchendo tanque...");
  /*while(boia sem contato){
    encher tanque
  }
  estadoTanque = true*/
}

void aquecerResistencia(float tempAgua)
{
  Serial.println("Ligando resistencia");
  /*while(temperatura < tempAgua && estadoTanque == true){
    aquecer agua
  }*/
}

void misturar()
{
  Serial.println("Misturando Solucao");
  /* misturando solucao
  digitalWrite(relayMistura);
  delay(tempo especifico de mistura)*/
}

void liberarAgua()
{
  /*if(temperatura == 75){
    digitalWrite(JogaNoSistema, LOW)
    delay(tempo necessario para despejar 50L)
  }
  digitalWrite(JogaNoSistema, LOW)*/
}

void cicloCIP()
{
  ////////////////// ciclo base //////////////////

  encherTanque();               // adicionar agua
  adicionarBase();              // adicionar solucao
  aquecerResistencia(tempBase); // aquecer
  liberarAgua();                // liberar apos atingir temperatura

  ////////////////// ciclo acido //////////////////

  encherTanque();                // adicionar agua
  adicionarAcido();              // adicionar solucao
  aquecerResistencia(tempAcido); // aquecer
  liberarAgua();                 // liberar apos atingir temperatura

  ////////////////// ciclo sanitizante //////////////////

  encherTanque();                      // adicionar agua
  adicionarSanitizante();              // adicionar solucao
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
 * @brief Controle das tres bombas peristalticas HIGH - Desativado LOW - Ativado
 *
 * @param bombaAcid
 * @param bombaAlc
 * @param bombaSanit
 */
void estadoBombas(uint8_t bombaAcid, uint8_t bombaAlc, uint8_t bombaSanit)
{
  digitalWrite(relayAcid, bombaAcid);
  digitalWrite(relayAlc, bombaAlc);
  digitalWrite(relaySanit, bombaSanit);
}

/**
 * @brief Printando informacoes na tela
 *
 */
void printInformacoes()
{
  Serial.println("");
  Serial.println("");
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

  Serial.println("Temp Agua:");
  Serial.println("Temp Tanque:");
}
