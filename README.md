# Minilang Compiler

Minilang is a simple programming language that supports functions with parameters, nested namespaces, conditional statements, loops, and I/O. The compiler generates LLVM IR, which is then translated into ARM assembly code. The project demonstrates the main stages of compilation: lexical, syntax, semantic analysis, and code generation

## Features

- Types: `int`, `bool`
- Функции с параметрами и возвращаемыми значениями
- Statements: `while`, `if` (with optional `else`)
- Built-in I/O functions:
  - `printInt(x)` — prints single integer
  - `printBool(b)` — prints single bool as `true` or `false`
  - `readInt()` — reads an integer from stdin (only one integer on a single line is supported)
- Assembler code generation

## Requirements

- C++20 compiler
- LLVM 15+
- Google Test for running tests

## How to build

1. Clone the repository:

   ```
   git clone https://github.com/ilyapvl/minilang.git
   cd minilang
   ```

2. Check that `llvm-config` is in PATH

3. Build:

   ```
   mkdir build 
   cd build
   cmake ..
   make minilang
   ```

   This will create `minilang` executable

## Usage

Compile a file with minilang source code:

```
./minilang <path-to-file>
```

This will produce `output.s`. Then, build an executable using the provided syscall assembly module:

```
g++ output.s ../syscalls/syscalls.s -o main
```

Run the program:

```
./main
```

## Language syntax

### Variable declarations

```c
int x;
int y = 10;
bool flag = true;
```

### Functions

```
func add(int a, int b) -> int
{
    return a + b;
}

func main() -> int
{
    int result = add(3, 5);
    printInt(result);

    return 0;
}
```

### Namespaces

```
@my_namespace
{
    func test() -> int { return 0; } 
}

func main() -> int
{
    my_namespace::test();

    return 0;
}

```

### If/while statement

```
if (x > 0)
{
    printInt(x);
}

else
{
    printInt(-x);
}

while (i < 10)
{
    i = i + 1;
}
```

### In/out

```
int x = readInt();
printInt(x);
printBool(x > 0);
```



### Comments

```
//single line

/* multi
        line */
```

## Project structure

- `ast.hpp`, `ast.cpp` — abstract syntax tree classes
- `grammar.hpp`, `grammar.cpp` — grammar and building of LR(1)/LALR tables
- `lexer.hpp`, `lexer.cpp` — lexical analyxer
- `parser.hpp`, `parser.cpp` — syntax analyzer
- `semantic.hpp`, `semantic.cpp` — semantical analyzer
- `irgen.hpp`, `irgen.cpp` — LLVM IR generation
- `codegen.hpp`, `codegen.cpp` — assembler code generation
- `syscalls.s` — syscalls

## Examples


### Factorial

```
func factorial(int n) -> int
{
    int result = 1;
    while (n > 1)
    {
        result = result * n;
        n = n - 1;
    }
    return result;
}

func main() -> int
{
    int n = readInt();
    printInt(factorial(n));
    return 0;
}
```


### Namespaces

```
@ns
{
    func test(int x, int y) -> int
    {
        return x + y;
    }
}

func test(int x, int y) -> int
{
    return x - y;
}

func main() -> int
{
    int x = readInt();
    int y = readInt();

    printInt(ns::test(x, y)); // = x + y
    printInt(test(x, y));     // = x - y

    return 0; 
}

```


