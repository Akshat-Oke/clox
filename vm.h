#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "table.h"
#include "value.h"
#define STACK_MAX 256
typedef struct
{
  Chunk *chunk;
  // points to the instruction to be executed
  uint8_t *ip; // pointer to instruction in chunk
  Value stack[STACK_MAX];
  // top = ununsed space at the top,
  // next value pushed is here
  Value *stackTop;
  // interned strings (unique strings stored only once)
  Table strings;
  // objects as linked list
  Obj *objects;
} VM;

typedef enum
{
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char *source);
void push(Value);
Value pop();

#endif