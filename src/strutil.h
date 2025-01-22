/*
 * strutil.h - Thread-Safe String Utility Library
 *
 * A comprehensive string manipulation library providing thread-safe operations
 * with dynamic memory management. Features optimized string handling with
 * safety checks and efficient memory usage.
 *
 * Key Features:
 * - Thread-safe operations with mutex protection
 * - Dynamic memory management with safety checks
 * - Optimized string manipulations
 * - Buffer overflow protection
 * - Power-of-2 memory growth
 * - Configurable size limits
 *
 * Core Functions:
 * Memory Management:
 * - str_init(): Initialize string structure
 * - str_free(): Clean up resources
 * - str_clear(): Clear string content
 * - str_alloc(): Safe memory allocation
 * - str_realloc(): Safe memory reallocation
 *
 * String Operations:
 * - str_add(): Append string
 * - str_set(): Set string content
 * - str_get(): Get string content
 * - str_get_size(): Get string length
 * - str_is_empty(): Check if empty
 *
 * String Manipulation:
 * - str_to_upper(): Convert to uppercase
 * - str_to_lower(): Convert to lowercase
 * - str_to_title_case(): Convert to title case
 * - str_reverse(): Reverse string
 * - str_rem_word(): Remove word
 * - str_swap_word(): Replace word
 *
 * Input/Output:
 * - str_input(): Read from stdin
 * - str_add_input(): Append from stdin
 * - str_print(): Write to stdout
 * - get_dyn_input(): Dynamic input reading
 *
 * Safety Features:
 * - Mutex-based thread safety
 * - NULL pointer checks
 * - Buffer overflow prevention
 * - Memory leak prevention
 * - Size limit enforcement
 * - Error reporting system
 *
 * Author: Emrah Akgül
 * Created: February 2024
 * License: unlic
 *
 * Copyright (c) 2024 Emrah Akgül
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 */

#ifndef STRUTIL_H
#define STRUTIL_H

/*    HEADERS    */
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

#if defined(__GNUC__) || defined(__clang__)
    #define STR_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
    #define STR_NONNULL __attribute__((nonnull))
    #define STR_PURE __attribute__((pure))
#else
    #define STR_WARN_UNUSED_RESULT
    #define STR_NONNULL
    #define STR_PURE
#endif

#define STRDEBUGMODE 1

// Safe maximum string size
#if SIZE_MAX > UINT32_MAX
    static const size_t MAX_STRING_SIZE = (1ULL << 31) - 1;  // 2GB limit
#else
    static const size_t MAX_STRING_SIZE = (1UL << 30) - 1;   // 1GB limit
#endif

// Optimized chunk size for memory operations
static const size_t CHUNK_SIZE = 4096;  // Page size

typedef struct Str {
	char	*data;
	size_t length;  // Mevcut uzunluk
	size_t capacity;  // Ayrılan bellek boyutu
	unsigned char is_dynamic;
	pthread_mutex_t lock;
} str;

typedef enum Str_err_t{
	STR_OK = 0,
	STR_NULL,
	STR_INVALID,
	STR_NOMEM,
	STR_ERRCPY,
	STR_MAXSIZE,
	STR_ALLOC,
	STR_EMPTY,
	STR_FAIL,
	STR_OVERFLOW
}Str_err_t;

/* Function Declarations */
/* Core Functions */
str *str_init(void);
void str_free(str *self);
void str_clear(str *self);

/* String Operations */
Str_err_t str_add(str *self, const char *_data);
Str_err_t str_set(str *self, const char *arr);
const char *str_get(const str *self);
const char *str_get_data(const str *self);
size_t str_get_size(const str *self);
bool str_is_empty(str *self);

/* String Manipulation */
Str_err_t str_to_upper(str *self);
Str_err_t str_to_lower(str *self);
Str_err_t str_to_title_case(str *self);
Str_err_t str_reverse(str *self);
Str_err_t str_rem_word(str *self, const char *needle);
Str_err_t str_swap_word(str *self, const char *old_word, const char *new_word);
Str_err_t str_pop_back(str *self, char sep);

