/*
 * strutil.c - Implementation of a thread-safe, dynamic string library.
 *
 * This file implements a thread-safe, dynamically resizable string library.
 * All operations on the string object are protected by a recursive mutex to
 * ensure safe access in multi-threaded environments.
 *
 * "Talk is cheap. Show me the code." - Linus Torvalds
 *
 * Copyright (C) 2025  Your Name <your.email@example.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "strutil.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

/*
 * init_mutex_attr - Initialize the mutex attribute for recursive locking.
 *
 * This helper function checks if the mutex attribute is already initialized;
 * if not, it initializes it and sets the type to PTHREAD_MUTEX_RECURSIVE.
 *
 * Returns:
 *   STR_OK on success or an error code (STR_LOCK) if initialization fails.
 */
static inline Str_err_t init_mutex_attr(void) {
	if (!mutex_attr_initialized) {
		if (pthread_mutexattr_init(&mutex_attr) != 0) {
			return STR_LOCK;
		}
		
		if (pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE) != 0) {
			pthread_mutexattr_destroy(&mutex_attr);
			return STR_LOCK;
		}
		
		mutex_attr_initialized = true;
	}
	return STR_OK;
}

/*
 * cleanup_mutex_attr - Clean up the mutex attribute.
 *
 * This helper function destroys the mutex attribute if it has been initialized.
 */
static void cleanup_mutex_attr(void) {
	if (mutex_attr_initialized) {
		pthread_mutexattr_destroy(&mutex_attr);
		mutex_attr_initialized = false;
	}
}

/*
 * str_init - Allocate and initialize a new dynamic string object.
 *
 * Allocates memory for a new 'str' structure, zeroes its contents, and
 * initializes its recursive mutex. Marks the string as dynamic.
 *
 * Returns:
 *   Pointer to the new string object on success, or NULL on failure.
 */
STR_WARN_UNUSED_RESULT STR_MALLOC str* str_init(void) {
	Str_err_t err = init_mutex_attr();
	if (err != STR_OK)
		return NULL;
	
	str* self = (str*)malloc(sizeof(str));
	if (!self)
		return NULL;
	
	memset(self, 0, sizeof(str));
	
	if (pthread_mutex_init(&self->lock, &mutex_attr) != 0) {
		free(self);
		return NULL;
	}

	self->is_dynamic = true;
	return self;
}

/*
 * str_free - Free a dynamic string object.
 *
 * Releases the internal data buffer, destroys the mutex, and frees the object.
 *
 * Parameters:
 *   self - Pointer to the string object to be freed.
 */
STR_NONNULL_ALL void str_free(str *self) {
    if (!self)
        return;
    
    if (self->data) {
        free(self->data);
        self->data = NULL;
    }
    
    pthread_mutex_destroy(&self->lock);
    free(self);
}

/*
 * str_clear - Clear the content of the string.
 *
 * Resets the string's data buffer to zero and sets its length to 0.
 *
 * Parameters:
 *   self - Pointer to the string object to be cleared.
 */
STR_NONNULL_ALL void str_clear(str *self) {
	if (self) {
		if(self->data) {
			memset(self->data, 0, self->capacity);
			self->length = 0;
		}
	}
}

/*
 * str_grow - Increase the capacity of the string's internal buffer.
 *
 * Grows the internal buffer to at least 'min_capacity' by doubling its size.
 * Checks for maximum allowed size to prevent overflow.
 *
 * Parameters:
 *   self         - Pointer to the string object.
 *   min_capacity - The minimum required capacity.
 *
 * Returns:
 *   STR_OK on success or an appropriate error code on failure.
 */
STR_NONNULL_ALL Str_err_t str_grow(str *self, size_t min_capacity) {
    if (!self)
        return STR_NULL;
    
    if (min_capacity > STR_MAX_STRING_SIZE)
        return STR_OVERFLOW;
    
    size_t new_capacity = (self->capacity == 0) ? MIN_CAPACITY : self->capacity;
    while (new_capacity < min_capacity) {
        new_capacity *= 2;
        /* Check for overflow */
        if (new_capacity > STR_MAX_STRING_SIZE) {
            new_capacity = STR_MAX_STRING_SIZE;
            break;
        }
    }
    
    char *new_data = (char *)realloc(self->data, new_capacity);
    if (!new_data)
        return STR_NOMEM;
    
    self->data = new_data;
    self->capacity = new_capacity;
    return STR_OK;
}

