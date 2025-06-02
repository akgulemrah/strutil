/*
 * strutil.h - Thread-safe dynamic string library.
 *
 * This library provides a thread-safe, dynamically resizable string data type
 * along with a comprehensive set of string manipulation functions.
 * It is designed to simplify string operations in multithreaded applications,
 * offering functionality for concatenation, insertion, deletion, case conversion,
 * trimming, and more.
 *
 * For detailed usage instructions and examples, please refer to the README.md file.
 */

#ifndef STRUTIL_H
#define STRUTIL_H

/* Headers */
#include <stdio.h>
#include <stdbool.h>

#define STR_NPOS ((size_t)-1)

// Macros for convenient string object management.
// str_free: Safely frees a string object and sets the pointer to NULL.
#define str_free(str_ptr) _str_free(&(str_ptr))
// str_realloc: Reallocates the internal buffer of a string object.
// If new_size is 0, it behaves like str_free, fully deallocating the object.
#define str_realloc(str_ptr, new_size) _str_realloc(&(str_ptr), (new_size))

/* Debug mode configuration */
#ifndef STRDEBUGMODE
    #define STRDEBUGMODE 0
#endif

/* String flags enum definition */
typedef enum StrFlags {
    STR_FLAG_NONE       = 0,
    STR_FLAG_DYNAMIC    = (1 << 0),  /* Dynamically allocated string structure */
    STR_FLAG_MUTEX_INIT = (1 << 1),  /* Mutex has been successfully initialized */
    STR_FLAG_READONLY   = (1 << 2),  /* String content is read-only (cannot be modified) */
    STR_FLAG_FIXED_SIZE = (1 << 3),  /* String has fixed maximum capacity (cannot grow) */
    STR_FLAG_TEMPORARY  = (1 << 4),  /* Indicates a temporary string, likely to be freed soon */
    STR_FLAG_MODIFIED   = (1 << 5),  /* String content has been modified since last operation */
    STR_FLAG_LOCKED     = (1 << 6),  /* String's mutex is currently locked (internal state) */
    STR_FLAG_ERROR      = (1 << 7)   /* String is in an error state */
} StrFlags;

/* Atomic flag operations (using GCC/Clang intrinsics for atomic safety) */
#define SET_FLAG(flags, FLG) (__atomic_or_fetch(&(flags), (FLG), __ATOMIC_SEQ_CST))
#define CLEAR_FLAG(flags, FLG) (__atomic_and_fetch(&(flags), ~(FLG), __ATOMIC_SEQ_CST))
#define CHECK_FLAG(flags, FLG) (__atomic_load_n(&(flags), __ATOMIC_SEQ_CST) & (FLG))
#define TOGGLE_FLAG(flags, FLG) (__atomic_xor_fetch(&(flags), (FLG), __ATOMIC_SEQ_CST))

static const size_t STR_MAX_STRING_SIZE = (size_t)(32 << 20); // Maximum string size limit (32MB)
struct str; // Forward declaration of the opaque string structure

/* Error codes */
typedef enum Str_err_t {
    STR_OK = 0,                      // Success
    STR_NULL,                        // NULL pointer encountered
    STR_INVALID,                     // Invalid argument provided
    STR_NOMEM,                       // Memory allocation failed
    STR_COPY_FAIL,                   // String copy operation failed
    STR_MAXSIZE,                     // Maximum size exceeded or fixed size capacity constraint violated
    STR_ALLOC,                       // General allocation error (less specific than NOMEM)
    STR_EMPTY,                       // String is empty or operation requires non-empty string
    STR_FAIL,                        // General operation failure
    STR_OVERFLOW,                    // Buffer or size overflow condition
    STR_LOCK,                        // Mutex locking or unlocking error
    STR_STREAM_ERR                   // File stream I/O error
} Str_err_t;

/* Function declarations */
/* Core Functions */
struct str *str_init(void);                   // Initializes a new string object
void _str_free(struct str **self);            // Internal: frees a string object (used by str_free macro)
void str_clear(struct str *self);             // Clears the content of the string

/* Memory Management */
struct str* str_alloc(size_t size);           // Allocates a new string object with a specified initial buffer size
Str_err_t _str_realloc(struct str **self, const size_t new_size); // Internal: reallocates buffer (used by str_realloc macro)
Str_err_t str_copy(struct str *dest, const struct str *source, size_t max_len); // Copies content from one 'str' to another, with a max length
Str_err_t str_mov(struct str *dest, struct str *src);           // Moves content from 'src' to 'dest', 'src' is consumed

/* String Operations */
Str_err_t str_add(struct str *self, const char *source);            // Appends a C-style string to the current string
Str_err_t str_set(struct str *self, const char *source);            // Sets the string content to a new C-style string (overwrites)
Str_err_t str_assign_n(struct str *self, const char *source, size_t count); // Assigns a C-style string, copying up to 'count' characters

const char* str_get_data(const struct str *self);   // Returns the internal C-string buffer
size_t str_get_size(const struct str *self);        // Returns the current length of the string
size_t str_get_capacity(const struct str *self);    // Returns the allocated capacity of the string buffer
bool str_is_empty(const struct str *self);          // Checks if the string is empty

/* String Manipulation */
Str_err_t str_to_upper(struct str *self);           // Converts string to uppercase
Str_err_t str_to_lower(struct str *self);           // Converts string to lowercase
Str_err_t str_to_title_case(struct str *self);      // Converts string to title case
Str_err_t str_reverse(struct str *self);            // Reverses the string in place
Str_err_t str_remove_word(struct str *self, const char *needle); // Removes the first occurrence of a substring
Str_err_t str_replace_word(struct str *self, const char *old_word, const char *new_word); // Replaces first occurrence of old_word with new_word

/* Advanced String Operations */
Str_err_t str_insert(struct str *self, const size_t pos, const char *str_to_insert); // Inserts a string at a specified position
size_t str_find(const struct str *self, const char *substr, size_t pos);             // Finds the first occurrence of a substring
bool str_starts_with(const struct str *self, const char *prefix);    // Checks if the string starts with a prefix
bool str_ends_with(const struct str *self, const char *suffix);      // Checks if the string ends with a suffix

/* String Formatting */
Str_err_t str_pad_left(struct str *self, size_t total_length, char pad_char);  // Left-pads the string
Str_err_t str_pad_right(struct str *self, size_t total_length, char pad_char); // Right-pads the string
Str_err_t str_trim(struct str *self);              // Trims leading and trailing whitespace
Str_err_t str_trim_left(struct str *self);         // Trims leading whitespace
Str_err_t str_trim_right(struct str *self);        // Trims trailing whitespace

/* Input/Output */
Str_err_t str_read_line(struct str *self, FILE *stream); // Reads a line from a FILE stream and sets string content
Str_err_t str_read_word(struct str *self, FILE *stream); // Reads a word from a FILE stream and appends it to string

void str_print(const struct str *self);                    // Prints the string to stdout
char* get_dyn_input(size_t max_str_size);                   // Reads dynamic input from stdin into a new C-string

/* Error Handling */
void str_check_err(const Str_err_t err, const char *optional_message); // Prints an error message for Str_err_t codes

#endif /* STRUTIL_H */