/* Input/Output */
Str_err_t str_input(str *self);
Str_err_t str_add_input(str *self);
void str_print(str *self);
char* get_dyn_input(size_t max_str_size);

/* Helper Functions */
static char *str_copy(char *dest, const char *source, size_t max_len);
static Str_err_t str_concat(char *dest, const char *source, size_t max_len);
static Str_err_t str_nconcat(char *dest, const char *src, size_t n);
static Str_err_t str_ncopy(char *dest, const char *src, size_t n);
static size_t str_length(const char *s);
int str_check_err(Str_err_t err, const char *msg);

/* Memory Management Helpers */
static inline size_t str_next_power_2(size_t n);
static Str_err_t str_grow(str *self, size_t min_capacity);
static inline void* STR_WARN_UNUSED_RESULT str_alloc(size_t size);
static inline void* STR_WARN_UNUSED_RESULT str_realloc(void *ptr, size_t new_size);

/* Function Implementations */
/*
 * str_init - Initialize a new string structure
 * @return: Pointer to new string structure or NULL on failure
 *
 * Allocates memory and initializes mutex. Caller must free memory.
 */
str *str_init(void)
{
	str *tmp = (str *)str_alloc(sizeof(str));
	if (!tmp)
		return NULL;
		
	tmp->data = NULL;
	tmp->length = 0;
	tmp->capacity = 0;
	tmp->is_dynamic = 1;
	
	if (pthread_mutex_init(&tmp->lock, NULL) != 0) {
		free(tmp);
		return NULL;
	}
	
	return tmp;
}


/*
 * str_add - Append string to existing string structure
 * @self: String structure
 * @_data: String to append
 * @return: STR_OK on success, error code on failure
 *
 * Thread-safe string append operation. Reallocates memory if needed.
 */
Str_err_t str_add(str *self, const char *_data)
{
	if (!self || !_data)
		return STR_NULL;

	pthread_mutex_lock(&self->lock);
	size_t size = str_length(_data);

	if (self->data) {
		size += str_length(self->data);

		char *p = (char *)realloc(self->data, (size + 1));
		if (!p) {
			pthread_mutex_unlock(&self->lock);
			return STR_NOMEM;
		}

		self->data = p;	
		str_concat(self->data, _data, size);
	} else {
		self->data = (char *)malloc((size + 1));
		if (!self->data) {
			pthread_mutex_unlock(&self->lock);
			return STR_NOMEM;
		}

		if (str_copy(self->data, _data, size) != self->data) {
			free(self->data);
			self->data = NULL;
			pthread_mutex_unlock(&self->lock);
			return STR_ERRCPY;
		}
	}

	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}


/*
 * str_input - Read string from standard input
 * @self: String structure
 * @return: STR_OK on success, error code on failure
 *
 * Thread-safe stdin read operation.
 */
Str_err_t str_input(str *self)
{
	if (!self) {
		return STR_NULL;
	} else if(self->data != NULL) {
		return STR_INVALID;
	}
	
	pthread_mutex_lock(&self->lock);
	self->data = get_dyn_input(MAX_STRING_SIZE);
	pthread_mutex_unlock(&self->lock);
	return (self->data ? STR_OK : STR_MAXSIZE);
}


/*
 * str_print - Print string content
 * @self: String structure
 *
 * Thread-safe stdout write operation.
 */
void str_print(str *self)
{
	if (self) {
		pthread_mutex_lock(&self->lock);
		if (self->data) {
			printf("%s", self->data);
			fflush(stdout);
		}
		pthread_mutex_unlock(&self->lock);
	}
}


/*
 * str_free - Clean up string structure
 * @self: String structure
 *
 * Frees memory and destroys mutex. Handles dynamic structures.
 */
void str_free(str *self)
{
	if (!self) return;
	
	pthread_mutex_lock(&self->lock);
	if (self->data) {
		memset(self->data, 0, self->capacity);  // Güvenli temizlik
		free(self->data);
		self->data = NULL;
		self->length = 0;
		self->capacity = 0;
	}
	pthread_mutex_unlock(&self->lock);
	
	if (self->is_dynamic) {
		pthread_mutex_destroy(&self->lock);
		memset(self, 0, sizeof(str));  // Güvenli temizlik
		free(self);
	}
}


