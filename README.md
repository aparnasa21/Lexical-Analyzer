# Lexical Analyzer in C

A hand-written lexical analyzer (lexer / scanner) for a subset of C, written in C.
It reads a C source file, breaks it into **tokens**, labels each token with its type,
and reports invalid tokens without stopping.

```
$ cat example.c
sum = 3+2;

$ ./lexer example.c
Identifier         : sum
Operator           : =
Constant           : 3
Operator           : +
Constant           : 2
Special character  : ;

Total errors: 0
```

## What is a lexical analyzer?

Lexical analysis is the first stage of a compiler. It turns a stream of characters
into a stream of tokens, where each token is a piece of text (the *lexeme*) together
with its category. The lexer does **not** check whether the tokens make sense together
(for example, whether every `(` has a matching `)`). That is the parser's job, which
comes later in a compiler.

## Features

| Token type | Examples |
|---|---|
| Keyword | `int`, `float`, `return`, `while`, `struct` (20 keywords) |
| Identifier | `count`, `_total`, `printf` |
| Constant | `42`, `10UL`, `0x1F`, `3.14`, `1e-5`, `1.5f`, `'A'`, `"hello\n"` |
| Operator | `+`, `==`, `<=`, `&&`, `++`, `->`, `.`, `<<=`, `>>=` |
| Special character | `, ; { } ( ) [ ]` |
| Array | a name directly followed by `[`, such as `nums[5]` or `nums [5]` |
| Preprocessor | a whole directive line, such as `#include <stdio.h>` |

Other behaviour:

- Skips whitespace, `// line comments` and `/* block comments */`.
- Recognizes the longest operator that fits (`<<=` rather than `<<` and `=`).
- Handles escape sequences inside strings and character literals (`"a\"b"`).
- Reports malformed input as an error and **keeps going**, so one mistake does not hide the rest of the file. This includes malformed numbers (`1abc`, `0x`, `1e`, `10LLL`), unterminated strings, empty character literals (`''`), and unknown characters (`@`, `$`).
- Works with both Linux (`\n`) and Windows (`\r\n`) line endings.

## Project structure

```
.
├── lexer.h     token types, the Token struct, function declarations
├── lexer.c     the lexer: reading characters, building and classifying tokens
├── main.c      driver: opens the file, prints each token and the error count
└── tests/      small C files that exercise the lexer
```

## Build and run

You need a C compiler such as `gcc`.

```bash
gcc main.c lexer.c -o lexer
./lexer tests/basic.c
```

Running `./lexer` without a file name prints a usage message.

To see every warning the compiler can give:

```bash
gcc -Wall -Wextra -pedantic -std=c99 main.c lexer.c -o lexer
```

## How it works

Each call to `getNextToken()` produces one token in three steps:

```
1. COLLECT    skip whitespace and comments, then read characters into a lexeme.
              The first character decides which reader is used:
                #            -> readPreprocessor
                letter or _  -> readWord
                digit        -> readNumber
                " or '       -> readQuoted
                operator     -> readOperator
                anything else -> a single character

2. CLASSIFY   categorizeToken() asks the helper checks in order:
              isKeyword, isOperator, isSpecialCharacter, isConstant, isIdentifier.
              Nothing matches -> UNKNOWN (reported as an error).

3. ARRAY      if the token is an identifier and the next non-space
              character is '[', it is labelled an array.
```

Two design choices are worth noting:

- **Collecting is separate from classifying.** The readers only gather characters.
  They do not decide what kind of token they found. This keeps each function small
  and means a new token type needs a change in one place.
- **Read greedily, then judge.** `readNumber` collects the whole run of letters and
  digits, so `1abc` becomes one lexeme and is then rejected by `isConstant`. Without
  this, the lexer would silently split it into the number `1` and the identifier `abc`.

Lookahead uses `ungetc`, which only guarantees one character of pushback. The code
is written so that it never needs more than that.

## Tests

The `tests/` folder holds small inputs. Each one targets a specific area:

| File | What it checks | Expected errors |
|---|---|---|
| `basic.c` | keywords, identifiers, operators, strings, comments, arrays | 0 |
| `invalid_char.c` | an unknown character (`@`) is reported and scanning continues | 1 |
| `program.c` | a realistic program with `#include`, `#define`, a loop and `printf` | 0 |
| `numbers.c` | valid numbers (hex, float, exponent, suffixes) and invalid ones | 5 |
| `operators.c` | `<<=`, `>>=`, `->`, `.`, `>=` | 0 |
| `literals_arrays.c` | character literals, empty `''`, arrays written with spaces | 1 |
| `edge_cases.c` | `1abc`, `0x1F`, `->`, `>>`, `1e5`, `''`, `a [5]` | 2 |

Run one with `./lexer tests/numbers.c`, or all of them:

```bash
for f in tests/*.c; do echo "== $f"; ./lexer "$f" | tail -1; done
```

## Known limitations

This is a learning project, not a full C lexer.

- **Only 20 keywords** are recognized. Others such as `long`, `short`, `unsigned`, `enum`, `typedef` and `goto` are reported as identifiers.
- **`:`, `?`, `^` and `~` are not supported** and are reported as invalid tokens. This also means labels, `case 1:` and the ternary operator produce errors.
- **`.5`** (a float with no leading digit) is read as the operator `.` followed by `5`.
- **`...`** is read as three separate `.` operators.
- **Arrays:** any name followed by `[` is labelled an array, so a pointer being indexed (`p[2]`) is also labelled an array. Telling them apart needs knowledge of declarations, which a lexer does not have.
- **An unterminated `/* ... */` comment** silently runs to the end of the file with no error.
- **Preprocessor directives** are one token per line. Directives continued with `\` over several lines are not handled.
- **Tokens longer than 99 characters** are truncated.
- **Grammar is not checked.** For example `1.2.3` is read as `1.2`, `.`, `3`, which is a valid token sequence. Rejecting it is a parser's job.

## Possible extensions

- Add the missing keywords and operators (`:`, `?`, `^`, `~`, `...`).
- Add line and column numbers to each token and to error messages.
- Free resources properly with a `closeLexer()` function that calls `fclose`.
- Feed the token stream into a small expression parser.

## Background

Built as a learning project to practise C and to understand the first stage of a
compiler. The project is based on a course skeleton (`lexer.h`, `main.c` and function
stubs), which was then extended step by step into the lexer described above.
