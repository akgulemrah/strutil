/*
 * strutil.c - Implementation of a thread-safe, dynamic string library.
 *
 * This file implements a thread-safe, dynamically resizable string library.
 * All operations on the string object are protected by a recursive mutex to
 * ensure safe access in multithreaded environments.
 *
 */
#define _POSIX_C_SOURCE 200809L // Required for POSIX features like pthread

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#include "strutil.h"

// Helper macros for safely stringifying CHUNK_SIZE_FSCANF
#define XSTR(s) STR(s)
#define STR(s) #s
// Define CHUNK_SIZE_FSCANF to be 1 less than CHUNK_SIZE to ensure space for null terminator with %s
#define CHUNK_SIZE_FSCANF (CHUNK_SIZE -1)

static const unsigned int CHUNK_SIZE = 4096;    // Default buffer size for input operations (e.g., fgets, fscanf)
static const unsigned int MIN_CAPACITY = 16;    // Minimum initial capacity for a new string's data buffer

struct str {
    unsigned int flags;             // Flags to track string state (e.g., dynamic, mutex initialized)
    char *data;                     // Pointer to the dynamically allocated string data
    size_t length;                  // Current number of characters in the string (excluding null terminator)
    size_t capacity;                // Total allocated capacity for the string data (including null terminator)
    pthread_mutex_t lock;           // Recursive mutex for thread-safe access
};

/*
 * str_init - Allocate and initialize a new dynamic string object.
 *
 * Allocates memory for a new 'str' structure, zeroes its contents, and
 * initializes its recursive mutex. Marks the string as dynamic.
 *
 * Returns:
 *   Pointer to the new string object on success, or NULL on failure.
 */
struct str* str_init(void)
{
    pthread_mutexattr_t mutex_attr; // Attributes for mutex initialization

    // Allocate memory for the main 'str' structure
    struct str *self = (struct str *)malloc(sizeof(struct str));
    if (!self) {
        fprintf(stderr, "Error: malloc for str struct failed\n");
        return NULL;
    }

    memset(self, 0, sizeof(struct str)); // Initialize all members to zero

    // Allocate initial data buffer with minimum capacity
    self->data = (char *)malloc(MIN_CAPACITY);
    if (!self->data) {
        free(self); // Free the 'str' struct if data allocation fails
        fprintf(stderr, "Error: malloc for data buffer failed\n");
        return NULL;
    }
    self->data[0] = '\0';          // Ensure null-termination for an empty string
    self->capacity = MIN_CAPACITY; // Set initial capacity
    self->length = 0;              // Initial length is 0

    // Initialize mutex attributes
    if (pthread_mutexattr_init(&mutex_attr) != 0) {
        free(self->data); // Clean up data buffer
        free(self);       // Clean up str struct
        fprintf(stderr, "Error: pthread_mutexattr_init failed\n");
        return NULL;
    }

    // Set mutex type to recursive to allow re-locking by the same thread
    if (pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE) != 0) {
        free(self->data);
        free(self);
        pthread_mutexattr_destroy(&mutex_attr); // CLEANUP: Destroy attributes on failure
        fprintf(stderr, "Error: pthread_mutexattr_settype failed\n");
        return NULL;
    }

    // Initialize the mutex
    if (pthread_mutex_init(&self->lock, &mutex_attr) != 0) {
        pthread_mutexattr_destroy(&mutex_attr); // CLEANUP: Destroy attributes
        free(self->data);
        free(self);
        fprintf(stderr, "Error: pthread_mutex_init failed\n");
        return NULL;
    }

    pthread_mutexattr_destroy(&mutex_attr); // Destroy attributes after mutex is initialized

    // Set flags to indicate dynamic allocation and mutex initialization
    SET_FLAG(self->flags, STR_FLAG_DYNAMIC);
    SET_FLAG(self->flags, STR_FLAG_MUTEX_INIT);
    return self;
}

/*
 * _str_free - Free a dynamic string object.
 *
 * Releases the internal data buffer, destroys the mutex, and frees the object.
 * This function takes a pointer-to-pointer so that the caller's pointer can be
 * set to NULL after freeing, preventing use-after-free errors.
 *
 * Parameters:
 *   self - Pointer to the string object's pointer to be freed. Will be set to NULL.
 */
void _str_free(struct str **self)
{
    // Return immediately if self or *self is NULL
    if (!self || !(*self))
        return;

    // Free the internal data buffer if it exists
    if ((*self)->data) {
        free((*self)->data);
        (*self)->data = NULL; // Nullify the data pointer
    }

    // Destroy the mutex if it was successfully initialized
    if (CHECK_FLAG((*self)->flags, STR_FLAG_MUTEX_INIT)) {
        pthread_mutex_destroy(&(*self)->lock);
        CLEAR_FLAG((*self)->flags, STR_FLAG_MUTEX_INIT); // Clear flag after destroy
    }

    // Free the 'str' structure itself if it was dynamically allocated
    if (CHECK_FLAG((*self)->flags, STR_FLAG_DYNAMIC)) {
        free(*self);
        *self = NULL; // Nullify the caller's pointer to the string object
    }
}

/*
 * str_clear - Clear the content of the string.
 *
 * Resets the string's data buffer (sets all characters to null) and its length to 0.
 * Thread-safe: Acquires and releases the mutex to prevent concurrent access issues.
 *
 * Parameters:
 *   self - Pointer to the string object to be cleared.
 */
void str_clear(struct str *self)
{
    if (!self)
        return;

    // Acquire mutex for thread-safe access
    if (pthread_mutex_lock(&self->lock) != 0) {
        // In case of a mutex error, set an error flag and return
        SET_FLAG(self->flags, STR_FLAG_ERROR);
        return;
    }

    // Clear the data buffer by setting all bytes to 0 and reset length
    if(self->data) {
        memset(self->data, 0, self->capacity);
        self->length = 0;
    }
    CLEAR_FLAG(self->flags, STR_FLAG_MODIFIED); // Clear modified flag
    pthread_mutex_unlock(&self->lock); // Release mutex
}

/*
 * str_grow - Increase the capacity of the string's internal buffer.
 *
 * Grows the internal buffer to at least 'min_capacity' by doubling its size
 * until it reaches or exceeds 'min_capacity'. Checks for maximum allowed size
 * and handles potential integer overflows.
 *
 * Parameters:
 *   self         - Pointer to the string object.
 *   min_capacity - The minimum required capacity.
 *
 * Returns:
 *   STR_OK on success or an appropriate error code on failure.
 * NOTE: It is assumed that the caller holds 'self->lock' before calling this function
 * if 'self' is shared between threads.
 */