/*
 * str_set - Set the string content to the provided C-string.
 *
 * Locks the string object, ensures sufficient capacity, copies the new data,
 * and updates the length.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *   arr  - The null-terminated string to set.
 *
 * Returns:
 *   STR_OK on success or an error code if an error occurs.
 */
STR_NONNULL_ALL Str_err_t str_set(str *self, const char *arr) {
	if (!self)
		return STR_NULL;
	if (!arr)
		return STR_NULL;
	
	if (pthread_mutex_lock(&self->lock) != 0)
		return STR_LOCK;
	
	size_t arr_size = strlen(arr) + 1;
	
	if (self->capacity < arr_size) {
		Str_err_t err = str_realloc(self, arr_size);
		if (err != STR_OK) {
			pthread_mutex_unlock(&self->lock);
			return err;
		}
	}
	
	char *result = str_copy(self->data, arr, arr_size);
	if (!result) {
		pthread_mutex_unlock(&self->lock);
		return STR_ERRCPY;
	}
	self->length = arr_size - 1;
	
	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}

/*
 * str_add - Append the given C-string to the end of the string.
 *
 * Locks the string object, grows the buffer if necessary, and appends the data.
 *
 * Parameters:
 *   self   - Pointer to the string object.
 *   _data  - The null-terminated string to append.
 *
 * Returns:
 *   STR_OK on success or an appropriate error code on failure.
 */
STR_NONNULL_ALL Str_err_t str_add(str *self, const char *_data) {
	if (!self || !_data)
		return STR_NULL;
	
	if (pthread_mutex_lock(&self->lock) != 0)
		return STR_LOCK;

	Str_err_t err = STR_OK;
	size_t data_len = strlen(_data);
	size_t new_len = self->length + data_len;
	
	if (new_len >= self->capacity) {
		err = str_grow(self, new_len + 1);
		if (err != STR_OK) {
			pthread_mutex_unlock(&self->lock);
			return err;
		}
	}
	
	memcpy(self->data + self->length, _data, data_len + 1);
	self->length = new_len;
	
	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}

/*
 * str_get - Retrieve the internal C-string.
 *
 * Returns the pointer to the internal data buffer.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   The internal C-string, or NULL if not available.
 */
STR_NONNULL_ALL STR_PURE const char* str_get(const str *self) {
	if (!self || !self->data)
		return NULL;

	return self->data;
}

/*
 * str_get_data - Alias for str_get.
 *
 * Returns the internal data buffer of the string.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   The internal C-string, or NULL if not available.
 */
STR_NONNULL_ALL STR_PURE const char* str_get_data(const str *self) {
	if (!self || !self->data)
		return NULL;

	return self->data;
}

/*
 * str_get_size - Get the current length of the string.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   The length of the string, or 0 if self is NULL.
 */
STR_NONNULL_ALL STR_PURE size_t str_get_size(const str *self) {
	if (!self)
		return 0;

	return self->length;
}

/*
 * str_is_empty - Check if the string is empty.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   true if the string is empty, false otherwise.
 */
STR_NONNULL_ALL STR_PURE bool str_is_empty(const str *self) {
	if (!self)
		return true;

	return (self->length == 0);
}

/*
 * str_to_upper - Convert the string to uppercase.
 *
 * Iterates over the string and converts each alphabetical character to
 * its uppercase equivalent. The string is locked during the operation.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   STR_OK on success or an error code if locking fails.
 */
STR_NONNULL_ALL Str_err_t str_to_upper(str *self) {
	if (!self || !self->data)
		return STR_NULL;
	
	if (pthread_mutex_lock(&self->lock) != 0)
		return STR_LOCK;
	
	for (size_t i = 0; i < self->length; i++)
		self->data[i] = toupper(self->data[i]);
	
	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}

/*
 * str_to_lower - Convert the string to lowercase.
 *
 * Iterates over the string and converts each alphabetical character to
 * its lowercase equivalent. The string is locked during the operation.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   STR_OK on success or an error code if locking fails.
 */
STR_NONNULL_ALL Str_err_t str_to_lower(str *self) {
	if (!self || !self->data)
		return STR_NULL;
	
	if (pthread_mutex_lock(&self->lock) != 0)
		return STR_LOCK;
	
	for (size_t i = 0; i < self->length; i++)
		self->data[i] = tolower(self->data[i]);
	
	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}

