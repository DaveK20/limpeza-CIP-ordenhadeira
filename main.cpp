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
  [] - Controlar valvula solenoide com sensor de nivel
  [x] - Prender projeto em uma placa de madeira 09/11/2022 - 09/11/2022
OBJETIVOS LONGO PRAZO
  [] - Testar resistencia
  [] - Testar contatora
MELHORIAS DO PRODUTO
  [] - Ciclo de enxague customizado
  [] - Display simplificado de acesso


  ========= ciclo padrao
  void ciclo_CIP(){
    rotina de enxague()
    rotina alcalina()
    rotina enxagueTanque()
    rotina acida()
    rotina sanitizante()
  }

  void cliclo_Custom(){
    salvar o ciclo em um array e com um loop for comparar cada posicao
    se assimilar um valor pro tipo de lavagem ex:
    1 - rotina de enxague()
    2 - rotina alcalina()
    3 - rotina enxagueTanque()
    4 - rotina acida()
    5 - rotina sanitizante()

    usar if para verificar cada uma das lavagens
    ex:
    for(int i=0; i<sizeof(vetor); i++){
      if(vetor[i]==1){ rotina de enxague()}
      if(vetor[i]==2){ rotina alcalina()}
      if(vetor[i]==3){ rotina enxagueTanque()}
      if(vetor[i]==4){ rotina acida()}
      if(vetor[i]==5){ rotina sanitizante()}
    }
  }
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

// CONTROLE BOMBAS PERISTALTICAS
int relayAcid = 2;
int relayAlc = 3;
int relaySanit = 4;

// VOLUME DE SOLUCAO A SER INSERIDO
float bombaAcid = 2.5;
float bombaAlc = 0.75;
float bombaSanit = 0; // ainda nao descoberto

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