/*
 * str_pop_back - Remove trailing content after separator
 * @self: String structure
 * @sep: Separator character
 * @return: STR_OK on success, error code on failure
 *
 * Removes content after last separator and shrinks memory.
 */
Str_err_t str_pop_back(str *self, char sep)
{
	if (!self) {
		return STR_NULL;
	} else if (!self->data || !str_length(self->data)) {
		return STR_EMPTY;
	}

	pthread_mutex_lock(&self->lock);

	char *p = strrchr(self->data, sep);
	if (!p) {
		pthread_mutex_unlock(&self->lock);
		return STR_FAIL;
	}

	p++;
	*p = '\0';

	char *self_data_ptr = (char *)realloc(self->data, str_length(self->data) + 1); // Trim memory
	if (!self_data_ptr) {
		pthread_mutex_unlock(&self->lock);
		return STR_ALLOC;
	}
	
	self->data = self_data_ptr;
	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}


/*
 * str_get_size - Get string length
 * @self: String structure
 * @return: String length or 0 on error
 */
size_t  str_get_size(const str *self)
{
	if (self) {
    		return (self->data ? str_length(self->data) : 0);
	} else {
		return 0;
	}
}


/*
 * str_clear - Clear string content
 * @self: String structure
 *
 * Frees content but keeps structure.
 */
void str_clear(str *self)
{
	if (!self) return;
	
	pthread_mutex_lock(&self->lock);
	if (self->data) {
		memset(self->data, 0, self->capacity);  // Güvenli temizlik
		free(self->data);
		self->data = NULL;
		self->length = 0;
		self->capacity = 0;
	}
	pthread_mutex_unlock(&self->lock);
}


/*
 * str_rem_word - Remove word from string
 * @self: String structure
 * @needle: Word to remove
 * @return: STR_OK on success, error code on failure
 *
 * Thread-safe word removal operation.
 */
Str_err_t str_rem_word(str *self, const char *needle)
{
	if (!self || !needle) {
		return STR_NULL;
	} else if (!self->data) {
		return STR_EMPTY;
	} 
        
	pthread_mutex_lock(&self->lock);
        size_t self_data_size = str_length(self->data);
        size_t needle_size = str_length(needle);
        
        if (needle_size > self_data_size) {
		pthread_mutex_unlock(&self->lock);
        	return STR_FAIL;
	}
            
        char *L = NULL;
        L = strstr(self->data, needle);
        if(!L) {
		pthread_mutex_unlock(&self->lock);
        	return STR_FAIL;
	}

    	memmove(L, L + needle_size, self_data_size - (L - self->data) - needle_size + 1);
	self->data[self_data_size - needle_size] = '\0';
        
        char *buf = (char*)realloc(self->data, 
                ((self_data_size - needle_size)) +1);
        
	if (!buf) {	// realloc başarısız oldu; ancak kelime diziden kaldırıldı ve sonuna NULL eklendi
		pthread_mutex_unlock(&self->lock);
		return STR_ALLOC;
	}

        if (buf)
        	self->data = buf;

	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}


/*
 * str_get_data - Get string content
 * @self: String structure
 * @return: String content or NULL on error
 *
 * Returns read-only content.
 */
const char *str_get_data(const str *self)
{
	if (self) {
		if (self->data) {
    			return (const char *)self->data;
		}
	}
	return NULL;
}


/*
 * str_add_input - Append input to existing string
 * @self: String structure
 * @return: STR_OK on success, error code on failure
 *
 * Reads from stdin and appends to existing content.
 */
