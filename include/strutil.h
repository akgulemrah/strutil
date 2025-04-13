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

#define str_free(str_ptr) _str_free(&(str_ptr))
#define str_realloc(str_ptr, new_size) _str_realloc(&(str_ptr), (new_size))

/* Debug mode configuration */
#ifndef STRDEBUGMODE
    #define STRDEBUGMODE 0
#endif

/* String flags enum definition */
typedef enum StrFlags {
    STR_FLAG_NONE      = 0,
    STR_FLAG_DYNAMIC   = (1 << 0),  /* Dynamically allocated string */
    STR_FLAG_MUTEX_INIT = (1 << 1), /* Mutex is initialized */
    STR_FLAG_READONLY  = (1 << 2),  /* String is read-only */
    STR_FLAG_FIXED_SIZE = (1 << 3), /* String has fixed size */
    STR_FLAG_TEMPORARY = (1 << 4),  /* Temporary string, will be freed soon */
    STR_FLAG_MODIFIED  = (1 << 5),  /* String has been modified */
    STR_FLAG_LOCKED    = (1 << 6),  /* String is currently locked */
    STR_FLAG_ERROR     = (1 << 7)   /* String is in error state */
} StrFlags;

/* Atomic flag operations */
#define SET_FLAG(flags, FLG) (__atomic_or_fetch(&(flags), (FLG), __ATOMIC_SEQ_CST))
#define CLEAR_FLAG(flags, FLG) (__atomic_and_fetch(&(flags), ~(FLG), __ATOMIC_SEQ_CST))
#define CHECK_FLAG(flags, FLG) (__atomic_load_n(&(flags), __ATOMIC_SEQ_CST) & (FLG))
#define TOGGLE_FLAG(flags, FLG) (__atomic_xor_fetch(&(flags), (FLG), __ATOMIC_SEQ_CST))

static const size_t STR_MAX_STRING_SIZE = (size_t)(32 << 20); //32MB
struct str;

/* Error codes */
typedef enum Str_err_t {
	STR_OK = 0,                     // Success
	STR_NULL,		        // NULL pointer
	STR_INVALID,	   	        // Invalid argument
	STR_NOMEM,		        // Memory allocation failed
	STR_CPY,		        // Copy operation failed
	STR_MAXSIZE,	   	        // Maximum size exceeded
	STR_ALLOC,		        // Allocation error
	STR_EMPTY,		        // Empty string
	STR_FAIL,		        // General failure
	STR_OVERFLOW,	                // Buffer overflow
	STR_LOCK,		        // Mutex lock error
	STR_STREAM			// Stream error
} Str_err_t;

/* Function declarations */
/* Core Functions */
struct str *str_init(void);
void _str_free(struct str **self);
void str_clear(struct str *self);

/* Memory Management */
struct str* str_alloc(size_t size);
Str_err_t _str_realloc(struct str **self, const size_t new_size);
Str_err_t str_grow(struct str *self, size_t min_capacity);
Str_err_t str_copy(struct str *dest, const struct str *source, size_t max_len);
Str_err_t str_mov(struct str *dest, struct str *src);

/* String Operations */
Str_err_t str_add(struct str *self, const char *source);
Str_err_t str_set(struct str *self, const char *source);
Str_err_t str_cpy(struct str *self, const char *source, size_t max_len);
const char* str_get_data(const struct str *self);
size_t str_get_size(const struct str *self);
size_t str_get_capacity(const struct str *self);
bool str_is_empty(const struct str *self);

/* String Manipulation */
Str_err_t str_to_upper(struct str *self);
Str_err_t str_to_lower(struct str *self);
Str_err_t str_to_title_case(struct str *self);
Str_err_t str_reverse(struct str *self);
Str_err_t str_rem_word(struct str *self, const char *needle);
Str_err_t str_swap_word(struct str *self, const char *old_word, const char *new_word);

/* Advanced String Operations */
Str_err_t str_insert(struct str *self, const size_t pos, const char *str);
size_t str_find(const struct str *self, const char *substr, size_t pos);
bool str_starts_with(const struct str *self, const char *prefix);
bool str_ends_with(const struct str *self, const char *suffix);

/* String Formatting */
Str_err_t str_pad_left(struct str *self, size_t total_length, char pad_char);
Str_err_t str_pad_right(struct str *self, size_t total_length, char pad_char);
Str_err_t str_trim(struct str *self);
Str_err_t str_trim_left(struct str *self);
Str_err_t str_trim_right(struct str *self);

/* Input/Output */
Str_err_t str_input(struct str *self, FILE *stream);
Str_err_t str_add_input(struct str *self, FILE *stream);
void str_print(const struct str *self);
char* get_dyn_input(size_t max_str_size);

/* Error Handling */
void str_check_err(const Str_err_t err, const char *optional_message);

#endif /* STRUTIL_H */
