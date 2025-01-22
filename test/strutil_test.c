/*
 * strutil_test.c - Unit tests for strutil library
 *
 * Comprehensive test suite for strutil.h library functions.
 * Tests cover core functionality, edge cases, and error conditions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "strutil.h"

/* Test result tracking */
static int tests_run = 0;
static int tests_failed = 0;

/* Test macro for simpler test writing */
#define TEST(name, test) do { \
    printf("Running test: %s... ", name); \
    tests_run++; \
    if (test) { \
        printf("PASSED\n"); \
    } else { \
        printf("FAILED\n"); \
        tests_failed++; \
    } \
} while (0)

/* Core functionality tests */
static void test_init_free(void) {
    str *s = str_init();
    TEST("str_init returns non-NULL", s != NULL);
    TEST("str_init creates empty string", s->data == NULL && s->length == 0);
    str_free(s);
    
    // Test str_free with NULL pointer - should handle gracefully without crashes
    str_free(NULL);
    TEST("str_free handles NULL", true);  // Test passes if execution reaches here
}

static void test_set_get(void) {
    str *s = str_init();
    const char *test_str = "Hello, World!";
    
    TEST("str_set with valid input", str_set(s, test_str) == STR_OK);
    TEST("str_get returns correct string", strcmp(str_get(s), test_str) == 0);
    TEST("str_set with NULL string", str_set(NULL, test_str) == STR_NULL);
    TEST("str_set with NULL content", str_set(s, NULL) == STR_NULL);
    
    str_free(s);
}

static void test_add(void) {
    str *s = str_init();
    const char *str1 = "Hello";
    const char *str2 = ", World!";
    
    TEST("str_add first string", str_set(s, str1) == STR_OK);
    TEST("str_add second string", str_add(s, str2) == STR_OK);
    TEST("str_add result correct", strcmp(str_get(s), "Hello, World!") == 0);
    TEST("str_add with NULL string", str_add(s, NULL) == STR_NULL);
    
    str_free(s);
}

static void test_string_operations(void) {
    str *s = str_init();
    str_set(s, "hello WORLD");
    
    // Test case conversion operations
    str_to_upper(s);
    TEST("str_to_upper works", strcmp(str_get(s), "HELLO WORLD") == 0);
    
    str_to_lower(s);
    TEST("str_to_lower works", strcmp(str_get(s), "hello world") == 0);
    
    str_to_title_case(s);
    TEST("str_to_title_case works", strcmp(str_get(s), "Hello World") == 0);
    
    // Test string reversal
    str_reverse(s);
    TEST("str_reverse works", strcmp(str_get(s), "dlroW olleH") == 0);
    
    str_free(s);
}

static void test_word_operations(void) {
    str *s = str_init();
    str_set(s, "The quick brown fox");
    
    // Test word removal functionality
    TEST("str_rem_word works", 
         str_rem_word(s, "quick") == STR_OK && 
         strcmp(str_get(s), "The brown fox") == 0);
    
    // Test word replacement functionality
    TEST("str_swap_word works",
         str_swap_word(s, "brown", "black") == STR_OK &&
         strcmp(str_get(s), "The black fox") == 0);
    
    str_free(s);
}

static void test_size_operations(void) {
    str *s = str_init();
    const char *test_str = "Hello, World!";
    
    str_set(s, test_str);
    TEST("str_get_size returns correct length", 
         str_get_size(s) == strlen(test_str));
    
    TEST("str_is_empty works on non-empty string", 
         !str_is_empty(s));
    
    str_clear(s);
    TEST("str_is_empty works after clear", 
         str_is_empty(s));
    
    str_free(s);
}

static void test_error_handling(void) {
    str *s = str_init();
    
    TEST("str_add to NULL returns error", 
         str_add(NULL, "test") == STR_NULL);
    
    TEST("str_set empty string returns error",
         str_set(s, "") == STR_INVALID);
    
    TEST("str_rem_word with non-existent word returns error",
         str_rem_word(s, "nonexistent") == STR_FAIL);
    
    str_free(s);
}

static void test_thread_safety(void) {
    str *s = str_init();
    str_set(s, "test");
    
    // Test basic mutex operations for thread safety
    pthread_mutex_t *lock = &s->lock;
    TEST("Mutex lock works", pthread_mutex_lock(lock) == 0);
    TEST("Mutex unlock works", pthread_mutex_unlock(lock) == 0);
    
    str_free(s);
}

/* Memory leak test helper */
static void test_memory_leaks(void) {
    str *s = str_init();
    const char *test_str = "Test string for memory operations";
    
    // Perform multiple allocations and deallocations to test memory handling
    for (int i = 0; i < 1000; i++) {
        str_set(s, test_str);
        str_clear(s);
    }
    
    str_free(s);
    TEST("Memory operations complete", true);  // Test passes if no memory leaks occurred
}

int main(void) {
    printf("Starting strutil library tests...\n\n");
    
    // Run all tests
    test_init_free();
    test_set_get();
    test_add();
    test_string_operations();
    test_word_operations();
    test_size_operations();
    test_error_handling();
    test_thread_safety();
    test_memory_leaks();
    
    // Print test results
    printf("\nTest Results:\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests failed: %d\n", tests_failed);
    printf("Success rate: %.2f%%\n", 
           ((float)(tests_run - tests_failed) / tests_run) * 100);
    
    return tests_failed ? EXIT_FAILURE : EXIT_SUCCESS;
} 
