# StrUtil - Thread-Safe String Utility Library 🚀

![License](https://img.shields.io/badge/license-Unlicense-blue.svg)
![Build](https://img.shields.io/badge/build-passing-success.svg)

A lightweight, thread-safe string manipulation library for C, featuring dynamic memory management and comprehensive string operations.

### 🌟 Features

- **Thread Safety**: All operations protected with recursive mutex locks
- **Dynamic Memory**: Automatic memory management with safety checks
- **Rich API**: Comprehensive set of string manipulation functions
- **Error Handling**: Robust error reporting system via `Str_err_t` enum
- **Memory Safety**: Buffer overflow protection and leak prevention
- **Performance**: Optimized operations with power-of-2 capacity growth
- **Stream Support**: Input/Output operations with `FILE` streams
- **Debug Mode**: Optional debug logging with `STRDEBUGMODE` flag

### 📋 Installation

1. Clone the repository:
```bash
git clone https://github.com/akgulemrah/strutil.git
cd strutil
```

2. Build using CMake:
```bash
mkdir build
cd build
cmake ..
make
```

### 🎯 Quick Start

```c
#include <stdio.h>
#include "strutil.h"

int main() {
    // Initialize a new string
    struct str *s = str_init();
    if (!s) {
        fprintf(stderr, "Failed to initialize string\n");
        return 1;
    }
    
    // Set content
    if (str_set(s, "Hello") != STR_OK) {
        fprintf(stderr, "Failed to set string\n");
        str_free(s);
        return 1;
    }
    
    // Append content
    if (str_add(s, " World!") != STR_OK) {
        fprintf(stderr, "Failed to append string\n");
        str_free(s);
        return 1;
    }
    
    // Print and cleanup
    str_print(s);  // Output: Hello World!
    str_free(s);
    return 0;
}
```

### 📚 API Reference

#### Core Functions
- `str_init()`: Initialize a new string object
- `str_free(ptr)`: Safely free the string object
- `str_clear()`: Clear the content of the string
- `str_alloc(size)`: Allocate a string with specific initial capacity
- `str_realloc(ptr, size)`: Reallocate the internal buffer
- `str_copy(dest, src, max)`: Copy content from one string to another
- `str_mov(dest, src)`: Move content from source to destination (consumes source)

#### String Operations
- `str_add(self, str)`: Append a C-string
- `str_set(self, str)`: Set content to a C-string
- `str_assign_n(self, str, n)`: Assign first N characters of a C-string
- `str_get_data()`: Get a copy of the string data (caller must free)
- `str_get_size()`: Get current length
- `str_get_capacity()`: Get current capacity
- `str_is_empty()`: Check if string is empty

#### String Manipulation
- `str_to_upper()`: Convert to uppercase
- `str_to_lower()`: Convert to lowercase
- `str_to_title_case()`: Convert to title case
- `str_reverse()`: Reverse string in place
- `str_remove_word(self, word)`: Remove first occurrence of a substring
- `str_replace_word(self, old, new)`: Replace first occurrence of a substring

#### Advanced Operations
- `str_insert(self, pos, str)`: Insert string at position
- `str_find(self, substr, pos)`: Find substring starting from position
- `str_starts_with(self, prefix)`: Check if starts with prefix
- `str_ends_with(self, suffix)`: Check if ends with suffix

#### Formatting
- `str_pad_left(self, len, char)`: Pad with character on the left
- `str_pad_right(self, len, char)`: Pad with character on the right
- `str_trim()`: Trim whitespace from both ends
- `str_trim_left()`: Trim leading whitespace
- `str_trim_right()`: Trim trailing whitespace

#### Input/Output
- `str_read_line(self, stream)`: Read a line from a FILE stream
- `str_read_word(self, stream)`: Read a word from a FILE stream
- `str_print()`: Print string to stdout
- `get_dyn_input(max)`: Read dynamic input from stdin

### 🧪 Running Tests

The project uses a custom test suite. To run the tests:

```bash
mkdir build
cd build
cmake ..
make
./tests/test_strutil
```

### 🤝 Contributing

1. Fork the repository
2. Create your feature branch
3. Commit your changes
4. Push to the branch
5. Open a Pull Request

## 📝 License

This project is licensed under the Unlicense - see the [LICENSE](LICENSE) file for details.