Str_err_t str_add_input(str *self)
{
	if (!self) {
		return STR_NULL;
	}

	pthread_mutex_lock(&self->lock);

	if (!self->data) {
		self->data = get_dyn_input(MAX_STRING_SIZE);
		
		if (self->data == NULL) {
			pthread_mutex_unlock(&self->lock);
			return STR_MAXSIZE;
		}
		pthread_mutex_unlock(&self->lock);
		return STR_OK;
	}

	size_t self_data_size = str_length(self->data);

	char *buf = get_dyn_input(MAX_STRING_SIZE - self_data_size);
	if (!buf) {
		pthread_mutex_unlock(&self->lock);
		return STR_ALLOC;
	}

	char *new_data = (char *)realloc(self->data, (self_data_size + str_length(buf) + 1));
	if (!new_data) {
		free(buf);
		pthread_mutex_unlock(&self->lock);
		return STR_ALLOC;
	} else {
		self->data = new_data;
	}

	Str_err_t concat_res = str_nconcat(self->data, buf, strlen(buf));
	if (concat_res != STR_OK) {
		char *res = (char *)realloc(self->data, self_data_size + 1);
		if (res) {
			self->data = res;
		}
		free(buf);
		pthread_mutex_unlock(&self->lock);
		return concat_res;
	}

	free(buf);
	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}


/*
 * str_to_upper - Convert string to uppercase
 * @self: String structure
 * @return: STR_OK on success, error code on failure
 */
Str_err_t str_to_upper(str *self)
{
	if (!self) {
		return STR_NULL;
	} else if (!self->data) {
		return STR_EMPTY;
	}
	pthread_mutex_lock(&self->lock);

	char *p = self->data;
	while (*p) {
		if ((*p >= 'a') && (*p <= 'z'))
			*p &= ~(1 << 5);
		p++;
	}
	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}


/*
 * str_to_lower - Convert string to lowercase
 * @self: String structure
 * @return: STR_OK on success, error code on failure
 */
Str_err_t str_to_lower(str *self)
{
	if (!self)
		return STR_NULL;
	if (!self->data)
		return STR_EMPTY;
	
	pthread_mutex_lock(&self->lock);
	char *p = self->data;
	while (*p) {
		if ((*p >= 'A') && (*p <= 'Z'))
			*p |= (1 << 5); // yes, faster than tolower :/
		p++;
	}

	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}


/*
 * get_dyn_input - Read input with dynamic memory
 * @max_str_size: Maximum string size
 * @return: Read string or NULL on failure
 *
 * Reads from stdin in chunks, growing memory as needed.
 */
char* get_dyn_input(size_t max_str_size)
{
	const int CHUNK_SIZE = 10;
	char* buffer = (char *)malloc(CHUNK_SIZE);
	if (!buffer) 
		return NULL;

	size_t current_size = CHUNK_SIZE; // Size of available memory.
	size_t length = 0; // Length of current string

	char *tmp = NULL;
	int c;
	while ((c = getchar()) != EOF && c != '\n') {
		if (length + 1 >= current_size) { // Expand memory
			current_size += CHUNK_SIZE;			

			tmp = (char *)realloc(buffer, current_size);
			if (!tmp) {
				free(buffer);
				return NULL;
			}
			buffer = tmp;
		}

		if (length >= (max_str_size - 1)) {
			free(buffer);
			return NULL;
		}

		buffer[length++] = (char)c;
		buffer[length] = '\0'; // End the series
	}

	// Finally release the extra memory
	tmp = (char *)realloc(buffer, length + 1);
	if (!tmp) {
		free(buffer);
		return NULL;
	}
	buffer = tmp;
	return buffer;
}


/*
 * str_swap_word - Replace word occurrences
 * @self: String structure
 * @old_word: Word to replace
 * @new_word: Replacement word
 * @return: STR_OK on success, error code on failure
 */
