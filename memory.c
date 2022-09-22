#include <stdlib.h>
#include "memory.h"
#include "vm.h"
// we can count bytes being used by this function
// since all memory allocations go through here
void *reallocate(void *pointer, size_t oldSize, size_t newSize)
{
  if (newSize == 0)
  {
    // free allocation
    free(pointer);
    return NULL;
  }
  void *result = realloc(pointer, newSize);
  // if NULL, sufficient memory was not found.
  if (result == NULL)
    exit(1);
  return result;
}
static void freeObject(Obj *object)
{
  switch (object->type)
  {
  case OBJ_CLOSURE:
  {
    // free the array, but not the Upvalues
    ObjClosure *closure = (ObjClosure *)object;
    FREE_ARRAY(ObjUpvalue *, closure->upvalues, closure->upvalueCount);
    // do not free the function because other
    // closures may use the same function
    FREE(ObjClosure, object);
    break;
  }
  case OBJ_FUNCTION:
  {
    ObjFunction *func = (ObjFunction *)object;
    freeChunk(&func->chunk);
    FREE(ObjFunction, object);
    break;
  }
  case OBJ_NATIVE:
    FREE(ObjNative, object);
    break;
  case OBJ_STRING:
  {
    ObjString *string = (ObjString *)object;
    FREE_ARRAY(char, string->chars, string->length + 1);
    FREE(ObjString, object);
    break;
  }
  case OBJ_UPVALUE:
    // upvalue does not own the value
    FREE(ObjUpvalue, object);
    break;
  }
}
void freeObjects()
{
  Obj *object = vm.objects;
  while (object != NULL)
  {
    Obj *next = object->next;
    freeObject(object);
    object = next;
  }
}