/*
 * str_to_title_case - Convert the string to title case.
 *
 * Changes the string so that the first alphabetical character of each word
 * is uppercase and all other alphabetical characters are lowercase.
 * Word boundaries are determined by whitespace, punctuation, or non-alpha characters.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   STR_OK on success or an error code if locking fails.
 */
STR_NONNULL_ALL Str_err_t str_to_title_case(str *self) {
	if (!self || !self->data)
		return STR_NULL;
	
	if (pthread_mutex_lock(&self->lock) != 0)
		return STR_LOCK;
	
	bool new_word = true;
	for (size_t i = 0; i < self->length; i++) {
		if (isspace(self->data[i]) || ispunct(self->data[i]) || !isalpha(self->data[i])) {
			new_word = true;
		} else if (new_word && isalpha(self->data[i])) {
			self->data[i] = toupper(self->data[i]);
			new_word = false;
		} else {
			self->data[i] = tolower(self->data[i]);
		}
	}
	
	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}

/*
 * str_reverse - Reverse the string in place.
 *
 * Swaps characters from the beginning and end moving inward until the entire
 * string is reversed.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   STR_OK on success or an error code if locking fails.
 */
STR_NONNULL_ALL Str_err_t str_reverse(str *self) {
	if (!self || !self->data)
		return STR_NULL;
	
	if (pthread_mutex_lock(&self->lock) != 0)
		return STR_LOCK;
	
	for (size_t i = 0; i < self->length / 2; i++) {
		char temp = self->data[i];
		self->data[i] = self->data[self->length - 1 - i];
		self->data[self->length - 1 - i] = temp;
	}
	
	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}

/*
 * str_rem_word - Remove the first occurrence of a substring.
 *
 * Searches for the given substring ('needle') in the string and removes it
 * by shifting the remaining data. If the substring is not found, returns STR_FAIL.
 *
 * Parameters:
 *   self   - Pointer to the string object.
 *   needle - The substring to remove.
 *
 * Returns:
 *   STR_OK on success, STR_FAIL if the substring is not found, or an error code.
 */
STR_NONNULL_ALL Str_err_t str_rem_word(str *self, const char *needle) {
	if (!self || !self->data || !needle)
		return STR_NULL;
	
	if (pthread_mutex_lock(&self->lock) != 0)
		return STR_LOCK;
	
	char *found = strstr(self->data, needle);
	if (!found) {
		pthread_mutex_unlock(&self->lock);
		return STR_FAIL;
	}
	
	size_t needle_len = strlen(needle);
	memmove(found, found + needle_len, strlen(found + needle_len) + 1);
	self->length -= needle_len;
	
	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}

/*
 * str_swap_word - Replace the first occurrence of a substring with another.
 *
 * Searches for the substring 'old_word' in the string and replaces it with
 * 'new_word'. If the new word is longer than the old one, the buffer is grown.
 *
 * Parameters:
 *   self     - Pointer to the string object.
 *   old_word - The substring to be replaced.
 *   new_word - The replacement substring.
 *
 * Returns:
 *   STR_OK on success, STR_FAIL if 'old_word' is not found, or an error code.
 */
STR_NONNULL_ALL Str_err_t str_swap_word(str *self, const char *old_word, const char *new_word) {
	if (!self || !self->data || !old_word || !new_word)
		return STR_NULL;
	
	if (pthread_mutex_lock(&self->lock) != 0)
		return STR_LOCK;
	
	char *found = strstr(self->data, old_word);
	if (!found) {
		pthread_mutex_unlock(&self->lock);
		return STR_FAIL;
	}
	
	Str_err_t  err = STR_OK;
	size_t old_len = strlen(old_word);
	size_t new_len = strlen(new_word);
	size_t diff = new_len > old_len ? new_len - old_len : old_len - new_len;
	
	if (new_len > old_len) {
		err = str_grow(self, self->length + diff + 1);
		if (err != STR_OK) {
			pthread_mutex_unlock(&self->lock);
			return err;
		}
	
		memmove(found + new_len, found + old_len, strlen(found + old_len) + 1);
	} else if (new_len < old_len) {
		memmove(found + new_len, found + old_len, strlen(found + old_len) + 1);
	}
	
	memcpy(found, new_word, new_len);
	self->length = self->length + new_len - old_len;
	
	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}

