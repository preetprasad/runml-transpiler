![runml-transpiler logo](logo.svg)

# 🛠️ runml – Mini-Language Transcompiler

### 🧑‍💻 Author

* **Preet Prasad**
* **Platform**: Apple/macOS (also compatible with Ubuntu Linux)

---

[![C Standard](https://img.shields.io/badge/C11-compliant-blue)](https://en.cppreference.com/w/c)
[![Build Status](https://img.shields.io/badge/build-manual-lightgrey)]()
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux-brightgreen)]()

## 📜 Overview

**runml** is a C11-based transpiler that converts a simplified *mini-language* (`.ml` files) into executable C programs. It performs a full transpilation-compile-execute cycle, allowing `.ml` code to be written and run just like a script.

The `ml` mini-language is a deliberately minimal language featuring basic arithmetic, function definitions, and print statements — perfect for exploring compiler design fundamentals.

---

## 🧠 Key Concepts

* Lexical and syntax parsing in C
* File-based transpilation pipeline
* C code generation and dynamic compilation
* Execution using system calls (`system()`, `getpid()`)
* Custom logging, error reporting, and CLI tool behavior

---

## 🏗️ Architecture

* **Language**: C11
* **Entry Point**: `runml.c`
* **Design Pattern**: Two-pass parser-transpiler

  * **Pass 1**: Syntax checking, function and variable declarations
  * **Pass 2**: C code generation and runtime wiring
* **Output**: Temporary files like `ml_<pid>.c` and compiled binaries

---

## ✅ Features

* Parses and executes custom `.ml` mini-language files
* Type inference for real numbers (float vs int formatting)
* Scopes for local and global variables
* Function definitions with tab-based indentation
* Handles command-line arguments as `arg0`, `arg1`, ...
* Clean and configurable debug logging (`-v` flag)
* Error detection for invalid syntax and file handling
* Auto-cleans generated files after execution

---

## 🚀 Quick Start

### Argument Handling

The `runml` transpiler supports runtime argument injection using `arg0`, `arg1`, ..., which correspond to command-line arguments passed to the transpiled program. These are parsed as real numbers and made available as `double` values in the resulting C code.

### 1. Compile the transpiler

```bash
cc -std=c11 -Wall -Werror -o runml runml.c
```

### 2. Run an `.ml` program

```bash
./runml examples/sample01.ml
```

### 3. Enable debug output (optional)

```bash
./runml examples/sample01.ml -v
```

### 4. Pass runtime arguments

```bash
./runml examples/sample02.ml 3.14 2.71
```

---

## 📘 Mini-Language Syntax

### Syntax Error Detection

The transpiler performs syntax validation to catch:

* Mismatched parentheses in expressions
* Invalid variable or function names
* Improper indentation (e.g., spaces instead of a single tab)
* Too many declared variables or functions

Note: The transpiler does not validate the internal structure of expressions (e.g., `a + * b`) per project requirements.

* File extension: `.ml`
* Only real numbers supported
* Statements per line, no semicolons
* Comments start with `#`
* Variables auto-initialized to 0.0
* Valid identifiers: 1–12 lowercase letters
* Access CLI args with `arg0`, `arg1`, etc.
* Control flow via function calls only (no loops/conditionals)
* Functions must be defined before use
* Function bodies use tab (`\t`) for indentation

Example `.ml` file:

```ml
a <- 10
b <- 20
print a + b
```

---

## 🧪 Output Formatting

### Variable Scoping

* All variables default to `0.0` if not assigned.
* Variables declared outside functions are global.
* Variables declared inside a function (including parameters) are scoped locally and are not accessible outside that function.

| Type       | Output Format    |
| ---------- | ---------------- |
| Integer    | `%d`             |
| Float      | `%.6f`           |
| Debug Info | `@ Debug [INFO]` |
| Errors     | `! Error [TYPE]` |

---

## 📂 Sample `.ml` Programs

### Example: Simple Math

```ml
a <- 5
b <- 3
c <- a * b + 2
print c
```

### Example: Function Definition and Return

```ml
function square(x)
	return x * x

n <- 4
result <- square(n)
print result
```

Sample test files (`sample01.ml` to `sample08.ml`) demonstrate assignments, function calls, and print logic. These are useful for testing the transpiler behavior and output formatting.

---

## 📦 Directory Structure

```
runml-transpiler/
├── runml.c             # Main transpiler source
├── README.md           # Project documentation
├── logo.svg            # Project branding asset (used in README)
├── examples/           # Sample .ml programs
│   ├── sample01.ml
│   ├── sample02.ml
│   └── ...
└── .github/            # GitHub assets and configuration (optional)
    └── social_preview.png
```

---

## 🚧 Limitations & Future Improvements

### Known Limitations

* No support for control flow (e.g., `if`, `while`, `for`)
* No expression validation (e.g., invalid arithmetic like `a + * b` is not flagged)
* No user-defined data types or strings
* Only numeric command-line arguments are supported
* Expressions are directly passed to C without full parsing or operator precedence enforcement

### Potential Enhancements

* Implement a full expression parser (e.g., using the Shunting Yard algorithm)
* Support conditionals and loops
* Add type-checking beyond `int`/`double` inference
* Better error messaging with line numbers and hints
* Add a test harness and CI/CD pipeline for automated regression testing

## 📍 Roadmap

This project is no longer actively developed, but the following roadmap outlines possible directions for contributors or future educational use:

### v1.1 (Contributor Milestone)

* [ ] Implement expression parsing with operator precedence
* [ ] Add support for conditional statements (`if`, `else`)
* [ ] Introduce loops (`while`, `for`) in ml syntax
* [ ] Display clearer error messages with line numbers and context

### v2.0 (Advanced Features)

* [ ] Add support for string and boolean data types
* [ ] Extend CLI with flags for debug levels, output paths
* [ ] Implement static type checking
* [ ] Build a web-based transpilation interface or CLI installer

## 📋 Issue Tracker Suggestions

For future maintainers or collaborators, here are some suggested GitHub issue labels:

* `bug`: Unexpected behavior during transpilation or execution
* `enhancement`: New features like control flow or string support
* `documentation`: Improvements to README or code comments
* `good first issue`: Simple issues for first-time contributors
* `help wanted`: More complex features that need collaborative input



## 🙏 Acknowledgements

* Project inspired by the CITS2002 curriculum at UWA
* Original idea and scaffolding by Prof. Chris McDonald
* POSIX man pages and C standard documentation

---

> *“Programs must be written for people to read, and only incidentally for machines to execute.”*
> — Harold Abelson
