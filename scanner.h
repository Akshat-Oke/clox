#ifndef clox_scanner_h
#define clox_scanner_h

typedef enum
{
  // Single-character tokens
  TOKEN_LEFT_PAREN, // 0
  TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE,
  TOKEN_RIGHT_BRACE,
  TOKEN_COMMA,
  TOKEN_DOT,
  TOKEN_MINUS,
  TOKEN_PLUS,
  TOKEN_SEMICOLON,
  TOKEN_SLASH,
  TOKEN_STAR, // 10
              //  One or two character tokens
  TOKEN_BANG, // 11
  TOKEN_BANG_EQUAL,
  TOKEN_EQUAL,
  TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER,
  TOKEN_GREATER_EQUAL,
  TOKEN_LESS,
  TOKEN_LESS_EQUAL, // 18
                    //  Literals
  TOKEN_IDENTIFIER, // 19
  TOKEN_STRING,
  TOKEN_NUMBER,
  // Keywords
  TOKEN_AND,
  TOKEN_CLASS, // 23
  TOKEN_ELSE,
  TOKEN_FALSE, // 25
  TOKEN_FOR,
  TOKEN_FUN,
  TOKEN_IF,
  TOKEN_NIL,
  TOKEN_OR, // 30
  TOKEN_PRINT,
  TOKEN_RETURN,
  TOKEN_SUPER,
  TOKEN_TRUE, // 34
  TOKEN_VAR,
  TOKEN_WHILE,
  TOKEN_THIS, // 37

  TOKEN_ERROR, // 38
  TOKEN_EOF
} TokenType;

typedef struct
{
  TokenType type;
  const char *start;
  int length;
  int line;
} Token;

void initScanner(const char *source);
Token scanToken();
#endif