/*
 * str_input - Read a line from stdin and set it as the string's content.
 *
 * Reads input using fgets into a fixed-size buffer, removes the trailing newline
 * if present, and calls str_set() to update the string.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   STR_OK on success or an error code if reading or setting the input fails.
 */
STR_NONNULL_ALL Str_err_t str_input(str *self) {
	if (!self)
		return STR_NULL;
	
	char buffer[CHUNK_SIZE];
	if (!fgets(buffer, sizeof(buffer), stdin))
		return STR_FAIL;
	
	size_t len = strlen(buffer);
	if (len > 0 && buffer[len-1] == '\n')
		buffer[--len] = '\0';
	
	return str_set(self, buffer);
}

/*
 * str_add_input - Read a line from stdin and append it to the string.
 *
 * Reads input using fgets into a fixed-size buffer, removes the trailing newline,
 * and appends the result to the existing string content.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   STR_OK on success or an error code if reading or appending fails.
 */
STR_NONNULL_ALL Str_err_t str_add_input(str *self) {
	if (!self)
		return STR_NULL;
	
	char buffer[CHUNK_SIZE];
	if (!fgets(buffer, sizeof(buffer), stdin))
		return STR_FAIL;
	
	size_t len = strlen(buffer);
	if (len > 0 && buffer[len-1] == '\n')
		buffer[--len] = '\0';
	
	return str_add(self, buffer);
}

/*
 * str_print - Print the string to stdout.
 *
 * Parameters:
 *   self - Pointer to the string object.
 */
STR_NONNULL_ALL void str_print(const str *self) {
	if (!self || !self->data)
		return;

	printf("%s", self->data);
}

/*
 * str_check_err - Print an error message corresponding to the error code.
 *
 * Maps error codes to descriptive messages and prints the error along with
 * an optional user message.
 *
 * Parameters:
 *   Error        - The error code.
 *   user_message - Additional context or message provided by the user.
 *
 * Returns:
 *   Always returns 0.
 */
