#include <stdio.h>
#include <stdarg.h>
#include "common.h"

void debugLog(const char *format, ...)
{
  return;
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);
}