# Minilang Compiler

Minilang - простой язык программирования, поддерживающий функции с параметрами, вложенные пространства имен, условные операторы, циклы и ввод-вывод. Компилятор генерирует LLVM IR, а затем транслирует его в ARMv8 ассемблерный код. Проект демонстрирует основные этапы компиляции: лексический, синтаксический, семантический анализ и генерацию кода.

## Особенности

- Типы данных: `int`, `bool`
- Функции с параметрами и возвращаемыми значениями
- Условный оператор `if` (с необязательным `else`)
- Цикл `while`
- Встроенные функции ввода-вывода:
  - `printInt(x)` — выводит целое число с переводом строки
  - `printBool(b)` — выводит `true` или `false` с переводом строки
  - `readInt()` — читает целое число со знаком из стандартного ввода (одно число на строке!)
- Генерация ассемблерного кода

## Требования

- Компилятор C++20
- Библиотека LLVM версии 15 или новее
- Google Test для запуска тестов

## Сборка

1. Клонируйте репозиторий:

   ```
   git clone https://github.com/ilyapvl/minilang.git
   cd minilang
   ```

2. Убедитесь, что `llvm-config` доступен в PATH

3. Выполните сборку:

   ```
   mkdir build 
   cd build
   cmake ..
   make minilang
   ```

   Будет создан исполняемый файл `minilang`

## Использование

Скомпилируйте исходный файл c кодом на minilang:

```
./minilang ../example.txt
```

Эта команда создаст файл `output.s` с ассемблерным кодом. Затем соберите исполняемый файл, используя предоставленный ассемблерный модуль системных вызовов:

```
gcc output.s ../syscalls/syscalls.s -o main
```

Запустите программу:

```
./main
```

## Синтаксис языка

### Объявление переменных

```c
int x;
int y = 10;
bool flag = true;
```

### Функции

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

### Пространства имен

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

### Условные операторы и циклы

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

### Ввод-вывод

```
int x = readInt();
printInt(x);
printBool(x > 0);
```



### Комментарии

```
//однострочный

/* много-
        строчный */
```

## Структура проекта

- `ast.hpp`, `ast.cpp` — классы абстрактного синтаксического дерева
- `grammar.hpp`, `grammar.cpp` — грамматика и построение LR(1)/LALR таблиц
- `lexer.hpp`, `lexer.cpp` — лексический анализатор
- `parser.hpp`, `parser.cpp` — синтаксический анализатор
- `semantic.hpp`, `semantic.cpp` — семантический анализатор и таблицы символов
- `irgen.hpp`, `irgen.cpp` — генератор LLVM IR, включая встроенные функции
- `codegen.hpp`, `codegen.cpp` — генерация ассемблерного кода и оптимизации
- `syscalls.s` — ассемблерный код системных вызовов
- `main.cpp` — точка входа компилятора

## Примеры


### Пример 1: Факториал

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


### Пример 2: пространства имен

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



## Лицензия

Этот проект распространяется под лицензией MIT. Подробнее см. в файле [LICENSE](LICENSE).
