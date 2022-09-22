# Lox

An object oriented programming language, made by [Robert Nystrom](https://craftinginterpreters.com/)

## About

Lox is an interpreted language, with strings supported.

## Usage

Firstly, `cd` into this folder.

1. **REPL**
   You can run Lox as a REPL by providing no command line arguments.

```bash
> ./clox
```

2. **Run a file**
   Provide the filename as the first argument to execute the lox program saved as that file. You can try the test file `z_test.lox`.

```bash
> ./clox z_test.lox
```

Since I have built this on Windows, you'll have to run `make` first to build for your OS and follow the above steps.

## Additional features

These are features that are not implemented in the book but I have added myself.

1. Arity check for native functions. Very straightforward implementation. (Ex. `clock(1)` is invalid)
2. [In development] Forward declaration support. Since this is a single pass compiler, there was no way to write mutually recursive functions. This mimicks `C` or `C++` forward declarations.

## Building

| Command     | Action                    |
| ----------- | ------------------------- |
| `make`      | Build                     |
| `make run`  | Run REPL                  |
| `make test` | Run z_test.clox           |
| `make go`   | Build and run z_test.clox |