Str_err_t str_grow(struct str *self, size_t min_capacity)
{
    if (!self)
        return STR_NULL;
    
    // Check if the requested minimum capacity exceeds the global maximum string size
    if (min_capacity > STR_MAX_STRING_SIZE)
        return STR_OVERFLOW;
    
    // Check if the string has a fixed size and cannot grow beyond its current capacity
    if (CHECK_FLAG(self->flags, STR_FLAG_FIXED_SIZE) && min_capacity > self->capacity)
        return STR_MAXSIZE;

    size_t new_capacity = (self->capacity == 0) ? MIN_CAPACITY : self->capacity;

    // Double the capacity until it meets or exceeds `min_capacity`
    // Includes checks for overflow of `new_capacity` during doubling
    while (new_capacity < min_capacity)
    {
        // Check for potential overflow before doubling to prevent wrap-around
        if (new_capacity > STR_MAX_STRING_SIZE / 2 && min_capacity > new_capacity) {
            new_capacity = STR_MAX_STRING_SIZE; // Cap at maximum allowed size
            break; // Stop doubling, capped size reached
        }
        new_capacity *= 2;
    }
    
    // Cap `new_capacity` at `STR_MAX_STRING_SIZE` if it somehow went over
    if (new_capacity > STR_MAX_STRING_SIZE) {
        new_capacity = STR_MAX_STRING_SIZE;
    }

    // Only reallocate if the calculated `new_capacity` is actually greater than current `self->capacity`
    if (new_capacity <= self->capacity) {
        return STR_OK; // No growth needed
    }

    // Attempt to reallocate the data buffer
    char *new_data = (char *)realloc(self->data, new_capacity);
    if (!new_data) {
        // CRITICAL FIX: If realloc fails, self->data is left untouched. Do NOT modify self->data!
        return STR_NOMEM; // Return memory error, preserving original data and pointer
    }

    self->data = new_data;      // Assign the new, possibly larger, buffer
    self->capacity = new_capacity; // Update the capacity

    // Ensure null-termination if string implicitly shrank (e.g., length exceeded new_capacity)
    // Though with grow, length is typically <= new_capacity.
    if (self->length >= self->capacity) {
        self->length = self->capacity - 1; // Adjust length if buffer truncated (should not happen with grow)
    }
    self->data[self->length] = '\0'; // Ensure null-termination at logical end
    
    return STR_OK;
}

/*
 * str_assign_n - Assign a new C-string to the string, copying up to 'count' characters.
 *
 * Locks the string object, ensures sufficient capacity, copies the new data
 * up to 'count' characters (or until null terminator in source is met if `count` is larger),
 * and updates the length. If `count` is 0, the string is cleared.
 *
 * Parameters:
 *   self       - Pointer to the string object.
 *   source_str - The null-terminated source C-string.
 *   count      - The maximum number of characters to copy from `source_str`.
 *
 * Returns:
 *   STR_OK on success or an error code if an error occurs.
 */
Str_err_t str_assign_n(struct str *self, const char *source_str, size_t count)
{
    if (!self || !source_str)
        return STR_NULL;
    
    // If 'count' is 0, clear the string to make it empty
    if (count == 0) {
        str_clear(self); // `str_clear` handles its own mutex locking
        return STR_OK;
    }

    // Validate 'count' against maximum allowed string size
    if (count > STR_MAX_STRING_SIZE)
        return STR_INVALID; 

    // Check if the string is read-only (cannot be modified)
    if (CHECK_FLAG(self->flags, STR_FLAG_READONLY))
        return STR_INVALID; 

    // Acquire mutex for thread-safe operation
    if (pthread_mutex_lock(&self->lock) != 0)
        return STR_LOCK;

    Str_err_t err = STR_OK;
    size_t src_len = strlen(source_str);
    // Determine actual number of characters to copy (minimum of source length and 'count')
    size_t copy_len = (src_len < count) ? src_len : count;

    // Check if new string length would exceed the capacity of a fixed-size string
    if (CHECK_FLAG(self->flags, STR_FLAG_FIXED_SIZE) && (copy_len + 1) > self->capacity) { // +1 for null terminator
        pthread_mutex_unlock(&self->lock);
        return STR_MAXSIZE; 
    }

    // Ensure that string's capacity is sufficient for the `copy_len` plus a null terminator
    if (self->capacity <= copy_len) {
        err = str_grow(self, copy_len + 1); // Request capacity for `copy_len` characters + 1 for null terminator
        if (err != STR_OK) {
            pthread_mutex_unlock(&self->lock);
            return err;
        }
    }

    // Copy the `source_str` content to `self->data` and ensure null-termination
    memcpy(self->data, source_str, copy_len);
    self->data[copy_len] = '\0'; // Explicitly null-terminate the copied string
    self->length = copy_len;     // Update the string's length
    SET_FLAG(self->flags, STR_FLAG_MODIFIED); // Set modified flag

    pthread_mutex_unlock(&self->lock); // Release mutex
    return STR_OK;
}


/*
 * str_mov - Move the content of one string to another.
 *
 * Moves the internal data buffer, length, and capacity from 'src' to 'dest'.
 * The 'src' string is then reset to an empty state and its mutex is destroyed.
 * If 'src' was dynamically allocated (STR_FLAG_DYNAMIC), its struct is also freed.
 * This effectively "consumes" the source string.
 *
 * Parameters:
 *   dest - Pointer to the destination string object.
 *   src  - Pointer to the source string object. This object will be freed by the function.
 *
 * Returns:
 *   STR_OK on success or an error code if an error occurs.
 * WARNING: The 'src' pointer will be invalidated and its mutex destroyed after this operation.
 * It is caller's responsibility to set any external references to `src` to `NULL`.
 */
Str_err_t str_mov(struct str *dest, struct str *src)
{
    if (!dest || !src)
        return STR_NULL;

    // To prevent deadlocks, acquire locks in a consistent order (e.g., by address)
    // Acquire lock for the one with lower address first.
    pthread_mutex_t *lock1 = &dest->lock;
    pthread_mutex_t *lock2 = &src->lock;

    if (lock1 > lock2) { // Determine canonical lock order based on memory addresses
        pthread_mutex_t *temp = lock1;
        lock1 = lock2;
        lock2 = temp;
    }
    
    // Acquire locks in the determined order
    if (pthread_mutex_lock(lock1) != 0) return STR_LOCK;
    if (lock1 != lock2 && pthread_mutex_lock(lock2) != 0) {
        pthread_mutex_unlock(lock1); // Release the first lock if the second acquisition fails
        return STR_LOCK;
    }

    // Free the destination's existing data before moving new data into it
    if (dest->data) {
        free(dest->data);
    }

    // Perform the "move": transfer src's internal data, length, and capacity to dest
    dest->data = src->data;
    dest->length = src->length;
    dest->capacity = src->capacity;
    SET_FLAG(dest->flags, STR_FLAG_MODIFIED);

    // Reset src to an empty, de-initialized state
    src->data = NULL;
    src->length = 0;
    src->capacity = 0;
    CLEAR_FLAG(src->flags, STR_FLAG_MODIFIED);
    
    // Release locks in the reverse order of acquisition
    if (lock1 != lock2) { // Ensure `lock2` is unlocked only if it's different from `lock1`
        pthread_mutex_unlock(lock2); 
    }
    pthread_mutex_unlock(lock1); 

    // Final cleanup of the source string: destroy its mutex and free its struct if dynamic
    if (CHECK_FLAG(src->flags, STR_FLAG_MUTEX_INIT)) {
        pthread_mutex_destroy(&src->lock); 
        CLEAR_FLAG(src->flags, STR_FLAG_MUTEX_INIT);
    }
    
    if (CHECK_FLAG(src->flags, STR_FLAG_DYNAMIC)) {
        free(src); // Free the 'str' struct itself
        // Note: The caller's `src` pointer will not be set to `NULL` by this function,
        // it is the caller's responsibility to handle `src`'s invalidity.
    }
    return STR_OK;
}