Str_err_t str_swap_word(str *self, const char *old_word, const char *new_word)
{
	if (!self || !old_word || !new_word)
		return STR_NULL;
	else if (!self->data)
		return STR_EMPTY;

	pthread_mutex_lock(&self->lock);

	size_t self_data_size = str_length(self->data);
	size_t old_word_size = str_length(old_word);
	size_t new_word_size = str_length(new_word);
	size_t result_length = 0;
	char *old_word_pos = NULL;
	char *buf = NULL;

	old_word_pos = strstr(self->data, old_word);
	if (!old_word_pos) {
		pthread_mutex_unlock(&self->lock);
		return STR_FAIL;
	}

	result_length = self_data_size - old_word_size + new_word_size;
	buf = (char *)malloc(sizeof(char) * (result_length + 1)); // +1 for null terminator
	if (!buf) {
		pthread_mutex_unlock(&self->lock);
		return STR_ALLOC;
	}

	// Copy everything before word1
	Str_err_t err =  str_ncopy(buf, self->data, old_word_pos - self->data);
	if (err != STR_OK) {
#if STRDEBUGMODE == ON
		str_check_err(err, "str_swap_word --> Str_err_t err =  str_ncopy(buf, self->data, old_word_pos - self->data)");
#endif
		free(buf);
		return err;
	}
	buf[old_word_pos - self->data] = '\0'; // Null terminate the buffer

	// Copy word2
	err = str_concat(buf, new_word, result_length);
	if (err != STR_OK) {
#if STRDEBUGMODE == ON
		str_check_err(err, "str_swap_word --> Str_err_t err = str_concat(buf, new_word, result_length)");
#endif
		free(buf);
		return err;
	}

	// Copy everything after word1
	err = str_concat(buf, old_word_pos + old_word_size, result_length - (old_word_pos - self->data) - old_word_size);
	if (err != STR_OK) {
#if STRDEBUGMODE == ON
		str_check_err(err, "str_swap_word --> Str_err_t err = str_concat(buf, old_word_pos + old_word_size, result_length - (old_word_pos - self->data) - old_word_size)");
#endif
		free(buf);
		return err;
	}

	free(self->data);
	self->data = buf;

	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}


/*
 * str_to_title_case - Capitalize first letter of each word
 * @self: String structure
 * @return: STR_OK on success, error code on failure
 */
Str_err_t str_to_title_case(str *self)
{
	if (!self)
		return STR_NULL;
	if (!self->data || !str_length(self->data))
		return STR_EMPTY;
	
	pthread_mutex_lock(&self->lock);

	char *p = self->data;
	short flag = 1;

	while (*p) {
		if (flag && (*p >= 'a') && (*p <= 'z')) {
			*p &= ~(1 << 5);
			flag = 0;
		} else if (*p == ' ') {
			flag = 1;
		}
		p++;
	}

	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}


/*
 * str_reverse - Reverse string content
 * @self: String structure
 * @return: STR_OK on success, error code on failure
 */
Str_err_t str_reverse(str *self)
{
	if (!self)
		return STR_NULL;
	else if (!self->data)
		return STR_NULL;
	else if (!str_length(self->data))
		return STR_EMPTY;
    
	pthread_mutex_lock(&self->lock);
	char buf = 0;
	size_t head = 0;
	size_t tail = str_length(self->data) - 1;

	while (head < tail) {
		buf = self->data[head];
		self->data[head] = self->data[tail];
		self->data[tail] = buf;
		head++;
		tail--;
	}

	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}


/*
 * str_is_empty - Check if string is empty
 * @self: String structure
 * @return: true if empty, false otherwise
 */
bool str_is_empty(str *self)
{
	if (self){
		if (!self->data){
			return true;
		} else if (!str_length(self->data)) {
			return true;
		}
	}
	return false;
}

Str_err_t str_set(str *self, const char *arr)
{
	if (!self )
		return STR_NULL;
	else if (self->data)
		return STR_INVALID; // str->data must be empty
	
	pthread_mutex_lock(&self->lock);

	size_t arr_size = str_length(arr);
	if (arr_size == 0) {
		pthread_mutex_unlock(&self->lock);
		return STR_INVALID;
	}
	
	self->data = (char *)malloc(sizeof(arr_size));
	if (!self->data) {
		pthread_mutex_unlock(&self->lock);
		return STR_ALLOC;
	}

	str_copy(self->data, arr, arr_size);
	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}

const char *str_get(const str *self)
{
	if (!self)
		return NULL;
	if (!self->data)
		return NULL;
	return (const char *)self->data;
}


static char *str_copy(char *dest, const char *source, size_t max_len)
{
	if (!dest || !source || max_len == 0)
		return NULL;
        
	char *d = dest;
	const char *s = source;
	size_t n = max_len;
    
	while (n > 0 && *s) {
		*d++ = *s++;
		n--;
	}
	*d = '\0';
    
	return dest;
}


