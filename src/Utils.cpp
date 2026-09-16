#include "Utils.h"
#include <avr/wdt.h>

void safeDelay(unsigned long ms)
{
  const unsigned int step = 500; // intervalo seguro (ms)

  while (ms > 0)
  {
    unsigned long chunk = (ms > step) ? step : ms;

    delay(chunk);
    wdt_reset(); // alimenta o watchdog

    ms -= chunk;
  }
}
