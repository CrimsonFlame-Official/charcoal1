# Charcoal1 — JavaScript Engine

Part of the **Spark** browser runtime. Charcoal1 is an independent execution
environment: source strings become an AST, the AST compiles to compact
bytecode, and a stack-based VM executes it.

## Pipeline

```
source → Spaceless Lexer → AST → Bytecode → VM (+ mark-and-sweep GC)
```

1. **Spaceless lexer** (`src/lexer.*`) — Maximal-munch / keyword-first greedy
   matching over a keyword trie, with symbol boundary triggers and grammar
   context tracking (so `function foo(){return 1;}` lexes correctly with zero
   whitespace).
2. **AST** (`src/ast.h`) — `VariableDeclaration`, `FunctionDeclaration`,
   `ExpressionStatement`, `IfStatement`, `ReturnStatement`, `BinaryExpression`,
   `Literal`, `Identifier`, `CallExpression`.
3. **Bytecode ISA** (`src/bytecode.h`, `src/compiler.*`) — `LOAD_CONST`,
   `LOAD_LOCAL`, `STORE_LOCAL`, `ADD`, `SUB`, `JUMP_IF_FALSE`, `CALL`, `RET`.
4. **VM** (`src/vm.*`) — tight switch-dispatch interpreter loop over the
   bytecode with an evaluation stack and call frames.
5. **GC** (`src/gc.h`) — mark-and-sweep heap for objects/arrays/strings;
   primitives live on frames.

## Build

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/charcoal1_demo
./build/charcoal1_tests
```

Requires a C++17 compiler and CMake 3.16+.

## Roadmap

- [x] Spaceless lexer with keyword trie
- [x] AST + recursive-descent parser
- [x] Bytecode compiler
- [x] Stack VM (arithmetic, locals, calls, conditionals)
- [x] Mark-and-sweep GC
- [ ] Full JS semantics (closures, prototypes, real scoping)
- [ ] Host object bridge to Ignite (see Spark binding layer)
