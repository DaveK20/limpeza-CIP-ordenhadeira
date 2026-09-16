#include "TemperatureSensor.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Config.h"
#include "Globals.h"
#include "Actuators.h"

static OneWire oneWire(tempSensor);
static DallasTemperature sensors(&oneWire); // encaminha referencias OneWire para o sensor

void tempSensorInit()
{
  sensors.begin();
}

float tempAgua()
{
  sensors.requestTemperatures();
  float temperatura = sensors.getTempCByIndex(0);
  return temperatura;
}

bool controlarTemperatura(float tempSolucao)
{
  if (interromper)
    return false;

  float temp = tempAgua();

  if (digitalRead(boiaSolucao))
  {
    if (temp < tempSolucao - 2)
    {
      aquecerResistencia(HIGH);
    }
    else if (temp > tempSolucao + 2)
    {
      aquecerResistencia(LOW);
    }
    else
    {
      aquecerResistencia(LOW);
      return true;
    }
  }
  else
  {
    aquecerResistencia(LOW);
  }

  return false;
}