STR_NONNULL_ALL int str_check_err(Str_err_t Error, const char *user_message) {
	static const char *err_msgs[] = {
	[STR_OK]	  = "OK",
	[STR_NULL]	= "NULL pointer",
	[STR_INVALID] = "Invalid argument",
	[STR_NOMEM]   = "No memory",
	[STR_ERRCPY]  = "Copy error",
	[STR_MAXSIZE] = "Max size exceeded",
	[STR_ALLOC]   = "Allocation failed",
	[STR_EMPTY]   = "Empty string",
	[STR_FAIL]	= "Operation failed",
	[STR_OVERFLOW]= "Buffer overflow",
	[STR_LOCK]	= "Mutex lock error"
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

/*
 * str_substr - Extract a substring from the string.
 *
 * Extracts 'len' characters from the string starting at position 'pos' and
 * stores the result in the provided 'result' string object. The original string
 * is locked during extraction.
 *
 * Parameters:
 *   self   - Pointer to the source string object.
 *   result - Pointer to the string object where the substring will be stored.
 *   pos    - Starting position for the substring.
 *   len    - Number of characters to extract.
 *
 * Returns:
 *   STR_OK on success or an appropriate error code.
 */
STR_NONNULL_ALL Str_err_t str_substr(str *self, str *result, size_t pos, size_t len) {
    if (!self || !result || !self->data)
        return STR_NULL;

    if (pos >= self->length)
        return STR_INVALID;

    Str_err_t err = str_lock(self);
    if (err != STR_OK) {
        return err;
    }

    if (len > self->length - pos)
        len = self->length - pos;

    /* Clear the result string object */
    str_clear(result);

    char *substr = (char*)malloc(len + 1);
    if (!substr) {
        str_unlock(self);
        return STR_NOMEM;
    }

    memcpy(substr, self->data + pos, len);
    substr[len] = '\0';

    /* Assign the new value to the result object */
    char *old_data = result->data;
    result->data = substr;
    result->length = len;
    result->capacity = len + 1;

    /* Free the old content */
    if (old_data) {
        free(old_data);
    }

    str_unlock(self);
    return STR_OK;
}

/*
 * str_insert - Insert a string into the current string at a given position.
 *
 * Inserts the provided null-terminated 'string' into the 'self' object at
 * position 'pos'. Adjusts the internal buffer if necessary.
 *
 * Parameters:
 *   self  - Pointer to the string object.
 *   pos   - Position at which to insert the new string.
 *   string- The null-terminated string to insert.
 *
 * Returns:
 *   STR_OK on success or an error code on failure.
 */
STR_NONNULL_ALL Str_err_t str_insert(str *self, size_t pos, const char *string) {
	if (!self || !string)
		return STR_NULL;
	
	if (pthread_mutex_lock(&self->lock) != 0)
		return STR_LOCK;
	
	if (pos > self->length) {
		pthread_mutex_unlock(&self->lock);
		return STR_INVALID;
	}
	
	size_t string_len = strlen(string);
	size_t new_len = self->length + string_len;
	Str_err_t err = STR_OK;
	
	if (new_len >= self->capacity) {
		err = str_grow(self, new_len + 1);
		if (err != STR_OK) {
			pthread_mutex_unlock(&self->lock);
			return err;
		}
	}
	
	memmove(self->data + pos + string_len, self->data + pos, self->length - pos + 1);
	memcpy(self->data + pos, string, string_len);
	self->length = new_len;
	
	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}

/*
 * str_find - Find the first occurrence of a substring.
 *
 * Searches for the first occurrence of 'substr' in the string starting from
 * position 'pos'. Returns the index of the occurrence or (size_t)-1 if not found.
 *
 * Parameters:
 *   self   - Pointer to the string object.
 *   substr - The substring to search for.
 *   pos    - The position from which to start the search.
 *
 * Returns:
 *   The index of the found substring or (size_t)-1 if not found.
 */
STR_NONNULL_ALL size_t str_find(const str *self, const char *substr, size_t pos) {
	if (!self || !substr || !self->data)
		return (size_t)-1;
	
	if (pos >= self->length)
		return (size_t)-1;
	
	size_t substr_len = strlen(substr);
	if (substr_len == 0)
		return pos;
	
	if (substr_len > self->length - pos)
		return (size_t)-1;
	
	const char *found = strstr(self->data + pos, substr);
	if (!found)
		return (size_t)-1;
	
	return (size_t)(found - self->data);
}

/*
 * str_starts_with - Check if the string starts with the given prefix.
 *
 * Parameters:
 *   self   - Pointer to the string object.
 *   prefix - The prefix to check.
 *
 * Returns:
 *   true if the string starts with 'prefix', false otherwise.
 */
STR_NONNULL_ALL bool str_starts_with(const str *self, const char *prefix) {
	if (!self || !prefix || !self->data)
		return false;
	
	size_t prefix_len = strlen(prefix);
	if (prefix_len > self->length)
		return false;
	
	return strncmp(self->data, prefix, prefix_len) == 0;
}

/*
 * str_ends_with - Check if the string ends with the given suffix.
 *
 * Parameters:
 *   self   - Pointer to the string object.
 *   suffix - The suffix to check.
 *
 * Returns:
 *   true if the string ends with 'suffix', false otherwise.
 */
STR_NONNULL_ALL bool str_ends_with(const str *self, const char *suffix) {
	if (!self || !suffix || !self->data)
		return false;
	
	size_t suffix_len = strlen(suffix);
	if (suffix_len > self->length)
		return false;
	
	return strcmp(self->data + self->length - suffix_len, suffix) == 0;
}

/*
 * str_pad_left - Left-pad the string with a specified character.
 *
 * Extends the string on the left with 'pad_char' until the total length
 * becomes 'total_length'. Allocates a new buffer and replaces the current one.
 *
 * Parameters:
 *   self         - Pointer to the string object.
 *   total_length - The desired total length after padding.
 *   pad_char     - The character to use for padding.
 *
 * Returns:
 *   STR_OK on success or an error code on failure.
 */
STR_NONNULL_ALL Str_err_t str_pad_left(str *self, size_t total_length, char pad_char) {
    if (!self || !self->data)
        return STR_NULL;
    
    if (pthread_mutex_lock(&self->lock) != 0)
        return STR_LOCK;
    
    if (total_length <= self->length) {
        pthread_mutex_unlock(&self->lock);
        return STR_OK;
    }
    
    size_t pad_length = total_length - self->length;
    char *new_data = (char*)malloc(total_length + 1); /* Allocate new buffer */
    if (!new_data) {
        pthread_mutex_unlock(&self->lock);
        return STR_NOMEM;
    }
    
    memset(new_data, pad_char, pad_length);
    memcpy(new_data + pad_length, self->data, self->length + 1);
    
    free(self->data);
    self->data = new_data;
    self->length = total_length;
    self->capacity = total_length + 1;
    
    pthread_mutex_unlock(&self->lock);
    return STR_OK;
}

/*
 * str_pad_right - Right-pad the string with a specified character.
 *
 * Extends the string on the right with 'pad_char' until the total length
 * becomes 'total_length'. Allocates a new buffer and replaces the current one.
 *
 * Parameters:
 *   self         - Pointer to the string object.
 *   total_length - The desired total length after padding.
 *   pad_char     - The character to use for padding.
 *
 * Returns:
 *   STR_OK on success or an error code on failure.
 */
STR_NONNULL_ALL Str_err_t str_pad_right(str *self, size_t total_length, char pad_char) {
    if (!self || !self->data)
        return STR_NULL;
    
    if (pthread_mutex_lock(&self->lock) != 0)
        return STR_LOCK;
    
    if (total_length <= self->length) {
        pthread_mutex_unlock(&self->lock);
        return STR_OK;
    }
    
    size_t pad_length = total_length - self->length;
    char *new_data = (char*)malloc(total_length + 1); /* Allocate new buffer */
    if (!new_data) {
        pthread_mutex_unlock(&self->lock);
        return STR_NOMEM;
    }
    
    memcpy(new_data, self->data, self->length);
    memset(new_data + self->length, pad_char, pad_length);
    new_data[total_length] = '\0';
    
    free(self->data);
    self->data = new_data;
    self->length = total_length;
    self->capacity = total_length + 1;
    
    pthread_mutex_unlock(&self->lock);
    return STR_OK;
}

/*
 * str_trim - Trim leading and trailing whitespace from the string.
 *
 * Removes any whitespace characters at the beginning and end of the string.
 * The string object is locked during the operation.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   STR_OK on success or an error code if locking fails.
 */
STR_NONNULL_ALL Str_err_t str_trim(str *self) {
	if (!self || !self->data)
		return STR_NULL;
	
	if (pthread_mutex_lock(&self->lock) != 0)
		return STR_LOCK;
	
	/* Trim left */
	size_t start = 0;
	while (start < self->length && isspace(self->data[start]))
		start++;
	
	if (start == self->length) {
		/* String contains only whitespace */
		self->data[0] = '\0';
		self->length = 0;
		pthread_mutex_unlock(&self->lock);
		return STR_OK;
	}
	
	/* Trim right */
	size_t end = self->length - 1;
	while (end > start && isspace(self->data[end]))
		end--;
	
	/* Move the trimmed string to the beginning if necessary */
	if (start > 0) {
		size_t new_len = end - start + 1;
		memmove(self->data, self->data + start, new_len);
		self->data[new_len] = '\0';
		self->length = new_len;
	} else {
		self->data[end + 1] = '\0';
		self->length = end + 1;
	}
	
	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}

/*
 * str_trim_left - Trim leading whitespace from the string.
 *
 * Removes whitespace characters from the beginning of the string.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   STR_OK on success or an error code if locking fails.
 */
STR_NONNULL_ALL Str_err_t str_trim_left(str *self) {
	if (!self || !self->data)
		return STR_NULL;
	
	if (pthread_mutex_lock(&self->lock) != 0)
		return STR_LOCK;
	
	size_t i = 0;
	while (i < self->length && isspace(self->data[i]))
		i++;
	
	if (i > 0) {
		memmove(self->data, self->data + i, self->length - i + 1);
		self->length -= i;
	}
	
	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}

/*
 * str_trim_right - Trim trailing whitespace from the string.
 *
 * Removes whitespace characters from the end of the string.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   STR_OK on success or an error code if locking fails.
 */
STR_NONNULL_ALL Str_err_t str_trim_right(str *self) {
	if (!self || !self->data)
		return STR_NULL;

	if (pthread_mutex_lock(&self->lock)!= 0)
		return STR_LOCK;
	
	while (self->length > 0 && isspace(self->data[self->length - 1]))
		self->data[--self->length] = '\0';
	
	pthread_mutex_unlock(&self->lock);
	return STR_OK;
}

/*
 * str_copy - Copy a C-string into a destination buffer.
 *
 * Copies characters from 'source' to 'dest' until either 'max_len - 1' characters
 * are copied or a null-terminator is encountered. Ensures that 'dest' is null-terminated.
 *
 * Parameters:
 *   dest    - Destination buffer.
 *   source  - Source C-string.
 *   max_len - Maximum number of characters to copy (including the null-terminator).
 *
 * Returns:
 *   The destination buffer, or NULL if input is invalid.
 */
STR_NONNULL_ALL char* str_copy(char *dest, const char *source, size_t max_len) {
	if (!dest || !source || max_len == 0)
		return NULL;
	
	size_t i;
	for (i = 0; i < max_len - 1 && source[i] != '\0'; i++)
		dest[i] = source[i];

	dest[i] = '\0';
	
	return dest;
}

/*
 * str_alloc - Allocate a new string object with an initial buffer size.
 *
 * Allocates memory for both the 'str' structure and its internal data buffer.
 * Initializes the recursive mutex.
 *
 * Parameters:
 *   size - The initial size for the data buffer.
 *
 * Returns:
 *   Pointer to the newly allocated string object or NULL on failure.
 */
STR_WARN_UNUSED_RESULT STR_MALLOC str* str_alloc(size_t size) {
    if (size == 0 || size > STR_MAX_STRING_SIZE)
        return NULL;
    
    str* tmp = (str*)malloc(sizeof(str));
    if (!tmp)
        return NULL;
    
    memset(tmp, 0, sizeof(str));
    
    tmp->data = (char*)calloc(size, sizeof(char));
    if (!tmp->data) {
        free(tmp);
        return NULL;
    }
    
    /* Initialize mutex attribute */
    Str_err_t emutex = init_mutex_attr();
    if (emutex != STR_OK) {
        free(tmp->data);
        free(tmp);
        return NULL;
    }
    
    if (pthread_mutex_init(&tmp->lock, &mutex_attr) != 0) {
        free(tmp->data);
        free(tmp);
        return NULL;
    }
    
    tmp->capacity = size;
    tmp->is_dynamic = true;
    return tmp;
}

/*
 * str_realloc - Reallocate the string's internal buffer.
 *
 * Changes the capacity of the internal buffer to 'new_size'. If the new size
 * is smaller than the current length, the length is adjusted accordingly.
 *
 * Parameters:
 *   self     - Pointer to the string object.
 *   new_size - The desired new size for the data buffer.
 *
 * Returns:
 *   STR_OK on success or an error code if reallocation fails.
 */
STR_WARN_UNUSED_RESULT Str_err_t str_realloc(str *self, size_t new_size) {
    if (!self)
        return STR_INVALID;
    
    if (new_size == 0) {
        free(self->data);
        self->data = NULL;
        self->capacity = 0;
        self->length = 0;
        return STR_OK;
    }
    
    if (new_size > STR_MAX_STRING_SIZE)
        return STR_OVERFLOW;
    
    char *new_ptr = (char *)realloc(self->data, new_size);
    if (!new_ptr)
        return STR_NOMEM;
    
    self->data = new_ptr;
    self->capacity = new_size;
    
    /* Adjust length if new capacity is less than current length */
    if (self->length >= new_size) {
        self->length = new_size - 1;
        self->data[self->length] = '\0';
    }
    
    return STR_OK;
}

/*
 * get_dyn_input - Dynamically read input from stdin.
 *
 * Reads data from stdin in chunks (of size CHUNK_SIZE) until a newline is
 * encountered. The buffer is reallocated as necessary up to 'max_str_size'.
 *
 * Parameters:
 *   max_str_size - The maximum allowable size for the input string.
 *
 * Returns:
 *   Pointer to the dynamically allocated input string, or NULL on error.
 */
STR_NONNULL_ALL char* get_dyn_input(size_t max_str_size) {
    if (max_str_size == 0 || max_str_size > STR_MAX_STRING_SIZE)
        return NULL;
    
    char *buffer = (char*)malloc(CHUNK_SIZE);
    if (!buffer)
        return NULL;
    
    size_t total_read = 0;
    while (fgets(buffer + total_read, CHUNK_SIZE, stdin)) {
        total_read += strlen(buffer + total_read);
        if (buffer[total_read - 1] == '\n') {
            buffer[total_read - 1] = '\0';
            break;
        }
        
        char *new_buffer = (char *)realloc(buffer, total_read + CHUNK_SIZE);
        if (!new_buffer) {
            free(buffer);
            return NULL;
        }
        buffer = new_buffer;
    }
    
    return buffer;
}
