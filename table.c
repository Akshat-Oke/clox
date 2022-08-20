#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "value.h"
#include "object.h"
#include "table.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table *table)
{
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}
void freeTable(Table *table)
{
  FREE_ARRAY(Entry, table->entries, table->capacity);
  initTable(table);
}
static Entry *findEntry(Entry *entries, int capacity, ObjString *key)
{
  uint32_t index = key->hash % capacity;
  Entry *tombstone = NULL;
  for (;;)
  {
    Entry *entry = &entries[index];
    if (entry->key == NULL)
    {
      if (IS_NIL(entry->value))
      {
        // means this is an empty entry, not
        // a tombstone
        return tombstone != NULL ? tombstone : entry;
      }
      else
      {
        // found a tombstone
        if (tombstone != NULL)
          tombstone = entry;
      }
    }
    index = (index + 1) % capacity;
  }
}
static void adjustCapacity(Table *table, int capacity)
{
  Entry *entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++)
  {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }
  // copy existing keys to new entries array
  table->count = 0;
  // we must not copy tombstones
  for (int i = 0; i < table->capacity; i++)
  {
    Entry *entry = &table->entries[i];
    if (entry->key == NULL)
      continue;
    Entry *dest = findEntry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }
  // delete old array
  FREE_ARRAY(Entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}
// Sets a key to value, overwriting if it exists
// returns if the key existed already
bool tableSet(Table *table, ObjString *key, Value value)
{
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD)
  {
    int capacity = GROW_CAPACITY(table->capacity);
    adjustCapacity(table, capacity);
  }
  Entry *entry = findEntry(table->entries, table->capacity, key);
  bool isNewKey = entry->key == NULL;
  // increase count only if we use an empty bucket
  // and not a tombstone
  if (isNewKey && IS_NIL(entry->value))
    table->count++;

  entry->key = key;
  entry->value = value;
  return isNewKey;
}
void tableAddAll(Table *from, Table *to)
{
  for (int i = 0; i < from->capacity; i++)
  {
    Entry *entry = &from->entries[i];
    if (entry->key != NULL)
    {
      tableSet(to, entry->key, entry->value);
    }
  }
}
// sets value to the found value and returns true
// if the key was found
bool tableGet(Table *table, ObjString *key, Value *value)
{
  if (table->count == 0)
    return false;
  Entry *entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL)
    return false;
  *value = entry->value;
  return true;
}
// returns if key was deleted
bool tableDelete(Table *table, ObjString *key)
{
  if (table->count == 0)
    return false;
  // find entry
  Entry *entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL)
    return false;
  // place tombstone in the entry
  entry->key = NULL;
  entry->value = BOOL_VAL(true);
  return true;
}
// here we compare two strings char by char
// the rest of the compiler can assume that strings having same
// memory addresses are also the same
ObjString *tableFindString(Table *table, const char *chars, int length, uint32_t hash)
{
  if (table->count == 0)
    return NULL;
  uint32_t index = hash % table->capacity;
  for (;;)
  {
    Entry *entry = &table->entries[index];
    if (entry->key == NULL)
    {
      // stop if empty, non-tombstone
      if (IS_NIL(entry->value))
        return NULL;
    }
    else if (entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->chars, chars, length) == 0)
    {
      // found the string in hash table
      return entry->key;
    }
    index = (index + 1) % table->capacity;
  }
}