static Str_err_t str_concat(char *dest, const char *source, size_t max_len)
{
	if (!dest || !source)
		return STR_NULL;
        
	size_t dest_len = str_length(dest);
	if (dest_len >= max_len)
		return STR_OVERFLOW;
        
	size_t remaining = max_len - dest_len;
	char *d = dest + dest_len;
    
	while (remaining > 1 && *source) {
		*d++ = *source++;
		remaining--;
	}
	*d = '\0';
    
	return STR_OK;
}


static Str_err_t str_nconcat(char *dest, const char *src, size_t n)
{
	if (!dest || !src)
		return STR_NULL;
	else if ( n < 1)
		return STR_INVALID;
	
	size_t src_size = str_length(src);
	if (n > src_size)
		return STR_INVALID;
	
	while (*dest)
		dest++;

	while (n) {
		*dest++ = *src++;
		n--;
	}
	*dest = '\0';

	return STR_OK;
}

static Str_err_t str_ncopy(char *dest, const char *src, size_t n)
{
	if (!dest || !src)
		return STR_NULL;
	else if (n < 1)
		return STR_INVALID;
	
	while (n) {
		*dest++ = *src++;
		n--;
	}
	*dest = '\0';
	return STR_OK;
}


static size_t str_length(const char *s)
{
	if (!s)
		return 0;

	size_t length = 0;
	while(*s++)
		length++;

	return length;
}

/*
 * str_check_err - Format and print error messages
 * @Error: Error code to check
 * @user_message: Optional error description
 * @return: Always returns 0
 *
 * Simplified error reporting with optional context.
 */
int str_check_err(Str_err_t Error, const char *user_message)
{
    static const char *err_msgs[] = {
        [STR_OK]      = "OK",
        [STR_NULL]    = "NULL pointer",
        [STR_INVALID] = "Invalid argument",
        [STR_NOMEM]   = "No memory",
        [STR_ERRCPY]  = "Copy error",
        [STR_MAXSIZE] = "Max size exceeded",
        [STR_ALLOC]   = "Allocation failed",
        [STR_EMPTY]   = "Empty string",
        [STR_FAIL]    = "Operation failed",
        [STR_OVERFLOW]= "Buffer overflow"
    };

    if (Error < 0 || Error >= sizeof(err_msgs) / sizeof(err_msgs[0])) {
        fprintf(stderr, "Unknown error code: %d\n", Error);
        return 0;
    }

    if (user_message)
        fprintf(stderr, "Error: %s - %s\n", err_msgs[Error], user_message);
    else
        fprintf(stderr, "Error: %s\n", err_msgs[Error]);

    return 0;
}

// Memory growth with power-of-2 sizing
static inline size_t str_next_power_2(size_t n)
{
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    #if SIZE_MAX > UINT32_MAX
    n |= n >> 32;
    #endif
    n++;
    return n;
}

// Safe memory growth
static Str_err_t str_grow(str *self, size_t min_capacity)
{
    if (!self || min_capacity > MAX_STRING_SIZE)
        return STR_INVALID;
        
    size_t new_capacity = self->capacity == 0 ? CHUNK_SIZE : self->capacity;
    while (new_capacity < min_capacity) {
        new_capacity = str_next_power_2(new_capacity + 1);
        if (new_capacity > MAX_STRING_SIZE)
            return STR_MAXSIZE;
    }
    
    char *new_data = (char *)str_realloc(self->data, new_capacity);
    if (!new_data)
        return STR_NOMEM;
        
    self->data = new_data;
    self->capacity = new_capacity;
    return STR_OK;
}

// Optimized memory allocation
static inline void* STR_WARN_UNUSED_RESULT str_alloc(size_t size)
{
    if (size == 0 || size > MAX_STRING_SIZE)
        return NULL;
        
    void *ptr = malloc(size);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

// Safe memory reallocation
static inline void* STR_WARN_UNUSED_RESULT str_realloc(void *ptr, size_t new_size)
{
    if (new_size == 0 || new_size > MAX_STRING_SIZE)
        return NULL;
        
    void *new_ptr = realloc(ptr, new_size);
    if (!new_ptr && ptr)
        free(ptr);  // Free original memory if realloc fails
    return new_ptr;
}

#endif /* STRUTIL_H */
