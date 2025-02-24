/*
 * strutil.h - Thread-safe dynamic string library.
 *
 * This library provides a thread-safe, dynamically resizable string data type
 * along with a comprehensive set of string manipulation functions.
 * It is designed to simplify string operations in multi-threaded applications,
 * offering functionality for concatenation, insertion, deletion, case conversion,
 * trimming, and more.
 *
 * For detailed usage instructions and examples, please refer to the README.md file.
 */

#ifndef STRUTIL_H
#define STRUTIL_H

/* Headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <stdbool.h>
#include <pthread.h>
#include <limits.h>

/* Compiler-specific attributes */
#if defined(__GNUC__) || defined(__clang__)
	#define STR_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
	#define STR_NONNULL_ALL __attribute__((nonnull))
	#define STR_NONNULL(args) __attribute__((nonnull args))
	#define STR_PURE __attribute__((pure))
	#define STR_MALLOC __attribute__((malloc))
	#define STR_CONST __attribute__((const))
	#define STR_DESTRUCTOR __attribute__((destructor))
#else
	#define STR_WARN_UNUSED_RESULT
	#define STR_NONNULL_ALL
	#define STR_NONNULL(args)
	#define STR_PURE
	#define STR_MALLOC
	#define STR_CONST
	#define STR_DESTRUCTOR
#endif

/* Debug mode configuration */
#ifndef STRDEBUGMODE
	#define STRDEBUGMODE 0
#endif		

#define STR_NPOS ((size_t)-1)

#define str_free(str_ptr) _str_free(&(str_ptr))

static const size_t STR_MAX_STRING_SIZE = (size_t)(32 << 20); //32MB
struct str;

/* Error codes */
typedef enum Str_err_t {
	STR_OK = 0,                     // Success
	STR_NULL,		        // NULL pointer
	STR_INVALID,	   	        // Invalid argument
	STR_NOMEM,		        // Memory allocation failed
	STR_ERRCPY,		        // Copy operation failed
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
Str_err_t str_realloc(struct str *self, size_t new_size);
Str_err_t str_grow(struct str *self, size_t min_capacity);
Str_err_t str_copy(struct str *dest, const struct str *source, size_t max_len);

/* String Operations */
Str_err_t str_add(struct str *self, const char *_data);
Str_err_t str_set(struct str *self, const char *arr);
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
Str_err_t str_insert(struct str *self, size_t pos, const char *str);
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
int str_check_err(Str_err_t Error, const char *user_message);

#endif /* STRUTIL_H */
