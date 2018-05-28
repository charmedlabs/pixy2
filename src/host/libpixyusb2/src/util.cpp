#include <stdio.h>
#include <time.h>
#include "util.h"


uint32_t millis()
{
  uint64_t c = clock();
  c *= 1000;
  c /= CLOCKS_PER_SEC;
  return c;
}

void delayMicroseconds(uint32_t us)
{
  // Called from TPixy2 class --  not needed because we are using USB not serial
}

Console Serial;


void Console::print(const char *msg)
{
  printf("%s", msg);
}

void Console::println(const char *msg)
{
  printf("%s\n", msg);
}


