#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "value.h"
#include "vm.h"
#include "table.h"
#include "object.h"

static void testTables();
static void repl()
{
  char line[1024];
  for (;;)
  {
    printf(">");
    if (!fgets(line, sizeof(line), stdin))
    {
      printf("\n");
      break;
    }
    interpret(line);
  }
}
static char *readFile(const char *path)
{
  FILE *file = fopen(path, "rb");
  // file not found
  if (file == NULL)
  {
    fprintf(stderr, "Could not open file \"%s\"\n", path);
    exit(74);
  }
  fseek(file, 0L, SEEK_END);
  size_t fileSize = ftell(file);
  rewind(file);

  char *buffer = (char *)malloc(fileSize + 1);
  if (buffer == NULL)
  {
    fprintf(stderr, "Not enough memory to read \"%s\"", path);
    exit(74);
  }
  size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
  if (bytesRead < fileSize)
  {
    fprintf(stderr, "Could not read file \"%s\"", path);
    exit(74);
  }
  buffer[bytesRead] = '\0';

  fclose(file);
  return buffer;
}
static void runFile(const char *path)
{
  char *source = readFile(path);
  InterpretResult result = interpret(source);
  free(source);
  if (result == INTERPRET_COMPILE_ERROR)
    exit(65);
  if (result == INTERPRET_RUNTIME_ERROR)
    exit(70);
}

int main(int argc, const char *argv[])
{
  // testTables();
  // return 0;
  initVM();

  if (argc == 1)
  {
    repl();
  }
  else if (argc == 2)
  {
    runFile(argv[1]);
  }
  else
  {
    fprintf(stderr, "Usage: ./clox [path]\n");
    exit(64);
  }
  freeVM();
  return 0;
}

static void testTables()
{
  Table table;
  initTable(&table);
  ObjString *o1 = takeString("a", 1);
  ObjString *o2 = takeString("b", 1);
  tableSet(&table, o1, NIL_VAL);
  Value v;
  printf("\n%d", tableGet(&table, o1, &v));
  printf("\n%d", tableGet(&table, o2, &v));
}