/*
 * str_add - Append the given C-string to the end of the string.
 *
 * Locks the string object, grows the buffer if necessary, and appends the data.
 *
 * Parameters:
 *   self   - Pointer to the string object.
 *   _data  - The null-terminated C-string to append.
 *
 * Returns:
 *   STR_OK on success or an appropriate error code on failure.
 */
Str_err_t str_add(struct str *self, const char *_data)
{
    if (!self || !_data)
        return STR_NULL;
    if (pthread_mutex_lock(&self->lock) != 0)
        return STR_LOCK;

    Str_err_t err = STR_OK;
    size_t data_len = strlen(_data);
    size_t new_len = self->length + data_len;

    // Check for potential string length overflow against global maximum (STR_MAX_STRING_SIZE)
    if (new_len > STR_MAX_STRING_SIZE) { // Check includes space for null terminator
        pthread_mutex_unlock(&self->lock);
        return STR_OVERFLOW;
    }
    
    // If the string is fixed-size, check if the new length would exceed its capacity
    if (CHECK_FLAG(self->flags, STR_FLAG_FIXED_SIZE) && (new_len + 1) > self->capacity) { // +1 for null
        pthread_mutex_unlock(&self->lock);
        return STR_MAXSIZE;
    }

    // Grow the string's internal buffer if current capacity is insufficient
    if (new_len >= self->capacity) {
        err = str_grow(self, new_len + 1); // Request capacity for `new_len` characters + 1 for null terminator
        if (err != STR_OK) {
            pthread_mutex_unlock(&self->lock);
            return err;
        }
    }

    // Append the `_data` C-string (including its null terminator) to `self->data`
    memcpy(self->data + self->length, _data, data_len + 1);
    self->length = new_len; // Update the string's logical length
    SET_FLAG(self->flags, STR_FLAG_MODIFIED); // Set modified flag

    pthread_mutex_unlock(&self->lock); // Release mutex
    return STR_OK;
}


/*
 * str_get_data - Returns the internal data buffer of the string.
 *
 * This function returns a const pointer to the internal C-string buffer.
 * Callers should treat this pointer as temporary and read-only. No locking is done
 * inside this getter to avoid performance overhead, meaning there is a potential
 * for inconsistency if the string's buffer is reallocated (pointer changes) by another
 * thread concurrently.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   The internal C-string, or NULL if the object or its data is not available.
 */
