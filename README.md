# StrUtil - Thread-Safe String Utility Library

![License](https://img.shields.io/badge/license-Unlicense-blue.svg)
![Build](https://img.shields.io/badge/build-passing-success.svg)

A lightweight, thread-safe string manipulation library for C, featuring dynamic memory management and comprehensive string operations.

## 🚀 Features

- **Thread Safety**: All operations are protected with mutex locks
- **Dynamic Memory**: Automatic memory management with safety checks
- **String Operations**: Rich set of string manipulation functions
- **Error Handling**: Comprehensive error reporting system
- **Memory Safety**: Buffer overflow protection and leak prevention
- **Performance**: Optimized operations with minimal overhead

## 📋 Installation

Simply include the header file in your project:

```c
#include "strutil.h"
```

Compile with threading support:

```bash
gcc -o your_program your_program.c -pthread
```

## 🎯 Quick Start

```c
#include "strutil.h"

int main() {
    // Initialize string
    str *s = str_init();
    
    // Set content
    str_set(s, "Hello");
    
    // Append string
    str_add(s, ", World!");
    
    // Print content
    str_print(s);  // Output: Hello, World!
    
    // Clean up
    str_free(s);
    return 0;
}
```

## 📚 API Reference

### Core Functions
- `str_init()`: Initialize new string structure
- `str_free()`: Clean up resources
- `str_clear()`: Clear string content

### String Operations
- `str_set()`: Set string content
- `str_get()`: Get string content
- `str_add()`: Append string
- `str_get_size()`: Get string length
- `str_is_empty()`: Check if empty

### String Manipulation
- `str_to_upper()`: Convert to uppercase
- `str_to_lower()`: Convert to lowercase
- `str_to_title_case()`: Capitalize words
- `str_reverse()`: Reverse string
- `str_rem_word()`: Remove word
- `str_swap_word()`: Replace word

### Input/Output
- `str_input()`: Read from stdin
- `str_add_input()`: Append from stdin
- `str_print()`: Write to stdout

## 🔍 Error Handling

The library uses an enumerated type `Str_err_t` for error reporting:

```c
Str_err_t result = str_add(s, "text");
if (result != STR_OK) {
    // Handle error
}
```

Common error codes:
- `STR_OK`: Success
- `STR_NULL`: NULL pointer error
- `STR_NOMEM`: Memory allocation failed
- `STR_INVALID`: Invalid argument
- `STR_OVERFLOW`: Buffer overflow prevented

## 🧪 Testing

Run the comprehensive test suite:

```bash
gcc -o strutil_test strutil_test.c -pthread
./strutil_test
```

## 📈 Performance

- Optimized memory allocation strategy
- Efficient string operations
- Minimal mutex locking overhead
- Power-of-2 capacity growth

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch
3. Commit your changes
4. Push to the branch
5. Open a Pull Request

## 📄 License

This project is licensed under the Unlicense - see the [LICENSE](LICENSE) file for details. This means the code is dedicated to the public domain and can be used freely, without any restrictions.

## ✍️ Author

**Emrah Akgül**
- GitHub: [@akgulemrah](https://github.com/akgulemrah)

## 🙏 Acknowledgments

- Inspired by modern string handling requirements
- Built with security and performance in mind
- Thoroughly tested in production environments 
