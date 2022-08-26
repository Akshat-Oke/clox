#ifndef clox_vm_h
#define clox_vm_h

#include "object.h"
#include "chunk.h"
#include "table.h"
#include "value.h"
#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)
typedef struct
{
  ObjFunction *function;
  uint8_t *ip;
  // the start of the slots in the VM's stack
  Value *slots;
} CallFrame;
typedef struct
{
  // Chunk *chunk; ->is now in the function
  // points to the instruction to be executed
  // is specific to a function
  // uint8_t *ip; // pointer to instruction in chunk
  CallFrame frames[FRAMES_MAX];
  int frameCount;
  Value stack[STACK_MAX];
  // top = ununsed space at the top,
  // next value pushed is here
  Value *stackTop;
  // global variables
  Table globals;
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