const char* str_get_data(const struct str *self)
{
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
 *   This getter does not acquire a lock. For simple `size_t` reads, this is usually acceptable.
 */
size_t str_get_size(const struct str *self) {
    if (!self)
        return 0;
    
    pthread_mutex_t *mutex = (pthread_mutex_t*)&self->lock;
    if (pthread_mutex_lock(mutex) != 0)
        return 0;
    
    size_t len = self->length;
    pthread_mutex_unlock(mutex);
    return len;
}

/*
 * str_get_capacity - Get the current capacity of the string.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   The capacity of the string, or 0 if self is NULL.
 *   This getter does not acquire a lock.
 */
size_t str_get_capacity(const struct str *self)
{
    if (!self)
        return 0;
    
    return self->capacity;
}

/*
 * str_is_empty - Check if the string is empty.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   true if the string is empty (length is 0), false otherwise.
 *   This getter does not acquire a lock.
 */
bool str_is_empty(const struct str *self)
{
    if (!self)
        return true;
    return (self->length == 0);
}

/*
 * str_to_upper - Convert the string to uppercase.
 *
 * Iterates over the string and converts each alphabetical character to
 * its uppercase equivalent. The string is locked during the operation to
 * ensure thread safety.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   STR_OK on success or an error code if mutex locking fails.
 */
Str_err_t str_to_upper(struct str *self)
{
    if (!self || !self->data)
        return STR_NULL;
    if (pthread_mutex_lock(&self->lock) != 0)
        return STR_LOCK;

    for (size_t i = 0; i < self->length; i++)
        self->data[i] = (char)toupper(self->data[i]);
    SET_FLAG(self->flags, STR_FLAG_MODIFIED);
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
 *   STR_OK on success or an error code if mutex locking fails.
 */
Str_err_t str_to_lower(struct str *self)
{
    if (!self || !self->data)
        return STR_NULL;
    if (pthread_mutex_lock(&self->lock) != 0)
        return STR_LOCK;

    for (size_t i = 0; i < self->length; i++)
        self->data[i] = (char)tolower(self->data[i]);
    SET_FLAG(self->flags, STR_FLAG_MODIFIED);
    pthread_mutex_unlock(&self->lock);
    return STR_OK;
}

/*
 * str_to_title_case - Convert the string to title case.
 *
 * Changes the string so that the first alphabetical character of each word
 * is uppercase and all other alphabetical characters are lowercase.
 * Word boundaries are determined by whitespace, punctuation, or non-alphabetical characters.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   STR_OK on success or an error code if mutex locking fails.
 */
Str_err_t str_to_title_case(struct str *self)
{
    if (!self || !self->data)
        return STR_NULL;
    if (pthread_mutex_lock(&self->lock) != 0)
        return STR_LOCK;

    bool new_word = true; // Flag to track if the current character is the start of a new word
    for (size_t i = 0; i < self->length; i++) {
        if (isspace(self->data[i]) || ispunct(self->data[i]) || !isalpha(self->data[i])) {
            new_word = true; // Current character is a word separator, next character is start of new word
        } else if (new_word && isalpha(self->data[i])) {
            self->data[i] = (char)toupper(self->data[i]); // Capitalize first letter of new word
            new_word = false; // Not a new word until next separator
        } else {
            self->data[i] = (char)tolower(self->data[i]); // Lowercase other letters in word
        }
    }
    SET_FLAG(self->flags, STR_FLAG_MODIFIED);
    pthread_mutex_unlock(&self->lock);
    return STR_OK;
}

/*
 * str_reverse - Reverse the string in place.
 *
 * Swaps characters from the beginning and end moving inward until the entire
 * string is reversed. Thread-safe using mutex.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   STR_OK on success or an error code if mutex locking fails.
 */
Str_err_t str_reverse(struct str *self)
{
    if (!self || !self->data)
        return STR_NULL;
    if (pthread_mutex_lock(&self->lock) != 0)
        return STR_LOCK;

    // Swap characters from beginning and end towards the middle
    for (size_t i = 0; i < self->length / 2; i++) {
        char temp = self->data[i];
        self->data[i] = self->data[self->length - 1 - i];
        self->data[self->length - 1 - i] = temp;
    }
    SET_FLAG(self->flags, STR_FLAG_MODIFIED);
    pthread_mutex_unlock(&self->lock);
    return STR_OK;
}

/*
 * str_remove_word - Remove the first occurrence of a substring.
 *
 * Searches for the given substring ('needle') in the string and removes it
 * by shifting the remaining data to the left. If the substring is not found,
 * the operation fails and returns STR_FAIL.
 *
 * Parameters:
 *   self   - Pointer to the string object.
 *   needle - The substring to remove.
 *
 * Returns:
 *   STR_OK on success, STR_FAIL if the substring is not found, or an error code.
 */
Str_err_t str_remove_word(struct str *self, const char *needle)
{
    if (!self || !self->data || !needle)
        return STR_NULL;
    if (pthread_mutex_lock(&self->lock) != 0)
        return STR_LOCK;

    char *found = strstr(self->data, needle); // Find the first occurrence of 'needle'
    if (!found) {
        pthread_mutex_unlock(&self->lock);
        return STR_FAIL; // Substring not found
    }

    size_t needle_len = strlen(needle);
    // Use memmove to shift the portion of the string after the 'needle'
    // leftwards, overwriting the 'needle'. `+1` includes the null terminator.
    memmove(found, found + needle_len, strlen(found + needle_len) + 1);
    self->length -= needle_len; // Update string length
    SET_FLAG(self->flags, STR_FLAG_MODIFIED);
    pthread_mutex_unlock(&self->lock);
    return STR_OK;
}

/*
 * str_replace_word - Replace the first occurrence of a substring with another.
 *
 * Searches for the substring 'old_word' in the string and replaces it with
 * 'new_word'. If 'new_word' is longer than 'old_word', the buffer is grown
 * to accommodate the new size.
 *
 * Parameters:
 *   self     - Pointer to the string object.
 *   old_word - The substring to be replaced.
 *   new_word - The replacement substring.
 *
 * Returns:
 *   STR_OK on success, STR_FAIL if 'old_word' is not found, or an error code.
 */
Str_err_t str_replace_word(struct str *self, const char *old_word, const char *new_word)
{
    if (!self || !self->data || !old_word || !new_word)
        return STR_NULL;
    if (pthread_mutex_lock(&self->lock) != 0)
        return STR_LOCK;

    char *found = strstr(self->data, old_word); // Find the first occurrence of 'old_word'
    if (!found) {
        pthread_mutex_unlock(&self->lock);
        return STR_FAIL; // 'old_word' not found
    }

    Str_err_t err = STR_OK;
    size_t old_len = strlen(old_word);
    size_t new_len = strlen(new_word);
    
    // Calculate the length of the string part that comes *after* 'old_word'
    size_t current_tail_len = strlen(found + old_len); 
    // Calculate the expected new total length of the string
    size_t expected_new_length = (found - self->data) + new_len + current_tail_len;

    // Check for string length overflow against the global maximum size
    if (expected_new_length > STR_MAX_STRING_SIZE) { 
        pthread_mutex_unlock(&self->lock);
        return STR_OVERFLOW;
    }

    // If string has a fixed size, check if the replacement fits within capacity
    if (CHECK_FLAG(self->flags, STR_FLAG_FIXED_SIZE) && (expected_new_length + 1) > self->capacity) { // +1 for null
        pthread_mutex_unlock(&self->lock);
        return STR_MAXSIZE;
    }

    if (new_len > old_len) {
        // If the new word is longer, grow the buffer if necessary
        err = str_grow(self, expected_new_length + 1); // +1 for null terminator
        if (err != STR_OK) {
            pthread_mutex_unlock(&self->lock);
            return err;
        }
        // Shift the characters after 'old_word' to the right to make space for 'new_word'
        memmove(found + new_len, found + old_len, current_tail_len + 1); // +1 includes the null terminator
    } else if (new_len < old_len) {
        // If the new word is shorter, shift characters after 'old_word' to the left
        memmove(found + new_len, found + old_len, current_tail_len + 1); // +1 includes the null terminator
    }
    
    // Copy the 'new_word' into the space previously occupied by 'old_word'
    memcpy(found, new_word, new_len);
    self->length = expected_new_length; // Update string's length
    self->data[self->length] = '\0'; // Ensure the string is null-terminated at its new end
    SET_FLAG(self->flags, STR_FLAG_MODIFIED);

    pthread_mutex_unlock(&self->lock);
    return STR_OK;
}

/*
 * str_read_line - Read a line from a FILE stream and set it as the string's content.
 *
 * Reads input using fgets into a fixed-size temporary buffer (CHUNK_SIZE),
 * removes the trailing newline if present, and then sets the string's content
 * using `str_assign_n`. The function ensures thread safety.
 *
 * Parameters:
 *   self   - Pointer to the string object.
 *   stream - The FILE stream to read from (e.g., stdin).
 *
 * Returns:
 *   STR_OK on success or an error code if reading or setting the input fails.
 */
Str_err_t str_read_line(struct str *self, FILE *stream)
{
    if (!self || !stream)
        return STR_NULL;
    
    // Acquire the mutex for thread safety covering the entire operation.
    // str_assign_n will acquire it recursively if called here.
    if (pthread_mutex_lock(&self->lock) != 0)
        return STR_LOCK;

    char buffer[CHUNK_SIZE]; // Use a fixed-size stack-allocated temporary buffer
    memset(buffer, 0, CHUNK_SIZE); // Ensure buffer is zeroed out

    // Read a line from the stream up to CHUNK_SIZE-1 characters.
    // If EOF is reached before any characters are read, fgets returns NULL.
    if (!fgets(buffer, (int)sizeof(buffer), stream)) {
        pthread_mutex_unlock(&self->lock);
        // Distinguish between EOF and actual read error for more specific error code.
        return feof(stream) ? STR_EMPTY : STR_STREAM_ERR; 
    }

    size_t len = strlen(buffer);
    // If a newline character is found at the end, remove it.
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[--len] = '\0'; 
    }
    
    // Release the current lock because str_assign_n will acquire it.
    // This allows for cleaner modularity if `str_assign_n` is to be fully independent.
    pthread_mutex_unlock(&self->lock); 

    // Use str_assign_n to set the string's content.
    // str_assign_n will handle mutex acquisition, growth, and copying.
    return str_assign_n(self, buffer, len);
}


/*
 * str_read_word - Read a single word from a FILE stream and append it to the string.
 *
 * Reads input using fscanf into a fixed-size temporary buffer (CHUNK_SIZE),
 * preventing buffer overflows using a width specifier. The read word is then
 * appended to the existing string content. A space character is prepended
 * if the string is not empty.
 *
 * Parameters:
 *   self   - Pointer to the string object.
 *   stream - The FILE stream to read from (e.g., stdin).
 *
 * Returns:
 *   STR_OK on success or an error code if reading or appending fails.
 */
Str_err_t str_read_word(struct str *self, FILE *stream)
{
    if (!self || !stream) {
        return STR_NULL;
    }

    // Acquire mutex for thread-safe operation.
    if (pthread_mutex_lock(&self->lock) != 0) {
        return STR_LOCK;
    }

    char buffer[CHUNK_SIZE]; // Temporary buffer for reading the word
    memset(buffer, 0, CHUNK_SIZE); // Zero out the buffer

    Str_err_t final_err = STR_OK;

    // CRITICAL FIX: Use `fscanf` with a width specifier to prevent buffer overflows.
    // The `%s` format specifier automatically null-terminates the string.
    // CHUNK_SIZE_FSCANF is CHUNK_SIZE - 1 to account for the null terminator.
    if (fscanf(stream, "%" XSTR(CHUNK_SIZE_FSCANF) "s", buffer) != 1) { 
        if (feof(stream)) final_err = STR_EMPTY;      // EOF reached, no word read
        else final_err = STR_STREAM_ERR; // Other stream error
        goto cleanup; // Jump to cleanup block to release mutex
    }

    size_t len = strlen(buffer);
    if (len == 0) {
        final_err = STR_EMPTY; // Should ideally not happen with "%s" if a word was found
        goto cleanup;
    }

    // Determine if a space needs to be added before the new word (if string is not empty)
    size_t space_to_add = (self->length > 0) ? 1 : 0;
    // Calculate the required total length after appending the word (and optional space)
    size_t required_len = self->length + space_to_add + len;

    // Check for string length overflow against global maximum
    if (required_len > STR_MAX_STRING_SIZE) { 
        final_err = STR_OVERFLOW;
        goto cleanup;
    }

    // Check if the string has a fixed size and the operation would exceed it
    if (CHECK_FLAG(self->flags, STR_FLAG_FIXED_SIZE) && (required_len + 1) > self->capacity) { // +1 for null
        final_err = STR_MAXSIZE;
        goto cleanup;
    }

    // Grow the string's internal buffer if current capacity is insufficient
    if ((required_len + 1) > self->capacity) { // +1 for null terminator
        final_err = str_grow(self, required_len + 1);
        if (final_err != STR_OK) {
            goto cleanup;
        }
    }

    // If a space is needed, append it
    if (space_to_add) {
        self->data[self->length] = ' ';
        self->length++; // Increment length to account for the added space
    }

    // Copy the word from `buffer` to the end of `self->data`
    memcpy(self->data + self->length, buffer, len + 1); // Copy including null terminator
    self->length = required_len; // Update total length
    SET_FLAG(self->flags, STR_FLAG_MODIFIED); // Set modified flag

cleanup:
    // Release mutex before returning
    if (pthread_mutex_unlock(&self->lock) != 0) {
        // If unlocking fails (unlikely in normal scenarios with recursive mutex),
        // set an error state, as this is a severe internal issue.
        SET_FLAG(self->flags, STR_FLAG_ERROR);
        return STR_LOCK; // Return STR_LOCK if mutex operation failed.
    }
    return final_err;
}


/*
 * str_print - Print the string to stdout.
 *
 * Parameters:
 *   self - Pointer to the string object.
 * This function does not acquire a mutex lock as `printf` itself handles
 * synchronization for stdout, and this is a read-only operation.
 */
void str_print(const struct str *self)
{
    if (!self || !self->data)
        return;

    printf("%s", self->data);
}

/*
 * str_check_err - Print an error message corresponding to the error code.
 *
 * Maps error codes to descriptive messages and prints the error along with
 * an optional user message to `stderr`.
 *
 * Parameters:
 *   err          - The error code from the Str_err_t enum.
 *   optional_message - An additional context or message provided by the user.
 */
void str_check_err(const Str_err_t err, const char *optional_message)
{
    // Array of error messages, indexed by Str_err_t enum values.
    static const char *err_msgs[] = {
        [STR_OK]        = "OK",
        [STR_NULL]      = "NULL pointer encountered",
        [STR_INVALID]   = "Invalid argument provided",
        [STR_NOMEM]     = "No memory (allocation failed)",
        [STR_COPY_FAIL] = "String copy operation failed",
        [STR_MAXSIZE]   = "Max size / fixed size capacity exceeded",
        []     = "General allocation error",
        [STR_EMPTY]     = "Empty string or no data to process",
        [STR_FAIL]      = "Operation failed",
        [STR_OVERFLOW]  = "Buffer overflow or size limit exceeded",
        [STR_LOCK]      = "Mutex lock/unlock operation failed",
        [STR_STREAM_ERR]= "File stream I/O error" // New error message
    };

    // Validate the error code to prevent out-of-bounds access
    if (err < 0 || (size_t)err >= sizeof(err_msgs) / sizeof(err_msgs[0])) {
        fprintf(stderr, "Error: Unknown error code: %d\n", err);
        return;
    }

    // Print the appropriate error message
    if (optional_message)
        fprintf(stderr, "Error: %s - %s\n", err_msgs[err], optional_message);
    else
        fprintf(stderr, "Error: %s\n", err_msgs[err]);
}


/*
 * str_insert - Insert a string into the current string at a given position.
 *
 * Inserts the provided null-terminated `str_to_insert` into the `self` object
 * at position `pos`. The internal buffer is grown if necessary.
 *
 * Parameters:
 *   self          - Pointer to the string object.
 *   pos           - The position (0-based index) at which to insert the new string.
 *   str_to_insert - The null-terminated string to insert.
 *
 * Returns:
 *   STR_OK on success or an error code on failure.
 */
Str_err_t str_insert(struct str *self, const size_t pos, const char *str_to_insert)
{
    if (!self || !str_to_insert)
        return STR_NULL;
    if (pthread_mutex_lock(&self->lock) != 0)
        return STR_LOCK;

    // If pos is greater than length, it's considered an invalid insertion point for standard insert.
    // For appending, use str_add.
    if (pos > self->length) {
        pthread_mutex_unlock(&self->lock);
        return STR_INVALID;
    }

    Str_err_t err = STR_OK;
    size_t string_len = strlen(str_to_insert);
    const size_t new_total_len = self->length + string_len;

    // Check for potential string length overflow against global maximum
    if (new_total_len > STR_MAX_STRING_SIZE) { 
        pthread_mutex_unlock(&self->lock);
        return STR_OVERFLOW;
    }

    // Check if fixed-size string can accommodate the new length
    if (CHECK_FLAG(self->flags, STR_FLAG_FIXED_SIZE) && (new_total_len + 1) > self->capacity) { // +1 for null
        pthread_mutex_unlock(&self->lock);
        return STR_MAXSIZE;
    }
    
    // Grow the string buffer if new total length exceeds current capacity
    if (new_total_len >= self->capacity) { // +1 for null terminator
        err = str_grow(self, new_total_len + 1);
        if (err != STR_OK) {
            pthread_mutex_unlock(&self->lock);
            return err;
        }
    }

    // Shift existing characters from `pos` onwards to the right to make space for the new string.
    // `+1` includes the original null terminator in the move.
    memmove(self->data + pos + string_len, self->data + pos, self->length - pos + 1);
    
    // Copy the `str_to_insert` into the newly created gap.
    memcpy(self->data + pos, str_to_insert, string_len);
    self->length = new_total_len; // Update string's length
    SET_FLAG(self->flags, STR_FLAG_MODIFIED);

    pthread_mutex_unlock(&self->lock);
    return STR_OK;
}

/*
 * str_find - Find the first occurrence of a substring.
 *
 * Searches for the first occurrence of `substr` in the string `self`, starting from
 * position `pos`. Returns the 0-based index of the occurrence or `STR_NPOS`
 * (which is ((size_t)-1)) if not found.
 *
 * Parameters:
 *   self   - Pointer to the string object.
 *   substr - The substring to search for.
 *   pos    - The position (0-based index) from which to start the search.
 *
 * Returns:
 *   The index of the found substring or `STR_NPOS` if not found or on error.
 */
size_t str_find(const struct str *self, const char *substr, size_t pos)
{
    if (!self || !substr || !self->data)
        return STR_NPOS; // Return `STR_NPOS` for NULL pointers

    // Check if starting position is within current string length
    if (pos >= self->length)
        return STR_NPOS;

    size_t substr_len = strlen(substr);
    // If searching for an empty substring, it's always found at `pos`
    if (substr_len == 0)
        return pos;

    // Check if there are enough remaining characters in `self` from `pos` onwards
    // to contain `substr`
    if (substr_len > self->length - pos)
        return STR_NPOS;

    // Use `strstr` to find the substring within the relevant portion of `self->data`
    const char *found = strstr(self->data + pos, substr);
    if (!found)
        return STR_NPOS; // Substring not found

    return (size_t)(found - self->data); // Calculate the 0-based index
}

/*
 * str_starts_with - Check if the string starts with the given prefix.
 *
 * Parameters:
 *   self   - Pointer to the string object.
 *   prefix - The prefix string to check.
 *
 * Returns:
 *   true if the string `self` starts with `prefix`, false otherwise.
 */
bool str_starts_with(const struct str *self, const char *prefix)
{
    if (!self || !prefix || !self->data)
        return false;

    size_t prefix_len = strlen(prefix);
    // Check if the prefix is longer than the string itself
    if (prefix_len > self->length)
        return false;

    // Use `strncmp` to compare the beginning of `self->data` with `prefix`
    return strncmp(self->data, prefix, prefix_len) == 0;
}

/*
 * str_ends_with - Check if the string ends with the given suffix.
 *
 * Parameters:
 *   self   - Pointer to the string object.
 *   suffix - The suffix string to check.
 *
 * Returns:
 *   true if the string `self` ends with `suffix`, false otherwise.
 */
bool str_ends_with(const struct str *self, const char *suffix)
{
    if (!self || !suffix || !self->data)
        return false;

    size_t suffix_len = strlen(suffix);
    // Check if the suffix is longer than the string itself
    if (suffix_len > self->length)
        return false;

    // Use `strcmp` on the substring of `self->data` that should match `suffix`
    return strcmp(self->data + self->length - suffix_len, suffix) == 0;
}

/*
 * str_pad_left - Left-pad the string with a specified character.
 *
 * Extends the string on the left with `pad_char` until the total length
 * becomes `total_length`. A new buffer is allocated to accommodate the padding.
 *
 * Parameters:
 *   self         - Pointer to the string object.
 *   total_length - The desired total length after padding.
 *   pad_char     - The character to use for padding.
 *
 * Returns:
 *   STR_OK on success or an error code on failure.
 */
Str_err_t str_pad_left(struct str *self, size_t total_length, char pad_char)
{
    if (!self || !self->data) // self->data can be null if not yet initialized, but should always have MIN_CAPACITY
        return STR_NULL;
    if (pthread_mutex_lock(&self->lock) != 0)
        return STR_LOCK;

    // If the string is already `total_length` or longer, no padding is needed.
    if (total_length <= self->length) {
        pthread_mutex_unlock(&self->lock);
        return STR_OK; 
    }

    // Check if requested `total_length` exceeds global maximum string size
    if (total_length > STR_MAX_STRING_SIZE) { 
        pthread_mutex_unlock(&self->lock);
        return STR_OVERFLOW;
    }
    // Check if string has a fixed size and cannot grow to `total_length`
    if (CHECK_FLAG(self->flags, STR_FLAG_FIXED_SIZE)) {
        pthread_mutex_unlock(&self->lock);
        return STR_MAXSIZE;
    }

    size_t pad_length = total_length - self->length;
    // Allocate a new buffer for the padded string plus the null terminator
    char *new_data = (char*)malloc(total_length + 1); 
    if (!new_data) {
        pthread_mutex_unlock(&self->lock);
        return STR_NOMEM;
    }

    // Fill the padding area at the beginning of the new buffer
    memset(new_data, pad_char, pad_length);
    // Copy the original string content (including its null terminator) after the padding
    memcpy(new_data + pad_length, self->data, self->length + 1); 
    
    free(self->data); // Free the old data buffer
    self->data = new_data;      // Assign the new padded data buffer
    self->length = total_length; // Update string's length
    self->capacity = total_length + 1; // Update string's capacity
    SET_FLAG(self->flags, STR_FLAG_MODIFIED);

    pthread_mutex_unlock(&self->lock);
    return STR_OK;
}

/*
 * str_pad_right - Right-pad the string with a specified character.
 *
 * Extends the string on the right with `pad_char` until the total length
 * becomes `total_length`. A new buffer is allocated for the padded string.
 *
 * Parameters:
 *   self         - Pointer to the string object.
 *   total_length - The desired total length after padding.
 *   pad_char     - The character to use for padding.
 *
 * Returns:
 *   STR_OK on success or an error code on failure.
 */
Str_err_t str_pad_right(struct str *self, size_t total_length, char pad_char)
{
    if (!self || !self->data)
        return STR_NULL;
    if (pthread_mutex_lock(&self->lock) != 0)
        return STR_LOCK;

    // If the string is already `total_length` or longer, no padding is needed.
    if (total_length <= self->length) {
        pthread_mutex_unlock(&self->lock);
        return STR_OK;
    }

    // Check if requested `total_length` exceeds global maximum string size
    if (total_length > STR_MAX_STRING_SIZE) { 
        pthread_mutex_unlock(&self->lock);
        return STR_OVERFLOW;
    }
    // Check if string has a fixed size and cannot grow to `total_length`
    if (CHECK_FLAG(self->flags, STR_FLAG_FIXED_SIZE)) {
        pthread_mutex_unlock(&self->lock);
        return STR_MAXSIZE;
    }

    size_t pad_length = total_length - self->length;
    // Allocate a new buffer for the padded string plus the null terminator
    char *new_data = (char*)malloc(total_length + 1);
    if (!new_data) {
        pthread_mutex_unlock(&self->lock);
        return STR_NOMEM;
    }

    // Copy the original string content to the beginning of the new buffer
    memcpy(new_data, self->data, self->length);
    // Fill the padding area after the original string content
    memset(new_data + self->length, pad_char, pad_length);
    new_data[total_length] = '\0'; // Null-terminate at the new total length

    free(self->data);           // Free the old data buffer
    self->data = new_data;      // Assign the new padded data buffer
    self->length = total_length; // Update string's length
    self->capacity = total_length + 1; // Update string's capacity
    SET_FLAG(self->flags, STR_FLAG_MODIFIED);

    pthread_mutex_unlock(&self->lock);
    return STR_OK;
}

/*
 * str_trim - Trim leading and trailing whitespace from the string.
 *
 * Removes any whitespace characters from both the beginning and the end of the string.
 * The string object's mutex is locked for thread safety.
 * This function internally calls `str_trim_left` and `str_trim_right`, which also
 * handle their own mutex locking, working correctly with recursive mutexes.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   STR_OK on success or an error code if mutex locking fails or trimming fails.
 */
Str_err_t str_trim(struct str *self)
{
    if (!self || !self->data)
        return STR_NULL;
    if (pthread_mutex_lock(&self->lock) != 0)
        return STR_LOCK;

    // Call str_trim_left to remove leading whitespace
    Str_err_t err = str_trim_left(self); 
    if (err != STR_OK) {
        pthread_mutex_unlock(&self->lock);
        return err;
    }

    // Call str_trim_right to remove trailing whitespace
    err = str_trim_right(self);
    if (err != STR_OK) {
        pthread_mutex_unlock(&self->lock);
        return err;
    }
    SET_FLAG(self->flags, STR_FLAG_MODIFIED);
    pthread_mutex_unlock(&self->lock);
    return STR_OK;
}

/*
 * str_trim_left - Trim leading whitespace from the string.
 *
 * Removes whitespace characters (as defined by `isspace()`) from the beginning of the string.
 * Thread-safe with mutex locking.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   STR_OK on success or an error code if mutex locking fails.
 */
Str_err_t str_trim_left(struct str *self)
{
    if (!self || !self->data)
        return STR_NULL;
    if (pthread_mutex_lock(&self->lock) != 0)
        return STR_LOCK;

    size_t i = 0;
    // Find the first non-whitespace character
    while (i < self->length && isspace(self->data[i]))
        i++;

    if (i > 0) { // If any leading whitespace was found
        // Shift the string content to the left to remove the leading whitespace.
        // `+1` ensures the null terminator is also moved.
        memmove(self->data, self->data + i, self->length - i + 1);
        self->length -= i; // Update the string's length
        SET_FLAG(self->flags, STR_FLAG_MODIFIED);
    }
    
    pthread_mutex_unlock(&self->lock);
    return STR_OK;
}

/*
 * str_trim_right - Trim trailing whitespace from the string.
 *
 * Removes whitespace characters (as defined by `isspace()`) from the end of the string.
 * Thread-safe with mutex locking.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *
 * Returns:
 *   STR_OK on success or an error code if mutex locking fails.
 */
Str_err_t str_trim_right(struct str *self)
{
    if (!self || !self->data)
        return STR_NULL;
    if (pthread_mutex_lock(&self->lock)!= 0)
        return STR_LOCK;

    // Loop from the end of the string, removing trailing whitespace characters
    while (self->length > 0 && isspace(self->data[self->length - 1]))
        self->data[--self->length] = '\0'; // Set new null terminator, effectively shortening string

    // If the string becomes entirely empty after trimming
    if (self->length == 0) { 
        self->data[0] = '\0'; // Explicitly null-terminate the first character
    }
    SET_FLAG(self->flags, STR_FLAG_MODIFIED);
    pthread_mutex_unlock(&self->lock);
    return STR_OK;
}

/*
 * str_copy - Copy content from source string object to destination string object.
 *
 * Copies characters from the `source` `str` object to the `dest` `str` object,
 * up to a maximum of `max_len` characters (or fewer if `source` is shorter).
 * Ensures that `dest` is null-terminated and its buffer is correctly managed.
 *
 * Parameters:
 *   dest    - The destination `str` object.
 *   source  - The source `str` object (contents are read-only).
 *   max_len - Maximum number of characters to copy (excluding the null-terminator).
 *
 * Returns:
 *   STR_OK on success or an error code if input is invalid or allocation fails.
 */
Str_err_t str_copy(struct str *dest, const struct str *source, size_t max_len)
{
    if (!dest || !source || !source->data)
        return STR_NULL;
    if (pthread_mutex_lock(&dest->lock) != 0) // Only lock the destination string, as source is `const`
        return STR_LOCK;

    Str_err_t err = STR_OK;
    size_t source_len = source->length; // Get actual length of the source string
    // Determine the number of characters to copy: minimum of source length and max_len
    size_t copy_len = (source_len < max_len) ? source_len : max_len;

    // Check for string length overflow against global maximum
    if (copy_len > STR_MAX_STRING_SIZE) { 
        pthread_mutex_unlock(&dest->lock);
        return STR_OVERFLOW;
    }

    // Check if the destination string is fixed-size and the copy would exceed its capacity
    if (CHECK_FLAG(dest->flags, STR_FLAG_FIXED_SIZE) && (copy_len + 1) > dest->capacity) { // +1 for null term
        pthread_mutex_unlock(&dest->lock);
        return STR_MAXSIZE; 
    }

    // Ensure `dest->data` buffer exists and has sufficient capacity for `copy_len` + null terminator
    if (!dest->data || dest->capacity <= copy_len) {
        err = str_grow(dest, copy_len + 1); // Request growth for required characters + null
        if (err != STR_OK) {
            pthread_mutex_unlock(&dest->lock);
            return err;
        }
    }

    // Copy data from source to destination buffer
    memcpy(dest->data, source->data, copy_len);
    dest->data[copy_len] = '\0'; // Explicitly null-terminate the copied string
    dest->length = copy_len; // Update destination's length
    SET_FLAG(dest->flags, STR_FLAG_MODIFIED);

    pthread_mutex_unlock(&dest->lock);
    return STR_OK;
}


/*
 *  - Allocate a new string object with an initial buffer size.
 *
 * Allocates memory for both the `str` structure and its internal data buffer
 * with a specified initial capacity `size`. Initializes the recursive mutex.
 *
 * Parameters:
 *   size - The desired initial capacity for the data buffer.
 *
 * Returns:
 *   Pointer to the newly allocated string object or NULL on failure.
 */
struct str *str_alloc(size_t size) {
    if (size == 0 || size > STR_MAX_STRING_SIZE) 
        return NULL;
    
    struct str *tmp = str_init();
    if (!tmp) 
        return NULL;
    
    if (tmp->capacity != size) {
        char *new_data = calloc(size, sizeof(char));
        if (!new_data) {
            pthread_mutex_destroy(&tmp->lock); // Destroy mutex
            free(tmp->data); // Free original buffer
            free(tmp);       // Free str struct
            return NULL;
        }
        free(tmp->data);
        tmp->data = new_data;
        tmp->capacity = size;
    }
    return tmp;
}

/*
 * _str_realloc - Reallocate the string's internal data buffer.
 *
 * Changes the capacity of the internal buffer to `new_size`. If `new_size` is
 * smaller than the current string length, the string will be truncated and null-terminated.
 * Special behavior: If `new_size` is 0, this function will free the entire `str` object
 * and set `*self` to NULL (consistent with `str_free`).
 *
 * Parameters:
 *   self     - Pointer to the string object's pointer. The pointer will be set to NULL if `new_size` is 0.
 *   new_size - The desired new capacity for the data buffer.
 *
 * Returns:
 *   STR_OK on success or an error code if reallocation fails.
 * NOTE: It is assumed that the caller holds `self->lock` before calling this function
 * if `self` is shared and mutable. This function does not acquire its own mutex.
 */
Str_err_t _str_realloc(struct str **self, const size_t new_size)
{
    if (!self || !(*self))
        return STR_NULL; // Invalid input pointer

    if (new_size == 0) { 
        // Special case: `new_size == 0` is interpreted as a request to completely free the object.
        _str_free(self); // Call the dedicated free function
        return STR_OK;
    }

    // Check if `new_size` exceeds global maximum allowed string size
    if (new_size > STR_MAX_STRING_SIZE)
        return STR_OVERFLOW;

    // Check fixed-size constraint: prevent increasing capacity of a fixed-size string.
    // Allow decreasing capacity.
    if (new_size > (*self)->capacity && CHECK_FLAG((*self)->flags, STR_FLAG_FIXED_SIZE)) {
        return STR_MAXSIZE;
    }

    // Attempt to reallocate the internal data buffer.
    char *new_ptr = (char *)realloc((*self)->data, new_size);
    if (!new_ptr) {
        // CRITICAL FIX: If realloc fails, it returns NULL but does NOT free the original buffer.
        // The original `(*self)->data` remains valid and must not be altered.
        return STR_NOMEM; // Return memory error, preserving existing buffer.
    }

    (*self)->data = new_ptr;      // Assign the new buffer
    (*self)->capacity = new_size; // Update the capacity

    /* Adjust string length if the new capacity is less than current length (truncation). */
    if ((*self)->length >= new_size) {
        (*self)->length = new_size - 1; // Truncate length, accounting for null terminator
        (*self)->data[(*self)->length] = '\0'; // Explicitly null-terminate at the new length
        SET_FLAG((*self)->flags, STR_FLAG_MODIFIED);
    } else {
        // If reallocated to a larger size, ensure the string remains null-terminated at its logical end.
        // This defensive measure ensures string validity, especially if data moved in memory.
        (*self)->data[(*self)->length] = '\0';
    }

    return STR_OK;
}


/*
 * get_dyn_input - Dynamically read input from stdin.
 *
 * Reads data from stdin in chunks (of `CHUNK_SIZE` bytes) until a newline character
 * is encountered or EOF is reached. The buffer is dynamically reallocated as necessary
 * to accommodate input, up to `max_str_size`.
 *
 * Parameters:
 *   max_str_size - The maximum allowable size for the input string (excluding null terminator).
 *
 * Returns:
 *   A pointer to the dynamically allocated input string on success.
 *   The caller is responsible for freeing this memory using `free()`.
 *   Returns `NULL` on allocation failure or invalid `max_str_size`.
 */
char* get_dyn_input(size_t max_str_size)
{
    // Validate max_str_size limit
    if (max_str_size == 0 || max_str_size > STR_MAX_STRING_SIZE)
        return NULL;

    // Allocate initial buffer chunk
    char *buffer = (char*)malloc(CHUNK_SIZE);
    if (!buffer)
        return NULL;

    size_t current_capacity = CHUNK_SIZE; // Current allocated buffer capacity
    size_t total_read = 0;              // Total characters successfully read

    // Read input line by line in chunks
    while (fgets(buffer + total_read, (int)(current_capacity - total_read), stdin)) {
        size_t last_chunk_read = strlen(buffer + total_read); // Characters read in current chunk
        total_read += last_chunk_read; // Accumulate total characters

        // Check if `max_str_size` has been reached or exceeded.
        if (total_read > max_str_size) { 
            total_read = max_str_size; // Truncate length to max_str_size
            buffer[max_str_size] = '\0'; // Ensure null termination
            break; // Max size reached, stop reading
        }

        // Check for newline character: if found, replace with null terminator and stop reading.
        if (total_read > 0 && buffer[total_read - 1] == '\n') {
            buffer[total_read - 1] = '\0'; // Replace newline with null terminator
            total_read--; // Decrement length by 1 for removed newline
            break; 
        }

        // Reallocate buffer if nearing capacity, preparing for next chunk read.
        // `+1` is for the null terminator.
        if (current_capacity - total_read <= 1) { 
            size_t new_capacity = current_capacity + CHUNK_SIZE;
            
            // Prevent reallocation beyond `max_str_size` (plus null terminator space)
            if (new_capacity > max_str_size + 1) { 
                new_capacity = max_str_size + 1;
                // If calculated new capacity is not larger than current, break to prevent infinite loop
                if (new_capacity <= current_capacity) {
                    break;
                }
            }

            char *new_buffer = (char *)realloc(buffer, new_capacity);
            if (!new_buffer) {
                free(buffer); // Free original buffer on realloc failure
                return NULL;
            }
            buffer = new_buffer;        // Assign new buffer
            current_capacity = new_capacity; // Update capacity
        }
    }
    
    // Optional: Shrink buffer to fit exact string length to optimize memory usage.
    // +1 for the null terminator.
    if (total_read < current_capacity - 1) { 
        char *final_buffer = (char *)realloc(buffer, total_read + 1);
        if (final_buffer) {
            buffer = final_buffer;
        }
        // If final realloc fails, original (larger) buffer is kept and valid, no leak.
    }
    buffer[total_read] = '\0'; // Final explicit null termination
    return buffer;
}

/*
 * str_set - Set the string content to a C-string.
 *
 * Replaces the current content of the string object `self` with a copy
 * of the provided C-string `data`. If `data` is an empty string, `self` is cleared.
 * This operation is thread-safe using a mutex.
 *
 * Parameters:
 *   self - Pointer to the string object.
 *   data - The null-terminated C-string to set as the new content.
 *
 * Returns:
 *   STR_OK on success or an error code if an error occurs.
 */
Str_err_t str_set(struct str *self, const char *data)
{
    if (!self || !data)
        return STR_NULL;
    
    // If the input data is an empty string, clear the current string's content.
    if (data[0] == '\0') {
        str_clear(self); // `str_clear` handles its own locking and setting length to 0
        return STR_OK;
    }

    // Check if the string is marked as read-only.
    if (CHECK_FLAG(self->flags, STR_FLAG_READONLY))
        return STR_INVALID;

    // Acquire mutex for thread-safe access.
    if (pthread_mutex_lock(&self->lock) != 0)
        return STR_LOCK;

    Str_err_t err = STR_OK;
    size_t new_len = strlen(data);
    
    // Check for string length overflow against global maximum.
    if (new_len > STR_MAX_STRING_SIZE) { // Check includes space for null terminator
        pthread_mutex_unlock(&self->lock);
        return STR_OVERFLOW;
    }
    
    // If string has a fixed size, check if the new length would exceed its capacity.
    if (CHECK_FLAG(self->flags, STR_FLAG_FIXED_SIZE) && (new_len + 1) > self->capacity) { // +1 for null
        pthread_mutex_unlock(&self->lock);
        return STR_MAXSIZE;
    }
    
    // If current capacity is insufficient, grow the buffer.
    if (self->capacity <= new_len) { // Check required capacity (new_len characters + null terminator)
        err = str_grow(self, new_len + 1);
        if (err != STR_OK) {
            pthread_mutex_unlock(&self->lock);
            return err;
        }
    }
    
    // Copy the new data to the string's buffer. `str_grow` handles `realloc`
    // (which preserves contents), so no explicit `free(self->data)` needed before `strcpy`.
    strcpy(self->data, data);
    self->length = new_len; // Update string's length
    SET_FLAG(self->flags, STR_FLAG_MODIFIED); // Set modified flag

    pthread_mutex_unlock(&self->lock); // Release mutex
    return STR_OK;
}

/* UNDEF SPACE */
// Undefine internal macros to prevent pollution of other compilation units
#undef XSTR
#undef STR
#undef CHUNK_SIZE_FSCANF
#undef CHUNK_SIZE
#undef MIN_CAPACITY
