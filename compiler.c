#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "vm.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct
{
  Token current;
  Token previous;
  bool hadError;
  bool panicMode;
} Parser;

typedef enum
{
  PREC_NONE,       // 0, lowest
  PREC_ASSIGNMENT, // lowest precendence
  PREC_OR,
  PREC_AND,        // as numbers increase to the
  PREC_EQUALITY,   // bottom, we use
  PREC_COMPARISON, // this same order
  PREC_TERM,       // to compare precendences
  PREC_FACTOR,
  PREC_UNARY,
  PREC_CALL,
  PREC_PRIMARY, // highest precendence
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct
{
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

Parser parser;
Chunk *compilingChunk;
static Chunk *currentChunk()
{
  return compilingChunk;
}
static void errorAt(Token *token, const char *message)
{
  if (parser.panicMode)
    return;
  parser.panicMode = true;
  fprintf(stderr, "[line %d] Error", token->line);
  if (token->type == TOKEN_EOF)
  {
    fprintf(stderr, "at end");
  }
  else if (token->type == TOKEN_ERROR)
  {
    // ignore
  }
  else
  {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }
  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}

static void error(const char *message)
{
  errorAt(&parser.previous, message);
}
static void errorAtCurrent(const char *message)
{
  errorAt(&parser.current, message);
}

static void advance()
{
  parser.previous = parser.current;
  // keep skipping error tokens and reporting them
  // stop at the next valid token and store
  // in current
  // Thus, the rest of the parser sees only valid tokens
  for (;;)
  {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR)
      break;
    errorAtCurrent(parser.current.start);
  }
}

static void consume(TokenType type, const char *message)
{
  if (parser.current.type == type)
  {
    advance();
    return;
  }
  errorAtCurrent(message);
}
static bool check(TokenType type)
{
  return parser.current.type == type;
}
static bool match(TokenType type)
{
  if (!check(type))
    return false;
  advance();
  return true;
}

static void emitByte(uint8_t byte)
{
  writeChunk(currentChunk(), byte, parser.previous.line);
}
static void emitBytes(uint8_t byte1, uint8_t byte2)
{
  emitByte(byte1);
  emitByte(byte2);
}
static void emitReturn()
{
  emitByte(OP_RETURN);
}
static uint8_t makeConstant(Value value)
{
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX)
  {
    error("Too many constants in one chunk");
    return 0;
  }
  return (uint8_t)constant;
}
static void emitConstant(Value value)
{
  emitBytes(OP_CONSTANT, makeConstant(value));
}
static void endCompiler()
{
  emitReturn();
#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError)
  {
    disassembleChunk(currentChunk(), "code");
  }
#endif
}
//////The grammar code begins
static void expression();
static void statement();
static void declaration();
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence p);

static uint8_t identifierConstant(Token *name)
{
  return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}
static uint8_t parseVariable(const char *errorMessage)
{
  consume(TOKEN_IDENTIFIER, errorMessage);
  return identifierConstant(&parser.previous);
}
static void defineVariable(uint8_t global)
{
  emitBytes(OP_DEFINE_GLOBAL, global);
}

