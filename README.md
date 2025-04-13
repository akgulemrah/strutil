# StrUtil - Thread-Safe String Utility Library 🚀

![License](https://img.shields.io/badge/license-Unlicense-blue.svg)
![Build](https://img.shields.io/badge/build-passing-success.svg)
![Tests](https://img.shields.io/badge/tests-32%20passing-success.svg)
![Coverage](https://img.shields.io/badge/coverage-100%25-success.svg)

A lightweight, thread-safe string manipulation library for C, featuring dynamic memory management and comprehensive string operations.

### 🌟 Features

- **Thread Safety**: All operations protected with recursive mutex locks
- **Dynamic Memory**: Automatic memory management with safety checks
- **Rich API**: Comprehensive set of string manipulation functions
- **Error Handling**: Robust error reporting system
- **Memory Safety**: Buffer overflow protection and leak prevention
- **Performance**: Optimized operations with power-of-2 capacity growth
- **Stream Support**: Input/Output operations with FILE streams
- **Debug Mode**: Optional debug logging with STRDEBUGMODE flag

### 📋 Installation

1. Install required dependencies:
```bash
# For Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential libcunit1 libcunit1-dev

# For CentOS/RHEL
sudo yum groupinstall "Development Tools"
sudo yum install CUnit CUnit-devel
```

2. Clone the repository:
```bash
git clone https://github.com/yourusername/strutil.git
cd strutil
```

3. Build the library:
```bash
gcc -c src/strutil.c -I./include -o strutil.o
ar rcs libstrutil.a strutil.o
```

4. Link with your project:
```bash
gcc your_program.c -I./include -L. -lstrutil -pthread -o your_program
```

### 🎯 Quick Start

```c
#include <stdio.h>
#include "strutil.h"

int main() {
    struct str *s = str_init();
    if (!s) {
        fprintf(stderr, "Failed to initialize string\n");
        return 1;
    }
    
    if (str_set(s, "Hello") != STR_OK) {
        fprintf(stderr, "Failed to set string\n");
        str_free(s);
        return 1;
    }
    
    if (str_add(s, " World!") != STR_OK) {
        fprintf(stderr, "Failed to append string\n");
        str_free(s);
        return 1;
    }
    
    str_print(s);  // Output: Hello World!
    str_free(s);
    return 0;
}
```

### 📚 API Reference

#### Core Functions
- `str_init()`: Initialize new string structure
- `str_free()`: Clean up resources
- `str_clear()`: Clear string content
- `str_set()`: Set string content
- `str_get_data()`: Get string content
- `str_get_size()`: Get string length
- `str_get_capacity()`: Get buffer capacity
- `str_is_empty()`: Check if empty

#### String Operations
- `str_add()`: Append string
- `str_insert()`: Insert at position
- `str_copy()`: Copy string
- `str_mov()`: Move string content
- `str_find()`: Find substring
- `str_starts_with()`: Check prefix
- `str_ends_with()`: Check suffix

#### String Manipulation
- `str_to_upper()`: Convert to uppercase
- `str_to_lower()`: Convert to lowercase
- `str_to_title_case()`: Title case conversion
- `str_reverse()`: Reverse string
- `str_trim()`: Remove whitespace
- `str_trim_left()`: Left trim
- `str_trim_right()`: Right trim
- `str_pad_left()`: Left padding
- `str_pad_right()`: Right padding
- `str_rem_word()`: Remove word
- `str_swap_word()`: Replace word

#### I/O Operations
- `str_input()`: Read from stream
- `str_add_input()`: Append from stream
- `str_print()`: Write to stdout
- `get_dyn_input()`: Dynamic input

### 🧪 Running Tests

```bash
# Build and run tests
gcc -pthread tests/test.c src/strutil.c -I./include -lcunit -o run_tests
./run_tests
```

### 📈 Performance

- Initial minimum capacity: 16 bytes
- Growth strategy: Power-of-2
- Maximum string size: 32MB
- I/O chunk size: 4KB

### 🤝 Contributing

1. Fork the repository
2. Create your feature branch
3. Commit your changes
4. Push to the branch
5. Open a Pull Request

## 📝 License

This project is licensed under the Unlicense - see the [LICENSE](LICENSE) file for details.
