/*
 * strutil.h - Thread-safe dynamic string library.
 *
 * This file is part of the strutil project.
 *
 * Copyright (C) 2025  Your Name <your.email@example.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library provides a thread-safe, dynamically resizable string data type
 * along with a comprehensive set of string manipulation functions.
 * It is designed to simplify string operations in multi-threaded applications,
 * offering functionality for concatenation, insertion, deletion, case conversion,
 * trimming, and more.
 *
 * For detailed usage instructions and examples, please refer to the README.md file.
 *
 * Maintainer: Your Name <your.email@example.com>
 *
 * ---------------------------------------------------------------------------
 *
 * "Talk is cheap. Show me the code." - Linus Torvalds
 *
 */

#ifndef STRUTIL_H
#define STRUTIL_H

/* Headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <pthread.h>
#include <limits.h>

#ifndef PTHREAD_MUTEX_RECURSIVE
#  ifdef PTHREAD_MUTEX_RECURSIVE_NP
#    define PTHREAD_MUTEX_RECURSIVE PTHREAD_MUTEX_RECURSIVE_NP
#  else
#    define PTHREAD_MUTEX_RECURSIVE 1
#    pragma message ("Warning: PTHREAD_MUTEX_RECURSIVE not defined; using fallback value")
#  endif
#endif



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

static const size_t STR_MAX_STRING_SIZE = (size_t)(512 << 20); //512MB

static const size_t CHUNK_SIZE = 4096;	// Page size
static const size_t MIN_CAPACITY = 16;	// Minimum initial capacity

/* Thread safety configuration */
static pthread_mutexattr_t mutex_attr;
static bool mutex_attr_initialized = false;

/* String structure */
typedef struct Str {
	char *data;		        // String data
	size_t length;		        // Current string length
	size_t capacity;		// Allocated capacity
	bool is_dynamic;		// Dynamic allocation flag
	pthread_mutex_t lock;           // Thread safety mutex
} str;

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
	STR_LOCK		        // Mutex lock error
} Str_err_t;

/* Function declarations */
/* Core Functions */
STR_WARN_UNUSED_RESULT STR_MALLOC str* str_init(void);
STR_NONNULL_ALL void str_free(str *self);
STR_NONNULL_ALL void str_clear(str *self);
STR_NONNULL_ALL Str_err_t str_lock(str *self);
STR_NONNULL_ALL Str_err_t str_unlock(str *self);
static void cleanup_mutex_attr(void) STR_DESTRUCTOR;
static void cleanup_mutex_attr(void) STR_DESTRUCTOR;


/* Memory Management */
STR_WARN_UNUSED_RESULT STR_MALLOC str* str_alloc(size_t size);
STR_WARN_UNUSED_RESULT Str_err_t str_realloc(str *self, size_t new_size);
STR_NONNULL_ALL Str_err_t str_grow(str *self, size_t min_capacity);
STR_NONNULL_ALL char* str_copy(char *dest, const char *source, size_t max_len);

/* String Operations */
STR_NONNULL_ALL Str_err_t str_add(str *self, const char *_data);
STR_NONNULL_ALL Str_err_t str_set(str *self, const char *arr);
STR_NONNULL_ALL STR_PURE const char* str_get(const str *self);
STR_NONNULL_ALL STR_PURE const char* str_get_data(const str *self);
STR_NONNULL_ALL STR_PURE size_t str_get_size(const str *self);
STR_NONNULL_ALL STR_PURE bool str_is_empty(const str *self);

/* String Manipulation */
STR_NONNULL_ALL Str_err_t str_to_upper(str *self);
STR_NONNULL_ALL Str_err_t str_to_lower(str *self);
STR_NONNULL_ALL Str_err_t str_to_title_case(str *self);
STR_NONNULL_ALL Str_err_t str_reverse(str *self);
STR_NONNULL_ALL Str_err_t str_rem_word(str *self, const char *needle);
STR_NONNULL_ALL Str_err_t str_swap_word(str *self, const char *old_word, const char *new_word);

/* Advanced String Operations */
STR_NONNULL_ALL Str_err_t str_substr(str *self, str *result, size_t pos, size_t len);
STR_NONNULL_ALL Str_err_t str_insert(str *self, size_t pos, const char *str);
STR_NONNULL_ALL Str_err_t str_erase(str *self, size_t pos, size_t len);
STR_NONNULL_ALL Str_err_t str_replace(str *self, size_t pos, size_t len, const char *str);
STR_NONNULL_ALL int str_compare(const str *self, const char *other);
STR_NONNULL_ALL size_t str_find(const str *self, const char *substr, size_t pos);
STR_NONNULL_ALL size_t str_find_last(const str *self, const char *substr, size_t pos);
STR_NONNULL_ALL bool str_starts_with(const str *self, const char *prefix);
STR_NONNULL_ALL bool str_ends_with(const str *self, const char *suffix);

/* String Formatting */
STR_NONNULL_ALL Str_err_t str_pad_left(str *self, size_t total_length, char pad_char);
STR_NONNULL_ALL Str_err_t str_pad_right(str *self, size_t total_length, char pad_char);
STR_NONNULL_ALL Str_err_t str_trim(str *self);
STR_NONNULL_ALL Str_err_t str_trim_left(str *self);
STR_NONNULL_ALL Str_err_t str_trim_right(str *self);

/* Input/Output */
STR_NONNULL_ALL Str_err_t str_input(str *self);
STR_NONNULL_ALL Str_err_t str_add_input(str *self);
STR_NONNULL_ALL void str_print(const str *self);
STR_NONNULL_ALL char* get_dyn_input(size_t max_str_size);

/* Error Handling */
STR_NONNULL_ALL int str_check_err(Str_err_t Error, const char *user_message);

#endif /* STRUTIL_H */