// In a prefix parser function,
// the leading operator is already consumed
// and in  an infix function, the operator is
// also consumed and the left operand compiled
static void binary(bool _canAssign)
{
  // left-right-operator will be the stack in VM
  TokenType operatorType = parser.previous.type;
  ParseRule *rule = getRule(operatorType);
  // compile right with higher precedence than this operator
  // this is because we want left associativity of
  // binary operators
  // 1+2+3 is ((1+2)+3)
  parsePrecedence((Precedence)(rule->precedence + 1));
  // example: 2*3+4
  // we already have 2 compiled and we see *
  // so compile with precedecne higher than *, which is just 3
  // so it will be (2*3) + 4
  // we now get to + with (2*3) compiled
  switch (operatorType)
  {
  case TOKEN_BANG_EQUAL:
    emitBytes(OP_EQUAL, OP_NOT);
    break;
  case TOKEN_EQUAL_EQUAL:
    emitByte(OP_EQUAL);
    break;
  case TOKEN_GREATER:
    emitByte(OP_GREATER);
    break;
  case TOKEN_GREATER_EQUAL:
    emitBytes(OP_LESS, OP_NOT);
    break;
  case TOKEN_LESS:
    emitByte(OP_LESS);
    break;
  case TOKEN_LESS_EQUAL:
    emitBytes(OP_GREATER, OP_NOT);
    break;
  case TOKEN_PLUS:
    emitByte(OP_ADD);
    break;
  case TOKEN_MINUS:
    emitByte(OP_SUBTRACT);
    break;
  case TOKEN_STAR:
    emitByte(OP_MULTIPLY);
    break;
  case TOKEN_SLASH:
    emitByte(OP_DIVIDE);
    break;
  default:
    return; // unreachable
  }
}
static void literal(bool _canAssign)
{
  switch (parser.previous.type)
  {
  case TOKEN_FALSE:
    emitByte(OP_FALSE);
    break;
  case TOKEN_NIL:
    emitByte(OP_NIL);
    break;
  case TOKEN_TRUE:
    emitByte(OP_TRUE);
    break;
  default:
    return; // unreachable
  }
}
// Grouping (expr) does not emit any bytecode
static void grouping(bool _canAssign)
{
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression");
}
static void number(bool _canAssign)
{
  double value = strtod(parser.previous.start, NULL);
  // printf("[Debug print] Found value %g\n", AS_NUMBER(NUMBER_VAL(value)));
  emitConstant(NUMBER_VAL(value));
}
static void string(bool _canAssign)
{
  emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
                                  parser.previous.length - 2)));
}
// Ex. foo = 4;
//'foo' is current token
static void namedVariable(Token name, bool canAssign)
{
  uint8_t arg = identifierConstant(&name);
  if (canAssign && match(TOKEN_EQUAL))
  {
    // means it is an assignment statement
    expression(); // compile the following expression
    emitBytes(OP_SET_GLOBAL, arg);
  }
  else
    emitBytes(OP_GET_GLOBAL, arg);
}
static void variable(bool canAssign)
{
  namedVariable(parser.previous, canAssign);
}
// -123
// '-' is in previous and '123'(operand) in current
static void unary(bool _canAssign)
{
  TokenType operatorType = parser.previous.type;
  // compile operand
  parsePrecedence(PREC_UNARY);
  // emit operand instruction
  switch (operatorType)
  {
  case TOKEN_BANG:
    emitByte(OP_NOT);
    break;
  case TOKEN_MINUS:
    emitByte(OP_NEGATE);
    break;
  default:
    return; // unreachable
  }
}
// the parse table
ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, binary, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};
static void parsePrecedence(Precedence precedence)
{
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL)
  {
    error("Expected expression");
    return;
  }
  // a*b=c+d; is not a valid assignment
  // so if we are in here with a*|b=c+d (looking at b)
  // and are going to call variable() through prefixRule(),
  // variable() should consider the trailing '=' as a valid assignment
  // only if the precedence of current expr (b=c+d) is lower than precedence
  // of '='.
  //
  // But since precedence of '=' is lowest, this only allows a top-level assignment
  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);
  while (precedence <= getRule(parser.current.type)->precedence)
  {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    // infix don't need this canAssign flag, but since
    // all these have the same function signatures in the table,
    // we have to include it
    infixRule(canAssign);
  }
  // if we encounter a rouge '=', we will end up here
  // since no precedence will be < '='
  if (canAssign && match(TOKEN_EQUAL))
  {
    error("Invalid assignment target");
  }
}
static void expression()
{
  parsePrecedence(PREC_ASSIGNMENT);
}
static void varDeclaration()
{
  uint8_t global = parseVariable("Expected variable name");
  if (match(TOKEN_EQUAL))
  {
    expression();
  }
  else
  {
    emitByte(OP_NIL); // assign nil to variable if there is no initializer
  }
  consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration");

  defineVariable(global);
}
static void expressionStatement()
{
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression");
  emitByte(OP_POP);
}
static void printStatement()
{
  expression();
  consume(TOKEN_SEMICOLON, "Expected ';' after expression");
  emitByte(OP_PRINT);
}
static void synchronize()
{
  parser.panicMode = false;
  while (parser.current.type != TOKEN_EOF)
  {
    if (parser.previous.type == TOKEN_SEMICOLON)
      return;
    switch (parser.current.type)
    {
    case TOKEN_CLASS:
    case TOKEN_FUN:
    case TOKEN_VAR:
    case TOKEN_FOR:
    case TOKEN_IF:
    case TOKEN_WHILE:
    case TOKEN_PRINT:
    case TOKEN_RETURN:
      return;
    default:;
    }
    advance();
  }
}
static void declaration()
{
  if (match(TOKEN_VAR))
  {
    varDeclaration();
  }
  else
  {
    statement();
  }
  if (parser.panicMode)
    synchronize();
}
static void statement()
{
  if (match(TOKEN_PRINT))
  {
    printStatement();
  }
  else
  {
    expressionStatement();
  }
}

static ParseRule *getRule(TokenType type)
{
  return &rules[type];
}
////// Grammar code ends
bool compile(const char *source, Chunk *chunk)
{
  initScanner(source);
  compilingChunk = chunk;

  parser.hadError = false;
  parser.panicMode = false;
  advance();
  // expression();
  while (!match(TOKEN_EOF))
  {
    declaration();
  }
  consume(TOKEN_EOF, "Expected end of input");
  endCompiler();
  return !parser.hadError;
  // int line = -1;
  // for (;;)
  // {
  //   Token token = scanToken();
  //   if (token.line != line)
  //   {
  //     printf("%4d ", token.line);
  //     line = token.line;
  //   }
  //   else
  //   {
  //     printf("   | ");
  //   }
  //   printf("%2d '%.*s'\n", token.type, token.length, token.start);
  //   if (token.type == TOKEN_EOF)
  //     break;
  // }
}